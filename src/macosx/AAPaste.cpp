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

#include <ApplicationServices/ApplicationServices.h>
#include "AAPaste.h"

bool AAPastePasteboardData(CFDataRef &outData) {
    static const CFStringRef plainTextFlavor = CFSTR("public.utf8-plain-text");
    
    OSStatus status;
    PasteboardRef pasteboard;
    bool success = false;
    
    status = PasteboardCreate(kPasteboardClipboard, &pasteboard);
    require_noerr(status, CantCreatePasteboard);
    
    ItemCount outItemCount;
    status = PasteboardGetItemCount(pasteboard, &outItemCount);
    require_noerr(status, CantGetItemCount);
    
    // do we have any pastes on the pasteboard?
    if (outItemCount < 1)
        goto CantGetItemCount;
    
    PasteboardItemID itemID;
    // Index starts out at 1 here.
    status = PasteboardGetItemIdentifier(pasteboard, 1, &itemID);
    require_noerr(status, CantGetItemID);
    
    CFDataRef data;
    status = PasteboardCopyItemFlavorData(pasteboard, itemID, plainTextFlavor, &data);
    require_noerr(status, CantGetDataFlavor);
    
    outData = data;
    success = true;
    
CantGetDataFlavor:
CantGetItemID:
CantGetItemCount:
    CFRelease(pasteboard);
CantCreatePasteboard:
    return success;
}
