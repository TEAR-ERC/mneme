cmake_minimum_required(VERSION 3.6)

project(mneme VERSION 0.1 LANGUAGES CXX)

enable_testing()
add_subdirectory(external/Catch2)
include(CTest)
include(external/Catch2/contrib/Catch.cmake)

add_library(mneme-test-runner test/test_main.cpp)
target_link_libraries(mneme-test-runner PUBLIC Catch2::Catch2)
target_include_directories(mneme-test-runner PUBLIC include)

add_executable(mneme-test test/mneme.cpp)
target_link_libraries(mneme-test mneme-test-runner)
set_target_properties(mneme-test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
catch_discover_tests(mneme-test)

