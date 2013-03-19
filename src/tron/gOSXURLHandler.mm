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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 ***************************************************************************
 
 */

#include "gOSXURLHandler.h"
#include "tString.h"
#include "nServerInfo.h"
#include "gCommandLineJumpStart.h"
#import <Cocoa/Cocoa.h>

@interface AAURLHandler : NSObject
{
    NSMutableString *_startupEvent; //! Set when the game is started via URL
    BOOL _shouldConnect;
}
- (void) setShouldConnect:(BOOL)shouldConnect showSplash:(bool *)showSplash;
@end

void ret_to_MainMenu();

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

- (void)connectToServer:(NSString *)server showSplash:(bool *)showSplash
{
    if ( ![server isEqualToString:@"armagetronad://"] )
    {
        tString raw( [server UTF8String] );
        tString servername;
        tString port;
        ExtractConnectionInformation( raw, servername, port );
        if ( servername != "" )
        {
            if ( showSplash )
                *showSplash = false;
            ret_to_MainMenu();
            nServerInfoRedirect server( servername, port.ToInt() );
            AAURLHandlerConnect( &server );
        }
    }
}

- (void) handleURLEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    if ( _shouldConnect )
    {
        [self connectToServer:[[[event descriptorAtIndex:1] stringValue] retain] showSplash:NULL];
    }
    // Save the event for later, we must finish initializing
    else
    {
        _startupEvent = [[[event descriptorAtIndex:1] stringValue] retain];
    }
}

//! Set after Armagetron is finished initializing.
- (void) setShouldConnect:(BOOL)shouldConnect showSplash:(bool *)showSplash
{
    _shouldConnect = shouldConnect;
    if ( _shouldConnect )
    {
        // _startupEvent is initialized to an empty string. Check if a URL event actually occured
        if ( ![_startupEvent isEqualToString:@""] )
        {
            [self connectToServer:_startupEvent showSplash:showSplash];
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

void sg_SetupAAURLHandler()
{
    urlhandler = [[AAURLHandler alloc] init];
}

void sg_StartAAURLHandler( bool & showSplash )
{
    [urlhandler setShouldConnect:YES showSplash:&showSplash];
}

void sg_CleanupAAURLHandler()
{
    [urlhandler release];
}