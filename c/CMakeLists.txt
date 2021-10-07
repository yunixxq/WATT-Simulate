cmake_minimum_required(VERSION 3.20)
project(c)

set(CMAKE_CXX_STANDARD 20)

add_library(lru algos/lruStackDist.cpp algos/random.hpp algos/EvictStrategy.hpp)

add_executable(evalAccessTable evalAccessTable/evalAccessTable.cpp)

target_link_libraries(evalAccessTable lru)