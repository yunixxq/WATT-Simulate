cmake_minimum_required(VERSION 3.20)
project(c)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


add_executable(evalAccessTable evalAccessTable/main.cpp algos/lfu_k.cpp evalAccessTable/evalAccessTable.cpp algos/lruStackDist.cpp algos/staticOpt.cpp)

target_compile_options(evalAccessTable PRIVATE -Werror -Wall -Wextra)

add_subdirectory(Boost_Test)