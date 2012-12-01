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
#include "gArena.h"

#include <vector>
#include <map>


class gRace {

public:
    static void     ZoneHit     ( ePlayerNetID * player );                      //!> called when a cycle hits a win zone
    static void     Sync        ( int alive, int ai_alive, int humans );        //!> update race state, called every second
    static bool     Done        ();                                             //!> returns true whether round time is over
    static void     NewWinZone     ( gWinZoneHack * winZone );                  //!> handle all win zones to let them vanish
    static void     NewDeathZone    ( gDeathZoneHack * deathZone);              //!> handle all death zones to let them vanish
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
    static std::vector <gDeathZoneHack*> deathZones_;

    static bool                         firstArrived_;
    static int                          countDown_;
    static bool                         roundFinished_;
    static bool                         winnerDeclared_;
    static int                          finishPlace_;
    static REAL                         firstFinishTime_;
    static tString                      firstToArive_;
};

class gRaceScores
{
public:
    gRaceScores(tString UserName);
    static gRaceScores * GetPlayer(tString name);
    static bool CheckPlayer(tString name);                                                  //!> Checks if that log name exists within the list
    static void Add(tString UserName, tString RealName, int WinScore, REAL reachTime);      //!> Adds the score and replace the time if lower than before
    static void Read();                                                                     //!> Reads in the data in the current map; name, score, time
    static void Write();                                                                    //!> Writes the stored data to the current map's txt file
    static void Reset();                                                                    //!> Resets the scores for the next map

    static void OutputStart();      //!< the ranks to display at start of round
    static void OutputEnd();        //!< the ranks to display at end of round

public:
    tString user_name;      // logged name
    tString real_name;      // screen name
    int     score;          // best score
    REAL    time;           // best time

    static void Switch(int i, int j);                                                       //!> Switches the i and j
    static void Sort();                                                                     //!> Sorts out by ordering Score (Highest - Lowest) and Time (Lowest - Highest)
    static bool InOrder(int i, int j);                                                      //!> Checks if they are in order
};

class gRacePlayer
{
    public:
        gRacePlayer(ePlayerNetID *p);
        static bool PlayerExists(ePlayerNetID *p);

        void NewCycle(gCycle *bike) {cycle = bike;}
        void DropChances(int dropValue)
        {
            int diff = chances - dropValue;
            if (diff >= chances)
            {
                chances--;
            }
            else
            {
                chances = 0;
            }
        }

    private:
        ePlayerNetID *player;
        gCycle *cycle;
        eCoord position;
        eCoord direction;
        int chances;

    public:
        ePlayerNetID    *Player()const{return player;}                  //!<  player's user
        eNetGameObject  *Cycle()const{return cycle;}                    //!<  player's cycle
        eCoord          SpawnPosition()const{return position;}          //!<  spawn position
        eCoord          SpawnDirection()const{return direction;}        //!<  spawn direction
        int             Chances()const{return chances;}                 //!<  player's chances to spawn to race one again
};

extern bool sg_RaceTimerEnabled;
extern int sg_RaceEndDelay;
extern int sg_raceTryoutsNumber;
extern int sg_scoreRaceComplete;

extern tString sg_currentMap;

#endif

