# src/engine/sound/sdl_mixer/ Directory

## Summary

SDL_mixer specific implementation of the Armagetron Advanced sound system.

## Details

This directory contains the concrete implementations of the sound system interfaces using the SDL_mixer library. These classes inherit from or mirror the base classes and provide the actual audio playback functionality.

## Directory Structure

```
.
├── eChannelSDLMixer.cpp      # SDL_mixer channel implementation
├── eChannelSDLMixer.h        # SDL_mixer channel header
├── eMusicTrackSDLMixer.cpp   # SDL_mixer music track implementation  
└── eMusicTrackSDLMixer.h     # SDL_mixer music track header
```

## Classes

### eWavDataSDLMixer
- Extends eWavData with SDL_mixer specific wave data handling
- Implements wave file loading using Mix_Chunk
- Manages volume control through SDL_mixer

### eChannelSDLMixer
- SDL_mixer implementation of audio channels
- Uses Mix_HaltChannel for stopping sounds
- Manages SDL_mixer channel IDs and state
- Implements 3D audio positioning using SDL_mixer effects

### eMusicTrackSDLMixer
- Music track playback using SDL_mixer
- Handles background music streaming
- Supports music volume and playback control

## Technologies

- **Language**: C++ (C++98/03)
- **Audio Library**: SDL_mixer
- **Dependencies**: Base sound classes (../base/), rSDL.h

## Key Features

- **Platform Specific**: Concrete implementation for SDL_mixer backend
- **Resource Management**: Automatic SDL_mixer resource cleanup
- **3D Audio Effects**: Uses SDL_mixer's effect system for positional audio
- **Conditional Compilation**: Only compiled in non-dedicated mode

## Integration

- **Extends**: Base classes from ../base/ directory
- **Used by**: Sound system when SDL_mixer is available