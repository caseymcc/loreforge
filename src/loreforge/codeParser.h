/**
 * @file codeParser.h
 * @brief Code parsing interface for LoreForge MCP server
 *
 * Provides functionality to:
 * - Parse source code files into semantic chunks
 * - Extract function/class definitions
 * - Support multiple programming languages
 */

#ifndef _loreforge_codeParser_h
#define _loreforge_codeParser_h

#include <string>
#include <vector>
#include <memory>

// Forward declarations for tree-sitter types
struct TSLanguage;
struct TSParser;

namespace loreforge
{

/**
 * @struct CodeChunk
 * @brief Represents a parsed chunk of source code
 */
struct CodeChunk
{
    std::string filePath; // Path to the source file
    int startLine;
    int endLine;
    std::string content;
    std::string identifier; // e.g., function name, class name
    std::string type;       // e.g., "function_definition", "class_specifier"
};

/**
 * @class CodeParser
 * @brief Parses source code files into semantic chunks
 *
 * Uses tree-sitter for language-aware parsing of:
 * - Function definitions
 * - Class/struct definitions
 * - Other code constructs
 */
class CodeParser
{
public:
    CodeParser();
    ~CodeParser();

    /**
     * @brief Parse a source code file
     * @param file_path Path to source file
     * @return Vector of parsed code chunks
     */
    std::vector<CodeChunk> parse(const std::string &file_path);
    /**
     * @brief Parse source code from memory
     * @param fileName Name of file (used for language detection)
     * @param content Source code content
     * @return Vector of parsed code chunks
     */
    std::vector<CodeChunk> parse(const std::string &fileName, const std::string &content);

private:
    /**
     * @brief Get tree-sitter language parser for file type
     * @param file_path Path to source file
     * @return Pointer to tree-sitter language parser
     */
    TSLanguage *getLanguage(const std::string &file_path);
    /**
     * @brief Find semantic chunks in source code
     * @param source_code Code content to parse
     * @param language tree-sitter language parser
     * @param[out] chunks Output vector for found chunks
     * @param filePath Source file path for reference
     */
    void findChunks(const std::string &source_code, TSLanguage *language, std::vector<CodeChunk> &chunks, const std::string &filePath);

    std::unique_ptr<TSParser, void(*)(TSParser *)> m_parser;
};

} // namespace loreforge

#endif // _loreforge_codeParser_h