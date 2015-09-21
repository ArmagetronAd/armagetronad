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

#ifndef ArmageTron_rCOLOR_H
#define ArmageTron_rCOLOR_H

#include "rRender.h"

#include "defs.h"
#include "tColor.h"

//! tColor extended by a function to apply the color directly
struct rColor: public tColor
{
public:
    rColor():tColor() {}       //!< Constructor
    rColor( REAL r, REAL g, REAL b, REAL a = 1 )      //!< Constructor
            :tColor(r,g,b,a) {}
    ~rColor(){}                         //!< Destructor
#ifndef DEDICATED
    //! Applies the color values stored using Color()
    void Apply(void) const
        {Color(r_,g_,b_,a_);}
#else
    void Apply(void) const {};
#endif
protected:
private:
};

#endif

