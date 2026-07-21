/**
 * @file
 * @brief       lfg-ctest unit testing API.
 */

/* POSIX.1-2008 surface (clock_gettime, CLOCK_MONOTONIC, struct timespec)
 * must be visible even when the consumer builds with strict -std=c99
 * (i.e. without _DEFAULT_SOURCE / _GNU_SOURCE). Same #ifndef guard, and
 * for the same reason, as lfg-ctest-fork.c: it avoids a redefinition
 * warning when the consumer already set the macro at a different level
 * via -D or a previously-included header. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <fnmatch.h>
#include <setjmp.h>
#if defined(LFG_CTEST_HAS_FLOAT) || defined(LFG_CTEST_HAS_DOUBLE)
#include <math.h>
#endif
#include "lfg-ctest.h"

/*============================================================================
 *  Defines/Typedefs
 *==========================================================================*/

/* CLOCK_MONOTONIC is a <time.h> macro on any POSIX.1-2001 target, so the
 * probe has to sit below the includes. Absent it (a freestanding or
 * pre-POSIX target), the timing site falls back to clock(). */
#if defined(CLOCK_MONOTONIC)
#define LFG_CT_HAVE_MONOTONIC 1
#endif

/*============================================================================
 *  Private Function Prototypes
 *==========================================================================*/

/* --rerun-failed bookkeeping, defined below the id helpers they build on
 * but needed by lfg_ct_end, which sits above them. */
static void _fail_keys_clear(void);
static void _replay_clear(void);
static void _failgroup_clear(void);

/* Durations bookkeeping, same placement reason as the block above. */
static void _durations_clear(void);

/*============================================================================
 *  Timing
 *==========================================================================*/

/* Seconds off a monotonic wall clock, for differencing only -- the epoch
 * is unspecified, so an absolute value here is meaningless.
 *
 * Wall time rather than CPU time is the contract lfg-ctest.h states for
 * lfg_ct_record_t.time_sec ("Elapsed wall-clock seconds"), and it is what
 * the fork path has always measured (lfg-ctest-fork.c). The in-process
 * path used clock() until #62, which reported ~0 for a test that slept or
 * blocked on I/O -- i.e. it under-reported exactly the slow tests a
 * durations ranking exists to surface, and left the two dispatch paths
 * reporting different quantities.
 *
 * CLOCK_MONOTONIC counts from boot on every target that has it, so the
 * seconds value stays small enough that a double keeps sub-microsecond
 * resolution; the fork TU's integer-domain differencing is only needed
 * there because it also drives a millisecond timeout deadline.
 *
 * The clock() fallback keeps a target without CLOCK_MONOTONIC building
 * and reporting *something*, at the old CPU-time semantics. */
static double
_monotonic_sec(void)
{
#ifdef LFG_CT_HAVE_MONOTONIC
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

/*============================================================================
 *  Variables
 *==========================================================================*/

static int _assertions_executed = 0;
static int _assertions_failed = 0;
static int _assertions_passed = 0;
static int _tests_executed = 0;
static int _tests_failed = 0;
static int _tests_passed = 0;
static int _tests_skipped = 0;
static int _tests_xfailed = 0;
static int _tests_xpassed = 0;
static int _current_test_failures = 0;
static int _current_suite_failures = 0;

/* Per-test disposition tracking. Reset at every lfg_ct_test_impl entry,
 * read at classification time. */
typedef enum
{
    LFG_CT_DISP_NORMAL = 0,
    LFG_CT_DISP_SKIPPED
} _disposition_t;

static _disposition_t _current_disposition = LFG_CT_DISP_NORMAL;
static const char *_current_skip_reason = NULL;
static int _current_xfail_set = 0;
static const char *_current_xfail_reason = NULL;

/* Snapshot of the xfail reason at classification time. _current_xfail_reason
 * is reset to NULL right after classification so a nested test cannot taint
 * the calling test's state; this keeps the last-classified value visible to
 * self-tests so they can verify "last call wins" semantics. */
static const char *_last_classified_xfail_reason = NULL;

/* Boundary for lfg_ct_skip's "return from body immediately" semantics.
 * Set up around the body invocation in _lfg_ct_test_impl_inproc;
 * lfg_ct_skip longjmps out when active and no-ops otherwise. That entry
 * save/restores this buffer + active flag so a nested lfg_ct_test_impl
 * (the pattern the framework's own self-tests use to drive mock tests
 * through the real runner) cannot strand the outer test's body without a
 * working boundary. */
static jmp_buf _skip_env;
static int _skip_env_active = 0;

/* --strict-xpass: flip an otherwise-clean run that contains xpass
 * outcomes to a non-zero exit code. */
static int _strict_xpass = 0;

/* -x / --fail-fast: stop executing once any test has failed.
 *
 * Whole-run scope, not suite-scoped -- the convention users arrive with
 * from pytest -x and go test -failfast. A suppressed suite is not
 * re-armed on the way out; once the run has a failure it is over.
 *
 * There is no run loop to break out of: lfg_ct_test() executes its body
 * at the call site and a suite is an ordinary C function, so the whole
 * nesting is the consumer's own main() calling down. This is therefore a
 * suppression gate, not an abort -- the process still walks every
 * remaining call site and does nothing at each one. The suite-level gate
 * is what keeps that cheap: a suppressed suite body is never entered. */
static int _fail_fast = 0;

/* The gate predicate, in one place so both call sites and the summary
 * marker agree on what "tripped" means.
 *
 * Keyed on _tests_failed rather than lfg_ct_failure_count(): that
 * accessor returns the *assertion* tally, and both dispatch paths --
 * the in-process classifier and the fork parent's _lfg_ct_record_external
 * projection -- converge on _tests_failed. Reading that one global is
 * what makes the gate isolation-agnostic with no fork-specific code.
 *
 * --strict-xpass deliberately does not feed this: an xpass is a verdict
 * modifier, not a test failure, so a strict-xpass run still executes to
 * completion and fails only at the end. */
static int
_fail_fast_tripped(void)
{
    return (_fail_fast && _tests_failed > 0) ? 1 : 0;
}

/* Per-test message capture: holds the first assertion-failure text
 * observed in the running test, formatted as "<file>:<line>: in
 * <function>(): <expr>". Reset at every lfg_ct_test_impl entry,
 * read by the reporter callback at the test's classification site
 * (the downstream package copies it if it wants to buffer beyond
 * the callback's lifetime).
 *
 * Storage is two-tier so an arbitrarily long message survives while
 * the common short one still costs nothing: text that fits lives in
 * the inline buffer, anything longer in a heap block owned by this
 * slot. The heap tier wins whenever it is non-NULL; exactly one tier
 * carries text at a time. Ownership is per-lfg_ct_test_impl level --
 * each level frees its own message on the way out and hands the
 * outer level's block back untouched (see the save/restore pair in
 * _lfg_ct_test_impl_inproc). */
#define LFG_CT_FAILURE_MSG_MAX 768
static char _failure_msg_inline[LFG_CT_FAILURE_MSG_MAX] = {0};
static char *_failure_msg_heap = NULL;

/* Current suite name surfaced as the reporter record's classname.
 * NULL when no suite is active (top-level tests). Tracked via
 * save/restore in lfg_ct_suite_impl so nested suites unwind
 * cleanly. */
static const char *_current_suite_name = NULL;

/* Registration site's __FILE__ for the test currently being dispatched.
 * A baton rather than a threaded parameter so the internal in-process and
 * fork re-entry paths (which re-check the filter) see the same id the
 * public entry gated on, without changing their signatures. Set and
 * restored around the dispatch in lfg_ct_test_impl_at; NULL when a caller
 * reached the runner through the retained bare-name entry. */
static const char *_current_test_file = NULL;

/* Reporter slot. Single pointer; one downstream consumer at a time.
 * Borrowed -- caller keeps the struct alive across runner calls
 * (file-static in the consumer is the intended usage).
 *
 * When --verbose is active, _reporter points at the built-in verbose
 * reporter (file-static, defined below) and _user_reporter holds the
 * consumer-installed slot. The verbose reporter prints the per-test
 * START / outcome banners then delegates to _user_reporter, so a
 * downstream package (e.g. contrib/junit-xml) keeps observing every
 * test event alongside the streamed output -- single fan-out site,
 * no parallel print paths. When verbose is off, _reporter == _user_reporter
 * and the chain collapses to the original one-slot behaviour. */
static const lfg_ct_reporter_t *_reporter = NULL;
static const lfg_ct_reporter_t *_user_reporter = NULL;

/* Forward declaration -- the verbose reporter struct (defined below)
 * needs its callbacks visible at the install site. */
static void _verbose_on_test_start(const char *suite_name, const char *test_name, void *userdata);
static void _verbose_on_record(const lfg_ct_record_t *record, void *userdata);
static void _verbose_on_run_complete(void *userdata);

static const lfg_ct_reporter_t _verbose_reporter = {
        _verbose_on_record,
        _verbose_on_run_complete,
        NULL,
        _verbose_on_test_start,
};

/* Output verbosity as one integer axis; -q and -v are aliases for its
 * ends and --verbosity <n> addresses it directly. Kept as a level rather
 * than a pair of booleans so "quiet" and "verbose" cannot both latch and
 * a future level (summary-only) costs no mutual-exclusion check.
 *
 * At VERBOSE the built-in verbose reporter streams a START line before
 * each test body and an outcome line (PASS / FAIL / SKIP / XFAIL /
 * XPASS) with elapsed milliseconds after classification, so that print
 * path goes through the same single reporter contract every other event
 * observer uses.
 *
 * At QUIET the runner's own progress prints are gated off at their call
 * sites -- see _quiet_mode(). That is deliberately not a reporter: the
 * record stream a user-installed reporter sees is identical at every
 * level. */
static lfg_ct_verbosity_t _verbosity = LFG_CT_VERBOSITY_DEFAULT;

/* Single predicate for the suppression sites so "what does quiet hide"
 * is one grep, and a level added above QUIET later does not have to
 * revisit each call site's comparison operator. */
static int
_quiet_mode(void)
{
    return _verbosity <= LFG_CT_VERBOSITY_QUIET ? 1 : 0;
}

/* Isolation mode + per-test timeout for fork mode. Both are runtime
 * settings; the platform gate and the LFG_CT_DISABLE_FORK opt-out are
 * confined to lfg-ctest-fork.c. set_isolation calls into that TU to
 * validate availability before committing the mode here. */
static lfg_ct_isolation_t _isolation = LFG_CT_ISOLATE_NONE;
static unsigned _fork_timeout_ms = 0;

/* Provided by lfg-ctest-fork.c. The fork TU always defines both --
 * the compile-time opt-out / unsupported-platform path provides a
 * stub returning 0 / -1 with no fork-related code linked in. */
extern int _lfg_ct_fork_available(void);
#ifdef LFG_CT_COMPAT_3ARG
extern int _lfg_ct_fork_run_test(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        unsigned timeout_ms);
#else
extern int _lfg_ct_fork_run_test(void (*fn)(void), const char *name, unsigned timeout_ms);
#endif

/* Forward declaration for the in-process dispatch helper -- definition
 * lives below; the isolation shim in lfg_ct_test_impl calls into it. */
#ifdef LFG_CT_COMPAT_3ARG
void _lfg_ct_test_impl_inproc(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name);
#else
void _lfg_ct_test_impl_inproc(void (*fn)(void), const char *name);
#endif

/* lfg_ct_parse_args() runtime state. The glob arrays hold borrowed pointers
 * into the caller's argv (which has program lifetime under standard main()
 * usage), so no string copies or frees are needed. */
#define LFG_CT_FILTER_MAX 64

/* Scratch size for a rendered <file>::<suite>::<test> id. Stack-local at
 * every use site so nothing is allocated on the registration hot path;
 * generous enough that real C identifiers and filenames never truncate. */
#define LFG_CT_ID_MAX 320

static int _list_mode = 0;
static const char *_filter_globs[LFG_CT_FILTER_MAX];
static int _filter_glob_count = 0;
static const char *_exclude_globs[LFG_CT_FILTER_MAX];
static int _exclude_glob_count = 0;
/* Depth of nested suites whose name matched the filter. Tests inside such
 * a suite inherit the filter-pass (so "--filter suite_foo*" runs every test
 * the suite holds without each test having to match individually). */
static int _filter_inherited_depth = 0;

/* --durations <n>: print the n slowest tests after the run. "Was the flag
 * given" is its own predicate rather than a sentinel because 0 is a
 * meaningful value the user types deliberately -- it means "every retained
 * test", not "none". The accumulator itself lives further down, next to
 * the report it feeds. */
static int _durations_set = 0;
static int _durations_limit = 0;

/* --seed <n>: user-supplied srand() seed. The "was it supplied" answer is
 * its own flag rather than a sentinel value, because 0 is a legal seed the
 * user may pass deliberately. When unset, lfg_ct_start() generates one. */
static int _seed_set = 0;
static unsigned _seed_value = 0;

/* Seed this run actually handed to srand(). Distinct from _seed_value,
 * which only carries a *supplied* seed: the state file has to persist the
 * generated one too, or a --rerun-failed replay would restore a selection
 * without the conditions it failed under. Set by lfg_ct_start. */
static unsigned _effective_seed = 0;

/* --rerun-failed (#56): replay exactly the previous run's failures.
 *
 * Every non-listing run persists a small line-oriented state file: a
 * format marker, the seed the run used, and the id of every test that
 * classified as FAILED. --rerun-failed reads it back, restores the seed,
 * and admits only the persisted ids. Because the file is rewritten on
 * every run -- including a --rerun-failed run -- successive invocations
 * narrow toward the tests that still fail rather than replaying the
 * original set forever.
 *
 * The persisted keys are the same <file>::<suite>::<test> ids --list
 * emits and --filter matches (#55), so nothing here invents a second
 * addressing scheme. They are recorded from the runner's own
 * classification, never from stdout -- deriving them from output text
 * would reproduce #50's empty-state-file failure in exactly the
 * redirected/CI cases the feature exists for. */
#define LFG_CT_STATE_PATH_DEFAULT ".lfg-ctest-last"
#define LFG_CT_STATE_MAGIC "lfg-ctest-state"
#define LFG_CT_STATE_VERSION 1

/* Suffix of the sibling temporary _state_write renames into place. Kept
 * next to the real file so the rename stays within one directory. */
#define LFG_CT_STATE_TMP_SUFFIX ".tmp"

/* Longest state-file line accepted. An id is capped at LFG_CT_ID_MAX, so
 * this is generous; anything longer is a malformed file, not a line to
 * silently split (splitting would fabricate keys). */
#define LFG_CT_STATE_LINE_MAX 1024

static int _rerun_failed = 0;
static const char *_state_path = LFG_CT_STATE_PATH_DEFAULT;

/* Ids this run classified as FAILED, in classification order. Owned
 * heap copies: the id is rendered into a stack buffer at the
 * classification site, so borrowing would dangle. */
static char **_fail_keys = NULL;
static int _fail_key_count = 0;
static int _fail_key_cap = 0;

/* Ids loaded from the state file under --rerun-failed, plus a parallel
 * "did a registered test claim this key" flag. The flags are what let the
 * end of the run tell a renamed/removed test (warn, keep going) from a
 * wholly stale state file (diagnostic + non-zero exit). */
static char **_replay_keys = NULL;
static unsigned char *_replay_resolved = NULL;
static int _replay_key_count = 0;
static int _replay_key_cap = 0;

/* Latched at summary time when a --rerun-failed run resolved none of its
 * persisted keys. Latched rather than recomputed so lfg_ct_return keeps
 * answering correctly after lfg_ct_end has released the replay set. */
static int _rerun_unresolved_fatal = 0;

/*============================================================================
 *  Self-Test Support (internal only)
 *
 *  When LFG_CTEST_SELF_TEST is defined, enables "expect failures" mode for
 *  testing the framework itself. Failures during this mode are counted but
 *  don't affect the final test result.
 *==========================================================================*/

#ifdef LFG_CTEST_SELF_TEST
static int _expect_failures_mode = 0;
static int _expected_failures_count = 0;

/* In expect-failures mode, count the failure but don't affect test results */
#define RECORD_FAILURE()                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (_expect_failures_mode)                                                                                     \
        {                                                                                                              \
            _expected_failures_count++;                                                                                \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            _current_test_failures++;                                                                                  \
            _assertions_failed++;                                                                                      \
        }                                                                                                              \
    } while (0)

#define RECORD_PASS()                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!_expect_failures_mode)                                                                                    \
        {                                                                                                              \
            _assertions_passed++;                                                                                      \
        }                                                                                                              \
    } while (0)

#else
/* Normal mode: always record failures/passes */
#define RECORD_FAILURE()                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        _current_test_failures++;                                                                                      \
        _assertions_failed++;                                                                                          \
    } while (0)

#define RECORD_PASS()                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        _assertions_passed++;                                                                                          \
    } while (0)

#endif /* LFG_CTEST_SELF_TEST */

/* Widest "... [truncated N bytes]" rendering, plus slack. */
#define LFG_CT_TRUNC_MARKER_MAX 48

/* Stamp an explicit, greppable truncation marker onto a message the
 * runner could not carry in full. @p used is the current string
 * length in @p buf, @p cap its total size, @p lost the number of
 * bytes dropped. The marker is appended when there is room and
 * overwrites the tail otherwise, so the emitted text always names
 * the loss instead of ending mid-token.
 *
 * Only reachable on the degraded paths (allocation failure, short
 * pipe write); the length-correct paths never call it.
 *
 * Deliberately not static: the fork transport declares it extern and
 * calls it too, so both sides emit one marker format. */
void
_lfg_ct_mark_truncated(char *buf, size_t cap, size_t used, size_t lost)
{
    char marker[LFG_CT_TRUNC_MARKER_MAX];
    int n;

    n = snprintf(marker, sizeof(marker), "... [truncated %lu bytes]", (unsigned long)lost);
    if (n < 0 || (size_t)n + 1 > cap)
    {
        return;
    }
    if (used + (size_t)n + 1 > cap)
    {
        used = cap - (size_t)n - 1;
    }
    memcpy(buf + used, marker, (size_t)n + 1);
}

/* Active per-test failure message, or NULL when none was captured.
 * The heap tier wins over the inline one; see the slot's declaration
 * for the ownership rules. */
static const char *
_lfg_ct_failure_msg(void)
{
    if (_failure_msg_heap)
    {
        return _failure_msg_heap;
    }
    return _failure_msg_inline[0] ? _failure_msg_inline : NULL;
}

/* Drop the message this lifecycle level captured, releasing the heap
 * tier if it is in use. Never called on a block the outer level
 * owns -- that one is moved out of the slot before the level runs. */
static void
_lfg_ct_failure_msg_clear(void)
{
    free(_failure_msg_heap);
    _failure_msg_heap = NULL;
    _failure_msg_inline[0] = '\0';
}

/* Capture "<file>:<line>: in <fn>(): <text>" into the slot, promoting
 * to the heap tier when the composed line outgrows the inline buffer.
 * Same two-pass shape as _lfg_ct_fail: snprintf reports the required
 * length, and only an over-long line pays for an allocation. */
static void
_lfg_ct_failure_msg_set(const char *file, int line, const char *function, const char *text)
{
    int need;

    need = snprintf(_failure_msg_inline, sizeof(_failure_msg_inline), "%s:%d: in %s(): %s", file, line, function,
            text);
    if (need < 0 || (size_t)need < sizeof(_failure_msg_inline))
    {
        return;
    }

    _failure_msg_heap = (char *)malloc((size_t)need + 1);
    if (NULL == _failure_msg_heap)
    {
        _lfg_ct_mark_truncated(_failure_msg_inline, sizeof(_failure_msg_inline), sizeof(_failure_msg_inline) - 1,
                (size_t)need - (sizeof(_failure_msg_inline) - 1));
        return;
    }
    snprintf(_failure_msg_heap, (size_t)need + 1, "%s:%d: in %s(): %s", file, line, function, text);
}

/* Consolidated assertion-failure helper. Formats @p fmt into a
 * scratch buffer, prints the standard "*** <file>: <line>:
 * FAILURE in <fn>(): <buf>" line to stdout (unchanged behavior),
 * bumps the per-test + global failure counters, and snapshots the
 * first failure per test into the failure-message slot (@c
 * _failure_msg_inline / @c _failure_msg_heap) so the reporter
 * callback can surface the assertion text on the record it hands
 * downstream.
 *
 * Expect-failures self-test mode bypasses both the counter bumps
 * and the message capture -- those failures are intentional and
 * shouldn't appear in a downstream report. The stdout line still
 * emits so the framework's own "did this assertion fail" tests
 * remain visibly noisy when something genuinely breaks.
 *
 * Existence rationale: every assertion impl used to inline the
 * same "printf(...) + RECORD_FAILURE();" pair, which was the
 * natural place to also stash the message text. Routing every
 * failure through one site keeps the data flow honest. */
static void
_lfg_ct_fail(const char *file, int line, const char *function, const char *fmt, ...)
{
    char stack_buf[512];
    char *heap_buf = NULL;
    const char *buf = stack_buf;
    va_list ap;
    va_list ap_retry;
    int need;

    /* Two-pass format. The stack buffer serves every message that
     * fits -- the common case, and the reason the assertion path
     * stays allocation-free. Anything longer is re-formatted into a
     * heap block sized from vsnprintf's own return value, so no
     * message is clipped by the runner's choice of buffer. The first
     * pass consumes ap, hence the va_copy taken before it. */
    va_start(ap, fmt);
    va_copy(ap_retry, ap);
    need = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
    va_end(ap);

    if (need >= (int)sizeof(stack_buf))
    {
        heap_buf = (char *)malloc((size_t)need + 1);
        if (heap_buf)
        {
            vsnprintf(heap_buf, (size_t)need + 1, fmt, ap_retry);
            buf = heap_buf;
        }
        else
        {
            /* Out of memory: emit what fits and name the loss.
             * Silent clipping is what made this defect expensive to
             * diagnose in the first place. */
            _lfg_ct_mark_truncated(stack_buf, sizeof(stack_buf), sizeof(stack_buf) - 1,
                    (size_t)need - (sizeof(stack_buf) - 1));
        }
    }
    va_end(ap_retry);

    printf("*** %s: %d: FAILURE in %s(): %s\r\n", file ? file : "(unknown)", line,
            function ? function : "(unknown)", buf);

#ifdef LFG_CTEST_SELF_TEST
    if (_expect_failures_mode)
    {
        _expected_failures_count++;
        free(heap_buf);
        return;
    }
#endif
    _current_test_failures++;
    _assertions_failed++;

    if (NULL == _lfg_ct_failure_msg())
    {
        _lfg_ct_failure_msg_set(file ? file : "(unknown)", line, function ? function : "(unknown)", buf);
    }
    free(heap_buf);
}

/*============================================================================
 *  Public API
 *==========================================================================*/

const char *
lfg_ct_version(void)
{
    return LFG_CTEST_VERSION_FULL;
}

/* FNV-1a fold of one value's bytes into the running hash. Bounded by
 * sizeof(v) so the shift never reaches the width of the type. */
static unsigned
_seed_mix(unsigned h, unsigned long v)
{
    unsigned i;

    for (i = 0; i < (unsigned)sizeof(v); i++)
    {
        h ^= (unsigned)((v >> (i * 8)) & 0xFFUL);
        h *= 16777619U;
    }
    return h;
}

/* Generate a seed spanning the full unsigned range from sources that are
 * available wherever C89 is: wall clock (coarse but always distinct across
 * runs seconds apart), process CPU clock (sub-second, distinct across runs
 * started within the same second), and a stack address (varies per run
 * under ASLR). Hashed together rather than added so no single weak source
 * dominates the low bits. Deliberately not a CSPRNG -- this seeds rand()
 * for scenario selection, not key material. */
static unsigned
_seed_generate(void)
{
    unsigned char probe = 0;
    unsigned h = 2166136261U;

    h = _seed_mix(h, (unsigned long)time(NULL));
    h = _seed_mix(h, (unsigned long)clock());
    h = _seed_mix(h, (unsigned long)(size_t)(void *)&probe);
    return h;
}

void lfg_ct_start(void)
{
    unsigned rand_seed = _seed_set ? _seed_value : _seed_generate();

    /* Remembered for the state file: a replay has to restore the
     * conditions the tests failed under, generated seed included. */
    _effective_seed = rand_seed;

    /* List mode wants stdout to be a clean newline-separated list of names;
     * skip the banner and the seed announcement. srand still runs so any
     * deterministic-by-seed test behavior stays consistent if the user
     * combines --list with other operations. */
    if (!_list_mode)
    {
        /* The seed line survives quiet on purpose: a quiet failing run
         * whose failures cannot be reproduced is a worse artifact than
         * one extra line. Only the banner goes. */
        if (!_quiet_mode())
        {
            printf("*** begin unit test\r\n");
        }
        printf("*** random seed is %u\r\n", rand_seed);
        if (_rerun_failed && 0 == _replay_key_count)
        {
            /* Say it plainly rather than running a silent empty suite --
             * "nothing ran" and "nothing was left to run" look identical
             * in the summary otherwise. Still a clean exit. */
            printf("*** rerun-failed: %s records no failures; nothing to replay\r\n", _state_path);
        }
    }
    srand(rand_seed);
}

void lfg_ct_end(void)
{
    /* Release the rerun bookkeeping. The fatal-replay verdict is already
     * latched, so lfg_ct_return keeps answering correctly after this. */
    _fail_keys_clear();
    _replay_clear();
    _failgroup_clear();
    _durations_clear();
}

/* Number of "::"-delimited components in @p s (a glob or an id). */
static int
_id_component_count(const char *s)
{
    const char *p = s;
    int n = 1;

    if (NULL == s)
    {
        return 0;
    }
    while (NULL != (p = strstr(p, LFG_CT_ID_SEPARATOR)))
    {
        p += sizeof(LFG_CT_ID_SEPARATOR) - 1;
        n++;
    }
    return n;
}

/* The trailing suffix of @p id consisting of exactly @p depth components,
 * or NULL when @p id has fewer than that many. */
static const char *
_id_suffix_at_depth(const char *id, int depth)
{
    const char *p = id;
    int skip;

    if (NULL == id)
    {
        return NULL;
    }
    skip = _id_component_count(id) - depth;
    if (skip < 0)
    {
        return NULL;
    }
    while (skip-- > 0)
    {
        p = strstr(p, LFG_CT_ID_SEPARATOR);
        p += sizeof(LFG_CT_ID_SEPARATOR) - 1;
    }
    return p;
}

/* Strip any directory prefix from @p path. The id has to be stable across
 * build directories and out-of-tree builds, and __FILE__ carries whatever
 * path the compiler was invoked with, so only the basename is addressable.
 * Handles both separators so a Windows-hosted build ids the same way. */
static const char *
_id_basename(const char *path)
{
    const char *p;
    const char *base;

    if (NULL == path || '\0' == path[0])
    {
        return LFG_CT_ID_NO_SUITE;
    }
    base = path;
    for (p = path; '\0' != *p; p++)
    {
        if ('/' == *p || '\\' == *p)
        {
            base = p + 1;
        }
    }
    /* A path ending in a separator leaves an empty basename; fall back
     * rather than emitting a zero-width component. */
    return ('\0' == base[0]) ? LFG_CT_ID_NO_SUITE : base;
}

/* Render an entry's addressable id into @p buf.
 *
 * Tests get <file>::<suite>::<test>; suites get <file>::<suite> (pass NULL
 * for @p test). A NULL component becomes LFG_CT_ID_NO_SUITE so the id is
 * always well-formed -- never a malformed "file::::test". Truncation is
 * silent (snprintf), which only costs addressability of pathologically
 * long names, never correctness of the run.
 *
 * Returns the snprintf(3) length the id *would* have needed, so a caller
 * that cares can detect truncation (result >= cap) or size a buffer with
 * a (NULL, 0) probe. The internal call sites deliberately ignore it. */
static int
_id_build(char *buf, size_t cap, const char *file, const char *suite, const char *test)
{
    const char *base = _id_basename(file);
    const char *mid = (NULL != suite && '\0' != suite[0]) ? suite : LFG_CT_ID_NO_SUITE;

    /* snprintf only accepts a NULL destination when the size is 0. */
    if (NULL == buf)
    {
        cap = 0;
    }
    if (NULL == test)
    {
        return snprintf(buf, cap, "%s" LFG_CT_ID_SEPARATOR "%s", base, mid);
    }
    return snprintf(buf, cap, "%s" LFG_CT_ID_SEPARATOR "%s" LFG_CT_ID_SEPARATOR "%s",
                    base, mid, test);
}

size_t
lfg_ct_format_id(char *buf, size_t cap, const char *file, const char *suite, const char *test)
{
    int needed;

    if (NULL != buf && cap > 0)
    {
        buf[0] = '\0';
    }
    needed = _id_build(buf, cap, file, suite, test);
    /* snprintf semantics: the length the id would have needed. A result
     * >= cap means the buffer held a truncated id. */
    return (needed < 0) ? 0 : (size_t)needed;
}

/* Match @p id against the glob list. A glob addresses exactly as many
 * trailing components as it spells out: it is fnmatch(3)'d against the
 * suffix of @p id whose component count equals its own. So
 * "file.c::suite::test", "suite::test" and "test" are all valid ways to
 * name the same entry, and a caller can qualify progressively until the
 * selection is unique.
 *
 * Pinning the depth per-glob is what makes the rule backward compatible.
 * Matching every suffix instead would let a bare-name glob reach the file
 * component -- "test_*" would match the id "test_math.c::suite::helper"
 * (fnmatch has no FNM_PATHNAME here and "*" would span "::"), silently
 * over-selecting for the ubiquitous "test_*.c" file naming convention.
 * A one-component glob now only ever sees a one-component suffix, so
 * every pre-id glob selects exactly what it always did, and "*" cannot
 * cross a "::". */
static int
_glob_list_matches_id(const char *id, const char *const *globs, int count)
{
    const char *suffix;
    int i;

    if (NULL == id)
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        suffix = _id_suffix_at_depth(id, _id_component_count(globs[i]));
        if (NULL != suffix && 0 == fnmatch(globs[i], suffix, 0))
        {
            return 1;
        }
    }
    return 0;
}

/*============================================================================
 *  --rerun-failed state file
 *==========================================================================*/

/* strdup(3) is not C99. */
static char *
_str_dup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);

    if (NULL != p)
    {
        memcpy(p, s, n);
    }
    return p;
}

static void
_fail_keys_clear(void)
{
    int i;

    for (i = 0; i < _fail_key_count; i++)
    {
        free(_fail_keys[i]);
    }
    free(_fail_keys);
    _fail_keys = NULL;
    _fail_key_count = 0;
    _fail_key_cap = 0;
}

static void
_replay_clear(void)
{
    int i;

    for (i = 0; i < _replay_key_count; i++)
    {
        free(_replay_keys[i]);
    }
    free(_replay_keys);
    free(_replay_resolved);
    _replay_keys = NULL;
    _replay_resolved = NULL;
    _replay_key_count = 0;
    _replay_key_cap = 0;
}

/*============================================================================
 *  Grouped failure summary
 *
 *  Collapses a run's FAILED outcomes into one line per distinct test
 *  name, so a large failure set reports how many *distinct* problems it
 *  represents instead of leaving that collapse to be hand-rolled over
 *  the captured log.
 *
 *  Fixed-cap static storage, matching the no-allocation style the rest
 *  of this TU's run-scoped tables use (see LFG_CT_FILTER_MAX): the
 *  accumulator runs during a failing run, which is the worst possible
 *  moment to introduce an allocation-failure path. The ceiling is
 *  documented rather than silent -- an overrun is reported in the block
 *  itself. Footprint is LFG_CT_FAILGROUP_MAX * ~324 bytes of BSS (~83 KB
 *  at the default cap); lower the cap if a target cannot spend it.
 *==========================================================================*/

/* Distinct failing test names the block can report. Beyond this the
 * failures are still counted, just not grouped -- and the block says so. */
#define LFG_CT_FAILGROUP_MAX 256

/* Per-group copies. Borrowing would be cheaper, but the test name is a
 * caller-supplied pointer with no lifetime guarantee and the origin is
 * carved out of the per-test message slot, which is cleared on the way
 * out of every dispatch level.
 *
 * The name is the grouping *key* -- it is compared, not just printed --
 * so its width is a correctness bound, not a cosmetic one: two tests
 * agreeing within the copy would fold into one group with no way to see
 * it had happened. 128 is sized against this repo's own convention
 * (test_<area>_<behavior>_<detail> tops out in the low 60s, so long
 * shared prefixes are the norm) with room for that to roughly double,
 * and stays well under LFG_CT_ID_MAX's 320, which has to hold a whole
 * <file>::<suite>::<test> id rather than a bare name.
 *
 * The origin is display-only -- nothing keys off it -- but 192 keeps a
 * deeply-nested path plus a descriptive function name inside the
 * documented "file:line: in fn()" shape, where 128 would cut the path
 * and take the closing "()" with it. */
#define LFG_CT_FAILGROUP_NAME_MAX 128
#define LFG_CT_FAILGROUP_ORIGIN_MAX 192

typedef struct
{
    char name[LFG_CT_FAILGROUP_NAME_MAX];
    char origin[LFG_CT_FAILGROUP_ORIGIN_MAX];
    int count;
} _failgroup_t;

static _failgroup_t _failgroups[LFG_CT_FAILGROUP_MAX];
static int _failgroup_count = 0;

/* Every failure fed to the accumulator, grouped or not. The block's
 * header reports this rather than _tests_failed so the two stay
 * independent -- the self-test hooks feed the accumulator without
 * driving a real classification. */
static int _failgroup_total = 0;

/* Failures that arrived after the table filled. Non-zero is what turns
 * the cap from a silent truncation into a reported one. */
static int _failgroup_dropped = 0;

/* Copy the location prefix of a captured failure message into @p out.
 *
 * The message is composed once, by _lfg_ct_failure_msg_set, as
 * "<file>:<line>: in <fn>(): <text>", and that is the only form that
 * reaches here carrying a location. Slicing it back apart looks
 * roundabout next to capturing file/line into their own slots at the
 * assertion site -- but fork mode's parent never runs the assertion and
 * has nothing *but* this string, so a parser is needed regardless.
 * Deriving both paths from it keeps the two feed sites reporting
 * identical text instead of two mechanisms that can drift.
 *
 * A message with no location (fork-mode's own diagnostics: a signal, a
 * timeout, a pipe failure) yields "(unknown)" rather than a guess. */
static void
_failgroup_origin(char *out, size_t cap, const char *message)
{
    const char *end;
    size_t n;

    end = (NULL != message) ? strstr(message, "(): ") : NULL;
    if (NULL == end)
    {
        snprintf(out, cap, "%s", "(unknown)");
        return;
    }

    /* Keep the "()" that closes the function name, drop the ": " that
     * begins the assertion text. */
    n = (size_t)(end - message) + 2;
    snprintf(out, cap, "%.*s", (int)n, message);
}

/* Fold one FAILED outcome into the accumulator. @p message is the
 * record's message -- the same string both feed sites already hand the
 * reporter -- and supplies the group's first-occurrence location when
 * the group is created. */
static void
_failgroup_record(const char *name, const char *message)
{
    const char *key = (NULL != name) ? name : "(unknown)";
    int i;

    _failgroup_total++;

    for (i = 0; i < _failgroup_count; i++)
    {
        if (0 == strcmp(_failgroups[i].name, key))
        {
            _failgroups[i].count++;
            return;
        }
    }

    if (_failgroup_count == LFG_CT_FAILGROUP_MAX)
    {
        _failgroup_dropped++;
        return;
    }

    snprintf(_failgroups[_failgroup_count].name, sizeof(_failgroups[_failgroup_count].name), "%s", key);
    _failgroup_origin(_failgroups[_failgroup_count].origin, sizeof(_failgroups[_failgroup_count].origin), message);
    _failgroups[_failgroup_count].count = 1;
    _failgroup_count++;
}

static void
_failgroup_clear(void)
{
    _failgroup_count = 0;
    _failgroup_total = 0;
    _failgroup_dropped = 0;
}

/* Fill @p order with group indices in display order: count descending,
 * ties broken by first-occurrence order.
 *
 * Descending count is the whole point of the block rather than a
 * presentation choice -- the largest group is usually one systemic
 * cause, and putting it first is what makes the output actionable. The
 * tie-break keeps the block byte-identical across runs of the same set.
 * Insertion sort over a bounded table, and stable, which is exactly what
 * makes the tie-break fall out of the table's own build order. */
static void
_failgroup_order(int *order)
{
    int i;
    int j;

    for (i = 0; i < _failgroup_count; i++)
    {
        for (j = i; j > 0 && _failgroups[order[j - 1]].count < _failgroups[i].count; j--)
        {
            order[j] = order[j - 1];
        }
        order[j] = i;
    }
}

/* Emit the block. Silent on a run with no failures: no header, no empty
 * body, so a green run's output is byte-identical to what it was before
 * this existed. */
static void
_failgroup_print(void)
{
    int order[LFG_CT_FAILGROUP_MAX];
    int i;

    if (0 == _failgroup_total)
    {
        return;
    }

    _failgroup_order(order);

    /* Once the cap is hit the distinct-test count stops being knowable:
     * the dropped failures span somewhere between one and
     * _failgroup_dropped further tests, and the table kept nothing that
     * could tell them apart. Say "at least" rather than print a number
     * the reader would reasonably take as exact -- and note below that
     * the two figures cannot be summed to recover the true one, since
     * the dropped tally counts failures and this one counts tests. */
    printf("*** Failure summary: %d failure%s in %s%d distinct test%s\r\n", _failgroup_total,
            (1 == _failgroup_total) ? "" : "s", (_failgroup_dropped > 0) ? "at least " : "", _failgroup_count,
            (1 == _failgroup_count) ? "" : "s");
    for (i = 0; i < _failgroup_count; i++)
    {
        printf("*** %5d  %-32s (first: %s)\r\n", _failgroups[order[i]].count, _failgroups[order[i]].name,
                _failgroups[order[i]].origin);
    }
    if (_failgroup_dropped > 0)
    {
        printf("*** %d further failure%s ungrouped: the distinct-test cap (LFG_CT_FAILGROUP_MAX = %d) was "
               "reached; their distinct-test count is unknown\r\n",
                _failgroup_dropped, (1 == _failgroup_dropped) ? "" : "s", LFG_CT_FAILGROUP_MAX);
    }
}

/*============================================================================
 *  Slowest-test durations report
 *
 *  Per-test elapsed time has always reached an installed reporter via
 *  lfg_ct_record_t.time_sec, but nothing aggregated it: answering "what
 *  makes this suite slow?" meant reading every -v line by eye. --durations
 *  <n> ranks the run's tests by elapsed time and prints the slowest n
 *  after the summary, pytest-style.
 *
 *  Fed by a direct call from both record fan-out sites -- the in-process
 *  classification and the fork parent's _lfg_ct_record_external -- rather
 *  than by a third reporter layered onto the chain. The chain is two deep
 *  by design (_verbose_reporter delegating to _user_reporter) and an
 *  internal accumulator has no business in the public reporter slot, where
 *  a consumer replacing the reporter mid-run would silently unhook it.
 *
 *  Collection is unconditional, not gated on the flag: it is three stores
 *  per test, and gating it would make the report depend on whether
 *  lfg_ct_parse_args ran before or after the first dispatch.
 *
 *  Fixed-cap static storage, matching LFG_CT_FILTER_MAX and the failure
 *  summary's table -- the core runner's run-scoped tables do not allocate.
 *  An overrun is reported in the block rather than silently truncating.
 *==========================================================================*/

/* Tests the report can rank. Sized past the ~650-test suite that motivated
 * the feature so a real run does not routinely trip the cap; footprint is
 * LFG_CT_DURATIONS_MAX * sizeof(_duration_t) of BSS (~24 KB at the default
 * cap on a 64-bit target). Beyond the cap tests still run and still
 * classify -- only their ranking is lost, and the block says so. */
#define LFG_CT_DURATIONS_MAX 1024

/* Borrowed pointers, unlike the failure summary's copies. Both names
 * originate in the test-registration macros as string literals with
 * program lifetime, and neither is a key here -- the entries are ranked by
 * time and printed, never compared -- so the width bound that forces the
 * failure summary to copy its grouping key does not apply. The message is
 * deliberately not retained: lfg-ctest.h documents it as valid only for
 * the duration of the reporter callback, and a durations line has no use
 * for it. */
typedef struct
{
    const char *suite_name;
    const char *test_name;
    double time_sec;
} _duration_t;

static _duration_t _durations[LFG_CT_DURATIONS_MAX];
static int _duration_count = 0;

/* Tests that classified after the table filled. Non-zero is what turns
 * the cap from a silent truncation into a reported one. */
static int _duration_dropped = 0;

/* Retain one classified test's elapsed time. Called from both fan-out
 * sites for every outcome, not just PASS: a slow SKIP or XFAIL is exactly
 * as interesting to a "why is this suite slow" question. */
static void
_durations_record(const char *suite_name, const char *test_name, double time_sec)
{
    if (_duration_count == LFG_CT_DURATIONS_MAX)
    {
        _duration_dropped++;
        return;
    }

    _durations[_duration_count].suite_name = suite_name;
    _durations[_duration_count].test_name = test_name;
    _durations[_duration_count].time_sec = time_sec;
    _duration_count++;
}

static void
_durations_clear(void)
{
    _duration_count = 0;
    _duration_dropped = 0;
}

/* Fill @p order with entry indices in display order: elapsed time
 * descending, ties broken by the order the tests classified in.
 *
 * The tie-break is a correctness requirement, not a nicety: tests whose
 * times land on the same value (trivially common at millisecond
 * granularity for a suite of fast tests) must not reshuffle between runs
 * of the same binary, or the block stops being diffable. Insertion sort,
 * and stable, which is what makes the tie-break fall out of the table's
 * own build order -- the same shape _failgroup_order uses. */
static void
_durations_order(int *order)
{
    int i;
    int j;

    for (i = 0; i < _duration_count; i++)
    {
        for (j = i; j > 0 && _durations[order[j - 1]].time_sec < _durations[i].time_sec; j--)
        {
            order[j] = order[j - 1];
        }
        order[j] = i;
    }
}

/* Entries the block would list given the parsed limit: every retained
 * test when --durations 0 or when n outruns the test count, n otherwise.
 * Split out so the self-tests observe the same arithmetic the print path
 * uses rather than a re-derivation of it. */
static int
_durations_shown(void)
{
    if (_durations_limit > 0 && _durations_limit < _duration_count)
    {
        return _durations_limit;
    }
    return _duration_count;
}

/* Emit the block. Silent unless --durations was given, and silent on a run
 * that executed nothing (--list, or a filter that admitted no test), so
 * output without the flag is byte-identical to what it was before this
 * existed and the flag never produces a bare header over an empty body. */
static void
_durations_print(void)
{
    int order[LFG_CT_DURATIONS_MAX];
    int shown;
    int i;

    if (!_durations_set || 0 == _duration_count)
    {
        return;
    }

    _durations_order(order);
    shown = _durations_shown();

    printf("*** Slowest %d of %d test%s:\r\n", shown, _duration_count, (1 == _duration_count) ? "" : "s");
    for (i = 0; i < shown; i++)
    {
        const _duration_t *d = &_durations[order[i]];

        /* Milliseconds at the same %.3f precision as the verbose
         * per-test banner, so the two renderings of one number agree. */
        printf("*** %10.3f ms  ", d->time_sec * 1000.0);
        if (d->suite_name && d->suite_name[0])
        {
            printf("%s::%s\r\n", d->suite_name, d->test_name ? d->test_name : "(unnamed)");
        }
        else
        {
            printf("%s\r\n", d->test_name ? d->test_name : "(unnamed)");
        }
    }
    if (_duration_dropped > 0)
    {
        printf("*** %d further test%s unranked: the durations cap (LFG_CT_DURATIONS_MAX = %d) was reached\r\n",
                _duration_dropped, (1 == _duration_dropped) ? "" : "s", LFG_CT_DURATIONS_MAX);
    }
}

/* Append @p id to this run's failure set, ignoring a repeat (a nested
 * dispatch can classify the same id twice). An allocation failure is
 * swallowed on purpose: the cost is one missing entry in the *next*
 * run's selection, never the correctness or the verdict of this one. */
static void
_fail_record_id(const char *id)
{
    char *copy;
    int i;

    if (NULL == id || '\0' == id[0])
    {
        return;
    }
    for (i = 0; i < _fail_key_count; i++)
    {
        if (0 == strcmp(_fail_keys[i], id))
        {
            return;
        }
    }
    if (_fail_key_count == _fail_key_cap)
    {
        int cap = (_fail_key_cap > 0) ? (_fail_key_cap * 2) : 16;
        char **grown = (char **)realloc(_fail_keys, (size_t)cap * sizeof(*grown));

        if (NULL == grown)
        {
            return;
        }
        _fail_keys = grown;
        _fail_key_cap = cap;
    }
    copy = _str_dup(id);
    if (NULL == copy)
    {
        return;
    }
    _fail_keys[_fail_key_count++] = copy;
}

/* Record the currently-dispatching test as failed. Rebuilds the id from
 * the same batons the admission gate used, so the key persisted here is
 * byte-identical to the one --rerun-failed will match against. */
static void
_fail_record_current(const char *name)
{
    char id[LFG_CT_ID_MAX];

    _id_build(id, sizeof(id), _current_test_file, _current_suite_name, name);
    _fail_record_id(id);
}

/* Returns 0 on success, -1 on allocation failure. */
static int
_replay_push(const char *key)
{
    char *copy;

    if (_replay_key_count == _replay_key_cap)
    {
        int cap = (_replay_key_cap > 0) ? (_replay_key_cap * 2) : 16;
        char **grown_keys = (char **)realloc(_replay_keys, (size_t)cap * sizeof(*grown_keys));
        unsigned char *grown_flags;

        if (NULL == grown_keys)
        {
            return -1;
        }
        _replay_keys = grown_keys;
        grown_flags = (unsigned char *)realloc(_replay_resolved, (size_t)cap * sizeof(*grown_flags));
        if (NULL == grown_flags)
        {
            return -1;
        }
        _replay_resolved = grown_flags;
        _replay_key_cap = cap;
    }
    copy = _str_dup(key);
    if (NULL == copy)
    {
        return -1;
    }
    _replay_resolved[_replay_key_count] = 0;
    _replay_keys[_replay_key_count++] = copy;
    return 0;
}

/* Does @p id name one of the persisted keys? Marks the key resolved on a
 * hit, which is what separates "this test was renamed" from "this whole
 * state file is stale" at the end of the run. */
static int
_rerun_admits_id(const char *id)
{
    int i;

    if (NULL == id)
    {
        return 0;
    }
    for (i = 0; i < _replay_key_count; i++)
    {
        if (0 == strcmp(_replay_keys[i], id))
        {
            _replay_resolved[i] = 1;
            return 1;
        }
    }
    return 0;
}

/* Account for the persisted keys belonging to a suite this run never
 * descended into.
 *
 * A suite-level --filter-exclude match is decisive and skips the suite
 * body outright, so the contained lfg_ct_test registrations never happen
 * and their keys never reach _rerun_admits_id. Without this those keys
 * would be reported as renamed-or-removed, and an exclusion covering the
 * whole replay set would latch the run fatal over tests the user
 * deliberately excluded. They are accounted for, not orphaned.
 *
 * @p suite_id is a 2-component <file>::<suite> id; a test key belongs to
 * it exactly when it starts with "<suite_id>::". */
static void
_rerun_mark_suite_seen(const char *suite_id)
{
    size_t len;
    int i;

    if (NULL == suite_id)
    {
        return;
    }
    len = strlen(suite_id);
    for (i = 0; i < _replay_key_count; i++)
    {
        if (0 == strncmp(_replay_keys[i], suite_id, len)
                && 0 == strncmp(_replay_keys[i] + len, LFG_CT_ID_SEPARATOR, sizeof(LFG_CT_ID_SEPARATOR) - 1))
        {
            _replay_resolved[i] = 1;
        }
    }
}

/* Read one newline-terminated line into @p buf, stripped of its
 * terminator. Returns 1 on a line, 0 at end of file, -1 when the line
 * did not fit (the caller treats that as a malformed file rather than
 * splitting it into two fabricated records). A trailing CR is dropped so
 * a state file that travelled through a Windows host still parses. */
static int
_state_read_line(FILE *fp, char *buf, size_t cap)
{
    size_t n = 0;
    int c;

    for (;;)
    {
        c = fgetc(fp);
        if (EOF == c)
        {
            if (0 == n)
            {
                return 0;
            }
            break;
        }
        if ('\n' == c)
        {
            break;
        }
        if (n + 1 >= cap)
        {
            return -1;
        }
        buf[n++] = (char)c;
    }
    if (n > 0 && '\r' == buf[n - 1])
    {
        n--;
    }
    buf[n] = '\0';
    return 1;
}

/* The one strictness rule shared by every unsigned scalar on this CLI
 * (--seed, --timeout) and by the persisted seed in the state file: a bare
 * decimal digit run that fits an unsigned. strtoul on its own accepts
 * leading signs and whitespace and wraps a negative into a huge unsigned,
 * so "-1" and " 7" must be rejected rather than coerced.
 *
 * Returns 0 on success, -1 if malformed, -2 if well-formed but out of
 * range. Callers that distinguish the two emit different diagnostics. */
static int
_parse_unsigned_arg(const char *val, unsigned *out)
{
    char *endptr = NULL;
    unsigned long parsed;

    if ('\0' == val[0] || val[0] < '0' || val[0] > '9')
    {
        return -1;
    }
    errno = 0;
    parsed = strtoul(val, &endptr, 10);
    if ('\0' != *endptr)
    {
        return -1;
    }
#if ULONG_MAX > UINT_MAX
    if (ERANGE == errno || parsed > (unsigned long)UINT_MAX)
#else
    if (ERANGE == errno)
#endif
    {
        return -2;
    }
    *out = (unsigned)parsed;
    return 0;
}

/* The over-long-line diagnostic, shared by the header read and the record
 * loop so a first line that does not fit reports why it did not fit
 * rather than being folded into "is not a v1 state file". */
static void
_state_report_overlong(const char *progname, const char *path)
{
    fprintf(stderr, "%s: --rerun-failed: %s has an over-long line (max %d bytes)\r\n", progname, path,
            LFG_CT_STATE_LINE_MAX - 1);
}

/* Load @p path into the replay set and hand back the persisted seed.
 *
 * Returns 0 on success, -1 with a diagnostic already on stderr
 * otherwise. On any error the replay set is emptied again: a half-parsed
 * file must never narrow a run, because the user would believe they
 * replayed their failures when they replayed some prefix of them. */
static int
_state_load(const char *progname, const char *path, unsigned *seed_out)
{
    char line[LFG_CT_STATE_LINE_MAX];
    char expect[64];
    FILE *fp;
    int seen_seed = 0;
    int lineno = 0;
    int rc;

    _replay_clear();

    fp = fopen(path, "r");
    if (NULL == fp)
    {
        fprintf(stderr, "%s: --rerun-failed: no state file at %s (run the suite once first)\r\n", progname, path);
        return -1;
    }

    snprintf(expect, sizeof(expect), "%s %d", LFG_CT_STATE_MAGIC, LFG_CT_STATE_VERSION);
    rc = _state_read_line(fp, line, sizeof(line));
    if (-1 == rc)
    {
        _state_report_overlong(progname, path);
        fclose(fp);
        _replay_clear();
        return -1;
    }
    if (1 != rc || 0 != strcmp(line, expect))
    {
        fprintf(stderr, "%s: --rerun-failed: %s is not a v%d state file (expected first line \"%s\")\r\n",
                progname, path, LFG_CT_STATE_VERSION, expect);
        fclose(fp);
        _replay_clear();
        return -1;
    }

    while (1 == (rc = _state_read_line(fp, line, sizeof(line))))
    {
        lineno++;
        if ('\0' == line[0])
        {
            continue;
        }
        if (0 == strncmp(line, "seed ", 5))
        {
            if (0 != _parse_unsigned_arg(line + 5, seed_out))
            {
                fprintf(stderr, "%s: --rerun-failed: %s has a malformed seed: %s\r\n", progname, path, line + 5);
                fclose(fp);
                _replay_clear();
                return -1;
            }
            seen_seed = 1;
            continue;
        }
        if (0 == strncmp(line, "fail ", 5) && '\0' != line[5])
        {
            if (0 != _replay_push(line + 5))
            {
                fprintf(stderr, "%s: --rerun-failed: out of memory reading %s\r\n", progname, path);
                fclose(fp);
                _replay_clear();
                return -1;
            }
            continue;
        }
        fprintf(stderr, "%s: --rerun-failed: %s is malformed at record %d: %s\r\n", progname, path, lineno, line);
        fclose(fp);
        _replay_clear();
        return -1;
    }
    fclose(fp);

    if (-1 == rc)
    {
        _state_report_overlong(progname, path);
        _replay_clear();
        return -1;
    }
    if (!seen_seed)
    {
        fprintf(stderr, "%s: --rerun-failed: %s carries no seed record\r\n", progname, path);
        _replay_clear();
        return -1;
    }
    return 0;
}

/* Persist this run's seed and failure set to @p path. LF-only, like
 * --list output and for the same reason: the file is machine input, and
 * a stray CR would ride into the key and match nothing on replay.
 *
 * Written to a sibling temporary and renamed into place, so @p path is
 * only ever replaced by a complete file. Truncating in place would leave
 * a crash, a signal or ENOSPC mid-write behind a file that is still
 * well-formed -- magic, seed, and a prefix of the fail records -- which
 * the loader would accept and silently replay as a narrowed failure set.
 * That is exactly the outcome _state_load refuses on the read side. A
 * stale-but-complete record beats a fresh-but-truncated one, so every
 * failure path here leaves the previous run's file untouched.
 *
 * rename(3) and remove(3) are C89 <stdio.h>, so this adds no platform
 * surface beyond what the runner already uses.
 *
 * Returns 0 on success, -1 if the file could not be written. */
static int
_state_write(const char *path, unsigned seed)
{
    /* --state-file is arbitrary-length argv, so the temporary path is
     * sized from the real path rather than a fixed buffer. */
    size_t path_len = strlen(path);
    char *tmp = (char *)malloc(path_len + sizeof(LFG_CT_STATE_TMP_SUFFIX));
    FILE *fp;
    int write_failed;
    int i;

    if (NULL == tmp)
    {
        return -1;
    }
    memcpy(tmp, path, path_len);
    memcpy(tmp + path_len, LFG_CT_STATE_TMP_SUFFIX, sizeof(LFG_CT_STATE_TMP_SUFFIX));

    fp = fopen(tmp, "w");
    if (NULL == fp)
    {
        free(tmp);
        return -1;
    }
    fprintf(fp, "%s %d\n", LFG_CT_STATE_MAGIC, LFG_CT_STATE_VERSION);
    fprintf(fp, "seed %u\n", seed);
    for (i = 0; i < _fail_key_count; i++)
    {
        fprintf(fp, "fail %s\n", _fail_keys[i]);
    }
    /* Both halves must run: || would short-circuit past the fclose and
     * leak the stream on a write error. */
    write_failed = (0 != ferror(fp));
    if (0 != fclose(fp))
    {
        write_failed = 1;
    }
    if (write_failed)
    {
        remove(tmp);
        free(tmp);
        return -1;
    }

#ifdef _WIN32
    /* POSIX rename() replaces an existing destination atomically; the ISO C
     * behaviour is implementation-defined and Windows fails outright. Drop the
     * old file first there -- the window this opens is Windows-only and the
     * alternative is that every run after the first fails to persist. */
    remove(path);
#endif
    if (0 != rename(tmp, path))
    {
        remove(tmp);
        free(tmp);
        return -1;
    }
    free(tmp);
    return 0;
}

/* End-of-run verdict on the persisted keys. A key nobody claimed means
 * the test was renamed or removed since it failed -- warn and keep the
 * rest of the replay. Every key unclaimed means the state file no longer
 * describes this binary at all, which is latched as fatal: silently
 * running nothing would read as "all fixed". */
static void
_rerun_report_unresolved(void)
{
    int resolved = 0;
    int i;

    for (i = 0; i < _replay_key_count; i++)
    {
        if (_replay_resolved[i])
        {
            resolved++;
            continue;
        }
        fprintf(stderr, "*** WARNING: --rerun-failed: no registered test matches key: %s\r\n", _replay_keys[i]);
    }
    if (_replay_key_count > 0 && 0 == resolved)
    {
        fprintf(stderr, "*** ERROR: --rerun-failed: none of the %d persisted key(s) in %s match a registered "
                        "test\r\n",
                _replay_key_count, _state_path);
        _rerun_unresolved_fatal = 1;
    }
}

/* Recompute the active reporter slot from the verbose toggle and the
 * consumer-installed reporter. Called whenever either input changes
 * (parse_args toggling verbose, lfg_ct_set_reporter installing or
 * clearing the user slot). Keeping the resolution centralized means
 * neither caller has to know about the other's state.
 *
 * When verbose is on the active reporter is always the built-in
 * verbose reporter; its callbacks pull _user_reporter at fire time
 * and chain to it. When verbose is off the active reporter is the
 * user's slot directly (which may itself be NULL). */
static void
_reporter_activate(void)
{
    if (_verbosity >= LFG_CT_VERBOSITY_VERBOSE)
    {
        _reporter = &_verbose_reporter;
    }
    else
    {
        _reporter = _user_reporter;
    }
}

static void
_filter_state_reset(void)
{
    _list_mode = 0;
    _filter_glob_count = 0;
    _exclude_glob_count = 0;
    _filter_inherited_depth = 0;
    _strict_xpass = 0;
    _fail_fast = 0;
    _verbosity = LFG_CT_VERBOSITY_DEFAULT;
    _seed_set = 0;
    _seed_value = 0;
    _durations_set = 0;
    _durations_limit = 0;
    _rerun_failed = 0;
    _state_path = LFG_CT_STATE_PATH_DEFAULT;
    _rerun_unresolved_fatal = 0;
    _replay_clear();
    _reporter_activate();
}

static void
_filter_print_usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [options]\r\n"
            "  --list                   List registered test/suite ids and exit 0\r\n"
            "  --filter <glob>          Run only entries whose id matches <glob>\r\n"
            "  --filter-exclude <glob>  Skip entries whose id matches <glob>\r\n"
            "  --strict-xpass           Treat any xpass outcome as a failure (exit non-zero)\r\n"
            "  -x, --fail-fast          Stop the run at the first failing test\r\n"
            "  --durations <n>          After the run, list the <n> slowest tests; 0 lists all\r\n"
            "  --seed <n>               Seed rand() with <n> to replay a prior run\r\n"
            "  --rerun-failed           Run only the tests the previous run recorded as failed,\r\n"
            "                           restoring that run's seed\r\n"
            "  --state-file <path>      Read/write the rerun state at <path> (default %s)\r\n"
            "  -v, --verbose            Stream per-test START / outcome lines with elapsed ms\r\n"
            "  -q, --quiet              Print only failures, the seed, and the final summary\r\n"
            "  --verbosity <n>          Set the level that -q / -v alias: 0 quiet, 1 default, 2 verbose\r\n"
            "  --isolation <mode>       Dispatch tests in-process (none) or fork-per-test (fork)\r\n"
            "  --timeout <ms>           Per-test timeout under fork isolation; 0 disables it\r\n"
            "-q and -v are one axis; the last of them on the command line wins.\r\n"
            "Verbosity is presentation only -- exit codes, --list output, and the\r\n"
            "records an installed reporter receives are identical at every level.\r\n"
            "-x stops the whole run, not just the enclosing suite; the summary says so.\r\n"
            "Globs use shell-style syntax (*, ?, [...]) via fnmatch(3).\r\n"
            "An entry id is <file>::<suite>::<test>; a glob addresses as many\r\n"
            "trailing ::-components as it spells out, so a bare test name still\r\n"
            "works and * never crosses a ::.\r\n"
            "--filter and --filter-exclude may be repeated; exclude wins on overlap.\r\n"
            "--rerun-failed intersects with --filter: the filter narrows the replayed set.\r\n"
            "--isolation and --timeout drive the same state the lfg_ct_set_isolation and\r\n"
            "lfg_ct_set_fork_timeout_ms setters own, so a later setter call overrides the\r\n"
            "command line; call lfg_ct_parse_args last for the CLI to win.\r\n",
            progname ? progname : "test", LFG_CT_STATE_PATH_DEFAULT);
}

int
lfg_ct_parse_args(int argc, char *argv[])
{
    const char *progname;
    int i;

    /* Staged rather than applied at the flag: _isolation and
     * _fork_timeout_ms are API-owned, so a parse that fails partway
     * must leave the programmatic configuration exactly as it found
     * it. Committed in one block once the whole argv reads clean. */
    int isolation_set = 0;
    lfg_ct_isolation_t isolation_value = LFG_CT_ISOLATE_NONE;
    int timeout_set = 0;
    unsigned timeout_value = 0;

    _filter_state_reset();
    progname = (argc > 0 && argv && argv[0]) ? argv[0] : "test";

    for (i = 1; i < argc; i++)
    {
        const char *a = argv[i];

        const char **slot = NULL;
        int *count = NULL;

        if (0 == strcmp(a, "--list"))
        {
            _list_mode = 1;
            continue;
        }
        if (0 == strcmp(a, "--strict-xpass"))
        {
            _strict_xpass = 1;
            continue;
        }
        /* Short+long pair in the same strcmp chain, following -v /
         * --verbose. No clustering: -x is matched whole, so -xv is an
         * unknown flag rather than two flags. */
        if (0 == strcmp(a, "-x") || 0 == strcmp(a, "--fail-fast"))
        {
            _fail_fast = 1;
            continue;
        }
        /* -v and -q are the two ends of one axis, so neither errors on
         * the other and neither latches: whichever came last on the
         * command line is simply the level left standing. */
        if (0 == strcmp(a, "-v") || 0 == strcmp(a, "--verbose"))
        {
            _verbosity = LFG_CT_VERBOSITY_VERBOSE;
            _reporter_activate();
            continue;
        }
        if (0 == strcmp(a, "-q") || 0 == strcmp(a, "--quiet"))
        {
            _verbosity = LFG_CT_VERBOSITY_QUIET;
            _reporter_activate();
            continue;
        }
        if (0 == strcmp(a, "--verbosity"))
        {
            /* Scalar with a value, so it carries its own bounds check
             * rather than riding the slot/count tail below -- same shape
             * as --seed. Last flag wins, including against -q / -v. */
            const char *val;
            char *endptr;
            unsigned long parsed;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            val = argv[++i];

            /* Bare digit run only, for the reason --seed spells out:
             * strtoul would otherwise coerce " 1" and wrap "-1" into a
             * huge unsigned that then passes an upper-bound check. */
            errno = 0;
            endptr = NULL;
            parsed = strtoul(val, &endptr, 10);
            if ('\0' == val[0] || val[0] < '0' || val[0] > '9' || NULL == endptr || '\0' != *endptr)
            {
                fprintf(stderr, "%s: %s requires an integer 0..%d, got: %s\r\n", progname, a,
                        (int)LFG_CT_VERBOSITY_VERBOSE, val);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            if (ERANGE == errno || parsed > (unsigned long)LFG_CT_VERBOSITY_VERBOSE)
            {
                fprintf(stderr, "%s: %s value out of range (max %d): %s\r\n", progname, a,
                        (int)LFG_CT_VERBOSITY_VERBOSE, val);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }

            _verbosity = (lfg_ct_verbosity_t)parsed;
            _reporter_activate();
            continue;
        }
        if (0 == strcmp(a, "--rerun-failed"))
        {
            _rerun_failed = 1;
            continue;
        }
        if (0 == strcmp(a, "--state-file"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            /* Borrowed from argv, like the filter globs. Last flag wins. */
            _state_path = argv[++i];
            continue;
        }
        if (0 == strcmp(a, "--seed"))
        {
            /* Scalar, not accumulating, so it gets its own bounds check
             * instead of riding the slot/count tail below. Last flag wins. */
            const char *val;
            unsigned parsed;
            int rc;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            val = argv[++i];

            rc = _parse_unsigned_arg(val, &parsed);
            if (0 != rc)
            {
                if (-2 == rc)
                {
                    fprintf(stderr, "%s: %s value out of range (max %u): %s\r\n", progname, a, UINT_MAX, val);
                }
                else
                {
                    fprintf(stderr, "%s: %s requires a non-negative integer, got: %s\r\n", progname, a, val);
                }
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }

            _seed_value = parsed;
            _seed_set = 1;
            continue;
        }
        if (0 == strcmp(a, "--durations"))
        {
            /* Scalar with a value, sharing --seed's parser: the bare
             * digit-run rule is what rejects "-1" here rather than
             * letting strtoul wrap it into a huge unsigned that then
             * reads as a legal count. Last flag wins. */
            const char *val;
            unsigned parsed;
            int rc;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            val = argv[++i];

            rc = _parse_unsigned_arg(val, &parsed);
            /* Capped at INT_MAX rather than UINT_MAX because the limit is
             * only ever compared against a test count, which is an int. */
            if (0 != rc || parsed > (unsigned)INT_MAX)
            {
                if (-2 == rc || (0 == rc && parsed > (unsigned)INT_MAX))
                {
                    fprintf(stderr, "%s: %s value out of range (max %d): %s\r\n", progname, a, INT_MAX, val);
                }
                else
                {
                    fprintf(stderr, "%s: %s requires a non-negative integer, got: %s\r\n", progname, a, val);
                }
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }

            _durations_limit = (int)parsed;
            _durations_set = 1;
            continue;
        }
        if (0 == strcmp(a, "--isolation"))
        {
            /* Scalar taking a mode name, so it needs its own branch
             * rather than the glob slot/count tail below. Last flag
             * wins, as with every other scalar here. */
            const char *val;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            val = argv[++i];

            if (0 == strcmp(val, "none"))
            {
                isolation_value = LFG_CT_ISOLATE_NONE;
            }
            else if (0 == strcmp(val, "fork"))
            {
                /* Refuse instead of falling back to in-process. The
                 * setter already declines an unavailable mode without
                 * changing state; a silent downgrade here would report
                 * a fork run that never forked. */
                if (!_lfg_ct_fork_available())
                {
                    fprintf(stderr, "%s: %s fork: built without fork isolation support\r\n", progname, a);
                    _filter_print_usage(progname);
                    _filter_state_reset();
                    return -1;
                }
                isolation_value = LFG_CT_ISOLATE_FORK;
            }
            else
            {
                fprintf(stderr, "%s: %s requires one of none|fork, got: %s\r\n", progname, a, val);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }

            isolation_set = 1;
            continue;
        }
        if (0 == strcmp(a, "--timeout"))
        {
            /* Same scalar shape and same bare-digit-run rule as --seed, so
             * it shares --seed's parser. 0 is a meaningful value (timeout
             * disabled), so a bad value must fail the parse rather than
             * coerce to it. */
            const char *val;
            unsigned parsed;
            int rc;

            if (i + 1 >= argc)
            {
                fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }
            val = argv[++i];

            rc = _parse_unsigned_arg(val, &parsed);
            if (0 != rc)
            {
                if (-2 == rc)
                {
                    fprintf(stderr, "%s: %s value out of range (max %u): %s\r\n", progname, a, UINT_MAX, val);
                }
                else
                {
                    fprintf(stderr, "%s: %s requires a non-negative integer, got: %s\r\n", progname, a, val);
                }
                _filter_print_usage(progname);
                _filter_state_reset();
                return -1;
            }

            timeout_value = parsed;
            timeout_set = 1;
            continue;
        }
        if (0 == strcmp(a, "--filter"))
        {
            slot = _filter_globs;
            count = &_filter_glob_count;
        }
        else if (0 == strcmp(a, "--filter-exclude"))
        {
            slot = _exclude_globs;
            count = &_exclude_glob_count;
        }
        else
        {
            fprintf(stderr, "%s: unrecognized option: %s\r\n", progname, a);
            _filter_print_usage(progname);
            _filter_state_reset();
            return -1;
        }

        if (i + 1 >= argc)
        {
            fprintf(stderr, "%s: %s requires an argument\r\n", progname, a);
            _filter_print_usage(progname);
            _filter_state_reset();
            return -1;
        }
        if (*count >= LFG_CT_FILTER_MAX)
        {
            fprintf(stderr, "%s: too many %s patterns (max %d)\r\n", progname, a, LFG_CT_FILTER_MAX);
            _filter_state_reset();
            return -1;
        }
        slot[(*count)++] = argv[++i];
    }

    /* Load after the whole argv is consumed, not at the flag, so
     * --state-file and --seed bind regardless of the order the user
     * spelled them in. An explicit --seed outranks the persisted one:
     * replaying the same selection under different conditions is a
     * deliberate gesture, so the flag the user typed wins. */
    if (_rerun_failed)
    {
        unsigned persisted = 0;

        if (0 != _state_load(progname, _state_path, &persisted))
        {
            _filter_state_reset();
            return -1;
        }
        if (!_seed_set)
        {
            _seed_value = persisted;
            _seed_set = 1;
        }
    }

    /* Commit the API-owned settings only now that every error path is
     * behind us. Deliberately absent from _filter_state_reset: a parse
     * that never mentions these flags must leave a prior setter call
     * standing, unlike the filter/verbosity state it does own. */
    if (isolation_set)
    {
        (void)lfg_ct_set_isolation(isolation_value);
    }
    if (timeout_set)
    {
        lfg_ct_set_fork_timeout_ms(timeout_value);
    }
    return 0;
}

int
lfg_ct_is_list_mode(void)
{
    return _list_mode ? 1 : 0;
}

lfg_ct_verbosity_t
lfg_ct_verbosity(void)
{
    return _verbosity;
}

int
lfg_ct_is_verbose(void)
{
    return _verbosity >= LFG_CT_VERBOSITY_VERBOSE ? 1 : 0;
}

int
lfg_ct_is_seed_set(void)
{
    return _seed_set ? 1 : 0;
}

unsigned
lfg_ct_get_seed(void)
{
    return _seed_value;
}

int
lfg_ct_is_rerun_failed(void)
{
    return _rerun_failed ? 1 : 0;
}

const char *
lfg_ct_state_path(void)
{
    return _state_path;
}

/* void return, following lfg_ct_set_fork_timeout_ms: a boolean mode has
 * no way to fail. Contrast lfg_ct_set_isolation, which returns int only
 * because fork can be unavailable at runtime. */
void
lfg_ct_set_fail_fast(int enabled)
{
    _fail_fast = enabled ? 1 : 0;
}

int
lfg_ct_get_fail_fast(void)
{
    return _fail_fast;
}

/* Pure id predicate -- ignores list mode (which suppresses execution
 * regardless of filter state). Exclude is decisive and is evaluated first. */
static int
_filter_admits_id(const char *id)
{
    /* --rerun-failed gates first and independently of the glob rules, so
     * combining it with --filter intersects: the replay set is the
     * candidate pool, the filter narrows it. Gating first also means a
     * suite-level filter match (which sets _filter_inherited_depth) cannot
     * drag an unpersisted test back into a replay. */
    if (_rerun_failed && !_rerun_admits_id(id))
    {
        return 0;
    }
    if (_exclude_glob_count > 0 && _glob_list_matches_id(id, _exclude_globs, _exclude_glob_count))
    {
        return 0;
    }
    if (0 == _filter_glob_count || _filter_inherited_depth > 0)
    {
        return 1;
    }
    return _glob_list_matches_id(id, _filter_globs, _filter_glob_count);
}

/* Admission for a test named @p name registered in the current file/suite
 * context. The id is rebuilt from the batons rather than passed down, so
 * the public entry, the in-process entry and the fork child all gate on the
 * identical string. */
static int
_filter_admits_test(const char *name)
{
    char id[LFG_CT_ID_MAX];

    _id_build(id, sizeof(id), _current_test_file, _current_suite_name, name);
    return _filter_admits_id(id);
}

int
lfg_ct_name_runs(const char *name)
{
    if (_list_mode)
    {
        return 0;
    }
    return _filter_admits_test(name);
}

int
lfg_ct_id_runs(const char *file, const char *suite, const char *name)
{
    char id[LFG_CT_ID_MAX];

    if (_list_mode)
    {
        return 0;
    }
    _id_build(id, sizeof(id), file, (NULL != suite) ? suite : _current_suite_name, name);
    return _filter_admits_id(id);
}

#ifdef LFG_CT_COMPAT_3ARG
/* Setup-failure detector retained for the LFG_CT_COMPAT_3ARG shim: any
 * assertion failure flips _assertions_failed (normal mode) or
 * _expected_failures_count (expect-failures self-test mode). Summing both
 * lets the lifecycle helper notice setup failures regardless of which
 * mode is active. */
static int
_lifecycle_failure_total(void)
{
#ifdef LFG_CTEST_SELF_TEST
    return _assertions_failed + _expected_failures_count;
#else
    return _assertions_failed;
#endif
}

/* Retained shared setup -> body -> teardown lifecycle backing the
 * deprecated 3-argument registration under LFG_CT_COMPAT_3ARG. Teardown
 * runs whenever provided, even if the body or its assertions failed. If
 * setup itself fails an assertion the body is skipped but teardown still
 * runs -- setup may have partially acquired resources before failing. The
 * skip-aware setjmp boundary lives in the per-test path (skip is a
 * per-test gesture; suite-context skips are intentionally a no-op warning
 * -- see lfg_ct_suite_impl). Removed when the compat window closes. */
static void
_lfg_ct_run_lifecycle(void (*setup)(void), void (*body)(void), void (*teardown)(void))
{
    int setup_failures_before = 0;
    int setup_failed = 0;

    if (setup)
    {
        setup_failures_before = _lifecycle_failure_total();
        setup();
        setup_failed = (_lifecycle_failure_total() > setup_failures_before);
    }

    if (!setup_failed && body)
    {
        body();
    }

    if (teardown)
    {
        teardown();
    }
}
#endif /* LFG_CT_COMPAT_3ARG */

#ifdef LFG_CT_COMPAT_3ARG
void lfg_ct_suite_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name)
{
    lfg_ct_suite_impl_at(setup, fn, teardown, name, NULL);
}

void lfg_ct_suite_impl_at(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        const char *file)
#else
void lfg_ct_suite_impl(void (*fn)(void), const char *name)
{
    lfg_ct_suite_impl_at(fn, name, NULL);
}

void lfg_ct_suite_impl_at(void (*fn)(void), const char *name, const char *file)
#endif
{
    int suite_assertions_failed_before;
    int inherited_pushed = 0;
    int saved_skip_active;
    const char *saved_suite_name;
    char id[LFG_CT_ID_MAX];

    /* A suite id has no test component: <file>::<suite>. */
    _id_build(id, sizeof(id), file, name, NULL);

    if (_list_mode)
    {
        /* Print the suite id and recurse into the body so contained
         * lfg_ct_test() calls can list themselves. The suite baton is
         * published across the descent even though nothing executes:
         * without it the listed ids would carry LFG_CT_ID_NO_SUITE where
         * the real run carries the suite name, and --list output would no
         * longer feed back into --filter.
         *
         * LF-only, unlike the CRLF the human-facing report uses: a listing
         * line is machine input (`--list | grep | xargs --filter`), and a
         * trailing CR would ride into the glob and match nothing. */
        printf("%s\n", id);
        if (fn)
        {
            saved_suite_name = _current_suite_name;
            _current_suite_name = name;
            fn();
            _current_suite_name = saved_suite_name;
        }
        return;
    }

    /* Fail-fast: skip the whole suite without entering its body. Sits
     * below the list-mode block above, which returns first and so is
     * never shadowed -- a listing executes no test and can never trip
     * this. No "suite FAILURE" banner is reachable from here because the
     * body never ran and the delta this suite would report against is
     * never taken. */
    if (_fail_fast_tripped())
    {
        return;
    }

    /* Exclude is checked first and is decisive: a matched suite is skipped
     * entirely, no descent. Filter is checked second; a non-match still
     * descends so inner tests can be evaluated individually. A suite that
     * matches the filter propagates the pass to every descendant. */
    if (_exclude_glob_count > 0 && _glob_list_matches_id(id, _exclude_globs, _exclude_glob_count))
    {
        /* No descent means the contained tests never register, so claim
         * their persisted keys here or the replay would mistake an
         * exclusion for a stale state file. */
        if (_rerun_failed)
        {
            _rerun_mark_suite_seen(id);
        }
        return;
    }
    if (_filter_glob_count > 0 && 0 == _filter_inherited_depth
            && _glob_list_matches_id(id, _filter_globs, _filter_glob_count))
    {
        _filter_inherited_depth++;
        inherited_pushed = 1;
    }

    /* A suite body may drive assertions directly (not just via contained
     * tests), so track real assertion failures across the whole suite
     * scope to decide whether to print "suite FAILURE". Use
     * _assertions_failed so the print stays quiet when self-tests
     * intentionally drive suite-level failures inside expect-failures
     * mode. */
    suite_assertions_failed_before = _assertions_failed;

    /* lfg_ct_skip is a per-test gesture; suite-context skips are
     * intentionally unsupported. Force the skip boundary off across the
     * suite body so a stray lfg_ct_skip from suite-level code warns to
     * stderr and no-ops rather than longjmping to a stale outer boundary
     * or silently skipping the suite body. lfg_ct_test calls inside the
     * body re-enable the boundary themselves on entry. */
    saved_skip_active = _skip_env_active;
    _skip_env_active = 0;

    /* Save/restore the suite-name baton. A nested lfg_ct_suite surfaces
     * the innermost suite as the reporter record's suite_name; on
     * return the outer suite's name is restored. */
    saved_suite_name = _current_suite_name;
    _current_suite_name = name;

    _current_suite_failures = 0;
#ifdef LFG_CT_COMPAT_3ARG
    _lfg_ct_run_lifecycle(setup, fn, teardown);
#else
    if (fn)
    {
        fn();
    }
#endif

    _current_suite_name = saved_suite_name;
    _skip_env_active = saved_skip_active;

    /* Suppressed at quiet despite naming a failure: it is an aggregate
     * that adds no localization over the per-test FAILURE line plus the
     * assertion detail, and a consumer registering many same-named
     * suites gets one repetition per invocation. That repetition is the
     * noise quiet exists to remove. */
    if (!_quiet_mode() && (_current_suite_failures > 0 || _assertions_failed > suite_assertions_failed_before))
    {
        printf("*** suite FAILURE: %s\r\n", name);
    }

    if (inherited_pushed)
    {
        _filter_inherited_depth--;
    }
}

/* Public dispatch. List-mode and filter checks are name-only and have
 * to happen in the parent regardless of isolation, so they sit here.
 * Fork mode delegates the rest to lfg-ctest-fork.c, which manages
 * fork/wait/payload and re-enters _lfg_ct_test_impl_inproc inside the
 * child. The in-process branch is the original code path, untouched.
 *
 * The reporter's @c on_test_start fires here -- AFTER the gates have
 * admitted the test, but BEFORE any fork. This single fire site keeps
 * verbose-mode START banners serialised in the parent regardless of
 * isolation, and the child's reentry into @c _lfg_ct_test_impl_inproc
 * intentionally does not re-fire to avoid duplicates. */
#ifdef LFG_CT_COMPAT_3ARG
void lfg_ct_test_impl(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name)
{
    lfg_ct_test_impl_at(setup, fn, teardown, name, NULL);
}

void lfg_ct_test_impl_at(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name,
        const char *file)
#else
void lfg_ct_test_impl(void (*fn)(void), const char *name)
{
    lfg_ct_test_impl_at(fn, name, NULL);
}

void lfg_ct_test_impl_at(void (*fn)(void), const char *name, const char *file)
#endif
{
    const char *saved_test_file;
    char id[LFG_CT_ID_MAX];

    _id_build(id, sizeof(id), file, _current_suite_name, name);

    /* Publish the file baton across the whole dispatch so the in-process
     * entry -- and the fork child re-entering it -- rebuild this same id
     * when they re-check the filter. Saved/restored because a test body may
     * itself register a nested test (the self-test pattern). */
    saved_test_file = _current_test_file;
    _current_test_file = file;

    if (_list_mode)
    {
        /* LF-only: see the suite listing above. */
        printf("%s\n", id);
        _current_test_file = saved_test_file;
        return;
    }

    /* Fail-fast: suppress before the filter check, the reporter's
     * on_test_start, and any dispatch. Placed above the isolation branch
     * on purpose -- under fork isolation the child that produced the
     * failure was already reaped by _lfg_ct_fork_run_test's synchronous
     * waitpid before its outcome reached _tests_failed, so by the time
     * this can observe the trip there is nothing left to tear down and no
     * window for an orphan. The gate needs no knowledge of which path
     * dispatched. */
    if (_fail_fast_tripped())
    {
        _current_test_file = saved_test_file;
        return;
    }

    if (!_filter_admits_id(id))
    {
        _current_test_file = saved_test_file;
        return;
    }
    if (_reporter && _reporter->on_test_start)
    {
        _reporter->on_test_start(_current_suite_name, name, _reporter->userdata);
    }
    if (LFG_CT_ISOLATE_FORK == _isolation)
    {
#ifdef LFG_CT_COMPAT_3ARG
        _lfg_ct_fork_run_test(setup, fn, teardown, name, _fork_timeout_ms);
#else
        _lfg_ct_fork_run_test(fn, name, _fork_timeout_ms);
#endif
        _current_test_file = saved_test_file;
        return;
    }
#ifdef LFG_CT_COMPAT_3ARG
    _lfg_ct_test_impl_inproc(setup, fn, teardown, name);
#else
    _lfg_ct_test_impl_inproc(fn, name);
#endif
    _current_test_file = saved_test_file;
}

/* Internal in-process dispatch. Was lfg_ct_test_impl historically; the
 * isolation shim now sits between the user and this entry point. Also
 * called directly by the fork TU from inside the child after the swap
 * to a capture reporter. */
#ifdef LFG_CT_COMPAT_3ARG
void _lfg_ct_test_impl_inproc(void (*setup)(void), void (*fn)(void), void (*teardown)(void), const char *name)
#else
void _lfg_ct_test_impl_inproc(void (*fn)(void), const char *name)
#endif
{
#ifdef LFG_CT_COMPAT_3ARG
    int setup_failures_before = 0;
    int setup_failed = 0;
#endif
    /* Save/restore the outer skip boundary so a nested lfg_ct_test_impl
     * (the self-test pattern that drives mock tests through the real
     * runner) cannot strand an in-progress outer body with an
     * overwritten _skip_env. The jmp_buf is an array type, so memcpy
     * is the portable way to snapshot it. */
    jmp_buf saved_env;
    int saved_active;
    char saved_failure_inline[LFG_CT_FAILURE_MSG_MAX];
    char *saved_failure_heap;
    int saved_failures;
    _disposition_t saved_disposition;
    const char *saved_skip_reason;
    int saved_xfail_set;
    const char *saved_xfail_reason;
    double time_start;
    double elapsed;

    if (_list_mode)
    {
        char id[LFG_CT_ID_MAX];

        _id_build(id, sizeof(id), _current_test_file, _current_suite_name, name);
        /* LF-only: see lfg_ct_suite_impl_at. */
        printf("%s\n", id);
        return;
    }
    if (!_filter_admits_test(name))
    {
        return;
    }

    _tests_executed++;

    /* Save/restore the per-test state block for the same reason the
     * failure-message slot below is saved: a nested test_impl call (the
     * self-test pattern) must neither inherit nor erase the enclosing
     * test's classification inputs. Zeroing without saving loses an
     * outer failure recorded before the nested dispatch, and the outer
     * is then silently bucketed PASSED. _last_classified_xfail_reason is
     * deliberately *not* nested -- it is last-call-wins by design. */
    saved_failures = _current_test_failures;
    saved_disposition = _current_disposition;
    saved_skip_reason = _current_skip_reason;
    saved_xfail_set = _current_xfail_set;
    saved_xfail_reason = _current_xfail_reason;
    _current_test_failures = 0;
    _current_disposition = LFG_CT_DISP_NORMAL;
    _current_skip_reason = NULL;
    _current_xfail_set = 0;
    _current_xfail_reason = NULL;

    /* Save/restore the failure-message slot so a nested test_impl call
     * doesn't smear its captured message over the outer test's. After
     * the nested call returns, the outer body may still log its own
     * assertion failure -- that must be the message the outer test
     * reports, not the nested one's. The heap tier is *moved* into
     * the saved slot rather than copied, so this level never owns --
     * and never frees -- the outer test's block. */
    memcpy(saved_failure_inline, _failure_msg_inline, sizeof(saved_failure_inline));
    saved_failure_heap = _failure_msg_heap;
    _failure_msg_inline[0] = '\0';
    _failure_msg_heap = NULL;
    time_start = _monotonic_sec();

    memcpy(saved_env, _skip_env, sizeof(jmp_buf));
    saved_active = _skip_env_active;

#ifdef LFG_CT_COMPAT_3ARG
    /* Deprecated 3-argument path: separate skip-aware boundary around
     * setup so a setup-side lfg_ct_skip or assertion failure skips the
     * body but still runs teardown (the retained pre-#35 semantics). */
    if (setup)
    {
        setup_failures_before = _lifecycle_failure_total();
        _skip_env_active = 1;
        if (0 == setjmp(_skip_env))
        {
            setup();
        }
        _skip_env_active = 0;
        setup_failed = (_lifecycle_failure_total() > setup_failures_before);
    }

    /* lfg_ct_skip in setup propagates by setting _current_disposition;
     * the body is skipped in that case so callers see the same
     * "preconditions not met" semantics whether the skip fired in setup
     * or in the body. */
    if (LFG_CT_DISP_SKIPPED != _current_disposition && !setup_failed && fn)
#else
    /* Single skip-aware boundary around the body. Setup and teardown are
     * the body's own concern now, so lfg_ct_skip inside the body unwinds
     * straight here; any teardown the body needs on the skip path runs
     * before the skip call (the blessed convention). */
    if (fn)
#endif
    {
        _skip_env_active = 1;
        if (0 == setjmp(_skip_env))
        {
            fn();
        }
        _skip_env_active = 0;
    }

#ifdef LFG_CT_COMPAT_3ARG
    if (teardown)
    {
        teardown();
    }
#endif

    memcpy(_skip_env, saved_env, sizeof(jmp_buf));
    _skip_env_active = saved_active;

    elapsed = _monotonic_sec() - time_start;
    if (elapsed < 0.0)
    {
        elapsed = 0.0;
    }

    /* Classification. SKIP wins over a stray failure that may have
     * occurred before the skip call -- the test author signalled intent
     * to skip, so honor it. XFAIL/XPASS only apply when no skip fired.
     * For XFAIL/XPASS/SKIP we absorb the per-test assertion failures
     * back out of the global _assertions_failed so the surrounding
     * suite-failure detector doesn't trip on them.
     *
     * After bucketing, hand a record to the reporter (if any) -- one
     * fire per classified test, with all the facts the downstream
     * consumer needs to render whatever report format it cares about. */
    {
        lfg_ct_outcome_t outcome = LFG_CT_PASSED;
        const char *message = NULL;

        if (LFG_CT_DISP_SKIPPED == _current_disposition)
        {
            if (_current_test_failures > 0)
            {
                _assertions_failed -= _current_test_failures;
            }
            _tests_skipped++;
            if (!_quiet_mode())
            {
                printf("*** test SKIP: %s: %s\r\n", name,
                        _current_skip_reason ? _current_skip_reason : "(no reason)");
            }
            outcome = LFG_CT_SKIPPED;
            message = _current_skip_reason;
        }
        else if (_current_xfail_set)
        {
            _last_classified_xfail_reason = _current_xfail_reason;
            if (_current_test_failures > 0)
            {
                _assertions_failed -= _current_test_failures;
                _tests_xfailed++;
                if (!_quiet_mode())
                {
                    printf("*** test XFAIL: %s: %s\r\n", name,
                            _current_xfail_reason ? _current_xfail_reason : "(no reason)");
                }
                outcome = LFG_CT_XFAIL;
                message = _current_xfail_reason;
            }
            else
            {
                _tests_xpassed++;
                if (!_quiet_mode())
                {
                    printf("*** test XPASS: %s: %s\r\n", name,
                            _current_xfail_reason ? _current_xfail_reason : "(no reason)");
                }
                outcome = LFG_CT_XPASS;
                message = _current_xfail_reason;
            }
        }
        else if (_current_test_failures > 0)
        {
            _current_suite_failures++;
            _tests_failed++;
            printf("*** test FAILURE: %s\r\n", name);
            outcome = LFG_CT_FAILED;
            message = _lfg_ct_failure_msg();
            _fail_record_current(name);
            _failgroup_record(name, message);
        }
        else
        {
            _tests_passed++;
            outcome = LFG_CT_PASSED;
            message = NULL;
        }

        /* First of the two durations feed sites. Ahead of the reporter
         * fire so the accumulator sees every classified test whether or
         * not a reporter is installed. */
        _durations_record(_current_suite_name, name, elapsed);

        if (_reporter && _reporter->on_record)
        {
            lfg_ct_record_t rec;
            rec.suite_name = _current_suite_name;
            rec.test_name = name;
            rec.time_sec = elapsed;
            rec.outcome = outcome;
            rec.message = message;
            _reporter->on_record(&rec, _reporter->userdata);
        }
    }

    /* Restore the enclosing level's per-test state. This level's own
     * values are discarded here, so a nested test_impl call leaks none of
     * its disposition/xfail flags or failure count into the calling
     * test's classification. */
    _current_test_failures = saved_failures;
    _current_disposition = saved_disposition;
    _current_skip_reason = saved_skip_reason;
    _current_xfail_set = saved_xfail_set;
    _current_xfail_reason = saved_xfail_reason;

    /* Release this level's own message (the reporter callback above
     * has already returned, so the borrowed-for-the-callback contract
     * is honoured) before moving the outer test's back in. */
    _lfg_ct_failure_msg_clear();
    memcpy(_failure_msg_inline, saved_failure_inline, sizeof(_failure_msg_inline));
    _failure_msg_heap = saved_failure_heap;
}

void lfg_ct_print_summary(void)
{
    int strict_xpass_fail;
    const char *verdict;

    if (_list_mode)
    {
        /* No tests ran; reporting "0 assertions / 0 tests" is misleading. */
        return;
    }

    strict_xpass_fail = (_strict_xpass && _tests_xpassed > 0);
    verdict = (_tests_failed > 0 || strict_xpass_fail) ? "FAIL" : "PASS";

    /* Split into two calls so the grouped failure block can land
     * between them. Both lines' text is unchanged: downstream tooling
     * and the framework's own tests match them exactly, and the verdict
     * stays last so tailing or grepping the end of a log still works. */
    printf("*** Executed %d assertions in %d tests. "
           "Failures: %d, Skipped: %d, XFail: %d, XPass: %d\r\n",
            _assertions_executed, _tests_executed, _tests_failed, _tests_skipped, _tests_xfailed, _tests_xpassed);

    /* Suppressed tests are not executed, so they land in no bucket --
     * _tests_executed simply ends up lower. Without this line a fail-fast
     * run is indistinguishable from one where most tests silently
     * vanished. Deliberately not reclassified as SKIP: that bucket is a
     * per-test disposition set by the body via lfg_ct_skip, and borrowing
     * it here would corrupt the tally semantics skip/xfail maintain.
     *
     * Printed at every verbosity, quiet included -- it qualifies the
     * counts immediately above it, so hiding it would leave quiet's
     * summary actively misleading rather than merely terse. */
    if (_fail_fast_tripped())
    {
        printf("*** Stopped early: --fail-fast tripped at the first test failure; "
               "remaining tests were not run.\r\n");
    }

    _failgroup_print();
    printf("*** Testing complete. Result: %s\r\n", verdict);

    /* Deliberately *after* the verdict, unlike the failure summary above.
     * The verdict-stays-last rule exists so a tail or a grep of a default
     * run's log still finds it; --durations is opt-in, so a run that asks
     * for the block has already accepted trailing output, and putting the
     * ranking last keeps it adjacent to the prompt where the reader is
     * looking. Still ahead of on_run_complete, so a buffering reporter's
     * flush stays the last thing the run emits. */
    _durations_print();

    /* Reporter's run-complete hook. A buffering reporter (e.g. the
     * contrib JUnit emitter) flushes its accumulated state here. A
     * failure inside the hook is the reporter's concern -- the
     * runner's exit-code contract is unchanged by reporter side
     * effects. */
    if (_reporter && _reporter->on_run_complete)
    {
        _reporter->on_run_complete(_reporter->userdata);
    }

    /* Persist last, once every test has classified. A --rerun-failed run
     * rewrites the file with *its* failures, so repeating the flag
     * narrows toward what still fails instead of replaying the original
     * set forever. --list returned above, so a listing never truncates
     * the file the last real run left behind. */
    if (_rerun_failed && !_fail_fast_tripped())
    {
        /* A key the run never reached is not a key that stopped naming a
         * registered test, so the unresolved warnings would all be false
         * alarms here. Skipped wholesale rather than per-key: once the
         * gate trips, "unclaimed" carries no information at all. */
        _rerun_report_unresolved();
    }
    if (0 != _state_write(_state_path, _effective_seed))
    {
        /* Advisory: an unwritable state file costs the next run its
         * replay, not this run its verdict. */
        fprintf(stderr, "*** WARNING: could not write rerun state file: %s\r\n", _state_path);
    }
}

void lfg_ct_set_reporter(const lfg_ct_reporter_t *reporter)
{
    _user_reporter = reporter;
    _reporter_activate();
}

/* Built-in verbose reporter. Prints a per-test START banner from
 * @c on_test_start (fired by the parent dispatcher before any fork)
 * and a per-test outcome banner from @c on_record (fired by either
 * the in-process classification or the fork TU's payload projection).
 * Both banners include the elapsed time in milliseconds with sub-ms
 * precision; the outcome banner uses the keywords the acceptance
 * criterion calls out (@c PASS / @c FAIL / @c SKIP / @c XFAIL /
 * @c XPASS). After printing, each callback delegates to
 * @c _user_reporter so a consumer-installed reporter (e.g. the
 * contrib JUnit emitter) keeps observing every event alongside the
 * streamed output -- single fan-out site, no parallel print path.
 *
 * @c fflush(stdout) after each line keeps the START line visible
 * before a slow body completes (useful for the "which test hung CI?"
 * question) and prevents the parent's stdout buffer from carrying
 * unflushed bytes into a forked child where the duplicate would
 * surface at the child's own buffer flush. */

static void
_verbose_print_qualified_name(const char *suite_name, const char *test_name)
{
    if (suite_name && suite_name[0])
    {
        printf("%s::%s", suite_name, test_name ? test_name : "(unnamed)");
    }
    else
    {
        printf("%s", test_name ? test_name : "(unnamed)");
    }
}

static void
_verbose_on_test_start(const char *suite_name, const char *test_name, void *userdata)
{
    (void)userdata;
    printf("*** START: ");
    _verbose_print_qualified_name(suite_name, test_name);
    printf("\r\n");
    fflush(stdout);

    if (_user_reporter && _user_reporter->on_test_start)
    {
        _user_reporter->on_test_start(suite_name, test_name, _user_reporter->userdata);
    }
}

static void
_verbose_on_record(const lfg_ct_record_t *record, void *userdata)
{
    const char *keyword;
    double elapsed_ms;

    (void)userdata;

    switch (record->outcome)
    {
        case LFG_CT_PASSED:
            keyword = "PASS";
            break;
        case LFG_CT_FAILED:
            keyword = "FAIL";
            break;
        case LFG_CT_SKIPPED:
            keyword = "SKIP";
            break;
        case LFG_CT_XFAIL:
            keyword = "XFAIL";
            break;
        case LFG_CT_XPASS:
            keyword = "XPASS";
            break;
        default:
            keyword = "?";
            break;
    }

    elapsed_ms = (record->time_sec < 0.0) ? 0.0 : record->time_sec * 1000.0;
    printf("*** %s: ", keyword);
    _verbose_print_qualified_name(record->suite_name, record->test_name);
    printf(" (%.3f ms)", elapsed_ms);
    if (record->message && record->message[0])
    {
        printf(": %s", record->message);
    }
    printf("\r\n");
    fflush(stdout);

    if (_user_reporter && _user_reporter->on_record)
    {
        _user_reporter->on_record(record, _user_reporter->userdata);
    }
}

static void
_verbose_on_run_complete(void *userdata)
{
    (void)userdata;
    if (_user_reporter && _user_reporter->on_run_complete)
    {
        _user_reporter->on_run_complete(_user_reporter->userdata);
    }
}

int lfg_ct_set_isolation(lfg_ct_isolation_t mode)
{
    if (LFG_CT_ISOLATE_FORK == mode && !_lfg_ct_fork_available())
    {
        return -1;
    }
    _isolation = mode;
    return 0;
}

lfg_ct_isolation_t lfg_ct_get_isolation(void)
{
    return _isolation;
}

void lfg_ct_set_fork_timeout_ms(unsigned timeout_ms)
{
    _fork_timeout_ms = timeout_ms;
}

unsigned lfg_ct_get_fork_timeout_ms(void)
{
    return _fork_timeout_ms;
}

/* Internal bridges consumed by lfg-ctest-fork.c. Not part of the
 * public API; kept out of lfg-ctest.h on purpose. Declared here so
 * the fork TU sees the prototypes via extern declarations local to
 * its own source file. */

const lfg_ct_reporter_t *_lfg_ct_get_reporter(void)
{
    return _reporter;
}

/* Direct-active-slot setter for the fork TU's child path. Bypasses
 * the verbose chain: assigns @p reporter to the active slot without
 * touching @c _user_reporter or @c _verbosity, so the child's
 * capture reporter receives @c on_record directly even when the
 * parent had verbose mode active. The child uses this to install
 * capture, run the body, and (best-effort) restore the prior active
 * slot before @c _exit -- the address space is about to be torn
 * down, so the restore is hygiene rather than required. */
void _lfg_ct_set_active_reporter_direct(const lfg_ct_reporter_t *reporter)
{
    _reporter = reporter;
}

void _lfg_ct_counter_snapshot(int *executed, int *passed, int *failed)
{
    if (executed)
    {
        *executed = _assertions_executed;
    }
    if (passed)
    {
        *passed = _assertions_passed;
    }
    if (failed)
    {
        *failed = _assertions_failed;
    }
}

/* Project a forked-child's classified outcome onto the parent's
 * bookkeeping. Mirrors the post-classification side of
 * _lfg_ct_test_impl_inproc: bumps the right counters, increments
 * suite-failure tracking on FAIL, fires the active reporter. When
 * @p print_outcome_line is non-zero the parent also emits the
 * per-test outcome banner (used for signal/timeout/payload-missing
 * paths where the child died before its own print landed); the
 * normal payload path passes 0 because the child already printed
 * via inherited stdout. */
void _lfg_ct_record_external(const char *name, double time_sec, lfg_ct_outcome_t outcome, const char *message,
        int assertions_executed_delta, int assertions_passed_delta, int assertions_failed_delta,
        int print_outcome_line)
{
#ifdef LFG_CTEST_SELF_TEST
    int suppress_failure = _expect_failures_mode && (LFG_CT_FAILED == outcome);
#else
    int suppress_failure = 0;
#endif

    /* Fork mode's record of the child's disposition. The parent is the
     * one that persists it -- the child's own copy of the failure set dies
     * with its address space -- and it does so from the projected outcome,
     * never from the child's stdout.
     *
     * Recorded ahead of the expect-failures suppression below on purpose:
     * the persisted set mirrors what the run *classified*, and that
     * suppression is a self-test-only exit-code device, not a
     * reclassification. The self-test binaries clear the set before their
     * summary so their own state file stays honest. */
    if (LFG_CT_FAILED == outcome)
    {
        _fail_record_current(name);
    }

    _tests_executed++;
    _assertions_executed += assertions_executed_delta;
    _assertions_passed += assertions_passed_delta;
    if (!suppress_failure)
    {
        _assertions_failed += assertions_failed_delta;
    }

    switch (outcome)
    {
        case LFG_CT_PASSED:
            _tests_passed++;
            break;
        case LFG_CT_FAILED:
            if (suppress_failure)
            {
#ifdef LFG_CTEST_SELF_TEST
                /* Self-test driver intentionally produced this FAILED
                 * outcome (e.g. drove an inner fork-mode test that
                 * crashes / times out / aborts). Mirror the assertion
                 * counter's expect-failures behavior: count it as
                 * expected and leave the global pass/fail tallies
                 * alone so the binary still exits 0. */
                _expected_failures_count++;
#endif
                break;
            }
            _tests_failed++;
            _current_suite_failures++;
            /* Second of the two accumulator feed sites. Reached only on
             * the non-suppressed path, so an expect-failures self-test
             * run leaves the block as empty as it leaves the tallies. */
            _failgroup_record(name, message);
            if (print_outcome_line)
            {
                printf("*** test FAILURE: %s\r\n", name);
                if (message)
                {
                    printf("*** %s\r\n", message);
                }
            }
            break;
        /* The FAILED arm above is deliberately not gated. Every call site
         * that passes print_outcome_line = 1 does so on an abnormal exit
         * -- pipe/fork failure, signal, timeout, truncated payload -- where
         * the child never got to report. That synthesized line is the only
         * diagnostic a quiet run of a crashing test has, so quiet keeps it.
         *
         * The non-failure arms below carry the gate defensively only: a
         * child that completes normally prints its own outcome line and is
         * recorded here with print_outcome_line = 0, so fork-mode quiet
         * suppression is really the child's inherited in-process gate
         * doing the work. No current call site pairs a non-FAILED outcome
         * with print_outcome_line = 1; the check is here so that adding
         * one cannot leak an outcome line past quiet. */
        case LFG_CT_SKIPPED:
            _tests_skipped++;
            if (print_outcome_line && !_quiet_mode())
            {
                printf("*** test SKIP: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
        case LFG_CT_XFAIL:
            _tests_xfailed++;
            if (print_outcome_line && !_quiet_mode())
            {
                printf("*** test XFAIL: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
        case LFG_CT_XPASS:
            _tests_xpassed++;
            if (print_outcome_line && !_quiet_mode())
            {
                printf("*** test XPASS: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
    }

    /* Second of the two durations feed sites. Missing this one would
     * leave a fork-isolated run reporting an empty durations block --
     * the parent projects the child's classification, so this is the
     * only place a forked test's elapsed time reaches the runner. */
    _durations_record(_current_suite_name, name, time_sec);

    if (_reporter && _reporter->on_record)
    {
        lfg_ct_record_t rec;
        rec.suite_name = _current_suite_name;
        rec.test_name = name;
        rec.time_sec = time_sec;
        rec.outcome = outcome;
        rec.message = message;
        _reporter->on_record(&rec, _reporter->userdata);
    }
}

int lfg_ct_return(void)
{
    /* Strict mode flips an otherwise-clean run with xpass outcomes to
     * non-zero. Negative sign mirrors the prior contract (-_tests_failed). */
    if (_tests_failed > 0)
    {
        return -_tests_failed;
    }
    /* A replay that resolved none of its keys ran nothing at all. Exiting
     * 0 there would read as "everything is fixed", which is the one
     * outcome the user must never be handed silently. */
    if (_rerun_unresolved_fatal)
    {
        return -1;
    }
    if (_strict_xpass && _tests_xpassed > 0)
    {
        return -1;
    }
    return 0;
}

size_t lfg_ct_failure_count(void)
{
    /* Global running tally, not a per-test slice. Within a single test or
     * setup body it is strictly monotonic -- every failed assertion bumps
     * it -- which is exactly what the snapshot-and-compare uses (loop
     * fail-fast, setup-failure detection) rely on. At a test boundary the
     * classifier absorbs a completed test's failures back out for
     * SKIP / XFAIL / XPASS, so the count is only guaranteed monotonic
     * inside one body; consumers snapshot at the top of a region and
     * compare, never assume cross-test monotonicity. Reads the same
     * counter the runner keeps internally (and that the LFG_CTEST_SELF_TEST
     * accessor exposes) -- the value is never negative in any observable
     * state, so the cast to size_t is total. */
    return (size_t)_assertions_failed;
}

void lfg_ct_skip_cleanup_impl(void (*cleanup)(void), const char *reason, const char *file, int line, const char *function)
{
    if (!_skip_env_active)
    {
        fprintf(stderr, "*** %s: %d: WARNING in %s(): skip(\"%s\") called outside a test context; ignoring\r\n",
                file ? file : "(unknown)", line, function ? function : "(unknown)", reason ? reason : "");
        return;
    }
    /* Run cleanup before the longjmp unwinds the body -- identical to the
     * two-line teardown-before-skip convention this call sugars over. */
    if (cleanup)
    {
        cleanup();
    }
    _current_disposition = LFG_CT_DISP_SKIPPED;
    _current_skip_reason = reason;
    longjmp(_skip_env, 1);
}

void lfg_ct_skip_impl(const char *reason, const char *file, int line, const char *function)
{
    lfg_ct_skip_cleanup_impl(NULL, reason, file, line, function);
}

void lfg_ct_xfail_impl(const char *reason, const char *file, int line, const char *function)
{
    if (!_skip_env_active)
    {
        fprintf(stderr, "*** %s: %d: WARNING in %s(): lfg_ct_xfail(\"%s\") called outside a test context; ignoring\r\n",
                file ? file : "(unknown)", line, function ? function : "(unknown)", reason ? reason : "");
        return;
    }
    /* Last reason wins -- repeated calls just overwrite. */
    _current_xfail_set = 1;
    _current_xfail_reason = reason;
}

/*============================================================================
 *  Self-Test API (only available when LFG_CTEST_SELF_TEST is defined)
 *==========================================================================*/

#ifdef LFG_CTEST_SELF_TEST

void lfg_ct_expect_failures_begin(void)
{
    _expect_failures_mode = 1;
    _expected_failures_count = 0;
}

int lfg_ct_expect_failures_end(void)
{
    _expect_failures_mode = 0;
    return _expected_failures_count;
}

int lfg_ct_self_skipped_count(void)
{
    return _tests_skipped;
}

int lfg_ct_self_xfailed_count(void)
{
    return _tests_xfailed;
}

int lfg_ct_self_xpassed_count(void)
{
    return _tests_xpassed;
}

int lfg_ct_self_failed_count(void)
{
    return _tests_failed;
}

int lfg_ct_self_assertions_failed(void)
{
    return _assertions_failed;
}

void lfg_ct_self_set_strict_xpass(int enabled)
{
    _strict_xpass = enabled ? 1 : 0;
}

/* Fail-fast trip injection. Same shape, and same reason, as the rerun and
 * failgroup hooks below: expect-failures mode suppresses a genuine
 * failure before it ever bumps _tests_failed, so the framework's own
 * tests cannot trip the gate by failing a nested test on purpose.
 *
 * Save/restore rather than a reset-to-zero so a real failure recorded
 * before the armed window survives it and the self-test binary's own exit
 * code stays honest. Nothing can be lost inside the window: with the gate
 * tripped no test classifies, so no test can bump the counter.
 *
 * The armed flag is what makes that guarantee hold for callers too. A
 * disarm with no matching arm must not write anything back -- otherwise
 * it restores a stale save slot over a live tally, which is the exact
 * clobber the save/restore exists to prevent. Both entry points are
 * therefore idempotent: disarm-without-arm and arm-while-armed are
 * no-ops, so a shared reset helper can call disarm unconditionally. */
static int _fail_fast_saved_tests_failed = 0;
static int _fail_fast_armed = 0;

void lfg_ct_self_fail_fast_arm(void)
{
    if (_fail_fast_armed)
    {
        return;
    }
    _fail_fast_saved_tests_failed = _tests_failed;
    _fail_fast_armed = 1;
    _tests_failed = 1;
}

void lfg_ct_self_fail_fast_disarm(void)
{
    if (!_fail_fast_armed)
    {
        return;
    }
    _tests_failed = _fail_fast_saved_tests_failed;
    _fail_fast_armed = 0;
}

int lfg_ct_self_return_code(void)
{
    return lfg_ct_return();
}

const char *lfg_ct_self_last_xfail_reason(void)
{
    return _last_classified_xfail_reason;
}

void lfg_ct_self_rerun_note_failure(const char *file, const char *suite, const char *test)
{
    char id[LFG_CT_ID_MAX];

    _id_build(id, sizeof(id), file, suite, test);
    _fail_record_id(id);
}

void lfg_ct_self_rerun_reset(void)
{
    _fail_keys_clear();
}

/* Failure-summary hooks. Same shape, and same reason, as the rerun set's
 * above: expect-failures mode suppresses a genuine failure before it
 * ever reaches the accumulator, so the framework's own tests need a way
 * to feed it directly. Reaching the cap through real tests would
 * otherwise take LFG_CT_FAILGROUP_MAX + 1 distinct registrations. */
void lfg_ct_self_failgroup_note(const char *test, const char *message)
{
    _failgroup_record(test, message);
}

void lfg_ct_self_failgroup_reset(void)
{
    _failgroup_clear();
}

int lfg_ct_self_failgroup_count(void)
{
    return _failgroup_count;
}

int lfg_ct_self_failgroup_total(void)
{
    return _failgroup_total;
}

int lfg_ct_self_failgroup_dropped(void)
{
    return _failgroup_dropped;
}

int lfg_ct_self_failgroup_cap(void)
{
    return LFG_CT_FAILGROUP_MAX;
}

/* Rank-indexed accessors: @p rank walks the block's display order
 * (count descending, ties in first-occurrence order), not the table's
 * build order, so the ordering contract itself is what the self-tests
 * observe. */
static int
_failgroup_at(int rank)
{
    int order[LFG_CT_FAILGROUP_MAX];

    if (rank < 0 || rank >= _failgroup_count)
    {
        return -1;
    }
    _failgroup_order(order);
    return order[rank];
}

const char *lfg_ct_self_failgroup_name_at(int rank)
{
    int i = _failgroup_at(rank);

    return (i < 0) ? NULL : _failgroups[i].name;
}

const char *lfg_ct_self_failgroup_origin_at(int rank)
{
    int i = _failgroup_at(rank);

    return (i < 0) ? NULL : _failgroups[i].origin;
}

int lfg_ct_self_failgroup_count_at(int rank)
{
    int i = _failgroup_at(rank);

    return (i < 0) ? -1 : _failgroups[i].count;
}

/* Durations hooks. The accumulator is fed by real dispatches, so unlike
 * the two blocks above these exist less to bypass expect-failures mode
 * than to make the *ranking* observable without parsing stdout -- and to
 * let a cap test seed LFG_CT_DURATIONS_MAX + 1 entries without
 * registering that many tests. Note that the runner's own tests keep
 * appending to this table as they classify, so a self-test that inspects
 * it must reset first. */
void lfg_ct_self_durations_note(const char *suite, const char *test, double time_sec)
{
    _durations_record(suite, test, time_sec);
}

void lfg_ct_self_durations_reset(void)
{
    _durations_clear();
}

void lfg_ct_self_durations_set(int enabled, int limit)
{
    _durations_set = enabled ? 1 : 0;
    _durations_limit = limit;
}

int lfg_ct_self_durations_enabled(void)
{
    return _durations_set;
}

int lfg_ct_self_durations_limit(void)
{
    return _durations_limit;
}

int lfg_ct_self_durations_count(void)
{
    return _duration_count;
}

int lfg_ct_self_durations_dropped(void)
{
    return _duration_dropped;
}

int lfg_ct_self_durations_cap(void)
{
    return LFG_CT_DURATIONS_MAX;
}

int lfg_ct_self_durations_shown(void)
{
    if (!_durations_set || 0 == _duration_count)
    {
        return 0;
    }
    return _durations_shown();
}

/* Rank-indexed accessors, same contract as the failure summary's: @p rank
 * walks the block's display order (time descending, ties in
 * classification order), so the ordering itself is what is observed. */
static int
_durations_at(int rank)
{
    int order[LFG_CT_DURATIONS_MAX];

    if (rank < 0 || rank >= _duration_count)
    {
        return -1;
    }
    _durations_order(order);
    return order[rank];
}

const char *lfg_ct_self_durations_name_at(int rank)
{
    int i = _durations_at(rank);

    return (i < 0) ? NULL : _durations[i].test_name;
}

const char *lfg_ct_self_durations_suite_at(int rank)
{
    int i = _durations_at(rank);

    return (i < 0) ? NULL : _durations[i].suite_name;
}

double lfg_ct_self_durations_time_at(int rank)
{
    int i = _durations_at(rank);

    return (i < 0) ? -1.0 : _durations[i].time_sec;
}

int lfg_ct_self_rerun_recorded_count(void)
{
    return _fail_key_count;
}

int lfg_ct_self_rerun_unresolved_count(void)
{
    int unresolved = 0;
    int i;

    for (i = 0; i < _replay_key_count; i++)
    {
        if (!_replay_resolved[i])
        {
            unresolved++;
        }
    }
    return unresolved;
}

int lfg_ct_self_rerun_key_count(void)
{
    return _replay_key_count;
}

int lfg_ct_self_state_write(const char *path, unsigned seed)
{
    return _state_write(path, seed);
}

#endif /* LFG_CTEST_SELF_TEST */

int lfg_ct_assert_false_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str)
{
    _assertions_executed++;
    if (condition)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should be false", condition_str);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_true_impl(
        bool condition, char *filename, int line_no, const char *function, const char *condition_str)
{
    _assertions_executed++;
    if (!condition)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should be true", condition_str);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should equal %d", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int_not_equal_impl(
        int expected, int actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %d", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%u) should equal %u", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint_not_equal_impl(unsigned expected, unsigned actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %u", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint8_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%02X) should equal 0x%02X", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint8_not_equal_impl(uint8_t expected, uint8_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal 0x%02X", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint16_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%04X) should equal 0x%04X", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint16_not_equal_impl(uint16_t expected, uint16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal 0x%04X", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint32_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%08X) should equal 0x%08X", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint32_not_equal_impl(uint32_t expected, uint32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal 0x%08X", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_equal_impl(void *expected, void *actual, const char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%p) should equal %p", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_not_equal_impl(void *expected, void *actual, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %p", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_not_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not be NULL", actual_expr_str);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_ptr_null(
        void *actual, const char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should be NULL but is %p", actual_expr_str, actual);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int8_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should equal %d", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int8_not_equal_impl(
        int8_t expected, int8_t actual, char *filename, int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %d", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int16_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should equal %d", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int16_not_equal_impl(int16_t expected, int16_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %d", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int32_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should equal %d", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int32_not_equal_impl(int32_t expected, int32_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %d", actual_expr_str, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int64_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no, const char *function,
        const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%lld) should equal %lld", actual_expr_str, (long long)actual,
                (long long)expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_int64_not_equal_impl(int64_t expected, int64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal %lld", actual_expr_str, (long long)expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint64_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected != actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%016llX) should equal 0x%016llX", actual_expr_str,
                (unsigned long long)actual, (unsigned long long)expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_uint64_not_equal_impl(uint64_t expected, uint64_t actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (expected == actual)
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal 0x%016llX", actual_expr_str,
                (unsigned long long)expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_str_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            _lfg_ct_fail(filename, line_no, function, "%s (%p) should equal %p (NULL mismatch)", actual_expr_str,
                    (void *)actual, (void *)expected);
            return -1;
        }
    }
    else if (strcmp(expected, actual) != 0)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (\"%s\") should equal \"%s\"", actual_expr_str, actual, expected);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_str_not_equal_impl(const char *expected, const char *actual, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) || (expected != NULL && actual != NULL && strcmp(expected, actual) == 0))
    {
        _lfg_ct_fail(filename, line_no, function, "%s should not equal \"%s\"", actual_expr_str,
                expected ? expected : "(null)");
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_strn_equal_impl(const char *expected, const char *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            _lfg_ct_fail(filename, line_no, function, "%s (%p) should equal %p (NULL mismatch)", actual_expr_str,
                    (void *)actual, (void *)expected);
            return -1;
        }
    }
    else if (strncmp(expected, actual, n) != 0)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (first %zu chars) does not match expected", actual_expr_str, n);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_mem_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if (NULL == expected || NULL == actual)
    {
        if (expected != actual)
        {
            _lfg_ct_fail(filename, line_no, function, "%s (%p) should equal %p (NULL mismatch)", actual_expr_str,
                    actual, expected);
            return -1;
        }
    }
    else if (memcmp(expected, actual, n) != 0)
    {
        _lfg_ct_fail(filename, line_no, function, "%s memory (%zu bytes) does not match expected", actual_expr_str, n);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_mem_not_equal_impl(const void *expected, const void *actual, size_t n, char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    if ((NULL == expected && NULL == actual) ||
            (expected != NULL && actual != NULL && memcmp(expected, actual, n) == 0))
    {
        _lfg_ct_fail(filename, line_no, function, "%s memory (%zu bytes) should not match", actual_expr_str, n);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_greater_than_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a > b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should be > %s (%d)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_less_than_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a < b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should be < %s (%d)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_greater_or_equal_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a >= b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should be >= %s (%d)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_less_or_equal_impl(
        int a, int b, char *filename, int line_no, const char *function, const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a <= b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should be <= %s (%d)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_in_range_impl(
        int val, int min, int max, char *filename, int line_no, const char *function, const char *val_expr_str)
{
    _assertions_executed++;
    if (!(val >= min && val <= max))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%d) should be in range [%d, %d]", val_expr_str, val, min, max);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bit_set_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num)
{
    _assertions_executed++;
    if (!(val & (1u << bit)))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%08X) should have bit %u set", val_expr_str, val, bit_num);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bit_clear_impl(unsigned val, unsigned bit, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned bit_num)
{
    _assertions_executed++;
    if (val & (1u << bit))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (0x%08X) should have bit %u clear", val_expr_str, val, bit_num);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bits_set_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val)
{
    _assertions_executed++;
    if ((val & mask) != mask)
    {
        _lfg_ct_fail(
                filename, line_no, function, "%s (0x%08X) should have bits 0x%08X set", val_expr_str, val, mask_val);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_bits_clear_impl(unsigned val, unsigned mask, char *filename, int line_no, const char *function,
        const char *val_expr_str, unsigned mask_val)
{
    _assertions_executed++;
    if (val & mask)
    {
        _lfg_ct_fail(
                filename, line_no, function, "%s (0x%08X) should have bits 0x%08X clear", val_expr_str, val, mask_val);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_fail_impl(char *filename, int line_no, const char *function, const char *message)
{
    _assertions_executed++;
    _lfg_ct_fail(filename, line_no, function, "%s", message ? message : "Explicit failure");
    return -1;
}

/*============================================================================
 *  32-bit Float Assertions (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_FLOAT

int lfg_ct_assert_float_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    float diff = fabsf(expected - actual);
    if (diff > epsilon)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should equal %.6g (diff=%.6g, eps=%.6g)", actual_expr_str,
                actual, expected, diff, epsilon);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_not_equal_impl(float expected, float actual, float epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    float diff = fabsf(expected - actual);
    if (diff <= epsilon)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should not equal %.6g (diff=%.6g, eps=%.6g)",
                actual_expr_str, actual, expected, diff, epsilon);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_greater_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a > b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should be > %s (%.6g)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_less_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a < b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should be < %s (%.6g)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_ge_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a >= b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should be >= %s (%.6g)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_le_impl(float a, float b, const char *filename, int line_no, const char *function,
        const char *a_expr_str, const char *b_expr_str)
{
    _assertions_executed++;
    if (!(a <= b))
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.6g) should be <= %s (%.6g)", a_expr_str, a, b_expr_str, b);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_float_in_range_impl(float val, float min, float max, const char *filename, int line_no,
        const char *function, const char *val_expr_str)
{
    _assertions_executed++;
    if (!(val >= min && val <= max))
    {
        _lfg_ct_fail(
                filename, line_no, function, "%s (%.6g) should be in range [%.6g, %.6g]", val_expr_str, val, min, max);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

#endif /* LFG_CTEST_HAS_FLOAT */

/*============================================================================
 *  64-bit Double Assertions (optional)
 *==========================================================================*/

#ifdef LFG_CTEST_HAS_DOUBLE

int lfg_ct_assert_double_equal_impl(double expected, double actual, double epsilon, const char *filename, int line_no,
        const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    double diff = fabs(expected - actual);
    if (diff > epsilon)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.10g) should equal %.10g (diff=%.10g, eps=%.10g)",
                actual_expr_str, actual, expected, diff, epsilon);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

int lfg_ct_assert_double_not_equal_impl(double expected, double actual, double epsilon, const char *filename,
        int line_no, const char *function, const char *actual_expr_str)
{
    _assertions_executed++;
    double diff = fabs(expected - actual);
    if (diff <= epsilon)
    {
        _lfg_ct_fail(filename, line_no, function, "%s (%.10g) should not equal %.10g (diff=%.10g, eps=%.10g)",
                actual_expr_str, actual, expected, diff, epsilon);
        return -1;
    }
    RECORD_PASS();
    return 0;
}

#endif /* LFG_CTEST_HAS_DOUBLE */

/*============================================================================
 *  Private Functions
 *==========================================================================*/
