# 9. Mocking real libraries

The earlier chapters mocked toy dependencies. In practice you'll mock the
libraries your code actually links: the C standard library, a crypto library,
an HTTP client. This chapter is a reference shelf of self-contained, realistic
mocks for three common cases.

Every example here stands alone: each compiles against **only its named
library** (its public headers) plus lfg-ctest — no other dependency. Copy a
block, point the replacement at your module under test, and go. The patterns
are the same `DECLARE`/`DEFINE` + transparent-substitution mechanics from
[chapter 4](04-first-mock.md); what changes is the real-world signatures.

> All three examples use macro substitution (`#define real_fn real_fn__mock`)
> to swap the library function for its mock in the translation unit under
> test. Define the `*_MOCK_REPLACE` guard before including the mock header, as
> in chapter 4. Always sweep mock state with `mock_reset_all()`
> ([chapter 6](06-teardown-and-cleanup.md)) — omitted from the bodies below
> for brevity.

## C standard library — injecting `malloc` failure

A frequent untested branch is the allocation-failure path: "what does this do
when `malloc` returns `NULL`?" Mock `malloc` and queue a `NULL` for the call
you want to fail.

`void *malloc(size_t)` returns a value and takes one parameter -> `R_1`.

```c
/* libc_mock.h */
#ifndef LIBC_MOCK_H_
#define LIBC_MOCK_H_

#include <lfg-ctest-mock.h>
#include <stddef.h>

/* void *malloc(size_t size);  -> returns void*, 1 param */
DECLARE_MOCK_R_1(malloc, void *, size_t);
/* void  free(void *ptr);      -> void return, 1 param  */
DECLARE_MOCK_V_1(free, void *);

#if defined(LIBC_MOCK_REPLACE)
#define malloc  malloc__mock
#define free    free__mock
#endif

#endif /* LIBC_MOCK_H_ */
```

```c
/* libc_mock.c */
#include "libc_mock.h"

DEFINE_MOCK_R_1(malloc, void *, size_t)
DEFINE_MOCK_V_1(free, void *)
```

Suppose the module under test is:

```c
/* widget.c (compiled with LIBC_MOCK_REPLACE active via its test header) */
struct widget *widget_create(size_t n)
{
    struct widget *w = malloc(sizeof(*w) + n);
    if (w == NULL)
    {
        return NULL;             /* the branch we want to cover */
    }
    w->capacity = n;
    return w;
}
```

The success path queues a real buffer; the failure path queues `NULL`:

```c
#include <lfg-ctest.h>
#include "libc_mock.h"
#include "widget.h"

static void test_create_succeeds(void)
{
    static char backing[256];
    malloc__return_queue[0] = backing;     /* hand back a real buffer */

    struct widget *w = widget_create(64);

    ASSERT_NOT_NULL(w);
    ASSERT_EQ(1, malloc__call_count);
    ASSERT_TRUE(malloc__param_history[0].p0 >= sizeof(*w) + 64);
}

static void test_create_handles_oom(void)
{
    malloc__return_queue[0] = NULL;        /* simulate allocation failure */

    struct widget *w = widget_create(64);

    ASSERT_NULL(w);                        /* the OOM branch is covered */
    ASSERT_EQ(1, malloc__call_count);
}
```

Queuing a real static buffer for the success case (rather than a real
`malloc`) keeps the test allocation-free and deterministic — there is nothing
to leak and nothing for `free__mock` to actually release. If your module pairs
`malloc`/`free`, assert the pairing with `free__call_count` and
`free__param_history[0].p0`.

## mbedTLS — deterministic randomness via an injected RNG

Crypto code calls a random-number generator, which makes it non-deterministic
and miserable to test. Mock the DRBG and **inject** known bytes into its output
buffer with a `mock_param_mem_write` action
([chapter 5](05-param-actions-and-callbacks.md)) — now the "random" output is
whatever the test chose.

mbedTLS's generator entry point is
`int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t len)`:
returns an `int` status, three parameters -> `R_3`.

```c
/* drbg_mock.h */
#ifndef DRBG_MOCK_H_
#define DRBG_MOCK_H_

#include <lfg-ctest-mock.h>
#include <stddef.h>

/* int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t len); */
DECLARE_MOCK_R_3(mbedtls_ctr_drbg_random, int, void *, unsigned char *, size_t);

#if defined(DRBG_MOCK_REPLACE)
#define mbedtls_ctr_drbg_random  mbedtls_ctr_drbg_random__mock
#endif

#endif /* DRBG_MOCK_H_ */
```

```c
/* drbg_mock.c */
#include "drbg_mock.h"

DEFINE_MOCK_R_3(mbedtls_ctr_drbg_random, int, void *, unsigned char *, size_t)
```

The module under test generates a hex token from 4 random bytes:

```c
/* token.c (test build active) */
#include <mbedtls/ctr_drbg.h>

int make_token(mbedtls_ctr_drbg_context *rng, char out[9])
{
    unsigned char raw[4];
    if (mbedtls_ctr_drbg_random(rng, raw, sizeof(raw)) != 0)
    {
        return -1;
    }
    snprintf(out, 9, "%02x%02x%02x%02x", raw[0], raw[1], raw[2], raw[3]);
    return 0;
}
```

The test injects four chosen bytes into parameter 1 (`output`) and returns
success (`0`):

```c
#include <lfg-ctest.h>
#include "drbg_mock.h"
#include "token.h"

static void test_token_is_deterministic_under_injected_rng(void)
{
    unsigned char fake[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    /* On call 0, write our 4 bytes into parameter 1 (the output buffer). */
    mbedtls_ctr_drbg_random__param_actions =
        mock_param_mem_write(NULL, 0, 1, fake, sizeof(fake));
    mbedtls_ctr_drbg_random__return_queue[0] = 0;   /* MBEDTLS success */

    char out[9] = {0};
    int rc = make_token(NULL, out);

    ASSERT_EQ(0, rc);
    ASSERT_STR_EQUAL("deadbeef", out);
    ASSERT_EQ(4, mbedtls_ctr_drbg_random__param_history[0].p2);  /* len */
}

static void test_token_reports_rng_failure(void)
{
    /* A negative mbedTLS error code; no injection needed. */
    mbedtls_ctr_drbg_random__return_queue[0] = -0x0034;   /* e.g. ENTROPY_SOURCE_FAILED */

    char out[9] = {0};
    ASSERT_EQ(-1, make_token(NULL, out));
}
```

The same shape works for any "fill this buffer" crypto primitive — a hash
output, a cipher block, a nonce: mock the function, inject the bytes you want
to assert against, return the library's success code.

## libcurl — an HTTP client without a network

libcurl is the canonical "I can't unit-test this, it hits the network" case.
Mock the easy-interface lifecycle and the transfer never leaves the process.

The functions a simple GET touches:

```c
CURL     *curl_easy_init(void);                            /* R_V  */
CURLcode  curl_easy_setopt(CURL *, CURLoption, ...);       /* see note */
CURLcode  curl_easy_perform(CURL *);                       /* R_1  */
void      curl_easy_cleanup(CURL *);                       /* V_1  */
```

> **`curl_easy_setopt` is variadic.** The mock macros take a fixed parameter
> count, so mock it at a fixed arity of three with a `void *` final parameter:
> `DECLARE_MOCK_R_3(curl_easy_setopt, CURLcode, CURL *, CURLoption, void *)`.
> Because substitution is a macro, each real call site
> (`curl_easy_setopt(h, CURLOPT_URL, url)`) expands to a 3-argument mock call,
> and the single variadic argument lands in `p2`. Object-pointer options
> (`CURLOPT_URL`, `CURLOPT_WRITEDATA`) pass cleanly. *Function*-pointer options
> (`CURLOPT_WRITEFUNCTION`) rely on the function-pointer->`void *` conversion,
> which POSIX allows but ISO C does not — fine on a normal build, but if you
> compile the module under test with `-pedantic-errors`, route those options
> through a thin non-pedantic wrapper TU.

```c
/* curl_mock.h */
#ifndef CURL_MOCK_H_
#define CURL_MOCK_H_

#include <lfg-ctest-mock.h>
#include <curl/curl.h>

DECLARE_MOCK_R_V(curl_easy_init, CURL *);
DECLARE_MOCK_R_3(curl_easy_setopt, CURLcode, CURL *, CURLoption, void *);
DECLARE_MOCK_R_1(curl_easy_perform, CURLcode, CURL *);
DECLARE_MOCK_V_1(curl_easy_cleanup, CURL *);

#if defined(CURL_MOCK_REPLACE)
#define curl_easy_init     curl_easy_init__mock
#define curl_easy_setopt   curl_easy_setopt__mock
#define curl_easy_perform  curl_easy_perform__mock
#define curl_easy_cleanup  curl_easy_cleanup__mock
#endif

#endif /* CURL_MOCK_H_ */
```

```c
/* curl_mock.c */
#include "curl_mock.h"

DEFINE_MOCK_R_V(curl_easy_init, CURL *)
DEFINE_MOCK_R_3(curl_easy_setopt, CURLcode, CURL *, CURLoption, void *)
DEFINE_MOCK_R_1(curl_easy_perform, CURLcode, CURL *)
DEFINE_MOCK_V_1(curl_easy_cleanup, CURL *)
```

The module under test fetches a URL and maps the result to a small status:

```c
/* fetch.c (test build active) */
#include <curl/curl.h>

int fetch_ok(const char *url)
{
    CURL *h = curl_easy_init();
    if (h == NULL)
    {
        return -1;
    }
    curl_easy_setopt(h, CURLOPT_URL, url);
    CURLcode rc = curl_easy_perform(h);
    curl_easy_cleanup(h);
    return (rc == CURLE_OK) ? 0 : -1;
}
```

The test drives the whole lifecycle with no socket in sight:

```c
#include <lfg-ctest.h>
#include "curl_mock.h"
#include "fetch.h"

static void test_fetch_success(void)
{
    static int handle;                          /* any non-NULL token */
    curl_easy_init__return_queue[0]    = (CURL *)&handle;
    curl_easy_setopt__return_queue[0]  = CURLE_OK;
    curl_easy_perform__return_queue[0] = CURLE_OK;

    int rc = fetch_ok("https://example.test/data");

    ASSERT_EQ(0, rc);
    ASSERT_EQ(1, curl_easy_perform__call_count);
    /* the URL was forwarded as the setopt variadic argument (p2) */
    ASSERT_STR_EQUAL("https://example.test/data",
                     (const char *)curl_easy_setopt__param_history[0].p2);
    ASSERT_EQ(1, curl_easy_cleanup__call_count);   /* handle released */
}

static void test_fetch_maps_transfer_error(void)
{
    static int handle;
    curl_easy_init__return_queue[0]    = (CURL *)&handle;
    curl_easy_perform__return_queue[0] = CURLE_COULDNT_CONNECT;

    ASSERT_EQ(-1, fetch_ok("https://example.test/data"));
    ASSERT_EQ(1, curl_easy_cleanup__call_count);   /* still cleaned up */
}

static void test_fetch_handles_init_failure(void)
{
    curl_easy_init__return_queue[0] = NULL;        /* init failed */

    ASSERT_EQ(-1, fetch_ok("https://example.test/data"));
    ASSERT_EQ(0, curl_easy_perform__call_count);   /* never got that far */
}
```

### Simulating a response body

To test code that *consumes* a response, libcurl's real flow is "`perform`
calls your registered `CURLOPT_WRITEFUNCTION` with the body bytes". Reproduce
that with a `curl_easy_perform__callback` ([chapter 5](05-param-actions-and-callbacks.md))
that invokes the write callback the test holds, then returns the queued
`CURLE_OK`:

```c
static size_t (*g_write_cb)(char *, size_t, size_t, void *);
static void   *g_write_data;

static void on_perform(size_t call_index, CURLcode *return_override, CURL *h)
{
    (void)call_index;
    (void)return_override;     /* let the queued CURLE_OK flow through */
    (void)h;
    char body[] = "HTTP 200 OK";
    g_write_cb(body, 1, sizeof(body) - 1, g_write_data);
}
```

Capture `g_write_cb` / `g_write_data` from the `curl_easy_setopt` history (the
`CURLOPT_WRITEFUNCTION` / `CURLOPT_WRITEDATA` calls land in `__param_history`
as `p1` = the option, `p2` = the value), wire `curl_easy_perform__callback =
on_perform`, and the code under test sees a fully simulated transfer. This is
the same side-effect-only callback pattern from chapter 5, applied to a real
library.

## Adapting these to your library

The recipe generalises to any C library:

1. Write the real prototype down and read off the macro shape:
   return-or-void, parameter count, `_S` only for struct-by-value params.
2. `DECLARE` in a mock header (with the `*_MOCK_REPLACE` substitution guard),
   `DEFINE` in one mock `.c`.
3. In the test: queue return values (`__return_queue`), inject/capture buffers
   (`__param_actions`), or compute results live (`__callback`).
4. Assert on `__call_count` and `__param_history`.
5. `mock_reset_all()` to sweep mock state (at the top of the next test, or in
   a teardown the body calls).

Variadic functions get fixed at a representative arity via the macro
substitution (the libcurl note above); everything else maps directly.

## Where to go next

You've reached the end of the tutorial. From here:

- [api.md](../api.md) — the exhaustive reference for every symbol touched here.
- [architecture.md](../architecture.md) — how the framework is built, if you
  want to understand the machinery behind the macros.
