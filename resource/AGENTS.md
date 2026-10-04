# resource/ Directory

## Summary
Game resource files including textures, models, sounds, and map templates.

## Details

The resource directory contains all static game assets and resource templates used by Armagetron Advanced. Resources include 2D textures, 3D models, sound effects, and map definitions. The directory is organized with source resources in the `proto/` subdirectory, which are then processed and sorted during the build process.

The `proto/` directory contains the source resource files. These are processed by the build system, sorted for consistent ordering, and installed to the appropriate data directory. The `proto/` directory is further organized with subdirectories for different resource types.

Resource loading in the game uses a search path that typically includes:
1. User-specific resource directory
2. System-wide resource directory (e.g., `/usr/share/games/armagetronad/resource/`)
3. Built-in fallback resources

## Directory Structure

```
.
├── Makefile.am           # Resource build configuration
└── proto/                # Source resource files
    └── [various resource files and subdirectories]
```

## Technologies

- **Resource Management**: `tResourceManager` class in src/tools/
- **Processing**: Custom build scripts for sorting and packaging
- **Installation**: Resources installed to `${datadir}/resource/`

## Coding Conventions

- **Resource Paths**: Referenced via abstract paths in code
- **Fallback**: Multiple fallback mechanisms for missing resources
- **Caching**: Resources cached after first load

## Key Patterns

- Resource manager pattern
- Fallback chain pattern
- Lazy loading pattern
- Resource caching pattern

## Build System

- `Makefile.am` processes files in `proto/` directory
- Uses `batch/make/sortresources.py` to sort resources consistently
- Excludes CVS directories and backup files (`*~`, `*.bak`, etc.)
- Resources are copied, not compiled
- Installed to `${datadir}/resource/` or `${datadir}/resource/included/`
