# 5. Parameter actions and callbacks

[Chapter 4](04-first-mock.md) left a gap: a bare mock records its arguments
and returns a queued value, but it can't *write through* a pointer parameter,
and it can't run custom logic at call time. Two facilities close that gap:

- **Parameter actions** (`__param_actions`) — declaratively capture bytes
  *out of* a pointer parameter, or inject bytes *into* one.
- **The callback hook** (`__callback`) — run arbitrary C on every call, with
  access to the parameters and (for value-returning mocks) the return value.

## Parameter actions

A pointer parameter is a two-way street. The code under test might *write*
into it (an output buffer the mock should capture) or *read* from it (an input
buffer the mock should fill in). Four functions build an action chain that the
mock applies automatically:

| Function | Direction | Use for |
|----------|-----------|---------|
| `mock_param_mem_read(action, call, param, buf, size)` | capture | Copy `size` bytes *out of* the parameter into `buf`. |
| `mock_param_mem_write(action, call, param, buf, size)` | inject | Copy `size` bytes *from* `buf` into the parameter. |
| `mock_param_str_read(action, call, param, buf, size)` | capture | Like `mem_read`, but stops at the NUL (uses `snprintf`). |
| `mock_param_str_write(action, call, param, str, size)` | inject | Like `mem_write`, but for a NUL-terminated string. |

"read" / "write" name the direction *from the mock's point of view*: a `read`
action reads the caller's buffer (capturing what the code under test produced),
a `write` action writes into it (injecting what the code under test will
consume). `call` and `param` are both 0-indexed.

Prefer the `str_` variants for `const char *` parameters: they stop at the NUL
terminator, so they won't over-read a short string literal (which `mem_read`
can do, tripping ASan).

### Injecting a value (the `store_get` fix)

Back to chapter 4's `store_get(const char *key, int *out_value)`. The mock now
needs to *write* the looked-up integer into parameter 1 (`out_value`). That is
an inject — `mock_param_mem_write`:

```c
#include <lfg-ctest.h>
#include "store_mock.h"
#include "settings.h"

static void test_returns_stored_value(void)
{
    int stored = 42;

    /* On call 0, write sizeof(int) bytes from &stored into parameter 1. */
    store_get__param_actions =
        mock_param_mem_write(NULL, 0, 1, &stored, sizeof(stored));
    store_get__return_queue[0] = 0;   /* and report success */

    int result = setting_or_default("volume", 11);

    ASSERT_EQ(42, result);            /* the injected value flows through */
    mock_reset_all();
}
```

The first argument is the action *chain* to extend; pass `NULL` to start a new
one. The assignment `store_get__param_actions = ...` hands the finished chain
to the mock.

### Capturing a buffer

The mirror case: the code under test passes a buffer the mock should record so
the test can assert on its contents. That is a capture — `mock_param_mem_read`.
This is the heart of the [I2C worked example](../example.md):

```c
static void test_capture_buffer_contents(void)
{
    uint8_t captured[16] = {0};

    /* Capture 16 bytes out of parameter 1 of call 0. */
    i2c_write__param_actions =
        mock_param_mem_read(NULL, 0, 1, captured, 16);

    function_under_test();    /* calls i2c_write(addr, buf, ...) internally */

    ASSERT_UINT8_EQUAL(0x01, captured[0]);
    ASSERT_UINT8_EQUAL(0x80, captured[1]);
    mock_reset_all();
}
```

### Chaining actions

Each `mock_param_*` call returns the head of the chain; feed it back in as the
first argument to add another. One chain can target different calls and
different parameters:

```c
mock_param_action_t action = NULL;
action = mock_param_mem_read (action, 0, 1, buf1, 4);   /* call 0, param 1 */
action = mock_param_mem_write(action, 0, 2, buf2, 4);   /* call 0, param 2 */
action = mock_param_mem_read (action, 1, 1, buf3, 4);   /* call 1, param 1 */
my_func__param_actions = action;
```

A common idiom is capturing the same string parameter across several
consecutive calls with a loop:

```c
char cap[3][256] = {{0}};
mock_param_action_t action = NULL;
for (int i = 0; i < 3; i++)
{
    action = mock_param_str_read(action, i, 0, cap[i], sizeof(cap[i]));
}
mkdir__param_actions = action;

function_under_test();        /* calls mkdir three times */

ASSERT_STR_EQUAL("src",     cap[0]);
ASSERT_STR_EQUAL("src/lib", cap[1]);
ASSERT_STR_EQUAL("src/bin", cap[2]);
```

You never free the chain by hand — `mock_reset_all()` (and the per-mock reset)
destroys it for you. Parameter actions are unavailable on `_S` struct-safe
mocks; a struct-by-value parameter is captured whole, so inspect it directly
as `__param_history[i].pX.field`.

## The callback hook

When a parameter action isn't expressive enough — the mock needs to *do*
something, or compute its return value from live state — set `__callback`. It
fires on every call, receiving the call index and all parameters. For a
value-returning (`R_*`) mock it also receives a pointer to the value about to
be returned, so it can override the queue on a per-call basis.

The callback type is generated per mock:

```c
/* For DECLARE_MOCK_V_2(set_value, int, const char *):
     typedef void (*set_value__callback_t)(size_t call_index, int p0, const char *p1); */

/* For DECLARE_MOCK_R_2(get_value, int, int, const char *):
     typedef void (*get_value__callback_t)(size_t call_index,
                                            int *return_override,
                                            int p0, const char *p1); */
```

The override is opt-in *per call*: write to `*return_override` to replace this
call's queued value, or leave it alone to let the queue value flow through.

### Side effects only

A frequent need: the code under test registers a callback with a dependency,
then the dependency is supposed to invoke it. Drive that from the mock's
callback without touching the return value:

```c
static void on_perform(size_t call_index, int *return_override, conn_t *c)
{
    (void)call_index;
    (void)return_override;   /* leave the queued return in place */
    (void)c;
    /* feed the captured write-callback the simulated response */
    write_cb("HTTP 200 OK", 1, 11, write_cb_userdata);
}

static void test_request_drives_write_callback(void)
{
    perform__return_queue[0] = 0;       /* success, via the queue */
    perform__callback = on_perform;     /* side effect, via the callback */

    function_under_test();

    ASSERT_EQ(1, perform__call_count);
    mock_reset_all();
}
```

### State-driven return

When the return value depends on per-call state, skip the queue entirely and
compute it in the callback:

```c
static void on_is_file(size_t call_index, int *ret, const char *path)
{
    (void)call_index;
    *ret = seeded_state_has(path);   /* override: answer from live state */
}

static void test_state_driven_query(void)
{
    is_file__callback = on_is_file;
    seed_state("a");                 /* "a" exists, "c" does not */

    process_manifest("a,b,c");

    mock_reset_all();
}
```

### Execution order inside the mock body

Knowing the order matters when a callback inspects `*return_override`:

1. overflow check
2. store `__param_history`
3. load the return value from `__return_queue` (R_* only)
4. apply `__param_actions`
5. invoke `__callback` (may overwrite `*return_override` from step 3)
6. increment `__call_count`
7. return

Because the queue value is already loaded by step 5, a callback can *read*
`*return_override` to see the queued value before deciding whether to replace
it — useful for "modify only if state says X" logic. The `call_index` the
callback sees is the pre-increment index, matching the `__param_history` slot
for this call.

## Where to go next

- [Chapter 6 — Teardown and cleanup](06-teardown-and-cleanup.md): the
  `mock_reset_all()` you've been calling, and how to extend it for mocks that
  own heap state.
