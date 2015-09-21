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

#ifndef ArmageTron_tEXCEPTION_H
#define ArmageTron_tEXCEPTION_H

#include "tString.h"

//! Base class for all exceptions
class tException
{
public:
    tString GetName()           const;                      //!< returns the name of the exception
    tString GetDescription()    const;                      //!< returns a detailed description

protected:
    virtual ~tException();                                  //!< destructor

private:
    virtual tString DoGetName()         const = 0;          //!< returns the name of the exception
    virtual tString DoGetDescription()  const;              //!< returns a detailed description
};

//! Simple, generic exception class for locally fatal errors (that abort a function,
//  but not the program)
class tGenericException: public tException
{
public:
    virtual ~tGenericException();                                             //!< destructor
    tGenericException( char const * description, char const * name = NULL );  //!< constructor from description and name
private:
    virtual tString DoGetName()         const;              //!< returns the name of the exception
    virtual tString DoGetDescription()  const;              //!< returns a detailed description

    tString description_;   //!< exception description
    tString name_;          //!< exception name
};

#endif
