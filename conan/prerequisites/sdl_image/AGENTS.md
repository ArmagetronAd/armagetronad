# conan/prerequisites/sdl_image/ Directory

## Summary
Conan configuration for building SDL_image from source.

## Details

This directory contains Conan configuration files for building SDL_image from source using the Conan package manager. SDL_image is a companion library to SDL that provides image loading support for various image formats.

SDL_image is used by Armagetron Advanced for:
- Loading texture images (PNG, JPEG, etc.)
- Surface creation from image files
- Image format detection and handling
- Optional support for various image formats

By providing custom Conan builds, developers can:
- Use specific SDL_image versions
- Build with custom image format support
- Cross-compile for different platforms
- Create statically linked versions
- Enable or disable specific image format support

This is useful when system packages don't have the required version or when custom image format support is needed.

## Directory Structure

```
.
└── conanfile.py    # Conan configuration for SDL_image build
```

## Technologies

- **Library**: SDL_image
- **Package Manager**: Conan
- **Language**: Python (Conan file)
- **Dependencies**: SDL 1.2

## Coding Conventions

- **Conan Configuration**: Standard Conan file format
- **Dependencies**: Defines SDL dependency
- **Options**: Configurable image format support

## Key Patterns

- Conan package configuration pattern
- Image library build pattern
- Dependency chain pattern (SDL_image -> SDL)

## Build System

- Conan file defines SDL_image build requirements
- Used by parent prerequisites/build.sh
- Can be built standalone via `conan install`
- Supports cross-compilation via Conan profiles
- Supports static linking for standalone builds
