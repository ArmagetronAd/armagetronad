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

#ifndef ARMAGETRON_G_ROTATION_H
#define ARMAGETRON_G_ROTATION_H

#include "tCallback.h"
#include "tLinkedList.h"
#include "tString.h"
#include "tRandom.h"
#include "ePlayer.h"
#include "eTimer.h"

#ifdef HAVE_LIBRUBY
class gRoundEventRuby : public tCallbackRuby {
public:
    gRoundEventRuby();
    static void DoRoundEvents();
};

class gMatchEventRuby : public tCallbackRuby {
public:
    gMatchEventRuby();
    static void DoMatchEvents();
};
#endif // HAVE_LIBRUBY

enum gRotationType
{
    gROTATION_NEVER = 0,
    gROTATION_ORDERED_ROUND = 1,
    gROTATION_ORDERED_MATCH = 2,
    gROTATION_RANDOM_ROUND = 3,
    gROTATION_RANDOM_MATCH = 4,
    gROTATION_COUNTER = 5,
    gROTATION_ROUND = 6
};
tCONFIG_ENUM( gRotationType );

class gRotationItem
{
    public:
        gRotationItem(tString name, int round = 0)
        {
            this->name_ = name;
            this->round_ = round;
        }

        tString Name() { return name_; }
        int Round()    { return round_; }

        void SetName(tString name) { name_ = name; }
        void SetRound(int round)   { round_ = round; }

    private:
        tString name_;
        int round_;
};

class gRotationRoundSelection
{
    public:
        gRotationRoundSelection(int round)
        {
            this->round_   = round;
            this->current_ = 0;
        }

        int Round() { return round_; }

        int Size() { return items_.size(); }

        // returns the current value
        gRotationItem *Current()
        {
            //tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );
            return items_[current_];
        }

        void Reset() { current_ = 0; }

        void Clear()
        {
            items_.clear();
            current_ = 0;
        }

        gRotationItem *Get(int itemID) const
        {
            if ((itemID >= 0) && (itemID < static_cast<signed>(items_.size())))
                return items_[itemID];
            else
                return NULL;
        }

        void Add(gRotationItem *resourceItem)
        {
            if (resourceItem->Name().Filter() != "")
            {
                items_.push_back(resourceItem);
            }
        }

        void Remove(gRotationItem *resourceItem)
        {
            std::deque<gRotationItem *>::iterator res = items_.begin();
            for(; res != items_.end(); res++)
            {
                gRotationItem *rotItem = *res;
                if (rotItem && (rotItem == resourceItem))
                {
                    items_.erase(res);
                    break;
                }
            }
        }

        int ID() { return current_;}

        //!< This is for the rotation loading limit
        void Rotate()
        {
            if ( ++current_ >= static_cast<signed>(items_.size()) )
            {
                current_ = 0;
            }
        }

    private:
        std::deque<gRotationItem *> items_; // the various values the rotating config can take
        int current_;                // the index of the current
        int round_;                  // round of which items should be loaded in
};

class gRotationRound
{
    public:
        gRotationRound() {}

        std::deque<gRotationRoundSelection *> roundsList_;

        void Add(gRotationRoundSelection *roundSelection)
        {
            roundsList_.push_back(roundSelection);
        }

        void Clear()
        {
            roundsList_.clear();
        }

        //!< Global functions for checking
        bool Check(int round)
        {
            std::deque<gRotationRoundSelection *>::iterator res = roundsList_.begin();
            for(; res != roundsList_.end(); res++)
            {
                gRotationRoundSelection *roundSel = *res;
                if (roundSel && (roundSel->Round() == round))
                    return true;
            }
            return false;
        }

        gRotationRoundSelection *Get(int round)
        {
            std::deque<gRotationRoundSelection *>::iterator res = roundsList_.begin();
            for(; res != roundsList_.end(); res++)
            {
                gRotationRoundSelection *roundSel = *res;
                if (roundSel && (roundSel->Round() == round))
                    return roundSel;
            }
            return NULL;
        }
};

class gRotation
{
public:
    gRotation()
    {
        current_ = 0;
    }

    // the number of items
    int Size() { return static_cast<signed>(items_.size()); }

    // returns the current value
    gRotationItem *Current()
    {
        //tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

        return items_[current_];
    }

    // rotates
    void OrderedRotate()
    {
        if ( ++current_ >= static_cast<signed>(items_.size()) )
        {
            current_ = 0;
        }
    }

    void RandomRotate()
    {
        tRandomizer & randamizer = tRandomizer::GetInstance();
        bool goodnumber = false;
        while (goodnumber == false)
        {
            int random_ = randamizer.Get(static_cast<signed>(items_.size()));
            if ((random_ < static_cast<signed>(items_.size())) || (random_ >= 0))
            {
                current_ = random_;
                goodnumber = true;
            }
        }
    }

    void Reset() { current_ = 0; }

    void Clear()
    {
        items_.clear();
        current_ = 0;
    }

    gRotationItem *Get(int itemID) const
    {
        if ((itemID >= 0) && (itemID < static_cast<signed>(items_.size())))
            return items_[itemID];
        else
            return NULL;
    }

    void Add(gRotationItem *resourceItem)
    {
        items_.push_back(resourceItem);
    }

    void Remove(gRotationItem *resourceItem)
    {
        std::deque<gRotationItem *>::iterator res = items_.begin();
        for(; res != items_.end(); res++)
        {
            gRotationItem *rotItem = *res;
            if (rotItem && (rotItem == resourceItem))
            {
                items_.erase(res);
                break;
            }
        }
    }

    int ID() { return current_;}

    void SetID(int newID) { current_ = newID; }

    //!< This is for the rotation loading limit
    static void AddCounter() { counter_ ++; }
    static void ResetCounter() { counter_ = 1; }
    static int Counter() { return counter_; }

private:

    std::deque<gRotationItem *> items_; // the various values the rotating config can take
    int current_;           // the index of the current

    static int counter_;
    static bool queueActive_;

public:
    static void HandleNewRound(int rounds);
    static void HandleNewMatch();
};

class gQueueing
{
    public:
        // the number of items
        int Size() const { return items_.Len(); }

        void Remove(int itemID) { items_.RemoveAt(itemID); }

        void Reset() { current_ = 0; }

        // returns the current value
        tString Current() const
        {
            tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

            return items_[current_];
        }

        // returns the current id
        int CurrentID() const { return current_; }

        void Insert(tString item_name) { items_.Insert(item_name); }

        tString Get(int itemID) { return items_[itemID]; }

    private:
        tArray<tString> items_;
        int current_;
};

class gQueuePlayers
{
    public:
        gQueuePlayers(ePlayerNetID *player);
        gQueuePlayers(tString name);

        tString Name() { return name_; }
        ePlayerNetID *Player() { return owner_; }
        void SetOwner(ePlayerNetID *p) { owner_ = p; }

        void RemovePlayer();

        REAL PlayedTime() { return played_; }
        REAL RefillTime() { return refill_; }
        void SetPlayedTime(REAL newValue) { played_ = newValue; }
        void SetRefillTime(REAL newValue) { refill_ = newValue; }

        int Queues() { return queues_; }
        int QueueDefault() { return queuesDefault; }

        void SetQueue(int newValue) { queues_ = newValue; }
        void SetDefaultQueue(int newValue) { queuesDefault = newValue; }

        static bool PlayerExists(ePlayerNetID *player);
        static bool PlayerExists(tString name);
        static gQueuePlayers *GetData(ePlayerNetID *player);
        static gQueuePlayers *GetData(tString name);

        static void Timestep(REAL time);

        static void Save();
        static void Reset(REAL time = se_GameTime());
        static void Load();

        static bool CanQueue(ePlayerNetID *p);

        //!< Holds list of queue players
        static tList<gQueuePlayers> queuePlayers;

    private:
        tString name_;          //!< Name of the owner
        ePlayerNetID *owner_;   //!< Owner of this queue data

        REAL played_;   //!< Time player has played in server
        REAL refill_;   //!< Time player has to play to refill their queue

        int queues_;        //!< The amount they still have to use the queue feature
        int queuesDefault;  //!< The amount they origially had

        REAL lastTime_;
};

void QueueShowPlayer(ePlayerNetID *player);
void RotationShowPlayer(ePlayerNetID *player, std::istream &s);

void sg_DisplayRotationList(ePlayerNetID *p, std::istream &s, tString command);
void sg_AddqueueingItems(ePlayerNetID *p, std::istream &s, tString command);
void sg_LogQueue(ePlayerNetID *p, tString command, tString params, tString item);

extern gRotationType rotationtype;
extern int sg_rotationMax;

#endif
