# CMakeLists.txt - Build configuration for c_tester with CTest support
#
# Purpose: Provides CMake build system with CTest integration
# Design: Minimal configuration - add c_tester as executable,
#         enable testing, add test for c_tester.

cmake_minimum_required(VERSION 3.10)
project(c_tester C)

# Build c_tester executable
add_executable(c_tester c_tester.c)

# Enable testing support for CTest
enable_testing()

# Add CTest test - runs c_tester on a sample bad code file
add_test(
    NAME c_tester_bad_code
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/c_tester
            ${CMAKE_CURRENT_SOURCE_DIR}/bad_codes/aliasing.c
)

# Add test that runs the full test suite
add_test(
    NAME c_tester_test_suite
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/c_tester
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/clean.c
)
