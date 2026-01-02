#ifndef _loreforge_codeChunk_h
#define _loreforge_codeChunk_h

#include <string>

#include <faiss/MetricType.h>

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
    std::string chunkId;    // Unique ID for this chunk (e.g., hash of file path, start line, identifier)
    faiss::idx_t faissId = -1; // FAISS internal ID, -1 if not yet indexed
    float score = 0.0f;     // Similarity score (distance)
};

} // namespace loreforge

#endif // _loreforge_codeChunk_h