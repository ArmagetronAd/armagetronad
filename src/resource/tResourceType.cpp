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

#include "tString.h"

#include "tResourceType.h"


/****************************************************************
 *       tResourceType                                          *
 ***************************************************************/

/** This constructor is the one you should always use.  Make sure
 *  you create a function with the signature for tNewResourceType
 *  that returns a newly created instance of your tResource subclass
 *  that has been typecasted to tResource.  The Resource Manager will
 *  use the function you provide to create new instances of your
 *  resource type
 *
 *  \param name the name of the resource type
 *  \param description a human readable description of the resource type
 *  \param extension the filename extension that will be found on disk
 *  \param creator a function that creates a new instance of the resource
 */
tResourceType::tResourceType(const char* name, const char* description, const char* extension,
                  tNewResourceType creator) {
    m_Name = tString(name);
}




