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

#include "tStatFile.h"
#include "tDirectories.h"

tStatFile::tStatFile(tString name, tList<tStatEntry> *inList)
{
    fileName = name << ".txt";
    betweenWrites = 0;

    theList = inList;
}

void tStatFile::read()
{
    if (!myFileD.is_open())
    {
        open();
    }

    while (myFileD.good() && !myFileD.eof())
    {
        //		std::cout << "file " << fileName << ": reading line\n";
        REAL t1;
        tString t2;

        myFileD >> t1;
        t2.ReadLine(myFileD);

        //		std::cout << "file " << fileName << ": " << t1 << "-" << t2 << ".\n";

        if (t2 != "")
        {
            tStatEntry *newone = new tStatEntry(t2, t1);
            theList->Add(newone, newone->id);
            //			delete newone; should this be here?
        }
    }
}

void tStatFile::write()
{
    betweenWrites++;

    if (betweenWrites == 20)
    {
        flushWrites();
        betweenWrites = 0;
    }
}

void tStatFile::open()
{
    bool open = tDirectories::Var().Open(myFileD, fileName);
    if (!open)
    {
        std::ofstream t;
        tDirectories::Var().Open(t, fileName);
        t << std::flush;
        t.close();
        tDirectories::Var().Open(myFileD, fileName);
    }
}

void tStatFile::flushWrites()
{
    for (int c = theList->Len() - 1; c >= 0; c--)
    {
        if ((*theList)[c]->toWrite == true)
        {
            //			std::cout << "file " << fileName << ": un-buffering " << (*theList)[c]->getName() << " (" << c << ")" << std::endl;

            myFileD.clear();
            myFileD.seekg(0, std::ios::beg);
            //read thru each line in while loop !eof until the line matching the name is found or the end of file is hit
            //update the line or add the new one at the end. sorting can wait
            //fixed spacing for values important otherwise writing in place... tricky

            tString out;
            out << (*theList)[c]->getValue();
            out.SetPos(9, true);
            out << " " << (*theList)[c]->getName() << "\n";

            int found = 0;
            REAL t1;
            tString t2;
            while (!myFileD.eof())
            {
                myFileD >> t1;
                t2.ReadLine(myFileD);
                if (t2 == (*theList)[c]->getName())
                {
                    found = 1;
                    int pos = myFileD.tellg();
                    pos = pos - t2.Len() - 9;
                    myFileD.clear(); //in case last line?
                    myFileD.seekg(pos, std::ios::beg);
                    myFileD << out;
                }
            }

            if (found == 0)
            {
                myFileD.clear();
                myFileD << out;
            }
            (*theList)[c]->toWrite = false;
        }
    }

    myFileD.flush();
}

void tStatFile::close()
{
    if (myFileD.is_open())
    {
        myFileD.close();
    }
}

tStatFile::~tStatFile()
{
    //	std::cout << fileName << ": deconstruction!\n";
    //necessary!
    flushWrites();
    close();
}
