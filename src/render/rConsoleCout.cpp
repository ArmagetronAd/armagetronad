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

#include "config.h"
#include "rConsole.h"
#include "rFont.h"
#include "tConfiguration.h"
#include "tRecorder.h"
#include "tDirectories.h"
#include "tLocale.h"

#include <map>

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
#include <sys/wait.h>
#endif

#ifdef TOP_SOURCE_DIR
 #include "nTrueVersion.h"
 #endif

 #ifndef TRUE_ARMAGETRONAD_VERSION
 #define TRUE_ARMAGETRONAD_VERSION VERSION
 #endif

class rStream: public tReferencable< rStream >
{
    rStream( rStream const & other );
public:
    rStream(){};
    virtual ~rStream(){};

    // reads from the descriptor and
    // executes commands on newlines.
    // Return value of 'false' means the stream should be removed.
    virtual bool HandleInput(){ return true; }

    // writes output to potential scripts
    virtual void Output( char const * output ){}
};

class rInputStream: public rStream
{
public:
#ifdef WIN32
    typedef HANDLE Descriptor;
#else
    typedef int Descriptor;
#endif

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

    rInputStream( Descriptor descriptor, char const * name, FILE * file = NULL )
    : descriptor_( descriptor ), file_( file ), name_( name )
    {
        Unblock();
    }

    // reads from the descriptor and
    // executes commands on newlines
    bool HandleInput();

    ~rInputStream()
    {
        if( file_ )
        {
            fclose( file_ );
            file_ = NULL;
        }
    }

    tString const & GetName()
    {
        return name_;
    }
protected:
    Descriptor descriptor_;
    FILE * file_;
    tString name_;

private:
    void Unblock()
    {
#ifndef WIN32
        int flag=fcntl(descriptor_,F_GETFL);
        fcntl(descriptor_,F_SETFL,flag | O_NONBLOCK);
#endif
    }

    tString line_in_;
};

#ifndef WIN32
// stream to and from an external script
class rScriptStream: public rInputStream
{
public:
    explicit rScriptStream( Descriptor in, Descriptor out, char const * name, pid_t pid )
    : rInputStream( in, name ), pid_( pid ), outDescriptor_( out )
    {
    }

    virtual bool HandleInput()
    {
        return rInputStream::HandleInput() && ( 0 == kill( pid_, 0 ) );
    }

    // writes output to potential scripts
    virtual void Output( char const * output )
    {
        int len = strlen( output );
        while( len > 0 )
        {
            int written = write( outDescriptor_, output, len );
            if( written <= 0 )
            {
                std::cerr << "Error writing to input stream of script '" << name_ << "' : "
                          << strerror( errno ) << ". Killing script.\n";
                Close();
                return;
            }
            output += written;
            len -= written;
        }
    }

    ~rScriptStream()
    {
        Close();
    }

    // closes the streams, hopefully killing the script
    void Close()
    {
        // close the streams
        if( !file_ && descriptor_ >= 0 )
        {
            close( descriptor_ );
            descriptor_ = -1;
        }
        if( outDescriptor_ >= 0 )
        {
            close( outDescriptor_ );
            outDescriptor_ = -1;
        }

        name_ = "";
    }
private:
    pid_t pid_;
    Descriptor outDescriptor_;
};
#endif


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

#ifndef WIN32
static void sr_HandleSigChild( int signal )
{
    int stat;

    /*Kills all the zombie processes*/
    while(waitpid(-1, &stat, WNOHANG) > 0) {}
}
#endif

void sr_Unblock_stdin(){
#ifndef WIN32
    if ( !unblocked )
    {
        signal( SIGCONT, &sr_HandleSigCont );
        signal( SIGCHLD, &sr_HandleSigChild );
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

static tArray< tJUST_CONTROLLED_PTR< rStream > > sr_inputStreams;
static bool sr_daemon;

// passes ladderlog output to external scripts
void sr_InputForScripts( char const * input )
{
    for( int i = sr_inputStreams.Len()-1; i >= 0; --i )
    {
        sr_inputStreams[i]->Output( input );
    }
}

void sr_Read_stdin(){
    static bool inited = false;
    if( !inited )
    {
        inited = true;
        if( !sr_daemon )
        {
            sr_Unblock_stdin();
            sr_inputStreams[sr_inputStreams.Len()]= tNEW(rInputStream)();
        }
    }

    for( int i = sr_inputStreams.Len()-1; i >= 0; --i )
    {
        if( !sr_inputStreams[i]->HandleInput() )
        {
            // delete stream
            if( i < sr_inputStreams.Len()-1 )
            {
                sr_inputStreams[i] = sr_inputStreams[ sr_inputStreams.Len()-1 ];
            }
            else
            {
                sr_inputStreams[i] = 0;
            }
            sr_inputStreams.SetLen( sr_inputStreams.Len()-1 );
        }
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
                sr_inputStreams[sr_inputStreams.Len()] = tNEW(rInputStream)( fileno(f), pipe, f );
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

bool rInputStream::HandleInput()
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

    return true;
#else
    char c = 0;
    int lenRead;
    while ( (lenRead=read(descriptor_,&c,1))>0){
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

    // 0 return on lenRead means end of file,
    // -1 means an error unless errno has these specific values,
    // in which case there is just no data currently.
    return ( lenRead == -1 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) || ( lenRead == 0 && file_ );
#endif
}

void rConsole::DisplayAtNewline(){
}


#ifdef HAVE_UNISTD_H

#define READ 0
#define WRITE 1

// launches shell command with pipes attached to it
pid_t
popen2(const char *command, int *infp, int *outfp, char const ** envp)
{
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);

        execle("/bin/sh", "sh", "-c", command, NULL, envp);
        perror("execl");
        exit(1);
    }

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    return pid;
}

#ifdef KRAWALL_SERVER
static rScriptStream * sr_FindScriptStream( tString const & name )
{
    for( int i = sr_inputStreams.Len()-1; i >= 0; --i )
    {
        rScriptStream * script = dynamic_cast< rScriptStream * >( (rStream*)sr_inputStreams[i] );
        if( script && script->GetName() == name )
        {
            return script;
        }
    }

    return NULL;
}

static bool sr_sanityCheckScript = true;
static tConfItem<bool> src_sanityCheckScript( "CHECK_SCRIPT", sr_sanityCheckScript );
static tAccessLevelSetter src_sanityCheckScriptALS( src_sanityCheckScript, tAccessLevel_Owner );

class rEnvironment
{
public:
    const char ** GetRaw()
    {
        // fill and terminate envp
        envp_.SetLen(0);
        for( int i = 0; i < strings_.Len(); ++i )
        {
            envp_[i] = strings_[i];
        }
        envp_[strings_.Len()] = NULL;

        return &envp_[0];
    }

    rEnvironment()
    {
    }

    void AddAll( const std::map< tString, tString > & m )
    {
        std::map< tString, tString >::const_iterator it = m.begin();
        for ( ; it != m.end(); ++it )
        {
            Add( it->first, it->second );
        }
    }
    void Add( char const * var, tString const & value )
    {
        strings_[strings_.Len()] = tString(var) + "=" + value;
    }

    void AddPath( char const * var, tPath const & path )
    {
        Add( var, path.GetPaths(":","") );
    }
private:
    tArray< char const * > envp_;
    tArray< tString > strings_;
};

static std::map< tString, tString > sr_globalScriptEnv;

static void sr_ScriptEnv( std::istream & s )
{
    tString key, value;
    s >> key;
    s >> value;
    sr_globalScriptEnv[key] = value;
}

static tConfItemFunc sr_scriptEnvConf( "SCRIPT_ENV", sr_ScriptEnv );
static tAccessLevelSetter sr_scriptEnvALS( sr_scriptEnvConf, tAccessLevel_Owner );

static void sr_SpawnScript( tString const & command )
{
    // yes, rincludes are the one bit where CASACL is forbidden. And Maps, which
    // are equivalent to RINCLUDES.
    // change that assumption and hopefully, the name of this
    // function will tip you off something needs to be changed here.
    if( tCasaclPreventer::InRInclude() )
    {
        con << "Launching scripts from RINCLUDE or maps is not possible for security reasons. Work around it by delegating the actual script launch to a local configuration file.\n";
        return;
    }

    if( sr_sanityCheckScript )
    {
        char needle[2];
        needle[1] = 0;
        char const * allowed = ".,?/\\\"!@#$%^*-_+=[]~";

        for( int i = command.Len()-2; i >= 0; --i )
        {
            // only whitespace and alphanumeric characters plus a few exceptions are allowed
            needle[0] = command[i];
            if( !isalnum(needle[0]) && !isblank(needle[0]) && !strstr( allowed, needle ) )
            {
                con << "External command \'" << command << "\' contains forbidden characters, only alphanumeric or blank characters are allowed, plus any of '" << allowed << "'.\n";
                return;
            }
        }
    }

    if( !tRecorder::IsPlayingBack() )
    {
        tString fullCommand;

        {
            tString script;
            tString arguments;
            {
                std::stringstream s( &command[0] );
                s >> script;
                arguments.ReadLine(s);
            }

            // find full path
            tString path = tDirectories::Data().GetReadPath(tString("scripts/") + script);
            if( path.Len() > 1 )
            {
                fullCommand = path + ' ' + arguments;
            }
            else if( sr_sanityCheckScript )
            {
                con << "External command \'" << command << "\' not found anywhere in <datapath>/scripts/.\n";
                return;
            }
            else
            {
                fullCommand = command;
            }
        }

        con << "Launching external command \'" << fullCommand << "\'...\n";

        // get the var directory; it's the last entry
        // tArray<tString> varPaths;
        // tDirectories::Var().Paths(varPaths);
        // tString varPath = varPaths(varPaths.Len()-1);
        tString varPath = tDirectories::Var().GetWritePath("x");
        if( varPath.Len() > 2 )
        {
            varPath = varPath.SubStr(0, varPath.Len()-3);
        }
        else
        {
            varPath = "./";
        }

        tString resourcePath = tDirectories::Resource().GetWritePath("x");
        if( resourcePath.Len() > 2 )
        {
            resourcePath = resourcePath.SubStr(0, resourcePath.Len()-3);
        }
        else
        {
            resourcePath = "./";
        }

        rEnvironment env;
        tString newPath=
        tDirectories::Data().GetPaths("/scripts:","/scripts:") +
        tDirectories::Config().GetPaths(":",":") +
        getenv("PATH");
        env.Add( "PATH", newPath );

        // add special directories
        env.Add( "ARMAGETRONAD_DIR_VAR", varPath );
        env.Add( "ARMAGETRONAD_DIR_RESOURCE", resourcePath );
        env.Add( "ARMAGETRONAD_DIR_RESOURCE_INCLUDED", tDirectories::Resource().GetIncluded() );

        // add path collections
        env.AddPath( "ARMAGETRONAD_PATH_DATA", tDirectories::Data() );
        env.AddPath( "ARMAGETRONAD_PATH_CONFIG", tDirectories::Config() );
        env.AddPath( "ARMAGETRONAD_PATH_VAR", tDirectories::Var() );
        env.AddPath( "ARMAGETRONAD_PATH_SCREENSHOT", tDirectories::Screenshot() );
        env.AddPath( "ARMAGETRONAD_PATH_RESOURCE", tDirectories::Resource() );

        // add other data
        env.Add("ARMAGETRONAD_ENCODING", st_internalEncoding);
        env.Add( "ARMAGETRONAD_VERSION", tString( TRUE_ARMAGETRONAD_VERSION ) );

        // add user-specified variables
        env.AddAll( sr_globalScriptEnv );

        // add all settings
        tConfItemBase::tConfItemMap const & confItemMap = tConfItemBase::GetConfItemMap();
        for( tConfItemBase::tConfItemMap::const_iterator iter = confItemMap.begin();
             iter != confItemMap.end(); ++iter )
        {
            if( !(*iter).second->CanSave() || (*iter).first.StartsWith( "PASSWORD" ) )
            {
                // yeah, well, password storage. Not really an issue, unlikely to be
                // set on the server anyway, the script can read the config directly
                // just as well, but we don't want script authors ot freak out when they
                // notice it in the environment.
                continue;
            }

            std::stringstream s;
            (*iter).second->WriteVal(s);
            env.Add( tString("ARMAGETRONAD_") + (*iter).first, tString( s.str().c_str() ) );
        }

        int infp, outfp;
        pid_t pid = popen2( fullCommand, &infp, &outfp, env.GetRaw() );
        sr_inputStreams[sr_inputStreams.Len()] = tNEW(rScriptStream)( outfp, infp, command, pid );
    }
    else
    {
        con << "Launching external command \'" << command << "\'...\n";
    }
}

// spawns a script
static void sr_SpawnScriptCommand( std::istream & s )
{
    tString command;
    command.ReadLine(s);

    sr_SpawnScript( command );
}

static tConfItemFunc sr_spawnScript( "SPAWN_SCRIPT", sr_SpawnScriptCommand );
static tAccessLevelSetter sr_spawnScriptALS( sr_spawnScript, tAccessLevel_Owner );

// respawns a script
static void sr_RespawnScriptCommand( std::istream & s )
{
    tString command;
    command.ReadLine(s);

    if( !sr_FindScriptStream( command ) )
    {
        sr_SpawnScript( command );
    }
}

static tConfItemFunc sr_respawnScript( "RESPAWN_SCRIPT", sr_RespawnScriptCommand );
static tAccessLevelSetter sr_respawnScriptALS( sr_respawnScript, tAccessLevel_Owner );

static void sr_KillScript( tString const & command, bool shouldWarn=true )
{
    rScriptStream * stream = sr_FindScriptStream( command );
    if( stream )
    {
        con << "Killing script \'" << command << "\'.\n";
        stream->Close();
    }
    else if ( shouldWarn )
    {
        con << "No script named \'" << command << "\' running.\n";
    }
}

// force respawn a script
static void sr_ForceRespawnScriptCommand( std::istream & s )
{
    tString command;
    command.ReadLine(s);

    sr_KillScript( command, false );
    sr_SpawnScript( command );
}

static tConfItemFunc sr_forceRespawnScript( "FORCE_RESPAWN_SCRIPT", sr_ForceRespawnScriptCommand );
static tAccessLevelSetter sr_forceRespawnScriptALS( sr_forceRespawnScript, tAccessLevel_Owner );

// kills a script
static void sr_KillScriptCommand( std::istream & s )
{
    tString command;
    command.ReadLine(s);

    sr_KillScript( command );
}

static tConfItemFunc sr_killScript( "KILL_SCRIPT", sr_KillScriptCommand );
static tAccessLevelSetter sr_killScriptALS( sr_killScript, tAccessLevel_Owner );

void sr_ListScriptsCommand( std::istream & s )
{
    int numberScripts = 0;
    for( int i = sr_inputStreams.Len()-1; i >= 0; --i )
    {
        rScriptStream * script = dynamic_cast< rScriptStream * >( (rStream*)sr_inputStreams[i] );
        if( script )
        {
            numberScripts++;
            con << "Script: " << script->GetName() << '\n';
        }
    }
    if (!numberScripts)
        con << "No scripts are currently running.\n";
}
static tConfItemFunc sr_listScripts( "LIST_SCRIPTS", sr_ListScriptsCommand );
static tAccessLevelSetter sr_listScriptALS( sr_listScripts, tAccessLevel_Owner );

#endif /* KRAWALL_SERVER */
#endif /* HAVE_UNISTD_H */
