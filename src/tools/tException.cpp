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

#include "tException.h"

// *******************************************************************************************
// *
// *    GetName
// *
// *******************************************************************************************
//!
//!     @return    the name of the exception
//!
// *******************************************************************************************

tString tException::GetName( void ) const
{
    return DoGetName();
}

// *******************************************************************************************
// *
// *    GetDescription
// *
// *******************************************************************************************
//!
//!     @return    detailed description of the exception
//!
// *******************************************************************************************

tString tException::GetDescription( void ) const
{
    return DoGetDescription();
}

// *******************************************************************************************
// *
// *   ~tException
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tException::~tException( void )
{
}

// *******************************************************************************************
// *
// *   DoGetName
// *
// *******************************************************************************************
//!
//!     @return    the name of the exception
//!
// *******************************************************************************************

tString tException::DoGetName( void ) const
{
    return tString("tException");
}

// *******************************************************************************************
// *
// *  DoGetDescription
// *
// *******************************************************************************************
//!
//!        @return    detailed description of the exception
//!
// *******************************************************************************************

tString tException::DoGetDescription( void ) const
{
    // fallback
    return DoGetName();
}

// *******************************************************************************************
// *
// *   ~tSimpleException
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tGenericException::~tGenericException( void )
{
}

// *******************************************************************************************
// *
// *   tGenericException
// *
// *******************************************************************************************
//!
//!        @param  description error description
//!     @param  name        error name
//!
// *******************************************************************************************

tGenericException::tGenericException( char const * description, char const * name )
        : description_(description), name_( name ? name : "Error" )
{
}

// *******************************************************************************************
// *
// *   DoGetName
// *
// *******************************************************************************************
//!
//!        @return     the name
//!
// *******************************************************************************************

tString tGenericException::DoGetName( void ) const
{
    return name_;
}

// *******************************************************************************************
// *
// *   DoGetDescription
// *
// *******************************************************************************************
//!
//!        @return     the description
//!
// *******************************************************************************************

tString tGenericException::DoGetDescription( void ) const
{
    return description_;
}

// *******************************************************************************************
// *
// *   DoGetDescription
// *
// *******************************************************************************************
//!
//!        @return     the description
//!
// *******************************************************************************************

tString tCleanQuit::DoGetName() const
{
    return tString("");
}
