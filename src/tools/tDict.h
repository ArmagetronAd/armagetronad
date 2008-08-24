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


#ifndef TDICT_H
#define TDICT_H

#include <map>
#include <string>
#include <functional>

#include "tString.h"

class RuntimeStringCmp {
public:
    // the comparison
    bool operator() (const tString& s1, const tString& s2) const {
            return s1.Compare(s2);
    }
};

template <typename T, typename A, typename S=std::less<T> >
class tDict : public std::map<T, A, S> {
public:    
    inline bool HasKey(T x) {
        return (this->find(x)!=this->end() );
    }
};

template <typename R>
class tStringDict : public tDict<tString, R, RuntimeStringCmp> {
public:
    tStringDict() : std::map<tString, R, RuntimeStringCmp>(RuntimeStringCmp()) { };
};

#endif
