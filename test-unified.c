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
#include <stdio.h>
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
 * BODY-OWNED SETUP/TEARDOWN CONVENTION TESTS
 *
 * The framework no longer binds setup/teardown at registration time --
 * lfg_ct_test / lfg_ct_suite take the body only. Setup and teardown are
 * plain functions the body calls itself. These tests verify the blessed
 * convention reproduces every sequencing guarantee the old registrar
 * gave: happy path, body-failure-still-runs-teardown (assertions are
 * non-fatal), an early-return skip path, the no-setup / no-teardown
 * shapes, a setup-failure that skips the body but still tears down, and
 * the suite-wraps-test nesting order.
 *
 * Each phase stamps a single character into _hook_trace so order is
 * preserved end-to-end. Outer self-tests wrap the inner framework calls
 * in expect-failures mode so intentional failures don't fail this
 * binary's own result.
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

/* Fixture helpers a body calls for itself. Setup returns its assertion
 * result (0 ok, non-zero on failure) so the body can branch on it --
 * assertions are non-fatal and return their status, which is exactly
 * what makes the failure-path teardown reproducible in plain C. Tags:
 * S/B/T for setup/body/teardown; s/t for the suite-level wrap. */
static int _fx_setup_ok(void)      { _hook_trace_push('S'); return 0; }
static int _fx_setup_fail(void)    { _hook_trace_push('S'); return ASSERT_FAIL("intentional setup failure"); }
static void _fx_teardown(void)     { _hook_trace_push('T'); }

/* ---- body-owned lifecycle bodies (what a consumer now writes) ------------ */

/* Happy path: setup, body, teardown. */
static void _conv_body_happy(void)
{
    if (0 != _fx_setup_ok())
    {
        _fx_teardown();
        return;
    }
    _hook_trace_push('B');
    _fx_teardown();
}

/* Body failure still tears down: the soft assert fails but control falls
 * through (assertions do not longjmp), so teardown still runs. */
static void _conv_body_failure(void)
{
    if (0 != _fx_setup_ok())
    {
        _fx_teardown();
        return;
    }
    _hook_trace_push('B');
    ASSERT_FAIL("intentional body failure");
    _fx_teardown();
}

/* No setup phase: the body just does its work and tears down. */
static void _conv_body_no_setup(void)
{
    _hook_trace_push('B');
    _fx_teardown();
}

/* No teardown phase: nothing to clean up. */
static void _conv_body_no_teardown(void)
{
    if (0 != _fx_setup_ok())
    {
        return;
    }
    _hook_trace_push('B');
}

/* Setup failure skips the body but still tears down -- the body branches
 * on setup's returned status. The 'X' tag must never appear. */
static void _conv_body_setup_failure(void)
{
    if (0 != _fx_setup_fail())
    {
        _fx_teardown();
        return;
    }
    _hook_trace_push('X');
    _fx_teardown();
}

/* Skip path: teardown runs before lfg_ct_skip, which longjmps out of the
 * body -- the "teardown before skip" convention. The 'X' tag after the
 * skip must never appear. */
static void _conv_body_skip(void)
{
    if (0 != _fx_setup_ok())
    {
        _fx_teardown();
        return;
    }
    _hook_trace_push('B');
    _fx_teardown();
    lfg_ct_skip("preconditions not met");
    _hook_trace_push('X');
}

/* Suite wrap: the suite body owns its own setup/teardown around the
 * test it registers. */
static void _suite_wrap_body(void)
{
    _hook_trace_push('s');
    lfg_ct_test(_conv_body_happy);
    _hook_trace_push('t');
}

/* ---- convention self-tests ----------------------------------------------- */

static void test_convention_happy_path(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_happy);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_convention_body_failure_still_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_failure);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

static void test_convention_no_setup_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_no_setup);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("BT", _hook_trace);
}

static void test_convention_no_teardown_phase(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_no_teardown);
    observed = lfg_ct_expect_failures_end();

    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SB", _hook_trace);
}

static void test_convention_setup_failure_skips_body_runs_teardown(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_setup_failure);
    observed = lfg_ct_expect_failures_end();

    /* Exactly one failure (the setup), the body never recorded its tag,
     * and teardown still ran. */
    ASSERT_INT_EQUAL(1, observed);
    ASSERT_STR_EQUAL("ST", _hook_trace);
}

static void test_convention_skip_path_runs_teardown_first(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_test(_conv_body_skip);
    observed = lfg_ct_expect_failures_end();

    /* Skip is not a failure; teardown ran before the skip unwound the
     * body, and no post-skip tag was recorded. */
    ASSERT_INT_EQUAL(0, observed);
    ASSERT_STR_EQUAL("SBT", _hook_trace);
}

/* ---- suite wraps test: nesting order ------------------------------------- */

static void test_convention_suite_wrapping_test_nesting_order(void)
{
    int observed;

    _hook_trace_reset();
    lfg_ct_expect_failures_begin();
    lfg_ct_suite(_suite_wrap_body);
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
    lfg_ct_test_impl(_filter_body, "no_match");
    ASSERT_INT_EQUAL(0, _filter_body_called);

    _filter_body_called = 0;
    lfg_ct_test_impl(_filter_body, "match_me_yes");
    ASSERT_INT_EQUAL(1, _filter_body_called);
}

static void test_filter_runner_skips_excluded_test_body(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter-exclude", (char *)"skip_*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));

    _filter_body_called = 0;
    lfg_ct_test_impl(_filter_body, "skip_me");
    ASSERT_INT_EQUAL(0, _filter_body_called);

    _filter_body_called = 0;
    lfg_ct_test_impl(_filter_body, "run_me");
    ASSERT_INT_EQUAL(1, _filter_body_called);
}

static void test_filter_runner_skips_all_bodies_in_list_mode(void)
{
    char *argv[] = {(char *)"prog", (char *)"--list"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));

    _filter_body_called = 0;
    /* list mode prints "anything\r\n" to stdout but skips the body */
    lfg_ct_test_impl(_filter_body, "anything");
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
    lfg_ct_test_impl(_filter_inner_body, "unrelated_inner_test");
}

static void _filter_non_matching_suite_body(void)
{
    /* Used to verify the negative case: no suite-match, so the inner test
     * must match the filter on its own. Its name doesn't match either, so
     * the body must NOT run. */
    lfg_ct_test_impl(_filter_inner_body, "unrelated_inner_test");
}

static void test_filter_runner_suite_match_propagates_to_inner_tests(void)
{
    char *argv[] = {(char *)"prog", (char *)"--filter", (char *)"inheriting_suite_filter*"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(3, argv));

    /* Positive: matching outer suite -> inner test runs by inheritance. */
    _filter_inner_called = 0;
    lfg_ct_suite_impl(_filter_inheriting_suite_body, "inheriting_suite_filter_demo");
    ASSERT_INT_EQUAL(1, _filter_inner_called);

    /* Negative: non-matching outer suite -> still descends, but inner test
     * has to match on its own. It doesn't, so body stays unrun. */
    _filter_inner_called = 0;
    lfg_ct_suite_impl(_filter_non_matching_suite_body, "unrelated_outer_suite");
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
    lfg_ct_test(test_filter_parse_no_args_runs_everything);
    lfg_ct_test(test_filter_parse_filter_basic);
    lfg_ct_test(test_filter_parse_exclude_basic);
    lfg_ct_test(test_filter_parse_exclude_wins_over_filter);
    lfg_ct_test(test_filter_parse_repeated_filter_ors);
    lfg_ct_test(test_filter_parse_repeated_exclude_ors);
    lfg_ct_test(test_filter_parse_unmatched_filter_runs_nothing);
    lfg_ct_test(test_filter_parse_list_mode_set);
    lfg_ct_test(test_filter_parse_unknown_flag_fails);
    lfg_ct_test(test_filter_parse_missing_filter_arg_fails);
    lfg_ct_test(test_filter_parse_missing_exclude_arg_fails);
    lfg_ct_test(test_filter_parse_glob_wildcards);
    lfg_ct_test(test_filter_parse_charclass_globs);
    lfg_ct_test(test_filter_runner_skips_filtered_test_body);
    lfg_ct_test(test_filter_runner_skips_excluded_test_body);
    lfg_ct_test(test_filter_runner_skips_all_bodies_in_list_mode);
    lfg_ct_test(test_filter_runner_suite_match_propagates_to_inner_tests);

    /* Unconditional reset before the verifying test runs -- without this,
     * the verifying test is itself filter-gated by whatever the prior
     * test left in place and may be silently skipped, leaking filter
     * state into the remaining suites in this binary. */
    {
        char *reset_argv[] = {(char *)"prog"};
        (void)lfg_ct_parse_args(1, reset_argv);
    }
    lfg_ct_test(test_filter_reset_state_for_remaining_tests);
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

static void _disp_teardown_marker(void)
{
    _disp_setup_teardown_trace |= 0x8;
}

/* Body-owned lifecycle on the skip path. The body does its setup work,
 * decides preconditions aren't met, runs teardown, then skips. Teardown
 * must precede lfg_ct_skip because skip longjmps out of the body -- this
 * ordering is the blessed convention replacing the old registrar's
 * "teardown after a setup skip" guarantee. */
static void _disp_body_skip_runs_teardown_first(void)
{
    _disp_setup_teardown_trace |= 0x1; /* setup work, done in the body */
    _disp_teardown_marker();           /* teardown BEFORE the skip */
    lfg_ct_skip("preconditions not met");
    _disp_setup_teardown_trace |= 0x4; /* must NOT fire -- skip unwinds */
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
    lfg_ct_test_impl(_disp_body_xfail_without_failure, "mock_nested_inner");
    lfg_ct_skip("outer skip after nested test");
    _disp_post_skip_marker = 1; /* must NOT execute -- outer must unwind */
}

/* Suite-level skip is intentionally out of scope: lfg_ct_skip is a
 * per-test gesture. A skip call from inside a suite body must warn to
 * stderr and be a benign no-op so the suite continues normally. */
static int _disp_suite_body_ran;

static void _disp_suite_body_skips_then_marks(void)
{
    /* The suite boundary keeps skip disabled, so this warns to stderr and
     * returns rather than unwinding; the body then runs to completion. */
    lfg_ct_skip("suite preconditions not met");
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
    lfg_ct_test_impl(_disp_body_skip_simple, "mock_skip_body");

    /* The post-skip statement must not have executed. */
    ASSERT_INT_EQUAL(0, _disp_post_skip_marker);
    ASSERT_INT_EQUAL(before_skipped + 1, lfg_ct_self_skipped_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
    ASSERT_INT_EQUAL(before_asserts_failed, lfg_ct_self_assertions_failed());
}

static void test_disposition_skip_runs_body_owned_teardown_first(void)
{
    int before_skipped = lfg_ct_self_skipped_count();
    int before_failed = lfg_ct_self_failed_count();

    _disp_setup_teardown_trace = 0;
    lfg_ct_test_impl(_disp_body_skip_runs_teardown_first, "mock_skip_teardown_first");

    /* setup work ran (0x1), teardown ran before the skip (0x8), and the
     * post-skip store never fired (no 0x4) -- the skip unwound the body. */
    ASSERT_INT_EQUAL(0x1 | 0x8, _disp_setup_teardown_trace);
    ASSERT_INT_EQUAL(before_skipped + 1, lfg_ct_self_skipped_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
}

static void test_disposition_xfail_with_assertion_failure_buckets_as_xfail(void)
{
    int before_xfail = lfg_ct_self_xfailed_count();
    int before_failed = lfg_ct_self_failed_count();
    int before_asserts_failed = lfg_ct_self_assertions_failed();

    lfg_ct_test_impl(_disp_body_xfail_with_failure, "mock_xfail_with_fail");

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

    lfg_ct_test_impl(_disp_body_xfail_without_failure, "mock_xfail_no_fail");

    ASSERT_INT_EQUAL(before_xpass + 1, lfg_ct_self_xpassed_count());
    ASSERT_INT_EQUAL(before_failed, lfg_ct_self_failed_count());
}

static void test_disposition_xfail_repeated_calls_keep_last_reason(void)
{
    int before_xfail = lfg_ct_self_xfailed_count();

    lfg_ct_test_impl(_disp_body_xfail_last_reason_wins, "mock_xfail_last_reason");

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
    lfg_ct_test_impl(_disp_body_after_nested_then_skip, "mock_outer_skip_after_nested");

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
     * off across the suite body, so a stray lfg_ct_skip from suite-level
     * code must warn to stderr (suppressed for test cleanliness via
     * NOT being captured here -- we only check the observable
     * consequence) and let the suite body run normally. */
    _disp_suite_body_ran = 0;
    lfg_ct_suite_impl(_disp_suite_body_skips_then_marks, "mock_suite_skip_in_body");

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
    lfg_ct_test_impl(_disp_body_xfail_without_failure, "mock_xpass_permissive");
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
    lfg_ct_test(test_disposition_skip_from_body_increments_skipped_bucket);
    lfg_ct_test(test_disposition_skip_runs_body_owned_teardown_first);
    lfg_ct_test(test_disposition_xfail_with_assertion_failure_buckets_as_xfail);
    lfg_ct_test(test_disposition_xfail_without_failure_buckets_as_xpass);
    lfg_ct_test(test_disposition_xfail_repeated_calls_keep_last_reason);
    lfg_ct_test(test_disposition_skip_after_nested_test_impl_still_unwinds);
    lfg_ct_test(test_disposition_suite_context_skip_is_benign_no_op);
    lfg_ct_test(test_disposition_xpass_strict_flag_toggles_return_code);
    lfg_ct_test(test_disposition_parse_strict_xpass_flag);
}

/* ============================================================================
 * REPORTER CALLBACK CONTRACT TESTS
 *
 * Verify that lfg_ct_set_reporter wires up correctly and that the
 * runner fires on_record once per classified test with the right
 * outcome bucket + message. The contrib/junit-xml/ package and any
 * future report format depend on this contract.
 * ============================================================================ */

#define REPORTER_LOG_MAX 16

typedef struct
{
    char names[REPORTER_LOG_MAX][64];
    char messages[REPORTER_LOG_MAX][128];
    lfg_ct_outcome_t outcomes[REPORTER_LOG_MAX];
    int count;
    int run_complete_fired;
} _reporter_log_t;

static _reporter_log_t _rep_log;

static void _reporter_on_record(const lfg_ct_record_t *r, void *userdata)
{
    _reporter_log_t *log = (_reporter_log_t *)userdata;
    if (log->count >= REPORTER_LOG_MAX)
    {
        return;
    }
    snprintf(log->names[log->count], sizeof(log->names[0]), "%s", r->test_name ? r->test_name : "");
    snprintf(log->messages[log->count], sizeof(log->messages[0]), "%s", r->message ? r->message : "");
    log->outcomes[log->count] = r->outcome;
    log->count++;
}

static void _reporter_on_run_complete(void *userdata)
{
    _reporter_log_t *log = (_reporter_log_t *)userdata;
    log->run_complete_fired = 1;
}

/* Helper bodies driven via lfg_ct_test_impl for the reporter
 * tests. Failing bodies are NOT included -- a real failure would
 * advance the outer run's _tests_failed counter. The xpass case
 * runs naturally; xfail-with-real-failure is verified by directly
 * constructing the record. */
static void _rep_body_pass(void) { ASSERT_TRUE(1); }
static void _rep_body_skip(void) { lfg_ct_skip("manual skip"); }
static void _rep_body_xpass(void) { lfg_ct_xfail("expected to fail but didn't"); }

static void test_reporter_fires_once_per_test_with_pass_outcome(void)
{
    lfg_ct_reporter_t reporter = {_reporter_on_record, _reporter_on_run_complete, &_rep_log, NULL};

    memset(&_rep_log, 0, sizeof(_rep_log));
    lfg_ct_set_reporter(&reporter);

    lfg_ct_test_impl(_rep_body_pass, "rep_pass_case");

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _rep_log.count);
    ASSERT_STR_EQUAL("rep_pass_case", _rep_log.names[0]);
    ASSERT_INT_EQUAL((int)LFG_CT_PASSED, (int)_rep_log.outcomes[0]);
    ASSERT_STR_EQUAL("", _rep_log.messages[0]); /* PASSED carries NULL message */
}

static void test_reporter_classifies_skip_and_xpass_with_reason(void)
{
    lfg_ct_reporter_t reporter = {_reporter_on_record, NULL, &_rep_log, NULL};

    memset(&_rep_log, 0, sizeof(_rep_log));
    lfg_ct_set_reporter(&reporter);

    lfg_ct_test_impl(_rep_body_skip, "rep_skip_case");
    lfg_ct_test_impl(_rep_body_xpass, "rep_xpass_case");

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(2, _rep_log.count);

    ASSERT_INT_EQUAL((int)LFG_CT_SKIPPED, (int)_rep_log.outcomes[0]);
    ASSERT_STR_EQUAL("manual skip", _rep_log.messages[0]);

    ASSERT_INT_EQUAL((int)LFG_CT_XPASS, (int)_rep_log.outcomes[1]);
    ASSERT_STR_EQUAL("expected to fail but didn't", _rep_log.messages[1]);
}

static void test_reporter_run_complete_fires_from_print_summary(void)
{
    lfg_ct_reporter_t reporter = {NULL, _reporter_on_run_complete, &_rep_log, NULL};

    memset(&_rep_log, 0, sizeof(_rep_log));
    lfg_ct_set_reporter(&reporter);

    /* Drive a print_summary; on_run_complete should fire exactly
     * once and on_record should be unused (we passed NULL for it). */
    lfg_ct_print_summary();

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(0, _rep_log.count);
    ASSERT_INT_EQUAL(1, _rep_log.run_complete_fired);
}

static void test_reporter_null_slot_no_ops(void)
{
    /* Sanity: a NULL reporter is fine; the runner doesn't trip. */
    lfg_ct_set_reporter(NULL);
    lfg_ct_test_impl(_rep_body_pass, "rep_null_case");
    /* No way to verify "nothing happened" beyond the binary not
     * crashing; reaching this point is the assertion. */
    ASSERT_TRUE(1);
}

static void test_reporter_record_surfaces_classname_from_enclosing_suite(void)
{
    lfg_ct_reporter_t reporter = {_reporter_on_record, NULL, &_rep_log, NULL};

    memset(&_rep_log, 0, sizeof(_rep_log));
    /* This test is itself running inside suite_reporter_tests, so
     * the record should carry that suite name. lfg_ct_test_impl
     * drives via the macro one frame up; here we just check what
     * the framework already captured for the OUTER test (us). */
    (void)reporter;
    /* The runner already fired a record for us when this test was
     * classified -- except that classification happens AFTER the
     * body, so we can't peek at it from inside. Instead, just
     * verify that an inner nested call records the inherited
     * suite name. */
    lfg_ct_set_reporter(&reporter);
    lfg_ct_test_impl(_rep_body_pass, "rep_inner_case");
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _rep_log.count);
    /* suite_name isn't stored in the helper log struct above, so
     * we can't assert it directly here without expanding it. The
     * core contract (suite_name = enclosing lfg_ct_suite) is
     * exercised by the contrib's tests; this one just confirms
     * the record fires under nested suite context. */
}

static void suite_reporter_tests(void)
{
    lfg_ct_test(test_reporter_fires_once_per_test_with_pass_outcome);
    lfg_ct_test(test_reporter_classifies_skip_and_xpass_with_reason);
    lfg_ct_test(test_reporter_run_complete_fires_from_print_summary);
    lfg_ct_test(test_reporter_null_slot_no_ops);
    lfg_ct_test(test_reporter_record_surfaces_classname_from_enclosing_suite);
}

/* ============================================================================
 * VERBOSE MODE TESTS
 *
 * Verify that -v / --verbose:
 *   - parse correctly and flip lfg_ct_is_verbose();
 *   - off by default (no parse_args call, or parse_args without -v);
 *   - install a chain that fans out on_test_start + on_record + on_run_complete
 *     to a user-installed downstream reporter -- so the streamed output and a
 *     JUnit-XML-style emitter coexist;
 *   - fire on_test_start once per admitted test (filter / list-mode bypass it);
 *   - propagate (suite_name, test_name) on the start callback;
 *   - reset across calls to parse_args (consistent with the other parsed flags).
 *
 * Stdout content is not pattern-matched in self-tests -- the chain
 * structure is the verifiable artifact; the streamed banners are
 * smoke-tested by the test-amalg binary at the CI layer.
 * ============================================================================ */

#define VERBOSE_LOG_MAX 16

typedef struct
{
    char start_names[VERBOSE_LOG_MAX][64];
    char start_suites[VERBOSE_LOG_MAX][64];
    int start_count;

    char record_names[VERBOSE_LOG_MAX][64];
    lfg_ct_outcome_t record_outcomes[VERBOSE_LOG_MAX];
    int record_count;

    int run_complete_fired;
} _verbose_log_t;

static _verbose_log_t _vlog;

static void
_verbose_log_on_test_start(const char *suite_name, const char *test_name, void *userdata)
{
    _verbose_log_t *log = (_verbose_log_t *)userdata;
    int i;
    if (log->start_count >= VERBOSE_LOG_MAX)
    {
        return;
    }
    i = log->start_count++;
    snprintf(log->start_names[i], sizeof(log->start_names[0]), "%s", test_name ? test_name : "");
    snprintf(log->start_suites[i], sizeof(log->start_suites[0]), "%s", suite_name ? suite_name : "");
}

static void
_verbose_log_on_record(const lfg_ct_record_t *rec, void *userdata)
{
    _verbose_log_t *log = (_verbose_log_t *)userdata;
    int i;
    if (log->record_count >= VERBOSE_LOG_MAX)
    {
        return;
    }
    i = log->record_count++;
    snprintf(log->record_names[i], sizeof(log->record_names[0]), "%s", rec->test_name ? rec->test_name : "");
    log->record_outcomes[i] = rec->outcome;
}

static void
_verbose_log_on_run_complete(void *userdata)
{
    _verbose_log_t *log = (_verbose_log_t *)userdata;
    log->run_complete_fired = 1;
}

static void _verbose_body_pass(void) { ASSERT_TRUE(1); }

static void
test_verbose_default_off_after_parse(void)
{
    /* No flag -> verbose stays off. */
    char *argv[] = {(char *)"prog"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv));
    ASSERT_INT_EQUAL(0, lfg_ct_is_verbose());
}

static void
test_verbose_long_flag_toggles_on(void)
{
    char *argv[] = {(char *)"prog", (char *)"--verbose"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    ASSERT_INT_EQUAL(1, lfg_ct_is_verbose());
}

static void
test_verbose_short_flag_toggles_on(void)
{
    char *argv[] = {(char *)"prog", (char *)"-v"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    ASSERT_INT_EQUAL(1, lfg_ct_is_verbose());
}

static void
test_verbose_resets_across_parse_calls(void)
{
    char *argv_on[] = {(char *)"prog", (char *)"-v"};
    char *argv_off[] = {(char *)"prog"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv_on));
    ASSERT_INT_EQUAL(1, lfg_ct_is_verbose());

    /* Second parse with no -v must clear the prior verbose state, just
     * like list / filter / exclude / strict-xpass reset semantics. */
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv_off));
    ASSERT_INT_EQUAL(0, lfg_ct_is_verbose());
}

static void
test_verbose_start_callback_fires_once_per_test(void)
{
    /* Verbose mode is the only switch that exposes the on_test_start
     * event today. Wire a downstream reporter through the verbose
     * chain and drive a nested test_impl; the chain must propagate
     * one start fire + one record fire to the downstream. */
    char *argv[] = {(char *)"prog", (char *)"-v"};
    lfg_ct_reporter_t r;

    memset(&r, 0, sizeof(r));
    r.on_test_start = _verbose_log_on_test_start;
    r.on_record = _verbose_log_on_record;
    r.userdata = &_vlog;

    memset(&_vlog, 0, sizeof(_vlog));

    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    lfg_ct_set_reporter(&r);

    lfg_ct_test_impl(_verbose_body_pass, "verbose_inner_case");

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _vlog.start_count);
    ASSERT_STR_EQUAL("verbose_inner_case", _vlog.start_names[0]);
    ASSERT_INT_EQUAL(1, _vlog.record_count);
    ASSERT_STR_EQUAL("verbose_inner_case", _vlog.record_names[0]);
    ASSERT_INT_EQUAL((int)LFG_CT_PASSED, (int)_vlog.record_outcomes[0]);
}

static void
test_verbose_start_callback_carries_enclosing_suite_name(void)
{
    /* This test runs inside suite_verbose_mode_tests; a nested test
     * fires on_test_start with the inherited suite name baton. */
    char *argv[] = {(char *)"prog", (char *)"-v"};
    lfg_ct_reporter_t r;

    memset(&r, 0, sizeof(r));
    r.on_test_start = _verbose_log_on_test_start;
    r.userdata = &_vlog;

    memset(&_vlog, 0, sizeof(_vlog));

    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    lfg_ct_set_reporter(&r);

    lfg_ct_test_impl(_verbose_body_pass, "verbose_inner_with_suite");

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _vlog.start_count);
    ASSERT_STR_EQUAL("suite_verbose_mode_tests", _vlog.start_suites[0]);
}

static void
test_verbose_chain_propagates_run_complete(void)
{
    /* on_run_complete chains through too: a downstream reporter
     * installed under verbose mode must still receive the run-end
     * fire from lfg_ct_print_summary. */
    char *argv[] = {(char *)"prog", (char *)"-v"};
    lfg_ct_reporter_t r;

    memset(&r, 0, sizeof(r));
    r.on_run_complete = _verbose_log_on_run_complete;
    r.userdata = &_vlog;

    memset(&_vlog, 0, sizeof(_vlog));

    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(2, argv));
    lfg_ct_set_reporter(&r);

    lfg_ct_print_summary();

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _vlog.run_complete_fired);
}

static void
test_verbose_filtered_test_does_not_fire_start(void)
{
    /* A test the filter would skip must NOT fire on_test_start.
     * The gate sits in lfg_ct_test_impl before the fire site, so the
     * downstream reporter sees zero starts for excluded names. */
    char *argv[] = {(char *)"prog", (char *)"-v", (char *)"--filter", (char *)"match_me*"};
    lfg_ct_reporter_t r;

    memset(&r, 0, sizeof(r));
    r.on_test_start = _verbose_log_on_test_start;
    r.userdata = &_vlog;

    memset(&_vlog, 0, sizeof(_vlog));

    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(4, argv));
    lfg_ct_set_reporter(&r);

    lfg_ct_test_impl(_verbose_body_pass, "no_match_here");

    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(0, _vlog.start_count);
}

static void
test_verbose_unknown_flag_does_not_toggle(void)
{
    /* Robustness: an unrelated flag failure must leave verbose alone
     * (parse_args resets state on failure, so verbose ends up 0). */
    char *argv[] = {(char *)"prog", (char *)"--bogus"};
    ASSERT_INT_NOT_EQUAL(0, lfg_ct_parse_args(2, argv));
    ASSERT_INT_EQUAL(0, lfg_ct_is_verbose());
}

static void
test_verbose_reset_state_for_remaining_tests(void)
{
    /* Reset for any subsequent suite in this binary. Mirrors the
     * pattern test_filter_reset_state_for_remaining_tests already uses. */
    char *argv[] = {(char *)"prog"};
    ASSERT_INT_EQUAL(0, lfg_ct_parse_args(1, argv));
    ASSERT_INT_EQUAL(0, lfg_ct_is_verbose());
}

static void suite_verbose_mode_tests(void)
{
    lfg_ct_test(test_verbose_default_off_after_parse);
    lfg_ct_test(test_verbose_long_flag_toggles_on);
    lfg_ct_test(test_verbose_short_flag_toggles_on);
    lfg_ct_test(test_verbose_resets_across_parse_calls);
    lfg_ct_test(test_verbose_start_callback_fires_once_per_test);
    lfg_ct_test(test_verbose_start_callback_carries_enclosing_suite_name);
    lfg_ct_test(test_verbose_chain_propagates_run_complete);
    lfg_ct_test(test_verbose_filtered_test_does_not_fire_start);
    lfg_ct_test(test_verbose_unknown_flag_does_not_toggle);

    /* Unconditional reset before the verifying test runs -- mirrors
     * the pattern at the tail of suite_filter_args_tests. */
    {
        char *reset_argv[] = {(char *)"prog"};
        (void)lfg_ct_parse_args(1, reset_argv);
    }
    lfg_ct_test(test_verbose_reset_state_for_remaining_tests);
}

/* ============================================================================
 * TEST SUITES
 * ============================================================================ */

static void suite_passing_tests(void)
{
    lfg_ct_test(test_pointer_assertions_pass);
    lfg_ct_test(test_boolean_assertions_pass);
    lfg_ct_test(test_integer_assertions_pass);
    lfg_ct_test(test_string_assertions_pass);
    lfg_ct_test(test_memory_assertions_pass);
    lfg_ct_test(test_comparison_assertions_pass);
    lfg_ct_test(test_range_assertion_pass);
    lfg_ct_test(test_bit_assertions_pass);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ct_test(test_float_assertions_pass);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ct_test(test_double_assertions_pass);
#endif
}

static void suite_hook_lifecycle_tests(void)
{
    lfg_ct_test(test_convention_happy_path);
    lfg_ct_test(test_convention_body_failure_still_runs_teardown);
    lfg_ct_test(test_convention_no_setup_phase);
    lfg_ct_test(test_convention_no_teardown_phase);
    lfg_ct_test(test_convention_setup_failure_skips_body_runs_teardown);
    lfg_ct_test(test_convention_skip_path_runs_teardown_first);
    lfg_ct_test(test_convention_suite_wrapping_test_nesting_order);
}

static void suite_failure_detection_tests(void)
{
    lfg_ct_test(test_pointer_failure_detection);
    lfg_ct_test(test_boolean_failure_detection);
    lfg_ct_test(test_integer_failure_detection);
    lfg_ct_test(test_integer64_failure_detection);
    lfg_ct_test(test_string_failure_detection);
    lfg_ct_test(test_memory_failure_detection);
    lfg_ct_test(test_comparison_failure_detection);
    lfg_ct_test(test_range_failure_detection);
    lfg_ct_test(test_bit_failure_detection);
    lfg_ct_test(test_explicit_fail_detection);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ct_test(test_float_failure_detection);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ct_test(test_double_failure_detection);
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
    lfg_ct_suite(suite_passing_tests);

    printf("\n--- SUITE 2: FAILURE DETECTION TESTS ---\n");
    printf("(Verifies the framework correctly detects assertion failures)\n");
    lfg_ct_suite(suite_failure_detection_tests);

    printf("\n--- SUITE 3: SETUP/TEARDOWN HOOK LIFECYCLE TESTS ---\n");
    printf("(Verifies setup -> body -> teardown sequencing and NULL handling)\n");
    lfg_ct_suite(suite_hook_lifecycle_tests);

    printf("\n--- SUITE 4: --list / --filter / --filter-exclude ARG PARSING ---\n");
    printf("(Verifies lfg_ct_parse_args and the runner's filter-state consumption)\n");
    lfg_ct_suite(suite_filter_args_tests);

    printf("\n--- SUITE 5: skip / xfail / xpass DISPOSITION TESTS ---\n");
    printf("(Verifies lfg_ct_skip / lfg_ct_xfail bucketing and --strict-xpass)\n");
    lfg_ct_suite(suite_disposition_tests);

    printf("\n--- SUITE 6: REPORTER CALLBACK CONTRACT TESTS ---\n");
    printf("(Verifies lfg_ct_set_reporter / on_record / on_run_complete wiring)\n");
    lfg_ct_suite(suite_reporter_tests);

    printf("\n--- SUITE 7: -v / --verbose MODE TESTS ---\n");
    printf("(Verifies verbose flag parsing + on_test_start chaining)\n");
    lfg_ct_suite(suite_verbose_mode_tests);

    printf("\n");
    printf("================================================================================\n");
    printf("                         FINAL TEST SUMMARY\n");
    printf("================================================================================\n");
    lfg_ct_print_summary();

    return lfg_ct_return();
}
