#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "protos/loreforge.grpc.pb.h"

#include <tree_sitter/api.h>

// Declare the external C functions for the C++ and Python grammars
extern "C" TSLanguage* tree_sitter_cpp();
extern "C" TSLanguage* tree_sitter_python();

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

        TSParser* parser = ts_parser_new();
        TSLanguage* language = nullptr;

        std::string filePath = request->file_path();
        if (filePath.ends_with(".cpp")) {
            language = tree_sitter_cpp();
        } else if (filePath.ends_with(".py")) {
            language = tree_sitter_python();
        } else {
            std::cerr << "Unsupported language for file: " << filePath << std::endl;
            response->set_success(false);
            ts_parser_delete(parser);
            return Status::OK;
        }

        ts_parser_set_language(parser, language);

        std::string content = request->content();
        TSTree* tree = ts_parser_parse_string(parser, NULL, content.c_str(), content.length());
        TSNode root_node = ts_tree_root_node(tree);

        // A more reliable way to check for errors is to see if the S-expression string
        // contains an ERROR node.
        char* s_expression = ts_node_string(root_node);
        std::string tree_string(s_expression);
        free(s_expression);

        // If the tree contains an ERROR node, the parse was not successful.
        // We also check for a completely null root node.
        bool success = !ts_node_is_null(root_node) && (tree_string.find("ERROR") == std::string::npos) && (tree_string.find("MISSING") == std::string::npos);
        response->set_success(success);

        ts_tree_delete(tree);
        ts_parser_delete(parser);

        return Status::OK;
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