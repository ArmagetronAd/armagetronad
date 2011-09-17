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

#include "gStuff.h"
#include "tSysTime.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "rViewport.h"
#include "rConsole.h"
#include "gGame.h"
#include "gLogo.h"
#include "gCommandLineJumpStart.h"

#include "eSoundMixer.h"

#include "rScreen.h"
#include "rSysdep.h"
#include "uInputQueue.h"
//#include "eTess.h"
#include "rTexture.h"
#include "tConfiguration.h"
#include "tRandom.h"
#include "tRecorder.h"
#include "tCommandLine.h"
#include "eAdvWall.h"
#include "eGameObject.h"
#include "uMenu.h"
#include "ePlayer.h"
#include "gLanguageMenu.h"
#include "gAICharacter.h"
#include "gCycle.h"
//#include <unistd>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include "nServerInfo.h"
#include "nSocket.h"
#include "tScripting.h"
#ifndef DEDICATED
#include "rRender.h"
#include "rSDL.h"
#include <SDL_syswm.h>

static gCommandLineJumpStartAnalyzer sg_jumpStartAnalyzer;
#endif

#ifndef DEDICATED
#ifdef MACOSX
#include "AAURLHandler.h"
#include "version.h"
#endif
#endif

// data structure for command line parsing
class gMainCommandLineAnalyzer: public tCommandLineAnalyzer
{
public:
    bool     fullscreen_;
    bool     windowed_;
    bool     use_directx_;
    bool     dont_use_directx_;

    gMainCommandLineAnalyzer()
    {
        windowed_ = false;
        fullscreen_ = false;
        use_directx_ = false;
        dont_use_directx_ = false;
    }


private:
    virtual bool DoAnalyze( tCommandLineParser & parser )
    {
        if ( parser.GetSwitch( "-fullscreen", "-f" ) )
        {
            fullscreen_=true;
        }
        else if ( parser.GetSwitch( "-window", "-w" ) ||  parser.GetSwitch( "-windowed") )
        {
            windowed_=true;
        }
#ifdef WIN32
        else if ( parser.GetSwitch( "+directx") )
        {
            use_directx_=true;
        }
        else if ( parser.GetSwitch( "-directx") )
        {
            dont_use_directx_=true;
        }
#endif
        else
        {
            return false;
        }

        return true;
    }

    virtual void DoHelp( std::ostream & s )
    {                                      //
#ifndef DEDICATED
        s << "-f, --fullscreen             : start in fullscreen mode\n";
        s << "-w, --window, --windowed     : start in windowed mode\n\n";
#ifdef WIN32
        s << "+directx, -directx           : enable/disable usage of DirectX for screen\n"
        << "                               initialisation under MS Windows\n\n";
        s << "\n\nYes, I know this looks ugly. Sorry about that.\n";
#endif
#endif
    }
};

static gMainCommandLineAnalyzer commandLineAnalyzer;

static bool use_directx=true;
#ifdef WIN32
static tConfItem<bool> udx("USE_DIRECTX","makes use of the DirectX input "
                           "fuctions; causes some graphic cards to fail to work (VooDoo 3,...)",
                           use_directx);
#endif

extern void exit_game_objects(eGrid *grid);

enum gConnection
{
    gLeave,
    gDialup,
    gISDN,
    gDSL
    // gT1
};

// initial setup menu
void sg_StartupPlayerMenu()
{
    uMenu firstSetup("$first_setup", false);
    firstSetup.SetBot(-.2);
    
    uMenuItemExit e2(&firstSetup, "$menuitem_accept", "$menuitem_accept_help");
    
    ePlayer * player = ePlayer::PlayerConfig(0);
    tASSERT( player );

    gConnection connection = gDSL;

    uMenuItemSelection<gConnection> net(&firstSetup, "$first_setup_net", "$first_setup_net_help", connection );
    if ( !st_FirstUse )
    {
        net.NewChoice( "$first_setup_leave", "$first_setup_leave_help", gLeave );
        connection = gLeave;
    }
    net.NewChoice( "$first_setup_net_dialup", "$first_setup_net_dialup_help", gDialup );
    net.NewChoice( "$first_setup_net_isdn", "$first_setup_net_isdn_help", gISDN );
    net.NewChoice( "$first_setup_net_dsl", "$first_setup_net_dsl_help", gDSL );

    tString keyboardTemplate("keys_cursor.cfg");
    uMenuItemSelection<tString> k(&firstSetup, "$first_setup_keys", "$first_setup_keys_help", keyboardTemplate );
    if ( !st_FirstUse )
    {
        k.NewChoice( "$first_setup_leave", "$first_setup_leave_help", tString("") );
        keyboardTemplate="";
    }
    k.NewChoice( "$first_setup_keys_cursor", "$first_setup_keys_cursor_help", tString("keys_cursor.cfg") );
    k.NewChoice( "$first_setup_keys_wasd", "$first_setup_keys_wasd_help", tString("keys_wasd.cfg") );
    k.NewChoice( "$first_setup_keys_zqsd", "$first_setup_keys_zqsd_help", tString("keys_zqsd.cfg") );
    k.NewChoice( "$first_setup_keys_cursor_single", "$first_setup_keys_cursor_single_help", tString("keys_cursor_single.cfg") );
    // k.NewChoice( "$first_setup_keys_both", "$first_setup_keys_both_help", tString("keys_twohand.cfg") );
    k.NewChoice( "$first_setup_keys_x", "$first_setup_keys_x_help", tString("keys_x.cfg") );

    tColor leave(0,0,0,0);
    tColor color(1,0,0);
    uMenuItemSelection<tColor> c(&firstSetup,
                                 "$first_setup_color",
                                 "$first_setup_color_help",
                                 color);   

    if ( !st_FirstUse )
    {
        color = leave;
        c.NewChoice( "$first_setup_leave", "$first_setup_leave_help", leave );
    }

    c.NewChoice( "$first_setup_color_red", "", tColor(1,0,0) );
    c.NewChoice( "$first_setup_color_blue", "", tColor(0,0,1) );
    c.NewChoice( "$first_setup_color_green", "", tColor(0,1,0) );
    c.NewChoice( "$first_setup_color_yellow", "", tColor(1,1,0) );
    c.NewChoice( "$first_setup_color_orange", "", tColor(1,.5,0) );
    c.NewChoice( "$first_setup_color_purple", "", tColor(.5,0,1) );
    c.NewChoice( "$first_setup_color_magenta", "", tColor(1,0,1) );
    c.NewChoice( "$first_setup_color_cyan", "", tColor(0,1,1) );
    c.NewChoice( "$first_setup_color_white", "", tColor(1,1,1) );
    c.NewChoice( "$first_setup_color_dark", "", tColor(0,0,0) );
    
    if ( st_FirstUse )
    {
        for(int i=tRandomizer::GetInstance().Get(4); i>=0; --i)
        {
            c.LeftRight(1);
        }
    }

    uMenuItemString n(&firstSetup,
                      "$player_name_text",
                      "$player_name_help",
                      player->name, 16);
    
    uMenuItemExit e(&firstSetup, "$menuitem_accept", "$menuitem_accept_help");
    
    firstSetup.Enter();

    // apply network rates
    switch(connection)
    {
    case gDialup:
        sn_maxRateIn  = 6;
        sn_maxRateOut = 4;
        break;
    case gISDN:
        sn_maxRateIn  = 8;
        sn_maxRateOut = 8;
        break;
    case gDSL:
        sn_maxRateIn  = 64;
        sn_maxRateOut = 16;
        break;
    case gLeave:
        break;
    }

    // store color
    if( ! (color == leave) )
    {
        player->rgb[0] = int(color.r_*15);
        player->rgb[1] = int(color.g_*15);
        player->rgb[2] = int(color.b_*15);
    }

    // load keyboard layout
    if( keyboardTemplate.Len() > 1 )
    {
        std::ifstream s;
        if( tConfItemBase::OpenFile( s, keyboardTemplate, tConfItemBase::Config ) )
        {
            tCurrentAccessLevel level( tAccessLevel_Owner, true );
            tConfItemBase::ReadFile( s );
        }
    }
}

#ifndef DEDICATED
static void welcome(){
    bool textOutBack = sr_textOut;
    sr_textOut = false;

#ifdef DEBUG_XXXX
    {
        for (int i = 20; i>=0; i--)
        {
            sr_ClearGL();
            {
                rTextField c(-.8,.6, .1, .1);
                tString s;
                s << ColorString(1,1,1);
                s << "Test";
                s << ColorString(1,0,0);
                s << "bla bla blubb blaa blaa blubbb blaaa blaaa blubbbb blaaaa blaaaa blubbbbb blaaaaa blaaaaa blubbbbbb blaaaaaa\n";
                c << s;
            }
            sr_SwapGL();
        }
    }
#endif

    REAL timeout = tSysTimeFloat() + .2;
    SDL_Event tEvent;

    if (st_FirstUse)
    {
        sr_LoadDefaultConfig();
        textOutBack = sr_textOut;
        sr_textOut = false;
        gLogo::SetBig(false);
        gLogo::SetSpinning(true);
    }
    else
    {
        bool showSplash = true;
#ifdef DEBUG
        showSplash = false;
#endif

        // Start the music up
        eSoundMixer* mixer;
        mixer = eSoundMixer::GetMixer();
        mixer->SetMode(TITLE_TRACK);
        mixer->Update();

        // disable splash screen when recording (it's annoying)
        static const char * splashSection = "SPLASH";
        if ( tRecorder::IsRunning() )
        {
            showSplash = false;

            // but keep it for old recordings where the splash screen was always active
            if ( !tRecorder::Playback( splashSection, showSplash ) )
                showSplash = tRecorder::IsPlayingBack();
        }

#ifndef DEDICATED
        if ( sg_jumpStartAnalyzer.ShouldConnect() )
        {
            showSplash = false;
            gLogo::SetDisplayed(false);
            sg_jumpStartAnalyzer.Connect();
        }
#endif

#ifdef MACOSX
        StartAAURLHandler( showSplash );
#endif
        tRecorder::Record( splashSection, showSplash );

        if ( showSplash )
        {
            timeout = tSysTimeFloat() + 6;

            uInputProcessGuard inputProcessGuard;
            while((!su_GetSDLInput(tEvent) || tEvent.type!=SDL_KEYDOWN) &&
                    tSysTimeFloat() < timeout)
            {
                if ( sr_glOut )
                {
                    sr_ResetRenderState(true);
                    rViewport::s_viewportFullscreen.Select();

                    rSysDep::ClearGL();

                    uMenu::GenericBackground();

                    rSysDep::SwapGL();
                }

                tAdvanceFrame();
            }
        }

        // catch some keyboard input
        {
            uInputProcessGuard inputProcessGuard;
            while (su_GetSDLInput(tEvent)) ;
        }

        sr_textOut = textOutBack;
        return;
    }

    if ( sr_glOut )
    {
        rSysDep::ClearGL();
        //        rFont::s_defaultFont.Select();
        //        rFont::s_defaultFontSmall.Select();
        gLogo::Display();
        rSysDep::ClearGL();
    }
    rSysDep::SwapGL();

    sr_textOut = textOutBack;
    sg_StartupLanguageMenu();

    sr_textOut = textOutBack;
    sg_StartupPlayerMenu();

    st_FirstUse=false;

    sr_textOut = textOutBack;
    uMenu::Message( tOutput("$welcome_message_heading"), tOutput("$welcome_message"), 300 );

    // start a first single player game
    sg_currentSettings->speedFactor = -2;
    sg_currentSettings->autoNum = 0;
    sr_textOut = textOutBack;
    sg_SinglePlayerGame();
    sg_currentSettings->autoNum = 1;
    sg_currentSettings->speedFactor = 0;

    sr_textOut = textOutBack;
    uMenu::Message( tOutput("$welcome_message_2_heading"), tOutput("$welcome_message_2"), 300 );

    sr_textOut = textOutBack;
}
#endif

void cleanup(eGrid *grid){
    static bool reentry=true;
    if (reentry){
        reentry=false;
        su_contInput=false;

        exit_game_objects(grid);
        /*
          for(int i=MAX_PLAYERS-1;i>=0;i--){
          if (playerConfig[i])
          destroy(playerConfig[i]->cam);
          }


          gNetPlayerWall::Clear();

          eFace::Clear();
          eEdge::Clear();
          ePoint::Clear();

          eFace::Clear();
          eEdge::Clear();
          ePoint::Clear();

          eGameObject::DeleteAll();


        */

#ifdef POWERPAK_DEB
        if (pp_out){
            PD_Quit();
            PP_Quit();
        }
#endif
        nNetObject::ClearAll();

        if (sr_glOut){
            rITexture::UnloadAll();
        }

        sr_glOut=false;
        sr_ExitDisplay();

#ifndef DEDICATED
        sr_RendererCleanup();
#endif

    }
}

#ifndef DEDICATED
int filter(const SDL_Event *tEvent){
    // recursion avoidance
    static bool recursion = false;
    if ( !recursion )
    {
        class RecursionGuard
        {
        public:
            RecursionGuard( bool& recursion )
                    :recursion_( recursion )
            {
                recursion = true;
            }

            ~RecursionGuard()
            {
                recursion_ = false;
            }

        private:
            bool& recursion_;
        };

        RecursionGuard guard( recursion );

        // boss key or OS X quit command
        if ((tEvent->type==SDL_KEYDOWN && tEvent->key.keysym.sym==27 &&
                tEvent->key.keysym.mod & KMOD_SHIFT) ||
                (tEvent->type==SDL_KEYDOWN && tEvent->key.keysym.sym==113 &&
                 tEvent->key.keysym.mod & KMOD_META) ||
                (tEvent->type==SDL_QUIT)){
            // sn_SetNetState(nSTANDALONE);
            // sn_Receive();

            // register end of recording
            tRecorder::Record("END");

            st_SaveConfig();
            uMenu::quickexit=uMenu::QuickExit_Total;
            return false;
        }

        if(tEvent->type==SDL_MOUSEMOTION)
            if(tEvent->motion.x==sr_screenWidth/2 && tEvent->motion.y==sr_screenHeight/2)
                return 0;
        if (su_mouseGrab &&
                tEvent->type!=SDL_MOUSEBUTTONDOWN &&
                tEvent->type!=SDL_MOUSEBUTTONUP &&
                ((tEvent->motion.x>=sr_screenWidth-10  || tEvent->motion.x<=10) ||
                 (tEvent->motion.y>=sr_screenHeight-10 || tEvent->motion.y<=10)))
            SDL_WarpMouse(sr_screenWidth/2,sr_screenHeight/2);

        // fetch alt-tab

        if (tEvent->type==SDL_ACTIVEEVENT)
        {
            // Jonathans fullscreen bugfix.
#ifdef MACOSX
            if(currentScreensetting.fullscreen ^ lastSuccess.fullscreen) return false;
#endif
            int flags = SDL_APPINPUTFOCUS;
            if ( tEvent->active.gain && tEvent->active.state & flags )
                Activate(true);
            if ( !tEvent->active.gain && tEvent->active.state & flags )
                Activate(false);

            // reload GL stuff if application gets reactivated
            if ( tEvent->active.gain && tEvent->active.state & SDL_APPACTIVE )
            {
                // just treat it like a screen mode change, gets the job done
                rCallbackBeforeScreenModeChange::Exec();
                rCallbackAfterScreenModeChange::Exec();
            }
            return false;
        }

        if (su_prefetchInput){
            return su_StoreSDLEvent(*tEvent);
        }

    }

    return 1;
}
#endif

//from game.C
void Update_netPlayer();

void sg_SetIcon()
{
#ifndef DEDICATED
#ifndef MACOSX
#ifdef  WIN32
    SDL_SysWMinfo	info;
    HICON			icon;
    // get the HWND handle
    SDL_VERSION( &info.version );
    if( SDL_GetWMInfo( &info ) )
    {
        icon = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( 1 ) );
        SetClassLong( info.window, GCL_HICON, (LONG) icon );
    }
#else
    rSurface tex( "desktop/icons/medium/armagetronad.png" );
    //    SDL_Surface *tex=IMG_Load( tDirectories::Data().GetReadPath( "textures/icon.png" ) );

    if (tex.GetSurface())
        SDL_WM_SetIcon(tex.GetSurface(),NULL);
#endif
#endif
#endif
}

class gAutoStringArray
{
public:
    ~gAutoStringArray()
    {
        for ( std::vector< char * >::iterator i = strings.begin(); i != strings.end(); ++i )
        {
            free( *i );
        }
    }

    char * Store( char const * s )
    {
        char * ret = strdup( s );
        strings.push_back( ret );
        return ret;
    }
private:
    std::vector< char * > strings; // the stored raw C strings
};

// wrapper for putenv, taking care of the peculiarity that the argument
// is kept in use for the rest of the program's lifetime
void sg_PutEnv( char const * s )
{
    static gAutoStringArray store;
    putenv( store.Store( s ) );
}

int main(int argc,char **argv){
    //std::cout << "enter\n";
    //  net_test();

    bool dedicatedServer = false;

    //  std::cout << "Running " << argv[0] << "...\n";

    // tERR_MESSAGE( "Start!" );

    try
    {
        tCommandLineData commandLine;
        commandLine.programVersion_  = &st_programVersion;

        // analyse command line
        // tERR_MESSAGE( "Analyzing command line." );
        if ( ! commandLine.Analyse(argc, argv) )
            return 0;


        {
            // embed version in recording
            const char * versionSection = "VERSION";
            tString version( st_programVersion );
            tRecorder::Playback( versionSection, version );
            tRecorder::Record( versionSection, version );
        }

        {
            // read/write server/client mode from/to recording
            const char * dedicatedSection = "DEDICATED";
            if ( !tRecorder::PlaybackStrict( dedicatedSection, dedicatedServer ) )
            {
#ifdef DEDICATED          
                dedicatedServer = true;
#endif
            }
            tRecorder::Record( dedicatedSection, dedicatedServer );
        }


        // while DGA mouse is buggy in XFree 4.0:
#ifdef linux
        // Sam 5/23 - Don't ever use DGA, we don't need it for this game.
        // no longer needed, the bug this compensated was fixed a long time
        // ago.
        /*
        if ( ! getenv("SDL_VIDEO_X11_DGAMOUSE") ) {
            sg_PutEnv("SDL_VIDEO_X11_DGAMOUSE=0");
        }
        */
#endif

#ifdef WIN32
        // disable DirectX by default; it causes problems with some boards.
        if (!use_directx && !getenv("SDL_VIDEODRIVER") ) {
            sg_PutEnv("SDL_VIDEODRIVER=windib");
        }
#endif

        // atexit(ANET_Shutdown);

#ifndef WIN32
#ifdef DEBUG
#define NOSOUND
#endif
#endif

#ifndef DEDICATED
        Uint32 flags = SDL_INIT_VIDEO;
#ifdef DEBUG
        flags |= SDL_INIT_NOPARACHUTE;
#endif // DEBUG
        if (SDL_Init(flags) < 0) {
            tERR_ERROR("Couldn't initialize SDL: " << SDL_GetError());
        }
        atexit(SDL_Quit);
        // su_KeyInit();

        su_KeyInit();

#ifndef NOJOYSTICK
        if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
            std::cout << "Error initializing joystick subsystem\n";
        else
        {
#ifdef DEBUG
            // std::cout << "Joystick(s) initialized\n";
#endif // DEBUG
            su_JoystickInit();
        }
#endif // NOJOYSTICK
#endif // DEDICATED

        // tERR_MESSAGE( "Initializing player data." );
        ePlayer::Init();

#ifdef ENABLE_SCRIPTING
        tScripting::GetInstance().InitializeInterpreter();
#endif

        // tERR_MESSAGE( "Loading configuration." );
        tLocale::Load("languages.txt");

        st_LoadConfig();

        // record and play back the recording debug level
        tRecorderSyncBase::GetDebugLevelPlayback();

        if ( commandLineAnalyzer.fullscreen_ )
            currentScreensetting.fullscreen   = true;
        if ( commandLineAnalyzer.windowed_ )
            currentScreensetting.fullscreen   = false;
        if ( commandLineAnalyzer.use_directx_ )
            use_directx                       = true;
        if ( commandLineAnalyzer.dont_use_directx_ )
            use_directx                       = false;

        //gAICharacter::LoadAll(tString( "aiplayers.cfg" ) );
        gAICharacter::LoadAll( aiPlayersConfig );

        sg_LanguageInit();
        atexit(tLocale::Clear);

        static eLadderLogWriter sg_encodingWriter( "ENCODING", true );
        sg_encodingWriter << "utf-8";
        sg_encodingWriter.write();

        if ( commandLine.Execute() )
        {
            gCycle::PrivateSettings();

            {
                std::ifstream t;

                if ( !tDirectories::Config().Open( t, "settings.cfg" ) )
                {
                    //		#ifdef WIN32
                    //                    tERR_ERROR( "Data files not found. You have to run Armagetron from its own directory." );
                    //		#else
                    tERR_ERROR( "Configuration files not found. Check your installation." );
                    //		#endif
                }
            }

            {
                std::ofstream s;
                if (! tDirectories::Var().Open( s, "scorelog.txt", std::ios::app ) )
                {
                    char const * error = "var directory not writable or does not exist. It should reside inside your user data directory and should have been created automatically on first start, but something must have gone wrong."
                    #ifdef WIN32
                                         " You can access your user data directory over one of the start menu entries we installed."
                    #else
                                         " Your user data directory is subdirectory named .armagetronad in your home directory."
                    #endif
                                         ;

                    tERR_ERROR( error );
                }
            }

            {
                std::ifstream t;

                if ( tDirectories::Data().Open( t, "moviepack/settings.cfg" ) )
                {
                    sg_moviepackInstalled=true;
                }
            }

#ifndef DEDICATED
            sr_glOut=1;
            //std::cout << "checked mp\n";

            sr_glRendererInit();

            SDL_SetEventFilter(&filter);

            //std::cout << "set filter\n";

            sg_SetIcon();

            tConsole::RegisterMessageCallback(&uMenu::Message);
            tConsole::RegisterIdleCallback(&uMenu::IdleInput);

            if (sr_InitDisplay()){

                try
                {
#ifdef HAVE_GLEW
                    // initialize GLEW
                    {
                        GLenum err = glewInit();
                        if (GLEW_OK != err)
                        {
                            // Problem: glewInit failed, something is seriously wrong
                            throw tGenericException( (const char *)glewGetErrorString(err), "GLEW Error" );
                        }
                        con << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << "\n";
                    }
#endif // HAVE_GLEW

                    //std::cout << "init disp\n";

                    //std::cout << "init sound\n";

                    welcome();

                    //std::cout << "atexit\n";

                    sr_con.autoDisplayAtSwap=false;

                    //std::cout << "sound started\n";

                    gLogo::SetBig(false);
                    gLogo::SetSpinning(true);

                    sn_bigBrotherString = renderer_identification + "VER=" + st_programVersion + "\n\n";

#ifdef ENABLE_SCRIPTING      
                    try {
                        // tScripting::GetInstance().Load(tDirectories::Data(), "scripts/menu.rb");
                        // tScripting::GetInstance().Load(tDirectories::Data(), "scripts/ai.rb");
                    }
                    catch (std::runtime_error & e) {
                        std::cerr << e.what() << '\n';
                    }
#endif

                    MainMenu();

                    // remove all players
                    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
                        se_PlayerNetIDs(i)->RemoveFromGame();

                    nNetObject::ClearAll();

                    rITexture::UnloadAll();
                }
                catch (tException const & e)
                {
                    gLogo::SetDisplayed(true);
                    uMenu::SetIdle(NULL);
                    sr_con.autoDisplayAtSwap=false;

                    // inform user of generic errors
                    tConsole::Message( e.GetName(), e.GetDescription(), 20 );
                }

                sr_ExitDisplay();
                sr_RendererCleanup();

                //std::cout << "exit\n";

                st_SaveConfig();

                //std::cout << "saved\n";

                //    cleanup(grid);
                SDL_QuitSubSystem(SDL_INIT_VIDEO);
            }

            eSoundMixer::ShutDown();

            SDL_Quit();
#else // DEDICATED
            sr_glOut=0;

            //  nServerInfo::TellMasterAboutMe();

            while (!uMenu::quickexit)
                sg_HostGame();
#endif // DEDICATED
            nNetObject::ClearAll();
            nServerInfo::DeleteAll();
        }

        ePlayer::Exit();

#ifdef ENABLE_SCRIPTING
        tScripting::GetInstance().CleanupInterpreter();
#endif

        //	tLocale::Clear();
    }
    catch( tException const & e )
    {
        try
        {
            st_PresentError( e.GetName(), e.GetDescription() );
        }
        catch(...)
        {
        }

        return 1;
    }
    catch ( std::exception & e )
    {
        try
        {
            st_PresentError("", e.what());
        }
        catch(...)
        {
        }
    }
#ifdef _MSC_VER
#pragma warning ( disable : 4286 )
    // GRR. Visual C++ dones not handle generic exceptions with the above general statement.
    // A specialized version is needed. The best part: it warns about the code below being redundant.
    catch( tGenericException const & e )
    {
        try
        {
            st_PresentError( e.GetName(), e.GetDescription() );
        }
        catch(...)
        {
        }

        return 1;
    }
#endif
    catch(...)
    {
        return 1;
    }

    return 0;
}

#ifdef DEDICATED
// settings missing in the dedicated server
static void st_Dummy(std::istream &s){tString rest; rest.ReadLine(s);}
static tConfItemFunc st_Dummy10("MASTER_QUERY_INTERVAL", &st_Dummy);
static tConfItemFunc st_Dummy11("MASTER_SAVE_INTERVAL", &st_Dummy);
static tConfItemFunc st_Dummy12("MASTER_IDLE", &st_Dummy);
static tConfItemFunc st_Dummy13("MASTER_PORT", &st_Dummy);
#endif



