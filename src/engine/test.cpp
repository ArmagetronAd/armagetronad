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

#include "eGrid.h"
#include <iostream>

eHalfEdge *leak = NULL;

int main(){
    tStackObject< eGrid > grid;
    grid.Create();
    grid.Check();
    grid.Check();

#ifdef DEBUG
    grid.doCheck = false;
#endif
    for (int i=2;i>=0;i--)
    {
        std::cout << i << "\n";
        grid.SimplifyAll(10);

        ePoint *p =grid.Insert(eCoord(0,0));
        p = grid.DrawLine(p, eCoord(1000+2*i,i), NULL);

#ifdef DEBUG
        if (i == -1)
        {
            grid.doCheck = true;
            grid.Check();
        }
#endif


        p = grid.DrawLine(p, eCoord(10+2*i,10+i), NULL);
        p = grid.DrawLine(p, eCoord(-10+2*i,10+i), NULL);
        p = grid.DrawLine(p, eCoord(-10+2*i,-10+i), NULL);
        p = grid.DrawLine(p, eCoord(-1000+2*i,1000+i), NULL);
        p = grid.DrawLine(p, eCoord(10,500+i), NULL);
        p = grid.DrawLine(p, eCoord(10,0+i), NULL);
        p = grid.DrawLine(p, eCoord(10,700+i), NULL);
        p = grid.DrawLine(p, eCoord(10,10+i), NULL);
    }
    grid.Check();
    grid.Clear();
}
