# scripts/examples/ Directory

## Summary
Example server administration and utility scripts.

## Details

The examples directory contains example scripts that demonstrate how to extend, customize, and automate server administration tasks for Armagetron Advanced. These scripts are primarily focused on server-side functionality.

Key scripts include:
- `flexban.sh` - An advanced, flexible player banning system that provides sophisticated ban management capabilities. It supports GeoIP-based banning (using `geoiplookup` if available), WhoIS lookups, and a configurable rule system for defining ban criteria.
- `flexban_cfg.sh` - Configuration file for the flexban system, allowing server operators to customize ban rules, thresholds, and behavior.

These example scripts are designed to be:
- Used as-is for common server administration tasks
- Customized for specific server needs
- Extended with additional functionality
- Studied as examples of how to integrate with Armagetron server events

The scripts are typically called by the Armagetron server on various player events (join, chat, etc.) and can perform automated actions based on those events.

## Directory Structure

```
.
├── flexban.sh       # Flexible ban system
└── flexban_cfg.sh   # Ban configuration
```

## Technologies

- **Language**: POSIX shell scripts
- **Dependencies**: Optional: geoiplookup (GeoIP), whois (domain lookup)
- **Integration**: Called by Armagetron server on player events

## Coding Conventions

- **Shell Syntax**: POSIX-compliant where possible
- **Modular**: Designed to be sourced and extended
- **Configuration**: Separate .cfg files for parameters
- **Comments**: Documented for administrator understanding

## Key Patterns

- Server administration automation pattern
- Event-driven script execution
- Modular configuration pattern
- GeoIP integration pattern

## Build System

- Example scripts installed to `${datadir}/scripts/examples/`
- `scripts_DATA` or `example_Scripts_DATA` in Makefile.am
- Can be customized by server operators
- Used as templates for creating custom scripts
