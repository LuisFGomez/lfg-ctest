cmake_minimum_required(VERSION 3.10)

project(lfg_ctest C)

# Set C standard to C99 (required by lfg-ctest)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")

# ==============================================================================
# Floating-Point Support Detection
# ==============================================================================
#
# lfg-ctest was originally designed for embedded platforms which may not have
# hardware floating-point support. Float and double are detected separately
# because some platforms have hardware float but software-emulated double.
#
# Usage:
#   - LFG_CTEST_ENABLE_FLOAT=ON   (default): Auto-detect 32-bit float support
#   - LFG_CTEST_ENABLE_DOUBLE=ON  (default): Auto-detect 64-bit double support
#   - Set to OFF to disable for embedded targets without FPU
#
# ==============================================================================

include(CheckIncludeFile)
include(CheckSymbolExists)

option(LFG_CTEST_ENABLE_FLOAT
    "Enable 32-bit float assertions (requires fabsf from math library)" ON)

option(LFG_CTEST_ENABLE_DOUBLE
    "Enable 64-bit double assertions (requires fabs from math library)" ON)

set(LFG_CTEST_HAS_FLOAT OFF)
set(LFG_CTEST_HAS_DOUBLE OFF)
set(LFG_CTEST_NEEDS_LIBM OFF)

# Check for math.h header (required for any floating-point support)
check_include_file(math.h HAVE_MATH_H)

if(HAVE_MATH_H)
    # Check for float support (fabsf)
    if(LFG_CTEST_ENABLE_FLOAT)
        set(CMAKE_REQUIRED_LIBRARIES m)
        check_symbol_exists(fabsf math.h HAVE_FABSF)
        unset(CMAKE_REQUIRED_LIBRARIES)

        if(HAVE_FABSF)
            set(LFG_CTEST_HAS_FLOAT ON)
            set(LFG_CTEST_NEEDS_LIBM ON)
            message(STATUS "lfg-ctest: 32-bit float support ENABLED")
        else()
            message(STATUS "lfg-ctest: 32-bit float support DISABLED (missing fabsf)")
        endif()
    else()
        message(STATUS "lfg-ctest: 32-bit float support DISABLED (user option)")
    endif()

    # Check for double support (fabs)
    if(LFG_CTEST_ENABLE_DOUBLE)
        set(CMAKE_REQUIRED_LIBRARIES m)
        check_symbol_exists(fabs math.h HAVE_FABS)
        unset(CMAKE_REQUIRED_LIBRARIES)

        if(HAVE_FABS)
            set(LFG_CTEST_HAS_DOUBLE ON)
            set(LFG_CTEST_NEEDS_LIBM ON)
            message(STATUS "lfg-ctest: 64-bit double support ENABLED")
        else()
            message(STATUS "lfg-ctest: 64-bit double support DISABLED (missing fabs)")
        endif()
    else()
        message(STATUS "lfg-ctest: 64-bit double support DISABLED (user option)")
    endif()
else()
    message(STATUS "lfg-ctest: Floating-point support DISABLED (no math.h)")
endif()

# ==============================================================================
# Library Build
# ==============================================================================

# Library source files
set(LFG_CTEST_SOURCES
    lfg_ctest.c
    lfg_ctest_mock.c
)

# Create static library
add_library(lfg_ctest STATIC ${LFG_CTEST_SOURCES})
target_include_directories(lfg_ctest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add compile definitions for float/double support
if(LFG_CTEST_HAS_FLOAT)
    target_compile_definitions(lfg_ctest PUBLIC LFG_CTEST_HAS_FLOAT=1)
endif()

if(LFG_CTEST_HAS_DOUBLE)
    target_compile_definitions(lfg_ctest PUBLIC LFG_CTEST_HAS_DOUBLE=1)
endif()

# Link math library only if needed
if(LFG_CTEST_NEEDS_LIBM)
    target_link_libraries(lfg_ctest PUBLIC m)
endif()

# ==============================================================================
# Tests (only when building standalone)
# ==============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
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
endif()

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS lfg_ctest
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES lfg_ctest.h lfg_ctest_mock.h
    DESTINATION include
)

# ==============================================================================
# Configuration Summary
# ==============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "")
    message(STATUS "====================================")
    message(STATUS "lfg-ctest Build Configuration")
    message(STATUS "====================================")
    message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
    message(STATUS "C Standard: C${CMAKE_C_STANDARD}")
    message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "Float (32-bit): ${LFG_CTEST_HAS_FLOAT}")
    message(STATUS "Double (64-bit): ${LFG_CTEST_HAS_DOUBLE}")
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
    message(STATUS "")
    message(STATUS "Disable for embedded (examples):")
    message(STATUS "  cmake -B build -DLFG_CTEST_ENABLE_FLOAT=OFF -DLFG_CTEST_ENABLE_DOUBLE=OFF")
    message(STATUS "  cmake -B build -DLFG_CTEST_ENABLE_DOUBLE=OFF  # float only")
    message(STATUS "====================================")
endif()
