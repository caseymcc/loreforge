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

#include "loreforge/codeChunk.h"

#include <string>
#include <vector>
#include <memory>
#include <arbiterAI/arbiterAI.h>

// Forward declarations for tree-sitter types
struct TSLanguage;
struct TSParser;

namespace loreforge
{

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
    std::vector<CodeChunk> parse(const std::string &filePath);
    /**
     * @brief Parse source code from memory
     * @param fileName Name of file (used for language detection)
     * @param content Source code content
     * @return Vector of parsed code chunks
     */
    std::vector<CodeChunk> parse(const std::string &fileName, const std::string &content);
    void setEmbeddingModel(std::shared_ptr<arbiterAI::ArbiterAI> model);
    std::shared_ptr<arbiterAI::ArbiterAI> getEmbeddingModel();

    struct FunctionSignature {
        std::string signature;
        std::string returnType;
        std::vector<std::string> parameters;
        bool found = false;
    };

    FunctionSignature extractFunctionSignature(const std::string& filePath, const std::string& functionName);
    std::vector<std::string> findCallees(const std::string& filePath, const std::string& functionName);

private:
    /**
     * @brief Get tree-sitter language parser for file type
     * @param file_path Path to source file
     * @return Pointer to tree-sitter language parser
     */
    TSLanguage *getLanguage(const std::string &filePath);
    /**
     * @brief Find semantic chunks in source code
     * @param source_code Code content to parse
     * @param language tree-sitter language parser
     * @param[out] chunks Output vector for found chunks
     * @param filePath Source file path for reference
     */
    void findChunks(const std::string &source_code, TSLanguage *language, std::vector<CodeChunk> &chunks, const std::string &filePath);

    std::unique_ptr<TSParser, void(*)(TSParser *)> m_parser;
    std::shared_ptr<arbiterAI::ArbiterAI> m_embeddingModel;
};

} // namespace loreforge

#endif // _loreforge_codeParser_h