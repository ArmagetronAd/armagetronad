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

Although it is already included in the GPL, let me clarify:
anybody, especially the Krawall Gaming network, is allowed
to run a server/master server build from modified versions of
this file WHITHOUT releasing the source to the public (provided
the executable is not distributed).


*/

#include "nKrawall.h"
#include "tString.h"
#include "tConsole.h"
#include "tArray.h"

#include <string>


#ifdef KRAWALL_SERVER

// TODO: REALLY change this!!
static nKrawall::nScrambledPassword key =
    {
        13, 12, 12, 12, 12, 12, 12, 12
    };


// secret key to encrypt server->master server league transfer
const nKrawall::nScrambledPassword& nKrawall::SecretLeagueKey()
{
    return key;
}

// fetch the scrambled password of username from the users database
void nKrawall::GetScrambledPassword(const tString& username,
                                    nScrambledPassword &scrambled)
{
    // TODO: REAL password retrieval
    const char *password = username;

    // this part may stay
    ScramblePassword(tString(password), scrambled);
}


// called ON THE MASTER when victim drives against killer's wall
void nKrawall::MasterFrag(const tString &killer, const tString& victim)
{
    con << killer << " killed " << victim << "\n";
    // TODO: REAL league management
}


// called ON THE MASTER at the end of a round; the last survivor is stored in
// players[numPlayers-1], the first death in players[0]
void nKrawall::MasterRoundEnd(const tString* players, int numPlayers)
{
    if (numPlayers > 1)
    {
        con << players[numPlayers-1] << " survived over ";
        for (int i = numPlayers-2; i>=0; i--)
        {
            con << players[i];
            if (i > 0)
                con << " and ";
        }
        con << ".\n";
    }
    // TODO: REAL league management
}



// first validity check for the league messages
bool nKrawall::IsFromKrawall(tString& adress, unsigned int port)
{
    return (adress.Len() > 3 &&
            !strncmp(adress, "127.0.0", 7));
}

// check if a user is from germany (so the master server will require
// a password check)
bool nKrawall::RequireMasterLogin(tString& adress, unsigned int port)
{
    return (adress.Len() > 3 &&
            !strncmp(adress, "127.0.0", 7));
}

#endif
