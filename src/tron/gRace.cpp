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

#include "gRace.h"
#include "eTimer.h"
#include "tString.h"
#include "tDirectories.h"

#include <string>

bool sg_RaceTimerEnabled = false;
bool restrictRaceTimerEnabled( bool const &newValue )
{
    if (sn_GetNetState() == nSERVER )
    {
        return true;
    }
    else
    {
        sn_ConsoleOut(tOutput( "$player_racetimerenabled_restrict"), 0);
        return false;
    }
}
static tSettingItem<bool> sg_RaceTimerEnabledConf( "RACE_TIMER_ENABLED", sg_RaceTimerEnabled, restrictRaceTimerEnabled );

int sg_RaceEndDelay = 25;
static tSettingItem<int> sg_RaceEndDelayConf( "RACE_END_DELAY", sg_RaceEndDelay );

int sg_scoreRaceComplete = 5;
static tSettingItem<int> sg_scoreRaceCompleteConf( "SCORE_RACE", sg_scoreRaceComplete );




//! STATIC VARIABLES
std::multimap <REAL, gRace::Goal>  gRace::goals_;
std::vector <gWinZoneHack*> gRace::winZones_;

bool gRace::firstArrived_ = false;
int  gRace::countDown_ = -1;
bool gRace::roundFinished_ = false;
bool gRace::winnerDeclared_ = false;


//! ZONE HIT
void gRace::ZoneHit( ePlayerNetID * player )
{
    if ( player && !player->raceArrived && !roundFinished_ && player->Object() != NULL &&  player->Object()->Alive() )
    {
        REAL time = se_GameTime();
        player->raceArrived = true;
        player->raceTime = time;

        if ( !firstArrived_ )
            firstArrived_ = true;

        tOutput win, lose;
        win << "$player_reach_race";
        player->AddScore( sg_scoreRaceComplete, win, lose );
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans )
{
    if ( alive == 0 )                                   // close the round if no human is alive
        roundFinished_ = true;

    if ( firstArrived_ || ( alive == 1 && ai_alive == 0 && humans > 1 ))     // start counter when someone arrives or when only 1 is alive but not alone
    {
        int arrivedAlive = 0;
        for ( int i = 0; i < se_PlayerNetIDs.Len(); i ++ )          // count who reached the end and is alive
        {
            ePlayerNetID * p = se_PlayerNetIDs[i];
            if ( ( p->raceArrived /*|| p->GetRawAuthenticatedName() == ""*/) && p->IsHuman() && p->Object() != NULL )
            {
                if ( p->Object()->Alive() )
                    arrivedAlive ++;
            }
        }

        if ( arrivedAlive == alive )                // close the round when all alive humans are arrived
            roundFinished_ = true;

        if ( sg_RaceEndDelay < 0 )                  // freestyle mode
        {
            if ( sg_RaceEndDelay < -1 )
                sg_RaceEndDelay = -1;
        }
        else                                        // countdown mode
        {
            if ( countDown_ < 0 )                   // not started yet
                countDown_ = sg_RaceEndDelay + 1;

            else if ( countDown_ == 0 )
                roundFinished_ = true;

            if ( !roundFinished_ )
            {
                countDown_ --;

                tOutput message;
                message << countDown_ << "                    ";
                sn_CenterMessage( message );
            }
        }
    }




    if ( roundFinished_ && !winZones_.empty() )
    {
        for ( int i = 0; i < winZones_.size(); i ++)
        {
            winZones_[i]->Vanish( 1.0 );
        }
        winZones_.clear();
    }
}

//! DONE
bool gRace::Done()
{
    return roundFinished_;
}

//! ADD GOAL
void gRace::AddGoal( REAL & time, const tColoredString & colName, const tString & name/*, const tString & authName */)
{
    Goal goal;
    goal.colName = colName;
    goal.name = name;
//    goal.authName = authName;
    goals_.insert ( std::pair<REAL, Goal> ( time, goal ) );
}

//! NEW ZONE
void gRace::NewZone( gWinZoneHack * winZone )
{
    winZones_.push_back( winZone );
}

//! RESET
void gRace::Reset()
{
    firstArrived_ = false;
    countDown_ = -1;
    roundFinished_ = false;
    winnerDeclared_ = false;

    winZones_.clear();
    goals_.clear();

    std::ofstream o;
    if ( tDirectories::Var().Open( o, "race_temp.txt" ) )
        o << "";
}

//! TIME TO STRING
static tString sg_TimeToString ( REAL time )
{
    int seconds = time;
    REAL ms = time - seconds;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    tString ret;
    ret << minutes << "'";

    if ( seconds < 10 )
        ret << "0";
    ret << seconds << '"';

    if ( ms < 0.1 )
        ret << "0";
    if ( ms < 0.01 )
        ret << "0";
    ret << int( ms * 1000 );

    return ret;
}

//! STRIP SPACES
static void ReplaceSpaces( std::string & str )
{
    while ( true )
    {
        size_t pos = str.find(' ');
        if ( pos == std::string::npos )
            break;
        else
            str.replace( pos, 1, "\\_" );
    }
}

//! END
void gRace::End()
{
    if ( goals_.empty() )
        return;

    tString raceTemp( "" );

    std::multimap<REAL, Goal>::iterator it;

    for ( it = goals_.begin(); it != goals_.end(); it ++ )      // print times
    {
        REAL time = it->first;
        std::string colName ( it->second.colName );
        std::string name ( it->second.name );
        //std::string authName ( it->second.authName );
        int len = name.size();

        // replace spaces with '\_' for race_temp.txt
        std::string tempName = name;
        ReplaceSpaces( tempName );
        std::string tempColName = colName;
        ReplaceSpaces( tempColName );
        //std::string tempAuthName = authName;

        raceTemp << tempName << " " << time << " " << tempColName /*<< " " << tempAuthName*/ << "\n";

        //print time results immediately
        /*for ( int i = 0; i < 16 - len; i ++ )
            message << " ";
        message << colName << "0xRESETT: " << sg_TimeToString( time ) << "\n";
        */
	//sn_ConsoleOut( "COLLAPSE_ZONE\n" );
    }
    //sn_ConsoleOut( message );

    std::ofstream o;
    if ( tDirectories::Var().Open( o, "race_temp.txt" ) )
        o << raceTemp;

    tOutput ladderLog;
    ladderLog << "RACE_DONE\n";
    se_SaveToLadderLog( ladderLog );
}

//! WINNER
eTeam * gRace::Winner()
{
    if ( winnerDeclared_ )
        return NULL;

    eTeam * winner = NULL;
    REAL bestTime = 0;

    for ( int i = 0; i < se_PlayerNetIDs.Len(); i ++ )
    {
        ePlayerNetID * p = se_PlayerNetIDs[i];

        if ( p->raceArrived )
        {
            p->raceArrived = false;

            AddGoal( p->raceTime, p->GetColoredName(), p->GetName()/*, p->GetRawAuthenticatedName() */);

            if ( p->raceTime < bestTime || bestTime == 0 )
            {
                bestTime = p->raceTime;
                winner = p->CurrentTeam();
            }
        }
    }
    winnerDeclared_ = true;

    return winner;
}

