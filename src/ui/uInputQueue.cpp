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

#include "uInputQueue.h"
#include "rScreen.h"
#include "tConfiguration.h"
#include <iostream>

#ifndef DEDICATED
#include "rSDL.h"
#endif

#include  "tRecorder.h"

#include  "uMenu.h"

static su_TimerCallback *timer=NULL;

su_TimerCallback::su_TimerCallback(){
    timer = this;
}

su_TimerCallback::~su_TimerCallback(){
    if (timer == this)
        timer = NULL;
}

static inline REAL Time(){
    if (timer)
        return timer->GetTime();
    else
        return 0;
}

bool su_prefetchInput=false;
bool su_contInput=true;

#define MAX_PENDING_INPUT 100

static REAL times[MAX_PENDING_INPUT];
static SDL_Event tEvents[MAX_PENDING_INPUT];

static int   currentIn=0,current_out=0,next_in=1;


static inline void increase(int &i){
    i++;
    if (i>=MAX_PENDING_INPUT)
        i=0;
}


static bool input_get=false;

void su_FetchAndStoreSDLInput()
{
#ifndef DEDICATED
#ifndef WIN32
#ifndef MACOSX
    if(!tRecorder::IsRunning() )
        SDL_PumpEvents();
#endif
#endif
#endif
}


bool su_StoreSDLEvent(const SDL_Event &tEvent){
    if (next_in!=current_out && !input_get){
        //con << "Extra input!\n";
        tEvents[currentIn]=tEvent;
        times[currentIn]=Time();
        increase(currentIn);
        next_in=currentIn;
        increase(next_in);
        return false;
    }
    return true;
}

// read and write operators for keysyms
#ifndef DEDICATED
tRECORDING_ENUM( SDLKey );
tRECORDING_ENUM( SDLMod );
#endif

static char const * recordingSection = "INPUT";

//! Read or write event data
template< class Archiver > class EventArchiver
{
public:
    static bool Archive( SDL_Event & event, REAL & time, bool & ret )
    {
        // start archive block if archiving is active
        Archiver archive;
        if ( archive.Initialize( recordingSection ) )
        {
#ifndef DEDICATED
            archive.Archive( ret );
            if ( !ret )
                return false;

            // write or read data
            archive.Archive(time).Archive(event.type);
            switch( event.type )
            {
            case SDL_ACTIVEEVENT:
                {
                    SDL_ActiveEvent & active = event.active;

                    archive.Archive(active.gain).Archive(active.state);
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    SDL_KeyboardEvent & key = event.key;

                    archive.Archive(key.state).Archive(key.keysym.scancode).Archive(key.keysym.sym).Archive(key.keysym.mod).Archive(key.keysym.unicode);
                }
                break;
            case SDL_MOUSEMOTION:
                {
                    SDL_MouseMotionEvent & motion = event.motion;

                    archive.Archive(motion.state).Archive(motion.x).Archive(motion.y).Archive(motion.xrel).Archive(motion.yrel);
                }
                break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
                {
                    SDL_MouseButtonEvent & button = event.button;

                    archive.Archive(button.button).Archive(button.state).Archive(button.x).Archive(button.y);
                }
                break;
            default:
                // do nothing
                break;
            }

#endif  // DEDICATED

            return true;
        }

        return false;
    }
};

static const char * su_end = "END";
static const char * su_endInput = "ENDINPUT";

// flag indicating input was made and an input start marker is needed for the next input loop
static bool su_markerRequired = false;

void su_EndGetSDLInput()
{
    if ( su_markerRequired )
    {
        // record end of input fetching
        tRecorder::Playback(su_endInput);
        tRecorder::Record(su_endInput);
        su_markerRequired = false;
    }
}

uInputProcessGuard::uInputProcessGuard()
{}
uInputProcessGuard::~uInputProcessGuard()
{
    su_EndGetSDLInput();
}

bool su_GetSDLInput(SDL_Event &tEvent,REAL &time){
    bool ret=false;

    // clear out data
    memset( &tEvent, 0, sizeof( SDL_Event ) );

    // find end of recording in playback
    if ( tRecorder::Playback(su_end) )
    {
        tRecorder::Record(su_end);
        uMenu::quickexit=true;
    }

    // try to fetch event from playback
    if ( !EventArchiver< tPlaybackBlock >::Archive( tEvent, time, ret ) )
    {
        // get real event
        sr_LockSDL();
        input_get=true;
        if (current_out!=currentIn){
            time=times[current_out];
            tEvent=tEvents[current_out];
            increase(current_out);
            ret=true;
        }
        else{
            time=Time();
            ret=
#ifndef DEDICATED
                SDL_PollEvent(&tEvent);
#else
                false;
#endif
        }
        sr_UnlockSDL();
        input_get=false;
    }

    su_markerRequired |= ret;

    // store event in recording
    if( ret )
        EventArchiver< tRecordingBlock >::Archive( tEvent, time, ret );

    return ret;
}

/*
int su_InputThread(void *){
    while (su_contInput){
        if (sr_screen && su_prefetchInput){
            sr_LockSDL();
#ifndef DEDICATED
            SDL_PumpEvents();
#endif
            sr_UnlockSDL();
        }
#ifndef WIN32
        usleep(100000);
#endif
    }
    return 0;
}
*/


