# api.md — lfg-ctest public API

Consumer-facing reference for the assertion API, the test runner, and the
mocking framework. For internals (how the framework is put together), see
[architecture.md](architecture.md). If you are learning the framework rather
than looking a symbol up, the [tutorial series](tutorial/README.md) walks the
same surface feature-by-feature with copy-pasteable examples; this page
cross-links into it from the relevant sections.

## Testing API

> Tutorial: [Your first test + runner](tutorial/01-first-test.md) introduces
> this section's runner calls, suites, setup/teardown, and the assertions
> gently, with a buildable example.

### Basic Test Structure

Tests are just `void` functions. The framework tracks pass/fail automatically.

```c
#include <lfg-ctest.h>

void test_example(void)
{
    ASSERT_TRUE(1 == 1);
    ASSERT_EQ(42, some_function());
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_test(test_example);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### Test Suites

Suites are also `void` functions that group related tests:

```c
void math_suite(void)
{
    lfg_ct_test(test_addition);
    lfg_ct_test(test_subtraction);
    lfg_ct_test(test_multiplication);
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_suite(math_suite);
    lfg_ct_suite(string_suite);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### Core Functions

| Function | Description |
|----------|-------------|
| `lfg_ct_start()` | Initialize test framework (call before any tests) |
| `lfg_ct_end()` | Finalize test framework |
| `lfg_ct_parse_args(argc, argv)` | Parse `--list` / `--filter <glob>` / `--filter-exclude <glob>` / `--strict-xpass` / `-x` / `--fail-fast` / `--seed <n>` / `--rerun-failed` / `--state-file <path>` / `-v` / `-q` / `--verbosity <n>` / `--isolation <mode>` / `--timeout <ms>` from `main(argc, argv)`. Returns 0 on success, non-zero on a bad flag or an unusable rerun state file (diagnostic printed to stderr). See [Listing and filtering](#listing-and-filtering), [Reproducing a randomized run](#reproducing-a-randomized-run), [Rerunning just the failures](#rerunning-just-the-failures), [Quiet output](#quiet-output), [Isolation modes](#isolation-modes), [Stopping at the first failure](#stopping-at-the-first-failure), and [Skip, xfail, xpass](#skip-xfail-xpass). |
| `lfg_ct_is_list_mode()` | Returns 1 if `--list` was parsed, 0 otherwise. |
| `lfg_ct_verbosity()` | Returns the `lfg_ct_verbosity_t` level in effect — `LFG_CT_VERBOSITY_QUIET` (0), `_DEFAULT` (1), or `_VERBOSE` (2). See [Quiet output](#quiet-output). |
| `lfg_ct_is_verbose()` | Returns 1 if the level is at least `LFG_CT_VERBOSITY_VERBOSE`, 0 otherwise. Derived from `lfg_ct_verbosity()`; semantics unchanged from when verbosity was a single boolean. |
| `lfg_ct_is_seed_set()` | Returns 1 if `--seed` was parsed **or** `--rerun-failed` restored a seed from the state file, 0 otherwise. Separate from the value because `0` is a legal seed. |
| `lfg_ct_get_seed()` | Returns the seed parsed from `--seed`, or 0 when none was given. Under `--rerun-failed` the persisted seed is loaded into this slot, so the predicate/value pair also reports a restored seed. An explicit `--seed` wins over the persisted one. |
| `lfg_ct_is_rerun_failed()` | Returns 1 if `--rerun-failed` was parsed, 0 otherwise. |
| `lfg_ct_set_fail_fast(enabled)` | Stop the run at the first failing test — the programmatic face of `-x` / `--fail-fast`. Whole-run scope. `void`, since a boolean mode cannot fail. Cleared by a later `lfg_ct_parse_args`. See [Stopping at the first failure](#stopping-at-the-first-failure). |
| `lfg_ct_get_fail_fast()` | Returns 1 if fail-fast is enabled, 0 otherwise. Reports the configured mode, not whether it has tripped. |
| `lfg_ct_state_path()` | Returns the rerun state file this run reads and writes — the `--state-file` value, or `".lfg-ctest-last"`. Never `NULL`. |
| `lfg_ct_name_runs(name)` | Returns 1 if a hypothetical test/suite named `name` would execute under the currently parsed filter state (0 if filtered, excluded, or in `--list` mode). Bare-name form, unchanged. **Cannot answer a qualified filter:** it builds the id from whatever file/suite batons are ambient, so called from `main()` (the documented usage) both are absent and the id is `(none)::(none)::name`. Under `--filter 'alpha.c::suite_one::test_db_roundtrip'` it returns 0 even though the test does run. Use `lfg_ct_id_runs` where that matters. |
| `lfg_ct_test_runs(name)` | Same query for the test named `name` **in this file and the enclosing suite**. Captures `__FILE__`, so it sees the same id `--list` prints. See [Entry ids](#entry-ids). |
| `lfg_ct_id_runs(file, suite, name)` | Explicit form behind `lfg_ct_test_runs`. `suite = NULL` means "the suite currently on the registration stack"; `""` means "no enclosing suite". |
| `lfg_ct_format_id(buf, cap, file, suite, test)` | Render an entry id into `buf` exactly as `--list` emits it. `test = NULL` renders a suite id. Truncates silently; returns the length the id needed, so `result >= cap` means truncation and `lfg_ct_format_id(NULL, 0, ...)` sizes a buffer. |
| `lfg_ct_test(fn)` | Execute a single test (`void fn(void)`). Body-only: any setup/teardown are plain functions the body calls itself. See [Setup and teardown](#setup-and-teardown). |
| `lfg_ct_suite(fn)` | Execute a test suite (`void fn(void)`). Body-only, same as `lfg_ct_test`; the suite body brackets its `lfg_ct_test` calls with any shared setup/teardown. |
| `lfg_ct_skip(reason)` | Mark current test as SKIP and return from the body immediately (`longjmp`s out). Legal from a setup helper the body calls; run any teardown *before* the skip. See [Skip, xfail, xpass](#skip-xfail-xpass). |
| `lfg_ct_xfail(reason)` | Mark current test as expected-to-fail; body runs to completion. Subsequent assertion failure -> XFAIL, no failure -> XPASS. Last reason wins on repeated calls. See [Skip, xfail, xpass](#skip-xfail-xpass). |
| `lfg_ct_print_summary()` | Print pass/fail/skip/xfail/xpass summary |
| `lfg_ct_return()` | Get overall return code (0=clean, non-zero=fail or xpass-with-`--strict-xpass`) |
| `lfg_ct_failure_count()` | Running tally of failed assertions (`size_t`). Snapshot-and-compare inside a body to cut a loop short or detect a setup failure. For run-level fail-fast use `-x` instead. See [Failure count](#failure-count). |
| `lfg_ct_version()` | Framework version string (`"M.m.p[+<sha>]"`) |

### Failure count

Every `ASSERT_*` is non-fatal (record-and-continue) and there is no
`REQUIRE`-style fatal variant, so `lfg_ct_failure_count()` is the supported
way to bail out of a region of assertions *inside one body*: snapshot it at
the top of the region and compare.

> For run-level fail-fast — stop the whole run at the first failing *test* —
> use `-x` / `--fail-fast` or `lfg_ct_set_fail_fast` instead; see
> [Stopping at the first failure](#stopping-at-the-first-failure). The two
> work at different granularities, and the recipe below remains the only
> tool for cutting a loop short within a single test body.

```c
void test_foo(void)
{
    size_t baseline = lfg_ct_failure_count();
    for (size_t i = 0; i < 1000; ++i)
    {
        run_bar_tests();                                /* many non-fatal asserts */
        if (lfg_ct_failure_count() > baseline) break;   /* fail-fast */
    }
}
```

The same snapshot-and-compare catches a failure buried in a setup helper
(an assert, not a returned status) without branching on each assert:

```c
size_t baseline = lfg_ct_failure_count();
setup();
if (lfg_ct_failure_count() > baseline)
{
    teardown();
    lfg_ct_skip("setup failed");   /* teardown before skip */
    return;
}
```

**Boundary semantics.** The returned value is the _global_ running count,
not a per-test slice. It is strictly monotonic only _within_ a single test
or setup body: at classification the runner absorbs a completed test's
assertion failures back out of the tally for `SKIP` / `XFAIL` / `XPASS`
outcomes, so the count can move down at a test boundary. Always
snapshot-and-compare inside one body; never rely on cross-test
monotonicity.

### Entry ids

> Tutorial: [Running and filtering](tutorial/02-running-and-filtering.md#entry-ids).

Test and suite names are not unique within a binary, so every registered entry
also has an id:

```
<file>::<suite>::<test>      a test
<file>::<suite>              a suite
```

`<file>` is the **basename** of the registration site's `__FILE__` — captured
by the `lfg_ct_test` / `lfg_ct_suite` macros, so no call site changes. Using
the basename keeps ids stable across build directories and out-of-tree builds;
nothing in an id derives from run order or an address, so a `--list` run and
the filtered run after it always agree.

A missing component is spelled `(none)` (`LFG_CT_ID_NO_SUITE`) rather than left
empty, so a test registered outside any suite is `foo.c::(none)::test_x` and
never a malformed `foo.c::::test_x`. The separator is `LFG_CT_ID_SEPARATOR`
(`::`) — the same one the verbose reporter already uses for `suite::test`.

Ids stay distinct under the single-header amalgamation: the registration macros
expand at the consumer's call site, so `__FILE__` is the consumer's test file,
not the amalgamated header.

Full qualification is unique unless the same test name is registered twice in
the same suite in the same file. That is a duplicate registration; the
framework does not diagnose it, and the id addresses both.

### Listing and filtering

> Tutorial: [Running and filtering](tutorial/02-running-and-filtering.md) walks
> `--list` / `--filter` and the one-binary-many-CTest-entries pattern.

`lfg_ct_parse_args(argc, argv)` lets a single test binary expose the
registered ids and a glob selector — the natural pairing for
CMake-driven `ctest --parallel` workloads where one binary backs many
`add_test` entries:

```cmake
foreach(ind sma ema wma)
  add_test(NAME test_ind_${ind} COMMAND test_indicators --filter "suite_${ind}_*")
endforeach()
```

Recognized flags:

| Flag | Effect |
|------|--------|
| `--list` | Print every test/suite [id](#entry-ids) encountered (one per line on stdout, `\n`-terminated). Suite bodies are still invoked so their contained tests can list themselves; setup/teardown of suites and tests are skipped. Each line is directly usable as a `--filter` argument. |
| `--filter <glob>` | Run only entries whose id matches the shell-style glob, which **addresses exactly as many trailing `::`-delimited components as it spells out** (`fnmatch(3)` syntax: `*`, `?`, `[...]`). Repeat the flag to OR-combine patterns. If a suite matches, every entry inside it inherits the match — useful with the `add_test` pattern above. |
| `--filter-exclude <glob>` | Skip entries whose id matches the glob, same component-depth rule. Same repeat / OR semantics. **Exclude wins** when both `--filter` and `--filter-exclude` match the same entry. |
| `--strict-xpass` | Flip an otherwise-clean run that contains one or more `xpass` outcomes to a non-zero exit code. Permissive (no exit-code effect) by default. See [Skip, xfail, xpass](#skip-xfail-xpass). |
| `-x`, `--fail-fast` | Stop the run at the first failing test. Scope is the **whole run**, not the enclosing suite. Off by default; the CLI face of `lfg_ct_set_fail_fast`. See [Stopping at the first failure](#stopping-at-the-first-failure). |
| `--seed <n>` | Seed `rand(3)` with `n` instead of a generated value, so a run that used `rand()` can be replayed exactly. Decimal, must fit an `unsigned`; `0` is a legal seed. Repeating the flag keeps the last value. A missing, non-numeric, or out-of-range value is an error. See [Reproducing a randomized run](#reproducing-a-randomized-run). |
| `--rerun-failed` | Run only the entries the previous run recorded as failed, restoring that run's seed. Intersects with `--filter` — the replayed set is the candidate pool and the filter narrows it further. An explicit `--seed` outranks the persisted one. A missing, malformed, or wholly stale state file is an error, never a silent full-suite run. See [Rerunning just the failures](#rerunning-just-the-failures). |
| `--state-file <path>` | Read and write the rerun state at `path` instead of `.lfg-ctest-last` in the current working directory. Repeating the flag keeps the last value. |
| `-v`, `--verbose` | Stream a per-test `START` line before each test body is dispatched and an outcome line (`PASS` / `FAIL` / `SKIP` / `XFAIL` / `XPASS`) with elapsed milliseconds after the test classifies. Off by default; orthogonal to other flags. Coexists with a user-installed reporter (e.g. JUnit-XML). See [Verbose output](#verbose-output). |
| `-q`, `--quiet` | Reduce output to failures plus the final summary. Alias for `--verbosity 0`. See [Quiet output](#quiet-output). |
| `--verbosity <n>` | Set the level that `-q` and `-v` alias, as an integer: `0` quiet, `1` default, `2` verbose. Repeating the flag keeps the last value. A missing, non-numeric, or out-of-range value is an error. |
| `--isolation <mode>` | Select the dispatch mode: `none` (in-process) or `fork` (fork-per-test). The CLI face of `lfg_ct_set_isolation`. Requesting `fork` on a build without fork support is an error with a stderr diagnostic — never a silent fallback to in-process. Any other mode name, and a missing value, are errors. Repeating the flag keeps the last value. See [Isolation modes](#isolation-modes). |
| `--timeout <ms>` | Per-test timeout applied under `LFG_CT_ISOLATE_FORK`. The CLI face of `lfg_ct_set_fork_timeout_ms`. `0` disables the timeout and is a legal value, so a missing, non-numeric, negative, or out-of-range value is an error rather than a silent coercion to `0`. Accepted on any build; inert while isolation is `none`. Repeating the flag keeps the last value. See [Isolation modes](#isolation-modes). |

`--isolation` and `--timeout` are the two flags that drive **API-owned**
state rather than parse-owned state. Consequences, spelled out under
[Isolation modes](#isolation-modes): a `lfg_ct_parse_args` call that omits
them leaves the current configuration standing instead of resetting it, a
failed parse applies neither, and a setter called afterwards overrides the
command line.

A glob addresses **exactly as many trailing components as it spells out**: a
glob with no `::` is matched against the test name alone, one with a single
`::` against `suite::test`, and one with two against the full id. Qualify as
far as you need to:

```bash
./test_indicators --filter 'test_roundtrip'                        # every test of that name
./test_indicators --filter 'suite_sma::test_roundtrip'             # ...in that suite
./test_indicators --filter 'test-ind-sma.c::suite_sma::test_roundtrip'  # exactly one
```

Because the depth is pinned by the glob itself, every glob that selected an
entry before ids existed selects exactly the same set now — existing
`--filter "suite_sma_*"` shards need no edit, wildcards included. This is why
the rule is depth-pinned rather than "match any suffix": under a plain
any-suffix rule a bare `--filter 'test_*'` would also match the *file*
component of `test_math.c::...` — and the suite id `test_math.c::math_suite`,
dragging in that whole suite — which would silently break the near-universal
`test_*.c` file naming convention.

`*` therefore never crosses a `::`; it fills exactly one component. To reach
the file component, spell it out:

```bash
./test_indicators --filter 'test-ind-sma.c::*::*'   # every test in that file
./test_indicators --filter '*::test_roundtrip'      # that test in any suite
```

`--list` composes back into `--filter`:

```bash
./test_indicators --list | grep roundtrip | xargs -I{} ./test_indicators --filter '{}'
```

> **Output change.** `--list` previously printed bare names terminated with
> `\r\n`. It now prints ids terminated with `\n` (the human-facing report still
> uses `\r\n`) so the listing pipes cleanly into `xargs`. A script that parsed
> the old listing needs updating; one that fed it back into `--filter` is
> unaffected.

An unmatched filter is not an error: zero tests run, the binary exits 0.
Unknown flags print usage to stderr and return non-zero — `main` should
propagate that. Wire it once at the top of `main`:

```c
int main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }
    lfg_ct_start();
    /* ... lfg_ct_suite / lfg_ct_test calls ... */
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

`lfg_ct_name_runs(name)` lets a consumer short-circuit expensive setup
outside the framework (e.g. opening a database connection) when the
current filter state would skip the test anyway. It keeps its bare-name
meaning; `lfg_ct_test_runs(name)` is the id-aware form, capturing `__FILE__`
so the query resolves the same entry `--list` names. `lfg_ct_is_list_mode()`
exposes the `--list` bit directly — gate your own diagnostic prints on
`!lfg_ct_is_list_mode()` if you want stdout to be a clean
newline-separated list of ids. `lfg_ct_is_verbose()` exposes the
`-v` / `--verbose` bit; consumer-side reporters can use it to decide
whether to suppress redundant output of their own. Each call to
`lfg_ct_parse_args` replaces any previously parsed state, including
the verbose toggle.

### Stopping at the first failure

When you are iterating on one known-broken area, the first failure is
enough to act on and the rest of the run is wasted time — and worse, the
failure you care about scrolls away under output that no longer matters.
`-x` / `--fail-fast` stops the run there:

```bash
./test_indicators -x
```

```
*** test FAILURE: test_sma_window_boundary
*** Executed 47 assertions in 12 tests. Failures: 1, Skipped: 0, XFail: 0, XPass: 0
*** Stopped early: --fail-fast tripped at the first test failure; remaining tests were not run.
*** Testing complete. Result: FAIL
```

The programmatic equivalent is `lfg_ct_set_fail_fast(1)`, with
`lfg_ct_get_fail_fast()` reading it back. Like every other parse-derived
setting, a later `lfg_ct_parse_args` call that omits the flag clears it.

**Scope is the whole run, not the enclosing suite.** Once any test has
failed, no later test body executes and no later suite body is entered,
across every remaining suite — matching `pytest -x` and
`go test -failfast`. The gate is never re-armed. The suite-scoped
alternative (abort only the suite that failed, continue with the next)
was considered and rejected: a `--filter`ed run is already narrowed to
the suites you care about, so the composition benefit is largely
delivered by the filter itself.

Mechanically this is a **suppression gate, not an abort**. `lfg_ct_test`
executes its body at the call site and a suite is an ordinary C function,
so there is no run loop for the framework to break out of and nothing is
unwound out of your `main()`. The process still walks every remaining
call site and does nothing at each one — one boolean test per site, and
the suite-level gate skips a whole suite body without entering it.

Consequences worth knowing:

- **The summary says the run stopped early**, because suppressed tests
  execute no body and so land in no bucket — `tests executed` simply ends
  up lower. Without that line a fail-fast run would be indistinguishable
  from one where most tests silently vanished. Suppressed tests are *not*
  reclassified as SKIP: that bucket is a per-test disposition set by
  `lfg_ct_skip`, and borrowing it here would corrupt the tally semantics
  skip/xfail maintain. The line prints at every verbosity, `-q` included.
- **Exit status is non-zero through the normal path** — `lfg_ct_return()`
  already returns `-tests_failed`. No second mechanism.
- **A clean `-x` run is byte-identical** to the same run without `-x`.
- **`--list` is unaffected.** A listing executes no test, so it can never
  trip the gate.
- **`--strict-xpass` does not trip it.** An XPASS is a verdict modifier,
  not a test failure, so such a run still executes to completion and
  fails only at the end. The two are independent.
- **Identical under both [isolation modes](#isolation-modes).** The gate
  reads the failed-*test* tally that the in-process classifier and the
  fork parent's projection both converge on, so it needs no knowledge of
  which path dispatched. Under `fork`, the child that produced the
  failure has already been reaped before the gate can observe it — there
  is no window in which an orphan can exist.
- **With [`--rerun-failed`](#rerunning-just-the-failures) the replay set
  shrinks, and it shrinks *lossily*.** The state file is rewritten from
  what this run actually classified, so a tripped run persists exactly one
  key — the test you stopped at. Every other key the previous run had
  recorded is dropped, including ones that are still failing and that this
  run simply never reached. Combining the two flags therefore narrows the
  replay set to a single test per iteration: fix it, rerun, discover the
  next one, and so on, rather than working through a known list. If you
  want the full failure set preserved across iterations, run
  `--rerun-failed` **without** `-x`. Relatedly, the unresolved-key
  warnings are suppressed when the gate trips: a key the run never
  reached is not a key that stopped naming a registered test, so every
  such warning would be a false alarm.

Note that `lfg_ct_failure_count()` covers a different granularity — see
[Failure count](#failure-count) — cutting a loop short *within* one test
body, not stopping the run.

### Reproducing a randomized run

`lfg_ct_start()` seeds `rand(3)` and announces the seed it used:

```
*** random seed is 3314123391
```

Without `--seed` the value is generated per run from the wall clock, the
process CPU clock, and a stack address, so it spans the full `unsigned`
range and two runs started within the same second get different seeds.
That makes a suite whose tests draw scenarios from `rand()` genuinely
varied — and, on its own, irreproducible.

`--seed <n>` closes the loop: read the seed off the failing run, pass it
back, and the `rand()` sequence replays exactly.

```bash
$ ./test_indicators                    # *** random seed is 3314123391 -- one test fails
$ ./test_indicators --seed 3314123391  # same scenario, as many times as you need
```

The banner prints the *effective* seed either way, so a replayed run and
the run it reproduces emit an identical line. Under `--list` the banner
stays suppressed (list output remains a clean newline-separated list of
ids) but the seed is still applied.

`lfg_ct_is_seed_set()` reports whether `--seed` was parsed and
`lfg_ct_get_seed()` returns the parsed value; the two are separate
because `0` is a legal seed rather than an "unset" sentinel. The
accessor reports only what `--seed` supplied — on a generated-seed run
it returns `0` — so a consumer driving its own RNG off the same flag
must guard on the predicate:

```c
if (lfg_ct_is_seed_set())
{
    my_rng_seed(lfg_ct_get_seed());
}
```

### Rerunning just the failures

Every non-listing run persists what it learned to a state file —
`.lfg-ctest-last` in the current working directory by default:

```
lfg-ctest-state 1
seed 3314123391
fail test-ind-sma.c::suite_sma::test_roundtrip
fail test-ind-ema.c::suite_ema::test_warmup
```

Three records, all of them load-bearing: a format marker, the seed the
run used, and one [entry id](#entry-ids) per failed test. The ids are the
same ones `--list` emits and `--filter` matches — there is no second
addressing scheme.

`--rerun-failed` reads it back, restores the seed, and runs exactly that
set:

```bash
$ ./test_indicators                 # 169 failures across 655 tests, two minutes
$ ./test_indicators --rerun-failed  # just those 169, under the same seed
```

The seed rides in the file because replaying the *selection* without the
conditions the tests failed under is not a reproduction. An explicit
`--seed` outranks the persisted one, for deliberately re-running the same
selection under different conditions:

```bash
$ ./test_indicators --rerun-failed --seed 42
```

A `--rerun-failed` run rewrites the file with **its** failures, so
repeating the flag converges on what still fails instead of replaying the
original set forever. Combine it with `--filter` to narrow further — the
two intersect, the replayed set being the candidate pool:

```bash
$ ./test_indicators --rerun-failed --filter 'suite_sma::*'
```

Note that such a run rewrites the state file from the **intersected**
set: the records for failures the filter excluded are dropped, not held
aside. Replaying 169 failures under `--filter 'suite_sma::*'` leaves a
file describing only the `suite_sma` outcomes. Pass `--state-file` to
work against a narrowed slice without spending the full record.

The failure records come from the runner's own classification, never from
parsed output, so the file is identical under `LFG_CT_ISOLATE_FORK` (the
parent records the child's disposition) and with stdout redirected to a
file or a pipe.

Failure modes are all loud, because the one outcome a user must never be
handed is a full-suite run they believe was narrowed:

| Situation | Behavior |
|-----------|----------|
| No state file | stderr diagnostic, non-zero exit. Never falls through to the whole suite. |
| Unreadable or malformed state file | stderr diagnostic, non-zero exit. Nothing is partially applied — a half-parsed file would narrow the run to some prefix of the failures. |
| Previous run was green (no `fail` records) | Runs nothing, says so on stdout, exits 0. |
| A persisted key no longer resolves (test renamed, removed, binary rebuilt) | Warns naming the key, runs the ones that do resolve. |
| A persisted key sits under a suite `--filter-exclude` skipped | Accounted for by the exclusion, not warned about. The suite body never runs, so the test never registers — that is the user's instruction, not a stale record. Ids are flat (`<file>::<suite>::<test>`), so a test in a suite *nested* inside the excluded one keeps the inner suite's name and is still reported as renamed-or-removed. |
| The state file cannot be written (crash, signal, `ENOSPC`, unwritable path) | The **previous** file survives intact. The new one is staged as a `<path>.tmp` sibling and renamed into place, so `<path>` is only ever replaced by a complete file — a truncated one would still parse and silently narrow the next replay. On Windows the old file is removed just before the rename, since `rename()` there fails on an existing destination; the replacement is not atomic on that host. |
| *No* persisted key resolves | stderr diagnostic, non-zero exit — running nothing silently would read as "all fixed". |
| `--list` | Neither writes nor truncates the file, so listing between two reruns is safe. |

`--state-file <path>` moves the file, which is what an out-of-tree build
wants when the working directory is not where the artifacts belong:

```cmake
add_test(NAME test_indicators COMMAND test_indicators
    --state-file ${CMAKE_CURRENT_BINARY_DIR}/.lfg-ctest-last)
```

Add the default name to your `.gitignore`.

### Verbose output

`-v` / `--verbose` streams the runner's per-test progress to `stdout`,
in the spirit of cmake's `ctest -V`. With the flag set:

- before each test body is dispatched, the runner emits
  `*** START: <suite>::<name>` (or `*** START: <name>` for top-level
  tests with no enclosing suite);
- after the test classifies, the runner emits
  `*** PASS|FAIL|SKIP|XFAIL|XPASS: <suite>::<name> (X.XXX ms)`, with
  the disposition reason or assertion text appended after `:` when one
  is available (SKIP / XFAIL / XPASS reasons, FAIL assertion text).

The disposition keywords match the labels used elsewhere in the
runner. Elapsed time is wall-clock milliseconds with sub-millisecond
precision; the source clock is the same `clock(3)`-based seconds
counter that feeds the reporter's `time_sec` field.

Verbose output is built on top of the [reporter contract](#reporter-callback)
as the framework's first built-in reporter: enabling `--verbose`
installs an internal reporter that prints the banners and then fans
out to whatever consumer reporter `lfg_ct_set_reporter` had attached.
So `--verbose` and a JUnit-XML report can be active in the same run
without interfering, and any future report format that registers
through the same hook inherits the same fan-out.

Default behaviour (no flag): no extra lines, byte-identical output to
prior framework versions. The flag is purely opt-in.

Under `LFG_CT_ISOLATE_FORK` the parent serialises both the `START`
banner (before `fork(2)`) and the outcome banner (after the child's
payload arrives), so no verbose line interleaves with another test's
stdout — and the child does not re-fire the start callback because
its reporter slot is swapped to the fork TU's capture reporter for
the duration of the child.

### Quiet output

`-q` / `--quiet` goes the other direction: it reduces the run to
failures plus the final summary. It exists for suites large enough that
the per-suite and per-test progress lines bury the assertion detail —
a 655-test run that emits 1400 lines needs `grep` to find the one thing
that went wrong.

`-q`, the default, and `-v` are three points on one integer axis rather
than independent switches. `--verbosity <n>` addresses that axis
directly (`0` quiet, `1` default, `2` verbose) and the two letter flags
are aliases for its ends, so `lfg_ct_verbosity()` reports the level and
`lfg_ct_is_verbose()` remains a derived predicate (`level >= 2`) with
its original meaning.

At quiet level these lines are **suppressed**:

| Line | Why |
|------|-----|
| `*** begin unit test` | Banner; carries no run-specific information. |
| `*** suite FAILURE: <name>` | An aggregate that adds no localization over the per-test `FAILURE` line plus the assertion detail. A consumer registering many identically-named suites gets one repetition per invocation, which is the bulk of the noise the flag exists to remove. |
| `*** test SKIP: ...` | Non-failure dispositions; still counted in the summary. |
| `*** test XFAIL: ...` | ditto |
| `*** test XPASS: ...` | ditto |

And these are **retained**:

| Line | Why |
|------|-----|
| `*** <file>:<line>: FAILURE in <fn>(): <msg>` | The assertion detail — the signal the progress lines were drowning. |
| `*** random seed is <n>` | Deliberately kept despite being a banner: a quiet failing run whose failures cannot be reproduced is a worse artifact than one extra line. This is the handle [`--seed`](#reproducing-a-randomized-run) takes. |
| `*** test FAILURE: <name>` | Names the failing test. |
| Final summary counters, [grouped failure summary](#grouped-failure-summary), and `Result: PASS/FAIL` | The verdict. |
| `--list` output | Unaffected; its name output is the entire purpose of that mode. |
| stderr misuse warnings | Diagnostics for API misuse, not progress. |

Mixing `-q` and `-v` is not an error — they are ends of one axis, so
the last one on the command line wins, in either order.

Quiet is a **presentation** flag only. It gates the runner's own
`printf` sites, not the [reporter chain](#reporter-callback): exit
codes, `--list` output, and the record stream a user-installed reporter
receives are identical at every level, so a JUnit-XML report emits the
same document whether or not `-q` was passed. The suppression also
holds under `LFG_CT_ISOLATE_FORK` — a forked failing test still prints
its `test FAILURE` line and assertion detail, and a forked
skip/xfail/xpass prints nothing.

> **Note.** The assertion-detail line is retained unconditionally, so an
> `xfail` test's assertion text still prints at quiet level while its
> `*** test XFAIL:` classification line does not. An expected failure
> therefore reads as a bare failure detail unless you cross-reference
> the summary counters.

### Skip, xfail, xpass

> Tutorial: [Skip, xfail, xpass](tutorial/03-skip-xfail-xpass.md) covers the
> three buckets and when to reach for each.

Tests that are known-failing for a tracked reason (a deferred fix, an
environment-specific path, a flake under investigation) can declare
their expected disposition inline. Two macros, callable from inside a
test body (and from any setup helper the body calls, where `lfg_ct_skip`
is the natural way to express "preconditions not met"):

```c
lfg_ct_skip("waiting on driver fix");
lfg_ct_xfail("known flaky under valgrind");
```

| Macro | Effect |
|-------|--------|
| `lfg_ct_skip(reason)` | Mark the test as **SKIP**, record `reason`, and return from the body immediately (`longjmp`s out). Called from a setup helper, it unwinds the rest of the body too — run any teardown before it. SKIP does not count toward pass or fail. |
| `lfg_ct_skip_cleanup(cleanup, reason)` | Same as `lfg_ct_skip`, but first invokes the `cleanup` function pointer (a plain `void(void)`, the shape a teardown already has) before unwinding. Sugar for the two-line `teardown(); lfg_ct_skip(reason);` step so the skip path is one call. `cleanup` may be `NULL`, which degrades to a plain `lfg_ct_skip`. Purely optional — the two-line form remains equally valid. |
| `lfg_ct_xfail(reason)` | Mark the test as expected-to-fail and continue executing. After the body completes: if any assertion failed, the test is **XFAIL** (separate bucket); if none failed, the test is **XPASS** (separate bucket). Repeated calls keep the latest reason. |

**Scope:** per-test only. Both macros are no-ops with a stderr warning
when called from anywhere else — `main`, a suite body, or between tests.
They are legal inside a test body and inside any setup/teardown helper
that body calls (which runs within the same per-test context).
Suite-level skip is not (yet) a first-class feature; if you need to skip
a whole suite based on a runtime precondition, gate the `lfg_ct_test(...)`
calls inside the suite body on a regular `if` instead.

Per-test reporting:

```
*** test SKIP: <name>: <reason>
*** test XFAIL: <name>: <reason>
*** test XPASS: <name>: <reason>
*** test FAILURE: <name>
```

Final summary (the `Failures:` count is `tests_failed` as before; the
three trailing counts are the new buckets):

```
*** Executed N assertions in M tests. Failures: F, Skipped: S, XFail: XF, XPass: XP
*** Testing complete. Result: PASS|FAIL
```

### Grouped failure summary

A run with at least one failure emits a block between those two lines,
collapsing the failures to one line per distinct test name:

```
*** Executed 1111 assertions in 655 tests. Failures: 169, Skipped: 0, XFail: 21, XPass: 35
*** Failure summary: 169 failures in 8 distinct tests
***    34  test_e2e_validation              (first: tests/e2e.c:412: in validate_column())
***    29  test_mtf_filter                  (first: tests/mtf.c:88: in test_mtf_filter())
***    28  test_e2e_lagged                  (first: tests/e2e.c:377: in validate_column())
...
*** Testing complete. Result: FAIL
```

- **Grouping key is the registered test name** — the framework's own
  identity for a failure. A helper shared across tests is still visible,
  through the `in <fn>()` of the first-occurrence line rather than as its
  own group.
- **Order is count descending**, ties broken by first-occurrence order so
  the block is byte-identical across runs of the same set. The largest
  group is usually one systemic cause; surfacing it first is what makes
  the block actionable.
- **First occurrence** is the `file:line: in fn()` of the first failing
  assertion recorded for that group, pinned when the group is created.
  A failure with no assertion location — fork mode reporting a signal, a
  timeout, or a `fork()`/`pipe()` failure — reads `(unknown)`.
- Only `LFG_CT_FAILED` outcomes appear. Skipped, XFail and XPass tests
  never do.
- A fully passing run and `--list` emit nothing: no block, no header.
- Fork mode (`LFG_CT_ISOLATE_FORK`) produces the identical block. The
  summary is accumulated in the parent, fed by both the in-process
  classification path and the fork projection path.

Groups are held in a fixed-size table (`LFG_CT_FAILGROUP_MAX`, 256),
matching the runner's allocation-free style — the accumulator runs during
a failing run, which is the worst moment to add an allocation-failure
path. Failures beyond that many *distinct* names still count toward the
header total and are reported explicitly rather than silently dropped:

```
*** Failure summary: 4213 failures in at least 256 distinct tests
...
*** 44 further failures ungrouped: the distinct-test cap (LFG_CT_FAILGROUP_MAX = 256) was reached; their distinct-test count is unknown
```

Once the cap is reached the header switches to **at least** N: the table
keeps nothing that could tell the dropped failures apart, so their
distinct-test count is genuinely unknown. Note the two figures cannot be
added to recover it — the header counts *tests*, the overflow line counts
*failures*.

A test name longer than `LFG_CT_FAILGROUP_NAME_MAX` (128) is truncated in
the stored key, so two names agreeing within that width would fold into a
single group. The width is sized well past the naming conventions this is
used with, but it is a bound on the key, not just on display.

The two surrounding lines are unchanged in text, and the verdict stays
last, so tooling that tails or greps the end of a log is unaffected.

Exit-code rules (`lfg_ct_return()`):

| Outcome | Exit |
|---------|------|
| Only PASS / SKIP / XFAIL outcomes | 0 |
| Any FAIL | non-zero |
| Any XPASS **and** `--strict-xpass` was parsed | non-zero |
| Any XPASS without `--strict-xpass` | 0 (warning only) |

Use `lfg_ct_xfail` when a test is documented as broken but you want to
keep exercising the code path; promote to a real failure later by
deleting the `lfg_ct_xfail` call (or pass `--strict-xpass` in CI to
catch the moment the bug fixes itself).

### Isolation modes

> Tutorial: [Fork-per-test isolation](tutorial/07-fork-isolation.md) explains
> what fork mode buys, the graceful-fallback wiring, and platform gating.

By default, every `lfg_ct_test` body runs in the test binary's own
process — the historical, fastest path. A crashing test (`abort`,
SIGSEGV, etc.) takes down the runner and stops subsequent tests; a
test that leaks or mutates a global pollutes the next one.

`lfg_ct_set_isolation(mode)` switches the dispatch shape for every
subsequent `lfg_ct_test` invocation:

| Mode | Effect |
|------|--------|
| `LFG_CT_ISOLATE_NONE` (default) | In-process dispatch. Fast; no isolation between tests. |
| `LFG_CT_ISOLATE_FORK` | Per-test `fork(2)`. Crash survival, true state reset, per-test sanitizer attribution. |

Both modes are also reachable from the command line via
`--isolation none|fork`, and the fork timeout via `--timeout <ms>`, so a
run can be reshaped without editing and rebuilding a test `main` —
attaching a debugger to one failing test, or dropping isolation when fork
mode is itself what you suspect.

The two surfaces share one piece of state, and the contract is
**last-writer-wins**:

```c
int main(int argc, char *argv[])
{
    lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);   /* the program's default */

    if (0 != lfg_ct_parse_args(argc, argv))      /* parse LAST, so the */
    {                                            /* CLI can override it */
        return 1;
    }
    ...
}
```

Call `lfg_ct_parse_args` **after** any `lfg_ct_set_isolation` /
`lfg_ct_set_fork_timeout_ms` call for the command line to win; call a setter
after it to pin a value the CLI cannot change. Unlike the filter and
verbosity state, these two are not reset by `lfg_ct_parse_args`: a parse that
does not mention the flags leaves them alone, and a parse that fails applies
neither.

On a build configured with `-DLFG_CTEST_ENABLE_FORK=OFF`, `--isolation fork`
fails the parse with a diagnostic naming the unavailability, mirroring
`lfg_ct_set_isolation`'s non-zero return. The mode is left at `none`; there
is no silent downgrade in either surface.

Fork mode mechanics:

- For each `lfg_ct_test(...)`, the parent `fork`s. The child runs the
  test body via the in-process path and ships an outcome payload back
  over a pipe. The parent decodes, projects the outcome onto its own
  counters / reporter, and continues with the next test.
- A child that dies on a signal (SIGSEGV, SIGABRT, SIGBUS, ...) is
  reaped by the parent and recorded as `FAILED` with a "killed by
  signal N" message; the runner advances to the next test. The crashed
  child is discarded whole — its entire address space dies with it, so
  crash-safety comes from fork isolation, not from teardown, and no
  in-body `teardown()` runs on that path (nor needs to).
- A child that exits non-zero **without** writing the payload (e.g.
  the body calls `_exit(N)`) is recorded as `FAILED` with a "child
  exited N with no payload" message — no silent pass.
- Stdout/stderr are shared with the child via the default fork fd
  inheritance, so the child's per-test banner reaches the user
  without any explicit capture wiring.
- Expensive shared setup stays in the parent: anything opened or
  computed before the dispatch (file descriptors, parsed fixtures,
  DB handles) is inherited by every child via COW.

Per-test timeout:

```c
lfg_ct_set_fork_timeout_ms(50);   /* 50ms per test; 0 disables (default) */
```

A child that has not exited within the configured timeout is sent
`SIGKILL` and recorded as `FAILED` with a "timed out after Nms"
message. The setting only takes effect under `LFG_CT_ISOLATE_FORK`
and persists across mode changes.

Platform + opt-out:

- Fork mode is Unix-family (`__unix__` / `__APPLE__`) only. On other
  platforms `lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK)` returns
  non-zero and leaves the configured mode unchanged. **No silent
  fallback.**
- Consumers who don't want the additional dependency surface (extra
  syscalls, signal-handling glue, `waitpid` wiring) can build the
  framework with the CMake option `-DLFG_CTEST_ENABLE_FORK=OFF`
  (equivalent to defining `LFG_CT_DISABLE_FORK=1` on the
  `lfg-ctest-fork.c` TU). The resulting binary has no
  fork/waitpid/signal-handling code linked in;
  `lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK)` returns the same
  documented error as the unsupported-platform path.
- The `LFG_CT_ISOLATE_FORK` enum value is part of the public ABI
  unconditionally — consumer code that references it still compiles
  in both builds.

Out of scope:

- **Nested forks inside a test.** Test code that calls `fork(2)`
  itself is on its own — the parent only waits on its direct child.
- **Windows isolation.** A `CreateProcess` stub would be a separate
  follow-up; the current cut documents Windows as `LFG_CT_ISOLATE_FORK`
  returning the clean error.

Example:

```c
int main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv)) return 1;

    /* Opt in to fork-per-test for tests that need crash survival or
     * true state reset. Fall back gracefully if the runtime support
     * is unavailable (Windows, or LFG_CT_DISABLE_FORK build). */
    if (0 != lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK)) {
        fprintf(stderr, "fork isolation unavailable; running in-process\n");
    }
    lfg_ct_set_fork_timeout_ms(5000);  /* 5s per test */

    lfg_ct_start();
    /* ... lfg_ct_suite / lfg_ct_test calls ... */
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### Reporter callback

> Tutorial: [Integration](tutorial/08-integration.md) shows the opt-in
> JUnit-XML reporter (a reference consumer of this hook) wired into a build.

`lfg_ct_set_reporter(const lfg_ct_reporter_t *)` installs a pluggable
hook that observes each test's classified outcome plus run completion.
Default reporter is `NULL` -- the core stays format-agnostic and zero
footprint by default. One slot, by design; fan-out is a downstream
concern.

```c
typedef enum {
    LFG_CT_PASSED, LFG_CT_FAILED,
    LFG_CT_SKIPPED, LFG_CT_XFAIL, LFG_CT_XPASS
} lfg_ct_outcome_t;

typedef struct {
    const char *suite_name;     /* enclosing lfg_ct_suite, or NULL */
    const char *test_name;
    double time_sec;
    lfg_ct_outcome_t outcome;
    const char *message;        /* failure text / skip reason / xfail reason */
} lfg_ct_record_t;

typedef struct {
    void (*on_record)(const lfg_ct_record_t *, void *userdata);
    void (*on_run_complete)(void *userdata);
    void *userdata;
    void (*on_test_start)(const char *suite_name, const char *test_name,
                          void *userdata);
} lfg_ct_reporter_t;
```

`on_record` fires once per classified test, at the end of
`lfg_ct_test_impl`. `on_run_complete` fires once at the end of
`lfg_ct_print_summary` -- the natural flush point for a buffering
reporter. `on_test_start` fires once per admitted test from the
parent dispatcher, immediately before the body is invoked; under
`LFG_CT_ISOLATE_FORK` this runs in the parent before `fork(2)`, so
the start callback is the natural seat for "this test is about to
run" annotations that must precede the child's stdout.

Field order is part of the contract: positional initialisers like
`{on_record, on_run_complete, &userdata}` written against the
original three-field bundle stay valid -- `on_test_start` lives at
the tail and defaults to `NULL`.

String fields on the record (`suite_name`, `test_name`, `message`) are
**borrowed** -- valid only for the duration of the callback. Buffer
them if you need them past that.

See `contrib/junit-xml/lfg-ctest-junit.[ch]` for a reference consumer.
Drop the contrib in via `add_subdirectory(deps/lfg-ctest/contrib/junit-xml)`
or omit it entirely if you don't need JUnit output.

### Version Macros

`lfg-ctest.h` transitively includes a generated `lfg-ctest-version.h`
(produced at build time from `git describe --match 'v*'`), exposing:

| Macro | Example |
|-------|---------|
| `LFG_CTEST_VERSION` | `"0.1.42"` |
| `LFG_CTEST_VERSION_FULL` | `"0.1.42+d2b1fa3"` |
| `LFG_CTEST_VERSION_MAJOR` / `_MINOR` / `_PATCH` | `0` / `1` / `42` |

Use the macros for compile-time gating and `lfg_ct_version()` for runtime
reporting. When the source tree has no matching tag (e.g. vendored
tarball), the macros fall back to `0.0.0`.

Tagging convention:

- **`v<M>.<m>`** (annotated) — the rolling base tag. In-development builds
  report `M.m.<distance>+<sha>`, e.g. `0.1.42+d2b1fa3`.
- **`release-v<M>.<m>.<p>`** (annotated, exact-match) — an immutable release
  marker. When HEAD is exactly on one, the `+<sha>` trailer is dropped and
  the version reports as a clean `M.m.p`, e.g. `0.1.42`. Releases are what
  CI builds and publishes; intermediate commits keep the base-tag form.

For maintainers: `cmake --build build --target release-tag` reads the
current `git describe` output, proposes the matching `release-v<M>.<m>.<p>`,
prompts before creating it, and opens `$EDITOR` for the annotation message.

### Setup and Teardown

The framework binds **no** lifecycle hooks. `lfg_ct_test` / `lfg_ct_suite`
take the body only; setup and teardown are plain static functions the body
calls itself. The calls are visible in the body — no registration-time
indirection — and, being ordinary function calls, they can take whatever
parameters and return whatever value you like.

The blessed convention:

- **Fixtureless test:** `lfg_ct_test(test);` — nothing else.
- **Test with cleanup:** call `setup()` at the top of the body and
  `teardown()` at the bottom. Assertions are non-fatal (record-and-continue;
  there is no fatal `REQUIRE`), so a trailing `teardown()` is still reached
  after a failed assertion in the body.
- **Early exit:** the only two paths that leave a body *before* the trailing
  call are an early `return` and `lfg_ct_skip(...)`. Call `teardown()`
  immediately before either. "Teardown before skip" is load-bearing — the
  skip `longjmp`s out of the body and unwinds past a trailing in-body
  `teardown()`. For the skip path, `lfg_ct_skip_cleanup(teardown, reason)`
  folds the two lines into one call (optional sugar; the explicit two-line
  form is equally valid).
- **Setup that can fail:** snapshot `lfg_ct_failure_count()` before `setup()`
  and compare after (catches a soft-fail anywhere in setup, including inside a
  helper); or, if `setup()` returns the assertion result, branch on it (every
  `ASSERT_*` returns `0` on pass, non-zero on failure). Either way run
  `teardown()` then `lfg_ct_skip(reason)` (or `teardown()` then `return` to
  count it failed). See [Failure count](#failure-count).
- **Mock-only cleanup:** `mock_reset_all()` is idempotent, so calling it at
  the top of each test is equivalent to a teardown and sidesteps the
  early-exit question entirely.

```c
static void setup(void)    { mock_reset_all(); global_state = initial_value; }
static void teardown(void) { mock_reset_all(); free(allocated_memory); }

void test_something(void)
{
    setup();
    ASSERT_EQ(expected, some_function());   /* soft-fails still reach teardown */
    teardown();
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_test(test_something);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

A suite is just a body that calls `lfg_ct_test`, so per-suite setup/teardown
is the same idea one level up — the suite body brackets its `lfg_ct_test`
calls with the shared acquire/release:

```c
void math_suite(void)
{
    suite_setup();
    lfg_ct_test(test_addition);
    lfg_ct_test(test_subtraction);
    suite_teardown();
}
```

Apply setup/teardown at whichever level fits the resource's scope: per-test
when each case needs isolation, per-suite when one acquisition is shared
across the whole batch, or nothing at all when no fixture is required.

Note the `--list` interaction for a body-owned suite setup: under `--list`
the suite body is still invoked so its contained tests can list their names
(see [Listing and filtering](#listing-and-filtering)), which means a
side-effectful `suite_setup()` would run during a plain listing. Gate it
behind `if (!lfg_ct_is_list_mode())` when it does real work.

**Crash-safety is not a teardown property.** Under
[`LFG_CT_ISOLATE_FORK`](#isolation-modes) a crashed child is discarded whole,
so a missed teardown on the crash path is harmless (the state died with the
child); under in-process mode a crash takes down the runner regardless.
Teardown is ordinary cleanup on the normal path, not a crash mechanism.

#### Deprecated: 3-argument registration (`LFG_CT_COMPAT_3ARG`)

The pre-body-only API bound setup/teardown at registration time —
`lfg_ct_test(setup, test, teardown)` and `lfg_ct_suite(setup, suite, teardown)`.
That signature is **removed** by default. For a **single-release migration
window**, defining `LFG_CT_COMPAT_3ARG` on the whole build restores the old
3-argument macros: they forward to the retained internal lifecycle helper
(`setup → body → teardown`, teardown always runs, a setup assertion failure or
`lfg_ct_skip` skips the body but still runs teardown), and every include emits
a deprecation `#warning`.

```c
/* -DLFG_CT_COMPAT_3ARG on the library build AND every consumer TU, so the
 * shared lfg_ct_test_impl / lfg_ct_suite_impl ABI signatures match. */
lfg_ct_test(my_setup, test_thing, my_teardown);   /* NULL for a skipped hook */
```

This shim exists only to avoid a flag-day migration; it is **slated for removal
after one release**. Migrate call sites to body-owned setup/teardown (above)
and drop the define. It is opt-in, so the default build stays body-only with no
`#warning`. Step-by-step recipe (the two-shaped transform, the
`tools/migrate-3arg.sh` codemod, lockstep vs staged sequencing):
[migration-3arg.md](migration-3arg.md).

### Assertion Reference

**49 assertions** covering all common C testing scenarios.

#### Pointer Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_PTR_EQUAL(expected, actual)` | Pointers are equal |
| `ASSERT_PTR_NOT_EQUAL(expected, actual)` | Pointers are not equal |
| `ASSERT_PTR_NULL(ptr)` | Pointer is NULL |
| `ASSERT_PTR_NOT_NULL(ptr)` | Pointer is not NULL |
| `ASSERT_NULL(ptr)` | Alias for `PTR_NULL` |
| `ASSERT_NOT_NULL(ptr)` | Alias for `PTR_NOT_NULL` |

> **Portability note — function pointers.** The pointer-assertion macros
> above cast their argument through `(void *)`. ISO C99 §6.3.2.3 does not
> define conversion between function pointers and `void *` (POSIX does),
> so `gcc -std=c99 -pedantic-errors` will diagnose
> `ASSERT_NULL(fn_ptr)` / `ASSERT_PTR_EQUAL(fn_a, fn_b)` etc. even though
> the assertion works correctly at runtime. Clang accepts it silently.
>
> **Workaround:** use the boolean form, which compares on the function-pointer
> type directly and stays within ISO C99:
>
> ```c
> ASSERT_TRUE(cb == NULL);       /* instead of ASSERT_NULL(cb) */
> ASSERT_TRUE(cb != NULL);       /* instead of ASSERT_NOT_NULL(cb) */
> ASSERT_TRUE(cb_a == cb_b);     /* instead of ASSERT_PTR_EQUAL(cb_a, cb_b) */
> ```
>
> A dedicated `ASSERT_FN_*` family is tracked for a future release.

#### Boolean Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_TRUE(condition)` | Condition is true |
| `ASSERT_FALSE(condition)` | Condition is false |

#### Integer Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_INT_EQUAL(expected, actual)` | Integers are equal |
| `ASSERT_INT_NOT_EQUAL(expected, actual)` | Integers are not equal |
| `ASSERT_EQ(expected, actual)` | Alias for `INT_EQUAL` |
| `ASSERT_NE(expected, actual)` | Alias for `INT_NOT_EQUAL` |
| `ASSERT_UINT_EQUAL(expected, actual)` | Unsigned integers are equal |
| `ASSERT_UINT_NOT_EQUAL(expected, actual)` | Unsigned integers are not equal |

#### Fixed-Width Integer Assertions

Signed variants (output in decimal):

| Assertion | Description |
|-----------|-------------|
| `ASSERT_INT8_EQUAL(expected, actual)` | `int8_t` values are equal |
| `ASSERT_INT8_NOT_EQUAL(expected, actual)` | `int8_t` values are not equal |
| `ASSERT_INT16_EQUAL(expected, actual)` | `int16_t` values are equal |
| `ASSERT_INT16_NOT_EQUAL(expected, actual)` | `int16_t` values are not equal |
| `ASSERT_INT32_EQUAL(expected, actual)` | `int32_t` values are equal |
| `ASSERT_INT32_NOT_EQUAL(expected, actual)` | `int32_t` values are not equal |
| `ASSERT_INT64_EQUAL(expected, actual)` | `int64_t` values are equal |
| `ASSERT_INT64_NOT_EQUAL(expected, actual)` | `int64_t` values are not equal |

Unsigned variants (output in hex with appropriate width):

| Assertion | Description |
|-----------|-------------|
| `ASSERT_UINT8_EQUAL(expected, actual)` | `uint8_t` values are equal (hex 0x00) |
| `ASSERT_UINT8_NOT_EQUAL(expected, actual)` | `uint8_t` values are not equal |
| `ASSERT_UINT16_EQUAL(expected, actual)` | `uint16_t` values are equal (hex 0x0000) |
| `ASSERT_UINT16_NOT_EQUAL(expected, actual)` | `uint16_t` values are not equal |
| `ASSERT_UINT32_EQUAL(expected, actual)` | `uint32_t` values are equal (hex 0x00000000) |
| `ASSERT_UINT32_NOT_EQUAL(expected, actual)` | `uint32_t` values are not equal |
| `ASSERT_UINT64_EQUAL(expected, actual)` | `uint64_t` values are equal (hex 0x0000000000000000) |
| `ASSERT_UINT64_NOT_EQUAL(expected, actual)` | `uint64_t` values are not equal |

#### String Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_STR_EQUAL(expected, actual)` | Strings are equal (strcmp) |
| `ASSERT_STR_NOT_EQUAL(expected, actual)` | Strings are not equal |
| `ASSERT_STRN_EQUAL(expected, actual, n)` | First n chars are equal (strncmp) |

#### Memory Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_MEM_EQUAL(expected, actual, n)` | n bytes are equal (memcmp) |
| `ASSERT_MEM_NOT_EQUAL(expected, actual, n)` | n bytes are not equal |

#### Comparison Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_GREATER_THAN(a, b)` | a > b |
| `ASSERT_LESS_THAN(a, b)` | a < b |
| `ASSERT_GREATER_OR_EQUAL(a, b)` | a >= b |
| `ASSERT_LESS_OR_EQUAL(a, b)` | a <= b |
| `ASSERT_GT(a, b)` | Alias for `GREATER_THAN` |
| `ASSERT_LT(a, b)` | Alias for `LESS_THAN` |
| `ASSERT_GE(a, b)` | Alias for `GREATER_OR_EQUAL` |
| `ASSERT_LE(a, b)` | Alias for `LESS_OR_EQUAL` |
| `ASSERT_IN_RANGE(val, min, max)` | val is within [min, max] inclusive |

#### Bit/Flag Assertions
| Assertion | Description |
|-----------|-------------|
| `ASSERT_BIT_SET(val, bit)` | Specific bit is set |
| `ASSERT_BIT_CLEAR(val, bit)` | Specific bit is clear |
| `ASSERT_BITS_SET(val, mask)` | All bits in mask are set |
| `ASSERT_BITS_CLEAR(val, mask)` | All bits in mask are clear |

#### Explicit Failure
| Assertion | Description |
|-----------|-------------|
| `ASSERT_FAIL(message)` | Unconditional failure with message |

#### Floating-Point Assertions (Optional)

These assertions require `LFG_CTEST_HAS_FLOAT` or `LFG_CTEST_HAS_DOUBLE` to be defined. See [Floating-Point Configuration](installation.md#floating-point-configuration).

**32-bit Float** (requires `LFG_CTEST_HAS_FLOAT`):

| Assertion | Description |
|-----------|-------------|
| `ASSERT_FLOAT_EQUAL(expected, actual, epsilon)` | Floats are equal within epsilon |
| `ASSERT_FLOAT_NOT_EQUAL(expected, actual, epsilon)` | Floats differ by more than epsilon |
| `ASSERT_FLOAT_GREATER_THAN(a, b)` | a > b |
| `ASSERT_FLOAT_LESS_THAN(a, b)` | a < b |
| `ASSERT_FLOAT_GREATER_OR_EQUAL(a, b)` | a >= b |
| `ASSERT_FLOAT_LESS_OR_EQUAL(a, b)` | a <= b |
| `ASSERT_FLOAT_IN_RANGE(val, min, max)` | val is within [min, max] inclusive |
| `ASSERT_FLT_EQ(e, a, eps)` | Alias for `FLOAT_EQUAL` |
| `ASSERT_FLT_NE(e, a, eps)` | Alias for `FLOAT_NOT_EQUAL` |
| `ASSERT_FLT_GT(a, b)` | Alias for `FLOAT_GREATER_THAN` |
| `ASSERT_FLT_LT(a, b)` | Alias for `FLOAT_LESS_THAN` |
| `ASSERT_FLT_GE(a, b)` | Alias for `FLOAT_GREATER_OR_EQUAL` |
| `ASSERT_FLT_LE(a, b)` | Alias for `FLOAT_LESS_OR_EQUAL` |

**64-bit Double** (requires `LFG_CTEST_HAS_DOUBLE`):

| Assertion | Description |
|-----------|-------------|
| `ASSERT_DOUBLE_EQUAL(expected, actual, epsilon)` | Doubles are equal within epsilon |
| `ASSERT_DOUBLE_NOT_EQUAL(expected, actual, epsilon)` | Doubles differ by more than epsilon |
| `ASSERT_DBL_EQ(e, a, eps)` | Alias for `DOUBLE_EQUAL` |
| `ASSERT_DBL_NE(e, a, eps)` | Alias for `DOUBLE_NOT_EQUAL` |

**Example usage:**
```c
void test_float_math(void)
{
    float result = calculate_something();
    ASSERT_FLOAT_EQUAL(3.14159f, result, 0.0001f);
    ASSERT_FLT_GT(result, 3.0f);
    ASSERT_FLOAT_IN_RANGE(result, 3.0f, 4.0f);
}

void test_double_precision(void)
{
    double pi = 3.141592653589793;
    ASSERT_DOUBLE_EQUAL(pi, acos(-1.0), 1e-15);
}
```

---

## Mocking API

The mocking framework provides macro-based mock generation for C functions.

> Tutorial: [Your first mock](tutorial/04-first-mock.md) builds a mock from
> scratch (declare/define, call count, param history, return queue);
> [chapter 9](tutorial/09-library-mocks.md) applies the same mechanics to the
> C standard library, mbedTLS, and libcurl.

### Macro Naming Convention

```
{DECLARE|DEFINE}_MOCK_{R|V}_{V|N}[_S]
```

- `DECLARE` / `DEFINE`: Header declaration vs source definition
- `R` = returns a value, `V` = void return
- Second position: `V` = no parameters, `1-9` = parameter count
- `_S` suffix = struct-safe: use **only** when a *parameter* is a struct passed
  by value. Drops `__param_actions` (`mock_param_mem_read` / `mock_param_mem_write`
  / `mock_param_str_read` / `mock_param_str_write`) in exchange for compiling
  when params can't be cast through `(void*)(size_t)`.

> **Struct return types do NOT require `_S`.** A function returning a struct
> with pointer or scalar parameters is fully supported by the plain `R_N`
> variants, which also keep `__param_actions` available. Reach for `_S` only
> when a *parameter* is a struct-by-value — never because of the return type.
> If a parameter genuinely is struct-by-value, `mock_param_mem_*` wouldn't help
> anyway (the mock receives a copy); use `__param_history[i].pX.field` to
> inspect it.

**Examples:**
- `DECLARE_MOCK_R_2` - Returns value, 2 parameters (return type may be a struct)
- `DEFINE_MOCK_V_V` - Void return, no parameters
- `DECLARE_MOCK_R_1_S` - Returns value, 1 struct-by-value parameter

### Creating a Mock

**Header file (my_module_mock.h):**
```c
#include <lfg-ctest-mock.h>

// Mock a function: int get_value(int id, const char *name);
DECLARE_MOCK_R_2(get_value, int, int, const char *);

// Optional: macro substitution for transparent mocking
#if defined(MY_MODULE_MOCK_REPLACE)
#define get_value  get_value__mock
#endif
```

**Source file (my_module_mock.c):**
```c
#include "my_module_mock.h"

DEFINE_MOCK_R_2(get_value, int, int, const char *)
```

### Mock-Generated Symbols

Each mock generates these symbols (using `get_value` as example):

| Symbol | Type | Description |
|--------|------|-------------|
| `get_value__mock(...)` | function | The mock function to call |
| `get_value__mock_reset()` | function | Reset this mock's state. Auto-registered with the framework's reset registry on first call; prefer `mock_reset_all()` for cleanup. |
| `get_value__call_count` | `size_t` | Number of times mock was called |
| `get_value__param_history[]` | array | Captured parameters from each call |
| `get_value__return_queue[]` | array | Return values (for R_* mocks) |
| `get_value__param_actions` | pointer | Parameter read/write actions |
| `get_value__callback` | function pointer | Optional callback invoked each call |
| `get_value__callback_t` | typedef | Callback function pointer type |
| `get_value_params` | typedef | Struct type for captured parameters |

### Resetting Mock State

> Tutorial: [Teardown and cleanup](tutorial/06-teardown-and-cleanup.md) covers
> `mock_reset_all()` and the `mock_register_cleanup` hook in context.

`mock_reset_all()` (declared in `<lfg-ctest-mock.h>`) is the canonical
mock-cleanup call — the teardown for mock state. Being idempotent, it works
equally at the top of each test or as the last call in a body-owned
`teardown()` (see [Setup and teardown](#setup-and-teardown)). Every
`DEFINE_MOCK_*` invocation auto-registers its `__mock_reset` thunk
with the framework's reset registry the first time the mock is called, so a
single `mock_reset_all()` sweeps every mock the test TU defined — no per-mock
bookkeeping. Mocks that own non-trivial heap state can opt their custom walker
into the same sweep via `mock_register_cleanup(void (*)(void))`; one call then
runs both the auto-generated thunks and the consumer hooks (see
[Consumer Cleanup Hooks](#consumer-cleanup-hooks)).

Always reach for `mock_reset_all()` when clearing mock state between tests.
Forgetting to add a matching per-mock reset after introducing a new
`DEFINE_MOCK_*` silently leaks state across tests; `mock_reset_all()` removes
the entire class of bug.

Per-mock `foo__mock_reset()` remains a lower-level escape hatch for the rare
case where a single mock must be reset mid-test without disturbing others — not
the recommended cleanup pattern.

### Storage Limits

Mock arrays are statically sized to `MOCK_CALL_STORAGE_MAX` (default 32). To increase:

```c
// Define before including the header
#define MOCK_CALL_STORAGE_MAX 64
#include <lfg-ctest-mock.h>
```

Exceeding the limit triggers `assert()` failure with a diagnostic message.

### Parameter History

Each call to a mock captures all parameters in `__param_history[]`. Parameters are accessed as `p0`, `p1`, `p2`, etc. (0-indexed):

```c
// For a mock: int add(int a, int b, int c);
DECLARE_MOCK_R_3(add, int, int, int, int);

// After calling: add__mock(10, 20, 30);
add__param_history[0].p0  // first call, param 'a' = 10
add__param_history[0].p1  // first call, param 'b' = 20
add__param_history[0].p2  // first call, param 'c' = 30

// After a second call: add__mock(1, 2, 3);
add__param_history[1].p0  // second call, param 'a' = 1
add__param_history[1].p1  // second call, param 'b' = 2
add__param_history[1].p2  // second call, param 'c' = 3
```

The generated `_params` typedef shows the struct layout:
```c
typedef struct { int p0; int p1; int p2; } add_params;
```

### Using Mocks in Tests

**Basic usage - verify call count and parameters:**
```c
void test_function_calls_dependency(void)
{
    // Setup: queue return values
    get_value__return_queue[0] = 42;
    get_value__return_queue[1] = -1;

    // Execute code under test
    int result = function_under_test();

    // Verify
    ASSERT_EQ(2, get_value__call_count);
    ASSERT_EQ(1, get_value__param_history[0].p0);  // first call, first param
    ASSERT_STR_EQUAL("test", get_value__param_history[0].p1);  // first call, second param
    ASSERT_EQ(100, result);

    mock_reset_all();
}
```

**Accessing captured callback parameters:**
```c
// Given: void register_callback(int id, void (*cb)(int), void *ctx);
DECLARE_MOCK_V_3(register_callback, int, void (*)(int), void *);

void test_callback_registration(void)
{
    function_under_test();  // calls register_callback internally

    // Retrieve captured callback and invoke it
    void (*captured_cb)(int) = register_callback__param_history[0].p1;
    void *captured_ctx = register_callback__param_history[0].p2;

    captured_cb(99);  // simulate callback invocation

    mock_reset_all();
}
```

### Parameter Actions (Read/Write Memory and Strings)

> Tutorial: [Parameter actions and callbacks](tutorial/05-param-actions-and-callbacks.md)
> works through capturing and injecting buffers with these functions.

For pointer parameters, capture or inject data using parameter actions:

| Function | Description |
|----------|-------------|
| `mock_param_mem_read(action, call_idx, param_idx, buf, size)` | Capture `size` bytes from parameter into `buf` |
| `mock_param_mem_write(action, call_idx, param_idx, buf, size)` | Inject `size` bytes from `buf` into parameter |
| `mock_param_str_read(action, call_idx, param_idx, buf, size)` | Capture null-terminated string from parameter into `buf` |
| `mock_param_str_write(action, call_idx, param_idx, str, size)` | Inject null-terminated string into parameter buffer |
| `mock_param_destroy(action)` | Free action chain (called automatically by `_reset`) |

The `str_` variants use `snprintf` instead of `memcpy`, so they stop at the null
terminator and never read past it. Use them for `const char*` parameters where
`mem_read` would over-read short string literals (e.g., ASAN redzone violations).

**Capture data from a pointer parameter:**
```c
void test_capture_buffer_contents(void)
{
    uint8_t captured_data[16] = {0};

    // Capture 16 bytes from parameter 1 of call 0
    i2c_write__param_actions = mock_param_mem_read(
        NULL,           // action chain (NULL to start new)
        0,              // call_index (0-based)
        1,              // parameter_index (0-based)
        captured_data,  // destination buffer
        16              // byte count
    );

    function_under_test();  // calls i2c_write internally

    // Verify captured data
    ASSERT_UINT8_EQUAL(0x01, captured_data[0]);
    ASSERT_UINT8_EQUAL(0x80, captured_data[1]);

    mock_reset_all();
}
```

**Inject data into a pointer parameter:**
```c
void test_inject_read_data(void)
{
    uint8_t inject_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

    // Write 4 bytes to parameter 2 of call 0
    i2c_read__param_actions = mock_param_mem_write(
        NULL,         // action chain
        0,            // call_index
        2,            // parameter_index (the rx_buf pointer)
        inject_data,  // source buffer
        4             // byte count
    );

    function_under_test();  // calls i2c_read, receives injected data

    mock_reset_all();
}
```

**Chaining multiple actions:**
```c
mock_param_action_t action = NULL;
action = mock_param_mem_read(action, 0, 1, buf1, 4);   // call 0, param 1
action = mock_param_mem_write(action, 0, 2, buf2, 4);  // call 0, param 2
action = mock_param_mem_read(action, 1, 1, buf3, 4);   // call 1, param 1
my_func__param_actions = action;
```

**Capture string parameters across multiple calls:**
```c
void test_captures_directory_names(void)
{
    char cap[3][256] = {{0}};
    mock_param_action_t action = NULL;

    // Capture the const char* param from 3 consecutive calls
    for (int i = 0; i < 3; i++)
    {
        action = mock_param_str_read(action, i, 0, cap[i], sizeof(cap[i]));
    }
    mkdir__param_actions = action;

    function_under_test();  // calls mkdir 3 times

    ASSERT_STR_EQUAL("src",     cap[0]);
    ASSERT_STR_EQUAL("src/lib", cap[1]);
    ASSERT_STR_EQUAL("src/bin", cap[2]);

    mock_reset_all();
}
```

### Mock Callbacks

> Tutorial: [Parameter actions and callbacks](tutorial/05-param-actions-and-callbacks.md#the-callback-hook)
> demonstrates side-effect-only and state-driven callbacks.

Each mock has an optional callback that fires on every call, receiving the call index and all parameters. Use callbacks when you need custom side effects during a mock call (e.g., invoking a captured write callback with response data) or when the mock's return value depends on per-call state the test (or the SUT) seeded.

The callback typedef is generated per-mock. R_* callbacks receive an extra `_rtype *return_override` argument that points at the value about to be returned (already loaded from `__return_queue[i]` by the time the callback runs). V_* callbacks have no return to control, so they get only the call index and parameters:

```c
// For DECLARE_MOCK_V_2(set_value, int, const char *):
//   typedef void (*set_value__callback_t)(size_t call_index, int p0, const char *p1);
//   extern set_value__callback_t set_value__callback;

// For DECLARE_MOCK_R_2(get_value, int, int, const char *):
//   typedef void (*get_value__callback_t)(size_t call_index, int *return_override, int p0, const char *p1);
//   extern get_value__callback_t get_value__callback;
```

The override is opt-in per call: writing to `*return_override` replaces the queue value for this call; not writing leaves the queue value in place. A side-effect-only callback can simply ignore the pointer.

**Example: side-effects only, queue-driven return.** The callback observes parameters and triggers an external action; it doesn't write to `*return_override`, so the queue value flows through unchanged:
```c
static void on_perform(size_t call_index, CURLcode *return_override, CURL *handle)
{
    (void)call_index;
    (void)return_override; /* leave queue value in place */
    (void)handle;
    /* Look up the write callback captured from curl_easy_setopt,
       then feed it the mock response body */
    write_cb("HTTP 200 OK", 1, 11, write_cb_userdata);
}

void test_http_request(void)
{
    mock_reset_all();
    curl_easy_perform__return_queue[0] = CURLE_OK;
    curl_easy_perform__callback = on_perform;

    function_under_test();

    ASSERT_EQ(1, curl_easy_perform__call_count);
    mock_reset_all();
}
```

**Example: state-driven return.** No queue priming; the callback computes the result from external state at call time:
```c
static void on_is_file(size_t call_index, int *ret, const char *path)
{
    (void)call_index;
    *ret = seeded_state_has(path);
}

void test_state_driven_query(void)
{
    mock_reset_all();
    is_file__callback = on_is_file;

    seed_state("a");        /* "a" exists, "c" does not */
    process_manifest("ab,c");

    mock_reset_all();
}
```

The callback can also read `*return_override` mid-body to inspect the queue-loaded value before deciding whether to override - useful for "modify if state says X" patterns.

**Callback execution order** within the mock body:
1. Overflow check
2. Store param_history
3. Read return value from queue (R_* only)
4. Process param_actions (if applicable)
5. Invoke callback (R_*: callback may write through `return_override` to replace step 3)
6. Increment call_count
7. Return

The callback sees the same `call_index` used for param_history indexing (pre-increment). Reset (`__mock_reset`) sets the callback to NULL.

### Struct-Safe Mocks

When function parameters include structs passed by value, use `_S` suffix macros:

```c
typedef struct { int x; int y; } point_t;

// Header
DECLARE_MOCK_R_1_S(calculate_distance, float, point_t);

// Source
DEFINE_MOCK_R_1_S(calculate_distance, float, point_t)

// Test
void test_with_struct_param(void)
{
    calculate_distance__return_queue[0] = 5.0f;

    point_t p = {3, 4};
    float result = calculate_distance__mock(p);

    ASSERT_EQ(3, calculate_distance__param_history[0].p0.x);
    ASSERT_EQ(4, calculate_distance__param_history[0].p0.y);

    mock_reset_all();
}
```

**Note:** `_S` variants do not support parameter actions (`mock_param_mem_read/write`, `mock_param_str_read/write`).

### Mock Limitations

- Maximum 9 parameters per function
- Standard mocks cast parameters to `void*` via `size_t` - use `_S` variants for struct-by-value
- See [Storage Limits](#storage-limits) for call history size (default 32)

### Consumer Cleanup Hooks

A mock TU whose mock owns nontrivial heap state (return queues holding allocated
blobs, etc.) the framework can't reach on its own can register a cleanup walker
via `mock_register_cleanup(void (*)(void))`. The walker is invoked alongside the
framework-generated `__mock_reset` thunks on every `mock_reset_all()` call —
single-function teardown, no two-step ritual for test authors.

Because `mock_reset_all()` clears the registry, cleanup hooks must re-register
after each reset cycle (same lifecycle as auto-registered thunks). Ordering
between the two classes is unspecified.

See [architecture.md](architecture.md) for the implementation rationale.
