# batch/make/ Directory

## Summary
Build helper scripts and utilities for resource processing and version management.

## Details

The make directory contains helper scripts and utilities used during the build process of Armagetron Advanced. These scripts assist with various build-related tasks such as resource processing, version generation, and changelog creation.

Key scripts include:
- `sortresources.py` - A Python script that sorts resource files to ensure consistent ordering. This is important for deterministic builds and proper resource loading order in the game.
- `sortresources` - A shell wrapper script for the Python resource sorter, making it easier to invoke from Makefiles.
- `version` - A shell script for generating or extracting version information during the build process.
- `patchnotes.py` - A Python script that generates patch notes and changelogs from git commit history, useful for release documentation.

These scripts are invoked by the main Makefile system during various build targets, particularly during resource processing and distribution creation.

## Directory Structure

```
.
├── patchnotes.py     # Patch notes/changelog generator from git
├── sortresources     # Shell wrapper for resource sorting
├── sortresources.py # Python script for sorting resource files
└── version          # Version information generator
```

## Technologies

- **Languages**: Python, POSIX shell
- **Version Control**: Git (for patchnotes generation)
- **Build Integration**: Invoked by Makefiles

## Coding Conventions

- **Python Scripts**: Standard Python with shebang lines
- **Shell Scripts**: POSIX-compliant syntax
- **Error Handling**: Exit with meaningful error codes
- **Usage**: Designed to be called from Makefiles

## Key Patterns

- Build automation helper pattern
- Resource processing pattern
- Version generation pattern
- Changelog automation pattern

## Build System

- Scripts are part of `EXTRA_DIST` in batch/Makefile.am
- `sortresources.py` is invoked during resource processing
- `patchnotes.py` is used for generating release documentation
- `version` script is used for version string generation
- Scripts may be installed to appropriate locations or used only during build
