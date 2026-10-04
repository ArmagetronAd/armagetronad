# src/swig/ Directory

## Summary

SWIG interface definitions for Armagetron Advanced's scripting and language binding support.

## Details

This directory contains SWIG (Simplified Wrapper and Interface Generator) interface files and configuration for generating language bindings for Armagetron Advanced. SWIG is used to create interfaces between C/C++ code and high-level languages like Python, allowing for scripting and automation capabilities.

The modern SWIG configuration is organized by module/extension, providing a structured approach to exposing the game's functionality to scripting languages.

## Directory Structure

```
.
└── ext/                 # Extension modules for different code areas
    ├── engine/          # Engine module SWIG interfaces
    ├── network/         # Network module SWIG interfaces
    ├── std/            # Standard library SWIG interfaces
    ├── tools/          # Tools module SWIG interfaces
    ├── tron/           # Game logic (tron) module SWIG interfaces
    └── ui/             # UI module SWIG interfaces
```

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Target Languages**: Likely Python (and potentially others)
- **Build System**: SWIG integration with Autotools build system

## Integration

- **Scripting Support**: Enables scripting capabilities for Armagetron Advanced
- **Build System**: SWIG files processed during build to generate language bindings
- **Module Organization**: Organized by functional areas (engine, network, tools, etc.)

## Key Features

- **Language Bindings**: Automatic generation of Python (or other) bindings
- **Module Extensions**: SWIG extensions for different functional areas
- **Type Mapping**: Automatic conversion between C++ and target language types
- **Functionality Exposure**: Controlled exposure of C++ functionality to scripting
- **Memory Management**: Handling of memory management between languages

## Usage

- **Scripting**: Enable Python scripting for game automation and customization
- **Plugin System**: Support for plugin development using scripting languages
- **Testing**: Script-based testing and validation
- **Automation**: Automated tasks and workflows using scripting languages

## SWIG Interface Files

- **Engine Module**: SWIG interfaces for engine functionality (eSensor.i, eNetGameObject.i, etc.)
- **Network Module**: SWIG interfaces for network functionality
- **Tools Module**: SWIG interfaces for utility functionality
- **Tron Module**: SWIG interfaces for game logic functionality
- **UI Module**: SWIG interfaces for user interface functionality
- **Standard Library**: SWIG interfaces for standard library components

## Build Process

- SWIG interface files processed by SWIG compiler during build
- Generated binding code compiled into language-specific modules
- Integration with main application for scripting support