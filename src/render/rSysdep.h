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
    enum rSwapMode
    {
        rSwap_Fastest = 0,
        rSwap_glFlush = 1,
        rSwap_glFinish = 2
    };

    enum rSwapOptimize
    {
        rSwap_Latency = 0,
        rSwap_Auto = 1,
        rSwap_Throughput = 2
    };

#ifndef DEDICATED
    // graphics initialisation and cleanup:
    static bool InitGL();
    static void ExitGL();

    // buffer swap:
    static void SwapGL();
    static void PostSwapGL(); // call after the game logic before the first new draw command of the next frame
    static void ClearGL(); // not really system depentent.......

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

    static const rSwapMode swapModeLatency_;
    static rSwapMode swapModeThroughput_;
    static rSwapOptimize swapOptimize_;
};

#endif
