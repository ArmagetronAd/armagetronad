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

#include "aa_config.h"
#include "rConsole.h"
#include "rFont.h"
#include "tConfiguration.h"

#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <io.h>
#include <windows.h>
//#define fileno _fileno
//#define fcntl _fcntl
#else
#include <signal.h>
#endif

class rInputStream
{
public:
#ifdef WIN32
    typedef HANDLE Descriptor;
#else
    typedef int Descriptor;
#endif

    rInputStream( rInputStream const & other )
    {
        descriptor_ = other.descriptor_;
        file_ = other.file_;
        line_in_ = other.line_in_;
        name_ = other.name_;

        other.file_ = 0;
    }

    rInputStream & operator = ( rInputStream const & other )
    {
        descriptor_ = other.descriptor_;
        file_ = other.file_;
        line_in_ = other.line_in_;
        name_ = other.name_;

        other.file_ = 0;

        return * this;
    }

    rInputStream()
    {
        descriptor_ = 
#ifdef WIN32
        GetStdHandle(STD_INPUT_HANDLE);
#else
        fileno(stdin);
#endif
        file_ = NULL;

        Unblock();
    }

    explicit rInputStream( Descriptor descriptor, char const * name, FILE * file = NULL )
    : descriptor_( descriptor ), file_( file ), name_( name )
    {
        Unblock();
    }

    // reads from the descriptor and
    // executes commands on newlines
    void HandleInput();

    ~rInputStream()
    {
        if( file_ )
        {
            fclose( file_ );
            file_ = NULL;
        }
    }
private:
    void Unblock()
    {
#ifndef WIN32
        int flag=fcntl(descriptor_,F_GETFL);
        fcntl(descriptor_,F_SETFL,flag | O_NONBLOCK);
#endif
    }

    Descriptor descriptor_;
    mutable FILE * file_;
    tString line_in_;
    tString name_;
};

void rConsole::DoCenterDisplay(const tString &s,REAL timeout,REAL r,REAL g,REAL b){
    std::cout << tColoredString::RemoveColors(s) << '\n';
    DisplayAtNewline();
}

static bool unblocked = false;

void sr_Unblock_stdin();

static void sr_HandleSigCont( int signal )
{
    // con << "Continuing.\n";

    // unblock stdin again
    sr_Unblock_stdin();
}

void sr_Unblock_stdin(){
#ifndef WIN32
    if ( !unblocked )
    {
        signal( SIGCONT, &sr_HandleSigCont );
    }
#endif

    unblocked = true;
    int stdin_descriptor=fileno( stdin );
#ifndef WIN32
    // if (isatty(stdin_descriptor))
    {
        int flag=fcntl(stdin_descriptor,F_GETFL);
        fcntl(stdin_descriptor,F_SETFL,flag | O_NONBLOCK);
    }
#endif
}

static tArray< rInputStream > sr_inputStreams;
static bool sr_daemon;

void sr_Read_stdin(){
    static bool inited = false;
    if( !inited )
    {
        inited = true;
        if( !sr_daemon )
        {
            sr_Unblock_stdin();
            sr_inputStreams[sr_inputStreams.Len()]=rInputStream();
        }
    }

    for( int i = sr_inputStreams.Len()-1; i >= 0; --i )
    {
        sr_inputStreams[i].HandleInput();
    }
}

#ifndef WIN32
#include "tCommandLine.h"

class rInputCommandLineAnalyzer: public tCommandLineAnalyzer
{
public:
    virtual bool DoAnalyze( tCommandLineParser & parser )
    {
        tString pipe;
        if ( parser.GetSwitch( "--daemon","-d") )
        {
            sr_daemon = true;

            return true;
        }
        else if( parser.GetOption( pipe, "--input" ) )
        {
            FILE * f = fopen( pipe, "r" );
            if( f )
            {
                sr_inputStreams[sr_inputStreams.Len()]=rInputStream( fileno(f), pipe, f );
                fseek( f, 0, SEEK_END );
            }
            else
            {
                std::cerr << "Error opening input file '" << pipe << "': "
                          << strerror( errno ) << ". Using stdin to poll for input.\n";
            }

            return true;
        }
        return false;
    }

    virtual void DoHelp( std::ostream & s )
    {                                      //
#ifndef WIN32
        s << "-d, --daemon                 : allow the dedicated server to run as a daemon\n"
          << "                               (will not poll for input, unless overridden by --input)\n";
        s << "--input <file>               : Poll for input from this file in addition to/instead of\n"
          <<  "                              (if -d is also given) stdin. Can be used multiple times.\n";
#endif
    }
};

static rInputCommandLineAnalyzer sr_analyzer;
#endif

void rInputStream::HandleInput()
{
    // stdin commands are executed at owner level
    tCurrentAccessLevel level( tAccessLevel_Owner, true );

    tConfItemBase::LoadPlayback( true );

#ifdef WIN32
    //  std::cerr << "\n";

    HANDLE stdouthandle = GetStdHandle(STD_OUTPUT_HANDLE);
    bool goon = true;
    while (goon)
    {
        unsigned long reallyread;
        INPUT_RECORD input;
        PeekConsoleInput(descriptor_, &input, 1, &reallyread);
        if (reallyread > 0)
        {
            ReadConsoleInput(descriptor_, &input, 1, &reallyread);
            if (input.EventType == KEY_EVENT)
            {
                char key = input.Event.KeyEvent.uChar.AsciiChar;
                DWORD written=0;

                if (key && input.Event.KeyEvent.bKeyDown)
                {
                    WriteConsole(stdouthandle, &key, 1, &written, NULL);
                    line_in_ << key;

                    if (key == 13 ){
                        line_in_<<'\n';
                        std::istringstream s((char const *)line_in_);
                        WriteConsole(stdouthandle, "\n", 1, &written, NULL);
                        tConfItemBase::LoadAll(s, true);
                        line_in_="";
                    }
                }
            }
        }
        else
            goon = false;
    }


#else
    char c;
    while ( read(descriptor_,&c,1)>0){
        line_in_ << c;
        if (c=='\n')
        {
            std::istringstream s((char const *)line_in_);
            if( name_.Len() > 1 )
            {
                con << name_ << " : " << line_in_;
            }
            tConfItemBase::LoadAll(s, true);
            line_in_="";
        }
    }
#endif
}

void rConsole::DisplayAtNewline(){
}

