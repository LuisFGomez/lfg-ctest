/**
 * @file
 * @brief       lfg-ctest unit testing API.
 */

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fnmatch.h>
#if defined(LFG_CTEST_HAS_FLOAT) || defined(LFG_CTEST_HAS_DOUBLE)
#include <math.h>
#endif
#include "lfg-ctest.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

/*============================================================================
 *  Private Function Prototypes
 *==========================================================================*/

/*============================================================================
 *  Variables
 *==========================================================================*/

static int _assertions_executed = 0;
static int _assertions_failed = 0;
static int _assertions_passed = 0;
static int _tests_executed = 0;
static int _tests_failed = 0;
static int _tests_passed = 0;
static int _current_test_failures = 0;
static int _current_suite_failures = 0;

/* lfg_ct_parse_args() runtime state. The glob arrays hold borrowed pointers
 * into the caller's argv (which has program lifetime under standard main()
 * usage), so no string copies or frees are needed. */
#define LFG_CT_FILTER_MAX 64

static int _list_mode = 0;
static const char *_filter_globs[LFG_CT_FILTER_MAX];
static int _filter_glob_count = 0;
static const char *_exclude_globs[LFG_CT_FILTER_MAX];
static int _exclude_glob_count = 0;
/* Depth of nested suites whose name matched the filter. Tests inside such
 * a suite inherit the filter-pass (so "--filter suite_foo*" runs every test
 * the suite holds without each test having to match individually). */
static int _filter_inherited_depth = 0;

/*============================================================================
 *  Self-Test Support (internal only)
 *
 *  When LFG_CTEST_SELF_TEST is defined, enables "expect failures" mode for
 *  testing the framework itself. Failures during this mode are counted but
 *  don't affect the final test result.
 *==========================================================================*/

#ifdef LFG_CTEST_SELF_TEST
static int _expect_failures_mode = 0;
static int _expected_failures_count = 0;

/* In expect-failures mode, count the failure but don't affect test results */
#define RECORD_FAILURE()                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (_expect_failures_mode)                                                                                     \
        {                                                                                                              \
            _expected_failures_count++;                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            _current_test_failures++;                                                                                  \
            _assertions_failed++;                                                                                      \
        }                                                                                                              \
    } while (0)

#define RECORD_PASS()                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!_expect_failures_mode)                                                                                    \
        {                                                                                                              \
            _assertions_passed++;                                                                                      \
        }                                                                                                              \
    } while (0)

#else
/* Normal mode: always record failures/passes */
#define RECORD_FAILURE()                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        _current_test_failures++;                                                                                      \
        _assertions_failed++;                                                                                          \
    } while (0)

#define RECORD_PASS()                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        _assertions_passed++;                                                                                          \
    } while (0)

#endif /* LFG_CTEST_SELF_TEST */

/*============================================================================
 *  Public API
 *==========================================================================*/

const char *
lfg_ct_version(void)
{
    return LFG_CTEST_VERSION_FULL;
}

void lfg_ct_start(void)
{
    unsigned rand_seed = time(NULL) % 1000;
    /* List mode wants stdout to be a clean newline-separated list of names;
     * skip the banner and the seed announcement. srand still runs so any
     * deterministic-by-seed test behavior stays consistent if the user
     * combines --list with other operations. */
    if (!_list_mode)
    {
        printf("*** begin unit test\r\n");
        printf("*** random seed is %u\r\n", rand_seed);
    }
    srand(rand_seed);
}

void lfg_ct_end(void)
{
}

/* Match @p name against any glob in @p globs (length @p count) using
 * fnmatch(3) shell-glob semantics. */
static int
_glob_list_matches(const char *name, const char *const *globs, int count)
{
    int i;

    if (NULL == name)
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        if (0 == fnmatch(globs[i], name, 0))
        {
            return 1;
        }
    }
    return 0;
}

static void
_filter_state_reset(void)
{
    _list_mode = 0;
    _filter_glob_count = 0;
    _exclude_glob_count = 0;
    _filter_inherited_depth = 0;
}

static void
_filter_print_usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [options]\r\n"
            "  --list                   List registered test/suite names and exit 0\r\n"
            "  --filter <glob>          Run only entries whose name matches <glob>\r\n"
            "  --filter-exclude <glob>  Skip entries whose name matches <glob>\r\n"
            "Globs use shell-style syntax (*, ?, [...]) via fnmatch(3).\r\n"
            "--filter and --filter-exclude may be repeated; exclude wins on overlap.\r\n",
            progname ? progname : "test");
}

int
lfg_ct_parse_args(int argc, char *argv[])
{
    const char *progname;
    int i;

    _filter_state_reset();
    progname = (argc > 0 && argv && argv[0]) ? argv[0] : "test";

    for (i = 1; i < argc; i++)
    {
        const char *a = argv[i];

        const char **slot = NULL;
        int *count = NULL;

        if (0 == strcmp(a, "--list"))
        {
            _list_mode = 1;
            continue;
        }
        if (0 == strcmp(a, "--filter"))
        {
            slot = _filter_globs;
            count = &_filter_glob_count;
        }
        else if (0 == strcmp(a, "--filter-exclude"))
        {
            slot = _exclude_globs;
            count = &_exclude_glob_count;
        }
        else
        {
            fprintf(stderr, "%s: unrecognized option: %s\r\n", progname, a);
            _filter_print_usage(progname);
            _filter_state_reset();
            return -1;
        }

        if (i + 1 >= argc)
        {
            fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
            _filter_print_usage(progname);
            _filter_state_reset();
            return -1;
        }
        if (*count >= LFG_CT_FILTER_MAX)
        {
            fprintf(stderr, "%s: too many %s patterns (max %d)\r\n", progname, a, LFG_CT_FILTER_MAX);
            _filter_state_reset();
            return -1;
        }
        slot[(*count)++] = argv[++i];
    }
    return 0;
}

int
lfg_ct_is_list_mode(void)
{
    return _list_mode ? 1 : 0;
}

/* Pure name predicate -- ignores list mode (which suppresses execution
 * regardless of filter state). */
static int
_filter_admits(const char *name)
{
    if (_exclude_glob_count > 0 && _glob_list_matches(name, _exclude_globs, _exclude_glob_count))
    {
        return 0;
    }
    if (0 == _filter_glob_count || _filter_inherited_depth > 0)
    {
        return 1;
    }
    return _glob_list_matches(name, _filter_globs, _filter_glob_count);
}

int
lfg_ct_name_runs(const char *name)
{
    if (_list_mode)
    {
        return 0;
    }
    return _filter_admits(name);
}

/* Setup-failure detector: any assertion failure flips
 * _assertions_failed (normal mode) or _expected_failures_count
 * (expect-failures self-test mode). Summing both lets the lifecycle
 * helper notice setup failures regardless of which mode is active.
 */
static int
_lifecycle_failure_total(void)
{
#ifdef LFG_CTEST_SELF_TEST
    return _assertions_failed + _expected_failures_count;
#else
    return _assertions_failed;
#endif
}

/* Shared setup -> body -> teardown lifecycle. Teardown runs whenever
 * provided, even if the body or its assertions failed. If setup itself
 * fails an assertion the body is skipped but teardown still runs --
 * setup may have partially acquired resources before failing.
 */
static void
_lfg_ct_run_lifecycle(void (*setup)(void), void (*body)(void), void (*teardown)(void))
{
    int setup_failures_before = 0;
    int setup_failed = 0;

    if (setup)
    {
        setup_failures_before = _lifecycle_failure_total();
        setup();
        setup_failed = (_lifecycle_failure_total() > setup_failures_before);
    }

    if (!setup_failed && body)
    {
        body();
    }

    if (teardown)
    {
        teardown();
    }
}

void lfg_ct_suite_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name)
{
    int suite_assertions_failed_before;
    int inherited_pushed = 0;

    if (_list_mode)
    {
        /* Print the suite name and recurse into the body without setup/
         * teardown so contained lfg_ct_test() calls can list themselves. */
        printf("%s\r\n", name);
        if (fn)
        {
            fn();
        }
        return;
    }

    /* Exclude is checked first and is decisive: a matched suite is skipped
     * entirely, no descent. Filter is checked second; a non-match still
     * descends so inner tests can be evaluated individually. A suite that
     * matches the filter propagates the pass to every descendant. */
    if (_exclude_glob_count > 0 && _glob_list_matches(name, _exclude_globs, _exclude_glob_count))
    {
        return;
    }
    if (_filter_glob_count > 0 && 0 == _filter_inherited_depth
            && _glob_list_matches(name, _filter_globs, _filter_glob_count))
    {
        _filter_inherited_depth++;
        inherited_pushed = 1;
    }

    /* Suite-level setup/teardown failures aren't owned by any test, so
     * track real assertion failures across the whole suite scope to
     * decide whether to print "suite FAILURE". Use _assertions_failed
     * (not _lifecycle_failure_total) so the print stays quiet when
     * self-tests intentionally drive suite-level failures inside
     * expect-failures mode. */
    suite_assertions_failed_before = _assertions_failed;

    _current_suite_failures = 0;
    _lfg_ct_run_lifecycle(setup, fn, teardown);
    if (_current_suite_failures > 0 || _assertions_failed > suite_assertions_failed_before)
    {
        printf("*** suite FAILURE: %s\r\n", name);
    }

    if (inherited_pushed)
    {
        _filter_inherited_depth--;
    }
}

void lfg_ct_test_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name)
{
    if (_list_mode)
    {
        printf("%s\r\n", name);
        return;
    }
    if (!_filter_admits(name))
    {
        return;
    }

    _tests_executed++;
    _current_test_failures = 0;
    _lfg_ct_run_lifecycle(setup, fn, teardown);
    if (_current_test_failures > 0)
    {
        _current_suite_failures++;
        _tests_failed++;
        printf("*** test FAILURE: %s\r\n", name);
    }
    else
    {
        _tests_passed++;
    }
}

void lfg_ct_print_summary(void)
{
    if (_list_mode)
    {
        /* No tests ran; reporting "0 assertions / 0 tests" is misleading. */
        return;
    }
    printf("*** Executed %d assertions in %d tests. Failures: %d\r\n"
           "*** Testing complete. Result: %s\r\n",
            _assertions_executed, _tests_executed, _tests_failed, _tests_failed ? "FAIL" : "PASS");
}

int lfg_ct_return(void)
{
    return -_tests_failed;
}

/*============================================================================
 *  Self-Test API (only available when LFG_CTEST_SELF_TEST is defined)
 *==========================================================================*/

#ifdef LFG_CTEST_SELF_TEST

void lfg_ct_expect_failures_begin(void)
{
    _expect_failures_mode = 1;
    _expected_failures_count = 0;
}

int lfg_ct_expect_failures_end(void)
{
    _expect_failures_mode = 0;
    return _expected_failures_count;
}

#endif /* LFG_CTEST_SELF_TEST */

int lfg_ct_assert_false_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str)
{
    _assertions_executed++;
    if (condition)
    {
        printf("*** %s: %u: FAILURE: in %s(): %s should be false\r\n", filename, line_no, function, condition_str);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_true_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str)
{
    _assertions_executed++;
    if (!condition)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should be true\r\n", filename, line_no, function, condition_str);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int_not_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%u) should equal %u\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint_not_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %u\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint8_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%02X) should equal 0x%02X\r\n", filename, line_no, function,
                actual_expr_str, actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint8_not_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%02X\r\n", filename, line_no, function,
                actual_expr_str, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint16_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%04X) should equal 0x%04X\r\n", filename, line_no, function,
                actual_expr_str, actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint16_not_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%04X\r\n", filename, line_no, function,
                actual_expr_str, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint32_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should equal 0x%08X\r\n", filename, line_no, function,
                actual_expr_str, actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint32_not_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%08X\r\n", filename, line_no, function,
                actual_expr_str, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_equal_impl(void *expected, void *actual, const char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_not_equal_impl(void *expected, void *actual, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %p\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_not_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not be NULL\r\n", filename, line_no, function, actual_expr_str);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should be NULL but is %p\r\n", filename, line_no, function,
                actual_expr_str, actual);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int8_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int8_not_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int16_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int16_not_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int32_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n", filename, line_no, function, actual_expr_str,
                actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int32_not_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n", filename, line_no, function, actual_expr_str,
                expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int64_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%lld) should equal %lld\r\n", filename, line_no, function,
                actual_expr_str, (long long)actual, (long long)expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int64_not_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %lld\r\n", filename, line_no, function,
                actual_expr_str, (long long)expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint64_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%016llX) should equal 0x%016llX\r\n", filename, line_no, function,
                actual_expr_str, (unsigned long long)actual, (unsigned long long)expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint64_not_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%016llX\r\n", filename, line_no, function,
                actual_expr_str, (unsigned long long)expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_str_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n", filename, line_no,
                    function, actual_expr_str, (void *)actual, (void *)expected);
            RECORD_FAILURE();
            return -1;
        }
    }
    else if (strcmp(expected, actual) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (\"%s\") should equal \"%s\"\r\n", filename, line_no, function,
                actual_expr_str, actual, expected);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_str_not_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) || (expected != NULL && actual != NULL && strcmp(expected, actual) == 0))
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal \"%s\"\r\n", filename, line_no, function,
                actual_expr_str, expected ? expected : "(null)");
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_strn_equal_impl(const char *expected, const char *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n", filename, line_no,
                    function, actual_expr_str, (void *)actual, (void *)expected);
            RECORD_FAILURE();
            return -1;
        }
    }
    else if (strncmp(expected, actual, n) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (first %zu chars) does not match expected\r\n", filename, line_no,
                function, actual_expr_str, n);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_mem_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n", filename, line_no,
                    function, actual_expr_str, actual, expected);
            RECORD_FAILURE();
            return -1;
        }
    }
    else if (memcmp(expected, actual, n) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s memory (%zu bytes) does not match expected\r\n", filename, line_no,
                function, actual_expr_str, n);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_mem_not_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) ||
            (expected != NULL && actual != NULL && memcmp(expected, actual, n) == 0))
    {
        printf("*** %s: %u: FAILURE in %s(): %s memory (%zu bytes) should not match\r\n", filename, line_no, function,
                actual_expr_str, n);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_greater_than_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a > b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be > %s (%d)\r\n", filename, line_no, function, a_expr_str,
                a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_less_than_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a < b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be < %s (%d)\r\n", filename, line_no, function, a_expr_str,
                a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_greater_or_equal_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a >= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be >= %s (%d)\r\n", filename, line_no, function, a_expr_str,
                a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_less_or_equal_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a <= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be <= %s (%d)\r\n", filename, line_no, function, a_expr_str,
                a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_in_range_impl(
        int val, int min, int max, char *filename, int line_no, const char *function, const char *val_expr_str)
{
    _assertions_executed++;
    if (!(val >= min && val <= max))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be in range [%d, %d]\r\n", filename, line_no, function,
                val_expr_str, val, min, max);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bit_set_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num)
{
    _assertions_executed++;
    if (!(val & (1u << bit)))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bit %u set\r\n", filename, line_no, function,
                val_expr_str, val, bit_num);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bit_clear_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num)
{
    _assertions_executed++;
    if (val & (1u << bit))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bit %u clear\r\n", filename, line_no, function,
                val_expr_str, val, bit_num);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bits_set_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val)
{
    _assertions_executed++;
    if ((val & mask) != mask)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bits 0x%08X set\r\n", filename, line_no, function,
                val_expr_str, val, mask_val);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bits_clear_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val)
{
    _assertions_executed++;
    if (val & mask)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bits 0x%08X clear\r\n", filename, line_no,
                function, val_expr_str, val, mask_val);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_fail_impl(char *filename, int line_no, const char *function, const char *message)
{
    _assertions_executed++;
    printf("*** %s: %u: FAILURE in %s(): %s\r\n", filename, line_no, function, message ? message : "Explicit failure");
    RECORD_FAILURE();
    return -1;
}

/*============================================================================
 *  32-bit Float Assertions (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_FLOAT

int lfg_ct_assert_float_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    float diff = fabsf(expected - actual);
    if (diff > epsilon)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should equal %.6g "
               "(diff=%.6g, eps=%.6g)\r\n",
                filename, line_no, function, actual_expr_str, actual, expected, diff, epsilon);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_not_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    float diff = fabsf(expected - actual);
    if (diff <= epsilon)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should not equal %.6g "
               "(diff=%.6g, eps=%.6g)\r\n",
                filename, line_no, function, actual_expr_str, actual, expected, diff, epsilon);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_greater_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a > b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should be > %s (%.6g)\r\n", filename, line_no, function,
                a_expr_str, a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_less_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a < b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should be < %s (%.6g)\r\n", filename, line_no, function,
                a_expr_str, a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_ge_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a >= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should be >= %s (%.6g)\r\n", filename, line_no, function,
                a_expr_str, a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_le_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a <= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should be <= %s (%.6g)\r\n", filename, line_no, function,
                a_expr_str, a, b_expr_str, b);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_in_range_impl(float val, float min, float max, const char *filename, int line_no,
        const char *function, const char *val_expr_str)
{
    _assertions_executed++;
    if (!(val >= min && val <= max))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.6g) should be in range "
               "[%.6g, %.6g]\r\n",
                filename, line_no, function, val_expr_str, val, min, max);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

#endif /* LFG_CTEST_HAS_FLOAT */

/*============================================================================
 *  64-bit Double Assertions (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_DOUBLE

int lfg_ct_assert_double_equal_impl(double expected, double actual, double epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    double diff = fabs(expected - actual);
    if (diff > epsilon)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.10g) should equal %.10g "
               "(diff=%.10g, eps=%.10g)\r\n",
                filename, line_no, function, actual_expr_str, actual, expected, diff, epsilon);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_double_not_equal_impl(double expected, double actual, double epsilon, const char *filename,
        int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    double diff = fabs(expected - actual);
    if (diff <= epsilon)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%.10g) should not equal %.10g "
               "(diff=%.10g, eps=%.10g)\r\n",
                filename, line_no, function, actual_expr_str, actual, expected, diff, epsilon);
        RECORD_FAILURE();
        return -1;
    }
    RECORD_PASS();
    return 0;
}

#endif /* LFG_CTEST_HAS_DOUBLE */

/*============================================================================
 *  Private Functions
 *==========================================================================*/
