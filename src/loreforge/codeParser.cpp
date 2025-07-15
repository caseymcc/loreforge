#include "loreforge/codeParser.h"

#include <tree_sitter/api.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstring>
#include <string>

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

    // Skip chunking for small files (under 100KB)
    if(sourceCode.size()>100*1024)
    {
        size_t chunkSize=1024*1024; // 1MB chunks
        size_t pos=0;

        while(pos<sourceCode.size())
        {
            size_t end=std::min(pos+chunkSize, sourceCode.size());
            std::string chunkContent=sourceCode.substr(pos, end-pos);
            CodeChunk chunk;
            chunk.filePath=filePath;
            chunk.startLine=1;
            chunk.endLine=1;
            chunk.content=chunkContent;
            chunk.type="chunk";
            chunks.push_back(chunk);
            pos=end;
        }
    }

    findChunks(sourceCode, language, chunks, filePath);
    return chunks;
}

std::vector<CodeChunk> CodeParser::parse(const std::string &fileName, const std::string &content)
{
    std::vector<CodeChunk> chunks;
    TSLanguage *language=getLanguage(fileName);
    if(language)
    {
        findChunks(content, language, chunks, fileName);
    }
    return chunks;
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

void CodeParser::findChunks(const std::string &sourceCode, TSLanguage *language, std::vector<CodeChunk> &chunks, const std::string &filePath)
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
    if(ts_node_has_error(root_node))
    {
        ts_tree_delete(tree);
        return;
    }

    const char *query_string;
    if(language==tree_sitter_cpp())
    {
        query_string="[(function_definition) @function (class_specifier) @class]";
    }
    else if(language==tree_sitter_python())
    {
        query_string="[(function_definition) @function (class_definition) @class]";
    }
    else
    {
        query_string="[(function_declaration) @function (class_declaration) @class]";
    }

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
            chunk.filePath=filePath;
            chunk.startLine=ts_node_start_point(captured_node).row+1;
            chunk.endLine=ts_node_end_point(captured_node).row+1;
            chunk.content=sourceCode.substr(start_byte, end_byte-start_byte);
            chunk.type=ts_node_type(captured_node);

            TSNode name_node;
            if(strcmp(chunk.type.c_str(), "class_specifier")==0||strcmp(chunk.type.c_str(), "class_definition")==0||strcmp(chunk.type.c_str(), "class_declaration")==0)
            {
                name_node=ts_node_child_by_field_name(captured_node, "name", 4);
            }
            else
            {
                name_node=ts_node_child_by_field_name(captured_node, "declarator", 10);
                if(ts_node_is_null(name_node))
                {
                    name_node=ts_node_child_by_field_name(captured_node, "name", 4);
                }
            }

            if(!ts_node_is_null(name_node))
            {
                uint32_t name_start_byte=ts_node_start_byte(name_node);
                uint32_t name_end_byte=ts_node_end_byte(name_node);
                chunk.identifier=sourceCode.substr(name_start_byte, name_end_byte-name_start_byte);
            }
            else
            {
                chunk.identifier="unknown";
            }

            chunks.push_back(chunk);
        }
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    ts_tree_delete(tree);
}

} // namespace loreforge