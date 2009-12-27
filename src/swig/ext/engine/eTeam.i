%{
#include "eTeam.h"
%}

%template(PlayersList) std::vector<ePlayerNetID *>;

%extend eTeam {
    static int num_teams()  { return eTeam::teams.Len(); }
    static eTeam *team(int i) { return eTeam::teams[i]; }
};

%rename(Team) eTeam;
class eTeam: public nNetObject{
public:					// public configuration options
%rename(rounds_played) RoundsPlayed;
    int RoundsPlayed() const;       //!< number of rounds played (updated right after spawning, so it includes the current round)

%rename(set_locked) SetLocked;
    void SetLocked( bool locked );  // sets the lock status (whether invitations are required)
%rename(is_locked) IsLocked;
    bool IsLocked() const;          // returns the lock status
%rename(is_locked_for) IsLockedFor;
    bool IsLockedFor( const ePlayerNetID * p ) const;   // returns if a team is locked to join by a certain player (due to ACCESS_LEVEL_PLAY)

%rename(invite) Invite;
    void Invite( ePlayerNetID * player );                // invite the player to join
%rename(uninvite) UnInvite;
    void UnInvite( ePlayerNetID * player );              // revoke an invitation
%rename(is_invited) IsInvited;
    bool IsInvited( ePlayerNetID const * player ) const; // check if a player is invited

%rename(is_enemy_player) Enemies( eTeam const * team, ePlayerNetID const * player );
    static bool Enemies( eTeam const * team, ePlayerNetID const * player ); //!< determines whether the player is an enemy of the team
%rename(is_enemy_team) Enemies( eTeam const * team1, eTeam const * team2 );
    static bool Enemies( eTeam const * team1, eTeam const * team2 ); //!< determines whether two teams are enemies
    
%rename(ranking) Ranking;
    static tString Ranking( int MAX = 6, bool cut = true );				// return ranking information
%rename(ranking_graph) RankingGraph;
    static float RankingGraph( float y, int MAX = 6 );				// print ranking information

%rename(add_player) AddPlayer;
    void 			AddPlayer   	( ePlayerNetID* player );				// register a player
%rename(remove_player) RemovePlayer;
    void 	 		RemovePlayer	( ePlayerNetID* player );				// deregister a player
%rename(player_may_join) PlayerMayJoin;
    virtual bool	PlayerMayJoin	( const ePlayerNetID* player ) const;		// see if the given player may join this team
%rename(is_new_team_allowed) NewTeamAllowed;
    static bool 	NewTeamAllowed	();											// is it allowed to create a new team?

%rename(swap_players) SwapPlayers;
    static void     SwapPlayers     ( ePlayerNetID* player1, ePlayerNetID *player2 ); //!< swaps the team positions of the two players (same team or not)

%rename(shuffle) Shuffle;
    void            Shuffle         ( int startID, int stopID ); //!< shuffles the player at team postion startID to stopID

%rename(balance_this_team) BalanceThisTeam;
    virtual bool BalanceThisTeam() const;
%rename(is_human) IsHuman;
    virtual bool IsHuman() const;

%rename(team_id) TeamID;
    int		TeamID			( void  ) const;

%rename(score) Score;
    int		Score			() const;
%rename(add_score) AddScore;
    void	AddScore		( int s );
%rename(reset_score) ResetScore;
    void	ResetScore		();
%rename(set_score) SetScore;
    void	SetScore		( int s );

%rename(add_score_output) AddScore;
    void 	AddScore		( int points,
                        const tOutput& reasonwin,
                        const tOutput& reasonlose );

    // player inquiry
%rename(num_players) NumPlayers;
    int	 		NumPlayers	() const;
%rename(player) Player;
    ePlayerNetID*	Player		( int i ) const;

    // Export a list of pointers to all the players in this team
%rename(players) GetAllMembers;
    std::vector < ePlayerNetID * > 	GetAllMembers () const;


%rename(num_human_players) NumHumanPlayers;
    int	 	NumHumanPlayers	() const; 		// number of human players
%rename(num_ai_players) NumAIPlayers;
    int	 	NumAIPlayers	() const; 		// number of AI players
%rename(alive_players) AlivePlayers;
    int	 	AlivePlayers	() const;		// how many of the current players are currently alive?
%rename(oldest_player) OldestPlayer;
    ePlayerNetID*	OldestPlayer		() const;		// the oldest player
%rename(oldest_human_player) OldestHumanPlayer;
    ePlayerNetID*	OldestHumanPlayer	() const;		// the oldest human player
%rename(oldest_ai_player) OldestAIPlayer;
    ePlayerNetID*	OldestAIPlayer		() const;		// the oldest AI player
%rename(youngest_player) YoungestPlayer;
    ePlayerNetID*	YoungestPlayer		() const;		// the youngest player
%rename(youngest_human_player) YoungestHumanPlayer;
    ePlayerNetID*	YoungestHumanPlayer	() const;		// the youngest human player
%rename(youngest_ai_player) YoungestAIPlayer;
    ePlayerNetID*	YoungestAIPlayer	() const;		// the youngest AI player
%rename(is_alive) Alive;
    bool			Alive		() const;		// is any of the players currently alive?

    // name and color
%rename(red) R;
    unsigned short	R() 	const;
%rename(green) G;
    unsigned short	G() 	const;
%rename(blue) B;
    unsigned short	B() 	const;
%rename(name) Name;
    const tString& 	Name() 	const;
};

