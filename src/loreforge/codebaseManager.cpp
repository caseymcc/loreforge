#include "loreforge/codebaseManager.h"
#include "loreforge/codeFile.h"

#include <faiss/index_io.h>
#include <faiss/IndexFlat.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <set>

namespace loreforge
{

namespace fs=std::filesystem;

CodebaseManager::CodebaseManager() :
    m_nextFaissId(0)
{
}

CodebaseManager::~CodebaseManager()=default;

bool CodebaseManager::init(std::shared_ptr<CodeParser> parser, std::shared_ptr<arbiterAI::ArbiterAI> arbiter,
    const std::string &codebasePath, const std::string &model)
{
    m_embeddingModel=arbiter;
    m_embeddingModelName=model;
    m_codeParser=parser;
    m_codeParser->setEmbeddingModel(arbiter);
    m_codebasePath=codebasePath;

    m_dimension=768; // This should be dynamically determined from the model

    m_faissBaseIndex.reset(new faiss::IndexFlatL2(m_dimension));
    m_faissIndex.reset(new faiss::IndexIDMap(m_faissBaseIndex.get()));

    if(m_codebasePath.empty()||m_embeddingModelName.empty())
    {
        return false;
    }

    fs::path cacheDir=fs::path(m_codebasePath)/".loreforge/cache";
    fs::create_directories(cacheDir);

    std::string sanitizedModelName=m_embeddingModelName;
    std::replace(sanitizedModelName.begin(), sanitizedModelName.end(), '/', '_');

    fs::path indexPath=cacheDir/(sanitizedModelName+".faiss");
    std::string indexFile=indexPath.string();

    if(fs::exists(indexFile)&&loadIndex(indexFile))
    {
        std::cout<<"Loaded index from cache: "<<indexFile<<std::endl;
    }
    else
    {
        std::cout<<"Generating new embeddings for codebase at "<<m_codebasePath<<std::endl;
        generateEmbeddings();
        std::cout<<"Saving index to cache: "<<indexFile<<std::endl;
        saveIndex(indexFile);
    }

    return true;
}

void CodebaseManager::generateEmbeddings()
{
    if(m_codebasePath.empty()||!fs::exists(m_codebasePath)||!fs::is_directory(m_codebasePath))
    {
        std::cerr<<"Codebase path is empty, does not exist, or is not a directory."<<std::endl;
        return;
    }

    m_faissIndex->reset();

    for(const auto &entry:fs::recursive_directory_iterator(m_codebasePath))
    {
        if(!entry.is_regular_file())
            continue;

        const auto &path=entry.path();
        const std::string pathStr=path.string();
        auto lastWriteTime=fs::last_write_time(path);
        auto systemTime=std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWriteTime-fs::file_time_type::clock::now()+std::chrono::system_clock::now());
        auto it=m_codeFilesMap.find(pathStr);
        
        if(it != m_codeFilesMap.end()) 
        {
            if(m_codeFiles[it->second].lastParsed >= systemTime)
                continue; // Skip file if not modified
        }

        auto chunks=m_codeParser->parse(pathStr);

        if(!chunks.empty())
        {
            m_codeFiles.emplace_back(pathStr, systemTime, chunks);
            m_codeFilesMap[pathStr] = m_codeFiles.size() - 1;
        }
    }

    // Rebuild the flat chunk list and FAISS index
    for(auto &codeFile:m_codeFiles)
    {
        for(auto &chunk:codeFile.chunks)
        {
            arbiterAI::EmbeddingRequest request;

            request.model=m_embeddingModelName;
            request.input=chunk.content;

            arbiterAI::EmbeddingResponse response;

            if(m_embeddingModel->getEmbeddings(request, response)==arbiterAI::ErrorCode::Success&&!response.data.empty())
            {
                m_faissIndex->add(1, response.data[0].embedding.data());

                chunk.faissId=getNextFaissId();
                m_embeddings[chunk.faissId] = response.data[0].embedding;
            }
            else
                std::cerr<<"Failed to create embedding for a chunk in "<<chunk.filePath<<std::endl;
        }
    }

    m_ready=true;
}

void CodebaseManager::saveIndex(const std::string &indexPath)
{
    faiss::write_index(m_faissIndex.get(), indexPath.c_str());

    nlohmann::json metadata;
    metadata["code_files"]=nlohmann::json::object();
    for(const auto &codeFile:m_codeFiles)
    {
        nlohmann::json fileJson;
        fileJson["last_parsed"]=std::chrono::system_clock::to_time_t(codeFile.lastParsed);
        fileJson["chunks"]=nlohmann::json::array();
        for(const auto &chunk:codeFile.chunks)
        {
            nlohmann::json chunkJson;
            chunkJson["start_line"]=chunk.startLine;
            chunkJson["end_line"]=chunk.endLine;
            chunkJson["content"]=chunk.content;
            chunkJson["identifier"]=chunk.identifier;
            chunkJson["type"]=chunk.type;
            chunkJson["chunk_id"]=chunk.chunkId;
            chunkJson["faiss_id"]=chunk.faissId;
            fileJson["chunks"].push_back(chunkJson);
        }
        metadata["code_files"][codeFile.filePath] = fileJson;
    }

   metadata["embeddings"] = m_embeddings;

    std::ofstream mapFile(indexPath+".map");
    mapFile<<metadata.dump(4);
}

bool CodebaseManager::loadIndex(const std::string &indexPath)
{
    try
    {
        std::string mapFilePath=indexPath+".map";

        if(!fs::exists(mapFilePath))
        {
            return false;
        }

        m_faissIndex.reset(dynamic_cast<faiss::IndexIDMap *>(faiss::read_index(indexPath.c_str())));

        std::ifstream mapFile(mapFilePath);
        if(!mapFile.is_open())
        {
            return false;
        }

        nlohmann::json metadata;
        mapFile>>metadata;

        m_codeFiles.clear();

        for(auto it=metadata["code_files"].begin(); it!=metadata["code_files"].end(); ++it)
        {
            std::string filePath=it.key();
            nlohmann::json fileJson=it.value();

            CodeFile codeFile;

            codeFile.filePath=filePath;
            codeFile.lastParsed=std::chrono::system_clock::from_time_t(fileJson["last_parsed"]);

            for(const auto &chunkJson:fileJson["chunks"])
            {
                CodeChunk chunk;

                chunk.filePath=filePath;
                chunk.startLine=chunkJson["start_line"];
                chunk.endLine=chunkJson["end_line"];
                chunk.content=chunkJson["content"];
                chunk.identifier=chunkJson["identifier"];
                chunk.type=chunkJson["type"];
                chunk.chunkId=chunkJson["chunk_id"];
                chunk.faissId=chunkJson["faiss_id"];

                codeFile.chunks.push_back(chunk);
            }
            m_codeFiles.push_back(codeFile);
            m_codeFilesMap[filePath] = m_codeFiles.size() - 1;
        }

       if (metadata.contains("embeddings"))
       {
           metadata.at("embeddings").get_to(m_embeddings);
       }

        m_ready=true;
        return true;
    }
    catch(const std::exception &e)
    {
        std::cerr<<"Error loading Faiss index: "<<e.what()<<std::endl;
        return false;
    }
}

bool CodebaseManager::isReady()
{
    return m_ready;
}

int CodebaseManager::getVectorCount()
{
    return m_faissIndex->ntotal;
}

std::shared_ptr<CodeParser> CodebaseManager::getCodeParser()
{
    return m_codeParser;
}

int CodebaseManager::getDimension()
{
    return m_dimension;
}

const std::vector<CodeFile>& CodebaseManager::getCodeFiles() const
{
    return m_codeFiles;
}

std::vector<float> CodebaseManager::getEmbedding(faiss::idx_t faissId)
{
    auto it = m_embeddings.find(faissId);
    if (it != m_embeddings.end())
    {
        return it->second;
    }
    return {};
}

faiss::idx_t CodebaseManager::getNextFaissId()
{
    if(!m_retiredFaissIds.empty())
    {
        faiss::idx_t id=m_retiredFaissIds.back();

        m_retiredFaissIds.pop_back();
        return id;
    }
    return m_nextFaissId++;
}

void CodebaseManager::retireFaissId(faiss::idx_t id)
{
    m_retiredFaissIds.push_back(id);
}

std::vector<CodeChunk> CodebaseManager::search(const std::string &query, const std::string &model, int k)
{
    std::vector<CodeChunk> results;

    arbiterAI::EmbeddingRequest request;
    
    request.model=model;
    request.input=query;

    arbiterAI::EmbeddingResponse response;

    if(m_embeddingModel->getEmbeddings(request, response)!=arbiterAI::ErrorCode::Success||response.data.empty())
    {
        std::cerr<<"Failed to get query embedding"<<std::endl;
        return results;
    }

    std::vector<faiss::idx_t> labels(k);
    std::vector<float> distances(k);

    m_faissIndex->search(1, response.data[0].embedding.data(), k, distances.data(), labels.data());

    for (size_t i = 0; i < labels.size(); ++i)
    {
        faiss::idx_t label = labels[i];
        
        if((label < 0) || (label >= m_chunkFileIndexes.size()))
            continue;

        if((m_chunkFileIndexes[label].fileIndex < 0) || (m_chunkFileIndexes[label].fileIndex >= m_codeFiles.size()))
            continue;

        CodeFile &file=m_codeFiles[m_chunkFileIndexes[label].fileIndex];

        if((m_chunkFileIndexes[label].chunkIndex < 0) || (m_chunkFileIndexes[label].chunkIndex >= file.chunks.size()))
            continue;
        
        CodeChunk chunk=file.chunks[m_chunkFileIndexes[label].chunkIndex];
        chunk.score = distances[i];

        results.push_back(chunk);
    }

    return results;
}

void CodebaseManager::updateCodebase()
{
    if(m_codebasePath.empty()||!fs::exists(m_codebasePath)||!fs::is_directory(m_codebasePath))
    {
        std::cerr<<"Codebase path is empty, does not exist, or is not a directory."<<std::endl;
        return;
    }

    std::set<std::string> currentFiles;

    for(const auto &entry:fs::recursive_directory_iterator(m_codebasePath))
    {
        if(entry.is_regular_file())
        {
            currentFiles.insert(entry.path().string());
        }
    }

    // Identify deleted files
    std::vector<std::string> deletedFiles;
    for (const auto& codeFile : m_codeFiles)
    {
        if (currentFiles.find(codeFile.filePath) == currentFiles.end())
        {
            deletedFiles.push_back(codeFile.filePath);
        }
    }

    for(const auto &filePath:deletedFiles)
    {
        auto it = m_codeFilesMap.find(filePath);
        if (it != m_codeFilesMap.end())
        {
            size_t index = it->second;
            handleFileDeletion(filePath, m_codeFiles[index].chunks);
            m_codeFiles.erase(m_codeFiles.begin() + index);
            m_codeFilesMap.erase(it);
            // Update map for subsequent elements
            for (auto& pair : m_codeFilesMap)
            {
                if (pair.second > index)
                {
                    pair.second--;
                }
            }
            // Update chunk file indexes
            for (auto& chunkIndex : m_chunkFileIndexes)
            {
                if (chunkIndex.fileIndex == index)
                {
                    chunkIndex.fileIndex = -1; // Mark as invalid
                }
                else if (chunkIndex.fileIndex > index)
                {
                    chunkIndex.fileIndex--;
                }
            }
        }
    }

    // Identify new and modified files
    for(const auto &filePath:currentFiles)
    {
        auto lastWriteTime=fs::last_write_time(filePath);
        auto systemTime=std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWriteTime-fs::file_time_type::clock::now()+std::chrono::system_clock::now());

        auto it = m_codeFilesMap.find(filePath);
        if (it == m_codeFilesMap.end())
        {
            // New file
            auto newChunks=m_codeParser->parse(filePath);
            if(!newChunks.empty())
            {
                m_codeFiles.emplace_back(filePath, systemTime, newChunks);
                m_codeFilesMap[filePath] = m_codeFiles.size() - 1;
                handleFileAddition(filePath, m_codeFiles.back().chunks);
            }
        }
        else if (m_codeFiles[it->second].lastParsed < systemTime)
        {
            // Modified file
            auto oldChunks = m_codeFiles[it->second].chunks;
            auto newChunks = m_codeParser->parse(filePath);
            handleFileModification(filePath, oldChunks, newChunks);
            m_codeFiles[it->second].chunks = newChunks;
            m_codeFiles[it->second].lastParsed = systemTime;
        }
    }
}

void CodebaseManager::handleFileModification(const std::string &filePath, std::vector<CodeChunk> &oldChunks, std::vector<CodeChunk> &newChunks)
{
    std::set<std::string> oldChunkIds;
    for(const auto &chunk:oldChunks) oldChunkIds.insert(chunk.chunkId);

    std::set<std::string> newChunkIds;
    for(const auto &chunk:newChunks) newChunkIds.insert(chunk.chunkId);

    // Chunks to remove from FAISS
    std::vector<faiss::idx_t> idsToRemove;
    for(const auto &oldChunk:oldChunks)
    {
        if(newChunkIds.find(oldChunk.chunkId)==newChunkIds.end())
        {
            idsToRemove.push_back(oldChunk.faissId);
            retireFaissId(oldChunk.faissId);
        }
    }
    if(!idsToRemove.empty())
    {
        m_faissIndex->remove_ids(faiss::IDSelectorArray{idsToRemove.size(), idsToRemove.data()});
    }

    // Chunks to add to FAISS
    std::vector<CodeChunk> chunksToAdd;
    for(const auto &newChunk:newChunks)
    {
        if(oldChunkIds.find(newChunk.chunkId)==oldChunkIds.end())
        {
            chunksToAdd.push_back(newChunk);
        }
        else
        {
            // Content has changed
            // This logic needs to be improved. For now, we assume if a chunk exists, it's the same.
            // A more robust solution would compare content.
        }
    }

    if(!chunksToAdd.empty())
    {
        handleFileAddition(filePath, chunksToAdd);
    }
}

void CodebaseManager::handleFileAddition(const std::string &filePath, std::vector<CodeChunk> &newChunks)
{
    std::vector<float> embeddings;
    std::vector<faiss::idx_t> faissIds;
    auto it = m_codeFilesMap.find(filePath);
    
    if(it == m_codeFilesMap.end())
        return;
    
    size_t fileIndex = it->second;

    for(size_t i=0; i<newChunks.size(); ++i)
    {
        auto &chunk=newChunks[i];
        arbiterAI::EmbeddingRequest request;
        request.model = m_embeddingModelName;
        request.input = chunk.content;

        arbiterAI::EmbeddingResponse response;
        if (m_embeddingModel->getEmbeddings(request, response) == arbiterAI::ErrorCode::Success && !response.data.empty())
        {
            embeddings.insert(embeddings.end(), response.data[0].embedding.begin(), response.data[0].embedding.end());
            
            faiss::idx_t newId=getNextFaissId();

            chunk.faissId=newId;
            faissIds.push_back(newId);

            m_chunkFileIndexes.push_back({static_cast<size_t>(fileIndex), i});
        }
        else
        {
            std::cerr << "Failed to create embedding for a chunk in " << chunk.filePath << std::endl;
        }
    }

    if(!embeddings.empty())
    {
        m_faissIndex->add_with_ids(faissIds.size(), embeddings.data(), faissIds.data());
    }
}

void CodebaseManager::handleFileDeletion(const std::string &filePath, std::vector<CodeChunk> &deletedChunks)
{
    std::vector<faiss::idx_t> idsToRemove;
    for (const auto& chunk : deletedChunks)
    {
        idsToRemove.push_back(chunk.faissId);
        retireFaissId(chunk.faissId);
    }

    if(!idsToRemove.empty())
    {
        m_faissIndex->remove_ids(faiss::IDSelectorArray{idsToRemove.size(), idsToRemove.data()});
    }
}

} // namespace loreforge