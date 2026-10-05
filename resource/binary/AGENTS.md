# resource/binary/ Directory

## Summary

Binary resource files for Armagetron Advanced, including pre-processed textures and game assets.

## Details

This directory contains binary resource files that have been pre-processed or are in their final binary format for use in the game. Unlike the proto/ directory which contains source resources, this directory contains ready-to-use assets.

The binary resources are typically pre-processed textures, images, or other assets that have been optimized or converted for efficient loading and use in the game.

## Directory Structure

```
.
├── Lucifer/             # Lucifer theme/texture resources
│   └── sick/           # Lucifer sick theme textures
└── wrtlprnft/          # wrtlprnft theme/texture resources
```

## Technologies

- **File Types**: Pre-processed image files (PNG, etc.)
- **Purpose**: Ready-to-use game assets
- **Processing**: Pre-processed for optimal performance

## Resource Types

- **Textures**: Pre-processed texture files with .aatex.png extension
- **Optimized Assets**: Images optimized for game use
- **Theme Resources**: Theme-specific textures and visual elements

## Integration

- **Resource System**: Loaded directly by the resource management system
- **Rendering**: Used by the rendering system for efficient texture loading
- **Performance**: Pre-processed for optimal loading performance

## Usage

- **Fast Loading**: Quick loading of pre-processed textures
- **Theme Support**: Theme-specific resources for different visual themes
- **Fallback System**: May provide fallback resources when source resources are unavailable

## Processing

- Files are typically generated from source resources in proto/ directory
- May include optimized formats, sizes, or compression
- Processed during build or resource preparation phase