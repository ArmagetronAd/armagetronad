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

#include "defs.h"

//! rgba color represented by floats between 0 and 1
struct tColor
{
public:
    tColor():r_(1), g_(1), b_(1), a_(1) {}       //!< Constructor
    tColor( REAL r, REAL g, REAL b, REAL a = 1 )      //!< Constructor
            :r_(r), g_(g), b_(b), a_(a) {}
    ~tColor(){}                         //!< Destructor

    // the colors are public because they are independent of each other
    REAL r_, g_, b_, a_;                    //!< Color values
protected:
private:
};

inline std::ostream &operator<< (std::ostream &s,const tColor &c){
    s << "color(" << c.r_ << "," << c.g_ << "," << c.b_ << "," << c.a_ << ")";
    return s;
}

#endif

