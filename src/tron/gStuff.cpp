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

#include "gStuff.h"
#include "eCoord.h"
#include "tConfiguration.h"
#include "tResourceManager.h"
#include "uInput.h"
#include "tInitExit.h"
#include "nConfig.h"

#include "rSDL.h"
#include "rScreen.h"
#ifndef DEDICATED
#include "rRender.h"

#endif

bool pp_out=1; // or 2d-output?
bool pp_tess_deb=0;

// network setting item for the reesource repository from tResourceManager.cpp
// may be dangerous, so it's disabled for now.
static nSettingItem<tString> conf_res_repo("RESOURCE_REPOSITORY_SERVER", tResourceManager::AccessRepoServer());

static tConfItem<bool> grab("MOUSE_GRAB",su_mouseGrab);
static tConfItem<bool> zt("ZTRICK",sr_ZTrick);

bool sg_moviepackInstalled=false; // do we have the mp on disk?
bool sg_moviepackUse=true;       // do we use it?
static tConfItem<bool> ump("MOVIEPACK",sg_moviepackUse);

// setting item for double bind (available since version 7)
static nSettingItemWatched<REAL> su_doubleBindTimeoutConf( "DOUBLEBIND_TIME", su_doubleBindTimeout, nConfItemVersionWatcher::Group_Cheating, 7  );

bool sg_MoviePack(){
    return sg_moviepackInstalled && sg_moviepackUse;
}

//const eCoord se_zeroCoord(0,0);

