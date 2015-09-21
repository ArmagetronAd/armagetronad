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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#include "tIniFile.h"
#include <iostream>
#include <fstream>

tIniFile::tIniFile(const char* filename) {
    LoadFile(filename);
}

tIniFile::~tIniFile() {
    // do nothing destructor
}

std::deque<tString> tIniFile::GetGroups() {
    tIniGroups::iterator aGroup;
    std::deque<tString> retList;

    for( aGroup = m_Groups.begin(); aGroup != m_Groups.end(); aGroup++) {
        retList.push_back( tString((*aGroup).first) );
    }

    return retList;
}

bool tIniFile::HasGroup(const char* group) {
    tIniGroups::iterator aGroup = m_Groups.find( tString(group) );

    if(aGroup != m_Groups.end())
        return true;
    return false;
}

bool tIniFile::HasKey(const char* group, const char* key) {
    tIniGroups::iterator aGroup = m_Groups.find( tString(group) );

    if(aGroup != m_Groups.end()) {
        IniValue::iterator aKey = (*aGroup).second.find( tString(key) );
        if( aKey != (*aGroup).second.end() ) {
            return true;
        }
    }
    return false;
}

IniValue tIniFile::GetGroup(const char* group) {
    if( HasGroup(group) ) {
        tIniGroups::iterator aGroup = m_Groups.find( tString(group) );
        return (*aGroup).second;
    }
    return IniValue();
}

tString tIniFile::GetValue(const char* group, const char* key) {
    //std::cout << "Getting value " << key << " from group " << group << "\n";
    if( HasKey(group, key) ) {
        //std::cout << "Found " << m_Groups[tString(group)][tString(key)] << "\n";
        return m_Groups[tString(group)][tString(key)];
    }
    //std::cout << "Not found!\n";
    return tString();
}

void tIniFile::Dump() {
    tIniGroups::iterator aGroup;

    std::cout << "Number of groups: " << m_Groups.size() << "\n";
    for(aGroup = m_Groups.begin(); aGroup != m_Groups.end(); aGroup++) {
        std::cout << "Key: " << (*aGroup).first << "\n";
    }
}

void tIniFile::LoadFile(const char* filename) {
    std::ifstream thefile(filename);
    tString currentGroup;
    int autokey = 0;
    tString key;
    tString value;
    if(thefile.good() ) {
        //std::cout << "Opened ini file\n";
    } else {
        //std::cout << "Couldn't open ini file: " << filename << "\n";
        return;
    }
    while( ! thefile.eof() && thefile.good() ) {
        tString oneLine;

        oneLine.ReadLine( thefile );

        oneLine = oneLine.Trim();
        // Ignore comments, comments can only be on lines by themselves
        if( !oneLine.StartsWith("#") ) {
            // First trim whitespace and see how big the line is
            tString oneLine1(oneLine.StripWhitespace()); //.StripWhitespace());

            if(oneLine1.Len() < 2) continue;

            // Now see if it's a group.  We use the stripped version for this
            if(oneLine1.StartsWith("[") && oneLine1.EndsWith("]") ) {
                currentGroup = oneLine1.SubStr(1, oneLine.StrPos("]")-1);
                autokey = 0;
                continue;
            }
            // At this point, if we have no group, we ignore the whole line
            if(currentGroup.Len() < 1) continue;

            // Now see if it's a key=value pair.  We go back to the original line
            int equalPos = oneLine.StrPos("=");
            if(equalPos != -1) {
                key = oneLine.SubStr(0, equalPos);
                value = oneLine.SubStr(equalPos + 1);
                key = key.StripWhitespace();
                value = value.Trim();
            } else {
                // If it's not a group, we have a current group, and it's not key=value,
                // then we need to use our own generated keys and store the whole line
                char buf[30];
                sprintf(buf, "%d", autokey);
                key = buf;
                value = oneLine;
                autokey++;
            }
            // Store the key value in the current group
            //std::cout << "Group: " << currentGroup << ", " << key << "=" << value << "\n";
            m_Groups[currentGroup][tString(key)] = tString(value);
        }
    }
}

