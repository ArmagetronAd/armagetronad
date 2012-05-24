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

#ifndef ArmageTron_RACE_H
#define ArmageTron_RACE_H

#include "defs.h"
#include "ePlayer.h"
#include "gWinZone.h"

#include <vector>
#include <map>


class gRace {

public:
    static void     ZoneHit     ( ePlayerNetID * player );                      //!> called when a cycle hits a win zone
    static void     Sync        ( int alive, int ai_alive, int humans );        //!> update race state, called every second
    static bool     Done        ();                                             //!> returns true whether round time is over
    static void     NewZone     ( gWinZoneHack * winZone );                     //!> handle all win zones to let them vanish
    static void     Reset       ();                                             //!> reset time and values
    static void     End         ();                                             //!> print time results

    static eTeam *  Winner      ();                                             //!> returns the race winner
    static void     playerLeft  ( ePlayerNetID * player );                      //!> remove player if stored in goals_

private:
    static void AddGoal( REAL & time, const tColoredString & colName, const tString & name/*, const tString & authName */);      //!> store player's time

private:
    struct Goal {
        tColoredString colName;
        tString name;
        //tString authName;
        int len;
    };

    static std::multimap <REAL, Goal>   goals_;
    static std::vector <gWinZoneHack*>  winZones_;

    static bool                         firstArrived_;
    static int                          countDown_;
    static bool                         roundFinished_;
    static bool                         winnerDeclared_;
};



extern bool sg_RaceTimerEnabled;
extern int sg_RaceEndDelay;

#endif

