# src/swig/ext/engine/ Directory

## Summary

SWIG interface definitions for Armagetron Advanced's engine module.

## Details

This directory contains SWIG interface files that expose the engine module's functionality to scripting languages. The engine module includes core game simulation components such as physics, game objects, collision detection, and other fundamental game mechanics.

## Directory Structure

```
. (SWIG interface files with .i extension)
```

## SWIG Interface Files

- **eSensor.i**: Collision detection sensor interfaces
- **eNetGameObject.i**: Networked game object interfaces
- **eGameObject.i**: Base game object interfaces
- **Other Engine Files**: Additional engine-related SWIG interfaces

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Target**: Likely Python (and potentially others)
- **Module**: Engine functionality

## Integration

- **Engine Module**: Exposes src/engine/ functionality to scripting
- **Parent Directory**: Part of src/swig/ext/ organization
- **Build System**: Processed by SWIG during build to generate language bindings

## Key Features

- **Physics Access**: Scripting access to physics calculations and simulations
- **Game Object Control**: Scripting control over game objects and entities
- **Collision Detection**: Scripting access to collision and interaction systems
- **Engine Utilities**: Access to engine utility functions and helpers

## Usage

- **Game Modding**: Modify game physics and behavior through scripting
- **Automation**: Automate engine-related tasks and calculations
- **Testing**: Script-based testing of engine functionality
- **Simulation**: Custom simulations and physics experiments