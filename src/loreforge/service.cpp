#include "loreforge/service.h"
#include "loreforge/globalCodebaseManager.h"
#include "loreforge/codeParser.h"

#include "arbiterAI/arbiterAI.h"

#include <sstream>

namespace loreforge
{

Service::Service(const std::string &repositoryPath, const std::string &model)
{
    m_arbiterAI=std::make_shared<arbiterAI::ArbiterAI>();
    m_arbiterAI->initialize({"/app/src/arbiterAI/configs/defaults"});
    m_globalCodebaseManager=std::make_unique<GlobalCodebaseManager>();

    m_codeParser=std::make_shared<CodeParser>();

    if(!repositoryPath.empty())
    {
        auto codebaseManager=std::make_unique<CodebaseManager>();

        codebaseManager->init(m_codeParser, m_arbiterAI, repositoryPath, model);
        m_globalCodebaseManager->addCodebase("default", std::move(codebaseManager));
    }
}

Service::~Service()=default;

GlobalCodebaseManager *Service::getGlobalCodebaseManager()
{
    return m_globalCodebaseManager.get();
}

grpc::Status Service::QueryCodebase(
    grpc::ServerContext *context, const QueryRequest *request,
    grpc::ServerWriter<QueryResponse> *writer)
{
    if(request->query().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "query is required");
    }
    if(request->model().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "model is required");
    }

    std::vector<CodeChunk> chunks;

    if(request->codebase_name().empty())
    {
        chunks=m_globalCodebaseManager->search(request->query(), request->model(), 5);
    }
    else
    {
        auto codebase=m_globalCodebaseManager->getCodebase(request->codebase_name());

        if(!codebase)
        {
            return grpc::Status(grpc::StatusCode::NOT_FOUND,
                "Codebase not found: "+request->codebase_name());
        }
        if(!codebase->isReady())
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                "Codebase not ready: "+request->codebase_name());
        }
        chunks=codebase->search(request->query(), request->model(), 5);
    }

    const std::string &query=request->query();

    std::stringstream prompt;

    prompt<<"You are a helpful coding assistant. Use the following code snippets to answer the user's query.\n\n";
    prompt<<"User Query: "<<query<<"\n\n";
    prompt<<"Relevant Code Snippets:\n\n";

    for(const CodeChunk &chunk:chunks)
    {
        prompt<<"File: "<<chunk.filePath<<"\n";
        prompt<<"```\n"<<chunk.content<<"\n```\n\n";
    }

    prompt<<"Based on the provided code, answer the user's query.";

    arbiterAI::CompletionRequest completionRequest;

    completionRequest.model=request->model();
    completionRequest.messages.push_back({"user", prompt.str()});

    m_arbiterAI->streamingCompletion(
        completionRequest, [writer](const std::string &chunk)
        {
            loreforge::QueryResponse response;
            response.set_response_chunk(chunk);
            writer->Write(response);
        });

    return grpc::Status::OK;
}

grpc::Status Service::IndexRepository(
    grpc::ServerContext *context, const IndexRepositoryRequest *request,
    IndexRepositoryResponse *response)
{
    if(request->codebase_name().empty())
    {
        response->set_success(false);
        response->set_message("codebase_name is required");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "codebase_name is required");
    }

    try
    {
        auto codebase=m_globalCodebaseManager->getCodebase(request->codebase_name());

        if(codebase)
        {
            codebase->generateEmbeddings();
        }
        else
        {
            if(request->repository_path().empty())
            {
                response->set_success(false);
                response->set_message("repository_path is required for new codebases");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                    "repository_path is required for new codebases");
            }
            if(request->model().empty())
            {
                response->set_success(false);
                response->set_message("model is required for new codebases");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                    "model is required for new codebases");
            }

            auto codeParser=std::make_shared<CodeParser>();
            auto newCodebase=std::make_unique<CodebaseManager>();

            if(!newCodebase->init(codeParser, m_arbiterAI, request->repository_path(), request->model()))
            {
                response->set_success(false);
                response->set_message("Failed to initialize codebase: invalid path or model");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                    "Failed to initialize codebase");
            }
            m_globalCodebaseManager->addCodebase(request->codebase_name(), std::move(newCodebase));
        }

        response->set_success(true);
        response->set_message("Repository indexed successfully.");
        return grpc::Status::OK;
    }
    catch(const std::exception &e)
    {
        response->set_success(false);
        response->set_message(e.what());
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Service::HealthCheck(
    grpc::ServerContext *context, const HealthCheckRequest *request,
    HealthCheckResponse *response)
{
    response->set_status("SERVING");
    return grpc::Status::OK;
}

grpc::Status Service::ParseFile(
    grpc::ServerContext *context, const ParseFileRequest *request,
    ParseFileResponse *response)
{
    if(request->file_path().empty())
    {
        response->set_success(false);
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "file_path is required");
    }

    try
    {
        auto chunks=m_codeParser->parse(request->file_path(), request->content());
        response->set_success(!chunks.empty());
        return grpc::Status::OK;
    }
    catch(const std::exception &e)
    {
        response->set_success(false);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

grpc::Status Service::AnalyzeFunction(
    grpc::ServerContext *context, const AnalyzeFunctionRequest *request,
    AnalyzeFunctionResponse *response)
{
    if(request->file_path().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "file_path is required");
    }
    if(request->function_name().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "function_name is required");
    }

    auto result=m_codeParser->extractFunctionSignature(request->file_path(), request->function_name());

    response->set_found(result.found);
    if(result.found)
    {
        response->set_signature(result.signature);
        response->set_return_type(result.returnType);
        for(const auto &param:result.parameters)
        {
            response->add_parameters(param);
        }
    }
    return grpc::Status::OK;
}

grpc::Status Service::FindSimilarCode(
    grpc::ServerContext *context, const FindSimilarCodeRequest *request,
    FindSimilarCodeResponse *response)
{
    if(request->query().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "query is required");
    }
    if(request->model().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "model is required");
    }

    int topK=request->top_k();

    if(topK<=0)
    {
        topK=5;
    }

    std::vector<CodeChunk> chunks;

    if(request->codebase_name().empty())
    {
        chunks=m_globalCodebaseManager->search(request->query(), request->model(), topK);
    }
    else
    {
        chunks=m_globalCodebaseManager->searchInCodebase(
            request->codebase_name(), request->query(), request->model(), topK);
    }

    for(const auto &chunk:chunks)
    {
        auto *protoChunk=response->add_similar_chunks();

        protoChunk->set_file_path(chunk.filePath);
        protoChunk->set_start_line(chunk.startLine);
        protoChunk->set_end_line(chunk.endLine);
        protoChunk->set_content(chunk.content);
        protoChunk->set_function_name(chunk.identifier);
        protoChunk->set_score(chunk.score);
    }
    return grpc::Status::OK;
}

grpc::Status Service::GetCallGraph(
    grpc::ServerContext *context, const GetCallGraphRequest *request,
    GetCallGraphResponse *response)
{
    if(request->file_path().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "file_path is required");
    }
    if(request->function_name().empty())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "function_name is required");
    }

    auto callees=m_codeParser->findCallees(request->file_path(), request->function_name());

    response->set_found(!callees.empty()||!request->function_name().empty());
    auto *root=response->mutable_root();

    root->set_function_name(request->function_name());
    root->set_file_path(request->file_path());

    for(const auto &calleeName:callees)
    {
        auto *calleeNode=root->add_callees();
        calleeNode->set_function_name(calleeName);
    }
    return grpc::Status::OK;
}

} // namespace loreforge