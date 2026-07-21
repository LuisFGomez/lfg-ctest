# build-and-test.md — lfg-ctest

Concrete commands and patterns for working on the framework itself.

## Build

```
cmake --preset debug           # configures ./build with Ninja, Debug
cmake --build build            # builds libs + self-tests
```

The only preset is `debug` (see `CMakePresets.json`).

### When built as a subproject

`CMakeLists.txt` guards self-tests and `LFG_CTEST_SELF_TEST` on
`CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR`. When a consumer does
`add_subdirectory(deps/lfg-ctest)`:

- `test-unified` and `test-mock` are not built.
- `LFG_CTEST_SELF_TEST` is not defined → `lfg_ct_expect_failures_*` is not
  exposed.
- The configuration summary block is skipped.

You don't need to do anything to get this behavior — it's automatic.

### Disabling float / double

```
cmake --preset debug -DLFG_CTEST_ENABLE_FLOAT=OFF -DLFG_CTEST_ENABLE_DOUBLE=OFF
cmake --preset debug -DLFG_CTEST_ENABLE_DOUBLE=OFF   # float only
```

Defaults are ON, auto-detected against `math.h` / `fabsf` / `fabs`.

## Run tests

```
ctest --test-dir build --output-on-failure    # run all via CTest
cmake --build build --target run_all_tests    # verbose wrapper
./build/test-unified                          # direct, core self-tests
./build/test-mock                             # direct, mock self-tests
./build/test-amalg                            # amalgamated-header smoke
./build/test-quiet                            # quiet output contract
```

`test-unified` intentionally exercises assertion failure paths (wrapped in
expect-failures mode), so a clean run is "all passed" even though the binary
internally triggered many failures.

`test-amalg` builds against the generated `dist/lfg-ctest.h` and does not link
against the `lfg-ctest` static library — it provides its own impl via
`LFG_CTEST_IMPLEMENTATION`. CTest registers it alongside the other two.

`test-quiet` covers what `-q` / `--verbosity 0` actually put on stdout, which
the parse-level tests in `test-unified` cannot see. Each case runs a scenario in
a forked child whose stdout is a regular file, then greps the captured bytes,
and every "quiet suppresses X" assertion is paired with a default-verbosity
control so a case cannot pass by capturing nothing. Registered under `if(UNIX)`:
the capture needs `fork` / `waitpid` / `mkstemp`. The children drive genuinely
unsuppressed failures and `_exit()` with a verdict code, so the outer binary
stays green without expect-failures mode.

`test-id-roundtrip` is not a C binary: it is `tools/check-id-roundtrip.sh`,
which runs `--list | grep | xargs --filter` against `test-amalg` for real and
asserts each listed id selects exactly one test. The matching *rule* is covered
in-process by `test-unified`; only a subprocess can cover the stdout bytes the
pipeline actually depends on. Registered under `if(UNIX)` since it needs a
POSIX shell.

`test-rerun-failed` is the same shape: `tools/check-rerun-failed.sh`, also
`if(UNIX)`. It covers the parts of `--rerun-failed` that only exist at process
scope — that a normal exit persists `.lfg-ctest-last` even with stdout
redirected to a file, that the persisted seed matches the announced one, that
`--list` leaves the file untouched, and that each error path (missing,
malformed, wholly stale state file) exits non-zero instead of running the whole
suite. Parsing, selection, seed precedence and the narrowing cycle are covered
in-process by `test-unified`; fork-mode persistence by `test-fork`.

Every self-test binary persists a rerun state file on exit, so `CMakeLists.txt`
gives each `add_test` its own `--state-file` under the build directory:
sharing the default `.lfg-ctest-last` would have them clobber one another
under `ctest -j`. No test outcome reads the file — this just keeps the
artifact honest, and dogfoods the flag the docs recommend for out-of-tree
builds. `test-unified` and `test-fork` re-parse their real `argv` right before
the final summary, because their arg-parsing self-tests reset the parsed state
(including `--state-file`) as a side effect. Self-tests that drive a *nested*
`lfg_ct_print_summary` aim it at their throwaway path for the same reason.

`test-fork` also clears its recorded set before the summary, because the
intentional fork-mode failures it drives are real classified outcomes and
would otherwise describe a red run.

### Run one specific test

Every registered entry has an id — `<file>::<suite>::<test>` — and `--filter`
addresses as many trailing `::`-components as the glob spells out. Ask the
binary what it contains, then name one:

```
./build/test-unified --list
./build/test-unified --filter 'test-unified.c::suite_filter_args_tests::test_id_bare_name_glob_still_selects'
```

No source edit, no rebuild. See
[docs/api.md — Entry ids](api.md#entry-ids) for the format and the
component-depth rule.

Note that `test-unified`'s own filter tests call `lfg_ct_parse_args` mid-run to
exercise the parser, which resets the filter for everything registered after
them — so a filtered run of *that* binary reports more tests than it selected.
Every other self-test binary filters normally.

## Add a self-test

### For the core framework (in `test-unified.c`)

Most new core self-tests need to verify that an assertion macro fails when it
should. Use the expect-failures pattern:

```c
static void test_my_new_assert_detects_mismatch(void)
{
    int expected_failures = 1;
    int actual_failures;

    lfg_ct_expect_failures_begin();
    ASSERT_MY_NEW_THING(1, 2);          /* should fail */
    actual_failures = lfg_ct_expect_failures_end();
    ASSERT_INT_EQUAL(expected_failures, actual_failures);
}
```

Then register it in whichever suite function is appropriate:

```c
lfg_ct_test(test_my_new_assert_detects_mismatch);
```

Positive cases (assertion passes when it should) don't need expect-failures —
just call the assertion directly inside a `void` test.

### For the mock framework (in `test-mock.c`)

Declare and define the mock at file scope, then write a test that exercises
it:

```c
DECLARE_MOCK_R_2(widget_read, int, uint8_t, uint8_t *);
DEFINE_MOCK_R_2(widget_read, int, uint8_t, uint8_t *)

static void test_widget_read_captures_params(void)
{
    widget_read__return_queue[0] = 0;

    uint8_t buf[4];
    (void)widget_read__mock(0x42, buf);

    ASSERT_EQ(1, widget_read__call_count);
    ASSERT_UINT8_EQUAL(0x42, widget_read__param_history[0].p0);

    mock_reset_all();
}
```

Register in the appropriate suite. Call `mock_reset_all()` at the end of each
test (or in a per-test teardown helper) — state is file-scope and persists
across tests otherwise. Every `DEFINE_MOCK_*` auto-registers its
`__mock_reset` thunk, so the single call sweeps every mock the TU defined;
see [api.md — Resetting Mock State](api.md#resetting-mock-state).

## Amalgamation

The single-header form at `dist/lfg-ctest.h` is generated, not committed
(`dist/` is gitignored). Regenerate it whenever you touch the split sources
or the manifest:

```
cmake --build build --target amalgamate
```

The `amalgamate` target depends on `tools/amalgamate.c`, the manifest, and
every source listed in `LFG_CTEST_AMALG_SOURCES` in `CMakeLists.txt`. Adding
a new split source file means updating **both** that list and
`tools/amalgamate.manifest` — otherwise the new file is silently omitted from
the generated header, and `test-amalg` will fail to link as soon as it
references anything from the missing file.

`test-amalg` runs automatically as part of `ctest`. When investigating drift,
run it in isolation to see the symbol that went missing:

```
cmake --build build --target test-amalg
./build/test-amalg
```

The amalgamator's guard-stripping assumes include guards end in `_H_`
(trailing underscore — this repo's convention). If you add a header whose
guard doesn't follow that convention, the tool will emit both the `#ifndef`
and the paste-in content, which typically shows up as duplicate-definition
errors. Fix by renaming the guard, not by tweaking the tool.

## Versioning

`lfg-ctest.h` transitively includes `lfg-ctest-version.h`, a generated header
produced by `tools/mkversion.c` at build time. The tool runs
`git -C <src> describe --match 'v*'` and emits C macros for the captured
version. The CMake side pipes stdout through `copy_if_different` so a rebuild
only retriggers downstream compilation when the version actually changed.

Public surface (see [api.md](api.md)):

- `LFG_CTEST_VERSION` / `LFG_CTEST_VERSION_FULL` / `LFG_CTEST_VERSION_MAJOR|_MINOR|_PATCH`
  (compile-time).
- `const char *lfg_ct_version(void)` (runtime, returns `_FULL`).

Tagging convention (two tiers):

1. **`v<M>.<m>`** — the rolling base tag. Annotated, e.g.
   `git tag -a v0.1 -m "..." <sha>`. Governs everyday in-development builds;
   `git describe` produces `v<M>.<m>-<distance>-g<shorthash>` and the
   stamper emits `M.m.<distance>+<shorthash>`.
2. **`release-v<M>.<m>.<p>`** — an immutable release marker. Annotated,
   placed on whichever commit is being shipped. The stamper tries
   `git describe --match 'release-v*' --exact-match` first; on a hit it
   strips the prefix and emits a clean `M.m.p` with no `+<sha>` trailer
   (rationale: releases are fixed points; the hash is redundant noise and
   semver 2.0 treats `+build` as metadata tools should ignore).

HEAD not on a release tag → fall through to tier 1. Lightweight tags in
either form are ignored by `git describe`.

No git tag, or lfg-ctest vendored into a downstream without `.git` → the
macros fall back to `0.0.0`. This is intentional so `add_subdirectory`
embedders who ship tarballs still compile.

Cutting a release:

```
cmake --build build --target release-tag
```

The `release-tag` target runs `tools/mkrelease.c`, which reads the current
`git describe` output, proposes the matching `release-v<M>.<m>.<p>`, and
prompts for confirmation before execing `git tag -a <tag>` (so `$EDITOR`
handles the annotation message, same as `git tag -a` by hand). Then:

```
git push origin release-v0.1.49    # triggers the CI release job
```

The target exists because hand-typing the version from a moving
`git describe` output kept drifting — the distance number advances with
every commit, and a stale tag silently ships the wrong version string.

Direct alternatives if scripting or the prompt is in the way:

```
git tag -a release-v0.1.49 -m "release 0.1.49: <summary>"         # manual
```

Gitea's release UI can also create the tag+release atomically — pick the
target branch/commit in the dropdown.

`mkversion.c` is deliberately prefix-agnostic — invoked as
`lfg_ct_mkversion LFG_CTEST <src>` — so it can be lifted into a shared
project once a third consumer shows up. See the jotsms repo for the second
inline copy of the same pattern; converge via `git mv` + a small
`.cmake` helper when extraction is warranted.

## Formatting

`.clang-format` at the repo root is authoritative. Apply:

```
clang-format -i lfg-ctest.c lfg-ctest.h \
                lfg-ctest-mock.c lfg-ctest-mock.h \
                test-unified.c test-mock.c
```

**Not wired into CMake or CI.** It's a developer-invoked check — run it
before committing if you've touched any of those files.

## CI

`.gitea/workflows/ci.yml` runs on every push. On a self-hosted runner
(`popstation, host`) it configures with the `debug` preset, builds, then
runs `ctest --test-dir build --output-on-failure`. Same commands as local —
no CI-only build flags.
