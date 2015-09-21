/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "config.h"

#ifndef ArmageTron_SOUNDMIXER_H
#define ArmageTron_SOUNDMIXER_H

#include "rSDL.h"
//#include "defs.h"
// #include "eGrid.h"
#include "tString.h"
#include "tSafePTR.h"
//#include "tLinkedList.h"
#ifndef DEDICATED
#include <SDL_mixer.h>
#else
// dummy types
typedef int Mix_Chunk;
typedef int Mix_Music;
#endif
#include "tPlayList.h"

#include "eChannel.h"

#include <deque>

void se_SoundMenu();

// This enum is likely temporary, and should be replaced with a proper map
enum MusicMode {
    TITLE_TRACK, GUI_TRACK, GRID_TRACK
};

enum SoundEffect {
    CYCLE_TURN,
    CYCLE_EXPLOSION,
    ANNOUNCER_1,
    ANNOUNCER_2,
    ANNOUNCER_3,
    ANNOUNCER_GO,
    NEW_MATCH,
    NEW_ROUND,
    ROUND_WINNER,
    MATCH_WINNER,
    ZONE_SPAWN,
    CYCLE_MOTOR,
    CYCLE_GRIND_WALL
};

extern char* MusicModeString[];
extern char* SoundEffectString[];

// Forward declarations from other files
class eGameObject;
class eCoord;

// Forward declarations from this file
class eMusicTrack;
class eChannel;

/*******************************************************************************
 *
 * eSoundMixer
 *      The sound mixer.  Use this to play all sounds and music.  It should
 *      only ever be created once, and it does that automatically.
 *
 *******************************************************************************
 *
 *  Use this class with a simple construct:
 *     eSoundMixer* mixer = eSoundMixer::GetMixer();
 *     mixer->DoWhatEverYouWant();
 *
 *******************************************************************************/

class eSoundMixer {

public:
    ~eSoundMixer();
    // Use this function to get the instance of the sound mixer object
    //   If it doesn't exist, it will be created
    static eSoundMixer* GetMixer();

    static void ShutDown();
    void Init();
    // SetMode sets what music mode we're in.  Modes are TITLE_TRACK, GUI_TRACK,
    //    or GAME_TRACK
    static void SetMode(MusicMode newMode);
    void PushButton(int soundEffect);
    void PushButton(int soundEffect, eCoord location);
    // What type is velocity likely to be passed in as?  Do we need it for a pushbutton
    //   effect?  'velocity' is meant to be the velocity of the object making the sound,
    //   so we can doppler shift it.
    // void PushButton(int soundEffect, eCoord* location, velocity);

    void SetMicrophoneOwner(eGameObject* newOwner);

    void PlayContinuous(int soundEffect, eGameObject* owner);
    void RemoveContinuous(int soundEffect, eGameObject* owner);

    // Callback for SDL_Mixer to call when a file is finished playing
    // It just passes down the line to the current track
    static void SDLMusicFinished();

    // Callback for an eMusicTrack to call when a song is finished playing
    // This is where the real work is done to handle when a song is finished
    static void SongFinished();

    static bool Music_play(REAL x);
    static bool Music_next_song(REAL x);
    static bool Music_previous_song(REAL x);
    static bool Music_stop(REAL x);
    static bool Music_pause(REAL x);
    static bool Music_volume_up(REAL x);
    static bool Music_volume_down(REAL x);

    void LoadPlaylist();

    // Callback for SDL_Mixer to call when any channel is finished playing
    static void ChannelFinished(int channel);
private:
    void __channelFinished(int channel);

public:
    // Call this method from the game loop to check if the mixer needs to do anything
    void Update();
private:
    // Gets the first available channel
    int FirstAvailableChannel();

    static eSoundMixer* _instance;
    bool sound_is_there;
    SDL_AudioSpec audio;

    static int m_Mode;
    tJUST_CONTROLLED_PTR< eGameObject > m_Owner;

    // Use m_isDirty to indicate a mode change.  The update() method will pick up the
    //    change.  This should be handled automatically by SetMode()
    static bool m_isDirty;
    // m_PlayMusic is a flag to disable music specifically
    bool m_PlayMusic;
    // m_active is a flag to disable sound effects specifically
    bool m_active;

    tSong m_CurrentSong;
    tPlayList* m_Playlist;

    std::deque<eWavData> m_SoundEffects;
    std::deque<eChannel> m_Channels;
    int m_numChannels;

    static eMusicTrack* m_TitleTrack;
    static eMusicTrack* m_GuiTrack;
    static eMusicTrack* m_GameTrack;

    // Used for the music track, when a song ends, mark m_isDirty true.  Then, when
    //   you start playing a new song, mark it false.
    static bool m_musicIsPlaying;

    // Nobody should ever construct this thing on its own
    eSoundMixer();
};

#endif



