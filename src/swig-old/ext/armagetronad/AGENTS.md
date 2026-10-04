# src/swig-old/ext/armagetronad/ Directory

## Summary

Legacy Armagetron-specific SWIG extension modules.

## Details

This directory contains legacy SWIG extension modules specific to Armagetron Advanced, organized by functional area. This represents an older approach to SWIG interface organization that was specifically tailored for the Armagetron project.

## Directory Structure

```
.
├── engine/          # Legacy engine module interfaces
├── network/         # Legacy network module interfaces
├── render/          # Legacy render module interfaces
├── tools/          # Legacy tools module interfaces
├── tron/           # Legacy tron (game logic) module interfaces
└── ui/             # Legacy UI module interfaces
```

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Project**: Armagetron Advanced-specific
- **Status**: Legacy
- **Organization**: Functional area separation

## Integration

- **Legacy SWIG**: Part of the legacy SWIG configuration in src/swig-old/
- **Armagetron Specific**: Tailored specifically for Armagetron Advanced
- **Historical Context**: Older approach to project-specific SWIG organization

## Key Features

- **Project-Specific**: SWIG interfaces designed specifically for Armagetron
- **Functional Modules**: Organization by functional areas (engine, network, etc.)
- **Legacy Coverage**: Extensive coverage of Armagetron functionality
- **Historical Development**: Earlier approach to SWIG integration for the project

## Comparison with Other SWIG Directories

- **Modern SWIG**: More generic organization in src/swig/
- **Legacy SWIG-old**: Generic legacy organization in src/swig-old/ext/
- **Armagetron Specific**: Project-specific organization with armagetronad/ prefix