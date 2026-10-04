# models/ Directory

## Summary
3D model definitions and geometry data for game assets.

## Details

The models directory contains 3D model files used for rendering various game elements in Armagetron Advanced. These models define the geometry for lightcycles, walls, and other in-game objects.

The model files use a custom `.mod` format which is parsed by the `rModel` class in the render subsystem. This format is designed to be simple and efficient for the game's rendering needs.

Key model files include:
- `cycle_body.mod` - The main lightcycle body model
- Additional models for cycle parts (wheels, front, rear, etc.)
- Models for various game objects and effects

Each `.mod` file contains vertex and face definitions that describe the 3D geometry. The format supports:
- Vertex definitions with coordinates (x, y, z)
- Face definitions referencing vertices
- Normal calculations for lighting
- Texture coordinate support

## Directory Structure

```
.
└── cycle_body.mod    # Main lightcycle 3D model
```

## Technologies

- **Format**: Custom .mod 3D model format
- **Parsing**: `rModel` class in src/render/
- **Rendering**: OpenGL 1.x

## Coding Conventions

- **File Format**: `.mod` extension for model files
- **Vertex Format**: `v <id> <x> <y> <z>` for vertex definitions
- **Face Format**: `f <v1> <v2> <v3>` for triangular faces
- **Comments**: Lines starting with `#` are ignored

## Key Patterns

- Custom 3D model format
- Efficient geometry storage
- Direct OpenGL rendering

## Build System

- Model files installed to `${datadir}/models/`
- Loaded at runtime by `rModel` class
- Can be overridden by user-provided models
- Supports fallback to built-in models
