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

#ifndef ArmageTron_tFunction_H
#define ArmageTron_tFunction_H

#include "defs.h"
#include "tError.h"

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
class tFunction
{
public:
    tFunction();  //!< constructor
    tFunction(REAL offset, REAL slope);  //!< constructor
    tFunction(std::string const & parsed, REAL const * sizeMult);  //!< string parsing constructor
    ~tFunction(); //!< destructor

    REAL Evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator

    // function parameters: currently limited to offset_ + slope_ * argument
    REAL offset_; //!< offset value
    REAL slope_;  //!< function slope

public:
    inline tFunction & SetOffset( REAL const & offset );	//!< Sets offset value
    inline REAL const & GetOffset( void ) const;	//!< Gets offset value
    inline tFunction const & GetOffset( REAL & offset ) const;	//!< Gets offset value
    inline tFunction & SetSlope( REAL const & slope );	//!< Sets function slope
    inline REAL const & GetSlope( void ) const;	//!< Gets function slope
    inline tFunction const & GetSlope( REAL & slope ) const;	//!< Gets function slope
protected:
private:
};

//These templates are probably only useable with nMessage as parameter.
//The reason they're templates is that nMessage isn't available in src/tools
//and that tFunction might be needed to be in src/tools in the future.
//Imagine a constructor for vValue that converts from a tFunction as an
//example (or the other way, as far as possible). --wrtlprnft

template<typename T> T & operator << ( T & m, tFunction const & f ); //! function network message writing operator
template<typename T> T & operator >> ( T & m, tFunction & f );       //! function network message reading operator

// *******************************************************************************
// *
// *	operator ( )
// *
// *******************************************************************************
//!
//!		@return		the function value
//!
// *******************************************************************************

REAL tFunction::operator ( )( REAL argument ) const
{
    return Evaluate( argument );
}

// *******************************************************************************
// *
// *	GetOffset
// *
// *******************************************************************************
//!
//!		@return		offset value
//!
// *******************************************************************************

REAL const & tFunction::GetOffset( void ) const
{
    return this->offset_;
}

// *******************************************************************************
// *
// *	GetOffset
// *
// *******************************************************************************
//!
//!		@param	offset	offset value to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction const & tFunction::GetOffset( REAL & offset ) const
{
    offset = this->offset_;
    return *this;
}

// *******************************************************************************
// *
// *	SetOffset
// *
// *******************************************************************************
//!
//!		@param	offset	offset value to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction & tFunction::SetOffset( REAL const & offset )
{
    this->offset_ = offset;
    return *this;
}

// *******************************************************************************
// *
// *	GetSlope
// *
// *******************************************************************************
//!
//!		@return		function slope
//!
// *******************************************************************************

REAL const & tFunction::GetSlope( void ) const
{
    return this->slope_;
}

// *******************************************************************************
// *
// *	GetSlope
// *
// *******************************************************************************
//!
//!		@param	slope	function slope to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction const & tFunction::GetSlope( REAL & slope ) const
{
    slope = this->slope_;
    return *this;
}

// *******************************************************************************
// *
// *	SetSlope
// *
// *******************************************************************************
//!
//!		@param	slope	function slope to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction & tFunction::SetSlope( REAL const & slope )
{
    this->slope_ = slope;
    return *this;
}

// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	m	message to write to
//!		@param	f	function to write
//!		@return		reference to message for chaining
//!
// *******************************************************************************

template<typename T> T & operator << ( T & m, tFunction const & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID = 1;
    m.Write( ID );

    // write values
    m << f.GetOffset();
    m << f.GetSlope();

    return m;
}

// *******************************************************************************
// *
// *	operator >>
// *
// *******************************************************************************
//!
//!     @param  m   message to read from
//!     @param  f   function to read to
//!     @return     reference to message for chaining
//!
// *******************************************************************************

template<typename T> T & operator >> ( T & m, tFunction & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID;
    m.Read(ID);
    tASSERT( ID == 1 );

    // read values
    REAL slope, offset;
    m >> offset >> slope;

    // store values
    f.SetOffset( offset ).SetSlope( slope );

    return m;
}

#endif
