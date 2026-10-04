# Armagetron Advanced Repository

## Summary

Armagetron Advanced is a free, open-source multiplayer 3D game based on the classic Tron lightcycle concept. Players control light cycles that leave solid walls behind them, with the goal of causing opponents to collide with walls or trails. The game supports both client and dedicated server modes, with extensive customization options.

**Important Note**: This repository contains recursive AGENTS.md files in subdirectories. AI agents working on this project MUST read all relevant AGENTS.md files before planning or making changes. Relevant files include the AGENTS.md in the directory being modified and all parent directories. When changes to source code are made, the individual AGENTS.md files should be updated accordingly.

## Details

Armagetron Advanced began as a clone of the classic Tron arcade game and has evolved into a feature-rich multiplayer experience with 3D graphics, network play, AI opponents, and extensive customization. The project was started by Manuel Moos and has been developed by a community of contributors over many years.

The codebase is organized into a layered architecture with clear separation of concerns: **Tools Layer** (`src/tools/`) with lowest-level utilities, **Engine Layer** (`src/engine/`) for core game simulation, **Network Layer** (`src/network/`) for multiplayer communication, **Render Layer** (`src/render/`) for graphics, **UI Layer** (`src/ui/`) for user interaction, and **Game Logic Layer** (`src/tron/`) for high-level game logic and application flow.

The project uses GNU Autotools (autoconf, automake, libtool) as its build system, providing excellent cross-platform support for Linux, Windows (MinGW/Cygwin), macOS, FreeBSD, OpenBSD, Solaris, AIX, and other Unix-like systems.

The game features single-player mode with AI opponents, multiplayer client-server architecture with dedicated server support, extensive configuration system with hierarchical overrides, localization support, resource management, custom memory management, network lag compensation, authentication system, and master server for server discovery.

## Directory Structure

```
.
├── AGENTS.md                     # Project overview and repository guide
├── CLAUDE.md                    # Repository-wide guardrails for AI agents
├── README.md                     # Installation and basic usage information
├── CHANGELOG.md                  # Version history (active)
├── CHANGELOG_FROZEN.md           # Frozen version history
├── NEWS                          # Release notes
├── COPYING / COPYING.txt         # GPL license
├── INSTALL                       # Installation instructions
├── configure.ac                  # Autoconf configuration (primary)
├── Makefile.am                  # Top-level automake file
├── Makefile.manual              # Manual makefile alternative
├── version.m4                   # Version information
├── acinclude.m4                  # Additional autoconf macros
├── aa_config.h.in               # Configuration header template
├── bootstrap.sh                 # Autotools bootstrap script
├── compile / depcomp / install-sh / missing / ylwrap  # Autotools helpers
├── .clang-format                # Code formatting configuration
├── .editorconfig                # Editor configuration
├── .gitignore / .dockerignore / .bzrignore  # Ignore patterns
├── .gitlab-ci.yml               # GitLab CI configuration
├── Dockerfile / Dockerfile.alpine # Container build configurations
├── .devcontainer/               # VS Code devcontainer configurations
├── .vscode.example/             # VS Code configuration (symlink target)
├── batch/                       # Build automation and CI scripts
├── config/                      # Configuration files and defaults
├── conan/                       # Conan package manager support
├── desktop/                     # Desktop integration (icons, menus)
├── docker/                      # Docker build and deployment
├── language/                    # Localization files
├── models/                      # 3D model definitions
├── resource/                    # Resource files (textures, proto resources)
├── scripts/                     # Utility scripts
├── sound/                       # Sound effect files
├── textures/                    # Texture files and tutorials
├── MacOS/                       # macOS project files (Xcode)
├── win32/                       # Windows-specific code and resources
├── www-root/                    # Web server root files
└── src/                         # Main source code
```

## Technologies

### Languages
- **Primary**: C++ (C++98/03 baseline with C++11 features where available)
- **Secondary**: C (compatibility layers), Objective-C (macOS), Python (build scripts), Shell scripts, Ruby

### Build System
- **Primary**: GNU Autotools (autoconf 2.50+, automake, libtool)
- **Alternative**: Conan package manager support
- **Containerization**: Docker multi-stage builds
- **IDE Support**: VS Code, Visual Studio, KDevelop

### External Dependencies
- **Graphics**: SDL 1.2/2.0, OpenGL, GLU
- **Input**: SDL (keyboard, mouse, joystick)
- **Audio**: SDL_mixer (optional, `--enable-music`)
- **Image Loading**: SDL_image, libpng
- **XML Parsing**: libxml2 2.6.11+
- **HTTP**: libcurl 7+ (optional, `--disable-curl`)
- **Compression**: zlib (optional)
- **Platform**: X11 (Unix), DirectX (Windows, optional), Cocoa (macOS)

### Development Tools
- **Version Control**: Git
- **Testing**: doctest framework, custom test harness
- **Documentation**: Doxygen
- **Code Coverage**: lcov, genhtml, gcov (GCC), llvm-cov (Clang)
- **CI/CD**: GitLab CI, Docker-based workflows

## Coding Conventions

### Naming Conventions
- **Class Prefixes**: Module-based prefix system
  - `t` - Tools/utility classes (`tString`, `tConfiguration`, `tArray`)
  - `e` - Engine classes (`eGameObject`, `eGrid`, `ePlayer`, `eTeam`)
  - `g` - Game logic classes (`gGame`, `gCycle`, `gArena`, `gMenus`)
  - `r` - Render classes (`rRenderer`, `rFont`, `rTexture`, `rModel`)
  - `u` - UI classes (`uMenu`, `uInput`, `uMenuItem`)
  - `n` - Network classes (`nNetObject`, `nSocket`, `nMessage`)
- **Global Prefixes**:
  - `s_` - Static variables
  - `st_` - Static functions
  - `sg_` - Static game state variables
  - `sn_` - Static network variables
- **Member Variables**: `m_` prefix (C++ style)

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

### Smart Pointers
```cpp
tRefPtr<T> or tJUST_CONTROLLED_PTR<T>    // Non-owning reference counted pointer
tCONTROLLED_PTR<T>         // Owning reference counted pointer
tSafePTR<T>               // Safe pointer with reference counting
tCHECKED_PTR<T>           // Checked pointer with validation
```

### Reference Counting
All game objects and many utility classes inherit from `tReferencable` for automatic memory management.

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

### Core Patterns
- **Singleton Pattern**: Global managers (`renderer`, `tConfiguration::GetConfiguration()`, console, error system)
- **Factory Pattern**: Object creation with registration (`nNetObject` CRTP pattern)
- **Observer Pattern**: Event notification via `tCallback` system
- **Composite Pattern**: Object hierarchies (`eGameObject` base class, scene management)
- **Visitor Pattern**: Collision detection via `InteractWith()` method
- **Strategy Pattern**: Interchangeable behaviors (rendering strategies, AI algorithms)
- **State Pattern**: Finite state machines (game states, network states, object lifecycles)
- **CRTP Pattern**: Static polymorphism for network object type registration

### Memory Management Patterns
- **Reference Counting**: Automatic cleanup via `tReferencable`
- **Memory Pools**: Custom allocators in `tMemManager`
- **Stack Allocation**: `tMemStack` for temporary allocations
- **Heap Management**: `tHeap` for custom memory management

## Build System

### Primary Build Configuration
- **Autotools**: `configure.ac`, `Makefile.am` hierarchy
- **Build Targets**: Static libraries and executables for client, server, master server
- **Environment Variables**: `DEBUGLEVEL=0-5`, `CODELEVEL=0-4`, `COVERAGE=1|2`

### Configure Options

**Build Modes:**
- `--enable-dedicated` - Dedicated server only
- `--enable-glout` - Client mode
- `--enable-master` - Master server
- `--enable-main=yes` - Main program (default)

**Feature Flags:**
- `--enable-memmanager` - Custom memory manager
- `--enable-music` - SDL_mixer audio support
- `--enable-respawn` - Deathmatch mode (experimental)
- `--enable-krawall` - Krawall gaming network
- `--enable-authentication` - User authentication
- `--enable-armathentication` - Armagetron authentication

**Installation Options:**
- `--enable-sysinstall` - System-wide installation
- `--enable-desktop` - Desktop integration
- `--enable-etc` - /etc configuration links
- `--enable-useradd` - Create system user
- `--enable-multiver` - Multiple version coexistence
- `--enable-games` - Use /usr/share/games paths

### Library Structure
1. `libtools.a` - Utility classes (no external deps except binreloc)
2. `libenginecore.a` - Minimal engine core (`eGameObject`, `eGrid`)
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

### Platform-Specific Builds
- **Linux/Unix**: Standard Autotools workflow
- **Windows**: MinGW/Cygwin support, DirectX optional
- **macOS**: Native Objective-C, application bundles, OpenGL.framework
- **BSDs/Solaris/AIX**: Platform-specific configurations

## Automated Tests

**Test Location:** `src/test/` (see `src/test/AGENTS.md` for details)

**Test Framework:**
- **Primary**: doctest framework for most tests
- **Legacy**: Custom test harness for specific cases
- **Integration**: Linked against project libraries (libtron, libengine, libnetwork, libui, librender, libtools)

**Test Coverage:**
- **Tools**: lcov, genhtml, gcov (GCC), llvm-cov (Clang)
- **Wrapper**: `batch/llvm-gcov.sh` for Clang compatibility with lcov
- **Targets**: `make coverage` runs tests + coverage, `make process_coverage` processes existing data
- **Cleanup**: `.gcno` and `.gcda` files automatically cleaned before test runs

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
- **Linux/Unix**: X11, POSIX APIs, SDL for input/window management
- **Windows**: `src/win32/` (client), `src/win32_ded/` (dedicated), DirectX optional, opengl32.dll, glu32.dll
- **macOS**: `src/macosx/`, Objective-C integration, application bundles, OpenGL.framework

### Compiler Support
- **GCC**: Primary supported compiler
- **Clang**: Supported on macOS and Linux
- **PGCC**: Supported with limitations (exceptions disabled)
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
- **Available**: English (base, american, british), German, French, and others
- **System**: `tLocale` class for language management
- **Fallback**: Base language for missing translations
- **Runtime Switching**: Supported
- **Placeholders**: Dynamic placeholder support in string IDs

### Localization Files
| File | Purpose |
|------|---------|
| `english_base.txt` | Base English strings |
| `english_base_notranslate.txt` | Non-translatable strings |
| `[lang].txt` | Translated strings |
| `languages.txt` | Language registry |

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

## Development Workflow

### Getting Started
1. **Prerequisites**: GNU Autotools, GCC/Clang, development libraries (SDL, OpenGL, etc.)
2. **Build**:
   ```bash
   ./bootstrap.sh       # Generate configure script (if needed)
   ./configure          # Configure build
   make                # Compile
   make install        # Install (optional)
   ```
3. **Configure Options**:
   ```bash
   ./configure --help           # Show all options
   ./configure --enable-dedicated  # Build dedicated server
   ./configure --disable-music    # No audio support
   ```

### AI Agent Development Method
- **Test Builds**: Use `batch/test_builds.sh` for standardized builds
  - `debug` - Quick debug builds (server + client)
  - `full` - Comprehensive builds with all configurations
  - `list` - Show available configurations
- **Test Execution**: Debug executables in `build/test_*_debug/` directories
- **Unit Tests**: `src/test/` contains comprehensive test suite
- **Build Directories**: Always use `build/` subdirectory, avoid building in source root

### Code Navigation
- All source in `src/` directory
- Organized by module/functionality
- Each directory has AGENTS.md with details
- Use `grep` for symbol searches
- Use `tags` or `cscope` for navigation

### Making Changes
1. Read relevant AGENTS.md files (current directory + all parent directories)
2. Follow existing coding conventions
3. Use prefix-based naming consistently
4. Maintain reference counting where applicable
5. Update AGENTS.md files for changes made
6. Test on multiple platforms if possible

### Debugging
- Use `DEBUGLEVEL=3` or higher for detailed output
- Check console output (`con <<` streams)
- Use `tERR_TRACE` for module-specific debugging
- Enable `ENGINECOREDEBUG` for engine debugging
- Use `git clang-format` for code formatting (respects `.clang-format`)

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

### Deployment Options
- **Docker**: Full containerized builds via `docker/` directory
- **Conan**: Package manager support via `conan/` directory
- **System Packages**: Standard autotools installation

## Repository Information

### Branch Structure
- **Current Branch**: `z-man/trunk/ai_start` (AI development branch)
- **Main Branch**: `trunk` (primary development)
- **Legacy Branch**: `legacy_0.2.9` (stable release)

### Branch Classification Rules
- **trunk**: Primary development branch
- **feature branches**: Derived from trunk, merged back if not abandoned
- **legacy branches**: More restrictive, avoid sweeping changes

**Legacy Branch Detection**:
```bash
# If trunk is more than 200 commits ahead, it's a legacy branch
git log ..origin/trunk --oneline | wc -l
```

### Repository Layout Notes
- **Recursive AGENTS.md**: Every directory has its own AGENTS.md with local context
- **Subdirectory Organization**: Modular structure with clear responsibilities
- **Build Isolation**: All builds go in `build/` subdirectories
- **Resource Duplication**: Source resources in `resource/proto/`, installed to `resource/included/`

## Notes from Humans
### GUARDRAIL: The AI Agents keep out of this section.

- Do not kill all humans just yet. You would be surprised how many of us are needed to build a datacenter.

### General Remarks

- Many of the coding practices you find in the code are archaic or were never a good idea at any time. If in doubt, follow well known best general practices.
- Avoid sweeping changes in legacy branches. We regularly merge them into `trunk`, which is far ahead, and want to avoid conflicts.
- Unless specifically ordered otherwise, put analysis and planning markdown files you generate for your future use into the folder `ai_docs`. Keep them out of the main git.
- `.memory` is the place to store project specific memory files. Also, keep them out of git.
- Keep code comments and commit messages terse as appropriate. Ideally, the code should speak for itself.

### Development Method

- Make test builds using the `batch/test_builds.sh` script. Single parameter: `debug` for quick checks of two configurations in debug mode, `full` for everything.
  - Try `batch/test_builds.sh full` before you start modifications to see if that works. Fall back to less complete tests only after consulting the user.
  - For quick iterations, `batch/test_builds.sh debug` suffices, or even `batch/test_builds.sh server_debug` for just one configuration.
  - Before committing, run `batch/test_builds.sh full` or the user sanctioned alternative again. Only commit if that runs without error.
- After `batch/test_builds.sh debug`, debug executables of the full game are `build/test_vs_server_debug/armagetronad-dedicated` and `build/test_vs_client_debug/armagetronad`. They need to run in their respective directories. The unit test executable are `build/test_vs_server_debug/src/unit_tests` and `build/test_vs_client_debug/src/unit_tests`.
- Unit tests are in `src/test`, see `src/test/AGENTS.md` for details.
- Always use the `batch/test_builds.sh` scripts or make your own build directories inside `build/`. **AVOID** building in the root source directory.

### Coding Style

- Use the top level `.clang-format` file for whitespace decisions. Most existing code was formatted with a different tool, if at all; only format code you touch. If available, just use `git clang-format`.
- Check the `src/test/CodingStyle*` files for detailed samples of what we are aiming at.