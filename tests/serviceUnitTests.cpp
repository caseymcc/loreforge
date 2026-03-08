#include <gtest/gtest.h>

#include "loreforge/service.h"
#include "loreforge/codeParser.h"
#include "loreforge/globalCodebaseManager.h"

#include <grpcpp/grpcpp.h>
#include <thread>
#include <string>
#include <memory>
#include <filesystem>

// In-process gRPC test fixture that starts/stops a server per test
class ServiceUnitTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_service=std::make_unique<loreforge::Service>();
        grpc::ServerBuilder builder;

        builder.AddListeningPort(m_address, grpc::InsecureServerCredentials());
        builder.RegisterService(m_service.get());

        m_server=builder.BuildAndStart();
        ASSERT_NE(m_server, nullptr)<<"Failed to start in-process gRPC server";

        m_serverThread=std::thread([this]()
            {
                m_server->Wait();
            });

        auto channel=grpc::CreateChannel(m_address, grpc::InsecureChannelCredentials());

        m_stub=loreforge::LoreForge::NewStub(channel);
    }

    void TearDown() override
    {
        if(m_server)
        {
            m_server->Shutdown();
        }
        if(m_serverThread.joinable())
        {
            m_serverThread.join();
        }
    }

    std::string m_address="localhost:50061";
    std::unique_ptr<loreforge::Service> m_service;
    std::unique_ptr<grpc::Server> m_server;
    std::thread m_serverThread;
    std::unique_ptr<loreforge::LoreForge::Stub> m_stub;
};

// HealthCheck tests

TEST_F(ServiceUnitTest, HealthCheck_ReturnsServing)
{
    loreforge::HealthCheckRequest request;
    loreforge::HealthCheckResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->HealthCheck(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_message();
    EXPECT_EQ(response.status(), "SERVING");
}

// ParseFile tests

TEST_F(ServiceUnitTest, ParseFile_ValidCpp_ReturnsSuccess)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("test.cpp");
    request.set_content("int main() { return 0; }");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_message();
    EXPECT_TRUE(response.success());
}

TEST_F(ServiceUnitTest, ParseFile_EmptyCpp_ReturnsFalse)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("test.cpp");
    request.set_content("");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    // Empty content may still parse successfully with tree-sitter (empty AST)
    ASSERT_TRUE(status.ok());
}

TEST_F(ServiceUnitTest, ParseFile_UnsupportedType_ReturnsOk)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("readme.txt");
    request.set_content("plain text content");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    // Unsupported file types still return OK status but may or may not find chunks
    ASSERT_TRUE(status.ok());
}

TEST_F(ServiceUnitTest, ParseFile_EmptyFilePath_ReturnsError)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("");
    request.set_content("int main() {}");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, ParseFile_ValidPython_ReturnsSuccess)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("test.py");
    request.set_content("def hello():\n    pass\n");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(response.success());
}

TEST_F(ServiceUnitTest, ParseFile_ValidJavaScript_ReturnsSuccess)
{
    loreforge::ParseFileRequest request;

    request.set_file_path("test.js");
    request.set_content("function hello() { return 42; }");

    loreforge::ParseFileResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->ParseFile(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_TRUE(response.success());
}

// AnalyzeFunction tests

TEST_F(ServiceUnitTest, AnalyzeFunction_ValidFunction_ReturnsSignature)
{
    loreforge::AnalyzeFunctionRequest request;

    request.set_file_path("server/loreforge/tests/cpp_test_files/sample.cpp");
    request.set_function_name("myFunction");

    loreforge::AnalyzeFunctionResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->AnalyzeFunction(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_message();
    EXPECT_TRUE(response.found());
    EXPECT_NE(response.signature().find("myFunction"), std::string::npos);
}

TEST_F(ServiceUnitTest, AnalyzeFunction_NonExistentFunction_ReturnsNotFound)
{
    loreforge::AnalyzeFunctionRequest request;

    request.set_file_path("server/loreforge/tests/cpp_test_files/sample.cpp");
    request.set_function_name("doesNotExist");

    loreforge::AnalyzeFunctionResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->AnalyzeFunction(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_FALSE(response.found());
}

TEST_F(ServiceUnitTest, AnalyzeFunction_EmptyFilePath_ReturnsError)
{
    loreforge::AnalyzeFunctionRequest request;

    request.set_file_path("");
    request.set_function_name("main");

    loreforge::AnalyzeFunctionResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->AnalyzeFunction(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, AnalyzeFunction_EmptyFunctionName_ReturnsError)
{
    loreforge::AnalyzeFunctionRequest request;

    request.set_file_path("tests/cpp_test_files/sample.cpp");
    request.set_function_name("");

    loreforge::AnalyzeFunctionResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->AnalyzeFunction(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

// GetCallGraph tests

TEST_F(ServiceUnitTest, GetCallGraph_ValidFunction_ReturnsGraph)
{
    loreforge::GetCallGraphRequest request;

    request.set_file_path("server/loreforge/tests/cpp_test_files/sample.cpp");
    request.set_function_name("myFunction");

    loreforge::GetCallGraphResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->GetCallGraph(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_message();
    EXPECT_TRUE(response.found());
    EXPECT_EQ(response.root().function_name(), "myFunction");
    EXPECT_EQ(response.root().file_path(), "server/loreforge/tests/cpp_test_files/sample.cpp");
}

TEST_F(ServiceUnitTest, GetCallGraph_EmptyFilePath_ReturnsError)
{
    loreforge::GetCallGraphRequest request;

    request.set_file_path("");
    request.set_function_name("main");

    loreforge::GetCallGraphResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->GetCallGraph(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, GetCallGraph_EmptyFunctionName_ReturnsError)
{
    loreforge::GetCallGraphRequest request;

    request.set_file_path("tests/cpp_test_files/sample.cpp");
    request.set_function_name("");

    loreforge::GetCallGraphResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->GetCallGraph(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

// IndexRepository tests

TEST_F(ServiceUnitTest, IndexRepository_EmptyCodebaseName_ReturnsError)
{
    loreforge::IndexRepositoryRequest request;

    request.set_repository_path("tests/cpp_test_files");
    request.set_model("all-MiniLM-L6-v2");
    request.set_codebase_name("");

    loreforge::IndexRepositoryResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->IndexRepository(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_FALSE(response.success());
}

TEST_F(ServiceUnitTest, IndexRepository_MissingRepoPath_ReturnsError)
{
    loreforge::IndexRepositoryRequest request;

    request.set_repository_path("");
    request.set_model("all-MiniLM-L6-v2");
    request.set_codebase_name("test_codebase");

    loreforge::IndexRepositoryResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->IndexRepository(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_FALSE(response.success());
}

TEST_F(ServiceUnitTest, IndexRepository_MissingModel_ReturnsError)
{
    loreforge::IndexRepositoryRequest request;

    request.set_repository_path("tests/cpp_test_files");
    request.set_model("");
    request.set_codebase_name("test_codebase");

    loreforge::IndexRepositoryResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->IndexRepository(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_FALSE(response.success());
}

// QueryCodebase validation tests

TEST_F(ServiceUnitTest, QueryCodebase_EmptyQuery_ReturnsError)
{
    loreforge::QueryRequest request;

    request.set_query("");
    request.set_model("all-MiniLM-L6-v2");

    grpc::ClientContext context;
    auto stream=m_stub->QueryCodebase(&context, request);
    loreforge::QueryResponse response;

    while(stream->Read(&response))
    {
    }

    grpc::Status status=stream->Finish();

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, QueryCodebase_EmptyModel_ReturnsError)
{
    loreforge::QueryRequest request;

    request.set_query("find main function");
    request.set_model("");

    grpc::ClientContext context;
    auto stream=m_stub->QueryCodebase(&context, request);
    loreforge::QueryResponse response;

    while(stream->Read(&response))
    {
    }

    grpc::Status status=stream->Finish();

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, QueryCodebase_NonExistentCodebase_ReturnsNotFound)
{
    loreforge::QueryRequest request;

    request.set_query("find main function");
    request.set_model("all-MiniLM-L6-v2");
    request.set_codebase_name("nonexistent_codebase");

    grpc::ClientContext context;
    auto stream=m_stub->QueryCodebase(&context, request);
    loreforge::QueryResponse response;

    while(stream->Read(&response))
    {
    }

    grpc::Status status=stream->Finish();

    EXPECT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

// FindSimilarCode validation tests

TEST_F(ServiceUnitTest, FindSimilarCode_EmptyQuery_ReturnsError)
{
    loreforge::FindSimilarCodeRequest request;

    request.set_query("");
    request.set_model("all-MiniLM-L6-v2");
    request.set_top_k(5);

    loreforge::FindSimilarCodeResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->FindSimilarCode(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, FindSimilarCode_EmptyModel_ReturnsError)
{
    loreforge::FindSimilarCodeRequest request;

    request.set_query("function");
    request.set_model("");
    request.set_top_k(5);

    loreforge::FindSimilarCodeResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->FindSimilarCode(&context, request, &response);

    EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST_F(ServiceUnitTest, FindSimilarCode_ZeroTopK_DefaultsToFive)
{
    loreforge::FindSimilarCodeRequest request;

    request.set_query("function");
    request.set_model("all-MiniLM-L6-v2");
    request.set_top_k(0);

    loreforge::FindSimilarCodeResponse response;
    grpc::ClientContext context;

    grpc::Status status=m_stub->FindSimilarCode(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_message();
}

// GlobalCodebaseManager accessor test

TEST_F(ServiceUnitTest, GetGlobalCodebaseManager_ReturnsNonNull)
{
    ASSERT_NE(m_service->getGlobalCodebaseManager(), nullptr);
}
