# src/thirdparty/mathexpr/ Directory

## Summary

Mathematical expression parsing and evaluation library for complex mathematical operations.

## Details

This directory contains the mathexpr library, which provides functionality for parsing and evaluating mathematical expressions from strings. This is useful for advanced configuration options, formula-based calculations, and dynamic value computation in the game.

The library supports both C and C++ interfaces, making it versatile for integration throughout the codebase.

## Directory Structure

```
.
├── example.cpp          # C++ usage examples
├── example_c.cpp        # C usage examples
├── mathexpr.cpp         # Main expression parser implementation
├── mathexpr.h           # C++ header for expression parsing
├── mathexpr_c.cpp       # C interface implementation
├── mathexpr_c.h         # C header for expression parsing
├── Makefile.am          # Build configuration
├── Makefile.in          # Generated build configuration
└── COPYING             # License information
```

## Technologies

- **Language**: C and C++ (dual interface)
- **Build System**: GNU Autotools (Makefile.am)
- **Dependencies**: Standard C/C++ libraries

## Key Components

### Expression Parser
- **mathexpr.cpp/h**: C++ interface for mathematical expression parsing
- **mathexpr_c.cpp/h**: C interface for mathematical expression parsing
- **Functionality**: Parse string expressions, evaluate results, variable substitution

### Features
- **Mathematical Operations**: Support for arithmetic, trigonometric, logarithmic functions
- **Variable Support**: Dynamic variables and constants in expressions
- **Error Handling**: Graceful handling of syntax errors and invalid expressions
- **Type Safety**: Proper type checking and conversion

## Integration

- **Used by**: Game configuration system for formula-based settings
- **Purpose**: Enable complex mathematical calculations from configuration files
- **Build**: Compiled as part of the tools or engine libraries

## Usage Examples

- **Configuration**: `SETTING value "sin(x) + cos(y) * 2"`
- **Dynamic Values**: Compute game parameters based on mathematical formulas
- **Conditional Logic**: Mathematical expressions for game rule calculations

## Build Configuration

- Integrated into the main build system via Makefile.am
- May be included as part of libtools.a or compiled separately
- Supports both C and C++ compilation modes