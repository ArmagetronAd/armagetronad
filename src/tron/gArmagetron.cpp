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

#ifndef DEDICATED
#include "rRender.h"
#include "rSDL.h"

static gCommandLineJumpStartAnalyzer sg_jumpStartAnalyzer;
#endif

#ifndef DEDICATED
#ifdef MACOSX
#include "AAURLHandler.h"
#endif
#endif


// data structure for command line parsing
class gMainCommandLineAnalyzer: public tCommandLineAnalyzer
{
public:
    bool     daemon_;
    bool     fullscreen_;
    bool     windowed_;
    bool     use_directx_;
    bool     dont_use_directx_;

    gMainCommandLineAnalyzer()
    {
        daemon_ = false;
        windowed_ = false;
        fullscreen_ = false;
        use_directx_ = false;
        dont_use_directx_ = false;
    }


private:
    virtual bool DoAnalyze( tCommandLineParser & parser )
    {
        if ( parser.GetSwitch( "--daemon","-d") )
        {
            daemon_ = true;
        }
        else if ( parser.GetSwitch( "-fullscreen", "-f" ) )
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
#else
#ifndef WIN32
        s << "-d, --daemon                 : allow the dedicated server to run as a daemon\n"
        << "                               (will not poll for input on stdin)\n";
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
        st_FirstUse=false;
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
        StartAAURLHandler();
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
            while (su_GetSDLInput(tEvent));
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

    sg_LanguageMenu();

    // catch some keyboard input
    {
        uInputProcessGuard inputProcessGuard;
        while (su_GetSDLInput(tEvent));
    }

    timeout = tSysTimeFloat() + 10;

    sr_UnlockSDL();
    uInputProcessGuard inputProcessGuard;
    while((!su_GetSDLInput(tEvent) || tEvent.type!=SDL_KEYDOWN) &&
            tSysTimeFloat() < timeout){

        sr_ResetRenderState(true);
        rViewport::s_viewportFullscreen.Select();

        if ( sr_glOut )
        {
            rSysDep::ClearGL();

            uMenu::GenericBackground();

            REAL w=16*2/640.0;
            REAL h=32*2/480.0;


            //REAL middle=-.6;

            Color(1,1,1);
            DisplayText(0,.8,w,h,tOutput("$welcome_message_heading"));

            w/=2;
            h/=2;

            rTextField c(-.8,.6, w, h);


            c << tOutput("$welcome_message_intro");

            c.SetIndent(12);

            c << tOutput("$welcome_message_vendor")   << gl_vendor   << '\n';
            c << tOutput("$welcome_message_renderer") << gl_renderer << '\n';
            c << tOutput("$welcome_message_version")  << gl_version  << '\n';

            c.SetIndent(0);

            c << tOutput("$welcome_message_finish");

            rSysDep::SwapGL();
        }

        tAdvanceFrame();
    }
    sr_LockSDL();

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
            uMenu::quickexit=true;
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
    rSurface tex( "desktop/icons/medium/armagetronad.png" );
    //    SDL_Surface *tex=IMG_Load( tDirectories::Data().GetReadPath( "textures/icon.png" ) );

    if (tex.GetSurface())
        SDL_WM_SetIcon(tex.GetSurface(),NULL);
#endif
#endif
}

static const char * dedicatedSection = "DEDICATED";

int main(int argc,char **argv){
    //std::cout << "enter\n";
    //  net_test();

    bool dedicatedServer = false;

    //  std::cout << "Running " << argv[0] << "...\n";

    // tERR_MESSAGE( "Start!" );

    try
    {
        tCommandLineData commandLine;
        commandLine.programVersion_  = &sn_programVersion;

        // analyse command line
        // tERR_MESSAGE( "Analyzing command line." );
        if ( ! commandLine.Analyse(argc, argv) )
            return 0;

        // read RAND_MAX from recording or write it to recording
        if ( !tRecorder::PlaybackStrict( dedicatedSection, dedicatedServer ) )
        {
#ifdef DEDICATED
            dedicatedServer = true;
#endif
        }
        tRecorder::Record( dedicatedSection, dedicatedServer );

        // tERR_MESSAGE( "Initializing player data." );
        ePlayer::Init();

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

        gAICharacter::LoadAll(tString( "aiplayers.cfg" ) );

        sg_LanguageInit();
        atexit(tLocale::Clear);

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
		#endif

            //std::cout << "checked mp\n";

            // while DGA mouse is buggy in XFree 4.0:
		#ifdef linux
            // Sam 5/23 - Don't ever use DGA, we don't need it for this game.
            if ( ! getenv("SDL_VIDEO_X11_DGAMOUSE") ) {
                putenv("SDL_VIDEO_X11_DGAMOUSE=0");
            }
		#endif

		#ifdef WIN32
            // disable DirectX by default; it causes problems with some boards.
            if (!use_directx && !getenv("SDL_VIDEODRIVER") ) {
                putenv("SDL_VIDEODRIVER=windib");
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
#endif
            if (SDL_Init(flags) < 0) {
                tERR_ERROR("Couldn't initialize SDL: " << SDL_GetError());
            }
            atexit(SDL_Quit);

            sr_glRendererInit();

		#ifndef NOJOYSTICK
            if (SDL_InitSubSystem(SDL_INIT_JOYSTICK))
                std::cout << "Error initializing joystick subsystem\n";
            else
            {
                std::cout << "Joystick(s) initialized\n";
                su_JoystickInit();
            }
		#endif

            SDL_SetEventFilter(&filter);

            //std::cout << "set filter\n";

            sg_SetIcon();

            tConsole::RegisterMessageCallback(&uMenu::Message);

            if (sr_InitDisplay()){

                try
                {
                    //std::cout << "init disp\n";

                    //std::cout << "init sound\n";

                    welcome();

                    //std::cout << "atexit\n";

                    sr_con.autoDisplayAtSwap=false;

                    //std::cout << "sound started\n";

                    gLogo::SetBig(false);
                    gLogo::SetSpinning(true);

                    sn_bigBrotherString = renderer_identification + "VER=" + sn_programVersion + "\n\n";

                    MainMenu();

                    // remove all players
                    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
                        se_PlayerNetIDs(i)->RemoveFromGame();

                    nNetObject::ClearAll();

                    rITexture::UnloadAll();
                    sr_RendererCleanup();
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

                //std::cout << "exit\n";

                st_SaveConfig();

                //std::cout << "saved\n";

                //    cleanup(grid);
                SDL_QuitSubSystem(SDL_INIT_VIDEO);
            }

            eSoundMixer::ShutDown();

            SDL_Quit();
		#else
            if (!commandLineAnalyzer.daemon_)
                sr_Unblock_stdin();

            sr_glOut=0;

            //  nServerInfo::TellMasterAboutMe();

            while (!uMenu::quickexit)
                sg_HostGame();
		#endif
            nNetObject::ClearAll();
            nServerInfo::DeleteAll();
        }

        ePlayer::Exit();


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



