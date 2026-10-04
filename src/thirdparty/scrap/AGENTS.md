# src/thirdparty/scrap/ Directory

## Summary

Clipboard and data scrap handling library for cross-platform clipboard operations.

## Details

This directory contains the scrap library, which provides clipboard functionality for Armagetron Advanced. It handles clipboard text and data operations in various formats, enabling copy/paste operations and data transfer between applications.

The library provides a simple C interface for clipboard operations, making it easy to integrate with the rest of the codebase.

## Directory Structure

```
.
├── scrap.cpp             # Clipboard implementation
├── scrap.h              # Clipboard header with C interface
├── Makefile.am           # Build configuration
├── Makefile.in           # Generated build configuration
└── COPYING              # License information
```

## Technologies

- **Language**: C and C++
- **Build System**: GNU Autotools (Makefile.am)
- **Dependencies**: Platform-specific clipboard APIs

## Key Components

### Clipboard Interface
- **scrap.h**: C interface header with clipboard function declarations
- **scrap.cpp**: Platform-specific clipboard implementation

### Functions
- **init_scrap()**: Initialize clipboard system
- **lost_scrap()**: Clean up clipboard resources
- **put_scrap()**: Copy data to clipboard
- **get_scrap()**: Retrieve data from clipboard

### Data Types
- **SCRAP_TEXT**: Text data format identifier (0x54455854)
- Support for arbitrary data formats beyond text

## Integration

- **Used by**: UI layer for copy/paste functionality
- **Purpose**: Enable clipboard operations in game interfaces
- **Build**: Compiled as part of the tools or UI libraries

## Platform Support

- **Cross-platform**: Designed to work across different operating systems
- **Abstraction**: Provides consistent interface regardless of underlying platform
- **Fallback**: Graceful handling of platforms without clipboard support

## Usage Examples

- **Text Copy**: Copy game information or chat text to clipboard
- **Data Export**: Export game data or configurations via clipboard
- **Cross-Application**: Integration with external tools and applications

## Build Configuration

- Integrated into main build system via Makefile.am
- Platform-specific implementations selected during compilation