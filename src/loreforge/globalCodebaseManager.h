#ifndef _LOREFORGE_GLOBALCODEBASEMANAGER_H_
#define _LOREFORGE_GLOBALCODEBASEMANAGER_H_

#include "loreforge/codebaseManager.h"
#include "loreforge/codeFile.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <faiss/IndexIDMap.h>
#include <faiss/IndexFlat.h>

namespace loreforge
{

class GlobalCodebaseManager
{
public:
    GlobalCodebaseManager();
    ~GlobalCodebaseManager();

    void addCodebase(const std::string& name, std::unique_ptr<CodebaseManager> codebaseManager);
    void removeCodebase(const std::string& name);
    CodebaseManager* getCodebase(const std::string& name);
    std::vector<CodeChunk> search(const std::string& query, const std::string& model, int k);
    std::vector<CodeChunk> searchInCodebase(const std::string& name, const std::string& query, const std::string& model, int k);
    void reindex();

private:
    std::map<std::string, std::unique_ptr<CodebaseManager>> m_codebaseManagers;
    std::unique_ptr<faiss::IndexFlatL2> m_faissBaseIndex;
    std::unique_ptr<faiss::IndexIDMap> m_faissIndex;
    std::vector<GlobalChunkIndex> m_chunkFileIndexes;
    int m_dimension;
};

} // namespace loreforge

#endif // _LOREFORGE_GLOBALCODEBASEMANAGER_H_