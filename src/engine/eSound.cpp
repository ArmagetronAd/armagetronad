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

#include "eSound.h"
#include "config.h"
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
#include <stdlib.h>
#include "eGrid.h"
#include "tException.h"

//eGrid* eSoundPlayer::S_Grid = NULL;

#ifdef WIN32
#define HAVE_LIBSDL_MIXER 1
#endif

#ifndef DEDICATED
#ifdef  HAVE_LIBSDL_MIXER
#include <SDL_mixer.h>
static Mix_Music* music = NULL;
#endif

static SDL_AudioSpec audio;
static bool sound_is_there=false;
static bool uses_sdl_mixer=false;
#endif

// sound quality

#define SOUND_OFF 0
#define SOUND_LOW 1
#define SOUND_MED 2
#define SOUND_HIGH 3

#ifdef WIN32
static int buffer_shift=1;
#else
static int buffer_shift=0;
#endif

static tConfItem<int> bs("SOUND_BUFFER_SHIFT",buffer_shift);

int sound_quality=SOUND_MED;
static tConfItem<int> sq("SOUND_QUALITY",sound_quality);

static int sound_sources=10;
static tConfItem<int> ss("SOUND_SOURCES",sound_sources);
static REAL loudness_thresh=0;
static int real_sound_sources=0;

static tList<eSoundPlayer> se_globalPlayers;


void fill_audio(void *udata, Uint8 *stream, int len)
{
#ifndef DEDICATED
    real_sound_sources=0;
    int i;
    if (eGrid::CurrentGrid())
        for(i=eGrid::CurrentGrid()->Cameras().Len()-1;i>=0;i--)
            eGrid::CurrentGrid()->Cameras()(i)->SoundMix(stream,len);

    for(i=se_globalPlayers.Len()-1;i>=0;i--)
        se_globalPlayers(i)->Mix(stream,len,0,1,1);

    if (real_sound_sources>sound_sources+4)
        loudness_thresh+=.01;
    if (real_sound_sources>sound_sources+1)
        loudness_thresh+=.001;
    if (real_sound_sources<sound_sources-4)
        loudness_thresh-=.001;
    if (real_sound_sources<sound_sources-1)
        loudness_thresh-=.0001;
    if (loudness_thresh<0)
        loudness_thresh=0;
#endif
}

#ifndef DEDICATED
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

void se_SoundInit()
{
#ifndef DEDICATED
    // save configuration file with sound disabled on first use so we don't try again
    bool needSave = false;
    static bool firstRun = true;
    if ( st_FirstUse )
    {
        needSave = true;
        int sound_quality_back = sound_quality;
        sound_quality = SOUND_OFF;
        st_SaveConfig();
        if ( firstRun )
            con << tOutput("$sound_firstinit");
        sound_quality=sound_quality_back;
    }

    if ( sound_quality != SOUND_OFF )
    {
#ifdef DEFAULT_SDL_AUDIODRIVER
        static bool init = se_SoundInitPrepare();
        if ( !init )
            return;
#endif
        if ( firstRun && !SDL_WasInit( SDL_INIT_AUDIO ) )
            return;
        firstRun = false;
    }

    if (!sound_is_there && sound_quality!=SOUND_OFF)
    {
        SDL_AudioSpec desired;

        switch (sound_quality)
        {
        case SOUND_LOW:
            desired.freq=11025; break;
        case SOUND_MED:
            desired.freq=22050; break;
        case SOUND_HIGH:
            desired.freq=44100; break;
        default:
            desired.freq=22050;
        }

        desired.format=AUDIO_S16SYS;
        desired.samples=128;
        while (desired.samples <= desired.freq >> (6-buffer_shift))
            desired.samples <<= 1;
        desired.channels = 2;
        desired.callback = fill_audio;
        desired.userdata = NULL;

#ifdef HAVE_LIBSDL_MIXER
        uses_sdl_mixer=true;

        // init using SDL_Mixer
        sound_is_there=(Mix_OpenAudio(desired.freq, desired.format, desired.channels, desired.samples)>=0);

        if ( sound_is_there )
        {
            // query actual sound info
            audio = desired;
            int channels;
            Mix_QuerySpec( &audio.freq, &audio.format, &channels );
            audio.channels = channels;

            // register callback
            Mix_SetPostMix( &fill_audio, NULL );

            const tPath& vpath = tDirectories::Data();
            tString musFile = vpath.GetReadPath( "music/fire.xm" );

            music = Mix_LoadMUS( musFile );

            if ( music )
                Mix_FadeInMusic( music, -1, 2000 );

        }
#else
        // just use SDL to init sound
        uses_sdl_mixer=false;
        sound_is_there=(SDL_OpenAudio(&desired,&audio)>=0);
#endif
        if (sound_is_there && (audio.format!=AUDIO_S16SYS || audio.channels!=2))
        {
            uses_sdl_mixer=false;
            se_SoundExit();
            // force emulation of 16 bit stereo; sadly, this cannot use SDL_Mixer :-(
            audio.format=AUDIO_S16SYS;
            audio.channels=2;
            sound_is_there=(SDL_OpenAudio(&audio,NULL)>=0);
            con << tOutput("$sound_error_no16bit");
        }
        if (!sound_is_there)
            con << tOutput("$sound_error_initfailed");
        else
        {
            //for(int i=wavs.Len()-1;i>=0;i--)
            //wavs(i)->Init();
#ifdef DEBUG
            tOutput o;
            o.SetTemplateParameter(1,audio.freq);
            o.SetTemplateParameter(2,audio.samples);
            o << "$sound_inited";
            con << o;
#endif
        }
    }

    // save sound settings, they appear to work
    if ( needSave )
    {
        st_SaveConfig();
    }
#endif
}

void se_SoundExit(){
#ifndef DEDICATED
    se_SoundLock();

    eWavData::UnloadAll();
    se_SoundPause(true);

    se_SoundUnlock();

    if (sound_is_there){
#ifdef DEBUG
        con << tOutput("$sound_disabling");
#endif
        //		se_SoundPause(false);
        //    for(int i=wavs.Len()-1;i>=0;i--)
        //wavs(i)->Exit();

#ifdef HAVE_LIBSDL_MIXER
        if ( music )
        {
            if( Mix_PlayingMusic() )
            {
                Mix_FadeOutMusic(100);
                SDL_Delay(100);
            }
            Mix_FreeMusic( music );
            music = NULL;
        }

        se_SoundPause(true);

        if ( uses_sdl_mixer )
            Mix_CloseAudio();
        else
#endif
            SDL_CloseAudio();

#ifdef DEBUG
        con << tOutput("$sound_disabling_done");
#endif
    }
    sound_is_there=false;
#endif
}

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

eWavData* eWavData::s_anchor = NULL;

eWavData::eWavData(const char * fileName,const char *alternative)
        :tListItem<eWavData>(s_anchor),data(NULL),len(0),freeData(false){
    //wavs.Add(this,id);
    filename     = fileName;
    filename_alt = alternative;

}

void eWavData::Load(){
    //wavs.Add(this,id);

    if (data)
        return;

#ifndef DEDICATED

    static char const * errorName = "Sound Error";

    freeData = false;

    alt=false;

    const tPath& path = tDirectories::Data();

    SDL_AudioSpec *result=SDL_LoadWAV( path.GetReadPath( filename ) ,&spec,&data,&len);
    if (result!=&spec || !data){
        if (filename_alt.Len()>1){
            result=SDL_LoadWAV( path.GetReadPath( filename_alt ),&spec,&data,&len);
            if (result!=&spec || !data)
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
            result=SDL_LoadWAV( path.GetReadPath( "sound/expl.wav" ) ,&spec,&data,&len);
            if (result!=&spec || !data)
            {
                tOutput err;
                err.SetTemplateParameter(1, "sound/expl.waw");
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
        samples=len>>1;
    //	else if(spec.format==AUDIO_U8)
    //		samples=len;
    else
    {
        // prepare error message
        tOutput err;
        err.SetTemplateParameter(1, filename);
        err << "$sound_error_unsupported";

        // convert to 16 bit system format
        SDL_AudioCVT cvt;
        if ( -1 == SDL_BuildAudioCVT( &cvt, spec.format, spec.channels, spec.freq, AUDIO_S16SYS, spec.channels, spec.freq ) )
        {
            throw tGenericException(err, errorName);
        }

        cvt.buf=reinterpret_cast<Uint8 *>( malloc( len * cvt.len_mult ) );
        cvt.len=len;
        memcpy(cvt.buf, data, len);
        freeData = true;


        if ( -1 == SDL_ConvertAudio( &cvt ) )
        {
            throw tGenericException(err, errorName);
        }

        SDL_FreeWAV( data );
        data = cvt.buf;
        spec.format = AUDIO_S16SYS;
        len    = len * cvt.len_mult;

        samples = len >> 1;
    }

    samples/=spec.channels;

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

    con << samples << " samples in " << len << " bytes.\n";
#endif
#endif
#endif
}

void eWavData::Unload(){
#ifndef DEDICATED
    //wavs.Add(this,id);
    if (data){
        se_SoundLock();
        if ( freeData )
        {

            free(data);

        }

        else

        {

            SDL_FreeWAV(data);

        }



        data=NULL;
        len=0;
        se_SoundUnlock();
    }
#endif
}

void eWavData::UnloadAll(){
    //wavs.Add(this,id);
    eWavData* wav = s_anchor;
    while ( wav )
    {
        wav->Unload();
        wav = wav->Next();
    }

}

eWavData::~eWavData(){
#ifndef DEDICATED
    Unload();
#endif
}

bool eWavData::Mix(Uint8 *dest,Uint32 playlen,eAudioPos &pos,
                   REAL Rvol,REAL Lvol,REAL Speed,bool loop){
#ifndef DEDICATED
    if ( !data )
    {
        return false;
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

#define SPEED_FRACTION (1<<20)

#define VOL_SHIFT 16
#define VOL_FRACTION (1<<VOL_SHIFT)

#define MAX_VAL ((1<<16)-1)
#define MIN_VAL -(1<<16)

    // first, split the speed into the part before and after the decimal:
    if (Speed<0) Speed=0;

    // adjust for different sample rates:
    Speed*=spec.freq;
    Speed/=audio.freq;

    int speed=int(floor(Speed));
    int speed_fraction=int(SPEED_FRACTION*(Speed-speed));

    // secondly, make integers out of the volumes:
    int rvol=int(Rvol*VOL_FRACTION);
    int lvol=int(Lvol*VOL_FRACTION);


    bool goon=true;

    while (goon){
        if (spec.channels==2){
            if (spec.format==AUDIO_U8)
                while (playlen>0 && pos.pos<samples){
                    // fix endian problems for the Mac port, as well as support for other
                    // formats than  stereo...
                    int l=((short *)dest)[0];
                    int r=((short *)dest)[1];
                    r += (rvol*(data[(pos.pos<<1)  ]-128)) >> (VOL_SHIFT-8);
                    l += (lvol*(data[(pos.pos<<1)+1]-128)) >> (VOL_SHIFT-8);
                    if (r>MAX_VAL) r=MAX_VAL;
                    if (l>MAX_VAL) l=MAX_VAL;
                    if (r<MIN_VAL) r=MIN_VAL;
                    if (l<MIN_VAL) l=MIN_VAL;

                    ((short *)dest)[0]=l;
                    ((short *)dest)[1]=r;

                    dest+=4;

                    pos.pos+=speed;

                    pos.fraction+=speed_fraction;
                    while (pos.fraction>=SPEED_FRACTION){
                        pos.fraction-=SPEED_FRACTION;
                        pos.pos++;
                    }

                    playlen--;
                }
            else{
                while (playlen>0 && pos.pos<samples){
                    int l=((short *)dest)[0];
                    int r=((short *)dest)[1];
                    r += (rvol*(((short *)data)[(pos.pos<<1)  ])) >> VOL_SHIFT;
                    l += (lvol*(((short *)data)[(pos.pos<<1)+1])) >> VOL_SHIFT;
                    if (r>MAX_VAL) r=MAX_VAL;
                    if (l>MAX_VAL) l=MAX_VAL;
                    if (r<MIN_VAL) r=MIN_VAL;
                    if (l<MIN_VAL) l=MIN_VAL;

                    ((short *)dest)[0]=l;
                    ((short *)dest)[1]=r;

                    dest+=4;

                    pos.pos+=speed;

                    pos.fraction+=speed_fraction;
                    while (pos.fraction>=SPEED_FRACTION){
                        pos.fraction-=SPEED_FRACTION;
                        pos.pos++;
                    }
                    playlen--;
                }
            }
        }
        else{
            if (spec.format==AUDIO_U8){
                while (playlen>0 && pos.pos<samples){
                    // fix endian problems for the Mac port, as well as support for other
                    // formats than  stereo...
                    int l=((short *)dest)[0];
                    int r=((short *)dest)[1];
                    int d=data[pos.pos]-128;
                    l += (lvol*d) >> (VOL_SHIFT-8);
                    r += (rvol*d) >> (VOL_SHIFT-8);
                    if (r>MAX_VAL) r=MAX_VAL;
                    if (l>MAX_VAL) l=MAX_VAL;
                    if (r<MIN_VAL) r=MIN_VAL;
                    if (l<MIN_VAL) l=MIN_VAL;

                    ((short *)dest)[0]=l;
                    ((short *)dest)[1]=r;

                    dest+=4;

                    pos.pos+=speed;

                    pos.fraction+=speed_fraction;
                    while (pos.fraction>=SPEED_FRACTION){
                        pos.fraction-=SPEED_FRACTION;
                        pos.pos++;
                    }

                    playlen--;
                }
            }
            else
                while (playlen>0 && pos.pos<samples){
                    int l=((short *)dest)[0];
                    int r=((short *)dest)[1];
                    int d=((short *)data)[pos.pos];
                    l += (lvol*d) >> VOL_SHIFT;
                    r += (rvol*d) >> VOL_SHIFT;
                    if (r>MAX_VAL) r=MAX_VAL;
                    if (l>MAX_VAL) l=MAX_VAL;
                    if (r<MIN_VAL) r=MIN_VAL;
                    if (l<MIN_VAL) l=MIN_VAL;

                    ((short *)dest)[0]=l;
                    ((short *)dest)[1]=r;

                    dest+=4;

                    pos.pos+=speed;

                    pos.fraction+=speed_fraction;
                    while (pos.fraction>=SPEED_FRACTION){
                        pos.fraction-=SPEED_FRACTION;
                        pos.pos++;
                    }
                    playlen--;
                }
        }

        if (loop && pos.pos>=samples)
            pos.pos-=samples;
        else
            goon=false;
    }
#endif
    return (playlen>0);

}

void eWavData::Loop(){
#ifndef DEDICATED
    Uint8 *buff2=tNEW(Uint8) [len];

    if (buff2){
        memcpy(buff2,data,len);
        Uint32 samples;

        if (spec.format==AUDIO_U8){
            samples=len;
            for(int i=samples-1;i>=0;i--){
                Uint32 j=i+((len>>2)<<1);
                if (j>=len) j-=len;

                REAL a=fabs(100*(j/REAL(samples)-.5));
                if (a>1) a=1;
                REAL b=1-a;

                data[i]=int(a*buff2[i]+b*buff2[j]);
            }
        }
        else if (spec.format==AUDIO_S16SYS){
            samples=len>>1;
            for(int i=samples-1;i>=0;i--){

                /*
                  REAL a=2*i/REAL(samples);
                  if (a>1) a=2-a;
                  REAL b=1-a;
                */


                Uint32 j=i+((samples>>2)<<1);
                while (j>=samples) j-=samples;

                REAL a=fabs(100*(j/REAL(samples)-.5));
                if (a>1) a=1;
                REAL b=1-a;


                ((short *)data)[i]=int(a*((short *)buff2)[i]+b*((short *)buff2)[j]);
            }
        }
        delete[] buff2;
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



eSoundPlayer::eSoundPlayer(eWavData &w,bool l)
        :id(-1),wav(&w),loop(l){
    if (l)
        wav->Load();

    for(int i=MAX_VIEWERS-1;i>=0;i--)
        goon[i]=true;
}

eSoundPlayer::~eSoundPlayer(){}

bool eSoundPlayer::Mix(Uint8 *dest,
                       Uint32 len,
                       int viewer,
                       REAL rvol,
                       REAL lvol,
                       REAL speed){

    if (goon[viewer]){
        if (rvol+lvol>loudness_thresh){
            real_sound_sources++;
            return goon[viewer]=!wav->Mix(dest,len,pos[viewer],rvol,lvol,speed,loop);
        }
        else
            return true;
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

    se_SoundLock();
    se_globalPlayers.Add(this,id);
    se_SoundUnlock();
}


// ***************************************************************

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

