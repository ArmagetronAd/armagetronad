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

#ifndef ArmageTron_SysTime_H
#define ArmageTron_SysTime_H

bool tTimerIsAccurate();                      //! returns true if a timer with more than millisecond accuracy is available
double tSysTimeFloat();                       //! returns the current frame's time ( from the playback )
double tRealSysTimeFloat();                   //! returns the current frame's time ( from the real system )
void tAdvanceFrame( int usecdelay = 0);       //! andvances one frame: updates the system time
void tDelay( int usecdelay );                 //! delays for the specified number of microseconds
void tDelayForce( int usecdelay );            //! delays for the specified number of microseconds, even when playing back

#endif
