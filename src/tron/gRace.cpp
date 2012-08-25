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
#include "gGame.h"
#include "eTimer.h"
#include "tString.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "eAdvWall.h"
#include "gParser.h"

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

bool sg_raceLogTime = false;
static tSettingItem<bool> sg_raceLogTimeConf("RACE_LOG_TIME", sg_raceLogTime);

enum gRaceScoreType
{
    gRACESCORE_NO_SORTING = 0,
    gRACESCORE_BEST_SCORE = 1,
    gRACESCORE_BEST_TIME = 2
};
tCONFIG_ENUM( gRaceScoreType );

static gRaceScoreType racescoretype = gRACESCORE_NO_SORTING;
bool restrictScoreSortTypes(const gRaceScoreType &newValue)
{
    if ((newValue > gRACESCORE_BEST_TIME) || (newValue < gRACESCORE_NO_SORTING))
    {
        return false;
    }
    return true;
}
static tSettingItem<gRaceScoreType> conf_raceScoretype("RACE_SCORE_TYPE",racescoretype, &restrictScoreSortTypes);

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
int gRace::finishPlace_ = 0;
REAL gRace::firstFinishTime_ = 0;
tString gRace::firstToArive_;

tList<gRaceScores> sn_RaceScores;

gRaceScores::gRaceScores(tString UserName)
{
    this->user_name = UserName;

    sn_RaceScores.Insert(this);
}

gRaceScores * gRaceScores::GetPlayer(tString name)
{
    for (int i=0; i < sn_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sn_RaceScores[i];
        if (rS->user_name == name)
            return rS;
    }
}

bool gRaceScores::CheckPlayer(tString name)
{
    for (int i=0; i < sn_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sn_RaceScores[i];
        if (rS->user_name == name)
            return true;
    }
    return false;
}

void gRaceScores::Switch(int i, int j)
{
    Swap(sn_RaceScores[i], sn_RaceScores[j]);
}

bool gRaceScores::InOrder(int i,int j)
{
    gRaceScores *rSP = sn_RaceScores[i];
    gRaceScores *rSR = sn_RaceScores[j];
    if (racescoretype == gRACESCORE_BEST_TIME)
    {
        if (rSP->time == -1) return false;
        if (rSR->time == -1) return true;
        else return (rSP->time <= rSR->time);
    }
    else if (racescoretype == gRACESCORE_BEST_SCORE) return (rSP->score >= rSR->score);
}

void gRaceScores::Sort()
{
    for(int i=1;i<sn_RaceScores.Len();i++)
        for(int j=i;j>=1 && !InOrder(j-1,j);j--)
            Switch(j,j-1);
}

void gRaceScores::Add(tString UserName, tString RealName, int WinScore, REAL reachTime)
{
    if (CheckPlayer(UserName))
    {
        gRaceScores *rS = GetPlayer(UserName);
        rS->real_name = RealName;
        rS->score += WinScore;
        if (((reachTime < rS->time) && reachTime != -1) || (rS->time == -1 && reachTime != -1))
        {
            rS->time = reachTime;
            tOutput newTime;
            newTime.SetTemplateParameter(1, reachTime);
            newTime << "$player_personal_best_reach_time";

            ePlayerNetID *p = ePlayerNetID::FindPlayerByName(UserName);
            if (p)
            {
                sn_ConsoleOut(newTime, p->Owner());
            }
        }
    }
    else
    {
        gRaceScores *rS = new gRaceScores(UserName);
        rS->real_name = RealName;
        rS->score = WinScore;
        rS->time = reachTime;
        if (reachTime != -1)
        {
            tOutput newTime;
            newTime.SetTemplateParameter(1, reachTime);
            newTime << "$player_personal_best_reach_time";
        }
    }
}

void gRaceScores::Read()
{
    tString CurrentMapStr(sg_currentMap);
    tString CurrentMapName;
    /*int linenum, rlinenum;
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
    }*/

    tString Input, params;
    //Input << "race_scores/" << CurrentMapName;
    Input << "race_scores/" << sg_currentMap << ".txt";
    //tTextFileRecorder r(tDirectories::Var(), Input);
    std::ifstream r;
    if ( tDirectories::Var().Open(r, Input) ) {
        while (!r.eof())
        {
            //std::stringstream s(r.getline());

            std::string sayLine;
            std::getline(r, sayLine);
            std::istringstream s(sayLine);

            params.ReadLine(s, true);

            if (params != "")
            {
                int rLin = 0, pLin = 0;
                int iCount = 1;
                gRaceScores *rS = NULL;
                while (pLin != -1)
                {
                    rLin = pLin + 1;
                    pLin = params.StrPos(rLin, " ");
                    if (iCount == 1)
                    {
                        tString name = params.SubStr(0, pLin);
                        if (!CheckPlayer(name))
                        {
                            rS = new gRaceScores(name);
                        }
                        else
                        {
                            rS = GetPlayer(name);
                        }
                    }
                    else if (iCount == 2)
                    {
                        rS->score = atoi(params.SubStr(rLin, pLin));
                    }
                    else if (iCount == 3)
                    {
                        rS->time = atof(params.SubStr(rLin, pLin));
                    }
                    else if (iCount == 4)
                    {
                        rS->real_name = params.SubStr(rLin, (params.Len() - rLin));
                    }
                    iCount++;
                }
            }
        }
    }
}

void gRaceScores::Write()
{
    tString Output;

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

    //Output << /*tDirectories::Var().GetPaths()*/"race_scores/" << CurrentMapName;
    Output << /*tDirectories::Var().GetPaths()*/"race_scores/" << sg_currentMap << ".txt";

    if (sn_RaceScores.Len() > 0)
    {
        std::ofstream w;
        if ( tDirectories::Var().Open(w, Output) ) {
            for (int i=0; i < sn_RaceScores.Len(); i++)
            {
                gRaceScores *rS = sn_RaceScores[i];
                w << rS->user_name << " " << rS->score << " " << rS->time << " " << rS->real_name << "\n";
            }
        }
    }
}

void gRaceScores::Reset()
{
    for(int i=0; i < sn_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sn_RaceScores[i];
        sn_RaceScores.RemoveAt(i);
        delete rS;
        i -= 1;
    }
}

void gRaceScores::OutputTopTen()
{
    Sort();

    tColoredString Output;

    tColoredString barLine;
    barLine << "0xa00060";
    for (int a=0; a < 63; a++)
    {
        barLine << "#";
    }

    Output << barLine << "\n";

    int iCount = 0;
    if (sn_RaceScores.Len() >= 10)
    {
        iCount = 10;
        Output << "0xffff77Top 10 time records.\n";
    }
    else
    {
        iCount = sn_RaceScores.Len();
        Output << "0xffff77Top " << iCount << " time records available.\n";
    }

    Output << barLine << "\n";

    tColoredString line;
    line << "0xa00060# 0xff6622RANK";
    line.SetPos(10, false);
    line << "0xa00060# 0xff6622PLAYER";
    line.SetPos(30, false);
    line << "0xa00060# 0xff6622SCORE";
    line.SetPos(47, false);
    line << "0xa00060# 0xff6622BEST TIME";
    line.SetPos(63, false);
    line << "0xa00060#";
    line << "\n";

    Output << line;
    Output << barLine << "\n";

    int rank = 1;
    for (int i=0; i < iCount; i++)
    {
        gRaceScores *rS = sn_RaceScores[i];
        tColoredString ret;
        ret << "0xa00060# 0xffff77" << rank;
        ret.SetPos(10, false);
        ret << "0xa00060# 0xe0a0ff" << rS->real_name;
        ret.SetPos(30, false);
        ret << "0xa00060# 0x80ffff" << rS->score;
        ret.SetPos(47, false);
        ret << "0xa00060# 0x80ffff" << rS->time;
        ret.SetPos(63, false);
        ret << "0xa00060#";

        Output << ret << "\n";

        rank++;
    }

    Output << "0xff6622Current Map: 0x00ff44" << sg_currentMap << "\n";
    Output << barLine << "\n";
    sn_ConsoleOut(Output);

    rank = 1;
    for (int a=0; a < sn_RaceScores.Len(); a++)
    {
        gRaceScores *rS = sn_RaceScores[a];
        for (int b=0; b < se_PlayerNetIDs.Len(); b++)
        {
            ePlayerNetID *p = se_PlayerNetIDs[b];
            if (p && (p->GetUserName() == rS->user_name))
            {
                tOutput Message;
                Message.SetTemplateParameter(1, rank);
                Message.SetTemplateParameter(2, rS->time);
                Message << "$player_message_race_rank";
                sn_ConsoleOut(Message, p->Owner());
            }
        }
        rank++;
    }
}

void gRaceScores::RaceCommands(ePlayerNetID *p, std::istream &s, tString command)
{
    tString params;
    if (command == "!race")
    {
        params.ReadLine(s, true);
        int pos = 0;

        tString argument = params.ExtractNonBlankSubString(pos);
        if (argument == "stats")
        {
            tString extraArg = params.ExtractNonBlankSubString(pos);
            if (extraArg == "")
            {
                Sort();
                tColoredString Output;

                tColoredString barLine;
                barLine << "0xa00060";
                for (int a=0; a < 63; a++)
                {
                    barLine << "#";
                }

                Output << barLine << "\n";

                tColoredString line;
                line << "0xa00060# 0xff6622RANK";
                line.SetPos(10, false);
                line << "0xa00060# 0xff6622PLAYER";
                line.SetPos(30, false);
                line << "0xa00060# 0xff6622SCORE";
                line.SetPos(47, false);
                line << "0xa00060# 0xff6622BEST TIME";
                line.SetPos(63, false);
                line << "0xa00060#";
                line << "\n";

                Output << line;
                Output << barLine << "\n";

                int rank = 1;
                for (int i=0; i < sn_RaceScores.Len(); i++)
                {
                    gRaceScores *rS = sn_RaceScores[i];
                    if (rS->user_name == p->GetUserName())
                    {
                        tColoredString ret;
                        ret << "0xa00060# 0xffff77" << rank;
                        ret.SetPos(10, false);
                        ret << "0xa00060# 0xe0a0ff" << rS->real_name;
                        ret.SetPos(30, false);
                        ret << "0xa00060# 0x80ffff" << rS->score;
                        ret.SetPos(47, false);
                        ret << "0xa00060# 0x80ffff" << rS->time;
                        ret.SetPos(63, false);
                        ret << "0xa00060#";

                        Output << ret << "\n";

                        break;
                    }
                    rank++;
                }
                Output << "0xff6622Current Map: 0x00ff44" << sg_currentMap << "\n";
                Output << barLine << "\n";
                sn_ConsoleOut(Output, p->Owner());
            }
            else
            {
                Sort();
                tColoredString Output;

                tArray<gRaceScores *> searchList;
                for (int i=0; i < sn_RaceScores.Len(); i++)
                {
                    gRaceScores *rS = sn_RaceScores[i];
                    if (rS)
                    {
                        tString nametoSearch = ePlayerNetID::FilterName(extraArg);
                        tString searchSource = ePlayerNetID::FilterName(rS->real_name);
                        if (searchSource.Contains(nametoSearch))
                        {
                            searchList.Insert(rS);
                        }
                    }
                }
                if (searchList.Len() == 1)
                {
                    tOutput SendMessage;
                    int rank = 1;
                    gRaceScores *rS = searchList[0];
                    for (int i=0; i < sn_RaceScores.Len(); i++)
                    {
                        gRaceScores *rSR = sn_RaceScores[i];
                        if (rSR->user_name == rS->user_name)
                        {
                            SendMessage.SetTemplateParameter(1, rS->real_name);
                            SendMessage.SetTemplateParameter(2, rank);
                            SendMessage.SetTemplateParameter(3, rS->time);
                            SendMessage << "$player_race_record_search_found";
                            sn_ConsoleOut(SendMessage, p->Owner());
                            break;
                        }
                        rank++;
                    }
                }
                else if (searchList.Len() == 0)
                {
                    tOutput SendMessage;
                    SendMessage.SetTemplateParameter(1, extraArg);
                    SendMessage << "$player_race_record_search_not_found";
                    sn_ConsoleOut(SendMessage, p->Owner());
                    return;
                }
                else
                {
                    tOutput SendMessage;
                    SendMessage.SetTemplateParameter(1, extraArg);
                    SendMessage << "$player_race_record_search_many_found";
                    sn_ConsoleOut(SendMessage, p->Owner());
                    return;
                }
                Output << "0xff6622Current Map: 0x00ff44" << sg_currentMap << "\n";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
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
            firstFinishTime_ = player->raceTime;
            firstToArive_ = player->GetUserName();
        }

        REAL reachTime = player->raceTime;
        gRaceScores::Add(player->GetUserName(), player->GetName() , RacingScore, reachTime);

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

        finishPlace_++;
        if (!sg_raceLogTime)
        {
            win.SetTemplateParameter(1, player->GetColoredName() );
            win.SetTemplateParameter(2, RacingScore);
            win.SetTemplateParameter(3, player->raceTime);
            win << "$player_reach_race";
        }
        else
        {
            if (finishPlace_ == 1)
            {
                win.SetTemplateParameter(1, player->GetColoredName());
                win.SetTemplateParameter(2, reachTime);
                win << "$player_reach_race_first";
            }
            else
            {
                win.SetTemplateParameter(1, player->GetColoredName());
                win.SetTemplateParameter(2, finishPlace_);
                win.SetTemplateParameter(3, (reachTime - firstFinishTime_));
                win << "$player_reach_race_time";
            }
        }

        player->AddScore( RacingScore, win, "" );
        if (RacingScore >= 2) RacingScore -= sg_scoreRaceDeplete;
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans)
{
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

    if (roundFinished_)
    {
        for (int x=0; x < se_PlayerNetIDs.Len(); x++)
        {
            ePlayerNetID *p = se_PlayerNetIDs[x];
            if (!p->raceArrived && p->IsHuman())
            {
                gRaceScores::Add(p->GetUserName(), p->GetName(), 0, -1);
                p->raceArrived = true;
            }
        }
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
    firstArrived_ = false;
    countDown_ = -1;
    roundFinished_ = false;
    winnerDeclared_ = false;
    finishPlace_ = 0;
    firstFinishTime_ = 0;
    firstToArive_ = "";
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

    //gRaceScores::Write();
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
    //REAL bestTime = firstFinishTime_;

    for ( int i = 0; i < se_PlayerNetIDs.Len(); i ++ )
    {
        ePlayerNetID * p = se_PlayerNetIDs[i];

        if ( firstToArive_ == p->GetUserName() && p->raceArrived )
        {
            //p->raceArrived = false;
            //p->raceTryouts = sg_raceTryoutsNumber;

            //AddGoal( p->raceTime, p->GetColoredName(), p->GetName() /*, p->GetRawAuthenticatedName() */);

            /*if ( p->raceTime < bestTime || bestTime == 0 )
            {
                bestTime = p->raceTime;
                winner = p->CurrentTeam();
            }*/

            winner = p->CurrentTeam();
            break;
        }
    }
    winnerDeclared_ = true;

    return winner;
}
