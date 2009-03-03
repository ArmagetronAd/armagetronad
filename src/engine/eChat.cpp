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


class IsPrefixPredicate
{
public:
    IsPrefixPredicate( const tString & s ) :s_( s ) { }
    bool operator() ( const tString & other ) { return s_.StartsWith( other ); }
private:
    const tString s_;
};

class eChatPrefixSpamTester
{
public:
    eChatPrefixSpamTester( ePlayerNetID * player, const eChatSaidEntry & say );
    virtual ~eChatPrefixSpamTester();
    bool Check( tString & out );
private:
    bool ShouldCheckMessage( const eChatMessageType type ) const;

    ePlayerNetID * player_;
    const eChatSaidEntry & say_;
};

// ******************************************************************************************
// *
// *	se_EscapeColors
// *
// ******************************************************************************************
//!
//!     @param      s the string to escape
//!     @return     a string with color codes escaped. Example: 0xfff000 -> #fff000
//!
// ******************************************************************************************
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
    
    // count color codes. We hate them. We really do. (Yeah, this calculation is inefficient.)
    int colorCodes = (say_.Len() - tColoredString::RemoveColors( say_ ).Len())/8;
    if ( colorCodes < 0 ) colorCodes = 0;
    
    
    // apply them to the spam severity factor. Burn in hell, color code abusers.
    static const double log2 = log(2);
    factor *= log( 2 + colorCodes )/log2;
    
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

// The length that a prefix must be for it to count as prefix spam
static int se_prefixSpamMinLength = 3;
static tConfItem< int > se_prefixSpamMinLengthConf( "PREFIX_SPAM_MIN_LENGTH", se_prefixSpamMinLength );

// The number of times the prefix must appear to be considered a prefix
static int se_prefixSpamMinTimesAppeared = 3;
static tConfItem< int > se_prefixSpamMinTimesAppearingConf( "PREFIX_SPAM_MIN_TIMES_APPEARED", se_prefixSpamMinTimesAppeared );


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
    if ( !ShouldCheckMessage( say_.Type() ) )
        return false;

    eChatLastSaid::SaidList & lastSaid = player_->lastSaid_.LastSaid();
    const size_t saidSize = lastSaid.size();
    
    // check from known prefixes
    {
        const eChatLastSaid::StringList & prefixes = player_->lastSaid_.KnownPrefixes();
        eChatLastSaid::StringList::const_iterator it =
            std::find_if( prefixes.begin(), prefixes.end(), IsPrefixPredicate( say_.Said() ) );
        
        if ( it != prefixes.end() )
        {
            out = se_EscapeColors( *it );
            return true;
        }
    }
    
    std::map< int, int > foundPrefixes;
    
    for ( size_t i = 0; i < saidSize; i++ )
    {
        eChatSaidEntry & said = lastSaid[i];
        
        if ( !ShouldCheckMessage( said.Type() ) )
            continue;
        
        if ( say_.Said() == said.Said() )
            continue;
        
        int common = CommonPrefix( say_.Said(), said.Said() );
        
        if ( common >= se_prefixSpamMinLength )
        {
            if ( foundPrefixes.find(common) == foundPrefixes.end() )
                foundPrefixes[common] = 0;

            foundPrefixes[common] += 1;

            if ( foundPrefixes[common] >= se_prefixSpamMinTimesAppeared )
            {
                const tString prefix = say_.Said().SubStr(0, common);
                
                // User is talking to a player. Not prefix spam
                // Example: Player 1: grind center. [etc...]
                {
                    tString possiblePlayer( prefix );
                    
                    // When using 0.3 name completion at the start of a message,
                    // ':' is appended to the end of the player name.
                    if ( possiblePlayer[possiblePlayer.Len()] == ':' )
                        possiblePlayer = possiblePlayer.SubStr( 0, possiblePlayer.Len() - 1 );
                    
                    if ( ePlayerNetID::FindPlayerByName( possiblePlayer ) )
                    {
                        // mark message so we don't need to check it next time
                        said.SetType( eChatMessageType_Public_Direct );
                        return false;
                    }                    
                }
                
                // it is prefix spam
                
                player_->lastSaid_.AddPrefix( prefix );
                out = se_EscapeColors( prefix );
                return true;
            }
        }
    }
    return false;
}

bool eChatPrefixSpamTester::ShouldCheckMessage( const eChatMessageType type ) const
{
    return type >= eChatMessageType_Public;
}









