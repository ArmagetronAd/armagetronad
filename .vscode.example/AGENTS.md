# .vscode.example/ Directory

## Summary
Sample Visual Studio Code configuration for Armagetron Advanced development.

## Details

This directory contains a sample VS Code configuration that provides a starting point for developing Armagetron Advanced. The configuration includes settings for code intelligence, build tasks, debug configurations, and recommended extensions.

The configuration is designed to work with the project's Autotools-based build system. It can be used by either copying to `.vscode/` for customization or symlinking for automatic updates.

## Directory Structure

```
.
├── README.md                 # Setup instructions and overview
├── .gitignore                # Ignore patterns for VS Code
├── c_cpp_properties.json     # C/C++ language server configuration
├── extensions.json           # Recommended VS Code extensions
├── launch.json               # Debug configurations
├── settings.json             # VS Code workspace settings
└── tasks.json               # Build task configurations
```

## Technologies

- **Editor**: Visual Studio Code
- **Language Support**: C/C++ extension for IntelliSense
- **Build System**: GNU Autotools integration via tasks
- **Debugging**: GDB/LLDB support for client and server

## Features

- **IntelliSense**: Configured to understand project include paths and compiler flags
- **Build Tasks**: Predefined tasks for building client/server in debug/release modes
- **Debug Configurations**: Setup for debugging with record/playback functionality
- **Test Integration**: Works with TestMate C++ extension for test discovery and execution
- **Recommended Extensions**: C/C++, TestMate C++, and other helpful extensions

## Setup

Two setup options are available:

1. **Copy for customization (recommended):**
   ```bash
   cp -r .vscode.example .vscode
   ```
   This allows customization without affecting the repository.

2. **Symlink for updates:**
   ```bash
   ln -s .vscode.example .vscode
   ```
   This keeps the configuration in sync with repository updates.

## Notes from Humans
### GUARDRAIL: The AI Agents keep out of this section.
