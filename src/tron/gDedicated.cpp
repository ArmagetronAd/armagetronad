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
#include "tRuby.h"


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
#ifndef WIN32
        s << "-d, --daemon                 : allow the dedicated server to run as a daemon\n"
        << "                               (will not poll for input on stdin)\n";
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
    }
}


//from game.C
void Update_netPlayer();

void sg_SetIcon()
{
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
                dedicatedServer = true;
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

        // tERR_MESSAGE( "Initializing player data." );
        ePlayer::Init();

#ifdef HAVE_LIBRUBY
        tRuby::InitializeInterpreter();
        try {
            tRuby::Load(tDirectories::Data(), "scripts/initialize.rb");
        }
        catch (std::runtime_error & e) {
            std::cerr << e.what() << '\n';
        }
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

            if (!commandLineAnalyzer.daemon_)
                sr_Unblock_stdin();

            sr_glOut=0;

            //  nServerInfo::TellMasterAboutMe();

            while (!uMenu::quickexit)
                sg_HostGame();
            nNetObject::ClearAll();
            nServerInfo::DeleteAll();
        }

        ePlayer::Exit();

#ifdef HAVE_LIBRUBY
        tRuby::CleanupInterpreter();
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

// settings missing in the dedicated server
static void st_Dummy(std::istream &s){tString rest; rest.ReadLine(s);}
static tConfItemFunc st_Dummy10("MASTER_QUERY_INTERVAL", &st_Dummy);
static tConfItemFunc st_Dummy11("MASTER_SAVE_INTERVAL", &st_Dummy);
static tConfItemFunc st_Dummy12("MASTER_IDLE", &st_Dummy);
static tConfItemFunc st_Dummy13("MASTER_PORT", &st_Dummy);



