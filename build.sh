#!/bin/bash

# --- Default values ---
OS="linux"
ARCH="x64"
BUILD_TYPE="debug"
CUSTOM_C_FLAGS=""
CUSTOM_CXX_FLAGS=""
REBUILD_CMAKE=false
REBUILD_APP=false

# --- Function to show usage ---
usage() {
    echo "Usage: $0 [options...]"
    echo "Options can be provided in any order:"
    echo "  linux|macos|windows    Operating system target (default: linux)"
    echo "  x64|armv7|arm64       Architecture target (default: x64)"
    echo "  debug|release         Build type (default: debug)"
    echo "  --cflags \"<flags>\"    C compiler flags"
    echo "  --cxxflags \"<flags>\"  C++ compiler flags"
    echo "  --rebuild-cmake       Force clean and re-run CMake"
    echo "  --rebuild             Force a clean build of the application"
    echo "  -h, --help            Show this help message"
    exit 1
}

# --- Process arguments ---
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
                echo "Error: --cflags requires an argument." >&2
                usage
            fi
            ;;
        --cxxflags)
            if [ -n "$2" ] && [[ "$2" != --* ]]; then
                CUSTOM_CXX_FLAGS="$2"
                shift 2
            else
                echo "Error: --cxxflags requires an argument." >&2
                usage
            fi
            ;;
        --rebuild-cmake)
            REBUILD_CMAKE=true
            shift
            ;;
        --rebuild)
            REBUILD_APP=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Error: Unknown option '$1'" >&2
            usage
            ;;
    esac
done

# --- Set up build directory and paths ---
BUILD_DIR="build/${OS}_${ARCH}_${BUILD_TYPE}"

# --- Stop server if running ---
if [ -f "/tmp/loreforge.pid" ]; then
    echo "--- Stopping running server before build ---"
    ./runServer.sh stop --os "$OS" --arch "$ARCH" --build "$BUILD_TYPE"
fi

# --- Handle CMake rebuild ---
if [ "$REBUILD_CMAKE" = true ]; then
    echo "--- Rebuilding CMake configuration ---"
    rm -rf "$BUILD_DIR"
fi

# --- Configure CMake if build directory doesn't exist ---
if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/build.ninja" ]; then
    echo "--- Build directory not ready for Ninja. Configuring CMake for $BUILD_TYPE, $OS on $ARCH... ---"
    mkdir -p "$BUILD_DIR"

    CMAKE_ARGS=(
        "-G" "Ninja"
        "-S" "."
        "-B" "$BUILD_DIR"
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE^}"
        "-DCMAKE_C_FLAGS=${CUSTOM_C_FLAGS}"
        "-DCMAKE_CXX_FLAGS=${CUSTOM_CXX_FLAGS}"
        "-DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake"
        "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=/app/cmake/toolchains/${OS}-${ARCH}.cmake"
        "-DVCPKG_OVERLAY_PORTS=/app/vcpkg/custom_ports"
    )

    cmake "${CMAKE_ARGS[@]}"
fi


# --- Build the application ---
if [ "$REBUILD_APP" = true ]; then
    echo "--- Rebuilding application (clean build) with Ninja ---"
    ninja -C "$BUILD_DIR" clean
    ninja -C "$BUILD_DIR"
else
    echo "--- Building application with Ninja ---"
    ninja -C "$BUILD_DIR"
fi
