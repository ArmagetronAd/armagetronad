# src/tools/values/ Directory

## Summary

Value expression parsing and evaluation system for Armagetron Advanced's configuration and mathematical calculations.

## Details

This directory contains the value expression system, which provides advanced mathematical expression parsing, evaluation, and comparison capabilities. It forms the core of Armagetron Advanced's powerful configuration system, allowing complex calculations and dynamic value generation from configuration files.

The system supports mathematical expressions, logical operations, comparisons, and function calls, making it one of the most sophisticated configuration systems in the game.

## Directory Structure

```
.
├── vRegistry.cpp/h      # Value registry and variable management
├── vCore.cpp/h         # Core value expression system
├── vParser.ypp/h        # Expression parser (Bison/Flex)
├── vCollection.cpp/h   # Value collections and containers
├── veMath.cpp/h        # Mathematical expression evaluation
├── veComparison.cpp/h  # Comparison operations
├── veLogic.cpp/h        # Logical operations and expressions
├── vebMathExpr.cpp/h   # Mathematical expression building
├── vebLegacy.cpp/h     # Legacy expression compatibility
└── vebCFunction.h      # Custom function support
```

## Technologies

- **Language**: C++
- **Parser Generator**: Bison (vParser.ypp) for expression grammar
- **Build System**: GNU Autotools
- **Dependencies**: Standard C++ library, expression parsing utilities

## Key Components

### Core System
- **vCore**: Core value expression classes and base functionality
- **vRegistry**: Variable registration and management system
- **vParser**: Expression parsing using generated parser (Bison)

### Mathematical Operations
- **veMath**: Mathematical expression evaluation engine
- **vebMathExpr**: Mathematical expression building and construction
- **veComparison**: Comparison operators and relational expressions
- **veLogic**: Logical operations (AND, OR, NOT, etc.)

### Advanced Features
- **vCollection**: Collections of values and complex data structures
- **vebLegacy**: Compatibility with legacy expression formats
- **vebCFunction**: Support for custom user-defined functions

## Integration

- **Used by**: Configuration system, game rules, AI decision making
- **Purpose**: Enable complex mathematical and logical expressions in configurations
- **Build**: Compiled as part of libtools.a

## Key Features

- **Mathematical Expressions**: Support for arithmetic, trigonometric, logarithmic, and other mathematical functions
- **Variable Support**: Dynamic variables and constants with scoping
- **Function Calls**: User-defined and built-in functions
- **Logical Operations**: Boolean logic and conditional expressions
- **Comparisons**: Relational operators for value comparisons
- **Type System**: Strong typing with automatic type conversion

## Usage Examples

- **Configuration**: `SETTING value "sin(time) * 2 + cos(angle)"`
- **Game Rules**: Complex conditions for game mechanics
- **AI Decisions**: Mathematical expressions for AI behavior calculations
- **Dynamic Values**: Real-time calculations based on game state

## Parser Technology

- **Bison Grammar**: vParser.ypp defines the expression grammar
- **Recursive Descent**: Parsing of complex nested expressions
- **Error Handling**: Graceful error recovery and reporting

## Performance Considerations

- **Caching**: Expression results may be cached for performance
- **Optimization**: Constant expression folding and simplification
- **Lazy Evaluation**: On-demand evaluation of expressions