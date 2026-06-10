# 2. Running and filtering

A single test binary often backs many independent test cases. `lfg_ct_parse_args`
lets you list the registered names and select a subset by glob from the command
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
    lfg_ct_suite(NULL, math_suite, NULL);
    lfg_ct_suite(NULL, string_suite, NULL);
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
| `--list` | Print every test/suite name encountered, one per line on stdout. |
| `--filter <glob>` | Run only entries whose name matches the `fnmatch(3)` glob (`*`, `?`, `[...]`). Repeat to OR-combine. |
| `--filter-exclude <glob>` | Skip entries whose name matches. Repeat to OR-combine. **Exclude wins** if both match the same name. |
| `--strict-xpass` | Make an otherwise-clean run exit non-zero if any test XPASSed (see [chapter 3](03-skip-xfail-xpass.md)). |
| `-v`, `--verbose` | Stream a per-test progress line. |

## Listing what a binary contains

`--list` walks the registrations and prints names without running any test
bodies' assertions:

```bash
$ ./test_indicators --list
math_suite
test_add_positive
test_add_negative
string_suite
test_trim
test_split
```

Suite bodies are still *invoked* under `--list` (that is how the tests inside
them get a chance to announce themselves), but the setup/teardown of suites
and tests are skipped. If you print your own diagnostics from inside a suite
body, gate them so they don't pollute the clean name list:

```c
if (!lfg_ct_is_list_mode())
{
    fprintf(stderr, "building fixture data...\n");
}
```

## Filtering by name

`--filter` takes a shell-style glob matched against the registered name.
Matching a *suite* name pulls in every test inside it — which is what makes
the one-binary-many-entries pattern work:

```bash
./test_indicators --filter 'math_suite'      # the suite and all its tests
./test_indicators --filter 'test_add_*'      # just the add tests
./test_indicators --filter 'test_*' --filter-exclude '*_slow'
```

An unmatched filter is **not** an error: zero tests run and the binary exits
`0`. Unknown *flags*, by contrast, are fatal (non-zero) — the distinction
keeps a selective CI shard that happens to match nothing from being mistaken
for a configuration error.

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

## Where to go next

- [Chapter 3 — Skip, xfail, xpass](03-skip-xfail-xpass.md): outcomes beyond
  pass/fail, and what `--strict-xpass` is for.
- [Chapter 8 — Integration](08-integration.md): the full CMake/CTest wiring.
