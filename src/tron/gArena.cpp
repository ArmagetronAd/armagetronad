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

#include "tMemManager.h"

#include "nConfig.h"

#include "gArena.h"

#include "eGrid.h"
#include "eSpawn.h"
#include "gWall.h"
#include "gParser.h"
#include "tRandom.h"
#include "eRectangle.h"
#include "gCycle.h"

static float sizeMultiplier = .5f;
static nSettingItem<float> conf_mult ("REAL_ARENA_SIZE_FACTOR", sizeMultiplier);

static int axes = 4;
static nSettingItemWatched<int> conf_axes ("ARENA_AXES", axes,  nConfItemVersionWatcher::Group_Breaking, 6 );

// formation constants: how much wingmen are placed sideways and backwards
// relative to the leader
static REAL sg_spawnBack = 2.202896;
static REAL sg_spawnSide = 2.75362;
static tSettingItem< REAL > sg_spawnBackConf( "SPAWN_WINGMEN_BACK", sg_spawnBack );
static tSettingItem< REAL > sg_spawnSideConf( "SPAWN_WINGMEN_SIDE", sg_spawnSide );

gSpawnPoint::gSpawnPoint(const eCoord &loc,const eCoord &dir)
:eSpawnPoint(loc, dir)
{}

void gSpawnPoint::FindPos(eCoord &loc, eCoord &dir)
{
    int d,away;
    if (numberOfUses%2==1){
        d=1;
        away=(numberOfUses+1)/2;
    }
    else{
        d=-1;
        away=numberOfUses/2;
    }

    dir = direction;
    loc = location * gArena::SizeMultiplier() - dir.Turn(sg_spawnBack,-d*sg_spawnSide) * away * gCycle::SpeedMultiplier();
}

gArena::gArena():spawnPoints()
{
}

void gArena::NewSpawnPoint(const eCoord &loc,const eCoord &dir){
    gSpawnPoint *s=tNEW(gSpawnPoint) (loc,dir);
    spawnPoints.Add(s,s->id);
}


void gArena::RemoveAllSpawn() {
    while (spawnPoints.Len()){
        gSpawnPoint *s=spawnPoints(0);
        spawnPoints.Remove(s,s->id);
        delete s;
    }
}

gArena::~gArena()
{
    // usually, we would write this:
    // RemoveAllSpawn();

    // but because of a bug in GCC version 3.3.5 to 3.4.x (4.0 is unaffected),
    // we have to dublicate the code of the function.
    while (spawnPoints.Len())
    {
        gSpawnPoint *s=spawnPoints(0);
        spawnPoints.Remove(s,s->id);
        delete s;
    }
}

bool init_grid_in_process;

// from gGame.cpp
void exit_game_objects(eGrid *grid);

eCoord gArena::GetRandomPos(REAL factor) const
{
    tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();
    REAL x = randomizer.Get();
    REAL y = randomizer.Get();
    //  REAL x = REAL(rand())/RAND_MAX;
    //  REAL y = REAL(rand())/RAND_MAX;

    //    return eCoord( sizeMultiplier * 500.0 * ( x * factor + .5f * ( 1 - factor ) ), sizeMultiplier * 500.0 * ( y * factor + .5f * ( 1 - factor ) ) );
    return eWallRim::GetBounds().GetPoint( eCoord( x * factor + .5f * ( 1 - factor ), y * factor + .5f * ( 1 - factor ) ) );
}

void gArena::PrepareGrid(eGrid *grid, gParser *aParser)
{
    RemoveAllSpawn();

    init_grid_in_process=true;
    grid->Create();
    grid->SetWinding(axes);

    aParser->setSizeMultiplier(sizeMultiplier);
    aParser->Parse();

    init_grid_in_process=false;

    int i;
    for(i=0;i<spawnPoints.Len();i++)
        spawnPoints(i)->Clear();

    // update arena bounds
    eWallRim::UpdateBounds();
}

// find the closest gSpawnPoint to a location 
gSpawnPoint * gArena::ClosestSpawnPoint(eCoord pos)
{
    gSpawnPoint *ret=NULL;
    REAL distance = -1;

    for(int i=0;i<spawnPoints.Len();i++)
    {
        // get the distance
        REAL tempDistance = (pos - (spawnPoints(i)->location * SizeMultiplier())).Norm();

        if ((tempDistance < distance) || (distance < 0))
        {
            ret = spawnPoints(i);
            distance = tempDistance;

            //con << "Spawn point chosen: " << spawnPoints(i)->location << " " << pos << '\n';
        }
//        else
            //con << "Spawn point not chosen: " << spawnPoints(i)->location << " " << pos << '\n';
    }

    if (!ret)
        tERR_ERROR("No spawnpoint available!");

    return ret;
} 

// find the best gSpawnPoint
gSpawnPoint * gArena::LeastDangerousSpawnPoint()
{
    REAL mindanger=1E+30;
    gSpawnPoint *ret=NULL;

    for(int i=0;i<spawnPoints.Len();i++)
    {
        REAL newDanger = spawnPoints(i)->Danger();
        if (newDanger < mindanger - EPS )
        {
            ret = spawnPoints(i);
            mindanger = newDanger;
        }
    }

    if (!ret)
    {
        throw tGenericException( "No spawnpoint available." );
    }

#ifdef DEBUG
    //	std::cout << "Spawn at " << ret->location << "\n";
#endif

    return ret;
}

float gArena::SizeMultiplier()
{
    return sizeMultiplier;
}

void gArena::SetSizeMultiplier(float mult)
{
    conf_mult.Set( mult );
}

float gArena::GetSizeMultiplier()
{
    return sizeMultiplier;
}
