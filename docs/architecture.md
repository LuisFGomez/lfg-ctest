# architecture.md — lfg-ctest internals

This doc describes how the framework itself is put together. For the public
API, see [api.md](api.md).

## Two concerns, one static library

CMake produces a single archive `lfg-ctest` from:

- `lfg-ctest.c` — test runner and assertion implementations.
- `lfg-ctest-mock.c` — mock runtime (param-action chain, reset registry).

Consumers include `lfg-ctest.h` for assertions/runner and optionally
`lfg-ctest-mock.h` for mocking. The two headers are independent; you can use
the runner without ever pulling in the mock macros.

A `contrib/` subtree holds optional packages that build against the core
library but never become part of it -- the contract is "pluggable reporter
plus whatever the package wants". `contrib/junit-xml/` is the first such
package; see [api.md — Reporter callback](api.md#reporter-callback) for the
hook it consumes.

## Core runner (`lfg-ctest.c`)

State lives in a small set of file-scope statics (pass/fail counters, current
test name, etc.). All public entry points (`lfg_ct_start`, `lfg_ct_test`,
`lfg_ct_suite`, `lfg_ct_print_summary`, `lfg_ct_return`) mutate this state.
There is no reentrancy guarantee — one test run at a time.

### Test dispatch and the body-owned lifecycle

The framework binds **no** setup/teardown at registration time. `lfg_ct_test`
and `lfg_ct_suite` take the body only; setup and teardown are plain functions
the body calls itself (the [blessed convention](api.md#setup-and-teardown)).
There is no `_lfg_ct_run_lifecycle` helper — that machinery was removed with
the registration-time signature. `lfg_ct_test_impl` is just an isolation shim:
in-process it tail-calls `_lfg_ct_test_impl_inproc(fn, name)`; under
`LFG_CT_ISOLATE_FORK` it forks and the child re-enters the same in-process
entry point (see [Isolation modes](api.md#isolation-modes)).

`_lfg_ct_test_impl_inproc` runs the body inside a **single** skip-aware
boundary and classifies the outcome afterwards:

1. Snapshot per-test state (`_current_test_failures`,
   `_current_disposition`, `_current_skip_reason`, `_current_xfail_set`,
   `_current_xfail_reason`, and the
   failure-message slot — an inline buffer plus an optional heap block
   for over-long messages, *moved* rather than copied so a nested
   lifecycle never frees the outer test's text), then zero the live
   state so this level starts clean. Also snapshot the outer
   `_skip_env` + `_skip_env_active` (so a nested `lfg_ct_test_impl` — the
   self-test pattern that drives mock tests through the real runner —
   can't strand the outer body with an overwritten `jmp_buf`).
2. Set `_skip_env_active = 1`, `setjmp(_skip_env)`, and call `body`. Setup and
   teardown are the body's own concern, so `lfg_ct_skip` from anywhere in the
   body's dynamic extent (including a setup helper it called) `longjmp`s
   straight back to this one boundary — which is why "teardown before skip" is
   load-bearing: a trailing in-body `teardown()` is unwound past.
3. Restore the saved `_skip_env` + active flag, classify (PASS / FAIL /
   SKIP / XFAIL / XPASS), then restore the rest of the snapshotted
   per-test state.

Assertion failures need no unwinding of their own: assertion impls return `0`
on pass and non-zero (e.g. `-1`) on failure and bump a counter; they don't
abort, so a trailing `teardown()` stays reachable after a soft body failure
without any `setjmp`/`longjmp`. A body that wants to detect a failure *inside*
its own setup — to skip or bail before the real work — reads the promoted
`lfg_ct_failure_count()` accessor (snapshot-and-compare); the runner keeps that
same `_assertions_failed` counter internally.

`lfg_ct_suite_impl` installs **no** skip boundary; it explicitly forces
`_skip_env_active = 0` across the suite body so a stray `lfg_ct_skip` from
suite-level code warns to stderr and no-ops rather than unwinding (suite-level
skip is intentionally out of scope for now).

Every assertion macro in `lfg-ctest.h` ultimately routes to an internal
failure path that:

1. Formats a diagnostic (including `__func__` if available — see the
   `LFG_CTEST_NO_FUNC` / `__FUNCTION__` fallback chain in `README.md`).
2. Writes to stderr **unless** expect-failures mode is active.
3. Increments a counter.

## Self-test mode (`LFG_CTEST_SELF_TEST`)

The CMakeLists sets `LFG_CTEST_SELF_TEST=1` on the library and both self-test
binaries when this repo is the top-level CMake source
(`CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR`). When built as a
subproject, self-tests and this flag are off, and the extra internal API stays
hidden.

The flag gates:

- `lfg_ct_expect_failures_begin()` / `lfg_ct_expect_failures_end()` —
  declared in `lfg-ctest.h` under `#ifdef LFG_CTEST_SELF_TEST`.
- The `_expect_failures_mode` static in `lfg-ctest.c` and the branches in
  `_lfg_ct_fail`-path macros that consult it.

### Expect-failures mode

Used to verify that assertion macros *actually fail* without polluting test
output. Pattern:

```c
int expected_failures = 6;
int actual_failures;

lfg_ct_expect_failures_begin();
ASSERT_EQ(1, 2);           /* would normally fail loudly */
ASSERT_TRUE(0);            /* counted silently */
/* ... */
actual_failures = lfg_ct_expect_failures_end();
ASSERT_INT_EQUAL(expected_failures, actual_failures);
```

While the mode is active, stderr output from failing assertions is suppressed
and the pass/fail counters are not incremented at the top level — only the
local failure counter increments. `_end()` returns the count and restores
normal behavior.

Every "does assertion X fail correctly?" self-test in `test-unified.c` uses
this pattern.

## Skip / xfail dispositions

Per-test disposition is tracked in three statics in `lfg-ctest.c`:

- `_current_disposition` (`NORMAL` / `SKIPPED`),
- `_current_skip_reason`,
- `_current_xfail_set` + `_current_xfail_reason`.

`lfg_ct_skip(reason)` sets the disposition, records the reason, and
`longjmp`s through `_skip_env` to the single test-level boundary
`_lfg_ct_test_impl_inproc` installs around the body. `_skip_env` is one
static `jmp_buf`; because setup and teardown are now body-owned calls (not
separate framework phases), that single boundary spans the whole body's
dynamic extent — a `lfg_ct_skip` from a setup helper the body called unwinds
to the same place, past any trailing in-body `teardown()`. The impl sets
`_skip_env_active = 1` only while the boundary is live and the skip impl
no-ops with a stderr warning when called outside that window (from `main`,
from any suite-level code — `lfg_ct_suite_impl` explicitly forces
`_skip_env_active = 0` for the duration of the suite — or between tests).
`_lfg_ct_test_impl_inproc` snapshots `_skip_env` + `_skip_env_active` on entry
and restores both on exit so a nested `lfg_ct_test_impl` (the self-test
pattern that drives mock tests through the real runner) cannot strand an
in-progress outer body with an overwritten buffer or a cleared active flag —
an outer test body can call `lfg_ct_skip` after a nested call returns and
still unwind correctly.

`lfg_ct_xfail(reason)` does not unwind; it sets `_current_xfail_set`
and overwrites `_current_xfail_reason` (last call wins). The body runs
to completion and the test's classification at the end of
`lfg_ct_test_impl` reads the flags to decide which bucket the test
falls into:

| Final state | Bucket | Counter bumped |
|-------------|--------|----------------|
| `_current_disposition == SKIPPED` | SKIP | `_tests_skipped` |
| `_current_xfail_set && _current_test_failures > 0` | XFAIL | `_tests_xfailed` |
| `_current_xfail_set && _current_test_failures == 0` | XPASS | `_tests_xpassed` |
| `_current_test_failures > 0` (no xfail) | FAIL | `_tests_failed` |
| otherwise | PASS | `_tests_passed` |

For SKIP and XFAIL the classification also subtracts
`_current_test_failures` from the global `_assertions_failed` -- the
absorbed failures must not trip the surrounding suite-failure detector
(which compares `_assertions_failed` snapshots around the suite body).
After classification the per-test state is restored to the values
snapshotted on entry (step 1 above), so a *nested* `lfg_ct_test_impl`
call (the pattern the framework's own self-tests use to drive mock tests
through the real runner) neither leaks its own disposition and failure
count into the calling test's classification nor erases the caller's.
Each level therefore absorbs only the failures it recorded itself.
`_last_classified_xfail_reason` is deliberately excluded from the
save/restore: it is last-call-wins across nesting so a self-test can
observe the most recently classified test's reason.

`--strict-xpass` flips the exit-code calculation in `lfg_ct_return`:
when set and `_tests_xpassed > 0` and no real failures occurred, return
`-1` instead of `0`. The summary line shows `XPass: N` regardless of
the flag; only the exit code changes.

## Fork-per-test isolation (`lfg-ctest-fork.c`)

Optional runtime support for `LFG_CT_ISOLATE_FORK`, selected via
`lfg_ct_set_isolation(...)`. The implementation lives in its own
translation unit so the platform gate (`__unix__` / `__APPLE__`) and
the compile-time opt-out (`LFG_CT_DISABLE_FORK`) stay contained --
the rest of the framework has zero `#ifdef`s for this feature.

### Dispatch shape

`lfg_ct_test_impl` is now a thin shim. After the list-mode + filter
checks (which are name-only and have to happen in the parent
regardless of isolation), it delegates:

- `LFG_CT_ISOLATE_NONE` -> `_lfg_ct_test_impl_inproc`, the original
  in-process path (now renamed). Unchanged behavior, no extra cost
  for default-mode consumers.
- `LFG_CT_ISOLATE_FORK` -> `_lfg_ct_fork_run_test`, in `lfg-ctest-fork.c`.

The fork runner:

1. Opens a pipe and `fork(2)`s.
2. **Child**: swaps the user's reporter for a capture reporter,
   snapshots the assertion counters, calls `_lfg_ct_test_impl_inproc`
   directly, diffs the counters back out, writes a fixed-layout
   `_fork_header_t` followed by `header.msg_len` message bytes to the
   pipe, and `_exit`s. Counter deltas are shipped because the child's
   increments live in its own address space and don't survive `_exit`.
   Length-prefixing is what lets an assertion message of any size
   cross; a *complete header* remains the "payload present" signal, so
   the "no payload means the child crashed" test is unchanged.
3. **Parent**: drains the pipe *while* waiting, never after — a message
   larger than the pipe buffer blocks the child in `write(2)`, so
   reaping first would deadlock the pair. With a timeout configured
   the drain uses non-blocking reads, then a 5ms `waitpid(WNOHANG)`
   poll, both bounded by a single `CLOCK_MONOTONIC` deadline that
   escalates to `kill(SIGKILL)` on expiry; without one it blocks on
   reads to EOF and then reaps. Note that EOF is *not* child exit —
   the child closes the write end before its final `fflush(NULL)` —
   so the reap is polled under the same deadline rather than blocked
   on, or a child wedged in that flush would escape the timeout.
   Decodes `WIFSIGNALED` / `WIFEXITED` and projects the outcome via
   `_lfg_ct_record_external`.

Both sides keep a stack fast path for short messages and allocate
only past it. Every degraded path (allocation failure on either side,
a child that died mid-write) appends an explicit
`... [truncated N bytes]` marker rather than clipping silently.

Stdout/stderr fd inheritance is the default `fork(2)` behavior, so
the child's per-test banner reaches the user without any capture
wiring. The parent only emits the banner itself on signal / timeout /
"exited without payload" paths, where the child died before its own
print landed.

That inheritance is on the *descriptor*, not on the stdio buffer, so
it depends on a two-sided `fflush(NULL)`: the parent flushes before
`fork(2)`, the child flushes before `_exit`. Both halves are load
bearing. Without the child's, everything it buffered is discarded --
`_exit` deliberately skips stdio cleanup, so on a fully-buffered
stdout (any pipe or file, i.e. every CI run) the per-test detail
output vanishes while the same run on a TTY is complete, because line
buffering flushes at each newline. Without the parent's, the child
inherits a copy of the parent's pending buffer and re-emits it on its
own flush, duplicating parent output once per forked test.

The signal- and timeout-killed paths never reach the child's flush, so
detail the body printed before dying is still lost under redirection
while a TTY run shows it. That gap is inherent to `SIGKILL` and to
crashing, not something the two-sided flush can close.

### Bridge surface (lfg-ctest.c <-> lfg-ctest-fork.c)

Three internal symbols (extern-declared in the fork TU, not in any
public header):

| Symbol | Direction | Role |
|--------|-----------|------|
| `_lfg_ct_test_impl_inproc` | `lfg-ctest.c` exports | In-process dispatch the child re-enters after the reporter swap. |
| `_lfg_ct_counter_snapshot` | `lfg-ctest.c` exports | Read the assertion counters; child uses it to compute deltas. |
| `_lfg_ct_get_reporter` | `lfg-ctest.c` exports | Save the active reporter so the child can restore after the swap. |
| `_lfg_ct_record_external` | `lfg-ctest.c` exports | Project a child's classified outcome onto parent-side counters + reporter. Mirrors the post-classification block of the in-process path. |
| `_lfg_ct_fork_available` | `lfg-ctest-fork.c` exports | `lfg_ct_set_isolation` calls this to decide whether to accept `LFG_CT_ISOLATE_FORK`. The disabled-build / non-Unix stub returns 0. |
| `_lfg_ct_fork_run_test` | `lfg-ctest-fork.c` exports | The dispatcher itself. Disabled-build / non-Unix stub returns -1 with no fork-related code linked. |

### Compile-time opt-out (`LFG_CT_DISABLE_FORK`)

CMake option `LFG_CTEST_ENABLE_FORK` (default `ON`). When `OFF`, the
library is built with `LFG_CT_DISABLE_FORK=1` on the
`lfg-ctest-fork.c` TU; the file compiles to two stubs and the linker
drops every reference to `fork` / `waitpid` / `kill` / `usleep`. The
`LFG_CT_ISOLATE_FORK` enum value is unconditional in the header so
consumer code that references it still compiles.

`test-fork-disabled` rebuilds the framework sources directly with
`LFG_CT_DISABLE_FORK=1` to exercise the opt-out path -- mirrors the
self-contained pattern that `test-amalg` uses. `nm -u` on the
resulting binary shows zero fork-family imports.

### Self-test interaction (`_expect_failures_mode`)

`_lfg_ct_record_external` consults `_expect_failures_mode` when the
incoming outcome is `LFG_CT_FAILED`: in expect-failures mode, the
test-level counter bumps (`_tests_failed`, `_current_suite_failures`,
`_assertions_failed`) are redirected to `_expected_failures_count`
just like assertion-level failures already are. This lets
`test-fork.c` drive an inner fork-mode test that crashes / times out
/ aborts -- and verify the reporter receives `FAILED` -- without
polluting the global counters and making the binary exit non-zero.
The extension is consistent with the existing semantic of the flag:
"intentional failures during this block do not affect the run's
verdict."

## Verbose mode (built-in reporter)

`-v` / `--verbose` is the first built-in reporter built on the
[Reporter callback](api.md#reporter-callback) contract. Implementation
lives in `lfg-ctest.c` (no separate TU; the verbose reporter is small
and the core was the natural seat for the reporter-chain plumbing).

Key statics:

- `_verbose_mode` -- the parsed-flag bit, toggled by
  `lfg_ct_parse_args` when it sees `-v` / `--verbose` and cleared by
  `_filter_state_reset` along with the other parsed-flag state.
- `_user_reporter` -- the slot a consumer set via
  `lfg_ct_set_reporter`. The user-facing pointer; never the active
  fire target when verbose mode is on.
- `_reporter` -- the active fire target. Equal to `_user_reporter`
  when verbose is off, equal to the built-in `_verbose_reporter`
  struct when verbose is on.
- `_reporter_activate()` -- recomputes `_reporter` from
  `_verbose_mode` and `_user_reporter`. Called wherever either input
  changes.

The built-in reporter's callbacks (`_verbose_on_test_start`,
`_verbose_on_record`, `_verbose_on_run_complete`) print the
streamed banners on `stdout`, `fflush` to keep them visible
before a slow body completes and before any `fork(2)`, then chain
into `_user_reporter`'s callbacks so a downstream consumer reporter
(e.g. the JUnit-XML emitter) still observes every event. The fan-out
keeps a single print path through the reporter contract -- there is
no parallel "verbose output" code path elsewhere in the runner.

### Fire sites

`on_test_start` fires from `lfg_ct_test_impl` (the parent-side
dispatcher) **after** the list-mode / filter / exclude gates and
**before** delegation to either the in-process or the fork path.
Single fire site per admitted test:

1. The list-mode and filter checks gate-keep first; an excluded test
   sees nothing.
2. `_reporter->on_test_start` fires once with the current suite name
   (`_current_suite_name`) and the test name.
3. Isolation dispatch follows: `_lfg_ct_test_impl_inproc` directly,
   or `_lfg_ct_fork_run_test` for fork mode.

`_lfg_ct_test_impl_inproc` itself does **not** fire `on_test_start`.
The fork TU re-enters that helper inside the child, but the parent
already fired the start callback before forking, so a child-side
re-fire would double-up. Direct callers of `_lfg_ct_test_impl_inproc`
are limited to that re-entry path; the public dispatch flow always
goes through `lfg_ct_test_impl`.

### Fork-mode interaction

Under `LFG_CT_ISOLATE_FORK`:

- Parent fires `on_test_start` via the verbose chain (prints banner,
  delegates to user reporter).
- Parent `fork(2)`s.
- Child swaps `_reporter` to the fork TU's capture reporter via the
  internal bridge `_lfg_ct_set_active_reporter_direct`. This bypasses
  the verbose chain (no banner printed from the child, no chain into
  `_user_reporter`); the child's classification calls
  `_child_capture` directly to fill the payload. `record->message` is
  borrowed for the callback only, so the child copies it.
- Child writes header + message, `_exit`s.
- Parent decodes the payload and calls `_lfg_ct_record_external`,
  which fires `on_record` on `_reporter` -- still the verbose
  reporter, so the parent prints the outcome banner and chains
  through to `_user_reporter`.

Net effect: each fork-isolated test produces exactly one start
banner and one outcome banner in the parent, no interleaving with
the child's stdout, and the user reporter receives one
`on_test_start` + one `on_record` per test just like the in-process
path. The direct-set bridge is the minimum surface needed to keep
the verbose chain quiet in the child; the rest of the fork TU is
unchanged.

## Mock system (`lfg-ctest-mock.[ch]`)

### Macro fanout

`DECLARE_MOCK_` / `DEFINE_MOCK_` come in these shapes:

- `V_V`, `V_1` … `V_9` — void return, N parameters (0–9).
- `R_V`, `R_1` … `R_9` — value return, N parameters (0–9).
- `V_V_S`, `V_1_S` … `V_3_S` — void return, struct-by-value parameter.
- `R_V_S`, `R_1_S` … `R_6_S` — value return, struct-by-value parameter.

Every `DEFINE_MOCK_*` generates the same suite of symbols for the target
function `foo`:

| Symbol | Purpose |
|--------|---------|
| `foo__mock(...)` | The mock itself. Consumer may `#define foo foo__mock` via an opt-in `*_MOCK_REPLACE` switch. |
| `foo__mock_reset()` | Zero call count, free `__param_actions`, null the callback. |
| `foo__call_count` | `size_t`, incremented after each call. |
| `foo__param_history[MOCK_CALL_STORAGE_MAX]` | Array of per-call parameter structs (`p0`..`pN`). |
| `foo__return_queue[MOCK_CALL_STORAGE_MAX]` | Return value for each call (R_* only). |
| `foo__param_actions` | Head of the `mock_param_action_t` chain applied during calls (non-`_S` only). |
| `foo__callback` | Optional void callback invoked after param capture + return + actions. |
| `foo__callback_t` | Typedef for the callback. |
| `foo_params` | Typedef for the captured-params struct. |

`MOCK_CALL_STORAGE_MAX` defaults to 32 and can be overridden per translation
unit by `#define`-ing it before including `lfg-ctest-mock.h`. Overflow asserts.

### Why `_S` exists

Standard (non-`_S`) mocks cast each captured parameter through
`(void *)(size_t)` into the param-history union. That coerces any scalar or
pointer, but won't compile for structs passed by value. The `_S` variants use
the actual parameter type in the history struct and skip the `(void*)(size_t)`
layer — at the cost of dropping the `__param_actions` mechanism
(`mock_param_mem_*` / `mock_param_str_*`), because those operate on pointer
parameters.

Rule of thumb: reach for `_S` **only** when a parameter (not the return type)
is a struct passed by value. Struct return types work fine with plain `R_N`.

### Param actions (`mock_param_*`)

A singly-linked list of `struct _mock_param_action` nodes, built by the
`mock_param_mem_read / _mem_write / _str_read / _str_write` constructors.
Each node specifies: direction (read/write, byte/string), call index,
parameter index, buffer, and size. `_str_*` variants use `snprintf` and stop
at the null terminator — prefer them for `const char *` to avoid over-reading
short string literals (ASAN redzone).

On every mock call, the mock body walks the chain for that `call_index` and
parameter index and executes matching actions. `mock_param_destroy()` frees
the chain; `__mock_reset()` calls it automatically.

Chains are appended at the tail so user-visible order (read first call's
param 1, then write its param 2, …) matches execution order.

### Reset registry and `mock_reset_all()`

`lfg-ctest-mock.c` holds a flat, file-scope array:

```c
static void (*_mock_reset_registry[MOCK_REGISTRY_MAX])(void);
static size_t _mock_reset_registry_count;
```

`MOCK_REGISTRY_MAX` defaults to 64. Registration is **lazy**: the generated
mock body calls `_MOCK_REGISTER(foo)` (expands to
`_mock_register_reset(foo__mock_reset)`) on each call, and
`_mock_register_reset` deduplicates so repeated calls are cheap. A mock that
is never called is never registered.

`mock_reset_all()` iterates the registry, invokes each reset function, then
zeros the count — so mocks re-register on their next call. This lets a
teardown drop every known mock's state without maintaining an explicit list.

### Consumer cleanup hooks (`mock_register_cleanup`)

`_mock_register_reset` is private — the macro chain feeds it `__mock_reset`
thunks and nothing else. Consumers reach the same registry through the public
wrapper `mock_register_cleanup(void (*)(void))`, declared in
`lfg-ctest-mock.h`, which forwards directly into `_mock_register_reset`. The
two share dedup, the `MOCK_REGISTRY_MAX` budget, and the registry walk.

The intended use is a mock TU whose mock owns nontrivial heap state that the
framework can't reach — return queues holding allocated blobs, for example.
The TU registers a `free_unconsumed` walker once at TU init (constructor
attribute, or piggybacked on the consuming mock's first call) and
`mock_reset_all()` then covers the framework-generated thunks **and** the
consumer hook in one call. Single-function teardown contract; no two-step
ritual for test authors to forget.

Because `mock_reset_all()` clears the registry, cleanup hooks must
re-register after each reset cycle — same lifecycle as the auto-registered
`__mock_reset` thunks. Ordering between the two classes is unspecified.

## Amalgamation (`tools/amalgamate.c`)

The framework ships in two forms: the split sources (default) and a generated
single-header at `dist/lfg-ctest.h` produced by the `amalgamate` CMake target.
The single-header path exists for consumers who want drop-in use without CMake,
submodules, or vendoring multiple files.

The amalgamator itself is a small C99 program. It reads
`tools/amalgamate.manifest` (two sections, `@header_begin/@header_end` and
`@impl_begin/@impl_end`), concatenates the listed files in order, and writes
the combined output wrapped in:

```c
#ifndef LFG_CTEST_SINGLE_H_
/* ...headers, inline... */
#ifdef LFG_CTEST_IMPLEMENTATION
/* ...impl, inline... */
#endif
#endif
```

Per-file filtering:

- Internal `#include "lfg-ctest*"` directives are stripped.
- Include guards are stripped. Guard detection is a **repo-convention match**:
  an identifier is treated as a guard only if it ends in `_H_` (trailing
  underscore). This catches `LFG_CTEST_H_` / `LFG_CTEST_MOCK_H_` and ignores
  operational macros like `LFG_CTEST_HAS_FLOAT` / `LFG_CTEST_HAS_DOUBLE` that
  would false-match a naive `_H` substring check. Guard state is tracked
  per-file: the tool remembers the opening `#ifndef FOO_H_`, strips the
  matching `#define FOO_H_`, and strips the terminating `#endif` whose comment
  mentions `FOO_H_`.
- System includes (`#include <...>`) are deduplicated across all files so each
  standard header appears once in the output.

File-scope `static` state in the two `.c` halves (pass/fail counters, mock
reset registry) stays `static` inside the `LFG_CTEST_IMPLEMENTATION` block,
so multi-TU links against the amalgamated header are safe as long as exactly
one TU defines the gate. `RECORD_FAILURE` / `RECORD_PASS` macros from
`lfg-ctest.c` leak into `lfg-ctest-mock.c` scope after concatenation, but the
mock impl doesn't reference those names, so there's no collision.

`test-amalg` is the drift-detection smoke: it `#define`s
`LFG_CTEST_IMPLEMENTATION`, includes `dist/lfg-ctest.h`, and exercises a small
but representative slice (basic asserts, a mock with return queue + param
history, optional float/double asserts). It depends on the `amalgamate` target
so the dist header is always fresh before the smoke compiles. It does **not**
link against the `lfg-ctest` static lib — it provides its own impl.

## Float / double gating

`CMakeLists.txt` uses `check_symbol_exists(fabsf math.h)` and
`check_symbol_exists(fabs math.h)` (with `libm` on the required-libraries
line) to decide whether to define `LFG_CTEST_HAS_FLOAT=1` and/or
`LFG_CTEST_HAS_DOUBLE=1` on the library target, and whether to link `libm`
(`LFG_CTEST_NEEDS_LIBM`). Users can force either off via the
`LFG_CTEST_ENABLE_FLOAT` / `LFG_CTEST_ENABLE_DOUBLE` cache options.

All float/double assertions in `lfg-ctest.h` sit under
`#ifdef LFG_CTEST_HAS_FLOAT` / `#ifdef LFG_CTEST_HAS_DOUBLE`, so the library
links cleanly on platforms without an FPU.
