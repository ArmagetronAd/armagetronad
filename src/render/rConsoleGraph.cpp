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

#include "config.h"

#ifndef DEDICATED

#include "rFont.h"
#include "tSysTime.h"
#include "rConsole.h"
#include "rSysdep.h"
#include "rScreen.h"
#include "rGL.h"
#include "tConfiguration.h"

static tColoredString sr_centerString;
static REAL center_r,center_g,center_b,center_fadetime;

static REAL Time;

static void sr_ConsolePerFrame(){
    if (sr_con.autoDisplayAtSwap)
        sr_con.Render();
}

static rPerFrameTask console_pf(&sr_ConsolePerFrame);


void rConsole::DisplayAtNewline(){
    bool sw=autoDisplayAtSwap;
    autoDisplayAtSwap=true;
    if (sr_glOut){
        rSysDep::SwapGL();
        rSysDep::ClearGL();
    }
    autoDisplayAtSwap=sw;
}

REAL centerMessageY=0;

static tConfItem<REAL> cmlocy("CM_LOCY",centerMessageY);

static int sr_columns = 78;
static tConfItem<int> sr_columnsConf("CONSOLE_COLUMNS",sr_columns);

void rConsole::Render(){
    if (!sr_glOut)
        return;

    sr_ResetRenderState(true);

    REAL W=sr_screenWidth;
    REAL H=sr_screenHeight;

    // previous logic
    //REAL MW=400;
    //REAL MH=(MW*3)/4;
    //if(W>MW)
    //    W=MW;
    //if(H>MH)
    //    H=MH;
    // rCWIDTH_CON=10/W;
    // rCHEIGHT_CON=18/H;

    // the text field has an openGL coordinate with of 1.9; cram the specified number
    // of columns in it
    rCWIDTH_CON=1.9/sr_columns;

    // but don't shrink the small font.
    if (rCWIDTH_CON < 10/W)
        rCWIDTH_CON = 10/W;

    // get corresponding character height
    rCHEIGHT_CON=rCWIDTH_CON*W*9/(5*H);

    if (sr_screen){
        Time=tSysTimeFloat();

        if (sr_textOut || rForceTextCallback::ForceText()){
            if(lastCustomTimeout<Time-5 &&
                    lastTimeout+timeout<Time && currentTop<currentIn){
                currentTop++;
                lastTimeout=Time;
            }

            rTextField::SetDefaultColor( tColor(1,1,1) );

            rTextField out(-.95f,.99f,rCWIDTH_CON,rCHEIGHT_CON);//,&rFont::s_defaultFontSmall);
            out.SetWidth(static_cast<int>(1.9f/rCWIDTH_CON));
	    out.EnableLineWrap();

            int i;
            for(i=currentTop;i<=currentIn && i<=currentTop+MaxHeight();i++)
                if (lines[i].Len()>1){
                    rTextField::SetDefaultColor( tColor(1,1,1) );
                    out << lines[i];
                    out.ResetColor();
                }
            if (i<currentIn){
                // rTextField::SetDefaultColor( tColor(1,.8,.5) );
                out << tColoredString::ColorString( 1,.8,.5) << "       v   v   v   v   v   v   v   v   v\n";
            }


            int over=out.Lines()-Height();
            if (over>0 && (rSmallConsoleCallback::SmallColsole() || lastCustomTimeout<tSysTimeFloat()-15)){
                lastTimeout=Time;
                currentTop+=(over+1)/2;
            }
        }

        if (Time-center_fadetime<2){
            REAL alpha=center_fadetime-Time+1;
            if (alpha>1) alpha=1;
            if (alpha<0) alpha=0;
            rTextField::SetDefaultColor( tColor(center_r,center_g,center_b,alpha) );

            REAL width=rCWIDTH_CON*4;
            REAL height=rCHEIGHT_CON*4;
            REAL len=sr_centerString.LongestLine();
            REAL lines=(REAL)(sr_centerString.Count('\n')+1);

            REAL space = 1.6;
            REAL needed = width * len;
            if (needed > space) {
                width *= space/needed;
                height *= space/needed;
            }
            space = 0.9 + centerMessageY;
            needed = height*lines;
            if (needed > space) {
                width *= space/needed;
                height *= space/needed;
            }

            DisplayText(0,centerMessageY,width,height,sr_centerString);
            //std::cerr << "DisplayText(0," << centerMessageY << "," << (rCWIDTH_CON*4*fak) << "," << (rCHEIGHT_CON*4*fak) << "," <<sr_centerString << ");\n";
        }

        rTextField::SetDefaultColor( tColor(1,1,1) );
    }
}


void CenterDisplay(const tString &s,REAL timeout,REAL r,REAL g,REAL b){
    rCenterDisplayCallback::CenterDisplay();

    sr_centerString=s;
    center_fadetime=timeout+tSysTimeFloat();
    center_r=r;
    center_g=g;
    center_b=b;
}

void rConsole::DoCenterDisplay(const tString &s,REAL timeout,REAL r,REAL g,REAL b){
    rCenterDisplayCallback::CenterDisplay();

    sr_centerString=s;
    center_fadetime=timeout+tSysTimeFloat();
    center_r=r;
    center_g=g;
    center_b=b;
}

#else
#include "rConsoleCout.cpp"
#endif
