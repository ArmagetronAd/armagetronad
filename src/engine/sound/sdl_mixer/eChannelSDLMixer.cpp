
#include "eChannelSDLMixer.h"

// I don't know how many of these includes are actually necessary
#include "rSDL.h"
#include "rScreen.h"
#include "defs.h"
// #include "eGrid.h"
#include "tString.h"
#include "tLinkedList.h"
#include "tDirectories.h"
#include "tConfiguration.h"
#include "tConsole.h"
#include "tError.h"
#include "eGameObject.h"
#include "eCoord.h"

#include "eSoundMixer.h"

// Possibly temporary?
#include <math.h>

// sound quality
#define SOUND_OFF 0
#define SOUND_LOW 1
#define SOUND_MED 2
#define SOUND_HIGH 3

// Playlist options
#define PLAYLIST_INTERNAL 0
#define PLAYLIST_CUSTOM 1
/*******************************************************************************
 *
 * eWavData
 *
 *******************************************************************************/

eWavData::eWavData() :
        m_Volume(64), m_Playable(false)
{
#ifdef HAVE_LIBSDL_MIXER
    // Do nothing constructor
    m_WavData = NULL;
#endif // DEDICATED
}

eWavData::~eWavData() {
#ifdef HAVE_LIBSDL_MIXER
    if(m_WavData != NULL) {
        Mix_FreeChunk(m_WavData);
    }
#endif // DEDICATED
}

eWavData::eWavData(const char* filename) {
#ifdef HAVE_LIBSDL_MIXER
    // Construct a new wavdata by loading a file
#endif // DEDICATED
}

void eWavData::LoadWavFile(const char* filename) {
#ifdef HAVE_LIBSDL_MIXER
#ifdef MACOSX
    // BUG: This call is very very slow on my system. Disable it until I figure out what is wrong. -- Dan
    return;
#endif

    m_WavData = Mix_LoadWAV(filename);

    if(!m_WavData) {
        tERR_WARN("Couldn't load wav file\n");
    } else {
        //std::cout << "Successfully loaded sound effect: " << filename << "\n";
    }
#endif // DEDICATED
}

/*******************************************************************************
 *
 * eChannel
 *
 *******************************************************************************/

int eChannel::numChannels = 0;

eChannel::eChannel() : m_Delayed(false) {
#ifdef HAVE_LIBSDL_MIXER
    m_ChannelID = numChannels;
    numChannels++;
    m_Volume = 50;
    m_isDirty = false;
    m_isPlaying = false;
    m_continuous = false;
#endif // DEDICATED
}

void eChannel::PlaySound(eWavData& sound) {
#ifdef HAVE_LIBSDL_MIXER
    if(m_isPlaying) {
        Mix_HaltChannel(m_ChannelID);
        //std::cerr << "Channel occupied, halting: " << m_ChannelID << "\n";
    }
    int theChannel;
    Mix_Volume(m_ChannelID, sound.GetVolume());
    theChannel = Mix_PlayChannel(m_ChannelID, sound.GetWavData(), 0);

    if(theChannel != m_ChannelID) {
        // do nothing, but there's an error
        //std::cerr << "Couldn't play a sound: " << m_ChannelID << "\n";
    } else {
        m_isPlaying = true;
        m_StartTime = SDL_GetTicks();
    }
#endif // DEDICATED
}

void eChannel::Set3d(eCoord home, eCoord soundPos, eCoord homeDirection) {
#ifdef HAVE_LIBSDL_MIXER
    // the sound position relative to the microphone
    eCoord soundPosRelative = (soundPos - home).Turn( homeDirection.Conj() );

    // the distance to the microphone
    REAL distance = soundPosRelative.Norm();

    // the bearing in radians (taken from the inverse of the direction to put the discontinuous jump in the right spot)
    double bearing = atan2( -soundPosRelative.x, -soundPosRelative.y );

    // We get it in radians and need to convert to degrees because SDL_mixer is lame.
    // atan2's return value is specified to be between -PI and PI,
    // so we need to linearily transform the result.
    int bearingInt = 180 + (int) (bearing * 180.0/M_PI);

    // and the distance needs to be an integer, too. The conversion
    // could use some tweaking/customization.
    const REAL microphoneSize = 3;
    int distanceInt = 255 - int( 255 * microphoneSize / ( sqrt(distance) + microphoneSize ) );
    // clamp. Distance == 0 seems to be dangerous :(
    distanceInt = distanceInt > 255 ? 255 : distanceInt;
    distanceInt = distanceInt < 1 ? 1 : distanceInt;

    // override bearing for close sounds, the value of atan2 is undefined then
    if ( distance < EPS * microphoneSize )
        bearingInt = 0;

    // Remove the old effect, if present
    Mix_SetPosition(m_ChannelID, 0, 0);
    // Add the new effect
    Mix_SetPosition(m_ChannelID, bearingInt, distanceInt );
#endif // DEDICATED
}

void eChannel::Update() {
#ifdef HAVE_LIBSDL_MIXER
    if(m_isDirty && !m_isPlaying) {
        Mix_UnregisterAllEffects(m_ChannelID);
        m_isDirty = false;
    }

    if(m_continuous) {
        if(m_Delayed) {
            if(m_StartNow) {
                m_StartNow = false;
                m_Delayed = false;
                LoopSound(m_Sound);
                Set3d(m_Home->Position(), m_Owner->Position(), m_Home->Direction());
                std::cout << "Starting delayed effect\n";
            }
        } else {
            if(!m_Home || !m_Owner) {
                StopSound();
                return;
            }
            Set3d(m_Home->Position(), m_Owner->Position(), m_Home->Direction());
        }
    }
#endif // DEDICATED
}

void eChannel::UnplaySound() {
#ifdef HAVE_LIBSDL_MIXER
    m_isDirty = true;
    m_isPlaying = false;
#endif // DEDICATED
}

void eChannel::SetOwner(eGameObject* owner) {
#ifdef HAVE_LIBSDL_MIXER
    m_Owner = owner;
#endif
}

void eChannel::LoopSound(eWavData& sound) {
#ifdef HAVE_LIBSDL_MIXER
    //std::cout << "Looping sound\n";
    if(m_isPlaying) {
        Mix_HaltChannel(m_ChannelID);
        //std::cerr << "Channel occupied, halting: " << m_ChannelID << "\n";
    }
    m_continuous = true;
    m_Sound = sound;

    if(m_Delayed) {
        return;
    }
    int theChannel;
    Mix_Volume(m_ChannelID, sound.GetVolume());
    theChannel = Mix_PlayChannel(m_ChannelID, m_Sound.GetWavData(), -1);

    //std::cout << "Looping sound\n";
    if(theChannel < 0) {
        // do nothing, but there's an error
        //std::cerr << "Couldn't play a sound on channel: " << m_ChannelID << "\n";
    } else {
        m_isPlaying = true;
    }
#endif
}

