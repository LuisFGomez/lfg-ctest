# lfg-ctest

C99 test + mocking framework. Single static library, zero deps (`-lm` only for
optional float/double asserts). Pairs a tiny test runner with a macro-based
mock generator that covers parameter capture, return queuing, parameter
read/write actions, and a per-call callback hook.

## At a glance

- 49 assertions: scalar / pointer / string / memory / bit / float / double.
- Mock macros (`DECLARE_MOCK_*` / `DEFINE_MOCK_*`) generate `__mock`,
  `__mock_reset`, `__call_count`, `__param_history[]`, `__return_queue[]`,
  `__param_actions`, `__callback` per target.
- Single-call teardown: every `DEFINE_MOCK_*` auto-registers its `__mock_reset`
  thunk, so `mock_reset_all()` sweeps every mock the test TU defined — no
  per-mock bookkeeping.
- Consumer-cleanup-hook on-ramp (`mock_register_cleanup`) so mocks that own
  heap state can share that same single-call teardown.
- CMake `add_subdirectory` integration, or single-header amalgamation
  (`dist/lfg-ctest.h`) for drop-in use without CMake or vendored sources.

## Quick start

```cmake
add_subdirectory(deps/lfg-ctest)
target_link_libraries(my_tests lfg-ctest)
```

```c
#include <lfg-ctest.h>

void test_example(void)
{
    ASSERT_EQ(42, some_function());
}

int main(void)
{
    lfg_ct_start();
    lfg_ctest(test_example);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

## Docs

| Doc | Covers |
|-----|--------|
| [docs/installation.md](docs/installation.md) | CMake integration, single-header amalgamation, float/double config, `__func__` reporting. |
| [docs/api.md](docs/api.md) | Public API: runner, assertions, mock macros, parameter actions, callback contract, cleanup hooks. |
| [docs/example.md](docs/example.md) | Worked example — unit-testing an I2C LED driver with conditional mock includes. |
| [docs/architecture.md](docs/architecture.md) | Framework internals: runner state, expect-failures mode, mock fanout, `_S` variants, reset registry, amalgamation, float/double gating. |
| [docs/build-and-test.md](docs/build-and-test.md) | Working on the framework itself: presets, self-test patterns, versioning, formatting, CI. |

## Layout

| Path | What's there |
|------|--------------|
| `lfg-ctest.[ch]` | Test runner + assertion macros. |
| `lfg-ctest-mock.[ch]` | Mock-generation macros, param-action runtime, reset registry. |
| `test-unified.c` / `test-mock.c` / `test-amalg.c` | Self-tests (built only when this repo is the top-level CMake source). |
| `tools/` | C99 amalgamator (`amalgamate.c` + `amalgamate.manifest`), version stamper (`mkversion.c`), release helper (`mkrelease.c`). |
| `dist/` | Generated single-header (gitignored; built by `cmake --build build --target amalgamate`). |
| `CMakeLists.txt`, `CMakePresets.json` | Build config — float/double auto-detection, install rules, only `debug` preset. |
| `.clang-format` | BSD/Allman, 4-space indent, 120-col, pointer-right, case labels flush with switch. Authoritative for this repo. |
| `samples/` | Untracked example consumer (I2C LED driver). |

## Build & run

```
cmake --preset debug                          # configure ./build with Ninja, Debug
cmake --build build                           # build library + self-tests
ctest --test-dir build --output-on-failure    # run all self-tests
cmake --build build --target amalgamate       # regenerate dist/lfg-ctest.h
```

See [docs/build-and-test.md](docs/build-and-test.md) for self-test patterns,
the `LFG_CTEST_SELF_TEST` gate, and the `release-tag` target.

## License

MIT — see [LICENSE](LICENSE).
