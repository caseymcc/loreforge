#include "loreforge_service.h"
#include "hermesaxiom/hermesaxiom.h"

LoreForgeServiceImpl::LoreForgeServiceImpl() : m_hermesAxiom(std::make_unique<hermesaxiom::hermesaxiom>()) {}

LoreForgeServiceImpl::~LoreForgeServiceImpl() = default;

grpc::Status LoreForgeServiceImpl::QueryCodebase(
    grpc::ServerContext* context, const loreforge::QueryRequest* request,
    grpc::ServerWriter<loreforge::QueryResponse>* writer) {
    loreforge::QueryResponse response;
    response.set_response_chunk("Received query: " + request->query());
    writer->Write(response);
    return grpc::Status::OK;
}