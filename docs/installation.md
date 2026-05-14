# installation.md — lfg-ctest

How consumers pull lfg-ctest into a project. Two integration paths: as a CMake
subdirectory (default), or as a single-header amalgamation (drop-in).

## Direct copy

Copy `lfg-ctest.h` (and `lfg-ctest-mock.h` if using mocking) into your
project's include path.

## CMake integration

```cmake
add_subdirectory(deps/lfg-ctest)
target_link_libraries(my_tests lfg-ctest)
```

## Single-header amalgamation

A single-header form can be generated from the split sources — useful for
drop-in use without CMake, submodules, or vendoring multiple files.

Generate it from a clone of this repo:

```bash
cmake -B build
cmake --build build --target amalgamate
# produces dist/lfg-ctest.h
```

Drop `dist/lfg-ctest.h` anywhere on your include path. In **exactly one**
translation unit, define `LFG_CTEST_IMPLEMENTATION` before the include; all
other TUs just include the header:

```c
/* one .c file in your project */
#define LFG_CTEST_IMPLEMENTATION
#include "lfg-ctest.h"
```

```c
/* every other .c file that uses assertions or mocks */
#include "lfg-ctest.h"
```

All existing configuration defines still apply and must be set consistently
across TUs:

| Define | Effect |
|--------|--------|
| `LFG_CTEST_HAS_FLOAT` | Enable 32-bit float assertions (needs `-lm`) |
| `LFG_CTEST_HAS_DOUBLE` | Enable 64-bit double assertions (needs `-lm`) |
| `LFG_CTEST_NO_FUNC` | Disable `__func__` reporting |

The `test-amalg` target in this repo is a smoke test that compiles against
`dist/lfg-ctest.h` and is registered with CTest, so `cmake --build build`
followed by `ctest --test-dir build` verifies the amalgamation stays in sync
with the split sources.

## Floating-Point Configuration

Floating-point assertions are optional and independently configurable. This is
useful for embedded platforms where:

- Hardware float (32-bit) is available but double (64-bit) uses slow software
  emulation
- No FPU is present at all

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `LFG_CTEST_ENABLE_FLOAT` | `ON` | Enable 32-bit float assertions (requires `fabsf`) |
| `LFG_CTEST_ENABLE_DOUBLE` | `ON` | Enable 64-bit double assertions (requires `fabs`) |

Examples:

```bash
# Default: both float and double enabled
cmake -B build

# Float only (embedded with hardware FPU, no double)
cmake -B build -DLFG_CTEST_ENABLE_DOUBLE=OFF

# No floating-point (embedded without FPU)
cmake -B build -DLFG_CTEST_ENABLE_FLOAT=OFF -DLFG_CTEST_ENABLE_DOUBLE=OFF
```

When enabled, the library defines `LFG_CTEST_HAS_FLOAT` and/or
`LFG_CTEST_HAS_DOUBLE` and links against `libm`.

## Function Name Reporting

Assertion failure messages include the function name where the failure
occurred. The library auto-detects the best available option:

| Priority | Identifier | Availability |
|----------|------------|--------------|
| 1 | `__func__` | C99 standard |
| 2 | `__FUNCTION__` | GCC/Clang/MSVC extension (C89) |
| 3 | `"(unknown)"` | Fallback |

To disable function name reporting entirely, define `LFG_CTEST_NO_FUNC` before
including the header:

```c
#define LFG_CTEST_NO_FUNC
#include <lfg-ctest.h>
```
