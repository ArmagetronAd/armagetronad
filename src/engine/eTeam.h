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

#ifndef ArmageTron_TEAM_H
#define ArmageTron_TEAM_H

#include "ePlayer.h"
#include "nNetObject.h"
#include "tList.h"
#include <vector>


template<class T> class nConfItem;


class eTeam: public nNetObject{
protected:							// protected attributes
    int colorID;					// ID of the team predefined color
    int listID; 					// ID in the list of all teams
    int score;						// score the team has accumulated

    int numHumans;					// number of human players on the team
    int numAIs;						// number of AI players on the team

    tList<ePlayerNetID> players;    // players in this team

    int maxPlayersLocal;			// maximum number of players allowed in this team
    int maxImbalanceLocal;			// maximum imbalance allowed here

    unsigned short r,g,b;			// team color
    tString	name;					// our name

    bool locked_;                   //!< if set, only invited players may join

    static void UpdateStaticFlags();// update all internal information

public:							// public configuration options
    static int  minTeams;			// minimum nuber of teams
    static int  maxTeams;    		// maximum nuber of teams
    static int  maxPlayers;   		// maximum number of players per team
    static int  minPlayers;   		// maximum number of players per team
    static int  maxImbalance;		// maximum difference of player numbers allowed
    static bool balanceWithAIs;	// use AI players to balance the teams?
    static bool enforceRulesOnQuit; // if the quitting of one player unbalances the teams, enforce the rules by redistributing

    static tList<eTeam> teams;		//  list of all teams

    void UpdateProperties();		// update internal properties ( player count )
    void UpdateAppearance();		// update name and color
    void Update();					// update all properties

    void SetLocked( bool locked );  // sets the lock status (whether invitations are required)
    bool IsLocked() const;          // returns the lock status

    void Invite( ePlayerNetID * player );                // invite the player to join
    void UnInvite( ePlayerNetID * player );              // revoke an invitation
    bool IsInvited( ePlayerNetID const * player ) const; // check if a player is invited

    static bool Enemies( eTeam const * team, ePlayerNetID const * player ); //!< determines whether the player is an enemy of the team
    static bool Enemies( eTeam const * team1, eTeam const * team2 ); //!< determines whether two teams are enemies

    static void Enforce( int minTeams, int maxTeams, int maxImbalance );
public:												// public methods
    static void	EnforceConstraints();					// make sure the limits on team number and such are met

    static void SortByScore();							// brings the teams into the right order

    static void SwapTeamsNo(int a,int b);             	// swaps the teams a and b

    static tString Ranking( int MAX = 6, bool cut = true );				// return ranking information
    static float RankingGraph( float y, int MAX = 6 );				// print ranking information

    bool			NameTeamAfterColor ( bool wish );	// inquire or set the ability to use a color as a team name

    void 			AddPlayer   	( ePlayerNetID* player );				// register a player
    void 			AddPlayerDirty 	( ePlayerNetID* player );				// register a player without calling UpdateProperties
    void 	 		RemovePlayer	( ePlayerNetID* player );				// deregister a player
    virtual bool	PlayerMayJoin	( const ePlayerNetID* player ) const;		// see if the given player may join this team
    static bool 	NewTeamAllowed	();											// is it allowed to create a new team?

    static void     SwapPlayers     ( ePlayerNetID* player1, ePlayerNetID *player2 ); //!< swaps the team positions of the two players (same team or not)

    void            Shuffle         ( int startID, int stopID ); //!< shuffles the player at team postion startID to stopID

    virtual bool BalanceThisTeam() const {
        return true;    // care about this team when balancing teams
    }
    virtual bool IsHuman() const {
        return true;    // does this team consist of humans?
    }

    int				TeamID			( void  ) const {
        return listID;
    }

    int				Score			(		) const {
        return score;
    }
    void			AddScore		( int s );
    void			ResetScore		(		);
    void			SetScore		( int s );

    void 			AddScore		( int points,
                        const tOutput& reasonwin,
                        const tOutput& reasonlose );

    // player inquiry
    int	 			NumPlayers		(		) const {
        return players.Len();    // total number of players
    }
    ePlayerNetID*	Player			( int i ) const {
        return players(i); 	   // player of index i
    }
    // Export a list of pointers to all the players in this team
    std::vector < ePlayerNetID * > 	GetAllMembers () const {
        std::vector <ePlayerNetID *> tmp;
        for (int i=0; i<players.Len(); i++) {
            tmp.push_back( players(i) );
        }
        return tmp;
    }


    int	 			NumHumanPlayers	(		) const; 							// number of human players
    int	 			NumAIPlayers	(		) const; 							// number of human players
    int	 			AlivePlayers	(		) const;							// how many of the current players are currently alive?
    ePlayerNetID*	OldestPlayer	(		) const;							// the oldest player
    ePlayerNetID*	OldestHumanPlayer(		) const;							// the oldest human player
    ePlayerNetID*	OldestAIPlayer	(		) const;							// the oldest AI player
    ePlayerNetID*	YoungestPlayer	(		) const;							// the youngest player
    ePlayerNetID*	YoungestHumanPlayer(	) const;							// the youngest human player
    ePlayerNetID*	YoungestAIPlayer(		) const;							// the youngest AI player
    bool			Alive			(		) const;							// is any of the players currently alive?

    // name and color
    unsigned short	R() 	const {
        return r;
    }
    unsigned short	G() 	const {
        return g;
    }
    unsigned short	B() 	const {
        return b;
    }
    const tString& 	Name() 	const {
        return name;
    }

    virtual void PrintName(tString &s) const;					// print out an understandable name in to s


    virtual bool ClearToTransmit(int user) const;		// we must not transmit an object that contains pointers to non-transmitted objects. this function is supposed to check that.

    // syncronisation functions:
    virtual void WriteSync(nMessage &m);				// store sync message in m
    virtual void ReadSync(nMessage &m);					// guess what
    virtual bool SyncIsNew(nMessage &m);				// is the message newer	than the last accepted sync
    virtual nDescriptor&	CreatorDescriptor() const;

    // the extra information sent on creation:
    virtual void WriteCreate(nMessage &m); // store sync message in m
    // the information written by this function should
    // be read from the message in the "message"- connstructor

    // control functions:
    virtual void ReceiveControlNet(nMessage &m);
    // receives the control message. the data written to the message created
    // by *NewControlMessage() can be read directly from m.

    // shall the server accept sync messages from the clients?
    virtual bool AcceptClientSync() const	{
        return false;
    }

    // con/desstruction
    eTeam();											// default constructor
    eTeam(nMessage &m);									// remote constructor
    ~eTeam();											// destructor

private:
    void 	 		RemovePlayerDirty( ePlayerNetID* player );				// just remove a player from the player list, no messages, no balancing
};

#endif

