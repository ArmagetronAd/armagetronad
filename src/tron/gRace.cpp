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

#include "gZone.h"

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
/*bool restrictRacetime_r(bool const &newValue)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (grid)
    {
        return false;
    }
    return true;
}*/
static tSettingItem<bool> sg_RaceTimerEnabledConf( "RACE_TIMER_ENABLED", sg_RaceTimerEnabled/*, &restrictRacetime_r*/);

int sg_RaceEndDelay = 60;
static tSettingItem<int> sg_RaceEndDelayConf( "RACE_END_DELAY", sg_RaceEndDelay );

static bool sg_raceSmartTimer = false;
static tSettingItem<bool> sg_raceSmartTimerConf("RACE_SMART_TIMER", sg_raceSmartTimer);

int sg_scoreRaceComplete = 10;
int RacingScore = sg_scoreRaceComplete;
bool restrictRaceScore(int const &newValue)
{
    if (newValue >= 1)
    {
        RacingScore = newValue;
    }
    else
    {
        RacingScore = 1;
        return false;
    }

    return true;
}
static tSettingItem<int> sg_scoreRaceCompleteConf( "SCORE_RACE", sg_scoreRaceComplete, &restrictRaceScore);

int sg_scoreRaceFinish = 10;
static tSettingItem<int> sg_scoreRaceFinishConf("SCORE_RACE_FINISH", sg_scoreRaceFinish);

int sg_scoreRaceDeplete = 1;
static tSettingItem<int> sg_scoreRaceDepleteConf( "RACE_SCORE_DEPLETE", sg_scoreRaceDeplete);

//! 0   -   if the player should receive points depending on SCORE_RACE_FINISH
//! 1   -   if the player should receive points depending on RACE_SCORE_DEPLETE
bool sg_racePointsType = true;
static tSettingItem<bool> sg_racePointsTypeConf("RACE_POINTS_TYPE", sg_racePointsType);

bool sg_raceLogTime = false;
static tSettingItem<bool> sg_raceLogTimeConf("RACE_LOG_TIME", sg_raceLogTime);

bool sg_raceLogUnfinished = false;
static tSettingItem<bool> sg_raceLogUnfinishedConf("RACE_LOG_UNFINISHED", sg_raceLogUnfinished);

static int sg_raceChances = 0;
static tSettingItem<int> sg_raceChancesConf("RACE_CHANCES", sg_raceChances);

static bool sg_raceFinishKill = false;
static tSettingItem<bool> sg_raceFinishKillConf("RACE_FINISH_KILL", sg_raceFinishKill);

static bool sg_raceLogLogin = true;
static tSettingItem<bool> sg_raceLogLoginConf("RACE_LOG_LOGIN", sg_raceLogLogin);

/*
static bool sg_raceRanksLimit = true;
static tSettingItem<bool> sg_raceRanksLimitConf("RACE_RANKS_LIMIT", sg_raceRanksLimit);

static int sg_raceRanksLimitTime = 1209600;
bool restrictRaceRanksLimitTime(const int &newValue)
{
    if (newValue <= 0) return false;

    return true;
}
static tSettingItem<int> sg_raceRanksLimitTimeConf("RACE_RANKS_LIMIT_TIME", sg_raceRanksLimitTime, &restrictRaceRanksLimitTime);
*/

static bool sg_raceIdleKill = false;
static tSettingItem<bool> sg_raceIdleKillConf("RACE_IDLE_KILL", sg_raceIdleKill);

static REAL sg_raceIdleTime = 5;
static REAL sg_raceIdleSpeed = 30;
bool restrictRaceIdle(const REAL &newValue)
{
    if (newValue < 0) return false;

    return true;
}

static tSettingItem<REAL> sg_raceIdleTimeConf("RACE_IDLE_TIME", sg_raceIdleTime, &restrictRaceIdle);
static tSettingItem<REAL> sg_raceIdleSpeedConf("RACE_IDLE_SPEED", sg_raceIdleSpeed, &restrictRaceIdle);

//! STATIC VARIABLES
bool gRace::firstArrived_ = false;
int  gRace::countDown_ = -1;
bool gRace::roundFinished_ = false;
bool gRace::winnerDeclared_ = false;
int gRace::finishPlace_ = 0;
REAL gRace::firstFinishTime_ = 0;
tString gRace::firstToArive_;

tString gRaceScores::mapFile_ = tString("");

bool sg_raceOutputSent = false;

tList<gRaceScores> sg_RaceScores;
tList<gRacePlayer> sg_RacePlayers;
tList<gRacePlayer> sg_RaceFinished;

gRaceScores::gRaceScores(tString UserName)
{
    this->userName_ = UserName;

    this->time_ = -1;
    this->played_ = 0;

    this->lastTime_ = se_Time();

    sg_RaceScores.Insert(this);

    this->rank_ = sg_RaceScores.Len();
}

gRaceScores *gRaceScores::GetPlayer(tString name)
{
    for (int i=0; i < sg_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sg_RaceScores[i];
        if (rS->userName_ == name)
            return rS;
    }
    return NULL;
}

bool gRaceScores::CheckPlayer(tString name)
{
    for (int i=0; i < sg_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sg_RaceScores[i];
        if (rS->userName_ == name)
            return true;
    }
    return false;
}

void gRaceScores::Switch(int i, int j)
{
    //  swap array positions
    Swap(sg_RaceScores[i], sg_RaceScores[j]);

    //  swap rank numbers
    gRaceScores *rSP = sg_RaceScores[i];
    gRaceScores *rSR = sg_RaceScores[j];

    Swap(rSP->rank_, rSR->rank_);
}

bool gRaceScores::InOrder(int i,int j)
{
    gRaceScores *rSP = sg_RaceScores[i];
    gRaceScores *rSR = sg_RaceScores[j];

    if (rSP->time_ <= 0) return false;
    if (rSR->time_ <= 0) return true;
    else return (rSP->time_ < rSR->time_);
}

void gRaceScores::Sort()
{
    for(int i=1;i<sg_RaceScores.Len();i++)
        for(int j=i;j>=1 && !InOrder(j-1,j);j--)
            Switch(j,j-1);
}

void gRaceScores::Add(gRacePlayer *racePlayer, bool finished)
{
    bool timeChanged = false;
    REAL timeDiff = 0;
    bool newRacer = false;
    gRaceScores *racingPlayer = NULL;

    tString username;
    if (racePlayer->Player()->HasLoggedIn() && (racePlayer->Player()->GetAuthenticatedName().Len() > 1))
        username = racePlayer->Player()->GetAuthenticatedName();
    else
        username = racePlayer->Player()->GetUserName();

    if (CheckPlayer(username))
    {
        racingPlayer = GetPlayer(username);

        //  ensure that only finished racers go through this code
        if ((racePlayer->Time() > 0) && finished)
        {
            timeDiff = racingPlayer->time_;

            if (((racePlayer->Time() < racingPlayer->Time()) && racePlayer->Time() > 0) || (racingPlayer->Time() <= 0 && racePlayer->Time() > 0))
            {
                racingPlayer->time_ = racePlayer->Time();
                tOutput newtime;
                newtime.SetTemplateParameter(1, racePlayer->Time());
                newtime << "$player_personal_best_reach_time";

                sn_ConsoleOut(newtime, racePlayer->Player()->Owner());

                timeChanged = true;
            }
        }
    }
    else
    {
        racingPlayer = new gRaceScores(username);
        racingPlayer->time_ = racePlayer->Time();
        timeDiff = racingPlayer->time_;

        if (racePlayer->Time() > 0)
        {
            tOutput newtime;
            newtime.SetTemplateParameter(1, racePlayer->Time());
            newtime << "$player_personal_best_reach_time";

            sn_ConsoleOut(newtime, racePlayer->Player()->Owner());

            timeChanged = true;
        }
        newRacer = true;
    }

    //  increment finished times when they crossed the zone
    if (finished) racingPlayer->SetPlayed(racingPlayer->Played() + 1);

    //  update the last time due to them finishing the map yet again
    if (finished) racingPlayer->SetLastTime(se_Time());

    //  ensure that times really have changed for this to take effect
    if (timeChanged && finished)
    {
        //  store the player's previous rank if their got better or worse
        int prevRank = racingPlayer->Rank();

        //  sort out ranks once player get better time
        Sort();

        //  get top player from that list
        gRaceScores *firstRanker = sg_RaceScores[0];

        //  check if it's the same player
        if (firstRanker == racingPlayer)
        {
            tOutput bestTime;
            bestTime.SetTemplateParameter(1, racePlayer->Player()->GetColoredName());
            bestTime.SetTemplateParameter(2, firstRanker->Time());
            bestTime.SetTemplateParameter(3, pz_mapName);
            bestTime << "$race_player_hold_best_time";

            sn_ConsoleOut(bestTime);
        }
        else
        {
            //  tell everyone whether the player got better or worse
            tOutput rankMsg;

            if (newRacer)
            {
                rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                rankMsg.SetTemplateParameter(2, racePlayer->Time());
                rankMsg.SetTemplateParameter(3, racingPlayer->Rank());
                rankMsg << "$race_player_hold_new_time";
            }
            else
            {
                if (prevRank == racingPlayer->Rank())
                {
                    rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                    rankMsg.SetTemplateParameter(2, racePlayer->Time());
                    rankMsg.SetTemplateParameter(3, timeDiff - racePlayer->Time());
                    rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
                    rankMsg << "$race_player_hold_faster_time";
                }
                else if (prevRank > racingPlayer->Rank())
                {
                    rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                    rankMsg.SetTemplateParameter(2, racePlayer->Time());
                    rankMsg.SetTemplateParameter(3, timeDiff - racePlayer->Time());
                    rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
                    rankMsg << "$race_player_hold_faster_rank";
                }
                else if (prevRank < racingPlayer->Rank())
                {
                    rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                    rankMsg.SetTemplateParameter(2, racePlayer->Time());
                    rankMsg.SetTemplateParameter(3, racePlayer->Time() - timeDiff);
                    rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
                    rankMsg << "$race_player_hold_slower_rank";
                }
            }

            sn_ConsoleOut(rankMsg);
        }
    }
    else
    {
        if (finished)
        {
            //  tell everyone the player did not improve their holding time
            tOutput rankMsg;
            rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
            rankMsg.SetTemplateParameter(2, racePlayer->Time());
            rankMsg.SetTemplateParameter(3, racePlayer->Time() - timeDiff);
            rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
            rankMsg << "$race_player_hold_same_rank";

            sn_ConsoleOut(rankMsg);
        }
    }
}

bool sg_raceRecordsLoad = true;
static tSettingItem<bool> sg_raceRecordsLoadConf("RACE_RECORDS_LOAD", sg_raceRecordsLoad);

void gRaceScores::Read()
{
    if (!sg_raceRecordsLoad) return;

    tString Input;
    //mapFile << pz_mapAuthor << "/" << pz_mapCategory << "/" << pz_mapName << "-" << pz_mapVersion << ".aamap.xml";
    Input << "race_scores/" << mapfile << ".txt";
    sn_ConsoleOut(tOutput("$race_ranks_loading", pz_mapName));

    mapFile_ = mapfile;

    //  clear old records if they exist to make way for the new
    if (sg_RaceScores.Len() > 0)
        sg_RaceScores.Clear();

    std::ifstream r;
    if (tDirectories::Var().Open(r, Input))
    {
        while (!r.eof())
        {
            //std::stringstream s(r.getline());

            std::string sayLine;
            std::getline(r, sayLine);
            std::istringstream s(sayLine);
            tString params;

            params.ReadLine(s);
            int pos = 0;

            if (params.Filter() != "")
            {
                gRaceScores *rS = NULL;

                //  name of the owner
                tString name = params.ExtractNonBlankSubString(pos);
                if (!CheckPlayer(name))
                    rS = new gRaceScores(name);
                else
                    rS = GetPlayer(name);

                //  time of finished
                rS->time_ = atof(params.ExtractNonBlankSubString(pos));

                //  how many times they crossed the finish line
                tString playedFor = params.ExtractNonBlankSubString(pos);
                if (playedFor.Filter() != "")
                    rS->played_ = atoi(playedFor);

                //  the last time they played in this map
                tString lastTimeStr = params.ExtractNonBlankSubString(pos);
                if (lastTimeStr.Filter() != "")
                    rS->lastTime_ = atof(lastTimeStr);
                else
                    rS->lastTime_ = se_Time();

                /*
                //  Checking for the race ranks limit.
                //  Cannot have records of people that on't play often.
                if (sg_raceRanksLimit && sg_raceRanksLimitTime > 0)
                {
                    //  checking if the last time + the time limit exceed today's time
                    if (se_Time() >= (rS->lastTime_ + sg_raceRanksLimitTime))
                    {
                        for(int i = 0; i < sg_RaceScores.Len(); i++)
                        {
                            gRaceScores *raceScorePlayer = sg_RaceScores[i];
                            if (raceScorePlayer && (raceScorePlayer == rS))
                            {
                                sg_RaceScores.RemoveAt(i);
                                delete rS;

                                break;
                            }
                        }
                    }
                }
                */
            }
        }
    }
    r.close();
}

bool sg_raceRecordsSave = true;
static tSettingItem<bool> sg_raceRecordsSaveConf("RACE_RECORDS_SAVE", sg_raceRecordsSave);

void gRaceScores::Write()
{
    if (!sg_raceRecordsSave) return;

    tString Output;

    Sort();

    //mapFile << pz_mapAuthor << "/" << pz_mapCategory << "/" << pz_mapName << "-" << pz_mapVersion << ".aamap.xml";
    Output << "race_scores/" << mapFile_ << ".txt";
    sn_ConsoleOut(tOutput("$race_ranks_saving", pz_mapName));

    if (sg_RaceScores.Len() > 0)
    {
        std::ofstream w;
        if (tDirectories::Var().Open(w, Output))
        {
            for (int i=0; i < sg_RaceScores.Len(); i++)
            {
                gRaceScores *rS = sg_RaceScores[i];
                if (rS)
                {
                    if (rS->time_ != 0)
                        w << rS->userName_ << " " << rS->time_ << " " << rS->played_ << " " << rS->lastTime_ << "\n";
                }
            }
        }
        w.close();
    }
}

void gRaceScores::Reset()
{
    for(int i = 0; i < sg_RaceScores.Len(); i++)
    {
        gRaceScores *rS = sg_RaceScores[i];
        delete rS;
    }
    sg_RaceScores.Clear();
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

tString sg_raceHighliter("0x00ff00");
static tSettingItem<tString> sg_raceHighliterConf("RACE_RECORD_HIGHLITER", sg_raceHighliter);

void gRaceScores::OutputStart()
{
    if (sg_raceRankShowStart == 0) return;

    Sort(); //  sort ranks

    tColoredString mess, header;
    tArray<tColoredString> ranksList;

    if (sg_RaceScores.Len() > 0)
    {
        int rank        = 0;
        int ranksPos    = 0;
        int playerPos   = 16;
        int time_sPos    = 0;

        if (sg_raceRankShowStart == 1)
        {
            if (sg_RaceScores.Len() >= sg_raceNumRanksShowStart)
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sg_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = strlen(ranksPosStr) + 5;

                for(int a = 0; a < sg_raceNumRanksShowStart; a++)
                {
                    gRaceScores *rScores = sg_RaceScores[a];
                    if (rScores)
                    {
                        tString time_Pos, time_Title;
                        time_Pos << rScores->time_;
                        time_Title << tOutput("$race_rank_title_time_name");
                        time_Title = tColoredString::RemoveColors(time_Title);

                        if (time_Title.Len() > ranksPos) time_sPos = time_Title.Len();
                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                    }
                }

                //ranksPos  += 5;
                playerPos += 4;
                time_sPos += 5;

                for(int b = 0;b < sg_raceNumRanksShowStart; b++)
                {
                    gRaceScores *rScores = sg_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->userName_.Len() > playerPos)
                        {
                            strippedName << rScores->userName_.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->userName_.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        if (rScores->time_ > 0)
                            line << " " << tOutput("$race_rank_border") << " " << rScores->time_;
                        else
                            line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                        line.SetPos(ranksPos + playerPos + time_sPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }
            else
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sg_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = ranksPosStr.Len() + 5;

                for(int a = 0; a < sg_RaceScores.Len(); a++)
                {
                    gRaceScores *rScores = sg_RaceScores[a];
                    if (rScores)
                    {
                        tString time_Pos, time_Title;
                        time_Pos << rScores->time_;
                        time_Title << tOutput("$race_rank_title_time_name");
                        time_Title = tColoredString::RemoveColors(time_Title);

                        if (time_Title.Len() > time_Pos.Len()) time_sPos = time_Title.Len();
                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                    }
                }

                //ranksPos  += 5;
                playerPos += 4;
                time_sPos += 5;

                for(int b = 0;b < sg_RaceScores.Len(); b++)
                {
                    gRaceScores *rScores = sg_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->userName_.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->time_;
                        line.SetPos(ranksPos + playerPos + time_sPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }

            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
            header.SetPos(ranksPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
            header.SetPos(ranksPos + playerPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
            header.SetPos(ranksPos + playerPos + time_sPos, false);
            header << " " << tOutput("$race_rank_border") << "\n";

            mess << tOutput("$race_rank_title_message", rank) << pz_mapName << "\n";
            mess << header;
            for(int j = 0; j < ranksList.Len(); j++)
            {
                mess << ranksList[j];
            }
            sn_ConsoleOut(mess);
        }
        else if (sg_raceRankShowStart == 2)
        {
            int rank        = 0;
            int ranksPos    = 0;
            int playerPos   = 16;
            int time_sPos    = 0;

            for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
            {
                ePlayerNetID *p = se_PlayerNetIDs[i];
                if (p && (p->IsHuman()))
                {
                    for(int j = 0; j < sg_RaceScores.Len(); j++)
                    {
                        gRaceScores *rScores = sg_RaceScores[j];
                        if (rScores)
                        {
                            if ((rScores->userName_ == p->GetUserName()) || (p->HasLoggedIn() && (rScores->userName_ == p->GetAuthenticatedName())))
                            {
                                tColoredString mess, line, header;

                                rank = j + 1;
                                mess << tOutput("$race_ranks_personal_message") << pz_mapName << "\n";

                                tString ranksPosStr, ranksTitle;
                                ranksPosStr << sg_RaceScores.Len();
                                ranksTitle << tOutput("$race_rank_title_name");
                                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                                if (ranksTitle.Len() > ranksPosStr.Len())
                                {
                                    ranksPos = ranksTitle.Len() + 5;
                                } else ranksPos = ranksPosStr.Len() + 5;

                                for(int a = 0; a < sg_RaceScores.Len(); a++)
                                {
                                    gRaceScores *rScores = sg_RaceScores[a];
                                    if (rScores)
                                    {
                                        tString time_Pos, time_Title;
                                        time_Pos << rScores->time_;
                                        time_Title << tOutput("$race_rank_title_time_name");
                                        time_Title = tColoredString::RemoveColors(time_Title);

                                        if (time_Title.Len() > time_Pos.Len()) time_sPos = time_Title.Len();
                                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                                    }
                                }

                                //ranksPos  += 5;
                                playerPos += 4;
                                time_sPos += 5;

                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
                                header.SetPos(ranksPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
                                header.SetPos(ranksPos + playerPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
                                header.SetPos(ranksPos + playerPos + time_sPos, false);
                                header << " " << tOutput("$race_rank_border") << "\n";
                                mess << header;

                                if ((j - 1) >= 0)
                                {
                                    gRaceScores *prev = sg_RaceScores[j - 1];
                                    line << " " << tOutput("$race_rank_border") << " " << (rank - 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tOutput("$race_rank_border") << " " << prev->userName_.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    if (prev->time_ > 0)
                                        line << " " << tOutput("$race_rank_border") << " " << prev->time_;
                                    else
                                        line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                                    line.SetPos(ranksPos + playerPos + time_sPos, false);
                                    line << " " << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << (rank);
                                line.SetPos(ranksPos, false);
                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << rScores->userName_.SubStr(0, 15);
                                line.SetPos(ranksPos + playerPos, false);
                                if (rScores->time_ > 0)
                                    line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << rScores->time_;
                                else
                                    line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << "UNDONE";
                                line.SetPos(ranksPos + playerPos + time_sPos, false);
                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                mess << line;
                                line = "";

                                if ((j + 1) <= (sg_RaceScores.Len() - 1))
                                {
                                    gRaceScores *next = sg_RaceScores[j + 1];
                                    line << " " << tOutput("$race_rank_border") << " " << (rank + 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tOutput("$race_rank_border") << " " << next->userName_.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    if (next->time_ > 0)
                                        line << " " << tOutput("$race_rank_border") << " " << next->time_;
                                    else
                                        line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                                    line.SetPos(ranksPos + playerPos + time_sPos, false);
                                    line << " " << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
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

    if (sg_RaceScores.Len() > 0)
    {
        if (sg_raceRankShowEnd == 1)
        {
            int rank        = 0;
            int ranksPos    = 0;
            int playerPos   = 16;
            int time_sPos    = 0;

             if (sg_RaceScores.Len() >= sg_raceNumRanksShowEnd)
             {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sg_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = strlen(ranksPosStr) + 5;

                for(int a = 0; a < sg_raceNumRanksShowEnd; a++)
                {
                    gRaceScores *rScores = sg_RaceScores[a];
                    if (rScores)
                    {
                        tString time_Pos, time_Title;
                        time_Pos << rScores->time_;
                        time_Title << tOutput("$race_rank_title_time_name");
                        time_Title = tColoredString::RemoveColors(time_Title);

                        if (time_Title.Len() > time_Pos.Len()) time_sPos = time_Title.Len();
                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                    }
                }

                //ranksPos  += 5;
                playerPos += 4;
                time_sPos += 5;

                for(int b = 0;b < sg_raceNumRanksShowEnd; b++)
                {
                    gRaceScores *rScores = sg_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->userName_.Len() > playerPos)
                        {
                            strippedName << rScores->userName_.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->userName_.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        if (rScores->time_ > 0)
                            line << " " << tOutput("$race_rank_border") << " " << rScores->time_;
                        else
                            line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                        line.SetPos(ranksPos + playerPos + time_sPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }
            else
            {
                tString ranksPosStr, ranksTitle;
                ranksPosStr << sg_RaceScores.Len();
                ranksTitle << tOutput("$race_rank_title_name");
                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                if (ranksTitle.Len() > ranksPosStr.Len())
                {
                    ranksPos = ranksTitle.Len() + 5;
                } else ranksPos = ranksPosStr.Len() + 5;

                for(int a = 0; a < sg_RaceScores.Len(); a++)
                {
                    gRaceScores *rScores = sg_RaceScores[a];
                    if (rScores)
                    {
                        tString time_Pos, time_Title;
                        time_Pos << rScores->time_;
                        time_Title << tOutput("$race_rank_title_time_name");
                        time_Title = tColoredString::RemoveColors(time_Title);

                        if (time_Title.Len() > time_Pos.Len()) time_sPos = time_Title.Len();
                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                    }
                }

                //ranksPos  += 5;
                playerPos += 4;
                time_sPos += 5;

                for(int b = 0;b < sg_RaceScores.Len(); b++)
                {
                    gRaceScores *rScores = sg_RaceScores[b];
                    if (rScores)
                    {
                        rank++;

                        tColoredString line;
                        tString strippedName;
                        bool isStripped = false;
                        if (rScores->userName_.Len() > playerPos)
                        {
                            strippedName << rScores->userName_.SubStr(0, playerPos - 1);
                            isStripped = true;
                        }

                        line << " " << tOutput("$race_rank_border") << " " << (rank);
                        line.SetPos(ranksPos, false);
                        line << " " << tOutput("$race_rank_border") << " " << rScores->userName_.SubStr(0, 15);
                        line.SetPos(ranksPos + playerPos, false);
                        if (rScores->time_ > 0)
                            line << " " << tOutput("$race_rank_border") << " " << rScores->time_;
                        else
                            line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                        line.SetPos(ranksPos + playerPos + time_sPos, false);
                        line << " " << tOutput("$race_rank_border") << "\n";
                        ranksList.Insert(line);
                    }
                }
            }

            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
            header.SetPos(ranksPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
            header.SetPos(ranksPos + playerPos, false);
            header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
            header.SetPos(ranksPos + playerPos + time_sPos, false);
            header << " " << tOutput("$race_rank_bordser") << "\n";

            mess << tOutput("$race_rank_title_message", rank) << " " << pz_mapName << "\n";
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
            int time_sPos    = 0;

            for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
            {
                ePlayerNetID *p = se_PlayerNetIDs[i];
                if (p && (p->IsHuman()))
                {
                    for(int j = 0; j < sg_RaceScores.Len(); j++)
                    {
                        gRaceScores *rScores = sg_RaceScores[j];
                        if (rScores)
                        {
                            if ((rScores->userName_ == p->GetUserName()) || (p->HasLoggedIn() && (rScores->userName_ == p->GetAuthenticatedName())))
                            {
                                tColoredString mess, line, header;

                                rank = j + 1;
                                mess << tOutput("$race_ranks_personal_message") << pz_mapName << "\n";

                                tString ranksPosStr, ranksTitle;
                                ranksPosStr << sg_RaceScores.Len();
                                ranksTitle << tOutput("$race_rank_title_name");
                                ranksTitle = tColoredString::RemoveColors(ranksTitle);

                                if (ranksTitle.Len() > ranksPosStr.Len())
                                {
                                    ranksPos = ranksTitle.Len() + 5;
                                } else ranksPos = ranksPosStr.Len() + 5;

                                for(int a = 0; a < sg_RaceScores.Len(); a++)
                                {
                                    gRaceScores *rScores = sg_RaceScores[a];
                                    if (rScores)
                                    {
                                        tString time_Pos, time_Title;
                                        time_Pos << rScores->time_;
                                        time_Title << tOutput("$race_rank_title_time_name");
                                        time_Title = tColoredString::RemoveColors(time_Title);

                                        if (time_Title.Len() > time_Pos.Len()) time_sPos = time_Title.Len();
                                        else if (time_Pos.Len() > time_sPos) time_sPos = time_Pos.Len();
                                    }
                                }

                                //ranksPos  += 5;
                                playerPos += 4;
                                time_sPos += 5;

                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_name");
                                header.SetPos(ranksPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_player_name");
                                header.SetPos(ranksPos + playerPos, false);
                                header << " " << tOutput("$race_rank_border") << " " << tOutput("$race_rank_title_time_name");
                                header.SetPos(ranksPos + playerPos + time_sPos, false);
                                header << " " << tOutput("$race_rank_border") << "\n";
                                mess << header;

                                if ((j - 1) >= 0)
                                {
                                    gRaceScores *prev = sg_RaceScores[j - 1];
                                    line << " " << tOutput("$race_rank_border") << " " << (rank - 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tOutput("$race_rank_border") << " " << prev->userName_.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    if (prev->time_ > 0)
                                        line << " " << tOutput("$race_rank_border") << " " << prev->time_;
                                    else
                                        line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                                    line.SetPos(ranksPos + playerPos + time_sPos, false);
                                    line << " " << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                    mess << line;
                                    line = "";
                                }

                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << (rank);
                                line.SetPos(ranksPos, false);
                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << rScores->userName_.SubStr(0, 15);
                                line.SetPos(ranksPos + playerPos, false);
                                if (rScores->time_ > 0)
                                    line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << rScores->time_;
                                else
                                    line << " " << sg_raceHighliter << tOutput("$race_rank_border") << " " << sg_raceHighliter << "UNDONE";
                                line.SetPos(ranksPos + playerPos + time_sPos, false);
                                line << " " << sg_raceHighliter << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
                                mess << line;
                                line = "";

                                if ((j + 1) <= (sg_RaceScores.Len() - 1))
                                {
                                    gRaceScores *next = sg_RaceScores[j + 1];
                                    line << " " << tOutput("$race_rank_border") << " " << (rank + 1);
                                    line.SetPos(ranksPos, false);
                                    line << " " << tOutput("$race_rank_border") << " " << next->userName_.SubStr(0, 15);
                                    line.SetPos(ranksPos + playerPos, false);
                                    if (next->time_ > 0)
                                        line << " " << tOutput("$race_rank_border") << " " << next->time_;
                                    else
                                        line << " " << tOutput("$race_rank_border") << " " << "UNDONE";
                                    line.SetPos(ranksPos + playerPos + time_sPos, false);
                                    line << " " << tOutput("$race_rank_border") << tColoredString::ColorString( 1,1,1 ) << "\n";
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
gRacePlayer::gRacePlayer(ePlayerNetID *player)
{
    this->player_ = player;
    this->cycle_ = NULL;

    this->time_  = -1;
    this->score_ = 0;

    this->lastTime_ = se_Time();

    this->chances_ = sg_raceChances;

    this->idle_ = false;
    this->idleLastTime_ = 0;
    this->idleNextTime_ = 0;

    sg_RacePlayers.Insert(this);
}

bool gRacePlayer::PlayerExists(tString username)
{
    if (sg_RacePlayers.Len() > 0)
    {
        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *rPlayer = sg_RacePlayers[i];
            if (rPlayer && rPlayer->Player())
            {
                if (rPlayer->Player()->HasLoggedIn() && (rPlayer->Player()->GetAuthenticatedName() == username))
                    return true;

                if (rPlayer->Player()->GetUserName() == username)
                    return true;
            }
        }
    }
    return false;
}

gRacePlayer *gRacePlayer::GetPlayer(tString username)
{
    if (sg_RacePlayers.Len() > 0)
    {
        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *rPlayer = sg_RacePlayers[i];
            if (rPlayer && rPlayer->Player())
            {
                if (rPlayer->Player()->HasLoggedIn() && (rPlayer->Player()->GetAuthenticatedName() == username))
                    return rPlayer;

                if (rPlayer->Player()->GetUserName() == username)
                    return rPlayer;
            }
        }
    }
    return NULL;
}

bool gRacePlayer::PlayerExists(ePlayerNetID *player)
{
    if (sg_RacePlayers.Len() > 0)
    {
        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *rPlayer = sg_RacePlayers[i];
            if (rPlayer && rPlayer->Player())
            {
                if (rPlayer->Player() == player)
                    return true;
            }
        }
    }
    return false;
}

gRacePlayer *gRacePlayer::GetPlayer(ePlayerNetID *player)
{
    if (sg_RacePlayers.Len() > 0)
    {
        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *rPlayer = sg_RacePlayers[i];
            if (rPlayer && rPlayer->Player())
            {
                if (rPlayer->Player() == player)
                    return rPlayer;
            }
        }
    }
    return NULL;
}

void gRacePlayer::ErasePlayer()
{
    for(int i = 0; i < sg_RacePlayers.Len(); i++)
    {
        gRacePlayer *rPlayer = sg_RacePlayers[i];
        if (rPlayer && (rPlayer == this))
        {
            if (rPlayer->Finished())
            {
                for(int j = 0; j < sg_RaceFinished.Len(); j++)
                {
                    gRacePlayer *finishedPlayer = sg_RaceFinished[j];
                    if (finishedPlayer && (finishedPlayer == this))
                    {
                        sg_RaceFinished.RemoveAt(j);
                        break;
                    }
                }
            }

            sg_RacePlayers.RemoveAt(i);
            break;
        }
    }

    delete this;
}

//! ZONE HIT
void gRace::ZoneHit( ePlayerNetID *player, REAL time )
{
    tString player_username;
    if (player->HasLoggedIn() && (player->GetAuthenticatedName().Len() > 1))
        player_username = player->GetAuthenticatedName();
    else
        player_username = player->GetUserName();

    gRacePlayer *racePlayer = gRacePlayer::GetPlayer(player_username);

    if (player && racePlayer && !racePlayer->Finished() && !roundFinished_ && player->Object() && player->Object()->Alive())
    {
        REAL reachtime_ = time;

        if (!firstArrived_)
        {
            firstFinishTime_ = reachtime_;
            firstToArive_ = player->GetName();
        }

        sg_RaceFinished.Insert(racePlayer);

        racePlayer->SetFinished(true);
        racePlayer->SetTime(reachtime_);
        racePlayer->SetScore(RacingScore);

        //  if enabled, ensure that only logged in players will get registered in the ranks
        //  if disabled, all players will get logged in race records and their time of finishing
        if ((sg_raceLogLogin && racePlayer->Player()->HasLoggedIn()) || (!sg_raceLogLogin))
        {
            //  ensure that punished players cannot be ranked
            if (racePlayer->Player()->GetAccessLevel() == tAccessLevel_Punished)
            {
                sn_ConsoleOut(tOutput("$race_nolog", racePlayer->Player()->GetName()), racePlayer->Player()->Owner());
            }
            else
            {
                gRaceScores::Add(racePlayer);
            }
        }
        else
        {
            if (sg_raceLogLogin)
            {
                sn_ConsoleOut(tOutput("$race_login_required"), racePlayer->Player()->Owner());
            }
        }


        tOutput win; //, lose;

        finishPlace_++;
        if (!sg_raceLogTime)
        {
            win.SetTemplateParameter(1, player->GetName() );
            win.SetTemplateParameter(2, racePlayer->Score());
            win.SetTemplateParameter(3, racePlayer->Time());
            win << "$player_reach_race";
        }
        else
        {
            if (!firstArrived_)
            {
                win.SetTemplateParameter(1, player->GetName());
                win.SetTemplateParameter(2, racePlayer->Time());
                win << "$player_reach_race_first";

                firstArrived_ = true;
            }
            else
            {
                win.SetTemplateParameter(1, player->GetName());
                win.SetTemplateParameter(2, finishPlace_);
                win.SetTemplateParameter(3, racePlayer->Time());
                win.SetTemplateParameter(4, (racePlayer->Time() - firstFinishTime_));
                win.SetTemplateParameter(5, firstToArive_);
                win << "$player_reach_race_time";
            }
        }

        if (!sg_racePointsType)
        {
            player->AddScore( sg_scoreRaceFinish, win, tOutput() );
        }
        else
        {
            player->AddScore( RacingScore, win, tOutput() );
            if (RacingScore >= 2) RacingScore -= sg_scoreRaceDeplete;
        }

        if (sg_raceFinishKill)
            player->Object()->Kill();
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans, REAL time )
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return;

    //  check chances and ensure players respawn
    if (!roundFinished_ && (sg_raceChances > 0))
    {
        if (grid)
        {
            for(int a = 0; a < sg_RacePlayers.Len(); a++)
            {
                gRacePlayer *rPlayer = sg_RacePlayers[a];
                if (rPlayer)
                {
                    if ((rPlayer->Cycle()) && rPlayer->Player() && !rPlayer->Finished())
                    {
                        if ((rPlayer->Chances() > 0) && (rPlayer->Chances() <= sg_raceChances))
                        {
                            //gRacePlayer::CreateNewCycle(rPlayer);
                            gCycle *cycle = new gCycle(grid, rPlayer->SpawnPosition(), rPlayer->SpawnDirection(), rPlayer->Player());
                            rPlayer->NewCycle(cycle);
                            rPlayer->Player()->ControlObject(cycle);
                            rPlayer->DropChances();    //  decrease chances by this many values
                            alive += 1;
                        }
                    }
                }
            }
        }
    }

    // close the round if no human is alive
    if ( alive == 0 )
        roundFinished_ = true;

    if (!roundFinished_ && sg_raceIdleKill)
    {
        for(int x = 0; x < sg_RacePlayers.Len(); x++)
        {
            gRacePlayer *racePlayer = sg_RacePlayers[x];
            if (racePlayer && racePlayer->Player() && !racePlayer->Player()->IsSpectating() && racePlayer->Player()->IsHuman())
            {
                if (racePlayer->Cycle() && racePlayer->Cycle()->Team() && racePlayer->Cycle()->Alive())
                {
                    //  check if player's speed is at idle or not
                    if (racePlayer->Cycle()->Speed() <= sg_raceIdleSpeed)
                    {
                        //  do second counting by 1
                        if ((time - racePlayer->IdleLastTime()) >= 1)
                        {
                            //  update the idle last time
                            racePlayer->SetIdleLastTime(time);

                            //  if the player is indeed travelling at idle speed
                            if (!racePlayer->IsIdle())
                            {
                                //  assign the next time for idle action to activate
                                if (racePlayer->IdleNextTime() == 0)
                                {
                                    racePlayer->SetIdleNextTime(time + sg_raceIdleTime);
                                }

                                if (time >= racePlayer->IdleNextTime())
                                {
                                    //  so they are idle
                                    racePlayer->SetIdle(true);

                                    //  reset the next time to apply later
                                    racePlayer->SetIdleNextTime(time + sg_raceIdleTime);

                                    tOutput msg;
                                    msg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                                    msg << "$race_idle_inform";
                                    sn_ConsoleOut(msg, racePlayer->Player()->Owner());
                                }
                            }
                            else
                            {
                                if (time >= racePlayer->IdleNextTime())
                                {
                                    //  time up! Let's kill them!
                                    racePlayer->Cycle()->Kill();

                                    racePlayer->SetIdle(false);

                                    tOutput msg;
                                    msg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                                    msg << "$race_idle_kill";
                                    sn_ConsoleOut(msg);
                                }
                            }
                        }
                    }
                    else
                    {
                        //  good, player doesn't need to get killed for being idle
                        racePlayer->SetIdle(false);
                        racePlayer->SetIdleLastTime(0);
                        racePlayer->SetIdleNextTime(0);
                    }
                }
            }
        }
    }

    // start counter when someone arrives or when only 1 is alive but not alone
    if (!roundFinished_ && ( firstArrived_ || ( alive == 1 && ai_alive == 0 && humans > 1 ) ))
    {
        /*if (!sg_raceFinishKill)
            if (sg_RaceFinished.Len() == alive)
                roundFinished_ = true;*/

        // freestyle mode
        if ( sg_RaceEndDelay < 0 )
        {
            if ( sg_RaceEndDelay < -1 )
                sg_RaceEndDelay = -1;
        }
        // countdown mode
        else
        {
            if (countDown_ < 0)
            {
                if (sg_raceSmartTimer)
                {
                    if (sg_RaceScores.Len() == 0)
                    {
                        countDown_ = sg_RaceEndDelay + 1;
                    }
                    if (sg_RaceScores.Len() == 1)
                    {
                        gRaceScores *rScores = sg_RaceScores[0];
                        if (rScores)
                        {
                            int timer = ceil(rScores->Time());
                            countDown_ = ceil(timer * 1.2) + 1;
                        }
                    }
                    else if (sg_RaceScores.Len() == 2)
                    {
                        gRaceScores *one = sg_RaceScores[0];
                        gRaceScores *two = sg_RaceScores[1];
                        if (one && two)
                        {
                            int timer = ceil((one->Time() + two->Time()) / 2);
                            countDown_ = ceil(timer * 1.2) + 1;
                        }
                    }
                    else if (sg_RaceScores.Len() >= 3)
                    {
                        gRaceScores *one = sg_RaceScores[0];
                        gRaceScores *two = sg_RaceScores[1];
                        gRaceScores *tre = sg_RaceScores[2];
                        if (one && two && tre)
                        {
                            int timer = ceil((one->Time() + two->Time() + tre->Time()) / 3);
                            countDown_ = ceil(timer * 1.2) + 1;
                        }
                    }
                }
                else
                {
                    countDown_ = sg_RaceEndDelay + 1;
                }
            }

            if ( !roundFinished_ )
            {
                // countdown working
                countDown_ --;

                if ( countDown_ < 0 )
                {
                    roundFinished_ = true;
                    countDown_ = -1;

                    //if (!firstArrived_)
                    //{
                        //  kill all players that haven't finished yet
                        for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
                        {
                            ePlayerNetID *p = se_PlayerNetIDs[i];
                            if (p && p->Object() && p->Object()->Alive())
                                p->Object()->Kill();
                        }
                    //}
                }
                else
                {
                    tOutput message;
                    message.SetTemplateParameter(1, countDown_);
                    message << "$timer_countdown" << "                    ";
                    sn_CenterMessage( message );
                }
            }
        }
    }

    //  once the round has finished, collapse all active zones
    if (roundFinished_)
    {
        //  vanish all zones
        const tList<eGameObject>& gameObjects = grid->GameObjects();
        for (int i = 0; i < gameObjects.Len(); i++)
        {
            gZone *Zone = dynamic_cast<gZone *>(gameObjects[i]);
            if (Zone)
            {
                //  0.5 fot smooth collapse
                Zone->Vanish(0.5f);
            }
        }
    }
    //  checks whether server should log the unfinished players
    if (roundFinished_ && sg_raceLogUnfinished)
    {
        //  if so, let's get to work
        for (int x = 0; x < sg_RacePlayers.Len(); x++)
        {
            gRacePlayer *racePlayer = sg_RacePlayers[x];
            if (racePlayer)
            {
                if (!racePlayer->Finished())
                {
                    //  set the default unfinished value
                    racePlayer->SetTime(-1);

                    //  log the players by time at -1
                    gRaceScores::Add(racePlayer, false);

                    //  now set that they have completed
                    racePlayer->SetFinished(true);
                }
            }
        }
    }

    if (roundFinished_ && !sg_raceOutputSent)
    {
        gRaceScores::OutputEnd();
        sg_raceOutputSent = true;
    }
}

//! DONE
bool gRace::Done()
{
    return roundFinished_;
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

    sg_raceOutputSent = false;

    //  reset race players list
    for(int b = 0; b < sg_RacePlayers.Len(); b++)
    {
        gRacePlayer *rPlayer = sg_RacePlayers[b];
        if (rPlayer)
        {
            rPlayer->SetTime(-1);
            rPlayer->SetScore(0);
            rPlayer->SetFinished(false);
            rPlayer->DestroyCycle();

            rPlayer->SetIdle(false);
            rPlayer->SetIdleLastTime(0);
            rPlayer->SetIdleNextTime(0);
        }
    }

    sg_RaceFinished.Clear();

    //gRaceScores::Write();
    gRaceScores::Reset();
    gRaceScores::Read();
}

//! time_ TO STRING
static tString sg_time_ToString ( REAL time_ )
{
    int seconds = time_;
    REAL ms = time_ - seconds;
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

eTeam *gRace::Winner()
{
    //  no need to declare once its done
    if (winnerDeclared_) return NULL;

    //  declaring the first to finish
    if (sg_RaceFinished.Len() > 0)
    {
        gRacePlayer *racePlayer = sg_RaceFinished[0];
        if (racePlayer)
        {
            winnerDeclared_ = true;
            return racePlayer->Player()->CurrentTeam();
        }
    }
}

void gRace::RaceChat(ePlayerNetID *player, tString command, std::istream &s)
{
    if (player)
    {
        tString params;
        int pos = 0;

        params.ReadLine(s);

        //  check on their stats
        if (command == "stats")
        {
            //  sort out the records
            gRaceScores::Sort();

            if(params.Filter() == "")
            {
                gRaceScores *selScores = NULL;
                for(int i = 0; i < sg_RaceScores.Len(); i++)
                {
                    gRaceScores *rScores = sg_RaceScores[i];
                    if (rScores)
                    {
                        if (rScores->Name() == player->GetUserName())
                        {
                            selScores = rScores;
                            break;
                        }
                    }
                }

                if (selScores)
                {
                    tOutput message;
                    message.SetTemplateParameter(1, selScores->Rank());
                    message.SetTemplateParameter(2, selScores->Time());
                    message << "$player_race_stats_self";

                    sn_ConsoleOut(message, player->Owner());
                }
            }
            else
            {
                tString player_name = params.ExtractNonBlankSubString(pos);
                if (sg_RaceScores.Len() > 0)
                {
                    gRaceScores *selScores = NULL;
                    for(int i = 0; i < sg_RaceScores.Len(); i++)
                    {
                        gRaceScores *rScores = sg_RaceScores[i];
                        if (rScores)
                        {
                            if (rScores->Name().Filter().Contains(player_name.Filter()))
                            {
                                selScores = rScores;
                                break;
                            }
                        }
                    }

                    if (selScores)
                    {
                        tOutput message;
                        message.SetTemplateParameter(1, selScores->Name());
                        message.SetTemplateParameter(2, selScores->Rank());
                        message.SetTemplateParameter(3, selScores->Time());
                        message << "$player_race_stats_other";

                        sn_ConsoleOut(message, player->Owner());
                    }
                }
            }
        }
        else if ((command == "ls") || (command == "list"))
        {
            tOutput amount, arrive;
            if (sg_RaceFinished.Len() > 0)
            {
                int max = 0;

                if (sg_RaceFinished.Len() >= 10)
                    max = 10;
                else
                    max = sg_RaceFinished.Len();

                //  display only the maximum allowed
                for(int i = 0; i < max; i++)
                {
                    gRacePlayer *racePlayer = sg_RaceFinished[i];
                    if (racePlayer)
                    {
                        tOutput message;
                        message.SetTemplateParameter(1, racePlayer->Player()->GetUserName());
                        message.SetTemplateParameter(2, racePlayer->Time());
                        message.SetTemplateParameter(3, racePlayer->Time() - firstFinishTime_);
                        message << "$race_finished_list_display";

                        sn_ConsoleOut(message, player->Owner());
                    }
                }

                //  message about displaying the allowed number of finished players
                arrive.SetTemplateParameter(1, max);
                arrive << "$race_finished_list_arrive";
                sn_ConsoleOut(arrive, player->Owner());

                //  message about the total number of players finished crossing the map
                amount.SetTemplateParameter(1, sg_RaceFinished.Len());
                amount << "$race_finished_list_amount";
                sn_ConsoleOut(amount, player->Owner());
            }
            else
            {
                tOutput message;
                message << "$race_finished_list_nobody";
                sn_ConsoleOut(message, player->Owner());
            }
        }
        else if (command == "load")
        {
            gRaceScores::Read();
            sn_ConsoleOut(tOutput("$race_records_manual_load", player->GetColoredName()));
        }
        else if (command == "save")
        {
            gRaceScores::Write();
            sn_ConsoleOut(tOutput("$race_records_manual_save", player->GetColoredName()));
        }
        else if (command == "help")
        {
            tOutput message;
            message << "0x66ff22!race ls           0x0055ff: Lists all of those who have already crossed the finish line.\n";
            message << "0x66ff22!race stats <name> 0x0055ff: Lists the current stats of players by <name>. Leave it blank to view your own stats.\n";
            sn_ConsoleOut(message, player->Owner());
        }
        else
        {
            tOutput message;
            message << "0xffdd00You seem to not know how this works. Check this out!\n";
            message << "0x66ff22!race help 0x0055ff: Lists all available commands for racing.\n";
            sn_ConsoleOut(message, player->Owner());
        }
    }
}
