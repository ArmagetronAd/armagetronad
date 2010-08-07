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

#include "tResource.h"

bool tResource::LoadFile(const char* filename, const char* uri) {
    m_Filename = tResourceManager::locateResource(filename, uri);
    return LoadXmlFile(m_Filename, uri);
}

bool tResource::ValidateXml(FILE* docfd, const char* uri, const char* filepath) {
    bool validated = tXmlParser::ValidateXml(docfd, uri, filepath);

    /* check filepath */
    if ( validated && filepath )
    {
        node root = GetRoot();

        if (!root) {
            con << "Empty document\n";
            return false;
        } else if (root.IsOfType("Resource")) {
            m_Path = tResourcePath (
                root.GetProp("author"),
                root.GetProp("category"),
                root.GetProp("name"),
                root.GetProp("version"),
                root.GetProp("type"),
                tString("xml"),
                tString("")
            );
            tString rightFilepath( m_Path.Path() );
            tString pureFilepath( filepath );
            int pos;
            while((pos = pureFilepath.StrPos("//")) != -1) {
                pureFilepath.RemoveSubStr(pos, 1);
            }
            tResourcePath purepath(pureFilepath);
            if ( m_Path.Valid() && purepath != m_Path )
            {
                con << "\nWARNING: incorrect filepath. The resource wants to be at \"" << rightFilepath << "\", but was loaded from \"" << filepath << "\".\n\n";
            }
        }
        else
        if (root.IsOfType("World") && root.GetProp("version") == "0.1")
        {
            // NOTE: Legacy map resource
        }
        else {
            con << "Root node is not of type 'Resource' but '" << root.GetName() << "'.\n";
            return false;
        }
    }

    return validated;
}

tXmlParser::node tResource::GetFileContents(void) {
    for(node cur = GetRoot().GetFirstChild(); cur; ++cur) {
        if(!cur.IsOfType("comment") && !cur.IsOfType("text")) {
            return cur;
        }
    }
    return 0;
}




