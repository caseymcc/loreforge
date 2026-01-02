#include "loreforge/service.h"
#include "loreforge/globalCodebaseManager.h"
#include "loreforge/codeParser.h"

#include "arbiterAI/arbiterAI.h"

#include <sstream>
#include <iostream>

namespace loreforge
{

Service::Service(const std::string &repositoryPath, const std::string &model)
{
    m_arbiterAI=std::make_shared<arbiterAI::ArbiterAI>();
    m_arbiterAI->initialize({ "/app/src/arbiterAI/configs/defaults" });
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
    std::vector<CodeChunk> chunks;
    if(request->codebase_name().empty())
    {
        chunks=m_globalCodebaseManager->search(request->query(), request->model(), 5);
    }
    else
    {
        auto codebase=m_globalCodebaseManager->getCodebase(request->codebase_name());
        if(!codebase||!codebase->isReady())
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Codebase not ready.");
        }
        chunks=codebase->search(request->query(), request->model(), 5);
    }

    // a. Get the query string from the request object.
    const std::string &query=request->query();

    // d. Construct a prompt for the LLM that includes the original query and the retrieved code chunks.
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

    // e. Use the CodebaseManager to send this prompt to the LLM and get a response.
    arbiterAI::CompletionRequest completionRequest;
    completionRequest.model=request->model();
    completionRequest.messages.push_back({ "user", prompt.str() });

    // f. Set the LLM's response in the reply object.
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
    try
    {
        auto codebase=m_globalCodebaseManager->getCodebase(request->codebase_name());
        if(codebase)
        {
            codebase->generateEmbeddings();
        }
        else
        {
            auto codeParser=std::make_shared<CodeParser>();
            auto newCodebase=std::make_unique<CodebaseManager>();
            if(!newCodebase->init(codeParser, m_arbiterAI, request->repository_path(), request->model()))
            {
                response->set_success(false);
                response->set_message("Failed to initialize codebase: invalid path or model");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Failed to initialize codebase");
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
    try
    {
        std::cout << "DEBUG: Entering Service::ParseFile" << std::endl;
        auto chunks=m_codeParser->parse(request->file_path(), request->content());
        std::cout << "DEBUG: Service::ParseFile returned " << chunks.size() << " chunks" << std::endl;

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
    auto result = m_codeParser->extractFunctionSignature(request->file_path(), request->function_name());
    response->set_found(result.found);
    if (result.found) {
        response->set_signature(result.signature);
        response->set_return_type(result.returnType);
        for (const auto& param : result.parameters) {
            response->add_parameters(param);
        }
    }
    return grpc::Status::OK;
}

grpc::Status Service::FindSimilarCode(
    grpc::ServerContext *context, const FindSimilarCodeRequest *request,
    FindSimilarCodeResponse *response)
{
    std::vector<CodeChunk> chunks;
    if (request->codebase_name().empty()) {
        chunks = m_globalCodebaseManager->search(request->query(), request->model(), request->top_k());
    } else {
        chunks = m_globalCodebaseManager->searchInCodebase(request->codebase_name(), request->query(), request->model(), request->top_k());
    }

    for (const auto& chunk : chunks) {
        auto* protoChunk = response->add_similar_chunks();
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
    auto callees = m_codeParser->findCallees(request->file_path(), request->function_name());
    
    response->set_found(true);
    auto* root = response->mutable_root();
    root->set_function_name(request->function_name());
    root->set_file_path(request->file_path());
    
    for (const auto& calleeName : callees) {
        auto* calleeNode = root->add_callees();
        calleeNode->set_function_name(calleeName);
    }
    return grpc::Status::OK;
}

} // namespace loreforge