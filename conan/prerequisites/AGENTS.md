# conan/prerequisites/ Directory

## Summary
Custom Conan-based dependency builds for SDL and related libraries.

## Details

The prerequisites directory contains Conan configuration and build scripts for creating custom builds of prerequisite libraries needed by Armagetron Advanced. This allows developers to build the game with specific or modified versions of dependencies, particularly useful for:

- Creating lean standalone versions
- Static linking scenarios
- Cross-compilation to different platforms
- Building with non-standard dependency versions
- Testing with development versions of libraries

The directory contains subdirectories for each prerequisite library:
- `sdl/` - Conan configuration and scripts for building SDL (Simple DirectMedia Layer)
- `sdl_image/` - Conan configuration and scripts for building SDL_image (image loading library)

Each subdirectory contains the Conan files (`conanfile.py`, etc.) and any additional scripts needed to build that specific library.

## Directory Structure

```
.
├── build.sh    # Build script for all prerequisites
├── clean.sh    # Clean script for built dependencies
├── sdl/        # SDL library custom build
└── sdl_image/  # SDL_image library custom build
```

## Technologies

- **Package Manager**: Conan
- **Build System**: Conan-based builds
- **Languages**: Python (Conan files), Shell scripts

## Coding Conventions

- **Conan Files**: Standard Conan Python syntax
- **Shell Scripts**: POSIX-compliant where possible
- **Build Scripts**: Designed to be called from Makefiles or directly

## Key Patterns

- Custom dependency build pattern
- Conan package management pattern
- Cross-compilation support pattern
- Static linking support pattern

## Build System

- `build.sh` triggers custom builds of SDL and SDL_image
- `clean.sh` removes built dependencies
- Each library subdirectory has its own Conan configuration
- Builds can be used when system packages are unavailable or insufficient
- Integration with main build via `conanautotoolstoolchain.sh` in configure.ac
