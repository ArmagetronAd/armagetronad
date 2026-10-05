# src/thirdparty/ Directory

## Summary
Third-party libraries integrated into the project. Contains binreloc for relocatable executables and a particle system for visual effects.

**Doxygen Groups**: 
- `@defgroup Thirdparty` - Third Party Components: Things from other sources we embed
- `@defgroup Thirdparty_doctest` - Doctest unit testing framework (subgroup of Thirdparty)
- `@defgroup Thirdparty_binreloc` - Binreloc: Reliable way to determine where the executable resides (subgroup of Thirdparty)
- `@defgroup Thirdparty_particles` - Particles: Particle subsystem (subgroup of Thirdparty)
  (all defined in README.md)

## Details

The thirdparty directory contains external code integrated into Armagetron Advanced. These libraries are maintained within the project tree rather than as external dependencies.

The `binreloc` subdirectory contains the BinReloc library (version from autopackage.org), which provides functionality for creating relocatable executables. This allows Armagetron Advanced to find its data files regardless of where the binary is installed. The library provides functions to determine the executable's installation path at runtime. Integration files are `prefix.c` and `prefix.h`. The tools library includes these sources directly in its compilation.

The `particles` subdirectory contains a particle effects system used for visual effects in the game (explosions, trails, etc.). It provides a flexible particle API (`papi.h`), action system (`actions.cpp/h`, `action_api.cpp`), OpenGL rendering (`opengl.cpp`), and core system (`system.cpp`) with vector math (`p_vector.h`) and general utilities (`general.h`). The particle system is used by the tron layer for visual effects. It's compiled as a separate static library `libparticles.a`.

The thirdparty code is generally kept unchanged from its original form, with minimal integration changes.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup definitions for thirdparty components
├── .clang-format        # Clang-format configuration for thirdparty code
├── binreloc/
│   ├── prefix.c           # BinReloc implementation
│   └── prefix.h          # BinReloc header
└── particles/
    ├── action_api.cpp     # Particle action API
    ├── actions.cpp        # Particle actions
    ├── general.h          # General utilities
    ├── Makefile.am        # Particle system build configuration
    ├── opengl.cpp         # OpenGL rendering for particles
    ├── papi.h             # Particle API header
    ├── p_vector.h         # Vector mathematics
    └── system.cpp         # Particle system core
```

## Technologies

- **Language**: C and C++
- **Build System**: Autotools (separate Makefile.am for particles)
- **Graphics**: OpenGL (for particle rendering)
- **Dependencies**: Standard C/C++ libraries

## Coding Conventions

- **BinReloc**: `ENABLE_BINRELOC` preprocessor flag controls inclusion
- **Particle API**: `ParticleGroup`, `PAHeader`, `_ParticleState` classes
- **Integration**: Particles included via `-I${top_srcdir}/src/thirdparty/particles` in tron compilation
- **Build**: `libparticles.a` compiled separately, binreloc sources included in libtools.a

## Key Patterns

- Library integration pattern
- Facade pattern for external APIs
- Adapter pattern for integration with project code
- Wrapper pattern for type safety

## Build System

- BinReloc sources compiled into `libtools.a`
- Particles compiled as separate `noinst_LIBRARIES = libparticles.a`
- Particles Makefile: `libparticles_a_SOURCES = action_api.cpp actions.cpp opengl.cpp system.cpp general.h papi.h p_vector.h`
- Particles includes: `-iquote ${top_srcdir}/src` for access to project headers
