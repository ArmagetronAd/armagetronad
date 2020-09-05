/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Copyright (C) 2004  Armagetron Advanced Team (http://sourceforge.net/projects/armagetronad/) 

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

// declaration
#ifndef		TCOMMANDLINE_H_INCLUDED
#include	"tCommandLine.h"
#endif

#include    "tLocale.h"
#include    "tConfiguration.h"
#include    "tException.h"

#ifdef WIN32
#include    <windows.h>
#include    <direct.h>
#endif

#undef 	INLINE_DEF
#define INLINE_DEF

static tCommandLineAnalyzer * s_commandLineAnalyzerAnchor;

static void quitWithMessagePrepare( const char* message )
{
#ifdef WIN32
#ifndef DEDICATED
#define USEBOX
#endif
#endif

#ifdef USEBOX
    int result = MessageBox (NULL, message , "Message", MB_OK);
#else
    std::cout << message;
#endif

    tLocale::Clear();
}

static void quitWithMessage( const char* message )
{
    tLocale::Clear();
    quitWithMessagePrepare( message );
    throw 1;

    // tGenericException( message, "Command Line Parsing Error" );
    // exit(1);
}

//#define QUIT(x) { std::ostringstream s; s << x; quitWithMessage(s.str().c_str()); name_.Clear(); } exit(0)
//#define QUIT(x) { std::ostringstream s; s << x; quitWithMessage(s.str().c_str()); name_.Clear(); } return false
#define QUIT(x) { std::ostringstream s; s << x; quitWithMessage(s.str().c_str()); name_.Clear();}
#define CLEAN_QUIT(x) { std::ostringstream s; s << x; quitWithMessagePrepare(s.str().c_str()); name_.Clear(); return false; }

bool tCommandLineData::Analyse(int argc,char **argv)
{
#ifdef DEBUG_X
#ifdef WIN32
#define getcwd _getcwd
#endif

    char * cwd = getcwd(0,0);
    tERR_MESSAGE( "Executable: " << argv[0] << ", CWD: " << cwd );
    free( cwd );
#endif

    tCommandLineParser parser( argc, argv );
    {
        tASSERT( programVersion_ );

        char const * run = parser.Current();
        while (*run)
        {
            if (*run == '\\' || *run == '/')
                name_ = run+1;
            run++;
        }

        if ( name_.Len() <= 3 )
        {
            name_ = "Armagetron";
        }
    }


    // initialize third party analyzers
    {
        tCommandLineAnalyzer * commandLineAnalyzer = s_commandLineAnalyzerAnchor;
        while ( commandLineAnalyzer )
        {
            commandLineAnalyzer->Initialize( parser );
            commandLineAnalyzer = commandLineAnalyzer->Next();
        }
    }

    parser.Advance();

    //std::cout << "config loaded\n";

#ifndef WIN32   
#define HELPAVAIL
#endif
#ifdef DEDICATED  
#define HELPAVAIL
#endif

    while ( !parser.End() )
    {
        if ( parser.GetSwitch( "--help", "-h" ) )
        {
            {
                std::ostringstream s;
                s << "\n\nUsage: " << name_ << " [Arguments]\n\n"
                << "Possible arguments:\n\n";
                s << "-h, --help                   : print this message\n";
#ifdef HELPAVAIL
                s << "--doc                        : print documentation for all console commands\n";
#endif
                s << "-v, --version                : print version number\n\n";

                // ask third party analyzers
                tCommandLineAnalyzer * commandLineAnalyzer = s_commandLineAnalyzerAnchor;
                while ( commandLineAnalyzer )
                {
                    commandLineAnalyzer->Help( s );
                    commandLineAnalyzer = commandLineAnalyzer->Next();
                    s << "\n";
                }

                s << "\n";

                name_.Clear();
                quitWithMessagePrepare( s.str().c_str() );
            }

            return false;
            // exit(0);
        }
#ifdef HELPAVAIL
        else if ( parser.GetSwitch( "--doc") )
        {
            doc_ = true;
        }
#endif
        else if ( parser.GetSwitch( "--version", "-v") )
        {
            CLEAN_QUIT( "This is " << name_ << " version " << *programVersion_ << ".\n" );
        }
        else
        {
            // let the registered command line anelyzers have a go
            tCommandLineAnalyzer * commandLineAnalyzer = s_commandLineAnalyzerAnchor;
            bool success = false;
            while ( commandLineAnalyzer )
            {
                if ( ( success = commandLineAnalyzer->Analyze( parser ) ) )
                    break;
                commandLineAnalyzer = commandLineAnalyzer->Next();
            }
            if ( success )
                continue;

            QUIT( "\n\nUnknown command line option " << parser.Current() << ". Type " << name_ << " -h to get a list of possible options.\n\n" );
        }
    }

    return true;
}

bool tCommandLineData::Execute()
{
    tString name;

    if ( doc_ )
    {
        std::cout << "Available console commands/config file settings:\n\n";
        tConfItemBase::DocAll( std::cout );

        return false;
        // QUIT("\n");
    }

    return true;
}

// *******************************************************************************************
// *
// *   GetSwitch
// *
// *******************************************************************************************
//!
//!        @param  option            long version of the switch
//!        @param  option_short      short version of the switch
//!       @return                   true if the switch was detected
//!
// *******************************************************************************************

bool tCommandLineParser::GetSwitch( char const * option, char const * option_short )
{
    if ( End() )
        return false;

    char * argument = argv[index];
    tASSERT( argument );
    if ( !strcmp(argument,option) || ( option_short && !strcmp(argument,option_short ) ) )
    {
        index++;
        return true;
    }

    return false;
}

// *******************************************************************************************
// *
// *   GetOption
// *
// *******************************************************************************************
//!
//!        @param  target            string to store option to
//!     @param  option            long version of the option
//!     @param  option_short      short version of the option
//!     @return                   true if the option was detected
//!
// *******************************************************************************************

bool tCommandLineParser::GetOption( tString & target, char const * option, char const * option_short )
{
    if ( End() )
        return false;

    char * argument = argv[index];
    tASSERT( argument );
    if ( GetSwitch( option, option_short ) )
    {
        if ( !End() )
        {
            target = argv[index];
            index++;
            return true;
        }
        else
        {
            index--;
            tString name_;
            QUIT( "  " << argument << " needs another argument.\n" );
        }
    }

    return false;
}

// *******************************************************************************************
// *
// *   End
// *
// *******************************************************************************************
//!
//!        @return     true if the options have been parsed to the end
//!
// *******************************************************************************************

bool tCommandLineParser::End( void ) const
{
    return ( index >= argc );
}

// *******************************************************************************************
// *
// *   tCommandLineParser
// *
// *******************************************************************************************
//!
//!        @param  a_argc  number of arguments
//!     @param  a_argv  arguments
//!
// *******************************************************************************************

tCommandLineParser::tCommandLineParser( int a_argc, char * * a_argv )
        : argc( a_argc ), argv( a_argv ), index( 0 )
{
}

// *******************************************************************************************
// *
// *   tCommandLineParser
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

/*
tCommandLineParser::tCommandLineParser( void )
{
}
*/

// *******************************************************************************************
// *
// *   Executable
// *
// *******************************************************************************************
//!
//!        @return     the full path of the executable
//!
// *******************************************************************************************

const char * tCommandLineParser::Executable( void ) const
{
    return argv[ 0 ];
}

// *******************************************************************************************
// *
// *   Current
// *
// *******************************************************************************************
//!
//!        @return     the current command line option
//!
// *******************************************************************************************

const char * tCommandLineParser::Current( void ) const
{
    return argv[ index ];
}

// *******************************************************************************************
// *
// *   Advance
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void tCommandLineParser::Advance( void )
{
    tASSERT( !End() );
    index ++;
}


// *******************************************************************************************
// *
// *   tCommandLineAnalyzer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tCommandLineAnalyzer::tCommandLineAnalyzer( void )
        : tListItem< tCommandLineAnalyzer >( s_commandLineAnalyzerAnchor )
{
}

// *******************************************************************************************
// *
// *   ~tCommandLineAnalyzer
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

tCommandLineAnalyzer::~tCommandLineAnalyzer( void )
{
}

// *******************************************************************************************
// *
// *   DoAnalyze
// *
// *******************************************************************************************
//!
//!        @param  parser  parser to analyze
//!
// *******************************************************************************************

void tCommandLineAnalyzer::DoInitialize( tCommandLineParser & parser )
{
}

// *******************************************************************************************
// *
// *   DoAnalyze
// *
// *******************************************************************************************
//!
//!        @param  parser  parser to analyze
//!       @return         true if anaysis was succesful
//!
// *******************************************************************************************

bool tCommandLineAnalyzer::DoAnalyze( tCommandLineParser & parser )
{
    return false;
}

// *******************************************************************************************
// *
// *   DoHelp
// *
// *******************************************************************************************
//!
//!        @param  s   string to write help to
//!
// *******************************************************************************************

void tCommandLineAnalyzer::DoHelp( std::ostream & s )
{
}

