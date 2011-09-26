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

#include "tMutex.h"

static boost::recursive_mutex st_mutex;

tArray<tTODO_FUNC *> tToDos;

void st_ToDo(tTODO_FUNC *td){ // postpone something
    boost::lock_guard< boost::recursive_mutex > lock( st_mutex );

    tToDos[tToDos.Len()]=td;
}

// the function currently in execution
static tTODO_FUNC * st_toDoCurrent = 0;

void st_ToDoOnce(tTODO_FUNC *td){ // postpone something, avoid double entries
    boost::lock_guard< boost::recursive_mutex > lock( st_mutex );

    if( st_toDoCurrent == td )
    {
        return;
    }
    for( int i = tToDos.Len()-1; i >= 0; --i )
    {
        if( tToDos[i]==td )
        {
            return;
        }
    }
    tToDos[tToDos.Len()]=td;
}

// a lone (but relatively safe) function pointer for things to do triggered by signals.
static tTODO_FUNC * st_toDoFromSignal = 0;

void st_DoToDo(){ // do the things that have been postponed
    if ( st_toDoFromSignal )
    {
        st_ToDo( st_toDoFromSignal );
        st_toDoFromSignal = 0;
    }
    boost::lock_guard< boost::recursive_mutex > lock( st_mutex );
    while (tToDos.Len()){
        tTODO_FUNC *last = st_toDoCurrent;
        tTODO_FUNC *td = tToDos[tToDos.Len()-1];
        st_toDoCurrent = td;
        tToDos.SetLen(tToDos.Len()-1);
        (*td)();
        st_toDoCurrent = last;
    }
}

void st_ToDo_Signal(tTODO_FUNC *td){ // postpone something
    // simply ignore double todos from signals.
    if ( st_toDoFromSignal )
    {
        return;
    }
    st_toDoFromSignal = td;
}
