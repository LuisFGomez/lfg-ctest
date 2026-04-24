# ai.md — lfg-ctest

**What this is.** Source for the `lfg-ctest` C99 test + mocking framework
itself — a single static library (`lfg_ctest`) that consumers embed via
`add_subdirectory`. You are most likely editing the framework, not using it.
Consumer-facing API is fully documented in `README.md`; this file orients you
to the code.

## TL;DR

- C99, zero dependencies (optional `libm` only when float/double asserts are enabled).
- Two concerns, one library: test runner + asserts (`lfg_ctest.[ch]`) and
  mock-generation macros + runtime (`lfg_ctest_mock.[ch]`).
- Build: CMake + Ninja, preset `debug`. Self-tests (`test_unified`, `test_mock`,
  `test_amalg`) only build when this repo is the top-level CMake source.
- Framework asserts verify themselves via `lfg_ct_expect_failures_begin/end` —
  a self-test-only mode that swallows expected failure output and returns the
  failure count. Gated on `LFG_CTEST_SELF_TEST`.
- Mock macros fan out by parameter count (`V_V`..`V_9`, `R_V`..`R_9`) plus `_S`
  variants for struct-by-value params. Non-`_S` casts params through
  `(void*)(size_t)` and can't compile for structs; `_S` drops `__param_actions`
  in exchange.
- A C99 amalgamator (`tools/amalgamate.c` + `amalgamate.manifest`) concatenates
  the split sources into a single-header form at `dist/lfg_ctest.h` (gitignored,
  regenerated on demand). Consumers define `LFG_CTEST_IMPLEMENTATION` in one TU.
  `test_amalg` compiles against the generated header and catches drift.
- `README.md` is the authoritative user spec. Mirror any user-visible change there.

## Layout

| Path | What's there |
|------|--------------|
| `lfg_ctest.h` / `.c` | Test runner, assertion macros, optional float/double asserts (gated on `LFG_CTEST_HAS_FLOAT` / `_HAS_DOUBLE`). |
| `lfg_ctest_mock.h` / `.c` | Mock declaration/definition macros, `mock_param_*` runtime, `mock_reset_all()` registry. |
| `test_unified.c` | Self-test for the core framework. Built standalone with `LFG_CTEST_SELF_TEST=1`. |
| `test_mock.c` | Self-test for the mock framework. Same gating. |
| `test_amalg.c` | Smoke test for the amalgamated single-header. Defines `LFG_CTEST_IMPLEMENTATION` itself; does not link against the static lib. |
| `tools/amalgamate.c` | C99 amalgamator. Concatenates sources per manifest, strips internal includes and `_H_`-suffixed include guards, dedupes system includes, wraps in `LFG_CTEST_IMPLEMENTATION` gate. |
| `tools/amalgamate.manifest` | Ordered list of header + impl files to fold into `dist/lfg_ctest.h`. |
| `dist/lfg_ctest.h` | Generated single-header (gitignored). Built by the `amalgamate` CMake target. |
| `CMakeLists.txt` | Float/double auto-detection, static-lib build, self-test targets, amalgamator target, install rules. |
| `CMakePresets.json` | `debug` preset → `build/` with Ninja. |
| `.clang-format` | Project formatter config (BSD/Allman, 4-space, 120 col). Copied from `~/.clang-format`; authoritative for this repo. |
| `README.md` | Public API reference (assertions, mock macros, examples). Authoritative. |
| `samples/` | Untracked example consumer (I2C LED driver). Not shipped, not in CI. |

## Pointers

| Doc | Covers |
|-----|--------|
| [.ai/architecture.md](.ai/architecture.md) | Core runner state, mock macro fanout, `_S` vs non-`_S`, param-action chain, reset registry, `LFG_CTEST_SELF_TEST`, expect-failures mode. |
| [.ai/build-and-test.md](.ai/build-and-test.md) | Build, run a single self-test, add new self-test cases (including the expect-failures pattern), formatting. |
| [README.md](README.md) | Public API. Mirror user-visible changes here. |

## Commands

```
cmake --preset debug                          # configure into ./build
cmake --build build                           # build library + self-tests
ctest --test-dir build --output-on-failure    # run all self-tests
cmake --build build --target run_all_tests    # same, via custom target
cmake --build build --target amalgamate       # regenerate dist/lfg_ctest.h
./build/test_unified                          # run core self-tests directly
./build/test_mock                             # run mock self-tests directly
./build/test_amalg                            # smoke-test the amalgamated header
```

## Style

C style is enforced by `.clang-format` at the repo root (BSD/Allman, 4-space
indent, 120 col, pointer-right, case labels flush with switch). Commit
conventions still come from `~/.claude/CLAUDE.md` (terse `[subject] …`).
Run `clang-format -i lfg_ctest.c lfg_ctest.h lfg_ctest_mock.c lfg_ctest_mock.h
test_unified.c test_mock.c` to apply.
