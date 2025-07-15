#include "loreforge/service.h"
#include "loreforge/codeParser.h"
#include "loreforge/codebaseManager.h"

#include "arbiterAI/arbiterAI.h"

#include <sstream>

namespace loreforge
{

Service::Service()
{
    auto codeParser = std::make_shared<CodeParser>();
    m_arbiterAI = std::make_shared<arbiterAI::arbiterAI>();
    m_arbiterAI->initialize({ "/app/src/arbiterAI/configs/defaults" });
    m_codebaseManager = std::make_unique<CodebaseManager>(codeParser, m_arbiterAI);
}

Service::~Service()=default;

CodebaseManager *Service::getCodebaseManager()
{
    return m_codebaseManager.get();
}

grpc::Status Service::QueryCodebase(
    grpc::ServerContext *context, const QueryRequest *request,
    grpc::ServerWriter<QueryResponse> *writer)
{

    if(!m_codebaseManager->isReady())
    {
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Codebase manager is not ready.");
    }

    auto chunks = m_codebaseManager->search(request->query(), request->model(), 5);

    std::stringstream prompt;
    prompt<<"You are a helpful coding assistant. Use the following code snippets to answer the user's query.\n\n";
    prompt<<"User Query: "<<request->query()<<"\n\n";
    prompt<<"Relevant Code Snippets:\n\n";

    for(const CodeChunk &chunk:chunks)
    {
        prompt<<"File: "<<chunk.filePath<<"\n";
        prompt<<"```\n"<<chunk.content<<"\n```\n\n";
    }

    prompt<<"Based on the provided code, answer the user's query.";

    arbiterAI::CompletionRequest completionRequest;
    completionRequest.model = request->model();
    completionRequest.messages.push_back({ "user", prompt.str() });

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
    grpc::ServerContext* context, const IndexRepositoryRequest* request,
    IndexRepositoryResponse* response)
{
    try
    {
        m_codebaseManager->processDirectory(request->repository_path(), request->model());
        response->set_success(true);
        response->set_message("Repository indexed successfully.");
        return grpc::Status::OK;
    }
    catch (const std::exception& e)
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
        // For now, we just check if parsing is successful without storing the result.
        // This can be expanded later.
        auto chunks = m_codebaseManager->getCodeParser()->parse(request->file_path(), request->content());
        response->set_success(!chunks.empty());
        return grpc::Status::OK;
    }
    catch (const std::exception &e)
    {
        response->set_success(false);
        return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
    }
}

}// namespace loreforge