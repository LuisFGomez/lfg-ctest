/**
 * @file
 * @brief       lfg-ctest unit testing helpers.
 */

#ifndef LFG_CTEST_H_
#define LFG_CTEST_H_

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Generated at configure/build time by tools/mkversion.c -- contains
 * LFG_CTEST_VERSION, LFG_CTEST_VERSION_FULL, and LFG_CTEST_VERSION_{MAJOR,MINOR,PATCH}. */
#include "lfg-ctest-version.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

#ifndef _STR
#define _STR(_x) #_x
#endif
#ifndef STR
#define STR(_x) _STR(_x)
#endif

/**
 * LFG_CT_FUNCTION - portable function name identifier
 *
 * Auto-detects the best available option:
 *   1. __func__     (C99 standard)
 *   2. __FUNCTION__ (GCC/Clang/MSVC extension for C89)
 *   3. "(unknown)"  (fallback)
 *
 * Define LFG_CTEST_NO_FUNC before including this header to disable
 * function name reporting entirely.
 */
#ifdef LFG_CTEST_NO_FUNC
#define LFG_CT_FUNCTION "(unknown)"
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define LFG_CT_FUNCTION __func__
#elif defined(__GNUC__) || defined(_MSC_VER)
#define LFG_CT_FUNCTION __FUNCTION__
#else
#define LFG_CT_FUNCTION "(unknown)"
#endif

/** Execute a test or suite of tests.
 */
#define ASSERT_FALSE(_cond) lfg_ct_assert_false_impl((_cond), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_cond))

#define ASSERT_TRUE(_cond) lfg_ct_assert_true_impl((_cond), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_cond))

#define ASSERT_INT_EQUAL(_e, _a)                                                                                       \
    lfg_ct_assert_int_equal_impl((int)(_e), (int)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT_NOT_EQUAL(_e, _a)                                                                                   \
    lfg_ct_assert_int_not_equal_impl((int)(_e), (int)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT_EQUAL(_e, _a)                                                                                      \
    lfg_ct_assert_uint_equal_impl((unsigned)(_e), (unsigned)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT_NOT_EQUAL(_e, _a)                                                                                  \
    lfg_ct_assert_uint_not_equal_impl((unsigned)(_e), (unsigned)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT8_EQUAL(_e, _a)                                                                                     \
    lfg_ct_assert_uint8_equal_impl((uint8_t)(_e), (uint8_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT8_NOT_EQUAL(_e, _a)                                                                                 \
    lfg_ct_assert_uint8_not_equal_impl((uint8_t)(_e), (uint8_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT16_EQUAL(_e, _a)                                                                                    \
    lfg_ct_assert_uint16_equal_impl((uint16_t)(_e), (uint16_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT16_NOT_EQUAL(_e, _a)                                                                                \
    lfg_ct_assert_uint16_not_equal_impl((uint16_t)(_e), (uint16_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT32_EQUAL(_e, _a)                                                                                    \
    lfg_ct_assert_uint32_equal_impl((uint32_t)(_e), (uint32_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT32_NOT_EQUAL(_e, _a)                                                                                \
    lfg_ct_assert_uint32_not_equal_impl((uint32_t)(_e), (uint32_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_PTR_EQUAL(_e, _a)                                                                                       \
    lfg_ct_assert_ptr_equal_impl((void *)(_e), (void *)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_PTR_NOT_EQUAL(_e, _a)                                                                                   \
    lfg_ct_assert_ptr_not_equal_impl((void *)(_e), (void *)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_PTR_NOT_NULL(_a) lfg_ct_assert_ptr_not_null((void *)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_PTR_NULL(_a) lfg_ct_assert_ptr_null((void *)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT8_EQUAL(_e, _a)                                                                                      \
    lfg_ct_assert_int8_equal_impl((int8_t)(_e), (int8_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT8_NOT_EQUAL(_e, _a)                                                                                  \
    lfg_ct_assert_int8_not_equal_impl((int8_t)(_e), (int8_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT16_EQUAL(_e, _a)                                                                                     \
    lfg_ct_assert_int16_equal_impl((int16_t)(_e), (int16_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT16_NOT_EQUAL(_e, _a)                                                                                 \
    lfg_ct_assert_int16_not_equal_impl((int16_t)(_e), (int16_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT32_EQUAL(_e, _a)                                                                                     \
    lfg_ct_assert_int32_equal_impl((int32_t)(_e), (int32_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT32_NOT_EQUAL(_e, _a)                                                                                 \
    lfg_ct_assert_int32_not_equal_impl((int32_t)(_e), (int32_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT64_EQUAL(_e, _a)                                                                                     \
    lfg_ct_assert_int64_equal_impl((int64_t)(_e), (int64_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_INT64_NOT_EQUAL(_e, _a)                                                                                 \
    lfg_ct_assert_int64_not_equal_impl((int64_t)(_e), (int64_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT64_EQUAL(_e, _a)                                                                                    \
    lfg_ct_assert_uint64_equal_impl((uint64_t)(_e), (uint64_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_UINT64_NOT_EQUAL(_e, _a)                                                                                \
    lfg_ct_assert_uint64_not_equal_impl((uint64_t)(_e), (uint64_t)(_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_STR_EQUAL(_e, _a) lfg_ct_assert_str_equal_impl((_e), (_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_STR_NOT_EQUAL(_e, _a)                                                                                   \
    lfg_ct_assert_str_not_equal_impl((_e), (_a), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_STRN_EQUAL(_e, _a, _n)                                                                                  \
    lfg_ct_assert_strn_equal_impl((_e), (_a), (_n), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_MEM_EQUAL(_e, _a, _n)                                                                                   \
    lfg_ct_assert_mem_equal_impl((_e), (_a), (_n), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_MEM_NOT_EQUAL(_e, _a, _n)                                                                               \
    lfg_ct_assert_mem_not_equal_impl((_e), (_a), (_n), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

#define ASSERT_GREATER_THAN(_a, _b)                                                                                    \
    lfg_ct_assert_greater_than_impl((int)(_a), (int)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

#define ASSERT_LESS_THAN(_a, _b)                                                                                       \
    lfg_ct_assert_less_than_impl((int)(_a), (int)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

#define ASSERT_GREATER_OR_EQUAL(_a, _b)                                                                                \
    lfg_ct_assert_greater_or_equal_impl((int)(_a), (int)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

#define ASSERT_LESS_OR_EQUAL(_a, _b)                                                                                   \
    lfg_ct_assert_less_or_equal_impl((int)(_a), (int)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

#define ASSERT_IN_RANGE(_val, _min, _max)                                                                              \
    lfg_ct_assert_in_range_impl((int)(_val), (int)(_min), (int)(_max), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val))

#define ASSERT_BIT_SET(_val, _bit)                                                                                     \
    lfg_ct_assert_bit_set_impl((unsigned)(_val), (_bit), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val), (_bit))

#define ASSERT_BIT_CLEAR(_val, _bit)                                                                                   \
    lfg_ct_assert_bit_clear_impl((unsigned)(_val), (_bit), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val), (_bit))

#define ASSERT_BITS_SET(_val, _mask)                                                                                   \
    lfg_ct_assert_bits_set_impl((unsigned)(_val), (_mask), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val), (_mask))

#define ASSERT_BITS_CLEAR(_val, _mask)                                                                                 \
    lfg_ct_assert_bits_clear_impl((unsigned)(_val), (_mask), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val), (_mask))

#define ASSERT_FAIL(_msg) lfg_ct_assert_fail_impl(__FILE__, __LINE__, LFG_CT_FUNCTION, (_msg))

/*============================================================================
 *  32-bit Float Assertions (optional - requires LFG_CTEST_HAS_FLOAT)
 *
 *  These assertions require fabsf from math library.
 *  On embedded platforms without FPU, disable with:
 *    cmake -DLFG_CTEST_ENABLE_FLOAT=OFF
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_FLOAT

/** Assert float values are equal within epsilon tolerance */
#define ASSERT_FLOAT_EQUAL(_e, _a, _eps)                                                                               \
    lfg_ct_assert_float_equal_impl(                                                                                    \
            (float)(_e), (float)(_a), (float)(_eps), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

/** Assert float values are not equal (differ by more than epsilon) */
#define ASSERT_FLOAT_NOT_EQUAL(_e, _a, _eps)                                                                           \
    lfg_ct_assert_float_not_equal_impl(                                                                                \
            (float)(_e), (float)(_a), (float)(_eps), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

/** Assert float a > b */
#define ASSERT_FLOAT_GREATER_THAN(_a, _b)                                                                              \
    lfg_ct_assert_float_greater_impl((float)(_a), (float)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

/** Assert float a < b */
#define ASSERT_FLOAT_LESS_THAN(_a, _b)                                                                                 \
    lfg_ct_assert_float_less_impl((float)(_a), (float)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

/** Assert float a >= b */
#define ASSERT_FLOAT_GREATER_OR_EQUAL(_a, _b)                                                                          \
    lfg_ct_assert_float_ge_impl((float)(_a), (float)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

/** Assert float a <= b */
#define ASSERT_FLOAT_LESS_OR_EQUAL(_a, _b)                                                                             \
    lfg_ct_assert_float_le_impl((float)(_a), (float)(_b), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a), STR(_b))

/** Assert float value is within range [min, max] */
#define ASSERT_FLOAT_IN_RANGE(_val, _min, _max)                                                                        \
    lfg_ct_assert_float_in_range_impl(                                                                                 \
            (float)(_val), (float)(_min), (float)(_max), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_val))

/* Shorthand aliases for float assertions */
#define ASSERT_FLT_EQ(_e, _a, _eps) ASSERT_FLOAT_EQUAL(_e, _a, _eps)
#define ASSERT_FLT_NE(_e, _a, _eps) ASSERT_FLOAT_NOT_EQUAL(_e, _a, _eps)
#define ASSERT_FLT_GT(_a, _b) ASSERT_FLOAT_GREATER_THAN(_a, _b)
#define ASSERT_FLT_LT(_a, _b) ASSERT_FLOAT_LESS_THAN(_a, _b)
#define ASSERT_FLT_GE(_a, _b) ASSERT_FLOAT_GREATER_OR_EQUAL(_a, _b)
#define ASSERT_FLT_LE(_a, _b) ASSERT_FLOAT_LESS_OR_EQUAL(_a, _b)

#endif /* LFG_CTEST_HAS_FLOAT */

/*============================================================================
 *  64-bit Double Assertions (optional - requires LFG_CTEST_HAS_DOUBLE)
 *
 *  These assertions require fabs from math library.
 *  Some embedded platforms have hardware float but software-emulated double.
 *  Disable double separately with:
 *    cmake -DLFG_CTEST_ENABLE_DOUBLE=OFF
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_DOUBLE

/** Assert double values are equal within epsilon tolerance */
#define ASSERT_DOUBLE_EQUAL(_e, _a, _eps)                                                                              \
    lfg_ct_assert_double_equal_impl(                                                                                   \
            (double)(_e), (double)(_a), (double)(_eps), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

/** Assert double values are not equal (differ by more than epsilon) */
#define ASSERT_DOUBLE_NOT_EQUAL(_e, _a, _eps)                                                                          \
    lfg_ct_assert_double_not_equal_impl(                                                                               \
            (double)(_e), (double)(_a), (double)(_eps), __FILE__, __LINE__, LFG_CT_FUNCTION, STR(_a))

/* Shorthand aliases for double assertions */
#define ASSERT_DBL_EQ(_e, _a, _eps) ASSERT_DOUBLE_EQUAL(_e, _a, _eps)
#define ASSERT_DBL_NE(_e, _a, _eps) ASSERT_DOUBLE_NOT_EQUAL(_e, _a, _eps)

#endif /* LFG_CTEST_HAS_DOUBLE */

/* Less verbose aliases */
#define ASSERT_EQ(_e, _a) ASSERT_INT_EQUAL(_e, _a)
#define ASSERT_NE(_e, _a) ASSERT_INT_NOT_EQUAL(_e, _a)
#define ASSERT_GT(_a, _b) ASSERT_GREATER_THAN(_a, _b)
#define ASSERT_LT(_a, _b) ASSERT_LESS_THAN(_a, _b)
#define ASSERT_GE(_a, _b) ASSERT_GREATER_OR_EQUAL(_a, _b)
#define ASSERT_LE(_a, _b) ASSERT_LESS_OR_EQUAL(_a, _b)
#define ASSERT_NULL(_a) ASSERT_PTR_NULL(_a)
#define ASSERT_NOT_NULL(_a) ASSERT_PTR_NOT_NULL(_a)

/** Execute a suite of tests with optional setup/teardown hooks.
 *  Pass NULL for @p _setup or @p _teardown to skip that phase.
 *  Teardown runs even if the body or its assertions fail; if @p _setup
 *  itself triggers an assertion failure the body is skipped but teardown
 *  still runs. All three callbacks are @c void(void).
 */
#define lfg_ct_suite(_setup, _suite, _teardown) lfg_ct_suite_impl((_setup), (_suite), (_teardown), #_suite)

/** Execute a single unit test with optional setup/teardown hooks.
 *  Same lifecycle semantics as @ref lfg_ct_suite.
 */
#define lfg_ct_test(_setup, _test, _teardown) lfg_ct_test_impl((_setup), (_test), (_teardown), #_test)

/** Mark the current test as skipped, record @p _reason, and return from
 *  the test body immediately. Buckets the test as SKIP (separate from
 *  pass/fail). Legal in setup; the body is then not invoked but
 *  @c teardown still runs (same guarantee as a setup-failure path).
 *  Outside a test context this is a no-op with a warning to stderr.
 */
#define lfg_ct_skip(_reason) lfg_ct_skip_impl((_reason), __FILE__, __LINE__, LFG_CT_FUNCTION)

/** Mark the current test as expected-to-fail and record @p _reason.
 *  Unlike @ref lfg_ct_skip, this does @b not return from the body --
 *  the body runs to completion and the disposition is decided after
 *  the fact:
 *    - any assertion failure in the body -> XFAIL (separate bucket,
 *      not counted as failure);
 *    - no assertion failures -> XPASS (separate bucket, not counted
 *      as pass; warning only unless @c --strict-xpass is set).
 *  Repeated calls keep the latest reason (last one wins). Outside a
 *  test context this is a no-op with a warning to stderr.
 */
#define lfg_ct_xfail(_reason) lfg_ct_xfail_impl((_reason), __FILE__, __LINE__, LFG_CT_FUNCTION)

/*============================================================================
 *  Public API
 *==========================================================================*/

/** Return the framework version string as "M.m.p[+<sha>]".
 *  The underlying macro is LFG_CTEST_VERSION_FULL; use this function when
 *  you need the version at runtime (e.g. logging a test harness banner).
 */
const char *lfg_ct_version(void);

/** Mark the beginning of unit testing.
 */
void lfg_ct_start(void);

/** Mark the end of unit testing.
 */
void lfg_ct_end(void);

/** Parse command-line flags consumed by the runner.
 *
 *  Recognized flags:
 *   - @c --list                  : print every test/suite name encountered
 *                                  (one per line, on stdout) without executing
 *                                  their bodies. Setup/teardown of tests and
 *                                  suites are skipped; a suite's body is still
 *                                  invoked so the names of contained tests
 *                                  can be listed.
 *   - @c --filter \<glob\>         : only run entries whose registered name
 *                                  matches the shell-style glob (@c fnmatch(3)
 *                                  syntax: @c *, @c ?, @c [...]). Repeating
 *                                  the flag OR-combines the patterns. If a
 *                                  suite's name matches, every entry inside
 *                                  the suite is considered matched.
 *   - @c --filter-exclude \<glob\> : skip entries whose registered name
 *                                  matches the glob. Inverse of @c --filter,
 *                                  same repeat / OR semantics. Exclude wins
 *                                  on overlap with @c --filter.
 *   - @c --strict-xpass          : flip an otherwise-clean run that contains
 *                                  one or more @c xpass outcomes to a
 *                                  non-zero exit code. Default behavior is
 *                                  permissive (xpass is a warning).
 *
 *  An unmatched filter is not an error -- zero tests execute and the program
 *  exits 0. Unknown flags print a short usage message to stderr and produce
 *  a non-zero return value; callers should propagate that to @c main.
 *
 *  Calling @c lfg_ct_parse_args again replaces any previously parsed state.
 *
 *  @param argc Standard @c main argc.
 *  @param argv Standard @c main argv (referenced for filter glob strings;
 *              must outlive subsequent test/suite invocations).
 *  @return     0 on success, non-zero on a malformed or unknown flag.
 */
int lfg_ct_parse_args(int argc, char *argv[]);

/** Query whether the runner is currently in @c --list mode.
 *  @return 1 if @c --list was parsed, 0 otherwise.
 */
int lfg_ct_is_list_mode(void);

/** Query whether a hypothetical test/suite with @p name would be executed by
 *  the runner under the currently parsed filter/exclude rules.
 *
 *  Useful to short-circuit expensive setup outside the framework, or to test
 *  the matching logic in isolation. List-mode is treated as "no execution"
 *  for the purpose of this query (returns 0 when @c --list is active).
 *
 *  @param name Registered test/suite name to check.
 *  @return 1 if the entry would run, 0 if filtered, excluded, or in list mode.
 */
int lfg_ct_name_runs(const char *name);

/** Execute a suite of tests with optional setup/teardown hooks.
 *  See @ref lfg_ct_suite for lifecycle semantics. Pass @c NULL for any
 *  hook that should be skipped.
 */
void lfg_ct_suite_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);

/** Execute a single unit test with optional setup/teardown hooks.
 *  See @ref lfg_ct_test for lifecycle semantics. Pass @c NULL for any
 *  hook that should be skipped.
 */
void lfg_ct_test_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);

/** Implementation backing the @ref lfg_ct_skip macro. Use the macro --
 *  it captures @c __FILE__ / @c __LINE__ / function name automatically.
 *  Triggers a @c longjmp out of the current test or setup phase; the
 *  function does not return to its caller. Outside a test context this
 *  is a no-op with a warning to stderr.
 */
void lfg_ct_skip_impl(const char *reason, const char *file, int line, const char *function);

/** Implementation backing the @ref lfg_ct_xfail macro. Use the macro --
 *  it captures @c __FILE__ / @c __LINE__ / function name automatically.
 *  Records the expected-to-fail intent and returns normally; the body
 *  continues to run. Outside a test context this is a no-op with a
 *  warning to stderr.
 */
void lfg_ct_xfail_impl(const char *reason, const char *file, int line, const char *function);

/** Print test summary.
 */
void lfg_ct_print_summary(void);

/** Retrieve the return code for the main unittest function. This is intended
 * to be called by the main() function such as:
 *      int main(int argc, char *argv[]) {
 *          // ... <execute suites or tests> ...
 *          return lfg_ct_return();
 *      }
 */
int lfg_ct_return(void);

int lfg_ct_assert_false_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str);

int lfg_ct_assert_true_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str);

int lfg_ct_assert_int_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_int_not_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_uint_not_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint8_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_uint8_not_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint16_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint16_not_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint32_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint32_not_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_ptr_equal_impl(void *expected, void *actual, const char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_ptr_not_equal_impl(void *expected, void *actual, const char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_ptr_not_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_ptr_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_int8_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_int8_not_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str);

int lfg_ct_assert_int16_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_int16_not_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_int32_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_int32_not_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_int64_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str);

int lfg_ct_assert_int64_not_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint64_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_uint64_not_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_str_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_str_not_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_strn_equal_impl(const char *expected, const char *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_mem_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_mem_not_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_greater_than_impl(int a, int b, char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_less_than_impl(int a, int b, char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_greater_or_equal_impl(int a, int b, char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_less_or_equal_impl(int a, int b, char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_in_range_impl(
        int val, int min, int max, char *filename, int line_no, const char *function, const char *val_expr_str);

int lfg_ct_assert_bit_set_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num);

int lfg_ct_assert_bit_clear_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num);

int lfg_ct_assert_bits_set_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val);

int lfg_ct_assert_bits_clear_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val);

int lfg_ct_assert_fail_impl(char *filename, int line_no, const char *function, const char *message);

/*============================================================================
 *  32-bit Float Assertion Implementations (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_FLOAT

int lfg_ct_assert_float_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_float_not_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_float_greater_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_float_less_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_float_ge_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_float_le_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str);

int lfg_ct_assert_float_in_range_impl(float val, float min, float max, const char *filename, int line_no,
        const char *function, const char *val_expr_str);

#endif /* LFG_CTEST_HAS_FLOAT */

/*============================================================================
 *  64-bit Double Assertion Implementations (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_DOUBLE

int lfg_ct_assert_double_equal_impl(double expected, double actual, double epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str);

int lfg_ct_assert_double_not_equal_impl(double expected, double actual, double epsilon, const char *filename,
        int line_no, const char *function, const char *actual_expr_str);

#endif /* LFG_CTEST_HAS_DOUBLE */

/*============================================================================
 *  Self-Test API (internal only - requires LFG_CTEST_SELF_TEST)
 *
 *  These functions enable "expect failures" mode for testing the framework
 *  itself. Assertion failures during this mode are counted but don't affect
 *  the final test result. This allows the framework's own tests to verify
 *  that assertions correctly detect failures while still passing.
 *==========================================================================*/

#ifdef LFG_CTEST_SELF_TEST

/** Begin expect-failures mode. Assertion failures will be counted but won't
 *  affect test results. Resets the expected failure counter.
 */
void lfg_ct_expect_failures_begin(void);

/** End expect-failures mode and return the number of failures that occurred.
 *  @return Number of assertion failures captured during expect-failures mode.
 */
int lfg_ct_expect_failures_end(void);

/** Self-test accessor: running tally of tests bucketed as SKIP. */
int lfg_ct_self_skipped_count(void);

/** Self-test accessor: running tally of tests bucketed as XFAIL. */
int lfg_ct_self_xfailed_count(void);

/** Self-test accessor: running tally of tests bucketed as XPASS. */
int lfg_ct_self_xpassed_count(void);

/** Self-test accessor: running tally of tests bucketed as FAIL. */
int lfg_ct_self_failed_count(void);

/** Self-test accessor: running tally of failed assertions (post the
 *  xfail/skip absorption performed at test classification).
 */
int lfg_ct_self_assertions_failed(void);

/** Self-test toggle for @c --strict-xpass. Lets the framework's own tests
 *  flip the flag without going through @ref lfg_ct_parse_args (which
 *  would also reset filter state).
 */
void lfg_ct_self_set_strict_xpass(int enabled);

/** Self-test accessor: returns the exit code @ref lfg_ct_return would
 *  produce right now, without printing the summary.
 */
int lfg_ct_self_return_code(void);

/** Self-test accessor: the @c xfail reason captured at the most recent
 *  XFAIL/XPASS classification. Snapshotted before the per-test reset
 *  clears @c _current_xfail_reason, so self-tests can verify that
 *  repeated @c lfg_ct_xfail calls keep the latest reason ("last one
 *  wins"). Pointer is borrowed from the caller's argument; lifetime is
 *  the caller's responsibility.
 */
const char *lfg_ct_self_last_xfail_reason(void);

#endif /* LFG_CTEST_SELF_TEST */

#endif /* LFG_CTEST_H_ */
