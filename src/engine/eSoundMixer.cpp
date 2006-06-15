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

eSoundMixer by Dave Fancella

*/

#include "config.h"

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
#include "sdl_mixer/eMusicTrackSDLMixer.h"
#include "sdl_mixer/eChannelSDLMixer.h"

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

#ifdef WIN32
//static int buffer_shift=1;
#else
//static int buffer_shift=0;
#endif

// MusicModes mapped to strings
// These need to appear in the exact order shown in the MusicMode enum!

char* MusicModeString[] = {
                              "Title track",
                              "GUI track",
                              "In-game track"
                          };

// Sound effects mapped to strings.  These need to appear in the exact order shown
//   in the SoundEffect enum!

char* SoundEffectString[] = {
                                "Cycle turn",
                                "Cycle explosion",
                                "Announcer says 1",
                                "Announcer says 2",
                                "Announcer says 3",
                                "Announcer says GO",
                                "New Match effect",
                                "New Round effect",
                                "Round Winner effect",
                                "Match Winner effect",
                                "Zone Spawn effect",
                                "Cycle motor effect",
                                "Cycle grinding a wall effect"
                            };

bool eSoundMixer::m_musicIsPlaying = false;

int sound_quality=SOUND_MED;
static tConfItem<int> sq("SOUND_QUALITY",sound_quality);

int musicVolume=64;
static tConfItem<int> sw("MUSIC_VOLUME",musicVolume);

int numSoundcardChannels=2;
static tConfItem<int> scc("SOUND_CHANNELS",numSoundcardChannels);

int musicActive = 1;
static tConfItem<int> se("MUSIC_ACTIVE", musicActive);

float buffersize = 0.5;
static tConfItem<float> sbs("SOUND_BUFFER_SIZE", buffersize);

tString titleTrack("music/titletrack.ogg");
static tConfItemLine stt("TITLE_TRACK", titleTrack);

tString guiTrack("music/when.ogg");
static tConfItemLine ste("GUI_TRACK", guiTrack);

tString playListFile("music/default.m3u");
static tConfItemLine stp("MUSIC_PLAYLIST", playListFile);

static tString customPlaylist("");
static tConfItemLine cpl("CUSTOM_MUSIC_PLAYLIST", customPlaylist);

static int usePlaylist = 0;
static tConfItem<int> upl("USE_CUSTOM_PLAYLIST", usePlaylist);

/* Configurable sound sources don't make sense for this, but here it is
     in case anyone figures out how to make it make sense
static int sound_sources=10;
static tConfItem<int> ss("SOUND_SOURCES",sound_sources);
*/

/*******************************************************************************
 *
 * eSoundMixer
 *
 *******************************************************************************/

int eSoundMixer::m_Mode = 0;
bool eSoundMixer::m_isDirty = true;
eMusicTrack* eSoundMixer::m_TitleTrack = 0;
eMusicTrack* eSoundMixer::m_GuiTrack = 0;
eMusicTrack* eSoundMixer::m_GameTrack = 0;

eSoundMixer::eSoundMixer()
{
#ifdef HAVE_LIBSDL_MIXER
    m_Owner = NULL;
    m_Playlist = NULL;

    if(musicActive == 1) {
        m_PlayMusic = true;
    } else {
        m_PlayMusic = false;
    }
#endif // DEDICATED
}

void eSoundMixer::SetMicrophoneOwner(eGameObject* newOwner) {
#ifdef HAVE_LIBSDL_MIXER
    //std::cout << "Setting new microphone owner\n";
    m_Owner = newOwner;
#endif // DEDICATED
}

void eSoundMixer::__channelFinished(int channel) {
#ifdef HAVE_LIBSDL_MIXER
    m_Channels[channel].UnplaySound();
#endif // DEDICATED
}

void eSoundMixer::ChannelFinished(int channel) {
#ifdef HAVE_LIBSDL_MIXER
    _instance->__channelFinished(channel);
#endif // DEDICATED
}

int eSoundMixer::FirstAvailableChannel() {
#ifdef HAVE_LIBSDL_MIXER
    Uint32 oldesttime=0;
    int oldestchannel=-1;

    for( int i=0; i < m_numChannels; i++) {
        if( !m_Channels[i].isBusy() ) return i;
        else {
            if( m_Channels[i].StartTime() < oldesttime && !m_Channels[i].IsContinuous() ) {
                oldesttime = m_Channels[i].StartTime();
                oldestchannel = i;
            }
        }
    }
    //std::cout << "Couldn't get an available channel!\n";

    return oldestchannel;
    // TODO: error handling
    tASSERT( 0 );
#endif // DEDICATED
    return -1;
}

// wraps call to SDL_InitSubSystem, setting the audio driver to a predefined value
#ifdef HAVE_LIBSDL_MIXER
static int se_Wrap_SDL_InitSubSystem()
{
#ifndef DEDICATED
#ifdef DEFAULT_SDL_AUDIODRIVER

    // stringification, yep, two levels required
#define XSTRING(s) #s
#define STRING(s) XSTRING(s)

    // initialize audio subsystem with predefined, hopefully good, driver
    if ( ! getenv("SDL_AUDIODRIVER") ) {
        char * arg = "SDL_AUDIODRIVER=" STRING(DEFAULT_SDL_AUDIODRIVER);
        putenv(arg);

        int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
        if ( ret >= 0 )
            return ret;

        putenv("SDL_AUDIODRIVER=");
    }
#endif
#endif

    // if that fails, try what the user wanted
    return SDL_InitSubSystem(SDL_INIT_AUDIO);
}
#endif

void eSoundMixer::Init() {
#ifdef HAVE_LIBSDL_MIXER
    bool initbase = false;

    m_active = false;

    const tPath& vpath = tDirectories::Data();

    // We haven't started playing any music yet
    m_musicIsPlaying = false;
    m_active = true;

    tString musFile;

    musFile = vpath.GetReadPath( titleTrack );
    std::cout << titleTrack << "\n";
    m_TitleTrack = new eMusicTrack(musFile, true);
    musFile = vpath.GetReadPath( guiTrack );
    std::cout << guiTrack << "\n";
    m_GuiTrack = new eMusicTrack(musFile, true);

    //m_GuiTrack->Loop();

    // Don't load a file for this, we don't know what we're playing until it's time
    // to play.
    m_GameTrack = new eMusicTrack();

    LoadPlaylist();

    if(!SDL_WasInit( SDL_INIT_AUDIO )) {
        int rc;
        rc = se_Wrap_SDL_InitSubSystem();
        if ( rc < 0 ) {
            //std::cerr << "Couldn't initialize audio, disabling.  I'm very sorry about that.\n";
            initbase = false;
            // todo: disable audio if we can't initialize it
        } else {
            initbase = true;
        }
    } else {
        initbase = true;
    }

    if( !initbase ) {
        return;
    }

    int rc;
    int frequency;

    switch (sound_quality)
    {
    case SOUND_LOW:
        frequency=11025; break;
    case SOUND_MED:
        frequency=22050; break;
    case SOUND_HIGH:
        frequency=44100; break;
    default:
        frequency=22050;
    }

    // guesstimate the desired number of samples to calculate in advance
    int samples = static_cast< int >( buffersize * 512 );

    rc = Mix_OpenAudio( frequency, AUDIO_S16LSB,
                        numSoundcardChannels, samples );

    if(rc==0) {
        // don't know what to do here
        int a,c;
        Uint16 b;
        Mix_QuerySpec(&a,&b,&c);
        //std::cout << "SDL_Mixer initialized with " << c << " channels.\n";
    } else {
        //std::cout << "Couldn't initialize SDL_Mixer, disabling sound.  I'm very sorry about that, I'll try to do better next time.\n";
        return;
    }

    // Register music finished callback
    Mix_VolumeMusic( musicVolume );
    Mix_HookMusicFinished( &eSoundMixer::SDLMusicFinished );

    // Now we're done with music and initializing sdl_mixer, we'll setup the sound
    // effect stuff
    //musFile = vpath.GetReadPath( titleTrack );

    m_SoundEffects.resize(15);
    m_SoundEffects[CYCLE_TURN].LoadWavFile(vpath.GetReadPath("sound/cycle_turn.ogg") );
    m_SoundEffects[CYCLE_TURN].SetVolume(60);
    m_SoundEffects[CYCLE_EXPLOSION].LoadWavFile(vpath.GetReadPath( "sound/expl.ogg" ));
    m_SoundEffects[ANNOUNCER_1].LoadWavFile(vpath.GetReadPath("sound/1voicemale.ogg") );
    m_SoundEffects[ANNOUNCER_2].LoadWavFile(vpath.GetReadPath("sound/2voicemale.ogg") );
    m_SoundEffects[ANNOUNCER_3].LoadWavFile(vpath.GetReadPath("sound/3voicemale.ogg") );
    m_SoundEffects[ANNOUNCER_GO].LoadWavFile(vpath.GetReadPath("sound/announcerGO.ogg"));
    //m_SoundEffects[NEW_MATCH].LoadWavFile(vpath.GetReadPath("sound/none.ogg") );
    //m_SoundEffects[NEW_ROUND].LoadWavFile(vpath.GetReadPath("sound/none.ogg") );
    //m_SoundEffects[ROUND_WINNER].LoadWavFile(vpath.GetReadPath("sound/none.ogg") );
    //m_SoundEffects[MATCH_WINNER].LoadWavFile(vpath.GetReadPath("sound/none.ogg") );
    m_SoundEffects[ZONE_SPAWN].LoadWavFile(vpath.GetReadPath("sound/zone_spawn.ogg") );
    m_SoundEffects[CYCLE_MOTOR].LoadWavFile(vpath.GetReadPath("sound/cyclrun.ogg") );
    m_SoundEffects[CYCLE_GRIND_WALL].LoadWavFile(vpath.GetReadPath("sound/grind.ogg") );

    Mix_Volume(-1, 50);
    Mix_VolumeMusic(100);

    m_numChannels = Mix_AllocateChannels(40);
    m_Channels.resize( m_numChannels );
    for(int i=0; i < m_numChannels; i++) {
        m_Channels[i].SetId(i);
    }
    Mix_ChannelFinished( &eSoundMixer::ChannelFinished );
#endif
}

eSoundMixer::~eSoundMixer() {
    // do nothing destructor
}

void eSoundMixer::LoadPlaylist() {
#ifdef HAVE_LIBSDL_MIXER
    if(!m_GameTrack) return;

    if(usePlaylist == 1) {
        m_GameTrack->SetPlaylist(usePlaylist);
        m_GameTrack->LoadPlaylist(customPlaylist);
    } else {
        const tPath& vpath = tDirectories::Data();
        m_GameTrack->SetPlaylist(usePlaylist);
        m_GameTrack->LoadPlaylist(vpath.GetReadPath(playListFile) );
    }
#endif
}

// SetMode shouldn't call any SDL_mixer functions, it will be called from SDL_mixer's
//   callbacks!
void eSoundMixer::SetMode(MusicMode newMode) {
#ifdef HAVE_LIBSDL_MIXER
    m_Mode = newMode;
    m_isDirty = true;
#endif // DEDICATED
}

void eSoundMixer::SongFinished() {
#ifdef HAVE_LIBSDL_MIXER
    // Music stopped
    m_musicIsPlaying = false;

    switch(m_Mode) {
    case TITLE_TRACK:
        // When the title track stops, we switch to the gui track
        SetMode(GUI_TRACK);
        break;
    case GUI_TRACK:
        m_isDirty = true;
        break;
    case GRID_TRACK:
        m_isDirty = true;
        break;
    default:
        break;
    }
#endif // DEDICATED

}

void eSoundMixer::SDLMusicFinished() {
#ifdef HAVE_LIBSDL_MIXER
    // We're guaranteed to have this, so we'll just use it
    m_TitleTrack->currentMusic->MusicFinished();
#endif
}

void eSoundMixer::PushButton( int soundEffect ) {
#ifdef HAVE_LIBSDL_MIXER
    if (!m_active) return;

    int theChannel = FirstAvailableChannel();
    if( theChannel < 0) return;
    m_Channels[ theChannel ].PlaySound(m_SoundEffects[soundEffect]);
#endif // DEDICATED
}

void eSoundMixer::PushButton( int soundEffect, eCoord location ) {
#ifdef HAVE_LIBSDL_MIXER
    if (!m_active) return;

    // If we don't have an owner yet, just call regular PushButton
    if(!m_Owner) return;

    int theChannel = FirstAvailableChannel();

    if( theChannel < 0) return;
    m_Channels[theChannel].Set3d(m_Owner->Position(), location, m_Owner->Direction());
    m_Channels[theChannel].PlaySound(m_SoundEffects[soundEffect]);
#endif // DEDICATED
}

void eSoundMixer::PlayContinuous(int soundEffect, eGameObject* owner) {
#ifdef HAVE_LIBSDL_MIXER
    std::cout << "Playcontinuous: " << SoundEffectString[soundEffect] << "\n";
    if (!m_active) return;
    return;

    int theChannel = FirstAvailableChannel();
    if( theChannel < 0 || m_Channels[theChannel].isBusy()) {
        std::cout << "Can't loop, sorry.\n";
        return;
    }

    m_Channels[theChannel].SetOwner(owner);
    // If we don't have an owner yet, we need to delay starting
    if(!m_Owner) m_Channels[theChannel].DelayStarting();
    m_Channels[theChannel].SetHome(m_Owner);
    m_Channels[theChannel].LoopSound(m_SoundEffects[soundEffect]);
#endif
}

void eSoundMixer::RemoveContinuous(int soundEffect, eGameObject* owner) {
#ifdef HAVE_LIBSDL_MIXER
    for(std::deque<eChannel>::iterator i = m_Channels.begin(); i != m_Channels.end(); ++i) {
        if ((*i).GetOwner() == owner) {
            (*i).StopSound();
            std::cout << "removed continuous sound\n";
        }
    }
#endif
}

void eSoundMixer::Update() {
#ifdef HAVE_LIBSDL_MIXER
    if (!m_active) return;
    // Stubbed my french fry

    // First we'll update all the channels
    for(int i=0; i<eChannel::numChannels; i++) {
        if( m_Channels[i].IsDelayed() ) {
            if( m_Owner ) {
                m_Channels[i].SetHome(m_Owner);
                m_Channels[i].Undelay();
            }
        }
        m_Channels[i].Update();
    }

    // We only act on music if the mode has changed, indicated by m_isDirty
    if (m_isDirty) {
        // Also, we only update if the music has stopped for some reason.
        if(!m_musicIsPlaying) {

            switch(m_Mode) {
            case TITLE_TRACK:
                m_TitleTrack->Play();
                m_Mode = TITLE_TRACK;
                m_isDirty = false;
                break;
            case GUI_TRACK:
                m_GuiTrack->Play();
                m_isDirty = false;
                break;
            case GRID_TRACK:
                if(m_GuiTrack->IsPlaying()) {
                    m_GuiTrack->FadeOut();
                    m_GameTrack->SetDirty();
                    m_GameTrack->Update();
                } else if(m_TitleTrack->IsPlaying()) {
                    m_TitleTrack->FadeOut();
                    m_GameTrack->SetDirty();
                    m_GameTrack->Update();
                } else {
                    m_GameTrack->Next();
                    m_GameTrack->Play();
                }
                m_isDirty = false;
                break;
            default:
                break;
            }
        }
    }
    // Go ahead and update the music track now.
    if ( m_GuiTrack && m_GuiTrack->currentMusic )
        m_GuiTrack->currentMusic->Update();
#endif // DEDICATED
}

tString eSoundMixer::GetCurrentSong() {
    if(m_GameTrack != NULL)
        if(m_GameTrack->currentMusic != NULL) {
            tString const &str = m_GameTrack->currentMusic->GetFileName();
#ifndef WIN32
            size_t pos = str.find_last_of('/');
#else
            size_t pos = str.find_last_of('\\');
#endif
            if(pos == tString::npos) return str;
            if(pos == str.size()) return tString();
            return str.SubStr(pos + 1);
        }
    return tString(" ");
}

eSoundMixer* eSoundMixer::_instance = 0;

void eSoundMixer::ShutDown() {
#ifdef HAVE_LIBSDL_MIXER
    Mix_CloseAudio();
    delete _instance;
    delete m_TitleTrack;
    delete m_GuiTrack;
    if(m_GameTrack) delete m_GameTrack;

    // if (SDL_WasInit( SDL_INIT_AUDIO ))
    SDL_QuitSubSystem( SDL_INIT_AUDIO );

#endif // DEDICATED
}

eSoundMixer* eSoundMixer::GetMixer() {
#ifdef HAVE_LIBSDL_MIXER
    if(_instance == 0) {
        _instance = new eSoundMixer();
        _instance->Init();
    }

#endif // DEDICATED
    return _instance;
}

#ifdef HAVE_LIBSDL_MIXER
// Play this every frame
static void updateMixer() {
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->Update();
}

static rPerFrameTask mixupdate(&updateMixer);

// ***************************************************************
// Sound Menu

#include "uMenu.h"

uMenu Sound_menu("$sound_menu_text");

static uMenuItemInt sources_men
(&Sound_menu,"$sound_channels",
 "$sound_channels_help",
 numSoundcardChannels,2,6,2);

static uMenuItemSelection<int> sq_men
(&Sound_menu,"$sound_menu_quality_text",
 "$sound_menu_quality_help",
 sound_quality);

static uSelectEntry<int> a(sq_men,
                           "$sound_menu_quality_off_text",
                           "$sound_menu_quality_off_help",
                           SOUND_OFF);

static uSelectEntry<int> b(sq_men,
                           "$sound_menu_quality_low_text",
                           "$sound_menu_quality_low_help",
                           SOUND_LOW);

static uSelectEntry<int> c(sq_men,
                           "$sound_menu_quality_medium_text",
                           "$sound_menu_quality_medium_help",
                           SOUND_MED);
static uSelectEntry<int> d(sq_men,
                           "$sound_menu_quality_high_text",
                           "$sound_menu_quality_high_help",
                           SOUND_HIGH);

static uMenuItemSelection<int> bm_men
(&Sound_menu,
 "$sound_playlist_text",
 "$sound_playlist_help",
 usePlaylist);

static uSelectEntry<int> ba(bm_men,
                            "$sound_playlistinternal_text",
                            "$sound_playlistinternal_help",
                            0);

static uSelectEntry<int> bb(bm_men,
                            "$sound_playlistcustom_text",
                            "$sound_playlistcustom_help",
                            1);

static uMenuItemString bc(&Sound_menu, "$sound_customplaylist_text",
                          "$sound_customplaylist_help",
                          customPlaylist, 255);

void se_SoundMenu(){
    int oldUsePlaylist = usePlaylist;
    tString oldCustomPlaylist = customPlaylist;
    eSoundMixer* mixer = eSoundMixer::GetMixer();

    Sound_menu.Enter();
    if( oldUsePlaylist != usePlaylist || oldCustomPlaylist != customPlaylist) mixer->LoadPlaylist();
}

// Media player keybinds
#include "uInput.h"

static uActionGlobal mediaPlay("MUSIC_PLAY");
static uActionGlobal mediaNext("MUSIC_NEXT");
static uActionGlobal mediaPrev("MUSIC_PREVIOUS");
static uActionGlobal mediaStop("MUSIC_STOP");
static uActionGlobal mediaPause("MUSIC_PAUSE");
static uActionGlobal mediaVolumeUp("MUSIC_VOLUME_UP");
static uActionGlobal mediaVolumeDown("MUSIC_VOLUME_DOWN");

bool eSoundMixer::Music_play(REAL x=1){
    if (x>0){
        switch(m_Mode) {
        case GRID_TRACK:
            m_GameTrack->Play();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_stop(REAL x=1){
    if (x>0){
        switch(m_Mode) {
        case GRID_TRACK:
            m_GameTrack->Stop();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_pause(REAL x=1){
    if (x>0){
        switch(m_Mode) {
        case GUI_TRACK:
            m_GuiTrack->Pause();
            break;
        case GRID_TRACK:
            m_GameTrack->Pause();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_volume_up(REAL x=1){
    if (x>0){
        switch(m_Mode) {
        case TITLE_TRACK:
            m_TitleTrack->VolumeUp();
            break;
        case GUI_TRACK:
            m_GuiTrack->VolumeUp();
            break;
        case GRID_TRACK:
            m_GameTrack->VolumeUp();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_volume_down(REAL x=1){
    if (x>0){
        switch(m_Mode) {
        case TITLE_TRACK:
            m_TitleTrack->VolumeDown();
            break;
        case GUI_TRACK:
            m_GuiTrack->VolumeDown();
            break;
        case GRID_TRACK:
            m_GameTrack->VolumeDown();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_next_song(REAL x=1) {
    if (x>0){
        switch(m_Mode) {
        case GRID_TRACK:
            m_GameTrack->Next();
            break;
        default:
            break;
        }
    }

    return true;
}

bool eSoundMixer::Music_previous_song(REAL x=1) {
    if (x>0){
        switch(m_Mode) {
        case GRID_TRACK:
            m_GameTrack->Previous();
            break;
        default:
            break;
        }
    }

    return true;
}

static uActionGlobalFunc mediapl(&mediaPlay,&eSoundMixer::Music_play, true );
static uActionGlobalFunc medianext(&mediaNext,&eSoundMixer::Music_next_song, true );
static uActionGlobalFunc mediaprev(&mediaPrev,&eSoundMixer::Music_previous_song, true );
static uActionGlobalFunc mediast(&mediaStop,&eSoundMixer::Music_stop, true );
static uActionGlobalFunc mediapa(&mediaPause,&eSoundMixer::Music_pause, true );
static uActionGlobalFunc mediavu(&mediaVolumeUp,&eSoundMixer::Music_volume_up, true );
static uActionGlobalFunc mediavd(&mediaVolumeDown,&eSoundMixer::Music_volume_down, true );
#else

void se_SoundMenu(){
    // Empty function
}


#endif // DEDICATED


