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

#ifndef ArmageTron_SYSDEP_H
#define ArmageTron_SYSDEP_H

class tString;

//! None of the functions in this class are really system dependant any more,
//! but they were once. This could just go into rScreen.cpp nowadays, or be
//! renamed to rSwapControl, as buffer swaps and syncing properly and making
//! screenshots is what happens here.
class rSysDep
{
public:
    enum rSwapOptimize
    {
        rSwap_Latency = 0, // optimize for low latency
        rSwap_Auto = 1,    // switch between latency and throughput automatically
        rSwap_Throughput = 2, // optimize for high framerates
        rSwap_ThroughputFlush = 3, // optimize for high framerates using glFlush to sync
        rSwap_ThroughputFastest = 4 // don't sync OpenGL at all
    };

    // in latency mode, how careful should we be to avoid dropped frames?
    enum rFramedropTolerance
    {
        rSwap_Lenient = 0, // framedrops don't worry me too much
        rSwap_Normal = 1,  // allow some drops
        rSwap_Strict = 2,  // try hard to avoid them
        rSwap_Draconic = 3 // don't do anything that may cause additional drops
    };

#ifndef DEDICATED
    static void SwapGL();  //!< swaps back and front buffer
    static void ClearGL(); //!< clears the backbuffer

    static bool IsBenchmark(); //!< returns true if a benchmark is running

    static void IsInGame(); //!< call while game content is rendered

    // starting and stopping of background network processing
    class rNetIdler
    {
    public:
        virtual ~rNetIdler(){};
        // only during Do(), gamestate may be modified.
        virtual bool Wait() = 0; //!< wait for something to do, return true if there is work
        virtual void Do() = 0;   //!< do the work.
    };
    static void StartNetSyncThread( rNetIdler * idler );
    static void StopNetSyncThread();
#endif

    static rSwapOptimize swapOptimize_;
    static rFramedropTolerance framedropTolerance_;
};

extern tString sr_screenshotName;

#endif
