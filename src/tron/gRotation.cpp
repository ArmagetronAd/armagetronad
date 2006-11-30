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

#include "gRotation.h"

void gRotation::HandleNewRound() {
    std::cerr << "round!\n";
#ifdef HAVE_LIBRUBY
    gRoundEventRuby::DoRoundEvents();
#endif
}
void gRotation::HandleNewMatch() {
    std::cerr << "match!\n";
#ifdef HAVE_LIBRUBY
    gMatchEventRuby::DoMatchEvents();
#endif
}

#ifdef HAVE_LIBRUBY

static tCallbackRuby *roundEventRuby_anchor;
gRoundEventRuby::gRoundEventRuby()
        :tCallbackRuby(roundEventRuby_anchor)
{
}

void gRoundEventRuby::DoRoundEvents()
{
    Exec(roundEventRuby_anchor);
}

static tCallbackRuby *matchEventRuby_anchor;

gMatchEventRuby::gMatchEventRuby()
        :tCallbackRuby(matchEventRuby_anchor)
{
}

void gMatchEventRuby::DoMatchEvents()
{
    Exec(matchEventRuby_anchor);
}
#endif


