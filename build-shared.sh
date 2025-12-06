#!/bin/bash

# Build script for loreforge with shared library configuration

set -e

echo "Building loreforge as shared library with MCP server..."

BUILD_DIR="build"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build
echo "Building..."
make -j$(nproc)

echo ""
echo "Build complete!"
echo ""
echo "Generated artifacts:"
echo "  - libloreforge.so (shared library)"
echo "  - grpc-server (gRPC server executable)"
echo "  - mcp-server (MCP server executable)"
echo "  - loreforge_tests (test suite)"
echo ""
echo "Usage:"
echo "  ./grpc-server --repository /path/to/repo --model model-name"
echo "  ./mcp-server --repository /path/to/repo --model model-name"
