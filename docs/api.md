# api.md — lfg-ctest public API

Consumer-facing reference for the assertion API, the test runner, and the
mocking framework. For internals (how the framework is put together), see
[architecture.md](architecture.md).

## Testing API

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
    lfg_ctest(test_example);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### Test Suites

Suites are also `void` functions that group related tests:

```c
void math_suite(void)
{
    lfg_ctest(test_addition);
    lfg_ctest(test_subtraction);
    lfg_ctest(test_multiplication);
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
| `lfg_ctest(fn)` | Execute a single test function (`void fn(void)`) |
| `lfg_ct_suite(fn)` | Execute a test suite (`void fn(void)`) |
| `lfg_ct_print_summary()` | Print pass/fail summary |
| `lfg_ct_return()` | Get overall return code (0=pass, non-zero=fail) |
| `lfg_ct_version()` | Framework version string (`"M.m.p[+<sha>]"`) |

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

lfg-ctest does not impose any setup/teardown mechanism. They're just functions you call however you like:

```c
static void setup(void)
{
    // initialize test state
    my_mock__mock_reset();
    global_state = initial_value;
}

static void teardown(void)
{
    // cleanup after test
    my_mock__mock_reset();
    free(allocated_memory);
}

void test_something(void)
{
    setup();

    // ... test logic ...
    ASSERT_EQ(expected, actual);

    teardown();
}
```

Call them per-test, per-suite, or not at all—your choice:

```c
void my_suite(void)
{
    setup();  // once for the whole suite

    lfg_ctest(test_case_1);
    lfg_ctest(test_case_2);
    lfg_ctest(test_case_3);

    teardown();
}

// Or per-test if each needs isolation:
void test_with_isolation(void)
{
    setup();
    ASSERT_TRUE(condition);
    teardown();
}
```

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
| `get_value__mock_reset()` | function | Reset all mock state |
| `get_value__call_count` | `size_t` | Number of times mock was called |
| `get_value__param_history[]` | array | Captured parameters from each call |
| `get_value__return_queue[]` | array | Return values (for R_* mocks) |
| `get_value__param_actions` | pointer | Parameter read/write actions |
| `get_value__callback` | function pointer | Optional callback invoked each call |
| `get_value__callback_t` | typedef | Callback function pointer type |
| `get_value_params` | typedef | Struct type for captured parameters |

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

    get_value__mock_reset();
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

    register_callback__mock_reset();
}
```

### Parameter Actions (Read/Write Memory and Strings)

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

    i2c_write__mock_reset();
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

    i2c_read__mock_reset();
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

    mkdir__mock_reset();
}
```

### Mock Callbacks

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
    curl_easy_perform__mock_reset();
    curl_easy_perform__return_queue[0] = CURLE_OK;
    curl_easy_perform__callback = on_perform;

    function_under_test();

    ASSERT_EQ(1, curl_easy_perform__call_count);
    curl_easy_perform__mock_reset();
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
    is_file__mock_reset();
    is_file__callback = on_is_file;

    seed_state("a");        /* "a" exists, "c" does not */
    process_manifest("ab,c");

    is_file__mock_reset();
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

    calculate_distance__mock_reset();
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
