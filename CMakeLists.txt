cmake_minimum_required(VERSION 3.20)
project(LoreForge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow REQUIRED)
find_package(unofficial-tree-sitter CONFIG REQUIRED)
find_package(Faiss REQUIRED)
find_package(OpenBLAS CONFIG REQUIRED)
find_package(BLAS REQUIRED)
find_package(LAPACK REQUIRED)
find_package(protobuf REQUIRED)
find_package(gRPC REQUIRED)



set(LOREFORGE_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/protos/loreforge.proto)

add_library(loreforge_grpc_obj OBJECT
    ${LOREFORGE_PROTO_FILES}
)

target_include_directories(loreforge_grpc_obj PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(loreforge_grpc_obj PUBLIC protobuf::libprotobuf)

add_executable(loreforge
    src/loreforge/main.cpp
    $<TARGET_OBJECTS:loreforge_grpc_obj>
)
set_target_properties(loreforge PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_include_directories(loreforge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(loreforge PRIVATE
    Crow::Crow
    unofficial::tree-sitter::tree-sitter
    faiss
    OpenBLAS::OpenBLAS
    ${LAPACK_LIBRARIES}
    ${BLAS_LIBRARIES}
    gRPC::grpc++
    protobuf::libprotobuf
)

enable_testing()

add_executable(loreforge_tests
    tests/main.cpp
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
    ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(loreforge_tests PRIVATE
    GTest::gtest
    GTest::gmock
    gRPC::grpc++
    protobuf::libprotobuf)

include(GoogleTest)