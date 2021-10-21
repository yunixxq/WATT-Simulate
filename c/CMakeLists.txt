cmake_minimum_required(VERSION 3.20)
project(c)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_library(algos
        algos/lruStackDist.cpp
        )
add_executable(evalAccessTable evalAccessTable/evalAccessTable.cpp )
target_compile_options(evalAccessTable PRIVATE -Werror -Wall -Wextra)


add_executable(test test/test.cpp)

target_link_libraries(evalAccessTable algos)

add_subdirectory(Boost_Test)