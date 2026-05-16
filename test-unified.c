/*
 * Unified Test Suite for lfg-ctest
 *
 * This suite exercises all assertion macros with both passing and failing tests
 * to verify complete functionality of the testing framework.
 *
 * Uses LFG_CTEST_SELF_TEST mode to verify assertion failures are correctly
 * detected without failing the test suite itself.
 *
 * Floating-point assertions are only tested when LFG_CTEST_HAS_FLOAT is defined.
 * Use cmake -DLFG_CTEST_ENABLE_FLOAT=OFF to build without float support.
 */

#include "lfg-ctest.h"
#include <stdint.h>
#include <string.h>

/* ============================================================================
 * PASSING TESTS - All assertions should succeed
 * ============================================================================ */

static void test_pointer_assertions_pass(void)
{
    int value = 42;
    int *ptr1 = &value;
    int *ptr2 = &value;
    int *ptr3 = NULL;

    ASSERT_PTR_EQUAL(ptr1, ptr2);
    ASSERT_PTR_NOT_EQUAL(ptr1, ptr3);
    ASSERT_PTR_NULL(ptr3);
    ASSERT_PTR_NOT_NULL(ptr1);
    ASSERT_NULL(ptr3);
    ASSERT_NOT_NULL(ptr1);
}

static void test_boolean_assertions_pass(void)
{
    ASSERT_TRUE(1);
    ASSERT_TRUE(5 > 3);
    ASSERT_FALSE(0);
    ASSERT_FALSE(2 < 1);
}

static void test_integer_assertions_pass(void)
{
    /* Generic int */
    ASSERT_INT_EQUAL(42, 42);
    ASSERT_INT_NOT_EQUAL(42, 43);
    ASSERT_EQ(100, 100);
    ASSERT_NE(100, 99);

    /* Unsigned */
    ASSERT_UINT_EQUAL(42U, 42U);
    ASSERT_UINT_NOT_EQUAL(42U, 43U);

    /* Fixed-width signed */
    ASSERT_INT8_EQUAL((int8_t)-128, (int8_t)-128);
    ASSERT_INT8_NOT_EQUAL((int8_t)127, (int8_t)-128);
    ASSERT_INT16_EQUAL((int16_t)-32768, (int16_t)-32768);
    ASSERT_INT16_NOT_EQUAL((int16_t)32767, (int16_t)-32768);
    ASSERT_INT32_EQUAL((int32_t)123456, (int32_t)123456);
    ASSERT_INT32_NOT_EQUAL((int32_t)123456, (int32_t)-123456);
    ASSERT_INT64_EQUAL((int64_t)9223372036854775807LL, (int64_t)9223372036854775807LL);
    ASSERT_INT64_NOT_EQUAL((int64_t)9223372036854775807LL, (int64_t)-9223372036854775807LL);

    /* Fixed-width unsigned */
    ASSERT_UINT8_EQUAL((uint8_t)255, (uint8_t)255);
    ASSERT_UINT8_NOT_EQUAL((uint8_t)255, (uint8_t)0);
    ASSERT_UINT16_EQUAL((uint16_t)65535, (uint16_t)65535);
    ASSERT_UINT16_NOT_EQUAL((uint16_t)65535, (uint16_t)0);
    ASSERT_UINT32_EQUAL((uint32_t)4294967295UL, (uint32_t)4294967295UL);
    ASSERT_UINT32_NOT_EQUAL((uint32_t)4294967295UL, (uint32_t)0);
    ASSERT_UINT64_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)18446744073709551615ULL);
    ASSERT_UINT64_NOT_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)0);
}

static void test_string_assertions_pass(void)
{
    const char *str1 = "hello";
    const char *str2 = "hello";
    const char *str3 = "world";
    const char *str4 = "hello world";

    ASSERT_STR_EQUAL(str1, str2);
    ASSERT_STR_NOT_EQUAL(str1, str3);
    ASSERT_STRN_EQUAL(str1, str4, 5);
}

static void test_memory_assertions_pass(void)
{
    uint8_t buf1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t buf2[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t buf3[] = {0xFF, 0xFF, 0xFF, 0xFF};

    ASSERT_MEM_EQUAL(buf1, buf2, 4);
    ASSERT_MEM_NOT_EQUAL(buf1, buf3, 4);
}

static void test_comparison_assertions_pass(void)
{
    ASSERT_GREATER_THAN(10, 5);
    ASSERT_GT(100, 50);

    ASSERT_LESS_THAN(5, 10);
    ASSERT_LT(50, 100);

    ASSERT_GREATER_OR_EQUAL(10, 10);
    ASSERT_GREATER_OR_EQUAL(10, 5);
    ASSERT_GE(100, 100);
    ASSERT_GE(100, 50);

    ASSERT_LESS_OR_EQUAL(5, 5);
    ASSERT_LESS_OR_EQUAL(5, 10);
    ASSERT_LE(50, 50);
    ASSERT_LE(50, 100);
}

static void test_range_assertion_pass(void)
{
    ASSERT_IN_RANGE(5, 1, 10);
    ASSERT_IN_RANGE(1, 1, 10);
    ASSERT_IN_RANGE(10, 1, 10);
}

static void test_bit_assertions_pass(void)
{
    uint8_t value = 0xAA; /* 0b10101010 */

    ASSERT_BIT_SET(value, 1);
    ASSERT_BIT_SET(value, 3);
    ASSERT_BIT_SET(value, 5);
    ASSERT_BIT_SET(value, 7);

    ASSERT_BIT_CLEAR(value, 0);
    ASSERT_BIT_CLEAR(value, 2);
    ASSERT_BIT_CLEAR(value, 4);
    ASSERT_BIT_CLEAR(value, 6);

    ASSERT_BITS_SET(value, 0xA0);   /* 0b10100000 */
    ASSERT_BITS_CLEAR(value, 0x55); /* 0b01010101 */
}

#ifdef LFG_CTEST_HAS_FLOAT
static void test_float_assertions_pass(void)
{
    /* Float equality with epsilon */
    ASSERT_FLOAT_EQUAL(3.14159f, 3.14159f, 0.0001f);
    ASSERT_FLT_EQ(100.0f, 100.0001f, 0.001f);

    /* Float not equal */
    ASSERT_FLOAT_NOT_EQUAL(1.0f, 2.0f, 0.1f);
    ASSERT_FLT_NE(0.0f, 1.0f, 0.5f);

    /* Float comparisons */
    ASSERT_FLOAT_GREATER_THAN(10.5f, 5.5f);
    ASSERT_FLT_GT(100.0f, 99.9f);

    ASSERT_FLOAT_LESS_THAN(5.5f, 10.5f);
    ASSERT_FLT_LT(99.9f, 100.0f);

    ASSERT_FLOAT_GREATER_OR_EQUAL(10.0f, 10.0f);
    ASSERT_FLOAT_GREATER_OR_EQUAL(10.0f, 5.0f);
    ASSERT_FLT_GE(100.0f, 100.0f);
    ASSERT_FLT_GE(100.0f, 50.0f);

    ASSERT_FLOAT_LESS_OR_EQUAL(5.0f, 5.0f);
    ASSERT_FLOAT_LESS_OR_EQUAL(5.0f, 10.0f);
    ASSERT_FLT_LE(50.0f, 50.0f);
    ASSERT_FLT_LE(50.0f, 100.0f);

    /* Float range */
    ASSERT_FLOAT_IN_RANGE(5.0f, 1.0f, 10.0f);
    ASSERT_FLOAT_IN_RANGE(1.0f, 1.0f, 10.0f);
    ASSERT_FLOAT_IN_RANGE(10.0f, 1.0f, 10.0f);
}
#endif

#ifdef LFG_CTEST_HAS_DOUBLE
static void test_double_assertions_pass(void)
{
    /* Double equality with epsilon */
    ASSERT_DOUBLE_EQUAL(3.141592653589793, 3.141592653589793, 1e-10);
    ASSERT_DBL_EQ(1e10, 1.00000001e10, 1e4);

    /* Double not equal */
    ASSERT_DOUBLE_NOT_EQUAL(1.0, 2.0, 0.1);
    ASSERT_DBL_NE(0.0, 1.0, 0.5);
}
#endif

/* ============================================================================
 * FAILURE DETECTION TESTS - Verify the framework detects failures correctly
 *
 * These tests use expect-failures mode to verify assertion failures are
 * correctly detected without failing the test suite.
 * ============================================================================ */

static void test_pointer_failure_detection(void)
{
    int value1 = 42;
    int value2 = 43;
    int *ptr1 = &value1;
    int *ptr2 = &value2;
    int *ptr3 = NULL;
    int expected_failures = 6;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_PTR_EQUAL(ptr1, ptr2);     /* FAIL: different pointers */
    ASSERT_PTR_NOT_EQUAL(ptr1, ptr1); /* FAIL: same pointer */
    ASSERT_PTR_NULL(ptr1);            /* FAIL: not null */
    ASSERT_PTR_NOT_NULL(ptr3);        /* FAIL: is null */
    ASSERT_NULL(ptr1);                /* FAIL: not null */
    ASSERT_NOT_NULL(ptr3);            /* FAIL: is null */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_boolean_failure_detection(void)
{
    int expected_failures = 4;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_TRUE(0);      /* FAIL: false */
    ASSERT_TRUE(2 < 1);  /* FAIL: false expression */
    ASSERT_FALSE(1);     /* FAIL: true */
    ASSERT_FALSE(5 > 3); /* FAIL: true expression */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_integer_failure_detection(void)
{
    int expected_failures = 20;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    /* Generic int */
    ASSERT_INT_EQUAL(42, 43);     /* FAIL: not equal */
    ASSERT_INT_NOT_EQUAL(42, 42); /* FAIL: equal */
    ASSERT_EQ(100, 99);           /* FAIL: not equal */
    ASSERT_NE(100, 100);          /* FAIL: equal */

    /* Unsigned */
    ASSERT_UINT_EQUAL(42U, 43U);     /* FAIL: not equal */
    ASSERT_UINT_NOT_EQUAL(42U, 42U); /* FAIL: equal */

    /* Fixed-width signed */
    ASSERT_INT8_EQUAL((int8_t)127, (int8_t)-128);                                           /* FAIL */
    ASSERT_INT8_NOT_EQUAL((int8_t)-128, (int8_t)-128);                                      /* FAIL */
    ASSERT_INT16_EQUAL((int16_t)32767, (int16_t)-32768);                                    /* FAIL */
    ASSERT_INT16_NOT_EQUAL((int16_t)-32768, (int16_t)-32768);                               /* FAIL */
    ASSERT_INT32_EQUAL((int32_t)123456, (int32_t)-123456);                                  /* FAIL */
    ASSERT_INT32_NOT_EQUAL((int32_t)123456, (int32_t)123456);                               /* FAIL */
    ASSERT_INT64_EQUAL((int64_t)9223372036854775807LL, (int64_t)-9223372036854775807LL);    /* FAIL */
    ASSERT_INT64_NOT_EQUAL((int64_t)9223372036854775807LL, (int64_t)9223372036854775807LL); /* FAIL */

    /* Fixed-width unsigned */
    ASSERT_UINT8_EQUAL((uint8_t)255, (uint8_t)0);                            /* FAIL */
    ASSERT_UINT8_NOT_EQUAL((uint8_t)255, (uint8_t)255);                      /* FAIL */
    ASSERT_UINT16_EQUAL((uint16_t)65535, (uint16_t)0);                       /* FAIL */
    ASSERT_UINT16_NOT_EQUAL((uint16_t)65535, (uint16_t)65535);               /* FAIL */
    ASSERT_UINT32_EQUAL((uint32_t)4294967295UL, (uint32_t)0);                /* FAIL */
    ASSERT_UINT32_NOT_EQUAL((uint32_t)4294967295UL, (uint32_t)4294967295UL); /* FAIL */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_integer64_failure_detection(void)
{
    int expected_failures = 4;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_UINT64_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)0);                           /* FAIL */
    ASSERT_UINT64_NOT_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)18446744073709551615ULL); /* FAIL */
    ASSERT_INT64_EQUAL((int64_t)0, (int64_t)1);                                                    /* FAIL */
    ASSERT_INT64_NOT_EQUAL((int64_t)0, (int64_t)0);                                                /* FAIL */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_string_failure_detection(void)
{
    const char *str1 = "hello";
    const char *str2 = "world";
    const char *str3 = "hello world";
    int expected_failures = 3;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_STR_EQUAL(str1, str2);      /* FAIL: different strings */
    ASSERT_STR_NOT_EQUAL(str1, str1);  /* FAIL: same string */
    ASSERT_STRN_EQUAL(str1, str3, 10); /* FAIL: first 10 chars differ */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_memory_failure_detection(void)
{
    uint8_t buf1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t buf2[] = {0xFF, 0xFF, 0xFF, 0xFF};
    int expected_failures = 2;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_MEM_EQUAL(buf1, buf2, 4);     /* FAIL: different memory */
    ASSERT_MEM_NOT_EQUAL(buf1, buf1, 4); /* FAIL: same memory */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_comparison_failure_detection(void)
{
    int expected_failures = 8;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_GREATER_THAN(5, 10); /* FAIL: 5 <= 10 */
    ASSERT_GT(50, 100);         /* FAIL: 50 <= 100 */

    ASSERT_LESS_THAN(10, 5); /* FAIL: 10 >= 5 */
    ASSERT_LT(100, 50);      /* FAIL: 100 >= 50 */

    ASSERT_GREATER_OR_EQUAL(5, 10); /* FAIL: 5 < 10 */
    ASSERT_GE(50, 100);             /* FAIL: 50 < 100 */

    ASSERT_LESS_OR_EQUAL(10, 5); /* FAIL: 10 > 5 */
    ASSERT_LE(100, 50);          /* FAIL: 100 > 50 */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_range_failure_detection(void)
{
    int expected_failures = 3;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_IN_RANGE(0, 1, 10);  /* FAIL: 0 < 1 */
    ASSERT_IN_RANGE(11, 1, 10); /* FAIL: 11 > 10 */
    ASSERT_IN_RANGE(-5, 0, 10); /* FAIL: -5 < 0 */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_bit_failure_detection(void)
{
    uint8_t value = 0xAA; /* 0b10101010 */
    int expected_failures = 6;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_BIT_SET(value, 0); /* FAIL: bit 0 is clear */
    ASSERT_BIT_SET(value, 2); /* FAIL: bit 2 is clear */

    ASSERT_BIT_CLEAR(value, 1); /* FAIL: bit 1 is set */
    ASSERT_BIT_CLEAR(value, 3); /* FAIL: bit 3 is set */

    ASSERT_BITS_SET(value, 0xFF);   /* FAIL: some bits clear */
    ASSERT_BITS_CLEAR(value, 0x80); /* FAIL: bit 7 is set */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

static void test_explicit_fail_detection(void)
{
    int expected_failures = 1;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    ASSERT_FAIL("This is an intentional failure for testing ASSERT_FAIL");

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}

#ifdef LFG_CTEST_HAS_FLOAT
static void test_float_failure_detection(void)
{
    int expected_failures = 14;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    /* Float equality with epsilon */
    ASSERT_FLOAT_EQUAL(1.0f, 2.0f, 0.1f);  /* FAIL: diff > epsilon */
    ASSERT_FLT_EQ(100.0f, 200.0f, 0.001f); /* FAIL: diff > epsilon */

    /* Float not equal */
    ASSERT_FLOAT_NOT_EQUAL(1.0f, 1.0f, 0.1f); /* FAIL: they are equal */
    ASSERT_FLT_NE(0.0f, 0.0001f, 0.001f);     /* FAIL: diff < epsilon */

    /* Float comparisons */
    ASSERT_FLOAT_GREATER_THAN(5.5f, 10.5f); /* FAIL: 5.5 <= 10.5 */
    ASSERT_FLT_GT(99.9f, 100.0f);           /* FAIL: 99.9 <= 100.0 */

    ASSERT_FLOAT_LESS_THAN(10.5f, 5.5f); /* FAIL: 10.5 >= 5.5 */
    ASSERT_FLT_LT(100.0f, 99.9f);        /* FAIL: 100.0 >= 99.9 */

    ASSERT_FLOAT_GREATER_OR_EQUAL(5.0f, 10.0f); /* FAIL: 5.0 < 10.0 */
    ASSERT_FLT_GE(50.0f, 100.0f);               /* FAIL: 50.0 < 100.0 */

    ASSERT_FLOAT_LESS_OR_EQUAL(10.0f, 5.0f); /* FAIL: 10.0 > 5.0 */
    ASSERT_FLT_LE(100.0f, 50.0f);            /* FAIL: 100.0 > 50.0 */

    /* Float range */
    ASSERT_FLOAT_IN_RANGE(0.5f, 1.0f, 10.0f);  /* FAIL: 0.5 < 1.0 */
    ASSERT_FLOAT_IN_RANGE(11.0f, 1.0f, 10.0f); /* FAIL: 11.0 > 10.0 */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}
#endif

#ifdef LFG_CTEST_HAS_DOUBLE
static void test_double_failure_detection(void)
{
    int expected_failures = 4;
    int actual_failures;

    lfg_ct_expect_failures_begin();

    /* Double equality with epsilon */
    ASSERT_DOUBLE_EQUAL(1.0, 2.0, 0.1); /* FAIL: diff > epsilon */
    ASSERT_DBL_EQ(1e10, 2e10, 1e4);     /* FAIL: diff > epsilon */

    /* Double not equal */
    ASSERT_DOUBLE_NOT_EQUAL(1.0, 1.0, 0.1); /* FAIL: they are equal */
    ASSERT_DBL_NE(0.0, 0.0001, 0.001);      /* FAIL: diff < epsilon */

    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}
#endif

/* ============================================================================
 * SETUP/TEARDOWN HOOK LIFECYCLE TESTS
 *
 * Verify the framework's setup -> body -> teardown sequencing on both
 * lfg_ct_test and lfg_ct_suite: happy path, body failure (teardown still
 * runs), NULL setup, NULL teardown, setup failure (body skipped, teardown
 * still runs), and the suite-wraps-test nesting order.
 *
 * Each hook stamps a single character into _hook_trace so order is
 * preserved end-to-end. Outer self-tests wrap the inner framework calls
 * in expect-failures mode so intentional setup/body failures don't fail
 * this binary's own result.
 * ============================================================================ */

static char _hook_trace[64];
static size_t _hook_trace_len;

static void _hook_trace_reset(void)
{
    _hook_trace_len = 0;
    _hook_trace[0] = '\0';
}

static void _hook_trace_push(char c)
{
    if (_hook_trace_len + 1 < sizeof(_hook_trace))
    {
        _hook_trace[_hook_trace_len++] = c;
        _hook_trace[_hook_trace_len] = '\0';
    }
}

/* Phase callbacks: each appends a tag and (where indicated) fails an
 * assertion. Tags: S/B/T for setup/body/teardown on the test-level
 * happy/failure paths; s/t for suite-level setup/teardown in the nested
 * ordering test. _body_unreachable stamps 'X' which the assertions
 * verify never appears (i.e. the body was skipped). */
static void _hook_setup_ok(void)         { _hook_trace_push('S'); }
static void _hook_setup_fail(void)       { _hook_trace_push('S'); ASSERT_FAIL("intentional setup failure"); }
static void _hook_body_ok(void)          { _hook_trace_push('B'); }
static void _hook_body_fail(void)        { _hook_trace_push('B'); ASSERT_FAIL("intentional body failure"); }
static void _hook_body_unreachable(void) { _hook_trace_push('X'); }
static void _hook_teardown_ok(void)      { _hook_trace_push('T'); }

static void _nested_suite_setup(void)    { _hook_trace_push('s'); }
static void _nested_suite_teardown(void) { _hook_trace_push('t'); }
static void _nested_suite_body(void)
{
    lfg_ct_test(_hook_setup_ok, _hook_body_ok, _hook_teardown_ok);
}

/* ---- lfg_ct_test variants ------------------------------------------------ */

static void test_test_hooks_happy_path(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_hook_setup_ok, _hook_body_ok, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_test_hooks_body_failure_still_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_hook_setup_ok, _hook_body_fail, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_test_hooks_null_setup_skips_setup_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(NULL, _hook_body_ok, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("BT", _hook_trace);
}

static void test_test_hooks_null_teardown_skips_teardown_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_hook_setup_ok, _hook_body_ok, NULL);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SB", _hook_trace);
}

static void test_test_hooks_setup_failure_skips_body_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_hook_setup_fail, _hook_body_unreachable, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    /* Exactly one failure (the setup), the body never recorded its tag,
     * and teardown still ran. */
    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("ST", _hook_trace);
}

/* ---- lfg_ct_suite variants ----------------------------------------------- */

static void test_suite_hooks_happy_path(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_hook_setup_ok, _hook_body_ok, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_suite_hooks_body_failure_still_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_hook_setup_ok, _hook_body_fail, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_suite_hooks_null_setup_skips_setup_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(NULL, _hook_body_ok, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("BT", _hook_trace);
}

static void test_suite_hooks_null_teardown_skips_teardown_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_hook_setup_ok, _hook_body_ok, NULL);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SB", _hook_trace);
}

static void test_suite_hooks_setup_failure_skips_body_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_hook_setup_fail, _hook_body_unreachable, _hook_teardown_ok);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("ST", _hook_trace);
}

/* ---- suite wraps test: nesting order ------------------------------------- */

static void test_suite_wrapping_test_fires_hooks_in_nesting_order(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_nested_suite_setup, _nested_suite_body, _nested_suite_teardown);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    /* suite-setup -> test-setup -> test-body -> test-teardown -> suite-teardown */
    ASSERT_STR_EQUAL("sSBTt", _hook_trace);
}

/* ============================================================================
 * --list / --filter / --filter-exclude argument-parsing tests
 *
 * These exercise lfg_ct_parse_args() and its consumption by the runner.
 * The nested lfg_ct_test_impl() calls in the body-skipping tests rely on
 * lfg_ct_test_impl() being a no-op when the name is filtered out -- in
 * that case it does not touch _current_test_failures, so the outer test's
 * assertion state is preserved.
 * ============================================================================ */

static int _filter_body_called;

static void _filter_body(void)
{
    _filter_body_called = 1;
}

static void test_filter_parse_no_args_runs_everything(void)
{
    char *argv[] = {(char *)"prog"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv));
    ASSERT_INT_EQUAL(0, lfg_ct_is_list_mode());
    ASSERT_TRUE(lfg_ct_name_runs("anything"));
    ASSERT_TRUE(lfg_ct_name_runs("suite_xyz"));
}

static void test_filter_parse_filter_basic(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"test_*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));
    ASSERT_TRUE(lfg_ct_name_runs("test_foo"));
    ASSERT_FALSE(lfg_ct_name_runs("suite_bar"));
}

static void test_filter_parse_exclude_basic(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter-exclude", (char *)"*_skip"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));
    ASSERT_FALSE(lfg_ct_name_runs("test_skip"));
    ASSERT_TRUE(lfg_ct_name_runs("test_run"));
}

static void test_filter_parse_exclude_wins_over_filter(void)
{
    char *argv[] = {
        (char *)"prog", (char *)"--filter", (char *)"test_*", (char *)"--filter-exclude", (char *)"*_skip"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(5, argv));
    ASSERT_FALSE(lfg_ct_name_runs("test_skip")); /* both match, exclude wins */
    ASSERT_TRUE(lfg_ct_name_runs("test_run"));
    ASSERT_FALSE(lfg_ct_name_runs("suite_bar")); /* filter doesn't match */
}

static void test_filter_parse_repeated_filter_ors(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"alpha*", (char *)"--filter", (char *)"beta*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(5, argv));
    ASSERT_TRUE(lfg_ct_name_runs("alpha_x"));
    ASSERT_TRUE(lfg_ct_name_runs("beta_y"));
    ASSERT_FALSE(lfg_ct_name_runs("gamma"));
}

static void test_filter_parse_repeated_exclude_ors(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter-exclude", (char *)"alpha*", (char *)"--filter-exclude",
            (char *)"beta*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(5, argv));
    ASSERT_FALSE(lfg_ct_name_runs("alpha_x"));
    ASSERT_FALSE(lfg_ct_name_runs("beta_y"));
    ASSERT_TRUE(lfg_ct_name_runs("gamma"));
}

static void test_filter_parse_unmatched_filter_runs_nothing(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"no_match*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));
    ASSERT_FALSE(lfg_ct_name_runs("test_foo"));
    ASSERT_FALSE(lfg_ct_name_runs("suite_bar"));
}

static void test_filter_parse_list_mode_set(void)
{
    char *argv[] = {(char *)"prog", (char *)"--list"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    ASSERT_INT_EQUAL(1, lfg_ct_is_list_mode());
    /* list mode reports name_runs == 0 because the body would not execute */
    ASSERT_FALSE(lfg_ct_name_runs("anything"));
}

static void test_filter_parse_unknown_flag_fails(void)
{
    char *argv[] = {(char *)"prog", (char *)"--bogus"};
    /* prints usage to stderr by design; we only check the return value */
    ASSERT_INT_NOT_EQUAL(0, lfg_ct_parse_args(2, argv));
    /* state must be reset on error so subsequent calls start clean */
    ASSERT_INT_EQUAL(0, lfg_ct_is_list_mode());
}

static void test_filter_parse_missing_filter_arg_fails(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter"};
    ASSERT_INT_NOT_EQUAL(0, lfg_ct_parse_args(2, argv));
}

static void test_filter_parse_missing_exclude_arg_fails(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter-exclude"};
    ASSERT_INT_NOT_EQUAL(0, lfg_ct_parse_args(2, argv));
}

static void test_filter_parse_glob_wildcards(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"suite_?ma_*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));
    ASSERT_TRUE(lfg_ct_name_runs("suite_sma_basic"));
    ASSERT_TRUE(lfg_ct_name_runs("suite_ema_basic"));
    ASSERT_FALSE(lfg_ct_name_runs("suite_xx_basic"));
    ASSERT_FALSE(lfg_ct_name_runs("suite_sma")); /* trailing _* requires an _ */
}

static void test_filter_parse_charclass_globs(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"test_[ab]*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));
    ASSERT_TRUE(lfg_ct_name_runs("test_apple"));
    ASSERT_TRUE(lfg_ct_name_runs("test_banana"));
    ASSERT_FALSE(lfg_ct_name_runs("test_cherry"));
}

static void test_filter_runner_skips_filtered_test_body(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"match_me*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));

    _filter_body_called = 0;
    lfg_ct_test_impl(NULL, _filter_body, NULL, "no_match");
    ASSERT_INT_EQUAL(0, _filter_body_called);

    _filter_body_called = 0;
    lfg_ct_test_impl(NULL, _filter_body, NULL, "match_me_yes");
    ASSERT_INT_EQUAL(1, _filter_body_called);
}

static void test_filter_runner_skips_excluded_test_body(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter-exclude", (char *)"skip_*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));

    _filter_body_called = 0;
    lfg_ct_test_impl(NULL, _filter_body, NULL, "skip_me");
    ASSERT_INT_EQUAL(0, _filter_body_called);

    _filter_body_called = 0;
    lfg_ct_test_impl(NULL, _filter_body, NULL, "run_me");
    ASSERT_INT_EQUAL(1, _filter_body_called);
}

static void test_filter_runner_skips_all_bodies_in_list_mode(void)
{
    char *argv[] = {(char *)"prog", (char *)"--list"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));

    _filter_body_called = 0;
    /* list mode prints "anything\r\n" to stdout but skips the body */
    lfg_ct_test_impl(NULL, _filter_body, NULL, "anything");
    ASSERT_INT_EQUAL(0, _filter_body_called);
}

static int _filter_inner_called;

static void _filter_inner_body(void)
{
    _filter_inner_called = 1;
}

static void _filter_inheriting_suite_body(void)
{
    /* The inner test's name does NOT match "inheriting_suite_filter*"; it
     * runs only because the outer suite's name matched and the match was
     * inherited down through _filter_inherited_depth. */
    lfg_ct_test_impl(NULL, _filter_inner_body, NULL, "unrelated_inner_test");
}

static void _filter_non_matching_suite_body(void)
{
    /* Used to verify the negative case: no suite-match, so the inner test
     * must match the filter on its own. Its name doesn't match either, so
     * the body must NOT run. */
    lfg_ct_test_impl(NULL, _filter_inner_body, NULL, "unrelated_inner_test");
}

static void test_filter_runner_suite_match_propagates_to_inner_tests(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"inheriting_suite_filter*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));

    /* Positive: matching outer suite -> inner test runs by inheritance. */
    _filter_inner_called = 0;
    lfg_ct_suite_impl(NULL, _filter_inheriting_suite_body, NULL, "inheriting_suite_filter_demo");
    ASSERT_INT_EQUAL(1, _filter_inner_called);

    /* Negative: non-matching outer suite -> still descends, but inner test
     * has to match on its own. It doesn't, so body stays unrun. */
    _filter_inner_called = 0;
    lfg_ct_suite_impl(NULL, _filter_non_matching_suite_body, NULL, "unrelated_outer_suite");
    ASSERT_INT_EQUAL(0, _filter_inner_called);
}

static void test_filter_reset_state_for_remaining_tests(void)
{
    /* The final test in this suite restores default state so subsequent
     * suites in this binary run unfiltered. */
    char *argv[] = {(char *)"prog"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv));
    ASSERT_INT_EQUAL(0, lfg_ct_is_list_mode());
    ASSERT_TRUE(lfg_ct_name_runs("anything"));
}

static void suite_filter_args_tests(void)
{
    lfg_ct_test(NULL, test_filter_parse_no_args_runs_everything, NULL);
    lfg_ct_test(NULL, test_filter_parse_filter_basic, NULL);
    lfg_ct_test(NULL, test_filter_parse_exclude_basic, NULL);
    lfg_ct_test(NULL, test_filter_parse_exclude_wins_over_filter, NULL);
    lfg_ct_test(NULL, test_filter_parse_repeated_filter_ors, NULL);
    lfg_ct_test(NULL, test_filter_parse_repeated_exclude_ors, NULL);
    lfg_ct_test(NULL, test_filter_parse_unmatched_filter_runs_nothing, NULL);
    lfg_ct_test(NULL, test_filter_parse_list_mode_set, NULL);
    lfg_ct_test(NULL, test_filter_parse_unknown_flag_fails, NULL);
    lfg_ct_test(NULL, test_filter_parse_missing_filter_arg_fails, NULL);
    lfg_ct_test(NULL, test_filter_parse_missing_exclude_arg_fails, NULL);
    lfg_ct_test(NULL, test_filter_parse_glob_wildcards, NULL);
    lfg_ct_test(NULL, test_filter_parse_charclass_globs, NULL);
    lfg_ct_test(NULL, test_filter_runner_skips_filtered_test_body, NULL);
    lfg_ct_test(NULL, test_filter_runner_skips_excluded_test_body, NULL);
    lfg_ct_test(NULL, test_filter_runner_skips_all_bodies_in_list_mode, NULL);
    lfg_ct_test(NULL, test_filter_runner_suite_match_propagates_to_inner_tests, NULL);

    /* Unconditional reset before the verifying test runs -- without this,
     * the verifying test is itself filter-gated by whatever the prior
     * test left in place and may be silently skipped, leaking filter
     * state into the remaining suites in this binary. */
    {
        char *reset_argv[] = {(char *)"prog"};
        (void)lfg_ct_parse_args(1, reset_argv);
    }
    lfg_ct_test(NULL, test_filter_reset_state_for_remaining_tests, NULL);
}

/* ============================================================================
 * skip / xfail / xpass disposition tests
 *
 * These exercise lfg_ct_skip and lfg_ct_xfail by running a *nested*
 * lfg_ct_test_impl from inside the outer self-test body, then observing
 * the runner's bucket counters (lfg_ct_self_*_count) before and after.
 * The nested call cannot run under lfg_ct_expect_failures mode because
 * that mode masks the real per-test failure counter, which the runner
 * needs to see in order to distinguish XFAIL from XPASS.
 *
 * The cost: the inner mock tests count toward the binary's tallies
 * (executed, plus their actual bucket). The pay-off: every outcome is
 * a real-runner outcome -- the same code path a consumer would hit.
 * ============================================================================ */

static int _disp_setup_teardown_trace;
static int _disp_post_skip_marker;

static void _disp_body_skip_simple(void)
{
    lfg_ct_skip("body asked to skip");
    /* Unreachable: lfg_ct_skip longjmps out. The store would observably
     * flip _disp_post_skip_marker if the longjmp ever failed, giving
     * the surrounding test something concrete to assert against. */
    _disp_post_skip_marker = 1;
}

static void _disp_setup_skip(void)
{
    _disp_setup_teardown_trace |= 0x1;
    lfg_ct_skip("preconditions not met");
    _disp_setup_teardown_trace |= 0x2; /* must NOT fire */
}

static void _disp_body_unreachable(void)
{
    _disp_setup_teardown_trace |= 0x4; /* must NOT fire when setup skipped */
}

static void _disp_teardown_marker(void)
{
    _disp_setup_teardown_trace |= 0x8;
}

static void _disp_body_xfail_with_failure(void)
{
    lfg_ct_xfail("known broken");
    ASSERT_FAIL("intentional body failure");
}

static void _disp_body_xfail_without_failure(void)
{
    lfg_ct_xfail("known broken but passes today");
    /* No assertion failure -> XPASS bucket. */
}

static void _disp_body_xfail_last_reason_wins(void)
{
    lfg_ct_xfail("first reason");
    lfg_ct_xfail("middle reason");
    lfg_ct_xfail("last reason");
    ASSERT_FAIL("intentional body failure");
}

/* Outer body that runs a nested lfg_ct_test_impl and *then* calls
 * lfg_ct_skip. The single-buffer skip boundary used to lose the outer
 * frame on the nested return (overwriting _skip_env, clearing
 * _skip_env_active) -- the lifecycle helper now save/restores both so
 * the outer skip still unwinds. */
static void _disp_body_after_nested_then_skip(void)
{
    lfg_ct_test_impl(NULL, _disp_body_xfail_without_failure, NULL, "mock_nested_inner");
    lfg_ct_skip("outer skip after nested test");
    _disp_post_skip_marker = 1; /* must NOT execute -- outer must unwind */
}

/* Suite-level skip is intentionally out of scope: lfg_ct_skip is a
 * per-test gesture. A skip call from inside a suite-level setup must
 * warn to stderr and be a benign no-op so the suite continues
 * normally. */
static int _disp_suite_body_ran;

static void _disp_suite_setup_skips(void)
{
    lfg_ct_skip("suite preconditions not met");
}

static void _disp_suite_body_marker(void)
{
    _disp_suite_body_ran = 1;
}

/* Capture the latest classification message by intercepting via a body
 * that records what lfg_ct_xfail saved -- we read the runner-visible
 * reason indirectly through the bucket-counter delta and trust the
 * runner to print the right reason on stdout. The "last reason wins"
 * test asserts the printed-reason behavior at print time below. */

static void test_disposition_skip_from_body_increments_skipped_bucket(void)
{
    int before_skipped = lfg_ct_self_skipped_count();
    int before_failed = lfg_ct_self_failed_count();
    int before_asserts_failed = lfg_ct_self_assertions_failed();

    _disp_post_skip_marker = 0;
    lfg_ct_test_impl(NULL, _disp_body_skip_simple, NULL, "mock_skip_body");

    /* The post-skip statement must not have executed. */
    ASSERT_INT_EQUAL(0, _disp_post_skip_marker);
    ASSERT_INT_EQUAL(before_skipped + 1, lfg_ct_self_skipped_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
    ASSERT_INT_EQUAL(before_asserts_failed, lfg_ct_self_assertions_failed());
}

static void test_disposition_skip_from_setup_skips_body_runs_teardown(void)
{
    int before_skipped = lfg_ct_self_skipped_count();
    int before_failed = lfg_ct_self_failed_count();

    _disp_setup_teardown_trace = 0;
    lfg_ct_test_impl(_disp_setup_skip, _disp_body_unreachable, _disp_teardown_marker, "mock_skip_setup");

    /* setup entered (0x1), did not progress past lfg_ct_skip (no 0x2),
     * body never invoked (no 0x4), teardown still ran (0x8). */
    ASSERT_INT_EQUAL(0x1 | 0x8, _disp_setup_teardown_trace);
    ASSERT_INT_EQUAL(before_skipped + 1, lfg_ct_self_skipped_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
}

static void test_disposition_xfail_with_assertion_failure_buckets_as_xfail(void)
{
    int before_xfail = lfg_ct_self_xfailed_count();
    int before_failed = lfg_ct_self_failed_count();
    int before_asserts_failed = lfg_ct_self_assertions_failed();

    lfg_ct_test_impl(NULL, _disp_body_xfail_with_failure, NULL, "mock_xfail_with_fail");

    ASSERT_INT_EQUAL(before_xfail + 1, lfg_ct_self_xfailed_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
    /* The assertion-failure count was absorbed back out -- xfail must
     * not contribute to it (else the surrounding suite-failure detector
     * would trip on a perfectly fine xfail test). */
    ASSERT_INT_EQUAL(before_asserts_failed, lfg_ct_self_assertions_failed());
}

static void test_disposition_xfail_without_failure_buckets_as_xpass(void)
{
    int before_xpass = lfg_ct_self_xpassed_count();
    int before_failed = lfg_ct_self_failed_count();

    lfg_ct_test_impl(NULL, _disp_body_xfail_without_failure, NULL, "mock_xfail_no_fail");

    ASSERT_INT_EQUAL(before_xpass + 1, lfg_ct_self_xpassed_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
}

static void test_disposition_xfail_repeated_calls_keep_last_reason(void)
{
    int before_xfail = lfg_ct_self_xfailed_count();

    lfg_ct_test_impl(NULL, _disp_body_xfail_last_reason_wins, NULL, "mock_xfail_last_reason");

    ASSERT_INT_EQUAL(before_xfail + 1, lfg_ct_self_xfailed_count());
    /* The body called lfg_ct_xfail three times with different reasons;
     * classification must have kept the latest. */
    ASSERT_STR_EQUAL("last reason", lfg_ct_self_last_xfail_reason());
}

static void test_disposition_skip_after_nested_test_impl_still_unwinds(void)
{
    int before_skipped = lfg_ct_self_skipped_count();
    int before_xpassed = lfg_ct_self_xpassed_count();

    _disp_post_skip_marker = 0;
    lfg_ct_test_impl(NULL, _disp_body_after_nested_then_skip, NULL, "mock_outer_skip_after_nested");

    /* Nested mock contributed one xpass; outer contributed one skip. */
    ASSERT_INT_EQUAL(before_xpassed + 1, lfg_ct_self_xpassed_count());
    ASSERT_INT_EQUAL(before_skipped + 1, lfg_ct_self_skipped_count());
    /* The outer skip must have unwound -- the post-skip store stays
     * untouched. This is the regression guard for the save/restore of
     * _skip_env around nested lifecycles. */
    ASSERT_INT_EQUAL(0, _disp_post_skip_marker);
}

static void test_disposition_suite_context_skip_is_benign_no_op(void)
{
    int before_skipped = lfg_ct_self_skipped_count();

    /* Suite-level skip is out of scope -- lfg_ct_skip is a per-test
     * gesture. lfg_ct_suite_impl explicitly turns the skip boundary
     * off across its lifecycle, so a stray lfg_ct_skip from a suite
     * setup must warn to stderr (suppressed for test cleanliness via
     * NOT being captured here -- we only check the observable
     * consequence) and let the suite body run normally. */
    _disp_suite_body_ran = 0;
    lfg_ct_suite_impl(_disp_suite_setup_skips, _disp_suite_body_marker, NULL, "mock_suite_skip_in_setup");

    ASSERT_INT_EQUAL(1, _disp_suite_body_ran);
    /* No test was bucketed -- suites do not bucket as SKIP. */
    ASSERT_INT_EQUAL(before_skipped, lfg_ct_self_skipped_count());
}

static void test_disposition_xpass_strict_flag_toggles_return_code(void)
{
    int before_xpass = lfg_ct_self_xpassed_count();
    int saved_failed = lfg_ct_self_failed_count();

    /* Permissive mode: an xpass-only test contributes nothing to a
     * non-zero exit code. */
    lfg_ct_self_set_strict_xpass(0);
    lfg_ct_test_impl(NULL, _disp_body_xfail_without_failure, NULL, "mock_xpass_permissive");
    ASSERT_INT_EQUAL(before_xpass + 1, lfg_ct_self_xpassed_count());
    if (0 == saved_failed)
    {
        ASSERT_INT_EQUAL(0, lfg_ct_self_return_code());
    }

    /* Strict mode: with at least one xpass in the run, the return code
     * goes non-zero (provided no real failures already pin it). */
    lfg_ct_self_set_strict_xpass(1);
    if (0 == saved_failed)
    {
        ASSERT_INT_NOT_EQUAL(0, lfg_ct_self_return_code());
    }

    /* Restore default for the rest of the run. */
    lfg_ct_self_set_strict_xpass(0);
}

/* --strict-xpass argument-parsing coverage -- complements the bucket
 * behavior tests above. */

static void test_disposition_parse_strict_xpass_flag(void)
{
    char *argv_strict[] = {(char *)"prog", (char *)"--strict-xpass"};
    char *argv_plain[] = {(char *)"prog"};

    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv_strict));
    /* Parsing the flag must not require an argument. Re-parsing without
     * the flag resets it. */
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv_plain));
}

static void suite_disposition_tests(void)
{
    lfg_ct_test(NULL, test_disposition_skip_from_body_increments_skipped_bucket, NULL);
    lfg_ct_test(NULL, test_disposition_skip_from_setup_skips_body_runs_teardown, NULL);
    lfg_ct_test(NULL, test_disposition_xfail_with_assertion_failure_buckets_as_xfail, NULL);
    lfg_ct_test(NULL, test_disposition_xfail_without_failure_buckets_as_xpass, NULL);
    lfg_ct_test(NULL, test_disposition_xfail_repeated_calls_keep_last_reason, NULL);
    lfg_ct_test(NULL, test_disposition_skip_after_nested_test_impl_still_unwinds, NULL);
    lfg_ct_test(NULL, test_disposition_suite_context_skip_is_benign_no_op, NULL);
    lfg_ct_test(NULL, test_disposition_xpass_strict_flag_toggles_return_code, NULL);
    lfg_ct_test(NULL, test_disposition_parse_strict_xpass_flag, NULL);
}

/* ============================================================================
 * TEST SUITES
 * ============================================================================ */

static void suite_passing_tests(void)
{
    lfg_ct_test(NULL, test_pointer_assertions_pass, NULL);
    lfg_ct_test(NULL, test_boolean_assertions_pass, NULL);
    lfg_ct_test(NULL, test_integer_assertions_pass, NULL);
    lfg_ct_test(NULL, test_string_assertions_pass, NULL);
    lfg_ct_test(NULL, test_memory_assertions_pass, NULL);
    lfg_ct_test(NULL, test_comparison_assertions_pass, NULL);
    lfg_ct_test(NULL, test_range_assertion_pass, NULL);
    lfg_ct_test(NULL, test_bit_assertions_pass, NULL);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ct_test(NULL, test_float_assertions_pass, NULL);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ct_test(NULL, test_double_assertions_pass, NULL);
#endif
}

static void suite_hook_lifecycle_tests(void)
{
    lfg_ct_test(NULL, test_test_hooks_happy_path, NULL);
    lfg_ct_test(NULL, test_test_hooks_body_failure_still_runs_teardown, NULL);
    lfg_ct_test(NULL, test_test_hooks_null_setup_skips_setup_phase, NULL);
    lfg_ct_test(NULL, test_test_hooks_null_teardown_skips_teardown_phase, NULL);
    lfg_ct_test(NULL, test_test_hooks_setup_failure_skips_body_runs_teardown, NULL);
    lfg_ct_test(NULL, test_suite_hooks_happy_path, NULL);
    lfg_ct_test(NULL, test_suite_hooks_body_failure_still_runs_teardown, NULL);
    lfg_ct_test(NULL, test_suite_hooks_null_setup_skips_setup_phase, NULL);
    lfg_ct_test(NULL, test_suite_hooks_null_teardown_skips_teardown_phase, NULL);
    lfg_ct_test(NULL, test_suite_hooks_setup_failure_skips_body_runs_teardown, NULL);
    lfg_ct_test(NULL, test_suite_wrapping_test_fires_hooks_in_nesting_order, NULL);
}

static void suite_failure_detection_tests(void)
{
    lfg_ct_test(NULL, test_pointer_failure_detection, NULL);
    lfg_ct_test(NULL, test_boolean_failure_detection, NULL);
    lfg_ct_test(NULL, test_integer_failure_detection, NULL);
    lfg_ct_test(NULL, test_integer64_failure_detection, NULL);
    lfg_ct_test(NULL, test_string_failure_detection, NULL);
    lfg_ct_test(NULL, test_memory_failure_detection, NULL);
    lfg_ct_test(NULL, test_comparison_failure_detection, NULL);
    lfg_ct_test(NULL, test_range_failure_detection, NULL);
    lfg_ct_test(NULL, test_bit_failure_detection, NULL);
    lfg_ct_test(NULL, test_explicit_fail_detection, NULL);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ct_test(NULL, test_float_failure_detection, NULL);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ct_test(NULL, test_double_failure_detection, NULL);
#endif
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }

    lfg_ct_start();

    printf("\n");
    printf("================================================================================\n");
    printf("                    lfg-ctest UNIFIED TEST SUITE\n");
    printf("================================================================================\n");
    printf("\n");
    printf("This suite exercises all assertions with both passing tests and\n");
    printf("failure detection tests to verify the framework works correctly.\n");
    printf("\n");

    printf("--- SUITE 1: PASSING TESTS ---\n");
    lfg_ct_suite(NULL, suite_passing_tests, NULL);

    printf("\n--- SUITE 2: FAILURE DETECTION TESTS ---\n");
    printf("(Verifies the framework correctly detects assertion failures)\n");
    lfg_ct_suite(NULL, suite_failure_detection_tests, NULL);

    printf("\n--- SUITE 3: SETUP/TEARDOWN HOOK LIFECYCLE TESTS ---\n");
    printf("(Verifies setup -> body -> teardown sequencing and NULL handling)\n");
    lfg_ct_suite(NULL, suite_hook_lifecycle_tests, NULL);

    printf("\n--- SUITE 4: --list / --filter / --filter-exclude ARG PARSING ---\n");
    printf("(Verifies lfg_ct_parse_args and the runner's filter-state consumption)\n");
    lfg_ct_suite(NULL, suite_filter_args_tests, NULL);

    printf("\n--- SUITE 5: skip / xfail / xpass DISPOSITION TESTS ---\n");
    printf("(Verifies lfg_ct_skip / lfg_ct_xfail bucketing and --strict-xpass)\n");
    lfg_ct_suite(NULL, suite_disposition_tests, NULL);

    printf("\n");
    printf("================================================================================\n");
    printf("                         FINAL TEST SUMMARY\n");
    printf("================================================================================\n");
    lfg_ct_print_summary();

    return lfg_ct_return();
}
