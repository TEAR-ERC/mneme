cmake_minimum_required(VERSION 3.6)

project(mneme VERSION 0.1 LANGUAGES CXX)

enable_testing()
include(CTest)
include(cmake/doctest.cmake)

add_library(mneme-test-runner test/test_main.cpp)
target_include_directories(mneme-test-runner PUBLIC include external)

add_executable(mneme-test test/mneme.cpp)
target_link_libraries(mneme-test mneme-test-runner)
set_target_properties(mneme-test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
doctest_discover_tests(mneme-test)

add_executable(mneme-test-2 test/test_2.cpp)
target_link_libraries(mneme-test-2 mneme-test-runner)
set_target_properties(mneme-test-2 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
doctest_discover_tests(mneme-test-2)

