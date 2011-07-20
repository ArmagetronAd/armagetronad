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

#ifndef ArmageTron_SPAWN_H
#define ArmageTron_SPAWN_H

#include "eCoord.h"
#include <vector>

class gSpawnPoint{
    friend class gArena;

    int   id;
    eCoord location,direction;
    REAL  lastTimeUsed;
    unsigned numberOfUses;

    std::vector<eCoord> subPositions;
    std::vector<eCoord> subDirections;

public:
    gSpawnPoint(const eCoord &loc,const eCoord &dir);
    ~gSpawnPoint(){}

    //enters valid spawn eCoordinates and direction in loc and dir
    void Spawn(eCoord &loc,eCoord &dir);

    void AddSubSpawn(eCoord const &loc, eCoord const &dir);

    // estimates the danger of spawning here (0: no problem, 10: certain death)
    REAL Danger();

    // mark it unused (for match start)
    void Clear();

    REAL LastTimeUsed() { return (lastTimeUsed); }
};


#endif
