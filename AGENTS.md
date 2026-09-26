# Armagetron Advanced Repository

## Summary

Armagetron Advanced is a free, open-source multiplayer 3D game based on the classic Tron lightcycle concept. Players control light cycles that leave solid walls behind them, with the goal of causing opponents to collide with walls or trails. The game supports both client and dedicated server modes, with extensive customization options.

**Important Note**: This repository contains recursive AGENTS.md files in subdirectories. AI agents working on this project MUST read all relevant AGENTS.md files before planning or making changes. Relevant files include the AGENTS.md in the directory being modified and all parent directories. When changes to source code are made, the individual AGENTS.md files should be updated accordingly.

## Details

Armagetron Advanced began as a clone of the classic Tron arcade game and has evolved into a feature-rich multiplayer experience with 3D graphics, network play, AI opponents, and extensive customization. The project was started by Manuel Moos and has been developed by a community of contributors over many years.

The codebase is organized into a layered architecture with clear separation of concerns:
- **Tools Layer** (`src/tools/`): Lowest-level utilities with no external dependencies (except binreloc)
- **Engine Layer** (`src/engine/`): Core game simulation (physics, objects, grid, players, teams)
- **Network Layer** (`src/network/`): Multiplayer communication and synchronization
- **Render Layer** (`src/render/`): Graphics and visualization (OpenGL-based)
- **UI Layer** (`src/ui/`): User interaction (input, menus)
- **Game Logic Layer** (`src/tron/`): High-level game logic, AI, and application flow

The project uses GNU Autotools (autoconf, automake, libtool) as its build system, providing excellent cross-platform support for Linux, Windows (MinGW/Cygwin), macOS, FreeBSD, OpenBSD, Solaris, AIX, and other Unix-like systems.

The game features:
- Single-player mode with AI opponents
- Multiplayer client-server architecture with dedicated server support
- Extensive configuration system with hierarchical overrides
- Localization support for multiple languages
- Resource management for textures, models, sounds
- Custom memory management (optional)
- Network lag compensation
- Authentication system (optional)
- Master server for server discovery

## Directory Structure

```
.
├── AGENTS.md                     # This file - project overview
├── README                        # Basic project information
├── CHANGELOG.md                  # Version history
├── CHANGELOG_FROZEN.md           # Frozen changelog
├── NEWS                          # Release notes
├── configure.ac                  # Autoconf configuration
├── Makefile.am                  # Top-level automake file
├── version.m4                   # Version information
├── accustomdir.m4                # Custom directory handling
├── batch/                       # Build automation scripts
├── config/                      # Configuration files
├── conan/                       # Conan package management
├── desktop/                     # Desktop integration files
├── docker/                      # Docker configuration
├── language/                    # Localization files
├── models/                      # 3D model definitions
├── resource/                    # Resource files (textures, sounds, etc.)
├── scripts/                     # Utility scripts
├── sound/                       # Sound effect files
├── textures/                    # Texture files
├── .vscode.example/            # VS Code sample configuration (symlinked from .vscode)
└── src/                         # Main source code
    ├── doc/                    # Documentation
    ├── dummy/                 # Stub implementations
    ├── engine/                # Core game engine
    ├── first/                 # First-time installation
    ├── macosx/                # macOS-specific code
    ├── network/               # Network communication
    ├── render/                # Rendering system
    ├── test/                  # Unit tests
    ├── thirdparty/            # External libraries
    ├── tools/                 # Utility libraries
    ├── tron/                  # Game logic
    ├── ui/                   # User interface
    ├── win32/                 # Windows code
    └── win32_ded/             # Windows dedicated server code
```

## Technologies

### Languages
- **Primary**: C++ (C++98/03 baseline with C++11 features where available)
- **Secondary**: C (for compatibility layers), Objective-C (macOS), Python (build scripts), Shell scripts

### Build System
- **GNU Autotools**: autoconf 2.50+, automake, libtool
- **Alternative**: Conan package manager support
- **Containerization**: Docker support for builds and deployment

### External Dependencies
- **Graphics**: SDL 1.2, OpenGL, GLU
- **Input**: SDL (keyboard, mouse, joystick)
- **Audio**: SDL_mixer (optional, with `--enable-music`)
- **Image Loading**: SDL_image, libpng
- **XML Parsing**: libxml2 2.6.11+
- **HTTP**: libcurl 7+ (optional, with `--disable-curl`)
- **Compression**: zlib (optional)
- **Platform**: X11 (Unix), DirectX (Windows, optional), Cocoa (macOS)

### Development Tools
- **Version Control**: Git
- **Testing**: Custom test harness, doctest framework
- **Documentation**: Doxygen
- **Code Coverage**: lcov, genhtml, gcov (GCC), llvm-cov (Clang)

## Coding Conventions

### Naming Conventions
- **Class Prefixes**: Module-based prefix system
  - `t` - Tools/utility classes (tString, tConfiguration, tArray)
  - `e` - Engine classes (eGameObject, eGrid, ePlayer, eTeam)
  - `g` - Game logic classes (gGame, gCycle, gArena, gMenus)
  - `r` - Render classes (rRenderer, rFont, rTexture, rModel)
  - `u` - UI classes (uMenu, uInput, uMenuItem)
  - `n` - Network classes (nNetObject, nSocket, nMessage)
- **Global Prefixes**:
  - `s_` - Static variables
  - `st_` - Static functions
  - `sg_` - Static game state variables
  - `sn_` - Static network variables
- **Member Variables**: `m_` prefix for member variables (C++ style)

### Header Guards
```cpp
#ifndef ArmageTron_<NAME>_H
#define ArmageTron_<NAME>_H
...
#endif
```

### Error Handling
```cpp
tASSERT(condition);           // Debug assertion
tERR_ERROR(message);         // Error level message
tERR_WARN(message);          // Warning level message
tERR_TRACE(channel, message); // Trace level message
tERR_DEBUG(message);         // Debug level message
```

Debug levels: very_low, low, normal, high
Debug channels: flow, dump, etc.

### Smart Pointers
```cpp
tRefPtr<T> or its alias tJUST_CONTROLLED_PTR<T>    // Non-owning reference counted pointer
tCONTROLLED_PTR<T>         // Owning reference counted pointer
tSafePTR<T>               // Safe pointer with reference counting
tCHECKED_PTR<T>           // Checked pointer with validation
```

### Reference Counting
All game objects and many utility classes inherit from `tReferencable` for automatic memory management via reference counting.

## Key Architecture Patterns

### Layered Architecture
```
User Interface (src/ui/)
    ↑
Game Logic (src/tron/)
    ↑
Network (src/network/)  →  Render (src/render/)
    ↑                             ↑
Engine (src/engine/)  ←  [Cross-communication]
    ↑
Tools/Utilities (src/tools/)
```

### Singleton Pattern
Global managers use singleton pattern for centralized access:
- `renderer` - Global renderer instance (src/render/rRender.h)
- `tConfiguration::GetConfiguration()` - Configuration manager
- Console system
- Error system

### Factory Pattern
Object creation with registration:
- Network objects registered via `nNetObject` CRTP pattern
- Game objects created through factory methods
- Resource loading with fallback chains

### Observer Pattern
Event notification system:
- `tCallback` system for generic callbacks
- Network observer pattern for state synchronization
- Event queue for deferred processing

### Composite Pattern
Object hierarchies:
- `eGameObject` as base for all game entities
- Tree structures for scene management
- Parent-child relationships in menus and UI

### Visitor Pattern
Collision detection and interaction:
- `InteractWith()` method for object interactions
- Dispatch to appropriate handlers based on object types

### Strategy Pattern
Interchangeable behaviors:
- Different rendering strategies
- Multiple path-finding algorithms
- Various AI difficulty levels

### State Pattern
Finite state machines:
- Game state management (menu, playing, paused, etc.)
- Network state (standalone, client, server)
- Object lifecycle states

### CRTP Pattern
Static polymorphism:
```cpp
template <typename T>
class nNOInitialisator : public nNetObject {
    // Type registration for network objects
};

class gGame : public nNOInitialisator<gGame> {
    // Game class with automatic network registration
};
```

### Memory Management Patterns
- **Reference Counting**: Automatic cleanup via `tReferencable`
- **Memory Pools**: Custom allocators in `tMemManager`
- **Stack Allocation**: `tMemStack` for temporary allocations
- **Heap Management**: `tHeap` for custom memory management

## Build System

### Configure Options

**Build Modes:**
- `--enable-dedicated` / `--enable-glout` - Client vs dedicated server
- `--enable-master` - Build master server
- `--enable-main` - Build main program (default: yes)

**Features:**
- `--enable-memmanager` - Custom memory manager
- `--enable-music` - SDL_mixer support for background music
- `--enable-respawn` - Deathmatch mode (experimental)
- `--enable-krawall` - Krawall gaming network support
- `--enable-authentication` / `--enable-armathentication` - Authentication support

**Installation:**
- `--enable-sysinstall` - System-wide installation
- `--enable-desktop` - Desktop integration
- `--enable-etc` - /etc configuration links
- `--enable-useradd` - Create system user
- `--enable-multiver` - Multiple version coexistence
- `--enable-games` - Use /usr/share/games/ paths

**Development:**
- `DEBUGLEVEL=0-5` - Debug verbosity (0=none, 5=maximum)
- `CODELEVEL=0-4` - Code checking strictness (0=none, 4=strict)
- `COVERAGE=1` or `COVERAGE=2` - Enable code coverage collection

### Code Coverage
The project supports code coverage analysis with both GCC and Clang compilers:
- **GCC**: Uses standard gcov tool for coverage data generation
- **Clang**: Uses llvm-cov gcov via `batch/llvm-gcov.sh` wrapper script that filters lcov-specific flags incompatible with llvm-cov
- **Coverage Data**: `.gcno` and `.gcda` files are automatically cleaned before test runs
- **Processing**: lcov and genhtml generate HTML reports in `coverage/` directory
- **Targets**: `make coverage` runs tests and processes coverage, `make process_coverage` generates reports from existing data

The `batch/llvm-gcov.sh` script enables seamless integration with lcov when using clang++.

### Platform Detection
```bash
case "$target" in
    *-*-cygwin* | *-*-mingw32*)   # Windows
        WIN32=1, opengl32, glu32, wsock32
    *-*-darwin*)                  # macOS
        MACOSX=1, OpenGL.framework
    *-*-linux*)                   # Linux
        LINUX=1, -lnsl, -lsocket
    *-*-solaris*)                 # Solaris
        SOLARIS=1, CAUTION_WALL=1
    *-*-freebsd* | *-*-openbsd*)  # BSD
        Platform-specific SDL config
    *)                            # Generic Unix
        Default Unix configuration
```

### Library Structure

**Static Libraries:**
1. `libtools.a` - Utility classes (no external deps except binreloc)
2. `libenginecore.a` - Minimal engine core (eGameObject, eGrid)
3. `libengine.a` - Full engine (all engine classes)
4. `libnetwork.a` - Network communication layer
5. `libui.a` - User interface layer
6. `librender.a` - Rendering subsystem
7. `libtron.a` - Game logic and main application
8. `libparticles.a` - Particle effects system

**Executables:**
- `armagetronad` - Main game client
- `armagetronad-dedicated` - Dedicated server
- `armagetronad-master` - Master server

### Makefile Structure

Top-level `Makefile.am` defines:
- Subdirectory recursion for all modules
- Global build flags (CPPFLAGS, CXXFLAGS, LIBS)
- Installation targets
- Clean targets

Each subdirectory has its own `Makefile.am` defining:
- Sources for that module
- Library or program targets
- Dependency relationships
- Installation paths

## Automated Tests

**Test Location:** `src/test/` (see `src/test/AGENTS.md` for details)

The test suite includes unit tests using the doctest framework and a legacy standalone test. Details about individual test files, test organization, coding style examples, and test coverage are documented in `src/test/AGENTS.md`.

**Test Framework:**
- **Primary**: doctest framework for most tests
- **Legacy**: Custom test harness for chat_prefix_test
- Links against project libraries (libtron, libengine, libnetwork, libui, librender, libtools)
- Uses assertion macros (tASSERT, etc.) and doctest assertions
- Integrated with build system via src/Makefile.am

**Running Tests:**
```bash
make check          # Run all tests
make check-TESTS   # Run specific test
```

## Cross-Platform Support

### Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Linux | Full | Primary development platform |
| Windows (MinGW) | Full | Recommended Windows build |
| Windows (Cygwin) | Full | POSIX compatibility layer |
| Windows (Native) | Partial | May require adjustments |
| macOS | Full | Native with Objective-C support |
| FreeBSD | Full | With sdl11-config |
| OpenBSD | Full | Custom init script handling |
| Solaris | Full | -lnsl -lsocket required |
| AIX | Full | -mthreads for GCC |
| BeOS | Legacy | Historical support |

### Platform-Specific Code

**Linux/Unix:**
- Uses X11 for OpenGL context
- Standard POSIX APIs
- SDL for input and window management

**Windows:**
- `src/win32/` - Client-specific compatibility
- `src/win32_ded/` - Dedicated server compatibility
- DirectX support (optional)
- opengl32.dll and glu32.dll for OpenGL
- wsock32.lib for networking

**macOS:**
- `src/macosx/` - macOS-specific code
- Objective-C support for native integration
- Application bundle creation
- OpenGL.framework for graphics

### Compiler Support

- **GCC**: Primary supported compiler
- **Clang**: Supported on macOS and Linux
- **PGCC**: Supported but with limitations (exceptions disabled)
- **MSVC**: Not directly supported (use MinGW/Cygwin)

**C++ Standard:**
- C++11 preferred (with fallback to C++0x)
- Features detected and enabled automatically
- Minimum baseline: C++98/03

## Configuration System

### Configuration Format

```
# Comment
SETTING_NAME value
SETTING_NAME value # inline comment

# Include other files
SINCLUDE filename.cfg
```

### Configuration Loading Order
1. Built-in defaults
2. System-wide config files (`/etc/armagetronad/`)
3. User-specific config files (`~/.armagetronad/`)
4. Command line arguments

### Configuration Files

| File | Purpose |
|------|---------|
| `config/settings.cfg` | Default client settings |
| `config/settings_dedicated.cfg` | Dedicated server defaults |
| `config/settings_visual.cfg` | Graphics/visual settings |
| `config/settings_authentication.cfg` | Authentication settings |
| `config/rc.config` | Runtime configuration |
| `config/aiplayers.cfg` | AI player configurations |
| `config/languages.txt` | Language index |

## Resource Management

### Resource Types

| Type | Directory | Files |
|------|-----------|-------|
| Textures | `resource/proto/`, `textures/` | .png, .jpg |
| Models | `models/` | .mod |
| Sounds | `sound/`, `resource/proto/` | .wav, .ogg |
| Languages | `language/` | .txt |
| Config | `config/` | .cfg |
| Scripts | `scripts/` | .sh |

### Resource Processing

1. Source resources in `resource/proto/`
2. Sorted by `batch/make/sortresources.py`
3. Installed to `${datadir}/resource/included/`
4. Loaded at runtime via search path

### Resource Loading

- `tResourceManager` - Central resource management
- Multiple fallback paths
- Caching for performance
- Lazy loading

## Localization

### Language Support

**Available Languages:**
- English (base, american, british variants)
- German (deutsch)
- French (french)
- Others may be available

### Localization Files

| File | Purpose |
|------|---------|
| `english_base.txt` | Base English strings |
| `english_base_notranslate.txt` | Non-translatable strings |
| `[lang].txt` | Translated strings for language |
| `languages.txt` | Language registry |

### Localization System

- `tLocale` class manages language selection
- Fallback to base language for missing translations
- Runtime language switching supported
- String IDs with dynamic placeholder support

## Network Architecture

### Network Modes

- `nSTANDALONE` - Single player, no network
- `nSERVER` - Multiplayer server
- `nCLIENT` - Multiplayer client

### Protocol Features

- Custom TCP-based protocol
- Message versioning (`nVersion`)
- Rate control (input and output)
- Spam protection
- Lag compensation
- Client-side prediction
- Server reconciliation

### Network Objects

- Objects inherit from `nNetObject`
- Automatic synchronization via CRTP
- Message descriptor system (`nDescriptor`)
- Per-client state tracking

### Authentication

- Optional authentication system
- Krawall protocol support
- MD5-based hashing
- Session management

## Command Line Arguments

### Common Options

```bash
armagetronad [options]

-fullscreen, -f       Fullscreen mode
-window, -w          Windowed mode
-dedicated           Dedicated server mode
-server PORT         Start server on PORT
-connect HOST:PORT   Connect to server
-config FILE         Load configuration from FILE
```

### Developer Options

```bash
DEBUGLEVEL=0-5       Debug verbosity (0=none, 5=max)
CODELEVEL=0-4        Code checking strictness

# Example:
DEBUGLEVEL=3 CODELEVEL=2 ./armagetronad
```

### Configuration Override

```bash
# Override specific settings from command line
armagetronad +set SETTING_NAME value
```

## Development Guidelines

### Getting Started

1. **Prerequisites:**
   - GNU Autotools (autoconf 2.50+, automake, libtool)
   - GCC or Clang
   - Development libraries (SDL, OpenGL, etc.)

2. **Build:**
   ```bash
   ./bootstrap.sh       # Generate configure script (if needed)
   ./configure          # Configure build
   make                # Compile
   make install        # Install (optional)
   ```

3. **Configure Options:**
   ```bash
   ./configure --help           # Show all options
   ./configure --enable-dedicated  # Build dedicated server
   ./configure --disable-music    # No audio support
   ```

### Code Navigation

- All source in `src/` directory
- Organized by module/functionality
- Each directory has AGENTS.md with details
- Use `grep` for symbol searches
- Use `tags` or `cscope` for navigation

### Making Changes

1. Read relevant AGENTS.md files
2. Follow existing coding conventions
3. Use prefix-based naming
4. Maintain reference counting
5. Update AGENTS.md for changes
6. Test on multiple platforms if possible

### Debugging

- Use `DEBUGLEVEL=3` or higher for detailed output
- Check console output (`con <<` streams)
- Use `tERR_TRACE` for module-specific debugging
- Enable `ENGINECOREDEBUG` for engine debugging

## Release Process

### Version Management

- Version from `version.m4` or `batch/make/version`
- Changelog from `CHANGELOG.md` and `CHANGELOG_FROZEN.md`
- Patch notes generated by `batch/make/patchnotes.py`

### Build Targets

```bash
make dist        # Create source distribution
make distcheck   # Create and test distribution
```

### Deployment

- Docker builds via `docker/` directory
- Conan packages via `conan/` directory
- System packages via standard autotools

## Repository Information

### Branch Information

- **Main Branch**: trunk
- **Legacy Branch**: legacy_0.2.9

### Version

- **Current Version**: 0.2.9 (legacy branch)
- **Version File**: version.m4
- **Source Date**: From git or SOURCE_DATE_EPOCH

## Notes from Humans
### GUARDRAIL: The AI Agents keep out of this section.

- Do not kill all humans just yet. You would be surprised how many of us are needed to build a datacenter.

### Branch Classification

We need to distinguish three kinds of branches.
- `trunk` itself
- feature branches, they are derived from `trunk` and will, if not abandoned, get merged back into `trunk` soon-ish
- legacy branches.

The same rules apply for feature branches and `trunk` itself. Legacy branches have more restrictions.

To find out whether you are on a legacy branch, check how many commits `trunk` is ahead,
usually by running `git log ..origin/trunk --oneline | wc -l`. If that is more than 200,
consider this branch legacy.

### General Remarks

- Many of the coding practices you find in the code are archaic or were never a good idea at any time. If on doubt, follow well know best general practices.
- Avoid sweeping changes in legacy branches. We regularly merge them into `trunk`, which is far ahead, and want to avoid conflicts.
- Unless specifically ordered otherwise, put analysis and planning markdown files you generate for your future use into the folder `ai_docs`. Keep them out of the main git.
- `.memory` is the place to store project specific memory files. Also, keep them out of git.
- Keep code comments and commit messages terse as appropriate. Ideally, the code should speak for itself.

### Development Method

- Make test builds using the `batch/test_builds.sh` script. Single parameter: `debug` for quick checks of two configurations in debug mode, `full` for everything.
  - Try `batch/test_builds.sh full` before you start modifications to see if that works. Fall back to less complete tests only after consulting the user.
  - For quick iterations, `batch/test_builds.sh debug` suffices, or even `batch/test_builds.sh server_debug` for just one configuration.
  - Before committing, run `batch/test_builds.sh full` or the user sanctioned alternative again. Only commit if that runs without error.
- After `batch/test_builds.sh debug`, debug executables of the full game are `build/test_vs_server_debug/armagetronad-dedicated` and `build/test_vs_client_debug/armagetronad`.
  They need to run in their respective directories. The unit test executable are `build/test_vs_server_debug/src/unit_tests` and `build/test_vs_client_debug/src/unit_tests`.
- Unit tests are in `src/test`, see `src/test/AGENTS.md` for details.
- Always use the `batch/test_builds.sh` scripts or make your own build directories inside `build/`. **AVOID** building in the root source directory. 

### Coding Style

- Use the top level `.clang-format` file for whitespace decisions. Most existing code was formatted with a different tool, if at all; only format code you touch. If available, just use `git clang-format`. 
- Check the `src/test/CodingStyle*` files for detailed samples of what we are aiming at.
