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

#ifndef ARMAGETRON_G_ROTATION_H
#define ARMAGETRON_G_ROTATION_H

#include "tCallback.h"
#include "tLinkedList.h"

#ifdef HAVE_LIBRUBY
class gRoundEventRuby : public tCallbackRuby {
public:
    gRoundEventRuby();
    static void DoRoundEvents();
};

class gMatchEventRuby : public tCallbackRuby {
public:
    gMatchEventRuby();
    static void DoMatchEvents();
};
#endif // HAVE_LIBRUBY

class gRotation
{
public:
    gRotation() {}
    virtual ~gRotation() {}
    static void HandleNewRound();
    static void HandleNewMatch();
};


#endif
