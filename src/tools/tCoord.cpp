/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "tCoord.h"
#include "tCoord.pb.h"

void tCoord::WriteSync( Tools::Position       & position  ) const
{
    position.set_x( x );
    position.set_y( y );
}

void tCoord::WriteSync( Tools::Direction      & direction ) const
{
    direction.set_x( x );
    direction.set_y( y );
}

void tCoord::ReadSync( Tools::Position  const & position  )
{
    x = position.x();
    y = position.y();
}

void tCoord::ReadSync( Tools::Direction const & direction )
{
    x = direction.x();
    y = direction.y();
}

