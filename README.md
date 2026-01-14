# lfg-ctest - C99-Compatible Test and Mocking Framework

A comprehensive, easy-to-use, zero-dependency, C99-compatible test framework with mocking built in.

## Features

- **C99 Compatible**: Works with any C99-compliant compiler
- **Zero Dependencies**: Only requires the standard C library
- **60+ Assertions**: Comprehensive assertion API for all common test scenarios
- **Built-in Mocking**: Macro-based mock generation with parameter capture and return value queuing
- **Struct-by-value Support**: Dedicated macros for functions with struct parameters
- **Embedded-Friendly**: Optional floating-point support with separate 32-bit float and 64-bit double configuration

## Installation

Copy `lfg_ctest.h` (and `lfg_ctest_mock.h` if using mocking) into your project's include path.

### CMake Integration

When using CMake, add as a subdirectory:

```cmake
add_subdirectory(deps/lfg-ctest)
target_link_libraries(my_tests lfg_ctest)
```

### Floating-Point Configuration

Floating-point assertions are optional and independently configurable. This is useful for embedded platforms where:
- Hardware float (32-bit) is available but double (64-bit) uses slow software emulation
- No FPU is present at all

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `LFG_CTEST_ENABLE_FLOAT` | `ON` | Enable 32-bit float assertions (requires `fabsf`) |
| `LFG_CTEST_ENABLE_DOUBLE` | `ON` | Enable 64-bit double assertions (requires `fabs`) |

Examples:
```bash
# Default: both float and double enabled
cmake -B build

# Float only (embedded with hardware FPU, no double)
cmake -B build -DLFG_CTEST_ENABLE_DOUBLE=OFF

# No floating-point (embedded without FPU)
cmake -B build -DLFG_CTEST_ENABLE_FLOAT=OFF -DLFG_CTEST_ENABLE_DOUBLE=OFF
```

When enabled, the library defines `LFG_CTEST_HAS_FLOAT` and/or `LFG_CTEST_HAS_DOUBLE` and links against `libm`.

### Function Name Reporting

Assertion failure messages include the function name where the failure occurred. The library auto-detects the best available option:

| Priority | Identifier | Availability |
|----------|------------|--------------|
| 1 | `__func__` | C99 standard |
| 2 | `__FUNCTION__` | GCC/Clang/MSVC extension (C89) |
| 3 | `"(unknown)"` | Fallback |

To disable function name reporting entirely, define `LFG_CTEST_NO_FUNC` before including the header:

```c
#define LFG_CTEST_NO_FUNC
#include <lfg_ctest.h>
```

---

## Testing API

### Basic Test Structure

Tests are just `void` functions. The framework tracks pass/fail automatically.

```c
#include <lfg_ctest.h>

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

These assertions require `LFG_CTEST_HAS_FLOAT` or `LFG_CTEST_HAS_DOUBLE` to be defined. See [Floating-Point Configuration](#floating-point-configuration).

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
- `_S` suffix = struct-safe (no param actions, works with struct-by-value)

**Examples:**
- `DECLARE_MOCK_R_2` - Returns value, 2 parameters
- `DEFINE_MOCK_V_V` - Void return, no parameters
- `DECLARE_MOCK_R_1_S` - Returns value, 1 parameter, struct-safe

### Creating a Mock

**Header file (my_module_mock.h):**
```c
#include <lfg_ctest_mock.h>

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
| `get_value_params` | typedef | Struct type for captured parameters |

### Storage Limits

Mock arrays are statically sized to `MOCK_CALL_STORAGE_MAX` (default 32). To increase:

```c
// Define before including the header
#define MOCK_CALL_STORAGE_MAX 64
#include <lfg_ctest_mock.h>
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

### Parameter Actions (Read/Write Memory)

For pointer parameters, capture or inject data using parameter actions:

| Function | Description |
|----------|-------------|
| `mock_param_mem_read(action, call_idx, param_idx, buf, size)` | Capture `size` bytes from parameter into `buf` |
| `mock_param_mem_write(action, call_idx, param_idx, buf, size)` | Inject `size` bytes from `buf` into parameter |
| `mock_param_destroy(action)` | Free action chain (called automatically by `_reset`) |

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

**Note:** `_S` variants do not support `mock_param_mem_read/write` actions.

### Mock Limitations

- Maximum 9 parameters per function
- Standard mocks cast parameters to `void*` via `size_t` - use `_S` variants for struct-by-value
- See [Storage Limits](#storage-limits) for call history size (default 32)

---

## Complete Example

This example demonstrates unit testing an I2C LED driver. The key pattern is **conditional includes** that swap real dependencies for mocks at compile time, allowing test code to live alongside production code.

### Production Code

**led_driver.h** - Public API:
```c
#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_

#include <stdint.h>

typedef void (*led_callback_t)(int status, void *ctx);

int led_reset(led_callback_t callback, void *ctx);
int led_set_brightness(uint8_t channel, uint16_t brightness);

#endif /* LED_DRIVER_H_ */
```

**led_driver.c** - Implementation with conditional includes:
```c
/*
 * This #if block is the ONLY modification needed in production code to enable
 * full unit testing. The test header pulls in mocks that transparently replace
 * real dependencies via macro substitution.
 */
#if defined(UNITTEST)
#include "led_driver_test.h"
#else
#include <stdint.h>
#include <i2c.h>
#include "led_driver.h"
#endif

#define LED_I2C_ADDR     0x42
#define LED_REG_RESET    0x01
#define LED_REG_BRIGHT   0x10

static led_callback_t _callback;
static void *_ctx;

int led_reset(led_callback_t callback, void *ctx)
{
    uint8_t buf[2] = {LED_REG_RESET, 0x80};
    _callback = callback;
    _ctx = ctx;
    return i2c_write(LED_I2C_ADDR, buf, 2, _i2c_done, NULL);
}

int led_set_brightness(uint8_t channel, uint16_t brightness)
{
    uint8_t buf[3] = {LED_REG_BRIGHT + channel, brightness & 0xFF, brightness >> 8};
    return i2c_write(LED_I2C_ADDR, buf, 3, NULL, NULL);
}

void _i2c_done(int status, void *ctx)
{
    if (_callback)
    {
        _callback(status, _ctx);
    }
}
```

### Mock Definitions

**i2c_mock.h** - Declares mock for `i2c_write()`:
```c
#ifndef I2C_MOCK_H_
#define I2C_MOCK_H_

#include <lfg_ctest_mock.h>
#include <stdint.h>

typedef void (*i2c_callback_t)(int status, void *ctx);

/* Mock: int i2c_write(uint8_t addr, uint8_t *buf, size_t len, i2c_callback_t cb, void *ctx) */
DECLARE_MOCK_R_5(i2c_write, int, uint8_t, uint8_t *, size_t, i2c_callback_t, void *);

#if defined(I2C_MOCK_REPLACE)
#define i2c_write  i2c_write__mock
#endif

#endif /* I2C_MOCK_H_ */
```

**i2c_mock.c** - Defines mock storage:
```c
#include "i2c_mock.h"

DEFINE_MOCK_R_5(i2c_write, int, uint8_t, uint8_t *, size_t, i2c_callback_t, void *)
```

### Test Code

**led_driver_test.h** - Test header that wires everything together:
```c
#ifndef LED_DRIVER_TEST_H_
#define LED_DRIVER_TEST_H_

/* Standard includes needed by the module under test */
#include <stdint.h>

/* Enable mock replacement BEFORE including mock headers */
#define I2C_MOCK_REPLACE
#include "i2c_mock.h"

/* Now include the module's public header */
#include "led_driver.h"

/* Test suite entry point */
void led_driver_suite(void);

#endif /* LED_DRIVER_TEST_H_ */
```

**led_driver_test.c** - Test implementation:
```c
#include <lfg_ctest.h>
#include "led_driver_test.h"

/* Test-local state to capture callback invocations */
static int _callback_status = -1;
static void *_callback_ctx = NULL;

static void _test_callback(int status, void *ctx)
{
    _callback_status = status;
    _callback_ctx = ctx;
}

static void _teardown(void)
{
    _callback_status = -1;
    _callback_ctx = NULL;
    i2c_write__mock_reset();
}

/* Test: led_reset sends correct I2C command */
static void test_led_reset_sends_correct_command(void)
{
    uint8_t captured_buf[4] = {0};
    int dummy_ctx = 42;

    /* Setup: capture the I2C buffer contents */
    i2c_write__param_actions = mock_param_mem_read(NULL, 0, 1, captured_buf, 2);
    i2c_write__return_queue[0] = 0;  /* i2c_write returns success */

    /* Execute */
    int result = led_reset(_test_callback, &dummy_ctx);

    /* Verify return value and I2C call */
    ASSERT_EQ(0, result);
    ASSERT_EQ(1, i2c_write__call_count);

    /* Verify I2C parameters via param_history */
    ASSERT_UINT8_EQUAL(0x42, i2c_write__param_history[0].p0);  /* addr */
    ASSERT_EQ(2, i2c_write__param_history[0].p2);              /* len */

    /* Verify buffer contents via captured data */
    ASSERT_UINT8_EQUAL(0x01, captured_buf[0]);  /* register */
    ASSERT_UINT8_EQUAL(0x80, captured_buf[1]);  /* reset command */

    /* Simulate I2C completion by invoking the captured callback */
    i2c_callback_t captured_cb = i2c_write__param_history[0].p3;
    void *captured_ctx = i2c_write__param_history[0].p4;
    captured_cb(0, captured_ctx);

    /* Verify our callback was invoked correctly */
    ASSERT_EQ(0, _callback_status);
    ASSERT_PTR_EQUAL(&dummy_ctx, _callback_ctx);

    _teardown();
}

/* Test: led_set_brightness sends correct I2C command */
static void test_led_set_brightness(void)
{
    uint8_t captured_buf[4] = {0};

    i2c_write__param_actions = mock_param_mem_read(NULL, 0, 1, captured_buf, 3);
    i2c_write__return_queue[0] = 0;

    int result = led_set_brightness(5, 0x0ABC);

    ASSERT_EQ(0, result);
    ASSERT_UINT8_EQUAL(0x15, captured_buf[0]);  /* register 0x10 + channel 5 */
    ASSERT_UINT8_EQUAL(0xBC, captured_buf[1]);  /* brightness low byte */
    ASSERT_UINT8_EQUAL(0x0A, captured_buf[2]);  /* brightness high byte */

    _teardown();
}

/* Test suite */
void led_driver_suite(void)
{
    lfg_ctest(test_led_reset_sends_correct_command);
    lfg_ctest(test_led_set_brightness);
}

/* Test runner */
int main(void)
{
    lfg_ct_start();
    lfg_ct_suite(led_driver_suite);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### File Summary

| File | Purpose | Included in |
|------|---------|-------------|
| `led_driver.h` | Production API | Production + Test |
| `led_driver.c` | Production implementation | Production + Test |
| `i2c_mock.h` | Mock declaration (reusable) | Test only |
| `i2c_mock.c` | Mock definition (reusable) | Test only |
| `led_driver_test.h` | Test wiring for this module | Test only |
| `led_driver_test.c` | Test implementation | Test only |

### Building a Reusable Mock Library

As your project grows, organize mocks into a shared directory. Each mock can be reused by any module that depends on that interface:

```
project/
├── src/
│   ├── drivers/
│   │   ├── led_driver.c      # production (has #if UNITTEST block)
│   │   ├── led_driver.h
│   │   ├── temp_sensor.c     # production (has #if UNITTEST block)
│   │   └── temp_sensor.h
│   └── hal/
│       ├── i2c.c             # production - real implementation
│       ├── i2c.h
│       ├── gpio.c
│       └── gpio.h
├── test/
│   ├── mock/                 # <-- reusable mock library
│   │   ├── i2c_mock.h        # used by led_driver, temp_sensor, etc.
│   │   ├── i2c_mock.c
│   │   ├── gpio_mock.h       # used by any module needing GPIO
│   │   └── gpio_mock.c
│   ├── led_driver_test.h     # test wiring (module-specific)
│   ├── led_driver_test.c
│   ├── temp_sensor_test.h
│   └── temp_sensor_test.c
└── Makefile
```

The mock files (`i2c_mock.*`, `gpio_mock.*`) are written once and reused across all tests. Only the `*_test.h` wiring header is module-specific—it selects which mocks to activate via `#define XXX_MOCK_REPLACE`.

### Build Configuration

Compile tests with `-DUNITTEST` to activate conditional includes:

```bash
# Test build
gcc -DUNITTEST -Itest/mock -o led_test \
    src/drivers/led_driver.c \
    test/led_driver_test.c \
    test/mock/i2c_mock.c \
    lfg_ctest.c lfg_ctest_mock.c

# Production build (links real HAL)
gcc -o firmware \
    src/drivers/led_driver.c \
    src/hal/i2c.c \
    main.c
```

---

## License

MIT License - see [LICENSE](LICENSE) for details.
