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

namespace eSpamSimilarity
{
    double SpamScore( tString const &, ePlayerNetID::LastSaid const &, nTimeRolling const &, REAL & );
}

// handles spam checking at the right time
eChatSpamTester::eChatSpamTester( ePlayerNetID * p, tString const & say )
: tested_( false ), shouldBlock_( false ), player_( p ), say_( say ), factor_( 1 )
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
    ePlayerNetID::LastSaid const & lastSaid = player_->lastSaid_;
    const size_t saidSize = lastSaid.size();
    for ( size_t i = 0; i < saidSize; i++ )
    {
        ePlayerNetID::SaidPair const & said = lastSaid[i];
        if( (say_.StripWhitespace() == said.first.StripWhitespace()) && ( (currentTime - said.second) < se_alreadySaidTimeout * factor_ ) )
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
    REAL similarityPercent = 0;
    factor *= eSpamSimilarity::SpamScore( say_, player_->lastSaid_, currentTime, similarityPercent );
    
    if ( CheckSpam( factor, tOutput("$spam_chat") ) )
    {
        sn_ConsoleOut( tOutput( "$spam_protection_similarity", similarityPercent, static_cast< int>( player_->lastSaid_.size() ) ), player_->Owner() );
        return true;
    }
        
    
    
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
        ePlayerNetID::LastSaid & said = player_->lastSaid_;
        if ( said.size() >= static_cast< size_t >( se_lastSaidMaxEntries ) )
            said.pop_back();
        
        ePlayerNetID::SaidPair pair( say_, currentTime );
        said.push_front( pair );
    }
    
    return false;
}

bool eChatSpamTester::CheckSpam( REAL factor, tOutput const & message )
{
    if ( nSpamProtection::Level_Mild <= player_->chatSpam_.CheckSpam( factor, player_->Owner(), message ) )
        return true;

    return false;
}
        

namespace eSpamSimilarity
{
    //! The minimum percent that the similarity score must give for us to consider it matching
    static REAL se_spamSimilarityPercent = 0.90;
    static tConfItem< REAL > confSpamPercent( "SPAM_SIMILARITY_PERCENT", se_spamSimilarityPercent );
    
    //! A general multiplier applied at the end of spam calculation
    static REAL se_spamSimilarityMultiplier = 1;
    static tConfItem< REAL > confSpamMultiplier( "SPAM_SIMILARITY_MULTIPLIER", se_spamSimilarityMultiplier );

    //! The common prefix length score multiplier
    static REAL se_spamSimilarityPrefixMultiplier = 0.15;
    static tConfItem< REAL > confSpamPrefixMultiplier( "SPAM_SIMILARITY_PREFIX_MULTIPLIER", se_spamSimilarityPrefixMultiplier );
    
    typedef std::vector< char > CommonCharT;
    
    template< typename T >
    inline const T & min3( const  T & a, const T & b, const T & c )
    {
        return std::min( a, std::min( b, c ) );
    }
    
    /*!
     * Returns the length of the prefix shared by boths strings, upto “maxPrefix”.
     */
    int CommonPrefix( tString const & s1, tString const & s2, int maxPrefix=6 )
    {
        size_t min = min3( s1.Size(), s2.Size(), maxPrefix );
        
        for ( size_t i = 0; i < min; i++ )
        {
            if ( s1[i] != s2[i] )
                return i;
        }
        return min;
    }
    
    /*!
     * Returns the matching characters from two strings. 
     */
    CommonCharT CommonCharacters( tString const & s1, tString const & s2, int maxDistance )
    {
        CommonCharT common;
        for ( int i = 0; i < s1.Size(); i++ )
        {
            const char & ch = s1[i];
            for ( int j = std::max( 0, i - maxDistance ); j < std::min( i + maxDistance, static_cast< int >( s2.Size() ) ); j++ )
            {
                if ( s2[j] == ch )
                {
                    common.push_back( s1[i] );
                    break;
                }
            }
        }
        return common;
    }
    
    /*!
     * Returns a suitable value for the limiting range in CommonCharacters()
     */
    int MaxDistance( const size_t a, const size_t b )
    {
        double smaller = static_cast< double >( std::min( a, b ) );
        return static_cast< int >( ceil( smaller / 2 ) ) - 1;
    }
    
    /*!
     * Returns the number of transpositions between two common character sets
     */
    int Transpositions( CommonCharT const & common1, CommonCharT const & common2 )
    {
        int transpositions = 0;
        size_t min = std::min( common1.size(), common2.size() );
        for ( size_t i = 0; i < min; i++ )
        {
            if ( common1[i] != common2[i] )
                transpositions++;
        }
        return transpositions / 2;
    }
    
    /*!
     * Returns the Jaro score between two strings
     * <http://en.wikipedia.org/wiki/Jaro-Winkler_distance>
     */
    double JaroScore( tString const & s1, tString const & s2 )
    {
        size_t size1 = s1.Size();
        size_t size2 = s2.Size();
        
        int maxDistance = MaxDistance( size1, size2 );
        
        CommonCharT common1 = CommonCharacters( s1, s2, maxDistance );
        CommonCharT common2 = CommonCharacters( s2, s1, maxDistance );
        
        const size_t commonSize1 = common1.size();
        const size_t commonSize2 = common2.size();
        
        if ( commonSize1 == 0 || commonSize2 == 0 )
            return 0.0;
        
        int transpositions = Transpositions( common1, common2 );
        
        double matching = static_cast< double >( commonSize1 );
        
        return ( ( matching / size1 ) + ( matching / size2 ) + ( ( matching - transpositions ) / matching ) ) / 3.0f;
    }
    
    /*!
     * Returns the Jaro-Winkler score between two strings
     * <http://en.wikipedia.org/wiki/Jaro-Winkler_distance>
     */
    double JaroWinklerScore( tString const & s1, tString const & s2 )
    {
        double jaro = JaroScore( s1, s2 );
        int commonPrefix = CommonPrefix( s1, s2 );
        return jaro + ( commonPrefix * se_spamSimilarityPrefixMultiplier * ( 1 - jaro ) ); 
    }

    
    /*!
     * Returns a spam score, based on similarity, for an input string.
     * 
     * \param say the input string
     * \param lastSaid previously said strings
     * \param currentTime
     * \param percent Set to the average percent similarity found
     */
    double SpamScore( tString const & say, ePlayerNetID::LastSaid const & lastSaid, nTimeRolling const & currentTime, REAL & outPercent )
    {
        outPercent = 0;
        const double log2 = log( 2 );
        double score = 0;
        const size_t saidSize = lastSaid.size();
        for ( size_t i = 0; i < saidSize; i++ )
        {
            ePlayerNetID::SaidPair const & said = lastSaid[i];
            
            double similarity = JaroWinklerScore( say, said.first );
            outPercent = std::max( outPercent, static_cast< REAL>( similarity ) );
            if ( similarity >= se_spamSimilarityPercent )
            {
                // Penalize vs order said
                similarity *= log( saidSize - i ) / log2;
                
                // Penalize vs time
                similarity *= 5.0 / log( currentTime - said.second );
                
                score += similarity;
            }
        }
        
        // Apply user multiplier
        score *= se_spamSimilarityMultiplier;             
        
        return score;
    }    
}