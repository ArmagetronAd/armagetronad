/*
 
 *************************************************************************
 
 ArmageTron -- Just another Tron Lightcycle Game in 3D.
 Copyright (C) 2005  by Daniel Harple
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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 ***************************************************************************
 
 */

#include "AAURLHandler.h"
#include "tString.h"
#include "gServerFavorites.h"
#include "gCommandLineJumpStart.h"

@interface AAURLHandler : NSObject
{
    NSMutableString *_startupEvent; //! Set when the game is started via URL
    BOOL _shouldConnect;
}
- (void) setShouldConnect:(BOOL)shouldConnect;
@end

@implementation AAURLHandler

- (id) init
{
    self = [super init];
    if ( self )
    {
        [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
                                                           andSelector:@selector(handleURLEvent:withReplyEvent:)
                                                         forEventClass:kInternetEventClass
                                                            andEventID:kAEGetURL];
        _startupEvent = [[NSMutableString alloc] init];
        _shouldConnect = NO;
    }
    return self;
}

- (void)connectToServer:(NSString *)server
{
    if ( ![server isEqualToString:@"armagetronad://"] )
    {
        tString raw( [server UTF8String] );
        tString servername;
        tString port;
        ExtractConnectionInformation( raw, servername, port );
        gServerInfoFavorite server( servername, port.ToInt() );
        AAURLHandlerConnect( &server );
    }
}

- (void) handleURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    if ( _shouldConnect )
    {
        [self connectToServer:[[[event descriptorAtIndex:1] stringValue] retain]];
    }
    // Save the event for later, we must finish initializing
    else
    {
        _startupEvent = [[[event descriptorAtIndex:1] stringValue] retain];
    }
}

//! Set after Armagetron is finished initializing.
- (void) setShouldConnect:(BOOL)shouldConnect
{
    _shouldConnect = shouldConnect;
    if ( _shouldConnect )
    {
        // _startupEvent is initialized to an empty string. Check if a URL event actually occured
        if ( ![_startupEvent isEqualToString:@""] )
        {
            [self connectToServer:_startupEvent];
        }
    }

}

- (void) dealloc
{
    [_startupEvent release];
    [super dealloc];
}

@end

AAURLHandler *urlhandler;

void SetupAAURLHandler()
{
    urlhandler = [[AAURLHandler alloc] init];
}

void StartAAURLHandler()
{
    [urlhandler setShouldConnect:YES];
}

void CleanupAAURLHandler()
{
    [urlhandler release];
}