# src/engine/sound/ Directory

## Summary
Sound system implementation for Armagetron Advanced, providing abstract base interfaces and SDL_mixer backend for audio playback. Built with conditional compilation for dedicated server support.

## Details
The sound directory contains the audio subsystem split into two layers: abstract base interfaces (`base/`) and SDL_mixer implementation (`sdl_mixer/`). It provides sound effects and music playback with 3D spatial positioning relative to game objects. The system integrates with the main engine sound classes (`eSound`, `eSoundMixer`) and uses conditional compilation to provide dummy implementations for dedicated server builds.

## Directory Structure

```
.
├── base/
│   └── eChannel.h           # Abstract sound channel interface
└── sdl_mixer/
    ├── eChannelSDLMixer.cpp    # SDL_mixer channel implementation
    ├── eChannelSDLMixer.h      # SDL_mixer channel header
    ├── eMusicTrackSDLMixer.cpp # SDL_mixer music track implementation
    └── eMusicTrackSDLMixer.h   # SDL_mixer music track header
```

## Technologies
- **Audio Backend**: SDL_mixer library (optional, enabled with `--enable-music`)
- **Build Flags**: `-iquote @srcdir@/engine/sound` and `-iquote @srcdir@/engine/sound/sdl_mixer`
- **Conditional Compilation**: `DEDICATED` define for server builds (dummy types), `HAVE_LIBSDL_MIXER` for SDL_mixer availability

## Coding Conventions
- **Class Prefix**: `e` for engine classes (`eChannel`, `eWavData`, `eMusicTrack`)
- **Dedicated Server Stubs**: Dummy `Mix_Chunk` and `Mix_Music` typedefs when `DEDICATED` is defined
- **3D Audio**: Position-based sound calculation using `eCoord` for spatial audio
- **Reference Counting**: Uses engine's reference counting system via `eReferencableGameObject`

## Key Components

### Base Interface Layer (`base/`)
- `eChannel`: Abstract sound channel handling with volume control, 3D positioning, play/stop/loop functionality
- `eWavData`: Base WAV data container for sound effects with volume management

### SDL_mixer Implementation Layer (`sdl_mixer/`)
- `eChannelSDLMixer`: Concrete SDL_mixer channel implementation with full 3D audio support
- `eMusicTrackSDLMixer`: Music track management with playlist support, volume control, and playback states

## Key Patterns
- **Abstract Base + Concrete Implementation**: Base interfaces in `base/` with SDL_mixer implementations in `sdl_mixer/`
- **3D Spatial Audio**: Uses `eCoord` and `eCamera` for position-based sound calculation
- **Dependency Injection**: Game object references for position tracking
- **Playlist System**: Integrated with `tPlayList` for music track management

## Build Integration
- Files listed as `EXTRA_DIST` in main `src/Makefile.am`
- Include paths configured via `-iquote` compiler flags
- Conditional compilation for dedicated server (`#ifndef DEDICATED` blocks)
- Integration with main sound system via `#include "engine/sound/base/eChannel.h"`

## Debugging Tips
- Use `DEBUGLEVEL=3` for sound-related debug output
- Check `DEDICATED` define for server vs client behavior differences
- Verify `HAVE_LIBSDL_MIXER` is defined when audio support is expected
- 3D positioning issues: verify `eCoord` parameters in `Set3d()` calls
- Channel allocation: monitor `numChannels` static counter for channel availability