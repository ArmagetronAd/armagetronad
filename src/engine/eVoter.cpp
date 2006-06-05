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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "rSDL.h"

#include "eVoter.h"

#include "tMemManager.h"
#include "tSysTime.h"

#include "uMenu.h"

#include "nConfig.h"

#include "rConsole.h"

#include "ePlayer.h"
#include "eGrid.h"

// use server controlled votes
static bool se_useServerControlledKick = false;
static nSettingItemWatched< bool > se_usc( "VOTE_USE_SERVER_CONTROLLED_KICK", se_useServerControlledKick, nConfItemVersionWatcher::Group_Annoying, 10 );

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

static int se_minVoters = 3;
static tSettingItem< int > se_mv( "MIN_VOTERS", se_minVoters );

// the number set here always acts as votes against a change.
static int se_votingBias = 0;
static tSettingItem< int > se_vb( "VOTING_BIAS", se_votingBias );

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

// time between name changes and you being allowed to issue votes again
static int se_votingMaturity = 300;
static tSettingItem< int > se_votingMaturitySI( "VOTING_MATURITY", se_votingMaturity );

static eVoter* se_GetVoter( const nMessage& m )
{
    return eVoter::GetVoter( m.SenderID(), true );
}

// something to vote on
class eVoteItem: public tListMember
{
    friend class eMenuItemVote;
public:
    // constructors/destructor
    eVoteItem( void ): creationTime_( tSysTimeFloat() ), user_( 0 ), id_( ++se_votingItemID ), menuItem_( 0 )
    {
        items_.Add( this );
    };

    virtual ~eVoteItem( void );

    bool FillFromMessage( nMessage& m )
    {
        // cloak the ID of the sener for privacy
        nCurrentSenderID cloak;
        if ( se_votingPrivacy > 1 )
            cloak.SetID(0);

        if ( !DoFillFromMessage( m ) )
            return false;

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
                sn_ConsoleOut( voteMessage );	// broadcast it
            else if ( se_votingPrivacy <= 1 )
                con << voteMessage;				// print it for the server admin

            tJUST_CONTROLLED_PTR< nMessage > ret = this->CreateMessage();
            for ( int i = MAXCLIENTS; i > 0; --i )
            {
                if ( sn_Connections[ i ].socket && i != m.SenderID() && 0 != eVoter::GetVoter( i ) )
                {
                    ret->Send( i );
                }
            }
            //			item->SendMessage();
        }

        con << tOutput( "$vote_new", GetDescription() );

        this->Evaluate();

        return true;
    };

    nMessage* CreateMessage( void ) const
    {
        nMessage* m = tNEW( nMessage )( this->DoGetDescriptor() );
        this->DoFillToMessage( *m );
        return m;
    }

    void SendMessage( void ) const
    {
        this->CreateMessage()->BroadCast();
    }

    // message sending
    void Vote( bool accept );														// called on the clients to accept or decline the vote

    static bool AcceptNewVote( nMessage& m )										// check if a new voting item should be accepted
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

        eVoter* voter = se_GetVoter( m );
        // check if voting is allowed
        if ( !voter )
        {
            return false;
        }

        // reject voting
        if ( !se_allowVoting )
        {
            tOutput message("$vote_disabled");
            sn_ConsoleOut( message, m.SenderID() );
            return false;
        }

        // spawn spectator voters
        for ( i = MAXCLIENTS; i > 0; --i )
        {
            if ( sn_Connections[ i ].socket )
                eVoter::GetVoter( i );
        }

        // enough voters online?
        if ( eVoter::voters_.Len() < se_minVoters )
        {
            tOutput message("$vote_toofew");
            sn_ConsoleOut( message, m.SenderID() );
            return false;
        }

        // check for spam
        if ( voter->IsSpamming( m.SenderID() ) )
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
            sn_ConsoleOut( message, m.SenderID() );
            return false;
        }

        if ( items_.Len() < se_maxVotes )
        {
            voter->Spam( m.SenderID(), se_votingSpamIssue );
            return true;
        }
        else
        {
            tOutput message("$vote_overflow");
            sn_ConsoleOut( message, m.SenderID() );
            return false;
        }
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
    static void GetControlMessage( nMessage& m )								   	// handles a voting message
    {
        if ( sn_GetNetState() == nSERVER )
        {
            unsigned short id;
            m.Read( id );

            bool result;
            m >> result;
            result = result ? 1 : 0;

            for ( int i = items_.Len()-1; i>=0; --i )
            {
                eVoteItem* vote = items_[i];
                if ( vote->id_ == id )
                {
                    // found the vote; find the voter
                    tCONTROLLED_PTR( eVoter ) voter = se_GetVoter( m );
                    if ( voter )
                    {
                        // prepare message
                        tOutput voteMessage;
                        voteMessage.SetTemplateParameter( 1, voter->Name( m.SenderID() ) );
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
        total = eVoter::voters_.Len();
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

    // evaluation
    virtual void Evaluate()																	// check if this voting item is to be kept around
    {
        int pro, con, total;

        GetStats( pro, con, total );

        // apply bias
        con 	+= se_votingBias;
        total 	+= se_votingBias;

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
                    this->suggestor_->Spam( user_, se_votingSpamReject );

                this->BroadcastMessage( tOutput( "$vote_rejected" ) );
                delete this;
                return;
            }

            // see if the vote has been accepted
            if ( pro >= con && pro * 2 > total )
            {
                this->BroadcastMessage( tOutput( "$vote_accepted" ) );

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
protected:
    virtual bool DoFillFromMessage( nMessage& m )
    {
        // get user
        user_ = m.SenderID();

        // get originator of vote
        if(sn_GetNetState()==nSERVER)
        {
            suggestor_ = se_GetVoter( m );
            if ( !suggestor_ )
                return false;

            // add suggestor to supporters
            this->voters_[1].Insert( suggestor_ );
        }
        else
        {
            m.Read( id_ );
        }

        return true;
    };

    virtual void DoFillToMessage( nMessage& m  ) const
    {
        if(sn_GetNetState()==nSERVER)
        {
            // write our message ID
            m.Write( id_ );
        }
    };

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
    virtual nDescriptor& DoGetDescriptor() const = 0;	// returns the creation descriptor
    virtual tString DoGetDescription() const = 0;		// returns the description of the voting item
    virtual void DoExecute() = 0;						// called when the voting was successful

    nTimeAbsolute creationTime_;					// time the vote was cast
    tCONTROLLED_PTR( eVoter ) suggestor_;			// the voter suggesting the vote
    unsigned int user_;								// user suggesting the vote
    tArray< tCONTROLLED_PTR( eVoter ) > voters_[2];	// array of voters approving or disapproving of the vote
    unsigned short id_;								// running id of voting item
    eMenuItemVote *menuItem_;						// menu item

    eVoteItem& operator=( const eVoteItem& );
    eVoteItem( const eVoteItem& );
};

tList< eVoteItem > eVoteItem::items_;				// list of vote items

static nDescriptor vote_handler(230,eVoteItem::GetControlMessage,"vote cast");

// called on the clients to accept or decline the vote
void eVoteItem::Vote( bool accept )
{
    tJUST_CONTROLLED_PTR< nMessage > m = tNEW( nMessage )( vote_handler );
    *m << id_;
    *m << accept;
    m->BroadCast();

    delete this;
}

//nDescriptor& eVoteItem::DoGetDescriptor() const;	// returns the creation descriptor
//tString eVoteItem::DoGetDescription() const;		// returns the description of the voting item
//void DoExecute();						// called when the voting was successful

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

static void se_HandleServerVoteChanged( nMessage& m );
static nDescriptor server_vote_expired_handler(233,se_HandleServerVoteChanged,"Server controlled vote expired");

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

    static void s_HandleChanged( nMessage & m )
    {
        unsigned short id;
        m.Read( id );
        for ( int i = items_.Len()-1; i>=0; --i )
        {
            eVoteItem* vote = items_[i];
            if ( vote->GetID() == id )
            {
                eVoteItemServerControlled * vote2 = dynamic_cast< eVoteItemServerControlled * >( vote );
                if ( vote2 )
                    vote2->HandleChanged( m );
            }
        }
    }

    void HandleChanged( nMessage & m )
    {
        unsigned short expired;
        m.Read( expired );
        expired_ = expired;
        m >> description_;
        m >> details_;

        Update();
        UpdateMenuItem();
    }

    void SendChanged()
    {
        tJUST_CONTROLLED_PTR< nMessage > m = tNEW( nMessage )( server_vote_expired_handler );
        *m << GetID();
        *m << (unsigned short)expired_;
        *m << description_;
        *m << details_;
        m->BroadCast();
    }
protected:
    virtual void Update() //!< update description and details
    {}

    virtual bool DoFillFromMessage( nMessage& m )
    {
        m >> description_;
        m >> details_;
        return true;
    };

    virtual void DoFillToMessage( nMessage& m ) const
    {
        m << description_;
        m << details_;
        eVoteItem::DoFillToMessage( m );
    };

    virtual void DoExecute(){};						// called when the voting was successful
protected:
    virtual nDescriptor& DoGetDescriptor() const;	// returns the creation descriptor

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

static void se_HandleServerVoteChanged( nMessage& m )
{
    eVoteItemServerControlled::s_HandleChanged( m );
}

static void se_HandleNewServerVote( nMessage& m )
{
    if ( eVoteItem::AcceptNewVote( m ) )
    {
        // accept message
        eVoteItem* item = tNEW( eVoteItemServerControlled )();
        if ( !item->FillFromMessage( m ) )
            delete item;
    }
}

static nDescriptor new_server_vote_handler(232,se_HandleNewServerVote,"Server controlled vote");

// returns the creation descriptor
nDescriptor& eVoteItemServerControlled::DoGetDescriptor() const
{
    return new_server_vote_handler;
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

// something to vote on
class eVoteItemKick: public virtual eVoteItem
{
public:
    // constructors/destructor
    eVoteItemKick( ePlayerNetID* player = 0 )
            : player_( player )
            , machine_(NULL)
            , name_( "(Player who already left)" )
    {}

    ~eVoteItemKick()
    {
        delete machine_;
        machine_ = NULL;
    }
protected:
    virtual bool DoFillFromMessage( nMessage& m )
    {
        double time = tSysTimeFloat();

        // check whether the issuer is allowed to start a vote
        eVoter * sender = eVoter::GetVoter( m.SenderID() );
        if ( sender && sender->lastChange_ + se_votingMaturity > tSysTimeFloat() )
        {
            tOutput message("$vote_maturity");
            sn_ConsoleOut( message, m.SenderID() );
            return false;
        }
        
        // prevent the sender from changing his name for confusion
        if ( sender )
            sender->lastKickVote_ = time;

        // read player ID
        unsigned short id;
        m.Read(id);
        tJUST_CONTROLLED_PTR< ePlayerNetID > p=dynamic_cast<ePlayerNetID *>(nNetObject::ObjectDangerous(id));
        player_ = p;

        // check if player is protected from kicking
        if ( p && sn_GetNetState() != nCLIENT )
        {
            name_ = p->GetName();
            eVoter * voter = eVoter::GetVoter( p->Owner() );
            if ( voter )
            {
                machine_ = tNEW( nMachineObserver )( voter->machine_ );

                if ( time < voter->lastKickVote_ + se_minTimeBetweenKicks )
                {
                    tOutput message("$vote_redundant");
                    sn_ConsoleOut( message, m.SenderID() );
                    return false;
                }
                else
                {
                    voter->lastKickVote_ = time;
                }
            }
        }

        eVoteItem::DoFillFromMessage( m );

        return true;
    };

    virtual void DoFillToMessage( nMessage& m  ) const
    {
        if ( player_ )
            m.Write( player_->ID() );
        else
            m.Write( 0 );

        eVoteItem::DoFillToMessage( m );
    };
protected:
    virtual nDescriptor& DoGetDescriptor() const;	// returns the creation descriptor

    virtual tString DoGetDescription() const		// returns the description of the voting item
    {
        // get name from player
        if ( player_ )
            name_ = player_->GetName();

        return tString( tOutput( "$kick_player_text", name_ ) );
    }

    virtual tString DoGetDetails() const		    // returns the detailed description of the voting item
    {
        // get name from player
        if ( player_ )
            name_ = player_->GetName();

        return eVoteItem::DoGetDetails() + tString( tOutput( "$kick_player_details_text", name_ ) );
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        if ( player_ )
        {
            // kick the player, he is online
            int user = player_->Owner();
            if ( user > 0 )
                sn_KickUser( user, tOutput("$voted_kill_kick") );
        }
        else if ( machine_ )
        {
            // the player left. Inform the machine that he would have gotten kicked.
            nMachine * machine = machine_->GetMachine();
            if ( machine )
            {
                // kick all players that connected from that machine
                bool kick = false;
                for ( int user = MAXCLIENTS; user > 0; --user )
                {
                    if ( &nMachine::GetMachine( user ) == machine )
                    {
                        sn_KickUser( user, tOutput("$voted_kill_kick")  );
                        kick = true;
                    }
                }

                // if no user could be kicked, notify at least the machine that
                // somebody would have been kicked.
                if ( !kick )
                    machine->OnKick();
            }
        }
    }

private:
    nObserverPtr< ePlayerNetID > player_;		// keep player referenced
    nMachineObserver * machine_;                // pointer to the machine of the player to be kicked
    mutable tString name_;                      // the name of the player to be kicked
};

// something to vote on
class eVoteItemKickServerControlled: public virtual eVoteItemServerControlled, public virtual eVoteItemKick
{
public:
    // constructors/destructor
    eVoteItemKickServerControlled( ePlayerNetID* player = 0 )
            : eVoteItemKick( player )
    {}

    ~eVoteItemKickServerControlled()
    {}
protected:
    virtual bool DoFillFromMessage( nMessage& m )
    {
        // should never be called on the client
        tASSERT( sn_GetNetState() != nCLIENT );

        // deletage
        bool ret = eVoteItemKick::DoFillFromMessage( m );

        // fill in description
        Update();

        return ret;
    };

    virtual void DoFillToMessage( nMessage& m  ) const
    {
        // should never be called on the client
        tASSERT( sn_GetNetState() != nCLIENT );

        eVoteItemServerControlled::DoFillToMessage( m );
    };
private:
    virtual void Update() //!< update description and details
    {
        description_ = eVoteItemKick::DoGetDescription();
        details_ = eVoteItemKick::DoGetDetails();
    }

    virtual nDescriptor& DoGetDescriptor() const	// returns the creation descriptor
    {
        return eVoteItemServerControlled::DoGetDescriptor();
    }

    virtual tString DoGetDescription() const		// returns the description of the voting item
    {
        return eVoteItemServerControlled::DoGetDescription();
    }

    virtual tString DoGetDetails() const		// returns the detailed description of the voting item
    {
        return eVoteItemServerControlled::DoGetDetails();
    }

    virtual void DoExecute()						// called when the voting was successful
    {
        eVoteItemKick::DoExecute();
    }
};

static void se_HandleKickVote( nMessage& m )
{
    if ( eVoteItem::AcceptNewVote( m ) )
    {
        // accept message
        eVoteItem* item = se_useServerControlledKick ? tNEW( eVoteItemKickServerControlled )() : tNEW( eVoteItemKick )();
        if ( !item->FillFromMessage( m ) )
            delete item;
    }
}

static nDescriptor kill_vote_handler(231,se_HandleKickVote,"Kick vote");

// returns the creation descriptor
nDescriptor& eVoteItemKick::DoGetDescriptor() const
{
    return kill_vote_handler;
}

static void se_SendKick( ePlayerNetID* p )
{
    eVoteItemKick kick( p );
    kick.SendMessage();
}

// **************************************************************************************
// **************************************************************************************


// menu item to silence selected players
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
            sn_KickUser( player_->Owner(), tOutput("$voted_kill_kick") );
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
    lastKickVote_ = -1E+40;
    lastChange_ = tSysTimeFloat();
}

eVoter::~eVoter()
{
    voters_.Remove( this );
}

void eVoter::Spam( int user, REAL spamLevel )
{
    if ( sn_GetNetState() == nSERVER )
        votingSpam_.CheckSpam( spamLevel, user );
}

bool eVoter::IsSpamming( int user )
{
    if ( sn_GetNetState() == nSERVER )
    {
        return nSpamProtection::Level_Ok != votingSpam_.CheckSpam( 0.0f, user );
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
// *	AllowNameChange
// *
// *******************************************************************************
//! @return true if the players belonging to this voter should be allowed to rename
//!
// *******************************************************************************

bool eVoter::AllowNameChange( void ) const
{
    return tSysTimeFloat() > this->lastKickVote_ + se_minTimeBetweenKicks;
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
    if ( !se_allowVotingSpectator )
    {
        bool player = false;
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);
            if ( p->Owner() == ID && !p->IsSpectating() )
                player = true;
        }
        if (!player)
        {
            if ( complain )
            {
                tOutput message("$vote_disabled_spectator");
                sn_ConsoleOut( message, ID );
            }
            return NULL;
        }
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
            nMessage* m = vote->CreateMessage();
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


