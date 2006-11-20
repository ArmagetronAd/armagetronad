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

#include "AAGrowlBridge.h"
#include "AAGrowlPlugin.h"
#include "tString.h"

void Growl(tString title, tString message)
{
    NSString *nstitle, *nsmessage;
    nstitle = [[NSString alloc] initWithCString:title.c_str()];
    nsmessage = [[NSString alloc] initWithCString:message.c_str()];
    [AAGrowlPlugin growl:nsmessage message:nsmessage];
    [nstitle release];
    [nsmessage release];
}

