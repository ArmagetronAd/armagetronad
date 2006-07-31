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

#ifndef ArmageTron_EVENT_QUEUE_H
#define ArmageTron_EVENT_QUEUE_H

#include "tList.h"
#include "tHeap.h"

/*
  we try to establish a future event management based on the following
  assumption that every possible event can guarantee that it won't happen
  for the next x seconds; for example, if you are in the center of
  an arena, your max speed is 5 mps and the next wall is 30 m away,
  you know you won't hit a wall for the next 6 seconds.

  To efficiently manage many such possible events, we store them
  in a heap-type data structure, where the most urgent events
  reside at the bottom (why is everything upside down in informatics?)
  of the heap.

  Note: time may be replaced by other monotonely increasing functions,
  like fuel usage,...
*/

// #define EVENT_DEB

// the events. WARNING: tEvents may be deleted by tEventQueue,
// so make sure that this is allways possible.


class tEventQueue;

class tEvent:public tHeapElement{
    friend class tEventQueue;
    virtual ~tEvent();
public:
    tEvent(){}

    virtual bool Check(REAL time)=0;
    // check the tEvent and update value. (the time we have to check it again)
    // return value: TRUE if the tEvent needs to be checked again
    //               FALSE if the tEvent happened and can be deleted.

    //  tEventQueue *Queue();
    // in wich queue are we?

    virtual void Render(){}
};


class tEventQueue:public tHeap<tEvent>{
    REAL currentTime;      // the current time

public:
    tEventQueue():currentTime(0){}
    ~tEventQueue();

    void Timestep(REAL time); // processes all the tEvents that
    // may happen until time.

};




#endif

