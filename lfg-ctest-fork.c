/**
 * @file
 * @brief       lfg-ctest fork-per-test isolation runner.
 *
 *  Implements the runtime support for @ref LFG_CT_ISOLATE_FORK. The
 *  platform gate (@c __unix__ / @c __APPLE__) and the compile-time
 *  opt-out (@c LFG_CT_DISABLE_FORK) are both scoped to this TU per
 *  the issue's "do not sprinkle @c #ifdef across the rest of the
 *  framework" constraint. When neither path is taken (e.g. a Windows
 *  consumer, or any consumer compiled with @c LFG_CT_DISABLE_FORK),
 *  the TU compiles to a pair of stubs that report the mode as
 *  unavailable and never reach any fork/waitpid/signal-handling
 *  symbols -- so the linker drops the dependency entirely.
 *
 *  Dispatch shape (Unix path):
 *    1. Parent @c pipe(2) + @c fork(2).
 *    2. Child swaps the user's reporter for a capture reporter, runs
 *       @ref _lfg_ct_test_impl_inproc, snapshots counter deltas, writes
 *       the resulting payload back via the pipe, and @c _exit()s.
 *    3. Parent waits (with optional polling timeout that escalates to
 *       @c SIGKILL on expiry), reads the payload, decodes the wait
 *       status, and projects the outcome onto its own counters /
 *       reporter through @ref _lfg_ct_record_external.
 *
 *  Stdout/stderr is shared via @c fork()'s default fd inheritance, so
 *  the child's per-test "*** test ...:" banner reaches the user
 *  without any explicit capture wiring; the parent only re-emits the
 *  banner when the child died before printing it (signal/timeout/no
 *  payload).
 */

/* POSIX.1-2008 surface (clock_gettime, CLOCK_MONOTONIC, struct timespec,
 * nanosleep) must be visible even when the consumer builds with strict
 * -std=c99 (i.e. without _DEFAULT_SOURCE / _GNU_SOURCE). #ifndef guard
 * avoids redefinition warnings when the consumer already set the macro
 * at a different level via -D or a previously-included header. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "lfg-ctest.h"

#if !defined(LFG_CT_DISABLE_FORK) && (defined(__unix__) || defined(__APPLE__))

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LFG_CT_FORK_MSG_MAX 768

/* Payload shipped child->parent on a clean child exit. Fixed-layout
 * struct serialised by raw write+read on a pipe; fork() guarantees
 * the child and parent share endianness/alignment, so no encoding. */
typedef struct
{
    int32_t outcome;             /* lfg_ct_outcome_t cast to int32 */
    int32_t assertions_executed; /* deltas vs. parent-snapshot baselines */
    int32_t assertions_passed;
    int32_t assertions_failed;
    char message[LFG_CT_FORK_MSG_MAX]; /* '\0' when no message */
} _fork_payload_t;

/* Bridges provided by lfg-ctest.c. Declared local to this TU so the
 * core header stays focused on the public surface. */
extern void _lfg_ct_test_impl_inproc(void (*fn)(void), const char *name);
extern void _lfg_ct_counter_snapshot(int *executed, int *passed, int *failed);
extern const lfg_ct_reporter_t *_lfg_ct_get_reporter(void);
extern void _lfg_ct_set_active_reporter_direct(const lfg_ct_reporter_t *reporter);
extern void _lfg_ct_record_external(const char *name, double time_sec, lfg_ct_outcome_t outcome, const char *message,
        int assertions_executed_delta, int assertions_passed_delta, int assertions_failed_delta,
        int print_outcome_line);

/* Capture reporter installed in the child. Records the in-process
 * classification's outcome+message into the payload that will be
 * written back to the parent. */
static void
_child_capture(const lfg_ct_record_t *record, void *userdata)
{
    _fork_payload_t *p = (_fork_payload_t *)userdata;
    p->outcome = (int32_t)record->outcome;
    if (record->message)
    {
        snprintf(p->message, sizeof(p->message), "%s", record->message);
    }
    else
    {
        p->message[0] = '\0';
    }
}

int
_lfg_ct_fork_available(void)
{
    return 1;
}

int
_lfg_ct_fork_run_test(void (*fn)(void), const char *name, unsigned timeout_ms)
{
    int pipefd[2];
    pid_t pid;
    struct timespec t_start;
    struct timespec t_end;
    int status = 0;
    int timed_out = 0;
    _fork_payload_t payload;
    ssize_t total = 0;
    double elapsed;

    if (0 != pipe(pipefd))
    {
        _lfg_ct_record_external(name, 0.0, LFG_CT_FAILED, "fork-mode: pipe() failed", 0, 0, 1, 1);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        _lfg_ct_record_external(name, 0.0, LFG_CT_FAILED, "fork-mode: fork() failed", 0, 0, 1, 1);
        return -1;
    }

    if (0 == pid)
    {
        /* CHILD. */
        _fork_payload_t out;
        int exec0;
        int pass0;
        int fail0;
        int exec1;
        int pass1;
        int fail1;
        lfg_ct_reporter_t capture;
        const lfg_ct_reporter_t *saved;
        ssize_t w;

        close(pipefd[0]);

        memset(&out, 0, sizeof(out));
        out.outcome = (int32_t)LFG_CT_PASSED;

        memset(&capture, 0, sizeof(capture));
        capture.on_record = _child_capture;
        capture.userdata = &out;

        /* Install capture directly: bypassing the public set-reporter
         * keeps the parent-side verbose chain from also firing here
         * (which would print verbose banners from inside the child
         * and double up with the parent-side ones). The address space
         * is throwaway, but the explicit restore keeps the contract
         * symmetric in case future child-side code reads back the
         * slot. */
        saved = _lfg_ct_get_reporter();
        _lfg_ct_set_active_reporter_direct(&capture);

        _lfg_ct_counter_snapshot(&exec0, &pass0, &fail0);
        _lfg_ct_test_impl_inproc(fn, name);
        _lfg_ct_counter_snapshot(&exec1, &pass1, &fail1);

        _lfg_ct_set_active_reporter_direct(saved);

        out.assertions_executed = (int32_t)(exec1 - exec0);
        out.assertions_passed = (int32_t)(pass1 - pass0);
        out.assertions_failed = (int32_t)(fail1 - fail0);

        w = write(pipefd[1], &out, sizeof(out));
        (void)w; /* short writes accepted; parent treats missing payload as crash */
        close(pipefd[1]);

        _exit(LFG_CT_FAILED == (lfg_ct_outcome_t)out.outcome ? 1 : 0);
    }

    /* PARENT. */
    close(pipefd[1]);

    if (timeout_ms > 0)
    {
        unsigned elapsed_ms = 0;
        const unsigned step_ms = 5;
        for (;;)
        {
            pid_t r = waitpid(pid, &status, WNOHANG);
            if (r == pid)
            {
                break;
            }
            if (r < 0 && EINTR != errno)
            {
                break;
            }
            if (elapsed_ms >= timeout_ms)
            {
                kill(pid, SIGKILL);
                while (waitpid(pid, &status, 0) < 0 && EINTR == errno)
                {
                }
                timed_out = 1;
                break;
            }
            nanosleep(&(struct timespec){0, step_ms * 1000000L}, NULL);
            elapsed_ms += step_ms;
        }
    }
    else
    {
        while (waitpid(pid, &status, 0) < 0 && EINTR == errno)
        {
        }
    }

    memset(&payload, 0, sizeof(payload));
    for (;;)
    {
        ssize_t r = read(pipefd[0], ((char *)&payload) + total, sizeof(payload) - (size_t)total);
        if (r > 0)
        {
            total += r;
            if ((size_t)total >= sizeof(payload))
            {
                break;
            }
            continue;
        }
        if (r < 0 && EINTR == errno)
        {
            continue;
        }
        break;
    }
    close(pipefd[0]);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    elapsed = (double)(t_end.tv_sec - t_start.tv_sec) + (double)(t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    if (elapsed < 0.0)
    {
        elapsed = 0.0;
    }

    if (timed_out)
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "fork-mode: test timed out after %ums", timeout_ms);
        _lfg_ct_record_external(name, elapsed, LFG_CT_FAILED, msg, 0, 0, 1, 1);
        return 0;
    }
    if (WIFSIGNALED(status))
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "fork-mode: killed by signal %d", WTERMSIG(status));
        _lfg_ct_record_external(name, elapsed, LFG_CT_FAILED, msg, 0, 0, 1, 1);
        return 0;
    }
    if ((size_t)total == sizeof(payload))
    {
        _lfg_ct_record_external(name, elapsed, (lfg_ct_outcome_t)payload.outcome,
                payload.message[0] ? payload.message : NULL, payload.assertions_executed, payload.assertions_passed,
                payload.assertions_failed, 0);
        return 0;
    }
    /* WIFEXITED-but-no-payload or other unexpected shape: surface as a failure
     * so the run does not silently pretend the test passed. */
    {
        char msg[128];
        if (WIFEXITED(status))
        {
            snprintf(msg, sizeof(msg), "fork-mode: child exited %d with no payload", WEXITSTATUS(status));
        }
        else
        {
            snprintf(msg, sizeof(msg), "fork-mode: child exited in unknown state");
        }
        _lfg_ct_record_external(name, elapsed, LFG_CT_FAILED, msg, 0, 0, 1, 1);
    }
    return 0;
}

#else /* fork mode disabled or unsupported platform */

int
_lfg_ct_fork_available(void)
{
    return 0;
}

int
_lfg_ct_fork_run_test(void (*fn)(void), const char *name, unsigned timeout_ms)
{
    (void)fn;
    (void)name;
    (void)timeout_ms;
    /* Reached only if a caller bypasses lfg_ct_set_isolation's
     * availability check; surface as a clean failure rather than a
     * silent pass. The lfg_ct_set_isolation gate is the documented
     * entry point and should make this path unreachable in practice. */
    return -1;
}

#endif
