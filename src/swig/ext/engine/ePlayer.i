%{
#include "ePlayer.h"
#include "gCycle.h"
#include "nNetwork.h"
%}

%include "std_list.i"

%extend ePlayerNetID {
    gCycle *cycle() {
        gCycle* c = dynamic_cast< gCycle* >( $self->Object() );
        return c;
    }
    static ePlayerNetID * find_player( tString const & name ) { return ePlayerNetID::FindPlayerByName( name );}
    void center_message(tString const & msg) {
        sn_CenterMessage(msg, $self->Owner());
    }
};

%rename(PlayerConf) ePlayer;
class ePlayer: public uPlayerPrototype {
public:
%rename(name) Name;
    virtual const char *Name() const;
%rename(team_name) Teamname;
    virtual const char *Teamname() const;
%rename(id) ID;
    int ID() const;

%rename(player_config) PlayerConfig;
    static ePlayer * PlayerConfig(int p);
%rename(is_in_game) PlayerIsInGame;
    static bool PlayerIsInGame(int p);
};

//! class managing access levels.
%rename(AccessLevelHolder) eAccessLevelHolder;
class eAccessLevelHolder {
public:
    eAccessLevelHolder();

%rename(get_access_level) GetAccessLevel;
    tAccessLevel GetAccessLevel() const;
%rename(set_access_level) SetAccessLevel;
    void SetAccessLevel( tAccessLevel level );
};

%extend ePlayerNetID {
    static int players_count()  { return se_PlayerNetIDs.Len(); }
    static ePlayerNetID *players_get(int i) { return se_PlayerNetIDs[i]; }
};

// the class that identifies players across the network
%rename(Player) ePlayerNetID;
class ePlayerNetID: public nNetObject, public eAccessLevelHolder {
public:
%rename(is_chatting) IsChatting;
    bool IsChatting() const;
%rename(is_spectating) IsSpectating;
    bool IsSpectating() const;
%rename(stealth_mode) StealthMode;
    bool StealthMode() const;

    // team management
%rename(is_team_change_allowed) TeamChangeAllowed;
    bool TeamChangeAllowed( bool informPlayer = false ) const; //!< is this player allowed to change teams?
%rename(set_team_change_allowed) SetTeamChangeAllowed;
    void SetTeamChangeAllowed(bool allowed) {allowTeamChange_ = allowed;} //!< set if this player should always be allowed to change teams

%rename(next_team) NextTeam;
    eTeam* NextTeam()    const;		// return the team I will be next round
%rename(current_team) CurrentTeam;
    eTeam* CurrentTeam() const;		// return the team I am in
%rename(team_position) TeamListID;
    int  TeamListID() const;		// return my position in the team
%rename(find_default_team) FindDefaultTeam;
    eTeam* FindDefaultTeam();		// find a good default team for us
%rename(join_default_team) SetDefaultTeam;
    void SetDefaultTeam();		// register me in a good default team
%rename(join_team) SetTeam;
    void SetTeam(eTeam* team);		// register me in the given team (callable on the server)
%rename(join_team_wish) SetTeamWish;
    void SetTeamWish(eTeam* team);	// express the wish to be part of the given team (always callable)
%rename(team_name) SetTeamname;
    void SetTeamname(const char *);	// set teamname to be used for my own team
%rename(update_team) UpdateTeam;
    void UpdateTeam();			// update team membership

%rename(create_new_team) CreateNewTeam;
    void CreateNewTeam(); 	    	// create a new team and join it (on the server)
%rename(create_new_team_wish) CreateNewTeamWish;
    void CreateNewTeamWish();	 	// express the wish to create a new team and join it

%rename(is_enemy) Enemies;
    static bool Enemies( ePlayerNetID const * a, ePlayerNetID const * b ); //!< determines whether two players are opponents and can score points against each other

%rename(remove_from_game) RemoveFromGame;
    void RemoveFromGame();

    // suspend the player from playing, forcing him to spectate
%rename(suspend) Suspend;
    void Suspend( int rounds = 5 );
#ifdef KRAWALL_SERVER
%rename(authenticate) Authenticate;
    void Authenticate( tString const & authName, 
                       tAccessLevel accessLevel = tAccessLevel_Authenticated,
                       ePlayerNetID const * admin = 0,
                       bool messages = true );    //!< make the authentification valid
%rename(deauthenticate) DeAuthenticate;
    void DeAuthenticate( ePlayerNetID const * admin = 0, bool messages = true );  //!< make the authentification invalid
%rename(is_authenticated) IsAuthenticated;
    bool IsAuthenticated() const;                     //!< is the authentification valid?
#endif

%rename(is_active) IsActive;
    bool IsActive() const;
%rename(is_silenced) IsSilenced;
    bool IsSilenced( void ) const;
%rename(set_silenced) SetSilenced;
    void SetSilenced( bool silenced );
%rename(is_suspended) IsSuspended;
    bool IsSuspended ( void );
%rename(is_human) IsHuman;
    virtual bool IsHuman() const;

%rename(add_score_output) AddScore;
    void AddScore(int points, const tOutput& reasonwin, const tOutput& reasonlose);
%rename(score) Score;
    int Score() const;
%rename(total_score) TotalScore;
    int TotalScore() const;

%rename(ranking) Ranking;
    static tString Ranking( int MAX=12, bool cut = true );	// returns a ranking list
%rename(ranking_graph) RankingGraph;
    static float RankingGraph( float y, int MAX );		// prints a ranking list

%rename(reset_score) ResetScore;
    static void  ResetScore();	// resets the ranking list

%rename(remove_chatbots) RemoveChatbots;
    static void RemoveChatbots();	//!< removes chatbots and idling players from the game
%rename(complete_rebuild) CompleteRebuild;
    static void CompleteRebuild();	// same as above, but rebuilds every ePlayerNetID.
%rename(clear_all) ClearAll;
    static void ClearAll();		// deletes all ePlayerNetIDs.
%rename(spectate_all) SpectateAll;
    static void SpectateAll( bool spectate=true ); // puts all players into spectator mode.

%rename(chat) Chat;
    void Chat(const tString &s);
    
%rename(set_color) Color;
    virtual void Color( REAL&r, REAL&g, REAL&b ) const;
%rename(set_trail_color) TrailColor;
    virtual void TrailColor( REAL&r, REAL&g, REAL&b ) const;

    //Remote Admin add-ins...
%rename(is_logged_in) IsLoggedIn;
    bool IsLoggedIn() const;
%rename(be_logged_in) BeLoggedIn;
    void BeLoggedIn();
%rename(be_not_logged_in) BeNotLoggedIn;
    void BeNotLoggedIn();
%rename(get_last_access_level) GetLastAccessLevel;
    tAccessLevel GetLastAccessLevel() const;

    //!< finds a player by name using lax name matching. Reports errors to the console or to the requesting player.
//%rename(find_player) FindPlayerByName;
//    static ePlayerNetID * FindPlayerByName( tString const & name, ePlayerNetID * requester = 0, bool print=true );

%rename(filter_name) FilterName;
    static tString FilterName( tString const & in );             //!< filters a name (removes unprintables, color codes and spaces)
%rename(is_allowed_to_rename) IsAllowedToRename;
    bool IsAllowedToRename ( void );                             //!< tells if the user can rename or not, takes care about everything
%rename(allow_rename) AllowRename;
    void AllowRename( bool allow );                              //!< Allows a player to rename (or not)

%rename(name) GetName;
    inline tString const & GetName( void ) const;		 //!< Gets this player's name without colors.

%rename(user_name) GetUserName;
    inline tString const & GetUserName( void ) const;		 //!< Gets this player's full name. Use for writing to files or comparing with admin input.

%rename(log_name) GetLogName;
    tString const & GetLogName( void ) const;		//!< Gets this player's name, cleared for system logs (with escaped special characters). Use for writing to files.
%rename(filtered_authenticated_name) GetFilteredAuthenticatedName;
    tString GetFilteredAuthenticatedName( void ) const;	//!< Gets the filtered, ecaped authentication name

%rename(set_name) SetName;
    ePlayerNetID & SetName( char    const * name ); //!< Sets this player's name. Sets processed names (colored, username, nameFromCLient) as well.

%rename(set_user_name) SetUserName;
    inline ePlayerNetID & SetUserName( tString const & userName );  //!< Sets this player's name, cleared for system logs. Use for writing to files or comparing with admin input. The other names stay unaffected.
};


//! create a global instance of this to write stuff to ladderlog.txt
%template(eLadderLogWriterList) std::list<eLadderLogWriter*>;
%rename(LadderLogWriter) eLadderLogWriter;
class eLadderLogWriter {
public:
    eLadderLogWriter(char const *ID, bool enabledByDefault);
    ~eLadderLogWriter();
%rename(find) getWriter;
    static eLadderLogWriter *getWriter(char const *ID);
%rename(is_enabled) isEnabled;
    bool isEnabled();	//!< check this if you're going to make expensive calculations for ladderlog output
%rename(enabled) Enabled;
    void Enabled(bool b); //!< set or unset enabled flag
%rename(set_all) setAll;
    static void setAll(bool enabled); //!< enable or disable all writers
    // bind a procedure from scripting language to this ladder log writer.
%rename(set_callback) setCallback;
    void setCallback(sCallable::ptr proc);
    // remove a procedure from scripting language previously binded to this ladder log writer.
%rename(unset_callback) unsetCallback;
    void unsetCallback(sCallable::ptr proc);
};

//! chat command
%rename(ChatCommand) eChatCommand;
class eChatCommand {
public:
    eChatCommand(const char *ID, sCallable::ptr proc, tAccessLevel level);
    ~eChatCommand();
%rename(set_access_level) setAccessLevel;
    void setAccessLevel(tAccessLevel level);
%rename(find) getChatCommand;
    static eChatCommand *getChatCommand(char const *ID);
};


