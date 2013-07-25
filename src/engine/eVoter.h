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

#ifndef ArmageTron_VOTER_H
#define ArmageTron_VOTER_H

class ePlayerNetID;
class eMenuItemVote;
//class eVoteItem;

#include "tSafePTR.h"
#include "tList.h"
#include "tString.h"

#include "nNetObject.h"

#include "nSpamProtection.h"

// information in eVoter accessible for ePlayer only
class eVoterPlayerInfo
{
public:
    friend class ePlayerNetID;

    eVoterPlayerInfo();
private:
    int             suspended_;  //! number of rounds the player is currently suspended from playing
    bool	    silenced_; //! is he silenced?
};

// class identifying a voter; all players coming from the same IP share the same voter.
class eVoter: public tReferencable< eVoter >, public tListMember, public nMachineDecorator, public eVoterPlayerInfo
{
    friend class eVoteItem;
    friend class eVoteItemHarm;
    friend class eVoteItemKick;
public:
    eVoter( nMachine & machine );
    ~eVoter();

    void PlayerChanged();                                       //!< call when a player changed (logged in or changed name). He'll be blocked from issuing kick votes for a while.

    void Spam( int user, REAL spamLevel, tOutput const & message );	// trigger spam guard
    bool IsSpamming( int user );								// check if user has been found spamming
    void RemoveFromGame();										// remove from game

    static void KickMenu();										// activate player kick menu
    static void VotingMenu();									// activate voting menu ( you can vote about suggestions there )
    static eVoter* GetVoter ( int ID, bool complain = false );  // find or create the voter for the specified user ID
    static eVoter* GetPersistentVoter (int ID );                // find or create the persistent voter for the specified ID
    static eVoter* GetPersistentVoter ( nMachine & machine );   // find or create the persistent voter for the specified machine
    static bool VotingPossible();								// returns whether voting is currently possible

    static void HandleChat( ePlayerNetID * p, std::istream & message ); //!< handles player "/vote" command.

    tString Name(int senderID = -1) const;						// returns the name of the voter

    REAL Age() const;                                           //!< how long does this voter exist?

    bool AllowNameChange() const;  //!< determines whether the player belonging to this voter should be allowed to change names

    //! returns the number of harmful votes against this player
    int HarmCount() const
    {
        return harmCount_;
    }
protected:
    virtual void OnDestroy();      //!< called when machine gets destroyed

private:
    //    static eVoter* GetVoterFromIP( tString IP );	// find or create the voter for the specified IP
    //    const tString IP_;								// IP address of voter
    nMachine & machine_;                            // the machine this voter comes from
    static tList< eVoter > voters_;					// list of all voters
    nSpamProtection votingSpam_;					// spam control
    tJUST_CONTROLLED_PTR< eVoter > selfReference_;  //!< reference to self
    int harmCount_;                                 //!< counts the number of harmful votes issued against this player
    double lastHarmVote_;                           //!< the last time a harmful vote was issued for this player
    double lastKickVote_;                           //!< the last time a kick vote was issued for this player
    double lastNameChangePreventor_;                //!< the last time something happened that should prevent the voter from changing names
    double lastChange_;                             //!< the last time a player assigned to this voter changed
};


#endif	// ArmageTron_VOTER_H

