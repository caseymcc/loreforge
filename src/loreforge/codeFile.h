#ifndef _loreforge_codeFile_h
#define _loreforge_codeFile_h

#include "codeChunk.h"

#include <string>
#include <vector>
#include <chrono>

namespace loreforge
{

struct ChunkFileIndex
{
    size_t fileIndex;
    size_t chunkIndex;
};

struct GlobalChunkIndex
{
    std::string codebaseName;
    size_t fileIndex;
    size_t chunkIndex;
};

/**
 * @struct CodeFile
 * @brief Represents a source file and its parsed chunks.
 */
struct CodeFile
{
    std::string filePath;
    std::chrono::system_clock::time_point lastParsed;
    std::vector<CodeChunk> chunks;
};

} // namespace loreforge

#endif // _loreforge_codeFile_h