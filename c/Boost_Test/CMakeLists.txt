set (Boost_USE_STATIC_LIBS OFF)
find_package (Boost REQUIRED COMPONENTS unit_test_framework)

include_directories (${Boost_INCLUDE_DIRS})

add_executable (Algo_tests algoTest.cpp)
target_link_libraries (Algo_tests ${Boost_LIBRARIES} algos)
