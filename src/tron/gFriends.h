/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2008  Manuel Moos (manuel@moosnet.de)

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

#include "tString.h"
#include "tList.h"
#include "tConfiguration.h"

#define MAX_FRIENDS 10

class gFriends
{
private:

public:
	tString friends[MAX_FRIENDS];
    std::auto_ptr< tConfItemLine > confItems[MAX_FRIENDS];

	gFriends();
    static void FriendsMenu();
};

bool getFriendsEnabled();
void FriendsToggle();
tString* getFriends();
