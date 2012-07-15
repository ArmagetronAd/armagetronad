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

#ifndef ArmageTron_GAME_H
#define ArmageTron_GAME_H

#include "rSDL.h"

#include "nNetObject.h"

class eGameObject;
class eSoundMixer;
class eGrid;
class nServerInfo;
class nServerInfoBase;
class eTeam;
class eLadderLogWriter;
class gParser;
class gArena;
class ePlayerNetID;
class tCoord;

namespace Game { class GameSync; }

typedef enum{gFREESTYLE,gDUEL,gHUMAN_VS_AI}
gGameType;

//extern gGameType sg_gameType;      // the current game type
extern bool      sg_TalkToMaster;  // should this server be known on the internet?

typedef enum{gFINISH_EXPRESS,gFINISH_IMMEDIATELY,gFINISH_SPEEDUP,gFINISH_NORMAL}
gFinishType;

struct DrawReason{};

#ifdef ENABLE_ZONESV2
class zZone;
#else
class gZone;
#endif

//extern gFinishType sg_finishType;

extern eLadderLogWriter sg_respawnWriter;

class gGame:public nNetObject{
    unsigned short state;      // the gamestate we are currently in
    unsigned short stateNext; // if a state change has been requested

#ifdef ENABLE_ZONESV2
    tJUST_CONTROLLED_PTR< zZone > winDeathZone_; // the win zone
#else
#ifdef ENABLE_ZONESV1
    tJUST_CONTROLLED_PTR< gZone > winDeathZone_; // the win zone
#endif
#endif

    bool goon;

    int rounds; // the number of rounds played

    double startTime; // time of the match start

    int warning; // timeout warnings

    tCONTROLLED_PTR(eGrid) grid;  // the grid the game takes place

    bool synced_; //!< flag indicating whether the game is considered synced

    void Init();

    gParser *aParser;

public:
    gGame();
    virtual ~gGame();

    //! creates a netobject form sync data
    gGame( Game::GameSync const &, nSenderInfo const & );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Game::GameSync const &, nSenderInfo const & );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Game::GameSync &, bool init ) const;

    static void NetSync(); // do all the network syncronisation.
    static void NetSyncIdle(); // do all the network syncronisation and wait a bit.

    virtual void Verify(); // verifies settings are OK, throws an exception if not.

    virtual void StateUpdate(); // switch to new gamestate (does all

    // the real work around here).
    virtual void  SetState(unsigned short act,unsigned short next);
    virtual short GetState(){return state;}

    // make sure the clients are catching up
    void SyncState(unsigned short state);

    virtual void Timestep(REAL time,bool cam=false); // do all the world simulation
    virtual void Analysis(REAL time); // do we have a winner?

    virtual bool GameLoop(bool input=true); // return values: exit the game loop?

    bool GridIsReady(int c); // can we transfer gameObjects that need the grid to exist?
    eGrid * Grid() const { return grid; }

    void NoLongerGoOn();

    void StartNewMatch();

    void StartNewMatchNow();

    eSoundMixer* m_Mixer;

private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
};

void update_settings( bool const * goon = 0 );

void ConnectToServer(nServerInfoBase *server);

void sg_EnterGame( nNetState enter_state );
void sg_HostGame();
void sg_HostGameMenu();

// runs a single player game
void sg_SinglePlayerGame();

void MainMenu(bool ingame=false);

bool GridIsReady(int c);

void Activate(bool act);

void sg_DeclareWinner( eTeam* team, char const * message );

void sg_FullscreenMessage(tOutput const & title, tOutput const & message,REAL timeout = 60, int client = 0); //!< Displays a message on a specific client or all clients that gets displayed on the whole screen, blocking view to the game
void sg_ClientFullscreenMessage( tOutput const & title, tOutput const & message, REAL timeout = 60 ); //!< Displays a message locally that gets displayed on the whole screen, blocking view to the game

void sg_RespawnAllAfter( REAL after, REAL time, eGrid *grid, gArena & arena,
    bool atSpawn=false );
void sg_RespawnAllAfter( REAL after, bool atSpawn=false );

void sg_RespawnPlayer(eGrid *, gArena *, tCoord & pos, tCoord & dir, ePlayerNetID *);
void sg_RespawnPlayer(eGrid *, gArena *, tCoord & near, ePlayerNetID *);
void sg_RespawnPlayer(eGrid *, gArena *, tCoord * near, ePlayerNetID *);
void sg_RespawnPlayer(eGameObject & near, ePlayerNetID *);
void sg_RespawnPlayer(eGrid *, gArena *, eGameObject * near, ePlayerNetID *);
void sg_RespawnPlayer(eGrid *grid, gArena *arena, ePlayerNetID *p,
    bool atSpawn=false);

// HACK
gArena * sg_GetArena();
// END OF HACK

void init_second_pass_zones(eGrid *grid, gParser *parser);

class gGameSettings
{
public:
    int scoreWin;    // score you get when you win a round

    int limitTime;   // match time limit
    int limitRounds; // match round limit
    int limitScore;  // match score limit
    int limitScoreMinLead;  // minimum lead for limitScore to take effect
    int maxBlowout;  // max score advance

    int numAIs;      // number of AI players
    int minPlayers;  // minimum number of players
    int AI_IQ;       // preferred IQ of AI players

    bool autoNum;    // automatically adjust number of AIs
    bool autoIQ;     // automatically adjust IQ of AIs

    int  speedFactor; // logarithm of cycle speed multiplier
    int  sizeFactor;  // logarithm of arena size multiplier

    int  autoAIFraction; // helper variable for the autoAI functions

    REAL winZoneMinRoundTime;	// minimum number of seconds a round must be going before the win zone is activated
    REAL winZoneMinLastDeath;	// minimum number of seconds the last death happended before the win zone is activated

    gGameType   gameType;      // what type of game is played?
    gFinishType finishType;    // what happens when all humans are dead?

    // team settings
    int			minTeams, maxTeams;
    int			minPlayersPerTeam, maxPlayersPerTeam;
    int			maxTeamImbalance;
    bool		balanceTeamsWithAIs, enforceTeamRulesOnQuit;

    // game mechanics settings
    REAL		wallsStayUpDelay;	// the time the cycle walls stay up ( negative values: they stay up forever )
    REAL		wallsLength;		// the maximum total length of the walls
    REAL		explosionRadius;	// the radius of the holes blewn in by an explosion

    gGameSettings(int a_scoreWin,
                  int a_limitTime, int a_limitRounds, int a_limitScore,
                  int a_limitScoreMinLead, int a_maxBlowout,
                  int a_numAIs,    int a_minPlayers,  int a_AI_IQ,
                  bool a_autoNum, bool a_autoIQ,
                  int a_speedFactor, int a_sizeFactor,
                  gGameType a_gameType,  gFinishType a_finishType,
                  int a_minTeams,
                  REAL a_winZoneMinRoundTime, REAL a_winZoneMinLastDeath
                 );

    void AutoAI(bool success);
    void Menu();
};

extern gGameSettings* sg_currentSettings;

void rotate();

#endif

