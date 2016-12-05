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

#ifndef ArmageTron_gSENSOR_H
#define ArmageTron_gSENSOR_H

#include "eSensor.h"

class eEdge;

typedef enum{gSENSOR_NONE,gSENSOR_RIM, gSENSOR_ENEMY,
             gSENSOR_TEAMMATE ,gSENSOR_SELF} gSensorWallType;

// sensor sent out to detect near eWalls
class gSensor: public eSensor{
public:
    gSensorWallType type;

    gSensor(eGameObject *o,const eCoord &start,const eCoord &d)
            :eSensor(o,start,d), type(gSENSOR_NONE){}

    virtual ePassEdgeResult PassEdge(const eWall *w,REAL time,REAL,int =1) override;
};
#endif
