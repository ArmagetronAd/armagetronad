# src/ Directory

## Summary
Main source code root containing core game logic, rendering, networking, and platform-specific implementations. All game code is organized within this directory.

**Important Note**: This repository contains recursive AGENTS.md files in subdirectories. AI agents working on this project MUST read all relevant AGENTS.md files before planning or making changes. Relevant files include the AGENTS.md in the directory being modified and all parent directories. When changes to source code are made, the individual AGENTS.md files should be updated accordingly.

## Details

The src/ directory is the primary location for all Armagetron Advanced source code. It contains the complete game implementation organized into modular subdirectories, each with a specific responsibility. The codebase is written in C++ with a custom build system based on GNU Autotools (autoconf, automake, libtool).

The architecture follows a layered approach with clear dependencies: tools (lowest level utilities) -> engine (core simulation) -> network/render/ui (specialized subsystems) -> tron (high-level game logic). This allows the dedicated server to be built without the rendering layer, and the client can selectively include features.

The directory contains extensive platform-specific code (win32, win32_ded, macosx) allowing the game to run on Windows, macOS, Linux, and various Unix-like systems. Each platform has its own compatibility layer and build configurations.

## Directory Structure

```
.
├── doc/                   # Documentation and Doxygen configuration
├── dummy/                # Stub implementations for build/testing
├── engine/               # Core game engine (physics, objects, grid)
├── first/                # First-time installation scripts
├── macosx/               # macOS-specific code and build scripts
├── network/              # Network communication layer
├── render/               # Rendering subsystem (OpenGL, SDL)
├── test/                 # Unit and integration tests
├── thirdparty/           # External libraries (binreloc, particles)
├── tools/                # Utility libraries and helpers
├── tron/                 # Game logic and main application
├── ui/                  # User interface (input, menus)
├── win32/                # Windows platform-specific code
├── win32_ded/            # Windows dedicated server code
├── config_ide.h          # IDE-specific configuration
├── defs.h                # Global definitions
└── Makefile.am           # Main source Makefile
```

## Technologies

- **Language**: C++ (C++98/03 with C++11 features where available)
- **Build System**: GNU Autotools (autoconf 2.50+, automake, libtool)
- **Windowing**: SDL 1.2
- **Graphics**: OpenGL 1.x
- **Networking**: Custom TCP/UDP protocol, Berkeley sockets
- **Dependencies**: libxml2, libcurl, libpng, SDL_image, SDL_mixer (optional)

## Coding Conventions

- **Prefix-based naming**: `t` for tools, `e` for engine, `g` for game, `r` for render, `u` for UI, `n` for network
- **Global prefix**: `s_` for static variables, `st_` for static functions, `sg_` for game globals
- **Header Guards**: `#ifndef ArmageTron_<NAME>_H` pattern
- **Smart Pointers**: `tJUST_CONTROLLED_PTR`, `tCONTROLLED_PTR`, `tSafePTR` for reference counting
- **Error Handling**: `tASSERT`, `tERR_ERROR`, `tERR_WARN`, `tERR_TRACE` macros with debug levels
- **Assertions**: Extensive use of assertions for invariant checking
- **Documentation**: Doxygen-style comments for public APIs

## Key Patterns

- **Layered Architecture**: Clear separation between tools, engine, network, render, UI, and game logic
- **Reference Counting**: Automatic memory management via smart pointers
- **Singleton Pattern**: Global managers (renderer, configuration, console)
- **Observer Pattern**: Event notification via callbacks
- **Composite Pattern**: Object hierarchies and scene graphs
- **Factory Pattern**: Object creation with registration
- **Visitor Pattern**: Collision detection and interaction
- **Strategy Pattern**: Different rendering/behavior strategies
- **State Pattern**: Game state management
- **CRTP Pattern**: Static polymorphism for type registration

## Build System

- **Static Libraries**: libtools.a, libenginecore.a, libengine.a, libnetwork.a, libui.a, librender.a, libtron.a, libparticles.a
- **Executables**: armagetronad (client), armagetronad-dedicated (dedicated server), armagetronad-master (master server)
- **Build Configurations**:
  - `--enable-dedicated`: Build dedicated server only
  - `--enable-master`: Build master server
  - `--enable-memmanager`: Custom memory manager
  - `--enable-music`: SDL_mixer support
  - `--enable-authentication`: User authentication
  - `--enable-krawall`: Krawall network features
  - `DEBUGLEVEL=0-5`: Debug verbosity
  - `CODELEVEL=0-4`: Code checking strictness

## Automated Tests

- Unit tests located in `src/test/` directory (see `src/test/AGENTS.md` for details)
- Test executables: `unit_tests` (doctest-based) and `chat_prefix_test` (legacy)
- Tests cover: geometry (eAxis, eCoord, eRectangle), data structures (tArray, tList, tLinkedList, tRing), memory management (tHeap, tMemStack), utilities (tString, tColor, tCallback, tRandomizer), exception handling, and XML parsing
- Tests integrated with build system via src/Makefile.am
- Uses doctest framework for most tests, custom harness for legacy chat_prefix_test

## Cross-Platform Support

- **Linux**: Primary development platform, full feature support
- **Windows**: MinGW/Cygwin, full support with DirectX option
- **macOS**: Native with Objective-C, application bundle support
- **FreeBSD/OpenBSD/Solaris/AIX**: Platform-specific configurations
- **BeOS**: Legacy support
