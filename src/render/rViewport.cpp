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

#include "rFont.h"
#include "rScreen.h"
#include "rViewport.h"
#include "rConsole.h"
#include "tConfiguration.h"

#ifndef DEDICATED
#include "rGL.h"
//#include <GL/glu>
#ifdef POWERPAK_DEB
#include "PowerPak/powerdraw.h"
#endif
#endif

#ifndef DEDICATED
void rViewport::Select(){
    if (sr_glOut)
        glViewport (GLsizei(sr_screenWidth*left),
                    GLsizei(sr_screenHeight*bottom),
                    GLsizei(sr_screenWidth*width),
                    GLsizei(sr_screenHeight*height));
}
#endif


REAL rViewport::UpDownFOV(REAL fov){
    REAL ratio=currentScreensetting.aspect*(width*sr_screenWidth)/(height*sr_screenHeight);

    // clamp ratio to 5/3
    REAL maxratio = 5.0/3.0;
    if (ratio > maxratio)
        ratio = maxratio;

    return 360*atan(tan(M_PI*fov/360)/ratio)/M_PI;
}

void rViewport::Perspective(REAL fov,REAL nnear,REAL ffar){
#ifndef DEDICATED
    if (!sr_glOut)
        return;
    REAL ratio=currentScreensetting.aspect*(width*sr_screenWidth)/(height*sr_screenHeight);
    // REAL udfov=360*atan(tan(M_PI*fov/360)/ratio)/M_PI;
    REAL udfov=UpDownFOV(fov);
    glMatrixMode(GL_PROJECTION);
    gluPerspective(
        udfov,
        ratio,
        nnear,
        ffar
    );
#endif
}

rViewport rViewport::s_viewportFullscreen(0,0,1,1);

rViewport rViewport::s_viewportTop(0,.5,1,.5);
rViewport rViewport::s_viewportBottom(0,0,1,.5);

rViewport rViewport::s_viewportLeft(0,0,.5,1);
rViewport rViewport::s_viewportRight(.5,0,.5,1);

rViewport rViewport::s_viewportTopLeft(0,.5,.5,.5);
rViewport rViewport::s_viewportBottomLeft(0,0,.5,.5);
rViewport rViewport::s_viewportTopRight(.5,.5,.5,.5);
rViewport rViewport::s_viewportBottomRight(.5,0,.5,.5);
rViewport rViewport::s_viewportDemonstation(.55,.05,.4,.4);

int   sr_viewportBelongsToPlayer[MAX_VIEWPORTS],
s_newViewportBelongsToPlayer[MAX_VIEWPORTS];

// ***********************************************************


rViewportConfiguration::rViewportConfiguration(rViewport *first)
        :num_viewports(1){
    viewports[0]=first;
}

rViewportConfiguration::rViewportConfiguration(rViewport *first,
        rViewport *second)
        :num_viewports(2){
    viewports[0]=first;
    viewports[1]=second;
}

rViewportConfiguration::rViewportConfiguration(rViewport *first,
        rViewport *second,
        rViewport *third)
        :num_viewports(3){
    viewports[0]=first;
    viewports[1]=second;
    viewports[2]=third;
}

rViewportConfiguration::rViewportConfiguration(rViewport *first,
        rViewport *second,
        rViewport *third,
        rViewport *forth)
        :num_viewports(4){
    viewports[0]=first;
    viewports[1]=second;
    viewports[2]=third;
    viewports[3]=forth;
}

#ifndef DEDICATED
void rViewportConfiguration::Select(int i){
    if (i>=0 && i <num_viewports)
        viewports[i]->Select();
}
#endif

rViewport * rViewportConfiguration::Port(int i){
    if (i>=0 && i <num_viewports)
        return viewports[i];
    else
        return NULL;
}

static rViewportConfiguration single_vp(&rViewport::s_viewportFullscreen);
static rViewportConfiguration two_vp(&rViewport::s_viewportTop,
                                     &rViewport::s_viewportBottom);
static rViewportConfiguration two_b(&rViewport::s_viewportLeft,
                                    &rViewport::s_viewportRight);
static rViewportConfiguration three_a(&rViewport::s_viewportTop,
                                      &rViewport::s_viewportBottomLeft,
                                      &rViewport::s_viewportBottomRight);
static rViewportConfiguration three_b(&rViewport::s_viewportTopLeft,
                                      &rViewport::s_viewportTopRight,
                                      &rViewport::s_viewportBottom);
static rViewportConfiguration four_vp(&rViewport::s_viewportTopLeft,
                                      &rViewport::s_viewportTopRight,
                                      &rViewport::s_viewportBottomLeft,
                                      &rViewport::s_viewportBottomRight);

rViewportConfiguration *rViewportConfiguration::s_viewportConfigurations[]={
            &single_vp,&two_vp,&two_b,&three_a,&three_b,&four_vp};

char *rViewportConfiguration::s_viewportConfigurationNames[]=
    {"$viewport_conf_name_0",
     "$viewport_conf_name_1",
     "$viewport_conf_name_2",
     "$viewport_conf_name_3",
     "$viewport_conf_name_4",
     "$viewport_conf_name_5"};

const int  rViewportConfiguration::s_viewportNumConfigurations=6;



// *******************************************************
//   Player menu
// *******************************************************

static int conf_num=0;
int rViewportConfiguration::next_conf_num=0;

static tConfItem<int> confn("VIEWPORT_CONF",
                            rViewportConfiguration::next_conf_num);

rViewportConfiguration *rViewportConfiguration::CurrentViewportConfiguration(){
    if (conf_num<0) conf_num=0;
    if (conf_num>=s_viewportNumConfigurations)
        conf_num=s_viewportNumConfigurations-1;

    return s_viewportConfigurations[conf_num];
}

#ifndef DEDICATED
void rViewportConfiguration::DemonstrateViewport(tString *titles){
    if (!sr_glOut)
        return;

    for(int i=s_viewportConfigurations[next_conf_num]->num_viewports-1;i>=0;i--){
        rViewport sub(rViewport::s_viewportDemonstation,*(s_viewportConfigurations[next_conf_num]->Port(i)));
        sub.Select();

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);

        glColor3f(.1,.1,.4);
        glRectf(-.9,-.9,.9,.9);

        glColor3f(.6,.6,.6);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-1,-1);
        glVertex2f(-1,1);
        glVertex2f(1,1);
        glVertex2f(1,-1);
        glEnd();

        glColor3f(1,1,1);
        DisplayText(0,0,.15,.5,titles[i]);
    }

    rViewport::s_viewportFullscreen.Select();
}
#endif


rViewport * rViewportConfiguration::CurrentViewport(int i){
    return CurrentViewportConfiguration()->Port(i);
}

void rViewportConfiguration::UpdateConf(){
    conf_num = next_conf_num;
}


static int vpb_dir[MAX_VIEWPORTS];

void rViewport::CorrectViewport(int i, int MAX_PLAYERS){
    if (vpb_dir[i]!=1 && vpb_dir[i]!=-1)
        vpb_dir[i]=1;
    s_newViewportBelongsToPlayer[i]+=MAX_PLAYERS-vpb_dir[i];
    s_newViewportBelongsToPlayer[i]%=MAX_PLAYERS;
    bool again;
    do{
        s_newViewportBelongsToPlayer[i]+=MAX_PLAYERS+vpb_dir[i];
        s_newViewportBelongsToPlayer[i]%=MAX_PLAYERS;
        again=false;
        int starta=rViewportConfiguration::s_viewportConfigurations[rViewportConfiguration::next_conf_num]->num_viewports-1;
        int startb=rViewportConfiguration::s_viewportConfigurations[     conf_num]->num_viewports-1;
        if (starta>startb)
            startb=starta;
        for(int j=starta;j>=0;j--)
            if (i!=j && s_newViewportBelongsToPlayer[i]
                    ==s_newViewportBelongsToPlayer[j])
                again=true;
    } while(again);
}

void rViewport::CorrectViewports(int MAX_PLAYERS){
    for (int i=rViewportConfiguration::s_viewportConfigurations[conf_num]->num_viewports-1;i>=0;i--)
        CorrectViewport(i, MAX_PLAYERS);
}


void rViewport::Update(int MAX_PLAYERS){
    rViewportConfiguration::UpdateConf();
    CorrectViewports(MAX_PLAYERS);

    int i;
    for(i=MAX_VIEWPORTS-1;i>=0;i--)
        sr_viewportBelongsToPlayer[i]=s_newViewportBelongsToPlayer[i];
}

void rViewport::SetDirectionOfCorrection(int vp, int dir){
    vpb_dir[vp] = dir;
}

// *******************************************************************************************
// *
// *	CorrectAspectBottom
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

rViewport rViewport::CorrectAspectBottom( void ) const
{
    rViewport ret( *this );
    ret.height = width * 4.0 / 3.0;

    return ret;
}


