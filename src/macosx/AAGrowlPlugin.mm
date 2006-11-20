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

#include <Growl-WithInstaller/GrowlApplicationBridge.h>
#include <Growl-WithInstaller/GrowlDefines.h>
#include "AAGrowlPlugin.h"
#include "tString.h"

#define PLAYER_LEFT @"Player left"
#define PLAYER_ENTERED @"Player entered"
#define PLAYER_RENAMED @"Player renamed"
#define DEATH_SUICIDE @"Death suicide"
#define DEATH_FRAG @"Death frag"
#define DEATH_TEAMKILL @"Death teamkill"
#define GAME_END @"Game end"
#define NEW_ROUND @"New Round"
#define ROUND_WINNER @"Round winner"
#define MATCH_WINNER @"Match winner"
#define NEW_MATCH @"New match"

@implementation AAGrowlPlugin

- (void)startGrowling
{    
    [GrowlApplicationBridge setGrowlDelegate:self];
}

//! Give Growl a list of all notifications we plan on sending
- (NSDictionary *)registrationDictionaryForGrowl
{
    NSArray *all_notes = [NSArray arrayWithObjects:PLAYER_LEFT,
                                               PLAYER_ENTERED,
                                               PLAYER_RENAMED,
                                               DEATH_SUICIDE,
                                               DEATH_FRAG,
                                               DEATH_TEAMKILL,
                                               GAME_END,
                                               NEW_ROUND,
                                               ROUND_WINNER,
                                               MATCH_WINNER,
                                               NEW_MATCH,
                                               nil];
    NSArray *def_notes = [NSArray arrayWithObjects:GAME_END,
                          NEW_ROUND,
                          ROUND_WINNER,
                          MATCH_WINNER,
                          NEW_MATCH,
                          nil];
    
            
    NSDictionary *growlNotes = [NSDictionary dictionaryWithObjectsAndKeys:
                                    all_notes, GROWL_NOTIFICATIONS_ALL,
                                    def_notes, GROWL_NOTIFICATIONS_DEFAULT,
                                    nil];
    return growlNotes;
}

- (NSString *)applicationNameForGrowl
{
    return @"Armagetron Advanced";
}

+ (void)growl:(NSString *)aTitle message:(NSString *)aMessage
{
    [GrowlApplicationBridge notifyWithTitle:aTitle
                                description:aMessage
                           notificationName:aTitle
                                   iconData:nil
                                   priority:0
                                   isSticky:NO
                               clickContext:nil];
}

@end
