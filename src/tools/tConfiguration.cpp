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
#include "utf8.h"

#ifndef DEDICATED
#include <SDL_version.h>
#endif

#include <vector>
#include <string.h>

#ifndef WIN32
#include <signal.h>
#endif

bool           tConfItemBase::printChange=true;
bool           tConfItemBase::printErrors=true;

// flag indicating the currently loaded config is in latin1 encoding (default: utf8)
static bool st_loadAsLatin1 = false;

static void st_ToggleConfigItem( std::istream & s )
{
    tString name;
    s >> name;

    if ( name.Size() == 0 )
    {
        con << tOutput( "$toggle_usage_error" );
        return;
    }

    tConfItemBase *base = tConfItemBase::FindConfigItem( name );

    if ( !base )
    {
        con << tOutput( "$config_command_unknown", name );
        return;
    }

    tConfItem< bool > *confItem = dynamic_cast< tConfItem< bool > * >( base );
    if ( confItem && confItem->Writable() )
    {
        confItem->SetVal( !*confItem->GetTarget() );
    }
    else
    {
        con << tOutput( "$toggle_invalid_config_item", name );
    }
}

static tConfItemFunc toggleConfItemFunc( "TOGGLE", st_ToggleConfigItem );

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
    tASSERT( currentLevel_ != tAccessLevel_Invalid );
    return currentLevel_;
}

// returns the name of an access level
tString tCurrentAccessLevel::GetName( tAccessLevel level )
{
    std::ostringstream s;
    s << "$config_accesslevel_" << level;
    return tString( tOutput( s.str().c_str() ) );
}

tAccessLevel tCurrentAccessLevel::currentLevel_ = tAccessLevel_Invalid; //!< the current access level

tAccessLevelSetter::tAccessLevelSetter( tConfItemBase & item, tAccessLevel level )
{
#ifdef KRAWALL_SERVER
    item.requiredLevel = level;
#endif
}

static std::map< tString, tConfItemBase * > * st_confMap = 0;
tConfItemBase::tConfItemMap & tConfItemBase::ConfItemMap()
{
    if (!st_confMap)
        st_confMap = tNEW( tConfItemMap );
    return *st_confMap;
}

tConfItemBase::tConfItemMap const & tConfItemBase::GetConfItemMap()
{
    return ConfItemMap();
}

static bool st_preventCasacl = false;

tCasaclPreventer::tCasaclPreventer( bool prevent )
{
    previous_ = st_preventCasacl;
    st_preventCasacl = prevent;
}

tCasaclPreventer::~tCasaclPreventer()
{
    st_preventCasacl = previous_;
}

//! returns whether we're currently in an RINCLUDE file
bool tCasaclPreventer::InRInclude()
{
    return st_preventCasacl;
}

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
            if(printErrors)
            {
                con << tOutput( "$access_level_usage" );
            }
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

            if( ci->requiredLevel < tCurrentAccessLevel::GetAccessLevel() )
            {
                con << tOutput( "$access_level_nochange_now",
                                name,
                                tCurrentAccessLevel::GetName( ci->requiredLevel ),
                                tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() ) );
            }
            else if( level < tCurrentAccessLevel::GetAccessLevel() )
            {
                con << tOutput( "$access_level_nochange_later",
                                name,
                                tCurrentAccessLevel::GetName( level ),
                                tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() ) );
            }
            else if ( ci->requiredLevel != level )
            {
                ci->requiredLevel = level;
                if(printChange)
                {
                    con << tOutput( "$access_level_change", name, tCurrentAccessLevel::GetName( level ) );
                }
            }
        }
        else if(printErrors)
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

    // CAN this be saved at all?
    virtual bool CanSave(){
        return false;
    }
};

static tConfItemLevel st_confLevel;

#ifdef KRAWALL_SERVER

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
            con << tOutput( "$casacl_usage" );
            throw tAbortLoading( st_casacl );
        }
        else if ( tCurrentAccessLevel::GetAccessLevel() > required )
        {
            con << tOutput( "$access_level_error",
                            "CASACL",
                            tCurrentAccessLevel::GetName( required ),
                            tCurrentAccessLevel::GetName( tCurrentAccessLevel::GetAccessLevel() )
                );
            throw tAbortLoading( st_casacl );
        }
        else if ( st_preventCasacl )
        {
            con << tOutput( "$casacl_not_allowed" );
            throw tAbortLoading( st_casacl );
        }
        else
        {
            tString().ReadLine(s); // prevent commands following this one without a newline
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

    // CAN this be saved at all?
    virtual bool CanSave(){
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
    setLevel      = tAccessLevel_Owner;
}

tConfItemBase::tConfItemBase(const char *t, const tOutput& h, callbackFunc *cb)
        :id(-1),title(t), help(h),
changed(false),callback(cb){

    tConfItemMap & confmap = ConfItemMap();
    if ( confmap.find( title ) != confmap.end() )
        tERR_ERROR_INT("Two tConfItems with the same name " << t << "!");

    confmap[title] = this;

    requiredLevel = tAccessLevel_Admin;
    setLevel      = tAccessLevel_Owner;
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

    if( s.good() )
    {
        s.putback(c);
    }

    return c;
}

void tConfItemBase::LoadLine(std::istream &s){
    if(!s.eof() && s.good()){
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
                ci->setLevel = tCurrentAccessLevel::GetAccessLevel();

                ci->ReadVal(s);
                if (ci->changed)
                {
                    ci->WasChanged();
                }
                else
                {
                    ci->changed=cb;
                }
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
    static char const* vetos_char[] =
        { "USE_DISPLAYLISTS", "CHECK_ERRORS", "ZDEPTH", "SWAP_OPTIMIZE",
          "COLORDEPTH", "FULLSCREEN ", "LOW_DPI_WINDOW", "ARMAGETRON_LAST_WINDOWSIZE",
          "ARMAGETRON_WINDOWSIZE", "ARMAGETRON_LAST_SCREENMODE",
          "ARMAGETRON_VSYNC", "ARMAGETRON_VSYNC_LAST",
          "ARMAGETRON_SCREENMODE", "CUSTOM_SCREEN", "SOUND",
          "PASSWORD", "ADMIN_PASS",
          "ZTRICK", "MOUSE_GRAB", "PNG_SCREENSHOT", // "WHITE_SPARKS", "SPARKS",
          "KEEP_WINDOW_ACTIVE", "TEXTURE_MODE", "TEXTURES_HI", "LAG_O_METER", "INFINITY_PLANE",
          "SKY_WOBBLE", "LOWER_SKY", "UPPER_SKY", "DITHER", "HIGH_RIM", "FLOOR_DETAIL",
          "FLOOR_MIRROR", "SHOW_FPS", "TEXT_OUT", "SMOOTH_SHADING", "ALPHA_BLEND",
          "PERSP_CORRECT", "POLY_ANTIALIAS", "LINE_ANTIALIAS", "FAST_FORWARD_MAXSTEP",
          "DEBUG_GNUPLOT", "FLOOR_", "MOVIEPACK_", "RIM_WALL_",
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
    return s_Veto( line, vetos ) || s_VetoPlayback( line );
}


//! @param s        stream to read from
//! @param record   set to true if the configuration is to be recorded and played back. That's usually only required if s is a file stream.
void tConfItemBase::LoadAll(std::istream &s, bool record ){
    tCurrentAccessLevel levelResetter;

    try{

    while(!s.eof() && s.good())
    {
        tString line;

        // read line from stream
        int indent = 0;
        line.ReadLine( s, false, -1, &indent );

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
            rest.ReadLine( s, false, indent );
            line << rest;
        }

        // convert to utf8
        if ( st_loadAsLatin1 )
        {
            line = st_Latin1ToUTF8( line );
        }

        // and check. No invalid utf8 string must enter the system.
        {
            tString::iterator reader = line.begin();
            try
            {
                while( reader != line.end() )
                {
                    // convert from utf8
                    utf8::next( reader, line.end() );
                }
            }
            catch(...)
            {
                con << "Illegal utf8 found in config file, ignoring most of it.\n";

                // save what there is to save
                line.NetFilter();
            }
        }

        if ( line.Len() <= 1 )
            continue;

        // write line to recording
        if ( record && !s_VetoRecording( line ) )
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
        if ( !record || !tRecorder::IsPlayingBack() || s_VetoPlayback( line ) )
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

//! @param s        stream to read from
//! @param record   set to true if the configuration is to be recorded and played back. That's usually only required if s is a file stream, so it defaults to true here.
void tConfItemBase::LoadAll(std::ifstream &s, bool record )
{
    std::istream &ss(s);
    LoadAll( ss, record );
}


//! @param s        file stream to be used for reading later
//! @param filename name of the file to open
//! @param path     whether to look in var directory
//! @return success flag
bool tConfItemBase::OpenFile( std::ifstream & s, tString const & filename )
{
    bool ret = tDirectories::Config().Open(s, filename );

    static char const * section = "INCLUDE_VOTE";
    tRecorder::Playback( section, ret );
    tRecorder::Record( section, ret );

    return ret;
}

//! @param s        file to read from
void tConfItemBase::ReadFile( std::ifstream & s )
{
    if ( !tRecorder::IsPlayingBack() )
    {
        tConfItemBase::LoadAll(s, true );
    }
    else
    {
        tConfItemBase::LoadPlayback();
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
        tConfItemBase::LoadAll( s, true );
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
    bool DoAnalyze( tCommandLineParser & parser, int pass ) override
    {
        if(pass > 0)
            return false;

        return parser.GetOption(extraConfig, "--extraconfig", "-e");
    }

    void DoHelp( std::ostream & s ) override
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
char const * st_userConfigs[] = { "user_3_1_utf8.cfg", 0 };
char const * st_userConfigsLatin1[] = { "user_3_1.cfg", "user_3_0.cfg", "user.cfg", 0 };

static void st_InstallSigHupHandler();

static bool st_LoadUserConfigs( char const * const * userConfig, bool latin1, tPath const & path )
{
    st_loadAsLatin1 = latin1;

    // load the first available user configuration file
    while ( *userConfig )
    {
        if ( Load( path, *userConfig ) )
        {
            st_loadAsLatin1 = false;
            return true;
        }
        userConfig++;
    }

    st_loadAsLatin1 = false;
    return false;
}

static bool st_LoadUserConfigs( char const * const * userConfig, bool latin1 )
{
    return
    st_LoadUserConfigs( userConfig, latin1, tDirectories::Config() ) ||
    st_LoadUserConfigs( userConfig, latin1, tDirectories::Var() );
}

void st_LoadConfig( bool printChange )
{
    // default include files are executed at owner level
    tCurrentAccessLevel level( tAccessLevel_Owner, true );

    st_InstallSigHupHandler();

    const tPath& config = tDirectories::Config();
    const tPath& data = tDirectories::Data();

    tConfItemBase::printChange=printChange;
#ifdef DEDICATED
    tConfItemBase::printErrors=false;
#endif
    st_LoadUserConfigs( st_userConfigs, false ) || st_LoadUserConfigs( st_userConfigsLatin1, true );

    tConfItemBase::printErrors=true;

    Load( config, "settings.cfg" );
#ifdef DEDICATED
    Load( config, "settings_dedicated.cfg" );
    if( extraConfig != "NONE" ) Load( config, extraConfig );
#else
    if (st_FirstUse)
    {
        Load( config, "default.cfg" );
#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2,0,0)
        Load( config, "sdl2/default.cfg" );
#else
        Load( config, "sdl1/default.cfg" );
#endif
#endif
    }
#endif

    Load( data, "textures/settings_textures.cfg" );
    Load( data, "moviepack/settings.cfg" );

    Load( config, "autoexec.cfg" );

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
    if ( tDirectories::Config().Open( s, st_userConfigs[0], std::ios::out, true ) )
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
            tColoredString oldval;
            oldval << *target << tColoredString::ColorString(1,1,1);
            tColoredString newval;
            newval << dummy << tColoredString::ColorString(1,1,1);
            tOutput o;
            o.SetTemplateParameter(1, title);
            o.SetTemplateParameter(2, oldval);
            o.SetTemplateParameter(3, newval);
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
    // slow, but correct: escape special characters on the fly
    int len = target->Len();
    for(int i = 0; i < len; ++i)
    {
        char c = (*target)[i];
        switch(c)
        {
            case 0:
                continue;
            case '\\':
            case '\'':
            case '"':
            case ' ':
                s << '\\';
        }
        s << c;
    }
}

tConfItemFunc::tConfItemFunc
(const char *title, CONF_FUNC *func)
    :tConfItemBase(title),f(func){}

tConfItemFunc::~tConfItemFunc(){}

void tConfItemFunc::ReadVal(std::istream &s){(*f)(s);}
void tConfItemFunc::WriteVal(std::ostream &){}

bool tConfItemFunc::Save(){return false;}

void st_Include( tString const & file )
{
    // refuse to load illegal paths
    if( !tPath::IsValidPath( file ) )
        return;

    if ( !tRecorder::IsPlayingBack() )
    {
        // really load include file
        if (!Load( tDirectories::Config(), file ) && tConfItemBase::printErrors )
        {
            con << tOutput( "$config_include_not_found", file );
        }
    }
    else
    {
        // just read configuration, and don't forget to reset the config level
        tCurrentAccessLevel levelResetter;
        tConfItemBase::LoadPlayback();
    }

    // mark section
    char const * section = "INCLUDE_END";
    tRecorder::Record( section );
    tRecorder::PlaybackStrict( section );

}

static void Include(std::istream& s )
{
    // allow CASACL
    tCasaclPreventer allower(false);

    tString file;
    s >> file;

    st_Include( file );
}

static void SInclude(std::istream& s )
{
    tNoisinessSetter setter( false, false );
    Include( s );
}

static tConfItemFunc s_Include("INCLUDE",  &Include);
static tConfItemFunc s_SInclude("SINCLUDE",  &SInclude);

tNoisinessSetter::tNoisinessSetter( bool printChange, bool printErrors )
{
    oldPrintChange = tConfItemBase::printChange;
    oldPrintErrors = tConfItemBase::printErrors;

    tConfItemBase::printChange = printChange;
    tConfItemBase::printErrors = printErrors;
}

tNoisinessSetter::~tNoisinessSetter()
{
    tConfItemBase::printChange = oldPrintChange;
    tConfItemBase::printErrors = oldPrintErrors;
}

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

namespace
{
std::vector<tConfigMigration::Callback> &st_MigrationCallbacks()
{
    static std::vector<tConfigMigration::Callback> callbacks;
    return callbacks;
}

#ifdef DEBUG
struct tConfigMigrationTester
{
    tConfigMigrationTester()
    {
        tASSERT(!tConfigMigration::SavedBefore("0.2.9","0.2.9"));

        auto AssertOrdered = [](char const *a, char const *b)
        {
            tASSERT(tConfigMigration::SavedBefore(a,b));
            tASSERT(!tConfigMigration::SavedBefore(b,a));
        };

        AssertOrdered("0.2.8","0.2.9");
        AssertOrdered("0.2.8","0.10.9");
        AssertOrdered("0.4.0_alpha100","0.4.0_beta1");
        AssertOrdered("0.4.0_beta229","0.4.0_rc1");
        AssertOrdered("0.4.0_rc19","0.4.0");
        AssertOrdered("0.","0.0");
        AssertOrdered("0.","0.1");
        AssertOrdered("0.","0.9");
        AssertOrdered("0","0.");
        AssertOrdered("0_","0");
    }
};

static tConfigMigrationTester st_ConfigMigrationTester;
#endif
}

tConfigMigration::tConfigMigration(tConfigMigration::Callback &&cb)
{
    st_MigrationCallbacks().push_back(std::move(cb));
}

void tConfigMigration::Migrate(const tString &savedInVersion)
{
    for(auto &callback: st_MigrationCallbacks())
    {
        callback(savedInVersion);
    }
}

bool tConfigMigration::SavedBefore(char const *savedInVersion, char const *beforeVersion)
{
    char const *pA = savedInVersion, *pB = beforeVersion;
    while(*pA && *pB)
    {
        // equality -> advance
        if(*pA == *pB)
        {
            ++pA;
            ++pB;
            continue;
        }

        // non-number difference -> regular alphabetic order
        if(!isdigit(*pA) || !isdigit(*pB))
        {
            return *pA < *pB;
        }

        // this leaves number differences.
        auto lengthOfNumber = [](char const *pBegin)
        {
            auto *pC = pBegin;
            while(isdigit(*pC))
                ++pC;
            return pC - pBegin;
        };

        // longer numbers are bigger
        auto const lA = lengthOfNumber(pA);
        auto const lB = lengthOfNumber(pB);
        if(lA < lB)
            return true;
        if(lA > lB)
            return false;

        // equal length numbers can be compared digit by digit
        while(isdigit(*pA))
        {
            if(*pA == *pB)
            {
                ++pA;
                ++pB;
                continue;
            }

            return *pA < *pB;
        }
    }
    if(!*pA && !*pB)
        return false;

    // reversed order if one version string is longer than the other;
    // we want _ to be less than nothing, but . more, and they sit on
    // opposite sides of the digits.
    if(!*pA)
    {
        return '9' >= *pB;
    }
    else
    {
        tASSERT(!*pB);
        return '9' < *pA;
    }
}
