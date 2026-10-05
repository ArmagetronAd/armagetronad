# src/tron/ Directory

## Summary
Game logic and main application layer implementing the Armagetron lightcycle game. Built as `libtron.a` and the main executables.

**Doxygen Group**: `@defgroup Game` - Game library. Concrete classes for cycles, walls, score keeping (defined in README.md)

## Details

This directory contains the high-level game logic that builds upon the engine layer. It implements the actual Tron/lightcycle gameplay, AI opponents, game modes, menus, and application flow. The library is built as `libtron.a` and includes the main entry points for both the client (`armagetronad_main`) and dedicated server (`armagetronad_main_master`).

Key components include: the main game class (`gGame`) managing game state, rounds, and settings; AI system (`gAIBase`, `gAICharacter`) for computer-controlled players; arena management (`gArena`); cycle physics and movement (`gCycle`, `gCycleMovement`); game entities like explosions (`gExplosion`), walls (`gWall`), win zones (`gWinZone`), sensors (`gSensor`), sparks (`gSparks`), particles (`gParticles`); spawn system (`gSpawn`); HUD and UI (`gHud`, `gLanguageMenu`, `gLogo`, `gMenus`); server browser (`gServerBrowser`); game parsing (`gParser`) for map/config files; team management (`gTeam`); camera control (`gCamera`); and friends/server favorites system.

The `gGame` class inherits from `nNetObject` for network synchronization. Game settings (`gGameSettings`) configure match parameters. The module coordinates between engine (physics), network (multiplayer), render (graphics), and UI (menus) layers.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup Game definition
├── gAIBase.cpp            # AI base class
├── gAIBase.h             # AI base class header
├── gAICharacter.cpp       # AI character implementation
├── gAICharacter.h        # AI character implementation header
├── gArena.cpp             # Arena/level management
├── gArena.h              # Arena/level management header
├── gArmagetron.cpp        # Main game entry point
├── gCamera.cpp            # Game camera control
├── gCamera.h             # Game camera control header
├── gCycle.cpp             # Lightcycle game logic
├── gCycle.h              # Lightcycle game logic header
├── gCycleMovement.cpp     # Cycle movement physics
├── gCycleMovement.h      # Cycle movement physics header
├── gExplosion.cpp         # Explosion effects
├── gExplosion.h          # Explosion effects header
├── gFloor.cpp             # Floor rendering
├── gGame.cpp              # Main game loop and state
├── gGame.h               # Main game loop and state header
├── gHud.cpp               # Heads-up display
├── gHud.h                # Heads-up display header
├── gLanguageMenu.cpp      # Language selection menu
├── gLanguageMenu.h       # Language selection menu header
├── gLogo.cpp              # Logo display
├── gLogo.h               # Logo display header
├── gMenus.cpp             # Menu system
├── gMenus.h              # Menu system header
├── gParser.cpp            # Configuration/map parsing
├── gParser.h             # Configuration/map parsing header
├── gParticles.cpp         # Particle effects
├── gParticles.h          # Particle effects header
├── gSensor.cpp            # Sensor logic
├── gSensor.h             # Sensor logic header
├── gServerBrowser.cpp     # Server browser
├── gServerBrowser.h      # Server browser header
├── gSparks.cpp            # Spark effects
├── gSparks.h             # Spark effects header
├── gSpawn.cpp             # Player spawning system
├── gSpawn.h              # Player spawning system header
├── gStuff.cpp             # Miscellaneous game utilities
├── gStuff.h              # Miscellaneous game utilities header
├── gTeam.cpp              # Team management
├── gTeam.h               # Team management header
├── gWall.cpp              # Wall management
├── gWall.h               # Wall management header
├── gWinZone.cpp           # Win zone logic
├── gWinZone.h            # Win zone logic header
├── gFriends.cpp           # Friends list management
├── gFriends.h            # Friends list management header
├── gServerFavorites.cpp   # Server favorites
└── gServerFavorites.h    # Server favorites header
```

## Technologies

- **Language**: C++
- **Build System**: Autotools
- **Dependencies**: libengine.a, libui.a, libnetwork.a, librender.a, libtools.a, thirdparty/particles

## Coding Conventions

- **Class Prefix**: `g` for game logic classes (gGame, gCycle, gArena, etc.)
- **Naming**: `sg_` prefix for global game state variables/functions
- **Game Loop**: `GameLoop(bool input)` in `gGame` drives simulation and rendering
- **Network Objects**: `gGame`, `gCycle` inherit from `nNetObject` for sync
- **Settings**: `gGameSettings` struct for match configuration
- **Menus**: `gMenus` implements in-game menu system
- **AI**: State-based AI with difficulty levels
- **Game States**: Finite state machine for match progression

## Key Patterns

- Singleton pattern for game state management
- State pattern for game modes and AI
- Observer pattern for game events
- Factory pattern for game object creation
- Mediator pattern for game coordination
- Command pattern for player actions

## Build System

- Compiled as `libtron.a` static library
- Main executables: `armagetronad_main` (client), `armagetronad_main_master` (dedicated server)
- `armagetronad_main` sources: `tron/gFloor.cpp` (entry point)
- `armagetronad_main_master` sources: `network/master.cpp`
- LDADD: `libtron.a libenginecore.a libengine.a libnetwork.a libui.a librender.a libtools.a`
- Includes `-I${top_srcdir}/src/thirdparty/particles -I${top_srcdir}/src/tron`
