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

#ifndef ArmageTron_AUTHENTIFICATION_H
#define ArmageTron_AUTHENTIFICATION_H

#include "nKrawall.h"

class tOutput;

namespace Network { class PasswordRequest; class PasswordAnswer; }
class nSenderInfo;

class nAuthentication: public nKrawall
{
public:
    //! callback where the game can register the password request/lookup
    typedef void UserPasswordCallback( nPasswordRequest const & request,
                                       nPasswordAnswer & answer );

    //! callback where the game can register the login success
    typedef void LoginResultCallback( nCheckResult const & result );

    //! let the game register the callbacks
    static void SetUserPasswordCallback(UserPasswordCallback* callback);
    static void SetLoginResultCallback (LoginResultCallback* callback);

    //! network handlers
    static void HandlePasswordRequest( Network::PasswordRequest const & request, nSenderInfo const & sender );
    static void HandlePasswordAnswer (  Network::PasswordAnswer const & answer, nSenderInfo const & sender );

    //! on the server: request user authentification from login slot
    static bool RequestLogin(const tString& authority, const tString& username, nNetObject & user, const tOutput& message );

    //! call when you have some time for lengthy authentication queries to servers
    static void OnBreak();

    //! returns whether a login is currently in process for the given user
    static bool LoginInProcess(nNetObject * user);
};

#endif
