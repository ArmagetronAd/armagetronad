# src/ui/ Directory

## Summary
User interface layer handling input, menus, and user interaction. Built as `libui.a`.

**Doxygen Group**: `@defgroup UI` - User Interface. Menus and input handling (defined in README.md)

## Details

The UI directory provides the user interaction layer for Armagetron Advanced. It manages input devices (keyboard, mouse, joystick) and the menu system. The UI layer is platform-agnostic, using SDL for input handling.

Key components include: input handling (`uInput.cpp/h`) for keyboard, mouse, and joystick events; input queue (`uInputQueue.cpp/h`) for buffered input processing and recording/playback; and menu system (`uMenu.cpp/h`) providing a comprehensive menu framework with various menu item types.

The menu system supports multiple item types: basic items, exits, selections (dropdowns), toggles, integer/float sliders, string inputs (with history), submenus, actions, and file selectors. Menus use a callback-based architecture with `tCallback` for event handling. Input can be processed immediately or queued for playback (used for demo recording/replay).

The layer integrates with the render system for displaying menus and with the console system for text input/output.

## Directory Structure

```
.
├── README.md             # Doxygen @defgroup UI definition
├── uInput.cpp            # Input handling (keyboard, mouse, joystick)
├── uInput.h             # Input handling header
├── uInputQueue.cpp       # Buffered input processing and recording
├── uInputQueue.h        # Buffered input processing and recording header
├── uMenu.cpp             # Menu system
└── uMenu.h              # Menu system header
```

## Technologies

- **Language**: C++
- **Input**: SDL (Simple DirectMedia Layer) events
- **Build System**: Autotools
- **Dependencies**: librender.a, libtools.a, SDL

## Coding Conventions

- **Class Prefix**: `u` for UI classes (uMenu, uInput, uMenuItem, etc.)
- **Global Prefix**: `u` for UI-related globals, `su_` for static UI functions
- **Menu Items**: Hierarchy with `uMenuItem` base class and specialized subclasses
- **Input Handling**: `HandleEvent(SDL_Event event)` pattern
- **Callbacks**: `FUNCPTR` and `INTFUNCPTR` typedefs for menu actions
- **Menu State**: `OnEnter()`/`OnExit()`/`OnRender()` lifecycle methods
- **Input Queue**: Recording and playback via `uInputQueue`
- **Selection Items**: Template-based `uMenuItemSelection<T>` for generic selection menus
- **Text Input**: `uMenuItemString` with cursor positioning and editing

## Key Patterns

- Composite pattern for menu hierarchies
- Observer pattern for input events
- Command pattern for menu actions
- State pattern for menu navigation
- Template method pattern for menu item types
- Mediator pattern for input dispatching

## Build System

- Compiled as `libui.a` static library
- Dependencies: `-iquote @srcdir@/ui`, requires librender.a and libtools.a
- Input queue used by recording system for demo playback
