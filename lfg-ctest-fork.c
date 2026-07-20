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
 *
 *  That inheritance contract is on the *descriptor*, not on the stdio
 *  buffer, so it holds only under a two-sided flush discipline:
 *
 *    - Parent flushes before @c fork(), or the child inherits a copy
 *      of the parent's pending buffer and re-emits it on its own
 *      flush -- duplicating parent output once per forked test.
 *    - Child flushes before @c _exit(), which by design does not run
 *      the stdio cleanup @c exit() would. Without it, everything the
 *      child buffered is discarded. Only visible when stdout is not a
 *      TTY: line buffering hides the bug by flushing at each newline,
 *      while a pipe or file buffers 4-8KB and loses the lot.
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
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Inline message capacity on both sides of the pipe. Not a cap on
 * what the transport can carry -- only the size below which neither
 * side allocates. */
#define LFG_CT_FORK_MSG_MAX 768

/* Fixed-layout header shipped child->parent on a clean child exit,
 * immediately followed by @c msg_len message bytes (no terminator --
 * the length is authoritative). fork() guarantees the child and
 * parent share endianness/alignment, so no encoding.
 *
 * Length-prefixing is what lets an assertion message of any size
 * cross the pipe. The parent still demands a *complete* header
 * before trusting anything, which preserves the "no payload means
 * the child crashed" signal the dispatcher is built on. */
typedef struct
{
    int32_t outcome;             /* lfg_ct_outcome_t cast to int32 */
    int32_t assertions_executed; /* deltas vs. parent-snapshot baselines */
    int32_t assertions_passed;
    int32_t assertions_failed;
    int32_t msg_len; /* message bytes following the header; 0 = none */
} _fork_header_t;

/* Child-side accumulator. The capture reporter owns a copy of the
 * record's message because that string is borrowed for the callback
 * only; short ones land in @c inline_msg, longer ones in an owned
 * heap block. */
typedef struct
{
    _fork_header_t hdr;
    char inline_msg[LFG_CT_FORK_MSG_MAX];
    char *heap_msg; /* owned; NULL when inline_msg carries the text */
} _fork_child_out_t;

/* Parent-side accumulator. Fills the header first, then sizes a
 * message buffer from the announced length and streams into it. */
typedef struct
{
    _fork_header_t hdr;
    size_t hdr_got; /* header bytes received so far */
    char inline_msg[LFG_CT_FORK_MSG_MAX];
    char *heap_msg;  /* owned; NULL when inline_msg is the sink */
    char *msg;      /* active sink; NULL when the child sent no message */
    size_t msg_cap; /* message bytes the sink can hold, excluding NUL */
    size_t msg_got; /* message bytes actually stored */
} _fork_parent_in_t;

/* Stamps an explicit, greppable truncation marker onto a message the
 * transport could not carry in full (child- or parent-side allocation
 * failure, or a child that died mid-write). Shared with the core
 * runner so the marker format cannot drift between the two. */
extern void _lfg_ct_mark_truncated(char *buf, size_t cap, size_t used, size_t lost);

/* write(2) until the whole buffer is out. Returns 0 on success, -1
 * on a write error; a partial write left behind by a dying child is
 * detected on the parent side, not here. */
static int
_fork_write_all(int fd, const void *data, size_t len)
{
    const char *p = (const char *)data;
    size_t off = 0;

    while (off < len)
    {
        ssize_t w = write(fd, p + off, len - off);
        if (w > 0)
        {
            off += (size_t)w;
            continue;
        }
        if (w < 0 && EINTR == errno)
        {
            continue;
        }
        return -1;
    }
    return 0;
}

/* Pick the sink for the announced message length, once the header is
 * complete. Falls back to the inline buffer (with the overflow
 * marked at finish time) when the allocation fails, so an
 * out-of-memory parent degrades to a marked message rather than
 * losing the record. */
static void
_fork_parent_begin_message(_fork_parent_in_t *in)
{
    size_t need;

    if (in->hdr.msg_len <= 0)
    {
        return;
    }
    need = (size_t)in->hdr.msg_len;

    if (need < sizeof(in->inline_msg))
    {
        in->msg = in->inline_msg;
        in->msg_cap = need;
        return;
    }

    in->heap_msg = (char *)malloc(need + 1);
    if (in->heap_msg)
    {
        in->msg = in->heap_msg;
        in->msg_cap = need;
        return;
    }

    in->msg = in->inline_msg;
    in->msg_cap = sizeof(in->inline_msg) - 1;
}

/* Feed one read(2) chunk into the accumulator: header bytes first,
 * message bytes after. Bytes past the sink's capacity are counted
 * and dropped -- only reachable when the sink allocation failed. */
static void
_fork_parent_consume(_fork_parent_in_t *in, const char *data, size_t len)
{
    if (in->hdr_got < sizeof(in->hdr))
    {
        size_t take = sizeof(in->hdr) - in->hdr_got;

        if (take > len)
        {
            take = len;
        }
        memcpy((char *)&in->hdr + in->hdr_got, data, take);
        in->hdr_got += take;
        data += take;
        len -= take;

        if (in->hdr_got == sizeof(in->hdr))
        {
            _fork_parent_begin_message(in);
        }
    }

    if (0 == len || NULL == in->msg)
    {
        return;
    }

    if (in->msg_got < in->msg_cap)
    {
        size_t room = in->msg_cap - in->msg_got;
        size_t take = len < room ? len : room;

        memcpy(in->msg + in->msg_got, data, take);
        in->msg_got += take;
    }
}

/* Terminate the accumulated message and return it, or NULL when the
 * child sent none. Anything the announced length promised but the
 * sink did not receive (short write from a dying child, or a sink
 * capped by allocation failure) is named in a truncation marker --
 * the transport never drops bytes silently. */
static const char *
_fork_parent_finish_message(_fork_parent_in_t *in)
{
    size_t lost;

    if (NULL == in->msg)
    {
        return NULL;
    }
    in->msg[in->msg_got] = '\0';

    lost = (size_t)in->hdr.msg_len - in->msg_got;
    if (lost > 0)
    {
        size_t cap = (in->msg == in->heap_msg) ? (size_t)in->hdr.msg_len + 1 : sizeof(in->inline_msg);

        _lfg_ct_mark_truncated(in->msg, cap, in->msg_got, lost);
    }
    return in->msg;
}

/* Put the read end in non-blocking mode so the timeout path can
 * interleave draining with its waitpid poll. */
static void
_fork_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags >= 0)
    {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

/* Milliseconds of CLOCK_MONOTONIC elapsed since @p start. The timed
 * path derives its deadline from this rather than summing its sleep
 * steps, so the timeout holds whether the child stalls (all sleep) or
 * streams (no sleep at all), and does not drift when a nanosleep
 * oversleeps or is cut short by a signal. */
static unsigned
_fork_elapsed_ms(const struct timespec *start)
{
    struct timespec now;
    long long ms;

    clock_gettime(CLOCK_MONOTONIC, &now);
    ms = (long long)(now.tv_sec - start->tv_sec) * 1000LL + (now.tv_nsec - start->tv_nsec) / 1000000LL;
    if (ms < 0)
    {
        return 0;
    }
    return (unsigned)ms;
}

/* Bridges provided by lfg-ctest.c. Declared local to this TU so the
 * core header stays focused on the public surface. */
#ifdef LFG_CT_COMPAT_3ARG
extern void _lfg_ct_test_impl_inproc(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);
#else
extern void _lfg_ct_test_impl_inproc(void (*fn)(void), const char *name);
#endif
extern void _lfg_ct_counter_snapshot(int *executed, int *passed, int *failed);
extern const lfg_ct_reporter_t *_lfg_ct_get_reporter(void);
extern void _lfg_ct_set_active_reporter_direct(const lfg_ct_reporter_t *reporter);
extern void _lfg_ct_record_external(const char *name, double time_sec, lfg_ct_outcome_t outcome, const char *message,
        int assertions_executed_delta, int assertions_passed_delta, int assertions_failed_delta,
        int print_outcome_line);

/* Capture reporter installed in the child. Records the in-process
 * classification's outcome+message into the payload that will be
 * written back to the parent.
 *
 * Allocation here is off the signal-handling path (this runs on the
 * child's normal return from the test body) and a failure degrades
 * to a marked, inline-sized message -- the parent's read loop is
 * driven by the announced length either way, so it cannot hang. */
static void
_child_capture(const lfg_ct_record_t *record, void *userdata)
{
    _fork_child_out_t *out = (_fork_child_out_t *)userdata;
    size_t len;
    size_t full_len;
    size_t lost = 0;

    out->hdr.outcome = (int32_t)record->outcome;

    /* record->message is borrowed for the callback only, so the text
     * is copied whichever tier ends up holding it. */
    free(out->heap_msg);
    out->heap_msg = NULL;
    out->inline_msg[0] = '\0';
    out->hdr.msg_len = 0;

    if (NULL == record->message)
    {
        return;
    }

    full_len = strlen(record->message);
    len = full_len;
    if (len < sizeof(out->inline_msg))
    {
        memcpy(out->inline_msg, record->message, len + 1);
        out->hdr.msg_len = (int32_t)len;
        return;
    }

    /* msg_len is int32 on the wire, and the parent reads a negative or
     * zero length as "no message" -- so a >=2GiB message must be
     * clamped rather than allowed to wrap into silent loss. Absurd in
     * practice; cheap to rule out. */
    if (len > (size_t)INT32_MAX)
    {
        lost = len - (size_t)INT32_MAX;
        len = (size_t)INT32_MAX;
    }

    out->heap_msg = (char *)malloc(len + 1);
    if (out->heap_msg)
    {
        memcpy(out->heap_msg, record->message, len);
        out->heap_msg[len] = '\0';
        if (lost > 0)
        {
            _lfg_ct_mark_truncated(out->heap_msg, len + 1, len, lost);
        }
        out->hdr.msg_len = (int32_t)strlen(out->heap_msg);
        return;
    }

    memcpy(out->inline_msg, record->message, sizeof(out->inline_msg) - 1);
    out->inline_msg[sizeof(out->inline_msg) - 1] = '\0';
    _lfg_ct_mark_truncated(out->inline_msg, sizeof(out->inline_msg), sizeof(out->inline_msg) - 1,
            full_len - (sizeof(out->inline_msg) - 1));
    out->hdr.msg_len = (int32_t)strlen(out->inline_msg);
}

int
_lfg_ct_fork_available(void)
{
    return 1;
}

int
#ifdef LFG_CT_COMPAT_3ARG
_lfg_ct_fork_run_test(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        unsigned timeout_ms)
#else
_lfg_ct_fork_run_test(void (*fn)(void), const char *name, unsigned timeout_ms)
#endif
{
    int pipefd[2];
    pid_t pid;
    struct timespec t_start;
    struct timespec t_end;
    int status = 0;
    int timed_out = 0;
    _fork_parent_in_t in;
    double elapsed;

    if (0 != pipe(pipefd))
    {
        _lfg_ct_record_external(name, 0.0, LFG_CT_FAILED, "fork-mode: pipe() failed", 0, 0, 1, 1);
        return -1;
    }

    /* Drain every output stream before the address space is copied:
     * whatever is still buffered here would otherwise be duplicated by
     * the child's own pre-_exit flush. NULL covers stdout, stderr and
     * any stream the test body opened. Kept above the clock read so the
     * cost of draining the parent's backlog isn't billed to the test. */
    fflush(NULL);
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
        _fork_child_out_t out;
        int exec0;
        int pass0;
        int fail0;
        int exec1;
        int pass1;
        int fail1;
        lfg_ct_reporter_t capture;
        const lfg_ct_reporter_t *saved;

        close(pipefd[0]);

        memset(&out, 0, sizeof(out));
        out.hdr.outcome = (int32_t)LFG_CT_PASSED;

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
#ifdef LFG_CT_COMPAT_3ARG
        _lfg_ct_test_impl_inproc(setup, fn, teardown, name);
#else
        _lfg_ct_test_impl_inproc(fn, name);
#endif
        _lfg_ct_counter_snapshot(&exec1, &pass1, &fail1);

        _lfg_ct_set_active_reporter_direct(saved);

        out.hdr.assertions_executed = (int32_t)(exec1 - exec0);
        out.hdr.assertions_passed = (int32_t)(pass1 - pass0);
        out.hdr.assertions_failed = (int32_t)(fail1 - fail0);

        /* Header then message. A message larger than the pipe buffer
         * blocks here until the parent drains -- which it does
         * concurrently with its wait, so the pair cannot deadlock.
         * Write errors are accepted silently: the parent reconciles
         * what arrived against the announced length. */
        if (0 == _fork_write_all(pipefd[1], &out.hdr, sizeof(out.hdr)) && out.hdr.msg_len > 0)
        {
            const char *msg = out.heap_msg ? out.heap_msg : out.inline_msg;

            (void)_fork_write_all(pipefd[1], msg, (size_t)out.hdr.msg_len);
        }
        free(out.heap_msg);
        close(pipefd[1]);

        /* _exit() skips stdio cleanup, so push the test's own output
         * (assertion-failure lines, anything the body printed) to the
         * shared descriptor first. Sole child exit point, so one flush
         * covers every path through the branch. */
        fflush(NULL);

        _exit(LFG_CT_FAILED == (lfg_ct_outcome_t)out.hdr.outcome ? 1 : 0);
    }

    /* PARENT. */
    close(pipefd[1]);

    /* Drain before (or alongside) the reap. The child blocks in
     * write(2) once a long message fills the pipe buffer, so waiting
     * for it to exit first would deadlock the pair on exactly the
     * multi-kilobyte messages this transport exists to carry. */
    memset(&in, 0, sizeof(in));

    if (timeout_ms > 0)
    {
        /* Timed path: non-blocking drain first, then a polled reap,
         * both governed by one monotonic deadline measured from
         * t_start. Deriving elapsed from the clock rather than
         * accumulating sleep steps is what makes the deadline hold for
         * a child that streams continuously (it never sleeps) as well
         * as one that stalls.
         *
         * EOF is not the same event as child exit: the child closes
         * the write end before its final fflush(NULL), so a child
         * wedged in that flush has already produced EOF. Hence the
         * reap below polls under the deadline too, instead of blocking
         * -- a blocking wait there would let such a child escape the
         * timeout entirely. */
        char chunk[4096];
        const unsigned step_ms = 5;
        int reaped = 0;

        _fork_set_nonblocking(pipefd[0]);
        for (;;)
        {
            ssize_t r = read(pipefd[0], chunk, sizeof(chunk));

            if (r > 0)
            {
                _fork_parent_consume(&in, chunk, (size_t)r);
            }
            else if (0 == r)
            {
                break; /* EOF: child closed the write end */
            }
            else if (EINTR == errno)
            {
                continue;
            }
            else if (EAGAIN != errno && EWOULDBLOCK != errno)
            {
                break;
            }

            if (_fork_elapsed_ms(&t_start) >= timeout_ms)
            {
                break;
            }
            if (r < 0)
            {
                /* Nothing readable yet -- back off before retrying. */
                nanosleep(&(struct timespec){0, step_ms * 1000000L}, NULL);
            }
        }

        while (!reaped)
        {
            pid_t w = waitpid(pid, &status, WNOHANG);

            if (w == pid)
            {
                reaped = 1;
                break;
            }
            if (w < 0 && EINTR != errno)
            {
                break;
            }
            if (_fork_elapsed_ms(&t_start) >= timeout_ms)
            {
                kill(pid, SIGKILL);
                while (waitpid(pid, &status, 0) < 0 && EINTR == errno)
                {
                }
                timed_out = 1;
                reaped = 1;
                break;
            }
            nanosleep(&(struct timespec){0, step_ms * 1000000L}, NULL);
        }
    }
    else
    {
        /* Untimed path: blocking reads to EOF, then the reap. No
         * polling, so the per-test latency of the common case is
         * unchanged. */
        char chunk[4096];

        for (;;)
        {
            ssize_t r = read(pipefd[0], chunk, sizeof(chunk));

            if (r > 0)
            {
                _fork_parent_consume(&in, chunk, (size_t)r);
                continue;
            }
            if (r < 0 && EINTR == errno)
            {
                continue;
            }
            break;
        }
        while (waitpid(pid, &status, 0) < 0 && EINTR == errno)
        {
        }
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
        free(in.heap_msg);
        return 0;
    }
    if (WIFSIGNALED(status))
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "fork-mode: killed by signal %d", WTERMSIG(status));
        _lfg_ct_record_external(name, elapsed, LFG_CT_FAILED, msg, 0, 0, 1, 1);
        free(in.heap_msg);
        return 0;
    }
    /* A complete header is the payload-present signal; the message
     * that follows may be short (child died mid-write), which
     * degrades to a marked message rather than a lost record. */
    if (sizeof(in.hdr) == in.hdr_got)
    {
        _lfg_ct_record_external(name, elapsed, (lfg_ct_outcome_t)in.hdr.outcome, _fork_parent_finish_message(&in),
                in.hdr.assertions_executed, in.hdr.assertions_passed, in.hdr.assertions_failed, 0);
        free(in.heap_msg);
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
    free(in.heap_msg);
    return 0;
}

#else /* fork mode disabled or unsupported platform */

int
_lfg_ct_fork_available(void)
{
    return 0;
}

int
#ifdef LFG_CT_COMPAT_3ARG
_lfg_ct_fork_run_test(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        unsigned timeout_ms)
#else
_lfg_ct_fork_run_test(void (*fn)(void), const char *name, unsigned timeout_ms)
#endif
{
#ifdef LFG_CT_COMPAT_3ARG
    (void)setup;
    (void)teardown;
#endif
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
