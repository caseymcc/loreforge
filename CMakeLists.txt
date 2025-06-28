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


set(loreforge_source
    src/loreforge/main.cpp
)

add_executable(loreforge
    ${loreforge_source}
)

target_include_directories(loreforge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(loreforge PRIVATE
    Crow::Crow
    unofficial::tree-sitter::tree-sitter
    faiss
    OpenBLAS::OpenBLAS
    ${LAPACK_LIBRARIES}
    ${BLAS_LIBRARIES}
)

enable_testing()

add_executable(loreforge_tests
    tests/main.cpp
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(loreforge_tests PRIVATE GTest::gtest GTest::gmock)

include(GoogleTest)
gtest_discover_tests(loreforge_tests)