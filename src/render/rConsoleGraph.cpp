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
#include "rRender.h"
#include "tSysTime.h"
#include "rConsole.h"
#include "rSysdep.h"
#include "rScreen.h"
#include "rGL.h"
#include "tConfiguration.h"
#include "rDisplayList.h"

static tString sr_centerString;
static REAL center_r,center_g,center_b,center_fadetime;

static REAL Time;

static rDisplayListAlphaSensitive sr_consoleDisplayList;

// flag memorizing whether the console already has been rendered this frame
static bool sr_alreadyDisplayed = false;

static void sr_ConsolePerFrame(){
    if (sr_con.autoDisplayAtSwap)
    {
        sr_con.Render();
    }
    sr_alreadyDisplayed = false;
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

static int sr_indent = 3;
static tConfItem<int> sr_indentConf("CONSOLE_INDENT",sr_indent);

void rConsole::Render(){
    if( sr_alreadyDisplayed )
    {
        return;
    }

    sr_alreadyDisplayed = true;

    if (!sr_glOut)
        return;

    static REAL lastBottom = -1.0;

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

    // get corresponding character height
    rCHEIGHT_CON=rCWIDTH_CON*W*9/(5*H);

    if (sr_screen){
        Time=tSysTimeFloat();

        if (Time-center_fadetime<2){
            REAL alpha=center_fadetime-Time+1;
            if (alpha>1) alpha=1;
            if (alpha<0) alpha=0;
            rTextField::SetDefaultColor( tColor(center_r,center_g,center_b,alpha) );

            REAL space = 1.6;
            REAL needed = rCWIDTH_CON * 4 * sr_centerString.Len();
            REAL fak = 1;
            if (needed > space)
                fak = space/needed;

            DisplayText(0,centerMessageY,rCWIDTH_CON*4*fak,rCHEIGHT_CON*4*fak,sr_centerString);
            RenderEnd();
            sr_ResetRenderState(true);
        }

        if (sr_textOut || rForceTextCallback::ForceText()){
            if (lastCustomTimeout<Time-5 &&
                    lastTimeout+timeout<Time && currentTop<currentIn){
                currentTop++;
                lastTimeout=Time;
            }

            rTextField out(-.95f,.99f,rCWIDTH_CON,rCHEIGHT_CON);//,&rFont::s_defaultFontSmall);
            out.SetWidth(static_cast<int>(1.9f/out.GetCWidth()));

            static int lastTop = currentTop;
            static int lastIn  = currentIn;
            REAL predictBottom = lastBottom - ( lastTop - currentTop + currentIn - lastIn ) * out.GetCHeight();

            if ( lastTop != currentTop || lastIn != currentIn )
            {
                lastTop = currentTop;
                lastIn  = currentIn;
                sr_consoleDisplayList.Clear();
            }

            rTextField::SetDefaultColor( tColor(1,1,1) );

            if ( sr_consoleDisplayList.Call() )
            {
                return;
            }
            rDisplayListFiller filler( sr_consoleDisplayList );

            out.SetIndent(sr_indent);

            if( sr_alphaBlend )
            {
                RenderEnd();
                glColor4f(0, 0, 0, .5f);
                glRectf(-1,predictBottom,1,1);
            }

            int i;
            for (i=currentTop;i<=currentIn && i<=currentTop+MaxHeight();i++)
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

            // check for mispredictions of console height
            lastBottom = out.GetBottom();
            if( fabs(predictBottom - lastBottom) > .0001 )
            {
                sr_consoleDisplayList.Clear();
            }

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

bool rConsole::CenterDisplayActive()
{
    return tSysTimeFloat() - center_fadetime < 1.5;
}


// passes ladderlog output to external scripts (do nothing here)
void sr_InputForScripts( char const * input )
{}

#else
#include "rConsoleCout.cpp"
#endif
