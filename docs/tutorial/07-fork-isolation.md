# 7. Fork-per-test isolation

`mock_reset_all()` resets *known* state between tests. It cannot undo a test
that corrupts the heap, leaks a file descriptor, scribbles past a buffer, or
crashes outright — by default every test body runs in the test binary's own
process, so a `SIGSEGV` or `abort()` takes the whole runner down with it and
every test after it never runs.

Fork-per-test isolation removes that failure mode. Each test runs in a fresh
`fork(2)`'d child; whatever it does to its process image dies with the child,
and the parent simply records the outcome and moves to the next test.

## Turning it on

One call, after `lfg_ct_parse_args` and before `lfg_ct_start`. That ordering
pins the mode in the program and makes `--isolation` a no-op — deliberate
here, but see [From the command line](#from-the-command-line) if you want the
flag to win instead:

```c
int main(int argc, char *argv[])
{
    if (0 != lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }

    /* Opt into fork-per-test. It can be unavailable (non-Unix host, or a
       build with fork support compiled out) — handle that gracefully
       instead of assuming success. */
    if (0 != lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK))
    {
        fprintf(stderr, "fork isolation unavailable; running in-process\n");
    }

    lfg_ct_start();
    /* ... lfg_ct_suite / lfg_ct_test calls ... */
    lfg_ct_print_summary();
    return lfg_ct_return();
}
```

`lfg_ct_set_isolation` returns `0` on success and non-zero when the mode is
recognised but unavailable — **with no silent fallback**; the previously
configured mode is left in place. The default is `LFG_CT_ISOLATE_NONE`
(in-process, the historical, fastest path).

### From the command line

`--isolation none|fork` reaches the same setting without a rebuild:

```sh
$ ./test_indicators --isolation fork          # enable it for this run
$ ./test_indicators --isolation none          # drop back in-process
```

That second form is the one you'll want when attaching a debugger to a single
failing test — stepping into a forked child is awkward, and `--isolation none`
is the escape hatch that doesn't require editing `main`.

There is a catch in the example above: it calls `lfg_ct_set_isolation`
**after** `lfg_ct_parse_args`, so the setter wins and `--isolation` is
ignored. The two surfaces share one piece of state and the rule is
last-writer-wins. Flip the order when you want the CLI to override:

```c
lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK);   /* the program's default */

if (0 != lfg_ct_parse_args(argc, argv))      /* parse LAST, so --isolation */
{                                            /* and --timeout can override */
    return 1;
}
```

Unlike `--filter` and the verbosity flags, `--isolation` and `--timeout` are
not reset by `lfg_ct_parse_args`: a parse that doesn't mention them leaves
your configuration alone, and a parse that *fails* applies neither.

## What it buys you

| Property | In-process (default) | Fork-per-test |
|----------|----------------------|---------------|
| Crash survival | A crashing test kills the runner. | The child dies; the parent records `FAILED` and continues. |
| State reset | Only what you reset by hand. | Every test starts from a pristine COW copy of the parent. |
| Sanitizer attribution | First leak blamed wherever it surfaces. | Leaks/UB attributed to the test that caused them. |
| Speed | Fastest. | One `fork` per test. |

Mechanically: for each `lfg_ct_test`, the parent `fork`s; the child runs the
test body by the normal in-process path and ships its outcome back over a
pipe; the parent decodes it onto its own counters and reporter. The failure
modes are all accounted for explicitly — no silent passes:

- A child killed by a signal (SIGSEGV, SIGABRT, SIGBUS, ...) is reaped and
  recorded as `FAILED` with a "killed by signal N" message.
- A child that exits non-zero **without** writing a payload (e.g. it called
  `_exit(N)` directly) is recorded as `FAILED` with "child exited N with no
  payload".

Crash-safety here is a property of fork isolation, **not** of teardown. When a
child crashes it is discarded whole — its entire address space, including any
fixture it acquired, dies with it, so there is nothing to tear down and no
in-body `teardown()` runs (nor needs to). The next test forks fresh from the
pristine parent. Don't rely on teardown as a crash mechanism; it is ordinary
cleanup on the normal path.

Expensive shared setup done *before* the dispatch — open file descriptors,
parsed fixtures, a DB handle — is inherited by every child for free via
copy-on-write, so you pay for it once in the parent, not once per test.

## Per-test timeout

Fork mode unlocks a per-test timeout, which an in-process runner can't offer
(a hung test would hang the runner). A child that hasn't exited in time is
sent `SIGKILL` and recorded as `FAILED` with a timeout diagnostic:

```c
lfg_ct_set_fork_timeout_ms(5000);   /* 5s per test; 0 disables (default) */
```

The setting only takes effect under `LFG_CT_ISOLATE_FORK`, but it persists
across mode changes, so you can set it once up front.

`--timeout <ms>` is the CLI equivalent, useful for tightening the budget on
one suspect run without a rebuild:

```sh
$ ./test_indicators --isolation fork --timeout 5000
$ ./test_indicators --isolation fork --timeout 0     # disable the timeout
```

`0` is a real value (timeout disabled), not a parse failure — which is why a
non-numeric, negative, or out-of-range value is rejected outright rather than
quietly becoming `0`. The flag is accepted on any build; it simply sits inert
until isolation is `fork`.

## Platform gating and opt-out

Fork mode is **Unix-family only** (`__unix__` / `__APPLE__`). Two ways it can
be unavailable, both reported the same clean way — `lfg_ct_set_isolation`
returns non-zero, mode unchanged:

1. **Non-Unix host** (e.g. Windows). A `CreateProcess`-based equivalent is
   explicitly out of scope for now.
2. **Compiled out.** A consumer who doesn't want the extra syscall/signal
   surface can build with `-DLFG_CTEST_ENABLE_FORK=OFF` (equivalently
   `LFG_CT_DISABLE_FORK=1` on the `lfg-ctest-fork.c` TU). The resulting binary
   has no fork/waitpid/signal code linked in at all.

On such a build `--isolation fork` fails the parse with a diagnostic naming
the unavailability, and the runner exits non-zero. That mirrors the setter:
asking for fork and getting in-process without being told would misreport
what actually ran.

Crucially, the `LFG_CT_ISOLATE_FORK` enum value is part of the public ABI
**unconditionally** — code that references it still compiles in both builds.
That is why the graceful-fallback `if` above is the right shape: write the
code once, and it does the right thing whether or not the runtime support is
present.

### Fork isolation with coverage or LSan

`-DLFG_CTEST_ENABLE_FORK=OFF` is the right choice when you don't need fork
isolation: there is no child and no `_exit()`, so gcov, LLVM-profile and LSan
results come out of the normal `exit()` path with nothing to lose. Keep fork
on and register a child-exit hook when you need both fork isolation and a
runtime that writes its results from an `atexit` handler:

```c
int lfg_ct_register_child_exit_hook(void (*hook)(void));
```

Registration returns 0 on success, and 0 again when the same pointer is already
registered (no-op). It returns -1 for `NULL` or once 8 distinct hooks are
registered. Register before running tests.

Hooks run only in a forked child, once per test, in registration order. The
sequence in the child is: stdio flush, hooks, stdio flush, `_exit()`. A hook
never runs in the parent, under `--isolation none`, or in a child that crashed.
Hook time is not a separate budget: it draws down
the same `--timeout` deadline as the test body, so a hook slow enough to push
total elapsed past that deadline gets the child killed mid-hook and the test
reported as a timeout, not a hook failure -- size the timeout to cover the
hook's own cost. A crash inside a hook marks the test `FAILED`
with "killed by signal N", even when the body passed. A registration made
inside a test body affects only that child. In a `LFG_CT_DISABLE_FORK` build the call returns the same
values and no hook fires, so the same consumer code builds in both.

Register the dump function for the runtime you build with. Guard the `extern`
and the call with your own coverage or sanitizer macro: the symbol exists only
when the TU is built with that runtime, and an unguarded reference fails the
link everywhere else.

```c
/* gcov: build with --coverage -DMY_COVERAGE */
#ifdef MY_COVERAGE
extern void __gcov_dump(void);
#endif

/* LLVM source-based coverage: build with -fprofile-instr-generate */
#ifdef MY_LLVM_PROFILE
extern int __llvm_profile_write_file(void);
static void llvm_profile_dump(void) { (void)__llvm_profile_write_file(); }
#endif

/* LeakSanitizer: build with -fsanitize=address (or =leak) */
#ifdef MY_LSAN
extern void __lsan_do_leak_check(void);
#endif

int main(void)
{
#ifdef MY_COVERAGE
    lfg_ct_register_child_exit_hook(__gcov_dump);
#endif
#ifdef MY_LLVM_PROFILE
    lfg_ct_register_child_exit_hook(llvm_profile_dump);
#endif
#ifdef MY_LSAN
    lfg_ct_register_child_exit_hook(__lsan_do_leak_check);
#endif
    /* ... run tests ... */
}
```

In a consumer binary built with `--coverage` and the gcov hook registered, a
run followed by `gcov` on the consumer's own object directory reports hits on
the lines that run only in the child, matching an `--isolation none` run. The
run must use fork mode: either the program pins it with
`lfg_ct_set_isolation(LFG_CT_ISOLATE_FORK)` or the command line passes
`--isolation fork`. The default is `none`, so a bare run never forks.

A couple of things stay out of scope: a test that itself calls `fork(2)` is on
its own (the parent only waits on its direct child), and Windows isolation is
a separate follow-up.

## When to reach for it

In-process is the right default — it's fastest and most tests don't need
isolation. Turn on fork mode when:

- a test exercises code that can legitimately crash (parsers on hostile input,
  anything with `assert`s you want to catch as failures rather than aborts);
- you're chasing cross-test contamination that survives `mock_reset_all()`;
- you want per-test sanitizer attribution or per-test timeouts.

## Where to go next

- [Chapter 8 — Integration](08-integration.md): wiring the framework into a
  build — CMake subdirectory vs single-header amalgamation — and the optional
  JUnit-XML reporter for CI.
