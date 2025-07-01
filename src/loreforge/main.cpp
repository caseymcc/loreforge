#include "loreforge/service.h"

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

void runServer()
{
    loreforge::Service service;
    grpc::ServerBuilder builder;
    std::string serverAddress("0.0.0.0:50051");

    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout<<"Server listening on "<<serverAddress<<std::endl;
    std::cout<<"Indexing codebase..."<<std::endl;
    service.getCodebaseManager()->indexDirectory(".");
    std::cout<<"Codebase indexed."<<std::endl;

    server->Wait();
}

int main(int argc, char **argv)
{
    runServer();
    return 0;
}