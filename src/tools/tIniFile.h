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

#ifndef TINIFILE_H
#define TINIFILE_H

#include "config.h"

#include <map>
#include <deque>
// Needed for sprintf, below
#include <stdio.h>

#include "tString.h"

// This hairy thing just means we use an associative array to reach each group,
// Followed by an associative array that is the content of each group
// For lines that don't have a key=value pair, we'll generate our own keys and store
// the line as the value
typedef std::map<tString, tString> IniValue;
typedef std::map<tString, IniValue > tIniGroups;


class tIniFile {
public:
    tIniFile();
    ~tIniFile();
    tIniFile(const char* filename);

    void LoadFile(const char* filename);

    bool HasGroup(const char* group);
    bool HasKey(const char* group, const char* key);
    IniValue GetGroup(const char* group);

    tString GetValue(const char* group, const char* key);
    std::deque<tString> GetGroups();
    std::deque<tString> GetKeys(const char* group);

    // Debugging method
    void Dump();
private:
    tIniGroups m_Groups;
};

#endif
