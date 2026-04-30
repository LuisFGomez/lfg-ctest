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
  signal a different return value to the wrapping mock body.

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

## Approach 1 — framework-level: opt-in return-decider hook

Introduce a NULL-by-default function pointer per R-style mock that, if
set, computes the return value from the captured params. When unset, the
existing `__return_queue[i]` path is used unchanged.

### API sketch

For an R_N mock of `int foo(T0, T1)`:

```c
/* Generated alongside foo__callback / foo__return_queue: */
typedef int (*foo__return_fn_t)(size_t call_index, T0, T1);
extern foo__return_fn_t foo__return_fn;
```

(Name `__return_fn` is consistent with the existing `__callback` pattern;
bikesheddable — `__compute_return`, `__return_decider`, `__return_for`
were all considered.)

Codegen change inside the R_N mock body, replacing the single
`ret = _func##__return_queue[i];` line:

```c
if (_func##__return_fn)
{
    ret = _func##__return_fn(i, _p0, _p1);
}
else
{
    ret = _func##__return_queue[i];
}
```

`__mock_reset` NULLs the pointer (one extra line, mirroring the existing
`__callback = NULL` in `_MOCK_RESET_R`). For `_S` variants, the signature
takes the struct by value, matching the existing `_S` callback shape.
For `R_V` (no params), the signature is `_rtype (size_t)`.

Test usage — the worked example above:

```c
static bool
my_is_file(size_t i, const char *path)
{
    (void)i;
    return fake_fs_has(path);
}

cweb_fio_is_file__return_fn = my_is_file;
```

### Properties

- **Opt-in per-mock.** Default codegen behavior is unchanged: every
  existing test that uses `__return_queue[i]` keeps working byte-for-byte.
  A mock with a NULL `__return_fn` (the default) behaves exactly as it
  does today.
- **Composes (or doesn't, by choice).** A return-fn that wants the queue
  as a fallback can read `_func##__return_queue[i]` itself — those arrays
  are public. A return-fn that wants pure state computation just ignores
  the queue.
- **No new macro fanout.** A single edit to each `DEFINE_MOCK_R_*` body
  (and the matching reset and `DECLARE_MOCK_R_*`) covers V/0/1/.../9 and
  the `_S` set. The amalgamation manifest is unchanged.
- **No effect on V_* mocks.** Void-return mocks have nothing to decide;
  the existing `__callback` already covers their needs.
- **Codegen cost.** One branch per call, predictable, optimizes to nothing
  when the pointer is NULL. One additional symbol per R-style mock
  (`__return_fn` + typedef).

### Trade-offs

- (–) Adds one publicly-named symbol to every R-style mock. README's
  "Generated symbols" table grows by a row.
- (–) Documentation surface: the public API now has two ways to influence
  a mock's return (the queue and the fn). The doc has to explain when to
  reach for which. Recommended phrasing: "queue for fixed/per-call
  values; return-fn for state-driven values; setting the fn replaces the
  queue load for that mock."
- (+) Solves the underlying mismatch for *every* future state-coupled
  seam, not just the `cweb_fio` ones that motivated this. Any consumer
  hitting the same shape — a network mock keyed on URL, a key-value mock
  keyed on key — gets the ergonomic for free.

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

Approach 1 (framework primitive) plus Approach 2 (cweb-side helper that
*uses* the primitive). With `__return_fn` available, the helper sketch
collapses:

```
cweb_fio_fake_install()
    mock_reset_all();
    cweb_fio_write_file__callback   = on_write;
    cweb_fio_foreach_line__callback = on_foreach;
    cweb_fio_is_file__return_fn     = on_is_file;   /* NEW: state-driven */
    cweb_fio_exists__return_fn      = on_exists;    /* NEW: state-driven */

cweb_fio_fake_set_file(path, data, size)
cweb_fio_fake_get(path, &data, &size)
/* predict_is_file_true and friends become unnecessary */
```

Tests stop priming per-call return slots entirely. The helper is smaller
than under Approach 2 because the slot-prediction sugar drops out.

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
| Codegen complexity (lfg-ctest) | + one branch & one symbol per R-mock | none | + one branch & one symbol per R-mock | none |
| Test ergonomics for is_file/exists | strong | weak | strong | none |
| Test ergonomics for write+read+foreach | weak (still need callbacks) | strong | strong | none |
| Consumer-agnostic invariant (lfg-ctest) | preserved (helper is per-mock, types defined by user) | preserved | preserved | preserved |
| Cross-repo coordination | single-repo (lfg-ctest) | single-repo (cweb) | two repos in sequence | none |
| Backward compat for existing tests | full (NULL default) | full | full | full |
| Reusable beyond cweb | yes | no | yes | no |

The single-repo-vs-cross-repo distinction is the load-bearing axis.
Approach 1 alone fixes the worst ergonomic in lfg-ctest and enables a
future cweb decision; Approach 2 alone does the easy 60% in cweb but
leaves the hard 40% unsolved; Approach 3 is the right sequence; Approach
4 punts.

## Recommendation

**Approach 3, in two ordered steps:**

1. **Upstream first**: file an lfg-ctest issue to add the opt-in
   `__return_fn` hook per Approach 1's sketch. This is the strict
   prerequisite — it is the only piece that resolves the queue-vs-state
   mismatch, and it is consumer-agnostic, opt-in per-mock, and
   default-off. Per issue #6's preference #2, the framework knob does
   not change default codegen behavior.

2. **Downstream follow-up**: file an lfg/cweb issue (draft body in the
   next section) to ship `cweb_fio_fake` as a helper that consumes the
   new framework primitive. This is Luis's call to make once the
   primitive is in — the doc does not commit to it on his behalf.

Per issue #6's preference #1, the recommendation is framed in the
framework-vs-consumer split: the framework piece is the bottleneck, and
the consumer fake is downstream of that decision. We pick "both" because
shipping only the framework primitive leaves cweb tests to rewrite the
state-map scaffolding by hand (Approach 4's failure mode), and shipping
only the cweb helper leaves the queue-vs-state mismatch unresolved
(Approach 2's failure mode).

## Draft body for the lfg-ctest follow-up issue

Title: `mock R-style: opt-in __return_fn hook for state-driven returns`

Body:

> ## Summary
>
> Add an opt-in, NULL-by-default function-pointer `<func>__return_fn` to
> every R-style generated mock (`R_V`, `R_1`..`R_9`, and the `_S`
> equivalents). When set, the mock body invokes it in place of loading
> from `<func>__return_queue[i]`. When NULL (the default), the existing
> queue path is taken, byte-for-byte.
>
> Motivation, design rationale, and trade-offs in
> `.ai/state-driven-mocks.md`.
>
> ## Codegen change
>
> Replace, in each `DEFINE_MOCK_R_*` body, the single line
> `ret = _func##__return_queue[i];` with:
>
> ```c
> if (_func##__return_fn)
> {
>     ret = _func##__return_fn(i, /* params... */);
> }
> else
> {
>     ret = _func##__return_queue[i];
> }
> ```
>
> Add `_func##__return_fn = NULL;` to `_MOCK_RESET_R`. Declare typedef
> `<func>__return_fn_t` and `extern` symbol in each `DECLARE_MOCK_R_*`.
> For `_S` variants, the signature takes the struct by value, matching
> the existing `_S` callback shape.
>
> ## Acceptance criteria
>
> - All existing self-tests pass unchanged.
> - New self-test in `test-mock.c` covers: (a) `__return_fn` overrides
>   the queue when set, (b) the queue is used when `__return_fn` is
>   NULL, (c) `__mock_reset` NULLs `__return_fn`, (d) the chosen
>   semantic for `_S` variants.
> - `dist/lfg-ctest.h` (amalgamation) regenerates with the new symbols
>   and `test-amalg` passes.
> - `README.md` "Generated symbols" table gains a `__return_fn` row;
>   "When to use" guidance distinguishes queue vs. fn use cases.

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
>     to capture writes into the fake state map;
>   - `cweb_fio_foreach_line__callback` to iterate the captured state
>     for the requested path;
>   - `cweb_fio_is_file__return_fn` / `cweb_fio_exists__return_fn` to
>     compute returns from the fake state map (depends on the lfg-ctest
>     `__return_fn` primitive — see the upstream issue).
> - `cweb_fio_fake_set_file(path, data, size)` — test-side seeding.
> - `cweb_fio_fake_get(path, &data, &size)` — inspect bytes the SUT
>   wrote.
>
> ## Depends on
>
> lfg/ctest#<N> (the `__return_fn` hook must land first).
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
  `R_V`, `R_1`..`R_9`, `R_V_S`, `R_1_S`..`R_6_S`. Test-amalg drift is the
  most likely regression vector; ensure `tools/amalgamate.c` does not need
  changes (it shouldn't — the new symbol is named under the existing
  per-mock convention).
- The `__return_fn` signature exactly mirrors `__callback`'s plus a
  return type. Documentation should treat the two as a pair: same
  call-site visibility (param history is already populated by the time
  either runs in the proposed order), same lifecycle (NULL'd by reset,
  set per-test).
- Ordering inside the new mock body: `__return_fn` resolves `ret`
  *before* `_MOCK_ACTION_LOOP` and `_MOCK_CALLBACK_N`. This matches the
  current ordering relative to `__return_queue[i]`. Param-actions and
  callback continue to operate on (potentially mutated) parameter
  buffers and observe the final `ret` indirectly through subsequent
  state checks, not through their signatures.
