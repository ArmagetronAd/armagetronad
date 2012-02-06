/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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

#include "rSDL.h"

#include "eVoter.h"

#include "tMemManager.h"
#include "tSysTime.h"
#include "tDirectories.h"

#include "uMenu.h"

#include "nConfig.h"
#include "nServerInfo.h"

#include "rConsole.h"
#include "nProtoBuf.h"

#include "ePlayer.h"
#include "eGrid.h"

#include "eVoter.pb.h"

#ifndef DEDICATED
// use server controlled votes (just for the client, to avoid UPGRADE messages)
static bool se_useServerControlledKick = false;
static nSettingItem< bool > se_usc( "VOTE_USE_SERVER_CONTROLLED_KICK", se_useServerControlledKick );
#endif

// basic vote timeout value
static unsigned short se_votingItemID = 0;
static float se_votingTimeout = 300.0f;
static nSettingItem< float > se_vt( "VOTING_TIMEOUT", se_votingTimeout );

// additional timeout for every voter present
static float se_votingTimeoutPerVoter = 0.0f;
static nSettingItem< float > se_vtp( "VOTING_TIMEOUT_PER_VOTER", se_votingTimeoutPerVoter );

static float se_votingStartDecay = 60.0f;
static nSettingItem< float > se_vsd( "VOTING_START_DECAY", se_votingStartDecay );

static float se_votingDecay = 60.0f;
static nSettingItem< float > se_vd( "VOTING_DECAY", se_votingDecay );

// spam level of issuing a vote
static float se_votingSpamIssue = 1.0f;
static nSettingItem< float > se_vsi( "VOTING_SPAM_ISSUE", se_votingSpamIssue );

// spam level of getting your vote rejected
static float se_votingSpamReject = 5.0f;
static nSettingItem< float > se_vsr( "VOTING_SPAM_REJECT", se_votingSpamReject );

static bool se_allowVoting = false;
static tSettingItem< bool > se_av( "ALLOW_VOTING", se_allowVoting );

static bool se_allowVotingSpectator = false;
static tSettingItem< bool > se_avo( "ALLOW_VOTING_SPECTATOR", se_allowVotingSpectator );

// number of rounds to suspend
static int se_suspendRounds = 5;
static tSettingItem< int > se_sr( "VOTING_SUSPEND_ROUNDS", se_suspendRounds );

static int se_minVoters = 3;
static tSettingItem< int > se_mv( "MIN_VOTERS", se_minVoters );

// the number set here always acts as votes against a change.
static int se_votingBias = 0;
static tSettingItem< int > se_vb( "VOTING_BIAS", se_votingBias );

// the number set here always acts as additional votes against a kick vote.
static int se_votingBiasKick = 0;
static tSettingItem< int > se_vbKick( "VOTING_BIAS_KICK", se_votingBiasKick );

// the number set here always acts as additional votes against a suspend vote.
static int se_votingBiasSuspend = 0;
static tSettingItem< int > se_vbSuspend( "VOTING_BIAS_SUSPEND", se_votingBiasSuspend );

// the number set here always acts as additional votes against a referee vote.
static int se_votingBiasReferee = 0;
static tSettingItem< int > se_vbReferee( "VOTING_BIAS_REFEREE", se_votingBiasReferee );

// the number set here always acts as additional votes against a demotereferee vote.
static int se_votingBiasCancelReferee = 0;
static tSettingItem< int > se_vbCancelReferee( "VOTING_BIAS_DEMOTEREFEREE", se_votingBiasCancelReferee );

// the number set here always acts as additional votes against a include vote.
static int se_votingBiasInclude = 0;
static tSettingItem< int > se_vbInclude( "VOTING_BIAS_INCLUDE", se_votingBiasInclude );

// the number set here always acts as additional votes against a command vote.
static int se_votingBiasCommand = 0;
static tSettingItem< int > se_vbCommand( "VOTING_BIAS_COMMAND", se_votingBiasCommand );

// the number set here always acts as additional votes against a command vote.
static int se_votingBiasScramble = 0;
static tSettingItem< int > se_vbScramble( "VOTING_BIAS_SCRAMBLE", se_votingBiasScramble );

// voting privacy level. -2 means total disclosure, +2 total secrecy.
static int se_votingPrivacy = 1;
static tSettingItem< int > se_vp( "VOTING_PRIVACY", se_votingPrivacy );

// maximum number of concurrent votes
static int se_maxVotes = 5;
static tSettingItem< int > se_maxVotesSI( "MAX_VOTES", se_maxVotes );

// maximum number of concurrent votes per voter
static int se_maxVotesPerVoter = 2;
static tSettingItem< int > se_maxVotesPerVoterSI( "MAX_VOTES_PER_VOTER", se_maxVotesPerVoter );

// time between kick votes against the same target in seconds
static int se_minTimeBetweenKicks = 300;
static tSettingItem< int > se_minTimeBetweenKicksSI( "VOTING_KICK_TIME", se_minTimeBetweenKicks );

// time between harmful votes against the same target in seconds
static int se_minTimeBetweenHarms = 180;
static tSettingItem< int > se_minTimeBetweenHarmsSI( "VOTING_HARM_TIME", se_minTimeBetweenHarms );

// time between name changes and you being allowed to issue votes again
static int se_votingMaturity = 300;
static tSettingItem< int > se_votingMaturitySI( "VOTING_MATURITY", se_votingMaturity );

#ifdef KRAWALL_SERVER
// minimal access level for kick votes
static tAccessLevel se_accessLevelVoteKick = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_accessLevelVoteKickSI( "ACCESS_LEVEL_VOTE_KICK", se_accessLevelVoteKick );
static tAccessLevelSetter se_accessLevelVoteKickSILevel( se_accessLevelVoteKickSI, tAccessLevel_Owner );

// minimal access level for suspend votes
static tAccessLevel se_accessLevelVoteSuspend = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_accessLevelVoteSuspendSI( "ACCESS_LEVEL_VOTE_SUSPEND", se_accessLevelVoteSuspend );
static tAccessLevelSetter se_accessLevelVoteSuspendSILevel( se_accessLevelVoteSuspendSI, tAccessLevel_Owner );

// minimal access level for scramble votes
static tAccessLevel se_accessLevelVoteScramble = tAccessLevel_DefaultAuthenticated;
static tSettingItem< tAccessLevel > se_accessLevelVoteScrambleSI( "ACCESS_LEVEL_VOTE_SCRAMBLE", se_accessLevelVoteScramble );
static tAccessLevelSetter se_accessLevelVoteScrambleSILevel( se_accessLevelVoteScrambleSI, tAccessLevel_Owner );

// minimal access level for referee votes
static tAccessLevel se_accessLevelVoteReferee = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteRefereeSI( "ACCESS_LEVEL_VOTE_REFEREE", se_accessLevelVoteReferee );
static tAccessLevelSetter se_accessLevelVoteRefereeSILevel( se_accessLevelVoteRefereeSI, tAccessLevel_Owner );

// minimal access level for demotereferee votes
static tAccessLevel se_accessLevelVoteCancelReferee = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteCancelRefereeSI( "ACCESS_LEVEL_VOTE_DEMOTEREFEREE", se_accessLevelVoteCancelReferee );
static tAccessLevelSetter se_accessLevelVoteCancelRefereeSILevel( se_accessLevelVoteCancelRefereeSI, tAccessLevel_Owner );


// minimal access level for include votes
static tAccessLevel se_accessLevelVoteInclude = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteIncludeSI( "ACCESS_LEVEL_VOTE_INCLUDE", se_accessLevelVoteInclude );
static tAccessLevelSetter se_accessLevelVoteIncludeSILevel( se_accessLevelVoteIncludeSI, tAccessLevel_Owner );

// minimal access level for include votes
static tAccessLevel se_accessLevelVoteIncludeExecute = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteIncludeExecuteSI( "ACCESS_LEVEL_VOTE_INCLUDE_EXECUTE", se_accessLevelVoteIncludeExecute );
static tAccessLevelSetter se_accessLevelVoteIncludeExecuteSILevel( se_accessLevelVoteIncludeExecuteSI, tAccessLevel_Owner );

// minimal access level for direct command votes
static tAccessLevel se_accessLevelVoteCommand = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteCommandSI( "ACCESS_LEVEL_VOTE_COMMAND", se_accessLevelVoteCommand );
static tAccessLevelSetter se_accessLevelVoteCommandSILevel( se_accessLevelVoteCommandSI, tAccessLevel_Owner );

// access level direct command votes will be executed with (minimal level is,
// however, the access level of the vote submitter)
static tAccessLevel se_accessLevelVoteCommandExecute = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelVoteCommandExecuteSI( "ACCESS_LEVEL_VOTE_COMMAND_EXECUTE", se_accessLevelVoteCommandExecute );
static tAccessLevelSetter se_accessLevelVoteCommandExecuteSILevel( se_accessLevelVoteCommandExecuteSI, tAccessLevel_Owner );

#endif

static REAL se_defaultVotesSuspendLength = 3;
static tSettingItem< REAL > se_defaultVotesSuspendLenght_Conf( "VOTES_SUSPEND_DEFAULT", se_defaultVotesSuspendLength );
static REAL se_votesSuspendTimeout = 0;

static eVoter* se_GetVoter( const nSenderInfo & sender )
{
    return eVoter::GetVoter( sender.SenderID(), true );
}

eVoterPlayerInfo::eVoterPlayerInfo(): suspended_(0), silenced_(0){}

static tAccessLevel se_GetAccessLevel( int userID )
{
    tAccessLevel ret = tAccessLevel_Default;

    // scan players of given user ID
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        
        if ( p->Owner() == userID )
        {
            if( p->GetAccessLevel() < ret )
            {
                ret = p->GetAccessLevel();
            }
        }
    }
    
    return ret;
}

static nVersionFeature serverControlledVotesBroken( 10 );
static nVersionFeature serverControlledVotes( 15 );


// something to vote on
class eVoteItem: public tListMember
{
    friend class eMenuItemVote;
public:
    // constructors/destructor
    eVoteItem( void ): creationTime_( tSysTimeFloat() ), user_( 0 ), id_( ++se_votingItemID ), menuItem_( 0 ), total_( 0 )
    {
        items_.Add( this );
    }

    virtual ~eVoteItem( void );

    template< class SELF, class PROTOBUF >
    static bool FillFromMessage( SELF * self, PROTOBUF const & protoBuf, nSenderInfo const & sender )
    {
        // cloak the ID of the sener for privacy
        nCurrentSenderID cloak;
        if ( se_votingPrivacy > 1 )
            cloak.SetID(0);

        if ( !self->DoFillFromMessage( protoBuf, sender ) )
            return false;

        if ( sn_GetNetState() == nSERVER )
        {
            if ( !self->CheckValid( sender.SenderID() ) )
                return false;
        }

        self->ReBroadcast( sender.SenderID() );

        return true;
    }

    
    void ReBroadcast( int exceptTo )
    {
        // rebroadcast message to all non-voters that may be able to vote
        if ( sn_GetNetState() == nSERVER )
        {
            // prepare message
            tOutput voteMessage;
            voteMessage.SetTemplateParameter( 1, suggestor_->Name( user_ ) );
            voteMessage.SetTemplateParameter( 2, GetDescription() );
            voteMessage << "$vote_submitted";

            // print it
            if ( se_votingPrivacy <= -1 )
            {
                sn_ConsoleOut( voteMessage );	// broadcast it
            }
            else
            {
                if ( exceptTo > 0 )
                {
                    sn_ConsoleOut( voteMessage, exceptTo );	// inform submitter
                }

                if ( se_votingPrivacy <= 1 )
                {
                    con << voteMessage;				// print it for the server admin
                }
            }

            // create messages for old and new clients
            tJUST_CONTROLLED_PTR< nMessageBase > retNew = this->CreateMessage();
            tJUST_CONTROLLED_PTR< nMessageBase > retLegacy = this->CreateMessageLegacy();

            // set so every voter ony gets each vote once
            std::set< eVoter * > sentTo;
            total_ = 1;

            for ( int i = MAXCLIENTS; i > 0; --i )
            {
                eVoter * voter = eVoter::GetVoter( i );
                if ( sn_Connections[ i ].socket && i != exceptTo && 0 != voter && 
                     sentTo.find(voter) == sentTo.end() )
                {

                    if ( serverControlledVotes.Supported( i ) )
                    {
                        sentTo.insert(voter);
                        retNew->Send( i );
                        total_++;
                    }
                    else if ( retLegacy )
                    {
                        sentTo.insert(voter);
                        retLegacy->Send( i );
                        total_++;
                    }
                }
            }
            //			item->SendMessage();

            if ( suggestor_ )
                suggestor_->Spam( exceptTo, se_votingSpamIssue, tOutput("$spam_vote_kick_issue") );
        }

        con << tOutput( "$vote_new", GetDescription() );

        this->Evaluate();
    }

    virtual nMessageBase * CreateMessage( void ) const = 0 ;

    virtual nMessageBase * CreateMessageLegacy( void ) const
    {
        return NULL;
    }

    void SendMessage( void ) const
    {
        this->CreateMessage()->BroadCast();
    }

    // message sending
    void Vote( bool accept );														// called on the clients to accept or decline the vote

    static bool AcceptNewVote( eVoter * voter, int senderID )										// check if a new voting item should be accepted
    {
        // cloak the ID of the sener for privacy
        nCurrentSenderID cloak;
        if ( se_votingPrivacy > 0 )
            cloak.SetID(0);

        int i;

        // let old messages time out
        for ( i = items_.Len()-1; i>=0; --i )
        {
            items_[i]->Evaluate();
        }

        // always accept in client mode
        if ( sn_GetNetState() == nCLIENT )
            return true;

        // check if voting is allowed
        if ( !voter )
        {
            return false;
        }

        // reject voting
        if ( !se_allowVoting )
        {
            tOutput message("$vote_disabled");
            sn_ConsoleOut( message, senderID );
            return false;
        }

        // enough voters online?
        if ( eVoter::voters_.Len() < se_minVoters || eVoter::voters_.Len() < 2 )
        {
            tOutput message("$vote_toofew");
            sn_ConsoleOut( message, senderID );
            return false;
        }

        // check for spam
        if ( voter->IsSpamming( senderID ) )
        {
            return false;
        }

        // count number of votes by the voter
        int voteCount = 0;
        for ( i = items_.Len()-1; i>=0; --i )
        {
            eVoteItem * other = items_[i];
            if ( other->suggestor_ == voter )
                voteCount ++;
        }
        if ( voteCount >= se_maxVotesPerVoter )
        {
            tOutput message("$vote_overflow");
            sn_ConsoleOut( message, senderID );
            return false;
        }

        if ( items_.Len() < se_maxVotes )
        {
            return true;
        }
        else
        {
            tOutput message("$vote_overflow");
            sn_ConsoleOut( message, senderID );
            return false;
        }
    }

    static bool AcceptNewVote( nSenderInfo const & sender )										// check if a new voting item should be accepted
    {
        return AcceptNewVote( se_GetVoter( sender ), sender.SenderID() );
    }

    void RemoveVoter( eVoter* voter )
    {
        // remove voter from the lists
        for ( int res = 1; res >= 0; --res )
            this->voters_[ res ].Remove( voter );
    }

    void RemoveVoterCompletely( eVoter* voter )
    {
        RemoveVoter( voter );
        if ( suggestor_ == voter )
        {
            suggestor_ = 0;
            user_ = 0;
        }
    }

    // message receival
    static void GetControlMessage( Engine::VoteSubmission const & submission, nSenderInfo const & sender )								   	// handles a voting message
    {
        if ( sn_GetNetState() == nSERVER )
        {
            unsigned short id = submission.vote_id();
            bool result       = submission.accept();

            for ( int i = items_.Len()-1; i>=0; --i )
            {
                eVoteItem* vote = items_[i];
                if ( vote->id_ == id )
                {
                    // found the vote; find the voter
                    tCONTROLLED_PTR( eVoter ) voter = se_GetVoter( sender );
                    if ( voter )
                    {
                        // prepare message
                        tOutput voteMessage;
                        voteMessage.SetTemplateParameter( 1, voter->Name( sender.SenderID() ) );
                        voteMessage.SetTemplateParameter( 2, vote->GetDescription() );
                        if ( result )
                            voteMessage << "$vote_vote_for";
                        else
                            voteMessage << "$vote_vote_against";

                        // print it
                        if ( se_votingPrivacy <= -2 )
                            sn_ConsoleOut( voteMessage );	// broadcast it
                        else if ( se_votingPrivacy <= 0 )
                            con << voteMessage;				// print it for the server admin
                        else
                        {
                            sn_ConsoleOut( voteMessage, sender.SenderID() );
                        }

                        // remove him from the lists
                        vote->RemoveVoter( voter );

                        // insert hum
                        vote->voters_[ result ].Insert( voter );
                    }

                    // are enough votes cast?
                    vote->Evaluate();
                    return;
                }
            }
        }
    }

    // information
    void GetStats( int& pro, int& con, int& total ) const						// returns voting statistics about this item
    {
        pro = voters_[1].Len();
        con = voters_[0].Len();
        total = total_;
    }

    // message
    void BroadcastMessage( const tOutput& message ) const
    {
        if ( sn_GetNetState() == nSERVER )
        {
            tOutput m;
            m.SetTemplateParameter( 1, this->GetDescription() );
            m.Append( message );

            sn_ConsoleOut( m );
        }
    }

    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return tAccessLevel_Default;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return 0;
    }

    // evaluation
    virtual void Evaluate()																	// check if this voting item is to be kept around
    {
        int pro, con, total;

        GetStats( pro, con, total );

        if ( sn_GetNetState() == nSERVER )
        {
            // see if there are enough voters
            if ( total < se_minVoters || total < 2 )
            {
                this->BroadcastMessage( tOutput("$vote_toofew") );
                delete this;
                return;
            }
        }

        int bias = se_votingBias + DoGetExtraBias();

        // apply bias
        con 	+= bias;
        total 	+= bias;

        // reduce number of total voters
        if ( se_votingDecay > 0 )
        {
            int reduce = int( ( tSysTimeFloat() - this->creationTime_ - se_votingStartDecay ) / se_votingDecay );
            if ( reduce > 0 )
            {
                total -= reduce;
            }
        }

        if ( sn_GetNetState() == nSERVER )
        {
            // see if the vote has been rejected
            if ( con >= pro && con * 2 >= total )
            {
                if ( this->suggestor_ )
                    this->suggestor_->Spam( user_, se_votingSpamReject, tOutput("$spam_vote_rejected") );

                tOutput voteMessage("$vote_rejected", GetDescription(), pro, con-bias, total-pro-con+bias );
                this->BroadcastMessage( voteMessage );
                delete this;
                return;
            }

            // see if the vote has been accepted
            if ( pro >= con && pro * 2 > total )
            {
                tOutput voteMessage("$vote_accepted", GetDescription(), pro, con-bias, total-pro-con+bias);
                this->BroadcastMessage( voteMessage );
                this->DoExecute();
                delete this;
                return;
            }
        }

        // see if the voting has timed out
        int relevantNumVoters = sn_GetNetState() == nCLIENT ? se_PlayerNetIDs.Len() + MAXCLIENTS : eVoter::voters_.Len(); // the number of voters (overestimate the value on the client)
        if ( this->creationTime_ < tSysTimeFloat() - se_votingTimeout - se_votingTimeoutPerVoter * relevantNumVoters )
        {
            this->BroadcastMessage( tOutput( "$vote_timeout" ) );

            delete this;
            return;
        }
    }

    // accessors
    static const tList< eVoteItem >& GetItems() { return items_; }					// returns the list of all items
    inline eVoter* GetSuggestor() const { return suggestor_; }						// returns the voter that suggested the item
    inline tString GetDescription() const{ return this->DoGetDescription(); }		// returns the description of the voting item
    inline tString GetDetails() const{ return this->DoGetDetails(); }               // returns the detailed description of the voting item

    unsigned short GetID(){ return id_; }
    void UpdateMenuItem();                // update the menu item about a status change

    // checks whether the vote is a valid vote to make
    bool CheckValid( int senderID )
    {
        if ( sn_GetNetState() != nSERVER )
        {
            return true;
        }

        if ( se_votesSuspendTimeout > tSysTimeFloat() )
        {
            sn_ConsoleOut(tOutput("$vote_rejected_voting_suspended"),
                          senderID );

            return false;
        }

        // fill suggestor
        if ( !suggestor_ )
        {
            suggestor_ = eVoter::GetVoter( senderID );
            if ( !suggestor_ )
                return false;

            // add suggestor to supporters
            this->voters_[1].Insert( suggestor_ );

            user_ = senderID;
        }

        // check access level
        tAccessLevel accessLevel = se_GetAccessLevel( senderID );
        if ( accessLevel < tCurrentAccessLevel::GetAccessLevel() )
        {
            accessLevel = tCurrentAccessLevel::GetAccessLevel();
        }

        tAccessLevel required = DoGetAccessLevel();
        if ( accessLevel > required )
        {
            sn_ConsoleOut(tOutput("$player_vote_accesslevel",
                                  tCurrentAccessLevel::GetName( accessLevel ),
                                  tCurrentAccessLevel::GetName( required ) ),
                          senderID );
            
            return false;
        }

        return DoCheckValid( senderID );
    }

    virtual void Update() //!< update description and details
    {}
protected:
    bool DoFillFromMessage( Engine::VoteItem const & item, nSenderInfo const & sender )
    {
        // get user
        user_ = sender.SenderID();

        // get originator of vote
        if(sn_GetNetState()!=nSERVER)
        {
            id_ = item.vote_id();
        }

        return true;
    }

    virtual bool DoCheckValid( int senderID ){ return true; }

    virtual void DoFillToMessage( Engine::VoteItem & item ) const
    {
        if(sn_GetNetState()==nSERVER)
        {
            // write our message ID
            item.set_vote_id( id_ );
        }
    }

protected:
    virtual tString DoGetDetails() const 		    // returns the detailed description of the voting item
    {
        tString ret;
        if ( se_votingPrivacy <= -1 && bool( suggestor_ ) )
            ret << tOutput( "$vote_submitter_text", suggestor_->Name( user_ ) ) << " ";

        return ret;
    }
    static tList< eVoteItem > items_;				// list of vote items
private:
    virtual tString DoGetDescription() const = 0;		// returns the description of the voting item
    virtual void DoExecute() = 0;						// called when the voting was successful

    nTimeAbsolute creationTime_;					// time the vote was cast
    tCONTROLLED_PTR( eVoter ) suggestor_;			// the voter suggesting the vote
    unsigned int user_;								// user suggesting the vote
    tArray< tCONTROLLED_PTR( eVoter ) > voters_[2];	// array of voters approving or disapproving of the vote
    unsigned short id_;								// running id of voting item
    eMenuItemVote *menuItem_;						// menu item
    int total_;                                     // total number of voters aware of this item

    eVoteItem& operator=( const eVoteItem& );
    eVoteItem( const eVoteItem& );
};

tList< eVoteItem > eVoteItem::items_;				// list of vote items

void se_CancelAllVotes( bool announce )
{
    if ( sn_GetNetState() == nCLIENT )
    {
        return;
    }

    if (announce)
    {
        sn_ConsoleOut( tOutput( "$vote_cancel_all" ) );
    }

    tList< eVoteItem > const & items = eVoteItem::GetItems();
    
    while ( items.Len() )
    {
        delete items(0);
    }
}

void se_CancelAllVotes( std::istream & )
{
	se_CancelAllVotes ( true );
}

static tConfItemFunc se_cancelAllVotes_conf( "VOTES_CANCEL", &se_CancelAllVotes );





void se_votesSuspend( REAL minutes, bool announce, std::istream & s )
{
    if ( sn_GetNetState() == nCLIENT )
    {
        return;
    }

    if ( minutes > 0)
    {
        s >> minutes;
    }

    se_CancelAllVotes( false );

    se_votesSuspendTimeout = tSysTimeFloat() + ( minutes * 60 );

    if ( announce && minutes > 0 )
    {
        sn_ConsoleOut( tOutput( "$voting_suspended", minutes ) );
    }
    else if ( announce && minutes <= 0 )
    {
        sn_ConsoleOut( tOutput( "$voting_unsuspended" ) );
    }

}

void se_SuspendVotes( std::istream & s )
{
    se_votesSuspend(  se_defaultVotesSuspendLength, true, s );
}
void se_UnSuspendVotes( std::istream & s )
{
    se_votesSuspend( 0, true, s );
}

static tConfItemFunc se_suspendVotes_conf( "VOTES_SUSPEND", &se_SuspendVotes );
static tConfItemFunc se_unSuspendVotes_conf( "VOTES_UNSUSPEND", &se_UnSuspendVotes );


static nProtoBufDescriptor< Engine::VoteSubmission > se_voteSubmissionDescriptor( 230, eVoteItem::GetControlMessage );

// called on the clients to accept or decline the vote
void eVoteItem::Vote( bool accept )
{
    Engine::VoteSubmission & submission = se_voteSubmissionDescriptor.Broadcast();
    submission.set_vote_id( id_ );
    submission.set_accept( accept );

    delete this;
}

// ****************************************************************************************
// ****************************************************************************************

// voting decision
enum Vote
{
    Vote_Approve,
    Vote_Reject,
    Vote_DontMind
};

#ifdef _MSC_VER
#pragma warning ( disable: 4355 )
#endif

// menu item to silence selected players
class eMenuItemVote: public uMenuItemSelection< Vote >
{
    friend class eVoteItem;
    friend class eVoteItemServerControlled;

public:
    eMenuItemVote(uMenu *m, eVoteItem* v )
            : uMenuItemSelection< Vote >( m, tOutput(""), tOutput("$vote_help"), vote_ )
            , item_( v )
            , vote_ ( Vote_DontMind )
            , reject_	( *this, "$vote_reject"		, "$vote_reject_help"		, Vote_Reject 	)
            , dontMind_	( *this, "$vote_dont_mind"	, "$vote_dont_mind_help"	, Vote_DontMind )
            , approve_	( *this, "$vote_approve"	, "$vote_approve_help"		, Vote_Approve 	)
    {
        tASSERT( v );

        if ( v )
        {
            v->menuItem_ = this;
            v->UpdateMenuItem();
        }
    }

    ~eMenuItemVote()
    {
        if ( item_ )
        {
            item_->menuItem_ = 0;

            switch ( vote_ )
            {
            case Vote_Approve:
                item_->Vote( true );
                break;
            case Vote_Reject:
                item_->Vote( false );
                break;
            default:
                break;
            }
        }
    }

private:
    eVoteItem* item_;										// vote item
    Vote vote_;												// result
    uSelectEntry< Vote >  reject_, dontMind_, approve_;		// selection entries
};

// **************************************************************************************
// **************************************************************************************

static void se_VoteChangesHandler( Engine::VoteChanges const & change, nSenderInfo const & sender );
static nProtoBufDescriptor< Engine::VoteChanges > server_vote_expired_handler( 233, se_VoteChangesHandler );

static void se_HandleNewServerVote( Engine::VoteItemServerControlled const & vote, nSenderInfo const & sender );
static nProtoBufDescriptor< Engine::VoteItemServerControlled > new_server_se_voteSubmissionDescriptor(232,se_HandleNewServerVote );

// something to vote on: completely controlled by the server
class eVoteItemServerControlled: public virtual eVoteItem
{
public:
    // constructors/destructor
    eVoteItemServerControlled()
            : description_( "No Info" )
            , details_( "No Info" )
            , expired_( false )
    {
    }

    eVoteItemServerControlled( tString const & description, tString const & details )
            : description_( description )
            , details_( details )
            , expired_( false )
    {}

    ~eVoteItemServerControlled()
    {
        if ( sn_GetNetState() == nSERVER )
        {
            expired_ = true;
            SendChanged();
        }
    }

    static void s_HandleChanged( Engine::VoteChanges const & change, nSenderInfo const & sender )
    {
        unsigned short id = change.vote_id();
        for ( int i = items_.Len()-1; i>=0; --i )
        {
            eVoteItem* vote = items_[i];
            if ( vote->GetID() == id )
            {
                eVoteItemServerControlled * vote2 = dynamic_cast< eVoteItemServerControlled * >( vote );
                if ( vote2 )
                    vote2->HandleChanged( change );
            }
        }
    }

    void HandleChanged( Engine::VoteChanges const & change )
    {
        expired_     = change.expired();
        description_ = change.properties().description();
        details_     = change.properties().details();

        Update();
        UpdateMenuItem();
    }

    void SendChanged()
    {
        Engine::VoteChanges & change = server_vote_expired_handler.Broadcast( serverControlledVotes );
        change.set_vote_id( GetID() );
        change.set_expired( expired_ );
        change.mutable_properties()->set_description( description_ );
        change.mutable_properties()->set_details( details_ );
    }

    bool DoFillFromMessage( Engine::VoteItemServerControlled const & item, nSenderInfo const & sender )
    {
        description_ = item.properties().description();
        details_     = item.properties().details();
        return eVoteItem::DoFillFromMessage( item.base(), sender );
    }

    void DoFillToMessage( Engine::VoteItemServerControlled & item ) const
    {
        item.mutable_properties()->set_description( description_ );
        item.mutable_properties()->set_details( details_ );

        eVoteItem::DoFillToMessage( *item.mutable_base() );
    }

    virtual nMessageBase * CreateMessage() const
    {
        nProtoBufMessage< Engine::VoteItemServerControlled > * m = new_server_se_voteSubmissionDescriptor.CreateMessage();
        DoFillToMessage( m->AccessProtoBuf() );

        return m;
    }

    virtual void DoExecute(){}						// called when the voting was successful
protected:
    virtual void Evaluate()
    {
        // update clients (i.e. if a player to be kicked changed his name)
        if ( sn_GetNetState() == nSERVER )
        {
            Update();
            SendChanged();
        }

        if ( expired_ )
            delete this;
        else
            eVoteItem::Evaluate();
    }

    virtual tString DoGetDescription() const		// returns the description of the voting item
    {
        return expired_ ? tString("Expired vote") : description_;
    }

    virtual tString DoGetDetails() const		    // returns the detailed description of the voting item
    {
        return expired_ ? tString("Expired vote") : details_;
    }
protected:
    mutable tString description_;              //!< the description of the vote
    mutable tString details_;                  //!< details on the vote
private:
    bool expired_;                             //!< flag set when the vote expired on the server
};

static void se_VoteChangesHandler( Engine::VoteChanges const & change, nSenderInfo const & sender )
{
    eVoteItemServerControlled::s_HandleChanged( change, sender );
}

static void se_HandleNewServerVote( Engine::VoteItemServerControlled const & vote, nSenderInfo const & sender )
{
    if ( sn_GetNetState() != nCLIENT ||  eVoteItem::AcceptNewVote( sender ) )
    {
        // accept message
        eVoteItemServerControlled * item = tNEW( eVoteItemServerControlled )();
        if ( !eVoteItem::FillFromMessage( item, vote, sender ) )
            delete item;
    }
}

// *******************************************************************************************
// *******************************************************************************************

class nMachineObserver: public nMachineDecorator
{
public:
    nMachineObserver( nMachine & machine )
            : nMachineDecorator( machine ), machine_( &machine ){}

    nMachine * GetMachine()
    {
        return machine_;
    }
protected:
    virtual void OnDestroy()
    {
        machine_ = 0;
    }
private:
    nMachine * machine_;
};

static tString se_voteKickToServer("");
static int se_voteKickToPort = 4534;
static tSettingItem< tString > se_voteKickToServerConf( "VOTE_KICK_TO_SERVER", se_voteKickToServer );
static tSettingItem< int > se_voteKickToPortConf( "VOTE_KICK_TO_PORT", se_voteKickToPort );

// minimal previous harmful votes (kick, silence, suspend) before
// a successful kick vote really results in a kick. Before that, the result is a
// suspension.
static int se_kickMinHarm = 0;
static tSettingItem< int > se_kickMinHarmSI( "VOTING_KICK_MINHARM", se_kickMinHarm );

// reason given on vote kicks
static tString se_voteKickReason("");
static tConfItemLine se_voteKickReasonConf( "VOTE_KICK_REASON", se_voteKickReason );

void se_VoteKickUser( int user )
{
    if ( user == 0 )
    {
        return;
    }

    tString reason;
    if ( se_voteKickReason.Len() >= 2 )
    {
        reason = se_voteKickReason;
    }
    else
    {
        reason = tOutput("$voted_kill_kick");
    }

    if ( se_voteKickToServer.Len() < 2 )
    {
        sn_KickUser( user, reason );
    }
    else
    {
        // kick player to default destination
        nServerInfoRedirect redirect( se_voteKickToServer, se_voteKickToPort );
        sn_KickUser( user, reason, 1, &redirect );
    }
}

void se_VoteKickPlayer( ePlayerNetID * p )
{
    if ( !p )
    {
        return;
    }

    se_VoteKickUser( p->Owner() );
}

static void se_VoteItemHarmHandler( Engine::VoteItemHarm const & kick, nSenderInfo const & sender );
static nProtoBufDescriptor< Engine::VoteItemHarm > se_voteItemHarmDescriptor( 231, se_VoteItemHarmHandler );

// something to vote on: harming a player
class eVoteItemHarm: public virtual eVoteItem
{
public:
    // constructors/destructor
    eVoteItemHarm( ePlayerNetID* player = 0 )
            : player_( player )
            , machine_(NULL)
            , name_( "(Player who already left)" )
    {}

    ~eVoteItemHarm()
    {
        delete machine_;
        machine_ = NULL;
    }

    // returns the player that is to be harmed
    ePlayerNetID * GetPlayer() const
    {
        ePlayerNetID const * player = player_;
        return const_cast< ePlayerNetID * >( player );
    }

    bool DoFillFromMessage( Engine::VoteItemHarm const & harm, nSenderInfo const & sender )
    {
        // read player ID
        unsigned short id = harm.player_id();
        nNetObject::IDToPointer( id, player_ );

        return eVoteItem::DoFillFromMessage( harm.base(), sender );
    }

protected:
    // lock legacy message creation to regular creation of this class
    virtual nMessageBase * CreateMessageLegacy() const
    {
        return eVoteItemHarm::CreateMessage();
    }

    virtual nMessageBase * CreateMessage( void ) const
    {
        nProtoBufMessage< Engine::VoteItemHarm > * m = se_voteItemHarmDescriptor.CreateMessage();
        DoFillToMessage( m->AccessProtoBuf() );
        return m;
    }

    virtual bool DoCheckValid( int senderID )
    {
        // always accept votes from server
        if ( sn_GetNetState() == nCLIENT && senderID == 0 )
        {
            return true;
        }

        eVoter * sender = eVoter::GetVoter( senderID  );

        double time = tSysTimeFloat();

        // get the oldest player from the sending client
        double timeCreated = time;
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID * senderPlayer = se_PlayerNetIDs(i);
            if( senderPlayer->Owner() == senderID && senderPlayer->GetTimeCreated() < timeCreated )
            {
                timeCreated = senderPlayer->GetTimeCreated();
            }
        }

        // don't let the player call a vote if s/he is silenced.
        if ( player_->IsSilenced() )
        {
            tOutput message("$vote_silenced");
            sn_ConsoleOut( message, player_->Owner() );
            return false;
        }

        // check whether the issuer is allowed to start a vote
        if ( sender && player_ && timeCreated + se_votingMaturity > time )
        {
            REAL timeLeft = timeCreated + se_votingMaturity - time;
            tOutput message( "$vote_maturity", timeLeft );
            sn_ConsoleOut( message, senderID );
            return false;
        }

        // prevent the sender from changing his name for confusion
        if ( sender )
            sender->lastNameChangePreventor_ = time;

        // check if player is protected from kicking
        if ( player_ && sn_GetNetState() != nCLIENT )
        {
            // check whether the player is on the server
            if ( player_->Owner() == 0 )
            {
                sn_ConsoleOut( tOutput( "$vote_kick_local", player_->GetName() ), senderID );
                return false;
            }

            name_ = player_->GetName();
            eVoter * voter = eVoter::GetVoter( player_->Owner() );
            if ( voter )
            {
                machine_ = tNEW( nMachineObserver )( voter->machine_ );

                if ( time < voter->lastHarmVote_ + se_minTimeBetweenHarms )
                {
                    tOutput message("$vote_redundant");
                    sn_ConsoleOut( message, senderID );
                    return false;
                }
                else
                {
                    voter->lastHarmVote_ = time;
                    voter->lastNameChangePreventor_ = time;
                }

                // count harmful votes
                voter->harmCount_++;
            }
        }

        return eVoteItem::DoCheckValid( senderID );
    }

    void DoFillToMessage( Engine::VoteItemHarm & harm ) const
    {
        harm.set_player_id( nNetObject::PointerToID( player_ ) );

        eVoteItem::DoFillToMessage( *harm.mutable_base() );
    }

protected:
    // get the language string prefix
    virtual char const * DoGetPrefix() const = 0;

    virtual tString DoGetDescription() const		// returns the description of the voting item
    {
        // get name from player
        if ( player_ )
            name_ = player_->GetName();

        return tString( tOutput( tString("$") + DoGetPrefix() + "_player_text", name_ ) );
    }

    virtual tString DoGetDetails() const		    // returns the detailed description of the voting item
    {
        // get name from player
        if ( player_ )
            name_ = player_->GetName();

        return eVoteItem::DoGetDetails() + tString( tOutput( tString("$") + DoGetPrefix() + "_player_details_text", name_ ) );
    }

    nMachine * GetMachine() const
    {
        if ( !machine_ )
        {
            return 0;
        }
        else
        {
            return machine_->GetMachine();
        }
    }
private:
    nObserverPtr< ePlayerNetID > player_;		// keep player referenced
    nMachineObserver * machine_;                // pointer to the machine of the player to be kicked
    mutable tString name_;                      // the name of the player to be kicked
};

// something to vote on: kicking a player
class eVoteItemKick: public virtual eVoteItemHarm
{
public:
    // constructors/destructor
    eVoteItemKick( ePlayerNetID* player )
        : eVoteItemHarm( player )
    {}

    ~eVoteItemKick()
    {}

protected:
    // get the language string prefix
    virtual char const * DoGetPrefix() const{ return "kick"; }

#ifdef KRAWALL_SERVER
    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteKick;
    }
#endif

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasKick;
    }

    virtual bool DoCheckValid( int senderID )
    {
        ePlayerNetID * player = GetPlayer();

        // check if player is protected from kicking
        if ( player && sn_GetNetState() != nCLIENT )
        {
            eVoter * voter = eVoter::GetVoter( player->Owner() );
            if ( voter )
            {
                double time = tSysTimeFloat();
                if ( time < voter->lastKickVote_ + se_minTimeBetweenKicks )
                {
                    tOutput message("$vote_redundant");
                    sn_ConsoleOut( message, senderID );
                    return false;
                }
                else
                {
                    voter->lastKickVote_ = time;
                    voter->lastNameChangePreventor_ = time;
                }
            }
        }

        return eVoteItemHarm::DoCheckValid( senderID );
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        ePlayerNetID * player = GetPlayer();
        nMachine * machine = GetMachine();
        if ( player )
        {
            // kick the player, he is online
            se_VoteKickPlayer( player );
        }
        else if ( machine )
        {
            // the player left. Inform the machine that he would have gotten kicked.
            // kick all players that connected from that machine
            bool kick = false;
            for ( int user = MAXCLIENTS; user > 0; --user )
            {
                if ( &nMachine::GetMachine( user ) == machine )
                {
                    se_VoteKickUser( user );
                    kick = true;
                }
            }
            
            // if no user could be kicked, notify at least the machine that
            // somebody would have been kicked.
            if ( !kick )
            {
                machine->OnKick();
            }
        }
    }

private:
};

// harming vote items, server controlled
class eVoteItemHarmServerControlled: public virtual eVoteItemServerControlled, public virtual eVoteItemHarm
{
public:
    // constructors/destructor
    eVoteItemHarmServerControlled( ePlayerNetID* player = 0 )
            : eVoteItemHarm( player )
    {}

    ~eVoteItemHarmServerControlled()
    {}
protected:
    virtual nMessageBase * CreateMessage() const
    {
        return eVoteItemServerControlled::CreateMessage();
    }

    bool DoFillFromMessage( Engine::VoteItemHarm const & harm, nSenderInfo const & sender )
    {
        // delegate
        bool ret = eVoteItemHarm::DoFillFromMessage( harm, sender );

        // fill in description
        Update();

        return ret;
    }

    void DoFillToMessage( Engine::VoteItemHarm & harm ) const
    {
        // should never be called on the client
        tASSERT( sn_GetNetState() != nCLIENT );

        eVoteItemHarm::DoFillToMessage( harm );
    }
private:
    virtual void Update() //!< update description and details
    {
        description_ = eVoteItemHarm::DoGetDescription();
        details_ = eVoteItemHarm::DoGetDetails();
    }

    virtual tString DoGetDescription() const		// returns the description of the voting item
    {
        return eVoteItemServerControlled::DoGetDescription();
    }

    virtual tString DoGetDetails() const		// returns the detailed description of the voting item
    {
        return eVoteItemServerControlled::DoGetDetails();
    }
};

// remove vote items, server controlled
class eVoteItemSuspend: public virtual eVoteItemHarmServerControlled
{
public:
    // constructors/destructor
    eVoteItemSuspend( ePlayerNetID* player = 0 )
        : eVoteItemHarm( player )
        {}

    ~eVoteItemSuspend()
    {}
protected:
    // get the language string prefix
    virtual char const * DoGetPrefix() const{ return "suspend"; }

#ifdef KRAWALL_SERVER
    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteSuspend;
    }
#endif

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasSuspend;
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        ePlayerNetID * player = GetPlayer();
        if ( player )
        {
            player->Suspend( se_suspendRounds );
        }
    }
};

// kick vote items, server controlled
class eVoteItemKickServerControlled: public virtual eVoteItemHarmServerControlled, public virtual eVoteItemKick
{
public:
    // constructors/destructor
    eVoteItemKickServerControlled( bool fromMenu, ePlayerNetID* player )
    : eVoteItemHarm( player ), eVoteItemKick( player ), fromMenu_( fromMenu )
    {}

    ~eVoteItemKickServerControlled()
    {}
protected:
    virtual bool DoCheckValid( int senderID )
    {
        // check whether enough harmful votes were collected already
        ePlayerNetID * p = GetPlayer();
        if ( p && !p->GetVoter() )
        {
            p->CreateVoter();
        }

        if ( fromMenu_ && p && p->GetVoter() && p->GetVoter()->HarmCount() < se_kickMinHarm )
        {
            // try to transfor the vote to a suspension
            eVoteItem * item = tNEW ( eVoteItemSuspend )( p );
            
            // let item check its validity
            if ( !item->CheckValid( senderID ) )
            {
                delete item;
            }
            else
            {
                // no objection? Broadcast it to everyone.
                item->Update();
                item->ReBroadcast( senderID );
            }

            // and cancel this item here.
            return false;
        }

        // no transformation needed or transformation failed. Proceed as usual.
        return eVoteItemHarm::DoCheckValid( senderID );
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        eVoteItemKick::DoExecute();
    }
private:
    bool fromMenu_; // flag set if the vote came from the menu
};

static void se_VoteItemHarmHandler( Engine::VoteItemHarm const & kick, nSenderInfo const & sender )
{
    // set high default access level for menu issued kick votes, the true access level
    // is taken from the highest level player from the sending client later
    tCurrentAccessLevel level( tAccessLevel_Owner, true );

    // accept message
    if ( eVoteItem::AcceptNewVote( sender ) )
    {
        eVoteItemHarm * item = tNEW( eVoteItemKickServerControlled )( true, 0 );
        if ( !eVoteItem::FillFromMessage( item, kick, sender ) )
        {
            delete item;
            return;
        }
    }
}

static void se_SendKick( ePlayerNetID* p )
{
    eVoteItemKick kick( p );
    kick.SendMessage();
}

#ifdef DEDICATED
#ifdef KRAWALL_SERVER

// console with filter for redirection to anyone with a certain access level
class eAccessConsoleFilter: public tConsoleFilter
{
public:
    eAccessConsoleFilter( tAccessLevel level )
            :level_( level )
    {
    }

    void Send()
    {
        bool canSee[ MAXCLIENTS+1 ];
        for( int i = MAXCLIENTS; i>=0; --i )
        {
            canSee[i] = false;
        }

        // look which clients have someone who can see the message
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);
            if ( player->GetAccessLevel() <= level_ )
            {
                canSee[ player->Owner() ] = true;
            }
        }

        // and send it
        for( int i = MAXCLIENTS; i>=0; --i )
        {
            if ( canSee[i] )
            {
                sn_ConsoleOut( message_, i );
            }
        }

        message_.Clear();
    }

    ~eAccessConsoleFilter()
    {
        Send();
    }
private:
    // we want to come first, the admins should get unfiltered output
    virtual int DoGetPriority() const{ return -100; }

    // don't actually filter; take line and add it to the message sent to the admin
    virtual void DoFilterLine( tString &line )
    {
        //tColoredString message;
        message_ << tColoredString::ColorString(1,.3,.3) << "RA: " << tColoredString::ColorString(1,1,1) << line << "\n";

        // don't let message grow indefinitely
        if (message_.Len() > 600)
        {            Send();
        }
    }

    tAccessLevel level_;     // the access level required 
    tColoredString message_; // the console message for the remote administrator
};

// include vote items
class eVoteItemInclude: public eVoteItemServerControlled
{
public:
    // constructors/destructor
    eVoteItemInclude( tString const & file, tAccessLevel submitterLevel )
    : eVoteItemServerControlled()
    , file_( file )
    {
        description_ = tOutput( "$vote_include_text", file );
        file_ = tString( "vote/" ) + file_;
        details_ = tOutput( "$vote_include_details_text", file_ );

        if ( submitterLevel > se_accessLevelVoteIncludeExecute )
        {
            submitterLevel = se_accessLevelVoteIncludeExecute;
        }
        level_ = submitterLevel;
    }

    ~eVoteItemInclude()
    {}
protected:
    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteInclude;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasInclude;
    }

    bool Open( std::ifstream & s, int userToNotify )
    {
        bool ret = tConfItemBase::OpenFile( s, file_, tConfItemBase::All );

        if ( ret )
        {
            return true;
        }
        else
        {
            con << tOutput( "$vote_include_error", file_ );
            sn_ConsoleOut( tOutput( "$vote_include_error", file_ ), userToNotify );
            return false;
        }
    }

    virtual bool DoCheckValid( int senderID )
    { 
        std::ifstream s;
        return ( Open( s, senderID ) && eVoteItemServerControlled::DoCheckValid( senderID ) );
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        // set the access level for the following operation
        tCurrentAccessLevel accessLevel( level_, true );

        // load contents of voted file for real
        std::ifstream s;
        if ( Open( s, 0 ) )
        {
            sn_ConsoleOut( tOutput( "$vote_include_message", file_, tCurrentAccessLevel::GetName( level_ ) ) );
            eAccessConsoleFilter filter( level_ );
            tConfItemBase::ReadFile( s );
        }
    }

    tString file_;       //!< the file to include (inside the vote/ subdirectory)
    tAccessLevel level_; //!< the level to execute the file with
};

// command vote items
class eVoteItemCommand: public eVoteItemServerControlled
{
public:
    // constructors/destructor
    eVoteItemCommand( tString const & command, tAccessLevel submitterLevel )
    : eVoteItemServerControlled()
    , command_( command )
    {
        description_ = tOutput( "$vote_command_text", command );
        details_ = tOutput( "$vote_command_details_text", command );

        if ( submitterLevel > se_accessLevelVoteCommandExecute )
        {
            submitterLevel = se_accessLevelVoteCommandExecute;
        }
        level_ = submitterLevel;
    }

    ~eVoteItemCommand()
    {}
protected:
    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteCommand;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasCommand;
    }

    virtual void DoExecute()                        // called when the voting was successful
    {
        // set the access level for the following operation
        tCurrentAccessLevel accessLevel( level_, true );

        // load contents of everytime.cfg for real
        std::istringstream s( static_cast< char const * >( command_ ) );
        sn_ConsoleOut( tOutput( "$vote_command_message" ) );
        eAccessConsoleFilter filter( tAccessLevel_Default );
        tConfItemBase::LoadLine(s);
    }

    tString command_;    //!< the command to execute
    tAccessLevel level_; //!< the level to execute the file with
};

// scramble vote items
class eVoteItemScramble: public eVoteItemServerControlled
{
public:
    // constructors/destructor
    eVoteItemScramble()
    : eVoteItemServerControlled()
    {
        description_ = tOutput( "$vote_scramble_text" );
        details_ = tOutput( "$vote_scramble_details_text" );

    }

    ~eVoteItemScramble()
    {}
protected:
    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteScramble;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasScramble;
    }

    virtual void DoExecute()                        // called when the voting was successful
    {
        ePlayerNetID::SetScramble();
    }
};

class eVoteItemReferee: public eVoteItemHarmServerControlled
{
public:
    eVoteItemReferee( ePlayerNetID* player = 0 )
    : eVoteItemHarm( player )
    {}

    ~eVoteItemReferee()
    {}
protected:
    // get the language string prefix
    virtual char const * DoGetPrefix() const{ return "referee"; }

    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteReferee;
    }

    virtual bool DoCheckValid( int senderID )
    {
        if ( !eVoteItemHarm::DoCheckValid( senderID ) )
            return false;

        ePlayerNetID * victim = GetPlayer();
        if ( victim->GetAccessLevel() <= tAccessLevel_Referee )
        {
            sn_ConsoleOut( tOutput("$vote_referee_already", victim->GetColoredName() ), senderID );
            return false;
        }

        return true;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasReferee;
    }
    virtual void DoExecute()                        // called when the voting was successful
    {
        ePlayerNetID * player = GetPlayer();
        if ( player )
        {
            se_MakeReferee ( player );
        }
    }

};

class eVoteItemCancelReferee: public eVoteItemHarmServerControlled
{
public:
    eVoteItemCancelReferee( ePlayerNetID* player = 0 )
    : eVoteItemHarm( player )
    {}

    ~eVoteItemCancelReferee()
    {}
protected:
    // get the language string prefix
    virtual char const * DoGetPrefix() const{ return "demotereferee"; }

    // access level required for this kind of vote
    virtual tAccessLevel DoGetAccessLevel() const
    {
        return se_accessLevelVoteReferee;
    }

    virtual bool DoCheckValid( int senderID )
    {
        if ( !eVoteItemHarm::DoCheckValid( senderID ) )
            return false;

        ePlayerNetID * victim = GetPlayer();
        if ( victim->GetAccessLevel() != tAccessLevel_Referee )
        {
            sn_ConsoleOut( tOutput("$vote_demotereferee_notref", victim->GetColoredName() ), senderID );
            return false;
        }

        return true;
    }

    // return vote-specific extra bias
    virtual int DoGetExtraBias() const
    {
        return se_votingBiasCancelReferee;
    }
    virtual void DoExecute()                        // called when the voting was successful
    {
        ePlayerNetID * player = GetPlayer();
        if ( player )
        {
            se_CancelReferee ( player );
        }
    }

};

#endif
#endif


// **************************************************************************************
// **************************************************************************************


// menu item to kick selected players
class eMenuItemKick: public uMenuItemAction
{
public:
    eMenuItemKick(uMenu *m, ePlayerNetID* p )
            : uMenuItemAction( m, tOutput(""),tOutput("$kick_player_help" ) )
    {
        this->name_.Clear();
        this->name_.SetTemplateParameter(1, p->GetName() );
        this->name_ << "$kick_player_text";
        player_ = p;
    }

    ~eMenuItemKick()
    {
    }

    virtual void Enter()
    {
        if(sn_GetNetState()==nSERVER)
        {
            // kill user directly
            se_VoteKickPlayer( player_ );
        }
        {
            // issue kick vote
            se_SendKick( player_ );
        }

        // leave menu to release smart pointers
        this->menu->Exit();
    }
private:
    tCONTROLLED_PTR( ePlayerNetID ) player_;		// keep player referenced
};


// ****************************************************************************************
// ****************************************************************************************

static nSpamProtectionSettings se_voteSpamProtection( 50.0f, "SPAM_PROTECTION_VOTE", tOutput("$vote_spam_protection") );

eVoter::eVoter( nMachine & machine )
        : nMachineDecorator( machine ), machine_( machine ), votingSpam_( se_voteSpamProtection )
{
    selfReference_ = this;
    voters_.Add( this );
    harmCount_ = 0;
    lastKickVote_ = -1E+40;
    lastHarmVote_ = -1E+40;
    lastNameChangePreventor_ = -1E+40;
    lastChange_ = tSysTimeFloat();
}

eVoter::~eVoter()
{
    voters_.Remove( this );
}

void eVoter::Spam( int user, REAL spamLevel, tOutput const & message )
{
    if ( sn_GetNetState() == nSERVER )
        votingSpam_.CheckSpam( spamLevel, user, message );
}

bool eVoter::IsSpamming( int user )
{
    if ( sn_GetNetState() == nSERVER )
    {
        return nSpamProtection::Level_Ok != votingSpam_.CheckSpam( 0.0f, user, tOutput("$spam_vote_kick_issue") );
    }

    return false;
}

// *******************************************************************************
// *
// *	OnDestroy
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void eVoter::OnDestroy( void )
{
    tJUST_CONTROLLED_PTR< eVoter > keepAlive( this );
    selfReference_ = 0;
}


// *******************************************************************************
// *
// *	OnDestroy
// *
// *******************************************************************************
//!
//! @return the last change to players on this voter in seconds
//! 
// *******************************************************************************

REAL eVoter::Age() const
{
    return tSysTimeFloat() - lastChange_;
}



// *******************************************************************************
// *
// *	AllowNameChange
// *
// *******************************************************************************
//! @return true if the players belonging to this voter should be allowed to rename
//!
// *******************************************************************************

bool eVoter::AllowNameChange( void ) const
{
    return tSysTimeFloat() > this->lastNameChangePreventor_ + se_minTimeBetweenKicks;
}

void eVoter::RemoveFromGame()
{
    tCONTROLLED_PTR( eVoter ) keeper( this );

    voters_.Remove( this );

    // remove from items
    for ( int i = eVoteItem::GetItems().Len()-1; i>=0; --i )
    {
        eVoteItem::GetItems()( i )->RemoveVoterCompletely( this );
    }
}

void eVoter::KickMenu()							// activate player kick menu
{
    uMenu menu( "$player_police_kick_text" );

    int size = se_PlayerNetIDs.Len();
    eMenuItemKick** items = tNEW( eMenuItemKick* )[ size ];

    int i;
    for ( i = size-1; i>=0; --i )
    {
        ePlayerNetID* player = se_PlayerNetIDs[ i ];
        if ( player->IsHuman() )
        {
            items[i] = tNEW( eMenuItemKick )( &menu, player );
        }
        else
        {
            items[i] = 0;
        }
    }

    menu.Enter();

    for ( i = size - 1; i>=0; --i )
    {
        if( items[i] )
            delete items[i];
    }
    delete[] items;
}

#ifndef DEDICATED
static bool se_KeepConsoleSmall()
{
    return true;
}
#endif

static uMenu* votingMenu = 0;

void eVoter::VotingMenu()						// activate voting menu ( you can vote about suggestions there )
{
    static bool recursion = false;
    if ( ! recursion )
    {

        // expire old items
        if ( !VotingPossible() )
            return;

#ifndef DEDICATED
        rSmallConsoleCallback SmallConsole( se_KeepConsoleSmall );

        // count items
        int size = eVoteItem::GetItems().Len();
        if ( size == 0 )
            return;

        // fill menu
        uMenu menu( "$voting_menu_text" );

        eMenuItemVote** items = tNEW( eMenuItemVote* )[ size ];

        int i;
        for ( i = size-1; i>=0; --i )
        {
            items[i] = tNEW( eMenuItemVote )( &menu, eVoteItem::GetItems()( i ) );
        }

        // enter menu
        recursion = true;
        votingMenu = &menu;
        menu.Enter();
        votingMenu = 0;
        recursion = false;

        for ( i = size - 1; i>=0; --i )
        {
            delete items[i];
        }
        delete[] items;

        // expire old items
        VotingPossible();
#endif
    }
}

bool eVoter::VotingPossible()
{
    // expire old items
    for ( int i = eVoteItem::GetItems().Len()-1; i>=0; --i )
    {
        eVoteItem::GetItems()( i )->Evaluate();
    }

    if ( sn_GetNetState() != nCLIENT )
    {
        return false;
    }

    return eVoteItem::GetItems().Len() > 0;
}

eVoter* eVoter::GetVoter( int ID, bool complain )			// find or create the voter for the specified ID
{
    // the server has no voter
#ifdef DEDICATED
    if ( ID == 0 )
        return NULL;
#endif

    // see if there is a real player on the specified ID
    bool spectator = false;
    bool player = false;
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        if ( p->Owner() == ID )
        {
            if( p->CurrentTeam() )
            {
                player = true;
            }
            spectator = true;
        }
    }

    if( !spectator )
    {
        // no support for old style, playerless spectators any more
        return NULL;
    }

    if ( !se_allowVotingSpectator && !player )
    {
        if ( complain )
        {
            tOutput message("$vote_disabled_spectator");
            sn_ConsoleOut( message, ID );
        }
        return NULL;
    }

    // get machine from network subsystem
    nMachine & machine = nMachine::GetMachine( ID );

    return GetVoter( machine );
}

eVoter* eVoter::GetVoter( nMachine & machine )			// find or create the voter for the specified machine
{
    // iterate through the machine's decorators, find a voter
    nMachineDecorator * run = machine.GetDecorators();
    while ( run )
    {
        eVoter * voter = dynamic_cast< eVoter * >( run );
        if ( voter )
        {
            // reinsert voter into lists
            if ( voter->ListID() < 0 )
            {
                voters_.Add( voter );
                voter->lastKickVote_ = -1E30;
                voter->lastNameChangePreventor_ = -1E30;
            }

            // return result
            return voter;
        }

        run = run->Next();
    }

    // create new voter
    return tNEW(eVoter)( machine );
}

tList< eVoter > eVoter::voters_;					// list of all voters

static void se_Cleanup()
{
    if ( nCallbackLoginLogout::User() == 0 )
    {
        if ( votingMenu )
        {
            votingMenu->Exit();
        }

        if ( !nCallbackLoginLogout::Login() && eGrid::CurrentGrid() )
        {
            //			uMenu::exitToMain = true;
        }

        // client login/logout: delete voting items
        const tList< eVoteItem >& list = eVoteItem::GetItems();
        while ( list.Len() > 0 )
        {
            delete list(0);
        }
    }
    else if ( nCallbackLoginLogout::Login() )
    {
        // new user: send pending voting items
        const tList< eVoteItem >& list = eVoteItem::GetItems();
        for ( int i = list.Len()-1; i >= 0; -- i)
        {
            eVoteItem* vote = list( i );
            nMessageBase * m = vote->CreateMessage();
            m->Send( nCallbackLoginLogout::User() );
        }
    }
}


static nCallbackLoginLogout se_cleanup( se_Cleanup );

eVoteItem::~eVoteItem( void )
{
    items_.Remove( this );

    if ( menuItem_ )
    {
        menuItem_->item_ = 0;
        menuItem_->title.Clear();
        menuItem_->helpText.Clear();
        menuItem_ = 0;
    }
}

void eVoteItem::UpdateMenuItem( void )
{
    if ( menuItem_ )
    {
        menuItem_->title.Clear();
        menuItem_->title = GetDescription();

        menuItem_->helpText.Clear();
        menuItem_->helpText << tString( tOutput( "$vote_details_help", GetDetails() ) );
    }
}

// *******************************************************************************************
// *
// *	Name
// *
// *******************************************************************************************
//!
//!		@param senderID 	the ID of the network user ( default: take any )
//!		@return				the name of the voter ( all players of that IP )
//!
// *******************************************************************************************

tString eVoter::Name( int senderID ) const
{
    tString name;

    // collect the names of all players associated with this voter
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        if ( eVoter::GetVoter( p->Owner() ) == this && ( senderID < 0 || p->Owner() == senderID ) )
        {
            if ( name.Len() > 1 )
                name << ", ";
            name << p->GetName();
        }
    }

    if ( name.Len() < 2 )
        name = machine_.GetIP();

    return name;
}

// *******************************************************************************
// *
// *	PlayerChanged
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void eVoter::PlayerChanged( void )
{
    this->lastChange_ = tSysTimeFloat();
}

// *******************************************************************************
// *
// *	HandleChat
// *
// *******************************************************************************
//! @param p the player chatting
//! @param message the rest of the message after "/vote"
// *******************************************************************************

void eVoter::HandleChat( ePlayerNetID * p, std::istream & message ) //!< handles player "/vote" command.
{
    // cloak the ID of the sener for privacy
    nCurrentSenderID cloak;
    if ( se_votingPrivacy > 1 )
        cloak.SetID(0);

    if ( !p )
    {
        return;
    }

    // don't let the player call a vote if s/he is silenced.
    if ( p->IsSilenced() )
    {
        tOutput message("$vote_silenced");
        sn_ConsoleOut( message, p->Owner() );
        return;
    }

    // read command part (kick, remove, include)
    tString command;
    message >> command;
    tToLower( command );

    eVoter * voter = eVoter::GetVoter( p->Owner(), true ); // can't use ePlayerNedID::GetVoter here,
                                                           // as it can't show a warning,
                                                           // for example if the voter has only spectators
    if ( !eVoteItem::AcceptNewVote( voter, p->Owner() ) )
    {
        return;
    }

    eVoteItem * item = 0;

    if ( command == "kick" )
    {
        tString name;
        name.ReadLine( message );
        ePlayerNetID * toKick = ePlayerNetID::FindPlayerByName( name, p );
        if ( toKick )
        {
            // accept message
            item = tNEW( eVoteItemKickServerControlled )( false, toKick );
        }
    }
    else if ( command == "suspend" )
    {
        tString name;
        name.ReadLine( message );
        ePlayerNetID * toSuspend = ePlayerNetID::FindPlayerByName( name, p );
        if ( toSuspend )
        {
            // accept message
            item = tNEW( eVoteItemSuspend )( toSuspend );
        }
    }
#ifdef DEDICATED
#ifdef KRAWALL_SERVER
    else if ( command == "include" )
    {
        tString file;
        file.ReadLine( message );
        {
            // accept message
            item = tNEW( eVoteItemInclude )( file, p->GetAccessLevel() );
        }
    }
    else if ( command == "referee" )
    {
        tString name;
        name.ReadLine( message );
        ePlayerNetID * toMakeReferee = ePlayerNetID::FindPlayerByName( name, p );

        if ( toMakeReferee )
        {
            // accept message
            item = tNEW( eVoteItemReferee )( toMakeReferee );
        }
    }
    else if ( command == "demotereferee" )
    {
        tString name;
        name.ReadLine( message );
        ePlayerNetID * toDemoteReferee = ePlayerNetID::FindPlayerByName( name, p );

        if ( toDemoteReferee )
        {
            // accept message
            item = tNEW( eVoteItemCancelReferee )( toDemoteReferee );
        }
    }
    else if ( command == "scramble" )
    {
        item = tNEW ( eVoteItemScramble );
    }
    else if ( command == "command" )
    {
        tString console;
        console.ReadLine( message );
        {
            // accept message
            item = tNEW( eVoteItemCommand )( console, p->GetAccessLevel() );
        }
    }
#endif
#endif
    else
    {
#if defined(DEDICATED) && defined(KRAWALL_SERVER)
        sn_ConsoleOut( tOutput("$vote_unknown_command", command, "command, demotereferee, include, kick, referee, suspend, scramble" ), p->Owner() );
#else
        sn_ConsoleOut( tOutput("$vote_unknown_command", command, "kick, suspend" ), p->Owner() );
#endif
    }

    // nothing created
    if ( !item )
    {
        return;
    }

    // let item check its validity
    if ( !item->CheckValid( p->Owner() ) )
    {
        delete item;
        return;
    }

    // no objection? Broadcast it to everyone.
    item->Update();
    item->ReBroadcast( p->Owner() );
}

