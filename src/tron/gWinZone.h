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

#ifndef ArmageTron_WinZone_H
#define ArmageTron_WinZone_H

#include "eNetGameObject.h"

#include "gCycle.h"
#include "rColor.h"

class eTeam;

// all the following zones are hacks until the full zone system is in place

/*
//! base zone: belongs to a team, enemy players who manage to stay inside win the round (will be replaced
class gBaseZoneHack: public gZone
{
public:
    gBaseZoneHack(eGrid *grid, const eCoord &pos );               //!< local constructor
    gBaseZoneHack(nMessage &m);                                   //!< network constructor
    ~gBaseZoneHack();                                             //!< destructor

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnConquest();                         //!< called when the zone gets conquered
    virtual void CheckSurvivor();                      //!< checks for the only surviving zone

    static void CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest ); //!< counts the zones belonging to the given team.

    REAL conquered_;                       //!< conquest status; zero if it is free, 1 if it has been completely conquered by the enemy
    int enemiesInside_;                     //!< count of enemies currently inside the zone
    int ownersInside_;                      //!< count of owners currently inside the zone

    bool onlySurvivor_;                     //!< flag set if this zone is the only survivor

    typedef std::vector< tJUST_CONTROLLED_PTR< eTeam > > TeamArray;
    TeamArray enemies_;                     //!< list of teams that currently have a player in the zone
    REAL lastEnemyContact_;                 //!< last time an enemy player was in the zone

    REAL teamDistance_;                     //!< distance to the closest member of the owning team

    //! possible states
    enum State
    {
        State_Safe,        //!< not yet conquered
        State_Conquering,  //!< conquering in this frame
        State_Conquered    //!< conquered
    };
    State currentState_;   //!< the current state

    REAL lastSync_;        //!< time of the last sync request
};
*/

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos );


#endif
