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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 ***************************************************************************
 
 */

#ifndef ArmageTron_eChat_H
#define ArmageTron_eChat_H

#include "ePlayer.h"
#include "tString.h"

extern int se_SpamMaxLen;	// maximal length of chat message

class eChatSpamTester
{
public:
    eChatSpamTester( ePlayerNetID * p, tString const & say );
    bool Block();
    bool Check();

    bool tested_;             //!< flag indicating whether the chat line has already been checked fro spam
    bool shouldBlock_;        //!< true if the message should be blocked for spam
    ePlayerNetID * player_;   //!< the chatting player
    tColoredString say_;      //!< the chat line
    REAL factor_;             //!< extra spam weight factor
private:
    bool CheckSpam( REAL factor, tOutput const & message );
};

#endif
