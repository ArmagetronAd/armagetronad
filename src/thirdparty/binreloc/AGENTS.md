# src/thirdparty/binreloc/ Directory

## Summary
BinReloc library for creating relocatable executables on Unix-like systems.

## Details

The binreloc directory contains the BinReloc library from autopackage.org, which allows Armagetron Advanced binaries to find their data files regardless of installation location. This is essential for creating portable builds and proper relocatable packages.

The library provides runtime path discovery by examining the executable's location in the filesystem. It uses /proc filesystem on Linux and other mechanisms on other platforms to determine where the binary was installed, then resolves relative paths to data directories.

The integration with Armagetron Advanced is minimal - the library is compiled directly into libtools.a and provides the `br_init()`, `br_init_lib()`, and related functions for path resolution.

## Directory Structure

```
.
├── prefix.c    # BinReloc implementation
└── prefix.h    # BinReloc header
```

## Technologies

- **Language**: C
- **Platform Support**: Linux, FreeBSD, OpenBSD, macOS, and other Unix-like systems
- **Dependencies**: Standard C library, system-specific headers

## Coding Conventions

- **External Code**: This is third-party code with minimal modifications
- **Inclusion**: Controlled by `ENABLE_BINRELOC` preprocessor define
- **Build**: Compiled directly into libtools.a

## Key Patterns

- Library integration pattern
- Platform abstraction for /proc filesystem access
- Path resolution utilities

## Build System

- Sources included in `libtools_a_SOURCES` in `src/tools/Makefile.am`
- Compilation controlled by `ENABLE_BINRELOC` configure option
- No separate library - integrated into libtools.a
