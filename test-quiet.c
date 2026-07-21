/**
 * @file
 * @brief       Self-tests for the quiet output contract (-q / --verbosity 0).
 *
 *  The rest of the verbosity coverage (in test-unified.c) asserts parse
 *  and accessor state: which level a given argv lands on. That is not
 *  the contract users depend on -- what quiet promises is a specific set
 *  of lines present and a specific set absent, on the real stdout bytes.
 *  A regression that dropped a @c !_quiet_mode() guard, or that gated a
 *  line the issue lists as retained, would leave every accessor test
 *  green.
 *
 *  Each case therefore runs a scenario in a forked child whose stdout is
 *  a regular file, then greps the captured bytes in the parent. The child
 *  drives genuinely unsuppressed failures -- its own tallies go red --
 *  but it @c _exit()s with a verdict code and never reports a summary
 *  upward, so the outer binary stays green. Every quiet assertion is
 *  paired with a default-verbosity control so a case cannot pass by
 *  capturing nothing at all.
 */

#include "lfg-ctest.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ============================================================================
 *  Capture harness
 * ========================================================================== */

#define _CAP_MAX 16384

/** Room for the caller's flags plus the --state-file pair the harness
 *  appends. */
#define _CAP_ARGV_MAX 16

/** Child exit codes for harness faults, kept clear of the runner's own
 *  0/1 verdict range so a broken capture never reads as a test verdict. */
#define _CAP_ERR_FREOPEN 120
#define _CAP_ERR_PARSE 121

typedef void (*_scenario_fn)(void);

/**
 * @brief   Read @p path into @p buf, reporting overflow rather than
 *          truncating silently.
 *
 *  Truncation would make the suppression assertions fail open: an
 *  @c ASSERT_NULL(strstr(out, "*** test SKIP:")) on a clipped capture
 *  passes for the wrong reason. Caller turns a 0 here into a harness
 *  fault, which the existing @c ASSERT_INT_EQUAL(1, _capture(...))
 *  already catches.
 *
 *  @return 1 on a complete read, 0 if unreadable or larger than @p cap.
 */
static int
_slurp(const char *path, char *buf, size_t cap)
{
    FILE *f;
    size_t n;

    buf[0] = '\0';
    f = fopen(path, "r");
    if (NULL == f)
    {
        return 0;
    }
    /* Read the full cap, not cap - 1: a read that fills the buffer leaves
     * no room for the terminator, which is exactly the overflow case. */
    n = fread(buf, 1, cap, f);
    fclose(f);
    if (n >= cap)
    {
        buf[cap - 1] = '\0';
        return 0;
    }
    buf[n] = '\0';
    return 1;
}

/**
 * @brief   Run @p scenario in a capture child and collect its stdout.
 *
 *  @c freopen (not @c dup2) is deliberate, matching test-fork.c: it also
 *  resets stdout to fully buffered, which is the shape a redirected real
 *  run has.
 *
 *  The child reports a summary, which persists rerun state; the harness
 *  hands it a private temp path for that. Parking it on /dev/null instead
 *  would make every capture spray a write warning onto the real stderr.
 *
 *  @param  argc,argv   Flags the child parses; --state-file is appended.
 *  @param  scenario    Registration body; runs between start and summary.
 *  @param  out         Receives the captured bytes.
 *  @param  cap         Size of @p out.
 *  @param  exit_code   Receives the child's @c lfg_ct_return() value.
 *  @return 1 on a clean capture, 0 if the harness itself failed.
 */
static int
_capture(int argc, char **argv, _scenario_fn scenario, char *out, size_t cap, int *exit_code)
{
    char tmpl[] = "/tmp/lfg-ctest-quiet-outXXXXXX";
    char state_tmpl[] = "/tmp/lfg-ctest-quiet-stateXXXXXX";
    char *child_argv[_CAP_ARGV_MAX];
    int child_argc = 0;
    int i;
    int fd;
    pid_t pid;
    int status = 0;

    out[0] = '\0';
    *exit_code = -1;

    if (argc + 2 > _CAP_ARGV_MAX)
    {
        return 0;
    }

    fd = mkstemp(tmpl);
    if (fd < 0)
    {
        return 0;
    }
    close(fd);

    fd = mkstemp(state_tmpl);
    if (fd < 0)
    {
        remove(tmpl);
        return 0;
    }
    close(fd);

    for (i = 0; i < argc; i++)
    {
        child_argv[child_argc++] = argv[i];
    }
    child_argv[child_argc++] = (char *)"--state-file";
    child_argv[child_argc++] = state_tmpl;

    /* Drain the parent's buffered stdout before forking, or the child
     * would inherit a copy and re-emit it into the capture file. */
    fflush(NULL);

    pid = fork();
    if (pid < 0)
    {
        remove(tmpl);
        remove(state_tmpl);
        return 0;
    }
    if (0 == pid)
    {
        /* CAPTURE CHILD. */
        if (NULL == freopen(tmpl, "w", stdout))
        {
            _exit(_CAP_ERR_FREOPEN);
        }
        if (0 != lfg_ct_parse_args(child_argc, child_argv))
        {
            _exit(_CAP_ERR_PARSE);
        }
        lfg_ct_start();
        scenario();
        lfg_ct_print_summary();
        fflush(NULL);
        _exit(lfg_ct_return());
    }

    while (waitpid(pid, &status, 0) < 0 && EINTR == errno)
    {
    }

    if (!_slurp(tmpl, out, cap))
    {
        remove(tmpl);
        remove(state_tmpl);
        return 0;
    }
    remove(tmpl);
    remove(state_tmpl);

    if (!WIFEXITED(status))
    {
        return 0;
    }
    *exit_code = WEXITSTATUS(status);
    return 1;
}

/* ============================================================================
 *  Scenarios
 * ========================================================================== */

static void
_body_fail(void)
{
    ASSERT_TRUE(0);
}

static void
_body_pass(void)
{
    ASSERT_TRUE(1);
}

static void
_body_skip(void)
{
    lfg_ct_skip("quiet-skip-reason");
}

static void
_body_xfail(void)
{
    lfg_ct_xfail("quiet-xfail-reason");
    ASSERT_TRUE(0);
}

static void
_body_xpass(void)
{
    lfg_ct_xfail("quiet-xpass-reason");
    ASSERT_TRUE(1);
}

static void
_body_crash(void)
{
    abort();
}

static void
_suite_one_failure(void)
{
    lfg_ct_test_impl(_body_fail, "quiet_fail_inner");
}

static void
_suite_one_crash(void)
{
    lfg_ct_test_impl(_body_crash, "quiet_crash_inner");
}

/* Every outcome quiet is supposed to swallow, in one suite, so a single
 * capture answers the whole Suppressed set at once. */
static void
_suite_non_failures(void)
{
    lfg_ct_test_impl(_body_pass, "quiet_pass_inner");
    lfg_ct_test_impl(_body_skip, "quiet_skip_inner");
    lfg_ct_test_impl(_body_xfail, "quiet_xfail_inner");
    lfg_ct_test_impl(_body_xpass, "quiet_xpass_inner");
}

static void
_scn_failure(void)
{
    lfg_ct_suite(_suite_one_failure);
}

static void
_scn_non_failures(void)
{
    lfg_ct_suite(_suite_non_failures);
}

/**
 * @brief   True when this build links fork isolation in.
 *
 *  The quiet contract itself is POSIX-only (the capture harness needs
 *  fork/waitpid/mkstemp), but fork *isolation* is a separate compile-time
 *  opt-out: a -DLFG_CTEST_ENABLE_FORK=OFF Unix build still runs the
 *  twelve non-fork cases fine. Probed rather than #ifdef'd because
 *  LFG_CT_DISABLE_FORK is confined to the implementation TU -- test
 *  sources cannot see it. lfg_ct_set_isolation reports an unavailable
 *  mode instead of falling back, and leaves the mode unchanged on error,
 *  so the failed probe is side-effect free.
 */
static int
_fork_available(void)
{
    if (0 != lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK))
    {
        return 0;
    }
    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    return 1;
}

static void
_scn_failure_forked(void)
{
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);
    lfg_ct_suite(_suite_one_failure);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
}

static void
_scn_non_failures_forked(void)
{
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);
    lfg_ct_suite(_suite_non_failures);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
}

/* A crashing child never reports an outcome of its own, so the parent
 * synthesizes one -- the only route that reaches
 * _lfg_ct_record_external with print_outcome_line set. */
static void
_scn_crash_forked(void)
{
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);
    lfg_ct_suite(_suite_one_crash);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
}

/* ============================================================================
 *  Argv builders
 *
 *  Every run pins --seed so the seed line is stable across captures; the
 *  rerun state path is appended by _capture, which owns a private temp
 *  file per run. The children drive real failures, and a shared default
 *  state file would leave this binary's artifact describing runs that
 *  never happened.
 * ========================================================================== */

static char *_argv_default[] = {(char *)"prog", (char *)"--seed", (char *)"4242"};
static char *_argv_quiet[] = {(char *)"prog", (char *)"-q", (char *)"--seed", (char *)"4242"};
static char *_argv_verbosity0[]
        = {(char *)"prog", (char *)"--verbosity", (char *)"0", (char *)"--seed", (char *)"4242"};
static char *_argv_list_default[] = {(char *)"prog", (char *)"--list"};
static char *_argv_list_quiet[] = {(char *)"prog", (char *)"-q", (char *)"--list"};

#define _ARGC(_a) ((int)(sizeof(_a) / sizeof((_a)[0])))

/* ============================================================================
 *  Suppressed set
 * ========================================================================== */

static void
test_quiet_suppresses_banner_but_keeps_seed(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_non_failures, out, sizeof(out), &code));

    /* The seed survives on purpose: a quiet failing run nobody can
     * reproduce is a worse artifact than one extra line. */
    ASSERT_NULL(strstr(out, "*** begin unit test"));
    ASSERT_NOT_NULL(strstr(out, "*** random seed is 4242"));
}

static void
test_default_prints_banner_and_seed(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_default), _argv_default, _scn_non_failures, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** begin unit test"));
    ASSERT_NOT_NULL(strstr(out, "*** random seed is 4242"));
}

static void
test_quiet_suppresses_skip_xfail_xpass_lines(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_non_failures, out, sizeof(out), &code));

    ASSERT_NULL(strstr(out, "*** test SKIP:"));
    ASSERT_NULL(strstr(out, "*** test XFAIL:"));
    ASSERT_NULL(strstr(out, "*** test XPASS:"));
}

static void
test_default_prints_skip_xfail_xpass_lines(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_default), _argv_default, _scn_non_failures, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** test SKIP: quiet_skip_inner:"));
    ASSERT_NOT_NULL(strstr(out, "*** test XFAIL: quiet_xfail_inner:"));
    ASSERT_NOT_NULL(strstr(out, "*** test XPASS: quiet_xpass_inner:"));
}

static void
test_quiet_suppresses_suite_failure_line(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, out, sizeof(out), &code));

    /* The aggregate adds no localization over the per-test FAILURE line
     * plus the assertion detail, both of which are still below. */
    ASSERT_NULL(strstr(out, "*** suite FAILURE:"));
}

static void
test_default_prints_suite_failure_line(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_default), _argv_default, _scn_failure, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** suite FAILURE: _suite_one_failure"));
}

/* ============================================================================
 *  Retained set
 * ========================================================================== */

static void
test_quiet_keeps_test_failure_and_assertion_detail(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** test FAILURE: quiet_fail_inner"));
    ASSERT_NOT_NULL(strstr(out, "FAILURE in _body_fail()"));
}

static void
test_quiet_keeps_final_summary(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** Executed "));
    ASSERT_NOT_NULL(strstr(out, "*** Testing complete. Result: "));
}

static void
test_quiet_keeps_failure_summary_block(void)
{
    char out[_CAP_MAX];
    int code = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** Failure summary:"));
    ASSERT_NOT_NULL(strstr(out, "quiet_fail_inner"));
}

/* ============================================================================
 *  Fork-mode mirrors
 *
 *  Two distinct mechanisms produce these lines, and each needs its own
 *  case:
 *
 *  - A child that completes normally prints its own outcome line, gated
 *    by the in-process _quiet_mode() it inherited across the fork. The
 *    parent then records the result with print_outcome_line = 0.
 *  - A child that crashes or times out never reports, so the parent
 *    synthesizes a FAILED outcome with print_outcome_line = 1. That arm
 *    is deliberately ungated -- a naive "AND quiet into the outcome
 *    print" would kill the retained crash FAILURE line, which is the one
 *    diagnostic a quiet run of a crashing test has left.
 * ========================================================================== */

static void
test_fork_quiet_keeps_failure_and_detail(void)
{
    char out[_CAP_MAX];
    int code = 0;

    if (!_fork_available())
    {
        lfg_ct_skip("built without fork isolation");
    }

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure_forked, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** test FAILURE: quiet_fail_inner"));
    ASSERT_NOT_NULL(strstr(out, "FAILURE in _body_fail()"));
}

static void
test_fork_quiet_suppresses_skip_xfail_xpass_lines(void)
{
    char out[_CAP_MAX];
    int code = 0;

    if (!_fork_available())
    {
        lfg_ct_skip("built without fork isolation");
    }

    ASSERT_INT_EQUAL(
            1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_non_failures_forked, out, sizeof(out), &code));

    ASSERT_NULL(strstr(out, "*** test SKIP:"));
    ASSERT_NULL(strstr(out, "*** test XFAIL:"));
    ASSERT_NULL(strstr(out, "*** test XPASS:"));
}

static void
test_fork_quiet_keeps_synthesized_crash_failure(void)
{
    char out[_CAP_MAX];
    int code = 0;

    if (!_fork_available())
    {
        lfg_ct_skip("built without fork isolation");
    }

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_crash_forked, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** test FAILURE: quiet_crash_inner"));
    ASSERT_NOT_NULL(strstr(out, "fork-mode: killed by signal"));
}

static void
test_fork_default_prints_skip_xfail_xpass_lines(void)
{
    char out[_CAP_MAX];
    int code = 0;

    if (!_fork_available())
    {
        lfg_ct_skip("built without fork isolation");
    }

    ASSERT_INT_EQUAL(
            1, _capture(_ARGC(_argv_default), _argv_default, _scn_non_failures_forked, out, sizeof(out), &code));

    ASSERT_NOT_NULL(strstr(out, "*** test SKIP: quiet_skip_inner:"));
    ASSERT_NOT_NULL(strstr(out, "*** test XFAIL: quiet_xfail_inner:"));
    ASSERT_NOT_NULL(strstr(out, "*** test XPASS: quiet_xpass_inner:"));
}

/* ============================================================================
 *  Cross-level invariants
 * ========================================================================== */

static void
test_quiet_alias_and_verbosity_zero_are_byte_identical(void)
{
    char via_alias[_CAP_MAX];
    char via_level[_CAP_MAX];
    int alias_ok;
    int level_ok;
    int alias_code = 0;
    int level_code = 0;

    /* Both captures are taken before the first assertion: the summary
     * counts include this binary's running tallies, so an ASSERT
     * between them would make the two runs differ for a reason that
     * has nothing to do with verbosity. */
    alias_ok = _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, via_alias, sizeof(via_alias), &alias_code);
    level_ok = _capture(
            _ARGC(_argv_verbosity0), _argv_verbosity0, _scn_failure, via_level, sizeof(via_level), &level_code);

    ASSERT_INT_EQUAL(1, alias_ok);
    ASSERT_INT_EQUAL(1, level_ok);
    ASSERT_STR_EQUAL(via_alias, via_level);
    ASSERT_INT_EQUAL(alias_code, level_code);
}

static void
test_quiet_leaves_list_output_byte_identical(void)
{
    char plain[_CAP_MAX];
    char quiet[_CAP_MAX];
    int plain_ok;
    int quiet_ok;
    int plain_code = 0;
    int quiet_code = 0;

    /* --list is a machine-readable contract (#55): quiet must not add or
     * remove a byte, or `--list | grep | xargs --filter` breaks. */
    plain_ok = _capture(
            _ARGC(_argv_list_default), _argv_list_default, _scn_non_failures, plain, sizeof(plain), &plain_code);
    quiet_ok
            = _capture(_ARGC(_argv_list_quiet), _argv_list_quiet, _scn_non_failures, quiet, sizeof(quiet), &quiet_code);

    ASSERT_INT_EQUAL(1, plain_ok);
    ASSERT_INT_EQUAL(1, quiet_ok);
    ASSERT_STR_EQUAL(plain, quiet);
    ASSERT_INT_EQUAL(plain_code, quiet_code);
}

static void
test_quiet_does_not_change_exit_code(void)
{
    char out[_CAP_MAX];
    int fail_plain = 0;
    int fail_quiet = 0;
    int ok_plain = 0;
    int ok_quiet = 0;

    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_default), _argv_default, _scn_failure, out, sizeof(out), &fail_plain));
    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_failure, out, sizeof(out), &fail_quiet));
    ASSERT_INT_EQUAL(
            1, _capture(_ARGC(_argv_default), _argv_default, _scn_non_failures, out, sizeof(out), &ok_plain));
    ASSERT_INT_EQUAL(1, _capture(_ARGC(_argv_quiet), _argv_quiet, _scn_non_failures, out, sizeof(out), &ok_quiet));

    ASSERT_INT_EQUAL(fail_plain, fail_quiet);
    ASSERT_INT_EQUAL(ok_plain, ok_quiet);

    /* Pin the polarity too, or the equality above would still hold if
     * quiet somehow zeroed both. */
    ASSERT_TRUE(fail_plain != 0);
    ASSERT_INT_EQUAL(0, ok_plain);
}

/* ============================================================================
 *  Suite
 * ========================================================================== */

static void
suite_quiet_output_contract(void)
{
    lfg_ct_test(test_quiet_suppresses_banner_but_keeps_seed);
    lfg_ct_test(test_default_prints_banner_and_seed);
    lfg_ct_test(test_quiet_suppresses_skip_xfail_xpass_lines);
    lfg_ct_test(test_default_prints_skip_xfail_xpass_lines);
    lfg_ct_test(test_quiet_suppresses_suite_failure_line);
    lfg_ct_test(test_default_prints_suite_failure_line);
    lfg_ct_test(test_quiet_keeps_test_failure_and_assertion_detail);
    lfg_ct_test(test_quiet_keeps_final_summary);
    lfg_ct_test(test_quiet_keeps_failure_summary_block);
    lfg_ct_test(test_fork_quiet_keeps_failure_and_detail);
    lfg_ct_test(test_fork_quiet_suppresses_skip_xfail_xpass_lines);
    lfg_ct_test(test_fork_quiet_keeps_synthesized_crash_failure);
    lfg_ct_test(test_fork_default_prints_skip_xfail_xpass_lines);
    lfg_ct_test(test_quiet_alias_and_verbosity_zero_are_byte_identical);
    lfg_ct_test(test_quiet_leaves_list_output_byte_identical);
    lfg_ct_test(test_quiet_does_not_change_exit_code);
}

int
main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }
    lfg_ct_start();
    printf("\n--- QUIET OUTPUT CONTRACT SELF-TESTS ---\n");
    lfg_ct_suite(suite_quiet_output_contract);

    lfg_ct_print_summary();
    return lfg_ct_return();
}
