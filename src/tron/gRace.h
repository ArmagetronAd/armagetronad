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
#include "gZone.h"
#include "gArena.h"

class gRacePlayer
{
    public:
        gRacePlayer(ePlayerNetID *player);                   //!<    create new instance

        static bool PlayerExists(tString username);          //!<    checks if player's race data exists
        static gRacePlayer *GetPlayer(tString username);     //!<    gets that player's race data

        void NewCycle(gCycle *bike)                          //!<    create a new cycle
        {
            if (bike)
                cycle_ = bike;
        }

        //!  if the player dies, their cycle's data is erased
        void DestroyCycle() { if (cycle_) cycle_ = NULL; }

        //! if the player leaves the game, erase their data
        void ErasePlayer();

        //!  drops the number of chances left
        void DropChances()
        {
            if (chances_ > 0)
            {
                chances_--;
            }
            else
            {
                chances_ = 0;
            }
        }

        bool Finished() { return hasFinished_; }
        void SetFinished (bool finished) { hasFinished_ = finished; }

        REAL Time() { return time_; }
        void SetTime(REAL newTime) { time_ = newTime; }

        int Score() { return score_; }
        void SetScore(int newScore) { score_ = newScore; }

    private:
        bool hasFinished_;

        REAL time_;
        int score_;

        ePlayerNetID *player_;

        gCycle *cycle_;

        eCoord position_;
        eCoord direction_;

        int chances_;

    public:
        ePlayerNetID    *Player() { return player_; }                  //!<  player's user
        gCycle          *Cycle() { return cycle_; }                    //!<  player's cycle
        eCoord          SpawnPosition() { return position_; }          //!<  spawn position
        eCoord          SpawnDirection() { return direction_; }        //!<  spawn direction
        int             Chances() { return chances_; }                 //!<  player's chances to spawn to race one again
};

class gRace
{
    public:
        static void ZoneHit( ePlayerNetID *player );                    //!> called when a cycle hits a win zone
        static void Sync( int alive, int ai_alive, int humans );        //!> update race state, called every second
        static bool Done();                                             //!> returns true whether round time is over
        static void Reset();                                            //!> reset time and values

        static void RaceChat(ePlayerNetID *player, tString command, std::istream &s);

        //!> returns the race winner
        static eTeam *Winner();

    private:
        static bool   firstArrived_;

        static int     countDown_;
        static bool    roundFinished_;
        static bool    winnerDeclared_;

        static int     finishPlace_;
        static REAL    firstFinishTime_;
        static tString firstToArive_;
};

class gRaceScores
{
    public:
        gRaceScores(tString UserName);

        static gRaceScores *GetPlayer(tString name);

        //!> Checks if that log name exists within the list
        static bool CheckPlayer(tString name);

        //!> Adds the score and replace the time if lower than before
        static void Add(gRacePlayer *racePlayer, bool finished = true);

        //!> Reads in the data in the current map; name, score, time
        static void Read();

        //!> Writes the stored data to the current map's txt file
        static void Write();

        //!> Resets the scores for the next map
        static void Reset();

        static void OutputStart();      //!< the ranks to display at start of round
        static void OutputEnd();        //!< the ranks to display at end of round

        static void Sort();                                                                     //!> Sorts out by ordering Score (Highest - Lowest) and Time (Lowest - Highest)

        tString Name() { return userName_; }
        REAL Time() { return time_; }
        int Played() { return played_; }

        void SetName(tString name) { userName_ = name; }
        void SetTime(REAL newTime) { time_ = newTime; }
        void SetPlayed(int newPlayed) { played_ = newPlayed; }

    private:
        tString userName_;      // logged name
        REAL    time_;          // best time
        int     played_;        // number of times finished map

        static void Switch(int i, int j);                                                       //!> Switches the i and j
        static bool InOrder(int i, int j);                                                      //!> Checks if they are in order
};

extern bool sg_RaceTimerEnabled;
extern int sg_RaceEndDelay;
extern int sg_raceTryoutsNumber;
extern int sg_scoreRaceComplete;

extern tString sg_currentMap;

#endif
