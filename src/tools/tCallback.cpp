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

#include "tCallback.h"
#include "tCallbackString.h"

tCallback::tCallback(tCallback*& anchor, VOIDFUNC *f)
        :tListItem<tCallback>(anchor), func(f){
    tASSERT(f != NULL);
}

void tCallback::Exec(tCallback *anchor){
    if (anchor){
        (*anchor->func)();
        Exec(anchor->Next());
    }
}



tCallbackAnd::tCallbackAnd(tCallbackAnd*& anchor, BOOLRETFUNC *f)
        :tListItem<tCallbackAnd>(anchor), func(f){
    tASSERT(f);
}

bool tCallbackAnd::Exec(tCallbackAnd *anchor){
    if (anchor)
        return (*anchor->func)() && Exec(anchor->Next());
    else
        return true;
}


tCallbackOr::tCallbackOr(tCallbackOr*& anchor, BOOLRETFUNC *f)
        :tListItem<tCallbackOr>(anchor), func(f){
    tASSERT(f);
}

bool tCallbackOr::Exec(tCallbackOr *anchor){
    if (anchor)
        return (*anchor->func)() || Exec(anchor->Next());
    else
        return false;
}



tCallbackString::tCallbackString(tCallbackString*& anchor, STRINGRETFUNC *f)
        :tListItem<tCallbackString>(anchor), func(f){
    tASSERT(f);
}

tString tCallbackString::Exec(tCallbackString *anchor){
    tString ret("");
    if (anchor)
        ret << (*anchor->func)() << Exec(anchor->Next());

    return ret;
}

