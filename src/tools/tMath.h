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

#ifndef ArmageTron_MATH_H
#define ArmageTron_MATH_H

//includes math headers
#include <math.h>

#ifdef WIN32
#include <float.h>
#define finite _finite
#define copysign _copysign
#endif

#ifdef SOLARIS
#include <ieeefp.h>
#endif

inline bool good( REAL f )
{
    return finite( f );
}

static inline bool clamp(REAL &c, REAL min, REAL max){
    tASSERT(min <= max);

    if (!finite(c))
    {
        c = 0;
        return true;
    }

    if (c<min)
    {
        c = min;
        return true;
    }

    if (c>max)
    {
        c = max;
        return true;
    }

    return false;
}

#endif
