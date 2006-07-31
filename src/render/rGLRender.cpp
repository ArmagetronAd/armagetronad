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

#include "config.h"

#ifndef DEDICATED

#define DONTDOIT
#include "rRender.h"
#include "rGL.h"
#include "tMemManager.h"
#include "tError.h"

class glRenderer: public rRenderer{
    GLenum lastPrimitive;
    bool   forceglEnd;

    GLenum lastMatrix;

    void BeginPrimitive(GLenum p, bool forceEnd = false){
        //  glBegin(p);
        //  return;

        if (lastPrimitive != p && lastPrimitive != GL_FALSE)
            glEnd();

        lastPrimitive = p;
        glBegin(p);

        forceglEnd = forceEnd;
    }

    void MatrixMode(GLenum mm){
        //    if (lastMatrix != mm)
        {
            glMatrixMode(mm);
            lastMatrix = mm;
        }
    }

public:
    glRenderer():lastPrimitive(GL_FALSE), lastMatrix(GL_FALSE){
        ChangeFlags(0xffffffff,0);
    };

    virtual ~glRenderer(){};

    virtual void Vertex(REAL x, REAL y){
        glVertex2f(x,y);
    };

    virtual void Vertex(REAL x, REAL y, REAL z){
        glVertex3f(x,y,z);
    }

    virtual void Vertex3(REAL *x){
        glVertex3fv(x);
    }

    virtual void Vertex(REAL x, REAL y, REAL z, REAL w){
        glVertex4f(x,y,z,w);
    }

    virtual void TexCoord(REAL u, REAL v){
        glTexCoord2f(u,v);
    }

    virtual void TexCoord(REAL u, REAL v, REAL w){
        glTexCoord3f(u,v,w);
    }

    virtual void TexCoord(REAL u, REAL v, REAL w, REAL t){
        glTexCoord4f(u,v,w,t);
    };

    virtual void TexVertex(REAL x, REAL y, REAL z,
                           REAL u, REAL v){
        glTexCoord2f(u,v);
        glVertex3f(x,y,z);
    }


    virtual void Color(REAL r, REAL g, REAL b){
        glColor3f(r,g,b);
    };

    virtual void Color(REAL r, REAL g, REAL b,REAL a){
        glColor4f(r,g,b,a);
    };


    virtual void End(bool force=false){
        //    glEnd();
        //    return;

        if ((forceglEnd || force || true) && lastPrimitive!=GL_FALSE)
        {
            forceglEnd = false;
            glEnd();
            lastPrimitive = GL_FALSE;
        }
    }

    virtual void BeginLines(){
        BeginPrimitive(GL_LINES);
    };

    virtual void BeginTriangles(){
        BeginPrimitive(GL_TRIANGLES);
    }

    virtual void BeginQuads(){
        BeginPrimitive(GL_QUADS);
    }

    virtual void IsEdge(bool ie){
        glEdgeFlag(ie ? GL_TRUE : GL_FALSE);
    };

    virtual void BeginLineStrip(){
        BeginPrimitive(GL_LINE_STRIP, true);
    };

    virtual void BeginLineLoop(){
        BeginPrimitive(GL_LINE_LOOP, true);
    };

    virtual void BeginTriangleStrip(){
        BeginPrimitive(GL_TRIANGLE_STRIP, true);
    };

    virtual void BeginQuadStrip(){
        BeginPrimitive(GL_QUAD_STRIP, true);
    };

    virtual void BeginTriangleFan(){
        BeginPrimitive(GL_TRIANGLE_FAN, true);
    };

    virtual void Line(REAL x1, REAL y1, REAL z1,
                      REAL x2, REAL y2, REAL z2){
        BeginPrimitive(GL_LINES);
        glVertex3f(x1,y1,z1);
        glVertex3f(x2,y2,z2);
        End();
    }




    virtual void ProjMatrix(){
        End(true);
        MatrixMode(GL_PROJECTION);
    };

    virtual void ModelMatrix(){
        End(true);
        MatrixMode(GL_MODELVIEW);
    };

    virtual void TexMatrix(){
        End(true);
        MatrixMode(GL_TEXTURE);
    };

    virtual void PushMatrix(){
        glPushMatrix();
    };

    virtual void PopMatrix(){
        End(true);
        glPopMatrix();
    };

    virtual void MultMatrix(REAL mdata[4][4]){
        End(true);
        tASSERT(sizeof(REAL) == sizeof(GLfloat));
        glMultMatrixf(reinterpret_cast<GLfloat *>(&mdata));
    };

    virtual void IdentityMatrix(){
        End(true);
        glLoadIdentity();
    };

    virtual void ScaleMatrix(REAL f){
        End(true);
        glScalef(f,f,f);
    };

    virtual void ScaleMatrix(REAL f1, REAL f2, REAL f3){
        End(true);
        glScalef(f1,f2,f2);
    };

    virtual void TranslateMatrix(REAL x1, REAL x2, REAL x3){
        End(true);
        glTranslatef(x1,x2,x3);
    }



    virtual void ReallySetFlag(flag f,bool c){
        GLenum fl = GL_DEPTH_TEST;
        switch (f)
        {
        case ALPHA_BLEND:
            fl = GL_BLEND;
            break;
        case DEPTH_TEST:
            fl = GL_DEPTH_TEST;
            break;
        default:
            break;
        }

        if (c)
            glEnable(fl);
        else
            glDisable(fl);
    };

};

void sr_glRendererInit(){
    tNEW(glRenderer);
}

#endif
