#include "loreforge/codeParser.h"

#include <tree_sitter/api.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iomanip>

namespace loreforge
{

extern "C" {
    TSLanguage *tree_sitter_cpp();
    TSLanguage *tree_sitter_python();
    TSLanguage *tree_sitter_javascript();
    TSLanguage *tree_sitter_typescript();
}

CodeParser::CodeParser() : m_parser(ts_parser_new(), ts_parser_delete) {}

CodeParser::~CodeParser()=default;

std::vector<CodeChunk> CodeParser::parse(const std::string &filePath)
{
    std::ifstream file(filePath);
    if(!file.is_open())
    {
        return {};
    }

    std::stringstream buffer;
    buffer<<file.rdbuf();
    return parse(filePath, buffer.str());
}

std::vector<CodeChunk> CodeParser::parse(const std::string &fileName, const std::string &content)
{
    std::vector<CodeChunk> chunks;
    TSLanguage *language=getLanguage(fileName);

    if(language)
    {
        findChunks(content, language, chunks, fileName);
    }

    if(chunks.empty())
    {
        // Add the whole file as a single chunk
        CodeChunk fileChunk;
        fileChunk.filePath=fileName;
        fileChunk.startLine=1;
        fileChunk.endLine=0;
        fileChunk.content=content;
        fileChunk.identifier="file";
        fileChunk.type="file";

        std::string chunkIdSource=fileChunk.filePath+":"+std::to_string(fileChunk.startLine)+":"+std::to_string(fileChunk.endLine)+":"+fileChunk.identifier;
        EVP_MD_CTX *mdctx=EVP_MD_CTX_new();
        const EVP_MD *md=EVP_sha256();
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len;
        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, chunkIdSource.c_str(), chunkIdSource.size());
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
        EVP_MD_CTX_free(mdctx);
        std::stringstream ss;
        for(unsigned int i=0; i<hash_len; i++)
        {
            ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i];
        }
        fileChunk.chunkId=ss.str();

        chunks.push_back(fileChunk);
    }

    return chunks;
}

void CodeParser::setEmbeddingModel(std::shared_ptr<arbiterAI::ArbiterAI> model)
{
    m_embeddingModel=model;
}

std::shared_ptr<arbiterAI::ArbiterAI> CodeParser::getEmbeddingModel()
{
    return m_embeddingModel;
}

void CodeParser::findChunks(const std::string &source_code, TSLanguage *language, std::vector<CodeChunk> &chunks, const std::string &filePath)
{
    if(!ts_parser_set_language(m_parser.get(), language))
    {
        return;
    }

    TSTree *tree=ts_parser_parse_string(m_parser.get(), nullptr, source_code.c_str(), source_code.length());
    if(!tree)
    {
        return;
    }

    TSNode root_node=ts_tree_root_node(tree);
    if(ts_node_has_error(root_node))
    {
        ts_tree_delete(tree);
        return;
    }

    const char *query_string;
    if(strcmp(ts_language_name(language), "python")==0)
    {
        query_string="[(function_definition) @function (class_definition) @class]";
    }
    else
    {
        query_string="[(function_definition) @function (class_specifier) @class]";
    }
    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query=ts_query_new(language, query_string, strlen(query_string), &error_offset, &error_type);

    if(error_type!=TSQueryErrorNone)
    {
        ts_query_delete(query);
        ts_tree_delete(tree);
        return;
    }

    TSQueryCursor *cursor=ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, root_node);

    TSQueryMatch match;
    while(ts_query_cursor_next_match(cursor, &match))
    {
        for(uint32_t i=0; i<match.capture_count; ++i)
        {
            TSNode captured_node=match.captures[i].node;
            uint32_t start_byte=ts_node_start_byte(captured_node);
            uint32_t end_byte=ts_node_end_byte(captured_node);

            CodeChunk chunk;
            chunk.filePath=filePath;
            chunk.startLine=ts_node_start_point(captured_node).row+1;
            chunk.endLine=ts_node_end_point(captured_node).row+1;
            chunk.content=source_code.substr(start_byte, end_byte-start_byte);
            chunk.type=ts_node_type(captured_node);

            // Attempt to find an identifier
            TSNode nameNode=ts_node_child_by_field_name(captured_node, "name", 4);
            if(!ts_node_is_null(nameNode))
            {
                uint32_t name_start_byte=ts_node_start_byte(nameNode);
                uint32_t name_end_byte=ts_node_end_byte(nameNode);
                chunk.identifier=source_code.substr(name_start_byte, name_end_byte-name_start_byte);
            }
            else
            {
                chunk.identifier="unnamed";
            }

            std::string chunkIdSource=chunk.filePath+":"+std::to_string(chunk.startLine)+":"+std::to_string(chunk.endLine)+":"+chunk.identifier;
            EVP_MD_CTX *mdctx=EVP_MD_CTX_new();
            const EVP_MD *md=EVP_sha256();
            unsigned char hash[EVP_MAX_MD_SIZE];
            unsigned int hash_len;
            EVP_DigestInit_ex(mdctx, md, NULL);
            EVP_DigestUpdate(mdctx, chunkIdSource.c_str(), chunkIdSource.size());
            EVP_DigestFinal_ex(mdctx, hash, &hash_len);
            EVP_MD_CTX_free(mdctx);
            std::stringstream ss;
            for(unsigned int i=0; i<hash_len; i++)
            {
                ss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i];
            }
            chunk.chunkId=ss.str();

            chunks.push_back(chunk);
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
}

TSLanguage *CodeParser::getLanguage(const std::string &filePath)
{
    if(filePath.ends_with(".cpp")||filePath.ends_with(".h"))
        return tree_sitter_cpp();
    else if(filePath.ends_with(".py"))
        return tree_sitter_python();
    else if(filePath.ends_with(".js"))
        return tree_sitter_javascript();
    else if(filePath.ends_with(".ts"))
        return tree_sitter_typescript();
    else
        return nullptr;
}

} // namespace loreforge