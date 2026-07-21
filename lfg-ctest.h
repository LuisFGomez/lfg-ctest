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

/** Separator between the components of an addressable entry id.
 *  An entry's id is @c \<file\>::\<suite\>::\<test\> (tests) or
 *  @c \<file\>::\<suite\> (suites); @c \<file\> is the basename of the
 *  registration site's @c __FILE__, so ids are stable across build
 *  directories and out-of-tree builds.
 */
#define LFG_CT_ID_SEPARATOR "::"

/** Placeholder occupying the suite component of a test registered outside
 *  any @ref lfg_ct_suite, so a top-level test still has a well-formed
 *  three-component id (@c file.c::(none)::test_x) rather than a malformed
 *  @c file.c::::test_x.
 */
#define LFG_CT_ID_NO_SUITE "(none)"

#ifdef LFG_CT_COMPAT_3ARG

/* Deprecated single-release compatibility window for the pre-#35
 * registration-time setup/teardown signature. Opt-in via
 * -DLFG_CT_COMPAT_3ARG on the whole build (library + consumer TUs, so the
 * shared ABI signatures match). The 3-argument macros forward to the
 * retained internal lifecycle helper (setup -> body -> teardown, with the
 * old setup-failure and skip semantics). Slated for removal after one
 * release -- migrate call sites to body-owned setup/teardown. */
#warning "LFG_CT_COMPAT_3ARG enables the deprecated 3-argument lfg_ct_test/lfg_ct_suite registration; migrate to body-only registration -- this shim is removed after one release."

/** Deprecated. Execute a suite of tests with registration-time
 *  setup/teardown hooks. Pass @c NULL for @p _setup or @p _teardown to
 *  skip that phase; teardown runs even if the body or its assertions
 *  fail, and a setup assertion failure skips the body but still runs
 *  teardown. Compiled only under @c LFG_CT_COMPAT_3ARG; migrate to the
 *  body-only @c lfg_ct_suite(_suite).
 */
#define lfg_ct_suite(_setup, _suite, _teardown) \
    lfg_ct_suite_impl_at((_setup), (_suite), (_teardown), #_suite, __FILE__)

/** Deprecated. Execute a single unit test with registration-time
 *  setup/teardown hooks. Same lifecycle semantics as the deprecated
 *  @ref lfg_ct_suite. Compiled only under @c LFG_CT_COMPAT_3ARG; migrate
 *  to the body-only @c lfg_ct_test(_test).
 */
#define lfg_ct_test(_setup, _test, _teardown) \
    lfg_ct_test_impl_at((_setup), (_test), (_teardown), #_test, __FILE__)

#else /* body-only registration (default) */

/** Execute a suite of tests. Registration is body-only: the suite
 *  function is the single argument. Setup and teardown are plain static
 *  functions the suite body calls itself -- the framework binds no
 *  lifecycle hooks at registration time.
 */
#define lfg_ct_suite(_suite) lfg_ct_suite_impl_at((_suite), #_suite, __FILE__)

/** Execute a single unit test. Registration is body-only: the test
 *  function is the single argument. Setup and teardown are plain static
 *  functions the test body calls itself; a failure-path teardown after a
 *  soft assert, early @c return, or @ref lfg_ct_skip is expressed in the
 *  body (assertions are non-fatal and return their result).
 */
#define lfg_ct_test(_test) lfg_ct_test_impl_at((_test), #_test, __FILE__)

#endif /* LFG_CT_COMPAT_3ARG */

/** Mark the current test as skipped, record @p _reason, and return from
 *  the test body immediately. Buckets the test as SKIP (separate from
 *  pass/fail). Because the body owns its own teardown, code that must run
 *  on the skip path belongs before the @ref lfg_ct_skip call.
 *
 *  @b Scope: per-test only. Calling from a suite body or from any other
 *  point outside an active test context is a no-op with a warning to
 *  stderr -- suite-level skip is intentionally
 *  not supported and would need its own design pass before being
 *  exposed as a first-class feature.
 */
#define lfg_ct_skip(_reason) lfg_ct_skip_impl((_reason), __FILE__, __LINE__, LFG_CT_FUNCTION)

/** Skip the current test after first invoking @p _cleanup. Ergonomic sugar
 *  over the blessed convention's teardown-before-skip step: instead of the
 *  two lines
 *  @code
 *      teardown();
 *      lfg_ct_skip("reason");
 *  @endcode
 *  write the single call @c lfg_ct_skip_cleanup(teardown, "reason"). The
 *  @p _cleanup function pointer (a plain @c void(void), the shape a
 *  body-owned teardown already has) runs before the skip unwinds the body,
 *  so any resources acquired on the skip path are released. Semantically
 *  identical to the two-line form; it only closes the skip hole and does
 *  not touch the setup-failure case or a plain C @c return.
 *
 *  @p _cleanup may be @c NULL, in which case this behaves exactly like
 *  @ref lfg_ct_skip. Same per-test scope as @ref lfg_ct_skip: outside an
 *  active test context it is a no-op with a warning to stderr and
 *  @p _cleanup is @b not invoked.
 */
#define lfg_ct_skip_cleanup(_cleanup, _reason) lfg_ct_skip_cleanup_impl((_cleanup), (_reason), __FILE__, __LINE__, LFG_CT_FUNCTION)

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
 *
 *  Seeds @c rand(3) and announces the effective seed as
 *  @c "*** random seed is \<n\>" (suppressed under @c --list). The seed is
 *  the @c --seed value when one was parsed, otherwise a generated one
 *  spanning the full @c unsigned range. Call after
 *  @ref lfg_ct_parse_args so a supplied seed is in place.
 */
void lfg_ct_start(void);

/** Mark the end of unit testing.
 */
void lfg_ct_end(void);

/** Parse command-line flags consumed by the runner.
 *
 *  Recognized flags:
 *   - @c --list                  : print every test/suite id encountered
 *                                  (one per line, on stdout) without executing
 *                                  their bodies. Setup/teardown of tests and
 *                                  suites are skipped; a suite's body is still
 *                                  invoked so the ids of contained tests
 *                                  can be listed. Each line is directly usable
 *                                  as a @c --filter argument.
 *   - @c --filter \<glob\>         : only run entries whose id matches the
 *                                  shell-style glob (@c fnmatch(3) syntax:
 *                                  @c *, @c ?, @c [...]), which addresses
 *                                  exactly as many trailing @c :: -delimited
 *                                  components as it spells out. A glob with
 *                                  no @c :: is matched against the test name
 *                                  alone, so pre-id filters keep their
 *                                  meaning; @c * fills one component and
 *                                  never crosses a @c :: . Repeating
 *                                  the flag OR-combines the patterns. If a
 *                                  suite matches, every entry inside
 *                                  the suite is considered matched.
 *   - @c --filter-exclude \<glob\> : skip entries whose id matches the glob
 *                                  under the same component-depth rule.
 *                                  Inverse of @c --filter, same repeat / OR
 *                                  semantics.
 *                                  Exclude wins on overlap with @c --filter.
 *   - @c --strict-xpass          : flip an otherwise-clean run that contains
 *                                  one or more @c xpass outcomes to a
 *                                  non-zero exit code. Default behavior is
 *                                  permissive (xpass is a warning).
 *   - @c --seed \<n\>             : seed @c rand(3) with @c n instead of a
 *                                  generated value. @ref lfg_ct_start prints
 *                                  the effective seed either way, so reading
 *                                  the seed off a failing run and passing it
 *                                  back reproduces that run's @c rand()
 *                                  sequence. @c n is decimal and must fit an
 *                                  @c unsigned; 0 is a legal seed. Repeating
 *                                  the flag keeps the last value.
 *   - @c --rerun-failed          : run only the entries the previous run
 *                                  recorded as failed, restoring that run's
 *                                  seed. The set and the seed come from the
 *                                  state file every non-listing run writes
 *                                  (see @ref lfg_ct_state_path). Combines
 *                                  with @c --filter by intersection: the
 *                                  replayed set is the candidate pool and the
 *                                  filter narrows it. An explicit @c --seed
 *                                  outranks the persisted one. A missing,
 *                                  unreadable or malformed state file, and a
 *                                  state file none of whose keys still name a
 *                                  registered test, each produce a stderr
 *                                  diagnostic and a non-zero exit -- never a
 *                                  silent full-suite run.
 *   - @c --state-file \<path\>    : read and write the rerun state at
 *                                  @c path instead of the default
 *                                  @c .lfg-ctest-last in the current
 *                                  working directory. Repeating the flag
 *                                  keeps the last value.
 *   - @c -v / @c --verbose       : stream a per-test @c START line before
 *                                  each test body is dispatched and a
 *                                  per-test outcome line (@c PASS / @c FAIL /
 *                                  @c SKIP / @c XFAIL / @c XPASS) with
 *                                  elapsed milliseconds after the test
 *                                  classifies. Compatible with an installed
 *                                  reporter (e.g. JUnit-XML) -- the verbose
 *                                  output is fanned out alongside the
 *                                  user-installed reporter, not in place of
 *                                  it. Under @ref LFG_CT_ISOLATE_FORK the
 *                                  parent serialises both the start and the
 *                                  outcome line, so no banner interleaves
 *                                  with another forked test's stdout.
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

/** Query whether verbose-mode streaming is currently active.
 *  @return 1 if @c -v / @c --verbose was parsed, 0 otherwise.
 */
int lfg_ct_is_verbose(void);

/** Query whether a seed was supplied via @c --seed.
 *  Distinct from the seed value because 0 is a legal seed.
 *  @return 1 if @c --seed was parsed, 0 otherwise.
 */
int lfg_ct_is_seed_set(void);

/** Query the seed supplied via @c --seed.
 *  @return The parsed seed, or 0 when no @c --seed was given (check
 *          @ref lfg_ct_is_seed_set to tell the two apart).
 */
unsigned lfg_ct_get_seed(void);

/** Query whether @c --rerun-failed was parsed.
 *  @return 1 if the run is replaying the previous run's failures, 0 otherwise.
 */
int lfg_ct_is_rerun_failed(void);

/** Path of the rerun state file this run reads and writes.
 *
 *  Every non-listing run rewrites this file from @ref lfg_ct_print_summary
 *  with a format marker, the seed the run used, and one
 *  @c \<file\>::\<suite\>::\<test\> id per failed test -- the same ids
 *  @c --list emits. The records come from the runner's own classification,
 *  not from parsed output, so they are identical under fork isolation and
 *  with stdout redirected. @c --list neither writes nor truncates it.
 *
 *  @return The @c --state-file value, or @c ".lfg-ctest-last" by default.
 *          Borrowed from @c argv when overridden; never @c NULL.
 */
const char *lfg_ct_state_path(void);

/** Query whether a hypothetical test/suite with @p name would be executed by
 *  the runner under the currently parsed filter/exclude rules.
 *
 *  Useful to short-circuit expensive setup outside the framework, or to test
 *  the matching logic in isolation. List-mode is treated as "no execution"
 *  for the purpose of this query (returns 0 when @c --list is active).
 *
 *  @note Cannot answer a *qualified* filter. The id is built from the
 *        ambient file/suite batons, which are absent when this is called
 *        from @c main() -- the documented usage -- so the id is
 *        @c (none)::(none)::name. Under @c --filter
 *        @c 'alpha.c::suite_one::test_x' this returns 0 even though the
 *        test runs. Use @ref lfg_ct_test_runs where that matters.
 *
 *  @param name Registered test/suite name to check.
 *  @return 1 if the entry would run, 0 if filtered, excluded, or in list mode.
 */
int lfg_ct_name_runs(const char *name);

/** Qualified form of @ref lfg_ct_name_runs: build the entry's full id from
 *  @p file / @p suite / @p name and evaluate the same filter rules against
 *  it -- each glob addressing as many trailing @c :: components as it
 *  spells out.
 *
 *  Additive -- @ref lfg_ct_name_runs keeps its bare-name meaning. Prefer
 *  the @ref lfg_ct_test_runs macro, which fills @p file in for you.
 *
 *  @param file  Registration site's @c __FILE__; basename only. @c NULL
 *               yields @ref LFG_CT_ID_NO_SUITE in that position.
 *  @param suite Enclosing suite name, or @c NULL to use the suite currently
 *               on the registration stack (@ref LFG_CT_ID_NO_SUITE at
 *               top level).
 *  @param name  Registered test name to check.
 *  @return 1 if the entry would run, 0 if filtered, excluded, or in list mode.
 */
int lfg_ct_id_runs(const char *file, const char *suite, const char *name);

/** Ask whether the test named @p _name, registered in the current file and
 *  the enclosing suite, would run under the parsed filters. Captures
 *  @c __FILE__ so the query sees the same id @c --list prints.
 */
#define lfg_ct_test_runs(_name) lfg_ct_id_runs(__FILE__, NULL, (_name))

/** Render an entry's addressable id into @p buf, exactly as @c --list emits
 *  it. Pass @c NULL for @p test to render a suite id (@c \<file\>::\<suite\>).
 *
 *  Truncation is silent: an id longer than @p cap is cut short rather than
 *  failing the run, which costs only addressability of pathologically long
 *  names. 320 bytes is comfortably above any real filename/identifier pair.
 *
 *  @param buf   Destination; always NUL-terminated when @p cap > 0. May be
 *               @c NULL with a @p cap of 0 to size a buffer from the return
 *               value without writing anything.
 *  @param cap   Size of @p buf in bytes.
 *  @param file  Registration site's @c __FILE__; only its basename is used.
 *               @c NULL yields @ref LFG_CT_ID_NO_SUITE in that position.
 *  @param suite Enclosing suite name, or @c NULL / empty for
 *               @ref LFG_CT_ID_NO_SUITE.
 *  @param test  Test name, or @c NULL to render a suite id.
 *  @return Length the id needed, excluding the terminator -- snprintf(3)
 *          semantics. A result @c >= @p cap means @p buf holds a truncated
 *          id; the untruncated length is the return value.
 */
size_t lfg_ct_format_id(char *buf, size_t cap, const char *file, const char *suite, const char *test);

#ifdef LFG_CT_COMPAT_3ARG

/** Deprecated 3-argument backing entry for @ref lfg_ct_suite under the
 *  @c LFG_CT_COMPAT_3ARG compatibility window. Pass @c NULL for any hook
 *  that should be skipped.
 */
void lfg_ct_suite_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);

/** Deprecated 3-argument backing entry for @ref lfg_ct_test under the
 *  @c LFG_CT_COMPAT_3ARG compatibility window. Pass @c NULL for any hook
 *  that should be skipped.
 */
void lfg_ct_test_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);

/** Deprecated 3-argument file-aware backing entry for @ref lfg_ct_suite.
 *  @see lfg_ct_suite_impl_at (body-only form) for the @p file semantics.
 */
void lfg_ct_suite_impl_at(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        const char *file);

/** Deprecated 3-argument file-aware backing entry for @ref lfg_ct_test.
 *  @see lfg_ct_test_impl_at (body-only form) for the @p file semantics.
 */
void lfg_ct_test_impl_at(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        const char *file);

#else /* body-only registration (default) */

/** Execute a suite of tests. See @ref lfg_ct_suite; the suite body owns
 *  any setup/teardown it needs.
 *
 *  Retained bare-name entry: equivalent to @ref lfg_ct_suite_impl_at with a
 *  @c NULL @p file, so the id's file component becomes
 *  @ref LFG_CT_ID_NO_SUITE. Prefer the @c _at form (what @ref lfg_ct_suite
 *  expands to) so the entry is addressable by its file.
 */
void lfg_ct_suite_impl(void (*fn)(void), const char *name);

/** Execute a single unit test. See @ref lfg_ct_test; the test body owns
 *  any setup/teardown it needs.
 *
 *  Retained bare-name entry: equivalent to @ref lfg_ct_test_impl_at with a
 *  @c NULL @p file. Prefer the @c _at form.
 */
void lfg_ct_test_impl(void (*fn)(void), const char *name);

/** Execute a suite of tests, recording @p file as the id's file component.
 *  This is what @ref lfg_ct_suite expands to; the macro supplies
 *  @c __FILE__ so consumers never write the argument themselves.
 *
 *  @param fn   Suite body.
 *  @param name Registered suite name.
 *  @param file Registration site's @c __FILE__; only its basename is used.
 *              @c NULL yields @ref LFG_CT_ID_NO_SUITE in that position.
 */
void lfg_ct_suite_impl_at(void (*fn)(void), const char *name, const char *file);

/** Execute a single unit test, recording @p file as the id's file component.
 *  This is what @ref lfg_ct_test expands to.
 *
 *  @param fn   Test body.
 *  @param name Registered test name.
 *  @param file Registration site's @c __FILE__; only its basename is used.
 *              @c NULL yields @ref LFG_CT_ID_NO_SUITE in that position.
 */
void lfg_ct_test_impl_at(void (*fn)(void), const char *name, const char *file);

#endif /* LFG_CT_COMPAT_3ARG */

/** Implementation backing the @ref lfg_ct_skip macro. Use the macro --
 *  it captures @c __FILE__ / @c __LINE__ / function name automatically.
 *  Triggers a @c longjmp out of the current test or setup phase; the
 *  function does not return to its caller. Outside a test context this
 *  is a no-op with a warning to stderr.
 */
void lfg_ct_skip_impl(const char *reason, const char *file, int line, const char *function);

/** Implementation backing the @ref lfg_ct_skip_cleanup macro. Use the
 *  macro -- it captures @c __FILE__ / @c __LINE__ / function name
 *  automatically. Invokes @p cleanup (unless @c NULL) before triggering a
 *  @c longjmp out of the current test or setup phase; the function does not
 *  return to its caller. Outside a test context this is a no-op with a
 *  warning to stderr and @p cleanup is not invoked.
 */
void lfg_ct_skip_cleanup_impl(void (*cleanup)(void), const char *reason, const char *file, int line, const char *function);

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

/*============================================================================
 *  Isolation modes
 *
 *  Selects how the runner dispatches the body of every subsequent
 *  @ref lfg_ct_test invocation. Default is @ref LFG_CT_ISOLATE_NONE
 *  (in-process; historical behavior). @ref LFG_CT_ISOLATE_FORK runs
 *  every test body in a fresh @c fork(2)'d child, so a crashing or
 *  hung test does not take down the runner and does not pollute
 *  state for the next test. See @c docs/api.md for the full contract.
 *
 *  @c LFG_CT_ISOLATE_FORK is part of the public ABI unconditionally,
 *  regardless of build flags. Whether it can be selected at runtime
 *  depends on the platform (Unix-family @c fork(2) only) and on the
 *  compile-time @c LFG_CT_DISABLE_FORK opt-out -- both confined to
 *  the implementation TU. @ref lfg_ct_set_isolation returns a non-zero
 *  error when an unavailable mode is requested; the previously
 *  configured mode is left unchanged. No silent fallback.
 *==========================================================================*/

typedef enum
{
    LFG_CT_ISOLATE_NONE = 0, /**< In-process dispatch (default, historical). */
    LFG_CT_ISOLATE_FORK = 1  /**< Fork-per-test isolation. Unix-family only. */
} lfg_ct_isolation_t;

/** Select the dispatch mode used for subsequent @ref lfg_ct_test calls.
 *  Returns 0 on success, non-zero if @p mode is recognised but the
 *  runtime support is unavailable (e.g. @ref LFG_CT_ISOLATE_FORK on a
 *  non-Unix host, or any consumer build with @c LFG_CT_DISABLE_FORK
 *  defined). On error the previously configured mode is unchanged.
 */
int lfg_ct_set_isolation(lfg_ct_isolation_t mode);

/** Query the currently configured isolation mode. */
lfg_ct_isolation_t lfg_ct_get_isolation(void);

/** Configure the per-test timeout used when @ref LFG_CT_ISOLATE_FORK is
 *  active. A child that has not exited within @p timeout_ms milliseconds
 *  is sent @c SIGKILL by the parent and the test is recorded as failed
 *  with a timeout diagnostic. @p timeout_ms of 0 disables the timeout
 *  (parent blocks until the child reaps naturally). Default is 0.
 *
 *  No-op when the active isolation is not @ref LFG_CT_ISOLATE_FORK; the
 *  setting persists across mode changes.
 */
void lfg_ct_set_fork_timeout_ms(unsigned timeout_ms);

/** Query the current fork-mode per-test timeout (milliseconds). */
unsigned lfg_ct_get_fork_timeout_ms(void);

/*============================================================================
 *  Reporter callback contract
 *
 *  Pluggable hook that lets a downstream package observe each test's
 *  classified outcome and run-end without growing the core's surface for
 *  every conceivable report format. The runner fires the registered
 *  reporter (if any) from one site -- the classification block at the end
 *  of every test -- and a second site at the tail of @ref lfg_ct_print_summary.
 *
 *  Default reporter is @c NULL (no-op). One reporter slot, by design --
 *  fan-out is a downstream concern; if you need TAP + JUnit + something
 *  else, write a reporter that demuxes. Keeping the contract single-slot
 *  is what stops the core from growing a registry it doesn't need.
 *
 *  The contrib JUnit-XML emitter under @c contrib/junit-xml/ is the
 *  reference consumer; consult it for the wiring pattern.
 *==========================================================================*/

/** Classified outcome bucket. Mirrors the runner's internal disposition
 *  + assertion-failure tally; one of these is set on every executed test.
 */
typedef enum
{
    LFG_CT_PASSED,
    LFG_CT_FAILED,
    LFG_CT_SKIPPED,
    LFG_CT_XFAIL,
    LFG_CT_XPASS
} lfg_ct_outcome_t;

/** One classified test, handed to the reporter's @c on_record callback at
 *  the end of @ref lfg_ct_test_impl.
 *
 *  String fields are @b borrowed -- valid for the duration of the callback
 *  only. A reporter that buffers records across tests must copy these
 *  strings (the runner overwrites @c message at the next test's entry).
 */
typedef struct
{
    const char *suite_name;  /**< Enclosing @ref lfg_ct_suite, or @c NULL for top-level tests. */
    const char *test_name;   /**< Registered test name; never @c NULL. */
    double time_sec;         /**< Elapsed wall-clock seconds (fractional). */
    lfg_ct_outcome_t outcome;
    const char *message;     /**< Outcome-specific annotation:
                              *   - @c FAILED: first assertion-failure text,
                              *     formatted as @c "file:line: in fn(): expr"
                              *   - @c SKIPPED: the reason from @c lfg_ct_skip
                              *   - @c XFAIL / @c XPASS: the reason from @c lfg_ct_xfail
                              *   - @c PASSED: @c NULL
                              */
} lfg_ct_record_t;

/** Reporter callback bundle. All function pointers are optional (NULL =
 *  ignored). @c userdata is opaquely forwarded; the runner does not
 *  interpret it.
 *
 *  Field order is part of the contract: positional initialisers like
 *  @c {on_record, on_run_complete, userdata} written against the
 *  original three-field bundle stay valid -- new callbacks live after
 *  @c userdata and default to @c NULL.
 */
typedef struct
{
    /** Invoked once per classified test, after the runner has bucketed
     *  the test but before the next test begins. */
    void (*on_record)(const lfg_ct_record_t *record, void *userdata);

    /** Invoked once at the tail of @ref lfg_ct_print_summary. Use this to
     *  flush a buffering reporter (e.g. the contrib JUnit emitter writes
     *  its XML file here). */
    void (*on_run_complete)(void *userdata);

    void *userdata;

    /** Invoked once per test immediately before its body is dispatched,
     *  fired from the parent side of @ref lfg_ct_test_impl after the
     *  list-mode / filter / exclude gates have admitted the test.
     *  Strings are borrowed for the duration of the callback. Outcome,
     *  elapsed time, and message are not yet known and are reported
     *  through @c on_record at the end of the test.
     *
     *  Under @ref LFG_CT_ISOLATE_FORK, this fires in the parent before
     *  @c fork(2) so the start banner is serialised relative to the
     *  forked child's stdout and the matching @c on_record at test
     *  completion. The child's in-process reporter is the fork TU's
     *  capture reporter, which does not re-fire @c on_test_start. */
    void (*on_test_start)(const char *suite_name, const char *test_name, void *userdata);
} lfg_ct_reporter_t;

/** Install or replace the active reporter. @p reporter is borrowed (the
 *  caller must keep the struct alive across runner calls); pass @c NULL
 *  to disable. The runner stores the pointer, not a copy -- file-static
 *  reporter structs in a downstream package are the intended usage.
 */
void lfg_ct_set_reporter(const lfg_ct_reporter_t *reporter);

/** Retrieve the return code for the main unittest function. This is intended
 * to be called by the main() function such as:
 *      int main(int argc, char *argv[]) {
 *          // ... <execute suites or tests> ...
 *          return lfg_ct_return();
 *      }
 */
int lfg_ct_return(void);

/** Running tally of failed assertions, promoted to the public ABI so a
 *  test body can detect its own failures without per-assert branching.
 *
 *  Every @c ASSERT_* is non-fatal (record-and-continue) and there is no
 *  @c REQUIRE-style fatal variant, so this accessor is the supported way
 *  to fail-fast out of a region of assertions: snapshot it at the top of
 *  the region and compare.
 *
 *  @code
 *  void test_foo(void)
 *  {
 *      size_t baseline = lfg_ct_failure_count();
 *      for (size_t i = 0; i < 1000; ++i)
 *      {
 *          run_bar_tests();                                // many non-fatal asserts
 *          if (lfg_ct_failure_count() > baseline) break;   // fail-fast
 *      }
 *  }
 *  @endcode
 *
 *  @par Boundary semantics
 *  The value is the @e global running count, not a per-test slice. It is
 *  strictly monotonic only @e within a single test or setup body: at
 *  classification the runner absorbs a completed test's assertion failures
 *  back out of the tally for @c SKIP / @c XFAIL / @c XPASS outcomes, so the
 *  count can move down at a test boundary. Always snapshot-and-compare
 *  inside one body; never rely on cross-test monotonicity.
 *
 *  @return Number of failed assertions recorded so far.
 */
size_t lfg_ct_failure_count(void);

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

/** Self-test hook: record @p file / @p suite / @p test into this run's
 *  rerun failure set exactly as the classifier does on a FAILED outcome.
 *  Lets the framework's own tests build a failure set without a genuine
 *  failure (which expect-failures mode would suppress).
 */
void lfg_ct_self_rerun_note_failure(const char *file, const char *suite, const char *test);

/** Self-test hook: drop the accumulated rerun failure set. Also how the
 *  self-test binaries keep their own end-of-run state file honest after
 *  driving intentional failures.
 */
void lfg_ct_self_rerun_reset(void);

/** Self-test accessor: number of ids in this run's rerun failure set. */
int lfg_ct_self_rerun_recorded_count(void);

/** Self-test accessor: number of keys loaded from the state file under
 *  @c --rerun-failed.
 */
int lfg_ct_self_rerun_key_count(void);

/** Self-test accessor: how many loaded keys no registered test has
 *  claimed so far this run.
 */
int lfg_ct_self_rerun_unresolved_count(void);

/** Self-test hook: write the current rerun failure set to @p path under
 *  @p seed, bypassing @ref lfg_ct_print_summary.
 *  @return 0 on success, -1 if the file could not be written.
 */
int lfg_ct_self_state_write(const char *path, unsigned seed);

#endif /* LFG_CTEST_SELF_TEST */

#endif /* LFG_CTEST_H_ */
