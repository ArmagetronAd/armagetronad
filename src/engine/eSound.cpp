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

#include "eSound.h"
#include "aa_config.h"
#include "tMemManager.h"
#include "tDirectories.h"
#include "tRandom.h"
#include "tError.h"
#include <string>
#include "tConfiguration.h"
#include "uMenu.h"
#include "eCamera.h"
//#include "tList.h"
#include <iostream>
#include <memory>
#include <stdlib.h>
#include "eGrid.h"
#include "tException.h"

//eGrid* eSoundPlayer::S_Grid = NULL;

#ifndef DEDICATED
#ifdef  HAVE_LIBSDL_MIXER
#include <SDL_mixer.h>
// static Mix_Music* music = NULL;
#endif

#endif

// sound quality

#define SOUND_OFF 0
#define SOUND_LOW 1
#define SOUND_MED 2
#define SOUND_HIGH 3

#if 0
#ifdef WIN32
static int buffer_shift=1;
#else
static int buffer_shift=0;
#endif

static tConfItem<int> bs("SOUND_BUFFER_SHIFT",buffer_shift);

int sound_quality=SOUND_MED;
static tConfItem<int> sq("SOUND_QUALITY",sound_quality);
#endif

static int sound_sources=10;
static tConfItem<int> ss("SOUND_SOURCES",sound_sources);
static REAL loudness_thresh=0;
static int real_sound_sources=0;
static REAL next_loudness_culled{0};
static REAL max_loudness_culled{0};
static REAL min_loudness_audible{100};
static REAL next_loudness_audible{100};

static tList<eSoundPlayer> se_globalPlayers;


void fill_audio_core(void *udata, Sint16 *stream, int len)
{
#ifndef DEDICATED
    real_sound_sources = 0;
    next_loudness_culled = 0;
    max_loudness_culled = 0;
    min_loudness_audible = 2;
    next_loudness_audible = 2;
    int i;
    if (eGrid::CurrentGrid())
        for(i=eGrid::CurrentGrid()->Cameras().Len()-1;i>=0;i--)
        {
            eCamera *pCam = eGrid::CurrentGrid()->Cameras()(i);
            if(pCam)
                pCam->SoundMix(stream,len);
        }

    for(i=se_globalPlayers.Len()-1;i>=0;i--)
        se_globalPlayers(i)->Mix(stream,len,0,1,1);

    if (real_sound_sources > sound_sources)
        loudness_thresh = .5f*(next_loudness_audible + min_loudness_audible);
    else if (real_sound_sources < sound_sources)
        loudness_thresh = .5f*(max_loudness_culled + next_loudness_culled);
    else
        loudness_thresh = .5f*(max_loudness_culled + min_loudness_audible);
#endif
}

#ifndef DEDICATED

namespace{
// checks whether a given buffer is 16 bit aligned
bool is_aligned_16(void *ptr, size_t space)
{
    const size_t alignment = alignof(Uint16);
#ifdef HAVE_CXX_ALIGN
    void *ptr_back{ptr};
    return 0 == (space % alignment) && std::align(alignment, space, ptr, space) == ptr_back;
#else
    // fallback for win32 and steam, assumes sensible ptr to uintptr conversion and linear memory model.
    return 0 == (reinterpret_cast<uintptr_t>(ptr) % alignment);
#endif
}

void fill_audio(void *udata, Uint8 *stream, int len)
{
    if(is_aligned_16(stream, len))
    {
        // aligment is good
        fill_audio_core(udata, reinterpret_cast<Sint16*>(stream), len);
        return;
    }
    
    // temp copy to 16 bit buffer
    static std::vector<Sint16> stream16;
    stream16.resize((len+1)/2);
    memcpy(&stream16[0], stream, len);
    fill_audio_core(udata, &stream16[0], len);
    memcpy(stream, &stream16[0], len);
}
}

#ifdef DEFAULT_SDL_AUDIODRIVER

// stringification, yep, two levels required
#define XSTRING(s) #s
#define STRING(s) XSTRING(s)

// call once to initialize SDL sound subsystem
static bool se_SoundInitPrepare()
{
    // initialize audio subsystem with predefined, hopefully good, driver
    if ( ! getenv("SDL_AUDIODRIVER") ) {
        char * arg = "SDL_AUDIODRIVER=" STRING(DEFAULT_SDL_AUDIODRIVER);
        putenv(arg);

        if ( SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0 )
            return true;

        putenv("SDL_AUDIODRIVER=");
    }

    // if that fails, try what the user wanted
    return ( SDL_InitSubSystem(SDL_INIT_AUDIO) >= 0 );
}
#endif
#endif

#ifndef DEDICATED
static unsigned int locks;
#endif

void se_SoundLock(){
#ifndef DEDICATED
    if (!locks)
        SDL_LockAudio();
    locks++;
#endif
}

void se_SoundUnlock(){
#ifndef DEDICATED
    locks--;
    if (!locks)
        SDL_UnlockAudio();
#endif
}

void se_SoundPause(bool p){
#ifndef DEDICATED
    SDL_PauseAudio(p);
#endif
}

// ***********************************************************

eLegacyWavData* eLegacyWavData::s_anchor = NULL;

eLegacyWavData::eLegacyWavData(const char * fileName,const char *alternative)
        :tListItem<eLegacyWavData>(s_anchor), loadError(false){
    //wavs.Add(this,id);
    filename     = fileName;
    filename_alt = alternative;

}

void eLegacyWavData::Load()
{
    //wavs.Add(this,id);

    if (!data.empty())
    {
        loadError = false;
        return;
    }

#ifndef DEDICATED

    static char const * errorName = "Sound Error";

    loadError = true;

    alt=false;

    const tPath& path = tDirectories::Data();

    Uint8 *byteData{};
    try
    {
        Uint32 len;
        SDL_AudioSpec *result=SDL_LoadWAV(path.GetReadPath(filename), &spec, &byteData, &len);
        if (result!=&spec || !byteData){
            if (filename_alt.Len()>1){
                result=SDL_LoadWAV(path.GetReadPath(filename_alt), &spec, &byteData, &len);
                if (result!=&spec || !byteData)
                {
                    tOutput err;
                    err.SetTemplateParameter(1, filename);
                    err << "$sound_error_filenotfound";
                    throw tGenericException(err, errorName);
                }
                else
                    alt=true;
            }
            else{
                result=SDL_LoadWAV(path.GetReadPath("sound/expl.ogg"), &spec, &byteData, &len);
                if (result!=&spec || !byteData)
                {
                    tOutput err;
                    err.SetTemplateParameter(1, "sound/expl.ogg");
                    err << "$sound_error_filenotfount";
                    throw tGenericException(err, errorName);
                }
                else
                    len=0;
            }
            /*
              tERR_ERROR("Sound file " << fileName << " not found. Have you called "
              "Armagetron from the right directory?"); */
        }

        if (spec.format==AUDIO_S16SYS)
        {
            SetData(byteData, len);
        }
        else
        {
            auto throwError = [&](){
                tOutput err;
                err.SetTemplateParameter(1, filename);
                err << "$sound_error_unsupported";
                throw tGenericException(err, errorName);
            };

            // convert to 16 bit system format
            SDL_AudioCVT cvt;
            if ( -1 == SDL_BuildAudioCVT( &cvt, spec.format, spec.channels, spec.freq, AUDIO_S16SYS, spec.channels, spec.freq ) )
            {
                throwError();
            }

            std::vector<Uint8> buf;
            buf.resize(len * cvt.len_mult);
            cvt.buf=&buf[0];
            cvt.len=len;
            memcpy(cvt.buf, byteData, len);

            if ( -1 == SDL_ConvertAudio( &cvt ) )
            {
                throwError();
            }

            spec.format = AUDIO_S16SYS;
            SetData(cvt.buf, cvt.len_cvt);
        }
    }
    catch(...){
        if(byteData)
            SDL_FreeWAV(byteData);
        throw;
    }

#ifdef DEBUG
#ifdef LINUX
    con << "Sound file " << filename << " loaded: ";
    switch (spec.format){
    case AUDIO_S16SYS: con << "16 bit "; break;
    case AUDIO_U8: con << "8 bit "; break;
    default: con << "unknown "; break;
    }
    if (spec.channels==2)
        con << "stereo ";
    else
        con << "mono ";

    con << "at " << spec.freq << " Hz,\n";

    con << samples << " samples.\n";

    loadError = false;
#endif
#endif
#endif
}

void eLegacyWavData::Unload(){
#ifndef DEDICATED
    loadError = false;

    eSoundLocker locker;

    data.clear();
    samples = 0;
#endif
}

void eLegacyWavData::SetData(Uint8 const *byteData, Uint32 lengthInBytes)
{
#ifndef DEDICATED
    data.resize((lengthInBytes+1)/2);
    memcpy(&data[0], byteData, lengthInBytes);
    samples =data.size()/spec.channels;
#endif
}

void eLegacyWavData::UnloadAll(){
    //wavs.Add(this,id);
    eLegacyWavData* wav = s_anchor;
    while ( wav )
    {
        wav->Unload();
        wav = wav->Next();
    }
}

eLegacyWavData::~eLegacyWavData(){
#ifndef DEDICATED
    Unload();
#endif
}

// from eSoundMixer.cpp
// extern int se_mixerFrequency;

#ifndef DEDICATED

#define SPEED_SHIFT 20
#define SPEED_FRACTION (1<<SPEED_SHIFT)

#define VOL_SHIFT 16
#define VOL_FRACTION (1<<VOL_SHIFT)

#define MAX_VAL ((1<<15)-1)
#define MIN_VAL (-(1<<15))

namespace
{
struct partial_mix
{
    eAudioPos &pos;
    Sint16 *dest{};
    Uint32 playlen{};
    Uint32 samples{};
    int speed{};
    int speed_fraction{};
    int lvol{};
    int rvol{};

    partial_mix(eAudioPos &pos_):pos(pos_){}

    template<typename POLLER> void mix(POLLER const &poller)
    {
        while (playlen>0 && pos.pos<samples){
            auto current = poller(pos.pos);
            auto next = poller((pos.pos+1) % samples);

            constexpr auto WEIGHT_SHIFT = 16;
            auto nextWeight = pos.fraction >> (SPEED_SHIFT-WEIGHT_SHIFT);
            auto currentWeight = (1 << WEIGHT_SHIFT) - nextWeight;

            int lNow = (current.first * currentWeight + next.first * nextWeight) >> WEIGHT_SHIFT;
            int rNow = (current.second * currentWeight + next.second * nextWeight) >> WEIGHT_SHIFT;

            int l=dest[0];
            int r=dest[1];
            l += (lvol*lNow) >> VOL_SHIFT;
            r += (rvol*rNow) >> VOL_SHIFT;
            if (r>MAX_VAL) r=MAX_VAL;
            if (l>MAX_VAL) l=MAX_VAL;
            if (r<MIN_VAL) r=MIN_VAL;
            if (l<MIN_VAL) l=MIN_VAL;

            dest[0]=l;
            dest[1]=r;

            dest+=2;

            pos.pos+=speed;

            pos.fraction+=speed_fraction;
            while (pos.fraction>=SPEED_FRACTION){
                pos.fraction-=SPEED_FRACTION;
                pos.pos++;
            }

            playlen--;
        }
    }
};

}

#endif

bool eLegacyWavData::Mix(Sint16 *dest,Uint32 playlen,eAudioPos &pos,
                   REAL Rvol,REAL Lvol,REAL Speed,bool loop){
#ifndef DEDICATED
    if ( data.empty() )
    {
        if( !loadError )
        {
            Load();
        }
        if ( data.empty() )
        {
            return false;
        }
    }

    playlen/=4;

    //	Rvol *= 4;
    //	Lvol *= 4;

    const REAL thresh = .25;

    if ( Rvol > thresh )
    {
        Rvol = thresh;
    }

    if ( Lvol > thresh )
    {
        Lvol = thresh;
    }

    // first, split the speed into the part before and after the decimal:
    if (Speed<0) Speed=0;

    // adjust for different sample rates:
    Speed*=spec.freq;
    Speed/=se_mixerFrequency;

    int speed=int(floor(Speed));
    int speed_fraction=int(SPEED_FRACTION*(Speed-speed));

    partial_mix mix{pos};
    mix.dest = dest;
    mix.playlen = playlen;
    mix.samples = samples;
    mix.speed = speed;
    mix.speed_fraction = speed_fraction;

    // make integers out of the volumes:
    mix.rvol=int(Rvol*VOL_FRACTION);
    mix.lvol=int(Lvol*VOL_FRACTION);

    bool goon=true;

    while (goon){
        if (spec.channels==2){
            switch(spec.format)
            {
                case AUDIO_S16SYS:
                {
                    auto poller = [&](Uint32 pos)
                    {
                        auto r = data[(pos<<1)  ];
                        auto l = data[(pos<<1)+1];
 
                        return std::make_pair(l, r);
                    };
                    mix.mix(poller);
                    break;
                }
                default:
                    tASSERT(false);
                    break;
            }
        }
        else if(spec.channels==1)
        {
            switch(spec.format)
            {
                case AUDIO_S16SYS:
                {
                    auto poller = [&](Uint32 pos)
                    {
                        int d = data[pos];
 
                        return std::make_pair(d, d);
                    };
                    mix.mix(poller);
                    break;
                }
                default:
                    tASSERT(false);
                    break;
            }
        }
        else
        {
            // spec.channels value unsupported
            tASSERT(spec.channels < 2);
            break;
        }

        if (loop && pos.pos>=samples)
            pos.pos-=samples;
        else
            goon=false;
    }
    return (mix.playlen>0);
#endif
    return false;
}

void eLegacyWavData::Loop(){
#ifndef DEDICATED
    if (spec.format==AUDIO_S16SYS){
        std::vector<Sint16> buff2;
        using std::swap;
        swap(buff2, data);
        data.resize(buff2.size());
        for(int i=samples-1;i>=0;i--){
            Uint32 j=i+((samples>>2)<<1);
            while (j>=samples) j-=samples;

            REAL a=fabs(100*(j/REAL(samples)-.5));
            if (a>1) a=1;
            REAL b=1-a;

            data[i]=int(a*buff2[i]+b*buff2[j]);
        }
    }
    else
    {
        tASSERT(false);
    }
#endif
}


// ******************************************************************

void eAudioPos::Reset(int randomize){
#ifndef DEDICATED
    if (randomize){
        tRandomizer & randomizer = tRandomizer::GetInstance();
        fraction = randomizer.Get( SPEED_FRACTION );
        // fraction=int(SPEED_FRACTION*(rand()/float(RAND_MAX)));
        pos=randomizer.Get( randomize );
        // pos=int(randomize*(rand()/float(RAND_MAX)));
    }
    else
        fraction=pos=0;
#endif
}



eSoundPlayer::eSoundPlayer(eLegacyWavData &w,bool l)
        :id(-1),wav(&w),loop(l){
    if (l)
        wav->Load();

    for(int i=MAX_VIEWERS-1;i>=0;i--)
        goon[i]=true;
}

eSoundPlayer::~eSoundPlayer()
{
    eSoundLocker locker;
    se_globalPlayers.Remove(this,id);
}

bool eSoundPlayer::Mix(Sint16 *dest,
                       Uint32 len,
                       int viewer,
                       REAL rvol,
                       REAL lvol,
                       REAL speed){

    if (goon[viewer]){
        auto const loudness = rvol + lvol;
        if (loudness > loudness_thresh){
            real_sound_sources++;

            if(loudness < min_loudness_audible)
            {
                next_loudness_audible = min_loudness_audible;
                min_loudness_audible = loudness;
            }
            else if(loudness < next_loudness_audible)
            {
                next_loudness_audible = loudness;
            }

            return goon[viewer]=!wav->Mix(dest,len,pos[viewer],rvol,lvol,speed,loop);
        }
        else
        {
            if(loudness > max_loudness_culled)
            {
                next_loudness_culled = max_loudness_culled;
                max_loudness_culled = loudness;
            }
            else if(loudness > next_loudness_culled)
            {
                next_loudness_culled = loudness;
            }

            return true;
        }
    }
    else
        return false;
}

void eSoundPlayer::Reset(int randomize){
    wav->Load();

    for(int i=MAX_VIEWERS-1;i>=0;i--){
        pos[i].Reset(randomize);
        goon[i]=true;
    }
}

void eSoundPlayer::End(){
    for(int i=MAX_VIEWERS-1;i>=0;i--){
        goon[i]=false;
    }
}


void eSoundPlayer::MakeGlobal(){
    wav->Load();

    eSoundLocker locker;
    se_globalPlayers.Add(this,id);
}

// ***************************************************************

#if 0
uMenu Sound_menu("$sound_menu_text");

static uMenuItemInt sources_men
(&Sound_menu,"$sound_menu_sources_text",
 "$sound_menu_sources_help",
 sound_sources,2,20,2);

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
 "$sound_menu_buffer_text",
 "$sound_menu_buffer_help",
 buffer_shift);

static uSelectEntry<int> ba(bm_men,
                            "$sound_menu_buffer_vsmall_text",
                            "$sound_menu_buffer_vsmall_help",
                            -2);

static uSelectEntry<int> bb(bm_men,
                            "$sound_menu_buffer_small_text",
                            "$sound_menu_buffer_small_help",
                            -1);

static uSelectEntry<int> bc(bm_men,
                            "$sound_menu_buffer_med_text",
                            "$sound_menu_buffer_med_help",
                            0);

static uSelectEntry<int> bd(bm_men,
                            "$sound_menu_buffer_high_text",
                            "$sound_menu_buffer_high_help",
                            1);

static uSelectEntry<int> be(bm_men,
                            "$sound_menu_buffer_vhigh_text",
                            "$sound_menu_buffer_vhigh_help",
                            2);


void se_SoundMenu(){
    //	se_SoundPause(true);
    //	se_SoundLock();
    int oldsettings=sound_quality;
    int oldshift=buffer_shift;
    Sound_menu.Enter();
    if (oldsettings!=sound_quality || oldshift!=buffer_shift){
        se_SoundExit();
        se_SoundInit();
    }
    //	se_SoundUnlock();
    //  se_SoundPause(false);
}
#endif

eSoundLocker::eSoundLocker()
{
    se_SoundLock();
}

eSoundLocker::~eSoundLocker()
{
    se_SoundUnlock();
}
