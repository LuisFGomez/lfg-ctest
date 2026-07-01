# 1. Your first test + runner

This chapter gets a test binary compiling, running, and reporting. By the end
you will have written a test, grouped tests into a suite, called
setup/teardown from a test body, and reached for the most common assertions.

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
    lfg_ct_test(test_add);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

Four runner calls frame every binary:

| Call | Role |
|------|------|
| `lfg_ct_start()` | Initialise the runner. Call once, before any test. |
| `lfg_ct_test(fn)` | Run one test. The body is the only argument; any setup/teardown are plain calls the body makes itself. |
| `lfg_ct_print_summary()` | Print the pass/fail/skip/xfail/xpass tally. |
| `lfg_ct_return()` | The process exit code: `0` clean, non-zero if anything failed. |

`lfg_ct_test`'s only argument is the test function; the framework derives
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
    lfg_ct_test(test_add_positive);
    lfg_ct_test(test_add_negative);
    lfg_ct_test(test_add_zero);
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_suite(math_suite);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

`lfg_ct_suite` runs the suite body and records the suite name as the
enclosing context for every test inside it — handy in reports and essential
for the name-glob filtering in [chapter 2](02-running-and-filtering.md).

## Setup and teardown

The framework binds **no** lifecycle hooks. Setup and teardown are plain
static functions your test body calls itself — the calls are right there in
the body, so a reader sees exactly when they fire (no hidden control flow to
chase back to a registration call). The habit:

- Call `setup()` at the top of the body and `teardown()` at the bottom.
- Because assertions are non-fatal (they record a failure and keep going —
  there is no fatal `REQUIRE`), a trailing `teardown()` is still reached after
  a failed assertion in the body. Nothing special is needed for that case.

```c
static FILE *fixture;

static void open_fixture(void)  { fixture = tmpfile(); ASSERT_NOT_NULL(fixture); }
static void close_fixture(void) { if (fixture) { fclose(fixture); fixture = NULL; } }

static void test_writes_header(void)
{
    open_fixture();
    write_header(fixture);
    /* ... assertions against the file ... */
    close_fixture();
}

int main(void)
{
    lfg_ct_start();
    lfg_ct_test(test_writes_header);
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

Only two paths leave a body *before* that trailing call: an early `return`
and `lfg_ct_skip(...)` (which `longjmp`s out). Put `teardown()` immediately
before either one — "teardown before skip" is load-bearing, since the skip
unwinds straight past a trailing in-body `teardown()`. [Chapter
3](03-skip-xfail-xpass.md) shows the skip path, and [chapter
6](06-teardown-and-cleanup.md) covers setup that can itself fail plus the
`mock_reset_all()` teardown you will reach for once you start mocking.

A suite is just a body that calls `lfg_ct_test`, so per-suite setup/teardown
is the same idea one level up: the suite body brackets its `lfg_ct_test`
calls with the shared acquire/release. Apply each at the level that matches
the resource's scope — per-test when each case needs a clean slate, per-suite
when one acquisition is shared across the whole batch.

One gotcha for a per-suite `setup`: the suite body still runs under `--list`
(that is how the contained tests announce their names), so a side-effectful
suite setup would fire during a plain listing. Gate it behind
`if (!lfg_ct_is_list_mode())` — see [chapter
2](02-running-and-filtering.md#listing-what-a-binary-contains).

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
