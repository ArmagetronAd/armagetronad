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

class rSysDep
{
public:
    enum rSwapOptimize
    {
        rSwap_Latency = 0,
        rSwap_Auto = 1,
        rSwap_Throughput = 2,
        rSwap_ThroughputFlush = 3,
        rSwap_ThroughputFastest = 4
    };

#ifndef DEDICATED
    // buffer swap:
    static void SwapGL();
    static void ClearGL(); // not really system depentent.......

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
};

#endif
