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

#include "aa_config.h"
#include "rConsole.h"
#include "rFont.h"
#include "tConfiguration.h"

#include <stdio.h>
#include <fcntl.h>
#include <sstream>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <io.h>
#include <windows.h>
//#define fileno _fileno
//#define fcntl _fcntl
#else
#include <signal.h>
#endif

void rConsole::DoCenterDisplay(const tString &s,REAL timeout,REAL r,REAL g,REAL b){
    std::cout << tColoredString::RemoveColors(s) << '\n';
    DisplayAtNewline();
}

static int stdin_descriptor;
static bool unblocked = false;

static void sr_HandleSigCont( int signal )
{
    // con << "Continuing.\n";

    // unblock stdin again
    sr_Unblock_stdin();
}

void sr_Unblock_stdin(){
#ifndef WIN32
    if ( !unblocked )
    {
        signal( SIGCONT, &sr_HandleSigCont );
    }
#endif

    unblocked = true;
    stdin_descriptor=fileno(stdin);
#ifndef WIN32
    // if (isatty(stdin_descriptor))
    {
        int flag=fcntl(stdin_descriptor,F_GETFL);
        fcntl(stdin_descriptor,F_SETFL,flag | O_NONBLOCK);
    }
#endif
}



#define MAXLINE 1000
static char line_in[MAXLINE+2];
static int currentIn=0;

void sr_Read_stdin(){
    tConfItemBase::LoadPlayback( true );

    if ( !unblocked )
    {
        return;
    }
#ifdef WIN32
    //  std::cerr << "\n";

    HANDLE stdinhandle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE stdouthandle = GetStdHandle(STD_OUTPUT_HANDLE);
    bool goon = true;
    while (goon)
    {
        unsigned long reallyread;
        INPUT_RECORD input;
        PeekConsoleInput(stdinhandle, &input, 1, &reallyread);
        if (reallyread > 0)
        {
            ReadConsoleInput(stdinhandle, &input, 1, &reallyread);
            if (input.EventType == KEY_EVENT)
            {
                char key = input.Event.KeyEvent.uChar.AsciiChar;
                DWORD written=0;

                if (key && input.Event.KeyEvent.bKeyDown)
                {
                    WriteConsole(stdouthandle, &key, 1, &written, NULL);
                    line_in[currentIn] = key;

                    if (key == 13 || currentIn>=MAXLINE-1){
                        line_in[currentIn]='\n';
                        line_in[currentIn+1]='\0';
                        std::stringstream s(line_in);
                        WriteConsole(stdouthandle, "\n", 1, &written, NULL);
                        tConfItemBase::LoadAll(s);
                        currentIn=0;
                    }
                    else
                        currentIn++;
                }
            }
            //		bool ret=ReadFile(stdinhandle, &line_in[currentIn], 1, &reallyread, NULL);
        }
        else
            goon = false;
    }


#else
    while ( read(stdin_descriptor,&line_in[currentIn],1)>0){
        if (line_in[currentIn]=='\n' || currentIn>=MAXLINE-1)
        {
            line_in[currentIn+1]='\0';
            std::stringstream s(line_in);
            tConfItemBase::LoadAll(s);
            currentIn=0;
        }
        else
            currentIn++;
    }
#endif
}


void rConsole::DisplayAtNewline(){
}

