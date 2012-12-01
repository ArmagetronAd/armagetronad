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

#ifndef ArmageTron_ARENA_H
#define ArmageTron_ARENA_H

#include "tList.h"
#include "eCoord.h"


class eGrid;
class gSpawnPoint;
class gParser;

// the fighting arena
class gArena{
    friend class gSpawnPoint;
    friend class gParser;

public:
    void NewSpawnPoint(gSpawnPoint *sp);

    gArena();
    virtual ~gArena();

    // draw the gArena
    virtual void PrepareGrid(eGrid *grid, gParser *aParser);

    // get a random position
    virtual eCoord		GetRandomPos( REAL factor ) const;

    // find the best gSpawnPoint
    gSpawnPoint * LeastDangerousSpawnPoint();

    // find the closes gSpawnPoint
    gSpawnPoint * ClosestSpawnPoint(eCoord loc);

    // access the size multiplier
    static float	SizeMultiplier();
    static void     SetSizeMultiplier(float mult);
    static float    GetSizeMultiplier();

    void RemoveAllSpawn();

public:
    tList<gSpawnPoint> spawnPoints; //!< the list of active spawn points
};

extern gArena Arena;

#endif
