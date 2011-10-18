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

#ifndef ArmageTron_SPAWN_H
#define ArmageTron_SPAWN_H

#include "eCoord.h"

class eSpawnPoint{

protected:
    int   id;
    eCoord location,direction;
    REAL  lastTimeUsed;
    int   numberOfUses;

public:
    eSpawnPoint(const eCoord &loc,const eCoord &dir);
    virtual ~eSpawnPoint(){}

    //enters valid spawn eCoordinates and direction in loc and dir
    void Spawn(eCoord &loc,eCoord &dir);

    //find a suitable location for the next spawnee
    virtual void FindPos(eCoord &loc,eCoord &dir);

    REAL LastTimeUsed();

    // estimates the danger of spawning here (0: no problem, 10: certain death)
    REAL Danger();

    // mark it unused (for match start)
    void Clear();
};


#endif
