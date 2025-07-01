#include "loreforge/service.h"
#include "loreforge/codeParser.h"
#include "loreforge/codebaseManager.h"

#include "hermesaxiom/hermesaxiom.h"

#include <sstream>

namespace loreforge
{

Service::Service()
{
    std::shared_ptr<CodeParser> codeParser=std::make_shared<CodeParser>();
    m_hermesAxiom=std::make_shared<hermesaxiom::hermesaxiom>();
    m_codebaseManager=std::make_unique<CodebaseManager>(codeParser, m_hermesAxiom);
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

    std::vector<CodeChunk> chunks=m_codebaseManager->query(request->query());

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

    hermesaxiom::CompletionRequest completionRequest;
    completionRequest.model="deepseek/deepseek-coder-v2";
    completionRequest.messages.push_back({ "user", prompt.str() });

    m_hermesAxiom->streamingCompletion(
        completionRequest, [writer](const std::string &chunk)
        {
            loreforge::QueryResponse response;
            response.set_response_chunk(chunk);
            writer->Write(response);
        });

    return grpc::Status::OK;
}

}// namespace loreforge