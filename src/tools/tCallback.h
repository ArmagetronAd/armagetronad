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

#ifndef ArmageTron_CALLBACK_H
#define ArmageTron_CALLBACK_H

#include "defs.h"
#include "tLinkedList.h"
#include "tRuby.h"

class tCallback:public tListItem<tCallback>{
    AA_VOIDFUNC *func;
public:
    tCallback(tCallback*& anchor, AA_VOIDFUNC *f);
    static void Exec(tCallback *anchor);
};

#ifdef HAVE_LIBRUBY
class tCallbackRuby : public tListItem<tCallbackRuby> {
	VALUE block;
protected:
	static VALUE ExecProtect(VALUE);	
public:
	tCallbackRuby(tCallbackRuby *& anchor);
	static void Exec(tCallbackRuby *anchor);
};
#endif // HAVE_LIBRUBY

class tCallbackAnd:public tListItem<tCallbackAnd>{
    BOOLRETFUNC *func;
public:
    tCallbackAnd(tCallbackAnd*& anchor, BOOLRETFUNC *f);
    static bool Exec(tCallbackAnd *anchor);
};

class tCallbackOr:public tListItem<tCallbackOr>{
    BOOLRETFUNC *func;
public:
    tCallbackOr(tCallbackOr*& anchor, BOOLRETFUNC *f);
    static bool Exec(tCallbackOr *anchor);
};

#endif
