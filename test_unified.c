/*
 * Unified Test Suite for lfg-ctest
 *
 * This suite exercises all assertion macros with both passing and failing tests
 * to verify complete functionality of the testing framework.
 *
 * Floating-point assertions are only tested when LFG_CTEST_HAS_FLOAT is defined.
 * Use cmake -DLFG_CTEST_ENABLE_FLOAT=OFF to build without float support.
 */

#include "lfg_ctest.h"
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
    // Generic int
    ASSERT_INT_EQUAL(42, 42);
    ASSERT_INT_NOT_EQUAL(42, 43);
    ASSERT_EQ(100, 100);
    ASSERT_NE(100, 99);

    // Unsigned
    ASSERT_UINT_EQUAL(42U, 42U);
    ASSERT_UINT_NOT_EQUAL(42U, 43U);

    // Fixed-width signed
    ASSERT_INT8_EQUAL((int8_t)-128, (int8_t)-128);
    ASSERT_INT8_NOT_EQUAL((int8_t)127, (int8_t)-128);
    ASSERT_INT16_EQUAL((int16_t)-32768, (int16_t)-32768);
    ASSERT_INT16_NOT_EQUAL((int16_t)32767, (int16_t)-32768);
    ASSERT_INT32_EQUAL((int32_t)123456, (int32_t)123456);
    ASSERT_INT32_NOT_EQUAL((int32_t)123456, (int32_t)-123456);
    ASSERT_INT64_EQUAL((int64_t)9223372036854775807LL, (int64_t)9223372036854775807LL);
    ASSERT_INT64_NOT_EQUAL((int64_t)9223372036854775807LL, (int64_t)-9223372036854775807LL);

    // Fixed-width unsigned
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
    uint8_t value = 0b10101010;

    ASSERT_BIT_SET(value, 1);
    ASSERT_BIT_SET(value, 3);
    ASSERT_BIT_SET(value, 5);
    ASSERT_BIT_SET(value, 7);

    ASSERT_BIT_CLEAR(value, 0);
    ASSERT_BIT_CLEAR(value, 2);
    ASSERT_BIT_CLEAR(value, 4);
    ASSERT_BIT_CLEAR(value, 6);

    ASSERT_BITS_SET(value, 0b10100000);
    ASSERT_BITS_CLEAR(value, 0b01010101);
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
 * FAILING TESTS - All assertions should fail
 * ============================================================================ */

static void test_pointer_assertions_fail(void)
{
    int value1 = 42;
    int value2 = 43;
    int *ptr1 = &value1;
    int *ptr2 = &value2;
    int *ptr3 = NULL;

    ASSERT_PTR_EQUAL(ptr1, ptr2);           // FAIL: different pointers
    ASSERT_PTR_NOT_EQUAL(ptr1, ptr1);       // FAIL: same pointer
    ASSERT_PTR_NULL(ptr1);                  // FAIL: not null
    ASSERT_PTR_NOT_NULL(ptr3);              // FAIL: is null
    ASSERT_NULL(ptr1);                      // FAIL: not null
    ASSERT_NOT_NULL(ptr3);                  // FAIL: is null
}

static void test_boolean_assertions_fail(void)
{
    ASSERT_TRUE(0);                         // FAIL: false
    ASSERT_TRUE(2 < 1);                     // FAIL: false expression
    ASSERT_FALSE(1);                        // FAIL: true
    ASSERT_FALSE(5 > 3);                    // FAIL: true expression
}

static void test_integer_assertions_fail(void)
{
    // Generic int
    ASSERT_INT_EQUAL(42, 43);               // FAIL: not equal
    ASSERT_INT_NOT_EQUAL(42, 42);           // FAIL: equal
    ASSERT_EQ(100, 99);                     // FAIL: not equal
    ASSERT_NE(100, 100);                    // FAIL: equal

    // Unsigned
    ASSERT_UINT_EQUAL(42U, 43U);            // FAIL: not equal
    ASSERT_UINT_NOT_EQUAL(42U, 42U);        // FAIL: equal

    // Fixed-width signed
    ASSERT_INT8_EQUAL((int8_t)127, (int8_t)-128);           // FAIL
    ASSERT_INT8_NOT_EQUAL((int8_t)-128, (int8_t)-128);      // FAIL
    ASSERT_INT16_EQUAL((int16_t)32767, (int16_t)-32768);    // FAIL
    ASSERT_INT16_NOT_EQUAL((int16_t)-32768, (int16_t)-32768); // FAIL
    ASSERT_INT32_EQUAL((int32_t)123456, (int32_t)-123456);  // FAIL
    ASSERT_INT32_NOT_EQUAL((int32_t)123456, (int32_t)123456); // FAIL
    ASSERT_INT64_EQUAL((int64_t)9223372036854775807LL, (int64_t)-9223372036854775807LL); // FAIL
    ASSERT_INT64_NOT_EQUAL((int64_t)9223372036854775807LL, (int64_t)9223372036854775807LL); // FAIL

    // Fixed-width unsigned
    ASSERT_UINT8_EQUAL((uint8_t)255, (uint8_t)0);           // FAIL
    ASSERT_UINT8_NOT_EQUAL((uint8_t)255, (uint8_t)255);     // FAIL
    ASSERT_UINT16_EQUAL((uint16_t)65535, (uint16_t)0);      // FAIL
    ASSERT_UINT16_NOT_EQUAL((uint16_t)65535, (uint16_t)65535); // FAIL
    ASSERT_UINT32_EQUAL((uint32_t)4294967295UL, (uint32_t)0); // FAIL
    ASSERT_UINT32_NOT_EQUAL((uint32_t)4294967295UL, (uint32_t)4294967295UL); // FAIL
    ASSERT_UINT64_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)0); // FAIL
    ASSERT_UINT64_NOT_EQUAL((uint64_t)18446744073709551615ULL, (uint64_t)18446744073709551615ULL); // FAIL
}

static void test_string_assertions_fail(void)
{
    const char *str1 = "hello";
    const char *str2 = "world";
    const char *str3 = "hello world";

    ASSERT_STR_EQUAL(str1, str2);           // FAIL: different strings
    ASSERT_STR_NOT_EQUAL(str1, str1);       // FAIL: same string
    ASSERT_STRN_EQUAL(str1, str3, 10);      // FAIL: first 10 chars differ
}

static void test_memory_assertions_fail(void)
{
    uint8_t buf1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t buf2[] = {0xFF, 0xFF, 0xFF, 0xFF};

    ASSERT_MEM_EQUAL(buf1, buf2, 4);        // FAIL: different memory
    ASSERT_MEM_NOT_EQUAL(buf1, buf1, 4);    // FAIL: same memory
}

static void test_comparison_assertions_fail(void)
{
    ASSERT_GREATER_THAN(5, 10);             // FAIL: 5 <= 10
    ASSERT_GT(50, 100);                     // FAIL: 50 <= 100

    ASSERT_LESS_THAN(10, 5);                // FAIL: 10 >= 5
    ASSERT_LT(100, 50);                     // FAIL: 100 >= 50

    ASSERT_GREATER_OR_EQUAL(5, 10);         // FAIL: 5 < 10
    ASSERT_GE(50, 100);                     // FAIL: 50 < 100

    ASSERT_LESS_OR_EQUAL(10, 5);            // FAIL: 10 > 5
    ASSERT_LE(100, 50);                     // FAIL: 100 > 50
}

static void test_range_assertion_fail(void)
{
    ASSERT_IN_RANGE(0, 1, 10);              // FAIL: 0 < 1
    ASSERT_IN_RANGE(11, 1, 10);             // FAIL: 11 > 10
    ASSERT_IN_RANGE(-5, 0, 10);             // FAIL: -5 < 0
}

static void test_bit_assertions_fail(void)
{
    uint8_t value = 0b10101010;

    ASSERT_BIT_SET(value, 0);               // FAIL: bit 0 is clear
    ASSERT_BIT_SET(value, 2);               // FAIL: bit 2 is clear

    ASSERT_BIT_CLEAR(value, 1);             // FAIL: bit 1 is set
    ASSERT_BIT_CLEAR(value, 3);             // FAIL: bit 3 is set

    ASSERT_BITS_SET(value, 0b11111111);     // FAIL: some bits clear
    ASSERT_BITS_CLEAR(value, 0b10000000);   // FAIL: bit 7 is set
}

static void test_explicit_fail(void)
{
    ASSERT_FAIL("This is an intentional failure for testing ASSERT_FAIL");
}

#ifdef LFG_CTEST_HAS_FLOAT
static void test_float_assertions_fail(void)
{
    /* Float equality with epsilon */
    ASSERT_FLOAT_EQUAL(1.0f, 2.0f, 0.1f);           // FAIL: diff > epsilon
    ASSERT_FLT_EQ(100.0f, 200.0f, 0.001f);          // FAIL: diff > epsilon

    /* Float not equal */
    ASSERT_FLOAT_NOT_EQUAL(1.0f, 1.0f, 0.1f);       // FAIL: they are equal
    ASSERT_FLT_NE(0.0f, 0.0001f, 0.001f);           // FAIL: diff < epsilon

    /* Float comparisons */
    ASSERT_FLOAT_GREATER_THAN(5.5f, 10.5f);         // FAIL: 5.5 <= 10.5
    ASSERT_FLT_GT(99.9f, 100.0f);                   // FAIL: 99.9 <= 100.0

    ASSERT_FLOAT_LESS_THAN(10.5f, 5.5f);            // FAIL: 10.5 >= 5.5
    ASSERT_FLT_LT(100.0f, 99.9f);                   // FAIL: 100.0 >= 99.9

    ASSERT_FLOAT_GREATER_OR_EQUAL(5.0f, 10.0f);     // FAIL: 5.0 < 10.0
    ASSERT_FLT_GE(50.0f, 100.0f);                   // FAIL: 50.0 < 100.0

    ASSERT_FLOAT_LESS_OR_EQUAL(10.0f, 5.0f);        // FAIL: 10.0 > 5.0
    ASSERT_FLT_LE(100.0f, 50.0f);                   // FAIL: 100.0 > 50.0

    /* Float range */
    ASSERT_FLOAT_IN_RANGE(0.5f, 1.0f, 10.0f);       // FAIL: 0.5 < 1.0
    ASSERT_FLOAT_IN_RANGE(11.0f, 1.0f, 10.0f);      // FAIL: 11.0 > 10.0
}
#endif

#ifdef LFG_CTEST_HAS_DOUBLE
static void test_double_assertions_fail(void)
{
    /* Double equality with epsilon */
    ASSERT_DOUBLE_EQUAL(1.0, 2.0, 0.1);             // FAIL: diff > epsilon
    ASSERT_DBL_EQ(1e10, 2e10, 1e4);                 // FAIL: diff > epsilon

    /* Double not equal */
    ASSERT_DOUBLE_NOT_EQUAL(1.0, 1.0, 0.1);         // FAIL: they are equal
    ASSERT_DBL_NE(0.0, 0.0001, 0.001);              // FAIL: diff < epsilon
}
#endif

/* ============================================================================
 * TEST SUITES
 * ============================================================================ */

static void suite_passing_tests(void)
{
    lfg_ctest(test_pointer_assertions_pass);
    lfg_ctest(test_boolean_assertions_pass);
    lfg_ctest(test_integer_assertions_pass);
    lfg_ctest(test_string_assertions_pass);
    lfg_ctest(test_memory_assertions_pass);
    lfg_ctest(test_comparison_assertions_pass);
    lfg_ctest(test_range_assertion_pass);
    lfg_ctest(test_bit_assertions_pass);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ctest(test_float_assertions_pass);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ctest(test_double_assertions_pass);
#endif
}

static void suite_failing_tests(void)
{
    lfg_ctest(test_pointer_assertions_fail);
    lfg_ctest(test_boolean_assertions_fail);
    lfg_ctest(test_integer_assertions_fail);
    lfg_ctest(test_string_assertions_fail);
    lfg_ctest(test_memory_assertions_fail);
    lfg_ctest(test_comparison_assertions_fail);
    lfg_ctest(test_range_assertion_fail);
    lfg_ctest(test_bit_assertions_fail);
    lfg_ctest(test_explicit_fail);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ctest(test_float_assertions_fail);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ctest(test_double_assertions_fail);
#endif
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void)
{
    lfg_ct_start();

    printf("\n");
    printf("================================================================================\n");
    printf("                    lfg-ctest UNIFIED TEST SUITE\n");
    printf("================================================================================\n");
    printf("\n");
    printf("This suite exercises all 49 assertions with both passing and failing tests.\n");
    printf("\n");

    printf("--- SUITE 1: PASSING TESTS (All assertions should PASS) ---\n");
    lfg_ct_suite(suite_passing_tests);

    printf("\n--- SUITE 2: FAILING TESTS (All assertions should FAIL) ---\n");
    lfg_ct_suite(suite_failing_tests);

    printf("\n");
    printf("================================================================================\n");
    printf("                         FINAL TEST SUMMARY\n");
    printf("================================================================================\n");
    lfg_ct_print_summary();

    printf("\n");
    printf("EXPECTED RESULTS:\n");
#if defined(LFG_CTEST_HAS_FLOAT) && defined(LFG_CTEST_HAS_DOUBLE)
    printf("  (Float and Double enabled)\n");
    printf("  - Suite 1: All tests PASS (10 tests with 85 passing assertions)\n");
    printf("  - Suite 2: All tests FAIL (11 tests with 75 failing assertions)\n");
    printf("  - Total: 21 tests, 10 pass, 11 fail, 160 total assertions\n");
#elif defined(LFG_CTEST_HAS_FLOAT)
    printf("  (Float enabled, Double disabled)\n");
    printf("  - Suite 1: All tests PASS (9 tests with 81 passing assertions)\n");
    printf("  - Suite 2: All tests FAIL (10 tests with 71 failing assertions)\n");
    printf("  - Total: 19 tests, 9 pass, 10 fail, 152 total assertions\n");
#elif defined(LFG_CTEST_HAS_DOUBLE)
    printf("  (Float disabled, Double enabled)\n");
    printf("  - Suite 1: All tests PASS (9 tests with 66 passing assertions)\n");
    printf("  - Suite 2: All tests FAIL (10 tests with 59 failing assertions)\n");
    printf("  - Total: 19 tests, 9 pass, 10 fail, 125 total assertions\n");
#else
    printf("  (Float and Double disabled)\n");
    printf("  - Suite 1: All tests PASS (8 tests with 62 passing assertions)\n");
    printf("  - Suite 2: All tests FAIL (9 tests with 55 failing assertions)\n");
    printf("  - Total: 17 tests, 8 pass, 9 fail, 117 total assertions\n");
#endif
    printf("\n");

    return lfg_ct_return();
}
