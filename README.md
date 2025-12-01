# lfg-ctest - C99-Compatible Test and Mocking Framework

A comprehensive, easy-to-use, zero-dependency, C99-compatible test framework with mocking built in.

## Features

- **C99 Compatible**: Works with any C99-compliant compiler
- **Zero Dependencies**: Only requires the standard C library
- **49 Assertions**: Comprehensive assertion API for all common test scenarios
- **Built-in Mocking**: Macro-based mock generation with parameter capture and return value queuing
- **Struct-by-value Support**: Dedicated macros for functions with struct parameters

## Installation

Copy `lfg_ctest.h` (and `lfg_ctest_mock.h` if using mocking) into your project's include path.

---

## Testing API

### Basic Test Structure

```c
#include <lfg_ctest.h>

int test_example(void)
{
    ASSERT_TRUE(1 == 1);
    ASSERT_EQ(42, some_function());
    return lfg_ct_current_test_return();
}

int main(int argc, char *argv[])
{
    lfg_ct_start();
    lfg_ctest(test_example);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

### Test Suites

Group related tests into suites for better organization:

```c
int math_suite(void)
{
    lfg_ctest(test_addition);
    lfg_ctest(test_subtraction);
    lfg_ctest(test_multiplication);
    return lfg_ct_current_suite_return();
}

int main(int argc, char *argv[])
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
| `lfg_ctest(fn)` | Execute a single test function |
| `lfg_ct_suite(fn)` | Execute a test suite |
| `lfg_ct_print_summary()` | Print pass/fail summary |
| `lfg_ct_return()` | Get overall return code (0=pass, non-zero=fail) |
| `lfg_ct_current_test_return()` | Get current test's return code |
| `lfg_ct_current_suite_return()` | Get current suite's return code |

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

### Using Mocks in Tests

**Basic usage - verify call count and parameters:**
```c
int test_function_calls_dependency(void)
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
    return lfg_ct_current_test_return();
}
```

**Accessing captured callback parameters:**
```c
// Given: void register_callback(int id, void (*cb)(int), void *ctx);
DECLARE_MOCK_V_3(register_callback, int, void (*)(int), void *);

int test_callback_registration(void)
{
    function_under_test();  // calls register_callback internally

    // Retrieve captured callback and invoke it
    void (*captured_cb)(int) = register_callback__param_history[0].p1;
    void *captured_ctx = register_callback__param_history[0].p2;

    captured_cb(99);  // simulate callback invocation

    register_callback__mock_reset();
    return lfg_ct_current_test_return();
}
```

### Parameter Actions (Read/Write Memory)

For pointer parameters, capture or inject data using parameter actions:

**Capture data from a pointer parameter:**
```c
int test_capture_buffer_contents(void)
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
    return lfg_ct_current_test_return();
}
```

**Inject data into a pointer parameter:**
```c
int test_inject_read_data(void)
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
    return lfg_ct_current_test_return();
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
int test_with_struct_param(void)
{
    calculate_distance__return_queue[0] = 5.0f;

    point_t p = {3, 4};
    float result = calculate_distance__mock(p);

    ASSERT_EQ(3, calculate_distance__param_history[0].p0.x);
    ASSERT_EQ(4, calculate_distance__param_history[0].p0.y);

    calculate_distance__mock_reset();
    return lfg_ct_current_test_return();
}
```

**Note:** `_S` variants do not support `mock_param_mem_read/write` actions.

### Mock Limitations

- Maximum 32 calls stored per mock (`MOCK_CALL_STORAGE_MAX`) - exceeding this aborts via `assert()`
- Maximum 9 parameters per function
- Standard mocks cast parameters to `void*` via `size_t` - use `_S` variants for struct-by-value

---

## Complete Example

```c
// i2c_driver_mock.h
#include <lfg_ctest_mock.h>

DECLARE_MOCK_R_3(i2c_write, int, uint8_t, uint8_t *, size_t);

#if defined(I2C_DRIVER_MOCK_REPLACE)
#define i2c_write  i2c_write__mock
#endif

// i2c_driver_mock.c
#include "i2c_driver_mock.h"
DEFINE_MOCK_R_3(i2c_write, int, uint8_t, uint8_t *, size_t)

// test_my_device.c
#define I2C_DRIVER_MOCK_REPLACE
#include <lfg_ctest.h>
#include "i2c_driver_mock.h"
#include "my_device.h"

static void teardown(void)
{
    i2c_write__mock_reset();
}

static int test_device_init(void)
{
    uint8_t captured_buf[4] = {0};

    // Capture what gets written
    i2c_write__param_actions = mock_param_mem_read(NULL, 0, 1, captured_buf, 2);
    i2c_write__return_queue[0] = 0;  // success

    int result = my_device_init();

    ASSERT_EQ(0, result);
    ASSERT_EQ(1, i2c_write__call_count);
    ASSERT_UINT8_EQUAL(0x42, i2c_write__param_history[0].p0);  // slave addr
    ASSERT_UINT8_EQUAL(0x01, captured_buf[0]);  // register
    ASSERT_UINT8_EQUAL(0x80, captured_buf[1]);  // value

    teardown();
    return lfg_ct_current_test_return();
}

int main(int argc, char *argv[])
{
    lfg_ct_start();
    lfg_ctest(test_device_init);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

---

## License

[Add license information here]
