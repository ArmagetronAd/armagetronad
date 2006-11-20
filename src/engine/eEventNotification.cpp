/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include <iostream>

#include "eEventNotification.h"
#include "ePlayer.h"
#include "nNetwork.h"
#include "tString.h"

void se_eventNotificationHandle( nMessage &m );

static nVersionFeature se_eventNotificationFeature( 20 );
static nDescriptor se_eventNotificationDescriptor(  199, se_eventNotificationHandle, "event_notification" );

void se_eventNotificationHandle( nMessage &m )
{
    tString title, message;
    m >> title;
    m >> message;
}

void se_sendEventNotification( tString title, tString message )
{
    for ( int user = MAXCLIENTS; user > 0; --user )
    {
        if ( sn_Connections[ user ].socket )
        {
            if ( se_eventNotificationFeature.Supported( user ) )
            {
                std::cout << "Found a supported peer. Sending...\n";
                nMessage *m = new nMessage( se_eventNotificationDescriptor );
                *m << title;
                *m << message;
                m->Send( user );
            }
            
        }
    }
}
