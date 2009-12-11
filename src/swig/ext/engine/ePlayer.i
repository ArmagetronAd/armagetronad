%{
#include "ePlayer.h"
%}

%include "std_list.i"


%rename(Player) ePlayer;
class ePlayer: public uPlayerPrototype {
public:
    virtual const char *Name() const;
    virtual const char *Teamname() const;
    int ID() const;

    static ePlayer * PlayerConfig(int p);
    static bool PlayerIsInGame(int p);
};

//! class managing access levels.
%rename(AccessLevelHolder) eAccessLevelHolder;
class eAccessLevelHolder {
public:
    eAccessLevelHolder();

    tAccessLevel GetAccessLevel() const;
    void SetAccessLevel( tAccessLevel level );
};

// the class that identifies players across the network
%rename(PlayerNetID) ePlayerNetID;
class ePlayerNetID: public nNetObject, public eAccessLevelHolder {
public:
    bool IsChatting() const;
    bool IsSpectating() const;
    bool StealthMode() const;

    // team management
    bool TeamChangeAllowed( bool informPlayer = false ) const; //!< is this player allowed to change teams?
    void SetTeamChangeAllowed(bool allowed) {allowTeamChange_ = allowed;} //!< set if this player should always be allowed to change teams

    eTeam* NextTeam()    const;		// return the team I will be next round
    eTeam* CurrentTeam() const;		// return the team I am in
    int  TeamListID() const;		// return my position in the team
    eTeam* FindDefaultTeam();		// find a good default team for us
    void SetDefaultTeam();		// register me in a good default team
    void SetTeam(eTeam* team);		// register me in the given team (callable on the server)
    void SetTeamWish(eTeam* team);	// express the wish to be part of the given team (always callable)
    void SetTeamname(const char *);	// set teamname to be used for my own team
    void UpdateTeamForce();		// update team membership without checks
    void UpdateTeam();			// update team membership

    void CreateNewTeam(); 	    	// create a new team and join it (on the server)
    void CreateNewTeamWish();	 	// express the wish to create a new team and join it

    static bool Enemies( ePlayerNetID const * a, ePlayerNetID const * b ); //!< determines whether two players are opponents and can score points against each other

    // print out an understandable name in to s
    virtual void PrintName(tString &s) const;

    void RemoveFromGame();

    // suspend the player from playing, forcing him to spectate
    void Suspend( int rounds = 5 );
#ifdef KRAWALL_SERVER
    void Authenticate( tString const & authName, 
                       tAccessLevel accessLevel = tAccessLevel_Authenticated,
                       ePlayerNetID const * admin = 0,
                       bool messages = true );    //!< make the authentification valid
    void DeAuthenticate( ePlayerNetID const * admin = 0, bool messages = true );  //!< make the authentification invalid
    bool IsAuthenticated() const;                     //!< is the authentification valid?
#endif

    bool IsActive() const;
    bool IsSilenced( void ) const;
    void SetSilenced( bool silenced );
    bool IsSuspended ( void );
    virtual bool IsHuman() const;

    eNetGameObject *Object() const;

    void AddScore(int points, const tOutput& reasonwin, const tOutput& reasonlose);
    int Score() const;
    int TotalScore() const;

    static void SortByScore();					// brings the players into the right order
    static tString Ranking( int MAX=12, bool cut = true );	// returns a ranking list
    static float RankingGraph( float y, int MAX );		// prints a ranking list

    static void  ResetScore();	// resets the ranking list

    static void RemoveChatbots();	//!< removes chatbots and idling players from the game
    static void CompleteRebuild();	// same as above, but rebuilds every ePlayerNetID.
    static void ClearAll();		// deletes all ePlayerNetIDs.
    static void SpectateAll( bool spectate=true ); // puts all players into spectator mode.

    void Chat(const tString &s);
    
    virtual void Color( REAL&r, REAL&g, REAL&b ) const;
    virtual void TrailColor( REAL&r, REAL&g, REAL&b ) const;

    //Remote Admin add-ins...
    bool IsLoggedIn() const;
    void BeLoggedIn();
    void BeNotLoggedIn();
    tAccessLevel GetLastAccessLevel() const;

    //!< finds a player by name using lax name matching. Reports errors to the console or to the requesting player.
    static ePlayerNetID * FindPlayerByName( tString const & name, ePlayerNetID * requester = 0, bool print=true );

    static void FilterName( tString const & in, tString & out ); //!< filters a name (removes unprintables, color codes and spaces)
    static tString FilterName( tString const & in );             //!< filters a name (removes unprintables, color codes and spaces)
    bool IsAllowedToRename ( void );                             //!< tells if the user can rename or not, takes care about everything
    void AllowRename( bool allow );                              //!< Allows a player to rename (or not)

    inline tString const & GetName( void ) const;		 //!< Gets this player's name without colors.
    inline ePlayerNetID const & GetName( tString & name ) const; //!< Gets this player's name without colors.

    inline tString const & GetUserName( void ) const;		 //!< Gets this player's full name. Use for writing to files or comparing with admin input.
    inline ePlayerNetID const & GetUserName( tString & userName ) const;	//!< Gets this player's name, cleared for system logs. Use for writing to files or comparing with admin input.

    tString const & GetLogName( void ) const;		//!< Gets this player's name, cleared for system logs (with escaped special characters). Use for writing to files.
    tString GetFilteredAuthenticatedName( void ) const;	//!< Gets the filtered, ecaped authentication name
#ifdef KRAWALL_SERVER
    tString const & GetRawAuthenticatedName( void ) const;	//!< Gets the raw, unescaped authentication name
    void SetRawAuthenticatedName( tString const & name );	//!< Sets the raw, unescaped authentication name
#endif

    ePlayerNetID & SetName( char    const * name ); //!< Sets this player's name. Sets processed names (colored, username, nameFromCLient) as well.

    inline ePlayerNetID & SetUserName( tString const & userName );  //!< Sets this player's name, cleared for system logs. Use for writing to files or comparing with admin input. The other names stay unaffected.
};


//! create a global instance of this to write stuff to ladderlog.txt
%template(eLadderLogWriterList) std::list<eLadderLogWriter*>;
%rename(LadderLogWriter) eLadderLogWriter;
class eLadderLogWriter {
public:
    eLadderLogWriter(char const *ID, bool enabledByDefault);
    ~eLadderLogWriter();
%rename(get_writer) getWriter;
    static eLadderLogWriter *getWriter(char const *ID);
%rename(is_enabled) isEnabled;
    bool isEnabled();	//!< check this if you're going to make expensive calculations for ladderlog output
%rename(set_all) setAll;
    static void setAll(bool enabled); //!< enable or disable all writers
    // bind a procedure from scripting language to this ladder log writer.
%rename(set_callback) setCallback;
    void setCallback(tScripting::proc_type proc);
};

