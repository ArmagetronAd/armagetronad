# src/swig/ext/ Directory

## Summary

SWIG extension modules organized by functional area for Armagetron Advanced.

## Details

This directory contains SWIG extension modules organized by functional areas of the codebase. Each subdirectory corresponds to a major module of Armagetron Advanced, containing SWIG interface files that expose that module's functionality to scripting languages.

The ext/ directory represents the modern, organized approach to SWIG interface definitions, with clear separation between different functional areas.

## Directory Structure

```
.
├── engine/          # Engine module SWIG interfaces
├── network/         # Network module SWIG interfaces
├── std/            # Standard library SWIG interfaces
├── tools/          # Tools module SWIG interfaces
├── tron/           # Game logic (tron) module SWIG interfaces
└── ui/             # UI module SWIG interfaces
```

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Organization**: Module-based organization for maintainability
- **Purpose**: Functional area separation in scripting interfaces

## Integration

- **Parent Directory**: Part of src/swig/ SWIG configuration
- **Module Mapping**: Each subdirectory maps to a major code module
- **Build System**: All extensions processed together during SWIG compilation

## Module Extensions

### engine/
- **Purpose**: SWIG interfaces for engine functionality
- **Files**: eSensor.i, eNetGameObject.i, eGameObject.i, etc.
- **Functionality**: Physics, game objects, collision detection, etc.

### network/
- **Purpose**: SWIG interfaces for network functionality
- **Files**: Network-related .i files
- **Functionality**: Network communication, synchronization, multiplayer

### std/
- **Purpose**: SWIG interfaces for standard library components
- **Files**: Standard library-related .i files (e.g., sstream.i)
- **Functionality**: Standard library bindings and utilities

### tools/
- **Purpose**: SWIG interfaces for tools/utility functionality
- **Files**: Tools-related .i files (e.g., tLocale.i)
- **Functionality**: Utility classes, data structures, helpers

### tron/
- **Purpose**: SWIG interfaces for game logic (tron) functionality
- **Files**: Game logic-related .i files (e.g., gCycle.i, gCycleMovement.i, etc.)
- **Functionality**: Game logic, AI, game objects, player management

### ui/
- **Purpose**: SWIG interfaces for user interface functionality
- **Files**: UI-related .i files
- **Functionality**: User interface, menus, input handling

## Usage

- **Comprehensive Coverage**: Each major functional area has dedicated SWIG interfaces
- **Modular Exposure**: Controlled exposure of functionality to scripting languages
- **Maintainability**: Organized structure for easy maintenance and updates