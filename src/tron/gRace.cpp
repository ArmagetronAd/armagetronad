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

#include "gArena.h"
#include "eGrid.h"

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
bool restrictRaceTimer(bool const &newValue)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (grid)
    {
        return false;
    }
    return true;
}
static tSettingItem<bool> sg_RaceTimerEnabledConf( "RACE_TIMER_ENABLED", sg_RaceTimerEnabled, &restrictRaceTimer);

int sg_RaceEndDelay = 60;
static tSettingItem<int> sg_RaceEndDelayConf( "RACE_END_DELAY", sg_RaceEndDelay );

static bool sg_raceSmartTimer = false;
static tSettingItem<bool> sg_raceSmartTimerConf("RACE_SMART_TIMER", sg_raceSmartTimer);

int sg_scoreRaceComplete = 10;
int RacingScore = sg_scoreRaceComplete;
static tSettingItem<int> sg_scoreRaceCompleteConf( "SCORE_RACE", sg_scoreRaceComplete );

int sg_scoreRaceDeplete = 1;
static tSettingItem<int> sg_scoreRaceDepleteConf( "RACE_SCORE_DEPLETE", sg_scoreRaceDeplete);

bool sg_raceLogTime = false;
static tSettingItem<bool> sg_raceLogTimeConf("RACE_LOG_TIME", sg_raceLogTime);

bool sg_raceLogUnfinished = false;
static tSettingItem<bool> sg_raceLogUnfinishedConf("RACE_LOG_UNFINISHED", sg_raceLogUnfinished);

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

static int sg_raceChances = 0;
static tSettingItem<int> sg_raceChancesConf("RACE_CHANCES", sg_raceChances);

static bool sg_raceFinishKill = false;
static tSettingItem<bool> sg_raceFinishKillConf("RACE_FINISH_KILL", sg_raceFinishKill);

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
tList<gRacePlayer> sn_RacePlayers;

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

static int sg_raceNumRanksShowStart = 3;
static tSettingItem<int> sg_raceNumRanksShowStartConf("RACE_NUM_RANKS_SHOW_START", sg_raceNumRanksShowStart);

static int sg_raceNumRanksShowEnd = 3;
static tSettingItem<int> sg_raceNumRanksShowEndConf("RACE_NUM_RANKS_SHOW_END", sg_raceNumRanksShowEnd);

static int sg_raceRankShowStart = 1;
static int sg_raceRankShowEnd = 2;
bool restrictRaceRanksShow(const int &newValue)
{
    if ((newValue < 0) || (newValue > 2)) return false;
    else return true;
}
static tSettingItem<int> sg_raceRankShowStartConf("RACE_RANKS_SHOW_START", sg_raceRankShowStart, &restrictRaceRanksShow);
static tSettingItem<int> sg_raceRankShowEndConf("RACE_RANKS_SHOW_END", sg_raceRankShowEnd, &restrictRaceRanksShow);

void gRaceScores::OutputStart()
{
    if (sg_raceRankShowStart == 0) return;

    Sort(); //  sort ranks

    tColoredString mess, header;
    tArray<tColoredString> ranksList;

    if (sn_RaceScores.Len() > 0)
    {
        int rank        = 0;
        int ranksPos    = 0;
        int playerPos   = 16;
        int scoresPos   = 0;
        int timesPos    = 0;

        if (sg_raceRankShowStart == 1)
        {
            if (sn_RaceScores.Len() >= sg_raceNumRanksShowStart)
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sn_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = strlen(ranksPosStr) + 5;

                for(int a = 0; a < sg_raceNumRanksShowStart; a++)
                {
                    gRaceScores *rScores = sn_RaceScores[a];
                    if (rScores)
                    {
                        tString scorePos, timePos, scoreTitle, timeTitle;
                        scorePos << rScores->score;
                        timePos << rScores->time;
                        scoreTitle << tOutput("$race_rank_title_score_name");
                        timeTitle << tOutput("$race_rank_title_time_name");

                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                    }
                }

                ranksPos    += 5;
                playerPos   += 4;
                scoresPos   += 4;
                timesPos    += 6;

                for(int b = 0;b < sg_raceNumRanksShowStart; b++)
                {
                    gRaceScores *rScores = sn_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->user_name.Len() > playerPos)
                        {
                            strippedName << rScores->user_name.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->user_name.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->score;
                        line.SetPos(ranksPos + playerPos + scoresPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->time;
                        line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }
            else
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sn_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len();
                } else ranksPos = ranksPosStr.Len();

                for(int a = 0; a < sn_RaceScores.Len(); a++)
                {
                    gRaceScores *rScores = sn_RaceScores[a];
                    if (rScores)
                    {
                        tString scorePos, timePos, scoreTitle, timeTitle;
                        scorePos << rScores->score;
                        timePos << rScores->time;
                        scoreTitle << tOutput("$race_rank_title_score_name");
                        timeTitle << tOutput("$race_rank_title_time_name");

                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                    }
                }

                ranksPos    += 5;
                playerPos   += 4;
                scoresPos   += 4;
                timesPos    += 6;

                for(int b = 0;b < sn_RaceScores.Len(); b++)
                {
                    gRaceScores *rScores = sn_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->user_name.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->score;
                        line.SetPos(ranksPos + playerPos + scoresPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->time;
                        line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }

            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
            header.SetPos(ranksPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
            header.SetPos(ranksPos + playerPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_score_name");
            header.SetPos(ranksPos + playerPos + scoresPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
            header.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
            header << " " << tOutput("$race_rank_border") << "\n";

            mess << tOutput("$race_rank_title_message", rank) << pz_mapName << "\n";
            mess << header;
            for(int j = 0; j < ranksList.Len(); j++)
            {
                mess << ranksList[j];
            }
            sn_ConsoleOut(mess);
        }
        else if (sg_raceRankShowEnd == 2)
        {
            int rank        = 0;
            int ranksPos    = 0;
            int playerPos   = 16;
            int scoresPos   = 0;
            int timesPos    = 0;

            for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
            {
                ePlayerNetID *p = se_PlayerNetIDs[i];
                if (p && (p->IsHuman()))
                {
                    for(int j = 0; j < sn_RaceScores.Len(); j++)
                    {
                        gRaceScores *rScores = sn_RaceScores[j];
                        if (rScores)
                        {
                            if (rScores->user_name == p->GetUserName())
                            {
                                tColoredString mess, line, header;

                                rank = j + 1;
                                mess << tOutput("$race_ranks_personal_message") << pz_mapName << "\n";

                                tString ranksPosStr, ranksTitle;
                                ranksPosStr << sn_RaceScores.Len();
                                ranksTitle << tOutput("$race_rank_title_name");

                                if (ranksTitle.Len() > ranksPosStr.Len())
                                {
                                    ranksPos = ranksTitle.Len();
                                } else ranksPos = ranksPosStr.Len();

                                for(int a = 0; a < sn_RaceScores.Len(); a++)
                                {
                                    gRaceScores *rScores = sn_RaceScores[a];
                                    if (rScores)
                                    {
                                        tString scorePos, timePos, scoreTitle, timeTitle;
                                        scorePos << rScores->score;
                                        timePos << rScores->time;
                                        scoreTitle << tOutput("$race_rank_title_score_name");
                                        timeTitle << tOutput("$race_rank_title_time_name");

                                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                                    }
                                }

                                ranksPos    += 5;
                                playerPos   += 4;
                                scoresPos   += 4;
                                timesPos    += 6;

                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
                                header.SetPos(ranksPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
                                header.SetPos(ranksPos + playerPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_score_name");
                                header.SetPos(ranksPos + playerPos + scoresPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
                                header.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                header << " " << tOutput("$race_rank_border") << "\n";
                                mess << header;

                                if ((j - 1) >= 0)
                                {
                                    gRaceScores *prev = sn_RaceScores[j - 1];
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << (rank - 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->user_name.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->score;
                                    line.SetPos(ranksPos + playerPos + scoresPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->time;
                                    line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << (rank);
                                line.SetPos(ranksPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->user_name.SubStr(0, 15);
                                line.SetPos(ranksPos + playerPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->score;
                                line.SetPos(ranksPos + playerPos + scoresPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->time;
                                line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                mess << line;
                                line = "";

                                if ((j + 1) <= (sn_RaceScores.Len() - 1))
                                {
                                    gRaceScores *next = sn_RaceScores[j + 1];
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << (rank + 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->user_name.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->score;
                                    line.SetPos(ranksPos + playerPos + scoresPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->time;
                                    line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                sn_ConsoleOut(mess, p->Owner());
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
}

void gRaceScores::OutputEnd()
{
    if (sg_raceRankShowEnd == 0) return;

    Sort();

    tColoredString mess, header;
    tArray<tColoredString> ranksList;

    if (sn_RaceScores.Len() > 0)
    {
        if (sg_raceRankShowEnd == 1)
        {
            int rank        = 0;
            int ranksPos    = 0;
            int playerPos   = 16;
            int scoresPos   = 0;
            int timesPos    = 0;

             if (sn_RaceScores.Len() >= sg_raceNumRanksShowEnd)
             {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sn_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = strlen(ranksPosStr) + 5;

                for(int a = 0; a < sg_raceNumRanksShowEnd; a++)
                {
                    gRaceScores *rScores = sn_RaceScores[a];
                    if (rScores)
                    {
                        tString scorePos, timePos, scoreTitle, timeTitle;
                        scorePos << rScores->score;
                        timePos << rScores->time;
                        scoreTitle << tOutput("$race_rank_title_score_name");
                        timeTitle << tOutput("$race_rank_title_time_name");

                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                    }
                }

                ranksPos    += 5;
                playerPos   += 4;
                scoresPos   += 4;
                timesPos    += 6;

                for(int b = 0;b < sg_raceNumRanksShowEnd; b++)
                {
                    gRaceScores *rScores = sn_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->user_name.Len() > playerPos)
                        {
                            strippedName << rScores->user_name.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->user_name.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->score;
                        line.SetPos(ranksPos + playerPos + scoresPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->time;
                        line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }
            else
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sn_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len();
                } else ranksPos = ranksPosStr.Len();

                for(int a = 0; a < sn_RaceScores.Len(); a++)
                {
                    gRaceScores *rScores = sn_RaceScores[a];
                    if (rScores)
                    {
                        tString scorePos, timePos, scoreTitle, timeTitle;
                        scorePos << rScores->score;
                        timePos << rScores->time;
                        scoreTitle << tOutput("$race_rank_title_score_name");
                        timeTitle << tOutput("$race_rank_title_time_name");

                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                    }
                }

                ranksPos    += 5;
                playerPos   += 4;
                scoresPos   += 4;
                timesPos    += 6;

                for(int b = 0;b < sn_RaceScores.Len(); b++)
                {
                    gRaceScores *rScores = sn_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->user_name.Len() > playerPos)
                        {
                            strippedName << rScores->user_name.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->user_name.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->score;
                        line.SetPos(ranksPos + playerPos + scoresPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->time;
                        line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }

            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
            header.SetPos(ranksPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
            header.SetPos(ranksPos + playerPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_score_name");
            header.SetPos(ranksPos + playerPos + scoresPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
            header.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
            header << " " << tOutput("$race_rank_border") << "\n";

            mess << tOutput("$race_rank_title_message", rank) << pz_mapName << "\n";
            mess << header;
            for(int j = 0; j < ranksList.Len(); j++)
            {
                mess << ranksList[j];
            }
            sn_ConsoleOut(mess);
        }
        else if (sg_raceRankShowEnd == 2)
        {
            int rank        = 0;
            int ranksPos    = 0;
            int playerPos   = 16;
            int scoresPos   = 0;
            int timesPos    = 0;

            for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
            {
                ePlayerNetID *p = se_PlayerNetIDs[i];
                if (p && (p->IsHuman()))
                {
                    for(int j = 0; j < sn_RaceScores.Len(); j++)
                    {
                        gRaceScores *rScores = sn_RaceScores[j];
                        if (rScores)
                        {
                            if (rScores->user_name == p->GetUserName())
                            {
                                tColoredString mess, line, header;

                                rank = j + 1;
                                mess << tOutput("$race_ranks_personal_message") << pz_mapName << "\n";

                                tString ranksPosStr, ranksTitle;
                                ranksPosStr << sn_RaceScores.Len();
                                ranksTitle << tOutput("$race_rank_title_name");

                                if (ranksTitle.Len() > ranksPosStr.Len())
                                {
                                    ranksPos = ranksTitle.Len();
                                } else ranksPos = ranksPosStr.Len();

                                for(int a = 0; a < sn_RaceScores.Len(); a++)
                                {
                                    gRaceScores *rScores = sn_RaceScores[a];
                                    if (rScores)
                                    {
                                        tString scorePos, timePos, scoreTitle, timeTitle;
                                        scorePos << rScores->score;
                                        timePos << rScores->time;
                                        scoreTitle << tOutput("$race_rank_title_score_name");
                                        timeTitle << tOutput("$race_rank_title_time_name");

                                        if (scoreTitle.Len() > scorePos.Len()) scoresPos = scoreTitle.Len();
                                        else if (scorePos.Len() > scoresPos) scoresPos = scorePos.Len();

                                        if (timeTitle.Len() > scorePos.Len()) timesPos = timeTitle.Len();
                                        else if (timePos.Len() > timesPos) timesPos = timePos.Len();
                                    }
                                }

                                ranksPos    += 5;
                                playerPos   += 4;
                                scoresPos   += 4;
                                timesPos    += 6;

                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
                                header.SetPos(ranksPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
                                header.SetPos(ranksPos + playerPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_score_name");
                                header.SetPos(ranksPos + playerPos + scoresPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
                                header.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                header << " " << tOutput("$race_rank_border") << "\n";
                                mess << header;

                                if ((j - 1) >= 0)
                                {
                                    gRaceScores *prev = sn_RaceScores[j - 1];
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << (rank - 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->user_name.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->score;
                                    line.SetPos(ranksPos + playerPos + scoresPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << prev->time;
                                    line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << (rank);
                                line.SetPos(ranksPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->user_name.SubStr(0, 15);
                                line.SetPos(ranksPos + playerPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->score;
                                line.SetPos(ranksPos + playerPos + scoresPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 1,1,.5 ) << rScores->time;
                                line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                line << " " << tColoredString::ColorString( 1,1,0.5 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                mess << line;
                                line = "";

                                if ((j + 1) <= (sn_RaceScores.Len() - 1))
                                {
                                    gRaceScores *next = sn_RaceScores[j + 1];
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << (rank + 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->user_name.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->score;
                                    line.SetPos(ranksPos + playerPos + scoresPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << " " << tColoredString::ColorString( 0, 0.5, 1 ) << next->time;
                                    line.SetPos(ranksPos + playerPos + scoresPos + timesPos, false);
                                    line << " " << tColoredString::ColorString( 0, 0.5, 1 ) << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                sn_ConsoleOut(mess, p->Owner());
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }

}

//! Racing player's things
gRacePlayer::gRacePlayer(ePlayerNetID *p)
{
    this->player = p;

    if (p && p->Object())
    {
        this->cycle = dynamic_cast<gCycle *>(p->Object());

        this->position = Cycle()->Position();
        this->direction = Cycle()->Direction();
    }

    this->chances = sg_raceChances;

    sn_RacePlayers.Insert(this);
}

bool gRacePlayer::PlayerExists(ePlayerNetID *p)
{
    for(int i = 0; i < sn_RacePlayers.Len(); i++)
    {
        gRacePlayer *rPlayer = sn_RacePlayers[i];
        if (rPlayer && (rPlayer->Player() == p))
        {
            return true;
        }
    }
    return false;
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

        if (sg_raceFinishKill) player->Object()->Kill();
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans)
{
    if (!roundFinished_ && (sg_raceChances > 0) )
    {
        eGrid *grid = eGrid::CurrentGrid();
        if (grid)
        {
            for(int a=0; a < sn_RacePlayers.Len(); a++)
            {
                gRacePlayer *rPlayer = sn_RacePlayers[a];
                if (rPlayer && !rPlayer->Cycle())
                {
                    if ((rPlayer->Chances() > 0) && (rPlayer->Chances() <= sg_raceChances))
                    {
                        gCycle *cycle = new gCycle(grid, rPlayer->Cycle()->Position(), rPlayer->Cycle()->Direction());
                        rPlayer->NewCycle(cycle);
                        rPlayer->Player()->ControlObject(cycle);
                        rPlayer->DropChances(1);    //  decrease chances by this many values
                        alive += 1;
                    }
                }
            }
        }
    }

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
            if (sg_raceSmartTimer)
            {
                if (!roundFinished_)
                {
                    if (sn_RaceScores.Len() == 0)
                    {
                        if ( countDown_ < 0 )                  // not started yet
                            countDown_ = sg_RaceEndDelay + 1;
                        else if ( countDown_ == 0 )
                            roundFinished_ = true;
                    }
                    if (sn_RaceScores.Len() == 1)
                    {
                        gRaceScores *rScores = sn_RaceScores[0];
                        if (rScores)
                        {
                            if (countDown_ < 0)
                            {
                                int timer = ceil(rScores->time);
                                countDown_ = ceil(timer * 1.2) + 1;
                            }
                        }
                    }
                    else if (sn_RaceScores.Len() == 2)
                    {
                        gRaceScores *one = sn_RaceScores[0];
                        gRaceScores *two = sn_RaceScores[1];
                        if (one && two)
                        {
                            if (countDown_ < 0)
                            {
                                int timer = ceil((one->time + two->time) / 2);
                                countDown_ = ceil(timer * 1.2) + 1;
                            }
                        }
                    }
                    else if (sn_RaceScores.Len() >= 3)
                    {
                        gRaceScores *one = sn_RaceScores[0];
                        gRaceScores *two = sn_RaceScores[1];
                        gRaceScores *tre = sn_RaceScores[2];
                        if (one && two && tre)
                        {
                            if (countDown_ < 0)
                            {
                                int timer = ceil((one->time + two->time + tre->time) / 3);
                                countDown_ = ceil(timer * 1.2) + 1;
                            }
                        }
                    }
                }
            }
            else
            {
                if ( countDown_ < 0 )                   // not started yet
                    countDown_ = sg_RaceEndDelay + 1;

                else if ( countDown_ == 0 )
                    roundFinished_ = true;
            }

            if ( !roundFinished_ )
            {
                countDown_ --;

                if ( countDown_ == 0 )
                {
                    roundFinished_ = true;
                    countDown_ = -1;
                    sn_CenterMessage("");
                }
                else
                {
                    tOutput message;
                    message << "0xff7777" << countDown_ << "                    ";
                    sn_CenterMessage( message );
                }
            }
        }
    }

    if ( roundFinished_ && !winZones_.empty() )
    {
        for ( int i = 0; i < winZones_.size(); i ++)
        {
            gWinZoneHack *winZone = winZones_[i];
            if (winZone) winZone->Vanish( 0.5 );
        }
        winZones_.clear();
    }
    if ( roundFinished_ && !deathZones_.empty())
    {
        for (int j = 0; j < deathZones_.size(); j++)
        {
            gDeathZoneHack *deathZone = deathZones_[j];
            if (deathZone) deathZone->Vanish( 0.5 );
        }
        deathZones_.clear();
    }

    if (roundFinished_ && sg_raceLogUnfinished)
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

    //  reset all race stats
    for (int i=0; i < se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];
        if (p->raceArrived) p->raceArrived = false;
    }

    //  reset race players list
    for(int b=0; b < sn_RacePlayers.Len(); b++)
    {
        gRacePlayer *rPlayer = sn_RacePlayers[b];
        if (rPlayer)
        {
            delete rPlayer;
            sn_RacePlayers.RemoveAt(b);
            b--;
        }
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
            sn_ConsoleOut(tOutput("$player_win_race", p->GetColoredName(), sg_scoreRaceComplete));
            break;
        }
    }
    winnerDeclared_ = true;

    return winner;
}
