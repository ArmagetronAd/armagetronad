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

#include "gArena.h"
#include "gRace.h"
#include "gGame.h"
#include "eTimer.h"
#include "tString.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "eAdvWall.h"
#include "gSpawn.h"

#include <string>

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

bool sg_RaceTimerEnabled = false;
static tSettingItem<bool> sg_RaceTimerEnabledConf( "RACE_TIMER_ENABLED", sg_RaceTimerEnabled );

int sg_RaceEndDelay = 60;
static tSettingItem<int> sg_RaceEndDelayConf( "RACE_END_DELAY", sg_RaceEndDelay );

int sg_scoreRaceComplete = 10;
int RacingScore = sg_scoreRaceComplete;
static tSettingItem<int> sg_scoreRaceCompleteConf( "SCORE_RACE", sg_scoreRaceComplete );

int sg_scoreRaceDeplete = 1;
static tSettingItem<int> sg_scoreRaceDepleteConf( "RACE_SCORE_DEPLETE", sg_scoreRaceDeplete);

enum gRaceScoreType
{
    gRACESCORE_NO_SORTING = 0,
    gRACESCORE_BEST_SCORE = 1,
    gRACESCORE_BEST_TIME = 2
};
tCONFIG_ENUM( gRaceScoreType );

static gRaceScoreType racescoretype = gRACESCORE_NO_SORTING;
static tSettingItem<gRaceScoreType> conf_raceScoretype("RACE_SCORE_TYPE",racescoretype);

// will be used later... no idea when...
bool restrictlocation(tString const &newValue)
{
    if (newValue == "") return false;
    else return true;
}
static tString sg_raceloglocation("NULL");
static tSettingItem<tString> sg_raceLogLocationConf("RACE_HIGHSCORES_LOCATION", sg_raceloglocation, &restrictlocation);

/* Causes the game to crash if a player enters in middle of round.
int sg_raceTryoutsNumber = 5;
bool restrictRaceTryouts(int const &newValue)
{
    if (newValue == 0 || newValue >= 0) return true;
    else if (newValue < 0)
    {
        tOutput o;
        o.SetTemplateParameter(1, 0);
        o << "$race_tryouts_lower";
        con << o << "\n";
        return false;
    }
}
static tSettingItem<int> sg_raceTryoutsNumberConf("RACE_TRYOUTS", sg_raceTryoutsNumber, &restrictRaceTryouts);
*/

tString sg_currentMap("");
static tString CurrentMapName("");

/* to check the correct map name
static void sg_currentMapLoad(std::istream &s)
{
    tString CurrentMapStr(sg_currentMap);
    tString ClrString("");
    int linenum, rlinenum;
    linenum = 0;
    while(linenum != -1)
    {
        rlinenum = linenum;
        linenum = CurrentMapStr.StrPos(linenum+1, "/");

        /* used for testing to get the map name only
        ClrString << "Current number is " << linenum << "\n";
        sn_ConsoleOut(ClrString);


        if (linenum == -1)
        {
            ClrString << "Current map name ";
            ClrString << CurrentMapStr.SubStr(rlinenum+1, (CurrentMapStr.Len() - rlinenum+1));
            CurrentMapName = CurrentMapStr.SubStr(rlinenum+1, (CurrentMapStr.Len() - rlinenum+1));
            CurrentMapName << ".txt";
            ClrString << "\n";
            sn_ConsoleOut(ClrString);
        }
    }
}
//! GET CURRENT MAP NAME ONLY
static tConfItemFunc sg_currentMapConf("MAP_NAME", &sg_currentMapLoad);
*/

//! STATIC VARIABLES
std::multimap <REAL, gRace::Goal>  gRace::goals_;
std::vector <gWinZoneHack*> gRace::winZones_;
std::vector <gDeathZoneHack*> gRace::deathZones_;

bool gRace::firstArrived_ = false;
int  gRace::countDown_ = -1;
bool gRace::roundFinished_ = false;
bool gRace::winnerDeclared_ = false;

tArray<tString> gRaceScores::raceRealName;
tArray<tString> gRaceScores::raceUserName;
tArray<int> gRaceScores::raceScore;
tArray<REAL> gRaceScores::raceTime;

void gRaceScores::Switch(int i, int j)
{
    Swap(raceUserName[i], raceUserName[j]);
    Swap(raceScore[i], raceScore[j]);
    Swap(raceTime[i], raceTime[j]);
    Swap(raceRealName[i], raceRealName[j]);
}

bool gRaceScores::InOrder(int i,int j)
{
    if (racescoretype == gRACESCORE_BEST_TIME)
    {
        if (raceTime[i] == 0) return false;
        if (raceTime[j] == 0) return true;
        else return (raceTime[i] <= raceTime[j]);
    }
    else if (racescoretype == gRACESCORE_BEST_SCORE) return (raceScore[i] >= raceScore[j]);
}

void gRaceScores::Sort()
{
    for(int i=1;i<raceUserName.Len();i++)
        for(int j=i;j>=1 && !InOrder(j-1,j);j--)
            Switch(j,j-1);
}

void gRaceScores::Add(tString RealName, tString UserName, int WinScore, REAL reachTime, bool arrived)
{
    int i, j;
    bool found;
    found = false;
    i = 0;
    j = raceUserName.Len();
    while(i < j)
    {
        if (raceUserName[i] == UserName)
        {
            found = true;
            break;
        }
        i++;
    }
    if (found == true)
    {
        raceRealName[i] = RealName;
        raceScore[i] += WinScore;
        if (arrived == true)
        {
            if (reachTime < raceTime[i]) raceTime[i] = reachTime;
        }
        else if (arrived == false)
        {
            if (raceTime[i] == 0) raceTime[i] = reachTime;
        }

        /* Test to see if the scores are actually being recorded in the arrays
        tString message;
        message << raceName[i] << " " << raceScore[i] << " " << raceTime[i] << "\n";
        sn_ConsoleOut(message);
        */
    }
    else
    {
        raceUserName.Insert(UserName);
        i = 0;
        j = raceUserName.Len();
        while(i < j)
        {
            if (raceUserName[i] == UserName)
            {
                raceRealName[i] = RealName;
                raceScore[i] = WinScore;
                if (arrived == true) raceTime[i] = reachTime;
                else if (arrived == false) raceTime[i] = 0;

                /* Test to see if the scores are actually being recorded in the arrays
                tString message;
                message << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << "\n";
                sn_ConsoleOut(message);
                */

                break;
            }
            i++;
        }
    }
}

void gRaceScores::Read()
{
    tString CurrentMapStr(sg_currentMap);
    int linenum, rlinenum;
    linenum = 0;
    while(linenum != -1)
    {
        rlinenum = linenum;
        linenum = CurrentMapStr.StrPos(linenum+1, "/");

        if (linenum == -1)
        {
            CurrentMapName = CurrentMapStr.SubStr(rlinenum+1, (CurrentMapStr.Len() - (rlinenum+1)));
            CurrentMapName << ".txt";
        }
    }

    tString Input("");
    if (sg_raceloglocation == "" || sg_raceloglocation == "NULL")
    {
        tString line, lines, linet;
        tString Message;
        Input << "race_scores/" << CurrentMapName;
        tTextFileRecorder r(tDirectories::Var(), Input);
        int i = 0;
        linenum = 0;
        while (!r.EndOfFile())
        {
            std::stringstream s(r.GetLine());

            s >> line;
            lines = line;
            lines.ReadLine(s, true);
            //sn_ConsoleOut(lines);
            if (line != "" && lines != "")
            {
                linenum = 0;
                rlinenum = linenum;
                linenum = line.StrPos(linenum, " ");
                raceUserName[i] = line.SubStr(rlinenum, linenum);
                //sn_ConsoleOut(line);

                linenum = 0;
                rlinenum = linenum;
                linenum = lines.StrPos(linenum, " ");
                raceScore[i] = atoi(lines.SubStr(rlinenum, linenum));
                //Message << "Race Score point: " << linenum << "\n";

                rlinenum = linenum+1;
                linenum = lines.StrPos(rlinenum, " ");
                if (linenum == -1)
                {
                    raceTime[i] = atof(lines.SubStr(rlinenum, (lines.Len() - rlinenum)));
                }
                else
                {
                    raceTime[i] = atof(lines.SubStr(rlinenum, linenum));
                }
                //Message << "Time Secto point: " << linenum << "\n";

                if (linenum == -1)
                {
                    raceRealName[i] = "";
                }
                else
                {
                    rlinenum = linenum+1;
                    raceRealName[i] = lines.SubStr(rlinenum, (lines.Len() - rlinenum));
                }
                //Message << rlinenum << " " << (lines.Len() - rlinenum) << "\n";
                //Message << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << " " << raceRealName[i] << "\n";
                //sn_ConsoleOut(Message);
                //*/
                line = "";
                lines = "";
                /*Message = "";
                Message << line << "" << lines << "\n";
                Message << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << " " << raceRealName[i] << "\n";
                sn_ConsoleOut(Message);*/
            }
            i++;
        }
    }
    else
    {
        tString line("");
        Input << sg_raceloglocation << "/" << CurrentMapName;
        //tTextFileRecorder r(tDirectories::Var(), CurrentMapName);
        //if ( tDirectories::Var().Open(r, Input, std::ios::in) ) {
        std::ifstream r;
        r.clear();
        r.open(Input);
        int i = 0;
        if (r.is_open())
        {
            while (!r.eof())
            {
                //std::stringstream s(r.GetLine());
                std::string s;
                getline(r, s);
                line << s;
                if (line != "")
                {
                    /*linenum = 0;
                    linenum = line.StrPos(0, "\n");
                    line = line.SubStr(0, linenum);
                    tString message;
                    message << linenum;
                    sn_ConsoleOut(message);*/

                    linenum = 0;
                    rlinenum = linenum;
                    linenum = line.StrPos(rlinenum, " ");
                    raceUserName[i] = line.SubStr(rlinenum, linenum);

                    rlinenum = linenum+1;
                    linenum = line.StrPos(rlinenum, " ");
                    raceScore[i] = atoi(line.SubStr(rlinenum, linenum));

                    rlinenum = linenum+1;
                    linenum = line.StrPos(rlinenum, " ");
                    raceTime[i] = atof(line.SubStr(rlinenum, (line.Len() - rlinenum)));

                    /*line << "\n";
                    sn_ConsoleOut(line);*/
                    line = "";

                    i++;
                }
            }
            r.close();
        }
    }
}

void gRaceScores::Write()
{
    tString Output("");

    if (racescoretype != gRACESCORE_NO_SORTING) gRaceScores::Sort();
    /*
    linenum = 0;
    while (linenum != -1)
    {
        rlinenum = linenum;
        linenum = tDirectories::Var().GetPaths().StrPos(linenum+1, " - ");
        if (linenum == -1)
        {
            Output = tDirectories::Var().GetPaths().SubStr(rlinenum+3, (tDirectories::Var().GetPaths().Len() - (rlinenum+3)));
        }
    }
    linenum = Output.StrPos(0, "\n");
    Output = Output.SubStr(0, linenum);*/
    if (sg_raceloglocation == "" || sg_raceloglocation == "NULL" || sg_raceloglocation == NULL)
    {
        Output << /*tDirectories::Var().GetPaths()*/"race_scores/" << CurrentMapName;

        if (raceUserName.Len() > 0)
        {
            std::ofstream w;
            if ( tDirectories::Var().Open(w, Output) ) {
                for (int i=0; i < raceUserName.Len(); i++)
                {
                    w << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << " " << raceRealName[i] << "\n";
                    /*tString message;
                    message << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << "\n";
                    sn_ConsoleOut(message);*/
                }
            }
            /*
            if (w.is_open())
            {
                for (int i=0; i < raceName.Len(); i++)
                {
                    w << raceName[i] << " " << raceScore[i] << " " << raceTime[i] << "\n";
                }
                w.close();
            }
            else sn_ConsoleOut(Output);
            */
        }
    }
    else
    {
        Output << sg_raceloglocation << "/" << CurrentMapName;
        if (raceUserName.Len() > 0)
        {
            std::ofstream w;
            w.open(Output);
            if (w.is_open())
            {
                for (int i=0; i < raceUserName.Len(); i++)
                {
                    if (raceUserName[i] != "") w << raceUserName[i] << " " << raceScore[i] << " " << raceTime[i] << " " << raceRealName[i] << "\n";
                }
                w.close();
            }
            else
            {
                Output << " cannot be accessed or doesn't exist. Please check if this is the correct path.\n";
                sn_ConsoleOut(Output);
            }
        }
    }//*/
}

void gRaceScores::Reset()
{
    /*
    if (raceName.Len() > 0)
    {
        for (int i=0; i < (raceName.Len()-1); i++)
        {
            raceName.RemoveAt(i);
            raceScore.RemoveAt(i);
            raceTime.RemoveAt(i);
        }
    }
    */
    raceUserName = NULL;
    raceScore = NULL;
    raceTime = NULL;
    raceRealName = NULL;
}

//! ZONE HIT
void gRace::ZoneHit( ePlayerNetID * player )
{
    if ( player && !player->raceArrived && !roundFinished_ && player->Object() != NULL &&  player->Object()->Alive() )
    {
        REAL time = se_GameTime();
        player->raceArrived = true;
        player->raceTime = time;

        if ( !firstArrived_ )
        {
            firstArrived_ = true;
            //player->SetRubber(player, 100);
        }

        REAL reachTime = time;
        gRaceScores::Add(player->GetName() , player->GetUserName(), RacingScore, reachTime, true);

        tOutput win; //, lose;
        //win << "$player_reach_race";*/

        /*
        tColoredString ColoredString;
        ColoredString << player->GetColoredName();
        ColoredString << tColoredString::ColorString( 1,1,1 );
        ColoredString << " finished with ";
        ColoredString << tColoredString::ColorString( 0,1,1 );
        ColoredString << RacingScore;
        ColoredString << tColoredString::ColorString( 1,1,1 );
        ColoredString << " points in ";
        ColoredString << time << " seconds\n";
        */

        win.SetTemplateParameter(1, player->GetColoredName() );
        win.SetTemplateParameter(2, RacingScore);
        win.SetTemplateParameter(3, player->raceTime);
        win << "$player_reach_race";

        player->AddScore( RacingScore, win, "" );
        if (RacingScore > 1) RacingScore -= sg_scoreRaceDeplete;
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans, eGrid *Grid, gArena & Arena)
{
    /* RACE_TRYOUTS code
    for (int i = 0; i < se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID * p = se_PlayerNetIDs[i];
        if ( (p->raceArrived == false) && (p->raceTryouts > 0) && (!p->Object()->Alive()) && (roundFinished_ == false))
        {
            eCoord pos,dir;
            /*if ( p->Object() )
            {
                dir = p->Object()->Direction();
                pos = p->Object()->Position();
                eWallRim::Bound( pos, 1 );
                eCoord displacement = pos - p->Object()->Position();
                if ( displacement.NormSquared() > .01 )
                {
                    dir = displacement;
                    dir.Normalize();
                }
            }

            else*
                Arena.LeastDangerousSpawnPoint()->Spawn( pos, dir );

            gCycle * Cycle = new gCycle( Grid, pos, dir, p );
            p->ControlObject(Cycle);

            p->raceTryouts--;

            tOutput o;
            o.SetTemplateParameter(1, p->raceTryouts);
            o << "$race_tries_left";
            sn_ConsoleOut(o, p->Owner());

            alive++;
        }
    }
    */

    if ( alive == 0 ) // close the round if no human is alive
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
                message << "0xff7777" << countDown_ << "                    ";
                sn_CenterMessage( message );
            }
        }
    }




    if ( roundFinished_ && !winZones_.empty() )
    {
        for ( int i = 0; i < winZones_.size(); i ++)
        {
            winZones_[i]->Vanish( 0.5 );
        }
        winZones_.clear();
    }
    if ( roundFinished_ && !deathZones_.empty())
    {
        for (int j = 0; j < deathZones_.size(); j++)
        {
            deathZones_[j]->Vanish( 0.5 );
        }
        deathZones_.clear();
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

//! NEW WIN ZONE
void gRace::NewWinZone( gWinZoneHack * winZone )
{
    winZones_.push_back( winZone );
}

//! NEW DEATH ZONE
void gRace::NewDeathZone( gDeathZoneHack * deathZone)
{
    deathZones_.push_back(deathZone);
}
//! RESET
void gRace::Reset()
{
    if (roundFinished_ == true)
    {
        for (int x=0; x < se_PlayerNetIDs.Len(); x++)
        {
            ePlayerNetID *p = se_PlayerNetIDs[x];
            if (!p->raceArrived)
            {
                gRaceScores::Add(p->GetName(), p->GetUserName(), 0, 0, false);
                p->raceArrived = true;
            }
        }
    }

    firstArrived_ = false;
    countDown_ = -1;
    roundFinished_ = false;
    winnerDeclared_ = false;
    RacingScore = sg_scoreRaceComplete;

    winZones_.clear();
    deathZones_.clear();
    goals_.clear();

    for (int i=0; i < se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];
        if (p->raceArrived) p->raceArrived = false;
    }

    std::ofstream o;
    if ( tDirectories::Var().Open( o, "race_temp.txt" ) )
        o << "";
    gRaceScores::Write();
    gRaceScores::Reset();
    gRaceScores::Read();
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
            //p->raceTryouts = sg_raceTryoutsNumber;

            AddGoal( p->raceTime, p->GetColoredName(), p->GetName() /*, p->GetRawAuthenticatedName() */);

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

