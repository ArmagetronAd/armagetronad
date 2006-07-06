/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_LAGCOMPENSATION_H
#define ArmageTron_LAGCOMPENSATION_H

#include "defs.h"

//! class for lag compensation functions in the game code
class eLag
{
public:
    //! call on the server: report that a client is lagging
    static void Report( int client, REAL lag );

    //! call on the server: ask for lag credit to backdate received commands
    static REAL TakeCredit( int client, REAL lag );

    //! call on the server: ask how much lag credit is left
    static REAL Credit( int client );

    //! call on the client: returns the amount of lag that is currently to be compensated by the game code
    static REAL Current();

    //! call on the client: advance time
    static void Timestep( REAL dt );
};

#endif
