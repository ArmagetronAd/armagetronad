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

#include "nAuthentification.h"
#include "nNetwork.h"
#include "nNetObject.h"
#include "tMemManager.h"
#include "tToDo.h"
#include "tLocale.h"
#include "tSysTime.h"

#include <memory>
#include <string>

static nAuthentification::UserPasswordCallback* S_UserPasswordCallback = NULL;
static nAuthentification::LoginResultCallback*  S_LoginResultCallback  = NULL;

// let the game register the callbacks
void nAuthentification::SetUserPasswordCallback(nAuthentification::UserPasswordCallback* callback)
{
    S_UserPasswordCallback = callback;
}

void nAuthentification::SetLoginResultCallback (nAuthentification::LoginResultCallback* callback)
{
    S_LoginResultCallback = callback;
}





static bool            S_UserAuthActive [MAXCLIENTS+2];
static tString         S_UserAuthName   [MAXCLIENTS+2];

#ifdef KRAWALL_SERVER
static nKrawall::nSalt S_UserAuthSalt   [MAXCLIENTS+2];
static REAL            S_UserAuthStarted[MAXCLIENTS+2];
#endif

static void Reset()
{
    S_UserAuthActive[nCallbackLoginLogout::User()] = false;
}

static nCallbackLoginLogout resetter(Reset);


// network handler declarations

static nDescriptor nPasswordRequest(40, &nAuthentification::HandlePasswordRequest, "password_request");

static nDescriptor nPasswordAnswer(41, &nAuthentification::HandlePasswordAnswer, "password_answer");


// on the server: request user authentification from login slot
void nAuthentification::RequestLogin(const tString& username, int user, const tOutput& message, bool failureOnLastTry)
{
#ifdef KRAWALL_SERVER

    // do nothing if there is another login in process for that client
    if (S_UserAuthActive[user])
        return;

    // create a random salt value
    nSalt salt;
    RandomSalt(salt);

    // save the login information
    memcpy(S_UserAuthSalt[user],salt, sizeof(nSalt));
    S_UserAuthActive[user]  = true;
    S_UserAuthStarted[user] = tSysTimeFloat();
    S_UserAuthName[user]    = username;

    // send the salt value and the username to the
    nMessage *m = tNEW(nMessage)(nPasswordRequest);
    WriteSalt(salt, *m);
    *m << username;
    *m << static_cast<tString>(message);
    *m << failureOnLastTry;
    m->Send(user);
#endif
}

static tString         S_username;
static tString         S_message;
static nKrawall::nSalt S_salt;
static int             S_user = -1;
static bool            S_failureOnLastTry = false;

// finish the request for username and password
static void FinishHandlePasswordRequest()
{
    nKrawall::nScrambledPassword scrambled, egg;

    // if the callback exists, get the scrambled password of the wanted user
    if (S_UserPasswordCallback)
        (*S_UserPasswordCallback)(S_username, S_message, scrambled, S_failureOnLastTry);

    // scramble it with the given salt
    nKrawall::ScrambleWithSalt(scrambled, S_salt, egg);

    // destroy the original password
    memset(scrambled, 0, sizeof(nKrawall::nScrambledPassword));

    // and send it back
    nMessage *ret = tNEW(nMessage)(nPasswordAnswer);
    nKrawall::WriteScrambledPassword(egg, *ret);
    *ret << S_username;
    ret->Send(S_user);

    S_user = -1;
}

// receive a password request
void nAuthentification::HandlePasswordRequest(nMessage& m)
{
    if (m.SenderID() > 0 || sn_GetNetState() != nCLIENT)
        Cheater(m.SenderID());

    // already in the process: return without answer
    if (S_user >= 0)
        return;

    // read salt and username from the message
    ReadSalt(m, S_salt);
    m >> S_username;
    m >> S_message;
    if (!m.End())
        m >> S_failureOnLastTry;
    else
        S_failureOnLastTry = true;
    S_user = m.SenderID();

    // postpone the answer for a better opportunity since it
    // most likely involves opening a menu and waiting a while (and we
    // are right now in the process of fetching network messages...)
    st_ToDo(&FinishHandlePasswordRequest);
}


void nAuthentification::HandlePasswordAnswer(nMessage& m)
{
#ifdef KRAWALL_SERVER
    // do nothing if the authorisation was not requested
    if (!S_UserAuthActive[m.SenderID()])
        return;

    S_UserAuthActive[m.SenderID()] = false;

    // read password and username from remote
    nScrambledPassword scrambledRemote;
    tString            username;
    ReadScrambledPassword(m, scrambledRemote);
    m >> username;

    // get the password from the user database
    nScrambledPassword temp, scrambledDatabase;
    GetScrambledPassword(username, temp);
    ScrambleWithSalt(temp, S_UserAuthSalt[m.SenderID()], scrambledDatabase);

    if (S_LoginResultCallback)
        (*S_LoginResultCallback)(username,
                                 S_UserAuthName[m.SenderID()],
                                 m.SenderID(),
                                 ArePasswordsEqual(scrambledRemote, scrambledDatabase));
#endif
}


