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

static gGameSettings sg_DefaultSettings()
{
    return gGameSettings(10,
                         30, 10, 100000, 100000,
                         0,   0, 30,
                         false, false,
                         0  ,  -3,
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

void gTutorialBase::RoundEnd( eTeam * winner )
{
    Analysis();
    uMenu::quickexit = uMenu::QuickExit_Game;
}

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
        uMenu::quickexit = uMenu::QuickExit_Game;
    }

    // always exit after one round
    void RoundEnd( eTeam * winner )
    {
        End( winner && winner->IsHuman() );
        Analysis();
    }

    void CreateMenu();

    // prepares
    virtual void Prepare()
    {
    }

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

    // back up settings
    void Backup()
    {
    }
    
    // restore settings
    void Restore()
    {
    }

    // called on success
    virtual void OnSuccess()
    {
        uMenu::Message( tOutput("$tutorial_success"),
                        tOutput("$tutorial_success_text"), 300 );
    }
    
    // called on failure
    virtual void OnFailure()
    {
        uMenu::Message( tOutput("$tutorial_failure"),
                        tOutput("$tutorial_failure_text"), 300 );
    }

    // runs the tutorial
    virtual bool Run()
    {
        success_ = false;
        Backup();
        Prepare();
        RunCore();
        Restore();
        return success_;
    }

    // called from the menu
    bool Activate()
    {
        Instructions();
        Run();

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

    static void AdvanceMenu( uMenu & menu )
    {
        int s = menu.GetSelected();
        if( s > menu.NumItems()-1 )
        {
            s = menu.NumItems()-1;
        }
        while(true)
        {
            if( s <= 0 )
            {
                break;
            }
            gTutorialMenuItem * i = dynamic_cast< gTutorialMenuItem * >( menu.Item( s ) );
            if( !i || !i->tutorial_.Complete() )
            {
                break;
            }

            s--;
        }
        menu.SetSelected( s );
    }

    virtual void Enter()
    {
        bool success = tutorial_.Activate();
        if( success )
        {
            AdvanceMenu( *menu );
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

class gTutorialTest: public gTutorial
{
public:
    gTutorialTest()
    : gTutorial( "test" )
    {
        settings_.speedFactor = -2;
        settings_.numAIs = 1;
    }

    void Analysis()
    {
    }
};

static gTutorialTest sg_tutorialTest;

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

