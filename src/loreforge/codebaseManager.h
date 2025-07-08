#ifndef _LOREFORGE_CODEBASEMANAGER_H_
#define _LOREFORGE_CODEBASEMANAGER_H_

#include "codeParser.h"
#include <hermesaxiom/hermesaxiom.h>

#include <string>
#include <vector>
#include <memory>
#include <map>

// Forward declaration for Faiss index
namespace faiss
{
struct Index;
}

namespace loreforge
{

class CodebaseManager
{
public:
    CodebaseManager(std::shared_ptr<CodeParser> parser, std::shared_ptr<hermesaxiom::hermesaxiom> hermes);
    ~CodebaseManager();

    void processDirectory(const std::string &directoryPath, const std::string &model);
    void saveIndex(const std::string &indexPath);
    bool loadIndex(const std::string &indexPath);
    std::vector<CodeChunk> search(const std::string &query, const std::string &model, int k);
    bool isReady();
    int getVectorCount();
    std::shared_ptr<CodeParser> getCodeParser();

private:
    void processFile(const std::string &filePath);
    void embedChunks(const std::vector<CodeChunk> &chunks);

    std::shared_ptr<hermesaxiom::hermesaxiom> m_embeddingModel;
    std::unique_ptr<faiss::Index> m_faissIndex;
    std::shared_ptr<CodeParser> m_codeParser;

    std::string m_embeddingModelName;
    std::vector<CodeChunk> m_codeChunks;
    std::map<long, int> m_faissIdToChunkIndex;
    int m_dimension;
    bool m_ready=false;
};

} // namespace loreforge

#endif // _LOREFORGE_CODEBASEMANAGER_H_