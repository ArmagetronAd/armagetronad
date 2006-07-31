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
formerly gStatEntry.cpp

*/

#include "tStatEntry.h"

tStatEntry::tStatEntry(tString name)
{
    myName = name;
    myValue = 0;

    toWrite = false;
    id = -1;
}

tStatEntry::tStatEntry(tString name, REAL value)
{
    myName = name;
    myValue = value;

    toWrite = false;
    id = -1;
}

tString tStatEntry::getName()
{
    return myName;
}

REAL tStatEntry::getValue()
{
    return myValue;
}

void tStatEntry::setValue(REAL value)
{
    myValue = value;
}

void tStatEntry::addValue(REAL value)
{
    myValue += value;
}

tStatEntry::~tStatEntry()
{
}
