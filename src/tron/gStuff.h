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

#ifndef ArmageTron_STUFF_H
#define ArmageTron_STUFF_H


#include "rSDL.h"

#ifdef _MSC_VER
// disable nasty conversion complains of MSVC++
#pragma warning ( disable : 4800 4081 4244 4305 4244)
#endif

#include "tMemManager.h"
#include "defs.h"
#include "tSafePTR.h"

// #define EPS     1E-16

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159
#endif


extern bool sr_glOut; // do we have gl-output?

extern bool su_mouseGrab; // grab the mouse in windowed mode

extern bool sr_ZTrick;     // Quake-Style z-buffer trick: do
// not delete the screen, just pait the background with depth test
// disabled. Gives 20% speedup.

//extern bool sr_textOut; // display game text graphically?

extern bool sg_moviepackInstalled; // do we have the mp on disk?
extern bool sg_moviepackUse;       // do we use it?

bool sg_MoviePack();

#ifdef POWERPAK_DEB
extern bool pp_out; // or 2d-output?
extern bool pp_tess_deb;
#endif

//! opens an URI in the default web browser
void sg_OpenURI( char const * uri );

// we are going to define all these classes and want to be free
// to declare pointers to them anytime:

class ePoint;
class eHalfEdge;
class eFace;
class ePlayer;
class gCycle;
class eWall;
class eGameObject;
class eCamera;

#endif
