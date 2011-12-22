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
#include "eSpawn.h"
#include <vector>

namespace Engine{ class TeamSync; }

tString & operator << ( tString&, const eTeam&);
std::ostream & operator << ( std::ostream&, const eTeam&);

template<class T> class nConfItem;

extern int se_matches;

class eTeam: public nNetObject{
protected:							// protected attributes
    int colorID;					// ID of the team predefined color
    int listID; 					// ID in the list of all teams
    int score;						// score the team has accumulated
    int lastScore_;                 //!< score from the beginning of the round

    int numHumans;					// number of human players on the team
    int numAIs;						// number of AI players on the team

    tList<ePlayerNetID> players;    // players in this team

    int maxPlayersLocal;			// maximum number of players allowed in this team
    int maxImbalanceLocal;			// maximum imbalance allowed here

    int roundsPlayed;               //!< number of rounds played

    tShortColor color;	            // team color
    tString	name;					// our name

    bool locked_;                   //!< if set, only invited players may join

    eSpawnPoint * spawnPoint;

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

    int RoundsPlayed() const;       //!< number of rounds played (updated right after spawning, so it includes the current round)
    void PlayRound();               //!< increase round counter

    void UpdateProperties();		// update internal properties ( player count )
    void UpdateAppearance();		// update name and color
    void Update();					// update all properties

    void SetLocked( bool locked );  // sets the lock status (whether invitations are required)
    bool IsLocked() const;          // returns the lock status
    bool IsLockedFor( const ePlayerNetID * p ) const;   // returns if a team is locked to join by a certain player (due to ACCESS_LEVEL_PLAY)

    void Invite( ePlayerNetID * player );                // invite the player to join
    void UnInvite( ePlayerNetID * player );              // revoke an invitation
    bool IsInvited( ePlayerNetID const * player ) const; // check if a player is invited

    static bool Enemies( eTeam const * team, ePlayerNetID const * player ); //!< determines whether the player is an enemy of the team
    static bool Enemies( eTeam const * team1, eTeam const * team2 ); //!< determines whether two teams are enemies

    static void Enforce( int minTeams, int maxTeams, int maxImbalance );
    
    static void WritePlayers( eLadderLogWriter & writer, const eTeam *team );
    static void WriteLaunchPositions(); // Logs player positions to ladderlog.txt
public:												// public methods
    static void	EnforceConstraints();					// make sure the limits on team number and such are met

    static void SortByScore();							// brings the teams into the right order

    static void SwapTeamsNo(int a,int b);             	// swaps the teams a and b

    static tString Ranking( int MAX = 6, bool cut = true );				// return ranking information
    static float RankingGraph( float y, int MAX = 6 );				// print ranking information

    bool			NameTeamAfterColor ( bool wish );	// set the ability to use a color as a team name, return status

    bool			TeamNamedAfterColor () const { return colorID >= 0; } //!< returns whether the team is currently named after a color

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

    eSpawnPoint * SpawnPoint() {
        return spawnPoint;
    }

    void SetSpawnPoint(eSpawnPoint * sp)
    {
        spawnPoint = sp;
    }

    static void ResetScoreDifferences(); //<! Resets the last stored score so ScoreDifferences takes this as a reference time
    static void LogScoreDifferences();   //<! Logs accumulated scores of all players since the last call to ResetScoreDifferences() to ladderlog.txt
    void LogScoreDifference();           //<! Logs accumulated scores since the last call to ResetScoreDifferences() to ladderlog.txt

    // player inquiry
    int	 			NumPlayers(		) const {
        return players.Len();    // total number of players, includes players that quit the team during the current round
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


    int	 			NumHumanPlayers	(		) const; 							//!< number of human players
    int	 			NumAIPlayers	(		) const; 							//!< number of human players
    int             NumActivePlayers(       ) const;                            //!< how many active players are there right now that can spawn next round?
    int	 			AlivePlayers	(		) const;							//!< how many of the current players are currently alive?
    ePlayerNetID*	OldestPlayer	(		) const;							//!< the oldest player
    ePlayerNetID*	OldestHumanPlayer(		) const;							//!< the oldest human player
    ePlayerNetID*	OldestAIPlayer	(		) const;							//!< the oldest AI player
    ePlayerNetID*	YoungestPlayer	(		) const;							//!< the youngest player
    ePlayerNetID*	YoungestHumanPlayer(	) const;							//!< the youngest human player
    ePlayerNetID*	YoungestAIPlayer(		) const;							//!< the youngest AI player
    bool			Alive			(		) const;							//!< is any of the players currently alive?
    bool            IsReady         (       ) const;                            //!< is the team ready to play?

    // color
    tShortColor const & Color() const
    {
        return color;
    }
    
    // name and color
    unsigned short	R() 	const {
        return color.r_;
    }
    unsigned short	G() 	const {
        return color.g_;
    }
    unsigned short	B() 	const {
        return color.b_;
    }
    const tString& 	Name() 	const {
        return name;
    }

    tColoredString GetColoredName(void) const;

    virtual void PrintName(tString &s) const;					// print out an understandable name in to s


    virtual bool ClearToTransmit(int user) const;		// we must not transmit an object that contains pointers to non-transmitted objects. this function is supposed to check that.

    //! creates a netobject form sync data
    eTeam( Engine::TeamSync const &, nSenderInfo const & );
    //! reads incremental sync data. Returns false if sync was invalid or old.
    void ReadSync( Engine::TeamSync const &, nSenderInfo const & );
    //! writes sync data (and initialization data if flat is set)
    void WriteSync( Engine::TeamSync &, bool init );
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;

    // shall the server accept sync messages from the clients?
    virtual bool AcceptClientSync() const	{
        return false;
    }

    // con/desstruction
    eTeam();											// default constructor
    ~eTeam();											// destructor

private:
    void 	 		RemovePlayerDirty( ePlayerNetID* player );				// just remove a player from the player list, no messages, no balancing
};

#endif

