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

#ifdef ENABLE_ZONESV2
#include "zone/zTimedZone.h"
#include "zone/zZone.h"
#endif
#ifdef ENABLE_ZONESV1
#include "gWinZone.h"
#endif
#ifdef ENABLE_ZONESV2
#define sg_CreateWinDeathZone sz_CreateTimedZone
#endif

#include "rConsole.h"

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
    
#ifndef DEDICATED

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
      , roundEndReached_( false )
      , warpedAhead_( 0 )
      , difficulty_ ( 0 )
    {
        CreateMenu();
    }

    virtual bool IsChallenge() const
    {
        return false;
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
        roundEndReached_ = true;
        finished_ = true;
    }

    // called sometime after End(false), right after a winner would have been declared
    // return true if special action was taken
    virtual bool OnUnspecifiedNonWin()
    {
        return false;
    }

    // round end result: human team must win without casualties
    void OnWin( eTeam * winner )
    {
        bool failed = finished_ && !success_;

        finished_ = true;
        if( winner && winner->OldestHumanPlayer() )
        {
            if ( failed )
            {
                if( OnUnspecifiedNonWin() )
                {
                    return;
                }
            }

            if( winner->AlivePlayers() == winner->NumPlayers() )
            {
                sn_CenterMessage(tOutput("$tutorial_success"));
                success_ = true;
            }
            else if( winner->OldestHumanPlayer()->Object() && winner->OldestHumanPlayer()->Object()->Alive() )
            {
                sn_CenterMessage(tOutput("$tutorial_teamkill"));
                success_ = false;
            }
            else if( winner->OldestHumanPlayer()->Object() && !winner->OldestHumanPlayer()->Object()->Alive() )
            {
                sn_CenterMessage("");
            }
            else
            {
                sn_CenterMessage("");
            }
        }
    }

    virtual void SetDifficulty( int difficulty )
    {
        settings_.speedFactor = -difficulty;
    }

    // analyzes the game
    virtual void Analysis()
    {
        // warp ahead in time
        if( se_GameTime() >= 0 && se_GameTime() < warpedAhead_ )
        {
            se_mainGameTimer->Reset( warpedAhead_ );
        }
    }

    void CreateMenu();

    // shows instructions
    virtual bool Instructions()
    {
        return uMenu::Message(
            tOutput( (tString("$tutorial_menu_") + Name() + "_text" ).c_str()),
            tOutput( (tString("$tutorial_menu_") + Name() + "_instructions" ).c_str()),
            300,
            tOutput( (tString("$tutorial_menu_") + Name() + "_animation" ).c_str() )
            );
    }

    // runs the tutorial
    virtual bool RunCore()
    {
        roundEndReached_ = false;
        sg_TutorialGame( settings_, *this );
        return success_;
    }

    // temporarily alter a setting
    void PushSetting( char const * setting, char const * value )
    {
        tempSettings_.push_back( tNEW(gTemporarySetting)( setting, value ) );
    }

    template< class T > void PushSetting( char const * setting, T const & value )
    {
        std::ostringstream s;
        s << value;
        tempSettings_.push_back( tNEW(gTemporarySetting)( setting, s.str().c_str() ) );
    }

    // find human team
    eTeam & HumanTeam()
    {
        for( int i = eTeam::teams.Len()-1; i >= 0; --i )
        {
            if( eTeam::teams[i]->IsHuman() && eTeam::teams[i]->OldestHumanPlayer() )
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

    // give a player a head start
    void HeadStart( ePlayerNetID & p, REAL time )
    {
        REAL f = pow( .5, .5 * settings_.speedFactor );
        time *= f;
        f *= .1;
        p.Object()->eGameObject::Timestep(-time);
        while(time > f)
        {
            time -= f;
            p.Object()->Timestep(-time);
        }
        p.Object()->Timestep(0);
    }

    void AddPath( gAIPlayer & ai, REAL x, REAL y, bool mindless = true )
    {
        ai.AddToPath( tCoord( x, y ) * pow( 2, settings_.sizeFactor*.5 ), mindless );
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
        warpedAhead_ = 0;

        finished_ = false;

        sn_CenterMessage("");
        
        // default map
        PushSetting( "MAP_FILE", "Anonymous/polygon/regular/square-1.0.1.aamap.xml" );

        // colored teams so the player is always blue here
        PushSetting( "ALLOW_TEAM_NAME_COLOR", "1" );
        PushSetting( "ALLOW_TEAM_NAME_PLAYER", "0" );
        PushSetting( "ALLOW_TEAM_NAME_LEADER", "0" );

        // one player mode
        PushSetting( "SPECTATOR_MODE_1", "0" );
        PushSetting( "VIEWPORT_CONF", "0" );
        PushSetting( "VIEWPORT_TO_PLAYER_1", "0" );

        // AIs are really timing sensitive here
        PushSetting( "TIMESTEP_MAX", ".01" );
        
        // default physics, the most important ones
        PushSetting( "CYCLE_DELAY", ".1" );
        PushSetting( "CYCLE_RUBBER", "1" );
        PushSetting( "CYCLE_SPEED", "20" );
        PushSetting( "CYCLE_START_SPEED", "20" );
        PushSetting( "CYCLE_SOUND_SPEED", "20" );
        PushSetting( "CYCLE_ACCEL", "15" );
        PushSetting( "CYCLE_ACCEL_OFFSET", "2" );
        PushSetting( "CYCLE_WALL_NEAR", "6" );

        // allow custom camera
        PushSetting( "CAMERA_FORBID_CUSTOM", "0" );

        // make chatbot useless
        PushSetting( "CHATBOT_MIN_TIMESTEP", "1000" );
        PushSetting( "CHATBOT_RANGE", "0.001" );

        // no names
        PushSetting( "FADEOUT_NAME_DELAY", "0" );

        SetDifficulty( difficulty_ );

        su_helpLevel = uActionTooltip::Level_Essential;
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
    virtual bool Activate()
    {
        success_ = false;
        finished_ = true;
        roundEndReached_ = true;
        difficulty_ = 0;
        bool instructions = true;
        bool secondGo = false;
        for( int tries = 0; roundEndReached_ && finished_ && !success_; tries ++ )
        {
            if( instructions && !Instructions() )
            {
                break;
            }
            Run();
            if( !roundEndReached_ )
            {
                break;
            }
            if( success_ )
            {
                // reset difficulty
                if( difficulty_ > 0 && !secondGo )
                {
                    uMenu::Message( tOutput("$tutorial_easy_head"), tOutput("$tutorial_easy_body"), 300 );
                    secondGo = true;
                    tries = 0;

                    success_ = false;

                    difficulty_ = 0; 
                }
            }
            else
            {
                if( IsChallenge() )
                {
                    // only give instructions again after third failure
                    instructions = false;
                    if( tries >= 3 )
                    {
                        // then again after 5
                        tries = -2;
                        instructions = true;
                    }
                }
                else if( ( !secondGo || ( (tries % 3) == 2) ) && difficulty_ < 4 )
                {
                    // decrease difficulty (every third time in the second iteration)
                    difficulty_++;
                }
            }
        }

        if( finished_ )
        {
            if( success_ )
            {
                OnSuccess();
                completed_ = true;
                return roundEndReached_;
            }
            else
            {
                OnFailure();
            }
        }
        else
        {
            return success_ && roundEndReached_;
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
            if( !run->IsChallenge() && !run->completed_ )
            {
                return false;
            }
            run = run->Next();
        }
        return true;
    }

    static bool AllIncomplete()
    {
        gTutorial * run = anchor_;
        while( run )
        {
            if( run->completed_ )
            {
                return false;
            }
            run = run->Next();
        }
        return true;
    }

    // warps dt seconds into the future for all game objects and AIs.
    // to be called in AfterSpawn().
    void TimeWarp( REAL dt )
    {
        PushSetting( "SPARKS", "0" );
        dt *= pow( .5, .5 * settings_.speedFactor );

        REAL last = warpedAhead_;
        warpedAhead_ += dt;
        REAL step = 0.1;
        while( last < warpedAhead_ - step )
        {
            last += step;
            eGameObject::s_Timestep( se_PlayerNetIDs(0)->Object()->Grid(), last, 1 );
            // simulate IAs
            for (int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            {
                gAIPlayer *ai = dynamic_cast<gAIPlayer*>(se_PlayerNetIDs(i));
                if (ai)
                    ai->Timestep(last);
            }
        }
        eGameObject::s_Timestep( se_PlayerNetIDs(0)->Object()->Grid(), warpedAhead_, 1 );
    }

    REAL WarpedAhead() const
    {
        return warpedAhead_;
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

protected:
    bool success_; //!< succeeded this time?
    bool finished_; //!< set when the tutorial is finished
    bool roundEndReached_; //!< has the real end of the round been reached properly?

private:
    // seconds warped into the round
    REAL warpedAhead_;
    // difficulty level; 0 is normal, 1 easy,...
    int difficulty_;

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

    virtual void Render(REAL x,REAL y,REAL alpha,bool selected)
    {
        if( tutorial_.Complete() )
        {
            alpha *= selected ? .75 : .5;
        }
        uMenuItemAction::Render( x, y, alpha, selected );
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
        bool previousSuccess = tutorial_.Complete();
        bool totalSuccess = gTutorial::AllComplete();
        bool success = tutorial_.Activate();
        // only auto-advance on freshly completed tutorials/challenges and take
        // a break after the last tutorial
        if( success && !previousSuccess && totalSuccess == gTutorial::AllComplete() )
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

 // challenges: like tutorials, only not mandatory
class gChallenge: public gTutorial
{
public:
    gChallenge( char const * name )
    : gTutorial( name )
    , firstRun_( true )
    {
    }

    // analyzes the game
    virtual void Analysis()
    {
        gTutorial::Analysis();

        // skip countdown on repeat runs
        REAL minTime = -.999;
        if( !firstRun_ && se_GameTime() < minTime )
        {
            se_mainGameTimer->Reset( minTime );
        }
    }

    virtual bool RunCore()
    {
        bool ret = gTutorial::RunCore();
        firstRun_ = false;
        return ret;
    }

    virtual bool IsChallenge() const
    {
        return true;
    }

    virtual void Prepare()
    {
        gTutorial::Prepare();

        // back to player color
        PushSetting( "ALLOW_TEAM_NAME_COLOR", "0" );
        PushSetting( "ALLOW_TEAM_NAME_PLAYER", "1" );
        PushSetting( "ALLOW_TEAM_NAME_LEADER", "1" );

        // full controls
        su_helpLevel = uActionTooltip::Level_Expert;
    }

    virtual bool Activate()
    {
        firstRun_ = true;
        return gTutorial::Activate();
    }
private:
    bool firstRun_;
};

// AI challenges
class gAIChallenge: public gChallenge
{
    // timeout values
    int initial_;       // initial time
    int bonusPerKill_;  // time awarded for each kill
    int killLimit_;     // total kills required to advance

    int timeOffset_;    // offset to time calculation, used to keep time left limited

protected:
    int scoreKill_;    // score awarded per kill
    int maxTime_;      // maximal time on the clock left
public:
    gAIChallenge( char const * name, int initial, int bonusPerKill, int killLimit )
    : gChallenge( name )
      , initial_( initial )
      , bonusPerKill_( bonusPerKill )
      , killLimit_( killLimit )
      , scoreKill_( 2 )
      , maxTime_( initial+bonusPerKill )
    {
        // settings_.gameType = gFREESTYLE;
        settings_.scoreWin = 0;

        if( maxTime_ < 60 )
        {
            maxTime_ = 60;
        }
    }

    virtual void BeforeSpawn()
    {
        gChallenge::BeforeSpawn();
        gAIPlayer::SetNumberOfAIs( settings_.numAIs, settings_.minPlayers, settings_.AI_IQ, 10 );

        // lowering player score so he gets the first spawn point
        // HumanTeam().AddScore(-1);
        // eTeam::SortByScore();
    }

    virtual void AfterSpawn()
    {
        gChallenge::AfterSpawn();
        
        // HumanTeam().AddScore(1);

        timeOffset_ = 0;
    }

    virtual bool OnUnspecifiedNonWin()
    {
        con.CenterDisplay( tString( tOutput("$tutorial_ai_timeout") ), 2 );

        return true;
    }

    // analyzes the game
    void Analysis()
    {
        gChallenge::Analysis();

        if( finished_ )
        {
            return;
        }

        if( HumanTeam().Score() >= scoreKill_ * killLimit_ )
        {
            End( true );
        }

        if( initial_ == 0 )
        {
            if( AITeam().AlivePlayers() == 0 )
            {
                End( true );
            }

            return;
        }

        sg_RespawnAllAfter( 5, false );

        if( se_GameTime() > 1  )
        {
            static int lastTimeLeft = -1, lastBonus = -1;
            int bonus = (HumanTeam().Score()/scoreKill_) * bonusPerKill_;
            int timeLeft = int(-se_GameTime() + bonus + initial_ + timeOffset_);
            static int skipNumbers = 0;
            if( bonus != lastBonus )
            {
                lastBonus = bonus;
                if( bonus != 0 )
                {
                    con.CenterDisplay( tString( tOutput("$tutorial_ai_extratime") ), 1 );
                    skipNumbers = 3;
                }
            }

            if( timeLeft != lastTimeLeft )
            {
                if( timeLeft > maxTime_ )
                {
                    timeOffset_ += maxTime_ - timeLeft;
                    timeLeft = maxTime_;
                }

                lastTimeLeft = timeLeft;
                if( timeLeft <= 0 )
                {
                    End( false );
                }
                else if ( --skipNumbers <= 0 )
                {
                    skipNumbers = 0;
                    std::ostringstream s;
                    s << timeLeft;
                    con.CenterDisplay( s.str(), 1.5 );
                }
            }
        }
    }

    // prepares
    virtual void Prepare()
    {
        gChallenge::Prepare();

        // invulnerability
        PushSetting( "CYCLE_INVULNERABLE_TIME", "2" );
        PushSetting( "CYCLE_WALL_TIME", "3" );

        // scoring
        PushSetting( "SCORE_KILL", scoreKill_ );
        PushSetting( "SCORE_DIE", "0" );
        PushSetting( "SCORE_SUICIDE", "0" );
        
        // map
        PushSetting( "MAP_FILE", "Z-Man/tutorial/ai_challenge-0.1.0.aamap.xml" );
    }
};

// AI challenges with given paramenters
class gAIChallengeFixed: public gAIChallenge
{
public:
    gAIChallengeFixed( char const * name, int num, int IQ, int size, int limitInitial, int bonusPerKill )
    : gAIChallenge( name, limitInitial, bonusPerKill, num )
    {
        settings_.numAIs = num > 3 ? 3 : num;
        if( num > 3 )
        {
            settings_.gameType = gFREESTYLE;
        }
        settings_.AI_IQ = IQ;
        settings_.sizeFactor = size;
        settings_.wallsLength = 400;
    }

    // prepares
    virtual void Prepare()
    {
        gAIChallenge::Prepare();
    }
};

// AI tutorial: just one AI
class gAITutorial: public gAIChallengeFixed
{
public:
    gAITutorial()
    : gAIChallengeFixed( "ai1", 1, 0, -3, 0, 0 )
    {
    }
    
    virtual bool IsChallenge() const
    {
        return false;
    }

    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // back to default map
        PushSetting( "MAP_FILE", "Anonymous/polygon/regular/square-1.0.1.aamap.xml" );
    }
};

extern uActionTooltip sg_brakeTooltip;

// brake boost showcase
class gShowcaseBrakeBoost: public gAIChallengeFixed
{
public:
    gShowcaseBrakeBoost()
    : gAIChallengeFixed( "brakeboost", 3, 0, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // brake = boost
        PushSetting( "CYCLE_BRAKE", "-30" );

        if( !Complete() )
        {
            sg_brakeTooltip.ShowAgain();
        }
    }
};

// more than 4 directions
class gShowcaseAxes: public gAIChallengeFixed
{
public:
    gShowcaseAxes()
    : gAIChallengeFixed( "axes", 3, 60, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // 8 axis
        PushSetting( "ARENA_AXES", "8" );

        // the AIs suck, so make it easier for them
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "CYCLE_DELAY", ".03" );
    }
};

// maps
class gShowcaseMap: public gAIChallengeFixed
{
public:
    gShowcaseMap()
    : gAIChallengeFixed( "map", 3, 100, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        PushSetting( "MAP_FILE", "Z-Man/tutorial/map-0.1.0.aamap.xml" );
    }
};

// high rubber showcase
class gShowcaseHighRubber: public gAIChallengeFixed
{
public:
    gShowcaseHighRubber()
    : gAIChallengeFixed( "highrubber", 3, 50, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // basic high rubber
        PushSetting( "CYCLE_RUBBER", "15" );
        PushSetting( "CYCLE_DELAY", ".01" );
    }
};

// open play showcase
class gShowcaseTurbo: public gAIChallengeFixed
{
public:
    gShowcaseTurbo()
    : gAIChallengeFixed( "turbo", 3, 0, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // very high speed
        PushSetting( "CYCLE_SPEED", "200" );
        PushSetting( "CYCLE_SOUND_SPEED", "200" );
        PushSetting( "CYCLE_START_SPEED", "200" );
        PushSetting( "CYCLE_ACCEL", "200" );
        PushSetting( "CYCLE_DELAY", ".05" );

        PushSetting( "HIGH_RIM", "0" );

        // back to default map
        PushSetting( "MAP_FILE", "Anonymous/polygon/regular/square-1.0.1.aamap.xml" );
    }
};

// open play showcase
class gShowcaseOpen: public gAIChallengeFixed
{
public:
    gShowcaseOpen()
    : gAIChallengeFixed( "open", 2, 40, -1, 0, 0 )
    {
    }
    
    virtual void Prepare()
    {
        gAIChallengeFixed::Prepare();

        // basic high rubber
        PushSetting( "CYCLE_RUBBER", "15" );
        PushSetting( "CYCLE_DELAY", ".01" );

        // mindistance for open play
        PushSetting( "CYCLE_RUBBER_MINDISTANCE", ".5" );
        PushSetting( "CYCLE_RUBBER_MINDISTANCE_GAP", ".5" );
        PushSetting( "CYCLE_RUBBER_MINADJUST", ".2" );
    }
};

// bullies force you into a maze
class gMazeChallenge: public gChallenge
{
public:
    gMazeChallenge( char const * name )
    : gChallenge( name )
    {
        lastWidth_ = 0;
        settings_.AI_IQ = 1000;
        settings_.sizeFactor = 0;
        settings_.numAIs = 2;
        settings_.explosionRadius = 0;
    }

    // analyzes the game
    virtual void Analysis()
    {
        gChallenge::Analysis();
    }

    virtual void OnWin( eTeam * winner )
    {
        gChallenge::OnWin( winner );

        if( winner && winner->OldestHumanPlayer() )
        {
            // let camera pan over maze
            PushSetting( "CAMERA_FORBID_SMART", "0" );
            PushSetting( "CAMERA_FORBID_FREE", "0" );

            eCamera * cam = ePlayer::PlayerConfig(0)->cam;
            if( !cam )
            {
                return;
            }

            eCoord pos = cam->CenterPos();
            cam->SetCamMode( CAMERA_SMART );
            for( int i = 100; i >= 0; --i )
            {
                cam->Timestep(.1);
                cam->SetCameraPos( pos * (-1) );
                cam->SetCameraZ( pos.Norm()*.5 );
            }
            cam->SetCamMode( CAMERA_FREE );
        }
    }

    // adjust width on the fly, to be called from Maze()
    void SetWidth( REAL width )
    {
        width_ = width;
    }

    void BeforeSpawn()
    {
        gChallenge::BeforeSpawn();

        AITeam().OldestPlayer()->SetTeamname(tOutput("$tutorial_menu_bullies_teamname"));
        AITeam().Update();
    }

    void AfterSpawn()
    {
        gChallenge::AfterSpawn();

        // give AIs a head start
        eTeam & ais = AITeam();
        for( int i = ais.NumPlayers()-1; i >= 0; --i )
        {
            ePlayerNetID & ai = *ais.Player(i);
            HeadStart( ai, 2 );
        }

        TimeWarp( 0.1 );

        // create maze
        width_ = 2;
        currentDir_ = eCoord(0,1);
        currentPos_ = eCoord( 0, 70 );
        maze_.clear();
        lastWidth_ = .5 * pow( 2, .5*settings_.sizeFactor );
        Maze();
        Add(1,0);
        ApplyMaze();
    }

    virtual void Maze()
    {
    }

    // prepares
    virtual void Prepare()
    {
        gChallenge::Prepare();

        PushSetting( "MAP_FILE", "Z-Man/tutorial/bullies-0.1.0.aamap.xml" );
        PushSetting( "CYCLE_ACCEL", "0" );
        PushSetting( "CYCLE_SPEED", "30" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/empty-0.0.1.aacockpit.xml" );

        // incam only
        PushSetting( "CAMERA_FORBID_CUSTOM", "1" );
        PushSetting( "CAMERA_FORBID_SERVER_CUSTOM", "1" );
        PushSetting( "CAMERA_FORBID_IN", "0" );
        PushSetting( "CAMERA_FORBID_SMART", "1" );
        PushSetting( "CAMERA_FORBID_FREE", "1" );
        PushSetting( "CAMERA_FORBID_FOLLOW", "1" );
        PushSetting( "CAMERA_FORBID_MER", "1" );

        // no console
        PushSetting( "TEXT_OUT", "0" );

        // create test maze to get correct size factor
        width_ = 2;
        currentDir_ = eCoord(0,1);
        currentPos_ = eCoord( 0, 120 );
        maze_.clear();
        Maze();
        Add(1,0);
        
        REAL maxRadius = 180;
        for( std::vector< MazePoint >::iterator i = maze_.begin(); i != maze_.end(); ++i )
        {
            if( fabs((*i).center.x) > maxRadius )
            {
                maxRadius = fabs((*i).center.x);
            }
            if( fabs((*i).center.y) > maxRadius )
            {
                maxRadius = fabs((*i).center.y);
            }
        }

        maxRadius += width_;

        settings_.sizeFactor = 0;
        while( maxRadius > 190 )
        {
            maxRadius*=sqrt(.5);
            settings_.sizeFactor++;
        }
    }
protected:
    void ApplyMaze()
    {
        eTeam & aiTeam = AITeam();
        gAIPlayer * ais[2] = { dynamic_cast< gAIPlayer * > ( aiTeam.Player(0) ),
                               dynamic_cast< gAIPlayer * > ( aiTeam.Player(1) ) };
        

        if( maze_.size() )
        {
            MazePoint const & point = maze_.back();
            eCoord side = point.dirBefore.Turn(0,1);
            
            REAL radius = ais[0]->Object()->Speed() * sg_delayCycle;
            eCoord center = point.center + width_ * 2 * point.dirBefore + side * width_ * .5;
            ais[0]->AddToPath( center+side*(radius+width_)+point.dirBefore*radius );
            ais[1]->AddToPath( center-side*(radius+width_)+point.dirBefore*radius );
            ais[0]->AddToPath( center+side*(radius+width_)-point.dirBefore*radius );
            ais[1]->AddToPath( center-side*(radius+width_)-point.dirBefore*radius );
            ais[0]->AddToPath( center-side*(radius+width_)-point.dirBefore*radius );
            ais[1]->AddToPath( center+side*(radius+width_)-point.dirBefore*radius );
            ais[0]->AddToPath( center-side*(radius+width_) );
            ais[1]->AddToPath( center+side*(radius+width_) );
            radius = width_+10;
            ais[0]->AddToPath( center+side*(radius+width_)+point.dirBefore*radius );
            ais[1]->AddToPath( center-side*(radius+width_)+point.dirBefore*radius );
            ais[0]->AddToPath( center+side*(radius+width_)-point.dirBefore*radius );
            ais[1]->AddToPath( center-side*(radius+width_)-point.dirBefore*radius );
            ais[0]->AddToPath( center-side*(radius+width_)-point.dirBefore*radius );
            ais[1]->AddToPath( center+side*(radius+width_)-point.dirBefore*radius );
            ais[0]->AddToPath( center-side*(radius+width_) );
            ais[1]->AddToPath( center+side*(radius+width_) );
            center = point.center + width_ * 2 * point.dirBefore;
            ais[0]->AddToPath( center-side*(width_) );
            ais[1]->AddToPath( center+side*(width_) );
        }

        while( maze_.size() )
        {
            MazePoint const & point = maze_.back();

            ais[0]->AddToPath( point.center+point.offset );
            ais[1]->AddToPath( point.center-point.offset );

            maze_.pop_back();
        }
    }

    void Go( REAL distance )
    {
        currentPos_ = currentPos_ + distance*currentDir_;
    }

    void Turn( int direction )
    {
        if( direction == 0 )
        {
            return;
        }
        if( direction > 1 )
        {
            Turn( direction-1 );
            Go( width_*2 );
            direction = 1;
        }
        if( direction < -1 )
        {
            Turn( direction+1 );
            Go( width_*2 );
            direction = -1;
        }

        MazePoint point;
        point.center = currentPos_;
        eCoord newDir = currentDir_.Turn(0,direction);
        point.offset = direction*(width_*currentDir_ - lastWidth_*newDir);
        lastWidth_ = width_;
        point.dirBefore = currentDir_;
        currentDir_ = newDir;
        maze_.push_back( point );
    }

    // adds a maze turn: turn direction, go distance
    void Add( int direction, REAL distance )
    {
        Turn( direction );
        Go( distance );
    }
private:
    // data for maze
    eCoord currentPos_;
    eCoord currentDir_;
    REAL width_;
    REAL lastWidth_;
    struct MazePoint
    {
        eCoord center;
        eCoord offset;
        eCoord dirBefore;
    };
    std::vector< MazePoint > maze_;
};

class gMazeChallenge1: public gMazeChallenge
{
public:
    gMazeChallenge1()
    : gMazeChallenge( "bullies" ){}

private:
    virtual void Maze()
    {
        Add( 1, 100 );
        Add( 1, 50 );
        Add( -1, 100 );
        Add( -1, 100 );
        Add( -1, 50 );
        Add( 1, 50 );
        //Add( -1, 30 );
        //Add( -1, 20 );
        //Add( -1, 20 );
        //Add( 1, 40 );
    }
};

class gMazeChallengeHilbertBase: public gMazeChallenge
{
protected:
    gMazeChallengeHilbertBase( char const * name )
    : gMazeChallenge( name )
    {
    }

    void HilbertMazeRec( int order, int direction, REAL len )
    {
        if( order == 1 )
        {
            Go( len );
            Turn( direction );
            Go( len );
            Turn( direction );
            Go( len );
        }
        else
        {
            HilbertMazeRec( order-1, -direction, len );
            if( (order & 1) == 0 ) Turn( direction );
            Go( len );
            if( (order & 1) == 1 ) Turn( direction );
            HilbertMazeRec( order-1, direction, len );
            if( (order & 1) == 0 ) Turn( -direction );
            Go( len );
            if( (order & 1) == 0 ) Turn( -direction );
            HilbertMazeRec( order-1, direction, len );
            if( (order & 1) == 1 ) Turn( direction );
            Go( len );
            if( (order & 1) == 0 ) Turn( direction );
            HilbertMazeRec( order-1, -direction, len );
        }
    }

    void HilbertMaze( int order, int direction, REAL len )
    {
        HilbertMazeRec( order, direction, len );
        Go( 50 );
    }
};

class gMazeChallengeHilbert: public gMazeChallengeHilbertBase
{
    int order_;
    REAL len_;
    REAL width_;
public:
    gMazeChallengeHilbert( char const * name, int order, REAL len, REAL width )
    : gMazeChallengeHilbertBase( name ), order_( order ), len_( len ), width_( width )
    {
    }

    virtual void Maze()
    {
        SetWidth( width_ );
        HilbertMaze( order_, 1, len_ );
    }

    // prepares
    virtual void Prepare()
    {
        gMazeChallengeHilbertBase::Prepare();

        // PushSetting( "CAMERA_FORBID_CUSTOM", "0" );
    }
};

class gMazeChallengeDragonBase: public gMazeChallenge
{
    int lastTurn_;
protected:
    gMazeChallengeDragonBase( char const * name )
    : gMazeChallenge( name )
    {
    }

    void DragonMazeRec( int order, int direction, REAL len )
    {
        if( order == 0 )
        {
            Go( len );
        }
        else
        {
            DragonMazeRec( order-1, -1, len );
            if( lastTurn_ == direction )
            {
                Turn( direction );
            }
            lastTurn_ = direction;
            DragonMazeRec( order-1, 1, len );
        }
    }

    void DragonMaze( int order, int direction, REAL len )
    {
        lastTurn_ = 0;
        DragonMazeRec( order, direction, len );
        Go( 50 );
    }
};

class gMazeChallengeDragon: public gMazeChallengeDragonBase
{
    int order_;
    REAL len_;
    REAL width_;
public:
    gMazeChallengeDragon( char const * name, int order, REAL len, REAL width )
    : gMazeChallengeDragonBase( name ), order_( order ), len_( len ), width_( width )
    {
    }

    virtual void Maze()
    {
        SetWidth( width_ );
        DragonMaze( order_, 1, len_ );
    }

    // prepares
    virtual void Prepare()
    {
        gMazeChallengeDragonBase::Prepare();

        // PushSetting( "CAMERA_FORBID_CUSTOM", "0" );
    }
};

// survival: survive a bit, get to the winzone
class gChallengeSurvival: public gChallenge
{
    int timeLimit_; // total time to survive
    REAL speedIncrease_; // speed increase rate
public:
    gChallengeSurvival( char const * name, int time, int size, REAL speedIncrease )
    : gChallenge( name )
      , timeLimit_( time )
      , speedIncrease_( speedIncrease )
    {
        // settings_.winZoneMinRoundTime = 30;
        // settings_.winZoneMinLastDeath = 0;
        settings_.sizeFactor = size;
        settings_.scoreWin = 0;
    }

    // analyzes the game
    void Analysis()
    { 
        gChallenge::Analysis();

        if( finished_ || !HumanPlayer().Object() || !HumanPlayer().Object()->Alive() || se_GameTime() < 1 )
        {
            return;
        }

        int timeLeft = int(timeLimit_ - se_GameTime());
        static int lastTimeLeft = -1;
        if( timeLeft != lastTimeLeft )
        {
            lastTimeLeft = timeLeft;
            if( timeLeft == 0 )
            {
                PushSetting( "CYCLE_SPEED_DECAY_ABOVE", ".5" );
                PushSetting( "CYCLE_SPEED", ".0001" );
                PushSetting( "CYCLE_SPEED_MIN", ".01" );
            }
            else if ( timeLeft <= -2 )
            {
                End( true );
            }
            else if ( timeLeft > 0 )
            {
                std::ostringstream s;
                s << timeLeft;
                con.CenterDisplay( s.str(), 1.5 );
            }
        }
    }

    // prepares
    virtual void Prepare()
    {
        gChallenge::Prepare();

        // PushSetting( "WIN_ZONE_RANDOMNESS", "0" );
        // PushSetting( "WIN_ZONE_EXPANSION", "0" );
        // PushSetting( "WIN_ZONE_INITIAL_SIZE", "5000" );
        // PushSetting( "WIN_ZONE_DEATHS", "0" );
        // PushSetting( "CYCLE_RUBBER", "0" );
        PushSetting( "HIGH_RIM", "0" );

        PushSetting( "CYCLE_SPEED_DECAY_ABOVE", -speedIncrease_ );
        PushSetting( "CYCLE_SPEED", "1" );
        PushSetting( "CYCLE_SPEED_MIN", "30" );
        PushSetting( "CYCLE_START_SPEED", "30" );

        PushSetting( "DOUBLEBIND_TIME", "1" );

        PushSetting( "CYCLE_BRAKE", "0" );

        PushSetting( "MAP_FILE", "Z-Man/tutorial/survival-0.1.0.aamap.xml" );
    }
};

// survival with nasty enemies
class gChallengeSurvivalWithEnemies: public gChallengeSurvival
{
public:
    gChallengeSurvivalWithEnemies( char const * name, int time, int size, REAL speedIncrease, int enemies )
    :gChallengeSurvival( name, time, size, speedIncrease )
    {
        settings_.minPlayers = 1+enemies;
        settings_.AI_IQ = 1000;
    }

    virtual void Analysis()
    {
        gChallengeSurvival::Analysis();

        if( HumanPlayer().Object() && HumanPlayer().Object()->Alive() )
        {
            sg_RespawnAllAfter( 1, false );
        }
    }

    virtual void Prepare()
    {
        gChallengeSurvival::Prepare();

        // invulnerability
        PushSetting( "CYCLE_INVULNERABLE_TIME", "1" );
        PushSetting( "CYCLE_WALL_TIME", "2" );

        // extra acceleration from enemy walls
        PushSetting( "CYCLE_ACCEL_ENEMY", "4" );
        PushSetting( "CYCLE_ACCEL_TEAM", "4" );
    }
};

// speed kills
class gTutorialSpeedKillBase: public gTutorial
{
public:
    gTutorialSpeedKillBase( char const * name )
    : gTutorial( name )
    {
        settings_.sizeFactor = 0;
        settings_.numAIs = 1;
    }

    // analyzes the game
    void Analysis()
    {
        gTutorial::Analysis();
    }

    void AfterSpawn()
    {
        gTutorial::AfterSpawn();
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "MAP_FILE", "Z-Man/tutorial/speedkill-0.1.0.aamap.xml" );
    }
};

// speed kill defense
class gTutorialSpeedKillDefense: public gTutorialSpeedKillBase
{
public:
    gTutorialSpeedKillDefense()
    : gTutorialSpeedKillBase( "speedkilldefense" )
    {
    }

    void Analysis()
    {
        gTutorialSpeedKillBase::Analysis();

        // survival is enough
        if( se_GameTime() > WarpedAhead() + 15 && HumanPlayer().Object() && HumanPlayer().Object()->Alive() )
        {
            End(true);
        }
    }

    void AfterSpawn()
    {
        // give human a head start
        HeadStart( HumanPlayer(), 10 );

        while( HumanPlayer().Object()->Position().y > AIPlayer().Object()->Position().y + 50 )
        {
            TimeWarp(.1);
        }
        
        // HeadStart( AIPlayer(), 5 );
        REAL y = HumanPlayer().Object()->Position().y + 50;
        AddPath( AIPlayer(), -100, y+10 ); 
        AddPath( AIPlayer(), .1, y+10 ); 
        AddPath( AIPlayer(), .1, 10 ); 
        AddPath( AIPlayer(), 10, 10 ); 
        AddPath( AIPlayer(), 10, -1 ); 
        AddPath( AIPlayer(), -.1, -1 ); 
        AddPath( AIPlayer(), -.1, y-10 ); 
        AddPath( AIPlayer(), -10, y-10 ); 
        AddPath( AIPlayer(), -10, y ); 
        AddPath( AIPlayer(), .1, y, false ); 

        gTutorialSpeedKillBase::AfterSpawn();
    }
};

// speed kill
class gTutorialSpeedKill: public gTutorialSpeedKillBase
{
public:
    gTutorialSpeedKill()
    : gTutorialSpeedKillBase( "speedkill" )
    {
    }

    void AfterSpawn()
    {
        // give human a head start
        HeadStart( AIPlayer(), 10 );
        // HeadStart( HumanPlayer(), 3 );

        while( HumanPlayer().Object()->Position().y < AIPlayer().Object()->Position().y - 50 )
        {
            TimeWarp(.1);
        }

        // get the spot where the human overtakes AI
        REAL hy = HumanPlayer().Object()->Position().y;
        REAL ay = AIPlayer().Object()->Position().y;
        REAL hs = HumanPlayer().Object()->Speed();
        REAL as = AIPlayer().Object()->Speed();
        REAL time = (ay-hy)/(hs-as);

        REAL y = hy+hs*time;
        AddPath( AIPlayer(), 50, y-100 ); 
        AddPath( AIPlayer(), 5, y-100 ); 
        AddPath( AIPlayer(), 5, y ); 
        AddPath( AIPlayer(), .1, y ); 

        // REAL y = 1000;
        // AddPath( AIPlayer(), -10, y ); 
        // AddPath( AIPlayer(), .1, y, false ); 

        gTutorialSpeedKillBase::AfterSpawn();
    }
};

//! teamstart tutorial
class gTutorialTeamstartBase: public gTutorial
{
    REAL starty_; // start position of player
public:
    gTutorialTeamstartBase( char const * name )
    : gTutorial( name )
    {
        settings_.sizeFactor = 0;
        settings_.numAIs = 0;
        settings_.maxTeams = 1;
        settings_.minPlayersPerTeam = 6;
        settings_.maxPlayersPerTeam = 6;
        settings_.wallsLength = 400;
        settings_.winZoneMinRoundTime = 3;
        settings_.winZoneMinLastDeath = 0;
        settings_.speedFactor = 0;
    }

    // analyzes the game
    void Analysis()
    {
        gTutorial::Analysis();

        // no team kills
        if( se_GameTime() > .5 && !finished_ )
        {
            eTeam & team = HumanTeam();
            if( team.AlivePlayers() < team.NumPlayers() )
            {
                End( false );
            }
        }

        // no backdoor
        eGameObject *o = HumanPlayer().Object();
        if( o && o->Position().y < starty_ - .5 )
        {
            End( false );
        }
    }

    // called sometime after End(false), right after a winner would have been declared
    // return true if special action was taken
    virtual bool OnUnspecifiedNonWin()
    {
        eTeam & team = HumanTeam();
        if( team.AlivePlayers() == team.NumPlayers() )
        {
            sn_CenterMessage(tOutput("$tutorial_fail_backdoor"));
            return true;
        }

        return false;
    }
    
    // set paths
    void BeforeSpawn()
    {
        gTutorial::BeforeSpawn();

        eTeam & team = HumanTeam();
        team.Shuffle(0,3);
    }

    void AfterSpawn()
    {
        gTutorial::AfterSpawn();

        starty_ = HumanPlayer().Object()->Position().y;
    }

    virtual void SetDifficulty( int difficulty )
    {
        gTutorial::SetDifficulty( difficulty );
        REAL f = pow( .5, .5 * settings_.speedFactor );

        // re-compensate for speed wingmen formation compensation
        PushSetting( "SPAWN_WINGMEN_BACK", f*2.2 );
        PushSetting( "SPAWN_WINGMEN_SIDE", f*2.75 );
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
        PushSetting( "WIN_ZONE_RANDOMNESS", "0" );
        PushSetting( "WIN_ZONE_EXPANSION", "0" );
        PushSetting( "WIN_ZONE_INITIAL_SIZE", "10" );
        PushSetting( "WIN_ZONE_DEATHS", "0" );
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "CYCLE_SPEED", "15" );
        PushSetting( "CYCLE_ACCEL", "10" );
        PushSetting( "CYCLE_START_SPEED", "15" );
        PushSetting( "TEXT_OUT", "0" );
    }
};

//! teamstart tutorial
class gTutorialTeamstart: public gTutorialTeamstartBase
{
public:
    gTutorialTeamstart()
    : gTutorialTeamstartBase( "teamstart" )
    {
    }

    void Path( gAIPlayer & ai, int side, int level )
    {
        REAL SpawnX = 255;
        REAL eps = .3*level;
        REAL step = 10;
        REAL s = 2.753; 
        REAL SpawnY = 50-level*step*.4;

        AddPath( ai, 500, 500 );
        AddPath( ai, SpawnX-side*eps, 500 );
        if( level > 0 )
        {
            AddPath( ai, SpawnX-side*eps, SpawnY+level*step );
        }
        if( level > 1 )
        {
            AddPath( ai, SpawnX-side*(s+eps), SpawnY+level*step );
            AddPath( ai, SpawnX-side*(s+eps), SpawnY+(level-1)*step );
        }
        if( level > 2 )
        {
            AddPath( ai, SpawnX-side*(2*s+eps), SpawnY+(level-1)*step );
            AddPath( ai, SpawnX-side*(2*s+eps), SpawnY+(level-2)*step );
        }
    }
    
    void AfterSpawn()
    {
        gTutorialTeamstartBase::AfterSpawn();

        eTeam & team = HumanTeam();

        Path( *dynamic_cast< gAIPlayer * >( team.Player( 0 ) ), 0, 0 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 1 ) ), 1, 1 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 2 ) ), -1, 1 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 4 ) ), -1, 2 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 5 ) ), 1, 3 );
        // Path( *dynamic_cast< gAIPlayer * >( team.Player( 6 ) ), -1, 3 );
    }
};

//! doublegrind tutorial
class gTutorialDoublegrind: public gTutorialTeamstartBase
{
public:
    gTutorialDoublegrind()
    : gTutorialTeamstartBase( "doublegrind" )
    {
        //settings_.minPlayersPerTeam = 2;
        //settings_.maxPlayersPerTeam = 2;
    }

    virtual bool IsChallenge() const
    {
        return true;
    }

    // set paths
    // void BeforeSpawn()
    //{
        // eTeam & team = HumanTeam();
        // team.Shuffle(0,0);
        //}

    void Path( gAIPlayer & ai, int side, int level )
    {
        REAL SpawnX = 255;
        REAL eps = .3*level;
        REAL step = 10;
        REAL s = 2.753; 
        REAL SpawnY = 50-level*step*.4;

        AddPath( ai, 500, 500 );
        AddPath( ai, SpawnX-side*eps, 500 );
        if( level == 1 )
        {
            AddPath( ai, SpawnX+side*s, 52 );
            AddPath( ai, SpawnX-3*side*eps, 52 );
            AddPath( ai, SpawnX-3*side*eps, SpawnY+(level+1)*step );
        }
        else if( level > 0 )
        {
            AddPath( ai, SpawnX-side*eps, SpawnY+(level+2)*step );
        }
        if( level > 1 )
        {
            AddPath( ai, SpawnX-side*(s+eps), SpawnY+level*step );
            AddPath( ai, SpawnX-side*(s+eps), SpawnY+(level-1)*step );
        }
        if( level > 2 )
        {
            AddPath( ai, SpawnX-side*(2*s+eps), SpawnY+(level-1)*step );
            AddPath( ai, SpawnX-side*(2*s+eps), SpawnY+(level-2)*step );
        }
    }
    
    void AfterSpawn()
    {
        eTeam & team = HumanTeam();

        Path( *dynamic_cast< gAIPlayer * >( team.Player( 0 ) ), 0, 0 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 1 ) ), 1, 1 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 2 ) ), -1, 1 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 4 ) ), -1, 2 );
        Path( *dynamic_cast< gAIPlayer * >( team.Player( 5 ) ), 1, 3 );
    }
};

//! conquest tutorial
class gTutorialConquest: public gTutorial
{
    REAL lastHint_;
    int hintType_;
public:
    gTutorialConquest()
    : gTutorial( "conquest" )
    {
        settings_.sizeFactor = 0;
        settings_.numAIs = 1;
        settings_.gameType = gFREESTYLE;
        settings_.wallsLength = 350;
    }

    void Hint( char const * a, char const * b )
    {
        sn_CenterMessage( tOutput( hintType_ ? b : a ) );

        hintType_++;
        if( hintType_ > 1 )
            hintType_ = 0;
    }

    // analyzes the game
    void Analysis()
    {
        gTutorial::Analysis();

        // fast forward the setup
        static REAL yMin = 500;
        if( se_GameTime() > 0 && HumanPlayer().Object() && HumanPlayer().Object()->Alive() )
        {
            if( HumanPlayer().Object()->Position().y < yMin )
            {
                yMin = HumanPlayer().Object()->Position().y;
            }
            REAL yThresh = 150;
            if ( se_GameTime() < 20 &&  yMin > yThresh )
            {
                REAL timeLeft = (yMin - yThresh)/HumanPlayer().Object()->Speed();
                if( timeLeft > 1 )
                {
                    se_mainGameTimer->speed = 10;
                }
                else
                {
                    se_mainGameTimer->speed = timeLeft*9+1;
                }
            }
            else
            {
                se_mainGameTimer->speed = 1;
            }
        }
        else
        {
            yMin = 500;
        }

        if( !finished_ && !success_ && se_GameTime() > lastHint_ )
        {
            lastHint_ = se_GameTime() + 8;
            if( HumanPlayer().Object() && HumanPlayer().Object()->Alive() )
            {
                // human still alive
                if ( !(AIPlayer().Object() && AIPlayer().Object()->Alive()) )
                {
                    // AI was killed
                    Hint( "$tutorial_conquest_ai_killed", "$tutorial_conquest_ai_go" );
                }
                else if( (AIPlayer().Object()->Position() - tCoord( 250, 50 )).Norm() > 100 )
                {
                    // AI retreated
                    Hint( "$tutorial_conquest_ai_retreated", "$tutorial_conquest_ai_go" );
                }
                else if( AIPlayer().GetState() == AI_PATH_GIVEN && se_GameTime() > 30 )
                {
                    // aim for the gap. Don't be obnoxious about it.
                    hintType_ = 0;
                    sn_CenterMessage( tOutput( "$tutorial_conquest_ai_gap" ) );
                    lastHint_ = se_GameTime() + 15;
                }
            }
        }
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
        lastHint_ = 10;
        hintType_ = 0;

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
        
        // doesn't work here, causes zone trouble.
        // TimeWarp( 10 );
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "MAP_FILE", "Z-Man/fortress/for_old_clients-0.1.0.aamap.xml" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
        PushSetting( "FORTRESS_CONQUEST_RATE", ".3" );
        PushSetting( "FORTRESS_DEFEND_RATE", ".25" );
        PushSetting( "FORTRESS_CONQUEST_DECAY_RATE", ".1" );
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "CYCLE_START_SPEED", "40" );
        PushSetting( "CYCLE_RUBBER_WALL_SHRINK", ".5" );
        PushSetting( "TEXT_OUT", "0" );
    }

};

//! grind tutorial
class gTutorialGrind: public gTutorial
{
public:
    gTutorialGrind()
    : gTutorial( "grind" )
    {
        settings_.numAIs = 1;
        settings_.sizeFactor -= 1;
    }

    // analyzes the game
    void Analysis()
    {
        gTutorial::Analysis();
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "MAP_FILE", "Z-Man/tutorial/grind-0.1.0.aamap.xml" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/spartanic-0.0.1.aacockpit.xml" );
        PushSetting( "CYCLE_RUBBER", "5" );
        PushSetting( "TEXT_OUT", "0" );
    }

};

// survival: survive a bit, get to the winzone
class gTutorialSurvival: public gTutorial
{
    int baseSizeFactor;
public:
    gTutorialSurvival()
    : gTutorial( "survival" )
    {
        settings_.winZoneMinRoundTime = 30;
        settings_.winZoneMinLastDeath = 0;
        settings_.sizeFactor = -5;
        settings_.gameType = gFREESTYLE;
        baseSizeFactor = settings_.sizeFactor;
    }

    // analyzes the game
    void Analysis()
    { 
        gTutorial::Analysis();
    }

    virtual void SetDifficulty( int difficulty )
    {
        settings_.sizeFactor = baseSizeFactor+difficulty;
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();

        PushSetting( "WIN_ZONE_RANDOMNESS", ".9" );
        PushSetting( "WIN_ZONE_EXPANSION", "0" );
        PushSetting( "WIN_ZONE_INITIAL_SIZE", "5" );
        PushSetting( "WIN_ZONE_DEATHS", "0" );
        // PushSetting( "CYCLE_RUBBER", "0" );
        PushSetting( "HIGH_RIM", "0" );
        PushSetting( "COCKPIT_FILE", "Z-Man/tutorial/empty-0.0.1.aacockpit.xml" );
        PushSetting( "MAP_FILE", "Z-Man/tutorial/survival-0.1.0.aamap.xml" );
        PushSetting( "TEXT_OUT", "0" );
    }
};

// input: learn the rest of teh controls
class gTutorialControls: public gTutorial
{
public:
    gTutorialControls()
    : gTutorial( "controls" )
    {
        settings_.sizeFactor = 2;
        settings_.wallsLength = 50;
    }

    // analyzes the game
    void Analysis()
    { 
        gTutorial::Analysis();

        static double next = tSysTimeFloat() + 5;

        sg_RespawnAllAfter( 0, false );

        // check if there are unfinished tooltips
        if( tSysTimeFloat() > next && !rConsole::CenterDisplayActive() )
        {
            next = tSysTimeFloat() + 10;
            if ( !uActionTooltip::Help(1) && !uActionTooltip::Help(0) )
            {
                End(true);
            }
        }
    }

    // prepares
    virtual void Prepare()
    {
        gTutorial::Prepare();
        PushSetting( "TEXT_OUT", "0" );
        PushSetting( "COCKPIT_FILE", "Anonymous/standard-0.0.1.aacockpit.xml" );
        su_helpLevel = uActionTooltip::Level_Advanced;

        // find all advanced action tooltips and make them activate at least once
        tConfItemBase::tConfItemMap const & map = tConfItemBase::GetConfItemMap();
        for( tConfItemBase::tConfItemMap::const_iterator i = map.begin(); i != map.end(); ++i )
        {
            uActionTooltip * u = dynamic_cast< uActionTooltip * >( (*i).second );
            if( u )
            {
                u->ShowAgain();
            }
        }
    }
};

//! navigation: get to the winzone
class gTutorialNavigation: public gTutorial
{
public:
    gTutorialNavigation()
    : gTutorial( "navigation" )
    {
        // settings_.sizeFactor += 1;
    }

    // analyzes the game
    void Analysis()
    {
        gTutorial::Analysis();
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

//! closing words
class gTutorialCongratulations: public gTutorial
{
public:
    gTutorialCongratulations()
    : gTutorial( "congratulations" )
    {
    }

    virtual bool Run()
    {
        success_ = true;
        return true;
    }
};

//! opens the tutorial menu
void sg_TutorialMenu()
{
    static uMenu & menu = sg_tutorialMenu;
    bool firstRun = gTutorial::AllIncomplete();
    if( firstRun )
    {
        menu.SetSelected(menu.NumItems()-1);
    }
    gTutorialMenuItem::AdvanceMenu( menu, firstRun );
    menu.Enter();
}

//! returns whether all tutorials have been completed
bool sg_TutorialsCompleted()
{
    return gTutorial::AllComplete();
}

// ok, that's one too many.
// static gMazeChallengeDragon sg_challengeDragon5("dragon5", 9, 15, 1.1);

static gMazeChallengeHilbert sg_challengeHilbert4("hilbert4", 4, 15, 1);
static gAIChallengeFixed sg_AIChallenge8("ai8", 15, 100, -2, 60, 20);
static gChallengeSurvivalWithEnemies sg_challengeSurvival4("survival4", 60, -5, -.1, 3 );
static gAIChallengeFixed sg_AIChallenge7("ai7", 10, 100, -2, 60, 30);
static gMazeChallengeDragon sg_challengeDragon4("dragon4", 8, 17, 1.25);
static gShowcaseOpen sg_showcaseOpen;
static gChallengeSurvival sg_challengeSurvival3("survival3", 60, -6, .05 );
static gShowcaseAxes sg_showcaseAxes;
static gAIChallengeFixed sg_AIChallenge6("ai6", 8, 80, -2, 60, 30);
static gShowcaseMap sg_showcaseMap;
static gMazeChallengeDragon sg_challengeDragon3("dragon3", 7, 20, 1.25);
static gShowcaseTurbo sg_showcaseTurbo;
static gChallengeSurvival sg_challengeSurvival2("survival2", 60, -5, .2 );
static gAIChallengeFixed sg_AIChallenge5("ai5", 6, 60, -2, 60, 30);
static gMazeChallengeHilbert sg_challengeHilbert3("hilbert3", 3, 25, 1.25);
static gAIChallengeFixed sg_AIChallenge4("ai4", 4, 50, -2, 60, 30);
static gMazeChallengeDragon sg_challengeDragon2("dragon2", 6, 25, 1.3);
static gShowcaseHighRubber sg_showcaseHighRubber;
static gMazeChallengeDragon sg_challengeDragon1("dragon1", 5, 40, 1.4);
static gShowcaseBrakeBoost sg_showcaseBrakeBoost;
static gMazeChallengeHilbert sg_challengeHilbert2("hilbert2", 2, 50, 1.5);
static gAIChallengeFixed sg_AIChallenge3("ai3", 3, 30, -2, 0, 0);
static gChallengeSurvival sg_challengeSurvival1("survival1", 30, -5, .05 );
static gTutorialDoublegrind sg_tutorialDoublegrind;
static gAIChallengeFixed sg_AIChallenge2("ai2", 2, 25, -2, 0, 0);
static gMazeChallenge1 sg_tutorialBullies1;

static gTutorialCongratulations sg_tutorialCongratulations;

static gAITutorial sg_tutorialTest;
static gTutorialControls sg_tutorialControls;
static gTutorialTeamstart sg_tutorialTeamstart;
static gTutorialSpeedKillDefense sg_tutorialSpeedKillDefense;
static gTutorialSpeedKill sg_tutorialSpeedKill;
static gTutorialConquest sg_tutorialConquest;
static gTutorialGrind sg_tutorialGrind;
static gTutorialSurvival sg_tutorialSurvival;
static gTutorialNavigation sg_tutorialNavigation;

#endif // DEDICATED
