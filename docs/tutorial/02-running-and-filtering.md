# 2. Running and filtering

A single test binary often backs many independent test cases. `lfg_ct_parse_args`
lets you list the registered entries and select a subset by glob from the command
line — so one executable can answer to many `ctest` entries, run in parallel,
and stay scriptable in CI.

This chapter assumes the runner skeleton from
[chapter 1](01-first-test.md).

## Wiring `lfg_ct_parse_args` into `main`

Parse the command line once, at the very top of `main`, before
`lfg_ct_start()`:

```c
#include <lfg-ctest.h>

int main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;   /* unknown flag: usage already printed to stderr */
    }

    lfg_ct_start();
    lfg_ct_suite(math_suite);
    lfg_ct_suite(string_suite);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

`lfg_ct_parse_args` returns `0` on success and non-zero on a bad flag (with a
usage message on stderr). Propagate that non-zero out of `main` so a typo in a
CI invocation fails loudly instead of silently running everything.

The flags it recognises:

| Flag | Effect |
|------|--------|
| `--list` | Print every test/suite **id** encountered, one per line on stdout. |
| `--filter <glob>` | Run only entries whose id matches the `fnmatch(3)` glob (`*`, `?`, `[...]`), which addresses as many trailing `::`-components as it spells out. Repeat to OR-combine. |
| `--filter-exclude <glob>` | Skip entries whose id matches, same rule. Repeat to OR-combine. **Exclude wins** if both match the same entry. |
| `--strict-xpass` | Make an otherwise-clean run exit non-zero if any test XPASSed (see [chapter 3](03-skip-xfail-xpass.md)). |
| `--seed <n>` | Seed `rand(3)` with `n` to replay a previous run's random scenarios (see [Reproducing a randomized run](#reproducing-a-randomized-run) below). |
| `-v`, `--verbose` | Stream a per-test progress line. |

## Entry ids

Test names are not unique inside a binary, and neither are suite names — a
project with one `test_roundtrip` per translation unit, or forty files each
declaring their own `static void suite_e2e(void)`, is normal. So every
registered entry also has an **id**:

```
<file>::<suite>::<test>      a test
<file>::<suite>              a suite
```

`<file>` is the **basename** of the registration site's `__FILE__`, so ids do
not change between build directories or under an out-of-tree build. Nothing in
an id derives from run order or an address: a `--list` run and the filtered run
that follows it always agree.

A component that does not exist is spelled `(none)` rather than left empty — a
test registered outside any suite is `foo.c::(none)::test_x`, never a malformed
`foo.c::::test_x`.

Two entries only collide after full qualification if the same test name is
registered twice in the same suite in the same file. The framework does not
diagnose that; it is a duplicate registration, and the id addresses both.

## Listing what a binary contains

`--list` walks the registrations and prints ids without running any test
bodies' assertions:

```bash
$ ./test_indicators --list
math.c::math_suite
math.c::math_suite::test_add_positive
math.c::math_suite::test_add_negative
strings.c::string_suite
strings.c::string_suite::test_trim
strings.c::string_suite::test_split
```

> **Output change.** Before ids existed, `--list` printed bare names. It now
> prints ids, and terminates each line with a bare `\n` (the human-facing
> report still uses `\r\n`) so the listing pipes cleanly. A script that parsed
> the old output needs updating; one that fed it straight back into `--filter`
> keeps working, because a bare name is still a valid way to name an entry.

Each line is directly usable as a `--filter` argument, which is what makes the
list-then-run pipeline work:

```bash
./test_indicators --list | grep roundtrip | xargs -I{} ./test_indicators --filter '{}'
```

Suite bodies are still *invoked* under `--list` (that is how the tests inside
them get a chance to announce themselves), but individual test bodies are not
run — so any setup, teardown, or assertions they contain never execute. If you
print your own diagnostics from inside a suite body, gate them so they don't
pollute the clean id list:

```c
if (!lfg_ct_is_list_mode())
{
    fprintf(stderr, "building fixture data...\n");
}
```

## Filtering

`--filter` takes a shell-style glob. It addresses **exactly as many trailing
components of the id as it spells out** — so `test`, `suite::test`, and
`file.c::suite::test` all name the same entry, and you can qualify only as far
as you need to make the selection unique:

```bash
./test_indicators --filter 'test_add_positive'                 # every test of that name
./test_indicators --filter 'math_suite::test_add_positive'     # ...in that suite
./test_indicators --filter 'math.c::math_suite::test_add_positive'  # exactly one
```

A glob with no `::` is matched against the test name alone, so **every glob
that worked before ids existed selects exactly what it selected before** —
existing `add_test(... --filter "suite_sma_*")` shards need no edit, wildcards
included.

Matching a *suite* pulls in every test inside it, which is what makes the
one-binary-many-entries pattern work:

```bash
./test_indicators --filter 'math_suite'      # the suite and all its tests
./test_indicators --filter 'test_add_*'      # just the add tests
./test_indicators --filter 'test_*' --filter-exclude '*_slow'
```

`*` never crosses a `::` — it fills exactly one component. That is what keeps
`--filter 'test_*'` above meaning what it always meant: in the near-universal
`test_*.c` naming convention it would otherwise match the *file* component of
every id in the file (and the suite id, pulling in the whole suite). To reach
the file, spell it out:

```bash
./test_indicators --filter 'math.c::*::*'        # every test in that file
./test_indicators --filter '*::test_add_positive'  # that test in any suite
```

An unmatched filter is **not** an error: zero tests run and the binary exits
`0`. Unknown *flags*, by contrast, are fatal (non-zero) — the distinction
keeps a selective CI shard that happens to match nothing from being mistaken
for a configuration error.

Under the single-header amalgamation the id is unaffected: the registration
macros expand at *your* call site, so `__FILE__` is still your test file, not
the amalgamated header.

## One binary, many CTest entries

This is the payoff. Register the same executable under several `add_test`
names, each with its own `--filter`, and `ctest --parallel` schedules them as
independent units:

```cmake
foreach(ind sma ema wma)
  add_test(NAME test_ind_${ind}
           COMMAND test_indicators --filter "suite_${ind}_*")
endforeach()
```

Each shard reports independently, fails independently, and runs concurrently —
without compiling a separate binary per indicator.

For a consumer that wants to short-circuit expensive work *outside* the
framework (opening a database, spinning up a fixture) when the current filter
would skip a test anyway, `lfg_ct_name_runs(name)` answers "would an entry
named `name` actually run right now?":

```c
if (lfg_ct_name_runs("test_db_roundtrip"))
{
    db = open_test_database();   /* skip the cost when filtered out */
}
```

`lfg_ct_name_runs` keeps its bare-name meaning, and that is also its limit: it
builds the id from whatever file/suite context is ambient, and in `main()`
there is none — so it answers against `(none)::(none)::test_db_roundtrip`. A
bare-name filter still resolves correctly, but under a *qualified* filter like
`--filter 'alpha.c::suite_one::test_db_roundtrip'` it returns 0 while the test
in fact runs, silently skipping the setup you were gating.

Use the `lfg_ct_test_runs` macro instead whenever a qualified filter is
possible — it captures `__FILE__` so the query sees the same id `--list`
prints:

```c
if (lfg_ct_test_runs("test_db_roundtrip"))
{
    db = open_test_database();
}
```

## Verbose progress

`-v` / `--verbose` streams the runner's per-test progress to stdout, in the
spirit of `ctest -V`. Before each test body it prints a `START` banner, and
after the test classifies it prints the outcome with elapsed milliseconds:

```
*** START: math_suite::test_add_positive
*** PASS: math_suite::test_add_positive (0.012 ms)
*** START: math_suite::test_add_negative
*** FAIL: math_suite::test_add_negative (0.034 ms): ASSERT_EQ(-5, add(-2, -3))
```

It is off by default (output is byte-identical to a build without the flag),
purely opt-in, and orthogonal to the other flags. It is also implemented as
the framework's first built-in *reporter*, so it coexists with a consumer
reporter such as JUnit-XML — both fire in the same run without interfering
(see [chapter 8](08-integration.md)). `lfg_ct_is_verbose()` exposes the bit if
your own code wants to suppress redundant output when verbose mode is on.

## Reproducing a randomized run

`lfg_ct_start()` seeds `rand(3)` for you and prints the seed it picked:

```
*** random seed is 3314123391
```

If your tests draw scenarios from `rand()` — a random symbol, a random date —
that variety is worth having, but a failure you cannot replay is not
diagnosable. `--seed <n>` replays it: copy the seed off the failing run and
hand it back.

```bash
$ ./test_indicators                    # *** random seed is 3314123391 -- one failure
$ ./test_indicators --seed 3314123391  # same scenario every time
```

The banner always reports the seed actually in use, so the replay prints the
same line as the run it reproduces. Without the flag the seed is generated
fresh per run across the full `unsigned` range — two runs launched in the same
second still differ. `0` is a legal seed, not "unset"; a missing, non-numeric,
or too-large value is a fatal flag error.

To drive your own RNG off the same flag, read it back after parsing:

```c
if (lfg_ct_is_seed_set())
{
    my_rng_seed(lfg_ct_get_seed());
}
```

## Rerunning just the failures

A big suite that comes back with a wall of failures leaves you with a bad
choice: re-run all 655 tests to check one fix, or hand-assemble a `--filter`
off the output. `--rerun-failed` is the third option.

Every run — this one included — quietly writes what it learned to
`.lfg-ctest-last` in the working directory:

```
lfg-ctest-state 1
seed 3314123391
fail test-ind-sma.c::suite_sma::test_roundtrip
fail test-ind-ema.c::suite_ema::test_warmup
```

The seed the run used, and one [entry id](#entry-ids) per failure — the same
ids `--list` prints. `--rerun-failed` reads it back and replays exactly that
set:

```bash
$ ./test_indicators                 # 169 failures out of 655, two minutes
$ ./test_indicators --rerun-failed  # just those 169, in seconds
```

Note that the seed comes back too. That is the point: if your tests draw
scenarios from `rand()`, replaying *which* tests failed without replaying the
conditions they failed under is not a reproduction at all. Pass `--seed`
explicitly when you *do* want the same selection under fresh conditions — the
flag you typed wins over the persisted one.

Each rerun rewrites the file with its own failures, so the loop converges:

```bash
$ ./test_indicators --rerun-failed  # 169 -> 12 still failing
$ ./test_indicators --rerun-failed  # 12 -> 3
$ ./test_indicators --rerun-failed  # 3 -> 0, and you are done
```

Narrow further by combining it with `--filter`; the two intersect, so the
filter picks from the replayed set rather than from the whole suite:

```bash
$ ./test_indicators --rerun-failed --filter 'suite_sma::*'
```

One caveat: that run rewrites the state file with only the intersected
subset's failures, so the 157 keys the filter excluded are gone from the
record, not parked. If you want to chip at one suite without spending the
other 157, give the narrowed run its own `--state-file`.

The flag never guesses. If there is no state file, or it is malformed, or none
of its keys still name a test in the binary, you get a diagnostic on stderr
and a non-zero exit — never a quiet full-suite run that you would read as a
narrowed one. A key that no longer resolves on its own (you renamed the test,
you rebuilt the binary) is just a warning; the rest still run. And if the
previous run was green, the rerun says so and exits 0.

Two housekeeping notes: `--list` deliberately leaves the state file alone, so
listing between reruns is safe, and `--state-file <path>` moves the file when
the working directory is not where you want build artifacts. Add
`.lfg-ctest-last` to your `.gitignore`.

## Where to go next

- [Chapter 3 — Skip, xfail, xpass](03-skip-xfail-xpass.md): outcomes beyond
  pass/fail, and what `--strict-xpass` is for.
- [Chapter 8 — Integration](08-integration.md): the full CMake/CTest wiring.
