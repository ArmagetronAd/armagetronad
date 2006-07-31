/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by Manuel Moos (z-man@users.sf.net)
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

#include "eRectangle.h"
#include "tError.h"

// ****************************************************************************
// *
// *   eRectangle
// *
// ****************************************************************************
//!
//!
// ****************************************************************************

eRectangle::eRectangle( void )
        : low_(1E+30, 1E+30), high_(-1E+30, -1E+30)
{
}

// ****************************************************************************
// *
// *   eRectangle
// *
// ****************************************************************************
//!
//!     @param  low   lowest part of the coordinates of the rectangle
//!     @param  high  highest part of the coordinates of the rectangle
//!
// ****************************************************************************

eRectangle::eRectangle( eCoord const & low, eCoord const & high )
        : low_(low), high_(high)
{
}

// ****************************************************************************
// *
// *   Clear
// *
// ****************************************************************************
//!
//!
// ****************************************************************************

void eRectangle::Clear( void )
{
    low_ = eCoord(1E+30, 1E+30);
    high_ = eCoord(-1E+30, -1E+30);
}

// ****************************************************************************
// *
// *   Include
// *
// ****************************************************************************
//!
//!        @param  point   the point to include
//!        @return reference to this for chaining
//!
// ****************************************************************************

eRectangle & eRectangle::Include( eCoord const & point )
{
    if (low_.x > point.x)
        low_.x = point.x;
    if (low_.y > point.y)
        low_.y = point.y;
    if (high_.x < point.x)
        high_.x = point.x;
    if (high_.y < point.y)
        high_.y = point.y;

    return *this;
}

// ****************************************************************************
// *
// *	Contains
// *
// ****************************************************************************
//!
//!		@param	point	the point to test
//!		@return true if the point lies inside
//!
// ****************************************************************************

bool eRectangle::Contains( eCoord const & point ) const
{
    return low_.x  <= point.x && low_.y  <= point.y &&
           high_.x >= point.x && high_.y >= point.y;
}

// clamp toClamp to reference in direction. Store the maximal move in max.
static inline void se_Clamp( REAL& toClamp, REAL reference, REAL direction, REAL& max )
{
    REAL move = (toClamp - reference)*direction;
    if (move > max)
        max = move;
    if (move > 0)
        toClamp = reference;
}

// ****************************************************************************
// *
// *	Clamp
// *
// ****************************************************************************
//!
//!		@param	point	the point to clamp
//!     @return distance the point needed to be moved
//!
// ****************************************************************************

REAL eRectangle::Clamp( eCoord & point ) const
{
    REAL ret = -1E+30;
    se_Clamp( point.x,  low_.x, -1, ret);
    se_Clamp( point.y,  low_.y, -1, ret);
    se_Clamp( point.x, high_.x,  1, ret);
    se_Clamp( point.y, high_.y,  1, ret);

    return ret;
}

// helper function clipping a line at one clipping line
static inline REAL se_Clip( eCoord const & start, eCoord & stop,
                            eCoord const & reference, eCoord const & normal )
{
    // calculate how far out on the other side of the reference point the stop point is
    REAL out = eCoord::F( normal, stop-reference );
    if ( out <= 0 )
        return 1; // nothing to clamp

    // calculate how far inside the startpoint lies
    REAL in = eCoord::F( normal, reference-start );
    if ( in < 0 )
        in = 0;

    // clip
    if ( out+in > 0 )
    {
        stop = (start * out + stop * in)*(1/(in+out));
        return in/(in+out);
    }

    return 1;
}

// ****************************************************************************
// *
// *	Clip
// *
// ****************************************************************************
//!
//!		@param	start	point (asserted to lie inside the rectangle) to serve as starting point
//!		@param	stop	point to clip
//!     @return         amount left of the start-stop line segment (1 for no clipping)
//!
// ****************************************************************************

REAL eRectangle::Clip( eCoord const & start, eCoord & stop ) const
{
    // clip in x-direction
    REAL rx = se_Clip( start, stop, low_, eCoord(-1,0) );
    if ( rx >= 1 )
    {
        rx = se_Clip( start, stop, high_, eCoord(1,0) );
    }

    // clip in y-direction
    REAL ry = se_Clip( start, stop, low_, eCoord(0,-1) );
    if ( ry >= 1 )
    {
        ry = se_Clip( start, stop, high_, eCoord(0,1) );
    }

    return rx * ry;
}

// ****************************************************************************
// *
// *	GetPoint
// *
// ****************************************************************************
//!
//!		@param	in	coordinates in the range of 0..1 of the point to get
//!		@return		point inside the rectangle
//!
// ****************************************************************************

eCoord eRectangle::GetPoint( eCoord const & in ) const
{
    eCoord diff = high_ - low_;
    return low_ + eCoord( diff.x * in.x, diff.y * in.y );
}

