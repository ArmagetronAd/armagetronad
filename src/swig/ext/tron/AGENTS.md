# src/swig/ext/tron/ Directory

## Summary

SWIG interface definitions for Armagetron Advanced's game logic (tron) module.

## Details

This directory contains SWIG interface files that expose the game logic module's functionality to scripting languages. The tron module includes high-level game logic, AI opponents, game objects, player management, and application flow.

## Directory Structure

```
. (Game logic SWIG interface files with .i extension)
```

## SWIG Interface Files

- **gCycle.i**: Cycle/lightcycle functionality interfaces
- **gCycleMovement.i**: Cycle movement and control interfaces
- **gAIBase.i**: AI base functionality interfaces
- **gSensor.i**: Game sensor and detection interfaces
- **Other Game Logic Files**: Additional tron-related SWIG interfaces

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Target**: Likely Python (and potentially others)
- **Module**: Game logic functionality

## Integration

- **Tron Module**: Exposes src/tron/ functionality to scripting
- **Parent Directory**: Part of src/swig/ext/ organization
- **Build System**: Processed by SWIG during build to generate language bindings

## Key Features

- **Game Logic Control**: Scripting access to game logic and rules
- **Cycle Control**: Scripting control over lightcycles and their behavior
- **AI Integration**: Access to AI systems and opponent behavior
- **Game Objects**: Scripting control over game objects and entities
- **Player Management**: Scripting access to player management systems

## Usage

- **Game Modding**: Modify game rules and behavior through scripting
- **AI Customization**: Custom AI opponents and behaviors through scripting
- **Automation**: Automate game-related tasks and workflows
- **Custom Game Modes**: Script-based custom game modes and variations
- **Testing**: Script-based testing of game logic functionality