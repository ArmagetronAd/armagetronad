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

#ifndef ArmageTron_tCOLOR_H
#define ArmageTron_tCOLOR_H

#include "tString.h"
#include "defs.h"

//! rgba color represented by floats between 0 and 1
class tColor
{
public:
    tColor();       //!< Constructor
    tColor( REAL r, REAL g, REAL b, REAL a = 1 );     //!< Constructor
    tColor( const char * c );		//!< Creates a tColor from a color code string
    tColor( const wchar_t * c );		//!< Creates a tColor from a color code string
//    tColor( const tString * c );		//!< Creates a tColor from a color code string
    ~tColor(){}                         //!< Destructor

    void FillFrom( const char * c );		//!< Fills this color object from a color code string
    void FillFrom( const wchar_t * c );		//!< Fills this color object from a color code string

    // the colors are public because they are independent of each other
    REAL r_, g_, b_, a_;                    //!< Color values

    bool IsDark( void );	//!< Has this color to be rendered on a bright background ?
protected:
private:
};

inline std::ostream &operator<< (std::ostream &s,const tColor &c){
    s << "color(" << c.r_ << "," << c.g_ << "," << c.b_ << "," << c.a_ << ")";
    return s;
}

inline bool operator==(const tColor &lColor, const tColor &rColor) {
    return ( (lColor.r_ == rColor.r_) && (lColor.g_ == rColor.g_) && (lColor.b_ == rColor.b_) && (lColor.a_ == rColor.a_));
}

inline bool operator!=(const tColor &lColor, const tColor &rColor) {
    return !operator==(lColor, rColor);
}

#endif

