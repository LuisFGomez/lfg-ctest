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

- `test_unified` and `test_mock` are not built.
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
./build/test_unified                          # direct, core self-tests
./build/test_mock                             # direct, mock self-tests
```

`test_unified` intentionally exercises assertion failure paths (wrapped in
expect-failures mode), so a clean run is "all passed" even though the binary
internally triggered many failures.

### Run one specific test

Test functions are plain `void fn(void)` compiled into the binary — there is
no per-test CLI filter. To run just one:

1. Edit the test's `main()` or suite function to register only the case(s)
   you care about.
2. Rebuild and rerun the binary.

Don't commit these edits. They're a local iteration tool, not a workflow.

## Add a self-test

### For the core framework (in `test_unified.c`)

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

### For the mock framework (in `test_mock.c`)

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

## Formatting

`.clang-format` at the repo root is authoritative. Apply:

```
clang-format -i lfg_ctest.c lfg_ctest.h \
                lfg_ctest_mock.c lfg_ctest_mock.h \
                test_unified.c test_mock.c
```

**Not wired into CMake or CI.** It's a developer-invoked check — run it
before committing if you've touched any of those files.

## CI

None in-repo at the moment. Self-tests are the regression gate; run them
locally before pushing.
