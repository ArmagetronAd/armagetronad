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
#ifdef MACOSX_XCODE
#ifndef DEDICATED
#   include "AAGrowlBridge.h"
#endif
#endif

#include "nProtoBuf.h"

#include "eEventNotification.pb.h"

static nVersionFeature se_eventNotificationFeature( 20 );

void se_eventNotificationHandle( Engine::EventNotification const & event, nSenderInfo const & )
{
    tString title = event.title();
    tString message = event.message();
#ifdef MACOSX_XCODE
#ifndef DEDICATED
    Growl(title, message);
#endif
#endif
}

static nProtoBufDescriptor< Engine::EventNotification > se_eventNotificationDescriptor(  199, se_eventNotificationHandle );

void se_sendEventNotification( tString title, tString message )
{
    if ( sn_GetNetState() != nSERVER )
    {
        return;
    }

    Engine::EventNotification & event = se_eventNotificationDescriptor.Broadcast( se_eventNotificationFeature );
    event.set_title( title );
    event.set_message( message );
}
