cmake_minimum_required(VERSION 3.20)
project(c)

set(CMAKE_CXX_STANDARD 20)

add_library(algos algos/lruStackDist.cpp algos/random.hpp algos/EvictStrategy.hpp algos/lru.hpp algos/opt.hpp algos/cf_lru.hpp algos/lru_wsr.hpp)

add_executable(evalAccessTable evalAccessTable/evalAccessTable.cpp )

target_link_libraries(evalAccessTable algos)