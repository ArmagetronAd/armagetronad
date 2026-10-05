# src/first/ Directory

## Summary
First-time installation and setup scripts.

## Details

The first directory contains scripts and configuration for performing first-time setup tasks when Armagetron Advanced is installed on a system. These scripts handle system-wide initialization that needs to happen once when the software is first installed.

The primary purpose is to create necessary directories, set default configurations, initialize user data, and perform other one-time setup tasks. This is separate from the regular runtime initialization that happens each time the game starts.

The scripts are executed during the package installation process (via make install) and are triggered by the automake install-exec-local and uninstall-local targets.

## Directory Structure

```
.
└── Makefile.am           # Build rules for first-time installation
```

## Technologies

- **Language**: Shell scripts
- **Build System**: Autotools integration

## Coding Conventions

- **Shell Scripts**: Standard POSIX shell scripting
- **Integration**: Uses ../../install-first and ../../uninstall-first from root

## Key Patterns

- Installation hook pattern
- Delegation to root-level scripts
- Automake target-based execution

## Build System

- Contains `install-exec-local` and `uninstall-local` targets
- Delegates to root-level scripts:
  - `$(SHELL) $(top_srcdir)/install-first $(DESTDIR)$(bindir)`
  - `$(SHELL) $(top_srcdir)/uninstall-first $(DESTDIR)$(bindir)`
- Executed during `make install` and `make uninstall`
