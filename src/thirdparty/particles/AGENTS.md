# src/thirdparty/particles/ Directory

## Summary
Particle effects system for visual effects (explosions, trails, etc.) in the game.

## Details

The particles directory contains a complete particle effects system used for rendering visual effects in Armagetron Advanced. The system provides a flexible framework for creating various particle-based effects including explosions, sparks, trails, and other dynamic visual elements.

Key components include: a flexible particle API (`papi.h`) defining the interface for creating and managing particle groups; action system (`actions.cpp/h`, `action_api.cpp`) implementing various particle behaviors and effects; OpenGL rendering (`opengl.cpp`) for rendering particles using OpenGL primitives; and core system (`system.cpp`) managing particle lifecycle and state. Vector mathematics (`p_vector.h`) and general utilities (`general.h`) support the system.

The particle system uses a data-driven approach where particle effects are defined declaratively and can be configured at runtime. It supports various render modes, blending modes, and particle behaviors.

## Directory Structure

```
.
├── action_api.cpp    # Particle action API
├── actions.cpp       # Particle action implementations
├── actions.h        # Particle action headers
├── general.h        # General utilities and definitions
├── Makefile.am      # Build configuration
├── opengl.cpp        # OpenGL rendering for particles
├── papi.h            # Public particle API header
├── p_vector.h        # Vector mathematics for particles
└── system.cpp        # Particle system core
```

## Technologies

- **Language**: C++
- **Graphics API**: OpenGL 1.x
- **Build System**: Autotools (separate Makefile.am)
- **Dependencies**: Standard C++ library, OpenGL headers

## Coding Conventions

- **External Code**: This is third-party code with minimal modifications
- **Classes**: `ParticleGroup`, `PAHeader`, `_ParticleState`
- **Build**: Compiled as separate static library

## Key Patterns

- Data-driven particle effects
- Factory pattern for particle creation
- Visitor pattern for particle rendering
- Flyweight pattern for particle sharing

## Build System

- Compiled as `noinst_LIBRARIES = libparticles.a`
- Sources: `action_api.cpp actions.cpp opengl.cpp system.cpp general.h papi.h p_vector.h`
- Includes: `-iquote ${top_srcdir}/src` for access to project headers
- Used by tron layer via `-I${top_srcdir}/src/thirdparty/particles` include path
