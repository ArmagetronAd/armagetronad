# music/ Directory

## Summary

Background music files for Armagetron Advanced's in-game audio system.

## Details

This directory contains music files used as background music in Armagetron Advanced. The music system enhances the gaming experience by providing atmospheric soundtracks during gameplay, menu navigation, and other game states.

Music files are typically in formats supported by SDL_mixer, such as OGG, MP3, or WAV, depending on the compilation options and available libraries.

## Directory Structure

```
. (Music files in various supported audio formats)
```

## Technologies

- **Audio Formats**: OGG, MP3, WAV (depending on SDL_mixer support)
- **Audio System**: SDL_mixer for music playback
- **Dependencies**: SDL_mixer library with music support (--enable-music)

## Integration

- **Sound System**: Managed by the game's sound system in src/engine/sound/
- **Configuration**: Music playback controlled by game configuration settings
- **Resource System**: Music files loaded and managed by the resource system
- **Game States**: Different music tracks for different game states (menu, gameplay, etc.)

## Key Features

- **Background Music**: Atmospheric soundtracks for different game contexts
- **Configurable**: Volume, playback, and selection configurable through settings
- **Streaming**: Efficient streaming of music files for memory efficiency
- **Playlist Support**: Support for multiple music tracks and playlists
- **Cross-Platform**: Works across different platforms with appropriate audio backends

## Usage

- **Menu Music**: Background music for main menu and submenus
- **Gameplay Music**: Atmospheric music during actual gameplay
- **State Music**: Different tracks for different game states (paused, loading, etc.)
- **Custom Music**: Support for user-provided music files

## Configuration

- **Volume Control**: Separate volume settings for music vs. sound effects
- **Track Selection**: Selection of available music tracks
- **Playback Modes**: Loop, shuffle, or sequential playback options
- **Enable/Disable**: Option to disable music entirely for performance or preference