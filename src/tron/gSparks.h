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

#ifndef ArmageTron_gSparks_H
#define ArmageTron_gSparks_H

#include "eSound.h"
#include "eGameObject.h"
#include "rModel.h"

class gSpark: public eGameObject{ // When the player nearly hits a eWall
    REAL createTime;

#define SPARKS 10

    Vec3 x         [SPARKS];
    Vec3 lastX     [SPARKS];
    Vec3 preLastX  [SPARKS];
    Vec3 xDot      [SPARKS];
    REAL heat      [SPARKS];
    REAL lastBreak [SPARKS];
    REAL sparkowncolor_r;
    REAL sparkowncolor_g;
    REAL sparkowncolor_b;
    REAL sparkenemycolor_r;
    REAL sparkenemycolor_g;
    REAL sparkenemycolor_b;

private:
public:
    gSpark(eGrid *grid, const eCoord &pos,const eCoord &dir,REAL time,REAL ocolor_r,REAL ocolor_g,REAL ocolor_b,REAL ecolor_r,REAL ecolor_g,REAL ecolor_b);
    virtual ~gSpark();

    virtual bool Timestep(REAL currentTime);

    virtual void InteractWith(eGameObject *target,REAL time,int recursion=1);

    virtual void PassEdge(const eWall *e,REAL time,REAL a,int recursion=1);

    virtual void Kill();

#ifndef DEDICATED
    virtual void Render(const eCamera *cam);

    virtual void SoundMix(Uint8 *dest,unsigned int len,
                          int viewer,REAL rvol,REAL lvol);
#endif
};

#endif
