#include "loreforge/codeParser.h"

#include <tree_sitter/api.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>

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
        // Handle error: file not found or could not be opened
        return {};
    }
    
    std::stringstream buffer;
    
    buffer<<file.rdbuf();
    
    std::string sourceCode=buffer.str();

    TSLanguage *language=getLanguage(filePath);
    
    if(!language)
    {
        // Handle error: unsupported language
        return {};
    }

    std::vector<CodeChunk> chunks;

    findChunks(sourceCode, language, chunks);
    return chunks;
}

TSLanguage *CodeParser::getLanguage(const std::string &filePath)
{
    std::string extension=filePath.substr(filePath.find_last_of(".")+1);
    static const std::map<std::string, TSLanguage *> language_map={
        {"cpp", tree_sitter_cpp()},
        {"h", tree_sitter_cpp()},
        {"hpp", tree_sitter_cpp()},
        {"py", tree_sitter_python()},
        {"js", tree_sitter_javascript()},
        {"ts", tree_sitter_typescript()}
    };

    auto it=language_map.find(extension);
    if(it!=language_map.end())
    {
        return it->second;
    }
    return nullptr;
}

void CodeParser::findChunks(const std::string &sourceCode, TSLanguage *language, std::vector<CodeChunk> &chunks)
{
    if(!ts_parser_set_language(m_parser.get(), language))
    {
        // Handle error: language version mismatch
        return;
    }

    TSTree *tree=ts_parser_parse_string(m_parser.get(), nullptr, sourceCode.c_str(), sourceCode.length());
    if(!tree)
    {
        // Handle error: parsing failed
        return;
    }

    TSNode root_node=ts_tree_root_node(tree);

    // A generic query for functions. This will need to be adapted for each language.
    const char *query_string="(function_definition) @function";
    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query=ts_query_new(language, query_string, strlen(query_string), &error_offset, &error_type);

    if(error_type!=TSQueryErrorNone)
    {
        // Handle error: query syntax error
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
            chunk.startLine=ts_node_start_point(captured_node).row+1;
            chunk.endLine=ts_node_end_point(captured_node).row+1;
            chunk.content=sourceCode.substr(start_byte, end_byte-start_byte);
            chunk.type=ts_node_type(captured_node);

            // Placeholder for identifier extraction
            chunk.identifier="unknown";

            chunks.push_back(chunk);
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
}

} // namespace loreforge