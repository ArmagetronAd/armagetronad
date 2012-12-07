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


#include "gStuff.h"
#include "eSound.h"
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
#include "rModel.h"
#include "uInput.h"
#include "ePlayer.h"
#include "gArena.h"
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
#include "gWinZone.h"
#include "eVoter.h"
#include "tRecorder.h"
#include "gSvgOutput.h"
#include "rScreen.h"

//HACK RACE
#include "gRace.h"

#include "gParser.h"
#include "tResourceManager.h"
#include "nAuthentication.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <ctype.h>
#include <time.h>
#include <map>

#include "gRotation.h"
#include "nSocket.h"

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

static tString mapuri("0");
static nSettingItem<tString> conf_mapuri("MAP_URI",mapuri);

#define DEFAULT_MAP "Anonymous/polygon/regular/square-1.0.1.aamap.xml"
tString mapfile(DEFAULT_MAP);

static bool sg_waitForExternalScript = false;
static tSettingItem< bool > sg_waitForExternalScriptConf( "WAIT_FOR_EXTERNAL_SCRIPT", sg_waitForExternalScript );

static REAL sg_waitForExternalScriptTimeout = 3;
static tSettingItem<REAL> sg_waitForExternalScriptTimeoutConf( "WAIT_FOR_EXTERNAL_SCRIPT_TIMEOUT", sg_waitForExternalScriptTimeout );

static REAL sg_svgOutputFreq = -1;
static tSettingItem<REAL> sg_svgOutputFreqConf( "SVG_OUTPUT_TIMING", sg_svgOutputFreq );

static bool sg_svgOutputScoreDifferences = false;
static tSettingItem<bool> sg_svgOutputScoreDifferencesConf( "SVG_OUTPUT_LOG_SCORE_DIFFERENCES", sg_svgOutputScoreDifferences );

// time in sec before player positioning starts
static REAL sg_playerPositioningStartTime = 5.0;
static tSettingItem<REAL> sg_playerPositioningStartTimeConf( "TACTICAL_POSITION_START_TIME", sg_playerPositioningStartTime );

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
    tString Current() const
    {
        tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

        return items_[current_];
    }

    // rotates
    void OrderedRotate()
    {
        if ( ++current_ >= items_.Len() )
        {
            current_ = 0;
        }
        /*
        tString CurrentMap(items_[current_]);
        tColoredString ColoredMapString;

        int linenum = CurrentMap.StrPos(0, "/");
        ColoredMapString << tColoredString::ColorString( 0.5,1,.5 );
        ColoredMapString << "Map path is: ";
        ColoredMapString << tColoredString::ColorString( 0,0.5,1 );
        ColoredMapString << items_[current_];
        ColoredMapString << "\n";
        ColoredMapString << tColoredString::ColorString( 0.5,1,.5 );
        ColoredMapString << "Map made by: ";
        ColoredMapString << tColoredString::ColorString( 0,0.5,1 );
        ColoredMapString << CurrentMap.SubStr(0, linenum);
        ColoredMapString << "\n";

        sn_ConsoleOut(ColoredMapString);
        */
    }

    void RandomRotate()
    {
        tRandomizer & randamizer = tRandomizer::GetInstance();
        bool goodnumber = false;
        while (goodnumber == false)
        {
            int random_ = randamizer.Get(items_.Len());
            if ((random_ < items_.Len()) || (random_ >= 0))
            {
                current_ = random_;
                /*
                tString CurrentMap(items_[current_]);
                tColoredString ColoredMapString;

                int linenum = CurrentMap.StrPos(0, "/");
                ColoredMapString << tColoredString::ColorString( 0.5,1,.5 );
                ColoredMapString << "Map path is: ";
                ColoredMapString << tColoredString::ColorString( 0,0.5,1 );
                ColoredMapString << items_[current_];
                ColoredMapString << "\n";
                ColoredMapString << tColoredString::ColorString( 0.5,1,.5 );
                ColoredMapString << "Map made by: ";
                ColoredMapString << tColoredString::ColorString( 0,0.5,1 );
                ColoredMapString << CurrentMap.SubStr(0, linenum);
                ColoredMapString << "\n";

                sn_ConsoleOut(ColoredMapString);
                */
                goodnumber = true;
            }
        }
    }

    void Reset()
    {
        current_ = 0;
    }

    tString Get(int itemID) const
    {
        return items_[itemID];
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

static int sg_configRotationType = 0;
bool restrictConfigRotationTypes(const int &newValue)
{
    if ((newValue < 0) || newValue > 1)
    {
        return false;
    }
    return true;
}
static tSettingItem<int> sg_configRotationTypeConf("CONFIG_ROTATION_TYPE", sg_configRotationType, &restrictConfigRotationTypes);

enum gRotationType
{
    gROTATION_NEVER = 0,
    gROTATION_ORDERED_ROUND = 1,
    gROTATION_ORDERED_MATCH = 2,
    gROTATION_RANDOM_ROUND = 3,
    gROTATION_RANDOM_MATCH = 4
};

tCONFIG_ENUM( gRotationType );

static gRotationType rotationtype = gROTATION_NEVER;
bool restrictRotatiobType(const gRotationType &newValue)
{
    if ((newValue < gROTATION_NEVER) || (newValue > gROTATION_RANDOM_MATCH))
    {
        return false;
    }
    return true;
}
static tSettingItem<gRotationType> conf_rotationtype("ROTATION_TYPE", rotationtype, &restrictRotatiobType);

void sg_ResetRotation()
{
    sg_mapRotation.Reset();
    sg_configRotation.Reset();
    if ( rotationtype != gROTATION_NEVER )
        con << tOutput( "$reset_rotation_message" ) << '\n';
}

void sg_ResetRotation( std::istream & )
{
    sg_ResetRotation();
}

static tConfItemFunc sg_resetRotationConfItemFunc( "RESET_ROTATION", sg_ResetRotation );

static bool sg_resetRotationOnNewMatch = false;
static tSettingItem< bool > sg_resetRotationOnNewMatchSettingItem( "RESET_ROTATION_ON_START_NEW_MATCH", sg_resetRotationOnNewMatch );

class tSettingMapQueing: public tConfItemBase
{
    public:
        tSettingMapQueing( char const * name )
        : tConfItemBase( name )
        {
        }

        // the number of items
        int Size() const
        {
            return items_.Len();
        }

        void Remove(int itemID)
        {
            items_.RemoveAt(itemID);
        }

        void Reset()
        {
            current_ = 0;
        }

        // returns the current value
        tString Current() const
        {
            tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

            return items_[current_];
        }

        // returns the current id
        int CurrentID() const
        {
            return current_;
        }

        void Insert(tString item_name)
        {
            items_.Insert(item_name);
        }

        tString Get(int itemID)
        {
            return items_[itemID];
        }

    private:
        virtual void ReadVal( std::istream &is )
        {
            tString items;
            items.ReadLine (is);
            if (items != "")
            {
                int pos = 0;
                tString item = items;
                bool mapFound = false;
                tArray<tString> searchFindings;
                for (int i=0; i < sg_mapRotation.Size(); i++)
                {
                    tString name = sg_mapRotation.Get(i);

                    tString filteredName, filteredSearch;
                    filteredName = ePlayerNetID::FilterName(name);
                    filteredSearch = ePlayerNetID::FilterName(item);
                    if (filteredName.Contains(filteredSearch))
                    {
                        mapFound = true;
                        searchFindings.Insert(name);
                    }
                }
                if (!mapFound)
                {
                    tOutput Output;
                    Output.SetTemplateParameter(1, item);
                    Output << "$map_queing_file_not_found";
                    sn_ConsoleOut(Output);
                }
                else
                {
                    if (searchFindings.Len() == 1)
                    {
                        bool found = false;
                        tString mapName = searchFindings[0];
                        for (int j=0; j < items_.Len(); j++)
                        {
                            tString item_name = items_[j];
                            if (item_name == mapName)
                            {
                                tOutput Output;
                                Output.SetTemplateParameter(1, mapName);
                                Output.SetTemplateParameter(2, j+1);
                                Output << "$map_queing_stored_file_found";
                                sn_ConsoleOut(Output);
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            items_.Insert(mapName);
                            tOutput Output;
                            Output.SetTemplateParameter(1, mapName);
                            Output << "$map_queing_file_stored";
                            sn_ConsoleOut(Output);
                        }
                    }
                    else
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queing_found_toomanyfiles";
                        sn_ConsoleOut(Output);
                    }
                }
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

        tArray<tString> items_;
        int current_;
};

class tSettingConfQueing: public tConfItemBase
{
    public:
        tSettingConfQueing( char const * name )
        : tConfItemBase( name )
        {
        }

        // the number of items
        int Size() const
        {
            return items_.Len();
        }

        void Remove(int itemID)
        {
            items_.RemoveAt(itemID);
        }

        void Reset()
        {
            current_ = 0;
        }

        // returns the current value
        tString Current() const
        {
            tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

            return items_[current_];
        }

        // returns the current id
        int CurrentID() const
        {
            return current_;
        }

        void Insert(tString item_name)
        {
            items_.Insert(item_name);
        }

        tString Get(int itemID)
        {
            return items_[itemID];
        }

    private:
        virtual void ReadVal( std::istream &is )
        {
            tString items;
            items.ReadLine (is);
            if (items != "")
            {
                int pos = 0;
                tString item = items;
                bool configFound = false;
                tArray<tString> searchFindings;
                for (int i=0; i < sg_configRotation.Size(); i++)
                {
                    tString name = sg_configRotation.Get(i);

                    tString filteredName, filteredSearch;
                    filteredName = ePlayerNetID::FilterName(name);
                    filteredSearch = ePlayerNetID::FilterName(item);
                    if (filteredName.Contains(filteredSearch))
                    {
                        configFound = true;
                        searchFindings.Insert(name);
                    }
                }
                if (!configFound)
                {
                    tOutput Output;
                    Output.SetTemplateParameter(1, item);
                    Output << "$config_queing_file_not_found";
                    sn_ConsoleOut(Output);
                }
                else
                {
                    if (searchFindings.Len() == 1)
                    {
                        bool found = false;
                        tString configName = searchFindings[0];
                        for (int j=0; j < items_.Len(); j++)
                        {
                            tString item_name = items_[j];
                            if (item_name == configName)
                            {
                                tOutput Output;
                                Output.SetTemplateParameter(1, configName);
                                Output.SetTemplateParameter(2, j+1);
                                Output << "$config_queing_stored_file_found";
                                sn_ConsoleOut(Output);
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            items_.Insert(configName);
                            tOutput Output;
                            Output.SetTemplateParameter(1, configName);
                            Output << "config_queing_file_stored";
                            sn_ConsoleOut(Output);
                        }
                    }
                    else
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queing_found_toomanyfiles";
                        sn_ConsoleOut(Output);
                    }
                }
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

        tArray<tString> items_;
        int current_;
};

static tSettingMapQueing sg_mapQueing("MAP_QUEING");
static tSettingConfQueing sg_configQueing("CONFIG_QUEING");

void sg_ResetMapQueing( std::istream &s)
{
    for (int i=0; i < sg_mapQueing.Size(); i++)
    {
        sg_mapQueing.Remove(i);
    }
}
static tConfItemFunc sg_resetMapQueingConf( "RESET_MAP_QUEING", &sg_ResetMapQueing );

void sg_ResetConfigQueing( std::istream &s )
{
    for (int i=0; i < sg_configQueing.Size(); i++)
    {
        sg_configQueing.Remove(i);
    }
}
static tConfItemFunc sg_ResetConfigQueingConf( "RESET_CONFIG_QUEING", &sg_ResetConfigQueing );

// minimal access level to add items to the map list
static tAccessLevel se_mapQueingAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_mapQueingAccessLevelConf( "ACCESS_LEVEL_QUE_MAPS", se_mapQueingAccessLevel );

// minimal access level to add items to the config list
static tAccessLevel se_configQueingAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_configQueingAccessLevelConf( "ACCESS_LEVEL_QUE_CONFIGS", se_configQueingAccessLevel );

void sg_AddQueingItems(ePlayerNetID *p, std::istream &s, tString command)
{
    tString params;
    int pos = 0;
    params.ReadLine(s);

    if (command == "/mq")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of maps in the que by order:\n";
            if (sg_mapQueing.Size() > 0)
            {
                for (int i=0; i < sg_mapQueing.Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_mapQueing.Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_mapQueing.Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items currently in the map queing system.";
            }
            Output << "\n";
            sn_ConsoleOut(Output, p->Owner());
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_mapQueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_mapRotation.Size(); i++)
                    {
                        tString name = sg_mapRotation.Get(i);

                        tString filteredName, filteredSearch;
                        filteredName = ePlayerNetID::FilterName(name);
                        filteredSearch = ePlayerNetID::FilterName(item);
                        if (filteredName.Contains(filteredSearch))
                        {
                            mapFound = true;
                            searchFindings.Insert(name);
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queing_file_not_found";
                        sn_ConsoleOut(Output);
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            bool found = false;
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_mapQueing.Size(); j++)
                            {
                                tString item_name = sg_mapQueing.Get(j);
                                if (item_name == mapName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, mapName);
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$map_queing_stored_file_found";
                                    sn_ConsoleOut(Output);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_mapQueing.Insert(mapName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, mapName);
                                Output << "$map_queing_file_stored";
                                sn_ConsoleOut(Output);
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output << "$map_queing_found_toomanyfiles";
                            sn_ConsoleOut(Output);
                        }
                    }
                    for (int i=0; i < searchFindings.Len(); i++)
                        searchFindings.RemoveAt(i);
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapQueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_mapQueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_mapQueing.Size(); i++)
                    {
                        tString name = sg_mapQueing.Get(i);

                        tString filteredName, filteredSearch;
                        filteredName = ePlayerNetID::FilterName(name);
                        filteredSearch = ePlayerNetID::FilterName(item);
                        if (filteredName.Contains(filteredSearch))
                        {
                            mapFound = true;
                            searchFindings.Insert(name);
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_mapQueing.Size(); j++)
                            {
                                tString item_name =  sg_mapQueing.Get(j);
                                if (item_name == mapName)
                                {
                                    sg_mapQueing.Remove(j);
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, mapName);
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$map_queing_file_removed";
                                    sn_ConsoleOut(Output);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output << "$map_queing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }
                    for (int i=0; i < searchFindings.Len(); i++)
                        searchFindings.RemoveAt(i);
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapQueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
    else if (command == "/cq")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of configs in the que by order:\n";
            if (sg_configQueing.Size() > 0)
            {
                for (int i=0; i < sg_configQueing.Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_configQueing.Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_configQueing.Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items in the config queing system.";
            }
            Output << "\n";
            sn_ConsoleOut(Output, p->Owner());
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_configQueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_configRotation.Size(); i++)
                    {
                        tString name = sg_configRotation.Get(i);

                        tString filteredName, filteredSearch;
                        filteredName = ePlayerNetID::FilterName(name);
                        filteredSearch = ePlayerNetID::FilterName(item);
                        if (filteredName.Contains(filteredSearch))
                        {
                            mapFound = true;
                            searchFindings.Insert(name);
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queing_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            bool found = false;
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_configQueing.Size(); j++)
                            {
                                tString item_name =  sg_configQueing.Get(j);
                                if (item_name == configName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$config_queing_stored_file_found";
                                    sn_ConsoleOut(Output, p->Owner());
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_mapQueing.Insert(configName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, configName);
                                Output.SetTemplateParameter(2, p->GetName());
                                Output << "config_queing_file_stored";
                                sn_ConsoleOut(Output, p->Owner());
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output << "$config_queing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }
                    for (int i=0; i < searchFindings.Len(); i++)
                        searchFindings.RemoveAt(i);
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configQueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_configQueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool configFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_configQueing.Size(); i++)
                    {
                        tString name = sg_configQueing.Get(i);

                        tString filteredName, filteredSearch;
                        filteredName = ePlayerNetID::FilterName(name);
                        filteredSearch = ePlayerNetID::FilterName(item);
                        if (filteredName.Contains(filteredSearch))
                        {
                            configFound = true;
                            searchFindings.Insert(name);
                        }
                    }
                    if (!configFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_configQueing.Size(); j++)
                            {
                                tString item_name =  sg_configQueing.Get(j);
                                if (item_name == configName)
                                {
                                    sg_configQueing.Remove(j);
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$config_queing_file_removed";
                                    sn_ConsoleOut(Output);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output << "$config_queing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }
                    for (int i=0; i < searchFindings.Len(); i++)
                        searchFindings.RemoveAt(i);
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configQueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
}

// enable/disable sound, supporting two different pause reasons:
// if fromActivity is set, the pause comes from losing input focus.
// otherwise, it's from game state changes.
static void sg_SoundPause( bool pause, bool fromActivity )
{
    static bool flags[2]={true, false};
    flags[ fromActivity ] = pause;
    se_SoundPause( flags[0] || flags[1] );
}

// bool globalingame=false;

void sg_PrintCurrentTime( char const * szFormat )
{
    con << st_GetCurrentTime(szFormat);
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


static eWavData intro("moviesounds/intro.wav");
static eWavData extro("moviesounds/extro.wav");


#define MAXAI (gAICharacter::s_Characters.Len())

#define AUTO_AI_MAXFRAC 6
#define AUTO_AI_WIN     3
#define AUTO_AI_LOSE    1

gGameSettings::gGameSettings(int a_scoreWin, int a_scoreDiffWin,
                             int a_limitTime, int a_limitRounds, int a_limitScore, int a_limitSet,
                             int a_numAIs,    int a_minPlayers,  int a_AI_IQ,
                             bool a_autoNum, bool a_autoIQ,
                             int a_speedFactor, int a_sizeFactor,
                             gGameType a_gameType,  gFinishType a_finishType,
                             int a_minTeams,
                             REAL a_winZoneMinRoundTime, REAL a_winZoneMinLastDeath
                            )
        :scoreWin(a_scoreWin), scoreDiffWin(a_scoreDiffWin),
        limitTime(a_limitTime), limitRounds(a_limitRounds), limitScore(a_limitScore), limitSet(a_limitSet),
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

    uMenuItemReal wzmr
    (&GameSettings,
     "$game_menu_wz_mr_text",
     "$game_menu_wz_mr_help",
     winZoneMinRoundTime, (REAL) 0, (REAL) 1000, (REAL) 10 );

    uMenuItemReal wzmld
    (&GameSettings,
     "$game_menu_wz_ld_text",
     "$game_menu_wz_ld_help",
     winZoneMinLastDeath, (REAL) 0 , (REAL) 1000, (REAL) 10 );

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

gGameSettings singlePlayer(10, 1,
                           30, 10, 100000, 1,
                           1,   0, 30,
                           true, true,
                           0  ,  -3,
                           gDUEL, gFINISH_IMMEDIATELY, 1,
                           100000, 1000000);

gGameSettings multiPlayer(10, 1,
                          30, 10, 100, 1,
                          0,   4, 100,
                          false, false,
                          0  ,  -3,
                          gDUEL, gFINISH_IMMEDIATELY, 2,
                          60, 30 );

gGameSettings* sg_currentSettings = &singlePlayer;

static tSettingItem<int> mp_sw("SCORE_WIN"   ,multiPlayer.scoreWin);
static tSettingItem<int> mp_sd("SCORE_DIFF_WIN"   ,multiPlayer.scoreDiffWin);
static tSettingItem<int> mp_lt("LIMIT_TIME"  ,multiPlayer.limitTime);
static tSettingItem<int> mp_lr("LIMIT_ROUNDS",multiPlayer.limitRounds);
static tSettingItem<int> mp_ls("LIMIT_SCORE" ,multiPlayer.limitScore);
static tSettingItem<int> mp_lset("LIMIT_SETS" ,multiPlayer.limitSet);

static tConfItem<int>    mp_na("NUM_AIS"     ,multiPlayer.numAIs);
static tConfItem<int>    mp_mp("MIN_PLAYERS" ,multiPlayer.minPlayers);
static tConfItem<int>    mp_iq("AI_IQ"       ,multiPlayer.AI_IQ);

static tConfItem<bool>   mp_an("AUTO_AIS"    ,multiPlayer.autoNum);
static tConfItem<bool>   mp_aq("AUTO_IQ"     ,multiPlayer.autoIQ);

static tConfItem<int>    mp_sf("SPEED_FACTOR",multiPlayer.speedFactor);
static tConfItem<int>    mp_zf("SIZE_FACTOR" ,multiPlayer.sizeFactor);

static tConfItem<gGameType>    mp_gt("GAME_TYPE",multiPlayer.gameType);
static tConfItem<gFinishType>  mp_ft("FINISH_TYPE",multiPlayer.finishType);

static tConfItem<REAL>   mp_wzmr("WIN_ZONE_MIN_ROUND_TIME",multiPlayer.winZoneMinRoundTime);
static tConfItem<REAL>   mp_wzld("WIN_ZONE_MIN_LAST_DEATH",multiPlayer.winZoneMinLastDeath);

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
static tSettingItem<int> sp_sd("SP_SCORE_DIFF_WIN"   ,singlePlayer.scoreDiffWin);
static tSettingItem<int> sp_lt("SP_LIMIT_TIME"  ,singlePlayer.limitTime);
static tSettingItem<int> sp_lr("SP_LIMIT_ROUNDS",singlePlayer.limitRounds);
static tSettingItem<int> sp_ls("SP_LIMIT_SCORE" ,singlePlayer.limitScore);
static tSettingItem<int> sp_lset("SP_LIMIT_SETS" ,singlePlayer.limitSet);

static tConfItem<int>    sp_na("SP_NUM_AIS"     ,singlePlayer.numAIs);
static tConfItem<int>    sp_mp("SP_MIN_PLAYERS" ,singlePlayer.minPlayers);
static tConfItem<int>    sp_iq("SP_AI_IQ"       ,singlePlayer.AI_IQ);

static tConfItem<bool>   sp_an("SP_AUTO_AIS"    ,singlePlayer.autoNum);
static tConfItem<bool>   sp_aq("SP_AUTO_IQ"     ,singlePlayer.autoIQ);

static tConfItem<int>    sp_sf("SP_SPEED_FACTOR",singlePlayer.speedFactor);
static tConfItem<int>    sp_zf("SP_SIZE_FACTOR" ,singlePlayer.sizeFactor);

static tConfItem<gGameType>    sp_gt("SP_GAME_TYPE",singlePlayer.gameType);
static tConfItem<gFinishType>  sp_ft("SP_FINISH_TYPE",singlePlayer.finishType);

static tConfItem<REAL>    sp_wzmr("SP_WIN_ZONE_MIN_ROUND_TIME",singlePlayer.winZoneMinRoundTime);
static tConfItem<REAL>    sp_wzld("SP_WIN_ZONE_MIN_LAST_DEATH",singlePlayer.winZoneMinLastDeath);

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

extern int ladder_highscore_output;

class gHighscoresBase{
    int id;
    static tList<gHighscoresBase> highscoreList;
protected:
    tArray<tString> highName;

    char const * highscore_file;
    tOutput desc;
    int     maxSize;

    // find the player at position i
    ePlayerNetID *online(int p){
        for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            if (se_PlayerNetIDs(i)->IsHuman() && !strcmp(se_PlayerNetIDs(i)->GetUserName(),highName[p]))
                return se_PlayerNetIDs(i);

        return NULL;
    }

    // i is the active player, j the passive one
    virtual void swap_entries(int i,int j){
        Swap(highName[i],highName[j]);

        // swap: i is now the passive player

        // send the poor guy who just dropped a message
        ePlayerNetID *p=online(i);
        if (p && ladder_highscore_output){
            tColoredString name;
            name << *p << tColoredString::ColorString(1,.5,.5);

            tOutput message;
            message.SetTemplateParameter(1, static_cast<const char *>(name));
            message.SetTemplateParameter(2, static_cast<const char *>(highName[j]));
            message.SetTemplateParameter(3, i+1);
            message.SetTemplateParameter(4, desc);

            if (i<j)
                message <<  "$league_message_rose" ;
            else
                message <<  "$league_message_dropped" ;

            message << "\n";

            tString s;
            s << message;
            sn_ConsoleOut(s,p->Owner());
            // con << message;
        }

    }

    void load_Name(std::istream &s,int i){
        std::ws( s );

        // read name
        highName[i].ReadLine( s );
    }

    void save_Name(std::ostream &s,int i){
        s << highName[i];
    }


public:

    virtual void Save()=0;
    virtual void Load()=0;

    // returns if i should stay above j
    virtual bool inorder(int i,int j)=0;

    void sort(){
        // since single score items travel up the
        // score ladder, this is the most efficient sort algorithm:

        for(int i=1;i<highName.Len();i++)
            for(int j=i;j>=1 && !inorder(j-1,j);j--)
                swap_entries(j,j-1);
    }


    int checkPos(int found){
        // move him up
        int newpos=found;
        while(newpos>0 && !inorder(newpos-1,newpos)){
            swap_entries(newpos,newpos-1);
            newpos--;
        }

        // move him down
        while(newpos<highName.Len()-1 && !inorder(newpos,newpos+1)){
            swap_entries(newpos,newpos+1);
            newpos++;
        }

        //Save();

        return newpos;
    }

    int Find(const char *name,bool force=false){
        int found=highName.Len();
        for(int i=found-1;i>=0 ;i--)
            if(highName[i].Len()<=1 || !strcmp(highName[i],name)){
                found=i;
            }

        if (force && highName[found].Len()<=1)
            highName[found]=name;

        return found;
    }

    gHighscoresBase(char const * name,char const * sd,int max=0)
            :id(-1),highscore_file(name),desc(sd),maxSize(max){
        highscoreList.Add(this,id);
    }

    virtual ~gHighscoresBase(){
        highscoreList.Remove(this,id);
    }

    virtual void greet_this(ePlayerNetID *p,tOutput &o){
        //    tOutput o;

        int f=Find(p->GetUserName())+1;
        int l=highName.Len();

        o.SetTemplateParameter(1, f);
        o.SetTemplateParameter(2, l);
        o.SetTemplateParameter(3, desc);

        if (l>=f)
            o << "$league_message_greet";
        else
            o << "$league_message_greet_new";

        //    s << o;
    }

    static void Greet(ePlayerNetID *p,tOutput &o){
        o << "$league_message_greet_intro";
        for(int i=highscoreList.Len()-1;i>=0;i--){
            highscoreList(i)->greet_this(p,o);
            if (i>1)
                o << "$league_message_greet_sep" << " ";
            if (i==1)
                o << " " << "$league_message_greet_lastsep" << " ";
        }
        o << ".\n";
    }

    static void SaveAll(){
        for(int i=highscoreList.Len()-1;i>=0;i--)
            highscoreList(i)->Save();
    }

    static void LoadAll(){
        for(int i=highscoreList.Len()-1;i>=0;i--)
            highscoreList(i)->Load();
    }
};


tString GreetHighscores()
{
    tOutput o;
    gHighscoresBase::Greet(eCallbackGreeting::Greeted(),o);
    tString s;
    s << o;
    return s;
}

static eCallbackGreeting g(GreetHighscores);

tList<gHighscoresBase> gHighscoresBase::highscoreList;

template<class T>class highscores: public gHighscoresBase{
    protected:
    tArray<T>    high_score;

    virtual void swap_entries(int i,int j){
        Swap(high_score[i],high_score[j]);
        gHighscoresBase::swap_entries(i,j);
    }

    public:
    virtual void Load(){
        tTextFileRecorder stream( tDirectories::Var(), highscore_file );
        int i=0;
        while ( !stream.EndOfFile() )
        {
            std::stringstream s( stream.GetLine() );
            s >> high_score[i];
            load_Name(s,i);
            // con << highName[i] << " " << high_score[i] << '\n';
            i++;
        }
    }

    // returns if i should stay above j
    virtual bool inorder(int i,int j)
    {
        return (highName[j].Len()<=1 || high_score[i]>=high_score[j]);
    }


    virtual void Save(){
        std::ofstream s;

        sort();

        tString filename(highscore_file);
        if ( tRecorder::IsPlayingBack() )
        {
            filename += ".playback";
        }

        if ( tDirectories::Var().Open ( s, filename ) )
        {
            int i=0;
            int max=high_score.Len();
            if (maxSize && max>maxSize)
                max=maxSize;
            while (i<max && highName[i].Len()>1){
                tString mess;
                mess << high_score[i];
                mess.SetPos(10, false );
                s << mess;
                save_Name(s,i);
                s << '\n';
                i++;
                //std::cout << mess;
                //save_Name(std::cout,i);
                //std::cout << '\n';
            }
        }
    }

    void checkPos(int found,const tString &name,T score){
        tOutput message;
        // find the name in the list
        bool isnew=false;

        message.SetTemplateParameter(1, name);
        message.SetTemplateParameter(2, desc);
        message.SetTemplateParameter(3, score);

        if (highName[found].Len()<=1){
            highName[found]=name;
            message << "$highscore_message_entered";
            high_score[found]=score;
            isnew=true;
        }
        else if (score>high_score[found]){
            message << "$highscore_message_improved";
            high_score[found]=score;
        }
        else
            return;

        // move him up
        int newpos=gHighscoresBase::checkPos(found);

        message.SetTemplateParameter(4, newpos + 1);

        if (newpos!=found || isnew)
            if (newpos==0)
                message << "$highscore_message_move_top";
            else
                message << "$highscore_message_move_pos";
        else
            if (newpos==0)
                message << "$highscore_message_stay_top";
            else
                message << "$highscore_message_stay_pos";

        message << "\n";

        ePlayerNetID *p=online(newpos);
        //con << message;
        if (p && ladder_highscore_output)
            sn_ConsoleOut(tString(message),p->Owner());
        //Save();
    }

    void Add( ePlayerNetID* player,T AddScore)
    {
        tASSERT( player );
        tString const & name = player->GetUserName();
        int f=Find(name,true);
        checkPos(f,name,AddScore+high_score[f]);
    }

    void Add( eTeam* team,T AddScore)
    {
        if ( team->NumHumanPlayers() > 0 )
        {
            for ( int i = team->NumPlayers() - 1 ; i>=0; --i )
            {
                ePlayerNetID* player = team->Player( i );
                if ( player->IsHuman() )
                {
                    this->Add( player, AddScore );
                }
            }
        }
    }

    void Check(const ePlayerNetID* player,T score){
        tASSERT( player );
        tString name = player->GetUserName();

        // find the name in the list
        int found=Find(name,true);
        checkPos(found,name,score);
    }

    highscores(char const * name,char const * sd,int max=0)
            :gHighscoresBase(name,sd,max){
        //		Load();
    }

    virtual ~highscores(){
        //		Save();
    }
};



static REAL ladder_min_bet=1;
static tSettingItem<REAL> ldd_mb("LADDER_MIN_BET",
                                 ladder_min_bet);

static REAL ladder_perc_bet=10;
static tSettingItem<REAL> ldd_pb("LADDER_PERCENT_BET",
                                 ladder_perc_bet);

static REAL ladder_tax=1;
static tSettingItem<REAL> ldd_tex("LADDER_TAX",
                                  ladder_tax);

static REAL ladder_lose_perc_on_load=.2;
static tSettingItem<REAL> ldd_lpl("LADDER_LOSE_PERCENT_ON_LOAD",
                                  ladder_lose_perc_on_load);

static REAL ladder_lose_min_on_load=.2;
static tSettingItem<REAL> ldd_lml("LADDER_LOSE_MIN_ON_LOAD",
                                  ladder_lose_min_on_load);

static REAL ladder_gain_extra=1;
static tSettingItem<REAL> ldd_ga("LADDER_GAIN_EXTRA",
                                 ladder_gain_extra);

static float sg_gameTimeInterval=-1;
static tSettingItem<float> sggti("LADDERLOG_GAME_TIME_INTERVAL",
                                 sg_gameTimeInterval);

static float sg_tacticalPositionInterval=5;
static tSettingItem<float> sgtpi("TACTICAL_POSITION_INTERVAL",
                                 sg_tacticalPositionInterval);

static float sg_gridPosInterval=1;
static tSettingItem<float> sggpi("GRID_POSITION_INTERVAL",
                                 sg_gridPosInterval);


class ladder: public highscores<REAL>{
public:
    virtual void Load(){
        highscores<REAL>::Load();

        sort();

        int end=highName.Len();

        for(int i=highName.Len()-1;i>=0;i--){

            // make them lose some points

            REAL loss=ladder_lose_perc_on_load*high_score[i]*.01;
            if (loss<ladder_lose_min_on_load)
                loss=ladder_lose_min_on_load;
            high_score[i]-=loss;

            if (high_score[i]<0)
                end=i;
        }

        // remove the bugggers with less than 0 points
        highName.SetLen(end);
        high_score.SetLen(end);
    }

    void checkPos(int found,const tString &name,REAL score){
        tOutput message;

        message.SetTemplateParameter(1, name);
        message.SetTemplateParameter(2, desc);
        message.SetTemplateParameter(3, score);

        // find the name in the list
        bool isnew=false;
        if (highName[found].Len()<=1){
            highName[found]=name;
            message << "$ladder_message_entered";
            high_score[found]=score;
            isnew=true;
        }
        else{
            REAL diff=score-high_score[found];
            message.SetTemplateParameter(5, static_cast<float>(fabs(diff)));

            if (diff>0)
                message << "$ladder_message_gained";
            else
                message << "$ladder_message_lost";

            high_score[found]=score;
        }

        // move him up
        int newpos=gHighscoresBase::checkPos(found);

        message.SetTemplateParameter(4, newpos + 1);

        if (newpos!=found || isnew)
            if (newpos==0)
                message << "$ladder_message_move_top";
            else
                message << "$ladder_message_move_pos";
        else
            if (newpos==0)
                message << "$ladder_message_stay_top";
            else
                message << "$ladder_message_stay_pos";

        message << "\n";

        ePlayerNetID *p=online(newpos);
        // con << message;
        if (p && ladder_highscore_output){
            sn_ConsoleOut(tString(message),p->Owner());
        }

        // Save();
    }

    void Add(const tString &name,REAL AddScore){
        int found=Find(name,true);
        checkPos(found,name,AddScore+high_score[found]);
    }

    // ladder mechanics: what happens if someone wins?
    void winner( eTeam *winningTeam ){
        tASSERT( winningTeam );

        // AI won? bail out.
        if ( winningTeam->NumHumanPlayers() <= 0 )
        {
            return;
        }

        // only do something in multiplayer mode
        int i;
        int count=0;

        tArray<ePlayerNetID*> active;

        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            ePlayerNetID* p=se_PlayerNetIDs(i);

            // only take enemies of the current winners (and the winners themselves) into the list
            if (p->IsHuman() && ( p->CurrentTeam() == winningTeam || eTeam::Enemies( winningTeam, p ) ) )
            {
                count++;
                active[active.Len()] = p;
            }
        }

        // only one active player? quit.
        if ( active.Len() <= 1 )
        {
            return;
        }

        // collect the bets
        tArray<int> nums;
        tArray<REAL> bet;

        REAL pot=0;

        for(i=active.Len()-1;i>=0;i--){

            nums[i]=Find(active(i)->GetUserName(),true);

            if (high_score[nums[i]]<0)
                high_score[nums[i]]=0;

            bet[i]=high_score[nums[i]]*ladder_perc_bet*.01;
            if (bet[i]<ladder_min_bet)
                bet[i]=ladder_min_bet;
            pot+=bet[i];
        }

        pot-=pot*ladder_tax*.01; // you have to pay to the bank :-)

        // now bet[i] tells us how much player nums[i] has betted
        // and pot is the overall bet. Add something to it, prop to
        // the winners ping+ping charity:

        // take the bet from the losers and give it to the winner
        for(i=active.Len()-1; i>=0; i--)
        {
            ePlayerNetID* player = active(i);
            if(player->CurrentTeam() == winningTeam )
            {
                REAL pc=player->ping + player->pingCharity*.001;
                if (pc<0)
                    pc=0;
                if (pc>.3) // add sensible bounds
                    pc=1;

                REAL potExtra = pc*ladder_gain_extra;

                if ( player->Object() && player->Object()->Alive() )
                {
                    potExtra *= 2.0f;
                }

                Add(player->GetUserName(), ( pot / winningTeam->NumHumanPlayers() + potExtra ) - bet[i] );
            }
            else
            {
                Add(player->GetUserName(),-bet[i]);
            }
        }
    }

    ladder(char const * name,char const * sd,int max=0)
            :highscores<REAL>(name,sd,max){
        //		Load();
    }

    virtual ~ladder(){
        //		Save();
    }
};


static highscores<int> highscore("highscores.txt","$highscore_description",100);
static highscores<int> highscore_won_rounds("won_rounds.txt",
        "$won_rounds_description");
static highscores<int> highscore_won_matches("won_matches.txt",
        "$won_matches_description");
static ladder highscore_ladder("ladder.txt",
                               "$ladder_description");

// *************************
// ***   delayed commands
// *************************

class delayedCommands {
private:
	static std::map<int, std::set<std::string> > cmd_map;
	// compute key for std::multimap
	static int Key(REAL time) {return int(ceil(time*10));};
public:
	// clear all delayed commands
	static void Clear() {
		cmd_map.clear();
		con << "Clearing delayed commands ...\n";
	};
	// add new delayed commands
	static void Add(REAL time, std::string cmd, int interval) {
        std::stringstream store;
        store << interval << " " << cmd;
        //std::string stored = store;
		cmd_map[Key(time)].insert(store.str());
	};
	// check, run and remove delayed commands
	static void Run(REAL currentTime);
};

std::map<int, std::set<std::string> > delayedCommands::cmd_map;

static void sg_AddDelayedCmd(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// first parse the line to get the param : delay or interval
    // if the param start by an r then it means we have the interval
	// if the param start by a +, assume that it's a delay relative to current game time ...
	int pos = 0;				 //
    int interval=0;
	tString delay_str = params.ExtractNonBlankSubString(pos);

	if (delay_str.SubStr(0,1)=="r")
    {
		interval = atoi(delay_str.SubStr(1));
        delay_str = params.ExtractNonBlankSubString(pos);
	}
    REAL delay = atof(delay_str);
    if (delay_str.SubStr(0,1)=="+") {
		REAL gt = se_GameTime();
		delay += gt;
	}
    // this will make sure it using the command if  start time has passed
    if ((interval > 0) && (se_GameTime() > delay)){
        REAL ogt = se_GameTime() - delay;
        int disposition = (ogt/interval)+1;
        delay = disposition*interval+delay;
    }
    tString cmd_name;
    cmd_name= params.ExtractNonBlankSubString(pos);
	std::stringstream cmd_str;
	cmd_str << cmd_name <<"  " << params.SubStr(pos+1);
	if (cmd_str.str().length()==0) return;

    int cLevel = tConfItemBase::AccessLevel(cmd_str);
	// add extracted command
    if (tCurrentAccessLevel::GetAccessLevel() <=cLevel){
        delayedCommands::Add(delay,cmd_str.str(),interval);
    }else{
        tToUpper( cmd_name );
        con << tOutput( "$access_level_error",
                    cmd_name,
                    tCurrentAccessLevel::GetName( static_cast< tAccessLevel >(cLevel) ),
                    tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() )
                    );
    }
	//con << "DELAY_COMMAND " << delay << " "<< interval<<" &" << cmd_str.str() << "&\n";
}

static tConfItemFunc sg_AddDelayedCmd_conf("DELAY_COMMAND",&sg_AddDelayedCmd);

static tAccessLevelSetter sg_AddDelayedCmdConfLevel( sg_AddDelayedCmd_conf, tAccessLevel_Owner );

void delayedCommands::Run(REAL currentTime) {
    if (cmd_map.empty()) return;
    std::map<int, std::set<std::string> >::iterator it = cmd_map.begin();
    while ((it != cmd_map.end())&&(it->first<=Key(currentTime))) {
        if (it->first>Key(currentTime-1.0)) {
            std::set<std::string>::iterator sit (it->second.begin()), send(it->second.end());
            for(;sit!=send;++sit) {
                std::istringstream stream(*sit);
                tString params;
                params.ReadLine( stream, true );
                int pos = 0;
                const tString interval_str = params.ExtractNonBlankSubString(pos);
                int interval = atoi(interval_str);
                tCurrentAccessLevel elevator( sg_AddDelayedCmd_conf.GetRequiredLevel(), true );
                std::stringstream command;
                const tString command_str = params.SubStr(interval_str.Len());
                command << command_str;
                tConfItemBase::LoadAll(command); // run command if it's not too old, otherwise, just skip it ...
                con << command << " " << interval <<"\n";
                if (interval>0){
                    delayedCommands::Add(currentTime+interval,command.str(),interval);
                }
            }
        }
        cmd_map.erase(it++); // erase current and get next iterator
    };
}

// *****************************
// ***   end delayed commands
// *****************************

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
    if ( team && !winner && !wishWinner )
    {
        wishWinner = team->TeamID() + 1;
        wishWinnerMessage = message;
    }
}

void check_hs(){
    if (sg_singlePlayer)
        if(se_PlayerNetIDs.Len()>0 && se_PlayerNetIDs(0)->IsHuman())
            highscore.Check(se_PlayerNetIDs(0),se_PlayerNetIDs(0)->Score());
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

gArena Arena;





void exit_game_grid(eGrid *grid){
    eSoundLocker soundLocker;

    grid->Clear();
}

extern std::vector<ePolyLine> se_unsplittedRimWalls;

void exit_game_objects(eGrid *grid){
    sr_con.fullscreen=true;

    su_prefetchInput=false;

    eSoundLocker soundLocker;

    int i;
    for(i=ePlayer::Num()-1;i>=0;i--){
        if (ePlayer::PlayerConfig(i))
            tDESTROY(ePlayer::PlayerConfig(i)->cam);
    }

    eGameObject::DeleteAll(grid);

    if (sn_GetNetState()!=nCLIENT)
        for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            if(se_PlayerNetIDs(i))
                se_PlayerNetIDs(i)->ClearObject();

    gNetPlayerWall::Clear();

	se_unsplittedRimWalls.clear();

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

extern REAL max_player_speed;
extern bool sg_axesIndicator;

void init_game_grid(eGrid *grid, gParser *aParser){
    se_ResetGameTimer();
    se_PauseGameTimer(true);

    eSoundLocker soundLocker;

#ifndef DEDICATED
    if (sr_glOut){
        sr_ResetRenderState();

        // rSysDep::ClearGL();

        // glViewport (0, 0, static_cast<GLsizei>(sr_screenWidth), static_cast<GLsizei>(sr_screenHeight));

        // rSysDep::SwapGL();
    }
    max_player_speed = 0.0;
#endif

    /*
      if(!speedup)
      SDL_Delay(1000);
    */

    {
        // let settings in the map file be executed with the rights of the person
        // who set the map
        tCurrentAccessLevel level( conf_mapfile.GetSetting().GetSetLevel(), true );

        // and disallow CASACL and script spawning just in case
        tCasaclPreventer preventer;

        Arena.PrepareGrid(grid, aParser);
    }

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
    eTeam::maxPlayers				= (sg_currentSettings->maxPlayersPerTeam)? sg_currentSettings->maxPlayersPerTeam : 1;
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

static void sg_endChallenge();
void update_settings( bool const * goon )
{
    if (sn_GetNetState()!=nCLIENT)
    {
#ifdef DEDICATED
        // wait for players to join
        {
            bool restarted = false;

            REAL timeout = tSysTimeFloat() + 3.0f;
            while ( sg_NumHumans() <= 0 && sg_NumUsers() > 0 && ( !goon || *goon ) && uMenu::quickexit == uMenu::QuickExit_Off )
            {
                if ( !restarted && bool(sg_currentGame) )
                {
                    sg_currentGame->StartNewMatch();
                    if (eTeam::ongoingChallenge) sg_endChallenge();
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

                // do tasks
                st_DoToDo();
                nAuthentication::OnBreak();

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

// 0 loser first 1 winners first
static int sg_spawnWinnersFirst = 0;
static tSettingItem<int> sg_spawnWinnersFirstConf( "SPAWN_WINNERS_FIRST", sg_spawnWinnersFirst);

static int sg_spawnAlternate = 0;
static tSettingItem<int> sg_spawnAlternateConf( "SPAWN_ALTERNATE", sg_spawnAlternate);

static eLadderLogWriter sg_spawnPositionTeamWriter("SPAWN_POSITION_TEAM", true);

void init_game_objects(eGrid *grid){
    eSoundLocker soundLocker;

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

        if( sg_spawnAlternate == 1 )
        {
            eTeam::SortByPosition();
        }

        for(int t=eTeam::teams.Len()-1;t>=0;t--)
        {
            int z;
            if (sg_spawnWinnersFirst == 1 || sg_spawnAlternate == 1){
                z =(-1*t)+ eTeam::teams.Len()-1;
            }
            else
            {
                z =t;
            }
            eTeam *team = eTeam::teams(z);

            if ( sg_spawnAlternate == 1 ) //switch position for next round
            {
                if ( (team->GetPosition() < 0) || (team->GetPosition() > eTeam::teams.Len()-1) ) //first spawn for this team
                {
                    team->SetPosition(z);
                }
                sg_spawnPositionTeamWriter << ePlayerNetID::FilterName( team->Name() );
                sg_spawnPositionTeamWriter << team->GetPosition();
                sg_spawnPositionTeamWriter.write();
                team->SetPosition(((z+1)>(eTeam::teams.Len()-1))?0:(z+1)); //next round
            }


            // reset team color of fresh teams
            if ( team->RoundsPlayed() == 0 )
            {
                team->NameTeamAfterColor( false );
            }

            // update team name

            team->Update();

            // increase round counter
            team->PlayRound();

            gSpawnPoint *spawn = Arena.LeastDangerousSpawnPoint();
            spawnPointsUsed++;

            // if the spawn points are grouped, make sure the last player is not in a goup of his
            // own by skipping one spawnpoint
            if ( ( eTeam::teams(0)->IsHuman() || eTeam::teams(0)->NumPlayers() == 1 ) && sg_spawnPointGroupSize > 2 && ( spawnPointsUsed % sg_spawnPointGroupSize == sg_spawnPointGroupSize - 1 ) )
            {
                // the next spawn point is the last of this group. Skip it if
                // either only two players are left (and the one would need to play alone)
                // or we can fill the remaining groups well with one player short.
                if ( z == 2 || ( ( z % ( sg_spawnPointGroupSize - 1 ) ) == 0 && z < ( sg_spawnPointGroupSize - 1 ) * ( sg_spawnPointGroupSize - 1 ) ) )
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
                    cycle = new gCycle(grid, pos, dir, pni);
                    pni->ControlObject(cycle);
                    nNetObject::SyncAll();
                }
                //    int i=se_PlayerNetIDs(p)->pID;

                // se_ResetGameTimer();
                // se_PauseGameTimer(true);
            }
        }

        eTeam::SortByScore(); //sort again by score, so new players will join the losing team.

#ifdef ALLOW_NO_TEAM
        for(int p=se_PlayerNetIDs.Len()-1;p>=0;p--){
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
    for(int i=ePlayer::Num()-1;i>=0;i--)
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
    eSoundLocker locker;
    REAL minstep = 0;
#ifdef DEDICATED
    minstep = 0.9/sg_dedicatedFPS;

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

    if(sr_glOut){
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

        for(int i=cameras.Len()-1;i>=0;i--){
            int p=sr_viewportBelongsToPlayer[i];
            conf->Select(i);
            rViewport *act=conf->Port(i);
            if (act && ePlayer::PlayerConfig(p))
                ePlayer::PlayerConfig(p)->Render();
            else con << "hey! viewport " << i << " does not exist!\n";
        }

        // glDisable( GL_FOG );
    }

    // render the console and scores so it appears behind the global HUD
    ePlayerNetID::DisplayScores();
    if( sr_con.autoDisplayAtSwap )
    {
        sr_con.Render();
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
        static bool lastMoviePack=sg_MoviePack();
        if(lastMoviePack!=sg_MoviePack())
        {
            lastMoviePack=sg_MoviePack();
            rDisplayList::ClearAll();
        }

        RenderAllViewports(grid);

        sr_ResetRenderState(true);
        gLogo::Display();

        if (swap){
            rSysDep::SwapGL();
            rSysDep::ClearGL();
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
            for(int i=MAXCLIENTS;i>0;i--)
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

static eLadderLogWriter sg_gameEndWriter("GAME_END", true);

static void own_game( nNetState enter_state ){
    tNEW(gGame);
    se_MakeGameTimer();
    max_player_speed = 0.0;
    sg_EnterGame( enter_state );

    // write scores one last time
    ePlayerNetID::LogScoreDifferences();
    ePlayerNetID::UpdateSuspensions();
    ePlayerNetID::UpdateShuffleSpamTesters();
    sg_gameEndWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
    sg_gameEndWriter.write();

    sg_currentGame=NULL;
    se_KillGameTimer();
}

void sg_SinglePlayerGame(){
    sn_SetNetState(nSTANDALONE);

    update_settings();
    ePlayerNetID::CompleteRebuild();

    own_game( nSTANDALONE );
}

void sg_HostGame(){
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

        while(numPlayers == 0 &&
                (ded_idle <= 0.0f || tSysTimeFloat()<startTime + ded_idle * 3600 ) && !uMenu::quickexit ){
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

        if (sg_NumUsers() <= 0 && ded_idle > 0.0f &&
                tSysTimeFloat()>= startTime + ded_idle * 3600 )
        {
            sg_Timestamp();
            con << "Server exiting due to DEDICATED_IDLE after " << (tSysTimeFloat() - startTime)/3600 << " hours.\n";
            uMenu::quickexit = uMenu::QuickExit_Total;
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

static void sg_StopQuickExit()
{
    st_DoToDo();

    // stop quick exit to game level
    if ( uMenu::quickexit == uMenu::QuickExit_Game )
    {
        uMenu::quickexit = uMenu::QuickExit_Off;
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

    bool to=sr_textOut;

    {
#ifndef DEDICATED
    rSysDep::SwapGL();
    rSysDep::ClearGL();
    rSysDep::SwapGL();
    rSysDep::ClearGL();
    eSoundLocker locker;
#endif

    sr_con.autoDisplayAtNewline=true;
    sr_con.fullscreen=true;

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
    case nABORT:
        return false;
        break;
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

    sn_SetNetState(nSTANDALONE);

    sg_currentGame = NULL;
    nNetObject::ClearAll();
    ePlayerNetID::ClearAll();

    if (!sg_RequestedDisconnection && uMenu::quickexit != uMenu::QuickExit_Total )
    {
        sg_StopQuickExit();

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
    }

    sr_con.autoDisplayAtNewline=false;
    sr_con.fullscreen=false;

    sr_textOut=to;

    return ret;
}

void ConnectToServer(nServerInfoBase *server)
{
    bool to = sr_textOut;

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

    sr_con.autoDisplayAtNewline=false;
    sr_con.fullscreen=false;

    sn_SetNetState(nSTANDALONE);

    sg_currentGame = NULL;
    nNetObject::ClearAll();
    ePlayerNetID::ClearAll();

    sr_textOut=to;

    sg_StopQuickExit();
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

    if(sg_currentGame)
        sg_currentGame->NoLongerGoOn();

    sn_SetNetState(nSTANDALONE);
    max_player_speed = 0.0;

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
     sn_maxRateOut,2,32);


    uMenuItemInt in_rate
    (&net_menu,"$network_opts_inrate_text",
     "$network_opts_inrate_help",
     sn_maxRateIn,3,64);


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
    virtual bool Wait() //!< wait for something to do, return true if there is work
    {
        return sn_BasicNetworkSystem.Select( 0.1 );
    }
    virtual void Do()  //!< do the work.
    {
        tAdvanceFrame();
        sg_Receive();
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

    uMenuItemFunction lan
    (&net_menu,"$network_menu_lan_text",
     "$network_menu_lan_help",&gServerBrowser::BrowseLAN);

    uMenuItemFunction inter
    (&net_menu,"$network_menu_internet_text",
     "$network_menu_internet_help",&gServerBrowser::BrowseMaster);

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
    	if (eTeam::ongoingChallenge) {
    		int sets = sg_currentSettings->limitSet;
    		if (sets < 1) sets = 1;
            tOutput message;
            message.SetTemplateParameter(1, sets*2-1 );
            message << "$gamestate_reset_challenge";
            sn_CenterMessage(message);
    	} else {
	        sn_CenterMessage("$gamestate_reset_center");
	        sn_ConsoleOut("$gamestate_reset_console");
    	}
    }
}

static void StartNewMatch_conf(std::istream &){
    StartNewMatch();
}

static tConfItemFunc snm("START_NEW_MATCH",&StartNewMatch_conf);

// ************************************
// Challenge management
// ************************************

// start a challenge : store current game state, lock teams, and reset game ...
// end a challenge : restore previous game state and open team
static int previous_limitSet;
static void sg_startChallenge(int sets=2) {
	if (!(eTeam::ongoingChallenge)) {
		// no ongoing challenge, let start one ...
		eTeam::ongoingChallenge = true;
		previous_limitSet = sg_currentSettings->limitSet;
		sg_currentSettings->limitSet = sets;
		// lock all teams
	    for ( int i = eTeam::teams.Len()-1; i>=0; --i )
	    {
	        (eTeam::teams(i))->SetLocked( true );
	    }
	    // start the challenge
	    StartNewMatch();
	} else {
		// there's already a challenge running ...
	}
}

static eLadderLogWriter sg_startChallengeWriter("START_CHALLENGE", true);
static eLadderLogWriter sg_endChallengeWriter("END_CHALLENGE", true);

static void sg_endChallenge() {
	if (eTeam::ongoingChallenge) {
       	sg_endChallengeWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S");
       	sg_endChallengeWriter.write();
		// end the current challenge ...
		eTeam::ongoingChallenge = false;
		sg_currentSettings->limitSet = previous_limitSet;
		// unlock all teams
	    for ( int i = eTeam::teams.Len()-1; i>=0; --i )
	    {
	        (eTeam::teams(i))->SetLocked( false );
	    }
	} else {
		// no challenge to end ...
	}
}

static void StartChallenge_conf(std::istream &s){
    // read nb sets
    REAL sets;
    s >> sets;

	if (sets<1) sets=1;

    sg_startChallenge(sets);
}

static tConfItemFunc ssc("START_CHALLENGE",&StartChallenge_conf);

static void EndChallenge_conf(std::istream &s){
    sg_endChallenge();
}

static tConfItemFunc sec("END_CHALLENGE",&EndChallenge_conf);

static eLadderLogWriter sg_Shutdown("SHUTDOWN", true);

#ifdef DEDICATED
static void Quit_conf(std::istream &){

    if ( sg_currentGame )
    {
        sg_currentGame->NoLongerGoOn();
    }

    // mark end of recording
    tRecorder::Playback("END");
    tRecorder::Record("END");
    uMenu::quickexit = uMenu::QuickExit_Total;

    // write to ladderlog.txt the time of server shutting down
    sg_Shutdown << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
    sg_Shutdown.write();
}

static tConfItemFunc quit_conf("QUIT",&Quit_conf);
static tConfItemFunc exit_conf("EXIT",&Quit_conf);
#endif

void st_PrintPathInfo(tOutput &buf);

static void PlayerLogIn()
{
    ePlayer::LogIn();

    if ( sg_IngameMenu )
    {
        sg_IngameMenu->Exit();
    }

    con << tOutput( "$player_authenticate_action" );
}

void sg_DisplayVersionInfo() {
    tOutput versionInfo;
    versionInfo << "$version_info_version" << "\n";
    st_PrintPathInfo(versionInfo);
    versionInfo << "$version_info_misc_stuff";

    versionInfo << "$version_info_gl_intro";
    versionInfo << "$version_info_gl_vendor";
    versionInfo << gl_vendor;
    versionInfo << "$version_info_gl_renderer";
    versionInfo << gl_renderer;
    versionInfo << "$version_info_gl_version";
    versionInfo << gl_version;

    sg_ClientFullscreenMessage("$version_info_title", versionInfo, 1000);
}

void sg_StartupPlayerMenu();

void MainMenu(bool ingame){
    //	update_settings();

    if (ingame)
    {
        sr_con.SetHeight(2);
        se_UserShowScores( false );
    }

    gLogo::SetDisplayed(true);

    tOutput gametitle;
    if (!ingame)
        gametitle << "$game_menu_text";
    else
        gametitle << "$game_menu_ingame_text";

    uMenu game_menu(gametitle);

    uMenuItemFunction *reset=NULL;

    if(ingame && sn_GetNetState()!=nCLIENT){
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
        start= new uMenuItemFunction(&game_menu,"$game_menu_start_text",
                                     "$game_menu_start_help",&sg_SinglePlayerGame);
        connect=new uMenuItemFunction
                (&game_menu,
                 "$network_menu_text",
                 "$network_menu_help",
                 &net_game);

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

    uMenuItemFunction * auth = 0;
    static nVersionFeature authentication( 15 );
    if ( sn_GetNetState() == nCLIENT && ingame && authentication.Supported(0) )
    {
        auth =tNEW(uMenuItemFunction)(&MainMenu,
                                      "$player_authenticate_text",
                                      "$player_authenticate_help",
                                      &PlayerLogIn );
    }

    uMenuItemFunction abb(&MainMenu,
                          "$main_menu_about_text",
                          "$main_menu_about_help",
                          &sg_DisplayVersionInfo);


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

    uMenuItemFunction first_setup
    (&misc,"$misc_initial_menu_title",
     "$misc_initial_menu_help",
     &sg_StartupPlayerMenu);

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

    if ( auth )
    {
        delete auth;
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
    catch( ... )
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
    virtual int DoGetPriority() const{ return -100; }

    // don't actually filter; take line and add it to the potential error message
    virtual void DoFilterLine( tString &line )
    {
        message_ << line << "\n";
    }
};

static void sg_ParseMap ( gParser * aParser, tString mapfile )
{
    FILE* mapFD = NULL;

    gMapLoadConsoleFilter consoleLog;
#ifdef DEBUG
    /*
      con << "Message stats:\n";
      for(int i=0;i<40;i++){
      con << i << ":" << sent_per_messid[i] << '\n';
      sent_per_messid[i]=0;
      }
    */

    con << tOutput( "$map_file_loading", mapfile );
#endif
    mapFD = tResourceManager::openResource(mapuri, mapfile);

    if (!mapFD || !aParser->LoadAndValidateMapXML("", mapFD, mapfile))
    {
        if (mapFD)
            fclose(mapFD);

        tOutput errorMessage( sn_GetNetState() == nCLIENT ? "$map_file_load_failure_server" : "$map_file_load_failure_self", mapfile );

#ifndef DEDICATED
        errorMessage << "\nLog:\n" << consoleLog.message_;
#endif

        tOutput errorTitle("$map_file_load_failure_title");

        if ( sn_GetNetState() != nSTANDALONE )
        {
            throw tGenericException( errorMessage, errorTitle );
        }

        mapFD = tResourceManager::openResource("", DEFAULT_MAP);
        if (!mapFD || !aParser->LoadAndValidateMapXML("", mapFD, DEFAULT_MAP)) {
            if (mapFD)
                fclose(mapFD);
            errorMessage << "$map_file_load_failure_default";
            throw tGenericException( errorMessage, errorTitle );
        }
    }

    if (mapFD)
        fclose(mapFD);
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
    Arena.LeastDangerousSpawnPoint();
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
        uMenu::quickexit=uMenu::QuickExit_Total;
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

        while(goon && tSysTimeFloat()<timeout){
            /*
              if (sg_currentGame)
              sg_currentGame->RequestSync();
            */

            sn_Sync(sg_Timeout*.1,true);
            NetSyncIdle();

            goon=false;
            for(int i=MAXCLIENTS;i>0;i--)
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

static eLadderLogWriter sg_newRoundWriter("NEW_ROUND", true);
static eLadderLogWriter sg_newSetWriter("NEW_SET", true);
static eLadderLogWriter sg_newMatchWriter("NEW_MATCH", true);
static eLadderLogWriter sg_waitForExternalScriptWriter("WAIT_FOR_EXTERNAL_SCRIPT", true);
static eLadderLogWriter sg_nextRoundWriter("NEXT_ROUND", false);
static eLadderLogWriter sg_roundCommencingWriter("ROUND_COMMENCING", false);
static SvgOutput sg_svgOutput;

bool sg_roundStartingChecker = true;

static eLadderLogWriter sg_currentMapWriter("CURRENT_MAP", false);
static bool sg_displayMapDetails = false;
static tSettingItem<bool> sg_displayMapDetailsConf("DISPLAY_MAP_DETAILS", sg_displayMapDetails);

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
                //sn_SetNetState( nSTANDALONE );
                goon = false;
            }

            se_ChatState( ePlayerNetID::ChatFlags_Away, 1 );
        }
	#endif

        // unsigned short int mes1 = 1, mes2 = 1, mes3 = 1, mes4 = 1, mes5 = 1;

        switch(state){
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

            ePlayerNetID::ApplySubstitutions();

            // log scores before players get renamed
            ePlayerNetID::LogScoreDifferences();

            if ((sg_mapQueing.Size() == 0) && (sg_configQueing.Size() == 0))
            {
                // rotate, if rotate is once per round
                if ( rotationtype == gROTATION_ORDERED_ROUND )
                    Orderedrotate();
                else if (rotationtype == gROTATION_RANDOM_ROUND)
                    Randomrotate();
            }
            else
            {
                QueRotate();
            }

            gRotation::HandleNewRound();

            // transfer game settings
            if ( nCLIENT != sn_GetNetState() )
            {
                update_settings( &goon );
                ePlayerNetID::RemoveChatbots();
            }

            rViewport::Update(MAX_PLAYERS);

            ePlayerNetID::UpdateSuspensions();
            ePlayerNetID::UpdateShuffleSpamTesters();

            sg_newRoundWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
            sg_newRoundWriter.write();
            if ( rounds < 0 )
            {
                sg_newMatchWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
                sg_newMatchWriter.write();
            }

            // kick spectators
            nMachine::KickSpectators();

            // rename players as per request
            if ( synced_ && sn_GetNetState() != nSERVER )
                ePlayerNetID::Update();

            // delete game objects again (in case new ones were spawned)
            exit_game_objects(grid);

            // if the map has changed on the server side, verify it
            static tString lastMapfile( DEFAULT_MAP );
            if ( nCLIENT != sn_GetNetState() && mapfile != lastMapfile )
            {
                try
                {
                    Verify();
                }
                catch (tException const & e)
                {
                    // clean up
                    exit_game_grid( grid );

                    // inform user of generic errors
                    tConsole::Message( e.GetName(), e.GetDescription(), 120000 );

                    // revert map
                    con << tOutput( "$map_file_reverting", lastMapfile );
                    conf_mapfile.Set( lastMapfile );
                }
            }

            sg_currentMapWriter << mapfile;
            sg_currentMapWriter.write();

            sg_currentMap = mapfile;

            nConfItemBase::s_SendConfig(false);
            // wait extra long for the clients to delete the grid; they really need to be
            // synced this time
            sn_Sync( sg_Timeout*.4, true );

            SetState(GS_CREATE_GRID,GS_CAMERA);
            break;

        case GS_CREATE_GRID:
            // sr_con.autoDisplayAtNewline=true;

            //HACK RACE begin
            if ( sg_RaceTimerEnabled )
            {
                gRace::Reset();
            }
            //HACK RACE end

            sg_ParseMap( aParser );

            sn_Statistics();
            sg_Timestamp();

            con << tOutput("$gamestate_creating_grid");

            tAdvanceFrame();

            exit_game_grid(grid);
            init_game_grid(grid, aParser);

            nNetObject::ClearAllDeleted();

            if (sg_svgOutputFreq >= 0) sg_svgOutput.Create();

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
            eTeam::WriteLaunchPositions();

            // do round begin stuff
            {
                const tList<eGameObject>& gameObjects = Grid()->GameObjects();
                for (int i=gameObjects.Len()-1;i>=0;i--)
                {
                    eGameObject * e = gameObjects(i);
                    if ( e )
                    {
                        e->OnRoundBegin();
                    }
                }
            }

            // do the first analysis of the round, now is the time to get it used to the number of teams
            Analysis( -1000 );

            s_Timestep(grid, se_GameTime(), false);
            SetState(GS_TRANSFER_OBJECTS,GS_CAMERA);

            if (sn_GetNetState() == nCLIENT && just_connected){
                sn_Sync(sg_Timeout*.1,true);
                sn_Sync(sg_Timeout*.1,true);
            }
            just_connected=false;

            break;
        case GS_TRANSFER_OBJECTS:
            // con << "Transferring objects...\n";
            rITexture::LoadAll();
            // se_ResetGameTimer();
            // se_PauseGameTimer(true);


            // push all data
            if(sn_GetNetState()==nSERVER){
                bool goon=true;
                double timeout=tSysTimeFloat()+sg_Timeout*.4;
                while(goon && tSysTimeFloat()<timeout){
                    NetSyncIdle();
                    goon=false;
                    for(int i=MAXCLIENTS;i>0;i--)
                        if (sn_Connections[i].socket)
                            for(int j=sn_netObjects.Len()-1;j>=0;j--)
                                if (sn_netObjects(j) &&
                                        !sn_netObjects(j)->HasBeenTransmitted(i) &&
                                        sn_netObjects(j)->syncRequested(i))
                                    goon=true;
                }
                if (tSysTimeFloat()<timeout)
                    con << tOutput("$gamestate_done");
                else{
                    con << tOutput("$gamestate_timeout_intro");
                    for(int i=MAXCLIENTS;i>0;i--)
                        if (sn_Connections[i].socket)
                            for(int j=sn_netObjects.Len()-1;j>=0;j--)
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
                    int setsPlayed = eTeam::SetsPlayed ( );
                    // first challenge log message if needed
                    if (eTeam::ongoingChallenge && setsPlayed==0) {
                    	sg_startChallengeWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
                    	sg_startChallengeWriter.write();
                    }
                    // second match log message if needed
                    StartNewMatchNow();
                    if (setsPlayed==0)
                    	se_SaveToScoreFile("$gamestate_resetnow_log");
                    // third set log message if needed
                    if (sg_currentSettings->limitSet>1) {
                       	tOutput mess;
                       	mess.SetTemplateParameter(1, setsPlayed+1);
                       	mess << "$gamestate_set_start_console";
						se_SaveToScoreFile(mess);
				        sg_newSetWriter << (setsPlayed+1) << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
				        sg_newSetWriter.write();
                    }
                    // finally, center message
                    if ((sg_currentSettings->limitSet>1) && (setsPlayed>0)) {
                    	sn_CenterMessage("$gamestate_set_start_center");
                    } else {
                    	sn_CenterMessage("$gamestate_resetnow_center");
                    }
                }

                tOutput mess;
                if (rounds < sg_currentSettings->limitRounds)
                {
                    mess.SetTemplateParameter(1, rounds+1);
                    mess.SetTemplateParameter(2, sg_currentSettings->limitRounds);
                    mess << "$gamestate_newround_console";

                    if (strlen(sg_roundConsoleMessage) > 2)
                        sn_ConsoleOut(sg_roundConsoleMessage + "\n");
				    sg_nextRoundWriter << rounds+1 << sg_currentSettings->limitRounds << mapfile << sg_roundCenterMessage;
				    sg_nextRoundWriter.write();
                }
                else
                    mess << "$gamestate_newround_goldengoal";
                sn_ConsoleOut(mess);

                se_SaveToScoreFile("$gamestate_newround_log");

                if (sg_displayMapDetails)
                {
                    tColoredString Output;
                    Output << "0xff6622Map Name   : 0x999999" << pz_mapName << "\n";
                    Output << "0xff6622Map Author : 0x999999" << pz_mapAuthor << "\n";
                    Output << "0xff6622Map Version: 0x999999" << pz_mapVersion << "\n";
                    sn_ConsoleOut(Output);
                }
            }
            //con << ePlayerNetID::Ranking();

            se_PauseGameTimer(false);
            sg_SoundPause( false, false );
            se_SyncGameTimer();
            sr_con.fullscreen=false;
            sr_con.autoDisplayAtNewline=false;

            break;
        case GS_PLAY:
            sg_SoundPause( true, false );
            sr_con.autoDisplayAtNewline=false;
#ifdef DEDICATED
            {
                // save current players into a file
                cp();

                if ( sg_NumUsers() <= 0 )
                    goon = 0;

                Analysis(0);

                delayedCommands::Clear();

                // log scores before players get renamed
                //ePlayerNetID::LogScoreDifferences();

                //ePlayerNetID::RankingLadderLog();

                sg_roundCommencingWriter << (rounds <= 0 ? 1 : rounds+1) << sg_currentSettings->limitRounds;
                sg_roundCommencingWriter.write();

/*                tOutput Message;
                Message << "The rounds: " << rounds << "\n";
                sn_ConsoleOut(Message);
*/
                // wait for external script to end its work if needed
                REAL timeout = tSysTimeFloat() + sg_waitForExternalScriptTimeout;
                if ( sg_waitForExternalScript )
                {
                    sg_waitForExternalScriptWriter.write();
                    // REAL waitingSince = tSysTimeFloat();
                }
                while ( sg_waitForExternalScript && timeout > tSysTimeFloat())
                {
                    sr_Read_stdin();

                    // wait for network messages
                    sn_BasicNetworkSystem.Select( 0.1f );
                    gGame::NetSyncIdle();
                }

                {
                    // default include files are executed at owner level
                    tCurrentAccessLevel level( tAccessLevel_Owner, true );

                    std::ifstream s;

                    // load contents of everytime.cfg for real
                    static const tString everytime("everytime.cfg");
                    if ( tConfItemBase::OpenFile(s, everytime, tConfItemBase::Config ) )
                        tConfItemBase::ReadFile(s);

                    s.close();

                    if ( tConfItemBase::OpenFile(s, everytime, tConfItemBase::Var ) )
                        tConfItemBase::ReadFile(s);
                }
            }
#endif

            sg_roundStartingChecker = true;

            // pings should not count as much in the between-round phase
            nPingAverager::SetWeight(1E-20);

            // se_UserShowScores(false);

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

            gHighscoresBase::SaveAll();
            //HACK RACE begin
            if ( sg_RaceTimerEnabled )
            {
                gRaceScores::Write();
            }
            //HACK RACE end
            con << tOutput("$gamestate_deleting_objects");
            exit_game_objects(grid);
            st_ToDo( sg_VoteMenuIdle );
            nNetObject::ClearAllDeleted();
            SetState(GS_DELETE_GRID,GS_TRANSFER_SETTINGS);

            break;
        default:
            break;
        }

        // now would be a good time to tend for pending tasks
        if( state != GS_PLAY )
        {
            nAuthentication::OnBreak();
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

static float sg_respawnTime = -1;
static tSettingItem<float> sg_respawnTimeConfig( "RESPAWN_TIME", sg_respawnTime );

static bool respawn_all=false;

static void respawnallenable(std::istream &s)
{
    respawn_all = true;
}
static tConfItemFunc sg_respawnall_conf("RESPAWN_ALL", respawnallenable);

// uncomment to activate respawning
#define RESPAWN_HACK

#ifdef RESPAWN_HACK
// Respawns cycles (crude test)
static void sg_Respawn( REAL time, eGrid *grid, gArena & arena )
{
    if (sg_respawnTime < 0)
    {
        return;
    }

    for ( int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID *p = se_PlayerNetIDs(i);

        if ( !p->CurrentTeam() )
            continue;

        eGameObject *e=p->Object();

        if ( ( (!e) || ((!e->Alive()) && (e->DeathTime() < (time - sg_respawnTime)) ) && (sn_GetNetState() != nCLIENT )))
        {
            eCoord pos,dir;
#if 0
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
#endif
                arena.LeastDangerousSpawnPoint()->Spawn( pos, dir );
#ifdef DEBUG
            //                std::cout << "spawning player " << pni->name << '\n';
#endif
            gCycle * cycle = new gCycle( grid, pos, dir, p );
            p->ControlObject(cycle);

            sg_Timestamp();
        }
    }
}
#endif

#ifdef RESPAWN_HACK
// Respawns all cycles (crude test)
static void sg_RespawnAll(eGrid *grid, gArena & arena, bool respawn_all)
{
    if (respawn_all == false)
    {
        return;
    }

    for ( int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID *p = se_PlayerNetIDs(i);

        eGameObject *e=p->Object();

        if ((p && !p->IsActive()) || (!e || !e->Alive()))
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
                arena.LeastDangerousSpawnPoint()->Spawn( pos, dir );
#ifdef DEBUG
            //                std::cout << "spawning player " << pni->name << '\n';
#endif
            gCycle * cycle = new gCycle( grid, pos, dir, p );
            p->ControlObject(cycle);

            sg_Timestamp();
        }
    }
}
#endif

static REAL sg_timestepMax = .2;
static tSettingItem<REAL> sg_timestepMaxConf( "TIMESTEP_MAX", sg_timestepMax );
static int sg_timestepMaxCount = 10;
static tSettingItem<int> sg_timestepMaxCountConf( "TIMESTEP_MAX_COUNT", sg_timestepMaxCount );

void gGame::Timestep(REAL time,bool cam){
#ifdef DEBUG
    tMemManBase::Check();
#endif

#ifdef RESPAWN_HACK
    // no respawining while deathzone is active.
    if( !winDeathZone_)
    {
        sg_Respawn(time,grid,Arena);
    }
    // if respawn_all command is activated
    if(respawn_all == true)
    {
        sg_RespawnAll(grid,Arena,respawn_all);
        respawn_all = false;
    }
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
    for(int i=1;i<=number_of_steps;i++)
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

static eLadderLogWriter sg_roundWinnerWriter("ROUND_WINNER", true);
static eLadderLogWriter sg_setWinnerWriter("SET_WINNER", true);
static eLadderLogWriter sg_matchWinnerWriter("MATCH_WINNER", true);

static eLadderLogWriter sg_roundEndedWriter("ROUND_ENDED", true);
static eLadderLogWriter sg_matchEndedWriter("MATCH_ENDED", true);

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
    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
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
    // int last_alive=-1;
    int last_team_alive=-1;
    // int last_alive_and_not_disconnected=-1;
    int humans = 0;
    int active_humans = 0;
    int ais    = 0;
    REAL deathTime=0;

    bool notyetloggedin = true;  // are all current users not yet logged in?

    for(i=eTeam::teams.Len()-1;i>=0;i--){
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
                if(g){
                    notyetloggedin = false;

                    if(g->Alive())
                    {

                        if ( p->IsHuman() )
                        {
                            alive++;
                            if (p->IsActive())
                            {
                                // last_alive_and_not_disconnected=i;
                                alive_and_not_disconnected++;
                            }
                        }
                        else
                            ai_alive++;

                        // last_alive=i;
                    }
                    else
                    {
                        REAL dt=g->DeathTime();
                        if (dt>deathTime)
                            deathTime=dt;
                    }
                }
#ifdef KRAWALL_SERVER
                else if (p->IsAuthenticated())
                    notyetloggedin = false;
#endif
            }
        }
        else
        {
            for (int j=t->NumPlayers()-1; j>=0; --j)
            {
                ePlayerNetID* p = t->Player(j);

                gCycle *g=dynamic_cast<gCycle *>(p->Object());
                if(g){
                    if(g->Alive())
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

    /*
    if (last_alive_and_not_disconnected >= 0)
        last_alive = last_alive_and_not_disconnected;
    */

    // kill all disconnected players if they are the only reason the round goes on:
    if ( alive_and_not_disconnected <= 1 && alive > alive_and_not_disconnected )
    {
        for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            gCycle *g=dynamic_cast<gCycle *>(se_PlayerNetIDs(i)->Object());
            if(g && !se_PlayerNetIDs(i)->IsActive())
            {
                g->Kill();
            }

            //			alive = alive_and_not_disconnected;
        }
    }

    // keep the game running when there are only login processes running
    if (notyetloggedin && humans > 0 && ais == 0)
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

    // activate instant win zone
    if ( winZone.Supported() && !bool( winDeathZone_ ) && winner == 0 && time - lastdeath > sg_currentSettings->winZoneMinLastDeath && time > sg_currentSettings->winZoneMinRoundTime )
    {
        winDeathZone_ = sg_CreateWinDeathZone( grid, Arena.GetRandomPos( sg_winZoneRandomness ) );
    }

    bool holdBackNextRound = false;

    // start a new match if the number of teams changes from 1 to 2 or from 2 to 1
    {
        static int lastTeams = 0; // the number of teams when this was last called.

        // check for relevant status change, from 0 to 1 or 1 to 2 or 2 to 1 or 1 to 0 human teams.
        int humanTeamsClamp = human_teams;
        if ( humanTeamsClamp > 2 )
            humanTeamsClamp = 2;
        if ( humanTeamsClamp != lastTeams )
        {
            StartNewMatch();
            if (eTeam::ongoingChallenge) sg_endChallenge();
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

    //HACK RACE begin

    if ( sg_RaceTimerEnabled )
    {
        gRace::Sync( alive, ai_alive, sg_NumHumans() );
        if ( !gRace::Done() )
            return;
        else                                    // time to close the round
        {
            eTeam * team = gRace::Winner();

            if (team)
            {
                tOutput message;
                message.SetTemplateParameter(2, sg_scoreRaceComplete);
                message << "$player_win_race";
                sg_DeclareWinner( team, message );
            }
/*          else
            {
                if ( alive > 0 )
                {
                    //TODO
                    sg_DeclareWinner( NULL, 0 );
                }
            }*/
        }
    }
    //HACK RACE end

    // who is alive?
    if ( time-lastdeath < 1.0f )
    {
        holdBackNextRound = true;
    }
    else if (winner==0 && absolute_winner==0 ){
        if( teams_alive <= 1 )
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

                if ( last_team_alive >= 0 )
                    sg_currentSettings->AutoAI( eTeam::teams( last_team_alive )->NumHumanPlayers() > 0 );

                if ( sg_currentSettings->gameType!=gFREESTYLE && winner > 0 )
                {
                    // check if the win was legitimate: at least one enemy team needs to bo online
                    if ( sg_EnemyExists( winner-1 ) )
                    {
#ifdef KRAWALL_SERVER_LEAGUE
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
                        if (sg_RaceTimerEnabled) eTeam::teams[winner-1]->AddScore(sg_scoreRaceComplete,messagetype, tOutput());
                        else eTeam::teams[winner-1]->AddScore(sg_currentSettings->scoreWin,messagetype, tOutput());
                        if (!sg_singlePlayer &&
                                eTeam::teams(winner-1)->NumHumanPlayers() >= 1 &&
                                eTeam::teams(winner-1)->NumPlayers() >= 1 )
                        {
                            highscore_won_rounds.Add(eTeam::teams[winner-1] ,1);
                            highscore_ladder.winner(eTeam::teams(winner-1));
                        }

                        // print winning message
                        if( sg_currentSettings->scoreWin != 0 )
                        {
                            tOutput message;
                            message << "$gamestate_winner_winner";
                            message << eTeam::teams[winner-1]->Name();
                            sn_CenterMessage(message);
                            message << '\n';
                            se_SaveToScoreFile(message);
                        }

                        sg_roundWinnerWriter << ePlayerNetID::FilterName( eTeam::teams[winner-1]->Name() );
                        eTeam::WritePlayers( sg_roundWinnerWriter, eTeam::teams[winner-1] );
                        sg_roundWinnerWriter.write();

                        sg_roundEndedWriter <<  st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
                        sg_roundEndedWriter.write();

                        /*if (sg_RaceTimerEnabled)
                            gRace::End();*/
                    }
                }
                check_hs();
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
        if ( eTeam::teams.Len() > 0 ) {
            if (eTeam::teams.Len() <= 1
                    || ( eTeam::teams(0)->Score() > eTeam::teams(1)->Score() && sg_EnemyExists(0))){

                // only then we can have a true winner:
                if (
                        ( eTeam::teams(0)->Score() >= sg_currentSettings->limitScore
			&& eTeam::teams.Len()>1 && eTeam::teams(0)->Score() >= eTeam::teams(1)->Score() + sg_currentSettings->scoreDiffWin ) ||		// the score limit must be hit
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
						// handle sets before declaring the winner ...
						bool flagSets=false; // true is this is the end of a set instead of a match ...
						int setsPlayed=0;
                    	if (sg_currentSettings->limitSet>1) {
							eTeam::teams(0)->IncrementSets();
							setsPlayed = eTeam::SetsPlayed();
							flagSets= eTeam::teams(0)->SetsWon()<sg_currentSettings->limitSet ? true : false;
                    	}

                        lastdeath = time + ( sg_currentSettings->finishType==gFINISH_EXPRESS ? 2.0f : 6.0f );

                        tOutput message;
                        tColoredString name;
                        name << eTeam::teams[0]->Name();
                        name << tColoredString::ColorString(1,1,1);

						if (flagSets) {
	                        {
	                            tOutput message;
	                            message.SetTemplateParameter(1, setsPlayed );
	                            message.SetTemplateParameter(2, name );
	                            message << "$gamestate_set_center";
	                            sn_CenterMessage(message);
	                        }

	                        sg_setWinnerWriter << ePlayerNetID::FilterName( eTeam::teams[0]->Name() );
	                        sg_setWinnerWriter.write();

                            message.SetTemplateParameter(1, setsPlayed );
                            message.SetTemplateParameter(2, name );
	                        message << "$gamestate_set_console";
						} else {
	                        {
	                            tOutput message;
	                            message.SetTemplateParameter(1, eTeam::teams[0]->Name() );
	                            message << "$gamestate_champ_center";
	                            sn_CenterMessage(message);
	                        }

	                        sg_matchWinnerWriter << ePlayerNetID::FilterName( eTeam::teams[0]->Name() );
                            eTeam::WritePlayers( sg_matchWinnerWriter, eTeam::teams[0] );
                            sg_matchWinnerWriter.write();

                            sg_matchEndedWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
                            sg_matchEndedWriter.write();

	                        message.SetTemplateParameter(1, name);
	                        message << "$gamestate_champ_console";
						}

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

						ePlayerNetID::LogMatchScores();

                        se_SaveToScoreFile(message);

                        if (flagSets) {
                        	tOutput mess;
                        	mess.SetTemplateParameter(1, setsPlayed);
                        	mess << "$gamestate_set_scores";
                        	se_SaveToScoreFile(mess);
                        } else {
                        	se_SaveToScoreFile("$gamestate_champ_finalscores");
                    		eTeam::ResetAllSets();
                    		if (eTeam::ongoingChallenge) {
								sg_endChallenge();
                    		}
                        }
                        se_SaveToScoreFile(eTeam::Ranking( -1, false ));
                        se_SaveToScoreFile(ePlayerNetID::Ranking( -1, false ));
                        se_SaveToScoreFile(st_GetCurrentTime( "Time: %Y/%m/%d %H:%M:%S\n" ));
                        se_SaveToScoreFile("\n\n");

                        eTeam* winningTeam = eTeam::teams(0);
                        if (!sg_singlePlayer && winningTeam->NumHumanPlayers() > 0 )
                        {
                            highscore_won_matches.Add( winningTeam, 1 );
                        }

                        sn_ConsoleOut(message);
                        wintimer=time;
                        absolute_winner=1;

                        if ( se_mainGameTimer )
                            se_mainGameTimer->speed = 1;

                        if ((sg_mapQueing.Size() == 0) || (sg_configQueing.Size() == 0))
                        {
                            //check for map rotation, new match...
                            if ( rotationtype == gROTATION_ORDERED_MATCH )
                                Orderedrotate();
                            else if ( rotationtype == gROTATION_RANDOM_MATCH)
                                Randomrotate();
                        }

                        gRotation::HandleNewMatch();

                        StartNewMatch();
                    }
                }
            }
        }
    }


    // time to wait after last death til we start a new round
    REAL fintime=6;

    if (sg_currentSettings->finishType==gFINISH_EXPRESS)
        fintime=2.5;

    if(( ( winner || absolute_winner ) && wintimer+fintime < time)
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
            se_mainGameTimer->speed*=1.41;
            if(se_mainGameTimer->speed>16)
                se_mainGameTimer->speed=16;
        }

    }
}

void Orderedrotate()
{
    if ( sg_mapRotation.Size() > 0 )
    {
        conf_mapfile.Set( sg_mapRotation.Current() );
        conf_mapfile.GetSetting().SetSetLevel( sg_mapRotation.GetSetLevel() );
        sg_mapRotation.OrderedRotate();
    }

    if ( sg_configRotation.Size() > 0 )
    {
        // transfer
        tCurrentAccessLevel level( sg_configRotation.GetSetLevel(), true );

        if (sg_configRotationType == 0)
        {
            st_Include( sg_configRotation.Current() );
            sg_configRotation.OrderedRotate();
        }
        else if (sg_configRotationType == 1)
        {
            tString rclcl = tResourceManager::locateResource(NULL, sg_configRotation.Current());
            if ( rclcl ) {
                std::ifstream rc(rclcl);
                tConfItemBase::LoadAll(rc, false );
                sg_configRotation.OrderedRotate();
                return;
            }

            con << tOutput( "$config_rinclude_not_found", sg_configRotation.Current() );
        }
    }
}

void Randomrotate()
{
    if ( sg_mapRotation.Size() > 0 )
    {
        conf_mapfile.Set( sg_mapRotation.Current() );
        conf_mapfile.GetSetting().SetSetLevel( sg_mapRotation.GetSetLevel() );
        sg_mapRotation.RandomRotate();
    }

    if ( sg_configRotation.Size() > 0 )
    {
        // transfer
        tCurrentAccessLevel level( sg_configRotation.GetSetLevel(), true );

        if (sg_configRotationType == 0)
        {
            st_Include( sg_configRotation.Current() );
            sg_configRotation.RandomRotate();
        }
        else if (sg_configRotationType == 1)
        {
            tString rclcl = tResourceManager::locateResource(NULL, sg_configRotation.Current());
            if ( rclcl ) {
                std::ifstream rc(rclcl);
                tConfItemBase::LoadAll(rc, false );
                sg_configRotation.RandomRotate();
                return;
            }

            con << tOutput( "$config_rinclude_not_found", sg_configRotation.Current() );
        }
    }
}

void QueRotate()
{
    if (sg_mapQueing.Size() > 0)
    {
        conf_mapfile.Set(sg_mapQueing.Current());
        conf_mapfile.GetSetting().SetSetLevel( sg_mapQueing.GetSetLevel() );
        sg_mapQueing.Remove(sg_mapQueing.CurrentID());
        if (sg_mapQueing.Size() == 0)
        {
            tOutput Output;
            Output << "$map_queing_is_empty";
            sn_ConsoleOut(Output);
        }
    }
    if (sg_configQueing.Size() > 0)
    {
        tCurrentAccessLevel level(sg_configQueing.GetSetLevel(), true );
        if (sg_configRotationType == 0)
        {
            st_Include(sg_configQueing.Current());
            sg_configQueing.Remove(sg_configQueing.CurrentID());
            if (sg_configQueing.Size() == 0)
            {
                tOutput Output;
                Output << "$config_queing_is_empty";
                sn_ConsoleOut(Output);
            }
        }
        else if (sg_configRotationType == 1)
        {
            tString rclcl = tResourceManager::locateResource(NULL, sg_configQueing.Current());
            sg_configQueing.Remove(sg_configQueing.CurrentID());
            if ( rclcl ) {
                std::ifstream rc(rclcl);
                tConfItemBase::LoadAll(rc, false );
                return;
            }

            con << tOutput( "$config_rinclude_not_found", sg_configQueing.Current() );
        }
    }
}

void gGame::StartNewMatch(){
    check_hs();
    rounds=-100;

}

void gGame::StartNewMatchNow(){
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
static uActionTooltip ingamemenuTooltip( ingamemenu, 1 );
#endif // dedicated

static eLadderLogWriter sg_gameTimeWriter("GAME_TIME", true);
static eLadderLogWriter sg_roundStartedWriter("ROUND_STARTED", true);

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
		    if (cd==1) ePlayerNetID::GridPosLadderLog();
            lastcountdown=cd;
            tString s;
            s << cd;
            con.CenterDisplay(s,0);
        }
    }
    //con << sg_netPlayerWalls.Len() << '\n';

#ifndef DEDICATED
    if (input){
        su_HandleDelayedEvents();

        SDL_Event tEvent;

        uInputProcessGuard inputProcessGuard;
        while(su_GetSDLInput(tEvent,time)){
            if (time>gtime) time=gtime;
            if(time>lastTime_gameloop){
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

    // do basic network receiving and sending. This pushes out all input the player makes as fast as possible.
    sg_Receive();
    sn_SendPlanned();

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

        // and trigger the scheduled auto-logons.
        ePlayer::SendAuthNames();

        synced_ = true;
    }

	delayedCommands::Run(gtime);
	if (sg_roundStartingChecker)
	{
        sg_roundStartedWriter << st_GetCurrentTime("%Y-%m-%d %H:%M:%S %Z");
        sg_roundStartedWriter.write();

        if (sg_RaceTimerEnabled)
        {
            gRaceScores::OutputStart();
        }
        sg_roundStartingChecker = false;
	}

    {
        static float lastTime = 1e42;
        if(sg_gameTimeInterval >= 0 && (gtime >= lastTime + sg_gameTimeInterval || gtime < lastTime)) {
            double time;
            if (sg_gameTimeInterval >0){
                time = (floor(gtime/sg_gameTimeInterval))*sg_gameTimeInterval;
            } else{
                time = gtime;
            }
            sg_gameTimeWriter << time;
            sg_gameTimeWriter.write();
            lastTime = time;
        }
    } {
        static float lastTime = 1e42;
        if((gtime > sg_playerPositioningStartTime) && (sg_tacticalPositionInterval >= 0) && (gtime >= lastTime + sg_tacticalPositionInterval || gtime < lastTime)) {
            gCycle::TacticalPositioning(gtime);
            lastTime = gtime;
        }
    } {
        static float lastTime = 1e42;
        if((sg_gridPosInterval >= 0) && (gtime > sg_gridPosInterval) && (gtime >= lastTime + sg_gridPosInterval + 1 || gtime < lastTime)) {
            ePlayerNetID::GridPosLadderLog();
            lastTime = gtime;
        }
    } {
        static float lastTime = 1e42;
        if(sg_svgOutputFreq >= 0 && (gtime >= lastTime + sg_svgOutputFreq || gtime < lastTime)) {
            sg_svgOutput.Create();
            if(sg_svgOutputScoreDifferences) {
                ePlayerNetID::LogScoreDifferences();
            }
            lastTime = gtime;
        }
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
                        //sn_SetNetState( nSTANDALONE );
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
            for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
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

        // send game object updates
        nNetObject::SyncAll();
        sn_SendPlanned();

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
        // send game object updates
        nNetObject::SyncAll();
        sn_SendPlanned();

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
    if(sn_GetNetState()==nSERVER){
        REAL t=tSysTimeFloat();
        for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--){
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
    if(sg_currentGame){
        tControlledPTR< nNetObject > keep( sg_currentGame );
        sg_currentGame->StateUpdate();
        goon=!uMenu::quickexit && bool(sg_currentGame) && sg_currentGame->GameLoop(input);
    }
    return goon;
}

void gameloop_idle()
{
    // se_UserShowScores( false );
    sg_Receive();
    nNetObject::SyncAll();
    sn_SendPlanned();
    GameLoop(false);
}

static eSoundPlayer introPlayer(intro);
static eSoundPlayer extroPlayer(extro);

static void sg_EnterGameCleanup();

void sg_EnterGameCore( nNetState enter_state ){
    se_UserShowScores( false );

    sg_RequestedDisconnection = false;

    sr_con.SetHeight(7);

    extroPlayer.End();
    introPlayer.MakeGlobal();
    introPlayer.Reset();

    sg_SoundPause( true, false );

    //  exit_game_objects(grid);

    // enter single player settings
    if ( sn_GetNetState() != nCLIENT )
    {
        sg_currentSettings = &singlePlayer;
        sg_copySettings();
    }

    gHighscoresBase::LoadAll();

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
            sg_currentGame->StateUpdate();
            REAL time=se_GameTime();
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

        goon=GameLoop();

        st_DoToDo();
    }

    sg_SoundPause( false, false );

    extroPlayer.MakeGlobal();
    extroPlayer.Reset();

    sg_EnterGameCleanup();
}

void sg_EnterGameCleanup()
{
    gHighscoresBase::SaveAll();
    //HACK RACE begin
    if ( sg_RaceTimerEnabled )
    {
        gRaceScores::Write();
    }
    //HACK RACE end

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

    rModel::ClearCache();
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

// another fullscreen fix ammendmend: avoid the short screen flicker
// by ignoring deactivation events in fullscreen mode completely.
#ifndef WIN32
#ifndef MACOSX
    if ( currentScreensetting.fullscreen && !act )
    {
        return;
    }
#endif
#endif

    sr_Activate( act );

    if (!sr_keepWindowActive || act)
    {
        sg_SoundPause( !act, true );
    }

    if ( !tRecorder::IsRunning() )
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

    // stop the game
    bool paused = se_mainGameTimer && se_mainGameTimer->speed < .0001;
    if( sn_GetNetState() != nCLIENT )
    {
        se_PauseGameTimer(true);
    }

    // put players into idle mode
    ePlayerNetID::SpectateAll();
    se_ChatState( ePlayerNetID::ChatFlags_Menu, true );

    // remove scores
    se_UserShowScores( false );

    // show message
    uMenu::Message( title, message, timeout );

    // and print it to the console
#ifndef DEDICATED
    con <<  title << "\n" << message << "\n";
#endif


    // continue the game
    if( sn_GetNetState() != nCLIENT )
    {
        se_PauseGameTimer(paused);
    }

    // get players out of idle mode again
    ePlayerNetID::SpectateAll(false);
    se_ChatState( ePlayerNetID::ChatFlags_Menu, false );
}

static tString sg_fullscreenMessageTitle;
static tString sg_fullscreenMessageMessage;
static REAL sg_fullscreenMessageTimeout = 0.0;
static void sg_TodoClientFullscreenMessage()
{
    sg_ClientFullscreenMessage( sg_fullscreenMessageTitle, sg_fullscreenMessageMessage, sg_fullscreenMessageTimeout );
}

static void sg_ClientFullscreenMessage(nMessage &m){
    if (sn_GetNetState()!=nSERVER){
        // to test timeouts during fullscreen message display:
        // m.Send( m.SenderID() );

        sg_fullscreenMessageTimeout = 60;

        m >> sg_fullscreenMessageTitle;
        m >> sg_fullscreenMessageMessage;
        m >> sg_fullscreenMessageTimeout;

        st_ToDo( sg_TodoClientFullscreenMessage );
    }
}

static nDescriptor sg_clientFullscreenMessage(312,sg_ClientFullscreenMessage,"client_fsm");

void sg_FullscreenMessageWait()
{
    if( sn_GetNetState() != nSERVER )
    {
        return;
    }
    // wait for the clients to have seen the message
    {
        // stop the game
        bool paused = se_mainGameTimer && se_mainGameTimer->speed < .0001;
        se_PauseGameTimer(true);
        gGame::NetSyncIdle();

        REAL waitTo = tSysTimeFloat() + sg_fullscreenMessageTimeout;
        REAL waitToMin = tSysTimeFloat() + 1.0;

        // wait for players to see it
        bool goon = true;
        while( goon && waitTo > tSysTimeFloat() )
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
            for(int c = MAXCLIENTS; c > 0; --c)
            {
                if ( sn_Connections[c].socket )
                {
                    if ( sg_fullscreenMessages.Supported(c) )
                    {
                        m->Send(c);
                    }
                    else
                    {
                        sn_ConsoleOut(complete, c);
                    }
                }
            }
        }

        double before = tSysTimeFloat();
        sg_ClientFullscreenMessage( title, message, timeout );
        double after = tSysTimeFloat();

        // store rest of timeout and wait for the clients.
        sg_fullscreenMessageTimeout = timeout - (after-before);
        st_ToDo( sg_FullscreenMessageWait );
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

    // flag indicating wether we were in client bode
    static bool wasClient = false;

    // lost connection to server/disconnected
    if( wasClient && !nCallbackLoginLogout::Login() && sn_GetNetState() == nSTANDALONE )
    {
        // drop all menu activity
        if ( !uMenu::quickexit )
        {
            uMenu::quickexit = uMenu::QuickExit_Game;
        }
    }

    wasClient = nCallbackLoginLogout::Login() && nCallbackLoginLogout::User() == 0;
}

static nCallbackLoginLogout lc(LoginCallback);

static void sg_FillServerSettings()
{
    nServerInfo::SettingsDigest & digest = *nCallbackFillServerInfo::ToFill();

    digest.SetFlag( nServerInfo::SettingsDigest::Flags_NondefaultMap,
                    mapfile != DEFAULT_MAP );
    digest.SetFlag( nServerInfo::SettingsDigest::Flags_TeamPlay,
                    multiPlayer.maxPlayersPerTeam > 1 );

    digest.wallsLength_ = multiPlayer.wallsLength/gCycleMovement::MaximalSpeed();
}

static nCallbackFillServerInfo sg_fillServerSettings(sg_FillServerSettings);

static void sg_DeclareRoundWinner(std::istream &s)
{
	tString params;
    params.ReadLine( s, true );
    int pos = 0;
    ePlayerNetID *winningPlayer = 0;
    tString targetPlayer = params.ExtractNonBlankSubString(pos);
    winningPlayer = ePlayerNetID::FindPlayerByName(targetPlayer, NULL);
    if(!winningPlayer)
        {
            return;
        }
    static const char* message="$player_win_command";
    sg_DeclareWinner( winningPlayer->CurrentTeam(), message );
}

static tConfItemFunc sg_DeclareRoundWinner_conf("DECLARE_ROUND_WINNER",&sg_DeclareRoundWinner);

static void sg_KillAllPlayers(std::istream &s)
{
    if (se_PlayerNetIDs.Len()>0){
        int max = se_PlayerNetIDs.Len();
        for(int i=0;i<max;i++){
            ePlayerNetID *p=se_PlayerNetIDs(i);
            if (p->IsActive() && p->Object() && p->Object()->Alive() )
            {
                p->Object()->Kill();
            }
        }
    }
}

static tConfItemFunc sg_KillAllPlayers_conf("KILL_ALL", &sg_KillAllPlayers);
