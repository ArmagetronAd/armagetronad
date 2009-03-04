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

#include <algorithm>
#include <map>
#include <vector>

#include "eChat.h"
#include "ePlayer.h"
#include "tSysTime.h"

//!< Should spam prefix checking be enabled?
static bool se_prefixSpamShouldEnable = true;
static tConfItem< bool > se_prefixSpamShouldEnableConf( "PREFIX_SPAM_ENABLE", se_prefixSpamShouldEnable );

//!< If a prefix begins with a color code it will have this multiplier applied to the score.
static REAL se_prefixSpamStartColorMultiplier = 2;
static tConfItem< REAL > se_prefixSpamStartColorMultiplierConf( "PREFIX_SPAM_START_COLOR_MULTIPLIER", se_prefixSpamStartColorMultiplier );

//!< Increase score by log( prefix_length * multiplier )
static REAL se_prefixSpamLengthMultiplier = 0.5;
static tConfItem< REAL > se_prefixSpamLengthMultiplierConf( "PREFIX_SPAM_LENGTH_MULTIPLIER", se_prefixSpamLengthMultiplier );

//!< Increase score by log( num_color_codes * multiplier )
static REAL se_prefixSpamNumberColorCodesMultiplier = 1;
static tConfItem< REAL > se_prefixSpamNumberColorCodesMultiplierConf( "PREFIX_SPAM_NUMBER_COLOR_CODES_MULTIPLIER",
                                                                      se_prefixSpamNumberColorCodesMultiplier );

//!< Increase score by log( know_prefixes * multiplier )
static REAL se_prefixNumberKnownPrefixesMultiplier = 1;
static tConfItem< REAL > se_prefixNumberKnownPrefixesMultiplierConf( "PREFIX_SPAM_NUMBER_KNOWN_PREFIXES_MULTIPLIER",
                                                                     se_prefixNumberKnownPrefixesMultiplier );

//!< The score, from prefix checking, a message must have for it to be considered spam.
static REAL se_prefixSpamRequiredScore = 15.0;
static tConfItem< REAL > se_prefixSpamRequiredScoreConf( "PREFIX_SPAM_REQUIRED_SCORE", se_prefixSpamRequiredScore );

/**
 * Helper class for predicate stl-comparisons
 */
class IsPrefixPredicate
{
public:
    IsPrefixPredicate( const tString & s ) :s_( s ) { }
    bool operator() ( const tString & other ) { return s_.StartsWith( other ); }
private:
    const tString s_;
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
     * @return Did the message have a common prefix?
     */
    bool Check( tString & out );
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
     * @return Did the message start with a known prefix?
     */
    bool HasKnownPrefix( tString & out ) const;
    
    /**
     * Tests if this message is directed towards another player.
     *
     * Example chat message directed to Player 1:
     *
     *     Player 1: change your name
     * 
     * @param prefix The possible player name
     * @return Was the prefix a player name?
     */
    bool ChatDirectedTowardsPlayer( const tString & prefix ) const;
    
    /**
     * We should only check certain message types. For example, commands
     * such as /admin should never be checked for prefix-spam.
     *
     * @param said The message that we might want to check
     * @return Should we check this message for prefix-spam?
     */
    bool ShouldCheckMessage( const eChatSaidEntry & said ) const;
    
    void CalcScore( PrefixEntry & data, const int & len, const tString & prefix );

    ePlayerNetID * player_;
    const eChatSaidEntry & say_;
};

/**
 * Example: se_EscapeColors( "0xfff000" ) -> "#fff000"
 * 
 * @param s The string to escape
 * @return A string with color codes escaped.
 */
static tString se_EscapeColors( const tString & s )
{
    tString ret;
    
    int size = s.Size();
    for ( int i = 0; i < size; i++ )
    {
        if ( s[i] == '0' && size - i >= 2 && s[i + 1] == 'x' )
        {
            ret << '#';
            i++;
        }
        else
        {
            ret << s[i];
        }
    }
    return ret;
}

/**
 * Example: se_CountColorCodes( "0xfff000" ) -> 1
 * 
 * @return The number of color codes in s.
 */
static int se_CountColorCodes( const tString & s )
{
    int colorCodes = ( s.Len() - tColoredString::RemoveColors( s ).Len() ) / 8;
    if ( colorCodes < 0 )
        colorCodes = 0;
    
    return colorCodes;
}

/**
 * Example: se_StartsWithColorCode( "0xfff000asdf" ) -> true
 * 
 * @return Does the string start with a color code?
 */
static bool se_StartsWithColorCode( const tString & s )
{
    return s.StartsWith( "0x" );
}

static double se_CalcScore( double a )
{
    return log( 2 + a ) / log( 2 );
}

eChatSaidEntry::eChatSaidEntry(const tString & said, const nTimeRolling & t, eChatMessageType type)
: said_( said ), time_( t ), type_( type )
{
}

eChatSaidEntry::~eChatSaidEntry()
{
}

const tString & eChatSaidEntry::Said() const
{
    return said_;
}

const nTimeRolling & eChatSaidEntry::Time() const
{
    return time_;
}

const eChatMessageType eChatSaidEntry::Type() const
{
    return type_;
}

void eChatSaidEntry::SetType(eChatMessageType newType)
{
    type_ = newType;
}

eChatLastSaid::eChatLastSaid()
: lastSaid_(), knownPrefixes_()
{
}

eChatLastSaid::~eChatLastSaid()
{
}

const eChatLastSaid::SaidList & eChatLastSaid::LastSaid() const
{
    return lastSaid_;
}

eChatLastSaid::SaidList & eChatLastSaid::LastSaid()
{
    return lastSaid_;
}

const eChatLastSaid::StringList & eChatLastSaid::KnownPrefixes() const
{
    return knownPrefixes_;
}

void eChatLastSaid::AddSaid( const eChatSaidEntry & saidEntry )
{
    if ( lastSaid_.size() >= static_cast< size_t >( se_lastSaidMaxEntries ) )
        lastSaid_.pop_back();
    
    lastSaid_.push_front( saidEntry );
}

void eChatLastSaid::AddPrefix( const tString & prefix )
{
    knownPrefixes_.push_back( prefix );
}

// handles spam checking at the right time
eChatSpamTester::eChatSpamTester( ePlayerNetID * p, tString const & say )
: tested_( false ), shouldBlock_( false ), player_( p ), say_( say ), factor_( 1 ), lastSaidType_( eChatMessageType_Public )
{
    say_.RemoveTrailingColor();
}

bool eChatSpamTester::Block()
{
    if ( !tested_ )
    {
        shouldBlock_ = Check();
        tested_ = true;
    }
    
    return shouldBlock_;
}

bool eChatSpamTester::Check()
{
    nTimeRolling currentTime = tSysTimeFloat();
    
    // check if the player already said the same thing not too long ago
    eChatLastSaid::SaidList const & lastSaid = player_->lastSaid_.LastSaid();
    const size_t saidSize = lastSaid.size();
    for ( size_t i = 0; i < saidSize; i++ )
    {
        eChatSaidEntry const & said = lastSaid[i];
        if( (say_.StripWhitespace() == said.Said().StripWhitespace()) && ( (currentTime - said.Time()) < se_alreadySaidTimeout * factor_ ) )
        {
            sn_ConsoleOut( tOutput("$spam_protection_repeat", say_ ), player_->Owner() );
            return true;
        }
    }
    
    eChatSaidEntry saidEntry( say_, currentTime, lastSaidType_ );
    
    // check for prefix spam
    if ( se_prefixSpamShouldEnable )
    {
        eChatPrefixSpamTester tester( player_, saidEntry );
        tString foundPrefix;
        if ( tester.Check( foundPrefix ) )
        {
            sn_ConsoleOut( tOutput("$spam_protection_prefix", foundPrefix ), player_->Owner() );
            return true;
        }
    }
    
    REAL lengthMalus = say_.Len() / 20.0;
    if ( lengthMalus > 4.0 )
    {
        lengthMalus = 4.0;
    }
    
    // extra spam severity factor
    REAL factor = factor_;
    factor *= 1 + lengthMalus;
    
    // Apply number of color codes to the spam severity factor. Burn in hell, color code abusers.
    factor *= se_CalcScore( se_CountColorCodes( say_ ) );
    
    if ( CheckSpam( factor, tOutput("$spam_chat") ) )
        return true;
    
#ifdef KRAWALL_SERVER
    if ( player_->GetAccessLevel() > se_chatAccessLevel )
    {
        // every once in a while, remind the public that someone has something to say
        static double nextRequest = 0;
        double now = tSysTimeFloat();
        if ( now > nextRequest && se_chatRequestTimeout > 0 )
        {
            sn_ConsoleOut( tOutput("$access_level_chat_request", player_->GetColoredName(), player_->GetLogName() ), player_->Owner() );
            nextRequest = now + se_chatRequestTimeout;
        }
        else
        {
            sn_ConsoleOut( tOutput("$access_level_chat_denied" ), player_->Owner() );
        }
        
        return true;
    }
#endif
    
    player_->lastSaid_.AddSaid( saidEntry );
    
    return false;
}

bool eChatSpamTester::CheckSpam( REAL factor, tOutput const & message ) const
{
    if ( nSpamProtection::Level_Mild <= player_->chatSpam_.CheckSpam( factor, player_->Owner(), message ) )
        return true;

    return false;
}

size_t CommonPrefix(const tString & a, const tString & b)
{
    bool aGreater = a > b;
    const tString & min = aGreater ? b : a;
    const tString & max = aGreater ? a : b;
    
    int n = min.Size();
    for (int i = 0; i < n; i++)
        if (min[i] != max[i])
            return i;
    
    return n;
}

eChatPrefixSpamTester::eChatPrefixSpamTester( ePlayerNetID * player, const eChatSaidEntry & say )
: player_( player ), say_( say )
{
}

eChatPrefixSpamTester::~eChatPrefixSpamTester()
{
}

bool eChatPrefixSpamTester::Check( tString & out )
{
    if ( !ShouldCheckMessage( say_ ) )
        return false;

    eChatLastSaid::SaidList & lastSaid = player_->lastSaid_.LastSaid();
    const size_t saidSize = lastSaid.size();
    
    // check from known prefixes
    if ( HasKnownPrefix( out ) )
        return true;
    
    // Map of PrefixLength => Data
    std::map< int, PrefixEntry > foundPrefixes;
        
    for ( size_t i = 0; i < saidSize; i++ )
    {
        eChatSaidEntry & said = lastSaid[i];
        
        if ( !ShouldCheckMessage( said ) || say_.Said() == said.Said() )
            continue;
        
        int common = CommonPrefix( say_.Said(), said.Said() );
        
        if ( common > 0 )
        {
            const tString prefix = say_.Said().SubStr(0, common);
            
            // User is talking to a player. Not prefix spam
            // Example: Player 1: grind center. [etc...]
            if ( ChatDirectedTowardsPlayer( prefix ) )
            {
                // mark message so we don't need to check it next time
                said.SetType( eChatMessageType_Public_Direct );
                return false;
            }
            
            if ( foundPrefixes.find(common) == foundPrefixes.end() )
                foundPrefixes[common] = PrefixEntry();
            
            PrefixEntry & data = foundPrefixes[common];

            data.occurrences += 1;
            CalcScore( data, common, prefix );
            if ( data.score >= se_prefixSpamRequiredScore )
            {
                player_->lastSaid_.AddPrefix( prefix );
                out = se_EscapeColors( prefix );
                return true;
            }
        }
    }

    return false;
}

void eChatPrefixSpamTester::CalcScore( PrefixEntry & data, const int & len, const tString & prefix )
{   
    // Apply based on length of found prefix.
    data.score += se_CalcScore( len * se_prefixSpamLengthMultiplier );

    // Apply based on number of color codes in prefix.
    data.score += se_CalcScore( se_CountColorCodes( prefix ) * se_prefixSpamNumberColorCodesMultiplier );

    // Apply based on number of known prefixes.
    data.score += se_CalcScore( player_->lastSaid_.KnownPrefixes().size() * se_prefixNumberKnownPrefixesMultiplier );

    // Apply multiplier for annoying color messages
    if ( se_StartsWithColorCode( prefix ) )
        data.score *= se_prefixSpamStartColorMultiplier;
    
    std::cout << "score: " << data.score << '\n';
}

bool eChatPrefixSpamTester::HasKnownPrefix( tString & out ) const
{
    const eChatLastSaid::StringList & prefixes = player_->lastSaid_.KnownPrefixes();
    eChatLastSaid::StringList::const_iterator it =
        std::find_if( prefixes.begin(), prefixes.end(), IsPrefixPredicate( say_.Said() ) );
    
    if ( it != prefixes.end() )
    {
        out = se_EscapeColors( *it );
        return true;
    }
    
    return false;
}

bool eChatPrefixSpamTester::ChatDirectedTowardsPlayer( const tString & prefix ) const
{
    tString possiblePlayer( prefix );
    
    // When using 0.3 name completion at the start of a message,
    // ": " is appended to the end of the player name.
    if ( st_StringEndsWith( possiblePlayer, ": " ) )
        possiblePlayer = possiblePlayer.SubStr( 0, possiblePlayer.Len() - 3 );
    
    if ( ePlayerNetID::FindPlayerByName( possiblePlayer ) )
        return true;
    
    return false;     
}

bool eChatPrefixSpamTester::ShouldCheckMessage( const eChatSaidEntry & said ) const
{
    return said.Type() >= eChatMessageType_Public;
}
