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

#include <cmath>
#include <ctype.h>
#include <iosfwd>

// Includes required for GCC 4.3 only. Remove them as soon as some of 
// us developers have GCC 4.3, then it is of course better to only put them
// where they are needed.
#include <string.h>
#include <memory>
#include <typeinfo>
#include <cstdlib>
// end of GCC 4.3 includes

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

// these are in std:: since C++ 11, which we are using.
// Apparently, old ubuntus are not fully compliant there, so we still
// need the configure checks and guards here.

#ifndef HAVE_SINF
using std::sinf;
#endif

#ifndef HAVE_COSF
using std::cosf;
#endif

#ifndef HAVE_TANF
using std::tanf;
#endif

#ifndef HAVE_ATAN2F
using std::atan2f;
#endif

#ifndef HAVE_ATAN2F
using std::sqrtf;
#endif

#ifndef HAVE_LOGF
using std::logf;
#endif

#ifndef HAVE_EXPF
using std::expf;
#endif

#ifndef HAVE_FABSF
using std::fabsf;
#endif

#ifndef HAVE_FLOORF
using std::floorf;
#endif

// use this function to explicitly ignore return values
template< typename T >
static void Ignore( T )
{}

#ifdef _MSC_VER

#include <iostream>
//#include <iostream>
//#include <strstrea>
#else
#include <iostream>
//#include <>
#endif

#endif
