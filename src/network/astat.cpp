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

// ASTAT utility: command line interface to server browser

#include "nServerInfo.h"
#include "nNetwork.h"
#include "tConsole.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "nSocket.h"
#include "tCommandLine.h"

#include <iostream>

// hide status messages
class nCon:public tConsole
{
public:
    virtual tConsole &Print(const tString &s){return *this;}
    nCon(){RegisterBetterConsole(this);}
};

static nCon ncon;




void Poll()
{
    // poll the servers
    nServerInfo::StartQueryAll();
    while (nServerInfo::DoQueryAll(10)) usleep(1000);
    sn_SetNetState(nSTANDALONE);

    int servers = 0, users = 0;

    // print the list
    nServerInfo* run = nServerInfo::GetFirstServer();
    while (run)
    {

        std::cout << run->GetConnectionName() << "\t" << run->GetPort() << "\t"
        << run->Users() << "\t" << run->Ping() << "\n";

        servers += run->Reachable() ? 1 : 0;
        users += run->Users();

        run = run->Next();
    }

    std::cout << "\nservers: " << servers
    << "\nusers  : " << users << "\n";

    nServerInfo::DeleteAll();

}

int main(int argc, char **argv)
{
    // get the list
    //  nServerInfo::GetFromLAN();          // or rather from the local net
    //  std::cout << "LAN:\n";
    //  Poll();

    tCommandLineData commandLine;
    commandLine.programVersion_  = &sn_programVersion;
    commandLine.Analyse(argc, argv);
    tLocale::Load("languages.txt");

    nServerInfo::GetFromMaster();   // from the master server
    //  std::cout << "MASTER:\n";
    Poll();

    tLocale::Clear();
    sn_BasicNetworkSystem.Shutdown();
}
