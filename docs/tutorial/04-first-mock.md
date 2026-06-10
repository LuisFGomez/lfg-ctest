# 4. Your first mock

So far the code under test stood alone. Real code calls *out* — to a driver, a
database, the network, the C library. To unit-test it you replace those
dependencies with **mocks**: stand-in functions that record how they were
called and return whatever the test tells them to.

lfg-ctest generates mocks from a pair of macros. This chapter covers
declaring and defining a mock, swapping it in for the real function, and the
three things every mock records: how many times it was called
(`__call_count`), with what arguments (`__param_history[]`), and what it
should hand back (`__return_queue[]`).

Everything here lives in `<lfg-ctest-mock.h>`.

## The naming convention

Every mock macro follows one pattern:

```
{DECLARE|DEFINE}_MOCK_{R|V}_{V|N}[_S]
```

- `DECLARE` goes in a header, `DEFINE` in exactly one `.c` file.
- `R` = the function **returns** a value; `V` = it returns `void`.
- Next position: `V` = no parameters, or a digit `1`–`9` = parameter count.
- `_S` suffix = struct-safe; only needed when a *parameter* is a struct passed
  by value (covered in [api.md](../api.md#struct-safe-mocks) — ignore it for
  now).

So `DECLARE_MOCK_R_2` is "returns a value, takes 2 parameters", and
`DEFINE_MOCK_V_V` is "returns void, takes no parameters". Up to 9 parameters
are supported.

## A worked example

Say the code under test reads a setting through a small storage layer:

```c
/* settings.h — the code we want to test */
int  store_get(const char *key, int *out_value);   /* the dependency */
int  setting_or_default(const char *key, int fallback);
```

```c
/* settings.c — implementation under test */
#include "settings.h"

int setting_or_default(const char *key, int fallback)
{
    int value = 0;
    if (store_get(key, &value) == 0)
    {
        return value;
    }
    return fallback;
}
```

`setting_or_default` calls `store_get`. We want to test its logic — "return
the stored value on success, the fallback on failure" — without a real store.
So we mock `store_get`.

### Declare the mock (header)

The signature is `int store_get(const char *key, int *out_value)`: returns
`int`, two parameters. That is `R_2`. List the return type first, then each
parameter type:

```c
/* store_mock.h */
#ifndef STORE_MOCK_H_
#define STORE_MOCK_H_

#include <lfg-ctest-mock.h>

/* Mock for: int store_get(const char *key, int *out_value); */
DECLARE_MOCK_R_2(store_get, int, const char *, int *);

/* Opt-in transparent replacement: when STORE_MOCK_REPLACE is defined
   before this header is included, every call to store_get() in the
   translation unit becomes a call to the mock. */
#if defined(STORE_MOCK_REPLACE)
#define store_get  store_get__mock
#endif

#endif /* STORE_MOCK_H_ */
```

### Define the mock (one source file)

`DEFINE_MOCK_*` emits the storage and the function body. It takes the **same
argument list** as the matching `DECLARE`, with no trailing semicolon:

```c
/* store_mock.c */
#include "store_mock.h"

DEFINE_MOCK_R_2(store_get, int, const char *, int *)
```

### What got generated

From that one declaration, the mock exposes a family of symbols, all prefixed
with the function name:

| Symbol | What it is |
|--------|------------|
| `store_get__mock(...)` | The stand-in function itself. |
| `store_get__call_count` | `size_t` — how many times it was called. |
| `store_get__param_history[]` | Array of captured parameters, one entry per call. |
| `store_get__return_queue[]` | Array of values it will return, one per call. |
| `store_get__param_actions` | Pointer-parameter read/write hooks ([chapter 5](05-param-actions-and-callbacks.md)). |
| `store_get__callback` | Optional per-call hook ([chapter 5](05-param-actions-and-callbacks.md)). |
| `store_get__mock_reset()` | Resets this one mock ([chapter 6](06-teardown-and-cleanup.md)). |

The `_S` and parameter details are in
[api.md](../api.md#mock-generated-symbols); the three you use constantly are
`__call_count`, `__param_history[]`, and `__return_queue[]`.

## Queuing return values

`__return_queue[i]` is what the mock returns on its *i*-th call (0-indexed).
Seed it before exercising the code:

```c
#include <lfg-ctest.h>
#include "store_mock.h"
#include "settings.h"

static void test_consults_the_store_once(void)
{
    /* First (and only) call to store_get returns 0 = success. */
    store_get__return_queue[0] = 0;

    setting_or_default("volume", 11);

    ASSERT_EQ(1, store_get__call_count);     /* it was called exactly once */
}
```

There is a subtlety to flag before going further: the queue only controls the
mock's *return value*, not the out-parameter. On the success path `store_get`
is supposed to write the looked-up number into `*out_value`, but a bare mock
leaves that buffer untouched — so a test that asserts on the *returned setting*
can't be written with the return queue alone. Making the mock *write* into
`*out_value` needs a parameter action, which is exactly what
[chapter 5](05-param-actions-and-callbacks.md) adds. The failure path, though,
depends only on the return value, so we can test it fully right now:

```c
static void test_returns_fallback_on_miss(void)
{
    store_get__return_queue[0] = -1;   /* store_get reports "not found" */

    int result = setting_or_default("volume", 11);

    ASSERT_EQ(1, store_get__call_count);
    ASSERT_EQ(11, result);             /* fell back, as intended */
}
```

The return queue is statically sized to `MOCK_CALL_STORAGE_MAX` (default 32).
If a test drives more calls than that, raise it by defining the macro before
the header:

```c
#define MOCK_CALL_STORAGE_MAX 64
#include <lfg-ctest-mock.h>
```

## Inspecting captured parameters

Every call records its arguments into `__param_history[]`. Parameters are
named positionally: `p0`, `p1`, ... in declaration order, 0-indexed. The call
index is the array index:

```c
static void test_forwards_the_key(void)
{
    store_get__return_queue[0] = -1;

    setting_or_default("brightness", 50);

    /* call 0, parameter 0 = the key string */
    ASSERT_STR_EQUAL("brightness", store_get__param_history[0].p0);
}
```

For a three-parameter mock called twice, the indexing reads naturally:

```c
/* Mock for: int add3(int a, int b, int c); */
DECLARE_MOCK_R_3(add3, int, int, int, int);

/* after add3__mock(10, 20, 30) then add3__mock(1, 2, 3): */
add3__param_history[0].p0;   /* 10 */
add3__param_history[0].p2;   /* 30 */
add3__param_history[1].p1;   /* 2  */
```

Pointer and function-pointer parameters are captured by value too, so you can
pull a captured callback back out of the history and invoke it yourself — a
pattern [chapter 5](05-param-actions-and-callbacks.md) builds on.

## Wiring the mock into the build

The mock replaces the real function via the `#define store_get store_get__mock`
in the header, gated on `STORE_MOCK_REPLACE`. The standard pattern (the same
one the [I2C worked example](../example.md) uses) is a per-module test header
that defines the replace macro *before* including the mock header, then
includes the module under test:

```c
/* settings_test.h */
#ifndef SETTINGS_TEST_H_
#define SETTINGS_TEST_H_

#define STORE_MOCK_REPLACE      /* turn on substitution... */
#include "store_mock.h"         /* ...before the mock header reads it */

#include "settings.h"           /* now the module sees store_get -> mock */
#endif
```

Compile the test binary from the module under test, its test file, the mock's
`.c`, and the framework sources:

```bash
cc -o settings_test \
    settings.c settings_test.c store_mock.c \
    lfg-ctest.c lfg-ctest-mock.c
```

(The conditional-include mechanics — how production code opts into mocking
with a single `#if` block — are laid out in full in the
[worked example](../example.md). Chapter 8 covers the CMake path.)

## Where to go next

- [Chapter 5 — Parameter actions and callbacks](05-param-actions-and-callbacks.md):
  make the mock *write* into out-parameters and run custom logic per call —
  the missing piece from the `store_get` example above.
- [Chapter 6 — Teardown and cleanup](06-teardown-and-cleanup.md): reset mock
  state between tests with a single call.
