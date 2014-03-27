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
static tSettingItem<bool> sg_RaceTimerEnabledConf( "RACE_TIMER_ENABLED", sg_RaceTimerEnabled);

int sg_RaceLaps = 1;
static tSettingItem<int> sg_RaceLapsConf("RACE_LAPS", sg_RaceLaps);

bool requireCheckpointHit(const int &newValue)
{
    if ((newValue < 0) || (newValue > 2)) return false;
    return true;
}
int sg_RaceCheckpointRequireHit = 1;
static tSettingItem<int> sg_RaceCheckpointRequireHitConf("RACE_CHECKPOINT_REQUIRE_HIT", sg_RaceCheckpointRequireHit, &requireCheckpointHit);

int sg_RaceCheckpointCountdown = 70;
static tSettingItem<int> sg_RaceCheckpointCountdownConf("RACE_CHECKPOINT_COUNTDOWN", sg_RaceCheckpointCountdown);

bool sg_RaceCheckpointLaps = true;
static tSettingItem<bool> sg_RaceCheckpointLapsConf("RACE_CHECKPOINT_LAPS", sg_RaceCheckpointLaps);

tString sg_RaceSafeAngles("");
static tSettingItem<tString> sg_RaceSafeAnglesConf("RACE_SAFE_ANGLES", sg_RaceSafeAngles);

bool sg_RaceUnsafeAnglesKill = false;
static tSettingItem<bool> sg_RaceUnsafeAnglesKillConf("RACE_UNSAFE_ANGLES_KILL", sg_RaceUnsafeAnglesKill);

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

static bool sg_raceFinishKill = true;
static tSettingItem<bool> sg_raceFinishKillConf("RACE_FINISH_KILL", sg_raceFinishKill);

static bool sg_raceLogLogin = true;
static tSettingItem<bool> sg_raceLogLoginConf("RACE_LOG_LOGIN", sg_raceLogLogin);

static bool sg_raceIdleKill = false;
static tSettingItem<bool> sg_raceIdleKillConf("RACE_IDLE_KILL", sg_raceIdleKill);

static REAL sg_raceIdleTime  = 5;
static REAL sg_raceIdleSpeed = 30;
bool restrictRaceIdle(const REAL &newValue)
{
    if (newValue < 0) return false;
    return true;
}

static tSettingItem<REAL> sg_raceIdleTimeConf("RACE_IDLE_TIME", sg_raceIdleTime, &restrictRaceIdle);
static tSettingItem<REAL> sg_raceIdleSpeedConf("RACE_IDLE_SPEED", sg_raceIdleSpeed, &restrictRaceIdle);

static int sg_raceIdleWarnings = 1;
bool restrictRaceIdleWarnings(const int &newValue)
{
    if (newValue <= 0) return false;
    return true;
}
static tSettingItem<int> sg_raceIdleWarningsConf("RACE_IDLE_WARNINGS", sg_raceIdleWarnings, &restrictRaceIdleWarnings);

static bool sg_raceFinishCollapse = true;
static tSettingItem<bool> sg_raceFinishCollapseConf("RACE_FINISH_COLLAPSE", sg_raceFinishCollapse);

static REAL sg_raceSmartTimerFactor = 1.2;
static tSettingItem<REAL> sg_raceSmartTimerFactorConf("RACE_SMART_TIMER_FACTOR", sg_raceSmartTimerFactor, &restrictRaceIdle);

static int sg_raceSmartTimerNum = 3;
static tSettingItem<int> sg_raceSmartTimerNumConf("RACE_SMART_TIMER_NUM", sg_raceSmartTimerNum, &restrictRaceIdleWarnings);

//! STATIC VARIABLES
bool gRace::firstArrived_ = false;
int  gRace::countDown_ = -1;
bool gRace::roundFinished_ = false;
bool gRace::winnerDeclared_ = false;
int gRace::finishPlace_ = 0;
REAL gRace::firstFinishTime_ = 0;
tString gRace::firstToArive_;

bool gRace::cannotFinish_[MAXCLIENTS+1];

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
    REAL oldTime = 0;
    bool newRacer = false;
    gRaceScores *racingPlayer = NULL;
    int prevRank = 0;

    tString username;
    if (racePlayer->Player()->HasLoggedIn() && (racePlayer->Player()->GetAuthenticatedName().Filter() != ""))
        username = racePlayer->Player()->GetAuthenticatedName();
    else
        username = racePlayer->Player()->GetUserName();

    if (CheckPlayer(username))
    {
        racingPlayer = GetPlayer(username);

        //  ensure that only finished racers go through this code
        if ((racePlayer->Time() > 0) && finished)
        {
            oldTime = racingPlayer->Time();

            if (((racePlayer->Time() < racingPlayer->Time()) && (racePlayer->Time() > 0)) || (racingPlayer->Time() <= 0 && racePlayer->Time() > 0))
            {
                //  store the player's previous rank if their got better or worse
                prevRank = racingPlayer->Rank();

                racingPlayer->time_ = racePlayer->Time();

                //  send message if this player isn't rank 1
                if (racingPlayer->Rank() > 1)
                {
                    tOutput newtime;
                    newtime.SetTemplateParameter(1, racePlayer->Time());
                    newtime << "$player_personal_best_reach_time";
                    sn_ConsoleOut(newtime, racePlayer->Player()->Owner());
                }

                timeChanged = true;
            }
        }
    }
    else
    {
        racingPlayer = new gRaceScores(username);
        racingPlayer->time_ = racePlayer->Time();
        oldTime = racingPlayer->Time();

        if (racePlayer->Time() > 0)
        {
            if (racingPlayer->Rank() > 1)
            {
                tOutput newtime;
                newtime.SetTemplateParameter(1, racePlayer->Time());
                newtime << "$player_personal_best_reach_time";
                sn_ConsoleOut(newtime, racePlayer->Player()->Owner());
            }

            timeChanged = true;
        }

        newRacer = true;
    }

    //  sort out ranks once player get better time
    Sort();

    if (finished)
    {
        //  increment finished times when they crossed the zone
        racingPlayer->SetPlayed(racingPlayer->Played() + 1);

        //  update the last time due to them finishing the map yet again
        racingPlayer->SetLastTime(se_Time());
    }

    //  ensure that times really have changed for this to take effect
    if (timeChanged && finished)
    {
        //  get top player from that list
        gRaceScores *firstRanker = sg_RaceScores[0];

        //  check if it's the same player
        if (firstRanker == racingPlayer)
        {
            tOutput bestTime;
            bestTime.SetTemplateParameter(1, racePlayer->Player()->GetName());
            bestTime.SetTemplateParameter(2, firstRanker->Time());
            bestTime.SetTemplateParameter(3, pz_mapName);
            bestTime << "$race_player_hold_best_time";
            sn_ConsoleOut(bestTime);

            LogWinnerCycleTurns(dynamic_cast<gCycle *>(racePlayer->Player()->Object()));
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
                    rankMsg.SetTemplateParameter(3, oldTime - racePlayer->Time());
                    rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
                    rankMsg << "$race_player_hold_same_rank";
                }
                else if (prevRank > racingPlayer->Rank())
                {
                    rankMsg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                    rankMsg.SetTemplateParameter(2, racePlayer->Time());
                    rankMsg.SetTemplateParameter(3, oldTime - racePlayer->Time());
                    rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
                    rankMsg << "$race_player_hold_rank_up";
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
            rankMsg.SetTemplateParameter(3, racePlayer->Time() - oldTime);
            rankMsg.SetTemplateParameter(4, racingPlayer->Rank());
            rankMsg << "$race_player_hold_slow_time";

            sn_ConsoleOut(rankMsg);
        }
    }
}

bool sg_raceRecordsLoad = true;
static tSettingItem<bool> sg_raceRecordsLoadConf("RACE_RECORDS_LOAD", sg_raceRecordsLoad);

void gRaceScores::Read()
{
    if (!sg_raceRecordsLoad) return;

    mapFile_ = mapfile;
    int pos = mapFile_.StrPos(0, "(");
    if (pos > 0)
        mapFile_ = mapFile_.SubStr(0, pos);

    tString Input;
    //mapFile << pz_mapAuthor << "/" << pz_mapCategory << "/" << pz_mapName << "-" << pz_mapVersion << ".aamap.xml";
    Input << "race_scores/" << mapFile_ << ".txt";
#ifdef DEBUG
    sn_ConsoleOut(tOutput("$race_ranks_loading", pz_mapName));
#endif

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
#ifdef DEBUG
    sn_ConsoleOut(tOutput("$race_ranks_saving", pz_mapName));
#endif

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

bool restrictRaceRankStringLength(const int &newValue)
{
    if (newValue < 7) return false;
    return true;
}
static int sg_raceRankShowPlayerLength = 15;
static int sg_raceRankShowLength = 7;
static tSettingItem<int> sg_raceRankShowPlayerLengthConf("RACE_RANK_SHOW_PLAYER_LENGTH", sg_raceRankShowPlayerLength, &restrictRaceRankStringLength);
static tSettingItem<int> sg_raceRankLengthConf("RACE_RANK_SHOW_LENGTH", sg_raceRankShowLength, &restrictRaceRankStringLength);

bool restrictRaceHeader(const int &newValue)
{
    if (newValue < 0) return false;
    return true;
}

static int sg_raceRankHeaderOrder       = 1;
static int sg_raceRankHeaderPlayerOrder = 2;
static int sg_raceRankHeaderTimeOrder   = 3;
static tSettingItem<int> sg_raceRankHeaderOrderConf("RACE_RANK_HEADER_ORDER", sg_raceRankHeaderOrder, &restrictRaceHeader);
static tSettingItem<int> sg_raceRankHeaderPlayerOrderConf("RACE_RANK_HEADER_PLAYER_ORDER", sg_raceRankHeaderPlayerOrder, &restrictRaceHeader);
static tSettingItem<int> sg_raceRankHeaderTimeOrderConf("RACE_RANK_HEADER_TIME_ORDER", sg_raceRankHeaderTimeOrder, &restrictRaceHeader);

static int sg_raceRankHeaderLength = 8;
static int sg_raceRankHeaderPlayerLength = 16;
static int sg_raceRankHeaderTimeLength = 0;
static tSettingItem<int> sg_raceRankHeaderLengthConf("RACE_RANK_HEADER_LENGTH", sg_raceRankHeaderLength, &restrictRaceHeader);
static tSettingItem<int> sg_raceRankHeaderPlayerLengthConf("RACE_RANK_HEADER_PLAYER_LENGTH", sg_raceRankHeaderPlayerLength, &restrictRaceHeader);
static tSettingItem<int> sg_raceRankHeaderTimeLengthConf("RACE_RANK_HEADER_TIME_LENGTH", sg_raceRankHeaderTimeLength, &restrictRaceHeader);

void gRaceScores::OutputTopRecords(int show)
{
    if (show == 0) return;

    sn_ConsoleOut(tOutput("$race_rank_title_message", show, pz_mapName));

    tString rankTitleStr(tOutput("$race_rank_title_name"));
    tString rankPlayerTitleStr(tOutput("$race_rank_title_player_name"));
    tString rankTimeTitleStr(tOutput("$race_rank_title_time_name"));
    tString rankBorderStr(tOutput("$race_rank_border"));

    tString rankTitleStrNoColors(tColoredString::RemoveColors(rankTitleStr));
    tString rankPlayerTitleStrNoColors(tColoredString::RemoveColors(rankPlayerTitleStr));
    tString rankTimeTitleStrNoColors(tColoredString::RemoveColors(rankTimeTitleStr));

    if (rankTitleStrNoColors.Len() <= sg_raceRankHeaderLength)
    {
        tString rankTitleStrAppend;
        rankTitleStrAppend.SetPos(sg_raceRankHeaderLength - rankTitleStrNoColors.Len(), false);
        rankTitleStr << rankTitleStrAppend;
    }

    if (rankPlayerTitleStrNoColors.Len() <= sg_raceRankHeaderPlayerLength)
    {
        tString rankTitlePlayerStrAppend;
        rankTitlePlayerStrAppend.SetPos(sg_raceRankHeaderPlayerLength - rankPlayerTitleStrNoColors.Len(), false);
        rankPlayerTitleStr << rankTitlePlayerStrAppend;
    }

    if (rankTimeTitleStrNoColors.Len() <= sg_raceRankHeaderTimeLength)
    {
        tString rankTitleTimeStrAppend;
        rankTitleTimeStrAppend.SetPos(sg_raceRankHeaderTimeLength - rankTimeTitleStrNoColors.Len(), false);
        rankTimeTitleStr << rankTitleTimeStrAppend;
    }

    tOutput msg_header;
    msg_header.SetTemplateParameter(sg_raceRankHeaderOrder, rankTitleStr);
    msg_header.SetTemplateParameter(sg_raceRankHeaderPlayerOrder, rankPlayerTitleStr);
    msg_header.SetTemplateParameter(sg_raceRankHeaderTimeOrder, rankTimeTitleStr);
    msg_header << "$race_rank_message_header";
    sn_ConsoleOut(msg_header);

    for(int i = 0; i < show; i++)
    {
        gRaceScores *racePlayer = sg_RaceScores[i];
        if (racePlayer)
        {
            tOutput msg;

            tString rankStr;
            rankStr << racePlayer->Rank();
            rankStr.SetPos(sg_raceRankShowLength, false);
            msg.SetTemplateParameter(1, rankStr);

            tString playerNameStr = racePlayer->Name();
            if (playerNameStr.Len() > sg_raceRankShowPlayerLength)
                playerNameStr = playerNameStr.SubStr(0, sg_raceRankShowPlayerLength - 1);
            if (playerNameStr.Len() < sg_raceRankShowPlayerLength)
                playerNameStr.SetPos(sg_raceRankShowPlayerLength, false);

            msg.SetTemplateParameter(2, playerNameStr);

            if (racePlayer->Time() > 0)
                msg.SetTemplateParameter(3, racePlayer->Time());
            else
                msg.SetTemplateParameter(3, "UNDONE");

            msg << "$race_rank_message_format";
            sn_ConsoleOut(msg);
        }
    }
}

void gRaceScores::OutputIndividualRecords()
{
    tString rankTitleStr(tOutput("$race_rank_title_name"));
    tString rankPlayerTitleStr(tOutput("$race_rank_title_player_name"));
    tString rankTimeTitleStr(tOutput("$race_rank_title_time_name"));
    tString rankBorderStr(tOutput("$race_rank_border"));

    tString rankTitleStrNoColors(tColoredString::RemoveColors(rankTitleStr));
    tString rankPlayerTitleStrNoColors(tColoredString::RemoveColors(rankPlayerTitleStr));

    if (rankTitleStrNoColors.Len() <= sg_raceRankHeaderLength)
    {
        tString rankTitleStrAppend;
        rankTitleStrAppend.SetPos(sg_raceRankHeaderLength - rankTitleStrNoColors.Len(), false);
        rankTitleStr << rankTitleStrAppend;
    }

    if (rankPlayerTitleStrNoColors.Len() <= sg_raceRankHeaderPlayerLength)
    {
        tString rankTitlePlayerStrAppend;
        rankTitlePlayerStrAppend.SetPos(sg_raceRankHeaderPlayerLength - rankPlayerTitleStrNoColors.Len(), false);
        rankPlayerTitleStr << rankTitlePlayerStrAppend;
    }

    for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];
        if (p)
        {
            tString player_username;
            if (p->HasLoggedIn() && (p->GetAuthenticatedName().Filter() != ""))
                player_username = p->GetAuthenticatedName();
            else
                player_username = p->GetUserName();

            gRaceScores *rcC = GetPlayer(player_username);
            if (rcC)
            {
                tOutput msg_header;
                msg_header.SetTemplateParameter(sg_raceRankHeaderOrder, rankTitleStr);
                msg_header.SetTemplateParameter(sg_raceRankHeaderPlayerOrder, rankPlayerTitleStr);
                msg_header.SetTemplateParameter(sg_raceRankHeaderTimeOrder, rankTimeTitleStr);
                msg_header << "$race_rank_message_header";
                sn_ConsoleOut(msg_header, p->Owner());

                int current_id = rcC->Rank() - 1;

                //  record above current
                if ((current_id - 1) >= 0)
                {
                    gRaceScores *rcP = sg_RaceScores[current_id - 1];
                    if (rcP)
                    {
                        tOutput msg;

                        tString rankStr;
                        rankStr << rcP->Rank();
                        rankStr.SetPos(sg_raceRankShowLength, false);
                        msg.SetTemplateParameter(1, rankStr);

                        tString playerNameStr = rcP->Name();
                        if (playerNameStr.Len() > sg_raceRankShowPlayerLength)
                            playerNameStr = playerNameStr.SubStr(0, sg_raceRankShowPlayerLength - 1);
                        if (playerNameStr.Len() < sg_raceRankShowPlayerLength)
                            playerNameStr.SetPos(sg_raceRankShowPlayerLength, false);

                        msg.SetTemplateParameter(2, playerNameStr);

                        if (rcP->Time() > 0)
                            msg.SetTemplateParameter(3, rcP->Time());
                        else
                            msg.SetTemplateParameter(3, "UNDONE");

                        msg << "$race_rank_message_format";
                        sn_ConsoleOut(msg, p->Owner());
                    }
                }

                tOutput msg;

                tString rankStr;
                rankStr << rcC->Rank();
                rankStr.SetPos(sg_raceRankShowLength, false);
                msg.SetTemplateParameter(1, rankStr);

                tString playerNameStr = rcC->Name();
                if (playerNameStr.Len() > sg_raceRankShowPlayerLength)
                    playerNameStr = playerNameStr.SubStr(0, sg_raceRankShowPlayerLength - 1);
                if (playerNameStr.Len() < sg_raceRankShowPlayerLength)
                    playerNameStr.SetPos(sg_raceRankShowPlayerLength, false);

                msg.SetTemplateParameter(2, playerNameStr);

                if (rcC->Time() > 0)
                    msg.SetTemplateParameter(3, rcC->Time());
                else
                    msg.SetTemplateParameter(3, "UNDONE");

                msg << "$race_rank_current_format";
                sn_ConsoleOut(msg, p->Owner());

                //  record below current
                if ((current_id + 1) < sg_RaceScores.Len())
                {
                    gRaceScores *rcN = sg_RaceScores[current_id + 1];
                    if (rcN)
                    {
                        tOutput msg;

                        tString rankStr;
                        rankStr << rcN->Rank();
                        rankStr.SetPos(sg_raceRankShowLength, false);
                        msg.SetTemplateParameter(1, rankStr);

                        tString playerNameStr = rcN->Name();
                        if (playerNameStr.Len() > sg_raceRankShowPlayerLength)
                            playerNameStr = playerNameStr.SubStr(0, sg_raceRankShowPlayerLength - 1);
                        if (playerNameStr.Len() < sg_raceRankShowPlayerLength)
                            playerNameStr.SetPos(sg_raceRankShowPlayerLength, false);

                        msg.SetTemplateParameter(2, playerNameStr);

                        if (rcN->Time() > 0)
                            msg.SetTemplateParameter(3, rcN->Time());
                        else
                            msg.SetTemplateParameter(3, "UNDONE");

                        msg << "$race_rank_message_format";
                        sn_ConsoleOut(msg, p->Owner());
                    }
                }
            }
        }
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

//  Output timed records of players at start of round
void gRaceScores::OutputStart()
{
    if (sg_raceRankShowStart == 0) return;
    if (sg_RaceScores.Len() == 0) return;

    Sort(); //  sort ranks
    //  show the top RACE_NUM_RANKS_SHOW_START records at the start of round
    if (sg_raceRankShowStart == 1)
    {
        //  number of ranks to display
        int ranks_show = sg_RaceScores.Len();
        if ((sg_raceNumRanksShowStart > 0) && (sg_RaceScores.Len() >= sg_raceNumRanksShowStart) && (sg_raceNumRanksShowStart < sg_RaceScores.Len()))
            ranks_show = sg_raceNumRanksShowStart;

        OutputTopRecords(ranks_show);
    }
    //  show individual records with a record above and below the current rank position
    else if (sg_raceRankShowStart == 2)
    {
        OutputIndividualRecords();
    }
}

//  Output timed records of players as the round finishes
void gRaceScores::OutputEnd()
{
    if (sg_raceRankShowEnd == 0) return;
    if (sg_RaceScores.Len() == 0) return;

    Sort();

    if (sg_raceRankShowEnd == 1)
    {
        //  number of ranks to display
        int ranks_show = sg_RaceScores.Len();
        if ((sg_raceNumRanksShowEnd > 0) && (sg_RaceScores.Len() >= sg_raceNumRanksShowEnd) && (sg_raceNumRanksShowEnd < sg_RaceScores.Len()))
            ranks_show = sg_raceNumRanksShowEnd;

        OutputTopRecords(ranks_show);
    }
    else if (sg_raceRankShowEnd == 2)
    {
        OutputIndividualRecords();
    }
}

//! Racing player's things
gRacePlayer::gRacePlayer(ePlayerNetID *player)
{
    this->player_ = player;

    this->time_  = -1;
    this->score_ = 0;

    this->lastTime_ = se_Time();

    this->hasFinished_ = false;
    this->isSafe_      = true;
    this->canFinish_   = true;

    this->chances_  = sg_raceChances;

    this->idle_         = false;
    this->idleCounter_  = 0;
    this->idleNextTime_ = 0;

    this->laps_ = 0;
    this->nextCheckpoint_  = -1;
    this->checkpointsDone_ = false;

    this->countdown_ = -1;

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
    gRacePlayer *racePlayer = gRacePlayer::GetPlayer(player);
    if (racePlayer && !racePlayer->Finished() && !roundFinished_ && player->Object() && player->Object()->Alive())
    {
        //  kill the player if they are not safe
        if (!racePlayer->IsSafe() && sg_RaceUnsafeAnglesKill)
        {
            player->Object()->Kill();
            return;
        }

        //  no need to repeat messages when once is enough
        //  this also includes the code within this statement
        if (!cannotFinish_[player->ListID()])
        {
            //  check if the checkpoints have been hit or not
            if ((sg_RaceCheckpointRequireHit > 0) && (sg_NumCheckpointZones() > 0))
            {
                int next_checkpoint = -1;

                tArray<gCheckpointZoneHack *> checkpointZonesList = sg_GetCheckpointZones();
                for(int i = 0; i < checkpointZonesList.Len(); i++)
                {
                    gCheckpointZoneHack *checkZone = checkpointZonesList[i];
                    if (checkZone)
                    {
                        int nextCheckZoneID = checkZone->CheckpointID();
                        bool checkpointDone = false;

                        std::deque<int>::iterator it = racePlayer->checkpointsDoneList.begin();
                        for(; it != racePlayer->checkpointsDoneList.end(); it++)
                        {
                            if (nextCheckZoneID == *it)
                            {
                                checkpointDone = true;
                                break;
                            }
                        }

                        //  don't process this checkpoint if it's done
                        if (checkpointDone) continue;

                        //  get the lowest checkpoint id (greater than 0)
                        if ((next_checkpoint == -1) || (nextCheckZoneID < next_checkpoint))
                            next_checkpoint = nextCheckZoneID;
                    }
                }

                //  if there is a checkpoint to assign, do it.
                if (next_checkpoint > 0)
                {
                    racePlayer->SetNextCheckpoint(next_checkpoint);
                    racePlayer->SetCanFinish(false);
                    racePlayer->SetCheckpointsDone(false);
                }
                //  if not, let them finish the race
                else
                {
                    racePlayer->SetCanFinish(true);
                    racePlayer->SetCheckpointsDone(true);
                }

                //  check whether the checkpoints are done or not
                if (!racePlayer->CheckpointsDone())
                {
                    racePlayer->SetCanFinish(false);
                    sn_ConsoleOut(tOutput("$race_checkpoint_miss", racePlayer->NextCheckpoint()), player->Owner());
                }
            }
            if (sg_RaceCheckpointRequireHit == 0)
                racePlayer->SetCanFinish(true);

            //  if the race laps is greater than 1
            if (sg_RaceLaps > 1)
            {
                bool laps_incremented = false;
                if (racePlayer->IsSafe())
                {
                    bool laps_process = true;

                    //  make sure laps process is not done until players complete all checkpoints
                    if ((sg_RaceCheckpointRequireHit > 0) && (sg_NumCheckpointZones() > 0) && !racePlayer->CheckpointsDone())
                        laps_process = false;

                    if (laps_process)
                    {
                        laps_incremented = true;
                        racePlayer->SetLaps(racePlayer->Laps() + 1);    //  increment the player's lap count
                    }
                }

                //  the number of laps done is less than required race_laps
                if (racePlayer->Laps() < sg_RaceLaps)
                {
                    racePlayer->SetCanFinish(false);
                    if (laps_incremented)
                    {
                        //  if the race checkpoints laps is enabled clear all the checkpoints done
                        if (sg_RaceCheckpointLaps)
                        {
                            racePlayer->SetNextCheckpoint(-1);
                            racePlayer->SetCheckpointsDone(false);
                            racePlayer->checkpointsDoneList.clear();
                        }

                        sn_ConsoleOut(tOutput("$race_laps_next", racePlayer->Laps(), (sg_RaceLaps - racePlayer->Laps())), player->Owner());
                    }
                }
                else racePlayer->SetCanFinish(true);
            }
            if (sg_RaceLaps <= 1)
                racePlayer->SetCanFinish(true);

            //  flag to indicate not to show this message again
            cannotFinish_[player->ListID()] = true;
        }

        //  don't let the player finish if they can't
        if (!racePlayer->CanFinish()) return;

        REAL reachtime_ = time;
        player->LogActivity(ACTIVITY_FINISHED_RACE);

        if (!firstArrived_)
        {
            firstFinishTime_ = reachtime_;
            firstToArive_    = player->GetName();
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

        firstArrived_ = true;

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

//!< ZONE OUT
void gRace::ZoneOut( ePlayerNetID *player, REAL time )
{
    if (cannotFinish_[player->ListID()])
        cannotFinish_[player->ListID()] = false;

    gRacePlayer *racePlayer = gRacePlayer::GetPlayer(player);
    if (racePlayer)
    {
        if (player->Object() && !player->Object()->Alive())
            racePlayer->SetCountdown(-1);
    }
}

//! SYNC
void gRace::Sync( int alive, int ai_alive, int humans, REAL time )
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return;

    if (se_mainGameTimer && ( se_mainGameTimer->speed <= 0 || se_mainGameTimer->Time() < 0 ) )
        return;

    //  check chances and ensure players respawn
    if (!roundFinished_ && (sg_raceChances > 0))
    {
        for(int a = 0; a < sg_RacePlayers.Len(); a++)
        {
            gRacePlayer *rPlayer = sg_RacePlayers[a];
            if (rPlayer && rPlayer->Player() && rPlayer->Player()->Object())
            {
                gCycle *rPCycle = dynamic_cast<gCycle *>(rPlayer->Player()->Object());
                if (rPCycle && !rPCycle->Alive() && !rPlayer->Finished() && (rPlayer->Chances() > 0) && (rPlayer->Chances() <= sg_raceChances))
                {
                    //gRacePlayer::CreateNewCycle(rPlayer);
                    gCycle *cycle = new gCycle(grid, rPlayer->SpawnPosition(), rPlayer->SpawnDirection(), rPlayer->Player());
                    rPlayer->Player()->ControlObject(cycle);
                    rPlayer->DropChances();    //  decrease chances by this many values
                    alive += 1;
                }
            }
        }
    }

    // close the round if no human is alive
    if ( ( alive == 0 ) && ( ai_alive >= 0 ) )
        roundFinished_ = true;

    if (!roundFinished_ && sg_raceIdleKill)
    {
        for(int x = 0; x < sg_RacePlayers.Len(); x++)
        {
            gRacePlayer *racePlayer = sg_RacePlayers[x];
            if (racePlayer && racePlayer->Player() && racePlayer->Player()->Object())
            {
                gCycle *rPCycle = dynamic_cast<gCycle *>(racePlayer->Player()->Object());

                //  ensure we have a cycle attached to this player
                if (rPCycle && rPCycle->Alive() && (rPCycle->Speed() <= sg_raceIdleSpeed))
                {
                    //  if the player is indeed travelling at idle speed
                    if (!racePlayer->IsIdle())
                    {
                        //  assign the next time for idle action to activate
                        if (racePlayer->IdleNextTime() == 0)
                            racePlayer->SetIdleNextTime(time + sg_raceIdleTime);

                        if (time >= racePlayer->IdleNextTime())
                        {
                            //  add counter to number of warnings displayed
                            racePlayer->AddIdleCounter();

                            tOutput msg;
                            msg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                            msg.SetTemplateParameter(2, racePlayer->IdleCounter());
                            msg.SetTemplateParameter(3, sg_raceIdleWarnings);
                            msg << "$race_idle_warning";
                            sn_ConsoleOut(msg, racePlayer->Player()->Owner());

                            if (racePlayer->IdleCounter() >= sg_raceIdleWarnings)
                            {
                                //  so they are idle
                                racePlayer->SetIdle(true);
                            }

                            //  reset the next time to apply later
                            racePlayer->SetIdleNextTime(time + sg_raceIdleTime);
                        }
                    }
                    else
                    {
                        if (time >= racePlayer->IdleNextTime())
                        {
                            //  time up! Let's kill them!
                            rPCycle->Kill();

                            tOutput msg;
                            msg.SetTemplateParameter(1, racePlayer->Player()->GetName());
                            msg << "$race_idle_kill";
                            sn_ConsoleOut(msg);
                        }
                    }
                }
                else
                {
                    //  good, player doesn't need to get killed for being idle
                    racePlayer->SetIdle(false);
                    racePlayer->SetIdleCounter(0);
                    racePlayer->SetIdleNextTime(0);
                }
            }
        }
    }

    // start counter when someone arrives or when only 1 is alive but not alone
    if (!roundFinished_ && ( firstArrived_ || ( alive == 1 && ai_alive == 0 && humans > 1 ) ))
    {
        // countdown mode
        if ((sg_RaceEndDelay > 0) && (countDown_ < 0))
        {
            countDown_ = sg_RaceEndDelay + 1;

            if (sg_raceSmartTimer && (sg_RaceScores.Len() > 0))
            {
                int max = sg_RaceScores.Len();
                if (max > sg_raceSmartTimerNum)
                    max = sg_raceSmartTimerNum;

                int timer = 0;
                for(int i = 0; i < max; i++)
                {
                    gRaceScores *rP = sg_RaceScores[i];
                    if (rP) timer += rP->Time();
                }

                if (timer > 0)
                    countDown_ = ceil((timer / max) * sg_raceSmartTimerFactor) + 1;
            }
        }

        if ((sg_RaceEndDelay > 0) && (countDown_ >= 0) )
        {
            // countdown working
            countDown_ --;

            if ( countDown_ < 0 )
            {
                roundFinished_ = true;
                countDown_ = -1;

                //if (!firstArrived_)
                //{
                    for(int i = 0; i < se_PlayerNetIDs.Len(); i++)
                    {
                        ePlayerNetID *p = se_PlayerNetIDs[i];
                        if (p && p->Object() && p->Object()->Alive())
                            p->Object()->Kill();
                    }
                //}
            }
        }
    }

    //  do the checkpoint countdown
    if (!roundFinished_ && (sg_NumCheckpointZones() > 0) && (sg_RaceCheckpointCountdown > 0))
    {
        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *racer = sg_RacePlayers[i];
            if (racer && racer->Player())
            {
                if (racer->Player()->Object() && racer->Player()->Object()->Alive() && !racer->Finished())
                {
                    //  set the racer's checkpoint countdown timer
                    if (racer->Countdown() == -1)
                        racer->SetCountdown(sg_RaceCheckpointCountdown + 1);

                    racer->SetCountdown(racer->Countdown() - 1);

                    if ( racer->Countdown() < 0 )
                    {
                        racer->SetCountdown(-1);
                        racer->Player()->Object()->Kill();
                    }
                }
            }
        }
    }

    //  show the countdown message
    if (!roundFinished_)
    {
        int NumberOfSpaces  = 22;
        int NumberOfEntries = 0;

        for(int i = 0; i < sg_RacePlayers.Len(); i++)
        {
            gRacePlayer *racer = sg_RacePlayers[i];
            if (racer && racer->Player())
            {
                tColoredString countdownMsg;

                //  increase the number of entries to show
                if (countDown_ >= 0)         NumberOfEntries++;
                if (racer->Countdown() >= 0) NumberOfEntries++;

                if (countDown_ >= 0)
                {
                    countdownMsg << "0xff7777" << countDown_;
                    countdownMsg.SetPos(NumberOfSpaces / NumberOfEntries);
                }

                if ( racer->Countdown() >= 0 )
                {
                    countdownMsg.SetPos(NumberOfSpaces / NumberOfEntries);
                    countdownMsg << "0xffff7f" << racer->Countdown();
                }

                if (NumberOfEntries > 0)
                    sn_CenterMessage(countdownMsg, racer->Player()->Owner());
            }
        }
    }

    //  once the round has finished, collapse all active zones
    if (roundFinished_ && sg_raceFinishCollapse)
    {
        //  vanish all zones
        const tList<eGameObject>& gameObjects = grid->GameObjects();
        for (int i = 0; i < gameObjects.Len(); i++)
        {
            gZone *Zone = dynamic_cast<gZone *>(gameObjects[i]);
            if (Zone)
            {
                //  0.5 fot smooth collapse
                Zone->Vanish(0.5);
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
            if (racePlayer && !racePlayer->Finished())
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

    if (roundFinished_ && !sg_raceOutputSent)
    {
        gRaceScores::OutputEnd();
        sg_raceOutputSent = true;

        gRaceScores::Write();
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

    for(int i=0; i<MAXCLIENTS; i++) cannotFinish_[i] = false;

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
            rPlayer->SetSafe(true);
            rPlayer->SetCanFinish(true);

            rPlayer->SetIdle(false);
            rPlayer->SetIdleCounter(0);
            rPlayer->SetIdleNextTime(0);

            rPlayer->SetLaps(0);
            rPlayer->SetNextCheckpoint(-1);
            rPlayer->SetCheckpointsDone(false);
            rPlayer->checkpointsDoneList.clear();
            rPlayer->SetCountdown(-1);
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

void gRace::DeclareWinner()
{
    //  no need to declare once its done
    if (winnerDeclared_) return;

    //  declaring the first to finish
    if ((sg_RaceFinished.Len() > 0) && firstArrived_)
    {
        gRacePlayer *racePlayer = sg_RaceFinished[0];
        eTeam *team = racePlayer->Player()->CurrentTeam();
        if (racePlayer && team)
        {
            winnerDeclared_ = true;
            sg_DeclareWinner( team, tOutput("$player_win_race", team->GetColoredName(), sg_scoreRaceComplete) );
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

                //  message about displaying the allowed number of finished players
                arrive.SetTemplateParameter(1, max);
                arrive << "$race_finished_list_arrive";
                sn_ConsoleOut(arrive, player->Owner());

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
