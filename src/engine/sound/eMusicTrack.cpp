#include "eMusicTrack.h"

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
#include <iostream>

#include "eSoundMixer.h"
#include "tIniFile.h"

// Possibly temporary?
#include <math.h>

// sound quality
#define SOUND_OFF 0
#define SOUND_LOW 1
#define SOUND_MED 2
#define SOUND_HIGH 3

/*******************************************************************************
 *
 * eMusicTrack
 *
 *******************************************************************************/

bool eMusicTrack::musicIsPlaying = 0;
eMusicTrack* eMusicTrack::currentMusic = 0;

eMusicTrack::eMusicTrack() {
#ifdef HAVE_LIBSDL_MIXER
    // Do nothing constructor
    if(! currentMusic) currentMusic = this;
    Init(true);
#endif // DEDICATED
}

eMusicTrack::eMusicTrack(const char* filename, bool isinstalled) {
#ifdef HAVE_LIBSDL_MIXER
    Init(isinstalled);
    // Load file
    tString musFile = tString(filename);
    m_CurrentSong = tSong(musFile);
    LoadSong( m_CurrentSong );
#endif // DEDICATED
}

void eMusicTrack::Init(bool isinstalled) {
#ifdef HAVE_LIBSDL_MIXER
    m_IsInstalled = isinstalled;
    m_Music = NULL;
    m_Playlist = tNEW(tPlayList)();
    m_Loop = false;
    m_Volume = 100;
    m_HasSong = false;
    m_mixer = eSoundMixer::GetMixer();
    m_SequencePos = m_Tracklist.begin();
#endif
}

eMusicTrack::~eMusicTrack() {
#ifdef HAVE_LIBSDL_MIXER
    // Do nothing destructor
    std::map<tString, Mix_Music*>::iterator trackIter;

    for(trackIter = m_Tracklist.begin(); trackIter != m_Tracklist.end(); trackIter++) {
        Mix_FreeMusic( (*trackIter).second );
    }

    delete m_Playlist;
#endif // DEDICATED
}

void eMusicTrack::UnloadSong() {
#ifdef HAVE_LIBSDL_MIXER
    if(m_Music) Mix_FreeMusic(m_Music);
    m_Pos = 0.0;
    m_StartTime = 0;
    m_HasSong = false;
#endif
}

bool eMusicTrack::LoadSong(tSong thesong) {
#ifdef HAVE_LIBSDL_MIXER
    tString extension;
    m_Filename = thesong.location;

    if(m_Music) Mix_FreeMusic(m_Music);

    extension = thesong.location.GetFileMimeExtension();
    if(extension == ".aatrack") {
        LoadAATrack(thesong);
    } else {
        LoadVorbisTrack(thesong);
    }
    std::map<tString, tString>::iterator trackIter;

    // const tPath& vpath = tDirectories::Data();
    tString basepath("music/");
    // Now load the tracks
    for(trackIter = thesong.tracklist.begin(); trackIter != thesong.tracklist.end(); trackIter++) {
        //if(m_IsInstalled) {
        //    tString tmpFile = (*trackIter).second;
        //    basepath = tString("music/") + tmpFile;
        //    basepath = vpath.GetReadPath(basepath);
        //} else {
        basepath = (*trackIter).second;
        //}
        m_Tracklist[(*trackIter).first] = LoadFile(basepath);
    }
    m_HasSong = true;
#endif // DEDICATED
    return true;
}

void eMusicTrack::LoadAATrack(tSong& thesong) {
#ifdef HAVE_LIBSDL_MIXER
    tIniFile theFile(thesong.location);
    //theFile.Dump();

    thesong.author = theFile.GetValue("header", "author");
    thesong.year = theFile.GetValue("header", "year");
    thesong.record = theFile.GetValue("header", "record");
    thesong.title = theFile.GetValue("header", "title");
    thesong.genre = theFile.GetValue("header", "genre");

    tString isInstalledS = theFile.GetValue("header", "installed");
    if(isInstalledS == "yes") m_IsInstalled = true;
    else m_IsInstalled = false;

    std::cout << thesong.title << ", by " << thesong.author << "\n";
    thesong.tracklist = theFile.GetGroup("tracks");
    thesong.sequence = theFile.GetGroup("sequence");
#endif
}

// Also loads mp3 and other tracks supported by sdl_mixer
void eMusicTrack::LoadVorbisTrack(tSong& thesong) {
#ifdef HAVE_LIBSDL_MIXER
    // We set these so the same code can use tSong to play a song, but this track
    // is a single song, probably from the user's own playlist
    thesong.author = "Unknown";
    thesong.year = "Unknown";
    thesong.record = "Unknown";
    thesong.title = "Unknown";
    thesong.genre = "Unknown";

    thesong.tracklist[tString("only")] = thesong.location;
    thesong.sequence[tString("0")] = "only";
    std::cout << "Track: " << thesong.location << "\n";
#endif
}

Mix_Music* eMusicTrack::LoadFile(const char* filename) {
    Mix_Music* theMusic = NULL;
#ifdef HAVE_LIBSDL_MIXER

    theMusic = Mix_LoadMUS( filename );

    if(!theMusic) {
        tERR_WARN("Failed to load track\n");
    }
#endif // DEDICATED
    return theMusic;
}

void eMusicTrack::MusicFinished() {
#ifdef HAVE_LIBSDL_MIXER
    m_SequencePos++;

    if(m_SequencePos == m_Tracklist.end()) {
        m_mixer->SongFinished();
        return;
    }
    m_SequenceChange = true;
    std::cout << "Sequence change\n";
#endif
}

bool eMusicTrack::Play() {
#ifdef HAVE_LIBSDL_MIXER
    //if(!m_HasSong) return false;

    // Fade out the song, if there's already one playing
    if(Mix_PlayingMusic() && !m_IsPlaying) {
        currentMusic->FadeOut();
    }

    m_SequencePos = m_Tracklist.begin();

    if(m_SequencePos == m_Tracklist.end()) {
        std::cout << "For some reason, this song has no tracks in the tracklist\n";
        musicIsPlaying = false;
        return false;
    } else {
        std::cout << "Should be playing the first sequence\n";
        m_HasSong = true;
        PlayCurrentSequence();
    }

#endif
    return musicIsPlaying;
}

void eMusicTrack::PlayCurrentSequence() {
#ifdef HAVE_LIBSDL_MIXER
    int pmres;
    //if(!m_HasSong) return;
    pmres = Mix_PlayMusic((*m_SequencePos).second, 1);

    if( pmres == 0) {
        Mix_VolumeMusic(m_Volume);
        musicIsPlaying = true;
        m_IsPlaying = true;
        currentMusic = this;
        std::cout << "Playing the sequence\n";
    } else {
        std::cout << "Didn't play the sequence " << (*m_SequencePos).first << "\n";
        musicIsPlaying = false;
        m_HasSong = false;
    }
#endif // DEDICATED
}

void eMusicTrack::Previous() {
#ifdef HAVE_LIBSDL_MIXER
    tSong newSong;
    tString musFile;
    // const tPath& vpath = tDirectories::Data();
    int numTries = 0;
    bool foundSong = false;

    if(! m_Playlist->empty() ) {
        do {
            numTries++;
            m_CurrentSong = m_Playlist->GetPreviousSong();
            foundSong = LoadSong( m_CurrentSong );
        } while(!foundSong && numTries < 5);
        if(foundSong) {
            m_Pos = 0.0;

            Play();
            m_isDirty = false;
        }
    }
#endif
}

void eMusicTrack::Next() {
#ifdef HAVE_LIBSDL_MIXER
    tSong newSong;
    tString musFile;
    int numTries = 0;
    bool foundSong = false;

    if(! m_Playlist->empty() ) {
        do {
            numTries++;
            m_CurrentSong = m_Playlist->GetNextSong();
            foundSong = LoadSong( m_CurrentSong );
        } while(!foundSong && numTries < 5);
        if(foundSong) {
            m_Pos = 0.0;
            m_HasSong = true;
            Play();
            m_isDirty = false;
        }
    }
#endif
}

void eMusicTrack::Pause() {
#ifdef HAVE_LIBSDL_MIXER
    if(Mix_PausedMusic()==0) {
        Mix_PauseMusic();
    } else {
        Mix_ResumeMusic();
    }
#endif
}

void eMusicTrack::VolumeUp() {
#ifdef HAVE_LIBSDL_MIXER
    m_Volume += 5;
    if(m_Volume > MIX_MAX_VOLUME) m_Volume = MIX_MAX_VOLUME;
    Mix_VolumeMusic(m_Volume);
#endif
}

void eMusicTrack::VolumeDown() {
#ifdef HAVE_LIBSDL_MIXER
    m_Volume -= 5;
    if(m_Volume < 0) m_Volume = 0;
    Mix_VolumeMusic(m_Volume);
#endif
}

void eMusicTrack::FadeOut() {
#ifdef HAVE_LIBSDL_MIXER
    if(m_IsPlaying) {
        if(Mix_PlayingMusic()) {
            Mix_FadeOutMusic(200);
        }
    }
    m_Pos = (double) (SDL_GetTicks() - m_StartTime) / 1000.0;
    //std::cout << "Fading out music, pos: " << m_Pos << "\n";
#endif // DEDICATED
}

void eMusicTrack::LoadPlaylist(const char* filename) {
#ifdef HAVE_LIBSDL_MIXER
    if(m_Playlist != NULL) delete m_Playlist;

    m_Playlist = new tPlayList();
    m_Playlist->SetPlaylist(usePlaylist);
    if ( strlen(filename) > 0 )
        m_Playlist->LoadPlaylist(filename);

    if( !m_Playlist->empty()) m_Playlist->Randomize();
#endif
}

void eMusicTrack::SetDirty() {
#ifdef HAVE_LIBSDL_MIXER
    m_isDirty = true;
#endif
}

void eMusicTrack::Update() {
#ifdef HAVE_LIBSDL_MIXER
    if(m_HasSong) {
        // First check to see if the sequence position has changed.  If so, start the
        // new track for it.
        if(m_SequenceChange==true) {
            PlayCurrentSequence();
            m_SequenceChange = false;
            std::cout << "Sequence change updating\n";
        }
        if(m_Playlist) {
            if(m_isDirty) {
                Next();
            }
        }
    }
#endif
}

