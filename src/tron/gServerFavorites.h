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

#ifndef ArmageTron_SERVERFAVORITES_H
#define ArmageTron_SERVERFAVORITES_H

#include "tString.h"
#include "nServerInfo.h"

class nServerInfoBase;

/*
//! favorite server information, just to connect
class gServerInfoFavorite: public nServerInfoBase
{
public:
    // construct a server directly with connection name and port
    gServerInfoFavorite( tString const & connectionName, unsigned int port );
    gServerInfoFavorite() {}
    void SetPort( unsigned int port ) { nServerInfoBase::SetPort( port ); }
    void SetConnectionName( tString const connectionName ) { nServerInfoBase::SetConnectionName( connectionName ); }
};
*/

class gServerFavorites
{
public:
    //! open the favorites menu
    static void FavoritesMenu();

    //! open the single custom connect menu
    static void CustomConnectMenu();

    //! add a server to the list of favorites
    static bool AddFavorite( nServerInfoBase const * server );

    //! test whether a server is bookmarked
    static bool IsFavorite( nServerInfoBase const * server );
};

#endif

