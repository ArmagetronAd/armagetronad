# src/engine/sound/base/ Directory

## Summary

Base sound system classes providing the fundamental audio interface for Armagetron Advanced.

## Details

This directory contains the platform-independent base classes for the sound system. These classes define the core interfaces and data structures that are implemented by platform-specific backends in the sdl_mixer/ directory.

## Directory Structure

```
.
└── eChannel.h          # Base audio classes: eWavData, eChannel
```

## Classes

### eWavData
- Manages wave file data and loading
- Handles volume control for sound effects
- Provides access to underlying audio data (Mix_Chunk*)

### eChannel
- Represents an audio playback channel
- Manages 3D positional audio calculations
- Handles sound playback, looping, and volume control
- Tracks channel state (playing, dirty, busy)
- Supports channel ownership by game objects
- Manages delayed sound playback

## Technologies

- **Language**: C++ (C++98/03)
- **Dependencies**: SDL_mixer types (with dedicated server fallbacks)

## Key Features

- **3D Audio**: Positional audio using camera and game object coordinates
- **Channel Management**: Static channel counting for resource management
- **State Tracking**: Dirty flags for efficient updates
- **Object Association**: Channels can be owned by game objects for automatic positioning

## Integration

- **Extended by**: sdl_mixer/ implementations provide concrete SDL_mixer functionality
- **Used by**: Engine sound system (eSound.cpp) for audio playback management