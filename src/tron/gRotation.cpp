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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "gRotation.h"
#include "gGame.h"
#include "tConfiguration.h"
#include "tDirectories.h"
#include "tRecorder.h"

#include "eGrid.h"

int gRotation::counter_ = 1;

void gRotation::HandleNewRound() {
#ifdef HAVE_LIBRUBY
    gRoundEventRuby::DoRoundEvents();
#endif
}
void gRotation::HandleNewMatch() {
#ifdef HAVE_LIBRUBY
    gMatchEventRuby::DoMatchEvents();
#endif
}

#ifdef HAVE_LIBRUBY

static tCallbackRuby *roundEventRuby_anchor;
gRoundEventRuby::gRoundEventRuby()
        :tCallbackRuby(roundEventRuby_anchor)
{
}

void gRoundEventRuby::DoRoundEvents()
{
    Exec(roundEventRuby_anchor);
}

static tCallbackRuby *matchEventRuby_anchor;

gMatchEventRuby::gMatchEventRuby()
        :tCallbackRuby(matchEventRuby_anchor)
{
}

void gMatchEventRuby::DoMatchEvents()
{
    Exec(matchEventRuby_anchor);
}
#endif

tList<gQueuePlayers> gQueuePlayers::queuePlayers;

int sg_queueLimit = 20;
static tSettingItem<int> sg_queueLimitConf("QUEUE_LIMIT", sg_queueLimit);

int sg_queueLimitExcempt = 2;
static tSettingItem<int> sg_queueLimitExcemptConf("QUEUE_LIMIT_EXCEMPT", sg_queueLimitExcempt);

bool sg_queueLimitEnabled = false;
static tSettingItem<bool> sg_queueLimitEnabledConf("QUEUE_LIMIT_ENABLED", sg_queueLimitEnabled);

int sg_queueIncrement = 0;
static tSettingItem<int> sg_queueIncrementConf("QUEUE_INCREMENT", sg_queueIncrement);

REAL sg_queueRefillTime = 1;
bool restrictRefillTime(const REAL &newValue)
{
    if (newValue <= 0) return false;
    return true;
}
static tSettingItem<REAL> sg_queueRefillTimeConf("QUEUE_REFILL_TIME", sg_queueRefillTime, &restrictRefillTime);

bool sg_queueRefillActive = true;
static tSettingItem<bool> sg_queueRefillActiveConf("QUEUE_REFILL_ACTIVE", sg_queueRefillActive);

static int sg_queueMax = 30;
static tSettingItem<int> sg_queueMaxConf("QUEUE_MAX", sg_queueMax);

static void sg_queueRefill(std::istream &s)
{
    tString player;
    tOutput msg;

    s >> player;

#ifdef DEBUG
    con << "Queuer Name:" << player << "\n";
#endif

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        for(int i = 0; i < gQueuePlayers::queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = gQueuePlayers::queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name().Contains(player))
                {
                    qPlayer->SetQueue(qPlayer->QueueDefault());

                    msg.SetTemplateParameter(1, qPlayer->Name());
                    msg << "$queue_refill_success";
                    sn_ConsoleOut(msg);

                    return;
                }
            }
        }
    }

    msg.SetTemplateParameter(1, player);
    msg << "$queue_find_failed";
    sn_ConsoleOut(msg, 0);
}
static tConfItemFunc sg_queueRefillConf("QUEUE_REFILL", &sg_queueRefill);

static void sg_queueGive(std::istream &s)
{
    tString player, queueStr;
    tOutput msg;

    s >> player;
    s >> queueStr;

#ifdef DEBUG
    con << "Queuer Name:" << player << "\n";
#endif

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        for(int i = 0; i < gQueuePlayers::queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = gQueuePlayers::queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name().Contains(player))
                {
                    int new_queues = atoi(queueStr);
                    if (new_queues > 0)
                    {
                        //  send message
                        msg.SetTemplateParameter(1, qPlayer->Name());
                        msg.SetTemplateParameter(2, qPlayer->Queues());
                        msg.SetTemplateParameter(3, new_queues);
                        msg << "$queue_give_success";
                        sn_ConsoleOut(msg);

                        //  apply
                        qPlayer->SetQueue(new_queues);
                    }

                    return;
                }
            }
        }
    }

    msg.SetTemplateParameter(1, player);
    msg << "$queue_find_failed";
    sn_ConsoleOut(msg, 0);
}
static tConfItemFunc sg_queueGiveConf("QUEUE_GIVE", &sg_queueGive);

gQueuePlayers::gQueuePlayers(ePlayerNetID *player)
{
    this->name_ = player->GetUserName();
    this->owner_ = NULL;

    this->played_ = 0;
    this->refill_ = -1;

    this->queuesDefault = sg_queueLimit;
    this->queues_ = this->queuesDefault;

    this->lastTime_ = se_GameTime();

    queuePlayers.Insert(this);
}

gQueuePlayers::gQueuePlayers(tString name)
{
    this->name_ = name;
    this->owner_ = NULL;

    this->played_ = 0;
    this->refill_ = 0;

    this->queuesDefault = sg_queueLimit;
    this->queues_ = this->queuesDefault;

    this->lastTime_ = se_GameTime();

    queuePlayers.Insert(this);
}

bool gQueuePlayers::PlayerExists(ePlayerNetID *player)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == player->GetUserName())
                        return true;
            }
        }
    }
    return false;
}

bool gQueuePlayers::PlayerExists(tString name)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == name)
                        return true;
            }
        }
    }
    return false;
}

gQueuePlayers *gQueuePlayers::GetData(ePlayerNetID *player)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == player->GetUserName())
                        return qPlayer;
            }
        }
    }
    return NULL;
}

gQueuePlayers *gQueuePlayers::GetData(tString name)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == name)
                        return qPlayer;
            }
        }
    }
    return NULL;
}

void gQueuePlayers::RemovePlayer()
{
    owner_ = NULL;
}

bool gQueuePlayers::Timestep(REAL time)
{
    if ((queuePlayers.Len() > 0) && sg_queueLimitEnabled)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Player() && (qPlayer->Name() == ""))
                    qPlayer->name_ = qPlayer->Player()->GetUserName();
            }
        }

        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                // account for play time
                REAL tick = time - qPlayer->lastTime_;

                if ((qPlayer->Player() && sg_queueRefillActive) || !sg_queueRefillActive)
                {
                    //qPlayer->played_ += tick;
                    qPlayer->played_ += 0.35f;

                    if (qPlayer->played_ >= qPlayer->refill_)
                    {
                        //  if queue increment is enabled
                        if (sg_queueIncrement > 0)
                        {
                            //  if queue max is below 0
                            if (sg_queueMax <= 0)
                            {
                                qPlayer->queuesDefault += sg_queueIncrement;
                            }
                            //  if queue max is above 0
                            else
                            {
                                if (qPlayer->queuesDefault < sg_queueMax)
                                {
                                    //  increase their default queue limit by this amount
                                    qPlayer->queuesDefault += sg_queueIncrement;
                                }
                            }

                            //  create the new time for the next time to refill
                            qPlayer->refill_ = qPlayer->played_ + sg_queueRefillTime;
                        }

                        //  refill queues with their original amount
                        if (qPlayer->queues_ == 0)
                            qPlayer->queues_ = qPlayer->queuesDefault;
                    }
                }

                qPlayer->lastTime_ = time;
            }
        }
    }
}

void gQueuePlayers::Save()
{
    if ((queuePlayers.Len() > 0) && sg_queueLimitEnabled)
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, "queuers.txt"))
        {
            for(int i = 0; i < queuePlayers.Len(); i++)
            {
                gQueuePlayers *qPlayer = queuePlayers[i];
                if (qPlayer)
                {
                    o << qPlayer->Name() << " " << qPlayer->Queues() << " " << qPlayer->QueueDefault() << " " << qPlayer->PlayedTime()<< " " << qPlayer->RefillTime() << "\n";
                }
            }
        }
    }
}

void gQueuePlayers::Load()
{
    if (queuePlayers.Len() > 0)
        queuePlayers.Clear();

    std::ifstream r;
    if (tDirectories::Var().Open(r, "queuers.txt"))
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
                gQueuePlayers *qPlayer = NULL;

                tString name = params.ExtractNonBlankSubString(pos);
                if (!PlayerExists(name))
                {
                    qPlayer = new gQueuePlayers(name);
                }
                else
                {
                    qPlayer = GetData(name);
                }

                qPlayer->queues_ = atoi(params.ExtractNonBlankSubString(pos));
                qPlayer->queuesDefault = atoi(params.ExtractNonBlankSubString(pos));
                qPlayer->played_ = atof(params.ExtractNonBlankSubString(pos));
                qPlayer->refill_ = atof(params.ExtractNonBlankSubString(pos));
            }
        }
    }
    r.close();
}

void gQueuePlayers::Reset(REAL time)
{
    for (int i = 0; i < queuePlayers.Len(); i++)
    {
        gQueuePlayers *qPlayer = queuePlayers[i];
        if (qPlayer)
            qPlayer->lastTime_ = time;
    }

    Save();
}

bool gQueuePlayers::CanQueue(ePlayerNetID *p)
{
    if (sg_queueLimitEnabled)
    {
        //  allow access level of players from excempted to queue
        if (p->GetAccessLevel() <= sg_queueLimitExcempt ) return true;

        gQueuePlayers *qPlayer = NULL;
        if (!gQueuePlayers::PlayerExists(p))
            qPlayer = new gQueuePlayers(p);
        else
            qPlayer = gQueuePlayers::GetData(p);
        qPlayer->SetOwner(p);

        if (!qPlayer) return false;

        if (qPlayer->Queues() == 0)
        {
            tOutput warning;
            warning.SetTemplateParameter(1, (qPlayer->RefillTime() - (qPlayer->PlayedTime())));
            warning << "$queue_refill_waiting";
            sn_ConsoleOut(warning, p->Owner());

            return false;
        }

        if (qPlayer->Queues() > 0)
        {
            qPlayer->queues_ --;

            if (qPlayer->Queues() == 0)
            {
                REAL refillTime = sg_queueRefillTime;
                qPlayer->refill_ = qPlayer->PlayedTime() + refillTime;

                tOutput warning;
                warning.SetTemplateParameter(1, refillTime);
                warning << "$queue_limit_reached";
                sn_ConsoleOut(warning, p->Owner());
            }
        }
    }
    return true;
}

bool sg_QueueLog = false;
static tSettingItem<bool> sg_QueueLogConf("QUEUE_LOG", sg_QueueLog);

void sg_LogQueue(ePlayerNetID *p, tString command, tString params, tString item)
{
    if (!sg_QueueLog) return;

    std::ofstream o;
    if (tDirectories::Var().Open(o, "queuelog.txt", std::ios::app))
    {
        o << "[" << st_GetCurrentTime("%Y/%m/%d-%H:%M:%S") << "] " << p->GetName() << " | " << command << " " << params << " " << item << "\n";
    }
    o.close();
}

static void sg_queuersList(std::istream &s)
{
    int max = 10;
    int showing = 0;

    tString params;
    int pos = 0;
    params.ReadLine(s);

    int showAmount = 0;
    tString amount = params.ExtractNonBlankSubString(pos);
    if (amount.Filter() != "") showAmount = atoi(amount);

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        if (showAmount < gQueuePlayers::queuePlayers.Len())
        {
            if (gQueuePlayers::queuePlayers.Len() < max) max = gQueuePlayers::queuePlayers.Len();
            if ((gQueuePlayers::queuePlayers.Len() - showAmount) < max) max = gQueuePlayers::queuePlayers.Len() - showAmount;

            if (max > 0)
            {
                sn_ConsoleOut(tOutput("$queuers_list"), 0);

                for(int i = 0; i < max; i++)
                {
                    int rotID = showAmount + i;
                    gQueuePlayers *queuePlayer = gQueuePlayers::queuePlayers[rotID];
                    if (queuePlayer)
                    {
                        tColoredString send;
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << "( ";
                        send << "0x55ffff" << queuePlayer->Name();
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0xff55ff" << queuePlayer->Queues() << " qs";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0x88ff22" << queuePlayer->PlayedTime() << " s";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0x88ff22" << queuePlayer->RefillTime() << " s";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " )\n";
                        sn_ConsoleOut(send, 0);

                        showing++;
                    }
                }

                tOutput show;
                show.SetTemplateParameter(1, showing);
                show.SetTemplateParameter(2, gQueuePlayers::queuePlayers.Len());
                show << "$queuers_list_show";
                sn_ConsoleOut(show, 0);
            }
        }
    }
}
static tConfItemFunc sg_queuersListConf("QUEUERS_LIST", &sg_queuersList);
static tAccessLevelSetter sg_queuersListConfLevel( sg_queuersListConf, tAccessLevel_Moderator );
