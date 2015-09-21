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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_tRECTANGLE_H
#define ArmageTron_tRECTANGLE_H

#include "tCoord.h"

//! the rectangle containing the arena
class tRectangle
{
public:
    tRectangle();                                           //!< constructor to empty rectangle
    tRectangle( tCoord const & low, tCoord const & high );  //!< constructor

    void Clear();                                  //!< resets to empty state
    tRectangle & Include( tCoord const & point );  //!< include the given point into the rectangle
    bool Contains( tCoord const & point ) const;   //!< checks whether the given point lies inside the rectangle
    REAL Clamp( tCoord & point ) const;            //!< clamps the point to lie inside the rectangle
    REAL Clip( tCoord const & start, tCoord & stop ) const; //!< clips stop to lie inside the rectangle without changing the direction start->stop
    tCoord GetPoint( tCoord const & in ) const;    //!< returns a point in the interior

private:
    tCoord low_;  //!< low corner
    tCoord high_; //!< high corner

    // accessors
public:
    inline tCoord const & GetLow( void ) const;	                //!< Gets low corner
    inline tRectangle const & GetLow( tCoord & low ) const;	    //!< Gets low corner
    inline tCoord const & GetHigh( void ) const;	            //!< Gets high corner
    inline tRectangle const & GetHigh( tCoord & high ) const;	//!< Gets high corner
protected:
private:
    inline tRectangle & SetLow( tCoord const & low );	        //!< Sets low corner
    inline tRectangle & SetHigh( tCoord const & high );	        //!< Sets high corner
};

// ****************************************************************************
// *
// *	GetLow
// *
// ****************************************************************************
//!
//!		@return		low corner
//!
// ****************************************************************************

tCoord const & tRectangle::GetLow( void ) const
{
    return this->low_;
}

// ****************************************************************************
// *
// *	GetLow
// *
// ****************************************************************************
//!
//!		@param	low	low corner to fill
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

tRectangle const & tRectangle::GetLow( tCoord & low ) const
{
    low = this->low_;
    return *this;
}

// ****************************************************************************
// *
// *	SetLow
// *
// ****************************************************************************
//!
//!		@param	low	low corner to set
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

tRectangle & tRectangle::SetLow( tCoord const & low )
{
    this->low_ = low;
    return *this;
}

// ****************************************************************************
// *
// *	GetHigh
// *
// ****************************************************************************
//!
//!		@return		high corner
//!
// ****************************************************************************

tCoord const & tRectangle::GetHigh( void ) const
{
    return this->high_;
}

// ****************************************************************************
// *
// *	GetHigh
// *
// ****************************************************************************
//!
//!		@param	high	high corner to fill
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

tRectangle const & tRectangle::GetHigh( tCoord & high ) const
{
    high = this->high_;
    return *this;
}

// ****************************************************************************
// *
// *	SetHigh
// *
// ****************************************************************************
//!
//!		@param	high	high corner to set
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

tRectangle & tRectangle::SetHigh( tCoord const & high )
{
    this->high_ = high;
    return *this;
}

#endif
