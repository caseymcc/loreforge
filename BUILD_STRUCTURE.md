# LoreForge Build Configuration

## Overview

LoreForge is now structured as:
- **libloreforge.so**: A shared library containing the core functionality
- **grpc-server**: The original gRPC server executable
- **mcp-server**: A new Model Context Protocol (MCP) server executable

## Build Structure

### Shared Library: libloreforge.so

The core loreforge functionality is built as a shared library that includes:
- `service.cpp` - gRPC service implementation
- `codeParser.cpp` - Code parsing functionality
- `codebaseManager.cpp` - Codebase management
- `globalCodebaseManager.cpp` - Global codebase management
- Protocol buffer objects

This library can be linked by any application that needs loreforge functionality.

### Executables

#### 1. grpc-server
The original gRPC server implementation.

**Usage:**
```bash
./grpc-server --repository /path/to/repo --model model-name
```

Starts a gRPC server listening on `0.0.0.0:50051`.

#### 2. mcp-server
A new MCP (Model Context Protocol) server that provides loreforge functionality through the MCP interface.

**Usage:**
```bash
./mcp-server --repository /path/to/repo --model model-name
```

The MCP server communicates via JSON-RPC over stdin/stdout and provides the following tools:
- `query_codebase` - Query the codebase using semantic search
- `index_repository` - Index a repository for semantic search
- `parse_file` - Parse a file and return its structure

**MCP Protocol:**
- Protocol version: 2024-11-05
- Communication: JSON-RPC 2.0 over stdin/stdout
- Server name: loreforge-mcp
- Version: 1.0.0

## Building

```bash
cd server/loreforge
mkdir build && cd build
cmake ..
make
```

This will generate:
- `libloreforge.so` (shared library)
- `grpc-server` (gRPC executable)
- `mcp-server` (MCP executable)
- `loreforge_tests` (test suite)

## Integration

### Using the Shared Library

To use libloreforge.so in your own application:

1. Link against the library:
```cmake
target_link_libraries(your_app PRIVATE loreforge)
```

2. Include the headers:
```cpp
#include "loreforge/service.h"
#include "loreforge/globalCodebaseManager.h"
```

3. Use the service:
```cpp
loreforge::Service service(repositoryPath, model);
// Use service methods...
```

## Dependencies

- nlohmann_json (for MCP server)
- gRPC and protobuf (for gRPC functionality)
- Crow (web server)
- tree-sitter (code parsing)
- Faiss (vector search)
- OpenBLAS, BLAS, LAPACK (linear algebra)
- arbiterai (AI functionality)

All dependencies are managed through vcpkg.json.
