# src/swig-old/ext/ Directory

## Summary

Legacy SWIG extension modules organized by functional area.

## Details

This directory contains legacy SWIG extension modules, similar to the modern src/swig/ext/ organization but representing an older approach. The modules are organized by functional areas of the codebase, with SWIG interface files that expose functionality to scripting languages.

## Directory Structure

```
.
└── armagetronad/        # Legacy Armagetron-specific SWIG extensions
    ├── engine/          # Legacy engine module interfaces
    ├── network/         # Legacy network module interfaces
    ├── render/          # Legacy render module interfaces
    ├── tools/          # Legacy tools module interfaces
    ├── tron/           # Legacy tron module interfaces
    └── ui/             # Legacy UI module interfaces
```

## Technologies

- **SWIG**: Simplified Wrapper and Interface Generator
- **Status**: Legacy
- **Organization**: Functional area separation

## Integration

- **Legacy System**: Part of legacy SWIG configuration in src/swig-old/
- **Historical Context**: Older approach to SWIG interface organization
- **Compatibility**: May provide compatibility with legacy scripts or systems

## Key Features

- **Functional Organization**: SWIG interfaces organized by functional modules
- **Legacy Coverage**: Extensive coverage of Armagetron Advanced functionality
- **Historical Development**: Representation of earlier SWIG integration approach

## Comparison with Modern Approach

- **Organization**: Similar to modern approach but with potentially different structure
- **Coverage**: May have different scope or coverage compared to modern SWIG files
- **Evolution**: Shows development from legacy to modern SWIG organization