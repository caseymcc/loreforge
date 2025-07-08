// IMPORTANT: To run these tests, you must download the hermes-2-pro-mistral-7b.Q4_K_M.gguf model
// and place it in a `models` directory in the project root.
#include <gtest/gtest.h>
#include "loreforge/codeParser.h"
#include "loreforge/codebaseManager.h"
#include "loreforge/service.h"
#include <vector>
#include <string>

// Test fixture for embedding tests
class EmbeddingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Set up any objects needed for the tests
    }

    void TearDown() override
    {
        // Clean up any objects created
    }
};

TEST_F(EmbeddingTest, CodeChunking_CPP)
{
    loreforge::CodeParser parser;
    std::string file_path="tests/cpp_test_files/sample.cpp";
    std::vector<loreforge::CodeChunk> chunks=parser.parse(file_path);

    ASSERT_EQ(chunks.size(), 3);

    bool found_class=false;

    for(const auto &chunk:chunks)
    {
        if(chunk.identifier=="MyClass")
        {
            found_class=true;
            EXPECT_EQ(chunk.type, "class_specifier");
            EXPECT_EQ(chunk.startLine, 1);
            EXPECT_EQ(chunk.endLine, 6);
        }
    }

    EXPECT_TRUE(found_class);
}

TEST_F(EmbeddingTest, CodeChunking_Python)
{
    loreforge::CodeParser parser;
    std::string file_path="tests/python_test_files/sample.py";
    std::vector<loreforge::CodeChunk> chunks=parser.parse(file_path);

    ASSERT_EQ(chunks.size(), 3);

    // Check the class
    EXPECT_EQ(chunks[0].identifier, "MyClass");
    EXPECT_EQ(chunks[0].type, "class_definition");
    EXPECT_EQ(chunks[0].startLine, 1);
    EXPECT_EQ(chunks[0].endLine, 3);
    EXPECT_EQ(chunks[0].filePath, file_path);

    // Check the method
    EXPECT_EQ(chunks[1].identifier, "my_method");
    EXPECT_EQ(chunks[1].type, "function_definition");
    EXPECT_EQ(chunks[1].startLine, 2);
    EXPECT_EQ(chunks[1].endLine, 3);
    EXPECT_EQ(chunks[1].filePath, file_path);

    // Check the function
    EXPECT_EQ(chunks[2].identifier, "my_function");
    EXPECT_EQ(chunks[2].type, "function_definition");
    EXPECT_EQ(chunks[2].startLine, 5);
    EXPECT_EQ(chunks[2].endLine, 6);
    EXPECT_EQ(chunks[2].filePath, file_path);
}

TEST_F(EmbeddingTest, EmbeddingGeneration)
{
    auto parser=std::make_shared<loreforge::CodeParser>();
    auto hermes=std::make_shared<hermesaxiom::hermesaxiom>();
    hermes->initialize({ "/app/src/hermesaxiom/configs/defaults" });
    auto manager=std::make_shared<loreforge::CodebaseManager>(parser, hermes);

    std::string test_dir="tests/cpp_test_files";
    manager->processDirectory(test_dir, "hermes-2-pro-mistral-7b.Q4_K_M.gguf");

    EXPECT_TRUE(manager->isReady());
    EXPECT_EQ(manager->getVectorCount(), 3);
}

TEST_F(EmbeddingTest, VectorStorage_SaveAndLoad)
{
    auto parser=std::make_shared<loreforge::CodeParser>();
    auto hermes=std::make_shared<hermesaxiom::hermesaxiom>();
    hermes->initialize({ "/app/src/hermesaxiom/configs/defaults" });
    auto manager1=std::make_shared<loreforge::CodebaseManager>(parser, hermes);

    std::string test_dir="tests/cpp_test_files";
    manager1->processDirectory(test_dir, "hermes-2-pro-mistral-7b.Q4_K_M.gguf");

    std::string index_path="test_index.faiss";
    manager1->saveIndex(index_path);

    auto manager2=std::make_shared<loreforge::CodebaseManager>(parser, hermes);
    EXPECT_TRUE(manager2->loadIndex(index_path));
    EXPECT_TRUE(manager2->isReady());
    EXPECT_EQ(manager2->getVectorCount(), 3);
}

TEST_F(EmbeddingTest, EndToEndPipeline)
{
    auto parser=std::make_shared<loreforge::CodeParser>();
    auto hermes=std::make_shared<hermesaxiom::hermesaxiom>();
    hermes->initialize({ "/app/src/hermesaxiom/configs/defaults" });
    auto manager=std::make_shared<loreforge::CodebaseManager>(parser, hermes);

    std::string test_dir="tests/cpp_test_files";
    manager->processDirectory(test_dir, "hermes-2-pro-mistral-7b.Q4_K_M.gguf");

    std::vector<loreforge::CodeChunk> results=manager->search("myFunction", "hermes-2-pro-mistral-7b.Q4_K_M.gguf", 1);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].identifier, "myFunction");
}

#include <grpcpp/grpcpp.h>
#include <thread>

TEST_F(EmbeddingTest, gRPC_SimilaritySearch)
{
    loreforge::Service service;
    grpc::ServerBuilder builder;
    std::string server_address("0.0.0.0:50051");
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::thread server_thread([&server]() { server->Wait(); });

    // Index the repository first
    service.getCodebaseManager()->processDirectory("tests/cpp_test_files", "hermes-2-pro-mistral-7b.Q4_K_M.gguf");

    auto channel=grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    auto stub=loreforge::LoreForge::NewStub(channel);

    loreforge::QueryRequest request;
    request.set_query("myFunction");
    request.set_model("hermes-2-pro-mistral-7b.Q4_K_M.gguf");

    loreforge::QueryResponse response;
    grpc::ClientContext context;
    auto stream=stub->QueryCodebase(&context, request);

    std::string full_response;
    while(stream->Read(&response))
    {
        full_response+=response.response_chunk();
    }
    grpc::Status status=stream->Finish();

    EXPECT_TRUE(status.ok());
    EXPECT_FALSE(full_response.empty());
    // A more robust test would be to check the content of the response
    // to ensure it contains the expected code chunk.
    EXPECT_NE(full_response.find("myFunction"), std::string::npos);

    server->Shutdown();
    server_thread.join();
}