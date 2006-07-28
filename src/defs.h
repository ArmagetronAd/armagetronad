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

#ifndef ARMAGETRON_DEFS_H
#define ARMAGETRON_DEFS_H

#ifdef _MSC_VER
// disable nasty conversion complains of MSVC++
#pragma warning ( disable : 4244 4305 4800 4250)
//#pragma warning ( disable : 4800 4081 4244 4305 4244 4250 4541)
#endif

#include "config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <math.h>
#include <iosfwd>

// maximum number of supported viewports
#ifndef MAX_VIEWERS
#define MAX_VIEWERS 4
#endif

#ifndef M_PI
#define M_PI 3.14159f
#endif

typedef float REAL;
const REAL EPS=REAL(1E-7);

template<class T> void Swap(T &a,T &b){
    T c=a;
    a=b;
    b=c;
}
typedef void VOIDFUNC();
typedef void INTFUNC(int);
typedef bool BOOLRETFUNC();

typedef VOIDFUNC *FUNCPTR;
typedef INTFUNC *INTFUNCPTR;

// replacements for float math functions
#ifndef HAVE_SINF
inline REAL sinf( REAL angle ) throw() { return REAL(sin( angle )); }
#endif

#ifndef HAVE_COSF
inline REAL cosf( REAL angle ) throw() { return REAL(cos( angle )); }
#endif

#ifndef HAVE_TANF
inline REAL tanf( REAL angle ) throw() { return REAL(tan( angle )); }
#endif

#ifndef HAVE_ATAN2F
inline REAL atan2f( REAL y, REAL x ) throw() { return REAL(atan2( y, x )); }
#endif

#ifndef HAVE_SQRTF
inline REAL sqrtf( REAL x ) throw() { return REAL(sqrt( x )); }
#endif

#ifndef HAVE_SQRTF
inline REAL logf( REAL x ) throw() { return REAL(log( x )); }
#endif

#ifndef HAVE_SQRTF
inline REAL expf( REAL x ) throw() { return REAL(exp( x )); }
#endif

#ifndef HAVE_FABSF
inline REAL fabsf( REAL x ) throw() { return REAL(fabs( x )); }
#endif

#ifndef HAVE_FLOORF
inline REAL floorf( REAL x ) throw() { return REAL(floor( x )); }
#endif

#ifdef WIN32
// this function does not seem to exist in windows...
inline bool isblank(int x){ return ((x == ' ') || (x == '\t')); }
#endif

#ifdef _MSC_VER

#include <iostream>
//#include <iostream>
//#include <strstrea>
#else
#include <iostream>
//#include <>
#endif

#ifndef HAVE_POW10
inline double pow10(double y) throw() { return pow(10.0, y); }
#endif

#ifndef HAVE_POW10F
inline float pow10f(float y) throw() { return powf(10.0, y); }
#endif

#endif
