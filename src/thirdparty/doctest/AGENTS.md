# src/thirdparty/doctest/ Directory

## Summary
The doctest single-header C++ testing framework integrated into the project.

## Details
Contains the doctest library (version 2.5.0), a feature-rich C++ single-header testing framework for unit tests and TDD. Distributed under MIT license. Used as the primary test framework for unit tests in `src/test/`.

## Directory Structure

```
.
├── doctest.h          # Main doctest header (auto-generated, do not modify)
└── update_doctest.sh  # Script to download latest doctest.h from GitHub
```

## Technologies
- **Language**: C++ (header-only, C++11+)
- **License**: MIT
- **Version**: 2.5.0

## Build System
Included via `-I${top_srcdir}/src/thirdparty/doctest` in test compilation. No separate library; header is included directly in test source files.

## Usage
Tests include `doctest.h` and use doctest macros (`TEST_CASE`, `SUBCASE`, `CHECK`, etc.).

```cpp
#define CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
TEST_CASE("example") { CHECK(1 == 1); }
```
