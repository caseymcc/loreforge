#pragma once

#include "hermesaxiom/hermesaxiom.h"
#include "protos/loreforge.grpc.pb.h"
#include <grpcpp/grpcpp.h>

class LoreForgeServiceImpl final : public loreforge::LoreForge::Service {
public:
    LoreForgeServiceImpl();

private:
    grpc::Status QueryCodebase(grpc::ServerContext* context,
                               const loreforge::QueryRequest* request,
                               loreforge::QueryResponse* response) override;

    hermesaxiom::hermesaxiom m_hermes;
};