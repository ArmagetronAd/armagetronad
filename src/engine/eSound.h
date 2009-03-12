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

#ifndef ArmageTron_SOUND_H
#define ArmageTron_SOUND_H

#include "rSDL.h"
#include "defs.h"
// #include "eGrid.h"
#include "tString.h"
#include "tLinkedList.h"

void se_SoundInit();
void se_SoundExit();
//void se_SoundLock();
//void se_SoundUnlock();
void se_SoundPause(bool p);
void se_SoundMenu();

//! locks sound while in existence.
class eSoundLocker
{
    eSoundLocker( eSoundLocker const & );
public:
    eSoundLocker();
    ~eSoundLocker();
};

class eAudioPos{
public:
    Uint32 pos;
    Uint32 fraction;

    void Reset(int randomize=0);

    eAudioPos(){Reset();}
};

class eWavData: public tListItem<eWavData>{
    SDL_AudioSpec spec; // the audio format
    Uint8         *data; // the sound data
    Uint32        len;   // the data's length
    Uint32        samples; // samples
    tString       filename; // the filename
    tString       filename_alt; // the filename
    bool          freeData; // manually free data or use SDL_FreeWAV?

    static eWavData* s_anchor; // list anchor

public:
    bool alt; // was the alternative used?

    eWavData(const char * fileName,const char *alternative_file=""); // load file
    ~eWavData();

    void Load(); 		// really load the file
    void Unload();	// remove the file from memory
    static void UnloadAll(); // unload all sounds


    bool Mix(Uint8 *dest,
             Uint32 len,
             eAudioPos &pos,
             REAL rvol,
             REAL lvol,
             REAL speed=1,
             bool loop=false);

    // mixes to the buffer at *dest with length len the data from the wav
    // from position pos (will be modified to contain the end position)
    // with volume rvol/lvol and play speed speed. If loop is set,
    // start over if you reached the end.
    // return value: end reached.

    void Loop(); // prepares the sample for smooth looped output;
    // none of the many sound-editors I tried had that feature...
};

class eSoundPlayer{
    int id; // ID in the global players list
    eWavData *wav; // the sound we should put out
    eAudioPos pos[MAX_VIEWERS]; // the position of all viewers
    bool goon[MAX_VIEWERS];
    bool loop;

public:
    eSoundPlayer(eWavData &w,bool loop=false);
    ~eSoundPlayer();

    bool Mix(Uint8 *dest,
             Uint32 len,
             int viewer,
             REAL rvol,
             REAL lvol,
             REAL speed=1);

    void Reset(int randomize=0);
    void End();

    void MakeGlobal();

    //  static eGrid *S_Grid; // the grid we play the sounds on
};


#endif
