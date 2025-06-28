set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=x86-64" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64" CACHE STRING "C++ compiler flags")