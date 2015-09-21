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
                         /*
                                 rSwap_150Hz = 5,
                                 rSwap_100Hz = 7,
                                 rSwap_80Hz  = 9,
                                 rSwap_60Hz  = 11
                         */
    };

#ifndef DEDICATED
    // graphics initialisation and cleanup:
    static bool InitGL();
    static void ExitGL();

    // buffer swap:
    static void SwapGL();
    static void ClearGL(); // not really system depentent.......
#endif

    static rSwapMode swapMode_;
};

#endif
