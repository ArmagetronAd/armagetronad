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

#ifndef ArmageTron_PLAYER_H
#define ArmageTron_PLAYER_H

#ifndef MAX_INSTANT_CHAT 
#define MAX_INSTANT_CHAT 25
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

#include <set>
#include <list>
#include <utility>
#include "eChat.h"


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

class ePlayer: public uPlayerPrototype{
    friend class eMenuItemChat;
    static uActionPlayer s_chat;

    tConfItemBase *configuration[PLAYER_CONFITEMS];
    int            CurrentConfitem;
    void   StoreConfitem(tConfItemBase *c);
    void   DeleteConfitems();
public:
    tString    name;                 // the player's screen name
    tString    globalID;             // the global ID of the player in user@authority form
    // REAL	   rubberstatus;
    bool       centerIncamOnTurn;
    bool       wobbleIncam;
    bool       autoSwitchIncam;

    bool       spectate;              // shall this player always spectate?
    bool       stealth;               // does this player wish to hide his/her identity?
    bool       autoLogin;             // should the player always request authentication on servers?

    bool 		nameTeamAfterMe; // player prefers to call his team after his name
    int			favoriteNumberOfPlayersPerTeam;

    eCamMode startCamera;
    bool     allowCam[10];
    int      startFOV;
    bool     smartCustomGlance; //!< flag making the smart camera use the custom settings for glancing

    tCHECKED_PTR(eCamera)           cam;
    tCONTROLLED_PTR(ePlayerNetID) netPlayer;

    int rgb[3]; // our color

    tString instantChatString[MAX_INSTANT_CHAT];
    // instant chat macros

    static uActionPlayer *se_instantChatAction[MAX_INSTANT_CHAT];

    ePlayer();
    virtual ~ePlayer();

    virtual const char *Name() const{return name;}

    virtual bool Act(uAction *act,REAL x);

    int ID() const {return id;};
#ifndef DEDICATED
    void Render();
#endif

    static ePlayer * PlayerConfig(int p);

    static bool PlayerIsInGame(int p);

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

// the class that identifies players across the network
class ePlayerNetID: public nNetObject, public eAccessLevelHolder{
    friend class ePlayer;
    friend class eTeam;
    friend class eNetGameObject;
    friend class tControlledPTR< ePlayerNetID >;
    // access level. lower numeric values are better.
public:
    typedef std::set< eTeam * > eTeamSet;
private:

    int listID;                          // ID in the list of all players
    int teamListID;                      // ID in the list of the team

    bool							silenced_;		// flag indicating whether the player has been silenced
    int                             suspended_;     //! number of rounds the player is currently suspended from playing

    nTimeAbsolute					timeJoinedTeam; // the time the player joined the team he is in now
    tCONTROLLED_PTR(eTeam)			nextTeam;		// the team we're in ( logically )
    tCONTROLLED_PTR(eTeam)			currentTeam;	// the team we currently are spawned for
    eTeamSet                        invitations_;   // teams this player is invited to
    tCONTROLLED_PTR(eVoter)			voter_;			// voter assigned to this player

    tCHECKED_PTR(eNetGameObject) object; // the object this player is
    // controlling

    int score; // points made so far
    int lastScore_; // last saved score

    int favoriteNumberOfPlayersPerTeam;		// join team if number of players on it is less than this; create new team otherwise
    bool nameTeamAfterMe; 					// player prefers to call his team after his name
    bool greeted;        					// did the server already greet him?
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
    // REAL	rubberstatus;
        
    unsigned short r,g,b; // our color

    unsigned short pingCharity; // max ping you are willing to take over

    REAL ping;

    double lastSync;         //!< time of the last sync request
    double lastActivity_;    //!< time of the last activity

    bool loginWanted;        //!< flag indicating whether this player currently wants to log on

    bool renameAllowed_;     //!< specifies if the player is allowed to rename or not, does not know about votes.

    nSpamProtection chatSpam_;
    
    eChatLastSaid lastSaid_; //!< last said information

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

    bool StealthMode() const { return stealth_; }

    // team management
    bool TeamChangeAllowed( bool informPlayer = false ) const; //!< is this player allowed to change teams?
    void SetTeamChangeAllowed(bool allowed) {allowTeamChange_ = allowed;} //!< set if this player should always be allowed to change teams
    eTeam* NextTeam()    const { return nextTeam; }				// return the team I will be next round
    eTeam* CurrentTeam() const { return currentTeam; }		// return the team I am in
    int  TeamListID() const { return teamListID; }		// return my position in the team
    void FindDefaultTeam();									// look for a good default team for us
    void SetTeamForce(eTeam* team );                 	// register me in the given team without checks
    void SetTeam(eTeam* team);          	// register me in the given team (callable on the server)
    void SetTeamWish(eTeam* team); 				// express the wish to be part of the given team (always callable)
    void UpdateTeamForce();							// update team membership without checks
    void UpdateTeam();							// update team membership

    eTeamSet const & GetInvitations() const ;   //!< teams this player is invited to

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

    // suspend the player from playing, forcing him to spectate
    void Suspend( int rounds = 5 );
#ifdef KRAWALL_SERVER
    void Authenticate( tString const & authName, 
                       tAccessLevel accessLevel = tAccessLevel_Authenticated,
                       ePlayerNetID const * admin = 0 );    //!< make the authentification valid
    void DeAuthenticate( ePlayerNetID const * admin = 0 );  //!< make the authentification invalid
    bool IsAuthenticated() const;                     //!< is the authentification valid?
#endif

    static void RequestScheduledLogins();  //!< initiates login processes for all pending wishes

    bool IsActive() const { return !disconnected; }

    bool IsSilenced( void ) const { return silenced_; }
    void SetSilenced( bool silenced ) { silenced_ = silenced; }
    bool& AccessSilenced( void ) { return silenced_; }

    bool IsSuspended ( void ) { return suspended_ > 0; }

    eVoter * GetVoter() const {return voter_;}     // returns our voter
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
    static void UpdateSuspensions();     //<! Decrements the number of rounds players are suspended for
    void LogScoreDifference();           //<! Logs accumulated scores since the last call to ResetScoreDifferences() to ladderlog.txt

    static void SortByScore(); // brings the players into the right order
    static tString Ranking( int MAX=12, bool cut = true );     // returns a ranking list
    static void RankingLadderLog();     // writes a small ranking list to ladderlog
    static void  ResetScore();  // resets the ranking list

    static void DisplayScores(); // display scores on the screen

    void GreetHighscores(tString &s); // tell him his positions in the
    // highscore lists (defined in game.cpp)

    static void Update();           // creates ePlayerNetIDs for new players
    // and destroys those of players that have left

#ifdef KRAWALL_SERVER
    static tAccessLevel AccessLevelRequiredToPlay(); // is authentication required to play on this server?
#endif

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
    bool IsLoggedIn() const { return GetAccessLevel() < tAccessLevel_Moderator; }
    void BeLoggedIn() { SetAccessLevel( tAccessLevel_Admin ); }
    void BeNotLoggedIn() { SetAccessLevel( tAccessLevel_Program ); }
    tAccessLevel GetLastAccessLevel() const { return lastAccessLevel; }

    static ePlayerNetID * FindPlayerByName( tString const & name, ePlayerNetID * requester = 0 ); //!< finds a player by name using lax name matching. Reports errors to the console or to the requesting player.

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
    int & AccessSuspended();

    //! returns the suspension count
    int GetSuspended() const;
};

extern tList<ePlayerNetID> se_PlayerNetIDs;
extern int    sr_viewportBelongsToPlayer[MAX_VIEWPORTS];

void se_ChatState( ePlayerNetID::ChatFlags flag, bool cs);

void se_SaveToScoreFile( tOutput const & out );  //!< writes something to scorelog.txt
void se_SaveToChatLog( tOutput const & out );  //!< writes something to chatlog.txt (if enabled) and/or ladderlog

//! create a global instance of this to write stuff to ladderlog.txt
class eLadderLogWriter {
    static std::list<eLadderLogWriter *> &writers();
    tString id;
    bool enabled;
    tSettingItem<bool> *conf;
    tColoredString cache;
public:
    eLadderLogWriter(char const *ID, bool enabledByDefault);
    ~eLadderLogWriter();
    //! append a field to the current message. Spaces are added automatically.
    template<typename T> eLadderLogWriter &operator<<(T const &s) {
        if(enabled) {
            cache << ' ' << s;
        }
        return *this;
    }
    void write(); //!< send to ladderlog and clear message

    bool isEnabled() { return enabled; } //!< check this if you're going to make expensive calculations for ladderlog output

    static void setAll(bool enabled); //!< enable or disable all writers
};

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

