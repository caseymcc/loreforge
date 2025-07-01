#include "loreforge/codebaseManager.h"

#include <faiss/IndexFlat.h>

#include <filesystem>
#include <iostream>

namespace loreforge
{

CodebaseManager::CodebaseManager(
    std::shared_ptr<CodeParser> parser,
    std::shared_ptr<hermesaxiom::hermesaxiom> hermes_axiom
) : m_parser(parser), m_hermesAxiom(hermes_axiom)
{
}

CodebaseManager::~CodebaseManager()
{
    if(m_indexFuture.valid())
    {
        m_indexFuture.wait();
    }
}

bool CodebaseManager::isReady() const
{
    return m_isReady.load();
}

void CodebaseManager::doIndexing(const std::string &path)
{
    try
    {
        std::vector<std::string> files;
        for(const auto &entry:std::filesystem::recursive_directory_iterator(path))
        {
            if(entry.is_regular_file())
            {
                files.push_back(entry.path().string());
            }
        }

        m_allChunks.clear();
        for(const auto &file:files)
        {
            auto chunks=m_parser->parse(file);
            m_allChunks.insert(m_allChunks.end(), chunks.begin(), chunks.end());
        }

        if(m_allChunks.empty())
        {
            std::cerr<<"No code chunks found to index."<<std::endl;
            m_isReady=true;
            return;
        }

        std::vector<std::string> texts_to_embed;
        for(const auto &chunk:m_allChunks)
        {
            texts_to_embed.push_back(chunk.content);
        }

        hermesaxiom::EmbeddingRequest request;
        request.input=texts_to_embed;
        hermesaxiom::EmbeddingResponse response;
        if(m_hermesAxiom->getEmbeddings(request, response)!=hermesaxiom::ErrorCode::Success)
        {
            std::cerr<<"Failed to generate embeddings."<<std::endl;
            m_isReady=true;
            return;
        }

        std::vector<std::vector<float>> embeddings;
        embeddings.reserve(response.data.size());
        for(const auto &emb:response.data)
        {
            embeddings.push_back(emb.embedding);
        }

        if(embeddings.empty())
        {
            std::cerr<<"No embeddings generated."<<std::endl;
            m_isReady=true;
            return;
        }

        int dimension=embeddings[0].size();
        m_faissIndex=std::make_unique<faiss::IndexFlatL2>(dimension);

        std::vector<float> flat_embeddings;
        flat_embeddings.reserve(embeddings.size()*dimension);
        for(const auto &vec:embeddings)
        {
            flat_embeddings.insert(flat_embeddings.end(), vec.begin(), vec.end());
        }

        m_faissIndex->add(embeddings.size(), flat_embeddings.data());

        m_isReady=true;
        std::cout<<"Indexing complete. Codebase is ready."<<std::endl;

    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error during indexing: "<<e.what()<<std::endl;
        m_isReady=true; // Mark as ready to avoid blocking, even on failure
    }
}

void CodebaseManager::indexDirectory(const std::string &path)
{
    if(m_indexFuture.valid()&&m_indexFuture.wait_for(std::chrono::seconds(0))!=std::future_status::ready)
    {
        std::cout<<"Indexing is already in progress."<<std::endl;
        return;
    }
    m_isReady=false;
    m_indexFuture=std::async(std::launch::async, &CodebaseManager::doIndexing, this, path);
}

std::vector<CodeChunk> CodebaseManager::query(const std::string &query_text, int k)
{
    if(!isReady())
    {
        return {};
    }

    if(!m_faissIndex||m_faissIndex->ntotal==0)
    {
        return {};
    }

    hermesaxiom::EmbeddingRequest request;
    request.input=query_text;
    hermesaxiom::EmbeddingResponse response;
    if(m_hermesAxiom->getEmbeddings(request, response)!=hermesaxiom::ErrorCode::Success||response.data.empty())
    {
        return {};
    }

    std::vector<float> flatAueryEmbedding=response.data[0].embedding;
    std::vector<faiss::idx_t> indices(k);
    std::vector<float> distances(k);

    m_faissIndex->search(1, flatAueryEmbedding.data(), k, distances.data(), indices.data());

    std::vector<CodeChunk> results;
    for(faiss::idx_t i:indices)
    {
        if(i>=0&&i<m_allChunks.size())
        {
            results.push_back(m_allChunks[i]);
        }
    }

    return results;
}

} // namespace loreforge