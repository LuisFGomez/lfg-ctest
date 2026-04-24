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
```

`test-unified` intentionally exercises assertion failure paths (wrapped in
expect-failures mode), so a clean run is "all passed" even though the binary
internally triggered many failures.

`test-amalg` builds against the generated `dist/lfg-ctest.h` and does not link
against the `lfg-ctest` static library — it provides its own impl via
`LFG_CTEST_IMPLEMENTATION`. CTest registers it alongside the other two.

### Run one specific test

Test functions are plain `void fn(void)` compiled into the binary — there is
no per-test CLI filter. To run just one:

1. Edit the test's `main()` or suite function to register only the case(s)
   you care about.
2. Rebuild and rerun the binary.

Don't commit these edits. They're a local iteration tool, not a workflow.

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
lfg_ctest(test_my_new_assert_detects_mismatch);
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

    widget_read__mock_reset();
}
```

Register in the appropriate suite. Reset at the end of each test (or in a
per-test teardown helper) — state is file-scope and persists across tests
otherwise.

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

Public surface (see `README.md`):

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
