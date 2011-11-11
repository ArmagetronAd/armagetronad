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

#include "eSpawn.h"
#include "eTimer.h"
#include "tConfiguration.h"

eSpawnPoint::eSpawnPoint(const eCoord &loc,const eCoord &dir)
        :id(-1),location(loc),direction(dir),
lastTimeUsed(se_GameTime()-1000000),numberOfUses(0){}

//enters valid spawn coordinates and direction in loc and dir
void eSpawnPoint::Spawn(eCoord &loc, eCoord &dir){
    FindPos(loc, dir);

    lastTimeUsed=se_GameTime();
    numberOfUses++;
}
void eSpawnPoint::FindPos(eCoord &loc, eCoord &dir)
{
    loc = location;
    dir = direction;
}

REAL eSpawnPoint::LastTimeUsed()
{
    return lastTimeUsed;
}

// estimates the danger of spawning here (0: no problem, 10: certain death)
REAL eSpawnPoint::Danger(){
    return numberOfUses+(100/(se_GameTime()+10-lastTimeUsed));
}

void eSpawnPoint::Clear(){
    lastTimeUsed=se_GameTime()-1000000;
    numberOfUses=0;
}



