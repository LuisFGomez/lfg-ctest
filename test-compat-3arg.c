/**
 * @file
 * @brief       Self-test for the deprecated 3-argument registration shim.
 *
 *  This TU is compiled with @c LFG_CT_COMPAT_3ARG and links the framework's
 *  sources directly (no static lib -- the lib is built body-only), so it
 *  builds every @c \#ifdef LFG_CT_COMPAT_3ARG branch across
 *  @c lfg-ctest.c, @c lfg-ctest-mock.c, and @c lfg-ctest-fork.c into the
 *  binary. Without this target nothing compiles the shim, so a signature
 *  drift in the three-way split could regress silently for the one release
 *  the shim is promised to adopters.
 *
 *  Compile coverage is the primary guarantee (the whole file references the
 *  3-argument @c lfg_ct_test / @c lfg_ct_suite macros). On top of that it
 *  verifies the retained pre-#35 lifecycle contract behaviourally, using a
 *  per-scenario trace buffer that each hook appends to:
 *
 *    - setup -> body -> teardown ordering on the happy path;
 *    - a @c NULL setup or @c NULL teardown is tolerated and simply omitted;
 *    - a body-side @c lfg_ct_skip still runs teardown (teardown always runs
 *      when provided, even off the skip path).
 *
 *  Failure-path semantics (setup-assertion failure skips the body; teardown
 *  runs after a failing body) are intentionally out of scope here: driving
 *  them needs the expect-failures self-test harness and would colour the
 *  summary red. This target stays green so it slots into the default CI
 *  build/test run with no special handling.
 */

#include "lfg-ctest.h"

#include <stdio.h>
#include <string.h>

/* --- Execution trace -------------------------------------------------- */

/* Each scenario records its hook sequence into a distinct buffer so the
 * verification pass can assert on the exact order after the run. */
static char trace_fixtureless[8];
static char trace_full[8];
static char trace_no_setup[8];
static char trace_no_teardown[8];
static char trace_skip[8];

static char *_active_trace;

static void
rec(char c)
{
    size_t n;

    if (!_active_trace)
    {
        return;
    }
    n = strlen(_active_trace);
    /* -1 for the char, -1 for the terminator. */
    if (n + 2 <= sizeof(trace_full))
    {
        _active_trace[n] = c;
        _active_trace[n + 1] = '\0';
    }
}

static void setup_hook(void)    { rec('s'); }
static void body_hook(void)     { rec('b'); }
static void teardown_hook(void) { rec('t'); }

static void
body_skips(void)
{
    rec('b');
    lfg_ct_skip("compat-3arg skip-path coverage");
    /* Not reached: skip longjmps out of the body boundary. */
    rec('X');
}

/* --- Scenario drivers ------------------------------------------------- */

/* Each driver aims the trace at its buffer, then registers via the 3-arg
 * form. The framework runs the registered test synchronously, so the
 * buffer is complete by the time the driver returns. */

static void
scenario_fixtureless(void)
{
    _active_trace = trace_fixtureless;
    trace_fixtureless[0] = '\0';
    lfg_ct_test(NULL, body_hook, NULL);
}

static void
scenario_full(void)
{
    _active_trace = trace_full;
    trace_full[0] = '\0';
    lfg_ct_test(setup_hook, body_hook, teardown_hook);
}

static void
scenario_no_setup(void)
{
    _active_trace = trace_no_setup;
    trace_no_setup[0] = '\0';
    lfg_ct_test(NULL, body_hook, teardown_hook);
}

static void
scenario_no_teardown(void)
{
    _active_trace = trace_no_teardown;
    trace_no_teardown[0] = '\0';
    lfg_ct_test(setup_hook, body_hook, NULL);
}

static void
scenario_skip_runs_teardown(void)
{
    _active_trace = trace_skip;
    trace_skip[0] = '\0';
    lfg_ct_test(setup_hook, body_skips, teardown_hook);
}

static void
suite_compat_scenarios(void)
{
    /* Registered via the deprecated 3-arg suite form (setup/teardown NULL);
     * this alone compiles the lfg_ct_suite_impl COMPAT signature. */
    scenario_fixtureless();
    scenario_full();
    scenario_no_setup();
    scenario_no_teardown();
    scenario_skip_runs_teardown();
}

/* --- Verification ----------------------------------------------------- */

/* These run after suite_compat_scenarios, so every trace is settled. Under
 * LFG_CT_COMPAT_3ARG the 1-arg macro form does not exist, so the checks
 * register via the fixtureless 3-arg form -- exactly what an adopter
 * mid-migration compiles. */

static void
check_fixtureless(void)
{
    ASSERT_STR_EQUAL("b", trace_fixtureless);
}

static void
check_full(void)
{
    ASSERT_STR_EQUAL("sbt", trace_full);
}

static void
check_no_setup(void)
{
    ASSERT_STR_EQUAL("bt", trace_no_setup);
}

static void
check_no_teardown(void)
{
    ASSERT_STR_EQUAL("sb", trace_no_teardown);
}

static void
check_skip_runs_teardown(void)
{
    /* setup, body (records 'b' then skips), teardown -- the 'X' after the
     * skip must never appear. */
    ASSERT_STR_EQUAL("sbt", trace_skip);
}

static void
suite_compat_checks(void)
{
    lfg_ct_test(NULL, check_fixtureless, NULL);
    lfg_ct_test(NULL, check_full, NULL);
    lfg_ct_test(NULL, check_no_setup, NULL);
    lfg_ct_test(NULL, check_no_teardown, NULL);
    lfg_ct_test(NULL, check_skip_runs_teardown, NULL);
}

int
main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }
    lfg_ct_start();
    printf("\n--- 3-ARG COMPAT SHIM (LFG_CT_COMPAT_3ARG) SELF-TESTS ---\n");
    lfg_ct_suite(NULL, suite_compat_scenarios, NULL);
    lfg_ct_suite(NULL, suite_compat_checks, NULL);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
