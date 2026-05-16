# ==============================================================================
# contrib/junit-xml -- optional JUnit-XML reporter for lfg-ctest
# ==============================================================================
#
# Builds a small static library (lfg-ctest-junit) that registers itself as the
# runner's reporter when the consuming binary calls lfg_ct_junit_consume_args.
# Independent from the core lfg-ctest library; an embedded consumer that
# doesn't need JUnit output never sees a byte of this code.
#
# Usage from a parent CMakeLists:
#
#     add_subdirectory(deps/lfg-ctest)
#     add_subdirectory(deps/lfg-ctest/contrib/junit-xml)
#     target_link_libraries(my_tests PRIVATE lfg-ctest lfg-ctest-junit)
#
# ==============================================================================

cmake_minimum_required(VERSION 3.10)

add_library(lfg-ctest-junit STATIC lfg-ctest-junit.c)
target_include_directories(lfg-ctest-junit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lfg-ctest-junit PUBLIC lfg-ctest)
target_compile_options(lfg-ctest-junit PRIVATE -Wall -Wextra -pedantic)

install(TARGETS lfg-ctest-junit
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(FILES lfg-ctest-junit.h DESTINATION include)

# ==============================================================================
# Self-test binary. Built unconditionally when this CMakeLists is invoked --
# the parent decides whether to invoke (gates the add_subdirectory on
# standalone in the top-level CMakeLists). An embedder doing
# add_subdirectory(deps/lfg-ctest) without the contrib never sees this.
# ==============================================================================

add_executable(test-junit test-junit.c)
target_link_libraries(test-junit PRIVATE lfg-ctest lfg-ctest-junit)
target_compile_definitions(test-junit PRIVATE LFG_CTEST_SELF_TEST=1)

enable_testing()
add_test(NAME test-junit COMMAND test-junit)
