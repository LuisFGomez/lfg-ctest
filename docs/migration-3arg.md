# migration-3arg.md — migrating off registration-time setup/teardown

The pre-body-only API bound setup and teardown at **registration time**:

```c
lfg_ct_test(setup, test_fn, teardown);      /* NULL, test_fn, NULL for fixtureless */
lfg_ct_suite(setup, suite_fn, teardown);
```

That signature is **removed**. `lfg_ct_test` / `lfg_ct_suite` now take the body
only; setup and teardown are plain static functions the body calls itself. The
rationale, the crash-safety analysis behind it, and the decomposition into
child issues are in the design spike (lfg/ctest#35, `## Spike output`). The
canonical convention that replaces the mechanism lives in
[api.md — Setup and Teardown](api.md#setup-and-teardown) and is walked
end-to-end in [tutorial chapter 6](tutorial/06-teardown-and-cleanup.md). This
page is the **migration recipe**: how to move existing call sites over. It does
not restate the convention — follow the links for the patterns.

## The transform is two-shaped

Every old 3-argument call site is one of two shapes. Sort your sites first; the
first shape is scriptable, the second is a per-site human edit.

### Shape 1 — fixtureless (mechanical, scriptable)

```c
lfg_ct_test(NULL, test_fn, NULL)   ->   lfg_ct_test(test_fn)
lfg_ct_suite(NULL, suite_fn, NULL) ->   lfg_ct_suite(suite_fn)
```

Nothing moves; the `NULL, ... , NULL` wrapper is stripped. Most call sites in a
typical repo are this shape (the `NULL` pollution the revert set out to remove).

Run the bundled codemod — [`tools/migrate-3arg.sh`](../tools/migrate-3arg.sh) —
over your test tree. It is a dry run by default: it prints what it would strip
**and** lists every Shape-2 site it deliberately left for you, changing
nothing. Re-run with `--apply` to rewrite in place.

```bash
# from your consumer repo, pointed at lfg/ctest's copy of the script:
path/to/lfg-ctest/tools/migrate-3arg.sh tests/            # dry run: preview + residue list
path/to/lfg-ctest/tools/migrate-3arg.sh --apply tests/    # rewrite the fixtureless sites
```

If you would rather not run the script, the mechanical case is a one-line
`sed` (GNU sed; whitespace-tolerant, handles both `test` and `suite`):

```bash
sed -E -i \
  's/(lfg_ct_(test|suite))[[:space:]]*\([[:space:]]*NULL[[:space:]]*,[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*,[[:space:]]*NULL[[:space:]]*\)/\1(\3)/g' \
  $(git ls-files 'tests/*.c')
```

The script is preferred over the bare `sed` because it also enumerates the
Shape-2 residue you still have to hand-edit — the `sed` silently leaves those
alone with no report.

> **Single-line assumption.** The codemod, the `sed`, and the verification grep
> below are all line-oriented. A single 3-arg registration split across multiple
> physical lines is neither stripped, reported as NEEDS HUMAN, nor caught by the
> verification grep — it silently passes as if migrated. Reflow such call sites
> onto one line (`clang-format` does this) before running the tooling.

### Shape 2 — fixtured (per-call-site human edit)

```c
lfg_ct_test(my_setup, test_fn, my_teardown)   ->   lfg_ct_test(test_fn)
```

...and `my_setup()` / `my_teardown()` move **into the body**. This is not
scriptable: where the calls go, and whether the failure-count / pre-skip
patterns apply, is a judgement per site. The codemod flags these ("NEEDS
HUMAN") rather than touching them.

Before:

```c
static void my_setup(void)    { mock_reset_all(); seed_fixture(); }
static void my_teardown(void) { mock_reset_all(); free_fixture(); }

void test_fn(void)
{
    ASSERT_EQ(expected, do_work());
}

void suite(void)
{
    lfg_ct_test(my_setup, test_fn, my_teardown);
}
```

After — the body brackets itself; registration is body-only:

```c
static void my_setup(void)    { mock_reset_all(); seed_fixture(); }
static void my_teardown(void) { mock_reset_all(); free_fixture(); }

void test_fn(void)
{
    my_setup();
    ASSERT_EQ(expected, do_work());     /* soft-fails still reach teardown */
    my_teardown();
}

void suite(void)
{
    lfg_ct_test(test_fn);
}
```

Two rules from the convention carry the weight of the old guarantee — do **not**
skip them when a site has an early exit or a fallible setup:

- **Teardown before an early exit.** An early `return` or `lfg_ct_skip(...)` is
  the only way to leave a body *before* the trailing `teardown()`; call
  `teardown()` immediately before either. "Teardown before skip" is
  load-bearing (the skip `longjmp`s past a trailing in-body call).
- **Fallible setup.** If `setup()` itself drives assertions, snapshot
  `lfg_ct_failure_count()` before it and compare after, then `teardown()` +
  `lfg_ct_skip(reason)` if the count moved.

Both patterns, with worked code, are in
[api.md — Setup and Teardown](api.md#setup-and-teardown) /
[Failure count](api.md#failure-count) and
[tutorial chapter 6](tutorial/06-teardown-and-cleanup.md#setup-that-can-fail).
For the common mock-only case, calling `mock_reset_all()` at the **top** of each
test is idempotent and sidesteps the early-exit question entirely — reach for
that before writing a bottom-of-body `teardown()`.

## Suites migrate the same way

`lfg_ct_suite(setup, suite_fn, teardown)` follows the identical two shapes. A
suite is just a body that calls `lfg_ct_test`, so shared per-suite setup/teardown
brackets the `lfg_ct_test` calls inside the suite body:

```c
void math_suite(void)
{
    suite_setup();
    lfg_ct_test(test_addition);
    lfg_ct_test(test_subtraction);
    suite_teardown();
}
```

Note the `--list` interaction: under `--list` the suite body is still invoked so
its tests can list their names, so gate a side-effectful `suite_setup()` behind
`if (!lfg_ct_is_list_mode())`. See
[api.md — Setup and Teardown](api.md#setup-and-teardown).

## Sequencing: lockstep or staged

Two ways to land the version bump and the migration relative to each other:

- **Lockstep (flag day).** Bump the vendored lfg/ctest and migrate every call
  site in the same PR. Cleanest end state, no compat surface — but the whole
  test tree must move at once.
- **Staged, via the compat shim.** If a flag day is impractical, build with
  `-DLFG_CT_COMPAT_3ARG` (on the library **and** every consumer TU) to restore
  the old 3-argument macros for **one release window**. They forward to the
  retained lifecycle helper and emit a deprecation `#warning`. Migrate call
  sites incrementally, then drop the define. The shim is slated for removal
  after one release; do not treat it as a resting state. Full contract:
  [api.md — Deprecated: 3-argument registration](api.md#deprecated-3-argument-registration-lfg_ct_compat_3arg).

Whether the shim is available is an owner call per the milestone plan; if it was
not taken, only the lockstep path exists.

## Verify a migrated repo

After migrating, confirm no 3-argument registration survives, then rebuild:

```bash
# any surviving 3-arg lfg_ct_test / lfg_ct_suite registration:
grep -rnE 'lfg_ct_(test|suite)[[:space:]]*\([^()]*,[^()]*,[^()]*\)' $(git ls-files '*.c')
```

Zero hits (outside the shim path) means every site is body-only. Rebuild and run
the suite: if you vendor the single-header amalgam (`dist/lfg-ctest.h`),
regenerate it against the bumped source first (`cmake --build build --target
amalgamate`) so the vendored copy carries the reverted signatures.

## Adopter tracking

Each consumer repo migrates in **its own PR** — the code edits do not land in
lfg/ctest. This table is the coordination view; update the status as each repo's
migration PR merges.

| Adopter repo | Status | Migration PR |
|--------------|--------|--------------|
| cweb | pending | — |
| clib | pending | — |
| jotsms | pending | — |
| license-server | pending | — |
| sf-* (each sf- repo on the test-overhaul) | pending | — |
| remaining test-overhaul migrations | pending | — |

Status legend: **pending** (not started) → **in progress** (PR open) →
**done** (PR merged, `dist/` regenerated if vendored). A repo is done only once
its `grep` verification above is clean and its suite builds green against the
reverted API.
