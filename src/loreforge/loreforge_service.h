#pragma once

#include "protos/loreforge.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>

namespace hermesaxiom {
    class hermesaxiom;
}

class LoreForgeServiceImpl final : public loreforge::LoreForge::Service {
public:
    LoreForgeServiceImpl();
    ~LoreForgeServiceImpl();

private:
    grpc::Status QueryCodebase(grpc::ServerContext* context,
                               const loreforge::QueryRequest* request,
                               grpc::ServerWriter<loreforge::QueryResponse>* writer) override;

    std::unique_ptr<hermesaxiom::hermesaxiom> m_hermesAxiom;
};