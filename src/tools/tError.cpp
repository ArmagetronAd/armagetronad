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
#include <iostream>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tError.h"
#include "tString.h"

#ifdef DEBUG
tLevel st_debugLevel[2]={static_cast<tLevel>(DEBUG),static_cast<tLevel>(DEBUG)};

int st_debugValid(tLevel l,tChannel c) {

    return st_debugLevel[c]>=static_cast<int>(l) ;}
#endif

void st_Breakpoint(){
#ifdef DEBUG
    std::cout << "Breakpoint!\n";
    int i=0; // a chance to get errors in GDB
    i++;
#endif
}

#ifndef WIN32

void st_PresentError( const char* caption, const char *message )
{
    std::cerr << caption << ": " << message << "\n";
    st_Breakpoint();
    static bool error = true; // to disable the error if it is inconvenient right now and you think it may not be fatal
    if ( error )
    {
        throw 1; //tSimpleException( message, caption );
        exit(-1);
    }
}

void st_PresentMessage( const char* caption, const char *message )
{
    std::cerr << caption << ": " << message << "\n";
    st_Breakpoint();
}

#else

#include <windows.h>

void st_PresentError( const char* caption, const char *message )
{
    std::cerr << message;

#ifdef DEBUG	
    int result = MessageBox (NULL, message , caption, MB_RETRYCANCEL);
#else
int result = MessageBox (NULL, message , caption, MB_OK);
#endif
    switch ( result )
    {
    case IDRETRY:
    case IDYES:
        st_Breakpoint();
        break;
    case IDABORT:
    case IDOK:
    case IDCANCEL:
        throw 1;
        // exit(-1);
        break;
    }
}

void st_PresentMessage( const char* caption, const char *message )
{
    std::cerr << message;
    MessageBox (NULL, message , caption, MB_OK);
}

#endif // WINDOWS

char ERROR_MESSAGE[1000];

