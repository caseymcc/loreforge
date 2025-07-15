#include <gtest/gtest.h>
#include "arbiterAI/modelDownloader.h"
#include <filesystem>

class ConfigLoaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create temp directory for tests
        tempDir = std::filesystem::temp_directory_path() / "arbiterai_test";
        std::filesystem::create_directories(tempDir);
    }

    void TearDown() override
    {
        // Cleanup temp directory
        std::filesystem::remove_all(tempDir);
    }

    std::filesystem::path tempDir;
};

TEST_F(ConfigLoaderTest, GitHubConfigDownload)
{
    arbiterAI::ModelDownloader downloader;
    auto future = downloader.downloadConfigFromRepo("caseymcc", "arbiterai", 
        "configs/defaults/models/llama.json");
    
    auto config = future.get();
    EXPECT_FALSE(config.empty());
    EXPECT_TRUE(config.contains("model"));
    EXPECT_TRUE(config.contains("provider"));
}

TEST_F(ConfigLoaderTest, ConfigValidation)
{
    arbiterAI::ModelDownloader downloader;
    std::string validJson = R"({
        "model": "test-model",
        "provider": "test-provider",
        "configVersion": "1.1.0",
        "minSchemaVersion": "1.0.0"
    })";
    
    auto parsed = downloader.parseConfigFromJSON(validJson);
    EXPECT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->at("model"), "test-model");
}

TEST_F(ConfigLoaderTest, InvalidConfig)
{
    arbiterAI::ModelDownloader downloader;
    std::string invalidJson = "invalid json";
    
    auto parsed = downloader.parseConfigFromJSON(invalidJson);
    EXPECT_FALSE(parsed.has_value());
}

TEST_F(ConfigLoaderTest, FileVerification)
{
    arbiterAI::ModelDownloader downloader;
    std::string testFile = (tempDir / "test.txt").string();
    std::ofstream(testFile) << "test content";
    
    // Test with correct hash
    EXPECT_TRUE(downloader.verifyFile(testFile, 
        "6ae8a75555209fd6c44157c0aed8016e763ff435a19cf186f76863140143ff72"));
    
    // Test with incorrect hash
    EXPECT_FALSE(downloader.verifyFile(testFile, "wronghash"));
}