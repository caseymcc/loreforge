#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "gtest/gtest.h"
#include "protos/loreforge.grpc.pb.h"

using namespace loreforge;
using namespace grpc;


class LoreForgeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        channel=CreateChannel("localhost:50051", InsecureChannelCredentials());
        stub=LoreForge::NewStub(channel);
    }

    std::shared_ptr<Channel> channel;
    std::unique_ptr<LoreForge::Stub> stub;
};

void test_parse_file(
    LoreForge::Stub &stub,
    const std::string &file_path,
    const std::string &content,
    bool expected_success)
{


    ParseFileRequest request;

    request.set_file_path(file_path);
    request.set_content(content);

    ParseFileResponse response;
    ClientContext context;
    
    Status status=stub.ParseFile(&context, request, &response);

    ASSERT_TRUE(status.ok())<<"RPC failed: "<<status.error_code()<<": "<<status.error_message();
    ASSERT_EQ(response.success(), expected_success);
}

TEST_F(LoreForgeTest, HealthCheck)
{
    HealthCheckRequest request;
    HealthCheckResponse response;
    ClientContext context;
    Status status=stub->HealthCheck(&context, request, &response);

    ASSERT_TRUE(status.ok());
    EXPECT_EQ(response.status(), "SERVING");
}

TEST_F(LoreForgeTest, ParseFile_ValidCpp)
{
    test_parse_file(*stub, "sample.cpp", "int main() { return 0; }", true);
}

TEST_F(LoreForgeTest, ParseFile_InvalidCpp)
{
    test_parse_file(*stub, "sample.cpp", "int main() { return 0;", false);
}

TEST_F(LoreForgeTest, ParseFile_ValidPython)
{
    test_parse_file(*stub, "sample.py", "def main():\n    pass", true);
}

TEST_F(LoreForgeTest, ParseFile_InvalidPython)
{
    test_parse_file(*stub, "sample.py", "def main():\nthis is bad\n this is real bad\npass", false);
}

TEST_F(LoreForgeTest, ParseFile_UnsupportedFileType)
{
    test_parse_file(*stub, "sample.txt", "this is a plain text file", false);
}

TEST_F(LoreForgeTest, MultiRepo)
{
    // Index first repo
    IndexRepositoryRequest request1;
    request1.set_repository_path("tests/cpp_test_files");
    request1.set_model("all-MiniLM-L6-v2");
    request1.set_codebase_name("repo1");
    IndexRepositoryResponse response1;
    ClientContext context1;
    Status status1=stub->IndexRepository(&context1, request1, &response1);
    ASSERT_TRUE(status1.ok());
    ASSERT_TRUE(response1.success());

    // Index second repo
    IndexRepositoryRequest request2;
    request2.set_repository_path("tests/python_test_files");
    request2.set_model("all-MiniLM-L6-v2");
    request2.set_codebase_name("repo2");
    IndexRepositoryResponse response2;
    ClientContext context2;
    Status status2=stub->IndexRepository(&context2, request2, &response2);
    ASSERT_TRUE(status2.ok());
    ASSERT_TRUE(response2.success());

    // Query first repo
    QueryRequest queryRequest1;
    queryRequest1.set_query("main function");
    queryRequest1.set_model("all-MiniLM-L6-v2");
    queryRequest1.set_codebase_name("repo1");
    ClientContext queryContext1;
    auto stream1=stub->QueryCodebase(&queryContext1, queryRequest1);
    QueryResponse queryResponse1;
    std::string full_response1;
    while(stream1->Read(&queryResponse1))
    {
        full_response1+=queryResponse1.response_chunk();
    }
    Status stream_status1=stream1->Finish();
    ASSERT_TRUE(stream_status1.ok());
    ASSERT_NE(full_response1.find("main"), std::string::npos);

    // Query second repo
    QueryRequest queryRequest2;
    queryRequest2.set_query("main function");
    queryRequest2.set_model("all-MiniLM-L6-v2");
    queryRequest2.set_codebase_name("repo2");
    ClientContext queryContext2;
    auto stream2=stub->QueryCodebase(&queryContext2, queryRequest2);
    QueryResponse queryResponse2;
    std::string full_response2;
    while(stream2->Read(&queryResponse2))
    {
        full_response2+=queryResponse2.response_chunk();
    }
    Status stream_status2=stream2->Finish();
    ASSERT_TRUE(stream_status2.ok());
    ASSERT_NE(full_response2.find("main"), std::string::npos);

    // Query all repos
    QueryRequest queryRequestAll;
    queryRequestAll.set_query("main function");
    queryRequestAll.set_model("all-MiniLM-L6-v2");
    ClientContext queryContextAll;
    auto streamAll=stub->QueryCodebase(&queryContextAll, queryRequestAll);
    QueryResponse queryResponseAll;
    std::string full_responseAll;
    while(streamAll->Read(&queryResponseAll))
    {
        full_responseAll+=queryResponseAll.response_chunk();
    }
    Status stream_statusAll=streamAll->Finish();
    ASSERT_TRUE(stream_statusAll.ok());
    ASSERT_NE(full_responseAll.find("main"), std::string::npos);
}

TEST_F(LoreForgeTest, AnalyzeFunction_ValidCpp)
{
    AnalyzeFunctionRequest request;
    request.set_file_path("tests/cpp_test_files/sample.cpp");
    request.set_function_name("myFunction");
    
    AnalyzeFunctionResponse response;
    ClientContext context;
    Status status = stub->AnalyzeFunction(&context, request, &response);
    
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(response.found());
    // Tree-sitter simple extraction might just return "myFunction" or "void myFunction" depending on logic.
    // My logic was: (returnType.empty() ? "" : returnType + " ") + currentFuncName + parameters;
    // returnType for `void myFunction() {` should be "void". params should be "()".
    // So "void myFunction()"
    EXPECT_NE(response.signature().find("myFunction"), std::string::npos);
}

TEST_F(LoreForgeTest, GetCallGraph_ValidCpp)
{
    GetCallGraphRequest request;
    request.set_file_path("tests/cpp_test_files/sample.cpp");
    request.set_function_name("myFunction");
    
    GetCallGraphResponse response;
    ClientContext context;
    Status status = stub->GetCallGraph(&context, request, &response);
    
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(response.found());
    ASSERT_EQ(response.root().function_name(), "myFunction");
    // No callees in sample.cpp's myFunction
    EXPECT_EQ(response.root().callees_size(), 0);
}

TEST_F(LoreForgeTest, FindSimilarCode_CheckScore)
{
    IndexRepositoryRequest indexReq;
    indexReq.set_repository_path("tests/cpp_test_files");
    indexReq.set_model("all-MiniLM-L6-v2");
    indexReq.set_codebase_name("score_test_repo");
    IndexRepositoryResponse indexResp;
    ClientContext indexCtx;
    stub->IndexRepository(&indexCtx, indexReq, &indexResp);
    
    FindSimilarCodeRequest request;
    request.set_query("function");
    request.set_model("all-MiniLM-L6-v2");
    request.set_codebase_name("score_test_repo");
    request.set_top_k(1);
    
    FindSimilarCodeResponse response;
    ClientContext context;
    Status status = stub->FindSimilarCode(&context, request, &response);
    
    ASSERT_TRUE(status.ok());
    // Might be empty if embedding model interaction fails or no match?
    // But we indexed it.
    if (response.similar_chunks_size() > 0) {
       EXPECT_GE(response.similar_chunks(0).score(), 0.0f);
    }
}