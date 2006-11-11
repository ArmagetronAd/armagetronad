%{
#include "ePlayer.h"
%}

%rename(Player) ePlayer;
class ePlayer: public uPlayerPrototype{
public:
    tString    name;
    tString     teamname;
%rename(center_incam_on_turn) centerIncamOnTurn;
    bool       centerIncamOnTurn;
%rename(wobble_incam) wobbleIncam;
    bool       wobbleIncam;
%rename(auto_switch_incam) autoSwitchIncam;
    bool       autoSwitchIncam;
    bool       spectate;
%rename(name_team_after_me) nameTeamAfterMe;
    bool 		nameTeamAfterMe;
%rename(favorite_number_of_players_per_team) favoriteNumberOfPlayersPerTeam;
    int			favoriteNumberOfPlayersPerTeam;
%rename(start_camera) startCamera;
    eCamMode startCamera;
%rename(allow_cam) allowCam;
    bool     allowCam[CAMERA_COUNT];
%rename(start_fov) startFOV;
    int      startFOV;
%rename(smart_custom_glance) smartCustomGlance;
    bool     smartCustomGlance; //!< flag making the smart camera use the custom settings for glancing

    // tCHECKED_PTR(eCamera)           cam;
// %rename(net_player) netPlayer;
    // tCONTROLLED_PTR(ePlayerNetID) netPlayer;

    int rgb[3]; // our color

%rename(instant_chat_string) instantChatString;
    tString instantChatString[MAX_INSTANT_CHAT];

    static uActionPlayer *se_instantChatAction[MAX_INSTANT_CHAT];

    ePlayer();
    virtual ~ePlayer();

    // virtual const char *Name() const;
    // virtual const char *Teamname() const;

    virtual bool Act(uAction *act,REAL x);

    int ID() const {return id;};
// #ifndef DEDICATED
//     void Render();
// #endif

    static ePlayer * PlayerConfig(int p);

    static bool PlayerIsInGame(int p);

    static rViewport * PlayerViewport(int p);

    static void Init();
    static void Exit();
};

%rename(PlayerNetID) ePlayerNetID;
class ePlayerNetID : public nNetObject{
    friend class ePlayer;
    friend class eTeam;
    friend class eNetGameObject;
    friend class tControlledPTR< ePlayerNetID >;

    int listID;                          // ID in the list of all players
    int teamListID;                      // ID in the list of the team

    bool							silenced_;		// flag indicating whether the player has been silenced

    nTimeAbsolute					timeJoinedTeam; // the time the player joined the team he is in now
	// SWIG-TODO
    // tCONTROLLED_PTR(eTeam)			nextTeam;		// the team we're in ( logically )
    // tCONTROLLED_PTR(eTeam)			currentTeam;	// the team we currently are spawned for
    // tCONTROLLED_PTR(eVoter)			voter_;			// voter assigned to this player
    // tCHECKED_PTR(eNetGameObject) object; // the object this player is
    // controlling
	
    int score; // points made so far
    int lastScore_; // last saved score

    int favoriteNumberOfPlayersPerTeam;		// join team if number of players on it is less than this; create new team otherwise
    bool nameTeamAfterMe; 					// player prefers to call his team after his name
    bool auth;            					// is this user valid?
    bool greeted;        					// did the server already greet him?
    bool disconnected;   					// did he disconnect from the game?

    static void SwapPlayersNo(int a,int b); // swaps the players a and b

    ePlayerNetID& operator= (const ePlayerNetID&); // forbid copy constructor

    bool			spectating_; //!< are we currently spectating? Spectators don't get assigned to teams.
    bool			chatting_;   //!< are we currently chatting?
    int				chatFlags_;  //!< different types of reasons for beeing chatting

    //For improved remoteadmin
    bool            loggedIn;       //Is this user logged in?
    int             accessLevel;    //If so, what can they do? 0 is default, aka, guest
    //but if they are not logged in, this should never be an issue

    nMachine *      registeredMachine_; //!< the machine the player is registered with
    void RegisterWithMachine();         //!< registers with a machine
    void UnregisterWithMachine();       //!< un registers with a machine
public:
    enum			ChatFlags
    {
        ChatFlags_Chat = 1,
        ChatFlags_Away = 2,
        ChatFlags_Menu = 4,
        ChatFlags_Console = 4
    };

    int    pID;
    tString teamname;
    // REAL	rubberstatus;
    tArray<tString> lastSaid;
    tArray<nTimeRolling> lastSaidTimes;
    //	void SetLastSaid(tString ls);
    unsigned short r,g,b; // our color

%rename(ping_charity) pingCharity;
    unsigned short pingCharity; // max ping you are willing to take over

    REAL ping;

%rename(last_sync) lastSync;
    double lastSync;         //!< time of the last sync request
%rename(last_activity) lastActivity_;
    double lastActivity_;    //!< time of the last activity

// SWIG-TODO: causing compile errors
// %rename (chat_spam) chatSpam_;
//     nSpamProtection chatSpam_;

    ePlayerNetID(int p=-1);
    ePlayerNetID(nMessage &m);
    virtual ~ePlayerNetID();

    virtual bool ActionOnQuit();
    virtual void ActionOnDelete();

    // chatting
    bool IsChatting() const { return chatting_; }
    void SetChatting ( ChatFlags flag, bool chatting );

    // spectating
    bool IsSpectating() const { return spectating_; }

    // team management
    eTeam* NextTeam()    const { return nextTeam; }		// return the team I will be next round
    eTeam* CurrentTeam() const { return currentTeam; }	// return the team I am in
    int  TeamListID() const { return teamListID; }		// return my position in the team
    eTeam* FindDefaultTeam();					// find a good default team for us
    void SetDefaultTeam();						// register me in a good default team
    void SetTeamForce(eTeam* team );           	// register me in the given team without checks
    void SetTeam(eTeam* team);          		// register me in the given team (callable on the server)
    void SetTeamWish(eTeam* team); 				// express the wish to be part of the given team (always callable)
    void SetTeamname(const char *);				// set teamname to be used for my own team
    void UpdateTeamForce();						// update team membership without checks
    void UpdateTeam();							// update team membership

    void CreateNewTeam(); 	    				// create a new team and join it (on the server)
    void CreateNewTeamWish();	 				// express the wish to create a new team and join it
    virtual void ReceiveControlNet(nMessage &m);// receive the team control wish

    static bool Enemies( ePlayerNetID const * a, ePlayerNetID const * b ); //!< determines whether two players are opponents and can score points against each other

    // print out an understandable name in to s
    virtual void 			PrintName(tString &s) const;

    virtual bool 			AcceptClientSync() const;
    virtual void 			WriteSync(nMessage &m);
    virtual void 			ReadSync(nMessage &m);
    virtual nDescriptor&	CreatorDescriptor() const;
    virtual void			InitAfterCreation();
    virtual bool			ClearToTransmit(int user) const;

    virtual void 			NewObject(){}        				// called when we control a new object
    virtual void 			RightBeforeDeath(int triesLeft){} 	// is called right before the vehicle gets destroyed.


    void RemoveFromGame();
    void ControlObject(eNetGameObject *c);
    void ClearObject();

    void Greet();
    void Auth(); 										// make the authentification valid
    bool IsAuth() const; 								// is the authentification valid?
    bool IsActive() const { return !disconnected; }

    bool IsSilenced( void ) const { return silenced_; }
    void SetSilenced( bool silenced ) { silenced_ = silenced; }
    bool& AccessSilenced( void ) { return silenced_; }

    void CreateVoter();						// create our voter or find it
    static void SilenceMenu();				// menu where you can silence players
    static void PoliceMenu();				// menu where you can silence and kick players

    virtual bool IsHuman() const { return true; }

    void Activity(); // call it if this player just showed some activity.
    REAL LastActivity() const; //!< returns how long the last activity of this player was ago

    eNetGameObject *Object() const;

    // void SetRubber(REAL rubber2);
    void AddScore(int points, const tOutput& reasonwin, const tOutput& reasonlose);
    int Score()const {return score;}
    int TotalScore() const;
    static void ResetScoreDifferences(); //<! Resets the last stored score so ScoreDifferences takes this as a reference time
    static void LogScoreDifferences();   //<! Logs accumulated scores of all players since the last call to ResetScoreDifferences() to ladderlog.txt
    void LogScoreDifference();           //<! Logs accumulated scores since the last call to ResetScoreDifferences() to ladderlog.txt

    static void SortByScore(); // brings the players into the right order
    static tString Ranking( int MAX=12, bool cut = true );     // returns a ranking list
    static float RankingGraph( float y, int MAX, bool showTeam );     // prints a ranking list
    static void  ResetScore();  // resets the ranking list

    static void DisplayScores(); // display scores on the screen

    void GreetHighscores(tString &s); // tell him his positions in the
    // highscore lists (defined in game.cpp)

    static void Update();           // creates ePlayerNetIDs for new players
    // and destroys those of players that have left

    static bool WaitToLeaveChat(); //!< waits for players to leave chat state. Returns true if the caller should wait to proceed with whatever he wants to do.

    static void RemoveChatbots(); //!< removes chatbots and idling players from the game

    static void CompleteRebuild(); // same as above, but rebuilds every ePlayerNetID.
    static void ClearAll(); // deletes all ePlayerNetIDs.
    static void SpectateAll( bool spectate=true ); // puts all players into spectator mode.

    static void ThrowOutDisconnected(); // get rid of everyone that disconnected from the game

    void GetScoreFromDisconnectedCopy(); // get the player's data from the previous login

    void Chat(const tString &s);

    virtual void Color( REAL&r, REAL&g, REAL&b ) const;
    virtual void TrailColor( REAL&r, REAL&g, REAL&b ) const;

    //Remote Admin add-ins...
    bool isLoggedIn() const { return loggedIn; }
    void beLoggedIn() { loggedIn = true; }
    void beNotLoggedIn() { loggedIn = false; }
    int getAccessLevel() const { return accessLevel; }
    void setAccessLevel(int in) { accessLevel = in; }

    void UpdateName();                                           //! update the player name from the client's wishes
    static void FilterName( tString const & in, tString & out ); //! filters a name (removes unprintables, color codes and spaces)
    static tString FilterName( tString const & in );             //! filters a name (removes unprintables, color codes and spaces)
private:
    tColoredString  nameFromClient_;        //! this player's name as the client wants it to be. Avoid using it when possilbe.
    tColoredString  nameFromServer_;        //! this player's name as the server wants it to be. Avoid using it when possilbe.
    tColoredString  coloredName_;           //! this player's name, cleared by the server. Use this for onscreen screen display.
    tString         name_;                  //! this player's name without colors.
    tString         userName_;              //! this player's name, cleared for system logs. Use for writing to files or comparing with admin input.

    REAL            wait_;                  //! time in seconds WaitToLeaveChat() will wait for this player

    void			MyInitAfterCreation();

protected:
    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to

    // private:
    //	virtual void AddRef();
    //	virtual void Release();

    // accessors
public:
    inline tColoredString const & GetNameFromClient( void ) const;
    inline ePlayerNetID const & GetNameFromClient( tColoredString & nameFromClient ) const;
    inline tColoredString const & GetColoredName( void ) const;
    inline ePlayerNetID const & GetColoredName( tColoredString & coloredName ) const;
    inline tString const & GetName( void ) const;
    inline ePlayerNetID const & GetName( tString & name ) const;
    inline tString const & GetUserName( void ) const;
    inline ePlayerNetID const & GetUserName( tString & userName ) const;

    ePlayerNetID & SetName( tString const & name );
    ePlayerNetID & SetName( char    const * name );
    inline ePlayerNetID & SetUserName( tString const & userName ); 
private:
    inline ePlayerNetID & SetNameFromClient( tColoredString const & nameFromClient );   //!< Sets this player's name as the client wants it to be. Avoid using it when possilbe.
    inline ePlayerNetID & SetColoredName( tColoredString const & coloredName ); //!< Sets this player's name, cleared by the server. Use this for onscreen screen display.
};