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
file by guru3/tank program

*/

//#include "tString.h"
#include "tStatEntry.h"
#include "tList.h"

#include <iostream>
#include <fstream>

class tStatFile {
public:
    tStatFile(tString name, tList<tStatEntry> *inList);
    void read();
    void write();
    ~tStatFile();

private:
    void open();
    void flushWrites();
    void close();

    tList<tStatEntry> *theList;
    tString fileName;
    int betweenWrites;

    tList<tStatEntry> buffer;

    //file descriptor
    std::fstream myFileD;
};
