# src/win32/ Directory

## Summary
Windows-specific platform code for Armagetron Advanced client.

## Details

The win32 directory contains Windows-specific compatibility code that provides POSIX-like functionality on Windows platforms (MinGW, Cygwin, or native Windows). This allows the cross-platform Armagetron codebase to compile and run on Windows systems.

Key files include: `dirent.c` and `dirent.h` which implement the POSIX directory scanning API (opendir, readdir, closedir) for Windows. This is essential for file system operations in the game that use POSIX directory functions.

The `strstream.h` header provides stream compatibility functionality for Windows, ensuring that stream operations work correctly across different Windows compilers and configurations.

These files are used when compiling for Windows targets (detected via `-*-cygwin*` or `-*-mingw32*` in configure.ac). The code in this directory complements the platform abstraction already present in the project.

## Directory Structure

```
.
├── dirent.c      # POSIX directory scanning implementation for Windows
├── dirent.h      # Header for dirent.c
└── strstream.h   # Stream compatibility header for Windows
```

## Technologies

- **Language**: C
- **Platform**: Windows (MinGW, Cygwin, native)
- **Dependencies**: Windows API headers

## Coding Conventions

- **Compatibility**: Provides POSIX API on Windows
- **Implementation**: Minimal, efficient wrappers around Windows API
- **Usage**: Used via `#include "dirent.h"` or similar in cross-platform code

## Key Patterns

- Compatibility layer pattern
- POSIX API emulation
- Platform-specific implementation

## Build System

- Compiled when target matches `*-*-cygwin* | *-*-mingw32*`
- Defines `WIN32` preprocessor symbol
- Uses `SYS_GL_LIB="opengl32"` and `SYS_GLU_LIB="glu32"` for OpenGL
- May check for `wsock32` library with `AC_CHECK_LIB(wsock32, connect)`
