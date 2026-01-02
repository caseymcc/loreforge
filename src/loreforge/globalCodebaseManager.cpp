#include "loreforge/globalCodebaseManager.h"

#include <stdexcept>

namespace loreforge
{

GlobalCodebaseManager::GlobalCodebaseManager() : m_dimension(0) {}

GlobalCodebaseManager::~GlobalCodebaseManager() {}

void GlobalCodebaseManager::addCodebase(const std::string& name, std::unique_ptr<CodebaseManager> codebaseManager)
{
    if (m_codebaseManagers.empty())
    {
        m_dimension = codebaseManager->getDimension();
    }
    m_codebaseManagers[name] = std::move(codebaseManager);
    reindex();
}

void GlobalCodebaseManager::removeCodebase(const std::string& name)
{
    m_codebaseManagers.erase(name);
    reindex();
}

CodebaseManager* GlobalCodebaseManager::getCodebase(const std::string& name)
{
    auto it = m_codebaseManagers.find(name);
    if (it != m_codebaseManagers.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::vector<CodeChunk> GlobalCodebaseManager::search(const std::string& query, const std::string& model, int k)
{
    if (m_codebaseManagers.empty())
    {
        return {};
    }

    // For now, we just use the first codebase's embedding model.
    auto& firstCodebase = m_codebaseManagers.begin()->second;
    arbiterAI::EmbeddingRequest request;
    request.model = model;
    request.input = query;
    arbiterAI::EmbeddingResponse response;
    firstCodebase->getCodeParser()->getEmbeddingModel()->getEmbeddings(request, response);
    auto embedding = response.data.empty() ? std::vector<float>() : response.data[0].embedding;

    if (embedding.empty())
    {
        return {};
    }

    std::vector<faiss::idx_t> labels(k);
    std::vector<float> distances(k);

    m_faissIndex->search(1, embedding.data(), k, distances.data(), labels.data());

    std::vector<CodeChunk> results;
    for (size_t i = 0; i < labels.size(); ++i) {
        faiss::idx_t label = labels[i];
        if (label < 0) continue;
        const auto& index = m_chunkFileIndexes[label];
        auto codebase = getCodebase(index.codebaseName);
        if (codebase)
        {
            const auto& codeFiles = codebase->getCodeFiles();
            if (index.fileIndex < codeFiles.size())
            {
                const auto& codeFile = codeFiles[index.fileIndex];
                if (index.chunkIndex < codeFile.chunks.size())
                {
                    auto chunk = codeFile.chunks[index.chunkIndex];
                    chunk.score = distances[i];
                    results.push_back(chunk);
                }
            }
        }
    }

    return results;
}

std::vector<CodeChunk> GlobalCodebaseManager::searchInCodebase(const std::string& name, const std::string& query, const std::string& model, int k)
{
    auto codebase = getCodebase(name);
    if (codebase)
    {
        return codebase->search(query, model, k);
    }
    return {};
}

void GlobalCodebaseManager::reindex()
{
    if (m_codebaseManagers.empty())
    {
        m_faissIndex.reset();
        m_faissBaseIndex.reset();
        m_chunkFileIndexes.clear();
        return;
    }

    if (m_dimension == 0)
    {
        for (const auto& pair : m_codebaseManagers)
        {
            if (pair.second->getDimension() > 0)
            {
                m_dimension = pair.second->getDimension();
                break;
            }
        }
    }

    if (m_dimension == 0) return;

    m_faissBaseIndex = std::make_unique<faiss::IndexFlatL2>(m_dimension);
    m_faissIndex = std::make_unique<faiss::IndexIDMap>(m_faissBaseIndex.get());

    m_chunkFileIndexes.clear();
    faiss::idx_t currentId = 0;

    for (const auto& pair : m_codebaseManagers)
    {
        const auto& codebaseName = pair.first;
        const auto& codebase = pair.second;
        const auto& codeFiles = codebase->getCodeFiles();
        for (size_t fileIndex = 0; fileIndex < codeFiles.size(); ++fileIndex)
        {
            const auto& codeFile = codeFiles[fileIndex];
            for (size_t chunkIndex = 0; chunkIndex < codeFile.chunks.size(); ++chunkIndex)
            {
                const auto& chunk = codeFile.chunks[chunkIndex];
                if (chunk.faissId != -1)
                {
                    auto embedding = codebase->getEmbedding(chunk.faissId);
                    if (!embedding.empty())
                    {
                        m_faissIndex->add_with_ids(1, embedding.data(), &currentId);
                        m_chunkFileIndexes.push_back({codebaseName, fileIndex, chunkIndex});
                        currentId++;
                    }
                }
            }
        }
    }
}

} // namespace loreforge