# src/render/ Directory

## Summary
Rendering subsystem providing OpenGL-based 3D graphics, font rendering, and display management. Built as `librender.a`.

**Doxygen Group**: `@defgroup Render` - Render library. Screen initialization and OpenGL utilities (defined in README.md)

## Details

The render directory implements the graphical output layer of Armagetron Advanced. It provides an abstraction over OpenGL (`rRenderer` interface with `rGLRender` implementation) allowing for different rendering backends, though currently only OpenGL is used. The system handles all visual aspects: 2D/3D rendering, text display, textures, models, viewports, and screen management.

Key components include: OpenGL wrapper (`rGL.h`, `rGL.cpp`) with safe GL function wrappers; renderer interface (`rRender.h`) defining the rendering API; display management (`rScreen.cpp/h`, `rSDL.h`) for screen initialization and SDL integration; console output (`rConsole.cpp/h`, `rConsoleGraph.cpp`) for in-game text display with graph-based layout; font system (`rFont.cpp/h`) for text rendering; texture management (`rTexture.cpp/h`); model loading and rendering (`rModel.cpp/h`); display lists (`rDisplayList.cpp/h`) for OpenGL display list management; viewport management (`rViewport.cpp/h`); and system-dependent code (`rSysdep.cpp/h`) for platform-specific rendering setup.

The renderer uses a state-stack pattern for managing OpenGL state changes (matrix stack, flag stack). It supports immediate mode and retained mode rendering. The console system provides both in-game overlay and dedicated console rendering.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup Render definition
├── rConsole.cpp           # Console system
├── rConsole.h            # Console system header
├── rConsoleGraph.cpp      # Graph-based console layout
├── rDisplayList.cpp       # OpenGL display lists
├── rDisplayList.h        # OpenGL display lists header
├── rFont.cpp              # Font rendering
├── rFont.h               # Font rendering header
├── rGL.cpp                # OpenGL wrapper
├── rGL.h                 # OpenGL wrapper header
├── rGLRender.cpp          # OpenGL renderer implementation
├── rModel.cpp             # 3D model loading
├── rModel.h              # 3D model loading header
├── rRender.cpp            # Renderer base
├── rRender.h             # Renderer base header
├── rScreen.cpp            # Screen management
├── rScreen.h             # Screen management header
├── rSDL.h                # SDL integration
├── rSysdep.cpp            # System-dependent rendering
├── rSysdep.h             # System-dependent rendering header
├── rTexture.cpp           # Texture management
├── rTexture.h            # Texture management header
├── rViewport.cpp          # Viewport management
└── rViewport.h           # Viewport management header
```

## Technologies

- **Language**: C++
- **Graphics API**: OpenGL (1.x immediate mode style)
- **Windowing**: SDL (Simple DirectMedia Layer)
- **Build System**: Autotools
- **Dependencies**: libtools.a, SDL, OpenGL

## Coding Conventions

- **Class Prefix**: `r` for render classes (rRenderer, rFont, rTexture, etc.)
- **Global Renderer**: `extern rRenderer *renderer` singleton pattern
- **Inline Wrappers**: Inline functions in `rRender.h` delegate to `renderer` instance
- **GL Safety**: `#define glBegin #error glBegin disabled` pattern prevents direct GL calls
- **State Management**: `PushFlags()`/`PopFlags()` for GL state stack
- **Matrix Stack**: `ProjMatrix()`, `ModelMatrix()`, `TexMatrix()` for matrix mode switching
- **Primitives**: `BeginLines()`, `BeginTriangles()`, `BeginQuads()`, etc. for primitive types
- **Console Output**: `con <<` streaming operator for console messages

## Key Patterns

- Singleton pattern for renderer
- Facade pattern for OpenGL API
- Adapter pattern for platform-specific code
- State pattern for GL state management
- Proxy pattern for GL function calls

## Build System

- Compiled as `librender.a` static library
- Platform-specific: `render_extra` variable for macOS Objective-C files (currently empty)
- Dependencies: `-iquote @srcdir@/render`, requires libtools.a
- `rConsoleCout.cpp` in `EXTRA_DIST` (not compiled into library)
