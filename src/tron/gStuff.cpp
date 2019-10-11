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

#include "gStuff.h"
#include "eCoord.h"
#include "tConfiguration.h"
#include "tResourceManager.h"
#include "uInput.h"
#include "tInitExit.h"
#include "nConfig.h"

#include <sstream>

#include "rSDL.h"
#include "rScreen.h"
#ifndef DEDICATED
#include "rRender.h"
#ifdef MACOSX
#   include <ApplicationServices/ApplicationServices.h>
#endif
#endif

bool pp_out=1; // or 2d-output?
bool pp_tess_deb=0;

// network setting item for the reesource repository from tResourceManager.cpp
// may be dangerous, so it's disabled for now.
static nSettingItem<tString> conf_res_repo("RESOURCE_REPOSITORY_SERVER", tResourceManager::AccessRepoServer());

static tConfItem<bool> grab("MOUSE_GRAB",su_mouseGrab);

bool sg_moviepackInstalled=false; // do we have the mp on disk?
bool sg_moviepackUse=true;       // do we use it?
static tConfItem<bool> ump("MOVIEPACK",sg_moviepackUse);

// setting item for double bind (available since version 7)
static nSettingItemWatched<REAL> su_doubleBindTimeoutConf( "DOUBLEBIND_TIME", su_doubleBindTimeout, nConfItemVersionWatcher::Group_Cheating, 7  );

bool sg_MoviePack(){
    return sg_moviepackInstalled && sg_moviepackUse;
}

#ifndef MACOSX
static bool sg_OpenStuff( char const * uri, bool tryBrowser )
{
#ifndef DEDICATED
    if( currentScreensetting.fullscreen )
    {
        // iconify; otherwise, the screen freezes while the browser is started,
        // and on Linux, the game gets window-ified without being noticed about it.
#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_MinimizeWindow(sr_screen);
#else
        SDL_WM_IconifyWindow();
#endif
    }
#endif

    // bool success = false;
#ifdef WIN32
    ShellExecute(NULL, "open", uri, NULL, NULL, SW_SHOWNORMAL);
#else
    // general unix
    std::ostringstream s; // composing a command
    if( tryBrowser )
    {
        s << "x-www-browser '" << uri << "' || ";
    }
    s << "xdg-open '" << uri << "' || " << "firefox '" << uri << "' &";
    // execute command
    return  0 == system( s.str().c_str() );
#endif
    return true;
}
#endif

bool sg_OpenURI( char const * uri )
{
#ifdef MACOSX
#ifndef DEDICATED
    CFURLRef URL = CFURLCreateWithBytes( NULL, (UInt8 *)uri, strlen(uri), kCFStringEncodingUTF8, NULL );
    LSOpenCFURLRef( URL, NULL );
    CFRelease( URL );
#endif
	return true;
#else
    return sg_OpenStuff( uri, true );
#endif
}

bool sg_OpenDirectory( char const * path )
{
#ifdef MACOSX
#ifndef DEDICATED
    FSRef ref;
    CFURLRef url = CFURLCreateFromFileSystemRepresentation( NULL, (UInt8 *)path, strlen( path ), true );
    // LSOpenCFURLRef() doesn't seem to traverse past "..", so we open a FSRef instead.
    CFURLGetFSRef( url, &ref );
    CFRelease( url );
    LSOpenFSRef( &ref, NULL );
#endif
	return true;
#else
    return sg_OpenStuff( path, false );
#endif
}

//const eCoord se_zeroCoord(0,0);

