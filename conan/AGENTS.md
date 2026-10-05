# conan/ Directory

## Summary
Conan package manager configuration for custom dependency builds and management.

## Details

The conan directory contains configuration and scripts for using the Conan package manager with Armagetron Advanced. Conan is used as an alternative or complementary dependency management system, particularly for building custom versions of dependencies or for cross-compilation scenarios.

The main `conanfile.py` provides the Conan configuration for Armagetron Advanced, defining dependencies, build requirements, and package options. This allows developers to use Conan for dependency resolution instead of or in addition to system package managers.

The `conanoptions.ini` file defines custom Conan options and settings specific to Armagetron Advanced, allowing fine-grained control over dependency versions and build configurations.

The `prerequisites/` subdirectory contains Conan files and scripts for building custom prerequisite dependencies. This includes:
- `build.sh` - Script for building SDL and SDL_Image from source using Conan
- `clean.sh` - Script for cleaning built dependencies
- Separate Conan files for each prerequisite library (sdl, sdl_image)

These custom builds are useful for:
- Creating lean standalone versions of Armagetron Advanced
- Static linking scenarios
- Cross-compilation to different platforms
- Building with non-standard dependency versions

## Directory Structure

```
.
├── Makefile.am              # Conan Makefile integration
├── README.md               # Conan setup instructions
├── conanoptions.ini        # Custom Conan options
├── conanfile.py            # Main Conan configuration
└── prerequisites/          # Custom dependency builds
    ├── build.sh           # Build script for prerequisites
    ├── clean.sh           # Clean script for built dependencies
    ├── sdl/               # SDL custom build
    └── sdl_image/         # SDL_image custom build
```

## Technologies

- **Package Manager**: Conan
- **Build System**: Conan integration with Autotools
- **Languages**: Python (Conan files), Shell scripts

## Coding Conventions

- **Conan Files**: Standard Conan Python syntax
- **Options**: Custom options in conanoptions.ini
- **Profiles**: Conan profiles for different build scenarios

## Key Patterns

- Package manager integration pattern
- Custom dependency build pattern
- Cross-compilation support pattern
- Static linking support pattern

## Build System

- Conan integration via `test -r $(dirname $0)/conan/conanautotoolstoolchain.sh && source ...` in configure.ac
- Can be used alongside or instead of system package managers
- Custom builds triggered via prerequisites/build.sh
- Build artifacts used when system packages are unavailable or insufficient
