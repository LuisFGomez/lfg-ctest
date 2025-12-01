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
#include "lfg_ctest.h"

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

/*============================================================================
 *  Public API
 *==========================================================================*/

void lfg_ct_start(void)
{
    unsigned rand_seed = time(NULL) % 1000;
    printf("*** begin unit test\r\n");
    printf("*** random seed is %u\r\n", rand_seed);
    srand(rand_seed);
}

void lfg_ct_end(void)
{
}

int lfg_ct_suite_impl(int (*fn)(void), const char *name)
{
    _current_suite_failures = 0;
    if (fn())
    {
        printf("*** suite \"%s\" FAILURE \r\n", name);
    }
    return -_current_suite_failures;
}

int lfg_ct_impl(int (*fn)(void), const char *name)
{
    _tests_executed++;
    _current_test_failures = 0;
    if (fn())
    {
        _current_suite_failures++;
        printf("*** test FAILURE: %s\r\n", name);
    }
    else
    {
        _tests_passed++;
    }
    return -_current_test_failures;
}

void lfg_ct_print_summary(void)
{
    printf("*** Executed %d assertions in %d tests. Failures: %u\r\n"
           "*** Testing complete. Result: %s\r\n",
           _assertions_executed, _tests_executed, _tests_failed,
           _tests_failed ? "FAIL" : "PASS");
}

int lfg_ct_current_test_return(void)
{
    return -_current_test_failures;
}

int lfg_ct_current_suite_return(void)
{
    return -_current_suite_failures;
}

int lfg_ct_return(void)
{
    return -_tests_failed;
}

int lfg_ct_assert_false_impl(bool condition,
                                 char *filename,
                                 int line_no,
                                 const char *function,
                                 const char *condition_str)
{
    _assertions_executed++;
    if (condition)
    {
        printf("*** %s: %u: FAILURE: in %s(): %s should be false\r\n",
               filename, line_no, function, condition_str);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_true_impl(bool condition,
                                char *filename,
                                int line_no,
                                const char *function,
                                const char *condition_str)
{
    _assertions_executed++;
    if (!condition)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should be true\r\n",
               filename, line_no, function, condition_str);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int_equal_impl(int expected,
                                     int actual,
                                     char *filename,
                                     int line_no,
                                     const char *function,
                                     const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int_not_equal_impl(int expected,
                                         int actual,
                                         char *filename,
                                         int line_no,
                                         const char *function,
                                         const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint_equal_impl(unsigned expected,
                                      unsigned actual,
                                      char *filename,
                                      int line_no,
                                      const char *function,
                                      const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%u) should equal %u\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint_not_equal_impl(unsigned expected,
                                              unsigned actual,
                                              char *filename,
                                              int line_no,
                                              const char *function,
                                              const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %u\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint8_equal_impl(uint8_t expected,
                                       uint8_t actual,
                                       char *filename,
                                       int line_no,
                                       const char *function,
                                       const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%02X) should equal 0x%02X\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint8_not_equal_impl(uint8_t expected,
                                           uint8_t actual,
                                           char *filename,
                                           int line_no,
                                           const char *function,
                                           const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%02X\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint16_equal_impl(uint16_t expected,
                                        uint16_t actual,
                                        char *filename,
                                        int line_no,
                                        const char *function,
                                        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%04X) should equal 0x%04X\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}


int lfg_ct_assert_uint16_not_equal_impl(uint16_t expected,
                                            uint16_t actual,
                                            char *filename,
                                            int line_no,
                                            const char *function,
                                            const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%04X\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint32_equal_impl(uint32_t expected,
                                        uint32_t actual,
                                        char *filename,
                                        int line_no,
                                        const char *function,
                                        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should equal 0x%08X\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint32_not_equal_impl(uint32_t expected,
                                            uint32_t actual,
                                            char *filename,
                                            int line_no,
                                            const char *function,
                                            const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%08X\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_ptr_equal_impl(void *expected,
                                     void *actual,
                                     const char *filename,
                                     int line_no,
                                     const char *function,
                                     const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_ptr_not_equal_impl(void *expected,
                                         void *actual,
                                         const char *filename,
                                         int line_no,
                                         const char *function,
                                         const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %p\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_ptr_not_null(void *actual,
                                   const char *filename,
                                   int line_no,
                                   const char *function,
                                   const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not be NULL\r\n",
               filename, line_no, function, actual_expr_str);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}


int lfg_ct_assert_ptr_null(void *actual,
                       const char *filename,
                       int line_no,
                       const char *function,
                       const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should be NULL but is %p\r\n",
               filename, line_no, function, actual_expr_str, actual);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int8_equal_impl(int8_t expected,
                               int8_t actual,
                               char *filename,
                               int line_no,
                               const char *function,
                               const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int8_not_equal_impl(int8_t expected,
                                   int8_t actual,
                                   char *filename,
                                   int line_no,
                                   const char *function,
                                   const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int16_equal_impl(int16_t expected,
                                int16_t actual,
                                char *filename,
                                int line_no,
                                const char *function,
                                const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int16_not_equal_impl(int16_t expected,
                                    int16_t actual,
                                    char *filename,
                                    int line_no,
                                    const char *function,
                                    const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int32_equal_impl(int32_t expected,
                                int32_t actual,
                                char *filename,
                                int line_no,
                                const char *function,
                                const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should equal %d\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int32_not_equal_impl(int32_t expected,
                                    int32_t actual,
                                    char *filename,
                                    int line_no,
                                    const char *function,
                                    const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %d\r\n",
               filename, line_no, function, actual_expr_str, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int64_equal_impl(int64_t expected,
                                int64_t actual,
                                char *filename,
                                int line_no,
                                const char *function,
                                const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%lld) should equal %lld\r\n",
               filename, line_no, function, actual_expr_str,
               (long long)actual, (long long)expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_int64_not_equal_impl(int64_t expected,
                                    int64_t actual,
                                    char *filename,
                                    int line_no,
                                    const char *function,
                                    const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal %lld\r\n",
               filename, line_no, function, actual_expr_str, (long long)expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint64_equal_impl(uint64_t expected,
                                 uint64_t actual,
                                 char *filename,
                                 int line_no,
                                 const char *function,
                                 const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%016llX) should equal 0x%016llX\r\n",
               filename, line_no, function, actual_expr_str,
               (unsigned long long)actual, (unsigned long long)expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_uint64_not_equal_impl(uint64_t expected,
                                     uint64_t actual,
                                     char *filename,
                                     int line_no,
                                     const char *function,
                                     const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal 0x%016llX\r\n",
               filename, line_no, function, actual_expr_str,
               (unsigned long long)expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_str_equal_impl(const char *expected,
                              const char *actual,
                              char *filename,
                              int line_no,
                              const char *function,
                              const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n",
                   filename, line_no, function, actual_expr_str,
                   (void*)actual, (void*)expected);
            _tests_failed++;
            _assertions_failed++;
            return -1;
        }
    }
    else if (strcmp(expected, actual) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (\"%s\") should equal \"%s\"\r\n",
               filename, line_no, function, actual_expr_str, actual, expected);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_str_not_equal_impl(const char *expected,
                                  const char *actual,
                                  char *filename,
                                  int line_no,
                                  const char *function,
                                  const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) == 0))
    {
        printf("*** %s: %u: FAILURE in %s(): %s should not equal \"%s\"\r\n",
               filename, line_no, function, actual_expr_str,
               expected ? expected : "(null)");
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_strn_equal_impl(const char *expected,
                               const char *actual,
                               size_t n,
                               char *filename,
                               int line_no,
                               const char *function,
                               const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n",
                   filename, line_no, function, actual_expr_str,
                   (void*)actual, (void*)expected);
            _tests_failed++;
            _assertions_failed++;
            return -1;
        }
    }
    else if (strncmp(expected, actual, n) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (first %zu chars) does not match expected\r\n",
               filename, line_no, function, actual_expr_str, n);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_mem_equal_impl(const void *expected,
                              const void *actual,
                              size_t n,
                              char *filename,
                              int line_no,
                              const char *function,
                              const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            printf("*** %s: %u: FAILURE in %s(): %s (%p) should equal %p (NULL mismatch)\r\n",
                   filename, line_no, function, actual_expr_str, actual, expected);
            _tests_failed++;
            _assertions_failed++;
            return -1;
        }
    }
    else if (memcmp(expected, actual, n) != 0)
    {
        printf("*** %s: %u: FAILURE in %s(): %s memory (%zu bytes) does not match expected\r\n",
               filename, line_no, function, actual_expr_str, n);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_mem_not_equal_impl(const void *expected,
                                  const void *actual,
                                  size_t n,
                                  char *filename,
                                  int line_no,
                                  const char *function,
                                  const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) ||
        (expected != NULL && actual != NULL && memcmp(expected, actual, n) == 0))
    {
        printf("*** %s: %u: FAILURE in %s(): %s memory (%zu bytes) should not match\r\n",
               filename, line_no, function, actual_expr_str, n);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_greater_than_impl(int a,
                                 int b,
                                 char *filename,
                                 int line_no,
                                 const char *function,
                                 const char *a_expr_str,
                                 const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a > b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be > %s (%d)\r\n",
               filename, line_no, function, a_expr_str, a, b_expr_str, b);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_less_than_impl(int a,
                              int b,
                              char *filename,
                              int line_no,
                              const char *function,
                              const char *a_expr_str,
                              const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a < b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be < %s (%d)\r\n",
               filename, line_no, function, a_expr_str, a, b_expr_str, b);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_greater_or_equal_impl(int a,
                                     int b,
                                     char *filename,
                                     int line_no,
                                     const char *function,
                                     const char *a_expr_str,
                                     const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a >= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be >= %s (%d)\r\n",
               filename, line_no, function, a_expr_str, a, b_expr_str, b);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_less_or_equal_impl(int a,
                                  int b,
                                  char *filename,
                                  int line_no,
                                  const char *function,
                                  const char *a_expr_str,
                                  const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a <= b))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be <= %s (%d)\r\n",
               filename, line_no, function, a_expr_str, a, b_expr_str, b);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_in_range_impl(int val,
                             int min,
                             int max,
                             char *filename,
                             int line_no,
                             const char *function,
                             const char *val_expr_str)
{
    _assertions_executed++;
    if (!(val >= min && val <= max))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (%d) should be in range [%d, %d]\r\n",
               filename, line_no, function, val_expr_str, val, min, max);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_bit_set_impl(unsigned val,
                            unsigned bit,
                            char *filename,
                            int line_no,
                            const char *function,
                            const char *val_expr_str,
                            unsigned bit_num)
{
    _assertions_executed++;
    if (!(val & (1u << bit)))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bit %u set\r\n",
               filename, line_no, function, val_expr_str, val, bit_num);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_bit_clear_impl(unsigned val,
                              unsigned bit,
                              char *filename,
                              int line_no,
                              const char *function,
                              const char *val_expr_str,
                              unsigned bit_num)
{
    _assertions_executed++;
    if (val & (1u << bit))
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bit %u clear\r\n",
               filename, line_no, function, val_expr_str, val, bit_num);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_bits_set_impl(unsigned val,
                             unsigned mask,
                             char *filename,
                             int line_no,
                             const char *function,
                             const char *val_expr_str,
                             unsigned mask_val)
{
    _assertions_executed++;
    if ((val & mask) != mask)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bits 0x%08X set\r\n",
               filename, line_no, function, val_expr_str, val, mask_val);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_bits_clear_impl(unsigned val,
                               unsigned mask,
                               char *filename,
                               int line_no,
                               const char *function,
                               const char *val_expr_str,
                               unsigned mask_val)
{
    _assertions_executed++;
    if (val & mask)
    {
        printf("*** %s: %u: FAILURE in %s(): %s (0x%08X) should have bits 0x%08X clear\r\n",
               filename, line_no, function, val_expr_str, val, mask_val);
        _tests_failed++;
        _assertions_failed++;
        return -1;
    }
    _assertions_passed++;
    return 0;
}

int lfg_ct_assert_fail_impl(char *filename,
                        int line_no,
                        const char *function,
                        const char *message)
{
    _assertions_executed++;
    printf("*** %s: %u: FAILURE in %s(): %s\r\n",
           filename, line_no, function, message ? message : "Explicit failure");
    _tests_failed++;
    _assertions_failed++;
    return -1;
}

/*============================================================================
 *  Private Functions
 *==========================================================================*/

