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

*/

#include "gStatList.h"
//#include "gStatistics.h"
#include <iostream>

gStatList::gStatList(const char* name, int oType)
{
    myName = name;
    outputType = oType;

    if (outputType == 0)
    {
        myFile = new tStatFile(myName, &entries);
        myFile->read();
    }

    //	std::cout << "i am " << name << " and i have " << entries.Len() << " values\n";
    //	int c = getPlaceInList(tString("tank program"));
    //	std::cout << "i am " << name << " and i have " << entries.Len() << " values\n";
    //	add(tString("Tank Program"), 8);
    //	add(tString("guru3"), 2);
    //	add(tString("insaneo"), 5);
    //	add(tString("randman"), 1);
    //	add(tString("aguy"), 7);
    //	std::cout << "we have " << entries[c]->getName() << " with " << entries[c]->getValue() << "!\n";

    //	std::cout << entries.Len() << " entries\n";

    //	for (int c = 0; c < entries.Len(); c++)
    //	{
    //		std::cout << getName() << ": " << entries[c]->getName() << " has score " << entries[c]->getValue() << " (" << c << ")\n";
    //	}

    //	std::cout << "constructed!\n";
}

//adds value to the existing one
void gStatList::add(tString name, REAL value)
{
    int c = getPlaceInList(name);
    entries[c]->addValue(value);
    output(c);
}

//replaces the existing value with value
void gStatList::update(tString name, REAL value)
{
    int c = getPlaceInList(name);
    entries[c]->setValue(value);
    output(c);
}

//if the new value is greated, replace
void gStatList::greater(tString name, REAL value)
{
    int c = getPlaceInList(name);
    if (entries[c]->getValue() < value)
    {
        update(name, value);
    }
}

//who am i?
tString gStatList::getName()
{
    return myName;
}

//return the value for person of name
REAL gStatList::getValue(tString name)
{
    return entries[getPlaceInList(name)]->getValue();
}

int gStatList::getPlaceInList(tString name)
{
    //	std::cout << myName << ": getting place\n";

    for (int c = 0; c < entries.Len(); c++)
    {
        if (entries[c]->getName() == name)
        {
            return c;
        }
    }

    //	std::cout << myName << ": adding place\n";

    //didn't find it, add it
    tStatEntry *newone = new tStatEntry(name);
    entries.Add(newone, newone->id);
    return getPlaceInList(name);
}

void gStatList::output(int c)
{
    entries[c]->toWrite = true;

    if (outputType == 0)
    {
        myFile->write();
    }
}

gStatList::~gStatList()
{
    if (outputType == 0)
    {
        delete myFile;
    }

    for (int c = entries.Len()-1; c >= 0; c--)
    {
        tStatEntry * remove = entries[c];
        entries.Remove(remove, remove->id);
        delete remove;
    }
}
