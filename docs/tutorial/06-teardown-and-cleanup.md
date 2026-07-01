# 6. Teardown and cleanup

Mock state is *sticky*. `__call_count`, `__param_history[]`, `__return_queue[]`,
`__param_actions`, and `__callback` all persist after a test returns. If the
next test doesn't start from a clean slate, it inherits the previous test's
call counts and queued returns — a classic source of tests that pass alone but
fail in a suite (or pass/fail depending on order).

The fix is one call. But first, the convention that decides *where* that call
goes.

## The convention: the body owns its own cleanup

The framework binds no teardown hook (that registration-time mechanism was
removed). Teardown is a plain function your test body calls itself. The rules
are short:

- **Fixtureless test:** `lfg_ct_test(test);` — nothing else.
- **Test with cleanup:** call `setup()` at the top of the body and
  `teardown()` at the bottom. Assertions are non-fatal (they record a failure
  and keep going — there is no fatal `REQUIRE`), so a trailing `teardown()` is
  still reached after a failed assertion in the body.
- **Early exit:** the only two ways out of a body that bypass the trailing
  call are an early `return` and `lfg_ct_skip(...)`. Call `teardown()`
  immediately before either one. "Teardown before skip" is load-bearing: the
  skip `longjmp`s out of the body and unwinds straight past a trailing in-body
  `teardown()`.

```c
static void test_with_cleanup(void)
{
    setup();

    if (!precondition_met())
    {
        teardown();                     /* release before leaving */
        lfg_ct_skip("preconditions not met");
    }

    ASSERT_EQ(expected, do_work());     /* soft-fails still reach the tail */

    teardown();
}
```

For the skip path specifically, `lfg_ct_skip_cleanup(cleanup, reason)` folds the
teardown-before-skip pair into a single call: it invokes `cleanup` (a plain
`void(void)`, the shape a teardown already has) and then skips. It is optional
sugar — nothing more than the two lines it replaces — so reach for whichever
reads clearer at the call site:

```c
    if (!precondition_met())
    {
        lfg_ct_skip_cleanup(teardown, "preconditions not met");
    }
```

Pass `NULL` for `cleanup` and it behaves exactly like a plain `lfg_ct_skip`.

## `mock_reset_all()` — the canonical teardown

`mock_reset_all()` (declared in `<lfg-ctest-mock.h>`) resets **every** mock the
test translation unit defined, in one sweep. It works because each
`DEFINE_MOCK_*` auto-registers its own `__mock_reset` thunk with the framework
the first time that mock is called — so you never maintain a per-mock reset
list by hand.

Because it is idempotent, `mock_reset_all()` sidesteps the early-exit question
entirely for the common mock-only case: call it at the **top** of each test
instead of writing a teardown at the bottom. A clean slate on entry is
equivalent to a clean slate on exit, and there is no trailing call for a `skip`
or `return` to jump past.

```c
#include <lfg-ctest.h>
#include "store_mock.h"

static void test_a(void)
{
    mock_reset_all();       /* start clean, regardless of any prior test */
    /* ... uses store_get mock ... */
}

static void test_b(void)
{
    mock_reset_all();
    /* ... starts clean, regardless of test_a ... */
}

static void settings_suite(void)
{
    lfg_ct_test(test_a);
    lfg_ct_test(test_b);
}
```

If you would rather clean up at the end — say the test also releases a non-mock
fixture you want torn down on the way out — write a `teardown()` and call it at
the bottom (and before any early `return` / `skip`, per the convention above):

```c
static void teardown(void)
{
    mock_reset_all();
    free(scratch_buffer);
    scratch_buffer = NULL;
}
```

Reset-at-top or teardown-at-bottom, `mock_reset_all()` is the single most
important habit in the mocking framework. The reason it's `mock_reset_all()`
and not a hand-written list of `foo__mock_reset()` calls is robustness: the day
you add a new `DEFINE_MOCK_*`, you would have to remember to add its reset to
every cleanup site, and forgetting silently leaks state across tests.
`mock_reset_all()` removes the entire class of bug — it always sweeps
everything.

### The per-mock escape hatch

`foo__mock_reset()` resets exactly one mock. It exists for the rare case where
you must clear a single mock *mid-test* without disturbing the others — not as
a cleanup pattern. For cleanup, always reach for `mock_reset_all()`.

```c
/* mid-test: forget the setup calls, then assert only on the real work */
store_get__mock_reset();
do_the_thing();
ASSERT_EQ(1, store_get__call_count);
```

## Setup that can fail

When `setup()` itself drives assertions — opening a fixture, seeding a mock —
a failure there means the body has nothing meaningful to test. Detect it and
bail cleanly. The robust, sign-independent way is to snapshot
`lfg_ct_failure_count()` around the setup and compare (this catches a soft-fail
*anywhere* in setup, including inside a helper it calls):

```c
static void test_needs_fixture(void)
{
    size_t baseline = lfg_ct_failure_count();
    setup();
    if (lfg_ct_failure_count() > baseline)
    {
        teardown();                     /* teardown before skip */
        lfg_ct_skip("setup failed");
    }

    /* ... the real test ... */
    teardown();
}
```

If your `setup()` returns the assertion result directly, you can branch on it
instead. Every `ASSERT_*` returns `0` on pass and non-zero on failure, so:

```c
static int setup(void)
{
    fixture = tmpfile();
    return ASSERT_NOT_NULL(fixture);    /* 0 on pass, non-zero on failure */
}

static void test_writes_header(void)
{
    if (0 != setup())
    {
        teardown();
        lfg_ct_skip("fixture unavailable");
    }
    /* ... */
    teardown();
}
```

The snapshot-and-compare form is preferred when setup runs more than one
assertion or hides them inside helpers — it needs no per-assert wiring. See
[api.md — Failure count](../api.md#failure-count) for the full accessor
semantics.

## When a mock owns heap state: `mock_register_cleanup`

`mock_reset_all()` knows how to zero the framework-generated arrays. What it
*can't* know about is heap state your mock owns on the side — say a
`__return_queue` whose entries are `malloc`'d blobs the test seeded, or a
lookup table the mock builds. Left alone, that leaks across the reset.

Register a custom cleanup walker with `mock_register_cleanup(void (*)(void))`.
It runs alongside the auto-registered `__mock_reset` thunks on every
`mock_reset_all()` — so test authors still call exactly one function:

```c
/* a mock whose return queue holds allocated buffers */
static char *response_bodies[MOCK_CALL_STORAGE_MAX];

static void free_response_bodies(void)
{
    for (size_t i = 0; i < MOCK_CALL_STORAGE_MAX; i++)
    {
        free(response_bodies[i]);
        response_bodies[i] = NULL;
    }
}

static void test_replays_responses(void)
{
    mock_register_cleanup(free_response_bodies);   /* re-register each cycle */
    response_bodies[0] = strdup("HTTP 200 OK");

    /* ... exercise the code under test ... */

    mock_reset_all();   /* runs the framework thunks AND free_response_bodies */
}
```

Two rules govern the hook:

- **Re-register every cycle.** `mock_reset_all()` clears the registry along
  with everything else, so a hook registered once would not survive into the
  next test. Register it at the top of each test body (as above), or in a
  `setup()` the body calls — the same lifecycle as the auto-generated thunks.
- **Ordering is unspecified** between the framework thunks and consumer hooks.
  Don't write a cleanup hook that depends on a mock's arrays still being
  populated (or already cleared) when it runs; make it self-contained.

The implementation rationale lives in
[architecture.md](../architecture.md); the contract is in
[api.md](../api.md#consumer-cleanup-hooks).

## Where to go next

- [Chapter 7 — Fork-per-test isolation](07-fork-isolation.md): when resetting
  state isn't enough — running each test in its own process so a crash or a
  leak can't reach the next test.
