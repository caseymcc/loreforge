#ifndef _loreforge_codebaseManager_h
#define _loreforge_codebaseManager_h

#include "loreforge/codeParser.h"
#include "hermesaxiom/hermesaxiom.h"
#include <string>
#include <vector>
#include <memory>
#include <future>
#include <atomic>

// Forward declaration for Faiss index
namespace faiss { struct Index; }

namespace loreforge
{

class CodebaseManager
{
public:
    CodebaseManager(
        std::shared_ptr<CodeParser> parser,
        std::shared_ptr<hermesaxiom::hermesaxiom> hermes_axiom
    );
    ~CodebaseManager();

    void indexDirectory(const std::string &path);
    std::vector<CodeChunk> query(const std::string &query_text, int k=5);
    bool isReady() const;

private:
    void doIndexing(const std::string &path);

    std::shared_ptr<CodeParser> m_parser;
    std::shared_ptr<hermesaxiom::hermesaxiom> m_hermesAxiom;

    std::vector<CodeChunk> m_allChunks;
    std::unique_ptr<faiss::Index> m_faissIndex;
    std::atomic<bool> m_isReady{ false };
    std::future<void> m_indexFuture;
};

} // namespace loreforge

#endif // _loreforge_codebaseManager_h