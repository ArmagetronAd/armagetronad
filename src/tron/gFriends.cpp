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

#include "gFriends.h"
#include "uMenu.h"

static bool sg_enableFriends = false;
static tConfItem< bool > sg_enableFriendsConf( "ENABLE_FRIENDS", sg_enableFriends );

static gFriends sg_friends;

gFriends::gFriends()
{
	tString name;
    for(int i = MAX_FRIENDS-1; i>=0; i--)
	{
		name = "FRIEND_";
		name << (i + 1);
		confItems[i] = std::auto_ptr< tConfItemLine >( tNEW(tConfItemLine) (tConfItemLine(name, friends[i])));
    }
}

void gFriends::FriendsMenu( void )
{
    uMenu net_menu("$friends_menu");

	uMenuItemToggle friends_enable (&net_menu, "$friends_enable", "$friends_enable_help", sg_enableFriends);

	tString name;
    for (int i = MAX_FRIENDS-1; i>=0; i--)
	{
		name = tOutput("$friend_word");
		name << " " << (i + 1);
		tNEW(uMenuItemString) (uMenuItemString(&net_menu, name, "$friend_help", sg_friends.friends[i]));
    }

    net_menu.Enter();
}

bool getFriendsEnabled()
{
	return sg_enableFriends;
}

void FriendsToggle()
{
	if (sg_enableFriends)
		sg_enableFriends = false;
	else
	sg_enableFriends = true;
}

tString* getFriends()
{
	return sg_friends.friends;
}
