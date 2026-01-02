#include "loreforge/codeParser.h"

#include <tree_sitter/api.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>
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

// Helper to check extension
static bool isPython(const std::string& path) {
    return path.ends_with(".py");
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

        // Don't return, try to parse what we can
    }

    uint32_t error_offset;
    TSQueryError error_type;
    const char *query_string;
    
    if(isPython(filePath))
    {
        query_string="[(function_definition) @function (class_definition) @class]";
    }
    else
    {
        // Assuming C++ for now as per previous logic
        query_string="[(function_definition) @function (class_specifier) @class]";
    }



    TSQuery *query=ts_query_new(language, query_string, strlen(query_string), &error_offset, &error_type);
    if(!query)
    {

        ts_query_delete(query);
        ts_tree_delete(tree);
        return;
    }



    TSQueryCursor *cursor=ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, root_node);
    

    TSQueryMatch match;
    int match_count = 0;
    while(ts_query_cursor_next_match(cursor, &match))
    {

        for(uint16_t i=0; i<match.capture_count; ++i)
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

CodeParser::FunctionSignature CodeParser::extractFunctionSignature(const std::string& filePath, const std::string& functionName) {
    FunctionSignature result;
    std::ifstream file(filePath);
    if (!file.is_open()) return result;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    TSLanguage* language = getLanguage(filePath);
    if (!language) return result;

    if (!ts_parser_set_language(m_parser.get(), language)) return result;

    TSTree* tree = ts_parser_parse_string(m_parser.get(), nullptr, content.c_str(), content.length());
    if (!tree) return result;

    TSNode root_node = ts_tree_root_node(tree);
    
    // Construct query based on language to find specific function
    std::string query_string;
    if (isPython(filePath)) {
        // Python query
        query_string = "(function_definition name: (identifier) @name parameters: (parameters) @params) @function";
    } else {
        // C++ query (simplified)
        query_string = "(function_definition type: (_) @type declarator: (function_declarator declarator: (identifier) @name parameters: (parameter_list) @params)) @function";
    }

    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery* query = ts_query_new(language, query_string.c_str(), query_string.length(), &error_offset, &error_type);
    
    if (!query) {
        ts_tree_delete(tree);
        return result;
    }

    TSQueryCursor* cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, root_node);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        std::string currentFuncName;
        std::string parameters;
        std::string returnType;
        std::string fullSignature;

        for (uint32_t i = 0; i < match.capture_count; ++i) {
            TSNode node = match.captures[i].node;
            uint32_t start = ts_node_start_byte(node);
            uint32_t end = ts_node_end_byte(node);
            std::string text = content.substr(start, end - start);
            
            uint32_t len;
            const char* capture_name = ts_query_capture_name_for_id(query, match.captures[i].index, &len);
            std::string capture(capture_name, len);

            if (capture == "name") currentFuncName = text;
            else if (capture == "params") parameters = text;
            else if (capture == "type") returnType = text;
            else if (capture == "function") {
                // Approximate signature from the definition line? 
                // Alternatively reconstruct it.
                // For now, let's just use the function name + params as signature if simple
                // Or capture the whole definition node line?
                // Use the text we found.
            }
        }


        if (currentFuncName == functionName) {
            result.found = true;
            result.signature = (returnType.empty() ? "" : returnType + " ") + currentFuncName + parameters;
            result.returnType = returnType;
            // Parse parameters string to vector (simplistic)
            result.parameters.push_back(parameters); 
            break;
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
    return result;
}

std::vector<std::string> CodeParser::findCallees(const std::string& filePath, const std::string& functionName) {
    std::vector<std::string> callees;
    // 1. Find the function definition first (reuse logic or copy)
    // 2. Inside the function body, search for call expressions.
    // implementation details ... 
    // For simplicity of this task, I will implement a basic version that scans the whole file for calls 
    // if I can't easily restrict to the function body with current structure.
    // BUT querying inside a specific node is possible with tree-sitter.
    
    std::ifstream file(filePath);
    if (!file.is_open()) return callees;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    TSLanguage* language = getLanguage(filePath);
    if (!language) return callees;

    if (!ts_parser_set_language(m_parser.get(), language)) return callees;

    TSTree* tree = ts_parser_parse_string(m_parser.get(), nullptr, content.c_str(), content.length());
    if (!tree) return callees;

    TSNode root_node = ts_tree_root_node(tree);

    // Locate the function definition node
    std::string def_query_str;
    if (isPython(filePath)) {
        def_query_str = "(function_definition name: (identifier) @name body: (block) @body) @function";
    } else {
        def_query_str = "(function_definition declarator: (function_declarator declarator: (identifier) @name) body: (compound_statement) @body) @function";
    }

    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery* def_query = ts_query_new(language, def_query_str.c_str(), def_query_str.length(), &error_offset, &error_type);
    if (!def_query) { ts_tree_delete(tree); return callees; }

    TSQueryCursor* cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, def_query, root_node);

    TSNode target_body_node;
    bool foundStr = false;
    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        std::string currentInfo;
        TSNode bodyNode;
        bool nameMatch = false;

        for (uint32_t i = 0; i < match.capture_count; ++i) {
             TSNode node = match.captures[i].node;
             uint32_t len;
             const char* name = ts_query_capture_name_for_id(def_query, match.captures[i].index, &len);
             if (strncmp(name, "name", len) == 0) {
                 uint32_t start = ts_node_start_byte(node);
                 uint32_t end = ts_node_end_byte(node);
                 if (content.substr(start, end - start) == functionName) nameMatch = true;
             } else if (strncmp(name, "body", len) == 0) {
                 bodyNode = node;
             }
        }
        if (nameMatch && !ts_node_is_null(bodyNode)) {
            target_body_node = bodyNode;
            foundStr = true;
            break;
        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(def_query);

    if (!foundStr) {
        ts_tree_delete(tree);
        return callees;
    }

    // Now query for calls inside target_body_node
    std::string call_query_str;
     if (isPython(filePath)) {
        call_query_str = "(call function: (identifier) @callee)";
    } else {
        call_query_str = "(call_expression function: (identifier) @callee)";
    }

    TSQuery* call_query = ts_query_new(language, call_query_str.c_str(), call_query_str.length(), &error_offset, &error_type);
    if (!call_query) { ts_tree_delete(tree); return callees; }

    cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, call_query, target_body_node);

    while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint32_t i = 0; i < match.capture_count; ++i) {
             TSNode node = match.captures[i].node;
             uint32_t start = ts_node_start_byte(node);
             uint32_t end = ts_node_end_byte(node);
             callees.push_back(content.substr(start, end - start));
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(call_query);
    ts_tree_delete(tree);
    return callees;
}

} // namespace loreforge