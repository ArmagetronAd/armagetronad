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
#include "eTimer.h"
#include "tConfiguration.h"
#include "tDirectories.h"
#include "tRecorder.h"

#include "eGrid.h"

int gRotation::counter_ = 0;

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

int sg_queueLimit = 5;
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

gQueuePlayers::gQueuePlayers(ePlayerNetID *player)
{
    this->name_ = player->GetUserName();
    this->owner_ = player;

    this->played_ = 0;
    this->refill_ = -1;

    this->queues_ = sg_queueLimit;

    queuePlayers.Insert(this);
}

gQueuePlayers::gQueuePlayers(tString name)
{
    this->name_ = name;
    this->owner_ = NULL;

    this->played_ = 0;
    this->refill_ = 0;

    this->queues_ = sg_queueLimit;
    this->queuesDefault = sg_queueLimit;

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
                if (qPlayer->Player())
                {
                    if (qPlayer->Player() == player)
                        return true;
                }
                else
                {
                    if (qPlayer->Name() == player->GetUserName())
                        return true;
                }
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
                if (qPlayer->Player())
                {
                    if (qPlayer->Player() == player)
                        return qPlayer;
                }
                else
                {
                    if (qPlayer->Name() == player->GetUserName())
                        return qPlayer;
                }
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
    if ((queuePlayers.Len() > 0) &&(sg_queueLimitEnabled))
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                // account for play time
                REAL tick = time - qPlayer->lastTime_;

                if (qPlayer->Player() || !sg_queueRefillActive)
                {
                    qPlayer->played_ += tick;

                    if ((qPlayer->played_ / 60) >= qPlayer->refill_)
                    {
                        //  if queue increment is enabled
                        if (sg_queueIncrement > 0)
                        {
                            //  increase their default queue limit by this amount
                            qPlayer->queuesDefault += sg_queueIncrement;
                        }

                        //  refill queues with their original amount
                        qPlayer->queues_ = qPlayer->queuesDefault;

                        //  create the new time for the next time to refill
                        qPlayer->refill_ = (qPlayer->played_ / 60) + (60 * sg_queueRefillTime);
                    }
                }

                qPlayer->lastTime_ = time;
            }
        }

        //  save data regularly
        Save();
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

void gQueuePlayers::Reset()
{
    for (int i = 0; i < queuePlayers.Len(); i++)
    {
        gQueuePlayers *qPlayer = queuePlayers[i];
        if (qPlayer)
            qPlayer->lastTime_ = -1;
    }
}

bool gQueuePlayers::CanQueue(ePlayerNetID *p)
{
    if (sg_queueLimitEnabled)
    {
        gQueuePlayers *qPlayer = gQueuePlayers::GetData(p);
        if (!qPlayer) return false;

        if (p->GetAccessLevel() <= sg_queueLimitExcempt ) return true;

        if (qPlayer->Queues() == 0)
        {
            tOutput warning;
            warning.SetTemplateParameter(1, (qPlayer->RefillTime() - (qPlayer->PlayedTime() / 60)));
            warning << "$queue_refill_waiting";
            sn_ConsoleOut(warning, p->Owner());

            return false;
        }

        if (qPlayer->Queues() > 0)
        {
            qPlayer->SetQueue(qPlayer->Queues() - 1);

            if (qPlayer->Queues() == 0)
            {
                REAL refillTime = 60 * sg_queueRefillTime;
                qPlayer->SetRefillTime((qPlayer->PlayedTime() / 60) + refillTime);

                tOutput warning;
                warning.SetTemplateParameter(1, refillTime);
                warning << "$queue_limit_reached";
                sn_ConsoleOut(warning, p->Owner());
            }
        }
    }
    return true;
}
