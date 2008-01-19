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

*/

#ifndef ArmageTron_SERVERFAVORITES_H
#define ArmageTron_SERVERFAVORITES_H

#include "tString.h"

class nServerInfoBase;

class gServerFavorites
{
public:
    //! open the alternative masters
    static void AlternativesMenu();

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

