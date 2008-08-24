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

#ifndef ArmageTron_RESOURCETYPE_H
#define ArmageTron_RESOURCETYPE_H

#include <boost/smart_ptr.hpp>

#include "tString.h"

class tResource;
    
typedef tResource* (*tNewResourceType)(const char* path);


/** tResourceType describes a resource type to the resource system.
 *     Upon instantiation, it reports itself to tResourceManager so
 *     it can be used by whoever needs to use it.
 **/
class tResourceType {
public:
    /// This constructor is used by the resource to create new instances
    /// of the resource type.
    tResourceType(const char* name, const char* description, const char* extension,
                  tNewResourceType creator);

    /// Returns the name of the resource type
    const tString& GetName() { return m_Name; };
    
    /// Returns the description of the resource type
    const tString& GetDescription() { return m_Description; };
    
    /// Returns the file extension of the resource type
    const tString& GetExtension() { return m_Extension; };
    
    //! Convenient typedef for a type that stores references to the resource
    /// type.
    typedef boost::shared_ptr<tResourceType> Reference;

    Reference Get_reference() { return Reference(this); };
private:
    /// We make this constructor private so nobody will use it
    tResourceType();

    /// This is the heart of the class, the function pointer that points
    /// to a user-defined function that creates a new instance of a
    /// tResource subclass
    tNewResourceType m_Creator;

    /// The name of the resource type
    tString m_Name;
    /// A human readable description of the resource type
    tString m_Description;
    /// The file extension found on disk for the resource type
    tString m_Extension;
};

//! Map type for storing tResourceTypes
typedef std::map< tString, tResourceType::Reference > tResourceTypeMap;



#endif

