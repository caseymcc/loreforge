cmake_minimum_required(VERSION 3.20)
project(LoreForge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Crow REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)
find_package(Faiss REQUIRED)
find_package(OpenBLAS CONFIG REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Handle arbiterAI dependency
# If arbiterAI target already exists (from parent project), use it
# Otherwise, fetch it independently for standalone builds
if(NOT TARGET arbiterai)
    message(STATUS "arbiterAI not found in parent project, fetching independently for loreforge")
    include(FetchContent)
    
    # Check if arbiterAI already exists locally
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../arbiterAI/CMakeLists.txt)
        message(STATUS "Using existing arbiterAI from ../arbiterAI")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../arbiterAI ${CMAKE_CURRENT_BINARY_DIR}/arbiterAI)
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/arbiterAI/CMakeLists.txt)
        message(STATUS "Using existing arbiterAI from src/arbiterAI")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/arbiterAI ${CMAKE_CURRENT_BINARY_DIR}/arbiterAI)
    else()
        # Fetch arbiterAI from GitHub
        FetchContent_Declare(
            arbiterAI
            GIT_REPOSITORY https://github.com/caseymcc/arbiterAI
            GIT_TAG main
        )
        FetchContent_MakeAvailable(arbiterAI)
    endif()
else()
    message(STATUS "Using arbiterAI from parent project")
endif()

set(LOREFORGE_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/protos/loreforge.proto)

add_library(loreforge_grpc_obj OBJECT
    ${LOREFORGE_PROTO_FILES}
)

set_target_properties(loreforge_grpc_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(loreforge_grpc_obj PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(loreforge_grpc_obj PUBLIC protobuf::libprotobuf)

# Build loreforge as a shared library
add_library(loreforge SHARED
    src/loreforge/service.cpp
    src/loreforge/codeParser.cpp
    src/loreforge/codebaseManager.cpp
    src/loreforge/globalCodebaseManager.cpp
    $<TARGET_OBJECTS:loreforge_grpc_obj>
)

set_target_properties(loreforge PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    VERSION 1.0.0
    SOVERSION 1
)

target_include_directories(loreforge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(loreforge PUBLIC
    Crow::Crow
    unofficial::tree-sitter::tree-sitter
    unofficial::tree-sitter::tree-sitter-cpp
    unofficial::tree-sitter::tree-sitter-python
    unofficial::tree-sitter::tree-sitter-javascript
    unofficial::tree-sitter::tree-sitter-typescript
    faiss
    OpenBLAS::OpenBLAS
    ${LAPACK_LIBRARIES}
    ${BLAS_LIBRARIES}
    gRPC::grpc++
    protobuf::libprotobuf
    arbiterai
    Threads::Threads
)

# Build the original gRPC server as grpc-server executable
add_executable(grpc-server
    src/loreforge/main.cpp
)
set_target_properties(grpc-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_link_libraries(grpc-server PRIVATE
    loreforge
)

# Build the MCP server executable
add_executable(mcp-server
    src/mcp-server/main.cpp
)
set_target_properties(mcp-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_link_libraries(mcp-server PRIVATE
    loreforge
    nlohmann_json::nlohmann_json
)

enable_testing()

add_executable(loreforge_tests
    tests/main.cpp
    tests/serverTests.cpp
    tests/embeddingTests.cpp
    tests/modelManagerTests.cpp
    $<TARGET_OBJECTS:loreforge_grpc_obj>
)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

protobuf_generate(TARGET loreforge_grpc_obj LANGUAGE cpp)
protobuf_generate(
    TARGET
        loreforge_grpc_obj
    LANGUAGE
        grpc
    GENERATE_EXTENSIONS
        .grpc.pb.h
        .grpc.pb.cc
    PLUGIN
        "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)

target_include_directories(loreforge_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(loreforge_tests PRIVATE
    loreforge
    GTest::gtest
    GTest::gmock)

include(GoogleTest)