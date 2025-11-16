// IMPORTANT: To run these tests, you must download the hermes-2-pro-mistral-7b.Q4_K_M.gguf model
// and place it in a `models` directory in the project root.
#include <gtest/gtest.h>
#include "loreforge/codeParser.h"
#include "loreforge/codebaseManager.h"
#include "loreforge/service.h"
#include "arbiterAI/modelManager.h"

#include <grpcpp/grpcpp.h>
#include <thread>
#include <vector>
#include <string>
#include <filesystem>

#define SKIP_IF_NO_MODEL(modelPath) \
    if (!std::filesystem::exists(modelPath)) { \
        GTEST_SKIP() << "Skipping test - model file not found: " << modelPath; \
    }

const std::string TEST_MODEL = "models/hermes-2-pro-mistral-7b.Q4_K_M.gguf";

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
    SKIP_IF_NO_MODEL(TEST_MODEL);

    auto parser=std::make_shared<loreforge::CodeParser>();
    auto arbiter = std::make_shared<arbiterAI::ArbiterAI>();
    arbiter->initialize({ "src/arbiterAI/configs/defaults" });
    auto manager = std::make_shared<loreforge::CodebaseManager>();
    manager->init(parser, arbiter, "tests/cpp_test_files", TEST_MODEL);

    manager->generateEmbeddings();

    EXPECT_TRUE(manager->isReady());
    EXPECT_EQ(manager->getVectorCount(), 3);
}

TEST_F(EmbeddingTest, VectorStorage_SaveAndLoad)
{
    SKIP_IF_NO_MODEL(TEST_MODEL);

    auto parser=std::make_shared<loreforge::CodeParser>();
    auto arbiter = std::make_shared<arbiterAI::ArbiterAI>();
    arbiter->initialize({ "src/arbiterAI/configs/defaults" });
    auto manager1 = std::make_shared<loreforge::CodebaseManager>();
    manager1->init(parser, arbiter, "tests/cpp_test_files", TEST_MODEL);

    manager1->generateEmbeddings();

    std::string index_path="test_index.faiss";
    manager1->saveIndex(index_path);

    auto manager2 = std::make_shared<loreforge::CodebaseManager>();
    manager2->init(parser, arbiter, "", "");
    EXPECT_TRUE(manager2->loadIndex(index_path));
    EXPECT_TRUE(manager2->isReady());
    EXPECT_EQ(manager2->getVectorCount(), 3);
}

TEST_F(EmbeddingTest, EndToEndPipeline)
{
    SKIP_IF_NO_MODEL(TEST_MODEL);

    auto parser=std::make_shared<loreforge::CodeParser>();
    auto arbiter = std::make_shared<arbiterAI::ArbiterAI>();
    arbiter->initialize({ "src/arbiterAI/configs/defaults" });
    auto manager = std::make_shared<loreforge::CodebaseManager>();
    manager->init(parser, arbiter, "tests/cpp_test_files", TEST_MODEL);

    manager->generateEmbeddings();

    std::vector<loreforge::CodeChunk> results=manager->search("myFunction", "hermes-2-pro-mistral-7b.Q4_K_M.gguf", 1);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].identifier, "myFunction");
}


TEST_F(EmbeddingTest, gRPC_SimilaritySearch)
{
    // Setup server
    loreforge::Service service("tests/cpp_test_files", TEST_MODEL);
    grpc::ServerBuilder builder;
    const std::string serverAddress("0.0.0.0:50051");
    const std::string modelName=TEST_MODEL;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    // Start server thread
    std::thread server_thread([&server]()
        {
            try
            {
                server->Wait();
            }
            catch(...)
            {
                // Ignore shutdown exceptions
            }
        });

    // Check for model file
    if(!std::filesystem::exists(modelName))
    {
        server->Shutdown();
        if(server_thread.joinable())
        {
            server_thread.join();
        }
        GTEST_SKIP()<<"Skipping test - model file not found: "<<modelName;
        return;
    }

    try
    {
        // Process test files
        service.getGlobalCodebaseManager()->getCodebase("default")->generateEmbeddings();

        // Setup client
        auto channel=grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
        auto stub=loreforge::LoreForge::NewStub(channel);

        // Create and send request
        loreforge::QueryRequest request;
        request.set_query("myFunction");
        request.set_model(modelName);

        // Get response stream
        loreforge::QueryResponse response;
        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<loreforge::QueryResponse>> stream=
            stub->QueryCodebase(&context, request);

        // Read response
        std::string full_response;
        while(stream->Read(&response))
        {
            full_response+=response.response_chunk();
        }
        grpc::Status status=stream->Finish();

        // Verify results
        EXPECT_TRUE(status.ok());
        EXPECT_FALSE(full_response.empty());
        EXPECT_NE(full_response.find("myFunction"), std::string::npos);

    }
    catch(const std::exception &e)
    {
        FAIL()<<"Exception in gRPC test: "<<e.what();
    }

    // Cleanup
    server->Shutdown();
    if(server_thread.joinable())
    {
        server_thread.join();
    }
}

// TEST_F(EmbeddingTest, SecureModelDownload)
// {
//     SKIP_IF_NO_MODEL(TEST_MODEL);
    
//     auto hermes = std::make_shared<arbiterAI::arbiterAI>();
//     hermes->initialize({ "src/arbiterAI/configs/defaults" });
    
//     // Test with valid model path
//     EXPECT_NO_THROW(hermes->loadModel(TEST_MODEL));
    
//     // Test with invalid model path
//     EXPECT_THROW(hermes->loadModel("invalid/path"), std::runtime_error);
// }

TEST_F(EmbeddingTest, ConfigValidation)
{
    auto hermes = std::make_shared<arbiterAI::ArbiterAI>();
    
    // Test valid config
    EXPECT_EQ(hermes->initialize({ "/app/src/arbiterAI_config/configs/defaults" }), arbiterAI::ErrorCode::Success);
    
    // Test invalid config path
    EXPECT_NE(hermes->initialize({ "invalid/config/path" }), arbiterAI::ErrorCode::Success);
}

// TEST_F(EmbeddingTest, ModelSecurityFeatures)
// {
//     SKIP_IF_NO_MODEL(TEST_MODEL);
    
//     auto hermes = std::make_shared<arbiterAI::arbiterAI>();
    
//     // Test with valid config that has security features
//     hermes->initialize({ "src/arbiterAI/configs/defaults" });
//     auto modelInfo = hermes->getModelManager().getModelInfo(TEST_MODEL);
    
//     EXPECT_TRUE(modelInfo.has_value());
//     if (modelInfo) {
//         // Verify security features exist
//         EXPECT_TRUE(modelInfo->download.has_value());
//         EXPECT_FALSE(modelInfo->download->sha256.empty());
//     }
// }