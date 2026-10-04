# scripts/ Directory

## Summary
Utility scripts for server administration and game management.

## Details

The scripts directory contains utility scripts primarily focused on server administration, player management, and automated tasks. These scripts are designed to be used by server operators to manage their Armagetron Advanced servers.

The `examples/` subdirectory contains example scripts demonstrating how to extend and customize server behavior:
- `flexban.sh` - An advanced player banning script that provides flexible ban management
- `flexban_cfg.sh` - Configuration for the flexban system

The flexban system provides:
- GeoIP-based banning using `geoiplookup` (if available)
- WhoIS lookup support for additional player information
- Flexible rule system for defining ban criteria
- Operates on the last player to enter the server
- Shell function library for ban, kick, and other administrative commands

These scripts are called by the server on various player events (join, chat, etc.) and can be customized or extended for specific server needs.

## Directory Structure

```
.
└── examples/              # Example server administration scripts
    ├── flexban.sh       # Flexible ban system
    └── flexban_cfg.sh   # Ban configuration
```

## Technologies

- **Language**: POSIX shell scripts
- **Dependencies**: Optional: geoiplookup (for GeoIP), whois (for domain lookups)
- **Integration**: Called by Armagetron server on player events

## Coding Conventions

- **Shell Scripts**: Standard POSIX shell syntax
- **Modular**: Scripts designed to be sourced and extended
- **Configuration**: Separate .cfg files for customizable parameters

## Key Patterns

- Server administration script pattern
- Event-driven script execution
- Modular configuration pattern
- GeoIP integration pattern

## Build System

- Scripts installed to `${datadir}/scripts/` or `${prefix}/share/games/armagetronad/scripts/`
- `scripts_DATA` in Makefile.am handles installation
- Examples in `scripts/examples/` subdirectory
- Can be customized by server operators
