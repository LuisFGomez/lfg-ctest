# architecture.md — lfg-ctest internals

This doc describes how the framework itself is put together. For the public
API, see `README.md`.

## Two concerns, one static library

CMake produces a single archive `lfg_ctest` from:

- `lfg_ctest.c` — test runner and assertion implementations.
- `lfg_ctest_mock.c` — mock runtime (param-action chain, reset registry).

Consumers include `lfg_ctest.h` for assertions/runner and optionally
`lfg_ctest_mock.h` for mocking. The two headers are independent; you can use
the runner without ever pulling in the mock macros.

## Core runner (`lfg_ctest.c`)

State lives in a small set of file-scope statics (pass/fail counters, current
test name, etc.). All public entry points (`lfg_ct_start`, `lfg_ctest`,
`lfg_ct_suite`, `lfg_ct_print_summary`, `lfg_ct_return`) mutate this state.
There is no reentrancy guarantee — one test run at a time.

Every assertion macro in `lfg_ctest.h` ultimately routes to an internal
failure path that:

1. Formats a diagnostic (including `__func__` if available — see the
   `LFG_CTEST_NO_FUNC` / `__FUNCTION__` fallback chain in `README.md`).
2. Writes to stderr **unless** expect-failures mode is active.
3. Increments a counter.

## Self-test mode (`LFG_CTEST_SELF_TEST`)

The CMakeLists sets `LFG_CTEST_SELF_TEST=1` on the library and both self-test
binaries when this repo is the top-level CMake source
(`CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR`). When built as a
subproject, self-tests and this flag are off, and the extra internal API stays
hidden.

The flag gates:

- `lfg_ct_expect_failures_begin()` / `lfg_ct_expect_failures_end()` —
  declared in `lfg_ctest.h` under `#ifdef LFG_CTEST_SELF_TEST`.
- The `_expect_failures_mode` static in `lfg_ctest.c` and the branches in
  `_lfg_ct_fail`-path macros that consult it.

### Expect-failures mode

Used to verify that assertion macros *actually fail* without polluting test
output. Pattern:

```c
int expected_failures = 6;
int actual_failures;

lfg_ct_expect_failures_begin();
ASSERT_EQ(1, 2);           /* would normally fail loudly */
ASSERT_TRUE(0);            /* counted silently */
/* ... */
actual_failures = lfg_ct_expect_failures_end();
ASSERT_INT_EQUAL(expected_failures, actual_failures);
```

While the mode is active, stderr output from failing assertions is suppressed
and the pass/fail counters are not incremented at the top level — only the
local failure counter increments. `_end()` returns the count and restores
normal behavior.

Every "does assertion X fail correctly?" self-test in `test_unified.c` uses
this pattern.

## Mock system (`lfg_ctest_mock.[ch]`)

### Macro fanout

`DECLARE_MOCK_` / `DEFINE_MOCK_` come in these shapes:

- `V_V`, `V_1` … `V_9` — void return, N parameters (0–9).
- `R_V`, `R_1` … `R_9` — value return, N parameters (0–9).
- `V_V_S`, `V_1_S` … `V_3_S` — void return, struct-by-value parameter.
- `R_V_S`, `R_1_S` … `R_6_S` — value return, struct-by-value parameter.

Every `DEFINE_MOCK_*` generates the same suite of symbols for the target
function `foo`:

| Symbol | Purpose |
|--------|---------|
| `foo__mock(...)` | The mock itself. Consumer may `#define foo foo__mock` via an opt-in `*_MOCK_REPLACE` switch. |
| `foo__mock_reset()` | Zero call count, free `__param_actions`, null the callback. |
| `foo__call_count` | `size_t`, incremented after each call. |
| `foo__param_history[MOCK_CALL_STORAGE_MAX]` | Array of per-call parameter structs (`p0`..`pN`). |
| `foo__return_queue[MOCK_CALL_STORAGE_MAX]` | Return value for each call (R_* only). |
| `foo__param_actions` | Head of the `mock_param_action_t` chain applied during calls (non-`_S` only). |
| `foo__callback` | Optional void callback invoked after param capture + return + actions. |
| `foo__callback_t` | Typedef for the callback. |
| `foo_params` | Typedef for the captured-params struct. |

`MOCK_CALL_STORAGE_MAX` defaults to 32 and can be overridden per translation
unit by `#define`-ing it before including `lfg_ctest_mock.h`. Overflow asserts.

### Why `_S` exists

Standard (non-`_S`) mocks cast each captured parameter through
`(void *)(size_t)` into the param-history union. That coerces any scalar or
pointer, but won't compile for structs passed by value. The `_S` variants use
the actual parameter type in the history struct and skip the `(void*)(size_t)`
layer — at the cost of dropping the `__param_actions` mechanism
(`mock_param_mem_*` / `mock_param_str_*`), because those operate on pointer
parameters.

Rule of thumb: reach for `_S` **only** when a parameter (not the return type)
is a struct passed by value. Struct return types work fine with plain `R_N`.

### Param actions (`mock_param_*`)

A singly-linked list of `struct _mock_param_action` nodes, built by the
`mock_param_mem_read / _mem_write / _str_read / _str_write` constructors.
Each node specifies: direction (read/write, byte/string), call index,
parameter index, buffer, and size. `_str_*` variants use `snprintf` and stop
at the null terminator — prefer them for `const char *` to avoid over-reading
short string literals (ASAN redzone).

On every mock call, the mock body walks the chain for that `call_index` and
parameter index and executes matching actions. `mock_param_destroy()` frees
the chain; `__mock_reset()` calls it automatically.

Chains are appended at the tail so user-visible order (read first call's
param 1, then write its param 2, …) matches execution order.

### Reset registry and `mock_reset_all()`

`lfg_ctest_mock.c` holds a flat, file-scope array:

```c
static void (*_mock_reset_registry[MOCK_REGISTRY_MAX])(void);
static size_t _mock_reset_registry_count;
```

`MOCK_REGISTRY_MAX` defaults to 64. Registration is **lazy**: the generated
mock body calls `_MOCK_REGISTER(foo)` (expands to
`_mock_register_reset(foo__mock_reset)`) on each call, and
`_mock_register_reset` deduplicates so repeated calls are cheap. A mock that
is never called is never registered.

`mock_reset_all()` iterates the registry, invokes each reset function, then
zeros the count — so mocks re-register on their next call. This lets a
teardown drop every known mock's state without maintaining an explicit list.

## Amalgamation (`tools/amalgamate.c`)

The framework ships in two forms: the split sources (default) and a generated
single-header at `dist/lfg_ctest.h` produced by the `amalgamate` CMake target.
The single-header path exists for consumers who want drop-in use without CMake,
submodules, or vendoring multiple files.

The amalgamator itself is a small C99 program. It reads
`tools/amalgamate.manifest` (two sections, `@header_begin/@header_end` and
`@impl_begin/@impl_end`), concatenates the listed files in order, and writes
the combined output wrapped in:

```c
#ifndef LFG_CTEST_SINGLE_H_
/* ...headers, inline... */
#ifdef LFG_CTEST_IMPLEMENTATION
/* ...impl, inline... */
#endif
#endif
```

Per-file filtering:

- Internal `#include "lfg_ctest*"` directives are stripped.
- Include guards are stripped. Guard detection is a **repo-convention match**:
  an identifier is treated as a guard only if it ends in `_H_` (trailing
  underscore). This catches `LFG_CTEST_H_` / `LFG_CTEST_MOCK_H_` and ignores
  operational macros like `LFG_CTEST_HAS_FLOAT` / `LFG_CTEST_HAS_DOUBLE` that
  would false-match a naive `_H` substring check. Guard state is tracked
  per-file: the tool remembers the opening `#ifndef FOO_H_`, strips the
  matching `#define FOO_H_`, and strips the terminating `#endif` whose comment
  mentions `FOO_H_`.
- System includes (`#include <...>`) are deduplicated across all files so each
  standard header appears once in the output.

File-scope `static` state in the two `.c` halves (pass/fail counters, mock
reset registry) stays `static` inside the `LFG_CTEST_IMPLEMENTATION` block,
so multi-TU links against the amalgamated header are safe as long as exactly
one TU defines the gate. `RECORD_FAILURE` / `RECORD_PASS` macros from
`lfg_ctest.c` leak into `lfg_ctest_mock.c` scope after concatenation, but the
mock impl doesn't reference those names, so there's no collision.

`test_amalg` is the drift-detection smoke: it `#define`s
`LFG_CTEST_IMPLEMENTATION`, includes `dist/lfg_ctest.h`, and exercises a small
but representative slice (basic asserts, a mock with return queue + param
history, optional float/double asserts). It depends on the `amalgamate` target
so the dist header is always fresh before the smoke compiles. It does **not**
link against the `lfg_ctest` static lib — it provides its own impl.

## Float / double gating

`CMakeLists.txt` uses `check_symbol_exists(fabsf math.h)` and
`check_symbol_exists(fabs math.h)` (with `libm` on the required-libraries
line) to decide whether to define `LFG_CTEST_HAS_FLOAT=1` and/or
`LFG_CTEST_HAS_DOUBLE=1` on the library target, and whether to link `libm`
(`LFG_CTEST_NEEDS_LIBM`). Users can force either off via the
`LFG_CTEST_ENABLE_FLOAT` / `LFG_CTEST_ENABLE_DOUBLE` cache options.

All float/double assertions in `lfg_ctest.h` sit under
`#ifdef LFG_CTEST_HAS_FLOAT` / `#ifdef LFG_CTEST_HAS_DOUBLE`, so the library
links cleanly on platforms without an FPU.
