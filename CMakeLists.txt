cmake_minimum_required(VERSION 3.10)

project(lfg_ctest C)

# Set C standard to C99 (required by lfg-ctest)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# Library source files
set(LFG_CTEST_SOURCES
    lfg_ctest.c
    lfg_ctest_mock.c
)

# Create static library
add_library(lfg_ctest STATIC ${LFG_CTEST_SOURCES})
target_include_directories(lfg_ctest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test executables
set(TEST_TARGETS
    test_unified
    test_mock
)

# Build each test executable
foreach(TEST_TARGET ${TEST_TARGETS})
    add_executable(${TEST_TARGET} ${TEST_TARGET}.c)
    target_link_libraries(${TEST_TARGET} lfg_ctest)
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
install(TARGETS lfg_ctest
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES lfg_ctest.h lfg_ctest_mock.h
    DESTINATION include
)

# Print configuration summary
message(STATUS "")
message(STATUS "====================================")
message(STATUS "lfg-ctest Build Configuration")
message(STATUS "====================================")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Tests: ${TEST_TARGETS}")
message(STATUS "====================================")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake -B build -G Ninja")
message(STATUS "  cmake --build build")
message(STATUS "")
message(STATUS "Run tests:")
message(STATUS "  ctest --test-dir build")
message(STATUS "  cmake --build build --target run_all_tests")
message(STATUS "====================================")
