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

#include <vector>
#include <algorithm>

#include "eChat.h"
#include "tSysTime.h"
#include "ePlayer.h"

eChatSaid::eChatSaid(const tString & said, const nTimeRolling & t, eChatMessageType type)
: said_( said ), time_( t ), type_( type )
{
}

eChatSaid::~eChatSaid()
{
}

const tString & eChatSaid::Said() const
{
    return said_;
}

const nTimeRolling & eChatSaid::Time() const
{
    return time_;
}

const eChatMessageType eChatSaid::Type() const
{
    return type_;
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
    eChatLastSaid const & lastSaid = player_->lastSaid_;
    const size_t saidSize = lastSaid.size();
    for ( size_t i = 0; i < saidSize; i++ )
    {
        eChatSaid const & said = lastSaid[i];
        if( (say_.StripWhitespace() == said.Said().StripWhitespace()) && ( (currentTime - said.Time()) < se_alreadySaidTimeout * factor_ ) )
        {
            sn_ConsoleOut( tOutput("$spam_protection_repeat", say_ ), player_->Owner() );
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
    
    // Apply similarity factor
    // if ( !se_IsTeamMessage( say_ ) )
    // {
    //     REAL similarityPercent = 0;
    //     factor *= eSpamSimilarity::SpamScore( say_, player_->lastSaid_, currentTime, similarityPercent );
    //     
    //     if ( CheckSpam( factor, tOutput("$spam_chat") ) )
    //     {
    //         sn_ConsoleOut( tOutput( "$spam_protection_similarity", similarityPercent, static_cast< int>( player_->lastSaid_.size() ) ), player_->Owner() );
    //         return true;
    //     }
    // }
    
    
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
    
    // update last said record
    {
        eChatLastSaid & said = player_->lastSaid_;
        if ( said.size() >= static_cast< size_t >( se_lastSaidMaxEntries ) )
            said.pop_back();
        
        eChatSaid saidEntry( say_, currentTime, lastSaidType_ );
        said.push_front( saidEntry );
    }
    
    return false;
}

bool eChatSpamTester::CheckSpam( REAL factor, tOutput const & message ) const
{
    if ( nSpamProtection::Level_Mild <= player_->chatSpam_.CheckSpam( factor, player_->Owner(), message ) )
        return true;

    return false;
}

class eChatSpamPrefix
{
public:
    eChatSpamPrefix();
    virtual ~eChatSpamPrefix();
};
        

namespace eSpamSimilarity
{
}
