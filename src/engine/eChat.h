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

class eChatPrefixSpamTester;

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
    eChatMessageType Type() const;
    
    /**
     * Set the message type
     * 
     * @param newType The new message type.
     * @see Type()
     */
    void SetType( eChatMessageType newType );
    
    /**
     * Does this message start with the other message?
     */
    bool StartsWith( const eChatSaidEntry & other ) const;

private:
    
    friend class eChatPrefixSpamTester;
    
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
    
    /**
     * Chat can be checked to guard against prefix-spam. When a prefix has
     * been consistently used in messages, it will stored in this list.
     * 
     * @return The known prefixes
     */
    const PrefixList & KnownPrefixes() const;
    
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
    friend class eChatPrefixSpamTester;
    
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

enum eChatPrefixSpamType
{
    eChatPrefixSpamType_New,
    eChatPrefixSpamType_Known
};

/**
 * Checks for prefix spam from a player
 */
class eChatPrefixSpamTester
{
public:
    /**
     * @param player The player to check for prefix spam
     * @param say the chat message to check, not yet sent to peers
     */
    eChatPrefixSpamTester( ePlayerNetID * player, const eChatSaidEntry & say );
    virtual ~eChatPrefixSpamTester();
    
    /**
     * Check for prefix spam.
     *
     * @param out Will contain the prefix, if one is found
     * @param timeOut
     * @return Did the message have a common prefix?
     */
    bool Check( tString & out, nTimeRolling & timeOut );
    bool Check( tString & out, nTimeRolling & timeOut, eChatPrefixSpamType & typeOut );
    
private:
        
    class PrefixEntry
    {
    public:
        PrefixEntry() : occurrences( 0 ), score( 0 ) { }
        ~PrefixEntry() { }
        
        int occurrences;
        REAL score;
    };
    
    /**
     * Tests the message against known prefixes.
     * 
     * @param out See Check()
     * @param timeOut See Check()
     * @return Did the message start with a known prefix?
     */
    bool HasKnownPrefix( tString & out, nTimeRolling & timeOut ) const;
    
    /**
     * Tests if this message is directed towards another player.
     *
     * Example chat message directed to Player 1:
     *
     *     Player 1: change your name
     * 
     * @param prefix The possible player name
     * @param nameLen The length of the name searched for
     * @return Was the prefix a player name?
     */
    bool ChatDirectedTowardsPlayer( const tString & prefix, int & nameLen ) const;
    
    /**
     * We should only check certain message types. For example, commands
     * such as /admin should never be checked for prefix-spam.
     *
     * @param said The message that we might want to check
     * @return Should we check this message for prefix-spam?
     */
    bool ShouldCheckMessage( const eChatSaidEntry & said ) const;
    
    /**
     * Calculate the score for a prefix
     */
    void CalcScore( PrefixEntry & data, const int & len, const tString & prefix ) const;
    
    /**
     * After a prefix has been found, remove all chat entries with it. We don't want
     * to penalize users after a prefix is found, and the user starts a message with
     * a word that begins with the found prefix.
     */
    void RemovePrefixEntries( const tString & prefix, const eChatSaidEntry & e ) const;
    
    void RemoveTimedOutPrefixes() const;
    nTimeRolling RemainingTime( nTimeRolling t ) const;

    ePlayerNetID * player_;
    const eChatSaidEntry & say_;
};

/**
 * Checks for annoying /shuffle message spam
 */
class eShuffleSpamTester
{
public:
    eShuffleSpamTester();
    bool ShouldAnnounce() const;
    void Reset();
    void Shuffle();
    tString ShuffleMessage( ePlayerNetID *player, int position ) const; //!< print message for player wishing to pre-join shuffle to position
    tString ShuffleMessage( ePlayerNetID *player, int fromPosition, int toPosition ) const; //!< print message for player shuffling in-team
protected:
    bool ShouldDisplaySuppressMessage() const;
    int numberShuffles_;
};

#endif
