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

#include "gJoystick.h"
#include "gCycleMovement.h"
#include "uInput.h"
#include "eGrid.h"
#include "tSysTime.h"

static uActionPlayer sg_JoyLeft("JOY_LEFT", -4);
static uActionPlayer sg_JoyRight("JOY_RIGHT", -4);
static uActionPlayer sg_JoyUp("JOY_UP", -3);
static uActionPlayer sg_JoyDown("JOY_DOWN", -3);
static uActionPlayer sg_JoyGlance("JOY_GLANCE", -2);

//! process input events
bool gJoystick::Act( uActionPlayer * act, REAL value )
{
    if ( joyDirection_.NormSquared() > .5 )
    {
        lastCommand_ = tSysTimeFloat();
    }

    if ( ActInternal( act, value ) )
    {
        // update directions
        if ( joyDirection_.NormSquared() > .5 )
        {
            // fetch driving direction if required
            if ( driveDirection_.NormSquared() < .1 )
            {
                // con << joyDirection_ << "\n";

                driveDirection_ = cycle_->Direction();
                driveDirection_.Normalize();
            }

            // fetch camera direction if required
            if ( cameraDirection_.NormSquared() < .1 )
            {
                cameraDirection_ = cycle_->CamDir();
                cameraDirection_.Normalize();
            }

            tCoord dir = joyDirection_;
            dir.Normalize();
            if ( glance_ )
            {
                // adapt viewing direction to driving direction
                cameraDirection_ = driveDirection_.Turn( dir.Conj() );
            }
            else
            {
                // adapt driving direction to view direction
                driveDirection_ = cameraDirection_.Turn( dir );
            }

            // possibly turn
            turnRequested_ = true;
            // Turn();
        }

        return true;
    }

    return false;
}

//! turn the cycle
void gJoystick::Turn()
{
    if ( joyDirection_.NormSquared() < .1 && lastCommand_ < tSysTimeFloat() - .03 )
    {
        // if ( driveDirection_.NormSquared() > .5 )
        // con << joyDirection_ << "\n";

        // joystick was released, reeset driving direction and camera direction.
        driveDirection_ = tCoord();
        cameraDirection_ = tCoord();
        return;
    }

    // nothing to do
    if ( !turnRequested_ )
    {
        return;
    }

    // fetch current and possible turn driving directions
    int winding = cycle_->WindingNumber();
    int windingLeft = winding, windingRight = winding;
    eGrid * grid = cycle_->Grid();
    grid->Turn( windingLeft, -1 );
    grid->Turn( windingRight, +1 );
    eCoord leftTurn = grid->GetDirection( windingLeft );
    eCoord rightTurn = grid->GetDirection( windingRight );
    eCoord straightOn = grid->GetDirection( winding );

    // normalize them (axes may be unnormalized)
    leftTurn.Normalize();
    rightTurn.Normalize();
    straightOn.Normalize();

    // calculate distances between desired and possible driving directions
    REAL left     = ( leftTurn - driveDirection_ ).NormSquared();
    REAL right    = ( rightTurn - driveDirection_ ).NormSquared();
    REAL straight = ( straightOn - driveDirection_ ).NormSquared();

    // possibly turn
    if ( left < right && left < straight * ( lastTurn_ != 1 ? 2 : .8 ) )
    {
        if ( cycle_->CanMakeTurn( -1 ) )
        {
            cycle_->Turn( -1 );
            lastTurn_ = -1;
        }
    }
    else if ( left > right && right < straight * ( lastTurn_ != -1 ? 2 : .8 )  )
    {
        if( cycle_->CanMakeTurn( +1 ) )
        {
            cycle_->Turn( +1 );
            lastTurn_ = +1;
        }
    }
    else
    {
        // not much use repeating this calculation until new input arrives
        turnRequested_ = false;
        lastTurn_ = 0;
    }
}

bool gJoystick::ActInternal( uActionPlayer * act, REAL value )
{
    if ( act == & sg_JoyUp )
    {
        joyDirection_.x = value;
        return true;
    }
    if ( act == & sg_JoyDown )
    {
        joyDirection_.x = -value;
        return true;
    }
    if ( act == & sg_JoyLeft )
    {
        joyDirection_.y = value;
        return true;
    }
    if ( act == & sg_JoyRight )
    {
        joyDirection_.y = -value;
        return true;
    }
    if ( act == & sg_JoyGlance )
    {
        glance_ = ( value > .5 );
        return true;
    }

    return false;
}
