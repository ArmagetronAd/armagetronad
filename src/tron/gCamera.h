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

#ifndef ArmageTron_gCAMERA_H
#define ArmageTron_gCAMERA_H

#include "eCamera.h"

class gCamera:public eCamera{
    void MyInit();
public:
    eGameObject * lastCenter; // the gameobject we were last watching

    gCamera(eGrid *grid, rViewport *vp,ePlayerNetID *owner,ePlayer *lp,eCamMode m=CAMERA_IN);
    virtual ~gCamera();

    virtual eCoord CenterCycleDir();

    virtual REAL SpeedMultiplier() const;

    virtual void Timestep(REAL ts);
};

#endif
