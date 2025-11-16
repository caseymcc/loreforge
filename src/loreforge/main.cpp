#include "loreforge/service.h"

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

void runServer(const std::string& repositoryPath, const std::string& model)
{
    loreforge::Service service(repositoryPath, model);
    grpc::ServerBuilder builder;
    std::string serverAddress("0.0.0.0:50051");

    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout<<"Server listening on "<<serverAddress<<std::endl;

    server->Wait();
}

int main(int argc, char **argv)
{
    std::string repositoryPath = "";
    std::string model = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--repository" && i + 1 < argc) {
            repositoryPath = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            model = argv[++i];
        }
    }

    runServer(repositoryPath, model);
    return 0;
}