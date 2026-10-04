# resource/proto/ Directory

## Summary
Prototype resource files and definitions for Armagetron Advanced.

## Details

The proto directory contains prototype resource files including textures, models, sounds, and other assets in their source/definition format. These files are processed and installed to the final resource locations. The directory is organized by contributor or content type, with each subdirectory containing resources from a specific author or for a specific purpose.

## Directory Structure

```
.
├── AATeam/              # Resources from AATeam
├── Anonymous/           # Anonymous contributor resources
│   ├── original/        # Original resource files
│   ├── polygon/         # Polygon-based resources
│   │   └── regular/     # Regular polygon resources
│   └── shapes/          # Various shape resources
├── Luke-Jr/             # Resources from Luke-Jr
│   └── n-gon/           # N-gon polygon resources
├── Your_mom/            # Resources from Your_mom
│   ├── inaktek/         # Inaktek resources
│   └── repeat/          # Repeating pattern resources
└── Z-Man/              # Resources from Z-Man
    └── fortress/         # Fortress-related resources
```

## Technologies

- **Formats**: Various image, model, and sound formats
- **Processing**: Resource compilation and optimization

## Key Patterns

- Contributor-organized resource structure
- Prototype to production resource pipeline
- Resource categorization by type and author
