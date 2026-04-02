#include <gtest/gtest.h>
#include "arbiterAI/modelManager.h"
#include <filesystem>

class ModelManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize with test configs
        std::vector<std::filesystem::path> configPaths = {
            "/app/src/arbiterAI_config/configs/defaults"
        };
        arbiterAI::ModelManager::instance().initialize(configPaths);
    }

    void TearDown() override
    {
        // Cleanup if needed
    }
};

TEST_F(ModelManagerTest, Initialization)
{
    auto& manager = arbiterAI::ModelManager::instance();
    EXPECT_TRUE(manager.getModelProviderMap().size() > 0);
}

TEST_F(ModelManagerTest, GetProvider)
{
    auto& manager = arbiterAI::ModelManager::instance();
    auto provider = manager.getProvider("llama-2-7b-chat.Q4_K_M.gguf");
    EXPECT_TRUE(provider.has_value());
}

TEST_F(ModelManagerTest, GetModelInfo)
{
    auto& manager = arbiterAI::ModelManager::instance();
    auto modelInfo = manager.getModelInfo("llama-2-7b-chat.Q4_K_M.gguf");
    
    EXPECT_TRUE(modelInfo.has_value());
    if (modelInfo) {
        EXPECT_FALSE(modelInfo->model.empty());
        EXPECT_FALSE(modelInfo->provider.empty());
    }
}

TEST_F(ModelManagerTest, GetModelsByProvider)
{
    auto& manager = arbiterAI::ModelManager::instance();
    auto models = manager.getModels("llama");
    
    EXPECT_FALSE(models.empty());
    for (const auto& model : models) {
        EXPECT_EQ(model.provider, "llama");
    }
}

TEST_F(ModelManagerTest, AddModel)
{
    auto& manager = arbiterAI::ModelManager::instance();
    
    arbiterAI::ModelInfo newModel{
        .model = "test-model",
        .provider = "test-provider",
        .mode = "chat",
        .contextWindow = 4096
    };
    
    manager.addModel(newModel);
    auto modelInfo = manager.getModelInfo("test-model");
    EXPECT_TRUE(modelInfo.has_value());
}

TEST_F(ModelManagerTest, SchemaVersionValidation)
{
    auto& manager = arbiterAI::ModelManager::instance();
    
    // Test valid version
    arbiterAI::ModelInfo validModel{
        .model = "valid-model",
        .provider = "test",
        .configVersion = "1.1.0",
        .minSchemaVersion = "1.0.0"
    };
    manager.addModel(validModel);
    EXPECT_TRUE(manager.getModelInfo("valid-model").has_value());

    // Test incompatible version
    arbiterAI::ModelInfo invalidModel{
        .model = "invalid-model",
        .provider = "test",
        .configVersion = "1.0.0",
        .minSchemaVersion = "1.2.0" // Requires newer schema than available
    };
    manager.addModel(invalidModel);
    EXPECT_FALSE(manager.getModelInfo("invalid-model").has_value());
}

TEST_F(ModelManagerTest, ModelRanking)
{
    auto& manager = arbiterAI::ModelManager::instance();
    
    arbiterAI::ModelInfo highRankModel{
        .model = "high-rank",
        .provider = "test",
        .ranking = 90
    };
    
    arbiterAI::ModelInfo lowRankModel{
        .model = "low-rank",
        .provider = "test",
        .ranking = 10
    };
    
    manager.addModel(highRankModel);
    manager.addModel(lowRankModel);
    
    auto rankedModels = manager.getModelsByRanking();
    ASSERT_GE(rankedModels.size(), 2);
    EXPECT_EQ(rankedModels[0].model, "high-rank");
    EXPECT_EQ(rankedModels[1].model, "low-rank");
}

TEST_F(ModelManagerTest, VersionComparison)
{
    EXPECT_EQ(arbiterAI::ModelManager::compareVersions("1.0.0", "1.0.0"), 0);
    EXPECT_GT(arbiterAI::ModelManager::compareVersions("1.1.0", "1.0.0"), 0);
    EXPECT_LT(arbiterAI::ModelManager::compareVersions("1.0.0", "1.1.0"), 0);
    EXPECT_GT(arbiterAI::ModelManager::compareVersions("2.0.0", "1.9.9"), 0);
    EXPECT_LT(arbiterAI::ModelManager::compareVersions("1.9.9", "2.0.0"), 0);
}