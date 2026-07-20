/**
 * @file
 * @brief       lfg-ctest unit testing API.
 */

/*============================================================================
 *  Includes
 *==========================================================================*/

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
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

/* --verbose: stream a START line before each test body and an outcome
 * line (PASS / FAIL / SKIP / XFAIL / XPASS) with elapsed milliseconds
 * after classification. Implemented via the built-in verbose reporter
 * above so the print path goes through the same single reporter
 * contract every other event observer uses. */
static int _verbose_mode = 0;

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

static int _list_mode = 0;
static const char *_filter_globs[LFG_CT_FILTER_MAX];
static int _filter_glob_count = 0;
static const char *_exclude_globs[LFG_CT_FILTER_MAX];
static int _exclude_glob_count = 0;
/* Depth of nested suites whose name matched the filter. Tests inside such
 * a suite inherit the filter-pass (so "--filter suite_foo*" runs every test
 * the suite holds without each test having to match individually). */
static int _filter_inherited_depth = 0;

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

void lfg_ct_start(void)
{
    unsigned rand_seed = time(NULL) % 1000;
    /* List mode wants stdout to be a clean newline-separated list of names;
     * skip the banner and the seed announcement. srand still runs so any
     * deterministic-by-seed test behavior stays consistent if the user
     * combines --list with other operations. */
    if (!_list_mode)
    {
        printf("*** begin unit test\r\n");
        printf("*** random seed is %u\r\n", rand_seed);
    }
    srand(rand_seed);
}

void lfg_ct_end(void)
{
}

/* Match @p name against any glob in @p globs (length @p count) using
 * fnmatch(3) shell-glob semantics. */
static int
_glob_list_matches(const char *name, const char *const *globs, int count)
{
    int i;

    if (NULL == name)
    {
        return 0;
    }
    for (i = 0; i < count; i++)
    {
        if (0 == fnmatch(globs[i], name, 0))
        {
            return 1;
        }
    }
    return 0;
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
    if (_verbose_mode)
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
    _verbose_mode = 0;
    _reporter_activate();
}

static void
_filter_print_usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [options]\r\n"
            "  --list                   List registered test/suite names and exit 0\r\n"
            "  --filter <glob>          Run only entries whose name matches <glob>\r\n"
            "  --filter-exclude <glob>  Skip entries whose name matches <glob>\r\n"
            "  --strict-xpass           Treat any xpass outcome as a failure (exit non-zero)\r\n"
            "  -v, --verbose            Stream per-test START / outcome lines with elapsed ms\r\n"
            "Globs use shell-style syntax (*, ?, [...]) via fnmatch(3).\r\n"
            "--filter and --filter-exclude may be repeated; exclude wins on overlap.\r\n",
            progname ? progname : "test");
}

int
lfg_ct_parse_args(int argc, char *argv[])
{
    const char *progname;
    int i;

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
        if (0 == strcmp(a, "-v") || 0 == strcmp(a, "--verbose"))
        {
            _verbose_mode = 1;
            _reporter_activate();
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
    return 0;
}

int
lfg_ct_is_list_mode(void)
{
    return _list_mode ? 1 : 0;
}

int
lfg_ct_is_verbose(void)
{
    return _verbose_mode ? 1 : 0;
}

/* Pure name predicate -- ignores list mode (which suppresses execution
 * regardless of filter state). */
static int
_filter_admits(const char *name)
{
    if (_exclude_glob_count > 0 && _glob_list_matches(name, _exclude_globs, _exclude_glob_count))
    {
        return 0;
    }
    if (0 == _filter_glob_count || _filter_inherited_depth > 0)
    {
        return 1;
    }
    return _glob_list_matches(name, _filter_globs, _filter_glob_count);
}

int
lfg_ct_name_runs(const char *name)
{
    if (_list_mode)
    {
        return 0;
    }
    return _filter_admits(name);
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
#else
void lfg_ct_suite_impl(void (*fn)(void), const char *name)
#endif
{
    int suite_assertions_failed_before;
    int inherited_pushed = 0;
    int saved_skip_active;
    const char *saved_suite_name;

    if (_list_mode)
    {
        /* Print the suite name and recurse into the body so contained
         * lfg_ct_test() calls can list themselves. */
        printf("%s\r\n", name);
        if (fn)
        {
            fn();
        }
        return;
    }

    /* Exclude is checked first and is decisive: a matched suite is skipped
     * entirely, no descent. Filter is checked second; a non-match still
     * descends so inner tests can be evaluated individually. A suite that
     * matches the filter propagates the pass to every descendant. */
    if (_exclude_glob_count > 0 && _glob_list_matches(name, _exclude_globs, _exclude_glob_count))
    {
        return;
    }
    if (_filter_glob_count > 0 && 0 == _filter_inherited_depth
            && _glob_list_matches(name, _filter_globs, _filter_glob_count))
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

    if (_current_suite_failures > 0 || _assertions_failed > suite_assertions_failed_before)
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
#else
void lfg_ct_test_impl(void (*fn)(void), const char *name)
#endif
{
    if (_list_mode)
    {
        printf("%s\r\n", name);
        return;
    }
    if (!_filter_admits(name))
    {
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
        return;
    }
#ifdef LFG_CT_COMPAT_3ARG
    _lfg_ct_test_impl_inproc(setup, fn, teardown, name);
#else
    _lfg_ct_test_impl_inproc(fn, name);
#endif
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
    clock_t time_start;
    double elapsed;

    if (_list_mode)
    {
        printf("%s\r\n", name);
        return;
    }
    if (!_filter_admits(name))
    {
        return;
    }

    _tests_executed++;
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
    time_start = clock();

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

    elapsed = (double)(clock() - time_start) / (double)CLOCKS_PER_SEC;
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
            printf("*** test SKIP: %s: %s\r\n", name, _current_skip_reason ? _current_skip_reason : "(no reason)");
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
                printf("*** test XFAIL: %s: %s\r\n", name,
                        _current_xfail_reason ? _current_xfail_reason : "(no reason)");
                outcome = LFG_CT_XFAIL;
                message = _current_xfail_reason;
            }
            else
            {
                _tests_xpassed++;
                printf("*** test XPASS: %s: %s\r\n", name,
                        _current_xfail_reason ? _current_xfail_reason : "(no reason)");
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
        }
        else
        {
            _tests_passed++;
            outcome = LFG_CT_PASSED;
            message = NULL;
        }

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

    /* Reset per-test state so a nested test_impl call doesn't leave its
     * disposition/xfail flags around for the calling test's own
     * classification at the end of the outer lifecycle. */
    _current_test_failures = 0;
    _current_disposition = LFG_CT_DISP_NORMAL;
    _current_skip_reason = NULL;
    _current_xfail_set = 0;
    _current_xfail_reason = NULL;

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

    printf("*** Executed %d assertions in %d tests. "
           "Failures: %d, Skipped: %d, XFail: %d, XPass: %d\r\n"
           "*** Testing complete. Result: %s\r\n",
            _assertions_executed, _tests_executed, _tests_failed, _tests_skipped, _tests_xfailed, _tests_xpassed,
            verdict);

    /* Reporter's run-complete hook. A buffering reporter (e.g. the
     * contrib JUnit emitter) flushes its accumulated state here. A
     * failure inside the hook is the reporter's concern -- the
     * runner's exit-code contract is unchanged by reporter side
     * effects. */
    if (_reporter && _reporter->on_run_complete)
    {
        _reporter->on_run_complete(_reporter->userdata);
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
 * touching @c _user_reporter or @c _verbose_mode, so the child's
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
            if (print_outcome_line)
            {
                printf("*** test FAILURE: %s\r\n", name);
                if (message)
                {
                    printf("*** %s\r\n", message);
                }
            }
            break;
        case LFG_CT_SKIPPED:
            _tests_skipped++;
            if (print_outcome_line)
            {
                printf("*** test SKIP: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
        case LFG_CT_XFAIL:
            _tests_xfailed++;
            if (print_outcome_line)
            {
                printf("*** test XFAIL: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
        case LFG_CT_XPASS:
            _tests_xpassed++;
            if (print_outcome_line)
            {
                printf("*** test XPASS: %s: %s\r\n", name, message ? message : "(no reason)");
            }
            break;
    }

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

int lfg_ct_self_return_code(void)
{
    return lfg_ct_return();
}

const char *lfg_ct_self_last_xfail_reason(void)
{
    return _last_classified_xfail_reason;
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
