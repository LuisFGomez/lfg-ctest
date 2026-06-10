# 3. Skip, xfail, xpass

Not every test is cleanly pass-or-fail. Some can't run right now (a missing
fixture, a platform-specific path); some are *known* broken for a tracked
reason but you still want to exercise the code path. lfg-ctest gives each of
these its own bucket so they don't distort the pass/fail tally.

Two macros, both callable from inside a test body (and from the test's
`setup`):

```c
lfg_ct_skip("waiting on driver fix");
lfg_ct_xfail("known flaky under valgrind");
```

## SKIP — "don't run this right now"

`lfg_ct_skip(reason)` marks the current test **SKIP**, records the reason, and
returns from the body immediately. A SKIP counts toward neither pass nor fail.

The idiomatic use is a runtime precondition the test can't meet:

```c
static void test_uses_ipv6(void)
{
    if (!host_has_ipv6())
    {
        lfg_ct_skip("no IPv6 on this host");
        /* nothing after this runs */
    }

    ASSERT_TRUE(connect_v6());
}
```

Calling `lfg_ct_skip` from the test's `setup` is also legal and natural — it
reads as "preconditions not met". The body is not invoked, but teardown still
runs, so anything setup acquired before the skip is still released.

There is no first-class *suite*-level skip. To skip a whole group on a runtime
condition, gate the `lfg_ct_test(...)` calls inside the suite body with an
ordinary `if`:

```c
static void hardware_suite(void)
{
    if (!device_present())
    {
        return;   /* register nothing; the suite is effectively skipped */
    }
    lfg_ct_test(NULL, test_device_reset, NULL);
    lfg_ct_test(NULL, test_device_read, NULL);
}
```

## XFAIL / XPASS — "known broken, but keep exercising it"

`lfg_ct_xfail(reason)` marks the current test as *expected to fail* and then
**keeps running the body**. After the body completes, the framework
classifies it by what actually happened:

- at least one assertion failed -> **XFAIL** (the expected outcome — its own
  bucket, not counted as a failure);
- no assertion failed -> **XPASS** (it unexpectedly passed — also its own
  bucket).

```c
static void test_parser_handles_nested_quotes(void)
{
    lfg_ct_xfail("nested-quote handling tracked in #214");

    /* This still executes. If it fails -> XFAIL; if the bug gets
       fixed and it passes -> XPASS, flagging that the xfail is stale. */
    ASSERT_STR_EQUAL("a\"b", unescape("a\\\"b"));
}
```

This is the key difference from SKIP: `xfail` *runs* the code, so a regression
that makes it crash is still visible, and the day someone fixes the underlying
bug the test flips to XPASS to tell you the `lfg_ct_xfail` line is now stale
and should be deleted.

If you call `lfg_ct_xfail` more than once in a body, the last reason wins.

## Scope: per-test only

Both macros are per-test. Called from anywhere else — `main`, a suite-level
setup/body/teardown, a per-test teardown, or between tests — they are no-ops
that print a warning to stderr. Keep them inside the test body (or, for
`lfg_ct_skip`, the test's setup).

## What the report looks like

Per-test lines carry the reason:

```
*** test SKIP: test_uses_ipv6: no IPv6 on this host
*** test XFAIL: test_parser_handles_nested_quotes: nested-quote handling tracked in #214
*** test XPASS: test_parser_handles_nested_quotes: nested-quote handling tracked in #214
```

The final summary breaks out the three buckets alongside the failure count:

```
*** Executed 412 assertions in 57 tests. Failures: 0, Skipped: 3, XFail: 2, XPass: 1
*** Testing complete. Result: PASS
```

## Exit codes and `--strict-xpass`

By default an XPASS is a *warning*, not a failure — a run with XPASSes but no
real failures still exits `0`. The full rule set for `lfg_ct_return()`:

| Outcome | Exit |
|---------|------|
| Only PASS / SKIP / XFAIL | `0` |
| Any FAIL | non-zero |
| Any XPASS **and** `--strict-xpass` was parsed | non-zero |
| Any XPASS without `--strict-xpass` | `0` (warning only) |

Pass `--strict-xpass` (see [chapter 2](02-running-and-filtering.md)) in CI when
you want the build to break the moment a bug fixes itself — that is the signal
to go delete the now-stale `lfg_ct_xfail` and let the test guard the fix for
real:

```bash
./test_parser --strict-xpass     # XPASS now fails the run
```

## Where to go next

- [Chapter 4 — Your first mock](04-first-mock.md): start replacing
  dependencies so you can test code that calls out to them.
