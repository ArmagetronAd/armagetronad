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

#include "tToDo.h"
#include "tArray.h"

#ifdef HAVE_LIBZTHREAD
#include <zthread/FastRecursiveMutex.h>

static ZThread::FastRecursiveMutex st_mutex;
#elif defined(HAVE_PTHREAD)
#include "pthread-binding.h"
static tPThreadRecursiveMutex st_mutex;
#else
class tMockMutex
{
public:
    void acquire(){};
    void release(){};
};

static tMockMutex st_mutex;
#endif

tArray<tTODO_FUNC *> tToDos;

void st_ToDo(tTODO_FUNC *td){ // postpone something
    st_mutex.acquire();
    tToDos[tToDos.Len()]=td;
    st_mutex.release();
}

// a lone (but relatively safe) function pointer for things to do triggered by signals.
static tTODO_FUNC * st_toDoFromSignal = 0;

void st_DoToDo(){ // do the things that have been postponed
    if ( st_toDoFromSignal )
    {
        st_ToDo( st_toDoFromSignal );
        st_toDoFromSignal = 0;
    }
    st_mutex.acquire();
    while (tToDos.Len()){
        tTODO_FUNC *td=tToDos[tToDos.Len()-1];
        tToDos.SetLen(tToDos.Len()-1);
        (*td)();
    }
    st_mutex.release();
}

void st_ToDo_Signal(tTODO_FUNC *td){ // postpone something
    // simply ignore double todos from signals.
    if ( st_toDoFromSignal )
    {
        return;
    }
    st_toDoFromSignal = td;
}
