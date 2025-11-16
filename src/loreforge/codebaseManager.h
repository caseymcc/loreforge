#ifndef _LOREFORGE_CODEBASEMANAGER_H_
#define _LOREFORGE_CODEBASEMANAGER_H_

#include "loreforge/codeParser.h"
#include "loreforge/codeFile.h"
#include <arbiterAI/arbiterAI.h>

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <tuple>
#include <chrono>

#include <faiss/IndexIDMap.h>
#include <faiss/IndexFlat.h>


namespace loreforge
{

class CodebaseManager
{
public:
    CodebaseManager();
    ~CodebaseManager();

    bool init(std::shared_ptr<CodeParser> parser, std::shared_ptr<arbiterAI::ArbiterAI> arbiter,
        const std::string &codebasePath, const std::string &model);

    void generateEmbeddings();
    void saveIndex(const std::string &indexPath);
    bool loadIndex(const std::string &indexPath);
    void updateCodebase();
    std::vector<CodeChunk> search(const std::string &query, const std::string &model, int k);
    bool isReady();
    int getVectorCount();
    std::shared_ptr<CodeParser> getCodeParser();
    int getDimension();
    const std::vector<CodeFile>& getCodeFiles() const;
    std::vector<float> getEmbedding(faiss::idx_t faissId);

private:
    faiss::idx_t getNextFaissId();
    void retireFaissId(faiss::idx_t id);

    void handleFileModification(const std::string& filePath, std::vector<CodeChunk>& oldChunks, std::vector<CodeChunk>& newChunks);
    void handleFileAddition(const std::string& filePath, std::vector<CodeChunk>& newChunks);
    void handleFileDeletion(const std::string& filePath, std::vector<CodeChunk>& deletedChunks);

    std::shared_ptr<arbiterAI::ArbiterAI> m_embeddingModel;
    std::string m_embeddingModelName;

    std::unique_ptr<faiss::IndexFlatL2> m_faissBaseIndex;
    std::unique_ptr<faiss::IndexIDMap> m_faissIndex;
    std::vector<faiss::idx_t> m_retiredFaissIds;
    faiss::idx_t m_nextFaissId;
    
    std::shared_ptr<CodeParser> m_codeParser;

    std::string m_codebasePath;
    std::map<std::string, size_t> m_codeFilesMap;
    std::vector<CodeFile> m_codeFiles;
    std::vector<ChunkFileIndex> m_chunkFileIndexes;
    std::map<faiss::idx_t, std::vector<float>> m_embeddings;

    int m_dimension;
    bool m_ready=false;
};

} // namespace loreforge

#endif // _LOREFORGE_CODEBASEMANAGER_H_