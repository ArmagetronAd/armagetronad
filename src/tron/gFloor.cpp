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

#include "rSDL.h"

#include "defs.h"
#include "gStuff.h"
#include "gLogo.h"
#include "eFloor.h"
#include "tConfiguration.h"
#include "rScreen.h"


// grid size
static REAL sg_gridSize=1;
static tSettingItem<REAL> g_s("GRID_SIZE",sg_gridSize);
static REAL sg_gridSizeMoviePack=2;
static tSettingItem<REAL> g_sm("GRID_SIZE_MOVIEPACK",sg_gridSizeMoviePack);

static REAL moviepack_floor_red=.5,moviepack_floor_green=.5,moviepack_floor_blue=.5;
static REAL floor_red=.15,floor_green=.3,floor_blue=.15;

static tSettingItem<REAL>
mfr("MOVIEPACK_FLOOR_RED",moviepack_floor_red),
mfg("MOVIEPACK_FLOOR_GREEN",moviepack_floor_green),
mfb("MOVIEPACK_FLOOR_BLUE",moviepack_floor_blue),
fr("FLOOR_RED",floor_red),
fg("FLOOR_GREEN",floor_green),
fb("FLOOR_BLUE",floor_blue);

#ifndef DEDICATED
#include "rTexture.h"
#include "rRender.h"
#include "uMenu.h"
#include "tSysTime.h"

#include "nConfig.h"
/*
static tString lala_floor_a("Anonymous/original/textures/floor_a.png");
static nSettingItem<tString> lalala_floor_a("TEXTURE_FLOOR_A", lala_floor_a);
rFileTexture floor_a(rTextureGroups::TEX_FLOOR, lala_floor_a, 1,1);

static tString lala_floor_b("Anonymous/original/textures/floor_b.png");
static nSettingItem<tString> lalala_floor_b("TEXTURE_FLOOR_B", lala_floor_b);
rFileTexture floor_b(rTextureGroups::TEX_FLOOR, lala_floor_b, 1,1);

static tString lala_mp_floor_a("Anonymous/original/moviepack/floor_a.png");
static nSettingItem<tString> lalala_mp_floor_a("TEXTURE_MP_FLOOR_A", lala_mp_floor_a);
rFileTexture mp_floor_a(rTextureGroups::TEX_FLOOR, lala_mp_floor_a, 1,1,true);

static tString lala_mp_floor_b("Anonymous/original/moviepack/floor_b.png");
static nSettingItem<tString> lalala_mp_floor_b("TEXTURE_MP_FLOOR_B", lala_mp_floor_b);
rFileTexture mp_floor_b(rTextureGroups::TEX_FLOOR, lala_mp_floor_b, 1,1,true);

static tString lala_floor("Anonymous/original/textures/floor.png");
static nSettingItem<tString> lalala_floor("TEXTURE_FLOOR", lala_floor);
rFileTexture ArmageTron_floor(rTextureGroups::TEX_FLOOR, lala_floor, 1,1);

static tString lala_mp_floor("Anonymous/original/moviepack/floor.png");
static nSettingItem<tString> lalala_mp_floor("TEXTURE_MP_FLOOR", lala_mp_floor);
rFileTexture ArmageTron_mp_floor(rTextureGroups::TEX_FLOOR, lala_mp_floor, 1,1);
*/

static rFileTexture floor_a(rTextureGroups::TEX_FLOOR,"textures/floor_a.png",1,1);
static rFileTexture floor_b(rTextureGroups::TEX_FLOOR,"textures/floor_b.png",1,1);
static rFileTexture mp_floor_a(rTextureGroups::TEX_FLOOR,"moviepack/floor_a.png",1,1,true);
static rFileTexture mp_floor_b(rTextureGroups::TEX_FLOOR,"moviepack/floor_b.png",1,1,true);
rFileTexture ArmageTron_floor(rTextureGroups::TEX_FLOOR,"textures/floor.png",1,1);
rFileTexture ArmageTron_mp_floor(rTextureGroups::TEX_FLOOR,"moviepack/floor.png",1,1);

class gFloor: public eFloor{
public:
    gFloor(){};
    virtual ~gFloor(){};

    virtual void glFloorColor(REAL alpha, REAL intens){
        if (!sr_glOut)
            return;

        REAL r, g, b;
        FloorColor(r, g, b);
        renderer->Color(r, g, b, alpha);
    }

    virtual void FloorColor(REAL& r, REAL& g, REAL&b){
        if (sg_MoviePack())
        {
            r = moviepack_floor_red;
            g = moviepack_floor_green;
            b = moviepack_floor_blue;
        }
        else
        {
            r = floor_red;
            g = floor_green;
            b = floor_blue;
        }
    }


    virtual void glFloorTexture(){
        if (sg_MoviePack())
            ArmageTron_mp_floor.Select();
        else
            ArmageTron_floor.Select();
    }

    virtual void glFloorTexture_a(){
        if (sg_MoviePack())
            mp_floor_a.Select();
        else
            floor_a.Select();
    }

    virtual void glFloorTexture_b(){
        if (sg_MoviePack())
            mp_floor_b.Select();
        else
            floor_b.Select();
    }

    virtual REAL GridSize(){
        if (sg_MoviePack())
            return sg_gridSizeMoviePack;
        else
            return sg_gridSize;
    }

    virtual bool BlackSky(){
        return sg_MoviePack();
    }

};

static gFloor GFLOOR;

static void MenuBackground(){
    if (rTextureGroups::TextureMode[rTextureGroups::TEX_FLOOR]>=0){
        se_glFloorTexture();
        se_glFloorColor(1,1);

        double x1=tSysTimeFloat()/3.0;
        double y1=tSysTimeFloat()/5.0;
        REAL width=16;
        REAL height=12;

        GLfloat tm[4][4]={{.8,.2,0,0},
                          {-.2,.8,0,0},
                          {0,0,1,0},
                          {0,0,0,1}};



        REAL scale = (sr_screenWidth*3.0)/(sr_screenHeight*4.0);
        tm[0][0] *= scale;
        tm[0][1] *= scale;

        // make texture coordinates not too big, wrap them around.
        // unfortunately, we need to transform them with tm, then clamp them,
        // then transform them back.
        double x2 = x1*tm[0][0] + y1*tm[1][0];
        double y2 = x1*tm[0][1] + y1*tm[1][1];
        x2-=floor(x2);
        y2-=floor(y2);
        REAL x = x2*tm[1][1] - y2*tm[1][0];
        REAL y = -x2*tm[0][1] + y2*tm[0][0];
        const REAL det=1/(tm[0][0]*tm[1][1]-tm[0][1]*tm[1][0]);
        x*=det;
        y*=det;
        //x=x1;
        //y=y1;

        TexMatrix();
        glLoadMatrixf(&tm[0][0]);

        // glScalef(1., 1., 1.);
        // glScalef((REAL)sr_screenWidth/sr_screenHeight/4. * 3., 1., 1.);


        BeginQuads();

        glTexCoord2d(x,y);
        glVertex2f(-1,-1);

        glTexCoord2d(x+width,y);
        glVertex2f(1,-1);

        glTexCoord2d(x+width,y-height);
        glVertex2f(1,1);

        glTexCoord2d(x,y-height);
        glVertex2f(-1,1);

        RenderEnd();

        TexMatrix();
        glLoadIdentity();
    }

    gLogo::Display();
}

static uCallbackMenuBackground backgr(&MenuBackground);

#endif
