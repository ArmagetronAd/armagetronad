# textures/ Directory

## Summary

Texture resources for Armagetron Advanced, including game graphics, UI elements, and visual assets.

## Details

This directory contains texture files used throughout Armagetron Advanced for rendering game elements, user interfaces, backgrounds, and other visual components. Textures are a fundamental part of the game's visual presentation and are loaded by the rendering system.

The textures include various formats (PNG, JPG) and cover all visual aspects of the game from cycle trails to menu backgrounds.

## Directory Structure

```
.
└── tutorials/           # Tutorial-specific textures and visual guides
    ├── clipart/         # Clipart-style tutorial graphics
    ├── conquest/        # Conquest mode tutorials
    ├── doublegrind/     # Double Grind mode tutorials
    ├── grinding/        # Grinding tutorial graphics
    ├── navigation/      # Navigation tutorial graphics
    ├── speedkill/       # Speed kill tutorial graphics
    ├── speedkilldefense/# Speed kill defense tutorials
    ├── survival/        # Survival mode tutorials
    └── teamstart/       # Team start tutorials
```

## Technologies

- **Image Formats**: PNG, JPG (primary formats)
- **Graphics**: 2D textures for OpenGL rendering
- **Tools**: Image editing software for texture creation
- **Rendering**: Loaded and rendered by the OpenGL-based rendering system

## Integration

- **Resource System**: Managed by the game's resource management system
- **Rendering Layer**: Used by src/render/ for texture rendering
- **Game Objects**: Applied to various game objects and UI elements
- **Configuration**: Texture paths and settings configurable through game configuration

## Key Features

- **Texture Atlas**: May include texture atlases for efficient rendering
- **Multiple Resolutions**: Support for different screen resolutions and quality settings
- **Compression**: Texture compression for optimal performance
- **Alpha Channels**: Support for transparency and blending effects
- **Mipmapping**: Mipmap generation for distance-appropriate texture quality

## Usage

- **Game Visuals**: Cycle trails, walls, floors, and other game elements
- **UI Elements**: Menus, buttons, dialogs, and HUD components
- **Backgrounds**: Game and menu background images
- **Effects**: Visual effects and animations
- **Tutorials**: Visual guides and instructional graphics

## Resource Management

- Loaded on demand by the resource manager
- Cached for performance optimization
- May have fallback textures for missing files
- Configurable texture quality and filtering options

## Tutorial Textures

The tutorials/ subdirectory contains specialized textures for the game's tutorial system, providing visual guides and examples for different game modes and techniques.