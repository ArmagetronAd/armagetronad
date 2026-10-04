# src/engine/ Directory

## Summary
Core game engine components implementing the fundamental game simulation framework. Built as `libenginecore.a` and `libengine.a`.

**Doxygen Group**: `@defgroup Engine` - Engine library containing Math, 2D Collisions, Game Objects, Players and Teams (defined in README.md)

## Details

The engine directory contains the core simulation logic for Armagetron Advanced. It implements the game world model with grids, walls, paths, game objects, players, teams, and the physics of lightcycle movement. The library is divided into `libenginecore.a` (minimal core: `eGameObject`, `eGrid`) and `libengine.a` (full engine).

Key components include: the grid system (`eGrid`) managing the playable arena geometry; game objects (`eGameObject` as abstract base for all entities, with `eReferencableGameObject` and `eStackGameObject` variants) that can interact, move, and be rendered; wall system (`eWall`, `eAdvWall`) for track boundaries; path finding (`ePath`); player management (`ePlayer`); team system (`eTeam`); coordinate system (`eCoord`); camera management (`eCamera`); sensor system (`eSensor`); timing (`eTimer`); voting (`eVoter`); sound (`eSound`); display management (`eDisplay`); debug visualization (`eDebugLine`); authentication (`eAuthentication`); network-integrated game objects (`eNetGameObject`); axes management (`eAxis`); floor/terrain (`eFloor`); lag compensation (`eLagCompensation`); and chat (`eChat`).

The engine heavily uses the tools library for data structures and utilities. It provides the foundation for game mechanics without UI or network-specific code. Game objects implement `Timestep()` for simulation, `Render()` for drawing, and `InteractWith()` for collision/physics.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup Engine definition
├── eAdvWall.cpp           # Advanced wall features
├── eAdvWall.h            # Advanced wall features header
├── eAuthentication.cpp    # Player authentication system
├── eAuthentication.h     # Player authentication system header
├── eAxis.cpp              # Axes management
├── eAxis.h               # Axes management header
├── eCamera.cpp            # Camera system
├── eCamera.h             # Camera system header
├── eChat.cpp              # In-game chat
├── eChat.h               # In-game chat header
├── eCoord.h               # Coordinate system
├── eDebugLine.cpp         # Debug visualization lines
├── eDebugLine.h          # Debug visualization lines header
├── eDisplay.cpp           # Display management
├── eFloor.cpp             # Floor/terrain rendering
├── eFloor.h              # Floor/terrain rendering header
├── eGameObject.cpp        # Base game object class
├── eGameObject.h         # Base game object class header
├── eGrid.cpp              # Game grid system
├── eGrid.h               # Game grid system header
├── eKrawall.cpp           # Krawall network features
├── eKrawall.h            # Krawall network features header
├── eLagCompensation.cpp   # Network lag compensation
├── eLagCompensation.h    # Network lag compensation header
├── eNetGameObject.cpp     # Networked game objects
├── eNetGameObject.h      # Networked game objects header
├── ePath.cpp              # Path finding
├── ePath.h               # Path finding header
├── ePlayer.cpp            # Player logic
├── ePlayer.h             # Player logic header
├── eRectangle.cpp          # Rectangular area management
├── eRectangle.h           # Rectangular area management header
├── eSensor.cpp            # Collision detection sensors
├── eSensor.h             # Collision detection sensors header
├── eSound.cpp             # Sound effects system
├── eSound.h              # Sound effects system header
├── eTeam.cpp              # Team management
├── eTeam.h               # Team management header
├── eTimer.cpp             # Game timing
├── eTimer.h              # Game timing header
├── eTess2.h               # Tessellation utilities
├── eVoter.cpp             # Voting system
├── eVoter.h              # Voting system header
├── eWall.cpp              # Wall collision and rendering
└── eWall.h               # Wall collision and rendering header
```

## Technologies

- **Language**: C++ (C++98)
- **Build System**: Autotools
- **Dependencies**: libtools.a, standard C++ library
- **Geometry**: Custom 2D coordinate system with wall/face topology

## Coding Conventions

- **Class Prefix**: `e` for engine classes (eGameObject, eGrid, eWall, etc.)
- **Reference Counting**: Inherits from `tReferencable` via `eReferencableGameObject`
- **Simulation Loop**: `Timestep(REAL currentTime)` pattern for physics updates
- **Rendering**: `Render(const eCamera *cam)` for visual representation
- **Interaction**: `InteractWith(eGameObject *target, REAL time, int recursion)` for collision
- **Network Sync**: `eNetGameObject` bridges engine objects to network layer
- **Path Finding**: Grid-based navigation with face/wall topology
- **Smart Pointers**: `tJUST_CONTROLLED_PTR<eTeam>`, `tCHECKED_PTR(eGrid)`

## Key Patterns

- Abstract base class pattern for `eGameObject`
- Visitor pattern for collision detection
- Composite pattern for object hierarchies
- Observer pattern for game state changes
- Strategy pattern for different wall types
- Flyweight pattern for shared game resources

## Build System

- `libenginecore.a`: Contains only `eGameObject.cpp/h` and `eGrid.cpp/h` (minimal core)
- `libengine.a`: Contains all other engine files
- `libenginecore.a` compiled with `-O2` unless `ENGINECOREDEBUG` is defined
- Dependencies: `-I${top_srcdir}/src/network -I${top_srcdir}/src/engine`

## Automated Tests
- Unit tests: `eCoord.cpp` in `src/test/` (from fn-1 and fn-3)
- Unit tests: `eRectangle.cpp`, `eAxis.cpp` in `src/test/` (from fn-3)
