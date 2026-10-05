# src/thirdparty/particles/ParticleDLL/ Directory

## Summary

Dynamic Link Library interface and extended particle system components for Armagetron Advanced's particle effects.

## Details

This directory contains the ParticleDLL implementation of the particle system, providing additional particle functionality and potentially dynamic loading capabilities. It appears to be an alternative or extended implementation of the particle system, possibly designed for plugin architecture or advanced particle features.

The directory includes both core particle system components and extended API functionality, with support for various particle behaviors, rendering techniques, and management systems.

## Directory Structure

```
.
├── ParticleState.cpp      # Particle state management implementation
├── ParticleState.h        # Particle state header
├── Particle.h             # Main particle class definitions
├── ParticleGroup.h        # Particle group management
├── other_api.cpp          # Additional API implementations
├── action_api.cpp         # Particle action API
├── actions.cpp            # Particle action implementations
├── actions.h              # Particle action headers
├── Makefile.old           # Legacy build configuration
└── ParticleDLL.old.vcproj  # Legacy Visual Studio project
```

## Technologies

- **Language**: C++
- **Build System**: Legacy support (Makefile.old, Visual Studio project)
- **Dependencies**: Main particle system headers (papi.h, pDomain.h, pVec.h)
- **Platform**: Originally designed for Windows DLL loading

## Key Components

### Particle Core
- **Particle.h**: Main particle class definitions and base functionality
- **ParticleState.cpp/h**: Particle state management and lifecycle
- **ParticleGroup.h**: Grouping and management of related particles

### Action System
- **action_api.cpp**: API for particle action system
- **actions.cpp/h**: Implementation of various particle actions and behaviors
- **other_api.cpp**: Additional particle functionality and extensions

### DLL Interface
- Designed for dynamic loading of particle implementations
- Plugin architecture for extendable particle system
- Cross-platform compatibility considerations

## Integration

- **Extends**: Main particle system in parent directory
- **Used by**: Particle effects rendering in the game
- **Purpose**: Provide advanced particle behaviors and dynamic loading
- **Build**: May be compiled as separate library or integrated into main particle system

## Features

- **Advanced Particle Actions**: Extended set of particle behaviors and effects
- **State Management**: Comprehensive particle lifecycle and state control
- **Group Management**: Efficient handling of particle groups and systems
- **API Extensions**: Additional particle functionality beyond core system

## Legacy Status

- Contains legacy build files (Makefile.old, .vcproj)
- May require modernization for current build system
- Maintains compatibility with existing particle system architecture

## Usage

- **Advanced Effects**: Complex particle systems for game visuals
- **Dynamic Loading**: Potential for runtime particle system extensions
- **Custom Behaviors**: Specialized particle actions for unique effects