# textures/ Directory

## Summary
Texture and image asset storage for Armagetron Advanced.

## Details

The textures directory is intended to store texture image files used for rendering various elements in Armagetron Advanced. These textures are applied to 3D models, walls, floors, and other game objects to provide visual detail.

Currently, this directory appears to be empty or not populated in this repository version. Texture files may be:
- Bundled with the resource directory (in resource/proto/)
- Distributed separately in binary packages
- Created by the resource processing system during build

When populated, the directory would typically contain:
- `.png` files for texture images with transparency
- `.jpg` files for background and full-color textures
- Subdirectories organizing textures by use (cycle, wall, floor, ui, etc.)

Common textures in a Tron game include:
- Cycle body and wheel textures
- Wall textures for different wall types
- Floor textures for arena surfaces
- HUD and UI element textures
- Font textures for text rendering

## Directory Structure

```
.
└── [texture files when present]
```

## Technologies

- **Formats**: PNG (with alpha), JPEG (full color), potentially others
- **Loading**: SDL_image via `rTexture` class in src/render/
- **Rendering**: OpenGL texture mapping

## Coding Conventions

- **File Naming**: Descriptive names for texture purposes
- **Power of Two**: Textures typically use power-of-two dimensions for OpenGL compatibility
- **Alpha Channels**: PNG files for textures requiring transparency

## Key Patterns

- Texture resource pattern
- Mipmapping pattern for distance textures
- Texture atlas pattern for UI elements
- Fallback texture pattern

## Build System

- Texture files may be installed to `${datadir}/textures/`
- Alternatively bundled in resource/included/ directory
- Loading handled by `rTexture` class at runtime
- Supports user-provided texture overrides
- Fallback to default textures if custom ones unavailable
