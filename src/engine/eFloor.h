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

#ifndef ArmageTron_floor_H
#define ArmageTron_floor_H

#include "defs.h"

class eFloor{
public:
    static eFloor *Floor;

    eFloor();
    virtual ~eFloor();
    virtual void glFloorColor(REAL alpha, REAL intens)=0;
    virtual void FloorColor(REAL& r, REAL& g, REAL &b)=0;
    virtual void glFloorTexture()=0;
    virtual void glFloorTexture_a()=0;
    virtual void glFloorTexture_b()=0;
    virtual REAL GridSize()=0;
    virtual bool BlackSky()=0;
};

void se_glFloorColor(REAL alpha = 1, REAL intens = 1);
void se_glFloorTexture();
void se_glFloorTexture_a();
void se_glFloorTexture_b();

REAL se_GridSize();
bool se_BlackSky();
void se_FloorColor(REAL& r, REAL& g, REAL &b);

void se_MakeColorValid(REAL& r, REAL & g, REAL& b, REAL f);
#endif
