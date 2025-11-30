cmake_minimum_required(VERSION 3.10)

project(lfgtest C)

# Set C standard to C99 (required by lfgtest)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Library source files
set(LFGTEST_SOURCES
    lfgtest.c
    lfgtest_mock.c
)

# Create static library
add_library(lfgtest STATIC ${LFGTEST_SOURCES})
target_include_directories(lfgtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test executables
set(TEST_TARGETS
    test_unified
    test_mock
)

# Build each test executable
foreach(TEST_TARGET ${TEST_TARGETS})
    add_executable(${TEST_TARGET} ${TEST_TARGET}.c)
    target_link_libraries(${TEST_TARGET} lfgtest)
endforeach()

# Enable testing
enable_testing()

# Add tests to CTest
# Note: test_unified includes intentional failures to verify framework behavior
add_test(NAME test_unified COMMAND test_unified)
add_test(NAME test_mock COMMAND test_mock)

# Custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS ${TEST_TARGETS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# Installation rules (optional)
install(TARGETS lfgtest
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES lfgtest.h lfgtest_mock.h
    DESTINATION include
)

# Print configuration summary
message(STATUS "")
message(STATUS "====================================")
message(STATUS "LFG Test Framework Build Configuration")
message(STATUS "====================================")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tests: ${TEST_TARGETS}")
message(STATUS "====================================")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  mkdir -p build && cd build")
message(STATUS "  cmake ..")
message(STATUS "  make")
message(STATUS "")
message(STATUS "Run tests:")
message(STATUS "  make test                  # Run via CTest")
message(STATUS "  make run_all_tests         # Run with verbose output")
message(STATUS "  ./test_unified             # Run unified test directly")
message(STATUS "====================================")
