# config/ Directory

## Summary
Configuration files for game settings, server defaults, and user preferences.

## Details

The config directory contains all configuration files used by Armagetron Advanced. These files define default settings, user preferences, server configurations, and other runtime parameters. The configuration system supports hierarchical loading, allowing system-wide defaults to be overridden by user-specific configurations.

Key files include: `settings.cfg` which contains the default client settings including display options, input bindings, network preferences, and gameplay parameters; `settings_dedicated.cfg` which provides defaults specifically for dedicated server operation; `settings_visual.cfg` for graphics and rendering settings; `settings_authentication.cfg` for authentication-related configuration; `rc.config` for runtime configuration that can be modified during game execution; `aiplayers.cfg` for AI opponent configurations; and `languages.txt` which indexes available language files.

The configuration files use a custom format with `SETTING_NAME value` syntax, supporting comments with `#` and file inclusion with `SINCLUDE filename.cfg`. Settings can be accessed and modified both from configuration files and from the command line.

## Directory Structure

```
.
├── Makefile.am           # Build configuration for config files
├── aiplayers.cfg         # AI player configurations
├── rc.config             # Runtime configuration
├── settings.cfg          # Default client settings
├── settings_authentication.cfg  # Authentication settings
├── settings_dedicated.cfg      # Dedicated server settings
├── settings_visual.cfg   # Visual/graphics settings
└── languages.txt         # Language index
```

## Technologies

- **Format**: Custom text-based configuration format
- **Parsing**: `tConfiguration` class in src/tools/
- **Inclusion**: Hierarchical with `SINCLUDE` directive

## Coding Conventions

- **Setting Format**: `SETTING_NAME value` or `SETTING_NAME value # comment`
- **Inclusion**: `SINCLUDE filename.cfg` to include other config files
- **Comments**: Lines starting with `#` are ignored
- **Overrides**: Later definitions override earlier ones

## Key Patterns

- Hierarchical configuration pattern
- Configuration override pattern
- File inclusion pattern

## Build System

- Configuration files are installed to `${sysconfdir}` (typically /etc/armagetronad/ or similar)
- `config_DATA` in Makefile.am lists files to install
- System-wide defaults can be overridden with `--enable-automakedefaults` or similar options
- Supports per-user configuration in `~/.armagetronad/` or similar
