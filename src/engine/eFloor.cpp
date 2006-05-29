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

#include "eFloor.h"
#include "rScreen.h"
#include "rTexture.h"
#include <stdlib.h>

eFloor::eFloor(){Floor = this;}

eFloor::~eFloor(){
    if (Floor == this)
        Floor = NULL;
}

REAL se_GridSize(){
    if (eFloor::Floor)
        return eFloor::Floor->GridSize();
    else
        return 4;
}

void se_glFloorColor(REAL alpha, REAL intens){
    if (eFloor::Floor)
        eFloor::Floor->glFloorColor(alpha, intens);
}

void se_glFloorTexture(){
    if (eFloor::Floor)
        eFloor::Floor->glFloorTexture();
}

void se_glFloorTexture_a(){
    if (eFloor::Floor)
        eFloor::Floor->glFloorTexture_a();
}

void se_glFloorTexture_b(){
    if (eFloor::Floor)
        eFloor::Floor->glFloorTexture_b();
}

bool se_BlackSky()
{
    if (eFloor::Floor)
        return eFloor::Floor->BlackSky();
    else
        return false;
}

void se_FloorColor(REAL& r, REAL& g, REAL &b)
{
    if (eFloor::Floor && sr_floorDetail > 1)
        eFloor::Floor->FloorColor(r,g,b);
    else
    {
        r = g = b = 0;
    }
}

void se_MakeColorValid(REAL& r, REAL& g, REAL& b, REAL f)
{
    REAL R, G, B; // the floor color
    se_FloorColor(R, G, B);

    /*
    	REAL wallInt = 2.0f;
    	if ( TextureMode[rTEX_WALL] <= 0 )
    	{
    		wallInt = 1.0f;
    	}

    	R *= wallInt;
    	G *= wallInt;
    	B *= wallInt;
    */  

    // if we are too close to the floor color, just change to white.
    while ((r < .95 && g < .95 && b < .95) &&
            (
                (fabs(R    - r*f) + fabs(G    - g*f) + fabs(B    - b*f) < .5 )  ||
                //			   (fabs(R*.5 - r*f) + fabs(G*.5 - g*f) + fabs(B*.5 - b*f) < .5) ||
                (fabs(r*f) + fabs(g*f) + fabs(b*f) < .5 )
            )
          )

    {
        REAL step = 1/(15.0 * 3);
        REAL step_r, step_g, step_b;
        REAL rgb_sum = 0;
        if ( r < .99 )
            rgb_sum += r;
        if ( g < .99 )
            rgb_sum += g;
        if ( b < .99 )
            rgb_sum += b;

        if ( rgb_sum < .02 )
        {
            step_r = step;
            step_g = step;
            step_b = step;
        }
        else
        {
            step_r = step * r / rgb_sum;
            step_g = step * g / rgb_sum;
            step_b = step * b / rgb_sum;
        }

        r += step_r;
        g += step_g;
        b += step_b;
        if (r > 1)
            r = 1;
        if (g > 1)
            g = 1;
        if (b > 1)
            b = 1;
    }
}



eFloor *eFloor::Floor;

