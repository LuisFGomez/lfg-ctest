/**
 * @file
 * @brief   Smoke test for the amalgamated single-header form.
 *
 * Exercises the generated @c dist/lfg-ctest.h with its own
 * @c LFG_CTEST_IMPLEMENTATION definition. Catches drift between the split
 * sources and the amalgamated output (missing file in the manifest,
 * guard-stripping regressions, mock-macro expansion breakage, etc.).
 *
 * Kept deliberately small -- this is a drop-in build of the header,
 * not a replacement for the full self-test coverage.
 */

#define LFG_CTEST_IMPLEMENTATION
#include "dist/lfg-ctest.h"

/*============================================================================
 *  Mock under test
 *==========================================================================*/

DECLARE_MOCK_R_2(amalg_add, int, int, int);
DEFINE_MOCK_R_2(amalg_add, int, int, int)

/*============================================================================
 *  Tests
 *==========================================================================*/

static void
test_assertions(void)
{
    ASSERT_TRUE(1);
    ASSERT_FALSE(0);
    ASSERT_INT_EQUAL(42, 42);
    ASSERT_UINT_EQUAL(7u, 7u);
    ASSERT_STR_EQUAL("hello", "hello");
    ASSERT_GREATER_THAN(10, 5);
    ASSERT_IN_RANGE(5, 0, 10);
}

static void
test_mock_return_and_params(void)
{
    amalg_add__mock_reset();
    amalg_add__return_queue[0] = 30;
    amalg_add__return_queue[1] = 15;

    int r0 = amalg_add__mock(10, 20);
    int r1 = amalg_add__mock(5, 10);

    ASSERT_INT_EQUAL(30, r0);
    ASSERT_INT_EQUAL(15, r1);
    ASSERT_INT_EQUAL(2, (int)amalg_add__call_count);
    ASSERT_INT_EQUAL(10, amalg_add__param_history[0].p0);
    ASSERT_INT_EQUAL(20, amalg_add__param_history[0].p1);
    ASSERT_INT_EQUAL(5, amalg_add__param_history[1].p0);
    ASSERT_INT_EQUAL(10, amalg_add__param_history[1].p1);
}

#ifdef LFG_CTEST_HAS_FLOAT
static void
test_float_assertions(void)
{
    ASSERT_FLOAT_EQUAL(1.5f, 1.5f, 0.0001f);
    ASSERT_FLOAT_EQUAL(1.0f, 1.0001f, 0.001f);
}
#endif

#ifdef LFG_CTEST_HAS_DOUBLE
static void
test_double_assertions(void)
{
    ASSERT_DOUBLE_EQUAL(2.25, 2.25, 0.0001);
    ASSERT_DOUBLE_EQUAL(3.14, 3.14159, 0.01);
}
#endif

/*============================================================================
 *  Suite
 *==========================================================================*/

static void
smoke_suite(void)
{
    lfg_ctest(test_assertions);
    lfg_ctest(test_mock_return_and_params);
#ifdef LFG_CTEST_HAS_FLOAT
    lfg_ctest(test_float_assertions);
#endif
#ifdef LFG_CTEST_HAS_DOUBLE
    lfg_ctest(test_double_assertions);
#endif
}

int
main(void)
{
    lfg_ct_start();
    lfg_ct_suite(smoke_suite);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
