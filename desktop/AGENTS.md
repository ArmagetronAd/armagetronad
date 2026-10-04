# desktop/ Directory

## Summary
Desktop integration files for Linux and Unix desktop environments.

## Details

The desktop directory contains files needed for integrating Armagetron Advanced into desktop environments on Linux and Unix-like systems. These files allow the game to appear in application menus, be launched from desktop icons, and be properly categorized in software centers.

The primary file is `armagetronad.desktop`, which is a desktop entry file following the freedesktop.org Desktop Entry Specification. This file defines how the game appears in desktop menus, including the name, icon, description, categories, and launch command. Template variables like @progname@, @progtitle@, and @progid@ are replaced during the build process with actual values from the configuration.

The `armagetronad.appdata.xml.in` file provides AppStream metadata for software centers and package managers. This XML file contains detailed application information including descriptions, screenshots, keywords, and other metadata that helps users discover and understand the software. Like the desktop file, it uses template variables that are replaced during build.

## Directory Structure

```
.
├── Makefile.am              # Build configuration for desktop files
├── armagetronad.desktop.in   # Desktop entry template
└── armagetronad.appdata.xml.in  # AppStream metadata template
```

## Technologies

- **Format**: freedesktop.org Desktop Entry Specification
- **Metadata**: AppStream XML format
- **Integration**: XDG desktop standards

## Coding Conventions

- **Template Variables**: @progname@, @progtitle@, @progid@ replaced during build
- **Desktop Entry**: Follows Desktop Entry Specification
- **AppStream**: Follows AppStream metadata specification

## Key Patterns

- Desktop integration pattern
- Template processing pattern
- Metadata provision pattern

## Build System

- Desktop files processed by configure to replace template variables
- `desktop_DATA` in Makefile.am installs processed files
- Installed to `${datadir}/applications/` for .desktop files
- Installed to `${datadir}/metainfo/` for .appdata.xml files
- Installation controlled by `--enable-desktop` configure option
