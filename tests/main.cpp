#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "protos/loreforge.grpc.pb.h"

int main(int argc, char** argv) {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials());

    std::unique_ptr<loreforge::LoreForge::Stub> stub = loreforge::LoreForge::NewStub(channel);

    // HealthCheck
    {
        loreforge::HealthCheckRequest request;
        loreforge::HealthCheckResponse response;
        grpc::ClientContext context;
        grpc::Status status = stub->HealthCheck(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "HealthCheck RPC failed: " << status.error_code() << ": "
                      << status.error_message() << std::endl;
            return 1;
        }
        std::cout << "HealthCheck successful. Status: " << response.status() << std::endl;
    }

    // ParseFile
    {
        loreforge::ParseFileRequest request;
        request.set_file_path("sample.cpp");
        request.set_content("int main() { return 0; }");

        loreforge::ParseFileResponse response;
        grpc::ClientContext context;
        grpc::Status status = stub->ParseFile(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "ParseFile RPC failed: " << status.error_code() << ": "
                      << status.error_message() << std::endl;
            return 1;
        }

        std::cout << "ParseFile call was successful: " << std::boolalpha << response.success() << std::endl;
        if (!response.success()) {
            std::cerr << "ParseFile logic indicates failure." << std::endl;
            return 1;
        }
    }

    return 0;
}