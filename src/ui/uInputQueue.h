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

#ifndef ArmageTron_INPUT_QUEUE_H
#define ArmageTron_INPUT_QUEUE_H


#include "defs.h"
#include "rSDL.h"

void su_FetchAndStoreSDLInput();

bool su_StoreSDLEvent(const SDL_Event &tEvent);    //!< stores an event so it will be returned by GetSDLInput() later
bool su_GetSDLInput(SDL_Event &tEvent,REAL &time); //!< fetches an event

//! have one object of this class around while processing input
class uInputProcessGuard
{
public:
    uInputProcessGuard();  //!< constructor initializing input processing
    ~uInputProcessGuard(); //!< destructor deinitializing input processing
};

inline bool su_GetSDLInput(SDL_Event &tEvent){
    REAL dummy;
    return su_GetSDLInput(tEvent,dummy);
}

extern bool su_prefetchInput;
extern bool su_contInput;

int su_InputThread(void *);

class su_TimerCallback{
public:
    virtual REAL GetTime()=0;
    su_TimerCallback();
    virtual ~su_TimerCallback();
};

#endif

