# LFG Test - C99-Compatible Test and Mocking Framework

A comprehensive, easy-to-use, zero-dependency, C99-compatible test framework with mocking built in.

## Basic Usage

### Testing

The most basic example would be a single test that fails:

```c
#include <lfgtest.h>

int main(int argc, char* argv[]) {
	ASSERT_FALSE(true);
	return 0;
}
```

### Mocking

TODO

