#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "protos/loreforge.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using loreforge::HealthCheckRequest;
using loreforge::HealthCheckResponse;
using loreforge::LoreForge;
using loreforge::ParseFileRequest;
using loreforge::ParseFileResponse;

// Service implementation
class LoreForgeServiceImpl final : public LoreForge::Service {
    Status HealthCheck(ServerContext* context, const HealthCheckRequest* request,
                       HealthCheckResponse* response) override {
        response->set_status("SERVING");
        return Status::OK;
    }

    Status ParseFile(ServerContext* context, const ParseFileRequest* request,
                     ParseFileResponse* response) override {
        std::cout << "Received ParseFile request for: " << request->file_path() << std::endl;
        response->set_success(true);
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    LoreForgeServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}