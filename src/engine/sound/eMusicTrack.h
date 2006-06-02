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

#include "rSDL.h"
#ifndef DEDICATED
#include <SDL_mixer.h>
#else
// dummy types
typedef int Mix_Chunk;
typedef int Mix_Music;
typedef int Mix_MusicType;
#endif
#include "tPlayList.h"


#ifndef EMUSICTRACK_H
#define EMUSICTRACK_H

#include <map>

// Forward declarations from other files
class eGameObject;
class eCoord;

// Forward declarations from this file
class eWavData;
class eSoundMixer;
class eChannel;

/*******************************************************************************
 *
 * eMusicTrack
 *      This class represents a music track
 *
 *******************************************************************************/

class eMusicTrack {
public:
    eMusicTrack();
    eMusicTrack(const char* filename, bool isinstalled=false);
    ~eMusicTrack();

    void SetVolume(int newVolume) { m_Volume = newVolume; };
    int GetVolume() { return m_Volume; };

    void Loop() { m_Loop = true; };

    tString const &GetFileName() { return m_Filename; };

    bool Play();
    void Stop() {
#ifndef DEDICATED
        Mix_HaltMusic();
#endif
    };
    void Next();
    void Previous();
    void Pause();
    void VolumeUp();
    void VolumeDown();
    void Mute();

    void Update();
    void SetDirty();

    bool LoadSong(tSong thesong);
    void LoadPlaylist(const char* filename);

    void SetPlaylist(int i) { usePlaylist = i; if(m_Playlist) m_Playlist->SetPlaylist(i); };

    void FadeOut(); // Fades out the track if it's currently playing
bool IsPlaying() { return m_IsPlaying; };
    bool HasSong() { return m_HasSong; };
    void UnloadSong();
    // Determines if *any* music is playing
    static bool musicIsPlaying;

    void MusicFinished();
    static eMusicTrack* currentMusic;
private:
    void Init(bool isinstalled);

    Mix_Music* LoadFile(const char* filename);
    void LoadAATrack(tSong& thesong);
    void LoadVorbisTrack(tSong& thesong);
    void PlayCurrentSequence();

    // Do we play the track from installed music?
    bool m_IsInstalled;

    int m_Volume;
    bool m_Loop;
    // Determines if *this* track is playing
    bool m_IsPlaying;

    int usePlaylist;

    bool m_isDirty;

    bool m_HasSong;

    bool m_SequenceChange;

    double m_Pos;
    std::map<tString, Mix_Music*>::iterator m_SequencePos;
    Uint32 m_StartTime;
    tSong m_CurrentSong;

    std::map<tString, Mix_Music*> m_Tracklist;
    eSoundMixer* m_mixer;

    // The file associated with this music track
    tString m_Filename;
    Mix_Music* m_Music;
    Mix_MusicType m_MusicType;

    tPlayList* m_Playlist;
};

#endif

