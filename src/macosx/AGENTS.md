# src/macosx/ Directory

## Summary
macOS-specific code and build configuration for Armagetron Advanced.

## Details

The macosx directory contains platform-specific code, configuration, and build scripts for macOS (Darwin) systems. It provides the necessary integration between the cross-platform Armagetron codebase and macOS-specific features and requirements.

Key files include: `build_bundle.sh.in` which is a template for the build bundle script that creates a proper macOS application bundle (.app). This script handles all the macOS-specific packaging, including code signing (if configured), creating the bundle structure, and setting up the Info.plist.

The `config_common.h` file provides common macOS-specific configuration and definitions that are used throughout the codebase when compiling for macOS. The `config_ide.h` file contains IDE-specific configurations for macOS development.

When building for macOS, the configure script defines `MACOSX` and sets appropriate compiler flags. The build system also handles Objective-C support for platform-specific features like native dialogs and integration with macOS services.

## Directory Structure

```
.
├── build_bundle.sh.in    # Application bundle build script template
├── config_common.h       # Common macOS configuration
└── config_ide.h          # IDE-specific configuration for macOS
```

## Technologies

- **Language**: Shell scripts, C/C++, Objective-C (for platform-specific features)
- **Build System**: Autotools with macOS-specific detection
- **Platform**: macOS/Darwin

## Coding Conventions

- **Conditional Compilation**: `#ifdef MACOSX` for macOS-specific code
- **Objective-C**: Used for native macOS integration where needed
- **Bundle Structure**: Application bundles with .app extension

## Key Patterns

- Platform-specific implementation pattern
- Build script template pattern
- Application bundle packaging

## Build System

- macOS detected via `case "$target" in *-*-darwin*)` in configure.ac
- Defines `MACOSX` preprocessor symbol
- Adds `-framework OpenGL` to LIBS for regular builds
- Adds `CXXFLAGS="${CXXFLAGS} -DGL_SILENCE_DEPRECATION"`
- `AM_CONDITIONAL(MACOSX, test x$macosx = xtrue)` for conditional compilation
- `render_extra` variable can contain macOS-specific Objective-C files
- Uses `AC_PROG_OBJC` for Objective-C compiler support
