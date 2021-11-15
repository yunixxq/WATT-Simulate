cmake_minimum_required(VERSION 3.20)
project(c)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(algos
        algos/lfu_k.cpp evalAccessTable/evalAccessTable.cpp algos/lruStackDist.cpp algos/staticOpt.cpp algos/Generators.cpp
        )
target_compile_options(algos PRIVATE -Werror -Wall -Wextra)


add_executable(evalAccessTable evalAccessTable/main.cpp)

target_compile_options(evalAccessTable PRIVATE -Werror -Wall -Wextra)

target_link_libraries(evalAccessTable algos)

add_subdirectory(Boost_Test)