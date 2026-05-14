# example.md — lfg-ctest worked example

End-to-end unit test for an I2C LED driver. The key pattern is **conditional
includes** that swap real dependencies for mocks at compile time, allowing test
code to live alongside production code.

## Production Code

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

## Mock Definitions

**i2c_mock.h** - Declares mock for `i2c_write()`:

```c
#ifndef I2C_MOCK_H_
#define I2C_MOCK_H_

#include <lfg-ctest-mock.h>
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

## Test Code

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
#include <lfg-ctest.h>
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
    mock_reset_all();
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

## File Summary

| File | Purpose | Included in |
|------|---------|-------------|
| `led_driver.h` | Production API | Production + Test |
| `led_driver.c` | Production implementation | Production + Test |
| `i2c_mock.h` | Mock declaration (reusable) | Test only |
| `i2c_mock.c` | Mock definition (reusable) | Test only |
| `led_driver_test.h` | Test wiring for this module | Test only |
| `led_driver_test.c` | Test implementation | Test only |

## Building a Reusable Mock Library

As your project grows, organize mocks into a shared directory. Each mock can
be reused by any module that depends on that interface:

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

The mock files (`i2c_mock.*`, `gpio_mock.*`) are written once and reused across
all tests. Only the `*_test.h` wiring header is module-specific — it selects
which mocks to activate via `#define XXX_MOCK_REPLACE`.

## Build Configuration

Compile tests with `-DUNITTEST` to activate conditional includes:

```bash
# Test build
gcc -DUNITTEST -Itest/mock -o led_test \
    src/drivers/led_driver.c \
    test/led_driver_test.c \
    test/mock/i2c_mock.c \
    lfg-ctest.c lfg-ctest-mock.c

# Production build (links real HAL)
gcc -o firmware \
    src/drivers/led_driver.c \
    src/hal/i2c.c \
    main.c
```
