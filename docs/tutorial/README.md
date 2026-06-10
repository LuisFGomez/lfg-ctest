# lfg-ctest tutorial

A progressive, feature-by-feature guide for developers writing their first
tests and mocks with lfg-ctest. Each chapter is driven by a small,
copy-pasteable worked example and builds on the one before it.

This series complements the reference docs — it does not replace them. When
you want the exhaustive contract for a symbol, jump to
[api.md](../api.md); when you want to understand how the framework is put
together internally, read [architecture.md](../architecture.md). The chapters
below stay above the internals level.

If you maintain the framework itself (rather than consume it), start at
[build-and-test.md](../build-and-test.md) instead.

## Chapters

| # | Chapter | Covers |
|---|---------|--------|
| 1 | [First test + runner](01-first-test.md) | `lfg_ct_start` / `lfg_ct_test` / `lfg_ct_print_summary` / `lfg_ct_return`, suites, setup/teardown, the core assertions. |
| 2 | [Running and filtering](02-running-and-filtering.md) | `lfg_ct_parse_args`: `--list`, `--filter`, `--filter-exclude`, `--strict-xpass`, `-v`. |
| 3 | [Skip, xfail, xpass](03-skip-xfail-xpass.md) | `lfg_ct_skip` / `lfg_ct_xfail` and the SKIP / XFAIL / XPASS buckets. |
| 4 | [Your first mock](04-first-mock.md) | `DECLARE_MOCK_*` / `DEFINE_MOCK_*`, `__call_count`, `__param_history[]`, `__return_queue[]`. |
| 5 | [Parameter actions and callbacks](05-param-actions-and-callbacks.md) | `__param_actions` (`mock_param_mem_*` / `mock_param_str_*`) and the per-call `__callback` hook. |
| 6 | [Teardown and cleanup](06-teardown-and-cleanup.md) | `mock_reset_all()` and the `mock_register_cleanup` consumer hook. |
| 7 | [Fork-per-test isolation](07-fork-isolation.md) | `LFG_CT_ISOLATE_FORK`: what it buys, platform gating, opt-out. |
| 8 | [Integration choices](08-integration.md) | CMake `add_subdirectory` vs single-header amalgamation, the opt-in JUnit-XML reporter. |
| 9 | [Mocking real libraries](09-library-mocks.md) | Self-contained worked mocks for the C standard library, mbedTLS, and libcurl. |

## How to read this

Chapters 1–3 are the foundation: enough to write and run a real test binary
with no mocking at all. Chapters 4–6 cover the mocking framework end to end.
Chapters 7–8 are operational concerns — isolation and how you wire the
framework into a build. Chapter 9 is a reference shelf of realistic,
copy-pasteable mocks against common third-party libraries.

Every code block in this series is self-contained: it compiles against
lfg-ctest (plus, in chapter 9, the one named third-party library) without any
other dependency. Copy a block, fill in your own function under test, and go.
