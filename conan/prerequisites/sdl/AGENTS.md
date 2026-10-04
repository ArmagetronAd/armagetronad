# conan/prerequisites/sdl/ Directory

## Summary
Conan configuration for building SDL (Simple DirectMedia Layer) from source.

## Details

This directory contains Conan configuration files for building SDL 1.2 from source using the Conan package manager. SDL is a critical dependency for Armagetron Advanced, providing:

- Window creation and management
- Input handling (keyboard, mouse, joystick)
- OpenGL context creation
- Audio support (via SDL_mixer)
- Timer functionality
- Thread support

By providing custom Conan builds, developers can:
- Use specific SDL versions not available in system packages
- Build with custom patches or modifications
- Cross-compile SDL for different platforms
- Enable or disable specific SDL features
- Create statically linked versions

This is particularly useful for creating portable builds or when system SDL versions are too old or incompatible.

## Directory Structure

```
.
└── conanfile.py    # Conan configuration for SDL build
```

## Technologies

- **Library**: SDL 1.2
- **Package Manager**: Conan
- **Language**: Python (Conan file)

## Coding Conventions

- **Conan Configuration**: Standard Conan file format
- **Dependencies**: Defines required dependencies for SDL
- **Options**: Configurable build options

## Key Patterns

- Conan package configuration pattern
- Custom library build pattern
- Dependency specification pattern

## Build System

- Conan file defines SDL build requirements
- Used by parent prerequisites/build.sh
- Can be built standalone via `conan install` or similar
- Supports cross-compilation via Conan profiles
