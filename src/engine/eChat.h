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

#include "tString.h"
#include "nNetwork.h"
#include <deque>

class ePlayerNetID;

enum eChatMessageType
{
    eChatMessageType_Command = 0, // A remotely issued command
    eChatMessageType_Private = 1, // Private message chat
    eChatMessageType_Team = 2,    // Team message chat
    eChatMessageType_Public = 3   // Public chat
};

class eChatSaid
{
public:
    eChatSaid(const tString &, const nTimeRolling &, eChatMessageType);
    ~eChatSaid();
    const tString & Said() const;
    const nTimeRolling & Time() const;
    const eChatMessageType Type() const;

private:
    const tString & said_;
    const nTimeRolling & time_;
    eChatMessageType type_;
};

typedef std::deque< eChatSaid > eChatLastSaid;

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
    eChatMessageType lastSaidType_; //!< The last said message type.
private:
    bool CheckSpam( REAL factor, tOutput const & message ) const;
};

#endif
