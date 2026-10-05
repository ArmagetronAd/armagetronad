# src/win32_ded/ Directory

## Summary
Windows-specific code for Armagetron Advanced dedicated server.

## Details

The win32_ded directory contains Windows-specific compatibility code for the dedicated server build. Similar to the win32/ directory, it provides POSIX-like functionality but specifically for the headless dedicated server variant.

The dedicated server doesn't require graphics or audio capabilities, so this directory provides only the minimal compatibility layer needed for file system operations and other non-graphical functionality.

Key files include: `dirent.c` and `dirent.h` for directory scanning, and `strstream.h` for stream compatibility. These mirror the files in win32/ but are kept separate to allow for potential dedicated-server-specific implementations.

This separation allows the dedicated server to have a leaner Windows compatibility layer without graphics-related code.

## Directory Structure

```
.
├── dirent.c      # POSIX directory scanning implementation for Windows dedicated server
├── dirent.h      # Header for dirent.c
└── strstream.h   # Stream compatibility header for Windows dedicated server
```

## Technologies

- **Language**: C
- **Platform**: Windows (MinGW, Cygwin, native)
- **Purpose**: Dedicated server only
- **Dependencies**: Windows API headers

## Coding Conventions

- **Compatibility**: Provides POSIX API on Windows for server
- **Minimal**: Only includes what's needed for headless operation
- **Similar to win32/**: But potentially optimized for server use

## Key Patterns

- Compatibility layer pattern
- POSIX API emulation for server
- Minimal dependency pattern

## Build System

- Compiled when building dedicated server for Windows
- Used in `armagetronad-dedicated.exe` builds
- Does not include graphics-related code
