/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "gTutorial.h"

#include "tLinkedList.h"
#include "tConfiguration.h"

#include "uMenu.h"

#include "ePlayer.h"
#include "eTeam.h"

#include "gGame.h"
#include "gCycle.h"
#include "gAIBase.h"

// temporarily override a setting item
class gTemporarySetting: public tReferencable< gTemporarySetting >
{
public:
    gTemporarySetting( char const * setting, char const * value )
    : confItem_( tConfItemBase::FindConfigItem( tString(setting) ) )
    {
        tASSERT( confItem_ );
        std::ostringstream o;
        confItem_->WriteVal( o );
        backup_ = o.str();
        
        std::istringstream i( value );
        confItem_->ReadVal( i );
    }
    
    ~gTemporarySetting()
    {
        std::istringstream i( backup_ );
        confItem_->ReadVal( i );
    }
private:
    tConfItemBase * confItem_;
    std::string backup_;
};

static gGameSettings sg_DefaultSettings()
{
    return gGameSettings(10,
                         30, 999, 100000, 100000,
                         0,   0, 30,
                         false, false,
                         0  ,  -2,
                         gDUEL, gFINISH_IMMEDIATELY, 1,
                         100000, 1000000);
 
}

/*
static bool sg_TestMatch()
{
    uMenu::Message( tOutput("$welcome_message_heading"), tOutput("$welcome_message"), 300 );

    // start a first single player game
    gGameSettings settings = sg_DefaultSettings();
    settings.speedFactor = -2;
    settings.numAIs = 1;
    sg_TutorialGame( settings, "Test" );

    uMenu::Message( tOutput("$welcome_message_2_heading"), tOutput("$welcome_message_2"), 300 );

    return true;
}
*/

class gTutorialMenuItem;

void gTutorialBase::OnWin( eTeam * winner )
{
}

void gTutorialBase::RoundEnd( eTeam * winner )
{
}

// called before objects are spawned
void gTutorialBase::BeforeSpawn(){}

    // called after objects are spawned
void gTutorialBase::AfterSpawn(){}
    

extern uActionTooltip::Level su_helpLevel;

//! tutorial class
class gTutorial: public tListItem< gTutorial >, public gTutorialBase
{
public:
    gTutorial( char const * name )
    : tListItem< gTutorial >( anchor_ )
      , settings_( sg_DefaultSettings() )
      , completed_( false )
      , completedConf_( tString("TUTORIAL_COMPLETE_") + tString(name).ToUpper(), completed_ )
      , name_( name )
      , success_( false )
      , finished_( false )
    {
        CreateMenu();
    }

    // call on success or failure during analysis
    void End( bool success )
    {
        success_ = success;
        finished_ = true;
        sg_DeclareWinner( &HumanTeam(), NULL );
    }

    // always exit after one round
    void RoundEnd( eTeam * winner )
    {
        finished_ = true;
    }

    // always exit after one round
    void OnWin( eTeam * winner )
    {
        finished_ = true;
        if( winner && winner->OldestHumanPlayer() )
        {
            sn_CenterMessage(tOutput("$tutorial_success"));
            success_ = true;
        }
    }

    void CreateMenu();

    // shows instructions
    virtual void Instructions()
    {
        uMenu::Message(
            tOutput( (tString("$tutorial_menu_") + Name() + "_text" ).c_str()),
            tOutput( (tString("$tutorial_menu_") + Name() + "_instructions" ).c_str()),
            300,
            tOutput( (tString("$tutorial_menu_") + Name() + "_animation" ).c_str() )
            );
    }

    // runs the tutorial
    virtual bool RunCore()
    {
        sg_TutorialGame( settings_, *this );
        return success_;
    }

    // temporarily alter a setting
    void PushSetting( char const * setting, char const * value )
    {
        tempSettings_.push_back( tNEW(gTemporarySetting)( setting, value ) );
    }

    // find human team
    eTeam & HumanTeam()
    {
        for( int i = eTeam::teams.Len()-1; i >= 0; --i )
        {
            if( eTeam::teams[i]->IsHuman() )
            {
                return *eTeam::teams[i];
            }
        }

        tASSERT( false );
        
        static tJUST_CONTROLLED_PTR< eTeam > emergency = tNEW( eTeam );
        return *emergency;
    }

    // find AI team
    eTeam & AITeam()
    {
        for( int i = eTeam::teams.Len()-1; i >= 0; --i )
        {
            if( 0 == eTeam::teams[i]->NumHumanPlayers() )
            {
                return *eTeam::teams[i];
            }
        }

        tASSERT( false );
        
        static tJUST_CONTROLLED_PTR< eTeam > emergency = tNEW( eTeam );
        return *emergency;
    }

    ePlayerNetID & HumanPlayer()
    {
        eTeam & team = HumanTeam();
        return *team.OldestHumanPlayer();
    }

    ePlayerNetID & AIPlayerRaw()
    {
        eTeam & team = AITeam();
        return *team.OldestPlayer();
    }

    gAIPlayer & AIPlayer()
    {
        return dynamic_cast< gAIPlayer & >( AIPlayerRaw() );
    }

    void AddPath( gAIPlayer & ai, REAL x, REAL y, bool mindless = true )
    {
        ai.AddToPath( tCoord( x, y ) * pow( 2, settings_.sizeFactor*.5 ) );
    }

    gCycle * HumanCycle()
    {
        return dynamic_cast< gCycle * >( HumanPlayer().Object() );
    }

    // restore settings
    void Restore()
    {
        su_helpLevel = uActionTooltip::Level_Expert;

        // destroy settings in deterministic order
        while( tempSettings_.size() )
            tempSettings_.pop_back();
    }

    // prepares
    virtual void Prepare()
    {
        finished_ = false;

        // default map
        PushSetting( "MAP_FILE", "Anonymous/polygon/regular/square-1.0.1.aamap.xml" );

        // colored teams so the player is always blue here
        PushSetting( "ALLOW_TEAM_NAME_COLOR", "1" );
        PushSetting( "ALLOW_TEAM_NAME_PLAYER", "0" );

        // one player mode
        PushSetting( "SPECTATOR_MODE_1", "0" );
        PushSetting( "VIEWPORT_CONF", "0" );
        PushSetting( "VIEWPORT_TO_PLAYER_1", "0" );

        // default physics, the most important ones
        PushSetting( "CYCLE_DELAY", ".1" );
        PushSetting( "CYCLE_RUBBER", "1" );
        PushSetting( "CYCLE_SPEED", "30" );
        PushSetting( "CYCLE_SOUND_SPEED", "30" );
        PushSetting( "CYCLE_ACCEL", "10" );
        PushSetting( "CYCLE_ACCEL_OFFSET", "2" );
        PushSetting( "CYCLE_WALL_NEAR", "6" );

        // allow custom camera
        PushSetting( "CAMERA_FORBID_CUSTOM", "0" );
    }

    // called on success
    virtual void OnSuccess()
    {
    }
    
    // called on failure
    virtual void OnFailure()
    {
    }

    // runs the tutorial
    virtual bool Run()
    {
        success_ = false;
        Prepare();
        RunCore();
        Restore();
        return success_;
    }

    // called from the menu
    bool Activate()
    {
        success_ = false;
        finished_ = true;
        for( int tries = 3; finished_ && !success_ && tries > 0; --tries )
        {
            Instructions();
            Run();
        }

        if( finished_ )
        {
            if( success_ )
            {
                OnSuccess();
                completed_ = true;
                return true;
            }
            else
            {
                OnFailure();
            }
        }

        return false;
    }

    bool Complete() const
    {
        return completed_;
    }

    virtual tString const & Name() const
    {
        return name_;
    }

    static bool AllComplete()
    {
        gTutorial * run = anchor_;
        while( run )
        {
            if( !run->completed_ )
            {
                return false;
            }
            run = run->Next();
        }
        return true;
    }
protected:
    gGameSettings settings_;
private:
    // succeeded ever?
    bool completed_;
    tConfItem< bool > completedConf_;
    tString name_;
    std::auto_ptr< gTutorialMenuItem > menuItem_;
    static gTutorial * anchor_;
    bool success_; //!< succeeded this time?
    bool finished_; //!< set when the tutorial is finished

    //! temporary setting overrides go here.
    std::vector< tJUST_CONTROLLED_PTR < gTemporarySetting > > tempSettings_;

    friend class gTutorialMenuItem;
};

gTutorial * gTutorial::anchor_ = NULL;

class gTutorialMenuItem: public uMenuItemAction
{
public:
    gTutorialMenuItem( uMenu * menu, gTutorial & tutorial )
    : uMenuItemAction( menu, 
                       ( tString("$tutorial_menu_") + tutorial.Name() + "_text" ).c_str(),
                       ( tString("$tutorial_menu_") + tutorial.Name() + "_help" ).c_str() )
      , tutorial_( tutorial )
    {
    }

    static void AdvanceMenu( uMenu & menu, bool enter = false )
    {
        int s = menu.GetSelected();
        if( s > menu.NumItems()-1 )
        {
            s = menu.NumItems()-1;
        }

        gTutorialMenuItem * i = NULL;
        while(true)
        {
            if( s <= 0 )
            {
                break;
            }
            i = dynamic_cast< gTutorialMenuItem * >( menu.Item( s ) );
            if( !i || !i->tutorial_.Complete() )
            {
                break;
            }
            
            s--;
        }
        menu.SetSelected( s );
        if( i && !i->tutorial_.Complete() && enter )
        {
            i->Enter();
        }
    }

    virtual void Enter()
    {
        bool success = tutorial_.Activate();
        if( success )
        {
            AdvanceMenu( *menu, true );
        }
    }
private:
    gTutorial & tutorial_;
};

static uMenu sg_tutorialMenu( "$game_menu_tutorials_text" );

void gTutorial::CreateMenu()
{
    menuItem_ = std::auto_ptr< gTutorialMenuItem >( tNEW( gTutorialMenuItem )( &sg_tutorialMenu, *this ) );
}

/*
static void sg_RunTutorial( BOOLRETFUNC * tutorial )
{
    
    (*tutorial)();
}
*/

// test tutorial
class gTutorialTest: public gTutorial
{
public:
    gTutorialTest()
    : gTutorial( "test" )
    {
        settings_.numAIs = 1;
    }

    // analyzes the game
    void Analysis()
    {
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        su_helpLevel = uActionTooltip::Level_Advanced;
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
    }

};
static gTutorialTest sg_tutorialTest;

//! conquest tutorial
class gTutorialConquest: public gTutorial
{
public:
    gTutorialConquest()
    : gTutorial( "conquest" )
    {
        settings_.sizeFactor = 0;
        settings_.numAIs = 1;
        settings_.wallsLength = 350;
    }

    // analyzes the game
    void Analysis()
    {
    }

    void AddSquare( gAIPlayer & ai, REAL radius )
    {
        REAL zoneX = 250;
        REAL zoneY = 50;

        AddPath( ai, zoneX+radius, zoneY-radius );
        AddPath( ai, zoneX-radius, zoneY-radius );
        AddPath( ai, zoneX-radius, zoneY+radius );
        AddPath( ai, zoneX+radius, zoneY+radius );
    }

    // set paths
    void AfterSpawn()
    {
        gAIPlayer & ai = AIPlayer();

        REAL zoneX = 250;
        REAL zoneY = 50;
 
        REAL radius = settings_.wallsLength/8 + .5;

        for( int i = 100; i > 0; --i )
        {
            AddSquare( ai, radius );
        }

        // radius-=.5;
        // AddSquare( ai, radius );
        // AddSquare( ai, 40 );
        // radius = 39;
        // AddPath( ai, zoneX+radius, zoneY+radius );
        // radius = 40;
        //AddPath( ai, zoneX+radius, zoneY+radius );
        AddPath( ai, zoneX+radius, zoneY, false );
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        su_helpLevel = uActionTooltip::Level_Advanced;
        PushSetting( "MAP_FILE", "Z-Man/fortress/for_old_clients-0.1.0.aamap.xml" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
        PushSetting( "FORTRESS_CONQUEST_RATE", ".3" );
        PushSetting( "FORTRESS_DEFEND_RATE", ".25" );
        PushSetting( "FORTRESS_CONQUEST_DECAY_RATE", ".1" );
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "CYCLE_RUBBER_WALL_SHRINK", ".5" );
        PushSetting( "TEXT_OUT", "0" );
    }

};
static gTutorialConquest sg_tutorialConquest;

//! grind tutorial
class gTutorialGrind: public gTutorial
{
public:
    gTutorialGrind()
    : gTutorial( "grind" )
    {
        settings_.numAIs = 1;
    }

    // analyzes the game
    void Analysis()
    {
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        su_helpLevel = uActionTooltip::Level_Advanced;
        PushSetting( "MAP_FILE", "Z-Man/tutorial/grind-0.1.0.aamap.xml" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "TEXT_OUT", "0" );
    }

};
static gTutorialGrind sg_tutorialGrind;

// survival: survive a bit, get to the winzone
class gTutorialSurvival: public gTutorial
{
public:
    gTutorialSurvival()
    : gTutorial( "survival" )
    {
        settings_.winZoneMinRoundTime = 30;
        settings_.winZoneMinLastDeath = 0;
        settings_.sizeFactor -= 2;
    }

    // analyzes the game
    void Analysis()
    {
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "WIN_ZONE_RANDOMNESS", ".9" );
        PushSetting( "WIN_ZONE_EXPANSION", "0" );
        PushSetting( "WIN_ZONE_INITIAL_SIZE", "5" );
        PushSetting( "WIN_ZONE_DEATHS", "0" );
        PushSetting( "CYCLE_RUBBER", "0" );
        PushSetting( "HIGH_RIM", "0" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/empty-0.0.1.aacockpit.xml" );
        PushSetting( "TEXT_OUT", "0" );
        su_helpLevel = uActionTooltip::Level_Advanced;
    }
};
static gTutorialSurvival sg_tutorialSurvival;

//! navigation: get to the winzone
class gTutorialNavigation: public gTutorial
{
public:
    gTutorialNavigation()
    : gTutorial( "navigation" )
    {
        settings_.sizeFactor += 1;
    }

    // analyzes the game
    void Analysis()
    {
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "TEXT_OUT", "0" );
        PushSetting( "CYCLE_RUBBER", "0" );
        PushSetting( "MAP_FILE", "Z-Man/tutorial/navigation-0.1.0.aamap.xml" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/empty-0.0.1.aacockpit.xml" );
        su_helpLevel = uActionTooltip::Level_Essential;
    }
};
static gTutorialNavigation sg_tutorialNavigation;

//! opens the tutorial menu
void sg_TutorialMenu()
{
    static uMenu & menu = sg_tutorialMenu;
    gTutorialMenuItem::AdvanceMenu( menu );
    menu.Enter();
}

//! returns whether all tutorials have been completed
bool sg_TutorialsCompleted()
{
    return gTutorial::AllComplete();
}

