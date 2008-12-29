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

#ifndef ArmageTron_TRESOURCE_H
#define ArmageTron_TRESOURCE_H

#include <boost/shared_ptr.hpp>

#include "tXmlParser.h"

#include "tResourceManager.h"

class tResource : public tXmlParser {
public:
    bool LoadFile(const char* filename, const char* uri="");

    //!< get the resource path this file was loaded from
    tResourcePath const &Path() const {return m_Path;}

    //! Convenient typedef for references to tResources
    typedef boost::shared_ptr<tResource> Reference;
protected:
    bool ValidateXml(FILE* docfd, const char* uri, const char* filepath);
    tResourcePath m_Path; //!< the resource identifier of this resource
    node GetFileContents(void); //!< Returns the node the "real" file contents are within
};

//! \deprecated
typedef tResource tXmlResource;

//! Map type for storing tResource subclasses
//typedef std::map< tString, tResource::Reference > tResourceMap;


#endif
