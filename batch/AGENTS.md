# batch/ Directory

## Summary
Build automation, deployment scripts, and utility tools for Armagetron Advanced.

## Details

The batch directory contains scripts and utilities for building, packaging, deploying, and maintaining Armagetron Advanced. These are development and release engineering tools rather than runtime components.

Key subdirectories include `make/` which contains build helpers and utilities. The `make/` directory has several important scripts:
- `sortresources.py` - A Python script that sorts resource files for consistent ordering during build
- `sortresources` - A shell wrapper for the Python script
- `version` - A script for generating version information
- `patchnotes.py` - Generates patch notes/changelog from git history

At the root of the batch directory, `rcd_startstop.in` is a template for init scripts that handle starting and stopping Armagetron servers. This is used for system service integration on Unix-like systems.

For AI and CI use, `test_builds.sh` provides a canonical way to build and test the project with multiple configurations. The `llvm-gcov.sh` wrapper script enables lcov code coverage analysis when using the Clang compiler, which produces coverage data in a format incompatible with the standard gcov tool.

## Directory Structure

```
.
├── Dockerfile              # Docker build configuration
├── llvm-gcov.sh           # LLVM gcov wrapper for lcov compatibility
├── rcd_startstop.in       # Init script template for server/services
├── test_builds.sh         # Multi-configuration test build script for AI/CI
└── make/                  # Build helper scripts
    ├── sortresources.py  # Resource sorting tool
    ├── sortresources     # Shell wrapper for sortresources.py
    ├── version           # Version generation script
    └── patchnotes.py     # Changelog/patch notes generator
```

## Technologies

- **Language**: Shell scripts, Python
- **Build Tools**: Autotools, Make
- **Version Control**: Git (for patchnotes generation)

## Coding Conventions

- **Script Naming**: Descriptive names with .py or .sh extensions as appropriate
- **Template Files**: .in extension for configure template files
- **Portability**: Shell scripts use POSIX-compliant syntax where possible

## Key Patterns

- Build automation pattern
- Template processing pattern
- Version generation pattern
- Changelog automation pattern

## Build System

- `batch_DATA` in Makefile.am installs scripts
- `rcd_startstop.in` processed by configure to create actual init scripts
- Scripts installed to appropriate locations based on `--enable-sysinstall` and related options
- `sortresources.py` used during build to process resource files

## Test Build Automation

- `test_builds.sh` - Canonical script for building and testing with multiple configurations
  - Environment variables for customization: TEST_ONLY, BUILD_ONLY, FORCE_RECONFIGURE, VERBOSE, COVERAGE, JOBS
  - Each configuration builds in a separate `/build/test_<name>` directory
  - Returns exit code equal to number of failed configurations
  
Example usage:
```bash
# Run debug builds with default compiler
./batch/test_builds.sh debug

# Run all configurations
./batch/test_builds.sh all

# Run all configurations with all available compilers
./batch/test_builds.sh full

# Run specific configurations
./batch/test_builds.sh dedicated

# List available configurations
./batch/test_builds.sh list

# Run with verbose output
VERBOSE=1 ./batch/test_builds.sh debug
```
