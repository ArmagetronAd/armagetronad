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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#include "tCommandLine.h"
#include "tLocale.h"
#include "tConfiguration.h"
#include "tException.h"

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#endif

#if !defined(WIN32) || defined(DEDICATED)
#define HELP_AVAILABLE
#endif

static void InformWithMessage( const char* message )
{
#if defined(WIN32) && !defined(DEDICATED)
    int result = MessageBox (NULL, message , "Message", MB_OK);
#else
    std::cout << message;
#endif
    tLocale::Clear();
}

#define INFORM_HELPER(x) { std::ostringstream s; s << x; InformWithMessage(s.str().c_str()); }

static tCommandLineAnalyzer *s_commandLineAnalyzerAnchor;

tCommandLineData::tCommandLineData( const tString & programVersion, tCommandLineAnalyzer *& anchor )
    :programVersion_( programVersion ), extraProgamUsage_(""), commandLineAnalyzerAnchor_( anchor )
{
}

tCommandLineData::tCommandLineData( const tString & programVersion )
    :programVersion_( programVersion ), extraProgamUsage_(""), commandLineAnalyzerAnchor_( s_commandLineAnalyzerAnchor )
{
}

bool tCommandLineData::Analyse(int argc,char **argv)
{
    tCommandLineParser parser( argc, argv );
    
    // initialize third party analyzers
    {
        tCommandLineAnalyzer * commandLineAnalyzer = commandLineAnalyzerAnchor_;
        while ( commandLineAnalyzer )
        {
            commandLineAnalyzer->Initialize( parser );
            commandLineAnalyzer = commandLineAnalyzer->Next();
        }
    }

    parser.Advance();

    while ( !parser.End() )
    {
        if ( parser.GetSwitch( "--help", "-h" ) )
        {
            std::ostringstream s;
            s << "Usage: " << parser.ExecutableName() << " [options]" << extraProgamUsage_ << '\n'
              << "Options:\n\n"
              << "-h, --help                   : print this message\n"
              << "-v, --version                : print version number\n";

            // ask third party analyzers
            tCommandLineAnalyzer * commandLineAnalyzer = commandLineAnalyzerAnchor_;
            while ( commandLineAnalyzer )
            {
                commandLineAnalyzer->Help( s );
                commandLineAnalyzer = commandLineAnalyzer->Next();
                s << "\n";
            }
            
            InformWithMessage( s.str().c_str() );
            return false;
        }
        else if ( parser.GetSwitch( "--version", "-v" ) )
        {
            INFORM_HELPER( "This is " << parser.ExecutableName() << " version " << programVersion_ << ".\n" );
            return false;
        }
        else
        {
            // let the registered command line anelyzers have a go
            tCommandLineAnalyzer * commandLineAnalyzer = commandLineAnalyzerAnchor_;
            bool success = false;
            while ( commandLineAnalyzer )
            {
                if ( ( success = commandLineAnalyzer->Analyze( parser ) ) )
                    break;
                commandLineAnalyzer = commandLineAnalyzer->Next();
            }
            
            if ( !success )
            {
                INFORM_HELPER( "\nUnknown command line option or error parsing option " << parser.Current() << ". Type " << parser.ExecutableName() << " -h to get a list of possible options.\n" );
                throw -1;
            }
        }
    }

    return true;
}

bool tCommandLineData::Execute()
{
    tCommandLineAnalyzer * commandLineAnalyzer = commandLineAnalyzerAnchor_;
    while ( commandLineAnalyzer )
    {
        if ( !commandLineAnalyzer->Execute() )
        {
            return false;
        }
        commandLineAnalyzer = commandLineAnalyzer->Next();
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
            INFORM_HELPER( "  " << argument << " needs another argument.\n" );
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

const char * tCommandLineParser::ExecutableName() const
{
    const char *run = argv[ 0 ];
    const char *name = run;
    while ( *run )
    {
        if ( *run == '\\' || *run == '/' )
            name = run + 1;
        run++;
    }
    return name;
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

tCommandLineAnalyzer::tCommandLineAnalyzer()
        : tListItem< tCommandLineAnalyzer >( s_commandLineAnalyzerAnchor )
{
}

tCommandLineAnalyzer::tCommandLineAnalyzer( tCommandLineAnalyzer *& anchor )
        : tListItem< tCommandLineAnalyzer >( anchor )
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

bool tCommandLineAnalyzer::DoExecute()
{
}

bool tDefaultCommandLineAnalyzer::DoAnalyze( tCommandLineParser & parser )
{
#ifdef HELP_AVAILABLE
    if ( parser.GetSwitch( "--doc" ) )
    {
        docOption_ = true;
        return true;
    }
#endif
    else if ( parser.GetSwitch( "--versioninfo") )
    {
        versioninfoOption_ = true;
        return true;
    }
    return false;
}

void tDefaultCommandLineAnalyzer::DoHelp( std::ostream & s )
{
#ifdef HELP_AVAILABLE
    s << "--doc                        : print documentation for all console commands\n";
#endif
    s << "--versioninfo                : print build source information\n\n";
}

bool tDefaultCommandLineAnalyzer::DoExecute()
{
    if ( docOption_ )
    {
        std::cout << "Available console commands/config file settings:\n\n";
        tConfItemBase::DocAll( std::cout );
        return false;
    }
    else if (versioninfoOption_)
    {
        std::ostringstream s;
        s << "Program Name                 : " << st_programName << "\n";
        s << "Version                      : " << st_programVersion << "\n";
        s << "Parent branch                : " << st_programBranchNick << "\n";
        s << "Parent branch's URL          : " << st_programBranchUrl << "\n";
        s << "Tag                          : " << st_programRevTag << "\n";
        s << "Revision number              : r" << st_programRevNo << "(z" << st_programRevZNr << ")\n";
        s << "Revision ID                  : " << st_programRevId << "\n";
        s << "Ancestor                     : r" << st_programBranchLca << "(z" << st_programBranchLcaZ << ")\n";
        s << "Source changed               : " << (st_programChanged? "Yes" : "No") << "\n";
        s << "Build date                   : " << st_programBuildDate << "\n";

        InformWithMessage( s.str().c_str() );
        return false;
    }
    
    return true;
}
