#!/bin/bash

# Function to show usage
usage() {
    echo "Usage: $0 [options...]"
    echo "Options can be provided in any order:"
    echo "  linux|macos|windows    Operating system target (default: linux)"
    echo "  x64|armv7|arm64       Architecture target (default: x64)"
    echo "  debug|release         Build type (default: debug)"
    echo "  --cflags \"<compiler flags>\"  C compiler flags (e.g., \"-Wall -Wextra\")"
    echo "  --cxxflags \"<compiler flags>\" C++ compiler flags (e.g., \"-std=c++17 -O3\")"
    echo
    echo "Example: $0 linux x64 release --cflags \"-O2\" --cxxflags \"-std=c++17 -DNDEBUG\""
    exit 1
}

# Show usage if help is requested
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    usage
fi

# Set default values
OS="linux"
ARCH="x64"
BUILD_TYPE="debug"
CUSTOM_C_FLAGS=""
CUSTOM_CXX_FLAGS=""

# Process arguments in any order
# Use a loop with shift to handle options that take arguments
while (( "$#" )); do
    case "$1" in
        linux|macos|windows)
            OS="$1"
            shift
            ;;
        x64|armv7|arm64)
            ARCH="$1"
            shift
            ;;
        debug|release)
            BUILD_TYPE="$1"
            shift
            ;;
        --cflags)
            if [ -n "$2" ] && [[ "$2" != --* ]]; then
                CUSTOM_C_FLAGS="$2"
                shift 2
            else
                echo "Error: --cflags requires an argument."
                usage
            fi
            ;;
        --cxxflags)
            if [ -n "$2" ] && [[ "$2" != --* ]]; then
                CUSTOM_CXX_FLAGS="$2"
                shift 2
            else
                echo "Error: --cxxflags requires an argument."
                usage
            fi
            ;;
        *)
            echo "Error: Unknown option '$1'"
            echo "Valid options are:"
            echo "  OS: linux, macos, windows"
            echo "  Architecture: x64, armv7, arm64"
            echo "  Build type: debug, release"
            echo "  Compiler flags: --cflags \"...\", --cxxflags \"...\""
            exit 1
            ;;
    esac
done

# Validate OS
case $OS in
    linux|macos|windows)
        ;;
    *)
        echo "Error: Invalid OS. Must be linux, macos, or windows"
        exit 1
        ;;
esac

# Validate architecture
case $ARCH in
    x64|armv7|arm64)
        ;;
    *)
        echo "Error: Invalid architecture. Must be x64, armv7, or arm64"
        exit 1
        ;;
esac

# Validate build type
case $BUILD_TYPE in
    debug|release)
        ;;
    *)
        echo "Error: Invalid build type. Must be debug or release"
        exit 1
        ;;
esac

# Create build directory name
BUILD_DIR="build/${OS}_${ARCH}_${BUILD_TYPE}"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Configure CMake based on OS and architecture
echo "Configuring CMake for $BUILD_TYPE, $OS on $ARCH..."
cd "$BUILD_DIR"

# Prepare CMake flags array
CMAKE_ARGS=(
    "../.."
    "-DCMAKE_TOOLCHAIN_FILE=/app/cmake/toolchains/${OS}-${ARCH}.cmake"
    "-DVCPKG_OVERLAY_PORTS=/app/vcpkg/custom_ports"
)

# Configure CMake with appropriate options
cmake "${CMAKE_ARGS[@]}"

echo "CMake configuration complete in $BUILD_DIR"