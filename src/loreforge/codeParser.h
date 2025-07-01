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

struct CodeChunk
{
    std::string filePath; // Path to the source file
    int startLine;
    int endLine;
    std::string content;
    std::string identifier; // e.g., function name, class name
    std::string type;       // e.g., "function_definition", "class_specifier"
};

class CodeParser
{
public:
    CodeParser();
    ~CodeParser();

    std::vector<CodeChunk> parse(const std::string &file_path);

private:
    TSLanguage *getLanguage(const std::string &file_path);
    void findChunks(const std::string &source_code, TSLanguage *language, std::vector<CodeChunk> &chunks);

    std::unique_ptr<TSParser, void(*)(TSParser *)> m_parser;
};

} // namespace loreforge

#endif // _loreforge_codeParser_h