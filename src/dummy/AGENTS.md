# src/dummy/ Directory

## Summary
Dummy/stub implementations for build and testing purposes.

## Details

The dummy directory provides minimal header files and stub implementations that are used when full configuration or functionality is not available or needed. These files are primarily used by:

- Static analysis tools that need parseable header files
- Linters and code checkers
- Partial builds during development
- IDEs for code navigation

The dummy configure.h provides a minimal configuration header that can be used as a fallback when the full autoconf-generated config.h is not available. This allows development tools to parse the code without requiring a full build configuration.

## Directory Structure

```
.
└── configure.h           # Minimal dummy config.h for linters
```

## Technologies

- **Language**: C/C++ preprocessor
- **Purpose**: Development tool support

## Coding Conventions

- **Minimal Definitions**: Only essential macros and definitions
- **Compatibility**: Designed to allow parsing without full configuration

## Key Patterns

- Stub pattern for development support
- Fallback header pattern

## Build System

- Not compiled into any library
- Used only for development/analysis purposes
- Provides minimal definitions that would normally come from configure.ac
