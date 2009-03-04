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
#include <vector>

class ePlayerNetID;

/**
 * Chat message types. Useful for applying spam-checking against certain
 * message types.
 */
enum eChatMessageType
{
    eChatMessageType_Command = 0,       //!< A remotely issued command
    eChatMessageType_Private = 1,       //!< Private message chat
    eChatMessageType_Team = 2,          //!< Team message chat
    eChatMessageType_Public_Direct = 3, //!< Public chat directed towards a player
    eChatMessageType_Public = 4,        //!< Public chat
    eChatMessageType_Me = 5             //!< /me
};

/**
 * Contains information about an individual chat message
 */
class eChatSaidEntry
{
public:
    eChatSaidEntry(const tString &, const nTimeRolling &, eChatMessageType);
    ~eChatSaidEntry();
    
    /**
     * @return The string that was said.
     */
    const tString & Said() const;
    
    /**
     * @return The time the user sent the message.
     */
    const nTimeRolling & Time() const;
    
    /**
     * @return The type of this message.
     */
    const eChatMessageType Type() const;
    
    /**
     * Set the message type
     * 
     * @param newType The new message type.
     * @see Type()
     */
    void SetType(eChatMessageType newType);
    
    /**
     * Does this message start with the other message?
     */
    bool StartsWith( const eChatSaidEntry & other ) const;

private:
    tString said_;
    nTimeRolling time_;
    eChatMessageType type_;
};

/**
 * Holds chat messages for one player
 */
class eChatLastSaid
{
public:    
    struct Prefix
    {
        Prefix( const tString & prefix, REAL score, nTimeRolling timeout );
        
        /**
         * Does this message start with the other message?
         */
        bool StartsWith( const Prefix & other ) const;
        
        tString prefix_;
        REAL score_;
        nTimeRolling timeout_;
    };
    
    typedef std::deque< eChatSaidEntry > SaidList;
    typedef std::vector< Prefix > PrefixList;
    
    
    eChatLastSaid();
    ~eChatLastSaid();
    
    /**
     * @return The last said entry
     */
    const SaidList & LastSaid() const;
    SaidList & LastSaid();
    
    /**
     * Chat can be checked to guard against prefix-spam. When a prefix has
     * been consistently used in messages, it will stored in this list.
     * 
     * @return The known prefixes
     */
    const PrefixList & KnownPrefixes() const;
    PrefixList & KnownPrefixes();
    
    /**
     * Add a new said entry
     * 
     * @param saidEntry the new entry
     */
    void AddSaid( const eChatSaidEntry & saidEntry );
    
    /**
     * Add a new chat prefix
     *
     * @param prefix the new prefix
     * @param score the score the prefixed received
     * @param now the time the prefix was found to be spam
     * @return the time this prefix will be ignored
     */
    nTimeRolling AddPrefix( const tString & prefix, REAL score, nTimeRolling now );
    
private:
    SaidList lastSaid_;
    PrefixList knownPrefixes_;
};

/**
 * Gaurd to stop chat spam.
 */
class eChatSpamTester
{
public:
    eChatSpamTester( ePlayerNetID * p, tString const & say );
    /**
     * Checks the message, if needed, for spam.
     *
     * @return Is this message spam?
     */
    bool Block();
    
    /**
     * Test the message to see if it spam.
     *
     * @return Is this message spam?
     * @see Block()
     */
    bool Check();

    bool tested_;                   //!< flag indicating whether the chat line has already been checked fro spam
    bool shouldBlock_;              //!< true if the message should be blocked for spam
    ePlayerNetID * player_;         //!< the chatting player
    tColoredString say_;            //!< the chat line
    REAL factor_;                   //!< extra spam weight factor
    eChatMessageType lastSaidType_; //!< The last said message type to be contained in an eChatSaidEntry record
private:
    bool CheckSpam( REAL factor, tOutput const & message ) const;
};

#endif
