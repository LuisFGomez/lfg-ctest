/**
 * @file
 * @brief       Self-tests for fork-per-test isolation (LFG_CT_ISOLATE_FORK).
 *
 *  Drives inner fork-mode tests from the outer in-process runner and
 *  verifies the dispatch contract: PASS / SKIP / XFAIL round-trip,
 *  crash-survival (abort / SIGSEGV), exit-code propagation, timeout
 *  reaping, and parent-side file-descriptor inheritance.
 *
 *  Tests that intentionally drive a FAILED inner outcome wrap the
 *  driver in @c lfg_ct_expect_failures_begin/end so the outer binary
 *  doesn't exit non-zero. The same suppression covers the synthetic
 *  failure shipped by @c _lfg_ct_record_external for signal / timeout
 *  paths.
 */

#include "lfg-ctest.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ============================================================================
 *  Reporter capture
 * ========================================================================== */

#define _FORK_LOG_SLOTS 8
#define _FORK_LOG_MSG_MAX 512
#define _FORK_LOG_NAME_MAX 128

typedef struct
{
    int count;
    lfg_ct_outcome_t outcomes[_FORK_LOG_SLOTS];
    char messages[_FORK_LOG_SLOTS][_FORK_LOG_MSG_MAX];
    char names[_FORK_LOG_SLOTS][_FORK_LOG_NAME_MAX];
} _fork_log_t;

static _fork_log_t _log;

static void
_fork_log_reset(void)
{
    memset(&_log, 0, sizeof(_log));
}

static void
_fork_on_record(const lfg_ct_record_t *record, void *userdata)
{
    _fork_log_t *log = (_fork_log_t *)userdata;
    int i;

    if (log->count >= _FORK_LOG_SLOTS)
    {
        return;
    }
    i = log->count++;
    log->outcomes[i] = record->outcome;
    snprintf(log->messages[i], _FORK_LOG_MSG_MAX, "%s", record->message ? record->message : "");
    snprintf(log->names[i], _FORK_LOG_NAME_MAX, "%s", record->test_name ? record->test_name : "");
}

static lfg_ct_reporter_t _capture = {_fork_on_record, NULL, &_log};

/* ============================================================================
 *  Inner bodies driven through the fork dispatcher
 * ========================================================================== */

static void
_body_pass(void)
{
    ASSERT_TRUE(1);
}

static void
_body_abort_crash(void)
{
    /* abort(3) raises SIGABRT; the child exits via a signal, parent
     * decodes WIFSIGNALED and records FAILED. */
    abort();
}

static void
_body_segfault_crash(void)
{
    /* Volatile to defeat clang's UB optimizer; we genuinely want the
     * null-deref to reach the page fault. */
    volatile int *p = NULL;
    *p = 42;
}

static void
_body_exit_nonzero(void)
{
    /* _exit bypasses the payload write -- parent sees WIFEXITED with
     * non-zero status and no payload, must report FAILED. */
    _exit(42);
}

static void
_body_sleep_forever(void)
{
    /* Sleep longer than any reasonable fork-mode timeout under test. */
    sleep(10);
}

static void
_body_skip(void)
{
    lfg_ct_skip("skip-from-fork-child");
    ASSERT_TRUE(0); /* unreachable */
}

static void
_body_xfail_then_fail(void)
{
    lfg_ct_xfail("xfail-from-fork-child");
    ASSERT_TRUE(0);
}

/* FD inheritance: outer test pre-loads _shared_read_fd before invoking
 * the fork dispatcher; the child reads from it via COW-inherited fd. */
static int _shared_read_fd = -1;

static void
_body_read_inherited_fd(void)
{
    char buf[6] = {0};
    ssize_t r = read(_shared_read_fd, buf, 5);
    ASSERT_INT_EQUAL(5, (int)r);
    ASSERT_STR_EQUAL("hello", buf);
}

/* ============================================================================
 *  Outer (in-process) tests
 * ========================================================================== */

static void
test_set_isolation_round_trip(void)
{
    /* Default is NONE; FORK is available on this build (Unix + opt-in). */
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_NONE, (int)lfg_ct_get_isolation());
    ASSERT_INT_EQUAL(0, lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK));
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_FORK, (int)lfg_ct_get_isolation());
    ASSERT_INT_EQUAL(0, lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE));
    ASSERT_INT_EQUAL((int)LFG_CT_ISOLATE_NONE, (int)lfg_ct_get_isolation());
}

static void
test_set_fork_timeout_round_trip(void)
{
    unsigned saved = lfg_ct_get_fork_timeout_ms();
    lfg_ct_set_fork_timeout_ms(1234);
    ASSERT_UINT_EQUAL(1234, lfg_ct_get_fork_timeout_ms());
    lfg_ct_set_fork_timeout_ms(saved);
    ASSERT_UINT_EQUAL(saved, lfg_ct_get_fork_timeout_ms());
}

static void
test_fork_pass_case(void)
{
    /* PASS round-trip: child runs the body, assertion passes,
     * classification = PASSED, payload reaches parent, reporter sees
     * PASSED for the inner test name. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_test_impl(NULL, _body_pass, NULL, "fork_pass_inner");

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_PASSED, (int)_log.outcomes[0]);
    ASSERT_STR_EQUAL("fork_pass_inner", _log.names[0]);
}

static void
test_fork_abort_crash_survives_and_reports_failed(void)
{
    /* The child calls abort() and dies on SIGABRT. Parent must NOT
     * propagate the signal (the binary continues). Reporter sees
     * FAILED with a "killed by signal" message. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_expect_failures_begin();
    lfg_ct_test_impl(NULL, _body_abort_crash, NULL, "fork_abort_inner");
    (void)lfg_ct_expect_failures_end();

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_FAILED, (int)_log.outcomes[0]);
    ASSERT_TRUE(NULL != strstr(_log.messages[0], "killed by signal"));
}

static void
test_fork_segfault_crash_survives_and_reports_failed(void)
{
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_expect_failures_begin();
    lfg_ct_test_impl(NULL, _body_segfault_crash, NULL, "fork_segv_inner");
    (void)lfg_ct_expect_failures_end();

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_FAILED, (int)_log.outcomes[0]);
    ASSERT_TRUE(NULL != strstr(_log.messages[0], "killed by signal"));
}

static void
test_fork_exit_nonzero_reports_failed(void)
{
    /* _exit(42) bypasses the pipe write; parent sees WIFEXITED + 42
     * with no payload. Must report FAILED, not silently pass. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_expect_failures_begin();
    lfg_ct_test_impl(NULL, _body_exit_nonzero, NULL, "fork_exit_inner");
    (void)lfg_ct_expect_failures_end();

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_FAILED, (int)_log.outcomes[0]);
    ASSERT_TRUE(NULL != strstr(_log.messages[0], "exited 42"));
}

static void
test_fork_timeout_kills_hung_child(void)
{
    /* sleep(10) inner against a 50ms parent timeout -- the child is
     * SIGKILL'd and the test is recorded as a timeout failure. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);
    lfg_ct_set_fork_timeout_ms(50);

    lfg_ct_expect_failures_begin();
    lfg_ct_test_impl(NULL, _body_sleep_forever, NULL, "fork_timeout_inner");
    (void)lfg_ct_expect_failures_end();

    lfg_ct_set_fork_timeout_ms(0);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_FAILED, (int)_log.outcomes[0]);
    ASSERT_TRUE(NULL != strstr(_log.messages[0], "timed out"));
}

static void
test_fork_subsequent_test_still_runs_after_crash(void)
{
    /* Two inner tests back to back: the first crashes, the second
     * passes. Crash survival means the runner reaches test #2. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_expect_failures_begin();
    lfg_ct_test_impl(NULL, _body_abort_crash, NULL, "fork_seq_crash");
    (void)lfg_ct_expect_failures_end();

    lfg_ct_test_impl(NULL, _body_pass, NULL, "fork_seq_pass");

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(2, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_FAILED, (int)_log.outcomes[0]);
    ASSERT_STR_EQUAL("fork_seq_crash", _log.names[0]);
    ASSERT_INT_EQUAL((int)LFG_CT_PASSED, (int)_log.outcomes[1]);
    ASSERT_STR_EQUAL("fork_seq_pass", _log.names[1]);
}

static void
test_fork_parent_fd_inheritance(void)
{
    /* Parent opens a temp file with known content, hands the read fd
     * to the child via COW inheritance. The child reads + asserts;
     * fork-mode passing means the read returns "hello". */
    char tmpl[] = "/tmp/lfg-ctest-fork-fdXXXXXX";
    int fd_write;
    ssize_t w;

    fd_write = mkstemp(tmpl);
    ASSERT_GT(fd_write, -1);
    w = write(fd_write, "hello", 5);
    ASSERT_INT_EQUAL(5, (int)w);
    close(fd_write);

    _shared_read_fd = open(tmpl, O_RDONLY);
    ASSERT_GT(_shared_read_fd, -1);

    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_test_impl(NULL, _body_read_inherited_fd, NULL, "fork_fd_inherit");

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    close(_shared_read_fd);
    _shared_read_fd = -1;
    unlink(tmpl);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_PASSED, (int)_log.outcomes[0]);
}

static void
test_fork_skip_round_trip(void)
{
    /* SKIP is per-test disposition; the child's in-process classification
     * fires the capture reporter with SKIPPED + reason, the payload
     * preserves both. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_test_impl(NULL, _body_skip, NULL, "fork_skip_inner");

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_SKIPPED, (int)_log.outcomes[0]);
    ASSERT_STR_EQUAL("skip-from-fork-child", _log.messages[0]);
}

static void
test_fork_xfail_round_trip(void)
{
    /* xfail body fails an assertion -> classifies XFAIL. The child's
     * in-process path absorbs the failure into the XFAIL bucket, so
     * the payload carries XFAIL with the reason. */
    _fork_log_reset();
    lfg_ct_set_reporter(&_capture);
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);

    lfg_ct_test_impl(NULL, _body_xfail_then_fail, NULL, "fork_xfail_inner");

    lfg_ct_set_isolation(LFG_CT_ISOLATE_NONE);
    lfg_ct_set_reporter(NULL);

    ASSERT_INT_EQUAL(1, _log.count);
    ASSERT_INT_EQUAL((int)LFG_CT_XFAIL, (int)_log.outcomes[0]);
    ASSERT_STR_EQUAL("xfail-from-fork-child", _log.messages[0]);
}

/* ============================================================================
 *  Suite + main
 * ========================================================================== */

static void
suite_fork_isolation_tests(void)
{
    lfg_ct_test(NULL, test_set_isolation_round_trip, NULL);
    lfg_ct_test(NULL, test_set_fork_timeout_round_trip, NULL);
    lfg_ct_test(NULL, test_fork_pass_case, NULL);
    lfg_ct_test(NULL, test_fork_abort_crash_survives_and_reports_failed, NULL);
    lfg_ct_test(NULL, test_fork_segfault_crash_survives_and_reports_failed, NULL);
    lfg_ct_test(NULL, test_fork_exit_nonzero_reports_failed, NULL);
    lfg_ct_test(NULL, test_fork_timeout_kills_hung_child, NULL);
    lfg_ct_test(NULL, test_fork_subsequent_test_still_runs_after_crash, NULL);
    lfg_ct_test(NULL, test_fork_parent_fd_inheritance, NULL);
    lfg_ct_test(NULL, test_fork_skip_round_trip, NULL);
    lfg_ct_test(NULL, test_fork_xfail_round_trip, NULL);
}

int
main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }
    lfg_ct_start();
    printf("\n--- FORK-PER-TEST ISOLATION SELF-TESTS ---\n");
    lfg_ct_suite(NULL, suite_fork_isolation_tests, NULL);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
