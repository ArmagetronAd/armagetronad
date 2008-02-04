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

#include "eEventNotification.h"
#include "eEventNotification.h"
#include "gStuff.h"
#include "eSoundMixer.h"
#include "eGrid.h"
#include "eTeam.h"
#include "tSysTime.h"
#include "gGame.h"
#include "rTexture.h"
#include "gWall.h"
#include "rConsole.h"
#include "gCycle.h"
#include "eCoord.h"
#include "eTimer.h"
#include "gAIBase.h"
#include "rSysdep.h"
#include "rFont.h"
#include "uMenu.h"
#include "nConfig.h"
#include "rScreen.h"
#include "rViewport.h"
#include "uInput.h"
#include "ePlayer.h"
#include "gSpawn.h"
#include "uInput.h"
#include "uInputQueue.h"
#include "nNetObject.h"
#include "tToDo.h"
#include "gMenus.h"
#include "gCamera.h"
#include "gServerBrowser.h"
#include "gServerFavorites.h"
#include "gFriends.h"
#include "gLogo.h"
#include "gLanguageMenu.h"
#include "nServerInfo.h"
#include "gAICharacter.h"
#include "tDirectories.h"
#include "gTeam.h"
#include "zone/zZone.h"
#include "eVoter.h"
#include "tRecorder.h"
#include "gStatistics.h"

#include "tXmlParser.h"
#include "gParser.h"

//#include "uWebinterface.h"

#include "gRotation.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <ctype.h>
#include <time.h>

#include "nSocket.h"

#include "gArena.h"
gArena Arena;

#ifdef KRAWALL_SERVER
#include "nKrawall.h"
#endif


#ifndef DEDICATED
#include "rSDL.h"
#include <SDL_thread.h>

#ifdef DEBUG
#ifndef WIN32
//#include <GL/xmesa>

//static bool fullscreen=1;
#endif
#endif
#endif

#ifdef DEBUG
//#define CONNECTION_STRESS
#endif

tCONFIG_ENUM( gGameType );
tCONFIG_ENUM( gFinishType );

// extra round pause time
static REAL sg_extraRoundTime = 0.0f;
static tSettingItem<REAL> sg_extraRoundTimeConf( "EXTRA_ROUND_TIME", sg_extraRoundTime );

static REAL sg_lastChatBreakTime = -1.0f;
static tSettingItem<REAL> sg_lastChatBreakTimeConf( "LAST_CHAT_BREAK_TIME", sg_lastChatBreakTime );

#define DEFAULT_MAP "Anonymous/polygon/regular/square-1.0.1.aamap.xml"
static tString mapfile(DEFAULT_MAP);

// The following are only relevant in the case of zones from maps using version 1
static REAL sg_conquestRate = .5;
static REAL sg_defendRate = .25;

static tSettingItem< REAL > sg_conquestRateConf( "FORTRESS_CONQUEST_RATE", sg_conquestRate );
static tSettingItem< REAL > sg_defendRateConf( "FORTRESS_DEFEND_RATE", sg_defendRate );

/*
static void sg_ParseMap ( gParser * aParser, tString map_file );
static void change_mapfile(std::istream &s)
{
    // read new MAP_FILE value
    tString new_mapfile;
    new_mapfile.ReadLine(s, true);

    if (new_mapfile == mapfile)
        return;

    // verify the map loads
    try {
        sg_ParseMap(aParser, new_mapfile);
    } catch (tGenericException &e) {
        sn_ConsoleOut( e.GetName(), e.GetDescription(), 120000 );
        sg_ParseMap(aParser);	// is this necessary?
        return;
    }

    if (printChange)
    {
        tOutput o;
        o.SetTemplateParameter(1, "MAP_FILE");
        o.SetTemplateParameter(2, mapfile);
        o.SetTemplateParameter(3, new_mapfile);
        o << "$config_value_changed";
        con << o;
    }

    mapfile = new_mapfile;
}
*/
static nSettingItemWatched<tString> conf_mapfile("MAP_FILE",mapfile, nConfItemVersionWatcher::Group_Breaking, 8 );

// config item for semi-colon deliminated list of maps/configs, needs semi-colon at the end
// ie, original/map-1.0.1.xml;original/map-1.0.1.xml;
class tSettingRotation: public tConfItemBase
{
public:
    tSettingRotation( char const * name )
    : tConfItemBase( name ),
      current_(0)
    {
    }

    // the number of items
    int Size() const
    {
        return items_.Len();
    }

    // returns the current value
    tString const & Current() const
    {
        tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

        return items_[current_];
    }

    // rotates
    void Rotate()
    {
        if ( ++current_ >= items_.Len() )
        {
            current_ = 0;
        }
    }
private:
    virtual void ReadVal( std::istream &is )
    {
        tString mapsT;
        mapsT.ReadLine (is);
        items_.SetLen (0);
        
        int strpos = 0;
        int nextsemicolon = mapsT.StrPos(";");
        
        if (nextsemicolon != -1)
        {
            do
            {
                tString const &map = mapsT.SubStr(strpos, nextsemicolon - strpos);
                
                strpos = nextsemicolon + 1;
                nextsemicolon = mapsT.StrPos(strpos, ";");
                
                items_.Insert(map);
            }
            while ((nextsemicolon = mapsT.StrPos(strpos, ";")) != -1);
        }

        // make sure the current value is correct
        if ( current_ >= items_.Len() )
        {
            current_ = 0;
        }
    }

    virtual void WriteVal(std::ostream &s){}
    virtual bool Writable(){return false;}
    virtual bool Save(){return false;}

    tArray<tString> items_; // the various values the rotating config can take
    int current_;           // the index of the current 
};

static tSettingRotation sg_mapRotation("MAP_ROTATION");
static tSettingRotation sg_configRotation("CONFIG_ROTATION");

//0 = never, 1 = round, 2 = match
static int rotationtype = 0;
static tSettingItem<int> conf_rotationtype("ROTATION_TYPE",rotationtype);

// bool globalingame=false;
tString sg_GetCurrentTime( char const * szFormat )
{
    char szTemp[128];
    time_t     now;
    struct tm *pTime;
    now = time(NULL);
    pTime = localtime(&now);
    strftime(szTemp,sizeof(szTemp),szFormat,pTime);
    return tString(szTemp);
}

void sg_PrintCurrentTime( char const * szFormat )
{
    con << sg_GetCurrentTime(szFormat);
}

void sg_PrintCurrentDate()
{
    sg_PrintCurrentTime( "%Y%m%d");
}

void sg_PrintCurrentTime()
{
    sg_PrintCurrentTime( "%H%M%S" );
}

void sg_Timestamp()
{
#ifdef DEDICATED
    sg_PrintCurrentTime( "Timestamp: %Y/%m/%d %H:%M:%S\n" );
    if ( tRecorder::IsRunning() )
    {
        con << "Uptime: " << int(tSysTimeFloat()) << " seconds.\n";

#ifdef DEBUG_X
        // to set breakpoints at specific round starts
        static double breakTime = 0;
        if ( tSysTimeFloat() > breakTime )
            st_Breakpoint();
#endif
    }
#endif
}

static REAL ded_idle=24;
static tSettingItem<REAL> dedicaded_idle("DEDICATED_IDLE",ded_idle);

static float sg_gameTimeInterval=-1;
static tSettingItem<float> sggti("LADDERLOG_GAME_TIME_INTERVAL",
                                 sg_gameTimeInterval);

#define MAXAI (gAICharacter::s_Characters.Len())

#define AUTO_AI_MAXFRAC 6
#define AUTO_AI_WIN     3
#define AUTO_AI_LOSE    1

gGameSettings::gGameSettings(int a_scoreWin,
                             int a_limitTime, int a_limitRounds, int a_limitScore,
                             int a_numAIs,    int a_minPlayers,  int a_AI_IQ,
                             bool a_autoNum, bool a_autoIQ,
                             int a_speedFactor, int a_sizeFactor,
                             gGameType a_gameType,  gFinishType a_finishType,
                             int a_minTeams,
                             int a_winZoneMinRoundTime, int a_winZoneMinLastDeath
                            )
        :scoreWin(a_scoreWin),
        limitTime(a_limitTime), limitRounds(a_limitRounds), limitScore(a_limitScore),
        numAIs(a_numAIs),       minPlayers(a_minPlayers),   AI_IQ(a_AI_IQ),
        autoNum(a_autoNum),     autoIQ(a_autoIQ),
        speedFactor(a_speedFactor), sizeFactor(a_sizeFactor),
        winZoneMinRoundTime( a_winZoneMinRoundTime ),winZoneMinLastDeath( a_winZoneMinLastDeath ),
        gameType(a_gameType),   finishType(a_finishType),
        minTeams(a_minTeams)
{
    autoAIFraction = AUTO_AI_MAXFRAC >> 1;

    maxTeams = 16;
    minPlayersPerTeam = 1;
    maxPlayersPerTeam = 10;
    maxTeamImbalance = 1;
    balanceTeamsWithAIs = true;
    enforceTeamRulesOnQuit = false;

    wallsStayUpDelay = 2.0f;
    wallsLength      = -1.0f;
    explosionRadius  = 4.0f;
}

void gGameSettings::AutoAI(bool success){
    if (!autoNum && !autoIQ)
        return;

    if (autoNum)
    {
        if (success)
        {
            autoAIFraction += AUTO_AI_WIN;
            while (autoAIFraction > AUTO_AI_MAXFRAC)
            {
                autoAIFraction -= AUTO_AI_MAXFRAC;
                if (numAIs < MAXAI) numAIs++;
            }
        }
        else
        {
            autoAIFraction -= AUTO_AI_LOSE;
            while (autoAIFraction < 0)
            {
                autoAIFraction += AUTO_AI_MAXFRAC;
                if (numAIs >= 2) numAIs--;
            }
        }
    }


    if (autoIQ)
    {
        if (!autoNum)
            AI_IQ += 4 * (success ? AUTO_AI_WIN : -AUTO_AI_LOSE);
        else
        {
            int total = numAIs * numAIs * AI_IQ;
            // try to keep total around 100: that is either 10 dumb AIs or
            // one smart AI.

            if (success)
                if (total > 100)
                    AI_IQ += AUTO_AI_WIN * 4;
                else
                    AI_IQ += AUTO_AI_WIN;
            else
                if (total < 100)
                    AI_IQ -= AUTO_AI_LOSE * 4;
                else
                    AI_IQ -= AUTO_AI_LOSE;
        }
    }

    if (AI_IQ > 100)
        AI_IQ = 100;
    if (AI_IQ < 0)
        AI_IQ = 0;
}


void gGameSettings::Menu()
{
    uMenu GameSettings("$game_settings_menu_text");

    uMenuItemInt wzmr
    (&GameSettings,
     "$game_menu_wz_mr_text",
     "$game_menu_wz_mr_help",
     winZoneMinRoundTime,0,1000,10);

    uMenuItemInt wzmld
    (&GameSettings,
     "$game_menu_wz_ld_text",
     "$game_menu_wz_ld_help",
     winZoneMinLastDeath,0,1000,10);

    uMenuItemToggle team_et
    (&GameSettings,
     "$game_menu_balance_quit_text",
     "$game_menu_balance_quit_help",
     enforceTeamRulesOnQuit);

    uMenuItemToggle team_bt
    (&GameSettings,
     "$game_menu_balance_ais_text",
     "$game_menu_balance_ais_help",
     balanceTeamsWithAIs);

    uMenuItemInt team_mi
    (&GameSettings,
     "$game_menu_imb_text",
     "$game_menu_imb_help",
     maxTeamImbalance,2,10);

    uMenuItemInt team_maxp
    (&GameSettings,
     "$game_menu_max_players_text",
     "$game_menu_max_players_help",
     maxPlayersPerTeam,1,16);

    uMenuItemInt team_minp
    (&GameSettings,
     "$game_menu_min_players_text",
     "$game_menu_min_players_help",
     minPlayersPerTeam,1,16);

    uMenuItemInt team_max
    (&GameSettings,
     "$game_menu_max_teams_text",
     "$game_menu_max_teams_help",
     maxTeams,1,16);

    uMenuItemInt team_min
    (&GameSettings,
     "$game_menu_min_teams_text",
     "$game_menu_min_teams_help",
     minTeams,1,16);

    uMenuItemSelection<gFinishType> finisht
    (&GameSettings,"$game_menu_finish_text",
     "$game_menu_finish_help",finishType);
    finisht.NewChoice("$game_menu_finish_expr_text",
                      "$game_menu_finish_expr_help",
                      gFINISH_EXPRESS);
    finisht.NewChoice("$game_menu_finish_stop_text",
                      "$game_menu_finish_stop_help",
                      gFINISH_IMMEDIATELY);
    finisht.NewChoice("$game_menu_finish_fast_text",
                      "$game_menu_finish_fast_help",
                      gFINISH_SPEEDUP);
    finisht.NewChoice("$game_menu_finish_normal_text",
                      "$game_menu_finish_normal_help",
                      gFINISH_NORMAL);

    uMenuItemSelection<gGameType> gamet
    (&GameSettings,"$game_menu_mode_text",
     "$game_menu_mode_help",gameType);
    gamet.NewChoice("$game_menu_mode_free_text",
                    "$game_menu_mode_free_help",
                    gFREESTYLE);
    gamet.NewChoice("$game_menu_mode_lms_text",
                    "$game_menu_mode_lms_help",
                    gDUEL);
    /*
    	gamet.NewChoice("$game_menu_mode_team_text",
    					"$game_menu_mode_team_help",
    					gHUMAN_VS_AI);
    */

    uMenuItemInt speedconf
    (&GameSettings,
     "$game_menu_speed_text",
     "$game_menu_speed_help",
     speedFactor,-10,10);

    uMenuItemInt sizeconf
    (&GameSettings,
     "$game_menu_size_text",
     "$game_menu_size_help",
     sizeFactor,-10,10);

    uMenuItemSelection<REAL> wsuconf
    (&GameSettings,
     "$game_menu_wallstayup_text",
     "$game_menu_wallstayup_help",
     wallsStayUpDelay);
    wsuconf.NewChoice( "$game_menu_wallstayup_infinite_text",
                       "$game_menu_wallstayup_infinite_help",
                       -1.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_immediate_text",
                       "$game_menu_wallstayup_immediate_help",
                       0.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_halfsecond_text",
                       "$game_menu_wallstayup_halfsecond_help",
                       0.5f );
    wsuconf.NewChoice( "$game_menu_wallstayup_second_text",
                       "$game_menu_wallstayup_second_help",
                       1.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_2second_text",
                       "$game_menu_wallstayup_2second_help",
                       2.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_4second_text",
                       "$game_menu_wallstayup_4second_help",
                       4.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_8second_text",
                       "$game_menu_wallstayup_8second_help",
                       8.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_16second_text",
                       "$game_menu_wallstayup_16second_help",
                       16.0f );
    wsuconf.NewChoice( "$game_menu_wallstayup_32second_text",
                       "$game_menu_wallstayup_32second_help",
                       32.0f );

    uMenuItemSelection<REAL> wlconf
    (&GameSettings,
     "$game_menu_wallslength_text",
     "$game_menu_wallslength_help",
     wallsLength);
    wlconf.NewChoice( "$game_menu_wallslength_infinite_text",
                      "$game_menu_wallslength_infinite_help",
                      -1.0f );
    wlconf.NewChoice( "$game_menu_wallslength_25meter_text",
                      "$game_menu_wallslength_25meter_help",
                      25.0f );
    wlconf.NewChoice( "$game_menu_wallslength_50meter_text",
                      "$game_menu_wallslength_50meter_help",
                      50.0f );
    wlconf.NewChoice( "$game_menu_wallslength_100meter_text",
                      "$game_menu_wallslength_100meter_help",
                      100.0f );
    wlconf.NewChoice( "$game_menu_wallslength_200meter_text",
                      "$game_menu_wallslength_200meter_help",
                      200.0f );
    wlconf.NewChoice( "$game_menu_wallslength_300meter_text",
                      "$game_menu_wallslength_300meter_help",
                      300.0f );
    wlconf.NewChoice( "$game_menu_wallslength_400meter_text",
                      "$game_menu_wallslength_400meter_help",
                      400.0f );
    wlconf.NewChoice( "$game_menu_wallslength_600meter_text",
                      "$game_menu_wallslength_600meter_help",
                      600.0f );
    wlconf.NewChoice( "$game_menu_wallslength_800meter_text",
                      "$game_menu_wallslength_800meter_help",
                      800.0f );
    wlconf.NewChoice( "$game_menu_wallslength_1200meter_text",
                      "$game_menu_wallslength_1200meter_help",
                      1200.0f );
    wlconf.NewChoice( "$game_menu_wallslength_1600meter_text",
                      "$game_menu_wallslength_1600meter_help",
                      1600.0f );

    uMenuItemSelection<REAL> erconf
    (&GameSettings,
     "$game_menu_exrad_text",
     "$game_menu_exrad_help",
     explosionRadius);
    erconf.NewChoice( "$game_menu_exrad_0_text",
                      "$game_menu_exrad_0_help",
                      0.0f );
    erconf.NewChoice( "$game_menu_exrad_2meters_text",
                      "$game_menu_exrad_2meters_help",
                      2.0f );
    erconf.NewChoice( "$game_menu_exrad_4meters_text",
                      "$game_menu_exrad_4meters_help",
                      4.0f );
    erconf.NewChoice( "$game_menu_exrad_8meters_text",
                      "$game_menu_exrad_8meters_help",
                      8.0f );
    erconf.NewChoice( "$game_menu_exrad_16meters_text",
                      "$game_menu_exrad_16meters_help",
                      16.0f );
    erconf.NewChoice( "$game_menu_exrad_32meters_text",
                      "$game_menu_exrad_32meters_help",
                      32.0f );
    erconf.NewChoice( "$game_menu_exrad_64meters_text",
                      "$game_menu_exrad_64meters_help",
                      64.0f );
    erconf.NewChoice( "$game_menu_exrad_128meters_text",
                      "$game_menu_exrad_128meters_help",
                      128.0f );

    uMenuItemToggle autoiqconf
    (&GameSettings,
     "$game_menu_autoiq_text",
     "$game_menu_autoiq_help",
     autoIQ);

    uMenuItemToggle autoaiconf
    (&GameSettings,
     "$game_menu_autoai_text",
     "$game_menu_autoai_help",
     autoNum);


    uMenuItemInt iqconf
    (&GameSettings,
     "$game_menu_iq_text",
     "$game_menu_iq_help",
     AI_IQ, 20, 100, 10);

    uMenuItemInt mpconf
    (&GameSettings,
     "$game_menu_minplayers_text",
     "$game_menu_minplayers_help",
     minPlayers,0,MAXAI);

    uMenuItemInt aiconf
    (&GameSettings,
     "$game_menu_ais_text",
     "$game_menu_ais_help",
     numAIs,0,MAXAI);


    GameSettings.Enter();
}

gGameSettings singlePlayer(10,
                           30, 10, 100000,
                           1,   0, 30,
                           true, true,
                           0  ,  -3,
                           gDUEL, gFINISH_IMMEDIATELY, 1,
                           100000, 1000000);

gGameSettings multiPlayer(10,
                          30, 10, 100,
                          0,   4, 100,
                          false, false,
                          0  ,  -3,
                          gDUEL, gFINISH_IMMEDIATELY, 2,
                          60, 30 );

gGameSettings* sg_currentSettings = &singlePlayer;



static tSettingItem<int> mp_sw("SCORE_WIN"   ,multiPlayer.scoreWin);
static tSettingItem<int> mp_lt("LIMIT_TIME"  ,multiPlayer.limitTime);
static tSettingItem<int> mp_lr("LIMIT_ROUNDS",multiPlayer.limitRounds);
static tSettingItem<int> mp_ls("LIMIT_SCORE" ,multiPlayer.limitScore);

static tConfItem<int>    mp_na("NUM_AIS"     ,multiPlayer.numAIs);
static tConfItem<int>    mp_mp("MIN_PLAYERS" ,multiPlayer.minPlayers);
static tConfItem<int>    mp_iq("AI_IQ"       ,multiPlayer.AI_IQ);

static tConfItem<bool>   mp_an("AUTO_AIS"    ,multiPlayer.autoNum);
static tConfItem<bool>   mp_aq("AUTO_IQ"     ,multiPlayer.autoIQ);

static tConfItem<int>    mp_sf("SPEED_FACTOR",multiPlayer.speedFactor);
static tConfItem<int>    mp_zf("SIZE_FACTOR" ,multiPlayer.sizeFactor);

static tConfItem<gGameType>    mp_gt("GAME_TYPE",multiPlayer.gameType);
static tConfItem<gFinishType>  mp_ft("FINISH_TYPE",multiPlayer.finishType);

static tConfItem<int>    mp_wzmr("WIN_ZONE_MIN_ROUND_TIME",multiPlayer.winZoneMinRoundTime);
static tConfItem<int>    mp_wzld("WIN_ZONE_MIN_LAST_DEATH",multiPlayer.winZoneMinLastDeath);

static tConfItem<int>    mp_tmin	("TEAMS_MIN",					multiPlayer.minTeams);
static tConfItem<int>    mp_tmax	("TEAMS_MAX",					multiPlayer.maxTeams);
static tConfItem<int>    mp_mtp		("TEAM_MIN_PLAYERS",			multiPlayer.minPlayersPerTeam);
static tConfItem<int>    mp_tp		("TEAM_MAX_PLAYERS",			multiPlayer.maxPlayersPerTeam);
static tConfItem<int>    mp_tib		("TEAM_MAX_IMBALANCE",			multiPlayer.maxTeamImbalance);
static tConfItem<bool>   mp_tbai	("TEAM_BALANCE_WITH_AIS",		multiPlayer.balanceTeamsWithAIs);
static tConfItem<bool>   mp_tboq	("TEAM_BALANCE_ON_QUIT",		multiPlayer.enforceTeamRulesOnQuit);

static tConfItem<REAL>   mp_wsu		("WALLS_STAY_UP_DELAY"	,		multiPlayer.wallsStayUpDelay);
static tConfItem<REAL>   mp_wl		("WALLS_LENGTH"		    ,		multiPlayer.wallsLength     );
static tConfItem<REAL>   mp_er		("EXPLOSION_RADIUS"		,		multiPlayer.explosionRadius );

static tSettingItem<int> sp_sw("SP_SCORE_WIN"   ,singlePlayer.scoreWin);
static tSettingItem<int> sp_lt("SP_LIMIT_TIME"  ,singlePlayer.limitTime);
static tSettingItem<int> sp_lr("SP_LIMIT_ROUNDS",singlePlayer.limitRounds);
static tSettingItem<int> sp_ls("SP_LIMIT_SCORE" ,singlePlayer.limitScore);

static tConfItem<int>    sp_na("SP_NUM_AIS"     ,singlePlayer.numAIs);
static tConfItem<int>    sp_mp("SP_MIN_PLAYERS" ,singlePlayer.minPlayers);
static tConfItem<int>    sp_iq("SP_AI_IQ"       ,singlePlayer.AI_IQ);

static tConfItem<bool>   sp_an("SP_AUTO_AIS"    ,singlePlayer.autoNum);
static tConfItem<bool>   sp_aq("SP_AUTO_IQ"     ,singlePlayer.autoIQ);

static tConfItem<int>    sp_sf("SP_SPEED_FACTOR",singlePlayer.speedFactor);
static tConfItem<int>    sp_zf("SP_SIZE_FACTOR" ,singlePlayer.sizeFactor);

static tConfItem<gGameType>    sp_gt("SP_GAME_TYPE",singlePlayer.gameType);
static tConfItem<gFinishType>  sp_ft("SP_FINISH_TYPE",singlePlayer.finishType);

static tConfItem<int>    sp_wzmr("SP_WIN_ZONE_MIN_ROUND_TIME",singlePlayer.winZoneMinRoundTime);
static tConfItem<int>    sp_wzld("SP_WIN_ZONE_MIN_LAST_DEATH",singlePlayer.winZoneMinLastDeath);

static tConfItem<int>    sp_tmin	("SP_TEAMS_MIN",					singlePlayer.minTeams);
static tConfItem<int>    sp_tmax	("SP_TEAMS_MAX",					singlePlayer.maxTeams);
static tConfItem<int>    sp_mtp		("SP_TEAM_MIN_PLAYERS",				singlePlayer.minPlayersPerTeam);
static tConfItem<int>    sp_tp		("SP_TEAM_MAX_PLAYERS",				singlePlayer.maxPlayersPerTeam);
static tConfItem<int>    sp_tib		("SP_TEAM_MAX_IMBALANCE",			singlePlayer.maxTeamImbalance);
static tConfItem<bool>   sp_tbai	("SP_TEAM_BALANCE_WITH_AIS",		singlePlayer.balanceTeamsWithAIs);
static tConfItem<bool>   sp_tboq	("SP_TEAM_BALANCE_ON_QUIT",		singlePlayer.enforceTeamRulesOnQuit);

static tConfItem<REAL>   sp_wsu		("SP_WALLS_STAY_UP_DELAY"	,		singlePlayer.wallsStayUpDelay);
static tConfItem<REAL>   sp_wl		("SP_WALLS_LENGTH"		    ,		singlePlayer.wallsLength     );
static tConfItem<REAL>   sp_er		("SP_EXPLOSION_RADIUS"		,		singlePlayer.explosionRadius );

static void GameSettingsMP(){
    multiPlayer.Menu();
}

static void GameSettingsSP(){
    singlePlayer.Menu();
}

static void GameSettingsCurrent(){
    sg_currentSettings->Menu();
}

static REAL sg_Timeout = 5.0f;
static tConfItem<REAL>   sg_ctimeout("GAME_TIMEOUT"		,		sg_Timeout );

bool sg_TalkToMaster = true;
static tSettingItem<bool> sg_ttm("TALK_TO_MASTER",
                                 sg_TalkToMaster);

#define PREPARE_TIME 4

static bool just_connected=true;
static bool sg_singlePlayer=0;
static int winner=0;
static int absolute_winner=0;
static REAL lastTime_gameloop=0;
static REAL lastTimeTimestep=0;

static int wishWinner = 0;
static char const * wishWinnerMessage = "";

void sg_DeclareWinner( eTeam* team, char const * message )
{
    if ( team && !winner )
    {
        wishWinner = team->TeamID() + 1;
        wishWinnerMessage = message;
    }
}


static tCONTROLLED_PTR(gGame) sg_currentGame;



/*
static gCycle *Cycle(int id){
  eGameObject *object=NULL;
  for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
    if (se_PlayerNetIDs[i]->pID==id && se_PlayerNetIDs[i]->Object())
      object=se_PlayerNetIDs[i]->Object();

  return dynamic_cast<gCycle *>(object);
}
*/

#ifdef POWERPAK_DEB
#include "PowerPak/poweruInput.h"
#include "PowerPak/joystick.h"
#endif

#include "rRender.h"



void exit_game_grid(eGrid *grid){
    grid->Clear();
}

void exit_game_objects(eGrid *grid){
    sr_con.fullscreen=true;

    su_prefetchInput=false;

    int i;
    for (i=ePlayer::Num()-1;i>=0;i--){
        if (ePlayer::PlayerConfig(i))
            tDESTROY(ePlayer::PlayerConfig(i)->cam);
    }

    eGameObject::DeleteAll(grid);

    if (sn_GetNetState()!=nCLIENT)
        for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            if (se_PlayerNetIDs(i))
                se_PlayerNetIDs(i)->ClearObject();

    gNetPlayerWall::Clear();

    exit_game_grid(grid);
}

REAL exponent(int i)
{
    int abs = i;
    if ( abs < 0 )
        abs = -abs;

    REAL ret = 1;
    REAL fac = sqrtf(2);

    while (abs > 0)
    {
        if ( 1 == (abs & 1) )
            ret *= fac;

        fac *= fac;
        abs >>= 1;
    }

    if (i < 0)
        ret = 1/ret;

    return ret;
}

#ifndef DEDICATED
extern REAL stc_fastestSpeedRound;
#endif
extern bool sg_axesIndicator;

void init_game_grid(eGrid *grid, gParser *aParser){
    se_ResetGameTimer();
    se_PauseGameTimer(true);

#ifndef DEDICATED
    if (sr_glOut){
        sr_ResetRenderState();

        // rSysDep::ClearGL();

        // glViewport (0, 0, static_cast<GLsizei>(sr_screenWidth), static_cast<GLsizei>(sr_screenHeight));

        // rSysDep::SwapGL();
    }
    stc_fastestSpeedRound = .0;
#endif

    /*
      if(!speedup)
      SDL_Delay(1000);
    */

    Arena.PrepareGrid(grid, aParser);

    absolute_winner=winner=wishWinner=0;
}

int sg_NumHumans()
{
    int humans = 0;
    for (int i = se_PlayerNetIDs.Len()-1; i>=0; i--)
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        if (p->IsHuman() && p->IsActive() && ( bool(p->CurrentTeam()) || bool(p->NextTeam()) ) )
            humans++;
    }

    return humans;
}

int sg_NumUsers()
{
    // return se_PlayerNetIDs.Len() ;
#ifdef DEDICATED
    return sn_NumUsers();
#else
    return sn_NumUsers() + 1;
#endif
}

static void sg_copySettings()
{
    eTeam::minTeams					= sg_currentSettings->minTeams;
    eTeam::maxTeams					= sg_currentSettings->maxTeams;
    eTeam::maxPlayers				= sg_currentSettings->maxPlayersPerTeam;
    eTeam::minPlayers				= sg_currentSettings->minPlayersPerTeam;
    eTeam::maxImbalance			 	= sg_currentSettings->maxTeamImbalance;
    eTeam::balanceWithAIs			= sg_currentSettings->balanceTeamsWithAIs;
    eTeam::enforceRulesOnQuit		= sg_currentSettings->enforceTeamRulesOnQuit;

    gCycle::SetWallsStayUpDelay	( sg_currentSettings->wallsStayUpDelay 	);
    gCycle::SetWallsLength		( sg_currentSettings->wallsLength	  	);
    gCycle::SetExplosionRadius	( sg_currentSettings->explosionRadius 	);
    gCycle::SetSpeedMultiplier	( exponent( sg_currentSettings->speedFactor ) );
    gArena::SetSizeMultiplier 	( exponent( sg_currentSettings->sizeFactor ) );
}

void update_settings()
{
    if (sn_GetNetState()!=nCLIENT)
    {
#ifdef DEDICATED
        // wait for players to join
        {
            bool restarted = false;

            REAL timeout = tSysTimeFloat() + 3.0f;
            while ( sg_NumHumans() <= 0 && sg_NumUsers() > 0 )
            {
                if ( !restarted && bool(sg_currentGame) )
                {
                    sg_currentGame->StartNewMatch();
                    restarted = true;
                }

                if ( tSysTimeFloat() > timeout )
                {
                    tOutput o("$gamestate_wait_players");
                    sn_CenterMessage(o);

                    tOutput o2("$gamestate_wait_players_con");
                    sn_ConsoleOut(o2);

                    timeout = tSysTimeFloat() + 10.0f;
                }

                // kick spectators and chatbots
                nMachine::KickSpectators();
                ePlayerNetID::RemoveChatbots();

                // wait for network messages
                sn_BasicNetworkSystem.Select( 0.1f );
                gGame::NetSyncIdle();

                // handle console input
                sr_Read_stdin();
            }
        }

        if ( sg_NumUsers() <= 0 && bool( sg_currentGame ) )
        {
            sg_currentGame->NoLongerGoOn();
        }

        // count the active players
        int humans = sg_NumHumans();

        bool newsg_singlePlayer = (humans<=1);
#else
        bool newsg_singlePlayer = (sn_GetNetState() == nSTANDALONE);
#endif
        if (sg_singlePlayer != newsg_singlePlayer && bool( sg_currentGame ) )
        {
            sg_currentGame->StartNewMatch();
        }
        sg_singlePlayer=newsg_singlePlayer;

        if (sg_singlePlayer)
            sg_currentSettings = &singlePlayer;
        else
            sg_currentSettings = &multiPlayer;

        sg_copySettings();
    }

    // update team properties
    for (int i = eTeam::teams.Len() - 1; i>=0; --i)
        eTeam::teams(i)->UpdateProperties();
}

static int sg_spawnPointGroupSize=0;
static tSettingItem< int > sg_spawnPointGroupSizeConf( "SPAWN_POINT_GROUP_SIZE", sg_spawnPointGroupSize );

void init_game_objects(eGrid *grid){
    /*
      static REAL r[MAX_PLAYERS]={1,.2,.2,1};
      static REAL g[MAX_PLAYERS]={.2,1,.2,1};
      static REAL b[MAX_PLAYERS]={.2,.2,1,.2};
    */

    if (sn_GetNetState()!=nCLIENT)
    {
        // update team settings
        gAIPlayer::SetNumberOfAIs(sg_currentSettings->numAIs,
                                  sg_currentSettings->minPlayers,
                                  sg_currentSettings->AI_IQ);

        int spawnPointsUsed = 0;
        for (int t=eTeam::teams.Len()-1;t>=0;t--)
        {
            eTeam *team = eTeam::teams(t);
            team->Update();

            gSpawnPoint *spawn = Arena.LeastDangerousSpawnPoint();
            spawnPointsUsed++;

            // if the spawn points are grouped, make sure the last player is not in a goup of his
            // own by skipping one spawnpoint
            if ( ( eTeam::teams(0)->IsHuman() || eTeam::teams(0)->NumPlayers() == 1 ) && sg_spawnPointGroupSize > 2 && ( spawnPointsUsed % sg_spawnPointGroupSize == sg_spawnPointGroupSize - 1 ) )
            {
                // the next spawn point is the last of this group. Skip it if
                // either only two players are left (and the one would need to play alone)
                // or we can fill the remaining groups well with one player short.
                if ( t == 2 || ( ( t % ( sg_spawnPointGroupSize - 1 ) ) == 0 && t < ( sg_spawnPointGroupSize - 1 ) * ( sg_spawnPointGroupSize - 1 ) ) )
                {
                    eCoord pos, dir;
                    spawn->Spawn( pos, dir );
                    spawn = Arena.LeastDangerousSpawnPoint();
                    spawnPointsUsed++;
                }
            }

            int numPlayers = team->NumPlayers();
            for (int p = 0; p<numPlayers; ++p)
            {
                ePlayerNetID *pni=team->Player( p );

                if ( !team->IsHuman() )
                {
                    spawn = Arena.LeastDangerousSpawnPoint();
                    spawnPointsUsed++;
                }

                //			bool isHuman = pni->IsHuman();

#ifdef KRAWALL_SERVER
                // don't allow unknown players to play
                if (!pni->IsAuth())
                    continue;
#endif

                // don't give inactive players a cycle
                if (!pni->IsActive())
                    continue;

                eCoord pos,dir;
                gCycle *cycle=NULL;
                if (sn_GetNetState()!=nCLIENT){
#ifdef DEBUG
                    //                std::cout << "spawning player " << pni->name << '\n';
#endif
                    spawn->Spawn(pos,dir);
                    pni->Greet();
                    cycle = new gCycle(grid, pos, dir, pni, 0);
                    pni->ControlObject(cycle);
                    nNetObject::SyncAll();
                }
                //    int i=se_PlayerNetIDs(p)->pID;

                // se_ResetGameTimer();
                // se_PauseGameTimer(true);
            }
        }


#ifdef ALLOW_NO_TEAM
        for (int p=se_PlayerNetIDs.Len()-1;p>=0;p--){
            ePlayerNetID *pni=se_PlayerNetIDs(p);

            gSpawnPoint *spawn=Arena.LeastDangerousSpawnPoint();

            if ( NULL == pni->CurrentTeam() )
            {
#ifdef KRAWALL_SERVER
                // don't allow unknown players to play
                if (!pni->IsAuth())
                    continue;
#endif

                // don't give inactive players a cycle
                if (!pni->IsActive())
                    continue;

                eCoord pos,dir;
                gCycle *cycle=NULL;
                if (sn_GetNetState()!=nCLIENT){
#ifdef DEBUG
                    //con << "spawning player " << se_PlayerNetIDs[p]->name << '\n';
#endif
                    st_Breakpoint();

                    spawn->Spawn(pos,dir);
                    pni->Greet();
                    cycle = new gCycle(grid, pos, dir, pni, 0);
                    pni->ControlObject(cycle);
                    nNetObject::SyncAll();
                }
                //    int i=se_PlayerNetIDs(p)->pID;

                // se_ResetGameTimer();
                // se_PauseGameTimer(true);
            }
        }
#endif
    }

    /*
    for(int p=se_PlayerNetIDs.Len()-1;p>=0;p--){
    	ePlayerNetID *pni=se_PlayerNetIDs(p);

    	bool isHuman = pni->IsHuman();

    #ifdef KRAWALL_SERVER
    	// don't allow unknown players to play
    	if (!pni->IsAuth())
    		continue;
    #endif

    	// don't give inactive players a cycle
    	if (!pni->IsActive())
    		continue;
       
    	eCoord pos,dir;
    	gCycle *cycle=NULL;
    	if (sn_GetNetState()!=nCLIENT){
    #ifdef DEBUG
    		//con << "spawning player " << se_PlayerNetIDs[p]->name << '\n';
    #endif
    		spawn->Spawn(pos,dir);
    		pni->Greet();
    		cycle = new gCycle(grid, pos, dir, pni, 0);
    		pni->ControlObject(cycle);
    		nNetObject::SyncAll();
    	}
    	//    int i=se_PlayerNetIDs(p)->pID;

    	se_ResetGameTimer();
    	se_PauseGameTimer(true);

    	if (sg_currentSettings->gameType==gHUMAN_VS_AI){
    		//      if (Cycle(p))
    		//	Cycle(p)->team=1;
    		if (cycle)
    		{
    			spawn=Arena.LeastDangerousSpawnPoint();
    			if (isHuman)
    				cycle->team=2;
    			else
    			{
    				cycle->team=1;
    			}
    		}
    	}
    	else{
    #ifdef DEBUG
    		//con << "new eSpawnPoint.\n";
    #endif
    		if (cycle)
    		{
    			spawn=Arena.LeastDangerousSpawnPoint();
    			if (isHuman)
    				cycle->team=p + 2;
    			else
    				cycle->team=1;
    		}
    		//      if (Cycle(p))
    		//	Cycle(p)->team=p+1;
    	}
    }
    */

    //	spawn=Arena.LeastDangerousSpawnPoint();

    /*
    static REAL rgb[MAXAI][3]={
      {1,1,.2},
      {1,.2,1},
      {.2,1,1},
      {1,.6,.2},
      {.2,1,.6},
      {.6,.2,1},
      {1,.2,.6},
      {.6,1,.2},
      {1,1,1},
      {.2,.2,.2}
    };
    */


    rITexture::LoadAll();
    // se_ResetGameTimer();
    // se_PauseGameTimer(true);

    su_prefetchInput=true;

    lastTime_gameloop=lastTimeTimestep=0;

    // reset scores, all players that are spawned need to get ladder points
    ePlayerNetID::ResetScoreDifferences();
}

void init_game_camera(eGrid *grid){
#ifndef DEDICATED
    for (int i=ePlayer::Num()-1;i>=0;i--)
        if (ePlayer::PlayerIsInGame(i)){
            ePlayerNetID *p=ePlayer::PlayerConfig(i)->netPlayer;

            if ( sg_currentSettings->finishType == gFINISH_EXPRESS && ( sn_GetNetState() != nCLIENT ) )
                se_ResetGameTimer( -PREPARE_TIME/5 - sg_extraRoundTime );
            else
                se_ResetGameTimer( -PREPARE_TIME - sg_extraRoundTime );

            // se_PauseGameTimer(true);

            ePlayer::PlayerConfig(i)->cam=new gCamera(grid,
                                          ePlayer::PlayerViewport(i),
                                          p,
                                          ePlayer::PlayerConfig(i),
                                          CAMERA_SMART);

            lastTime_gameloop=lastTimeTimestep=0;
        }
#else
    se_ResetGameTimer( -PREPARE_TIME - sg_extraRoundTime );
    se_PauseGameTimer(false);
#endif
    /*
      for(int p=se_PlayerNetIDs.Len()-1;p>=0;p--){
      int i=se_PlayerNetIDs(p)->pID;

      se_ResetGameTimer(-PREPARE_TIME);
      se_PauseGameTimer(true);

      if (i>=0)
         playerConfig[i]->cam=new eCamera(PlayerViewport(i),
      se_PlayerNetIDs(p),CAMERA_SMART);
      }

    */
}

bool think=1;

#ifdef DEDICATED
static int sg_dedicatedFPS = 40;
static tSettingItem<int> sg_dedicatedFPSConf( "DEDICATED_FPS", sg_dedicatedFPS );

static REAL sg_dedicatedFPSIdleFactor = 2.0;
static tSettingItem<REAL> sg_dedicatedFPSMinstepConf( "DEDICATED_FPS_IDLE_FACTOR", sg_dedicatedFPSIdleFactor );
#endif

void s_Timestep(eGrid *grid, REAL time,bool cam){
    gNetPlayerWall::s_CopyIntoGrid();
    REAL minstep = 0;
#ifdef DEDICATED
    minstep = 1.0/sg_dedicatedFPS;

    // the low possible simulation frequency, together with lazy timesteps, produces
    // this much possible extra time difference between gameobjects
    eGameObject::SetMaxLazyLag( ( 1 + sg_dedicatedFPSIdleFactor )/( sg_dedicatedFPSIdleFactor * sg_dedicatedFPS ) );
#endif
    eGameObject::s_Timestep(grid, time, minstep );

    if (cam)
        eCamera::s_Timestep(grid, time);

    lastTimeTimestep=time;
}

#ifndef DEDICATED
void RenderAllViewports(eGrid *grid){
    rViewportConfiguration *conf=rViewportConfiguration::CurrentViewportConfiguration();

    if (sr_glOut){
        sr_ResetRenderState();

        // enable distance based fog
        /*
        glFogi( GL_FOG_MODE, GL_EXP );
        glFogf( GL_FOG_DENSITY, .01/gArena::SizeMultiplier() );
        GLfloat black[3]={0,0,0};
        glFogfv( GL_FOG_COLOR, black );
        glEnable( GL_FOG );
        */

        const tList<eCamera>& cameras = grid->Cameras();

        for (int i=cameras.Len()-1;i>=0;i--){
            int p=sr_viewportBelongsToPlayer[i];
            conf->Select(i);
            rViewport *act=conf->Port(i);
            if (act && ePlayer::PlayerConfig(p))
                ePlayer::PlayerConfig(p)->Render();
            else con << "hey! viewport " << i << " does not exist!\n";
        }

        // glDisable( GL_FOG );
    }

#ifdef POWERPAK_DEB
    if (pp_out){
        eGameObject::PPDisplayAll();
        PD_ShowDoubleBuffer();
    }
#endif
}
#endif


void Render(eGrid *grid, REAL time, bool swap=true){
#ifdef DEBUG
    //  eFace::UpdateVisAll(10);
#endif
    //  se_debugExt=0;

#ifndef DEDICATED
    if (sr_glOut){
        RenderAllViewports(grid);

        sr_ResetRenderState(true);
        gLogo::Display();

        if (swap){
            rSysDep::SwapGL();

            if (!sr_ZTrick ||
                    (!sr_highRim && !sr_lowerSky && !sr_upperSky) ||
                    sr_floorDetail<rFLOOR_TEXTURE ||
                    sr_floorMirror==rMIRROR_OBJECTS){
                rSysDep::ClearGL();
            }
        }
    }
    else
    {
        if ( swap )
            rSysDep::SwapGL();

        tDelay( sn_defaultDelay );
    }
#endif
}

#ifdef DEDICATED
static void cp(){
    std::ofstream s;

    if ( tDirectories::Var().Open(s, "players.txt") ){
        if (se_PlayerNetIDs.Len()>0)
            s << tColoredString::RemoveColors(ePlayerNetID::Ranking( -1, false ));
        else{
            tOutput o;

            int count=0;
            for (int i=MAXCLIENTS;i>0;i--)
                if (sn_Connections[i].socket)
                    count++;
            if (count==0)
                o << "$online_activity_nobody";
            else if (count==1)
                o << "$online_activity_onespec";
            else
            {
                o.SetTemplateParameter(1, count);
                o << "$online_activity_manyspec";
            }
            o << "\n";
            s << o;
        }
    }
}
#endif


static void own_game( nNetState enter_state ){
    tNEW(gGame);
    se_MakeGameTimer();
#ifndef DEDICATED
    stc_fastestSpeedRound = .0;
#endif
    sg_EnterGame( enter_state );

    // write scores one last time
    ePlayerNetID::LogScoreDifferences();
    se_SaveToLadderLog(tString("GAME_END\n"));
    se_sendEventNotification(tString("Game end"), tString("The Game has ended"));

    sg_currentGame=NULL;
    se_KillGameTimer();
}

static void singlePlayer_game(){
    sn_SetNetState(nSTANDALONE);

    update_settings();
    ePlayerNetID::CompleteRebuild();

    own_game( nSTANDALONE );
}

void sg_HostGame(){
    //we'll want to collect stats
    gStats = new gStatistics();

    {
        // create a game to check map once
        tJUST_CONTROLLED_PTR< gGame > game = tNEW(gGame);
        game->Verify();
    }

    if (sg_TalkToMaster)
    {
        nServerInfo::TellMasterAboutMe( gServerBrowser::CurrentMaster() );
    }

    sn_SetNetState(nSERVER);

    update_settings();
    ePlayerNetID::CompleteRebuild();

    tAdvanceFrame();

    //#ifndef DEBUG
#ifdef DEDICATED
    static double startTime=tSysTimeFloat();

    if ( sg_NumUsers() == 0)
    {
        cp();
        con << tOutput("$online_activity_napping") << "\n";
        sg_Timestamp();

        int counter = -1;

        int numPlayers = 0;

        while (numPlayers == 0 &&
                (ded_idle<.0001 || tSysTimeFloat()<startTime + ded_idle * 3600 ) && !uMenu::quickexit ){
            sr_Read_stdin();
            st_DoToDo();
            gGame::NetSyncIdle();

            sn_BasicNetworkSystem.Select( 1.0f );

            // std::cout the players who are logged in:
            numPlayers = sg_NumUsers();
            /*
            			for (int i = se_PlayerNetIDs.Len()-1; i>=0; i--)
            				if (se_PlayerNetIDs(i)->IsAuth())
            					numPlayers++;
            */

            if (counter <= 0)
            {
                // restart network, just in case we lost the input socket
                sn_BasicNetworkSystem.AccessListener().Listen(false);
                sn_BasicNetworkSystem.AccessListener().Listen(true);
                counter = 50;
            }
        }

        if (sg_NumUsers() <= 0 && ded_idle>0.0001 &&
                tSysTimeFloat()>= startTime + ded_idle * 3600 )
        {
            uMenu::quickexit = true;
        }
    }
    cp();

    if (!uMenu::quickexit)
#endif
        //#endif
        own_game( nSERVER );

    sn_SetNetState(nSTANDALONE);

    // close listening sockets
    sn_BasicNetworkSystem.AccessListener().Listen(false);

    //wrap up the stats
    delete gStats;
}

static tString sg_roundCenterMessage("");
static tConfItemLine sn_roundCM_ci("ROUND_CENTER_MESSAGE",sg_roundCenterMessage);

static tString sg_roundConsoleMessage("");
static tConfItemLine sn_roundCcM1_ci("ROUND_CONSOLE_MESSAGE",sg_roundConsoleMessage);

static bool sg_RequestedDisconnection = false;

static bool sg_NetworkError( const tOutput& title, const tOutput& message, REAL timeout )
{
    tOutput message2 ( message );

    if ( sn_DenyReason.Len() > 2 )
    {
        message2.AddLiteral("\n\n");
        message2.AddLocale("network_kill_preface");
        message2.AddLiteral("\n");
        message2.AddLiteral(sn_DenyReason);
    }

    nServerInfoBase * redirect = sn_PeekRedirectTo();
    if ( redirect )
    {
        message2.Append( tOutput( "$network_redirect", redirect->GetConnectionName(), (int)redirect->GetPort() ) );
    }

    return tConsole::Message( title, message2, timeout );
}

// revert settings to defaults in the current scope
class gSettingsReverter
{
public:
    gSettingsReverter()
    {
        // restore default configuration of network settings, saving your settings
        nConfItemBase::s_SaveValues();
        nConfItemBase::s_RevertToDefaults();
    }

    ~gSettingsReverter()
    {
        // restore saved values
        nConfItemBase::s_RevertToSavedValues();
    }
};

// receive network data
void sg_Receive()
{
    sn_Receive();
    if ( sn_GetNetState() == nSERVER )
    {
        // servers should ignore data sent to the control socket. It probably would be better
        // to close the socket altogether, but that opens another can of worms.
        sn_DiscardFromControlSocket();
    }
}

// return code: false if there was an error or abort
bool ConnectToServerCore(nServerInfoBase *server)
{
    tASSERT( server );

    ePlayerNetID::ClearAll();

    // revert to default settings, restore current vlaues on exit
    gSettingsReverter reverter;

    sn_bigBrotherString = renderer_identification;

    nNetObject::ClearAll();

    just_connected=true;

    rViewport::Update(MAX_PLAYERS);
    // ePlayerNetID::Update();

#ifndef DEDICATED
    rSysDep::SwapGL();
    rSysDep::ClearGL();
    rSysDep::SwapGL();
    rSysDep::ClearGL();
#endif

    sr_con.autoDisplayAtNewline=true;
    sr_con.fullscreen=true;

    bool to=sr_textOut;
    sr_textOut=true;

    tOutput o;

    nConnectError error = nOK;

    nNetObject::ClearAll();

    o.SetTemplateParameter(1, server->GetName());
    o << "$network_connecting_to_server";
    con << o;
    error = server->Connect();

    switch (error)
    {
    case nOK:
        break;
    case nTIMEOUT:
        sg_NetworkError("$network_message_timeout_title", "$network_message_timeout_inter", 20);
        return false;
        break;

    case nDENIED:
        sg_NetworkError("$network_message_denied_title", sn_DenyReason.Len() > 2 ? "$network_message_denied_inter2" : "$network_message_denied_inter", 20);
        return false;
        break;
    }

    // ePlayerNetID::CompleteRebuild();

    if (sn_GetNetState()==nCLIENT){
        REAL endTime=tSysTimeFloat()+30;
        con << tOutput("$network_connecting_gamestate");
        while (!sg_currentGame && tSysTimeFloat()<endTime && (sn_GetNetState() != nSTANDALONE)){
            tAdvanceFrame();
            sg_Receive();
            nNetObject::SyncAll();
            tAdvanceFrame();
            sn_SendPlanned();
            st_DoToDo();

#ifndef DEDICATED
            rSysDep::SwapGL();
            rSysDep::ClearGL();
#endif

            sn_Delay();
        }
        if (sg_currentGame){
            sr_con.autoDisplayAtNewline=false;
            sr_con.fullscreen=false;

            con << tOutput("$network_syncing_gamestate");
            sg_EnterGame( nCLIENT );
        }
        else{
            //con << "Timeout. Try again!\n";
            sg_NetworkError("$network_message_lateto_title", "$network_message_lateto_inter", 20);
        }
    }

    bool ret = true;

    if (!sg_RequestedDisconnection && !uMenu::quickexit)
        switch (sn_GetLastError())
        {
        case nOK:
            ret = sg_NetworkError("$network_message_abortconn_title",
                                  "$network_message_abortconn_inter", 20);
            break;
        default:
            ret = sg_NetworkError("$network_message_lostconn_title",
                                  "$network_message_lostconn_inter", 20);
            break;
        }

    sr_con.autoDisplayAtNewline=false;
    sr_con.fullscreen=false;

    sn_SetNetState(nSTANDALONE);

    sg_currentGame = NULL;
    nNetObject::ClearAll();
    ePlayerNetID::ClearAll();

    sr_textOut=to;

    return ret;
}

void ConnectToServer(nServerInfoBase *server)
{
    bool okToRedirect = ConnectToServerCore( server );

    // REAL redirections = 0;
    // double lastTime = tSysTimeFloat();

    // check for redirection
    while( okToRedirect )
    {
        std::auto_ptr< nServerInfoBase > redirectTo( sn_GetRedirectTo() );

        // abort loop
        if ( !(&(*redirectTo)) )
        {
            break;
        }

        okToRedirect = ConnectToServerCore( redirectTo.get() );

        /*
        // redirection spam chain protection, allow one redirection every 30 seconds. Should
        // be short enough to allow hacky applications (server-to-server teleport), but
        // long enough to allow players to escape via the menu or at least shift-esc.
        static const REAL timeout = 30;
        static const REAL maxRedirections = 5;

        redirections = redirections + 1;
        if ( redirections > maxRedirections )
        {
            break;
        }

        // decay spam protection
        double newTime = tSysTimeFloat();
        REAL dt = newTime - lastTime;
        lastTime = newTime;
        redirections *= 1/(1 + dt * maxRedirections * timeout );
        */
    }
}

static tConfItem<int> mor("MAX_OUT_RATE",sn_maxRateOut);
static tConfItem<int> mir("MAX_IN_RATE",sn_maxRateIn);


static tConfItem<int> pc("PING_CHARITY",pingCharity);


uMenu *sg_IngameMenu=NULL;
uMenu *sg_HostMenu  =NULL;

void ret_to_MainMenu(){
    sg_RequestedDisconnection = true;

    if (sg_HostMenu)
        sg_HostMenu->Exit();

    if (sg_IngameMenu)
        sg_IngameMenu->Exit();

    sr_con.fullscreen=true;
    sr_con.autoDisplayAtNewline=true;

    if (sg_currentGame)
        sg_currentGame->NoLongerGoOn();

    sn_SetNetState(nSTANDALONE);

    uMenu::SetIdle(NULL);
}



void net_options(){
    uMenu net_menu("$network_opts_text");

    uMenuItemInt high_port
    (&net_menu,"$network_opts_maxport_text",
     "$network_opts_maxport_help",
     gServerBrowser::highPort,1024,65536);

    uMenuItemInt low_port
    (&net_menu,"$network_opts_minport_text",
     "$network_opts_minport_help",
     gServerBrowser::lowPort,1024,65536);

    /*
    	uMenuItemFunction delpw(&net_menu,
    							"$network_opts_deletepw_text",
    							"$network_opts_deletepw_help",
    							&se_DeletePasswords);
      
    	uMenuItemSelection<int> storepw(&net_menu,
    									"$login_storepw_text",
    									"$login_storepw_help",
    									se_PasswordStorageMode);
    	storepw.NewChoice("$login_storepw_dont_text",
    					  "$login_storepw_dont_help",
    					  -1);
    	storepw.NewChoice("$login_storepw_mem_text",
    					  "$login_storepw_mem_help",
    					  0);
    	storepw.NewChoice("$login_storepw_disk_text",
    					  "$login_storepw_disk_help",
    					  1);
    */

    /*
      uMenuItemToggle master
      (&net_menu,"$network_opts_master_text",
      "$network_opts_master_help",
      sg_TalkToMaster);
    */


    uMenuItemInt out_rate
    (&net_menu,"$network_opts_outrate_text",
     "$network_opts_outrate_help",
     sn_maxRateOut,1,20);


    uMenuItemInt in_rate
    (&net_menu,"$network_opts_inrate_text",
     "$network_opts_inrate_help",
     sn_maxRateIn,1,20);


    uMenuItemToggle po2
    (&net_menu,"$network_opts_predict_text",
     "$network_opts_predict_help",
     sr_predictObjects);

    uMenuItemToggle lm2
    (&net_menu,"$network_opts_lagometer_text",
     "$network_opts_lagometer_help",
     sr_laggometer);

    uMenuItemToggle ai
    (&net_menu,"$network_opts_axesindicator_text",
     "$network_opts_axesindicator_help",
     sg_axesIndicator);


    uMenuItemInt p_s
    (&net_menu,"$network_opts_pingchar_text",
     "$network_opts_pingchar_help",
     pingCharity,0,300,20);

    net_menu.Enter();

    if (gServerBrowser::lowPort > gServerBrowser::highPort)
        gServerBrowser::highPort = gServerBrowser::lowPort;
}

void sg_HostGameMenu(){
    uMenu net_menu("$network_host_text");

    sg_HostMenu = &net_menu;

    uMenuItemInt port(&net_menu, "$network_host_port_text",
                      "$network_host_port_help"
                      ,reinterpret_cast<int &>(sn_serverPort), gServerBrowser::lowPort, gServerBrowser::highPort);

    uMenuItemString serverName
    (&net_menu,"$network_host_name_text",
     "$network_host_name_help",
     sn_serverName);

    uMenuItemFunction settings1
    (&net_menu,
     "$game_settings_menu_text",
     "$game_settings_menu_help",
     &GameSettingsMP);

    uMenuItemFunction serv
    (&net_menu,"$network_host_host_text",
     "$network_host_host_help",&sg_HostGame);

    net_menu.ReverseItems();
    net_menu.SetSelected(0);
    net_menu.Enter();

    sg_HostMenu = NULL;
}

#ifndef DEDICATED
class gNetIdler: public rSysDep::rNetIdler
{
public:
    virtual bool Wait() //!< wait for something to do, return true fi there is work
    {
        return sn_BasicNetworkSystem.Select( 0.1 );
    }
    virtual void Do()  //!< do the work.
    {
        tAdvanceFrame();
        sg_Receive();
        tAdvanceFrame();
        sn_SendPlanned();
    }
};
#endif

void net_game(){
#ifndef DEDICATED
    uMenu net_menu("$network_menu_text");

    uMenuItemFunction cust
    (&net_menu,"$network_custjoin_text",
     "$network_custjoin_help",&gServerFavorites::CustomConnectMenu);

    uMenuItemFunction mas
    (&net_menu,"$masters_menu",
     "$masters_menu_help",&gServerFavorites::AlternativesMenu);

    uMenuItemFunction fav
    (&net_menu,"$bookmarks_menu",
     "$bookmarks_menu_help",&gServerFavorites::FavoritesMenu);

    uMenuItemFunction bud
    (&net_menu,"$friends_menu",
     "$friends_menu_help",&gFriends::FriendsMenu);

    uMenuItemFunction opt
    (&net_menu,"$network_opts_text",
     "$network_opts_help",&net_options);

    /*
      uMenuItemFunction serv
      (&net_menu,"$network_host_text",
      "$network_host_help",&sg_HostGameMenu);
    */

    uMenuItemFunction inter
    (&net_menu,"$network_menu_internet_text",
     "$network_menu_internet_help",&gServerBrowser::BrowseMaster);

    uMenuItemFunction lan
    (&net_menu,"$network_menu_lan_text",
     "$network_menu_lan_help",&gServerBrowser::BrowseLAN);

    gNetIdler idler;
    // rSysDep::StartNetSyncThread( &idler );
    net_menu.Enter();
    rSysDep::StopNetSyncThread();
#endif
}



static void StartNewMatch(){
    if (sg_currentGame)
        sg_currentGame->StartNewMatch();
    if (sn_GetNetState()!=nCLIENT){
        sn_CenterMessage("$gamestate_reset_center");
        sn_ConsoleOut("$gamestate_reset_console");
    }
}

static void StartNewMatch_conf(std::istream &){
    StartNewMatch();
}

static tConfItemFunc snm("START_NEW_MATCH",&StartNewMatch_conf);

#ifdef DEDICATED
static void Quit_conf(std::istream &){

    if ( sg_currentGame )
    {
        sg_currentGame->NoLongerGoOn();
    }

    // mark end of recording
    tRecorder::Playback("END");
    tRecorder::Record("END");
    uMenu::quickexit = true;
}

static tConfItemFunc quit_conf("QUIT",&Quit_conf);
static tConfItemFunc exit_conf("EXIT",&Quit_conf);
#endif

void st_PrintPathInfo(tOutput &buf);

void sg_DisplayVersionInfo() {
    tOutput versionInfo;
    versionInfo << "$version_info_version" << "\n";
    st_PrintPathInfo(versionInfo);
    versionInfo << "$version_info_misc_stuff";
    sg_FullscreenMessage("$version_info_title", versionInfo, 1000);
}

void MainMenu(bool ingame){
    //	update_settings();

    if (ingame)
        sr_con.SetHeight(2);

    //gLogo::SetDisplayed(true);

    tOutput gametitle;
    if (!ingame)
        gametitle << "$game_menu_text";
    else
        gametitle << "$game_menu_ingame_text";

    uMenu game_menu(gametitle);

    uMenuItemFunction *reset=NULL;

    if (ingame && sn_GetNetState()!=nCLIENT){
        reset=new uMenuItemFunction
              (&game_menu,"$game_menu_reset_text",
               "$game_menu_reset_help",
               &StartNewMatch);
    }

    uMenuItemFunction *settings1=NULL;

    if (!ingame)
        settings1=tNEW(uMenuItemFunction)
                  (&game_menu,
                   "$game_settings_menu_text",
                   "$game_settings_menu_help",
                   &GameSettingsSP);
    else
        settings1=tNEW(uMenuItemFunction)
                  (&game_menu,
                   "$game_settings_menu_text",
                   "$game_settings_menu_help",
                   &GameSettingsCurrent);

    uMenuItemFunction *connect=NULL,*start=NULL,*sound=NULL;

    if (!ingame){
        connect=new uMenuItemFunction
                (&game_menu,
                 "$network_menu_text",
                 "$network_menu_help",
                 &net_game);

        start= new uMenuItemFunction(&game_menu,"$game_menu_start_text",
                                     "$game_menu_start_help",&singlePlayer_game);
    }

    tOutput title;
    title.SetTemplateParameter( 1, sn_programVersion );
    if (!ingame)
        title << "$main_menu_text";
    else
        title << "$ingame_menu_text";

    uMenu MainMenu(title,false);

    if (ingame)
        sg_IngameMenu = &MainMenu;

    char const * extitle,* exhelp;
    if (!ingame){
        extitle="$main_menu_exit_text";
        exhelp="$main_menu_exit_help";
    }
    else{
        extitle="$ingame_menu_exit_text";
        exhelp="$ingame_menu_exit_help";
    }

    uMenuItemExit exx(&MainMenu,extitle,
                      exhelp);

    uMenuItemFunction abb(&MainMenu,
                          "$main_menu_about_text",
                          "$main_menu_about_help",
                          &sg_DisplayVersionInfo);


    uMenuItemFunction *return_to_main=NULL;

    if (ingame){
        if (sn_GetNetState()==nSTANDALONE)
            return_to_main=new uMenuItemFunction
                           (&MainMenu,"$game_menu_exit_text",
                            "$game_menu_exit_help",
                            &ret_to_MainMenu);
        else if (sn_GetNetState()==nCLIENT)
            return_to_main=new uMenuItemFunction
                           (&MainMenu,"$game_menu_disconnect_text",
                            "$game_menu_disconnect_help",
                            &ret_to_MainMenu);
        else
            return_to_main=new uMenuItemFunction
                           (&MainMenu,
                            "$game_menu_shutdown_text",
                            "game_menu_shutdown_help",
                            &ret_to_MainMenu);
    }



    uMenu Settings("$system_settings_menu_text");

    uMenuItemSubmenu subm_settings
    (&MainMenu,&Settings,
     "$system_settings_menu_help");


    uMenuItem* team = NULL;
    if ( ingame )
    {
        team = tNEW( uMenuItemFunction) ( &MainMenu,
                                          "$team_menu_title",
                                          "$team_menu_help",
                                          &gTeam::TeamMenu );
    }

    uMenuItemFunction *se_PlayerMenu=NULL;

    //   if (!ingame)
    se_PlayerMenu= new uMenuItemFunction
                   (&MainMenu,"$player_mainmenu_text",
                    "$player_mainmenu_help",
                    &sg_PlayerMenu);

    uMenuItemFunction *player_police=NULL;
    uMenuItemFunction *voting=NULL;
    if ( ingame && sn_GetNetState() != nSTANDALONE )
    {
        player_police = tNEW( uMenuItemFunction )( &MainMenu, "$player_police_text", "$player_police_help", ePlayerNetID::PoliceMenu );

        if ( eVoter::VotingPossible() )
            voting = tNEW( uMenuItemFunction )( &MainMenu, "$voting_menu_text", "$voting_menu_help", eVoter::VotingMenu );
    }

    uMenu misc("$misc_menu_text");

    //  misc.SetCenter(.25);

    uMenuItemFunction language
    (&misc,"$language_menu_title",
     "$language_menu_help",
     &sg_LanguageMenu);

    uMenuItemFunction global_key
    (&misc,"$misc_global_key_text",
     "$misc_global_key_help",
     &su_InputConfigGlobal);

    uMenuItemToggle wrap
    (&misc,"$misc_menuwrap_text",
     "$misc_menuwrap_help",
     uMenu::wrap);

    uMenuItemToggle to2
    (&misc,"$misc_textout_text",
     "$misc_textout_help",sr_textOut);



    uMenuItemToggle mp
    (&misc,"$misc_moviepack_text",
     "$misc_moviepack_help",sg_moviepackUse);


    uMenuItemSubmenu misc_sm
    (&Settings,&misc,
     "$misc_menu_help");

    sound = new uMenuItemFunction
            (&Settings,"$sound_menu_text",
             "$sound_menu_help",&se_SoundMenu);

    uMenuItemSubmenu subm
    (&Settings,&sg_screenMenu,
     "$display_settings_menu_help");

    uMenuItemSubmenu *gamemenuitem = NULL;
    if (sn_GetNetState() != nCLIENT)
    {
        char const * gamehelp;
        if (!ingame)
            gamehelp="$game_menu_main_help";
        else
            gamehelp="$game_menu_ingame_help";

        gamemenuitem = tNEW(uMenuItemSubmenu) (&MainMenu,
                                               &game_menu,
                                               gamehelp
                                              );
    }

    if (!ingame)
    {
        rViewport::Update(MAX_PLAYERS);
        // ePlayerNetID::Update();
    }

    MainMenu.Enter();

    if (settings1)
        delete settings1;
    if (team)
        delete team;
    if (gamemenuitem)
        delete gamemenuitem;
    if (sound)
        delete sound;
    if (connect)
        delete connect;
    if (start)
        delete start;
    if (return_to_main)
        delete return_to_main;
    if (se_PlayerMenu)
        delete se_PlayerMenu;
    if (reset)
        delete reset;
    if ( player_police )
        delete player_police;
    if ( voting )
        delete voting;

    if (ingame)
        sg_IngameMenu=NULL;

    if (ingame)
    {
        gLogo::SetDisplayed(false);
        sr_con.SetHeight(7);
    }
}



// Game states:

#define GS_CREATED 0        // newborn baby
#define GS_TRANSFER_SETTINGS	7
#define GS_CREATE_GRID			10
#define GS_CREATE_OBJECTS		20
#define GS_TRANSFER_OBJECTS		30
#define GS_CAMERA				35
#define GS_SYNCING				40
#define GS_SYNCING2				41
#define GS_SYNCING3				42
#define GS_PLAY					50
#define GS_DELETE_OBJECTS		60
#define GS_DELETE_GRID			70
#define GS_STATE_MAX			80

static bool sg_AbeforeB( int stateA, int stateB )
{
    return ( ( GS_STATE_MAX + stateA - stateB ) % GS_STATE_MAX > GS_STATE_MAX >> 2 );
}

#ifndef DEDICATED
static void ingame_menu_cleanup();
static void ingame_menu()
{
    //    globalingame=true;
    try
    {
        se_ChatState( ePlayerNetID::ChatFlags_Menu, true );
        if (sn_GetNetState()==nSTANDALONE)
            se_PauseGameTimer(true);
        MainMenu(true);
    }
    catch ( ... )
    {
        ingame_menu_cleanup();
        ret_to_MainMenu();
        sg_IngameMenu=NULL;
        sg_HostMenu=NULL;
        throw;
    }
    ingame_menu_cleanup();
}

static void ingame_menu_cleanup()
{
    if (sn_GetNetState()==nSTANDALONE)
        se_PauseGameTimer(false);
    se_ChatState(ePlayerNetID::ChatFlags_Menu, false);
    if ((bool(sg_currentGame) && sg_currentGame->GetState()!=GS_PLAY))
        //      || se_PlayerNetIDs.Len()==0)
    {
        rViewport::Update(MAX_PLAYERS);
        ePlayerNetID::Update();
    }
    //    globalingame=false;
}
#endif


static nNOInitialisator<gGame> game_init(310,"game");

nDescriptor &gGame::CreatorDescriptor() const{
    return game_init;
}


void gGame::Init(){
    m_Mixer = eSoundMixer::GetMixer();
    grid = tNEW(eGrid());
    state=GS_CREATED;
    stateNext=GS_TRANSFER_SETTINGS;
    goon=true;

    sg_currentGame=this;
#ifdef DEBUG
    // con << "Game created.\n";
#endif
    aParser = tNEW(gParser)(&Arena, grid);

    rounds = 0;
    StartNewMatchNow();
}


void gGame::NoLongerGoOn(){
    goon=false;
    //	stateNext=GS_DELETE_OBJECTS;
    if (sn_GetNetState()==nSERVER)
        RequestSync();
}

// console with filter for remote admin output redirection
class gMapLoadConsoleFilter:public tConsoleFilter{
public:
    gMapLoadConsoleFilter()
    {
    }

    ~gMapLoadConsoleFilter()
    {
    }

    tColoredString message_; // the console log for the error message
private:
    // we want to come first
    virtual int DoGetPriority() const{
        return -100;
    }

    // don't actually filter; take line and add it to the potential error message
    virtual void DoFilterLine( tString &line )
    {
        message_ << line << "\n";
    }
};

static void sg_ParseMap ( gParser * aParser, tString mapfile )
{
    {
        gMapLoadConsoleFilter consoleLog;
#ifdef DEBUG
        con << "Loading map " << mapfile << "...\n";
#endif
        if (!aParser->LoadWithoutParsing(mapfile))
        {
            tOutput errorMessage( sn_GetNetState() == nCLIENT ? "$map_file_load_failure_server" : "$map_file_load_failure_self", mapfile );

#ifndef DEDICATED
            errorMessage << "\nLog:\n" << consoleLog.message_;
#endif

            con << errorMessage;

            tOutput errorTitle("$map_file_load_failure_title");

            if ( sn_GetNetState() != nSTANDALONE )
            {
                throw tGenericException( errorMessage, errorTitle );
            }

            if (!aParser->LoadWithoutParsing(DEFAULT_MAP)) {
                errorMessage << "$map_file_load_failure_default";
                throw tGenericException( errorMessage, errorTitle );
            }
        }
    }
}

static void sg_ParseMap ( gParser * aParser )
{
    sg_ParseMap(aParser, mapfile);
}

void gGame::Verify()
{
    // test map and load map settings
    sg_ParseMap( aParser );
    init_game_grid(grid, aParser);
    exit_game_grid(grid);
}

gGame::gGame(){
    synced_ = true;
    gLogo::SetDisplayed(false);
    if (sn_GetNetState()!=nCLIENT)
        RequestSync();
    Init();
}

gGame::gGame(nMessage &m):nNetObject(m){
    synced_ = false;
    Init();
}


gGame::~gGame(){
#ifdef DEBUG
    //con << "deleting game...\n";
#endif
    //stateNext=GS_CREATE_GRID;
    //StateUpdate();

    tASSERT( sg_currentGame != this );

    exit_game_objects(grid);
    grid->Clear();
    ePlayerNetID::ThrowOutDisconnected();

    delete aParser;
}


void gGame::WriteSync(nMessage &m){
    nNetObject::WriteSync(m);
    m.Write(stateNext);
#ifdef DEBUG
    //con << "Wrote game state " << stateNext << ".\n";
#endif
}

void gGame::ReadSync(nMessage &m){
    nNetObject::ReadSync(m);

    unsigned short newState;
    m.Read(newState);

#ifdef DEBUG
    con << "Read gamestate " << newState << ".\n";
#endif

    // only update the state if it does not go back a few steps
    if ( sg_AbeforeB( stateNext, newState ) )
        stateNext = newState;

#ifdef DEBUG
    //con << "Read game state " << stateNext << ".\n";
#endif
}


void gGame::NetSync(){
    // find end of recording in playback
    if ( tRecorder::Playback("END") )
    {
        tRecorder::Record("END");
        uMenu::quickexit=true;
    }

#ifdef DEDICATED
    if (!sr_glOut && ePlayer::PlayerConfig(0)->cam)
        tERR_ERROR_INT("Someone messed with the camera!");
#endif
    sg_Receive();
    nNetObject::SyncAll();
    tAdvanceFrame();
    sn_SendPlanned();
}

void gGame::NetSyncIdle(){
    NetSync();
    sn_Delay();
}

unsigned short client_gamestate[MAXCLIENTS+2];

static void client_gamestate_handler(nMessage &m){
    unsigned short state;
    m.Read( state );
    client_gamestate[m.SenderID()] = state;
#ifdef DEBUG
    //	con << "User " << m.SenderID() << " is in Mode " << state << '\n';
#endif
}

static nDescriptor client_gs(311,client_gamestate_handler,"client_gamestate");



void gGame::SyncState(unsigned short state){
    if (sn_GetNetState()==nSERVER){

        bool firsttime=true;
        bool goon=true;
        REAL timeout=tSysTimeFloat()+sg_Timeout*.1;

        sn_Sync(sg_Timeout*.1,true);
        NetSyncIdle();

        while (goon && tSysTimeFloat()<timeout){
            /*
              if (sg_currentGame)
              sg_currentGame->RequestSync();
            */

            sn_Sync(sg_Timeout*.1,true);
            NetSyncIdle();

            goon=false;
            for (int i=MAXCLIENTS;i>0;i--)
                if ( sn_Connections[i].socket )
                {
                    int clientState = client_gamestate[i];
                    if ( sg_AbeforeB( clientState, state ) )
                    {
                        goon=true;
                    }
                }
            if (goon && firsttime){
                firsttime=false;
#ifdef DEBUG
                con << "Waiting for users to enter gamestate " << state << '\n';
                if (sn_Connections[1].ackPending>2)
                    con << "Ack_pending=" << sn_Connections[1].ackPending << ".\n";
#endif
            }
        }
    }
}


void gGame::SetState(unsigned short act,unsigned short next){
    state=act;
#ifdef DEBUG
    //con << "Entering gamestate " << state
    //<< ", next state: " << next << '\n';
#endif

    if (sn_GetNetState()==nSERVER){
        RequestSync();
        NetSyncIdle();
        SyncState(state);
    }

    if (stateNext!=next){
        if (sn_GetNetState()!=nCLIENT || goon==false){

            stateNext=next;

            if (sn_GetNetState()==nSERVER){
                RequestSync();
                NetSyncIdle();
            }
        }
    }
}

// from nNetwork.C
extern REAL planned_rate_control[MAXCLIENTS+2];
extern REAL sent_per_messid[100];

static REAL lastdeath=0;
static bool roundOver=false;   // flag set when the round winner is declared

static void sg_VoteMenuIdle()
{
    if ( !uMenu::MenuActive() )
    {
        se_ChatState( ePlayerNetID::ChatFlags_Menu, true );
        eVoter::VotingMenu();
        se_ChatState( ePlayerNetID::ChatFlags_Menu, false );
    }
}

void gGame::StateUpdate(){

    //	if (state==GS_CREATED)
    //		stateNext=GS_CREATE_GRID;

    while ( sg_AbeforeB( state, stateNext ) && goon )
    {
#ifdef CONNECTION_STRESS
        if ( sg_ConnectionStress )
        {
            tRandomizer & randomizer = tRandomizer::GetInstance();
            int random = randomizer.Get( 16 );
            if ( random == 0 )
            {
                sg_RequestedDisconnection = true;
                //						sn_SetNetState( nSTANDALONE );
                goon = false;
            }

            se_ChatState( ePlayerNetID::ChatFlags_Away, 1 );
        }
#endif

        // unsigned short int mes1 = 1, mes2 = 1, mes3 = 1, mes4 = 1, mes5 = 1;

        switch (state){
        case GS_DELETE_GRID:
            // sr_con.autoDisplayAtNewline=true;

            con << tOutput("$gamestate_deleting_grid");
            //				sn_ConsoleOut(sg_roundCenterMessage + "\n");
            sn_CenterMessage(sg_roundCenterMessage);

            //				for (unsigned short int mycy = 0; mycy > sg_roundConsoleMessage5.Len(); c++)

            exit_game_objects(grid);
            nNetObject::ClearAllDeleted();

            if (goon)
                SetState(GS_TRANSFER_SETTINGS,GS_CREATE_GRID);
            else
                state=GS_CREATE_GRID;
            break;
        case GS_CREATED:
        case GS_TRANSFER_SETTINGS:
            // sr_con.autoDisplayAtNewline=true;

            // transfer game settings
            if ( nCLIENT != sn_GetNetState() )
            {
                update_settings();
                ePlayerNetID::RemoveChatbots();
            }

            rViewport::Update(MAX_PLAYERS);

            // log scores before players get renamed
            ePlayerNetID::LogScoreDifferences();
            se_SaveToLadderLog(tString("NEW_ROUND\n"));
            se_sendEventNotification(tString("New Round"), tString("Starting a new round"));

            // kick spectators
            nMachine::KickSpectators();

            // rename players as per request
            if ( synced_ && sn_GetNetState() != nSERVER )
                ePlayerNetID::Update();

            // delete game objects again (in case new ones were spawned)
            exit_game_objects(grid);

            nConfItemBase::s_SendConfig(false);

            // wait extra long for the clients to delete the grid; they really need to be
            // synced this time
            sn_Sync( sg_Timeout*.4, true );

            SetState(GS_CREATE_GRID,GS_CAMERA);
            break;

        case GS_CREATE_GRID:
            // sr_con.autoDisplayAtNewline=true;

            sg_ParseMap( aParser );

            sn_Statistics();
            sg_Timestamp();

            con << tOutput("$gamestate_creating_grid");

            tAdvanceFrame();

            init_game_grid(grid, aParser);

            nNetObject::ClearAllDeleted();

            SetState(GS_CREATE_OBJECTS,GS_CAMERA);
            break;
        case GS_CREATE_OBJECTS:
            // con << "Creating objects...\n";

            lastdeath = -100;
            roundOver = false;

            // rename players as per request
            if ( synced_ )
                ePlayerNetID::Update();

            init_game_objects(grid);

            ePlayerNetID::RankingLadderLog();

            // do the first analysis of the round, now is the time to get it used to the number of teams
            Analysis( -1000 );

            s_Timestep(grid, se_GameTime(), false);
            SetState(GS_TRANSFER_OBJECTS,GS_CAMERA);

            if (sn_GetNetState() == nCLIENT && just_connected){
                sn_Sync(sg_Timeout*.1,true);
                sn_Sync(sg_Timeout*.1,true);
            }
            just_connected=false;

            init_second_pass_zones(grid, aParser);
            break;
        case GS_TRANSFER_OBJECTS:
            // con << "Transferring objects...\n";
            rITexture::LoadAll();
            // se_ResetGameTimer();
            // se_PauseGameTimer(true);


            // push all data
            if (sn_GetNetState()==nSERVER){
                bool goon=true;
                double timeout=tSysTimeFloat()+sg_Timeout*.4;
                while (goon && tSysTimeFloat()<timeout){
                    NetSyncIdle();
                    goon=false;
                    for (int i=MAXCLIENTS;i>0;i--)
                        if (sn_Connections[i].socket)
                            for (int j=sn_netObjects.Len()-1;j>=0;j--)
                                if (sn_netObjects(j) &&
                                        !sn_netObjects(j)->HasBeenTransmitted(i) &&
                                        sn_netObjects(j)->syncRequested(i))
                                    goon=true;
                }
                if (tSysTimeFloat()<timeout)
                    con << tOutput("$gamestate_done");
                else{
                    con << tOutput("$gamestate_timeout_intro");
                    for (int i=MAXCLIENTS;i>0;i--)
                        if (sn_Connections[i].socket)
                            for (int j=sn_netObjects.Len()-1;j>=0;j--)
                                if (sn_netObjects(j) && !sn_netObjects(j)->HasBeenTransmitted(i))
                                {
                                    tOutput o;
                                    tString name;
                                    sn_netObjects(j)->PrintName( name );
                                    o.SetTemplateParameter(1, i);
                                    o.SetTemplateParameter(2, j);
                                    o.SetTemplateParameter(3, name);
                                    o << "$gamestate_timeout_message";
                                    con << o;
                                }
                    con << "\n\n\n";
                }
            }

            // wait for players to ready
            if ( sn_GetNetState() == nSERVER )
                while ( ePlayerNetID::WaitToLeaveChat() )
                {
                    NetSyncIdle();
                    se_SyncGameTimer();
                }

            SetState(GS_CAMERA,GS_SYNCING);
            break;
        case GS_CAMERA:
            // con << "Setting camera position...\n";
            rITexture::LoadAll();
            // se_ResetGameTimer(-PREPARE_TIME);
            // se_PauseGameTimer(true);
            init_game_camera(grid);
            SetState(GS_SYNCING,GS_PLAY);
            break;
        case GS_SYNCING:
            // con << "Syncing timer...\n";
            rITexture::LoadAll();
            SetState(GS_PLAY,GS_PLAY);
            if (sn_GetNetState()!=nCLIENT){
                if (rounds<=0){
                    sn_ConsoleOut("$gamestate_resetnow_console");
                    StartNewMatchNow();
                    sn_CenterMessage("$gamestate_resetnow_center");
                    se_SaveToScoreFile("$gamestate_resetnow_log");
                }

                tOutput mess;
                if (rounds < sg_currentSettings->limitRounds)
                {
                    mess.SetTemplateParameter(1, rounds+1);
                    mess.SetTemplateParameter(2, sg_currentSettings->limitRounds);
                    mess << "$gamestate_newround_console";

                    if (strlen(sg_roundConsoleMessage) > 2)
                        sn_ConsoleOut(sg_roundConsoleMessage + "\n");
                }
                else
                    mess << "$gamestate_newround_goldengoal";
                sn_ConsoleOut(mess);

                se_SaveToScoreFile("$gamestate_newround_log");
            }
            //con << ePlayerNetID::Ranking();

            se_PauseGameTimer(false);
            se_SyncGameTimer();
            sr_con.fullscreen=false;
            sr_con.autoDisplayAtNewline=false;

            break;
        case GS_PLAY:
            sr_con.autoDisplayAtNewline=false;
#ifdef DEDICATED
            {
                // safe current players in a file
                cp();

                if ( sg_NumUsers() <= 0 )
                    goon = 0;

                Analysis(0);

                {
                    std::ifstream s;

                    // load contents of everytime.cfg for real
                    if ( tDirectories::Config().Open(s, "everytime.cfg" ) )
                        tConfItemBase::LoadAll(s);

                    s.close();

                    if ( tDirectories::Var().Open(s, "everytime.cfg" ) )
                        tConfItemBase::LoadAll(s);

                    // load contents of everytime.cfg from playback
                    tConfItemBase::LoadPlayback();
                }
            }
#endif
            // pings should not count as much in the between-round phase
            nPingAverager::SetWeight(1E-20);

            se_UserShowScores(false);

            // rotate, if rotate is once per round
            if (rotationtype == 1)
                rotate();
            gRotation::HandleNewRound();

            //con.autoDisplayAtNewline=true;
            sr_con.fullscreen=true;
            SetState(GS_DELETE_OBJECTS,GS_DELETE_GRID);
            break;
        case GS_DELETE_OBJECTS:

            winDeathZone_ = NULL;

            rViewport::Update(MAX_PLAYERS);
            if ( synced_ && sn_GetNetState() != nSERVER )
                ePlayerNetID::Update();

            // sr_con.autoDisplayAtNewline=true;

            //gStatistics - save all

            con << tOutput("$gamestate_deleting_objects");
            exit_game_objects(grid);
            st_ToDo( sg_VoteMenuIdle );
            nNetObject::ClearAllDeleted();
            SetState(GS_DELETE_GRID,GS_TRANSFER_SETTINGS);
            break;
        default:
            break;
        }
        if (sn_GetNetState()==nSERVER){
            NetSyncIdle();
            RequestSync();
            NetSyncIdle();
        }
        else if (sn_GetNetState()==nCLIENT){ // inform the server of our progress
            NetSyncIdle();
            nMessage *m=new nMessage(client_gs);
            m->Write(state);
            m->Send(0);
#ifdef DEBUG
            //			con << "sending gamestate " << state << '\n';
#endif
            NetSyncIdle();
        }
    }
}

// uncomment to activate respawning
// #define RESPAWN_HACK

#ifdef RESPAWN_HACK
// Respawns cycles (crude test)
static void sg_Respawn( REAL time, eGrid *grid, gArena & arena )
{
    for ( int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID *p = se_PlayerNetIDs(i);

        if ( !p->CurrentTeam() )
            continue;

        eGameObject *e=p->Object();

        if ( ( !e || !e->Alive() && e->DeathTime() < time - .5 ) && sn_GetNetState() != nCLIENT )
        {
            sg_RespawnPlayer(time, grid, &arena, p);
        }
    }
}
#endif

void sg_RespawnPlayer(eGrid *grid, gArena *arena, ePlayerNetID *p)
{
    eGameObject *e=p->Object();

    if ( ( !e || !e->Alive()) && sn_GetNetState() != nCLIENT )
    {
        eCoord pos,dir;
        if ( e )
        {
            dir = e->Direction();
            pos = e->Position();
            eWallRim::Bound( pos, 1 );
            eCoord displacement = pos - e->Position();
            if ( displacement.NormSquared() > .01 )
            {
                dir = displacement;
                dir.Normalize();
            }
        }
        else
            arena->LeastDangerousSpawnPoint()->Spawn( pos, dir );
#ifdef DEBUG
        //                std::cout << "spawning player " << pni->name << '\n';
#endif
        gCycle * cycle = new gCycle(grid, pos, dir, p, 0);
        p->ControlObject(cycle);

        sg_Timestamp();
    }
}

gArena * sg_GetArena() {
    return &Arena;
}


static REAL sg_timestepMax = .2;
static tSettingItem<REAL> sg_timestepMaxConf( "TIMESTEP_MAX", sg_timestepMax );
static int sg_timestepMaxCount = 10;
static tSettingItem<int> sg_timestepMaxCountConf( "TIMESTEP_MAX_COUNT", sg_timestepMaxCount );

void gGame::Timestep(REAL time,bool cam){
#ifdef DEBUG
    tMemMan::Check();
#endif

#ifdef RESPAWN_HACK
    sg_Respawn(time,grid,Arena);
#endif

    // chop timestep into small, managable bits
    REAL dt = time - lastTimeTimestep;
    if ( dt < 0 )
        return;
    REAL lt = lastTimeTimestep;

    // determine the number of bits
    int number_of_steps=int(fabs((dt)/sg_timestepMax));
    if (number_of_steps<1)
        number_of_steps=1;
    if ( number_of_steps > sg_timestepMaxCount )
    {
        number_of_steps = sg_timestepMaxCount;
    }

    // chop
    for (int i=1;i<=number_of_steps;i++)
    {
        REAL stepTime = lt + i * dt/number_of_steps;
        s_Timestep(grid, stepTime, cam);
    }
}

// check if team has any enemies currently
static bool sg_EnemyExists( int team )
{
    // check if the win was legitimate: at least one enemy team needs to bo online
    for ( int i = eTeam::teams.Len()-1; i>= 0; --i )
    {
        if ( i != team && eTeam::Enemies( eTeam::teams[i], eTeam::teams[team] ) )
            return true;
    }

    return false;
}

static REAL sg_winZoneRandomness = .8;
static tSettingItem< REAL > sg_winZoneSpreadConf( "WIN_ZONE_RANDOMNESS", sg_winZoneRandomness );

void gGame::Analysis(REAL time){
    if ( nCLIENT == sn_GetNetState() )
        return;

    static REAL wintimer=0;

    // only do this expensive stuff once a second
    {
        static double nextTime = -1;
        if ( tSysTimeFloat() > nextTime || time < -10 )
        {
            nextTime = tSysTimeFloat() + 1.0;
        }
        else
        {
            return;
        }
    }

    // send timeout warnings


    if (tSysTimeFloat()-startTime>10){
        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<10 && warning<6){
            tOutput o("$gamestate_tensecond_warn");
            sn_CenterMessage(o);
            sn_ConsoleOut(o);
            warning=6;
        }


        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<30 && warning<5){
            tOutput o("$gamestate_30seconds_warn");
            sn_CenterMessage(o);
            sn_ConsoleOut(o);
            warning=5;
        }

        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<60 && warning<4){
            tOutput o("$gamestate_minute_warn");
            sn_CenterMessage(o);
            sn_ConsoleOut(o);
            warning=4;
        }

        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<2*60 && warning<3){
            tOutput o("$gamestate_2minutes_warn");
            sn_ConsoleOut(o);
            warning=3;
        }

        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<5*60 && warning<2){
            tOutput o("$gamestate_5minutes_warn");
            sn_ConsoleOut(o);
            warning=2;
        }

        if ((startTime+sg_currentSettings->limitTime*60)-tSysTimeFloat()<10*60 && warning<1){
            tOutput o("$gamestate_10minutes_warn");
            sn_ConsoleOut(o);
            warning=1;
        }
    }

    // count active players
    int active = 0;
    int i;
    for (i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        ePlayerNetID *pni=se_PlayerNetIDs(i);
        if (pni->IsActive())
            active ++;
    }

    // count other statistics
    int alive_and_not_disconnected = 0;
    int alive=0;
    int ai_alive=0;
    int human_teams=0;
    int teams_alive=0;
    int last_alive=-1;
    int last_team_alive=-1;
    int last_alive_and_not_disconnected=-1;
    int humans = 0;
    int active_humans = 0;
    int ais    = 0;
    REAL deathTime=0;

    bool notyetloggedin = true;  // are all current users not yet logged in?

    for (i=eTeam::teams.Len()-1;i>=0;i--){
        eTeam *t = eTeam::teams(i);

        humans += t->NumHumanPlayers();
        ais    += t->NumAIPlayers();

        if ( t->Alive() )
        {
            teams_alive++;
            last_team_alive = i;
        }

        if ( t->NumHumanPlayers() > 0 )
        { // human players
            human_teams++;

            for (int j=t->NumPlayers()-1; j>=0; --j)
            {
                ePlayerNetID* p = t->Player(j);
                if (p->IsActive())
                    active_humans++;

                gCycle *g=dynamic_cast<gCycle *>(p->Object());
                if (g){
                    notyetloggedin = false;

                    if (g->Alive())
                    {

                        if ( p->IsHuman() )
                        {
                            alive++;
                            if (p->IsActive())
                            {
                                last_alive_and_not_disconnected=i;
                                alive_and_not_disconnected++;
                            }
                        }
                        else
                            ai_alive++;

                        last_alive=i;
                    }
                    else
                    {
                        REAL dt=g->DeathTime();
                        if (dt>deathTime)
                            deathTime=dt;
                    }
                }
                else if (p->IsAuth())
                    notyetloggedin = false;
            }
        }
        else
        {
            for (int j=t->NumPlayers()-1; j>=0; --j)
            {
                ePlayerNetID* p = t->Player(j);

                gCycle *g=dynamic_cast<gCycle *>(p->Object());
                if (g){
                    if (g->Alive())
                    {
                        ai_alive++;
                    }
                }
            }
        }
    }

#ifdef DEDICATED
    //activeHumans
    if (sg_NumUsers() <= 0)
        goon = false;
#endif


    if (last_alive_and_not_disconnected >= 0)
        last_alive = last_alive_and_not_disconnected;

    // kill all disconnected players if they are the only reason the round goes on:
    if ( alive_and_not_disconnected <= 1 && alive > alive_and_not_disconnected )
    {
        for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            gCycle *g=dynamic_cast<gCycle *>(se_PlayerNetIDs(i)->Object());
            if (g && !se_PlayerNetIDs(i)->IsActive())
            {
                g->Kill();
            }

            //			alive = alive_and_not_disconnected;
        }
    }

    // keep the game running when there are only login processes running
    if (notyetloggedin && humans > 0)
        alive++;


    static int all_alive_last=0;
    {
        int all_alive = ai_alive+alive;
        if (all_alive != all_alive_last)
        {
            all_alive_last = ai_alive+alive;
            lastdeath = time;
        }
    }

    static nVersionFeature winZone(2);

    /*
    ***************** ===================== ********************
    HACK
    THIS HAS SIMPLY BEEN DEACTIVATED.
    IT SHOULD BE REACTIVATED WITH THE NEW ZONE CODE
    ***************** ===================== ********************
    // activate instant win zone
    if ( winZone.Supported() && !bool( winDeathZone_ ) && winner == 0 && time - lastdeath > sg_currentSettings->winZoneMinLastDeath && time > sg_currentSettings->winZoneMinRoundTime )
    {
        winDeathZone_ = sg_CreateWinDeathZone( grid, Arena.GetRandomPos( sg_winZoneRandomness ) );
    }
    */

    bool holdBackNextRound = false;

    // start a new match if the number of teams changes from 1 to 2 or from 2 to 1
    {
        static int lastTeams = 0; // the number of teams when this was last called.

        // check for relevent status change, form 0 to 1 or 1 to 2 or 2 to 1 or 1 to 0 human teams.
        int humanTeamsClamp = human_teams;
        if ( humanTeamsClamp > 2 )
            humanTeamsClamp = 2;
        if ( humanTeamsClamp != lastTeams )
        {
            StartNewMatch();
            // if this is the beginning of a round, just start the match now
            if ( time < -100 )
            {
                StartNewMatchNow();
            }
            else
            {
                // start a new round and match.
                winner=-1;
                wintimer=time;
            }
        }

        lastTeams=humanTeamsClamp; // update last team count
    }

    // who is alive?
    if ( time-lastdeath < 1.0f )
    {
        holdBackNextRound = true;
    }
    else if (winner==0 && absolute_winner==0 ){
        if ( teams_alive <= 1 )
        {
            if ( sg_currentSettings->gameType!=gFREESTYLE )
            {
                if ( eTeam::teams.Len()>1 && teams_alive<=1 ){
                    winner=last_team_alive+1;
                    wintimer=time;
                }
            }
            else
            {
                if ( teams_alive < 1 )
                    winner=-1;
                wintimer=time;
            }
        }

        const char* survivor="$player_win_survivor";
        const char* messagetype = survivor;

        if ( wishWinner )
        {
            wintimer=time;
            winner = wishWinner;
            wishWinner = 0;
            messagetype	= wishWinnerMessage;
        }

        if (winner <= 0 && alive <= 0 && ai_alive <= 0 && teams_alive <= 0){
            if ( se_mainGameTimer )
                se_mainGameTimer->speed = 1;

            // emergency
            winner=-1;
            wintimer=time;
        }
        else
            if (winner){
                if ( se_mainGameTimer )
                    se_mainGameTimer->speed = 1;
                lastdeath = time;

                tOutput message;
                message << "$gamestate_winner_winner";

                if ( last_team_alive >= 0 )
                    sg_currentSettings->AutoAI( eTeam::teams( last_team_alive )->NumHumanPlayers() > 0 );


                if ( ( sg_currentSettings->scoreWin != 0 || sg_currentSettings->gameType != gFREESTYLE ) && winner > 0 )
                {
                    // check if the win was legitimate: at least one enemy team needs to be online
                    if ( sg_EnemyExists( winner-1 ) || sg_currentSettings->gameType==gFREESTYLE )
                    {
#ifdef KRAWALL_SERVER
                        // send the result to the master server
                        if (!dynamic_cast<gAIPlayer*>(eTeam::teams(winner-1)))
                        {
                            tArray<tString> players;
                            for (int i = eTeam::teams.Len()-1; i>=0; i--)
                                if (i != winner-1 &&
                                        eTeam::teams(i)->Alive() &&
                                        eTeam::teams(i)->IsHuman() )
                                {
                                    players[players.Len()] = eTeam::teams(i)->Name();
                                }

                            players[players.Len()] = eTeam::teams(winner-1)->Name();
                            if (players.Len() > 1)
                                nKrawall::ServerRoundEnd(&players(0), players.Len());
                        }
#endif

                        // scoring
                        eTeam::teams[winner-1]->AddScore
                        (sg_currentSettings->scoreWin,messagetype, tOutput());
                        if (!sg_singlePlayer && eTeam::teams(winner-1)->NumHumanPlayers() >= 1 && gStats &&
                                eTeam::teams(winner-1)->NumPlayers() >= 1 && eTeam::teams[winner-1]->NumHumanPlayers() > 0)
                        {
                            //gStatistics - won rounds add
                            for (int i = eTeam::teams[winner-1]->NumPlayers() - 1; i>=0; --i) //count down in case of mid count changes?
                            {
                                if (eTeam::teams[winner-1]->Player(i)->IsHuman())
                                {
                                    gStats->won_rounds->add(eTeam::teams[winner-1]->Player(i)->GetName(), 1);
                                }
                            }
                            //gStatistics - ladder add
                        }

                        // print winning message
                        tOutput message;
                        message << "$gamestate_winner_winner";
                        message << eTeam::teams[winner-1]->Name();

                        m_Mixer->PushButton(ROUND_WINNER);

                        sn_CenterMessage(message);
                        message << '\n';
                        se_SaveToScoreFile(message);

                        tString ladderLog;
                        ladderLog << "ROUND_WINNER " << ePlayerNetID::FilterName( eTeam::teams[winner-1]->Name() ) << "\n";
                        se_SaveToLadderLog( ladderLog );
                        tString notificationMessage( ePlayerNetID::FilterName( eTeam::teams[winner-1]->Name() ) );
                        notificationMessage << " has won the round";
                        se_sendEventNotification(tString("Round winner"), notificationMessage);
                    }
                }
                //gStatistics - highscore check
                if (sg_singlePlayer && gStats && se_PlayerNetIDs.Len() > 0 && se_PlayerNetIDs(0)->IsHuman())
                {
                    gStats->highscores->greater(se_PlayerNetIDs(0)->GetName(), se_PlayerNetIDs(0)->Score());
                }
                winner = -1;
            }
    }

    int winnerExtraRound = ( winner != 0 || alive == 0 ) ? 1 : 0;


    // do round end stuff one second after a winner was declared
    if ( winner && !roundOver && time-lastdeath >= 2.0f )
    {
        roundOver = true;

        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            eGameObject * e = gameObjects(i);
            if ( e )
            {
                e->OnRoundEnd();
            }
        }
    }

    // analyze the ranking list
    if ( time-lastdeath < 2.0f )
    {
        holdBackNextRound = true;
    }
    else if (absolute_winner==0 &&
             sn_GetNetState()!=nCLIENT && se_PlayerNetIDs.Len()){
        ePlayerNetID::SortByScore();
        eTeam::SortByScore();
        if ( eTeam::teams.Len() > 0 )
            if (eTeam::teams.Len() <= 1
                    || ( eTeam::teams(0)->Score() > eTeam::teams(1)->Score() && sg_EnemyExists(0))){

                // only then we can have a true winner:
                if (eTeam::teams(0)->Score() >= sg_currentSettings->limitScore ||		// the score limit must be hit
                        rounds + winnerExtraRound >= sg_currentSettings->limitRounds ||     // or the round limit
                        tSysTimeFloat()>=startTime+sg_currentSettings->limitTime*60 ||      // or the time limit
                        (active <= 1 && eTeam::teams.Len() > 1)								// or all but one players must have disconnected.
                   )
                {
                    bool declareChampion = true;
                    if ( winner!=0 && wintimer > time - ( sg_currentSettings->finishType==gFINISH_EXPRESS ? 2.0f : 4.0 ) )
                    {
                        declareChampion = false;
                        holdBackNextRound= true;
                    }

                    if ( declareChampion )
                    {
                        lastdeath = time + ( sg_currentSettings->finishType==gFINISH_EXPRESS ? 2.0f : 6.0f );

                        {
                            tOutput message;
                            message.SetTemplateParameter(1, eTeam::teams[0]->Name() );
                            message << "$gamestate_champ_center";
                            sn_CenterMessage(message);

                            m_Mixer->PushButton(MATCH_WINNER);
                        }

                        tOutput message;
                        tColoredString name;
                        name << eTeam::teams[0]->Name();
                        name << tColoredString::ColorString(1,1,1);

                        tString ladderLog;
                        ladderLog << "MATCH_WINNER " << ePlayerNetID::FilterName( eTeam::teams[0]->Name() ) << "\n";
                        se_SaveToLadderLog( ladderLog );
                        tString notificationMessage( ePlayerNetID::FilterName( eTeam::teams[0]->Name() ) );
                        notificationMessage << " has won the match";
                        se_sendEventNotification(tString("Match winner"), notificationMessage);


                        message.SetTemplateParameter(1, name);
                        message << "$gamestate_champ_console";

                        if (eTeam::teams(0)->Score() >=sg_currentSettings->limitScore)
                        {
                            message.SetTemplateParameter(1, eTeam::teams(0)->Score());
                            message.SetTemplateParameter(2, sg_currentSettings->limitScore);
                            message << "$gamestate_champ_scorehit";
                        }
                        else if ( tSysTimeFloat()>=startTime+sg_currentSettings->limitTime*60)
                        {
                            message.SetTemplateParameter(1, sg_currentSettings->limitTime);
                            message << "$gamestate_champ_timehit";
                        }
                        else
                        {
                            message.SetTemplateParameter(1, rounds + 1);
                            message << "$gamestate_champ_default";
                        }

                        se_SaveToScoreFile(message);
                        se_SaveToScoreFile("$gamestate_champ_finalscores");
                        se_SaveToScoreFile(eTeam::Ranking( -1, false ));
                        se_SaveToScoreFile(ePlayerNetID::Ranking( -1, false ));
                        se_SaveToScoreFile(sg_GetCurrentTime( "Time: %Y/%m/%d %H:%M:%S\n" ));
                        se_SaveToScoreFile("\n\n");

                        eTeam* winningTeam = eTeam::teams(0);
                        if (!sg_singlePlayer && winningTeam->NumHumanPlayers() > 0 && winningTeam->NumHumanPlayers() > 0 && gStats)
                        {
                            //gStatistics - won matches add
                            for (int i = winningTeam->NumPlayers() - 1; i>=0; --i) //count down in case of mid count changes?
                            {
                                if (winningTeam->Player(i)->IsHuman())
                                {
                                    gStats->won_matches->add(winningTeam->Player(i)->GetName(), 1);
                                }
                            }
                        }

                        sn_ConsoleOut(message);

                        m_Mixer->PushButton(MATCH_WINNER);

                        wintimer=time;
                        absolute_winner=1;

                        if ( se_mainGameTimer )
                            se_mainGameTimer->speed = 1;

                        //check for map rotation, new match...
                        if (rotationtype == 2)
                            rotate();

                        gRotation::HandleNewMatch();

                        StartNewMatch();
                    }
                }
            }
    }


    // time to wait after last death til we start a new round
    REAL fintime=6;

    if (sg_currentSettings->finishType==gFINISH_EXPRESS)
        fintime=2.5;

    if (( ( winner || absolute_winner ) && wintimer+fintime < time)
            || (((alive==0 && eTeam::teams.Len()>=
#ifndef DEDICATED
                  1
#else
                  0
#endif
                 )) && time-lastdeath >fintime-2
#ifndef DEDICATED
                && humans > 0
#endif
               )){
#ifdef DEBUG
        //con << "winner=" << winner << ' ';
        //con << "wintimer=" << wintimer << ' ';
        //con << "time=" << time << ' ';
        //con << "dt=" << deathTime << '\n';
#endif
        if (!holdBackNextRound &&
                (
                    ( sg_currentSettings->finishType==gFINISH_IMMEDIATELY ||
                      sg_currentSettings->finishType==gFINISH_EXPRESS ||
                      absolute_winner || ( teams_alive <= 1 && time-lastdeath>4.0f ) ) ||
                    ( winner && time - wintimer> 4.0f )
                )
           )
        {
            rounds++;
            SetState(GS_PLAY,GS_DELETE_OBJECTS);
        }

        if ( !winner && !absolute_winner && sg_currentSettings->finishType==gFINISH_SPEEDUP && se_mainGameTimer)
        {
            se_mainGameTimer->speed*=1.01;
            if (se_mainGameTimer->speed>16)
                se_mainGameTimer->speed=16;
        }

    }
}

void rotate()
{
    if ( sg_mapRotation.Size() > 0 )
    {
        conf_mapfile.Set( sg_mapRotation.Current() );
        sg_mapRotation.Rotate();
    }

    if ( sg_configRotation.Size() > 0 )
    {
        st_Include( sg_configRotation.Current() );
        sg_configRotation.Rotate();
    }
}

void gGame::StartNewMatch(){
    //gStatistics - highscore check
    if (sg_singlePlayer && gStats && se_PlayerNetIDs.Len() > 0 && se_PlayerNetIDs(0)->IsHuman())
    {
        gStats->highscores->greater(se_PlayerNetIDs(0)->GetName(), se_PlayerNetIDs(0)->Score());
    }

    rounds=-100;
}

void gGame::StartNewMatchNow(){
    if ( rounds != 0 )
    {
        se_SaveToLadderLog(tString("NEW_MATCH\n"));
        se_sendEventNotification(tString("New match"), tString("Starting a new match"));
    }


    rounds=0;
    warning=0;
    startTime=tSysTimeFloat()-10;

    ePlayerNetID::ThrowOutDisconnected();
    ePlayerNetID::ResetScore();
}

#ifdef DEBUG
extern bool debug_grid;
// from display.cpp
bool simplify_grid=1;
#endif

#ifndef DEDICATED
// mouse grab toggling
static uActionGlobal togglemousegrab( "TOGGLE_MOUSEGRAB" );
static bool togglemousegrab_func(REAL x){
    if (x>0){
        su_mouseGrab = !su_mouseGrab;
    }

    return true;
}
static uActionGlobalFunc togglemousegrab_action(&togglemousegrab,&togglemousegrab_func, true );

// pause game
static uActionGlobal pausegame( "PAUSE_GAME" );
static bool pausegame_func(REAL x){
    static bool paused=false;

    if (x>0){
        paused=!paused;
        se_PauseGameTimer(paused);
    }

    return true;
}
static uActionGlobalFunc pausegame_action(&pausegame,&pausegame_func, true );

// texture reloading
static uActionGlobal reloadtextures( "RELOAD_TEXTURES" );
static bool reloadtextures_func(REAL x){
    if (x>0){
        rITexture::UnloadAll();
    }

    return true;
}
static uActionGlobalFunc reloadtextures_action(&reloadtextures,&reloadtextures_func, true );

// menu
static uActionGlobal ingamemenu( "INGAME_MENU" );
static bool ingamemenu_func(REAL x){
    if (x>0){
        st_ToDo(&ingame_menu);
    }

    return true;
}
static uActionGlobalFunc ingamemenu_action(&ingamemenu,&ingamemenu_func, true );
#endif // dedicated

bool gGame::GameLoop(bool input){
    nNetState netstate = sn_GetNetState();

#ifdef DEBUG
    grid->Check();
    if (simplify_grid)
#endif
        grid->SimplifyAll(10);

#ifdef DEBUG
    grid->Check();
#endif

    //if (std::cin.good() && !std::cin.underflow())
    //tConfItemBase::LoadAll(std::cin);
    // network syncing
    REAL gtime=0;
    REAL time=0;
    se_SyncGameTimer();
    if (state==GS_PLAY){
        if ( netstate != sn_GetNetState() )
        {
            return false;
        }
        gtime=se_GameTime();
        time=gtime;

        if (sn_GetNetState()==nSTANDALONE && sg_IngameMenu)
            se_PauseGameTimer(true);

        static int lastcountdown=0;
        int cd=int(floor(-time))+1;
        if (cd>=0 && cd<PREPARE_TIME && cd!=lastcountdown && se_mainGameTimer && se_mainGameTimer->IsSynced() ){
            lastcountdown=cd;
            tString s;
            s << cd;

            switch (cd) {
            case 3:
                m_Mixer->PushButton(ANNOUNCER_3);
                break;
            case 2:
                m_Mixer->PushButton(ANNOUNCER_2);
                break;
            case 1:
                m_Mixer->PushButton(ANNOUNCER_1);
                break;
            case 0:
                m_Mixer->PushButton(ANNOUNCER_GO);
                break;
            }
            con.CenterDisplay(s,0);
        }
    }
    //con << sg_netPlayerWalls.Len() << '\n';

#ifndef DEDICATED
    if (input){
        su_HandleDelayedEvents();

        SDL_Event tEvent;

        uInputProcessGuard inputProcessGuard;
        while (su_GetSDLInput(tEvent,time)){
            if (time>gtime) time=gtime;
            if (time>lastTime_gameloop){
                Timestep(time);
                lastTime_gameloop=time;
            }


            if (!su_HandleEvent(tEvent, false))
                switch (tEvent.type){
                case SDL_MOUSEBUTTONDOWN:
                    break;
                case SDL_KEYDOWN:
                    switch (tEvent.key.keysym.sym){

                    case(27):
                                    //                                case('q'):
                                    st_ToDo(&ingame_menu);
                        break;
#ifdef DEBUG
                    case('d'):
                                    debug_grid=!debug_grid;
                        break;

                    case('a'):
                                    simplify_grid=!simplify_grid;
                        break;

                    case('l'):
                                    sr_glOut=!sr_glOut;
                        break;

                    case('b'):
                                    st_Breakpoint();
                        break;

                    case('t'):
                                    think=!think;
                        break;
                        /* #ifndef WIN32
                           case('f'):
                           fullscreen=(!fullscreen);
                           con << fullscreen << "x!\n";
                           XMesaSetFXmode(fullscreen ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW);
                           break;
                           #endif */
#endif

                    default:
                        break;
                    }
                    break;

                default:
                    break;
                }
        }

        su_InputSync();
    }
#endif

    if ( netstate != sn_GetNetState() )
{
        return false;
    }

    bool synced = se_mainGameTimer && ( se_mainGameTimer->IsSynced() || ( stateNext >= GS_DELETE_OBJECTS || stateNext <= GS_CREATE_GRID ) );

    if  (!synced)
    {
        // see if there is a game object owned by this client, if so, declare emergency sync
        for (int pi = MAX_PLAYERS-1; pi >= 0; --pi )
        {
            ePlayer * p = ePlayer::PlayerConfig(pi);
            if ( !p )
                break;
            ePlayerNetID * np = p->netPlayer;
            if ( !np )
                break;
            if ( np->Object() )
            {
                synced = true;
                break;
            }
        }
    }
    else if ( !synced_ )
    {
        // synced finally. Send our player info over so we can join the game.
        ePlayerNetID::Update();
        synced_ = true;
    }

    static float lastTime = -1;

    if (sg_gameTimeInterval >= 0 && (gtime >= lastTime + sg_gameTimeInterval || (gtime < lastTime && gtime >= 0))) {
        tOutput out;
        out << "GAME_TIME " << gtime << '\n';
        se_SaveToLadderLog(out);
        lastTime = gtime;
    }

    if (state==GS_PLAY){
        if (gtime<0 && gtime>-PREPARE_TIME+.3)
            eCamera::s_Timestep(grid, gtime);
        else{
            // keep ping weight high while playing, that gives the best meassurements
            nPingAverager::SetWeight(1);

            if (gtime>=lastTime_gameloop){

#ifdef CONNECTION_STRESS
                if ( sg_ConnectionStress )
                {
                    tRandomizer & randomizer = tRandomizer::GetInstance();
                    int random = randomizer.Get( 1000 );
                    if ( random == 0 )
                    {
                        sg_RequestedDisconnection = true;
                        //						sn_SetNetState( nSTANDALONE );
                        goon = false;
                    }
                }
#endif


                Timestep(gtime,true);
                lastTime_gameloop=gtime;
            }
            lastTime_gameloop=gtime;
        }
        //    else if (lastTime_gameloop>gtime+10)
        // lastTime_gameloop=gtime;



        if (sn_GetNetState()!=nCLIENT)
        {
            // simulate IAs
            for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            {
                gAIPlayer *ai = dynamic_cast<gAIPlayer*>(se_PlayerNetIDs(i));
                if (ai && think)
                    ai->Timestep(gtime);
            }

            Analysis(gtime);

            // wait for chatting players
            if ( sn_GetNetState()==nSERVER && gtime < sg_lastChatBreakTime + 1 )
                se_PauseGameTimer( gtime < sg_lastChatBreakTime && ePlayerNetID::WaitToLeaveChat() );
        }

        if ( gtime<=-PREPARE_TIME+.5 || !goon || !synced )
        {
#ifndef DEDICATED
            if (input)
            {
                if ( !synced )
                {
                    con.CenterDisplay(tString(tOutput("$network_login_sync")),0);
                }

                if ( sr_glOut )
                    rSysDep::ClearGL();
            }

            if ( input )
                rSysDep::SwapGL();
#endif
        }
        else
            Render(grid, gtime, input);

        if ( netstate != sn_GetNetState() )
        {
            return false;
        }

        return goon;
    }
    else{
        // between rounds, assume we're synced
        //if ( !synced_ && sn_GetNetState() != nSERVER )
        //    ePlayerNetID::Update();
        // synced_ = true;

#ifndef DEDICATED
        if (input)
        {
            if (sr_glOut)
                rSysDep::ClearGL();
            rSysDep::SwapGL();
        }
#endif
        tDelay( 10000 );
        return goon;
    }
}


bool gGame::GridIsReady(int client){
    return HasBeenTransmitted(client) &&
           (client_gamestate[client]>=GS_TRANSFER_OBJECTS &&
            client_gamestate[client]<=GS_DELETE_OBJECTS);
}

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

bool GameLoop(bool input=true){
    /*
      int oldflags = fcntl (fileno(stdin), F_GETFD, 0);
      if (oldflags<0)
      std::cout << errno << '\n';
       
      if (fcntl(fileno(stdin), F_SETFL, oldflags | O_NONBLOCK)<0)
      std::cout << errno << '\n';
      
    //if (std::cin && std::cin.good() && !std::cin.eof() && !std::cin.fail() && !std::cin.bad()){
    //if (std::cin.rdbuf()->overflow()!=EOF){
    //if (std::cin.rdbuf()->in_avail()>0){

    char c[10];
    int in=0;
    in=read (fileno(stdin),c,9);
    for (int i=0;i<in;i++)
    std::cout << c;
     */

    // see if pings have dramatically changed
    if (sn_GetNetState()==nSERVER){
        REAL t=tSysTimeFloat();
        for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--){
            ePlayerNetID *p=se_PlayerNetIDs(i);

            // get average and variance (increased a bit) of this player's ping
            nPingAverager const & averager = sn_Connections[p->Owner()].ping;
            REAL realping = averager.GetPing();
            REAL pingvariance = averager.GetSlowAverager().GetDataVariance() + realping * realping * .01;

            // deviation of real ping and synced ping
            REAL deviation = realping - p->ping;

            // measured against the variance, this player's ping deviates this much
            // from the communicated value
            REAL relativedeviation = deviation * deviation / pingvariance;

            if (0 == p->Owner())
                realping = 0;

            if ( ( t - p->lastSync - 5 ) * relativedeviation > 5 ){
                // if yes, send a message about it
                p->ping = realping;
                p->RequestSync(false);
            }
        }
    }

    bool goon=false;
    if (sg_currentGame){
        tControlledPTR< nNetObject > keep( sg_currentGame );
        sg_currentGame->StateUpdate();
        goon=!uMenu::quickexit && bool(sg_currentGame) && sg_currentGame->GameLoop(input);
    }
    return goon;
}

void gameloop_idle()
{
    se_UserShowScores( false );
    sg_Receive();
    nNetObject::SyncAll();
    sn_SendPlanned();
    GameLoop(false);
}

static void sg_EnterGameCleanup();

void sg_EnterGameCore( nNetState enter_state ){
    sg_RequestedDisconnection = false;

    sr_con.SetHeight(7);

    //  exit_game_objects(grid);
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->SetMode(GRID_TRACK);

    // enter single player settings
    if ( sn_GetNetState() != nCLIENT )
    {
        sg_currentSettings = &singlePlayer;
        sg_copySettings();

        // initiate rotation
        rotate();
        gRotation::HandleNewRound();
        gRotation::HandleNewMatch();
    }

    //gStatistics - original load call

    uMenu::SetIdle(gameloop_idle);
    sr_con.autoDisplayAtSwap=true;
    bool goon=true;
    while (bool(sg_currentGame) && goon && sn_GetNetState()==enter_state){
#ifdef DEDICATED // read input
        sr_Read_stdin();

        if ( sn_BasicNetworkSystem.Select( 1.0 / ( sg_dedicatedFPSIdleFactor * sg_dedicatedFPS )  ) )
        {
            // new network data arrived, do the most urgent work now
            tAdvanceFrame();
            sg_Receive();
            se_SyncGameTimer();
            REAL time=se_GameTime();
            sg_currentGame->StateUpdate();
            if ( time > 0 )
            {
                // only simulate the objects that have pending events to execute
                eGameObject::s_Timestep(sg_currentGame->Grid(), time, 1E+10 );

                // send out updates immediately
                nNetObject::SyncAll();
                tAdvanceFrame();
                sn_SendPlanned();
            }
        }
#endif

        // do the regular simulation
        tAdvanceFrame();

        sg_Receive();

        goon=GameLoop();

        nNetObject::SyncAll();
        tAdvanceFrame();
        sn_SendPlanned();

        st_DoToDo();
    }

    sg_EnterGameCleanup();
}

void sg_EnterGameCleanup()
{
    //gStatistics - save high scores

    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->SetMode(GUI_TRACK);

    sn_SetNetState( nSTANDALONE );

    // delete all AI players
    gAIPlayer::ClearAll();
    // gAIPlayer::SetNumberOfAIs(0, 0, 0, -1);
    // gAITeam::BalanceWithAIs( false );
    // gAIPlayer::ClearAll();

    gLogo::SetDisplayed(true);
    uMenu::SetIdle(NULL);
    sr_con.autoDisplayAtSwap=false;
    sr_con.fullscreen=false;

    //  exit_game_objects(grid);
    nNetObject::ClearAll();
    ePlayerNetID::ClearAll();
    sg_currentGame = NULL;
    uMenu::exitToMain = false;
}

void sg_EnterGame( nNetState enter_state )
{
    try
    {
        // enter the game
        sg_EnterGameCore( enter_state );
    }
    catch (tException const & e)
    {
        // cleanup
        sg_EnterGameCleanup();

        // inform user of generic errors
        tConsole::Message( e.GetName(), e.GetDescription(), 120000 );
    }
    // again: VC6 does not catch tGenericException with above statement
#ifdef _MSC_VER
#pragma warning ( disable : 4286 )
    catch (tGenericException const & e)
    {
        // cleanup
        sg_EnterGameCleanup();

        // inform user of generic errors
        tConsole::Message( e.GetName(), e.GetDescription(), 120000 );
    }
#endif

    // bounce teams
    for ( int i = eTeam::teams.Len()-1; i>=0; --i )
    {
        tJUST_CONTROLLED_PTR< eTeam > t = eTeam::teams(i);
    }
}

bool GridIsReady(int c){
    return bool(sg_currentGame) && sg_currentGame->GridIsReady(c);
}


// avoid transfer of game objects if the grid is not yes constructed
static bool notrans(){
    return !GridIsReady(eTransferInhibitor::User());
}

static eTransferInhibitor inh(&notrans);

// are we active?
void Activate(bool act){
#ifdef DEBUG
    return;
#endif
    sr_Activate( act );

    if (!tRecorder::IsRunning() )
    {
        if (sn_GetNetState()==nSTANDALONE)
        {
            se_PauseGameTimer(!act);
        }

        se_ChatState( ePlayerNetID::ChatFlags_Away, !act);
    }
}

// ************************************
// full screen messages from the server
// ************************************

static nVersionFeature sg_fullscreenMessages(14);

static void sg_FullscreenIdle()
{
    tAdvanceFrame();
    se_SyncGameTimer();
    gGame::NetSync();
    if ( sg_currentGame )
        sg_currentGame->StateUpdate();
}

void sg_ClientFullscreenMessage( tOutput const & title, tOutput const & message, REAL timeout ){
    // keep syncing the network
    rPerFrameTask idle( sg_FullscreenIdle );

    // put players into idle mode
    ePlayerNetID::SpectateAll();
    se_ChatState( ePlayerNetID::ChatFlags_Menu, true );

    // show message
    uMenu::Message( title, message, timeout );

    // and print it to the console
#ifndef DEDICATED
    con <<  title << "\n" << message << "\n";
#endif

    // get players out of idle mode again
    ePlayerNetID::SpectateAll(false);
    se_ChatState( ePlayerNetID::ChatFlags_Menu, false );
}

static tString sg_fullscreenMessageTitle;
static tString sg_fullscreenMessageMessage;
static REAL sg_fullscreenMessageTimeout;
static void sg_TodoClientFullscreenMessage()
{
    sg_ClientFullscreenMessage( sg_fullscreenMessageTitle, sg_fullscreenMessageMessage, sg_fullscreenMessageTimeout );
}

static void sg_ClientFullscreenMessage(nMessage &m){
    if (sn_GetNetState()!=nSERVER){
        sg_fullscreenMessageTimeout = 60;

        m >> sg_fullscreenMessageTitle;
        m >> sg_fullscreenMessageMessage;
        m >> sg_fullscreenMessageTimeout;

        st_ToDo( sg_TodoClientFullscreenMessage );
    }
}

static nDescriptor sg_clientFullscreenMessage(312,sg_ClientFullscreenMessage,"client_fsm");

// causes the connected clients to break and print a fullscreen message
void sg_FullscreenMessage(tOutput const & title, tOutput const & message, REAL timeout, int client){
    tJUST_CONTROLLED_PTR< nMessage > m=new nMessage(sg_clientFullscreenMessage);
    *m << title;
    *m << message;
    *m << timeout;

    tString complete( title );
    complete << "\n" << message << "\n";

    if (client <= 0){
        if ( sg_fullscreenMessages.Supported() )
            m->BroadCast();
        else
        {
            for (int c = MAXCLIENTS; c > 0; --c)
            {
                if ( sn_Connections[c].socket )
                {
                    if ( sg_fullscreenMessages.Supported(c) )
                        m->Send(c);
                    else
                        sn_ConsoleOut(complete, c);
                }
            }
        }

        // display the message locally, waiting for the clients to have seen it
        {
            // stop the game
            bool paused = se_mainGameTimer && se_mainGameTimer->speed < .0001;
            se_PauseGameTimer(true);
            gGame::NetSyncIdle();

            REAL waitTo = tSysTimeFloat() + timeout;
            REAL waitToMin = tSysTimeFloat() + 1.0;
            sg_ClientFullscreenMessage( title, message, timeout );

            // wait for players to see it
            bool goon = true;
            while ( goon && waitTo > tSysTimeFloat() )
            {
                sg_FullscreenIdle();
                gameloop_idle();
                if ( se_GameTime() > sg_lastChatBreakTime )
                    se_PauseGameTimer(true);

                // give the clients a second to enter chat state
                if ( tSysTimeFloat() > waitToMin )
                {
                    goon = false;
                    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
                    {
                        ePlayerNetID* player = se_PlayerNetIDs(i);
                        if ( player->IsChatting() )
                            goon = true;
                    }
                }
            }

            // continue the game
            se_PauseGameTimer(paused);
            gGame::NetSyncIdle();
        }
    }
    else
    {
        if ( sg_fullscreenMessages.Supported(client) )
            m->Send(client);
        else
            sn_ConsoleOut(complete, client);
    }
}

static void sg_FullscreenMessageConf(std::istream &s)
{
    // read the timeout
    REAL timeout;
    s >> timeout;

    if ( !s.good() || s.eof() )
    {
        con << "Usage: FULLSCREEN_MESSAGE <timeout> <message>\n";
        return;
    }

    // read the message
    tString message;
    message.ReadLine( s, true );

    message += "\n";

    // display it
    sg_FullscreenMessage( "$fullscreen_message_title", message , timeout );
}

static tConfItemFunc sg_fullscreenMessageConf("FULLSCREEN_MESSAGE",&sg_FullscreenMessageConf);

// **************
// Login callback
// **************

// message of day presented to clients logging in
tString sg_greeting("");
static tConfItemLine a_mod("MESSAGE_OF_DAY",sg_greeting);

tString sg_greetingTitle("");
static tConfItemLine a_tod("TITLE_OF_DAY",sg_greetingTitle);

REAL sg_greetingTimeout=60;
static tSettingItem< REAL > a_modt("MESSAGE_OF_DAY_TIMEOUT",sg_greetingTimeout);

static void LoginCallback(){
    client_gamestate[nCallbackLoginLogout::User()]=0;
    if ( nCallbackLoginLogout::Login() && nCallbackLoginLogout::User() > 0 )
    {
        if ( sg_greeting.Len()>1 )
        {
            if ( sg_greetingTitle.Len() <= 1 )
                sg_greetingTitle = "Server message";

            sg_FullscreenMessage( sg_greetingTitle, sg_greeting, sg_greetingTimeout, nCallbackLoginLogout::User() );
        }
    }
}

void oldFortressAutomaticAssignment(zZone *zone, zMonitorPtr monitor);

void init_second_pass_zones(eGrid *grid, gParser *parser)
{
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int i=gameObjects.Len()-1;i>=0;i--)
    {
        zZone *zone=dynamic_cast<zZone *>(gameObjects(i));

        if (zone && zone->getOldFortressAutomaticAssignmentBehavior() == true)
        {
            zMonitorPtr monitor = parser->getMonitor(zone->getName());
            oldFortressAutomaticAssignment(zone, monitor);
        }
    }
}

void oldFortressAutomaticAssignment(zZone *zone, zMonitorPtr monitor)
{
    if (zone->getOldFortressAutomaticAssignmentBehavior() == true)
    {      // Recreate the automatic assignment of zones to teams as found in Arthemis

        // Do only once
        zone->setOldFortressAutomaticAssignmentBehavior( false );

        //      teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = zone->Grid()->GameObjects();
        gCycle * closest = NULL;
        REAL closestDistance = 0;
        eTeam * team;
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

            if (other )
            {
                // eTeam * otherTeam = other->Player()->CurrentTeam();
                eCoord otherpos = other->Position() - zone->GetPosition();
                REAL distance = otherpos.NormSquared();
                if ( !closest || distance < closestDistance )
                {
                    // check whether other zones are already registered to that team
                    /*
                    zZone * farthest = NULL;
                    int count = 0;
                    if ( sg_baseZonesPerTeam > 0 )
                      CountZonesOfTeam( Grid(), otherTeam, count, farthest );

                    // only set team if not too many closer other zones are registered
                    //		  if ( sg_baseZonesPerTeam == 0 || count < sg_baseZonesPerTeam || farthest->teamDistance_ > distance )
                    */
                    {
                        closest = other;
                        closestDistance = distance;
                    }
                }
            }
        }

        if ( closest )
        {
            // take over team and color
            team = closest->Player()->CurrentTeam();
            rColor teamColor;
            teamColor.r_ = team->R()/15.0;
            teamColor.g_ = team->G()/15.0;
            teamColor.b_ = team->B()/15.0;
            zone->getShape()->setColor(teamColor);
            // teamDistance_ = closestDistance;

            { // Build the monitor influence objects for this zone


                // Load the associated monitor
                // Its name is the same as the zone, as both are generated in the same way
                // through a series of #


                //
                zEffectGroupPtr currentZoneEffect;

                // The part for the defender
                zValidatorPtr validator;
                if (sg_singlePlayer) {
                    // In single player mode, all the players teams have the ID 0, making team logic fail
                    gVectorExtra< nNetObjectID > playerOwners;
                    playerOwners.push_back(closest->Player()->ID());
                    currentZoneEffect = zEffectGroupPtr(new zEffectGroup(playerOwners, gVectorExtra< nNetObjectID >()));
                    validator = zValidatorPtr( new zValidatorOwner(_ignore, _ignore) );
                }
                else {
                    gVectorExtra< nNetObjectID > teamOwners;
                    teamOwners.push_back(team->ID());
                    currentZoneEffect = zEffectGroupPtr(new zEffectGroup(gVectorExtra< nNetObjectID >(), teamOwners));
                    validator = zValidatorPtr( new zValidatorOwnerTeam(_ignore, _ignore) );
                }

                zMonitorInfluencePtr inflDefender = zMonitorInfluencePtr(new zMonitorInfluence( monitor ));

                tPolynomial<nMessage> tpInfluenceSlide(2);
                tpInfluenceSlide[0] = -1.0 * sg_defendRate;
                inflDefender->setInfluenceSlide( tpInfluenceSlide );
                //                inflDefender->setInfluenceSlide( tFunction(-1.0 * sg_defendRate, 0.0) );

                // Store all the objects
                validator->addMonitorInfluence( inflDefender );
                currentZoneEffect->addValidator( validator );

                // The part for the attaquer
                if (sg_singlePlayer) {
                    // In single player mode, all the players teams have the ID 0, making team logic fail
                    validator = zValidatorPtr( new zValidatorAllButOwner(_ignore, _ignore) );
                }
                else {
                    validator = zValidatorPtr( new zValidatorAllButTeamOwner(_ignore, _ignore) );
                }

                zMonitorInfluencePtr inflAttaquer = zMonitorInfluencePtr(new zMonitorInfluence( monitor ));

                tpInfluenceSlide[0] = -1.0 * sg_defendRate;
                inflAttaquer->setInfluenceSlide( tpInfluenceSlide );
                //                inflAttaquer->setInfluenceSlide(  tFunction(sg_conquestRate, 0.0) );
                // Store all the objects
                validator->addMonitorInfluence( inflAttaquer );
                currentZoneEffect->addValidator( validator );



                zone->addEffectGroupInside( currentZoneEffect );

            }

            zone->RequestSync();
        }

        /*
        // if this zone does not belong to a team, discard it.
        if ( !team )
          {
        return true;
          }

        // check other zones owned by the same team. Discard the one farthest away
        // if the max count is exceeded
        if ( team && sg_baseZonesPerTeam > 0 )
          {
        gBaseZoneHack * farthest = 0;
        int count = 0;
        CountZonesOfTeam( Grid(), team, count, farthest );

        // discard team of farthest zone
        if ( count > sg_baseZonesPerTeam )
        farthest->team = NULL;
          }
        */
    }
}


static nCallbackLoginLogout lc(LoginCallback);


