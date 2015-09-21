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

#ifndef ArmageTron_RENDER_H
#define ArmageTron_RENDER_H

#include "defs.h"
#include "rGL.h"

#ifndef DONTDOIT
#define glBegin        #error glBegin disabled
#define glEnd          #error glEnd   disabled
#define glMatrixMode   #error glEnd   disabled
#endif

class rRenderer{
public:
    rRenderer();
    virtual ~rRenderer();

    virtual void Vertex(REAL x, REAL y)                 = 0;
    virtual void Vertex(REAL x, REAL y, REAL z)         = 0;
    virtual void Vertex3(REAL *x)                       = 0;
    virtual void Vertex(REAL x, REAL y, REAL z, REAL w) = 0;

    virtual void TexCoord(REAL u, REAL v)                 = 0;
    virtual void TexCoord(REAL u, REAL v, REAL w)         = 0;
    virtual void TexCoord(REAL u, REAL v, REAL w, REAL t) = 0;

    virtual void TexVertex(REAL x, REAL y, REAL z,
                           REAL u, REAL v);

    virtual void Color(REAL r, REAL g, REAL b)        = 0;
    virtual void Color(REAL r, REAL g, REAL b,REAL a) = 0;

    virtual void End(bool force=false)   = 0;

    virtual void BeginLines()      = 0;
    virtual void BeginTriangles()  = 0;
    virtual void BeginQuads()      = 0;

    virtual void BeginLineStrip()      = 0;
    virtual void BeginTriangleStrip()  = 0;
    virtual void BeginQuadStrip()      = 0;

    virtual void IsEdge(bool ie)  = 0;

    virtual void BeginTriangleFan()    = 0;
    virtual void BeginLineLoop()      = 0;

    virtual void Line(REAL x1, REAL y1, REAL z1,
                      REAL x2, REAL y2, REAL z2);


    virtual void ProjMatrix()     = 0;
    virtual void ModelMatrix()    = 0;
    virtual void TexMatrix()      = 0;

    virtual void PushMatrix()     = 0;
    virtual void PopMatrix()      = 0;
    virtual void MultMatrix(REAL mdata[4][4]) = 0;

    virtual void IdentityMatrix()                       = 0;
    virtual void ScaleMatrix(REAL f)                    = 0;
    virtual void ScaleMatrix(REAL f1, REAL f2, REAL f3) = 0;

    virtual void TranslateMatrix(REAL x1, REAL x2, REAL x3) = 0;


    typedef enum {BACKFACE_CULL=0, ALPHA_BLEND, ALPHA_TEST, DEPTH_TEST,
                  SMOOTH_SHADE, Z_OFFSET,
                  FLAG_END} flag;

    void PushFlags();
    void PopFlags();
    void SetFlag(flag f, bool c);

protected:
    virtual void ReallySetFlag(flag f, bool c) = 0;
    void         ChangeFlags(int before, int after) const;

#define STACK_DEPTH 100

    int flagstack[STACK_DEPTH];
    int stackpos;
};

extern rRenderer *renderer;

inline void Vertex(REAL x, REAL y){
    renderer->Vertex(x,y);
}

inline void Vertex(REAL x, REAL y, REAL z){
    renderer->Vertex(x,y,z);
}

inline void Vertex3(REAL *x){
    renderer->Vertex3(x);
}

inline void Vertex(REAL x, REAL y, REAL z, REAL w){
    renderer->Vertex(x,y,z,w);
}

inline void TexCoord(REAL u, REAL v){
    renderer->TexCoord(u,v);
}

inline void TexCoord(REAL u, REAL v, REAL w){
    renderer->TexCoord(u,v,w);
}

inline void TexCoord(REAL u, REAL v, REAL w, REAL t){
    renderer->TexCoord(u,v,w,t);
}

inline void TexVertex(REAL x, REAL y, REAL z,
                      REAL u, REAL v){
    renderer->TexVertex(x,y,z,u,v);
}

inline void Color(REAL r, REAL g, REAL b){
    renderer->Color(r,g,b);
}

inline void Color(REAL r, REAL g, REAL b,REAL a){
    renderer->Color(r,g,b,a);
}

inline void RenderEnd(bool force=false){
    renderer->End(force);
}

inline void BeginLines(){
    renderer->BeginLines();
}

inline void BeginTriangles(){
    renderer->BeginTriangles();
}

inline void BeginQuads(){
    renderer->BeginQuads();
}

inline void BeginLineStrip(){
    renderer->BeginLineStrip();
}

inline void BeginLineLoop(){
    renderer->BeginLineLoop();
}

inline void BeginTriangleStrip(){
    renderer->BeginTriangleStrip();
}

inline void BeginQuadStrip(){
    renderer->BeginQuadStrip();
}


inline void IsEdge(bool ie){
    renderer->IsEdge(ie);
}


inline void BeginTriangleFan(){
    renderer->BeginTriangleFan();
}


inline void Line(REAL x1, REAL y1, REAL z1,
                 REAL x2, REAL y2, REAL z2){
    renderer->Line(x1,y1,z1,x2,y2,z2);
}



inline void ProjMatrix(){
    renderer->ProjMatrix();
}

inline void ModelMatrix(){
    renderer->ModelMatrix();
}

inline void TexMatrix(){
    renderer->TexMatrix();
}


inline void PushMatrix(){
    renderer->PushMatrix();
}

inline void PopMatrix(){
    renderer->PopMatrix();
}


inline void MultMatrix(REAL mdata[4][4]){
    renderer->MultMatrix(mdata);
}


inline void IdentityMatrix(){
    renderer->IdentityMatrix();
}

inline void ScaleMatrix(REAL f){
    renderer->ScaleMatrix(f);
}

inline void ScaleMatrix(REAL f1, REAL f2, REAL f3){
    renderer->ScaleMatrix(f1,f2,f3);
}


inline void TranslateMatrix(REAL x1, REAL x2, REAL x3){
    renderer->TranslateMatrix(x1,x2,x3);
}

void sr_RendererCleanup();
void sr_glRendererInit();

#endif
