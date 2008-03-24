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

#ifndef ArmageTron_MODEL_H
#define ArmageTron_MODEL_H

#include "defs.h"
#include "tArray.h"
#include "tLinkedList.h"
#include <math.h>
#include "rGL.h"
#include "rDisplayList.h"

class Vec3{
public:
    float x[3];
    Vec3(REAL a=0,REAL b=0,REAL c=0){x[0]=a;x[1]=b;x[2]=c;}
    ~Vec3(){};

    REAL Norm(){return REAL(sqrt(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]));}

    Vec3 operator*(REAL y){return Vec3(x[0]*y,x[1]*y,x[2]*y);}
    void operator+=(const Vec3 &y){x[0]+=y.x[0];x[1]+=y.x[1];x[2]+=y.x[2];}

    void RenderVertex();
    void RenderNormal();
};

class rModelFace{
public:
    int A[3];
    rModelFace(int a=0,int b=0,int c=0){A[0]=a;A[1]=b;A[2]=c;}
    ~rModelFace(){};
};

class rModel
{
    rDisplayList displayList_;

    tArray<Vec3> vertices;
    tArray<Vec3> texVert;
    tArray<Vec3> normals;
    tArray<rModelFace> modelFaces;
    tArray<rModelFace> modelTexFaces;
    bool modelTexFacesCoherent; // if modelFaces and modelTexFaces are identical
    void Load(std::istream &s,const char *fileName);
public:
    rModel(const char *fileName,const char *fileName_alt="");
    ~rModel();

    void Render();
};

#endif



