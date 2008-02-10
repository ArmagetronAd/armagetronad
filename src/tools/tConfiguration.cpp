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
#include <fstream>
#include <iomanip>
#include <iostream>
#include "tConfiguration.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <sstream>
#include "tString.h"
#include "tToDo.h"
#include "tConsole.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "tRecorder.h"
#include "tCommandLine.h"
#include "tResourceManager.h"
#include "tError.h"

#include <vector>
#include <string.h>

#ifndef WIN32
#include <signal.h>
#endif

bool           tConfItemBase::printChange=true;
bool           tConfItemBase::printErrors=true;


//! @param newLevel         the new access level to set over the course of the lifetime of this object
//! @param allowElevation   only if set to true, getting higher access rights is possible. Use with extreme care.
tCurrentAccessLevel::tCurrentAccessLevel( tAccessLevel newLevel, bool allowElevation )
{
    // prevent elevation
    if ( !allowElevation && newLevel < currentLevel_ )
    {
        // you probably want to know when this happens in the debugger
        st_Breakpoint();
        newLevel = currentLevel_;
    }

    lastLevel_ = currentLevel_;
    currentLevel_ = newLevel;
}


tCurrentAccessLevel::tCurrentAccessLevel()
{
    lastLevel_ = currentLevel_;
}

tCurrentAccessLevel::~tCurrentAccessLevel()
{
    currentLevel_ = lastLevel_;
}

//! returns the current access level
tAccessLevel tCurrentAccessLevel::GetAccessLevel()
{
    return currentLevel_;
}

// returns the name of an access level
tString tCurrentAccessLevel::GetName( tAccessLevel level )
{
    std::ostringstream s;
    s << "$config_accesslevel_" << level;
    return tString( tOutput( s.str().c_str() ) );
}

tAccessLevel tCurrentAccessLevel::currentLevel_ = tAccessLevel_Owner; //!< the current access level

tAccessLevelSetter::tAccessLevelSetter( tConfItemBase & item, tAccessLevel level )
{
    item.requiredLevel = level;
}

static std::map< tString, tConfItemBase * > * st_confMap = 0;
tConfItemBase::tConfItemMap & tConfItemBase::ConfItemMap()
{
    if (!st_confMap)
        st_confMap = tNEW( tConfItemMap );
    return *st_confMap;
}

#ifdef KRAWALL_SERVER

// changes the access level of a configuration item
class tConfItemLevel: public tConfItemBase
{
public:
    tConfItemLevel()
    : tConfItemBase( "ACCESS_LEVEL" )
    {
        requiredLevel = tAccessLevel_Owner;
    }

    virtual void ReadVal(std::istream &s)
    {
        // read name and access level
        tString name;
        s >> name;

        int levelInt;
        s >> levelInt;
        tAccessLevel level = static_cast< tAccessLevel >( levelInt );

        if ( s.fail() )
        {
            con << tOutput( "$access_level_usage" );
            return;
        }

        // make name uppercase:
        tToUpper( name );

        // find the item
        tConfItemMap & confmap = ConfItemMap();
        tConfItemMap::iterator iter = confmap.find( name );
        if ( iter != confmap.end() )
        {
            // and change the level
            tConfItemBase * ci = (*iter).second;
            if ( ci->requiredLevel != level )
            {
                ci->requiredLevel = level;
                con << tOutput( "$access_level_change", name, tCurrentAccessLevel::GetName( level ) );
            }
        }
        else
        {
            con << tOutput( "$config_command_unknown", name );
        }
    }

    virtual void WriteVal(std::ostream &s)
    {
        tASSERT(0);
    }

    virtual bool Writable(){
        return false;
    }

    virtual bool Save(){
        return false;
    }
};

static tConfItemLevel st_confLevel;

static char const *st_casacl = "CASACL";

//! casacl (Check And Set ACcess Level) command: elevates the access level for the context of the current configuration file
class tCasacl: tConfItemBase
{
public:
    tCasacl()
    : tConfItemBase( st_casacl )
    {
        requiredLevel = tAccessLevel_Program;
    }

    virtual void ReadVal( std::istream & s )
    {
        int required_int = 0, elevated_int = 20;

        // read required and elevated access levels
        s >> required_int;
        s >> elevated_int;

        tAccessLevel elevated = static_cast< tAccessLevel >( elevated_int );
        tAccessLevel required = static_cast< tAccessLevel >( required_int );

        if ( s.fail() )
        {
            con << tOutput( "$sudo_usage" );
            throw tAbortLoading( st_casacl );
        }
        else if ( tCurrentAccessLevel::GetAccessLevel() > required )
        {
            con << tOutput( "$access_level_error",
                            "SUDO",  
                            tCurrentAccessLevel::GetName( required ),
                            tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() )
                );
            throw tAbortLoading( st_casacl );
        }
        else
        {
            tCurrentAccessLevel::currentLevel_ = elevated;
        }
    }

    virtual void WriteVal(std::ostream &s)
    {
        tASSERT(0);
    }

    virtual bool Writable(){
        return false;
    }

    virtual bool Save(){
        return false;
    }
};

static tCasacl st_sudo;

#endif

bool st_FirstUse=true;
static tConfItem<bool> fu("FIRST_USE",st_FirstUse);

tAbortLoading::tAbortLoading( char const * command )
: command_( command )
{
}

tString tAbortLoading::DoGetName() const
{
    return tString(tOutput( "$abort_loading_name"));
}

tString tAbortLoading::DoGetDescription() const
{
    return tString(tOutput( "$abort_loading_description", command_ ));
}

tConfItemBase::tConfItemBase(const char *t, callbackFunc *cb)
        :id(-1),title(t),
changed(false),callback(cb){

    tConfItemMap & confmap = ConfItemMap();
    if ( confmap.find( title ) != confmap.end() )
        tERR_ERROR_INT("Two tConfItems with the same name " << t << "!");

    // compose help name
    tString helpname;
    helpname << title << "_help";
    tToLower( helpname );

    const_cast<tOutput&>(help).AddLocale(helpname);

    confmap[title] = this;

    requiredLevel = tAccessLevel_Admin;
}

tConfItemBase::tConfItemBase(const char *t, const tOutput& h, callbackFunc *cb)
        :id(-1),title(t), help(h),
changed(false),callback(cb){

    tConfItemMap & confmap = ConfItemMap();
    if ( confmap.find( title ) != confmap.end() )
        tERR_ERROR_INT("Two tConfItems with the same name " << t << "!");

    confmap[title] = this;

    requiredLevel = tAccessLevel_Admin;
}

tConfItemBase::~tConfItemBase()
{
    tConfItemMap & confmap = ConfItemMap();
    confmap.erase(title);
    if ( confmap.size() == 0 )
    {
        delete st_confMap;
        st_confMap = 0;
    }
}

void tConfItemBase::SaveAll(std::ostream &s){
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * ci = (*iter).second;
        if (ci->Save()){
            s << std::setw(28) << ci->title << " ";
            ci->WriteVal(s);
            s << '\n';
        }
    }
}

int tConfItemBase::EatWhitespace(std::istream &s){
    int c=' ';

    while(isblank(c) &&
            c!='\n'    &&
            s.good()  &&
            !s.eof())
        c=s.get();

    s.putback(c);

    return c;
}

void tConfItemBase::LoadLine(std::istream &s){
    while(!s.eof() && s.good()){
        tString name;
        s >> name;

        // make name uppercase:
        tToUpper( name );

        bool found=false;

        if (name.Size()==0) // ignore empty lines
            found=true;
        else if (name[0]=='#'){ // comment. ignore rest of line
            char c=' ';
            while(c!='\n' && s.good() && !s.eof()) c=s.get();
            found=true;
        }

        tConfItemMap & confmap = ConfItemMap();
        tConfItemMap::iterator iter = confmap.find( name );
        if ( iter != confmap.end() )
        {
            tConfItemBase * ci = (*iter).second;

            bool cb=ci->changed;
            ci->changed=false;

            if ( ci->requiredLevel >= tCurrentAccessLevel::GetAccessLevel() )
            {
                ci->ReadVal(s);
                if (ci->changed)
                    ci->WasChanged();
                else
                    ci->changed=cb;
            }
            else
            {
                tString discard;
                discard.ReadLine(s);
                
                con << tOutput( "$access_level_error",
                                name,  
                                tCurrentAccessLevel::GetName( ci->requiredLevel ),
                                tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() )
                    );
                return;
            }

            found=true;
        }

        if (!found){
            // eat rest of input line
            tString rest;
            rest.ReadLine( s );
#ifdef MACOSX
            printErrors = false;
#endif
            if (printErrors)
            {
                tOutput o;
                o.SetTemplateParameter(1, name);
                o << "$config_command_unknown";
                con << o;

                if (printChange)
                {
                    int sim_maxlen=-1;

                    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
                    {
                        tConfItemBase * ci = (*iter).second;
                        if (strstr(ci->title,name) &&
                                static_cast<int>(strlen(ci->title)) > sim_maxlen)
                            sim_maxlen=strlen(ci->title);
                    }

                    if (sim_maxlen>0 && printChange ){
                        int len = name.Len()-1;
                        int printMax = 1 + 3 * len * len * len;
                        con << tOutput("$config_command_other");
                        for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
                        {
                            tConfItemBase * ci = (*iter).second;
                            if (strstr(ci->title,name))
                            {
                                tString help ( ci->help );
                                if ( --printMax > 0 )
                                {
                                    tString mess;
                                    mess << ci->title;
                                    mess.SetPos( sim_maxlen+2, false );
                                    mess << "(";
                                    mess << help;
                                    mess << ")\n";
                                    con << mess;
                                }
                            }
                        }
                        if (printMax <= 0 )
                            con << tOutput("$config_command_more");
                    }
                }
                else
                {
                    con << '\n';
                }
            }
        }
    }

    //  std::cout << line << " lines read.\n";
}

static char const * recordingSection = "CONFIG";

/*
bool LoadAllHelper(std::istream * s)
{
    tString line;

    // read line from recording
    if ( s || !tRecorder::Playback( recordingSection, line ) )
    {
        // return on failure
        if (!s)
            return false;

        // read line from stream
        line.ReadLine( *s );

        // write line to recording
        tRecorder::Record( recordingSection, line );
    }
    line << '\n';

    // process line
    std::stringstream str(static_cast< char const * >( line ) );
    tConfItemBase::LoadLine(str);

    return true;
}
*/

// test if a line should enter the recording; check if the line contains one of the passed
// substrings. The array of substrings is supposed to be zero terminated.
static bool s_Veto( tString line_in, std::vector< tString > const & vetos )
{
    // make name uppercase:
    tToUpper( line_in );

    // eat whitespace at the beginning
    char const * test = line_in;
    while( isblank(*test) )
        test++;

    // skip "LAST_"
    tString line( test );
    if ( line.StartsWith( "LAST_" ) )
        line = tString( static_cast< char const * >(line) + 5 );

    // iterate throug vetoed config items and test each one
    for ( std::vector< tString >::const_iterator iter = vetos.begin(); iter != vetos.end(); ++iter )
    {
        tString const & veto = *iter;

        if ( line.StartsWith( veto ) )
        {
#ifdef DEBUG_X
            if ( !line.StartsWith( "INCLUDE" ) && tRecorder::IsRunning() )
            {
                con << "Veto on config line: " << line << "\n";
            }
#endif

            return true;
        }
    }

    return false;
}

// test if a line should be read from a recording
// sound and video mode settings shold not
static std::vector< tString > st_Stringify( char const * vetos[] )
{
    std::vector< tString > ret;

    char const * * v = vetos;
    while ( *v )
    {
        ret.push_back( tString( *v ) );
        ++v;
    }

    return ret;
}

static bool s_VetoPlayback( tString const & line )
{
    static char const * vetos_char[]=
        { "USE_DISPLAYLISTS", "CHECK_ERRORS", "ZDEPTH",
          "COLORDEPTH", "FULLSCREEN ", "ARMAGETRON_LAST_WINDOWSIZE",
          "ARMAGETRON_WINDOWSIZE", "ARMAGETRON_LAST_SCREENMODE",
          "ARMAGETRON_SCREENMODE", "CUSTOM_SCREEN", "SOUND",
          "PASSWORD", "ADMIN_PASS", "RECORDING_DEBUGLEVEL",
          "ZTRICK", "MOUSE_GRAB", "PNG_SCREENSHOT", // "WHITE_SPARKS", "SPARKS",
          "KEEP_WINDOW_ACTIVE", "TEXTURE_MODE", "TEXTURES_HI", "LAG_O_METER", "INFINITY_PLANE",
          "SKY_WOBBLE", "LOWER_SKY", "UPPER_SKY", "DITHER", "HIGH_RIM", "FLOOR_DETAIL",
          "FLOOR_MIRROR", "SHOW_FPS", "TEXT_OUT", "SMOOTH_SHADING", "ALPHA_BLEND",
          "PERSP_CORRECT", "POLY_ANTIALIAS", "LINE_ANTIALIAS", "FAST_FORWARD_MAXSTEP",
          "DEBUG_GNUPLOT", "FLOOR_", "MOVIEPACK_", "RIM_WALL_", "INCLUDE", "SINCLUDE",
          0 };

    static std::vector< tString > vetos = st_Stringify( vetos_char );

    // delegate
    return s_Veto( line, vetos );
}

// test if a line should enter the recording
// passwords should not.
static bool s_VetoRecording( tString const & line )
{
    static char const * vetos_char[]=
        { "#", "PASSWORD", "ADMIN_PASS", "LOCAL_USER", "LOCAL_TEAM",
          0 };

    static std::vector< tString > vetos = st_Stringify( vetos_char );

    // delegate
    return s_Veto( line, vetos );
}

void tConfItemBase::LoadAll(std::istream &s){
    tCurrentAccessLevel levelResetter;

    try{

    while(!s.eof() && s.good())
    {
        tString line;

        // read line from stream
        line.ReadLine( s );

        /// concatenate lines ending in a backslash
        while ( line.Size() > 0 && line[line.Size()-1] == '\\' && s.good() && !s.eof() )
        {
            line = line.SubStr( 0, line.Size()-1 );

            // unless it is a double backslash
            if ( line.Size() > 0 && line[line.Size()-1] == '\\' )
            {
                break;
            }

            tString rest;
            rest.ReadLine( s );
            line << rest;
        }

        if ( line.Len() <= 1 )
            continue;

        // write line to recording
        if ( !s_VetoRecording( line ) )
        {
            // don't record supid admins' instant chat logins
            static tString instantChat("INSTANT_CHAT_STRING");
            if ( line.StartsWith( instantChat ) && strstr( line, "/login" ) )
            {
                tString newLine = line.SubStr( 0, strstr( line, "/login" ) - (char const *)line );
                newLine += "/login NONE";
                if ( line[strlen(line)-1] == '\\' )
                    newLine += '\\';
                tRecorder::Record( recordingSection, newLine );
            }
            else
                tRecorder::Record( recordingSection, line );
        }

        //        std::cout << line << '\n';

        // process line
        // line << '\n';
        if ( !tRecorder::IsPlayingBack() || s_VetoPlayback( line ) )
        {
            std::stringstream str(static_cast< char const * >( line ) );
            tConfItemBase::LoadLine(str);
            // std::cout << line << '\n';
        }
    }
    }
    catch( tAbortLoading const & e )
    {
        // loading was aborted
        con << e.GetDescription() << "\n";
    }
}

void tConfItemBase::DocAll(std::ostream &s){
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * ci = (*iter).second;

        tString help ( ci->help );
        if ( help != "UNDOCUMENTED" )
        {
            tString line;
            line << ci->title;
            line.SetPos( 30, false );
            line << help;
            s << line << '\n';
        }
    }
}

std::deque<tString> tConfItemBase::GetCommands(){
    std::deque<tString> ret;
    tConfItemMap & confmap = ConfItemMap();
    for(tConfItemMap::iterator iter = confmap.begin(); iter != confmap.end() ; ++iter)
    {
        tConfItemBase * ci = (*iter).second;

        ret.push_back(ci->title.ToLower());
    }
    return ret;
}
tConfItemBase *tConfItemBase::FindConfigItem(tString const &name) {
    tConfItemMap & confmap = ConfItemMap();
    tConfItemMap::iterator iter = confmap.find( name.ToUpper() );
    if ( iter != confmap.end() ) {
        return iter->second;
    } else {
        return 0;
    }
}

/*
  void tConfItemBase::ReadVal(std::istream &s);
  void tConfItemBase::WriteVal(std::istream &s);
*/

/*
tString configfile(){
	tString f;
	//#ifndef WIN32
	//  f << static_cast<const char *>(getenv("HOME"));
	//  f << st_LogDir << 
	//  f << "/.ArmageTronrc";
	//#else
		const tPath& vpath = tDirectories::Var();
		for ( int prio = vpath.MaxPriority(); prio>=0; --prio )
		{
			tString path = vpath.Path( prio );
			prio = -1;

	f << st_LogDir << "/user.cfg";
	//#endif
	return f;
}
*/

bool tConfItemBase::LoadPlayback( bool print )
{
    if ( !tRecorder::IsPlayingBack() )
        return false;

    // read line from recording
    tString line;
    while ( tRecorder::Playback( recordingSection, line ) )
    {
        tRecorder::Record( recordingSection, line );
        if ( !s_VetoPlayback( line ) )
        {
            // process line
            if ( print ) con << "Playback input : " << line << '\n';
            std::stringstream str(static_cast< char const * >( line ) );
            tConfItemBase::LoadLine(str);
        }
    }

    return true;
}

static bool Load( const tPath& path, const char* filename )
{
    // read from file
    if ( !filename )
    {
        return false;
    }

    std::ifstream s;
    if ( path.Open( s, filename ) )
    {
        tConfItemBase::LoadAll( s );
        return true;
    }
    else
    {
        return false;
    }
}

// flag indicating whether settings were read from a playback
static bool st_settingsFromRecording = false;

#ifdef DEDICATED
tString extraConfig("NONE");

class tExtraConfigCommandLineAnalyzer: public tCommandLineAnalyzer
{
private:
    virtual bool DoAnalyze( tCommandLineParser & parser )
    {
        return parser.GetOption(extraConfig, "--extraconfig", "-e");
    }

    virtual void DoHelp( std::ostream & s )
    {                                      //
        s << "-e, --extraconfig            : open an extra configuration file after\n"
        << "                               settings_dedicated.cfg\n";
    }
};

static tExtraConfigCommandLineAnalyzer s_extraAnalyzer;
#endif

// configuration files to load. The first one found will be loaded, and
// only the very first one will be written to. Use it to protect stable client's
// config from the experimental client's wrath.
char const * st_userConfigs[] = { "user_3_0.cfg", "user.cfg", 0 };

static void st_InstallSigHupHandler();

void st_LoadConfig( bool printChange )
{
    st_InstallSigHupHandler();

    const tPath& var = tDirectories::Var();
    const tPath& config = tDirectories::Config();
    const tPath& data = tDirectories::Data();

    tConfItemBase::printChange=printChange;
#ifdef DEDICATED
    tConfItemBase::printErrors=false;
#endif
    {
        // load the first available user configuration file
        char const * const * userConfig = st_userConfigs;
        while ( *userConfig && !Load( var, *userConfig ) )
            userConfig++;
    }
    tConfItemBase::printErrors=true;

    Load( config, "settings.cfg" );
#ifdef DEDICATED
    Load( config, "settings_dedicated.cfg" );
    if( extraConfig != "NONE" ) Load( config, extraConfig );
#else
    if (st_FirstUse)
    {
        Load( config, "default.cfg" );
    }
#endif

    Load( data, "moviepack/settings.cfg" );

    Load( config, "autoexec.cfg" );
    Load( var, "autoexec.cfg" );

    // load configuration from playback
    tConfItemBase::LoadPlayback();
    st_settingsFromRecording = tRecorder::IsPlayingBack();

    tConfItemBase::printChange=true;
}

void st_SaveConfig()
{
    // don't save while playing back
    if ( st_settingsFromRecording )
    {
        return;
    }

    std::ofstream s;
    if ( tDirectories::Var().Open( s, st_userConfigs[0], std::ios::out, true ) )
    {
        tConfItemBase::SaveAll(s);
    }
    else
    {
        tOutput o("$config_file_write_error");
        con << o;
        std::cerr << o;
    }
}

void st_LoadConfig()
{
    st_LoadConfig( false );
}

static void st_DoHandleSigHup()
{
    con << tOutput("$config_sighup");
    st_SaveConfig();
    st_LoadConfig();
}

static void st_HandleSigHup( int signal )
{
    st_ToDo_Signal( st_DoHandleSigHup );
}

static void st_InstallSigHupHandler()
{
#ifndef WIN32
    static bool installed = false;
    if ( !installed )
    {
        signal( SIGHUP, &st_HandleSigHup );
        installed = true;
    }
#endif
}

void tConfItemLine::ReadVal(std::istream &s){
    tString dummy;
    dummy.ReadLine(s, true);
    if(strcmp(dummy,*target)){
        if (printChange)
        {
            tOutput o;
            o.SetTemplateParameter(1, title);
            o.SetTemplateParameter(2, *target);
            o.SetTemplateParameter(3, dummy);
            o << "$config_value_changed";
            con << o;
        }
        *target=dummy;
        changed=true;

        ExecuteCallback();
    }

    *target=dummy;
}


void tConfItemLine::WriteVal(std::ostream &s){
    tConfItem<tString>::WriteVal(s);

    // double trailing backslash so it is read back as a single backslash, not
    // a continued line (HACK: this would actually be the job of the calling function)
    if ( target->Len() >= 2 &&
            target->operator()(target->Len() - 2) == '\\' )
        s << "\\";
}

tConfItemFunc::tConfItemFunc
(const char *title, CONF_FUNC *func)
    :tConfItemBase(title),f(func){}

tConfItemFunc::~tConfItemFunc(){}

void tConfItemFunc::ReadVal(std::istream &s){(*f)(s);}
void tConfItemFunc::WriteVal(std::ostream &){}

bool tConfItemFunc::Save(){return false;}

void st_Include( tString const & file, bool reportError )
{
    // refuse to load illegal paths
    if( !tPath::IsValidPath( file ) )
        return;

    if ( !Load( tDirectories::Var(), file ) )
    {
        if (!Load( tDirectories::Config(), file ) && reportError )
        {
            con << tOutput( "$config_include_not_found", file );
        }
    }
}

static void Include(std::istream& s, bool error )
{
    tString file;
    s >> file;

    st_Include( file, error );
}

static void Include(std::istream& s )
{
    Include( s, true );
}

static void SInclude(std::istream& s )
{
    Include( s, false );
}

static tConfItemFunc s_Include("INCLUDE",  &Include);
static tConfItemFunc s_SInclude("SINCLUDE",  &SInclude);

// obsoleted settings that still are around in some distruted configuration files
static void st_Dummy(std::istream &s){tString rest; rest.ReadLine(s);}
static tConfItemFunc st_DummyMpHack("MOVIEPACK_HACK",&st_Dummy);

#ifdef DEDICATED
// settings missing in the dedicated server
static tConfItemFunc st_Dummy1("ARENA_WALL_SHADOW_NEAR", &st_Dummy);
static tConfItemFunc st_Dummy2("ARENA_WALL_SHADOW_DIST", &st_Dummy);
static tConfItemFunc st_Dummy3("ARENA_WALL_SHADOW_SIDEDIST", &st_Dummy);
static tConfItemFunc st_Dummy4("ARENA_WALL_SHADOW_SIZE", &st_Dummy);
static tConfItemFunc st_Dummy5("BUG_TRANSPARENCY_DEMAND", &st_Dummy);
static tConfItemFunc st_Dummy6("BUG_TRANSPARENCY", &st_Dummy);
static tConfItemFunc st_Dummy7("SHOW_OWN_NAME", &st_Dummy);
static tConfItemFunc st_Dummy8("FADEOUT_NAME_DELAY", &st_Dummy);
static tConfItemFunc st_Dummy9("FLOOR_MIRROR_INT", &st_Dummy);
#endif
#ifndef DEBUG
// settings missing in optimized mode
static tConfItemFunc st_Dummy10("SIMULATE_RECEIVE_PACKET_LOSS", &st_Dummy);
static tConfItemFunc st_Dummy11("SIMULATE_SEND_PACKET_LOSS", &st_Dummy);
#endif

