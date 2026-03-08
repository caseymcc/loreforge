#include "loreforge/service.h"

#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <atomic>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

static std::atomic<bool> g_shutdownRequested{false};
static grpc::Server *g_serverPtr=nullptr;

void signalHandler(int signal)
{
    g_shutdownRequested.store(true);
    if(g_serverPtr)
    {
        g_serverPtr->Shutdown();
    }
}

void printUsage(const char *programName)
{
    std::cout<<"Usage: "<<programName<<" [options]\n"
        <<"\nOptions:\n"
        <<"  --address <host:port>  Server listen address (default: 0.0.0.0:50051)\n"
        <<"  --repository <path>   Path to repository to index on startup\n"
        <<"  --model <name>        Model name for embeddings\n"
        <<"  --help                Show this help message\n";
}

void runServer(const std::string &address, const std::string &repositoryPath,
    const std::string &model)
{
    loreforge::Service service(repositoryPath, model);
    grpc::ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if(!server)
    {
        std::cerr<<"Failed to start server on "<<address<<std::endl;
        return;
    }

    g_serverPtr=server.get();

    std::cout<<"Loreforge gRPC server listening on "<<address<<std::endl;

    if(!repositoryPath.empty())
    {
        std::cout<<"Default repository: "<<repositoryPath<<std::endl;
    }

    server->Wait();
    g_serverPtr=nullptr;

    std::cout<<"Server shut down gracefully."<<std::endl;
}

int main(int argc, char **argv)
{
    std::string address="0.0.0.0:50051";
    std::string repositoryPath;
    std::string model;

    for(int i=1; i<argc; ++i)
    {
        std::string arg=argv[i];

        if(arg=="--help")
        {
            printUsage(argv[0]);
            return 0;
        }
        else if(arg=="--address"&&i+1<argc)
        {
            address=argv[++i];
        }
        else if(arg=="--repository"&&i+1<argc)
        {
            repositoryPath=argv[++i];
        }
        else if(arg=="--model"&&i+1<argc)
        {
            model=argv[++i];
        }
        else
        {
            std::cerr<<"Unknown option: "<<arg<<std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    runServer(address, repositoryPath, model);
    return 0;
}