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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "rSDL.h"
#ifndef DEDICATED
#include <SDL_mixer.h>
#else
// dummy types
typedef int Mix_Chunk;
typedef int Mix_Music;
#endif
#include "tPlayList.h"
#include "eCoord.h"

#ifndef ECHANNEL_H
#define ECHANNEL_H

// Forward declarations from other files
class eGameObject;
class eCoord;

// Forward declarations from this file
class eWavData;
class eMusicTrack;
class eChannel;

/*******************************************************************************
 *
 * eWavData
 *
 *******************************************************************************/

class eWavData {
public:
    eWavData();
    ~eWavData();
    eWavData(const char* filename);
    void LoadWavFile(const char* filename);

    Mix_Chunk* GetWavData() { return m_WavData; };

    void SetVolume(int newVolume) { m_Volume = newVolume; };
    int GetVolume() { return m_Volume; };

private:
    Mix_Chunk* m_WavData;
    int m_Volume;
    bool m_Playable;
};

/*******************************************************************************
 *
 * eChannel
 *      Description of a mixer channel, corresponds to an SDL_mixer channel
 *
 *******************************************************************************/

class eChannel {
public:
    eChannel();
    void SetVolume(int volume);
    int GetId() { return m_ChannelID; };
    void SetId(int newID) { m_ChannelID = newID; };

    void Set3d(eCoord home, eCoord soundPos, eCoord homeDirection);
    void PlaySound(eWavData& sound);
    void LoopSound(eWavData& sound);
    void StopSound() {
#ifndef DEDICATED
        m_isDirty = true; m_isPlaying = false; Mix_HaltChannel(m_ChannelID);
#endif
    };

    // Removes any effects on the channel, should it get picked more or less at random
    // to play a sample.  Intended to be called from an SDL_mixer callback.
    void UnplaySound();

    Uint32 StartTime() { return m_StartTime; };
    bool IsContinuous() { return m_continuous; };

    // Call this whenever the object needs to be updated.
    void Update();

    // Call to see if the object needs to be updated
    bool isDirty() { return m_isDirty; };

    // Call to see if it's busy right now
    bool isBusy() { return m_isPlaying; };

    void SetOwner(eGameObject* owner);
    void DelayStarting() { m_Delayed = true; m_StartNow = false; };
    bool IsDelayed() { return m_Delayed; };
    void Undelay() { m_StartNow = true; };
    void SetHome(eGameObject* home) { m_Home = home; };
    eGameObject* GetOwner() { return m_Owner; };

    // A cop out, I should just iterate through all the channels and initialize it
    // properly, so they're guaranteed to be in sync, but this should work, as long
    // as the list of channels only gets built once.
    static int numChannels;

private:
    eWavData m_Sound;
    bool m_Delayed;
    bool m_StartNow;
    int m_Volume;
    int m_ChannelID;
    Uint32 m_StartTime;
    bool m_isDirty;
    bool m_isPlaying;
    eGameObject* m_Owner;
    eGameObject* m_Home;
    bool m_continuous;
};

#endif
