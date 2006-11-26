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

#include "gCycle.h"
#include "gSensor.h"
#include "gWall.h"
#include "eDebugLine.h"

extern REAL sg_delayCycle;

void gSensor::PassEdge(const eWall *ww,REAL time,REAL a,int r){
    if (!ww)
        return;

    try{
        eSensor::PassEdge(ww,time,a,r);
    }
    catch( eSensorFinished & e )
    {
        const gPlayerWall *w=dynamic_cast<const gPlayerWall*>(ww);
        if (w)
        {
            gCycle *owner=w->Cycle();
            if (owner && owner->IsMe( owned ) )
            {
                type=gSENSOR_SELF;
            }
            else if ( owner && owned && owner->Team() == owned->Team() )
            {
                type=gSENSOR_TEAMMATE;
            }
            else
            {
                type=gSENSOR_ENEMY;
            }

            if (w->EndTime() < w->BegTime())
                lr=-lr;
        }
        else if (dynamic_cast<const gWallRim*>(ww))
            type=gSENSOR_RIM;

        throw;
    }
}

