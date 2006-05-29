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

#include "rRender.h"

rRenderer *renderer;

void sr_RendererCleanup(){
    delete renderer;
    renderer = 0;
}

rRenderer::rRenderer(){
    renderer = this;

    // Clear the flag stack

    int i;

    stackpos = 0;

    for (i = STACK_DEPTH-1; i>=0; i--)
        flagstack[i] = 0;
}

rRenderer::~rRenderer(){
    if (renderer == this)
        renderer = 0;
}

void rRenderer::TexVertex(REAL x, REAL y, REAL z,
                          REAL u, REAL v){
    TexCoord(u,v);
    Vertex(x,y,z);
}

void rRenderer::Line(REAL x1, REAL y1, REAL z1,
                     REAL x2, REAL y2, REAL z2){
    BeginLines();
    Vertex(x1,y1,z1);
    Vertex(x2,y2,z2);
}

void rRenderer::SetFlag(flag f, bool c)
{
    ReallySetFlag(f, c);
}


void rRenderer::ChangeFlags(int before, int after) const{

}



