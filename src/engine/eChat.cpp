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
static tSettingItem< bool > se_prefixSpamShouldEnableConf( "PREFIX_SPAM_ENABLE", se_prefixSpamShouldEnable );

//!< If a prefix begins with a color code it will have this multiplier applied to the score.
static REAL se_prefixSpamStartColorMultiplier = 1.5;
static tSettingItem< REAL > se_prefixSpamStartColorMultiplierConf( "PREFIX_SPAM_START_COLOR_MULTIPLIER", se_prefixSpamStartColorMultiplier );

//!< Increase score by f( prefix_length * multiplier )
static REAL se_prefixSpamLengthMultiplier = 1.2;
static tSettingItem< REAL > se_prefixSpamLengthMultiplierConf( "PREFIX_SPAM_LENGTH_MULTIPLIER", se_prefixSpamLengthMultiplier );

//!< Increase score by f( num_color_codes * multiplier )
static REAL se_prefixSpamNumberColorCodesMultiplier = 1.2;
static tSettingItem< REAL > se_prefixSpamNumberColorCodesMultiplierConf( "PREFIX_SPAM_NUMBER_COLOR_CODES_MULTIPLIER",
                                                                      se_prefixSpamNumberColorCodesMultiplier );

//!< Increase score by f( know_prefixes * multiplier )
static REAL se_prefixNumberKnownPrefixesMultiplier = 1;
static tSettingItem< REAL > se_prefixNumberKnownPrefixesMultiplierConf( "PREFIX_SPAM_NUMBER_KNOWN_PREFIXES_MULTIPLIER",
                                                                     se_prefixNumberKnownPrefixesMultiplier );

//!< The score, from prefix checking, a message must have for it to be considered spam.
static REAL se_prefixSpamRequiredScore = 10.0;
static tSettingItem< REAL > se_prefixSpamRequiredScoreConf( "PREFIX_SPAM_REQUIRED_SCORE", se_prefixSpamRequiredScore );

//!< Found prefixes will timeout after f( score ) * multiplier seconds.
static REAL se_prefixSpamTimeoutMultiplier = 15.0;
static tSettingItem< REAL > se_prefixSpamTimeoutMultiplierConf( "PREFIX_SPAM_TIMEOUT_MULTIPLIER", se_prefixSpamTimeoutMultiplier );

/**
 * Helper class for predicate stl-comparisons
 */
template< typename T >
class IsPrefixPredicate
{
public:
    IsPrefixPredicate( const T & s, bool isSuperString=true ) :s_( s ), isSuperString_( isSuperString ) { }
    bool operator() ( const T & other )
    {
        const T & a = isSuperString_ ? s_ : other;
        const T & b = isSuperString_ ? other : s_;
        
        return a.StartsWith( b ); 
    }
private:
    const T s_;
    bool isSuperString_;
};

template< typename T >
class TimeOutPredicate
{
public:
    TimeOutPredicate( const T & then ) : then_( then ) { }
    bool operator() ( const T & now ) { return then_.timeout_ - now.timeout_ >= 0; }
private:
    T then_;
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

/**
 * \o/ logarithms
 * 
 * y = 1 for x=0
 */
static double se_CalcScore( double a )
{
    return log( 2 + a ) / log( 2 );
}

/** 
 * y = 0 for x=0
 */
static double se_CalcScore2( double a )
{
    return log( 1 + a ) / log( 2 );
}

static nTimeRolling se_CalcTimeout( double score )
{
    return se_CalcScore2( score ) * se_prefixSpamTimeoutMultiplier;
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

eChatMessageType eChatSaidEntry::Type() const
{
    return type_;
}

void eChatSaidEntry::SetType(eChatMessageType newType)
{
    type_ = newType;
}

bool eChatSaidEntry::StartsWith( const eChatSaidEntry & other ) const
{
    return said_.StartsWith( other.Said() );
}


eChatLastSaid::Prefix::Prefix( const tString & prefix, REAL score, nTimeRolling timeout )
: prefix_( prefix ), score_( score ), timeout_( timeout )
{
}

bool eChatLastSaid::Prefix::StartsWith( const eChatLastSaid::Prefix & other ) const
{
    return prefix_.StartsWith( other.prefix_ );
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

const eChatLastSaid::PrefixList & eChatLastSaid::KnownPrefixes() const
{
    return knownPrefixes_;
}

void eChatLastSaid::AddSaid( const eChatSaidEntry & saidEntry )
{
    if ( lastSaid_.size() >= static_cast< size_t >( se_lastSaidMaxEntries ) )
        lastSaid_.pop_back();
    
    lastSaid_.push_front( saidEntry );
}

nTimeRolling eChatLastSaid::AddPrefix( const tString & s, REAL score, nTimeRolling now )
{
    nTimeRolling timeoutAt = now + se_CalcTimeout( score );
    Prefix prefix( s, score, timeoutAt );
    knownPrefixes_.push_back( prefix );
    return timeoutAt;
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
        nTimeRolling timeOut;
        if ( tester.Check( foundPrefix, timeOut ) )
        {
            sn_ConsoleOut( tOutput("$spam_protection_prefix", foundPrefix, static_cast< float >( timeOut ) ), player_->Owner() );
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

/**
 * Brute force common prefix. Suitable for small inputs.
 */
size_t CommonPrefix(const tString & a, const tString & b)
{
    size_t n = std::min( a.Size(), b.Size() );
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i])
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

bool eChatPrefixSpamTester::Check( tString & out, nTimeRolling & timeOut )
{
    eChatPrefixSpamType typeOutIgnore;
    return Check( out, timeOut, typeOutIgnore );
}
bool eChatPrefixSpamTester::Check( tString & out, nTimeRolling & timeOut, eChatPrefixSpamType & typeOut )
{
    if ( !ShouldCheckMessage( say_ ) )
        return false;
        
    RemoveTimedOutPrefixes();
    
    // check from known prefixes
    if ( HasKnownPrefix( out, timeOut ) )
    {
        typeOut = eChatPrefixSpamType_Known;
        return true;
    }
        
    
    eChatLastSaid::SaidList & lastSaid = player_->lastSaid_.lastSaid_;
    
    // Map of Prefix => Data
    std::map< tString, PrefixEntry > foundPrefixes;
        
    for ( eChatLastSaid::SaidList::iterator it = lastSaid.begin(); it != lastSaid.end(); ++it )
    {
        eChatSaidEntry & said = *it;
        
        if ( !ShouldCheckMessage( said ) || say_.Said() == said.Said() )
            continue;
        
        int common = CommonPrefix( say_.Said(), said.Said() );
        
        if ( common > 0 )
        {
            const tString prefix = say_.Said().SubStr(0, common);
            
            // User is talking to a player. Not prefix spam, but we still check the
            // message text excluding the player name.
            // Example: Player 1: grind center. [etc...]
            int nameLen;
            if ( ChatDirectedTowardsPlayer( prefix, nameLen ) )
            {
                said.SetType( eChatMessageType_Public_Direct );
                said.said_ = said.said_.SubStr( nameLen + 1 );
                
                return false;
                
            }
            
            if ( foundPrefixes.find(prefix) == foundPrefixes.end() )
                foundPrefixes[prefix] = PrefixEntry();
            
            PrefixEntry & data = foundPrefixes[prefix];

            data.occurrences += 1;
            CalcScore( data, common, prefix );
            if ( data.score >= se_prefixSpamRequiredScore )
            {
                
#ifdef DEBUG
                con << "Spam prefix found: \"" << se_EscapeColors( prefix ) << "\" with score " << data.score << '\n';
#endif
                nTimeRolling t = player_->lastSaid_.AddPrefix( prefix, data.score, say_.Time() );
                timeOut = RemainingTime( t );
                
                // We caught the prefix. Don't catch words that start with the prefix.
                RemovePrefixEntries( prefix, said );
                
                out = se_EscapeColors( prefix );
                typeOut = eChatPrefixSpamType_New;
                
                return true;
            }
        }
    }

    return false;
}

void eChatPrefixSpamTester::CalcScore( PrefixEntry & data, const int & len, const tString & prefix ) const
{
    // Apply based on length of found prefix.
    data.score += se_CalcScore2( len * se_prefixSpamLengthMultiplier );

    // Apply based on number of color codes in prefix.
    data.score += se_CalcScore2( se_CountColorCodes( prefix ) * se_prefixSpamNumberColorCodesMultiplier );

    // Apply based on number of known prefixes.
    data.score += se_CalcScore2( player_->lastSaid_.KnownPrefixes().size() * se_prefixNumberKnownPrefixesMultiplier );

    // Apply multiplier for annoying color messages
    if ( se_StartsWithColorCode( prefix ) )
        data.score *= se_prefixSpamStartColorMultiplier;
}

void eChatPrefixSpamTester::RemovePrefixEntries( const tString & prefix, const eChatSaidEntry & e ) const
{
    eChatSaidEntry entry( prefix, e.Time(), e.Type() );
    eChatLastSaid::SaidList & xs = player_->lastSaid_.lastSaid_;
    xs.erase( std::remove_if( xs.begin(), xs.end(), IsPrefixPredicate< eChatSaidEntry >( entry, false ) ), xs.end() );
}

bool eChatPrefixSpamTester::HasKnownPrefix( tString & out, nTimeRolling & timeOut ) const
{
    eChatLastSaid::PrefixList & prefixes = player_->lastSaid_.knownPrefixes_;
    eChatLastSaid::Prefix testPrefix( say_.Said(), 0, 0 );
    
    eChatLastSaid::PrefixList::iterator it =
        std::find_if( prefixes.begin(), prefixes.end(), IsPrefixPredicate< eChatLastSaid::Prefix >( testPrefix ) );
    
    if ( it != prefixes.end() )
    {
        // Stop saying that!
        it->timeout_ += se_CalcTimeout( it->score_ ) / 3;
        
        out = se_EscapeColors( it->prefix_ );
        timeOut = RemainingTime( it->timeout_ );
        
        return true;
    }
    
    return false;
}

nTimeRolling eChatPrefixSpamTester::RemainingTime( nTimeRolling t ) const
{
    return t - say_.Time();
}

void eChatPrefixSpamTester::RemoveTimedOutPrefixes() const
{
    eChatLastSaid::PrefixList & xs = player_->lastSaid_.knownPrefixes_;
    tString empty;
    eChatLastSaid::Prefix entry( empty, 0, say_.Time() );
    xs.erase( std::remove_if( xs.begin(), xs.end(), TimeOutPredicate< eChatLastSaid::Prefix >( entry ) ), xs.end() );
}

bool eChatPrefixSpamTester::ChatDirectedTowardsPlayer( const tString & prefix, int & nameLen ) const
{
    tString possiblePlayer( prefix );
    
    // When using 0.3 name completion at the start of a message,
    // ": " is appended to the end of the player name.
    if ( st_StringEndsWith( possiblePlayer, ": " ) || st_StringEndsWith( possiblePlayer, ", " ) )
        possiblePlayer = possiblePlayer.SubStr( 0, possiblePlayer.Len() - 3 );
    
    if ( ePlayerNetID::FindPlayerByName( possiblePlayer, 0, false ) )
    {
        nameLen = possiblePlayer.Len();
        return true;
    }
    
    return false;     
}

bool eChatPrefixSpamTester::ShouldCheckMessage( const eChatSaidEntry & said ) const
{
    return said.Type() >= eChatMessageType_Public_Direct;
}

//
// Shuffle message spam checks
//

static int se_shuffleSpamMessagesPerRound = 3;
static tSettingItem< int > se_shuffleSpamMessagesPerRoundConf( "SHUFFLE_SPAM_MESSAGES_PER_ROUND", se_shuffleSpamMessagesPerRound );

eShuffleSpamTester::eShuffleSpamTester()
    :numberShuffles_( 0 )
{
}

void eShuffleSpamTester::Shuffle()
{
    numberShuffles_++;
}

void eShuffleSpamTester::Reset()
{
    numberShuffles_ = 0;
}

tString eShuffleSpamTester::ShuffleMessage( ePlayerNetID *player, int fromPosition, int toPosition ) const
{
    tString message;
    message << tOutput( "$team_shuffle", player->GetName(), fromPosition, toPosition );
    
    if ( ShouldDisplaySuppressMessage() )
        message << " " << tOutput( "$team_shuffle_suppress" );
    else
        message << '\n';
    
    return message;
}

bool eShuffleSpamTester::ShouldAnnounce() const
{
    return se_shuffleSpamMessagesPerRound <= 0 || numberShuffles_ < se_shuffleSpamMessagesPerRound;
}

bool eShuffleSpamTester::ShouldDisplaySuppressMessage() const
{
    return se_shuffleSpamMessagesPerRound > 0 && numberShuffles_ + 1 >= se_shuffleSpamMessagesPerRound;
}
