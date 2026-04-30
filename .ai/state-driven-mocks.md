# state-driven-mocks.md — design note

**Status.** Design only. No code changes are made by issue #6; implementation
follow-ups (if any) are filed as separate issues per the recommendation at
the bottom.

**Audience.** Anyone touching `lfg-ctest-mock.[ch]` codegen, or any consumer
(currently `lfg/cweb`) that wants a state-driven testing fake on top of these
mocks.

## The question

When mocking I/O primitives whose semantics are state-coupled — for
`cweb_fio` specifically: `cweb_fio_is_file`, `cweb_fio_exists`,
`cweb_fio_foreach_line` — the natural test-side mental model is
"the SUT writes bytes, then queries them": the mock's return is a
function of state the test (or the SUT, via prior writes) seeded, not a
fixed queue. Where do we place support for that pattern?

1. **Framework-level** in `lfg-ctest`: an opt-in mechanism letting the current
   call's return depend on test-side state.
2. **Consumer-level** in `lfg/cweb`: a `cweb_fio_fake` convenience layer atop
   the existing primitives.
3. **Both.**
4. **Neither** — accept the boilerplate and document the pattern.

## The queue-vs-state mismatch

Today's R-style mock body (see `DEFINE_MOCK_R_1` in `lfg-ctest-mock.h:765`,
and the same shape in `R_2` … `R_9` and the `_S` family) executes in this
order:

```c
size_t i = _func##__call_count;
_MOCK_REGISTER(_func)
_MOCK_OVERFLOW_CHECK_R(_func, _rtype)
p = &_func##__param_history[i];   /* capture params */
_MOCK_STORE_N
ret = _func##__return_queue[i];   /* (*) load return BEFORE callback */
_MOCK_ACTION_LOOP(_func, ...)     /* mock_param_mem_*/_str_* fan-out      */
_MOCK_CALLBACK_N(_func)           /* user callback, void-returning        */
_func##__call_count++;
return ret;
```

The marked `(*)` line is the friction. By the time `__callback` runs:

- `ret` is already a local snapshot of `__return_queue[i]`. Mutating
  `__return_queue[i]` from inside the callback is a no-op for *this* call
  (it would only affect a hypothetical later call at a higher index).
- The callback's signature is `void (size_t, params...)`. It cannot
  signal a different return value to the wrapping mock body. The
  void-return is the constraint that prevents fixing this in place.

So if a test wants "return `true` from `cweb_fio_is_file(path)` iff this
test has previously seeded `path`," it cannot phrase that against the
callback. It has to enumerate calls in advance:

```c
/* Test-side, today: */
cweb_fio_write_file__return_queue[0] = 0;       /* SUT writes "a" */
cweb_fio_is_file__return_queue[0]    = true;    /* SUT later checks "a" */
cweb_fio_is_file__return_queue[1]    = false;   /* and checks "b" */
/* Now invoke the SUT and pray you got the call ordering right. */
```

…which fails as soon as control flow inside the SUT depends on the
returns, or as soon as the test's mental model is "files exist or they
don't" rather than "this is call #N." The test ends up encoding the SUT's
internal call order, which is exactly the coupling the mock framework
exists to avoid.

The existing `cweb_fio_walk__mock_set` helper inside `lfg/cweb` is the
existing precedent that bridges this gap *for one specific seam* (a walk
yielding a list of paths) — it bundles queue-priming with state-shaped
inputs. The question is whether to generalize that pattern.

## Worked example: state-driven `is_file`

A SUT that accepts a manifest path, walks it line-by-line, and skips any
line that doesn't name an existing file:

```c
void
process_manifest(const char *manifest_path)
{
    cweb_fio_foreach_line(manifest_path, line_cb, &state);
}

static void
line_cb(const char *line, void *user)
{
    if (cweb_fio_is_file(line))
    {
        do_thing(line);
    }
}
```

Test goal: "manifest mentions `a` and `c`; `a` exists, `c` doesn't."
Today the test must:

1. Prime `cweb_fio_foreach_line__return_queue[0] = 0` and arrange via a
   callback to invoke the line-cb twice with `"a"` and `"c"`.
2. Know that the SUT will call `cweb_fio_is_file` exactly twice, in that
   order, and prime `cweb_fio_is_file__return_queue[0] = true` and
   `[1] = false`.

If `do_thing` happens to itself call `cweb_fio_is_file` for some
unrelated reason, the `[0]/[1]` slot accounting silently shifts and the
test asserts wrongly. The test is order-coupled to SUT internals.

What the test *wants* to write:

```c
fake_fs_install();
fake_fs_set_file("a", "...");
/* (no entry for "c") */
process_manifest("manifest");
ASSERT_TRUE(do_thing__call_count == 1);
ASSERT_STREQ(do_thing__param_history[0].p0, "a");
```

…with *no* per-call return queue priming. That requires the framework or
the consumer to compute the `is_file` return from a path lookup at call
time.

## Approach 1 — framework-level: extend the R_* callback to decide the return

The void-return on the R_* callback typedef is the only thing preventing
the existing `__callback` mechanism from solving this in place. Lift
that constraint: change the R_* callback typedef from
`void (size_t, params...)` to `_rtype (size_t, params...)`. The mock
body assigns the callback's return value to `ret` when the callback is
set, overriding the queue load.

V_* (void-return) mocks are unchanged — they have no return to decide.

### API sketch

For an R_N mock of `int foo(T0, T1)`:

```c
/* WAS: typedef void (*foo__callback_t)(size_t, T0, T1); */
typedef int (*foo__callback_t)(size_t, T0, T1);
extern foo__callback_t foo__callback;
```

Codegen change inside each `DEFINE_MOCK_R_*` body. The existing
`_MOCK_CALLBACK_N` invocation helper is shared between V_* and R_*
today; the cleanest implementation is to split it into V- and R-flavored
variants, or inline the R_* invocation directly:

```c
/* WAS: if (foo__callback) foo__callback(i, _p0, _p1); */
/* NOW: */
if (foo__callback)
{
    ret = foo__callback(i, _p0, _p1);
}
```

A side-effects-only callback that wants to preserve queue-driven returns
spells that intent explicitly:

```c
static int
my_side_effect(size_t i, const char *path)
{
    do_thing();
    return foo__return_queue[i];
}
```

A state-driven callback ignores the queue:

```c
static bool
my_is_file(size_t i, const char *path)
{
    (void)i;
    return fake_fs_has(path);
}
```

For `_S` variants, the signature takes the struct by value (matching the
existing `_S` callback shape) and gains the `_rtype` return.

### Properties

- **One symbol per R_* mock.** No new typedef, no new function pointer,
  no new row in the README's "Generated symbols" table. The existing
  symbol gets a richer signature.
- **Opt-in per-mock at runtime.** Default behavior — NULL callback —
  preserves the queue-driven path byte-for-byte. Only tests that *set*
  the callback are affected.
- **Composes with the queue.** A callback that returns
  `foo__return_queue[i]` delegates to the queue; one that ignores the
  queue computes `ret` from state. The two strategies coexist on a
  per-test, per-mock basis.
- **V_* unchanged.** V mocks have no return to decide; their callback
  stays `void (size_t, params...)`.

### Trade-offs

- **(–) Breaking change for R_* callbacks.** The callback typedef
  changes from `void (...)` to `_rtype (...)` for R-style mocks. Every
  existing R_* callback in lfg-ctest's self-tests and in downstream
  consumers (currently `lfg/cweb`) needs a one-line update to its
  function signature, plus an explicit `return foo__return_queue[i];`
  where the caller wanted queue-driven behavior. The compiler catches
  every site via typedef mismatch at the `foo__callback = …` assignment.
  Migration is mechanical, but it is a real source-level break —
  affordable for a pre-1.0 framework with one in-repo consumer; would
  be a much harder sell post-1.0.
- **(–) V_*/R_* callback signatures diverge.** V_* keeps
  `void (size_t, params...)`; R_* returns `_rtype`. The README has to
  say "R callbacks decide the return; V callbacks are side-effects only."
  Mild documentation wart; the asymmetry is directly justified by the
  return-type asymmetry.
- **(+) One conceptual model.** "The callback decides what this call
  does (return + side effects)." No queue-vs-fn disambiguation in docs
  or in the test author's head.
- **(+) Solves the underlying mismatch for every state-coupled seam, not
  just `cweb_fio`.** Any consumer hitting the same shape — a network
  mock keyed on URL, a key-value mock keyed on key — gets the ergonomic
  for free.
- **(0) Codegen cost.** One existing branch (`if (__callback)`) gets one
  assignment from the callback's return added to it. No new symbols, no
  new typedefs. The `_MOCK_CALLBACK_N` invocation helper splits or
  inlines per the implementation note above.

### Considered and rejected: a separate `__return_fn` symbol

An additive design (`__return_fn` next to the existing void `__callback`)
would avoid the source-level break. Costs of that design: a second
symbol per R-style mock, a second typedef, a doc burden of explaining
when to reach for which (the state-driven callback would have to live in
one function, side effects in another, or one would consult the other).
Rejected — the smaller API surface and single conceptual model of the
merged callback outweigh the one-time mechanical migration. (See PR #7
discussion.)

### Considered and rejected: out-pointer signature

Keeping the R_* callback's return type `void` and adding `_rtype *out_ret`
to the signature was considered. It preserves V_*/R_* return-type
symmetry, but the out-pointer is uglier at the call site
(`*out = state_lookup(path)` vs. `return state_lookup(path)`) and forces
an awkward "if you want the queue value, write `*out = foo__return_queue[i];`"
idiom. Rejected in favor of the rtype-return form.

## Approach 2 — consumer-level: `cweb_fio_fake` helper, no framework change

A pure-cweb helper layered on the existing `__return_queue` / `__callback`
primitives, per the reporter sketch:

```
deps/cweb/mock/cweb_fio_fake.[ch]   (or fold into existing test infra)

cweb_fio_fake_install()
    mock_reset_all();
    register_state_clear_hook();
    cweb_fio_write_file__callback   = on_write;
    cweb_fio_append_file__callback  = on_append;
    cweb_fio_foreach_line__callback = on_foreach;

cweb_fio_fake_set_file(path, data, size)   /* test seeds state           */
cweb_fio_fake_get(path, &data, &size)      /* test inspects what SUT wrote */
cweb_fio_fake_predict_is_file_true(slot)   /* sugar over __return_queue[N] */
```

### Properties

- **Zero framework change.** Lives entirely in `lfg/cweb`.
- **Captures writes cleanly.** `cweb_fio_write_file__callback` runs with
  full param visibility and can `memcpy` bytes into a fake-fs hashtable.
  This part works fine.
- **Drives `foreach_line` cleanly.** A callback can iterate the seeded
  state and synthesize line invocations. Also fine.

### Limits

- **Does not solve `is_file`/`exists`/related.** The boilerplate the
  helper removes is roughly 60-70% of the per-test setup. The remaining
  30-40% — predicting which call-index of `is_file` corresponds to which
  path — is exactly the queue-vs-state mismatch that lives in the
  framework, and the helper cannot move it. `predict_is_file_true(slot)`
  is honest about this: it only assigns to `__return_queue[N]` and the
  test still has to know `N`.
- **Per-consumer reinvention.** Any other consumer with the same mocked-
  I/O shape (a future `lfg/foo_io`) rebuilds the same scaffolding from
  scratch.

## Approach 3 — both

Approach 1 (extended R_* callback) plus Approach 2 (cweb-side helper that
*uses* the new callback semantics). With the merged callback, every
fake-fs hookup is a single `__callback` assignment regardless of whether
the mock's role is "capture writes" (V or R, side effects) or "decide
returns from state" (R, returns rtype):

```
cweb_fio_fake_install()
    mock_reset_all();
    cweb_fio_write_file__callback   = on_write;     /* side effect: capture bytes */
    cweb_fio_foreach_line__callback = on_foreach;   /* side effect: synthesize lines */
    cweb_fio_is_file__callback      = on_is_file;   /* returns bool from state map */
    cweb_fio_exists__callback       = on_exists;    /* returns bool from state map */

cweb_fio_fake_set_file(path, data, size)
cweb_fio_fake_get(path, &data, &size)
/* predict_is_file_true and friends become unnecessary */
```

Tests stop priming per-call return slots entirely.

## Approach 4 — neither

Document the queue-vs-state mismatch in the lfg-ctest README under "When
to use mocks" and let consumers handle it case-by-case. Each test that
needs it manually sets up a state map and primes the queue by hand.

### Properties

- (+) Zero new code, zero new API surface.
- (–) Every state-coupled mock seam in every consumer rebuilds the same
  scaffolding by hand. The reporter's discomfort with "this whole layer
  is generic to cweb_fio — any consumer rebuilds it" stands.
- (–) The order-coupling between SUT internals and test queue indices
  remains a recurring footgun.

## Trade-off summary

| Axis | Approach 1 | Approach 2 | Approach 3 | Approach 4 |
|------|-----------|-----------|-----------|-----------|
| API surface (lfg-ctest) | unchanged (no new symbols) | unchanged | unchanged (no new symbols) | unchanged |
| Backward compat for R_* callbacks | source-level break (mechanical migration; compiler-caught) | preserved | source-level break | preserved |
| Backward compat for runtime behavior | preserved (NULL callback unchanged) | preserved | preserved | preserved |
| Test ergonomics for is_file/exists | strong | weak | strong | none |
| Test ergonomics for write+read+foreach | weak (still need helper layer) | strong | strong | none |
| Consumer-agnostic invariant (lfg-ctest) | preserved | preserved | preserved | preserved |
| Cross-repo coordination | single-repo (lfg-ctest) | single-repo (cweb) | two repos in sequence | none |
| Reusable beyond cweb | yes | no | yes | no |

The R_* callback signature break is the load-bearing axis. It is a
one-time, compiler-caught, mechanical migration; pre-1.0 framework with
a single in-repo consumer makes it affordable.

## Recommendation

**Approach 3, in two ordered steps:**

1. **Upstream first**: file an lfg-ctest issue to extend the R_*
   callback typedef to return `_rtype`, per Approach 1. The change is
   opt-in at runtime (NULL callback preserves the queue path) and
   breaks compilation for any existing R_* callback — the compiler
   catches every site, and the migration is a one-line signature update
   plus an explicit `return foo__return_queue[i];` where the caller
   wanted queue-driven behavior. lfg-ctest's own self-tests and any
   in-repo callers are migrated in the same change.

2. **Downstream follow-up**: file an lfg/cweb issue (draft body in the
   next section) to ship `cweb_fio_fake` as a helper that consumes the
   new callback semantics. Luis's call once the upstream piece is in;
   the doc does not commit to it on his behalf.

Per issue #6's preference #1, the recommendation is framed in the
framework-vs-consumer split: the framework piece is the bottleneck,
and the consumer fake is downstream of that decision. Per preference
#2, the framework knob is opt-in per-mock — default-off — and does
not change runtime behavior for any test that does not set the
callback. The break is purely at the source level for the small number
of tests that already use R_* callbacks.

We pick "both" because the framework primitive alone leaves cweb tests
to rewrite the state-map scaffolding by hand (Approach 4's failure
mode), and the consumer helper alone leaves the queue-vs-state
mismatch unresolved (Approach 2's failure mode).

## Draft body for the lfg-ctest follow-up issue

Title: `mock R_*: extend __callback to return _rtype`

Body:

> ## Summary
>
> Change the R_* callback typedef from `void (size_t, params...)` to
> `_rtype (size_t, params...)`. Inside each `DEFINE_MOCK_R_*` body,
> assign the callback's return value to `ret` when the callback is set,
> overriding the queue load. V_* mocks unchanged.
>
> Motivation, design rationale, and trade-offs in
> `.ai/state-driven-mocks.md`.
>
> ## Codegen change
>
> In each `DECLARE_MOCK_R_*` and `DEFINE_MOCK_R_*`:
>
> - Update the callback typedef:
>
>   ```c
>   /* WAS: */ typedef void   (*_func##__callback_t)(size_t, /* params */);
>   /* NOW: */ typedef _rtype (*_func##__callback_t)(size_t, /* params */);
>   ```
>
> - Update the callback invocation to assign the returned value to `ret`:
>
>   ```c
>   /* WAS: if (_func##__callback) _func##__callback(i, _p0); */
>   /* NOW: if (_func##__callback) ret = _func##__callback(i, _p0); */
>   ```
>
> The existing `_MOCK_CALLBACK_N` invocation helpers are shared between
> V_* and R_* today. Splitting them into V- and R-flavored variants
> (`_MOCK_CALLBACK_V_N` / `_MOCK_CALLBACK_R_N`) is the cleanest way to
> apply this; the alternative is to inline the R_* invocation directly
> in the `DEFINE_MOCK_R_*` body.
>
> The `_S` variants follow the same shape (struct-by-value param,
> rtype return).
>
> ## Migration
>
> Self-test callbacks in `test-mock.c` and any in-repo callers update
> their callback function signatures: change return type from `void` to
> the mock's `_rtype`, and add `return <mock>__return_queue[i];` (or
> equivalent) where the test wanted queue-driven behavior. The compiler
> catches every site via typedef mismatch at the `<mock>__callback = …`
> assignment.
>
> ## Acceptance criteria
>
> - All existing self-tests pass after migration.
> - New self-test in `test-mock.c` covers: (a) callback return overrides
>   queue when callback is set, (b) queue is used when callback is NULL,
>   (c) callback that returns `<mock>__return_queue[i]` reproduces
>   today's "side effects + queue-driven return" pattern, (d) the `_S`
>   variant follows the same shape.
> - `dist/lfg-ctest.h` (amalgamation) regenerates against the new shape
>   and `test-amalg` passes.
> - `README.md` "Generated symbols" table updated; the section on
>   `__callback` distinguishes V_* (void) and R_* (returns rtype) shapes
>   and shows both the queue-driven and state-driven idioms.

## Draft body for the lfg/cweb follow-up issue

Title: `mock: cweb_fio_fake state-driven test helper`

Body:

> ## Summary
>
> Ship `cweb_fio_fake.[ch]` (location bikesheddable: a `mock/` subdir
> next to existing `cweb_fio_mock.c`, or folded into existing test
> infra). Public surface:
>
> - `cweb_fio_fake_install()` — calls `mock_reset_all()`, registers a
>   reset hook to clear fake state on next reset, and wires:
>   - `cweb_fio_write_file__callback` / `__append_file__callback`
>     to capture writes into the fake state map (side-effect callbacks);
>   - `cweb_fio_foreach_line__callback` to iterate the captured state
>     for the requested path and synthesize line invocations
>     (side-effect callback that returns the queue value to drive its
>     own return);
>   - `cweb_fio_is_file__callback` / `cweb_fio_exists__callback` to
>     compute returns from the fake state map (rtype-returning
>     callbacks; depends on the lfg-ctest typedef change — see the
>     upstream issue).
> - `cweb_fio_fake_set_file(path, data, size)` — test-side seeding.
> - `cweb_fio_fake_get(path, &data, &size)` — inspect bytes the SUT
>   wrote.
>
> ## Depends on
>
> lfg/ctest#<N> (the R_* callback typedef change must land first).
>
> ## Constraints
>
> - Helper must not introduce a circular `cweb` ↔ `cweb_fio_fake`
>   dependency: the fake lives next to the existing mock, not in
>   production paths.
> - The fake state map is fixed-size or registry-backed; no malloc
>   inside the mock callbacks if it would surprise.
>
> ## Acceptance criteria
>
> - A self-test demonstrates the worked example from
>   `lfg/ctest/.ai/state-driven-mocks.md` (manifest walking with
>   selective is_file).
> - `cweb_fio_fake_install()` is idempotent and safe to call from
>   per-test setup.
> - Reset hook clears the state map on `mock_reset_all()`.

## Notes for implementers

- The codegen change is small but touches every R-style macro family —
  `R_V`, `R_1`..`R_9`, `R_V_S`, `R_1_S`..`R_6_S`. Test-amalg drift is
  the most likely regression vector; ensure `tools/amalgamate.c` does
  not need changes (it shouldn't — only typedef and invocation forms
  change, no new symbols cross the amalgamation boundary).
- The current `_MOCK_CALLBACK_N` invocation helpers in
  `lfg-ctest-mock.h` are shared by V- and R-style mocks. Splitting
  them — `_MOCK_CALLBACK_V_N` for void-return invocation,
  `_MOCK_CALLBACK_R_N` for rtype-return-and-assign — keeps the
  `DEFINE_MOCK_*` bodies symmetric with today.
- Ordering inside the mock body: the callback continues to run after
  `_MOCK_ACTION_LOOP`, so a state-driven callback sees param-actions'
  effects on parameter buffers if any are configured. Param values
  themselves are unchanged across the action loop, so the common case
  (state lookup keyed on a parameter) doesn't depend on this ordering.
- `__mock_reset` already NULLs `__callback`; no change needed.
