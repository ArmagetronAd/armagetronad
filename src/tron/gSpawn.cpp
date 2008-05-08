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

#include "gSpawn.h"
#include "gArena.h"
#include "gCycle.h"
#include "eTimer.h"
#include "tConfiguration.h"

// formation constants: how much wingmen are placed sideways and backwards
// relative to the leader
static REAL sg_spawnBack = 2.202896;
static REAL sg_spawnSide = 2.75362;
static tSettingItem< REAL > sg_spawnBackConf( "SPAWN_WINGMEN_BACK", sg_spawnBack );
static tSettingItem< REAL > sg_spawnSideConf( "SPAWN_WINGMEN_SIDE", sg_spawnSide );

// number of spawns after which to start wrapping new spawns at the beginning
// without this, respawning eventually starts happening outside of walls...
static int sg_spawnWrap = 9;
static tSettingItem< int > sg_spawnWrapConf( "SPAWN_WRAP", sg_spawnWrap );

gSpawnPoint::gSpawnPoint(const eCoord &loc,const eCoord &dir)
        :id(-1),location(loc),direction(dir),
lastTimeUsed(se_GameTime()-1000000),numberOfUses(0){}

//enters valid spawn coordinates and direction in loc and dir
void gSpawnPoint::Spawn(eCoord &loc,eCoord &dir){
    /*
    if (ArmageTronTimer-lastTimeUsed>100)
      numberOfUses=0;
    */

    lastTimeUsed=se_GameTime();

    if(numberOfUses > 0 && numberOfUses <= subPositions.size()) {
        loc = subPositions[numberOfUses - 1] * gArena::SizeMultiplier();
        dir = subDirections[numberOfUses - 1];
        ++numberOfUses;
        return;
    }

    int d,away;
    int wrappedNumberOfUses = (numberOfUses ? numberOfUses- subPositions.size() : 0) % sg_spawnWrap;

    if (wrappedNumberOfUses%2==1){
        d=1;
        away=(wrappedNumberOfUses+1)/2;
    }
    else{
        d=-1;
        away=wrappedNumberOfUses/2;
    }

    dir=direction;

    loc=location * gArena::SizeMultiplier() - dir.Turn(sg_spawnBack,-d*sg_spawnSide) * away * gCycle::SpeedMultiplier();
    numberOfUses++;
}

// estimates the danger of spawning here (0: no problem, 10: certain death)
REAL gSpawnPoint::Danger(){
    return numberOfUses+(100/(se_GameTime()+10-lastTimeUsed));
}

void gSpawnPoint::Clear(){
    lastTimeUsed=se_GameTime()-1000000;
    numberOfUses=0;
}


void gSpawnPoint::AddSubSpawn(eCoord const &pos, eCoord const &dir){
    subPositions.push_back(pos);
    subDirections.push_back(dir);
}
