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

#ifndef ArmageTron_CONSOLE_H
#define ArmageTron_CONSOLE_H

#include "defs.h"
#include "tString.h"
#include "tConsole.h"
#include "tCallback.h"
#include "tArray.h"

extern REAL rCWIDTH_CON;
extern REAL rCHEIGHT_CON;

class rConsole:public tConsole{
    tArray<tString> lines;

    int currentTop; // the line currently at top of the screen
    int currentIn;  // the line currently written into

    double lastTimeout; // the time the last message dissapeared
    double lastCustomTimeout; // the time the last custom key was pressed

    // the following Variables are only of interest in non-fullscreen mode:
    int height; // the maximum height of the console in lines
    REAL timeout;  // the time until one message dissapears

    static int MaxHeight(); // max height

    void DisplayAtNewline();
public:
    bool fullscreen; // should the con be displayed fullscreen or
    // be limited to the upper edge of the screen?

    bool autoDisplayAtSwap; // do we need to call GL_Display manually?
    bool autoDisplayAtNewline;

    rConsole();

    int Height();
    REAL Timeout();

    void SetHeight(int h,bool stop_scroll=true);
    void SetTimeout(REAL to);

    void Render();

    //  rConsole & operator<<(const tString &s);
    virtual tConsole & DoPrint( const tString& s );

    //! scrolls up or down
    void Scroll(int dir);

    //! moves to the end, showing the last lines
    void End(int last);

    //! check whether a center display has recently been issued
    static bool CenterDisplayBusy();

    virtual void DoCenterDisplay(const tString &s,REAL timeout=2,REAL r=1,REAL g=1,REAL b=1);

    virtual tString ColorString(REAL r, REAL g, REAL b) const;

    //! returns whether a center display is currently in progress
    static bool CenterDisplayActive();
};


extern rConsole sr_con; // where all the output is directed to

#ifdef DEDICATED
void sr_Read_stdin();
#endif

// passes ladderlog output to external scripts
void sr_InputForScripts( char const * input );

class rForceTextCallback:public tCallbackOr{
public:
    rForceTextCallback(BOOLRETFUNC *f);
    static bool ForceText();
};

class rSmallConsoleCallback:public tCallbackOr{
public:
    rSmallConsoleCallback(BOOLRETFUNC *f);
    static bool SmallColsole();
};

class rCenterDisplayCallback:public tCallback{
public:
    rCenterDisplayCallback(AA_VOIDFUNC *f);
    static void CenterDisplay();
};

// if one of those callbacks returns true, auto display at newline is disabled.
class rNoAutoDisplayAtNewlineCallback:public tCallbackOr{
public:
    rNoAutoDisplayAtNewlineCallback(BOOLRETFUNC *f);
    static bool NoAutoDisplayAtNewline();
};

#endif
