# 6. Teardown and cleanup

Mock state is *sticky*. `__call_count`, `__param_history[]`, `__return_queue[]`,
`__param_actions`, and `__callback` all persist after a test returns. If the
next test doesn't start from a clean slate, it inherits the previous test's
call counts and queued returns — a classic source of tests that pass alone but
fail in a suite (or pass/fail depending on order).

The fix is one call.

## `mock_reset_all()` — the canonical teardown

`mock_reset_all()` (declared in `<lfg-ctest-mock.h>`) resets **every** mock the
test translation unit defined, in one sweep. It works because each
`DEFINE_MOCK_*` auto-registers its own `__mock_reset` thunk with the framework
the first time that mock is called — so you never maintain a per-mock reset
list by hand.

Put it in teardown so it runs after every test, even on assertion failure
(recall from [chapter 1](01-first-test.md) that teardown always runs):

```c
#include <lfg-ctest.h>
#include "store_mock.h"

static void teardown(void)
{
    mock_reset_all();
}

static void test_a(void) { /* ... uses store_get mock ... */ }
static void test_b(void) { /* ... starts clean, regardless of test_a ... */ }

static void settings_suite(void)
{
    lfg_ct_test(NULL, test_a, teardown);
    lfg_ct_test(NULL, test_b, teardown);
}
```

A teardown that also tears down non-mock fixtures just calls both:

```c
static void teardown(void)
{
    mock_reset_all();
    free(scratch_buffer);
    scratch_buffer = NULL;
}
```

This is the single most important habit in the mocking framework. The reason
it's `mock_reset_all()` and not a hand-written list of `foo__mock_reset()`
calls is robustness: the day you add a new `DEFINE_MOCK_*`, you would have to
remember to add its reset to every teardown, and forgetting silently leaks
state across tests. `mock_reset_all()` removes the entire class of bug — it
always sweeps everything.

### The per-mock escape hatch

`foo__mock_reset()` resets exactly one mock. It exists for the rare case where
you must clear a single mock *mid-test* without disturbing the others — not as
a teardown pattern. In teardown, always reach for `mock_reset_all()`.

```c
/* mid-test: forget the setup calls, then assert only on the real work */
store_get__mock_reset();
do_the_thing();
ASSERT_EQ(1, store_get__call_count);
```

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

static void setup(void)
{
    mock_register_cleanup(free_response_bodies);   /* re-register each cycle */
    response_bodies[0] = strdup("HTTP 200 OK");
}

static void teardown(void)
{
    mock_reset_all();   /* runs the framework thunks AND free_response_bodies */
}
```

Two rules govern the hook:

- **Re-register every cycle.** `mock_reset_all()` clears the registry along
  with everything else, so a hook registered once would not survive into the
  next test. Register it in `setup` (as above), or at the top of each test
  body — the same lifecycle as the auto-generated thunks.
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
