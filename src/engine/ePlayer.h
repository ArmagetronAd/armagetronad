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

#ifndef ArmageTron_PLAYER_H
#define ArmageTron_PLAYER_H

#ifndef MAX_INSTANT_CHAT
#define MAX_INSTANT_CHAT 26
#endif

#define MAX_PLAYERS 4

#include "rSDL.h"

#include "uInput.h"
#include "tList.h"
#include "tString.h"
#include "eCamera.h"
#include "eNetGameObject.h"
#include "tCallbackString.h"
#include "nSpamProtection.h"
#include "tColor.h"

#include <set>
#include <list>
#include <utility>
#include "eChat.h"

namespace Engine{ class PlayerNetIDSync; }

#define PLAYER_CONFITEMS (30+MAX_INSTANT_CHAT)

// maximal length of chat message
extern int se_SpamMaxLen;

// Maximum number of chat entries to save for spam analysis
extern int se_lastSaidMaxEntries;

// time during which no repeaded chat messages are printed
extern REAL se_alreadySaidTimeout;

// minimal access level for chat
extern tAccessLevel se_chatAccessLevel;

// time between public chat requests, set to 0 to disable
extern REAL se_chatRequestTimeout;

// call on commands that only work on the server; quit if it returns true
bool se_NeedsServer(char const * command, std::istream & s, bool strict = true );

class tConfItemBase;
class uAction;
class tOutput;
class eTeam;
class eVoter;

class eCockpitPrototype : public tReferencable<eCockpitPrototype> {
public:
    virtual ~eCockpitPrototype(){};
};

class ePlayer: public uPlayerPrototype{
    friend class eMenuItemChat;
    static uActionPlayer s_chat;
    static uActionTooltip s_chatTooltip;

    tConfItemBase *configuration[PLAYER_CONFITEMS];
    int            CurrentConfitem;
    void   StoreConfitem(tConfItemBase *c);
    void   DeleteConfitems();

    double lastTooltip_;
public:
    tString    name;                 // the player's screen name
    tString    globalID;             // the global ID of the player in user@authority form
    // REAL	   rubberstatus;
    tString     teamname;
    bool       centerIncamOnTurn;
    bool       wobbleIncam;
    bool       autoSwitchIncam;

    bool       spectate;              // shall this player always spectate?
    bool       stealth;               // does this player wish to hide his/her identity?
    bool       autoLogin;             // should the player always request authentication on servers?

    bool 		nameTeamAfterMe; // player prefers to call his team after his name
    int			favoriteNumberOfPlayersPerTeam;

    eCamMode startCamera;
    bool     allowCam[CAMERA_COUNT];
    int      startFOV;
    bool     smartCustomGlance; //!< flag making the smart camera use the custom settings for glancing

    tCHECKED_PTR(eCamera)           cam;
    tCONTROLLED_PTR(ePlayerNetID) netPlayer;
    tJUST_CONTROLLED_PTR<eCockpitPrototype>	cockpit;

    int rgb[3]; // our color

    tString instantChatString[MAX_INSTANT_CHAT];
    // instant chat macros

    static uActionPlayer *se_instantChatAction[MAX_INSTANT_CHAT];

    ePlayer();
    virtual ~ePlayer();

    virtual const char *Name() const{return name;}
    virtual const char *Teamname() const{return teamname;}

    virtual bool Act(uAction *act,REAL x);

    int ID() const {return id;}
#ifndef DEDICATED
    void Render();
#endif

    static ePlayer * PlayerConfig(int p);

    static bool PlayerIsInGame(int p);

    // veto function for tooltips that require a controllable game object
    static bool VetoActiveTooltip(int player);

    static rViewport * PlayerViewport(int p);

    static void LogIn();          //!< sends authentication login messages for all local players
    static void SendAuthNames();  //!< sends authentication names and authentication wishes for all local players

    static void Init();
    static void Exit();
};

//! class managing access levels.
class eAccessLevelHolder
{
public:
    eAccessLevelHolder();

    tAccessLevel GetAccessLevel() const { return accessLevel; }
    void SetAccessLevel( tAccessLevel level );

private:
    tAccessLevel     accessLevel;    //!< admin access level of the current user
};

//! detector for turn timing assist bots
class eUncannyTimingDetector
{
public:
    //! settings for a single analyzer
    struct eUncannyTimingSettings
    {
        REAL timescale; //!< the timescale. Events are divided in two buckets, one between 0 and timescale/2, the other from timescale/2 to timescale.
        REAL maxGoodRatio; //!< the maximal allowed recent ratio of events to land in the 'good' bucket
        REAL goodHumanRatio; //!< the maximal observed ratio for a human
        int  averageOverEvents; //!< number of events to average over
        
        mutable REAL bestRatio; //!< best ratio achieved by players during this session

        eUncannyTimingSettings( REAL ts, REAL human, REAL max )
        : timescale( ts ), maxGoodRatio( max ), goodHumanRatio(human), averageOverEvents(40)
        , bestRatio(0)
        {}

        ~eUncannyTimingSettings();
    };

    //! single analyzer with single timescale
    class eUncannyTimingAnalysis
    {
    public:
        //! analyze a single timing event
        REAL Analyze( REAL timing, eUncannyTimingSettings const & settings );
        eUncannyTimingAnalysis();
    private:
        REAL accurateRatio; //!< ratio of events in the more accurate half
        int turnsSoFar;     //!< number of turns accounted for so far
    };

    //! detection level of timing aid hacks
    enum DangerLevel
    {
        DangerLevel_Low,    //!< about 25% of the tolerance reached
        DangerLevel_Medium, //!< about 50% of the tolerance reached
        DangerLevel_High,   //!< about 75% of the tolerance reached
        DangerLevel_Max     //!< 100% of the tolerance reached, worst action triggered
    };

    eUncannyTimingDetector();

    //! analzye a timing event
    void Analyze( REAL timing, ePlayerNetID * player );
private:
    //! three analyzers for varying timescales
    eUncannyTimingAnalysis fast, medium, slow;

    DangerLevel dangerLevel;
};

// the class that identifies players across the network
class ePlayerNetID: public nNetObject, public eAccessLevelHolder{
    friend class ePlayer;
    friend class eTeam;
    friend class eNetGameObject;
    friend class tControlledPTR< ePlayerNetID >;
    // access level. lower numeric values are better.
public:
    typedef std::set< eTeam * > eTeamSet;
    static const int MAX_NAME_LENGTH = 15;
private:

    int listID;                          // ID in the list of all players
    int teamListID;                      // ID in the list of the team

    bool							silenced_;		// flag indicating whether the player has been silenced
    int                             suspended_;     //! number of rounds the player is currently suspended from playing

    nTimeAbsolute                   timeCreated_;   // the time the player was created
    nTimeAbsolute					timeJoinedTeam; // the time the player joined the team he is in now
    tCONTROLLED_PTR(eTeam)			nextTeam;		// the team we're in ( logically )
    tCONTROLLED_PTR(eTeam)			currentTeam;	// the team we currently are spawned for
    eTeamSet                        invitations_;   // teams this player is invited to
    bool                            invitationsChanged_;

    tCHECKED_PTR(eNetGameObject) object; // the object this player is
    // controlling

    int score; // points made so far
    int lastScore_; // last saved score

    int favoriteNumberOfPlayersPerTeam;		// join team if number of players on it is less than this; create new team otherwise
    bool nameTeamAfterMe; 					// player prefers to call his team after his name
    bool greeted;        					// did the server already greet him? (On the client: was the first sync back already received?)
    bool disconnected;   					// did he disconnect from the game?

    static void SwapPlayersNo(int a,int b); // swaps the players a and b

    ePlayerNetID& operator= (const ePlayerNetID&); // forbid copy constructor

    bool			spectating_; //!< are we currently spectating? Spectators don't get assigned to teams.
    bool			stealth_; //!< does this player want to hide his/her identity?
    bool			chatting_;   //!< are we currently chatting?
    int				chatFlags_;  //!< different types of reasons for beeing chatting
    bool			allowTeamChange_; //!< allow team changes even if ALLOW_TEAM_CHANGE is disabled?

    //For improved remoteadmin
    tAccessLevel     lastAccessLevel;//!< access level at the time of the last name update

    eUncannyTimingDetector uncannyTimingDetector_; //!< detector for timingbots

    nMachine *      registeredMachine_; //!< the machine the player is registered with
    void RegisterWithMachine();         //!< registers with a machine
    void UnregisterWithMachine();       //!< un registers with a machine
public:
    enum			ChatFlags
    {
        ChatFlags_Chat = 1,
        ChatFlags_Away = 2,
        ChatFlags_Menu = 4,
        ChatFlags_Console = 8
    };

    int    pID;
    tString teamname;
    // REAL	rubberstatus;
        
    tShortColor color; // our color

    bool ready;

    unsigned short pingCharity; // max ping you are willing to take over

    REAL ping;

    double lastSync;         //!< time of the last sync request
    double lastActivity_;    //!< time of the last activity

    bool loginWanted;        //!< flag indicating whether this player currently wants to log on

    bool renameAllowed_;     //!< specifies if the player is allowed to rename or not, does not know about votes.

    nSpamProtection & GetChatSpam();       //!< chat volume spam
    eChatLastSaid & GetLastSaid();         //!< last said information
    eShuffleSpamTester & GetShuffleSpam(); //!< shuffle message spam

    ePlayerNetID(int p=-1);
    virtual ~ePlayerNetID();

    virtual bool ActionOnQuit();
    virtual void ActionOnDelete();

    // Check if a player can be respawned. Relaying on team alone is not enough.
    // If a player enters as spectator, they are still assumed to be on a team.
    // When a player is suspeded they are also on a team until the end of the round.
    bool CanRespawn() const { return currentTeam && suspended_ == 0 && ! spectating_; }
 
    // chatting
    bool IsChatting() const { return chatting_; }
    void SetChatting ( ChatFlags flag, bool chatting );

    // spectating
    bool IsSpectating() const { return spectating_; }

    bool StealthMode() const { return stealth_; }

    // team management
    bool TeamChangeAllowed( bool informPlayer = false ) const; //!< is this player allowed to change teams?
    void SetTeamChangeAllowed(bool allowed) {allowTeamChange_ = allowed;} //!< set if this player should always be allowed to change teams

    eTeam* NextTeam()    const { return nextTeam; }		// return the team I will be next round
    eTeam* CurrentTeam() const { return currentTeam; }	// return the team I am in
    int  TeamListID() const { return teamListID; }		// return my position in the team
    void SetShuffleWish( int pos ); 	        //!< sets a desired team position
    eTeam* FindDefaultTeam();					// find a good default team for us
    void SetDefaultTeam( bool overrideNoAuto=false );    // register me in a good default team
    void SetDefaultTeamWish();                  // register me in a good default team (broadcasts with to server on client)
    void SetTeamForce(eTeam* team );           	// register me in the given team without checks
    void SetTeam(eTeam* team);          		// register me in the given team (callable on the server)
    void SetTeamWish(eTeam* team); 				// express the wish to be part of the given team (always callable)
    void SetTeamname(const char *);				// set teamname to be used for my own team
    void UpdateTeamForce();						// update team membership without checks
    void UpdateTeam();							// update team membership

    void AddInvitation( eTeam *team );
    bool RemoveInvitation( eTeam *team );
    eTeamSet const & GetInvitations() const ;   //!< teams this player is invited to

    void CreateNewTeam(); 	    				// create a new team and join it (on the server)
    void CreateNewTeamWish();	 				// express the wish to create a new team and join it
    virtual void ReceiveControlNet( Network::NetObjectControl const & control );// receive the team control wish

    // easier to implement conversion helpers: just extract the relevant sub-protbuf.
    virtual nProtoBuf       * ExtractControl( Network::NetObjectControl       & control );
    virtual nProtoBuf const * ExtractControl( Network::NetObjectControl const & control );

    static bool Enemies( ePlayerNetID const * a, ePlayerNetID const * b ); //!< determines whether two players are opponents and can score points against each other

    // print out an understandable name in to s
    virtual void 			PrintName(tString &s) const;

    virtual bool 			AcceptClientSync() const;
    virtual void			InitAfterCreation();
    virtual bool			ClearToTransmit(int user) const;

    //! creates a netobject form sync data
    ePlayerNetID( Engine::PlayerNetIDSync const & sync, nSenderInfo const & sender );
    //! reads incremental sync data. Returns false if sync was invalid or old.
    void ReadSync( Engine::PlayerNetIDSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flat is set)
    void WriteSync( Engine::PlayerNetIDSync & sync, bool init );
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
public:

    static bool Scramble;                   // Should we scramble the teams?
    static std::vector<ePlayerNetID*> ScramblePlayerIDs; // List of all the players to be scrambled

    virtual void 			NewObject(){}        				// called when we control a new object
    virtual void 			RightBeforeDeath(int triesLeft){} 	// is called right before the vehicle gets destroyed.


    void RemoveFromGame();
    virtual void ControlObject(eNetGameObject *c);
    virtual void ClearObject();

    void Greet();

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

    static void RequestScheduledLogins();  //!< initiates login processes for all pending wishes

    bool IsActive() const { return !disconnected; }

    bool IsSilenced( void ) const { return silenced_; }
    void SetSilenced( bool silenced ); // { silenced_ = silenced; }

    // only for the menu
    bool& AccessSilenced( void ) { return silenced_; }

    bool IsSuspended ( void ) const { return suspended_ > 0; }
    int  RoundsSuspended ( void ) const { return suspended_; }
    bool IsGreeted() const { return greeted; }

    static void SilenceMenu();				// menu where you can silence players
    static void PoliceMenu();				// menu where you can silence and kick players

    virtual bool IsHuman() const { return true; }

    void Activity(); // call it if this player just showed some activity.
    REAL LastActivity() const; //!< returns how long the last activity of this player was ago

    eNetGameObject *Object() const;

    // void SetRubber(REAL rubber2);
    void AddScore(int points, const tOutput& reasonwin, const tOutput& reasonlose, const tOutput& reasonfree=tOutput());
    int Score()const {return score;}
    int TotalScore() const;
    static void ResetScoreDifferences(); //<! Resets the last stored score so ScoreDifferences takes this as a reference time
    static void LogScoreDifferences();   //<! Logs accumulated scores of all players since the last call to ResetScoreDifferences() to ladderlog.txt
    static void UpdateSuspensions();     //<! Decrements the number of rounds players are suspended for
    static void UpdateShuffleSpamTesters();    //<! Reset shuffle spam checks
    void LogScoreDifference();           //<! Logs accumulated scores since the last call to ResetScoreDifferences() to ladderlog.txt

    void AnalyzeTiming( REAL timing );   //<! analzye a timing event for timebot detection

    static void SortByScore(); // brings the players into the right order
    static tString Ranking( int MAX=12, bool cut = true );     // returns a ranking list

    static float RankingGraph( float y, int MAX );     // prints a ranking list

    static void  ResetScore();  // resets the ranking list

    static void DisplayScores(); // display scores on the screen

    void GreetHighscores(tString &s); // tell him his positions in the
    // highscore lists (defined in game.cpp)

    static ePlayerNetID * ReadPlayer( std::istream & s ); //!< reads a player from the stream

    static void Update();           // creates ePlayerNetIDs for new players
    // and destroys those of players that have left

#ifdef KRAWALL_SERVER
    static tAccessLevel AccessLevelRequiredToPlay(); // is authentication required to play on this server?
#endif

    static bool WaitToLeaveChat(); //!< waits for players to leave chat state. Returns true if the caller should wait to proceed with whatever he wants to do.

    static void RemoveChatbots(); //!< removes chatbots and idling players from the game
    static void SetScramble(); //!< Scramble the teams the next round
    static void ScrambleTeams(); //!< scramble the teams

    static void CompleteRebuild(); // same as above, but rebuilds every ePlayerNetID.
    static void ClearAll(); // deletes all ePlayerNetIDs.
    static void SpectateAll( bool spectate=true ); // puts all players into spectator mode.

    static void ThrowOutDisconnected(); // get rid of everyone that disconnected from the game

    void GetScoreFromDisconnectedCopy(); // get the player's data from the previous login

    void Chat(const tString &s);
    
    nTimeAbsolute GetTimeCreated() const { return timeCreated_; }

    virtual void Color( REAL&r, REAL&g, REAL&b ) const;
    virtual void TrailColor( REAL&r, REAL&g, REAL&b ) const;

    //Remote Admin add-ins...
    bool IsLoggedIn() const { return GetAccessLevel() < tAccessLevel_Moderator; }
    void BeLoggedIn() { SetAccessLevel( tAccessLevel_Admin ); }
    void BeNotLoggedIn() { SetAccessLevel( tAccessLevel_Program ); }
    tAccessLevel GetLastAccessLevel() const { return lastAccessLevel; }

    static ePlayerNetID * FindPlayerByName( tString const & name, ePlayerNetID * requester = 0, bool print=true ); //!< finds a player by name using lax name matching. Reports errors to the console or to the requesting player.

    void UpdateName();                                           //!< update the player name from either the client's wishes, either the admin's wishes.
    static void FilterName( tString const & in, tString & out ); //!< filters a name (removes unprintables, color codes and spaces)
    static tString FilterName( tString const & in );             //!< filters a name (removes unprintables, color codes and spaces)
    bool IsAllowedToRename ( void );                             //!< tells if the user can rename or not, takes care about everything
    void AllowRename( bool allow );                              //!< Allows a player to rename (or not)

    static bool HasRenameCapability ( ePlayerNetID const *, ePlayerNetID const * admin ); //!< Checks if the admin can use the RENAME command. Used in IsAllowedToRename()

private:
    tColoredString  nameFromClient_;        //!< this player's name as the client wants it to be. Avoid using it when possilbe.
    tColoredString  nameFromServer_;        //!< this player's name as the server wants it to be. Avoid using it when possilbe.
    tColoredString  nameFromAdmin_;         //!< this player's name as the admin wants it to be. Avoid using it when possilbe.
    tColoredString  coloredName_;           //!< this player's name, cleared by the server. Use this for onscreen screen display.
    tString         name_;                  //!< this player's name without colors.
    tString         userName_;              //!< this player's name, cleared for system logs. Use for writing to files or comparing with admin input.

#ifdef KRAWALL_SERVER
    tString         rawAuthenticatedName_;  //!< the raw authenticated name in user@authority form.
#endif

    REAL            wait_;                  //!< time in seconds WaitToLeaveChat() will wait for this player

    void CreateVoter();						// create our voter or find it
    void			MyInitAfterCreation();

protected:
    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to

    // private:
    //	virtual void AddRef();
    //	virtual void Release();

    // accessors
public:
    inline tColoredString const & GetNameFromClient( void ) const;	//!< Gets this player's name as the client wants it to be. Avoid using it when possilbe.
    inline ePlayerNetID const & GetNameFromClient( tColoredString & nameFromClient ) const;	//!< Gets this player's name as the client wants it to be. Avoid using it when possilbe.
    inline tColoredString const & GetColoredName( void ) const;	//!< Gets this player's name, cleared by the server. Use this for onscreen screen display.
    inline ePlayerNetID const & GetColoredName( tColoredString & coloredName ) const;	//!< Gets this player's name, cleared by the server. Use this for onscreen screen display.
    inline tString const & GetName( void ) const;	//!< Gets this player's name without colors.
    inline ePlayerNetID const & GetName( tString & name ) const;	//!< Gets this player's name without colors.

    inline tString const & GetUserName( void ) const;	//!< Gets this player's full name. Use for writing to files or comparing with admin input.
    inline ePlayerNetID const & GetUserName( tString & userName ) const;	//!< Gets this player's name, cleared for system logs. Use for writing to files or comparing with admin input.

    tString const & GetLogName( void ) const{ return GetUserName(); }	//!< Gets this player's name, cleared for system logs (with escaped special characters). Use for writing to files.
    tString GetFilteredAuthenticatedName( void ) const;	//!< Gets the filtered, ecaped authentication name
#ifdef KRAWALL_SERVER
    tString const & GetRawAuthenticatedName( void ) const{ return rawAuthenticatedName_; }	//!< Gets the raw, unescaped authentication name
    void SetRawAuthenticatedName( tString const & name ){ if ( !IsAuthenticated()) rawAuthenticatedName_ = name; }	//!< Sets the raw, unescaped authentication name
#endif

    ePlayerNetID & SetName( tString const & name ); //!< Sets this player's name. Sets processed names (colored, username, nameFromCLient) as well.
    ePlayerNetID & SetName( char    const * name ); //!< Sets this player's name. Sets processed names (colored, username, nameFromCLient) as well.
    ePlayerNetID & SetName( tString const & name , bool force ); //!< Sets this player's name. Sets processed names (colored, username, nameFromCLient) as well.
    ePlayerNetID & ForceName( tString const & name ); //!< Forces this player's name. Forces processed names (colored, username, nameFromCLient) as well.    

    inline ePlayerNetID & SetUserName( tString const & userName );  //!< Sets this player's name, cleared for system logs. Use for writing to files or comparing with admin input. The other names stay unaffected.

private:
    inline ePlayerNetID & SetNameFromClient( tColoredString const & nameFromClient );   //!< Sets this player's name as the client wants it to be. Avoid using it when possilbe.
    inline ePlayerNetID & SetColoredName( tColoredString const & coloredName ); //!< Sets this player's name, cleared by the server. Use this for onscreen screen display.

    //! accesses the suspension count
    // int & AccessSuspended();

    //! returns the suspension count
    int GetSuspended() const;
};

extern tList<ePlayerNetID> se_PlayerNetIDs;
extern int    sr_viewportBelongsToPlayer[MAX_VIEWPORTS];
extern bool se_assignTeamAutomatically;

void se_ChatState( ePlayerNetID::ChatFlags flag, bool cs);

void se_SaveToScoreFile( tOutput const & out );  //!< writes something to scorelog.txt

tColoredString & operator << (tColoredString &s,const ePlayer &p);
tColoredString & operator << (tColoredString &s,const ePlayerNetID &p);

extern int pingCharity;

void se_AutoShowScores(); // show scores based on automated decision
void se_UserShowScores(bool show); // show scores based on user input
void se_SetShowScoresAuto(bool a); // disable/enable auto show scores

//Password stuff
void se_DeletePasswords();
extern int se_PasswordStorageMode; // 0: store in memory, -1: don't store, 1: store on file

tOutput& operator << (tOutput& o, const ePlayerNetID& p);

// greeting callback
class eCallbackGreeting: public tCallbackString
{
    static tCallbackString *anchor;
    static ePlayerNetID* greeted;

public:
    static tString Greet(ePlayerNetID* player);
    static ePlayerNetID* Greeted(){return greeted;}

    eCallbackGreeting(STRINGRETFUNC* f);
};

void ForceName ( std::istream & s );

void se_MakeReferee( ePlayerNetID * victim, ePlayerNetID * admin = 0 );
void se_CancelReferee( ePlayerNetID * victim, ePlayerNetID * admin = 0 );

// ******************************************************************************************
// *
// *	GetNameFromClient
// *
// ******************************************************************************************
//!
//!		@return		this player's name as the client wants it to be. Avoid using it when possilbe.
//!
// ******************************************************************************************

tColoredString const & ePlayerNetID::GetNameFromClient( void ) const
{
    return this->nameFromClient_;
}

// ******************************************************************************************
// *
// *	GetNameFromClient
// *
// ******************************************************************************************
//!
//!		@param	nameFromClient	this player's name as the client wants it to be. Avoid using it when possilbe. to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID const & ePlayerNetID::GetNameFromClient( tColoredString & nameFromClient ) const
{
    nameFromClient = this->nameFromClient_;
    return *this;
}

// ******************************************************************************************
// *
// *	SetNameFromClient
// *
// ******************************************************************************************
//!
//!		@param	nameFromClient	this player's name as the client wants it to be. Avoid using it when possilbe. to set
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID & ePlayerNetID::SetNameFromClient( tColoredString const & nameFromClient )
{
    this->nameFromClient_ = nameFromClient;
    return *this;
}

// ******************************************************************************************
// *
// *	GetColoredName
// *
// ******************************************************************************************
//!
//!		@return		this player's name, cleared by the server. Use this for onscreen screen display.
//!
// ******************************************************************************************

tColoredString const & ePlayerNetID::GetColoredName( void ) const
{
    return this->coloredName_;
}

// ******************************************************************************************
// *
// *	GetColoredName
// *
// ******************************************************************************************
//!
//!		@param	coloredName	this player's name, cleared by the server. Use this for onscreen screen display. to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID const & ePlayerNetID::GetColoredName( tColoredString & coloredName ) const
{
    coloredName = this->coloredName_;
    return *this;
}

// ******************************************************************************************
// *
// *	SetColoredName
// *
// ******************************************************************************************
//!
//!		@param	coloredName	this player's name, cleared by the server. Use this for onscreen screen display. to set
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID & ePlayerNetID::SetColoredName( tColoredString const & coloredName )
{
    this->coloredName_ = coloredName;
    return *this;
}

// ******************************************************************************************
// *
// *	GetName
// *
// ******************************************************************************************
//!
//!		@return		this player's name without colors.
//!
// ******************************************************************************************

tString const & ePlayerNetID::GetName( void ) const
{
    return this->name_;
}

// ******************************************************************************************
// *
// *	GetName
// *
// ******************************************************************************************
//!
//!		@param	name	this player's name without colors. to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID const & ePlayerNetID::GetName( tString & name ) const
{
    name = this->name_;
    return *this;
}

// ******************************************************************************************
// *
// *	GetUserName
// *
// ******************************************************************************************
//!
//!		@return		this player's name, cleared for system logs. Use for writing to files or comparing with admin input.
//!
// ******************************************************************************************

tString const & ePlayerNetID::GetUserName( void ) const
{
    return this->userName_;
}

// ******************************************************************************************
// *
// *	GetUserName
// *
// ******************************************************************************************
//!
//!		@param	userName	this player's name, cleared for system logs. Use for writing to files or comparing with admin input. to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID const & ePlayerNetID::GetUserName( tString & userName ) const
{
    userName = this->userName_;
    return *this;
}

// ******************************************************************************************
// *
// *	SetUserName
// *
// ******************************************************************************************
//!
//!		@param	userName	this player's name, cleared for system logs. Use for writing to files or comparing with admin input. to set
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID & ePlayerNetID::SetUserName( tString const & userName )
{
    this->userName_ = userName;
    return *this;
}

#endif

