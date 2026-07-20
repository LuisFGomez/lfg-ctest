cmake_minimum_required(VERSION 3.10)

project(lfg-ctest C)

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

# Fork-per-test isolation (LFG_CT_ISOLATE_FORK). On by default. Set OFF to
# build the lfg-ctest static lib without any fork/waitpid/signal-handling
# code linked in -- consumers can still reference the enum value, but
# lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK) returns the documented error.
option(LFG_CTEST_ENABLE_FORK
    "Enable fork-per-test isolation runtime support (Unix-family only)" ON)

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
# Version Header (generated from git describe)
# ==============================================================================
#
# tools/mkversion.c runs `git -C <src> describe --match 'v*'` and emits
# lfg-ctest-version.h into the binary dir. The custom target runs on every
# build (ALL); `copy_if_different` shields downstream compilation from
# spurious rebuilds when git state is unchanged.
#
# Falls back to 0.0.0 when there is no matching tag or no git repo --
# harmless for add_subdirectory embedders who vendor a tarball.
#
# ==============================================================================

add_executable(lfg_ct_mkversion tools/mkversion.c)
target_compile_options(lfg_ct_mkversion PRIVATE -Wall -Wextra -pedantic)

set(LFG_CTEST_VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/lfg-ctest-version.h)

add_custom_target(lfg_ct_version_header ALL
    COMMAND lfg_ct_mkversion LFG_CTEST ${CMAKE_CURRENT_SOURCE_DIR}
        > ${LFG_CTEST_VERSION_HEADER}.tmp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LFG_CTEST_VERSION_HEADER}.tmp ${LFG_CTEST_VERSION_HEADER}
    COMMAND ${CMAKE_COMMAND} -E remove ${LFG_CTEST_VERSION_HEADER}.tmp
    BYPRODUCTS ${LFG_CTEST_VERSION_HEADER}
    DEPENDS lfg_ct_mkversion
    COMMENT "Generating lfg-ctest-version.h from git describe"
)

# ==============================================================================
# Library Build
# ==============================================================================

# Library source files
set(LFG_CTEST_SOURCES
    lfg-ctest.c
    lfg-ctest-mock.c
    lfg-ctest-fork.c
)

# Create static library
add_library(lfg-ctest STATIC ${LFG_CTEST_SOURCES})
target_include_directories(lfg-ctest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(lfg-ctest lfg_ct_version_header)

# Enable self-test mode when building standalone (for framework's own tests)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_compile_definitions(lfg-ctest PRIVATE LFG_CTEST_SELF_TEST=1)
endif()

# Add compile definitions for float/double support
if(LFG_CTEST_HAS_FLOAT)
    target_compile_definitions(lfg-ctest PUBLIC LFG_CTEST_HAS_FLOAT=1)
endif()

if(LFG_CTEST_HAS_DOUBLE)
    target_compile_definitions(lfg-ctest PUBLIC LFG_CTEST_HAS_DOUBLE=1)
endif()

# Link math library only if needed
if(LFG_CTEST_NEEDS_LIBM)
    target_link_libraries(lfg-ctest PUBLIC m)
endif()

# Fork-mode opt-out. PRIVATE -- only lfg-ctest-fork.c checks this macro.
# Consumers that need to query whether fork support is compiled in should
# use lfg_ct_set_isolation()'s return value rather than testing the macro.
if(NOT LFG_CTEST_ENABLE_FORK)
    target_compile_definitions(lfg-ctest PRIVATE LFG_CT_DISABLE_FORK=1)
    message(STATUS "lfg-ctest: fork-per-test isolation DISABLED (build option)")
else()
    message(STATUS "lfg-ctest: fork-per-test isolation ENABLED (Unix-family runtime)")
endif()

# ==============================================================================
# Amalgamator -- C tool that produces dist/lfg-ctest.h (single-header form)
# ==============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_executable(lfg_ct_amalgamate tools/amalgamate.c)
    target_compile_options(lfg_ct_amalgamate PRIVATE -Wall -Wextra -pedantic)

    set(LFG_CTEST_AMALG_SOURCES
        lfg-ctest.h
        lfg-ctest-mock.h
        lfg-ctest.c
        lfg-ctest-mock.c
        tools/amalgamate.manifest
    )

    # Search dirs for the amalgamator: source root first (checked-in files),
    # then binary dir (generated lfg-ctest-version.h).
    # NB: DEPENDS lists the generated header by *path* as well as by target
    # name. The target name only establishes build order; the path makes
    # Ninja/Make retrigger amalgamation when the file's mtime changes
    # (which, thanks to copy_if_different, only happens on real content change).
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/dist/lfg-ctest.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/dist
        COMMAND lfg_ct_amalgamate
            ${CMAKE_SOURCE_DIR}/tools/amalgamate.manifest
            ${CMAKE_SOURCE_DIR}/dist/lfg-ctest.h
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS lfg_ct_amalgamate lfg_ct_version_header
                ${LFG_CTEST_VERSION_HEADER} ${LFG_CTEST_AMALG_SOURCES}
        COMMENT "Amalgamating dist/lfg-ctest.h"
    )

    add_custom_target(amalgamate DEPENDS ${CMAKE_SOURCE_DIR}/dist/lfg-ctest.h)

    # ── release-tag: interactive shortcut for release-v<M>.<m>.<p> ─
    # Reads `git describe`, proposes the right tag, prompts for
    # confirmation, then execs `git tag -a` so $EDITOR handles the
    # annotation. USES_TERMINAL gives the tool a real tty so the
    # prompt actually appears (Ninja otherwise captures stdio).
    add_executable(lfg_ct_mkrelease tools/mkrelease.c)
    target_compile_options(lfg_ct_mkrelease PRIVATE -Wall -Wextra -pedantic)
    add_custom_target(release-tag
        COMMAND lfg_ct_mkrelease ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS lfg_ct_mkrelease
        USES_TERMINAL
        VERBATIM
    )
endif()

# ==============================================================================
# Tests (only when building standalone)
# ==============================================================================

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Optional contrib packages -- only built standalone, never when this
    # repo is consumed via add_subdirectory. Embedded users get zero
    # footprint from contrib code.
    add_subdirectory(contrib/junit-xml)

    # Test executables
    set(TEST_TARGETS
        test-unified
        test-mock
    )

    # Add the fork-mode test only when the library was built with fork
    # support; otherwise the runtime check at the top of test-fork.c
    # would short-circuit every test into a no-op.
    if(LFG_CTEST_ENABLE_FORK)
        list(APPEND TEST_TARGETS test-fork)
    endif()

    # Build each test executable
    foreach(TEST_TARGET ${TEST_TARGETS})
        add_executable(${TEST_TARGET} ${TEST_TARGET}.c)
        target_link_libraries(${TEST_TARGET} lfg-ctest)
        # The test sources include lfg-ctest.h -> lfg-ctest-version.h (generated).
        # add_dependencies() is not transitive across target_link_libraries, so
        # state the prerequisite explicitly to avoid first-build races.
        add_dependencies(${TEST_TARGET} lfg_ct_version_header)
        # Enable self-test mode for framework's own tests
        target_compile_definitions(${TEST_TARGET} PRIVATE LFG_CTEST_SELF_TEST=1)
    endforeach()

    # test-fork-disabled rebuilds the framework sources with the
    # compile-time opt-out flag so the binary exercises the
    # "no fork support linked in" path. Does NOT link against the
    # lfg-ctest static lib (which has fork enabled by default); pattern
    # mirrors test-amalg's self-contained build.
    add_executable(test-fork-disabled
        test-fork-disabled.c
        lfg-ctest.c
        lfg-ctest-mock.c
        lfg-ctest-fork.c
    )
    target_compile_definitions(test-fork-disabled PRIVATE
        LFG_CTEST_SELF_TEST=1
        LFG_CT_DISABLE_FORK=1
    )
    target_include_directories(test-fork-disabled PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_dependencies(test-fork-disabled lfg_ct_version_header)
    if(LFG_CTEST_HAS_FLOAT)
        target_compile_definitions(test-fork-disabled PRIVATE LFG_CTEST_HAS_FLOAT=1)
    endif()
    if(LFG_CTEST_HAS_DOUBLE)
        target_compile_definitions(test-fork-disabled PRIVATE LFG_CTEST_HAS_DOUBLE=1)
    endif()
    if(LFG_CTEST_NEEDS_LIBM)
        target_link_libraries(test-fork-disabled PRIVATE m)
    endif()
    list(APPEND TEST_TARGETS test-fork-disabled)

    # test-compat-3arg rebuilds the framework sources with the deprecated
    # 3-argument registration shim enabled so CI compiles every
    # LFG_CT_COMPAT_3ARG #ifdef branch (spread across lfg-ctest.c,
    # lfg-ctest-mock.c, and lfg-ctest-fork.c) and exercises the retained
    # setup -> body -> teardown lifecycle. The static lib is body-only, so
    # like test-fork-disabled this target compiles the sources itself.
    add_executable(test-compat-3arg
        test-compat-3arg.c
        lfg-ctest.c
        lfg-ctest-mock.c
        lfg-ctest-fork.c
    )
    target_compile_definitions(test-compat-3arg PRIVATE
        LFG_CT_COMPAT_3ARG=1
    )
    target_include_directories(test-compat-3arg PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    add_dependencies(test-compat-3arg lfg_ct_version_header)
    if(LFG_CTEST_HAS_FLOAT)
        target_compile_definitions(test-compat-3arg PRIVATE LFG_CTEST_HAS_FLOAT=1)
    endif()
    if(LFG_CTEST_HAS_DOUBLE)
        target_compile_definitions(test-compat-3arg PRIVATE LFG_CTEST_HAS_DOUBLE=1)
    endif()
    if(LFG_CTEST_NEEDS_LIBM)
        target_link_libraries(test-compat-3arg PRIVATE m)
    endif()
    list(APPEND TEST_TARGETS test-compat-3arg)

    # Smoke test for the amalgamated header -- catches drift between the
    # split sources and dist/lfg-ctest.h. Self-contained (defines
    # LFG_CTEST_IMPLEMENTATION itself; does NOT link against lfg-ctest).
    add_executable(test-amalg test-amalg.c)
    add_dependencies(test-amalg amalgamate)
    target_include_directories(test-amalg PRIVATE ${CMAKE_SOURCE_DIR})
    if(LFG_CTEST_HAS_FLOAT)
        target_compile_definitions(test-amalg PRIVATE LFG_CTEST_HAS_FLOAT=1)
    endif()
    if(LFG_CTEST_HAS_DOUBLE)
        target_compile_definitions(test-amalg PRIVATE LFG_CTEST_HAS_DOUBLE=1)
    endif()
    if(LFG_CTEST_NEEDS_LIBM)
        target_link_libraries(test-amalg PRIVATE m)
    endif()
    list(APPEND TEST_TARGETS test-amalg)

    # Enable testing
    enable_testing()

    # Add tests to CTest
    # Note: test-unified includes intentional failures to verify framework behavior
    add_test(NAME test-unified COMMAND test-unified)
    add_test(NAME test-mock COMMAND test-mock)
    add_test(NAME test-amalg COMMAND test-amalg)
    if(LFG_CTEST_ENABLE_FORK)
        add_test(NAME test-fork COMMAND test-fork)
    endif()
    add_test(NAME test-fork-disabled COMMAND test-fork-disabled)
    add_test(NAME test-compat-3arg COMMAND test-compat-3arg)

    # `--list | grep | xargs --filter` is a documented contract (#55) and can
    # only be checked outside the runner, on the real stdout bytes. Needs a
    # POSIX shell, so it is skipped where one is not assumed.
    if(UNIX)
        add_test(NAME test-id-roundtrip
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/check-id-roundtrip.sh $<TARGET_FILE:test-amalg>)
    endif()

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

install(TARGETS lfg-ctest
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES
    lfg-ctest.h
    lfg-ctest-mock.h
    ${LFG_CTEST_VERSION_HEADER}
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
    message(STATUS "Fork isolation: ${LFG_CTEST_ENABLE_FORK}")
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
