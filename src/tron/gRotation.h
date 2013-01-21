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
#include "tConsole.h"

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

class gRotation
{
public:
    gRotation()
    {
        items_.SetLen(0);
        current_ = 0;
    }

    // the number of items
    int Size()
    {
        return items_.Len();
    }

    // returns the current value
    tString Current()
    {
        //tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

        return items_[current_];
    }

    // rotates
    void OrderedRotate()
    {
        if ( ++current_ >= items_.Len() )
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
            int random_ = randamizer.Get(items_.Len());
            if ((random_ < items_.Len()) || (random_ >= 0))
            {
                current_ = random_;
                goodnumber = true;
            }
        }
    }

    void Reset()
    {
        current_ = 0;
    }

    void Clear()
    {
        if (items_.Len() > 0)
        {
            for(int i = 0; i < items_.Len(); i++)
            {
                items_.RemoveAt(i);
                i--;
            }
        }

        items_.RemoveAt(0);
        items_.SetLen(0);
        current_ = 0;
    }

    tString Get(int itemID) const
    {
        return items_[itemID];
    }

    void Add(tString map_name)
    {
        if (map_name.Filter() != "")
            items_.Insert(map_name);
    }

private:

    tArray<tString> items_; // the various values the rotating config can take
    int current_;           // the index of the current

public:
    static void HandleNewRound();
    static void HandleNewMatch();
};

#endif
