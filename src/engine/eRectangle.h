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

#ifndef ArmageTron_eRECTANGLE_H
#define ArmageTron_eRECTANGLE_H

#include "eCoord.h"

//! the rectangle containing the arena
class eRectangle
{
public:
    eRectangle();                                           //!< constructor to empty rectangle
    eRectangle( eCoord const & low, eCoord const & high );  //!< constructor

    void Clear();                                  //!< resets to empty state
    eRectangle & Include( eCoord const & point );  //!< include the given point into the rectangle
    bool Contains( eCoord const & point ) const;   //!< checks whether the given point lies inside the rectangle
    REAL Clamp( eCoord & point ) const;            //!< clamps the point to lie inside the rectangle
    REAL Clip( eCoord const & start, eCoord & stop ) const; //!< clips stop to lie inside the rectangle without changing the direction start->stop
    eCoord GetPoint( eCoord const & in ) const;    //!< returns a point in the interior

private:
    eCoord low_;  //!< low corner
    eCoord high_; //!< high corner

    // accessors
public:
    inline eCoord const & GetLow( void ) const;	                //!< Gets low corner
    inline eRectangle const & GetLow( eCoord & low ) const;	    //!< Gets low corner
    inline eCoord const & GetHigh( void ) const;	            //!< Gets high corner
    inline eRectangle const & GetHigh( eCoord & high ) const;	//!< Gets high corner
protected:
private:
    inline eRectangle & SetLow( eCoord const & low );	        //!< Sets low corner
    inline eRectangle & SetHigh( eCoord const & high );	        //!< Sets high corner
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

eCoord const & eRectangle::GetLow( void ) const
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

eRectangle const & eRectangle::GetLow( eCoord & low ) const
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

eRectangle & eRectangle::SetLow( eCoord const & low )
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

eCoord const & eRectangle::GetHigh( void ) const
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

eRectangle const & eRectangle::GetHigh( eCoord & high ) const
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

eRectangle & eRectangle::SetHigh( eCoord const & high )
{
    this->high_ = high;
    return *this;
}

#endif
