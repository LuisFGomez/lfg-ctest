/**
 * @file
 * @brief       Self-test for the compile-time fork opt-out.
 *
 *  This TU is compiled with @c LFG_CT_DISABLE_FORK=1 and links the
 *  framework's sources directly (no static lib), so it builds the
 *  disabled-path stubs from @c lfg-ctest-fork.c into the binary. The
 *  test verifies the public contract documented for the opt-out:
 *
 *    - @c LFG_CT_ISOLATE_FORK is still a usable enum value (this file
 *      references it; if the enum were conditional, this TU would not
 *      compile).
 *    - @c lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK) returns a non-zero
 *      error code and leaves the configured mode unchanged -- no
 *      silent fallback to in-process or to a stub that pretends to
 *      fork.
 *    - The in-process isolation mode (@c LFG_CT_ISOLATE_NONE) still
 *      works in this build, so the binary is otherwise a complete
 *      framework consumer.
 */

#include "lfg-ctest.h"

#include <stdio.h>

static void
test_fork_isolate_enum_is_unconditionally_visible(void)
{
    /* The enum value must compile even with fork disabled. The cast
     * round-trip is the smoke check; this TU referencing the symbol
     * is the real proof. */
    lfg_ct_isolation_t v = LFG_CT_ISOLATE_FORK;
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_FORK, (int)v);
}

static void
test_fork_isolate_set_returns_error(void)
{
    /* Default after start is NONE. */
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_NONE, (int)lfg_ct_get_isolation());

    /* Selecting FORK on a fork-disabled build returns non-zero AND
     * leaves the configured mode at its previous value -- no silent
     * fallback. */
    ASSERT_TRUE(0 != lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK));
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_NONE, (int)lfg_ct_get_isolation());
}

static void
test_isolate_none_still_works(void)
{
    /* The in-process path is unaffected by the opt-out. */
    ASSERT_INT_EQUAL(0, lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE));
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_NONE, (int)lfg_ct_get_isolation());
}

static void
suite_fork_disabled_tests(void)
{
    lfg_ct_test(test_fork_isolate_enum_is_unconditionally_visible);
    lfg_ct_test(test_fork_isolate_set_returns_error);
    lfg_ct_test(test_isolate_none_still_works);
}

int
main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }
    lfg_ct_start();
    printf("\n--- FORK OPT-OUT (LFG_CT_DISABLE_FORK) SELF-TESTS ---\n");
    lfg_ct_suite(suite_fork_disabled_tests);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
