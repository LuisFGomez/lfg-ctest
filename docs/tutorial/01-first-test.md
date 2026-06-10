# 1. Your first test + runner

This chapter gets a test binary compiling, running, and reporting. By the end
you will have written a test, grouped tests into a suite, attached
setup/teardown, and reached for the most common assertions.

No mocking yet — just the runner and the assertion macros. Everything here
lives in `<lfg-ctest.h>`.

## The smallest possible test

A test is an ordinary `void fn(void)` function. Inside it you call assertion
macros; the framework tallies every assertion and every test for you.

```c
#include <lfg-ctest.h>

static int add(int a, int b)
{
    return a + b;
}

static void test_add(void)
{
    ASSERT_EQ(5, add(2, 3));
    ASSERT_EQ(0, add(-1, 1));
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_test(NULL, test_add, NULL);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

Four runner calls frame every binary:

| Call | Role |
|------|------|
| `lfg_ct_start()` | Initialise the runner. Call once, before any test. |
| `lfg_ct_test(setup, fn, teardown)` | Run one test. `setup` / `teardown` are optional — pass `NULL` to skip. |
| `lfg_ct_print_summary()` | Print the pass/fail/skip/xfail/xpass tally. |
| `lfg_ct_return()` | The process exit code: `0` clean, non-zero if anything failed. |

`lfg_ct_test`'s second argument is the test function; the framework derives
the reported name from the identifier you pass (`test_add` above), so name
your functions for the report you want to read.

Compile and run it (linking the two framework sources directly is the
no-CMake path — chapter 8 covers the alternatives):

```bash
cc -o test_add test_add.c lfg-ctest.c
./test_add
```

A passing run ends with a summary line and exits `0`; the moment an assertion
fails, that test is marked `FAILURE`, the failing expression and its
`file:line` are printed, and `lfg_ct_return()` hands `main` a non-zero exit
code — which is exactly what CTest and CI watch for.

## Grouping tests into a suite

Once you have more than a couple of tests, group them. A *suite* is also a
plain `void fn(void)` function; it just calls `lfg_ct_test` for each member.

```c
static void test_add_positive(void) { ASSERT_EQ(5, add(2, 3)); }
static void test_add_negative(void) { ASSERT_EQ(-5, add(-2, -3)); }
static void test_add_zero(void)     { ASSERT_EQ(7, add(7, 0)); }

static void math_suite(void)
{
    lfg_ct_test(NULL, test_add_positive, NULL);
    lfg_ct_test(NULL, test_add_negative, NULL);
    lfg_ct_test(NULL, test_add_zero, NULL);
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_suite(NULL, math_suite, NULL);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

`lfg_ct_suite` runs the suite body and records the suite name as the
enclosing context for every test inside it — handy in reports and essential
for the name-glob filtering in [chapter 2](02-running-and-filtering.md).

## Setup and teardown

Both `lfg_ct_test` and `lfg_ct_suite` take a `setup` and a `teardown`
callback. Each is a `void fn(void)`; pass `NULL` for any phase you don't
need. The lifecycle is:

1. `setup()` runs first (if non-`NULL`).
2. `fn()` (the body) runs next — **skipped** if `setup()` raised an assertion
   failure.
3. `teardown()` runs last (if non-`NULL`). **Always** runs — even if the body
   or setup failed.

That "teardown always runs" guarantee is what makes it safe to release
resources there:

```c
static FILE *fixture;

static void open_fixture(void)  { fixture = tmpfile(); ASSERT_NOT_NULL(fixture); }
static void close_fixture(void) { if (fixture) { fclose(fixture); fixture = NULL; } }

static void test_writes_header(void)
{
    write_header(fixture);
    /* ... assertions against the file ... */
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_test(open_fixture, test_writes_header, close_fixture);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

When a suite wraps tests, hooks nest in the natural order:

```
suite-setup -> test-setup -> test -> test-teardown -> suite-teardown
```

Apply each hook at the level that matches the resource's scope: per-test when
each case needs a clean slate, per-suite when one acquisition is shared across
the whole batch. Teardown is where `mock_reset_all()` belongs once you start
mocking — see [chapter 6](06-teardown-and-cleanup.md).

## The assertions you will actually use

There are 49 assertions in total ([full table in api.md](../api.md#assertion-reference)).
You only need a handful to be productive. The most common:

```c
/* Equality / truth */
ASSERT_TRUE(cond);
ASSERT_FALSE(cond);
ASSERT_EQ(expected, actual);          /* signed int equality (alias of ASSERT_INT_EQUAL) */
ASSERT_NE(expected, actual);

/* Pointers */
ASSERT_NOT_NULL(ptr);
ASSERT_NULL(ptr);
ASSERT_PTR_EQUAL(expected, actual);

/* Strings and memory */
ASSERT_STR_EQUAL("ready", status);    /* strcmp */
ASSERT_STRN_EQUAL("readyXX", status, 5);
ASSERT_MEM_EQUAL(expected_bytes, actual_bytes, n);

/* Width-specific integers (unsigned print in hex, signed in decimal) */
ASSERT_UINT8_EQUAL(0x42, reg);
ASSERT_INT32_EQUAL(-1, rc);

/* Ranges, comparisons, bits */
ASSERT_GT(a, b);
ASSERT_IN_RANGE(val, 0, 100);
ASSERT_BIT_SET(flags, 3);
ASSERT_BITS_SET(flags, 0x0C);

/* Unconditional failure (e.g. an unreachable branch) */
ASSERT_FAIL("should not get here");
```

The width-specific integer assertions (`ASSERT_UINT8_EQUAL`,
`ASSERT_INT32_EQUAL`, ...) are worth preferring over plain `ASSERT_EQ` whenever
the value has a defined width: the failure message prints the right width in
the right base, so a mismatched register or status byte reads correctly
instead of being sign-extended into a confusing `int`.

### Floating point is opt-in

Float and double assertions are *not* compiled in by default — they pull in
`-lm`. Enable them with `LFG_CTEST_HAS_FLOAT` / `LFG_CTEST_HAS_DOUBLE` (the
CMake build sets these for you; see
[installation.md](../installation.md#floating-point-configuration)). Once
enabled they take an explicit epsilon:

```c
ASSERT_FLOAT_EQUAL(3.14159f, result, 0.0001f);
ASSERT_DOUBLE_EQUAL(3.141592653589793, acos(-1.0), 1e-15);
```

### One portability footgun

The pointer-assertion macros cast through `(void *)`, which ISO C99 does not
define for *function* pointers (POSIX does). Under
`gcc -std=c99 -pedantic-errors`, `ASSERT_NULL(fn_ptr)` is diagnosed even
though it works. Compare function pointers with the boolean form instead:

```c
ASSERT_TRUE(cb == NULL);     /* not ASSERT_NULL(cb) */
ASSERT_TRUE(cb_a == cb_b);   /* not ASSERT_PTR_EQUAL(cb_a, cb_b) */
```

## Where to go next

You can now write, group, and run tests. Next:

- [Chapter 2 — Running and filtering](02-running-and-filtering.md): drive one
  binary as many CTest entries with `--filter`.
- [Chapter 4 — Your first mock](04-first-mock.md): replace a dependency so you
  can test code that calls out to it.
