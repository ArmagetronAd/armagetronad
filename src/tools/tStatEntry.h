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
file by guru3/tank program
formerly gStatEntry.h

*/

#ifndef ArmageTron_TSTATENTRY_H
#define ArmageTron_TSTATENTRY_H

#include "defs.h"
#include "tString.h"

class tStatEntry {
public:
    tStatEntry(tString name);
    tStatEntry(tString name, REAL value);

    tString getName();
    REAL getValue();

    void setValue(REAL value);
    void addValue(REAL value);

    ~tStatEntry();

    int id;

    bool toWrite;
private:
    tString myName;
    REAL myValue;
};

#endif
