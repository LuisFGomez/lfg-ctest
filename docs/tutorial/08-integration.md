# 8. Integration choices

You've been compiling the framework sources directly alongside your tests.
That works, but for a real project you'll pick one of two integration paths,
and you'll probably want machine-readable output for CI. This chapter covers
both, plus the opt-in JUnit-XML reporter.

The reference for integration is [installation.md](../installation.md); this
chapter frames the *choice*.

## Path A — CMake `add_subdirectory`

If your project uses CMake, vendor lfg-ctest (submodule, subtree, or a plain
copy under `deps/`) and pull it in as a subdirectory. You link one target:

```cmake
add_subdirectory(deps/lfg-ctest)

add_executable(settings_test settings.c settings_test.c store_mock.c)
target_link_libraries(settings_test PRIVATE lfg-ctest)

add_test(NAME settings COMMAND settings_test)
```

This is the path that gives you the most for free: the build configures
float/double support, installs headers, and the `lfg-ctest` target carries its
own include directories. Combined with the `--filter` pattern from
[chapter 2](02-running-and-filtering.md), one test executable backs many
`add_test` entries:

```cmake
foreach(grp parser writer codec)
  add_test(NAME unit_${grp} COMMAND settings_test --filter "${grp}_*")
endforeach()
```

Run them with CTest:

```bash
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Choose this path when you already use CMake and don't mind vendoring a few
source files — it's the default and the least friction.

## Path B — single-header amalgamation

When you *don't* want CMake, submodules, or several vendored files — a tiny
project, a quick reproducer, or a codebase with its own non-CMake build — use
the single-header form. It collapses the split sources into one
`dist/lfg-ctest.h`.

Generate it once from a clone of the framework:

```bash
cmake -B build
cmake --build build --target amalgamate    # writes dist/lfg-ctest.h
```

Drop that one header anywhere on your include path. In **exactly one**
translation unit, define `LFG_CTEST_IMPLEMENTATION` before including it; every
other TU just includes the header:

```c
/* one .c file in the project — provides the implementation */
#define LFG_CTEST_IMPLEMENTATION
#include "lfg-ctest.h"
```

```c
/* every other test .c — declarations only */
#include "lfg-ctest.h"
```

The same configuration defines apply, and must be set **consistently across
every TU**:

| Define | Effect |
|--------|--------|
| `LFG_CTEST_HAS_FLOAT` | Enable 32-bit float assertions (needs `-lm`). |
| `LFG_CTEST_HAS_DOUBLE` | Enable 64-bit double assertions (needs `-lm`). |
| `LFG_CTEST_NO_FUNC` | Disable `__func__` reporting. |

Choose this path for drop-in use without a build-system dependency. The
trade-off is that you regenerate the header when you upgrade the framework
(the repo's `test-amalg` CTest entry guards that the amalgamation stays in
sync with the split sources, so it won't silently drift).

### Which one?

| | CMake subdirectory | Single-header |
|--|--------------------|---------------|
| You already use CMake | natural fit | works, but more manual |
| No build system / non-CMake build | awkward | drop-in |
| Float/double auto-configured | yes | you set the defines |
| Upgrade story | bump the vendored copy | regenerate the header |

## Machine-readable output: the JUnit-XML reporter

CI systems want structured results, not scraped stdout. lfg-ctest ships an
**opt-in** JUnit-XML reporter under `contrib/junit-xml/`. It is *not* part of
the core library — you add it only if you need it — and it produces the
JUnit-XML shape that Gitea Actions, GitHub Actions `dorny/test-reporter`,
pytest `--junit-xml`, and `gotestsum` all consume.

Add it via CMake alongside the core:

```cmake
add_subdirectory(deps/lfg-ctest)
add_subdirectory(deps/lfg-ctest/contrib/junit-xml)

target_link_libraries(settings_test PRIVATE lfg-ctest lfg-ctest-junit)
```

Then wire it into `main`. It peels its own `--output-junit <path>` flag off
`argv` *before* `lfg_ct_parse_args` runs (the core parser treats unknown flags
as fatal, so the junit flag must be consumed first):

```c
#include <lfg-ctest.h>
#include <lfg-ctest-junit.h>

int main(int argc, char *argv[])
{
    argc = lfg_ct_junit_consume_args(argc, argv);   /* peel --output-junit */
    if (argc < 0 || lfg_ct_parse_args(argc, argv))
    {
        return 1;
    }

    lfg_ct_start();
    /* ... suites/tests ... */
    lfg_ct_print_summary();   /* also flushes the junit document */
    return lfg_ct_return();
}
```

Run it with the flag to emit a report; omit the flag and the reporter stays
dormant:

```bash
./settings_test --output-junit results.xml
```

The reporter installs through the framework's single reporter slot
(`lfg_ct_set_reporter`). Because `--verbose`
([chapter 2](02-running-and-filtering.md)) is itself implemented as a built-in
reporter that fans out to whatever consumer reporter is attached, you can run
`--verbose` and `--output-junit` together — the human-readable banners and the
XML document are produced in the same run without interfering.

For consumers writing their *own* reporter (a custom format, a live dashboard),
the contract — `on_test_start` / `on_record` / `on_run_complete`, the borrowed
string fields, the field-order ABI guarantee — is documented under
[Reporter callback](../api.md#reporter-callback).

## Where to go next

- [Chapter 9 — Mocking real libraries](09-library-mocks.md): self-contained,
  copy-pasteable mocks for the C standard library, mbedTLS, and libcurl.
