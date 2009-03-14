/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Portions Copyright (C) 2008  Luke Dashjr (luke@dashjr.org)

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

#ifndef ArmageTron_zFortress_H
#define ArmageTron_zFortress_H

#include "zone/zZone.h"

#include <vector>

class eTeam;
class gCycle;
class gParser;

//! fortress zone: belongs to a team, enemy players who manage to stay inside win the round
class zFortressZone: public zZone
{
public:
    static zZone* create(eGrid*grid, std::string const & type) { return new zFortressZone(grid); };
    zFortressZone(eGrid *grid);                                   //!< local constructor
    ~zFortressZone();                                             //!< destructor

    void setupVisuals(gParser::State_t &);
    void readXML(tXmlParser::node const &);

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnConquest();                         //!< called when the zone gets conquered
    virtual void CheckSurvivor();                      //!< checks for the only surviving zone
    virtual void OnRoundBegin();                       //!< called on the beginning of the round
    virtual void OnRoundEnd();                         //!< called on the end of the round

    void ZoneWasHeld();                                //!< call when the zone was held as long as possible with the set game rules

    static void CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, zFortressZone * & farthest ); //!< counts the zones belonging to the given team.

    REAL conquered_;                       //!< conquest status; zero if it is free, 1 if it has been completely conquered by the enemy
    int enemiesInside_;                     //!< count of enemies currently inside the zone
    int ownersInside_;                      //!< count of owners currently inside the zone

    tColoredString enemyPlayerName_;        //!< name of the first enemy player that was inside us
    tColoredString teamPlayerName_;         //!< name of the first team player that was inside us

    bool onlySurvivor_;                     //!< flag set if this zone is the only survivor

    typedef std::vector< tJUST_CONTROLLED_PTR< eTeam > > TeamArray;
    TeamArray enemies_;                     //!< list of teams that currently have a player in the zone
    REAL lastEnemyContact_;                 //!< last time an enemy player was in the zone

    REAL teamDistance_;                     //!< distance to the closest member of the owning team

    bool touchy_;                           //!< flag set when the zone is "touchy", which makes it get conquered on the slightest enemy touch

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

#endif
