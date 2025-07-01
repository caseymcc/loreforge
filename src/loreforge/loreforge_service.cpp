#include "loreforge_service.h"

LoreForgeServiceImpl::LoreForgeServiceImpl() : m_hermes() {}

grpc::Status LoreForgeServiceImpl::QueryCodebase(
    grpc::ServerContext* context, const loreforge::QueryRequest* request,
    loreforge::QueryResponse* response) {

    // For now, we'll just generate an embedding and return a placeholder.
    // The actual implementation will come in a later step.
    hermesaxiom::EmbeddingRequest embeddingRequest;
    embeddingRequest.model = "text-embedding-ada-002"; // Example model
    embeddingRequest.input = request->query();

    try {
        hermesaxiom::EmbeddingResponse embeddingResponse;
        m_hermes.getEmbeddings(embeddingRequest, embeddingResponse);
        // In the future, we will use this embedding to query Faiss.
        response->set_response("Successfully generated embedding for query: " + request->query());
    } catch (const std::exception& e) {
        // Proper error handling will be added later.
        response->set_response("Failed to generate embedding: " + std::string(e.what()));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Embedding generation failed.");
    }

    return grpc::Status::OK;
}