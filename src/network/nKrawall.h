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

#ifndef ArmageTron_NKRAWALL_H
#define ArmageTron_NKRAWALL_H

//#include <iosfwd>
//#include <iostream>

#include "defs.h"
#include "config.h"
#include "md5.h"

#ifndef _IOSFWD_
#endif

//#error x

class tString;
class nMessage;

class nKrawall
{
public:

    // the scrambled password data types
    typedef md5_byte_t nScrambledPassword[16]; // (freely changable)
    typedef nScrambledPassword nSalt;          // (freely changable)

    // network read/write operations of these data types
    static void WriteScrambledPassword(const nScrambledPassword& scrambled,
                                       nMessage &m);

    static void ReadScrambledPassword( nMessage &m,
                                       nScrambledPassword& scrambled);

    // file read/write operations of these data types
    static void WriteScrambledPassword(const nScrambledPassword& scrambled,
                                       std::ostream &s);

    static void ReadScrambledPassword( std::istream &s,
                                       nScrambledPassword& scrambled);


    static void WriteSalt(const nSalt& salt,
                          nMessage &m)
    {
        WriteScrambledPassword(salt, m);
    }

    static void ReadSalt( nMessage &m,
                          nSalt& salt)
    {
        ReadScrambledPassword(m, salt);
    }

    // compare two passwords
    static bool ArePasswordsEqual(const nScrambledPassword& a,
                                  const nScrambledPassword& b);


    // scramble a password locally (so it does not have to be stored on disk)
    static void ScramblePassword(const tString& username,
                                 nScrambledPassword &scrambled);

    // scramble it again before transfering it over the network
    static void ScrambleWithSalt(const nScrambledPassword& source,
                                 const nSalt& salt,
                                 nScrambledPassword& dest);


#ifdef KRAWALL_SERVER
    // get a random salt value
    static void RandomSalt(nSalt& salt);

    // secret key to encrypt server->master server league transfer
    static const nScrambledPassword& SecretLeagueKey();


    // fetch the scrambled password of username from the users database
    static void GetScrambledPassword(const tString& username,
                                     nScrambledPassword &scrambled);

    // called on the servers to create a league message
    static void SendLeagueMessage(const tString& message = *reinterpret_cast<tString *>(0));

    // called on the master server when the league message is received
    static void ReceiveLeagueMessage(const tString& message);

    // league message generation functions: fill message with the message to be
    // sent

    // called when victim drives against killer's wall
    static void Frag(const tString &killer, const tString& victim, tString& message);

    // called at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void RoundEnd(const tString* players, int numPlayers, tString& message);

    // called ON THE SERVER when victim drives against killer's wall
    static void ServerFrag(const tString &killer, const tString& victim);

    // called ON THE SERVER at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void ServerRoundEnd(const tString* players, int numPlayers);


    // called ON THE MASTER when victim drives against killer's wall
    static void MasterFrag(const tString &killer, const tString& victim);

    // called ON THE MASTER at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void MasterRoundEnd(const tString* players, int numPlayers);


    // Adress checking functions

    // first validity check for the league messages
    static bool IsFromKrawall(tString& adress, unsigned int port);

    // check if a user is from germany (so the master server will require
    // a password check)
    static bool RequireMasterLogin(tString& adress, unsigned int port);
#endif

    // only servers acknowledged by this funktion are from Krawall and
    // are allowed to request logins
    static bool MayRequirePassword(tString& adress, unsigned int port);

};

#endif
