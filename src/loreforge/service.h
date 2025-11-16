#ifndef _loreforge_service_h
#define _loreforge_service_h

#include "loreforge/globalCodebaseManager.h"
#include "protos/loreforge.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <memory>

namespace loreforge
{

class GlobalCodebaseManager;

class Service : public loreforge::LoreForge::Service
{
public:
    Service(const std::string& repositoryPath = "", const std::string& model = "");
    ~Service();

    loreforge::GlobalCodebaseManager *getGlobalCodebaseManager();

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

    std::unique_ptr<loreforge::GlobalCodebaseManager> m_globalCodebaseManager;
    std::shared_ptr<CodeParser> m_codeParser;
    std::shared_ptr<arbiterAI::ArbiterAI> m_arbiterAI;
};

}// namespace loreforge

#endif // _loreforge_service_h