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

//void se_FetchAndStoreSDLInput();

#include "rSDL.h"

#include "tConfiguration.h"

// floor mirror
#ifndef DEDICATED
static REAL sr_floorMirror_strength=.1;
static tSettingItem<REAL> f_m("FLOOR_MIRROR_INT",sr_floorMirror_strength);

#include "tEventQueue.h"
#include "uInputQueue.h"
#include "eTess2.h"
#include "rTexture.h"
#include "eGameObject.h"
#include "rFont.h"
#include "eTimer.h"
#include "eCamera.h"
#include "rScreen.h"
#include "rRender.h"
#include "eWall.h"
#include "eAdvWall.h"
#include "eFloor.h"
#include "ePath.h"
#include "eGrid.h"
#include "eDebugLine.h"
#include "tDirectories.h"
#include "eRectangle.h"

#define eWall_h 4
#define view_h 2.7

#ifdef DEBUG
bool debug_grid=0;
#endif

REAL upper_height=100;
REAL lower_height=50;


#ifndef DEDICATED

static rFileTexture sky(rTextureGroups::TEX_FLOOR,"textures/sky.png",1,1,true);
static rFileTexture sky_moviepack(rTextureGroups::TEX_FLOOR,"moviepack/sky.png",1,1,true);

extern bool sg_MoviePack();

static void sky_select(){
    if (sg_MoviePack()){
        // Since old movie packs usually don't include sky.png we need to
        // be nice and fall back to the default sky tecture. -k
        tString s = tDirectories::Data().GetReadPath( "moviepack/sky.png" );
        if(strlen(s) > 0)
            sky_moviepack.Select();
        else
            sky.Select();
    }
    else {
        sky.Select();
    }
}

// if the rip bug is activated, don't use the rim to draw the floor
extern short se_bugRip;

// passes a vertex with z-projected texture coordinates to OpenGL
static inline void TexVertex( REAL x, REAL y, REAL h)
{
    glTexCoord2f(x, y);
    glVertex3f  (x, y, h);
}

// renders a finite rectangle
static void finite_xy_plane( const eCoord &pos,const eCoord &dir,REAL h, eRectangle rect )
{
    // expand plane to camera position to avoid embarrasing reflection bug
    if ( sr_floorMirror )
        rect.Include( pos );

    // fetch rectangle coordinates
    REAL lx = rect.GetLow().x;
    REAL ly = rect.GetLow().y;
    REAL hx = rect.GetHigh().x;
    REAL hy = rect.GetHigh().y;

    // draw rectangle as triangle fan (good for avoiding artefacts near pos)
    BeginTriangleFan();
    TexVertex( pos.x-dir.x, pos.y-dir.y, h );
    TexVertex(lx, ly, h);
    TexVertex(lx, hy, h);
    TexVertex(hx, hy, h);
    TexVertex(hx, ly, h);
    TexVertex(lx, ly, h);
    RenderEnd();
}

static void infinity_xy_plane(eCoord const & pos, const eCoord &dir,REAL h=0){
    glEdgeFlag(GL_FALSE);

    bool use_rim=false;
    REAL zero=0;

    if (sr_highRim)
        use_rim=true;

    if ( se_bugRip )
        use_rim=false;

    // always use the rim if infinity rendering is turned off
    use_rim |= !sr_infinityPlane;

    if (use_rim){
        /*
          // the rim wall based rendering does not work properly for shaped arenas, so
          // it's been replaced.

                BeginTriangles();
                for(int i=se_rimWalls.Len()-1;i>=0;i--){
                    eCoord p1=se_rimWalls(i)->EndPoint(0);
                    eCoord p2=se_rimWalls(i)->EndPoint(1);

                    glTexCoord2f(pos.x, pos.y);
                    glVertex3f  (pos.x, pos.y, h);

                    glTexCoord2f(p1.x, p1.y);
                    glVertex3f  (p1.x, p1.y, h);

                    glTexCoord2f(p2.x, p2.y);
                    glVertex3f  (p2.x, p2.y, h);
                }
                RenderEnd();
        */
        finite_xy_plane( pos, dir, h, eWallRim::GetBounds() );
    }
    else
    {
        if (!sr_infinityPlane)
            zero=.001;

        BeginTriangleFan();

        glTexCoord4f(pos.x-dir.x, pos.y-dir.y, h, 1);
        glVertex4f  (pos.x-dir.x, pos.y-dir.y, h, 1);

        glTexCoord4f(1,0.1,zero*h,zero);
        glVertex4f  (1,0.1,zero*h,zero);

        glTexCoord4f(0.1,1.1,zero*h,zero);
        glVertex4f  (0.1,1.1,zero*h,zero);

        glTexCoord4f(-1,0.1,zero*h,zero);
        glVertex4f  (-1,0.1,zero*h,zero);

        glTexCoord4f(0.1,-1.1,zero*h,zero);
        glVertex4f  (0.1,-1.1,zero*h,zero);

        glTexCoord4f(1,0.1,zero*h,zero);
        glVertex4f  (1,0.1,zero*h,zero);

        RenderEnd();
    }
}

static REAL z=0;


int           eGrid::NumberOfCameras(){return cameras.Len();}
const eCoord& eGrid::CameraPos(int i){return cameras(i)->CameraPos();}
eCoord eGrid::CameraGlancePos(int i){return cameras(i)->CameraGlancePos();}
const eCoord& eGrid::CameraDir(int i){return cameras(i)->CameraDir();}
REAL          eGrid::CameraHeight(int i){return cameras(i)->CameraZ();}




eWall *displayed_eWall=0;

void draw_eWall(eGrid* grid, int v,int i, REAL& zNear, eCamera const * cam)
{
    if (i<se_wallsVisible[v].Len())
    {
        eWallView *view=se_wallsVisible[v](i);
#ifdef DEBUG
        if (view->Value()<=z)
        {
#endif
            displayed_eWall = view->Belongs();
            REAL len = displayed_eWall->Len();
            if ( len > .01)
            {
                REAL zDist = z - displayed_eWall->Height();
                if ( zDist < zNear )
                {
                    const eCoord& camPos = grid->CameraPos( v );
                    const eCoord& camDir = grid->CameraDir( v );
                    eCoord base = displayed_eWall->EndPoint(0);
                    eCoord end = displayed_eWall->EndPoint(1);

                    if ( eCoord::F( base-camPos, camDir ) > 0.01f || eCoord::F( end-camPos, camDir ) > 0.01f )
                    {
                        eCoord dirNorm = end - base;
                        dirNorm.Normalize();
                        eCoord camRelative = ( camPos - base ).Turn( dirNorm.Conj() );
                        REAL dist = fabs( camRelative.y );
                        if ( camRelative.x < 0 )
                        {
                            dist -= camRelative.x;
                        }
                        if ( camRelative.x > len )
                        {
                            dist += camRelative.x - len;
                        }
                        if ( dist < zDist )
                        {
                            dist = zDist;
                        }
                        // TODO: better criterion for ingoring of walls
                        if ( dist < zNear && dist > 0.001f )
                        {
                            zNear = dist;
                        }
                    }
                }

                displayed_eWall->Render(cam);
            }
            displayed_eWall=0;

            draw_eWall(grid,v,tHeapBase::UpperL(i),zNear,cam);
            draw_eWall(grid,v,tHeapBase::UpperR(i),zNear,cam);

#ifdef DEBUG
        }
#endif 
    }
}


void paint_sr_lowerSky(eGrid *grid, int viewer,bool sr_upperSky, eCoord const & camPos){
    TexMatrix();
    glLoadIdentity();
    glScalef(.005,.005,.005);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);

    if (sr_skyWobble){
        glTranslatef(se_GameTime()*.1,se_GameTime()*.07145,0);
        glScalef(1+.2*sin(se_GameTime()),1+.1*cos(se_GameTime()),1);
        glTranslatef(-300,-200,0);
    }

    sky_select();

    REAL sa=(lower_height-z)*.1;
    if (sa>1) sa=1;
    if (!sr_upperSky){
        sa=1;
        glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
    }
    if (sa>0){
        glColor4f(1,1,1,sa);
        infinity_xy_plane(camPos,grid->CameraDir(viewer),lower_height);
    }
    if (!sr_upperSky && sr_alphaBlend)
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

void eGrid::display_simple( int viewer,bool floor,
                            bool sr_upperSky,bool sr_lowerSky,
                            REAL flooralpha,
                            bool eWalls,bool gameObjects,
                            REAL& zNear){
    /*
    static GLfloat S[]={1,0,0,0};
    static GLfloat T[]={0,1,0,0};
    static GLfloat R[]={0,0,1,0};
    static GLfloat Q[]={0,0,0,1};

    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_S,GL_OBJECT_PLANE,S);

    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_T,GL_OBJECT_PLANE,T);

    glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_R,GL_OBJECT_PLANE,R);

    glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGenfv(GL_Q,GL_OBJECT_PLANE,Q);

    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    */


    glEdgeFlag(GL_FALSE);

    glDisable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);

    eCoord camPos = CameraGlancePos( viewer );
    // eWallRim::Bound( camPos, 10 );

    if (sr_upperSky || se_BlackSky()){
        if (se_BlackSky()){
            //glDisable(GL_TEXTURE);
            glDisable(GL_TEXTURE_2D);

            glColor3f(0,0,0);

            if ( z < lower_height )
                infinity_xy_plane(camPos, this->CameraDir(viewer),lower_height);

            glEnable(GL_TEXTURE_2D);
        }
        else {
            TexMatrix();
            glLoadIdentity();
            //      glScalef(.25,.25,.25);

            se_glFloorTexture();

            glColor3f(.5,.5,1);

            if ( z < upper_height )
                infinity_xy_plane(camPos, this->CameraDir(viewer),upper_height);
        }
    }

    if (sr_lowerSky && !sr_highRim){
        paint_sr_lowerSky(this, viewer,sr_upperSky, camPos);
    }

    if (floor){
        su_FetchAndStoreSDLInput();
        int floorDetail = sr_floorDetail;

        // no multitexturing without alpha blending
        if ( !sr_alphaBlend && floorDetail > rFLOOR_TEXTURE )
            floorDetail = rFLOOR_TEXTURE;

        switch(floorDetail){
        case rFLOOR_OFF:
            break;
        case rFLOOR_GRID:
            {
	#define SIDELEN   (se_GridSize())
	#define EXTENSION 10

                eCoord center = CameraPos(viewer) + CameraDir(viewer) * (SIDELEN * EXTENSION * .8);

                REAL x=center.x;
                REAL y=center.y;
                int xn=static_cast<int>(x/SIDELEN);
                int yn=static_cast<int>(y/SIDELEN);


                //glDisable(GL_TEXTURE);
                glDisable(GL_TEXTURE_2D);

	#define INTENSITY(x,xx) (1-(((x)-(xx))*((x)-(xx))/(EXTENSION*SIDELEN*EXTENSION*SIDELEN)))


                BeginLines();
                for(int i=xn-EXTENSION;i<=xn+EXTENSION;i++){
                    REAL intens=INTENSITY(i*SIDELEN,x);
                    if (intens<0) intens=0;
                    se_glFloorColor(intens,intens);
                    glVertex2f(i*SIDELEN,y-SIDELEN*(EXTENSION+1));
                    glVertex2f(i*SIDELEN,y+SIDELEN*(EXTENSION+1));
                }
                for(int j=yn-EXTENSION;j<=yn+EXTENSION;j++){
                    REAL intens=INTENSITY(j*SIDELEN,y);
                    if (intens<0) intens=0;
                    se_glFloorColor(intens,intens);
                    glVertex2f(x-(EXTENSION+1)*SIDELEN,j*SIDELEN);
                    glVertex2f(x+(EXTENSION+1)*SIDELEN,j*SIDELEN);
                }
                RenderEnd();
            }
            break;

        case rFLOOR_TEXTURE:
            TexMatrix();
            glLoadIdentity();
            glScalef(1/se_GridSize(),1/se_GridSize(),1.);

            se_glFloorTexture();
            se_glFloorColor(flooralpha);

            infinity_xy_plane( camPos, CameraDir(viewer) );

            /* old way: draw every triangle
            for(int i=eFace::faces.Len()-1;i>=0;i--){
            eFace *f=eFace::faces(i);

            if (f->visHeight[viewer]<z){
            glBegin(GL_TRIANGLES);
            for(int j=0;j<=2;j++){
            glVertex3f(f->p[j]->x,f->p[j]->y,0);
            }
            glEnd();
            }
            }
            */

            break;

        case rFLOOR_TWOTEXTURE:
            se_glFloorColor(flooralpha);

            TexMatrix();
            glLoadIdentity();
            REAL gs = 1/se_GridSize();
            glScalef(0.01*gs,gs,1.);

            se_glFloorTexture_a();
            infinity_xy_plane( camPos, CameraDir(viewer) );

            se_glFloorColor(flooralpha);

            TexMatrix();
            glLoadIdentity();
            glScalef(gs,.01*gs,1.);

            se_glFloorTexture_b();

            glDepthFunc(GL_LEQUAL);
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
            infinity_xy_plane( camPos, CameraDir(viewer) );
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

            break;
        }
    }

    TexMatrix();
    glLoadIdentity();
    ModelMatrix();

    //  glDisable(GL_TEXTURE_GEN_S);
    //  glDisable(GL_TEXTURE_GEN_T);
    //  glDisable(GL_TEXTURE_GEN_Q);
    //  glDisable(GL_TEXTURE_GEN_R);

    glEnable(GL_DEPTH_TEST);

    if(eWalls){
        for(int i=se_rimWalls.Len()-1;i>=0;i--){
            su_FetchAndStoreSDLInput();
            se_rimWalls(i)->RenderReal(cameras(viewer));
        }

        if (sr_lowerSky && sr_highRim){
            //      glEnable(GL_TEXTURE_GEN_S);
            //      glEnable(GL_TEXTURE_GEN_T);
            //      glEnable(GL_TEXTURE_GEN_Q);
            //      glEnable(GL_TEXTURE_GEN_R);

            paint_sr_lowerSky(this, viewer,sr_upperSky, camPos);

            //      glDisable(GL_TEXTURE_GEN_S);
            //      glDisable(GL_TEXTURE_GEN_T);
            //      glDisable(GL_TEXTURE_GEN_Q);
            //      glDisable(GL_TEXTURE_GEN_R);

            TexMatrix();
            glLoadIdentity();
            ModelMatrix();
        }

        glEnable(GL_DEPTH_TEST);
    }
    else
        glEnable(GL_DEPTH_TEST);

    if (eWalls){
        glDisable(GL_CULL_FACE);
        draw_eWall(this,viewer,0,zNear,cameras(viewer));

        /*
        #ifdef DEBUG
        for(int i=sg_netPlayerWalls.Len()-1;i>=0;i--){
          glMatrixMode(GL_MODELVIEW);
          glPushMatrix();
          if (sg_netPlayerWalls(i)->Preliminary())
        glTranslatef(0,0,4);
          else
        glTranslatef(0,0,8);
          if (sg_netPlayerWalls(i)->Wall())
        sg_netPlayerWalls(i)->Wall()->RenderList(false);
          glPopMatrix();
          }
        #endif
        */

        /*
        static int oldlen=0;
        int newlen=sg_netPlayerWalls.Len();
        if (newlen!=oldlen){
          con << "Number of player eWalls now " << newlen << '\n';
          oldlen=newlen;
        }
        */

    }

    if (gameObjects)
        eGameObject::RenderAll(this, cameras(viewer));

    eDebugLine::Render();
#ifdef DEBUG

    ePath::RenderLast();

    if (debug_grid){
        //glDisable(GL_TEXTURE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        BeginLines();

        int i;
        for(i=edges.Len()-1;i>=0;i--){
            eHalfEdge *e=edges[i];
            if (e->Face())
                glColor4f(1,1,1,1);
            else
                glColor4f(0,0,1,1);

            glVertex3f(e->Point()->x,e->Point()->y,10);
            glVertex3f(e->Point()->x,e->Point()->y,15);
            glVertex3f(e->Point()->x,e->Point()->y,.1);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,.1);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
            glVertex3f(e->other->Point()->x,e->other->Point()->y,15);

        }

        for(i=points.Len()-1;i>=0;i--){
            ePoint *p=points[i];
            glColor4f(1,0,0,1);
            glVertex3f(p->x,p->y,0);
            glVertex3f(p->x,p->y,(p->GetRefcount()+1)*5);
        }
        /*
        for(int i=sg_netPlayerWalls.Len()-1;i>=0;i--){
          eEdge *e=sg_netPlayerWalls[i]->Edge();
        glColor4f(0,1,0,1);

          glVertex3f(e->Point()->x,e->Point()->y,5);
          glVertex3f(e->Point()->x,e->Point()->y,10);
          glVertex3f(e->Point()->x,e->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,10);
          glVertex3f(e->other->Point()->x,e->other->Point()->y,5);
        }
        */
        RenderEnd();
    }
#endif

}
#endif

void eGrid::Render( eCamera* cam, int viewer, REAL& zNear ){
    if (!sr_glOut)
        return;
#ifndef DEDICATED
    ProjMatrix();

    z=CameraHeight(viewer);
    if ( zNear > z )
    {
        zNear = z;
    }

    if (sr_floorMirror){
        ModelMatrix();
        glScalef(1,1,-1);

        if (z>10) z=10;
        glFrontFace(GL_CW);

        bool us=false;
        bool ls=false;

        if (sr_floorMirror>=rMIRROR_ALL){
            us=sr_upperSky;
            ls=sr_lowerSky;
        }
        else if (sr_floorMirror>=rMIRROR_WALLS){
            if (sr_lowerSky)
                ls=true;
            else if (sr_upperSky)
                us=true;
        }

        cam->SetRenderingMain(false);
        display_simple(viewer,false,
                       us,ls,
                       0,
                       sr_floorMirror>=rMIRROR_WALLS,
                       sr_floorMirror>=rMIRROR_OBJECTS,
                       zNear);
        z=CameraHeight(viewer);
        glFrontFace(GL_CCW);
        ModelMatrix();
        glScalef(1,1,-1);


        cam->SetRenderingMain(true);
        display_simple(viewer,true,
                       sr_upperSky,sr_lowerSky,
                       1-sr_floorMirror_strength,
                       true,true,zNear);

    }
    else
    {
        cam->SetRenderingMain(true);
        display_simple(viewer,true,
                       sr_upperSky,sr_lowerSky,
                       1,
                       true,true,zNear);
    }


#ifdef EVENT_DEB
    //  for(int i=eEdge_crossing.Len()-1;i>=0;i--){
    //    eEdge_crossing(i)->Render();
    //  }
#endif
#endif
}


//void eEdgeViewer::Render(){}

/*
void eViewerCrossesEdge::Render(){
#ifndef DEDICATED
  ePoint *p1=e->Point();
  ePoint *p2=e->other->Point();

  REAL timeLeft=value-se_GameTime();

  REAL h;

  if (viewer==1){
    if (timeLeft>0){
      h=timeLeft+4;
      glColor4f(0,0,1,.5);
    }
    else{
      h=-timeLeft+4;
      glColor4f(1,0,0,.5);
    }

    //  else
    //glColor4f(1,0,0,.5);

    static rTexture ArmageTron_invis_eWall(rTEX_WALL,"textures/eWall2.png",1,0);
    
    ArmageTron_invis_eWall.Select();
    
    eWall::Render_helper(e,(p1->x+p1->y)/4,(p2->x+p2->y)/4,h,1,4);
  }
#endif
}
*/

#endif


