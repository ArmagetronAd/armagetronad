/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by Manuel Moos
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_gJoystick_H
#define ArmageTron_gJoystick_H

class gCycleMovement;
class uActionPlayer;

#include "tCoord.h"

//! special joystick controls for the lightcycles. Simple left-right controls
// just don't cut it there.

class gJoystick
{
    friend class gCycle;
public:
    gJoystick( gCycleMovement * cycle )
    : cycle_( cycle ), lastCommand_( 0 ), lastTurn_(0), glance_( false ), turnRequested_( false )
    {
    }

    //! process input events
    bool Act( uActionPlayer * act, REAL value );

    //! turn the cycle
    void Turn();
private:
    bool ActInternal( uActionPlayer * act, REAL value );

    gCycleMovement * cycle_; //!< the cycle controlled by this joystick
    tCoord cameraDirection_; //!< the direction the camera is supposed to look in
    tCoord driveDirection_;  //!< the direction the cycle is supposed to drive in
    tCoord joyDirection_;    //!< direction the joystick is currently pointing in

    double lastCommand_;     //!< last time the joystick was clearly pushed into some direction
    int lastTurn_;           //!< last ordered turn direction

    bool glance_;            //!< whether the glance button is pressed
    bool turnRequested_;     //!< internal flag indicating further turns may be required
};

#endif
