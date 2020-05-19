cmake_minimum_required(VERSION 3.6)

project(mneme VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
include(CTest)
include(cmake/doctest.cmake)

add_library(mneme-test-runner test/test_main.cpp)
target_include_directories(mneme-test-runner PUBLIC include external)

add_executable(mneme-test test/mneme.cpp)
target_link_libraries(mneme-test mneme-test-runner)
doctest_discover_tests(mneme-test)
