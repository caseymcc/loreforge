#ifndef _loreforge_service_h
#define _loreforge_service_h

#include "loreforge/codeParser.h"
#include "loreforge/codebaseManager.h"
#include "protos/loreforge.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <memory>

namespace loreforge
{

class CodebaseManager;

class Service final : public loreforge::LoreForge::Service
{
public:
    Service();
    ~Service();

    loreforge::CodebaseManager *getCodebaseManager();

private:
    grpc::Status QueryCodebase(grpc::ServerContext *context,
        const QueryRequest *request,
        grpc::ServerWriter<QueryResponse> *writer) override;

    grpc::Status IndexRepository(grpc::ServerContext *context,
        const IndexRepositoryRequest *request,
        IndexRepositoryResponse *response) override;

    grpc::Status HealthCheck(
        grpc::ServerContext *context, const HealthCheckRequest *request,
        HealthCheckResponse *response) override;

    grpc::Status ParseFile(
        grpc::ServerContext *context, const ParseFileRequest *request,
        ParseFileResponse *response) override;

    std::unique_ptr<loreforge::CodebaseManager> m_codebaseManager;
    std::shared_ptr<arbiterAI::arbiterAI> m_arbiterAI;
};

}// namespace loreforge

#endif // _loreforge_service_h