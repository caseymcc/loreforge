#include "codebaseManager.h"

#include <faiss/index_io.h>
#include <faiss/IndexFlat.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace loreforge {

namespace fs = std::filesystem;

CodebaseManager::CodebaseManager(std::shared_ptr<CodeParser> parser, std::shared_ptr<arbiterAI::arbiterAI> arbiter)
    : m_codeParser(parser), m_embeddingModel(arbiter) {
    m_dimension = 768; // This should be dynamically determined from the model
    m_faissIndex = std::make_unique<faiss::IndexFlatL2>(m_dimension);
}

CodebaseManager::~CodebaseManager() = default;

void CodebaseManager::processFile(const std::string& filePath) {
    auto chunks = m_codeParser->parse(filePath);
    if (!chunks.empty()) {
        embedChunks(chunks);
    }
}

void CodebaseManager::processDirectory(const std::string& directoryPath, const std::string& model) {
    m_embeddingModelName = model;
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            processFile(entry.path().string());
        }
    }
    m_ready = true;
}

void CodebaseManager::embedChunks(const std::vector<CodeChunk>& chunks) {
    arbiterAI::EmbeddingRequest request;
    request.model = m_embeddingModelName;

    std::vector<std::string> contents;
    for (const auto& chunk : chunks) {
        contents.push_back(chunk.content);
    }
    request.input = contents;

    arbiterAI::EmbeddingResponse response;
    arbiterAI::ErrorCode status = m_embeddingModel->getEmbeddings(request, response);
    if (status == arbiterAI::ErrorCode::Success) {
        for (const auto& embeddingData : response.data) {
            int newChunkIndex = m_codeChunks.size();
            m_codeChunks.push_back(chunks[embeddingData.index]);

            long id = m_faissIndex->ntotal;
            m_faissIndex->add(1, embeddingData.embedding.data());
            m_faissIdToChunkIndex[id] = newChunkIndex;
        }
    } else {
        if (!chunks.empty()) {
            std::cerr << "Failed to get embeddings for " << chunks[0].filePath << " with error code " << static_cast<int>(status) << std::endl;
        }
    }
}

void CodebaseManager::saveIndex(const std::string& indexPath) {
    faiss::write_index(m_faissIndex.get(), indexPath.c_str());
    std::ofstream mapFile(indexPath + ".map");
    for (const auto& pair : m_faissIdToChunkIndex) {
        mapFile << pair.first << " " << pair.second << "\n";
    }
}

bool CodebaseManager::loadIndex(const std::string& indexPath) {
    try {
        m_faissIndex.reset(faiss::read_index(indexPath.c_str()));
        std::ifstream mapFile(indexPath + ".map");
        long faiss_id;
        int chunk_index;
        m_faissIdToChunkIndex.clear();
        while (mapFile >> faiss_id >> chunk_index) {
            m_faissIdToChunkIndex[faiss_id] = chunk_index;
        }
        m_ready = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading Faiss index: " << e.what() << std::endl;
        return false;
    }
}

bool CodebaseManager::isReady() {
    return m_ready;
}

int CodebaseManager::getVectorCount() {
    return m_faissIndex->ntotal;
}

std::shared_ptr<CodeParser> CodebaseManager::getCodeParser() {
    return m_codeParser;
}

std::vector<CodeChunk> CodebaseManager::search(const std::string& query, const std::string& model, int k) {
    std::vector<CodeChunk> results;

    arbiterAI::EmbeddingRequest request;
    request.model = model;
    request.input = query;

    arbiterAI::EmbeddingResponse response;
    if (m_embeddingModel->getEmbeddings(request, response) != arbiterAI::ErrorCode::Success || response.data.empty()) {
        std::cerr << "Failed to get query embedding" << std::endl;
        return results;
    }

    std::vector<long> labels(k);
    std::vector<float> distances(k);

    m_faissIndex->search(1, response.data[0].embedding.data(), k, distances.data(), labels.data());

    for (long label : labels) {
        if (label >= 0 && m_faissIdToChunkIndex.count(label)) {
            results.push_back(m_codeChunks[m_faissIdToChunkIndex[label]]);
        }
    }

    return results;
}

} // namespace loreforge