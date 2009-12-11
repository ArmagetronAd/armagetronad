%{
#include "eTeam.h"
%}

%rename(Team) eTeam;
class eTeam: public nNetObject{
public:					// public configuration options
    static int  minTeams;		// minimum nuber of teams
    static int  maxTeams;    		// maximum nuber of teams
    static int  maxPlayers;   		// maximum number of players per team
    static int  minPlayers;   		// maximum number of players per team
    static int  maxImbalance;		// maximum difference of player numbers allowed
    static bool balanceWithAIs;		// use AI players to balance the teams?
    static bool enforceRulesOnQuit; 	// if the quitting of one player unbalances the teams, enforce the rules by redistributing

    static eTeam *GetTeam(int i);
    static int NbTeams();

    int RoundsPlayed() const;       //!< number of rounds played (updated right after spawning, so it includes the current round)

    void SetLocked( bool locked );  // sets the lock status (whether invitations are required)
    bool IsLocked() const;          // returns the lock status
    bool IsLockedFor( const ePlayerNetID * p ) const;   // returns if a team is locked to join by a certain player (due to ACCESS_LEVEL_PLAY)

    void Invite( ePlayerNetID * player );                // invite the player to join
    void UnInvite( ePlayerNetID * player );              // revoke an invitation
    bool IsInvited( ePlayerNetID const * player ) const; // check if a player is invited

    static bool Enemies( eTeam const * team, ePlayerNetID const * player ); //!< determines whether the player is an enemy of the team
    static bool Enemies( eTeam const * team1, eTeam const * team2 ); //!< determines whether two teams are enemies
    
    static void SortByScore();							// brings the teams into the right order

    static void SwapTeamsNo(int a,int b);             	// swaps the teams a and b

    static tString Ranking( int MAX = 6, bool cut = true );				// return ranking information
    static float RankingGraph( float y, int MAX = 6 );				// print ranking information

    void 			AddPlayer   	( ePlayerNetID* player );				// register a player
    void 	 		RemovePlayer	( ePlayerNetID* player );				// deregister a player
    virtual bool	PlayerMayJoin	( const ePlayerNetID* player ) const;		// see if the given player may join this team
    static bool 	NewTeamAllowed	();											// is it allowed to create a new team?

    static void     SwapPlayers     ( ePlayerNetID* player1, ePlayerNetID *player2 ); //!< swaps the team positions of the two players (same team or not)

    void            Shuffle         ( int startID, int stopID ); //!< shuffles the player at team postion startID to stopID

    virtual bool BalanceThisTeam() const;
    virtual bool IsHuman() const;

    int		TeamID			( void  ) const;

    int		Score			() const;
    void	AddScore		( int s );
    void	ResetScore		();
    void	SetScore		( int s );

    void 	AddScore		( int points,
                        const tOutput& reasonwin,
                        const tOutput& reasonlose );

    static void ResetScoreDifferences(); //<! Resets the last stored score so ScoreDifferences takes this as a reference time
    static void LogScoreDifferences();   //<! Logs accumulated scores of all players since the last call to ResetScoreDifferences() to ladderlog.txt
    void LogScoreDifference();           //<! Logs accumulated scores since the last call to ResetScoreDifferences() to ladderlog.txt

    // player inquiry
    int	 		NumPlayers	() const;
    ePlayerNetID*	Player		( int i ) const;

    // Export a list of pointers to all the players in this team
    std::vector < ePlayerNetID * > 	GetAllMembers () const;


    int	 	NumHumanPlayers	() const; 		// number of human players
    int	 	NumAIPlayers	() const; 		// number of human players
    int	 	AlivePlayers	() const;		// how many of the current players are currently alive?
    ePlayerNetID*	OldestPlayer		() const;		// the oldest player
    ePlayerNetID*	OldestHumanPlayer	() const;		// the oldest human player
    ePlayerNetID*	OldestAIPlayer		() const;		// the oldest AI player
    ePlayerNetID*	YoungestPlayer		() const;		// the youngest player
    ePlayerNetID*	YoungestHumanPlayer	() const;		// the youngest human player
    ePlayerNetID*	YoungestAIPlayer	() const;		// the youngest AI player
    bool			Alive		() const;		// is any of the players currently alive?

    // color
    tShortColor const & Color() const;
    
    // name and color
    unsigned short	R() 	const;
    unsigned short	G() 	const;
    unsigned short	B() 	const;
    const tString& 	Name() 	const;

    tColoredString GetColoredName(void) const;

    virtual void PrintName(tString &s) const;			// print out an understandable name in to s
};

