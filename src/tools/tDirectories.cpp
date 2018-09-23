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

#include "aa_config.h"

#include <errno.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "tLocale.h"
#include "tDirectories.h"
#include "tString.h"
#include "tConfiguration.h"
#include "tCommandLine.h"
#include "tMemManager.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

// include definition for top source directory
#ifdef TOP_SOURCE_DIR
static const char * s_topSourceDir = TOP_SOURCE_DIR;
#else
static const char * s_topSourceDir = ".";
#endif

// program name definition
#ifndef PROGNAME
#ifdef DEDICATED
#define PROGNAME "armagetronad-dedicated"
#else
#define PROGNAME "armagetronad"
#endif
#endif

#ifndef PROGNAMEBASE
#define PROGNAMEBASE "armagetronad"
#endif

#ifndef PROGDIR_SUFFIX
#define PROGDIR_SUFFIX ""
#endif

#ifdef WIN32
// activate used function from shlobj.h (z-man does not know if this is a bad hack)
#ifdef __MINGW32__
#define _WIN32_IE 0x400
#endif

#include <direct.h>
#include <windows.h>
#include <shlobj.h>
#define mkdir(x, y)   _mkdir(x)
#ifndef strdup
#define strdup        _strdup
#endif
#ifndef _stat
#define _stat stat
#endif
#else
#include <pwd.h>
#endif

#ifdef TOP_SOURCE_DIR
// #include "tPaths.h"
#include "tUniversalVariables.h"
#endif

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif

#ifndef BINDIR
#define BINDIR "/usr/local/bin"
#endif

// move variables out of binrelocs macros way
static tString st_prefixCompiled(PREFIX);
static tString st_bindirCompiled(BINDIR);

// Paths from root to binaries, data and config ( so we can do search&replace to get
// from the binary path to the data and config )
#ifndef SYSBINDIR
#define SYSBINDIR "/does/not/exist"
#endif

#ifndef DATASUFFIX
#define DATASUFFIX ""
#endif

#ifndef CONFIGSUFFIX
#define CONFIGSUFFIX ""
#endif

#define PROGDIR PROGNAME PROGDIR_SUFFIX

#ifdef WIN32
// activate used function from shlobj.h (z-man does not know if this is a bad hack)
#ifdef __MINGW32__
#define _WIN32_IE 0x400
#endif

#undef DATADIR
#include <direct.h>
#include <windows.h>
#include <shlobj.h>
#define mkdir(x, y)   _mkdir(x)
#ifndef strdup
#define strdup        _strdup
#endif
#ifndef _stat
#define _stat stat
#endif
#else
#include <pwd.h>
#endif

#ifdef DEBUG
// define this if you want file search debug output
//#define PRINTSEARCH
//#define DEBUG_PATH
#endif

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef ENABLE_BINRELOC
#include "thirdparty/binreloc/prefix.h"
#endif

tString expand_home(const tString & pathname);

#define expand_home_c(c) expand_home(tString(c))

#ifndef DATA_DIR
#define DATA_DIR "."
#endif
static tString st_DataDir(expand_home_c(DATA_DIR));    // directory for game data

#ifndef MUSIC_DIR
#define MUSIC_DIR "."
#endif
static tString st_MusicDir(expand_home_c(DATA_DIR));    // directory for game music

// use XDG directories?
#ifndef WIN32
#ifndef MACOSX
#define USE_XDG
#endif
#endif

#ifndef USER_DATA_DIR
#ifdef USE_XDG
#define USER_DATA_DIR "${XDG_DATA_HOME}/" PROGDIR
#else
#define USER_DATA_DIR "~/." PROGDIR
#endif
#endif
static tString st_UserDataDir(expand_home_c(USER_DATA_DIR));    // directory for game data

// load data from unbranded configuration directory on branded builds in Linux
#if !defined DEDICATED && !defined MACOSX && !defined LEGACY_USER_DATA_DIR && !defined DEBUG
#define LEGACY_USER_DATA_DIR "~/.armagetronad"
#define LEGACY_USER_DATA_DIR2 "~/." PROGDIR
#endif

#ifdef LEGACY_USER_DATA_DIR
static tString st_LegacyUserDataDir(expand_home_c(LEGACY_USER_DATA_DIR));    // directory for game data (old location)
#endif

#ifdef LEGACY_USER_DATA_DIR2
static tString st_LegacyUserDataDir2(expand_home_c(LEGACY_USER_DATA_DIR2));    // directory for game data
#endif

#ifndef CONFIG_DIR
#define CONFIG_DIR ""
#endif
static tString st_ConfigDir(expand_home_c(CONFIG_DIR));  // directory for static configuration files

#ifndef SCRIPT_DIR
#define SCRIPT_DIR ""
#endif
static tString st_ScriptDir(expand_home_c(SCRIPT_DIR));  // directory for scripts

#ifndef USER_CONFIG_DIR
#ifdef USE_XDG
#define USER_CONFIG_DIR "${XDG_CONFIG_HOME}/" PROGDIR
#else
#define USER_CONFIG_DIR ""
#endif
#endif
static tString st_UserConfigDir(expand_home_c(USER_CONFIG_DIR));  // directory for static configuration files

#ifndef VAR_DIR
#define VAR_DIR ""
#endif
static tString st_VarDir(expand_home_c(VAR_DIR));     // directory for dynamic logs and highscores

#ifdef DEDICATED
#ifndef WWWROOTDIR
#define WWWROOTDIR ""
#endif
static tString st_WwwDir(expand_home_c(WWWROOTDIR));     // directory for dynamic logs and highscores
#endif

#ifndef RESOURCE_DIR
#define RESOURCE_DIR ""
#endif
static tString st_ResourceDir(expand_home_c(RESOURCE_DIR));

#ifndef AUTORESOURCE_DIR
#define AUTORESOURCE_DIR ""
#endif
static tString st_AutoResourceDir(expand_home_c(AUTORESOURCE_DIR));

#ifndef INCLUDEDRESOURCE_DIR
#define INCLUDEDRESOURCE_DIR ""
#endif
static tString st_IncludedResourceDir(expand_home_c(INCLUDEDRESOURCE_DIR));

#ifndef SCREENSHOT_DIR
#define SCREENSHOT_DIR ""
#endif
static tString st_ScreenshotDir(expand_home_c(SCREENSHOT_DIR));

// checks whether a character is a path delimiter
static bool st_IsPathDelimiter( char c )
{
    return  ( c == '/' || c == '\\' );
}

/*
Recursively creates directories.
Parameters:
pathname: The path to create. Must have aat least one / and last directory must be suffix'd with a /
safe_path: The number of characters to assume safe for usage. Checking for .* is disabled for these.
        Returns true if successful. Returns false if unsuccessful or in hidden-file circumstances!
                */
static bool mkdir_recurse(char *pathname, size_t safe_path) {
    // method: strip path segments from the end, try to make the directory up to there.
    // once this succeeded, rebuild the path, making all the subdirectories as you go.

    size_t i;
    bool fs = false; // forward search flag. if set, start rebuilding the path
    bool e  = false; // error flag.

    size_t len = strlen(pathname); // the total length of the path

    for (i = len - 1; i < len && !e; fs ? ++i : --i) {
        if ( st_IsPathDelimiter( pathname[i] ) ) {
            if (pathname[i + 1] == '.' && i + 2 > safe_path)
                return false; // abort; we have a hidden file/dir!
            char delimiter = pathname[i];
            pathname[i] = '\0';

#ifdef DEBUG_PATH
            static bool first = true;
            if (fs || first )
            {
                first = false;
                con << "Making directory " << pathname << "\n";
            }
#endif

            if (!mkdir(pathname, 0777) || errno == EEXIST)
                fs = true;
            else if (fs)
                e = true;
            pathname[i] = delimiter;
        }
        if (i == 0 && !fs)
            e = true;
    }
    return !e;
}

static bool st_checkPathAbsolute = true; // check for absolute paths
static bool st_checkPathRelative = true; // check for relative paths
static bool st_checkPathHidden   = true; // check for hidden files/directories in paths

// *******************************************************************************************
// *
// *   IsValidPath
// *
// *******************************************************************************************
//!
//! checks whether a user given path, to be prepended with one of the AA directories, is
//! a valid path (does not use dirty filename tricks to access portions of the file system
//! it isn't supposed to access). It is called by all path finding functions of tPath
//! automatically, you should only call it from outside of tDirectories.cpp if you want
//! to avoid redundant error messages if you try several path resolutions.
//!
//! @todo make this throw exceptions instead, right now nobody is there to catch them
//!
//!        @param  filename the user given filename to check
//!        @return          true if no security issues were found
//!
// *******************************************************************************************

bool tPath::IsValidPath( char const * filename )
{
    // always check for empty paths
    if ( !filename || filename[0] == 0 )
    {
        con << tOutput( "$directory_path_null" );
        return false;
    }

    // check for absolute paths (the system would not make them absolute, so no real
    // danger comes from them, but we check anyway)
    if ( st_checkPathAbsolute && ( st_IsPathDelimiter( filename[0] ) || strstr(filename,":") ) )
    {
        con << tOutput( "$directory_path_absolute", filename );
        return false;
    }

    // check for relative paths. Those are the real danger. Search for .. and see
    // if it surrounded by path delimiters or string ends.
    if ( st_checkPathRelative )
    {
        // traverse through occurences of ..
        char const * run = filename;
        while ( run && *run )
        {
            // find next ..
            run = strstr(run,"..");

            // check if before and after that come no path delimiters
            if ( run )
            {
                if ( ( run == filename || st_IsPathDelimiter( run[-1] ) ) &&
                        ( run[2] == 0 || st_IsPathDelimiter( run[2] ) ) )
                {
                    con << tOutput( "$directory_path_relative", filename );
                    return false;
                }

                // go on searching
                run ++;
            }
        }
    }

    // hidden paths are a path delimiter followed by a ., but not ..<delimiter>.
    if ( st_checkPathHidden )
    {
        // iterate path segments
        char const * run = filename;
        while ( run && *run )
        {
            // check if it is a hidden file
            if ( run[0] == '.' )
            {
                // don't give false alarm for relative paths
                if ( !( st_IsPathDelimiter( run[1] ) || ( run[1] == '.' && st_IsPathDelimiter( run[2] ) ) ) )
                {
                    con << tOutput( "$directory_path_hidden", filename );
                    return false;
                }
            }

            // proceed to next path segments: find next path delimiter
            while ( *run && !st_IsPathDelimiter( *run ) )
                ++run;

            // go to next character after that
            if ( *run )
                ++run;
        }
    }

    return true;
}

/*
Parses "~", "~username", "${HOME}", "${HOME:username}", etc
*/
char *eh_getdir(const char *da, size_t *len) {
    const char *s, *r, *d;
    char *type = 0, *user = 0, *ret = 0;
    size_t l;

    // Step 1: Extract type, user, and *len
    if (da[0] == '~') {
        type = strdup("HOME");
        {
            // find first occurence of slash or backslash
            char const * slash = strchr(da, '/');
            char const * backslash = strchr(da, '\\');
            if ( slash && backslash )
                d = slash < backslash ? slash : backslash;
            else
                d = slash ? slash : backslash;
        }
        l = ((d ? d : strchr(da, '\0')) - da - 1) * sizeof(char);
        user = (char *)memcpy(malloc(l + sizeof(char)), da + 1, l); user[l] = '\0';
        *len = l + 1;
    } else if (!strncmp(da, "${", 2)) {
        s = strchr(da, '}');
        if (!s) {
            // Invalid structure
            return 0;
        }
        if ((r = strchr(da, ':'))) {
            l = (s - r - 1) * sizeof(char);
            user = (char *)memcpy(malloc(l + sizeof(char)), r + 1, l); user[l] = '\0';
        } else {
            r = s;
            user = (char *)malloc(sizeof(char));
            user[0] = '\0';
        }
        l = (r - da - 2) * sizeof(char);
        type = (char *)memcpy(malloc(l + sizeof(char)), da + 2, l); type[l] = '\0';
        *len = s - da + 1;
    }

    // Step 2: Resolve type/user into ret
#ifdef WIN32
    if (!strcmp(type, "HOME")) {
        free(type);
        type = strdup("HOMEPATH");
    }
#endif
    if ((ret = getenv(type)))
        ret = strdup(ret);
    else {
# ifdef WIN32
        {
            char path[MAX_PATH];
            int ssf = 0;

            // Bug! This assumes user == current user! (who cares, but... yeah)
            if (!strcmp(type, "HOME"))   ssf = 0x28;
            if (!strcmp(type, "APPDATA"))  ssf = 0x1a;
            if (!strcmp(type, "COMMONAPPDATA")) ssf = 0x23;
            if (!strcmp(type, "DESKTOP"))  ssf = 0x10;
            if (!strcmp(type, "LOCALAPPDATA")) ssf = 0x1c;
            if (!strcmp(type, "MYPICTURES")) ssf = 0x27;
            if (!strcmp(type, "PERSONAL"))  ssf = 0x05;
            if (!strcmp(type, "PROFILE"))  ssf = 0x28;
            if (!strcmp(type, "SYSTEM"))  ssf = 0x25;
            if (!strcmp(type, "WINDOWS"))  ssf = 0x24;

            //if (ssf && SUCCEEDED(SHGetFolderPath(NULL, ssf, NULL, 0, path)))
            if (ssf && SHGetSpecialFolderPath(NULL, path, ssf, 1))
                ret = strdup(path);
        }
# else

        if (type)
        {
            struct passwd *pw;

            if (user[0] == '\0') // Current user
                pw = getpwuid(getuid());
            else
                pw = getpwnam(user);

            if(pw)
            {
                if (!strcmp(type, "XDG_CONFIG_HOME"))
                {
                    char const * xdg_config = getenv("XDG_CONFIG_HOME");
                    tString config(xdg_config ? xdg_config : tString(pw->pw_dir) + "/.config");
                    ret = strdup( config );
                }
                else if (!strcmp(type, "XDG_DATA_HOME"))
                {
                    char const * xdg_data = getenv("XDG_DATA_HOME");
                    tString data(xdg_data ? xdg_data : tString(pw->pw_dir) + "/.local/share");
                    ret = strdup( data );
                }

                // fall back to default for HOME
                if (!strcmp(type, "HOME"))
                {
                    ret = strdup(pw->pw_dir);
                }
            }
        }
# endif
        // TODO: fall back to hardcoded stuff in some cases?
    }

#ifdef DEBUG_PATH
    std::cout << "Changing " << type << " to " << ret << "\n";
#endif

    // Step 3: Cleanup
    free(type);
    free(user);
    if (!ret && da[0] != '~') {
        // Valid, but undefined
        free(ret);
        ret = (char *)malloc(sizeof(char));
        ret[0] = '\0';
    }

    return ret;
}

tString expand_home(tString const & pathname) {
    const char *pn = (const char *)pathname;
    char *s;
    size_t len = 0;
    tString r;

    if ((pn[0] == '~' || !strncmp(pn, "${", 2)) && (s = eh_getdir(pn, &len))) {
        tString sStr(s);
        r = sStr << (pn + len);
        free(s);
    }
    else
        r = pathname;

#ifdef DEBUG
    if(strcmp(pathname, r))
    {
        printf("changed %s to %s\n", (const char *)pathname, (const char *)r);
    }
#endif
    return r;
}

class tPathConfig: public tPath
{
public:
    tPathConfig() {}
private:
    void Paths ( tArray< tString >& paths ) const
    {
        paths.SetLen( 0 );
        int pos = 0;

        paths[ pos++ ] = st_DataDir + "/config";

        if ( st_ConfigDir.Len() > 1 )
        {
            paths[ pos++ ] = st_ConfigDir;
        }

#ifdef LEGACY_USER_DATA_DIR
        if ( st_LegacyUserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir + "/config";
        }
#endif

#ifdef LEGACY_USER_DATA_DIR2
        if ( st_LegacyUserDataDir2.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir2 + "/config";
        }
#endif

        if ( st_UserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_UserDataDir + "/config";
        }

        if ( st_UserConfigDir.Len() > 1 )
        {
            paths[ pos++ ] = st_UserConfigDir;
        }
    }
};

static const tPathConfig st_Config;

class tPathData: public tPath
{
public:
    tPathData() {}
private:
    void Paths ( tArray< tString >& paths ) const
    {
        paths.SetLen( 0 );
        int pos = 0;

        paths[ pos++ ] = st_DataDir;


        // for finding data packet in 0install
        char const * extradata = getenv("ARMAGETRONAD_EXTRADATA");
        static bool isThere = extradata;
        // should probably be spit like proper unix paths, colon separated
        if( isThere )
        {
            static tString ed(extradata);
            paths[ pos++ ] = ed;
        }

#ifdef LEGACY_USER_DATA_DIR
        if ( st_LegacyUserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir;
        }
#endif

#ifdef LEGACY_USER_DATA_DIR2
        if ( st_LegacyUserDataDir2.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir2;
        }
#endif

        if ( st_UserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_UserDataDir;
        }
    }
};

static const tPathData st_Data;

// This doesn't work because I don't understand this stuff at all :(  --Lucifer
class tPathMusic: public tPath
{
public:
    tPathMusic() {}
private:
    void Paths ( tArray< tString >& paths ) const
    {
        paths.SetLen( 0 );
        int pos = 0;

        paths[ pos++ ] = st_MusicDir;
    }
};

static const tPathMusic st_Music;

#ifdef DEDICATED
void tPathWebroot::Paths ( tArray< tString >& paths ) const
{
    paths.SetLen( 0 );
    int pos = 0;

    paths[ pos++ ] = st_WwwDir;
}

tString tPathWebroot::GetDirPath() {
    //std::cout << st_WwwDir;
    return st_WwwDir;
}

static const tPathWebroot st_Webroot;

const tPathWebroot& tDirectories::Webroot()
{
    return st_Webroot;
}

#endif

class tPathVar: public tPath
{
public:
    tPathVar() {}
private:
    void Paths ( tArray< tString >& paths ) const
    {
        paths.SetLen( 0 );
        int pos = 0;

        paths[ pos++ ] = st_DataDir + "/var";

#ifdef LEGACY_USER_DATA_DIR
        if ( st_LegacyUserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir + "/var";
        }
#endif

#ifdef LEGACY_USER_DATA_DIR2
        if ( st_LegacyUserDataDir2.Len() > 1 )
        {
            paths[ pos++ ] = st_LegacyUserDataDir2 + "/var";
        }
#endif

        if ( st_UserDataDir.Len() > 1 )
        {
            paths[ pos++ ] = st_UserDataDir + "/var";
        }

        if ( st_VarDir.Len() > 1 )
        {
            paths[ pos++ ] = st_VarDir;
        }
    }
};

static const tPathVar st_Var;


class tPathScreenshot: public tPath
{
public:
    tPathScreenshot() {}
private:
    void Paths ( tArray< tString >& paths ) const
    {
        paths.SetLen( 0 );
        int pos = 0;

        paths[ pos++ ] = st_DataDir + "/screenshot";

        if ( st_UserDataDir.Len() > 1 )
            paths[ pos++ ] = st_UserDataDir + "/screenshot";

        if ( st_ScreenshotDir.Len() > 1 )
            paths[ pos++ ] = st_ScreenshotDir;
    }
};

static const tPathScreenshot st_Screenshot;


tString tPathResource::GetWritePath(const char *filename) const {
    if ( !tPath::IsValidPath( filename ) )
        return tString();

    tArray< tString > paths;
    Paths( paths );

    tString fullname;
    fullname << paths(0) << "/" << filename;

    {
        bool s;

        char *fpmr = strdup( static_cast< char const * >( fullname ) );
        s = mkdir_recurse(fpmr, paths(0).Len());
        free(fpmr);

        if (!s)
        {
            tERR_WARN( tOutput( "$directory_path_nonwritable",  fullname  ) );
            return tString();
        }
    }

    return fullname;
}

tString tPathResource::GetDirPath()
{
    if ( st_IncludedResourceDir.Len() > 2 )
        return st_IncludedResourceDir;
    else
        return st_DataDir + "/resource/included";
}

void tPathResource::Paths(tArray< tString >& paths) const {
    paths.SetLen( 0 );
    int pos = 0;

    if ( st_AutoResourceDir.Len() > 1 )
        paths[ pos++ ] = st_AutoResourceDir;
    else if ( st_ResourceDir.Len() > 1 )
        paths[ pos++ ] = st_ResourceDir + "/automatic";
    else if ( st_UserDataDir.Len() > 1 )
        paths[ pos++ ] = st_UserDataDir + "/resource/automatic";
    else
        paths[ pos++ ] = st_DataDir + "/resource/automatic";

    paths[ pos++ ] = st_DataDir + "/resource/included";
    if ( st_IncludedResourceDir.Len() > 1 )
        paths[ pos++ ] = st_IncludedResourceDir;

    paths[ pos++ ] = st_DataDir + "/resource";
    if ( st_UserDataDir.Len() > 1 )
        paths[ pos++ ] = st_UserDataDir + "/resource";
    if ( st_ResourceDir.Len() > 1 )
        paths[ pos++ ] = st_ResourceDir;
}


static const tPathResource st_Resource;

tString tPathScripts::GetDirPath()
{
    return st_DataDir + "/scripts";
}

void tPathScripts::Paths(tArray< tString >& paths) const {
    paths.SetLen(0);
    paths[0] = GetDirPath();
}

static const tPathScripts st_Scripts;

bool tPath::Open    ( std::ifstream& f,
                      const char* filename   ) const
{
    if ( !tPath::IsValidPath( filename ) )
        return false;

    tArray< tString > paths;
    Paths( paths );

    for ( int prio = paths.Len() - 1; prio>=0; --prio )
    {
        //  std::ifstream test;

        tString fullname;
        fullname << paths( prio ) << "/" << filename;

#ifdef PRINTSEARCH
#endif

        //  test.open( fullname );
        f.clear();
        f.open( fullname );

        //  if ( test )
        if ( f && f.good() )
        {
#ifdef PRINTSEARCH
            std::cout << "Trying to open " << fullname << " succeeded.";
#endif
            //   f.open( fullname );

            //   return f;
            return true;
        }

#ifdef PRINTSEARCH
        std::cout << "Trying to open " << fullname << " succeeded.";
#endif
    }

    return false;
}

static bool st_protectFiles = true;
tSettingItem<bool> st_protectFilesConf("PROTECT_SENSITIVE_FILES", st_protectFiles);

bool tPath::Open    ( std::ofstream& f,
                      const char* filename,
                      std::ios::openmode mode,
                      bool sensitive) const
{
    if ( !tPath::IsValidPath( filename ) )
        return false;

    // tArray< tString > paths;
    // Paths( paths );

    tString fullname = GetWritePath(filename);

#ifndef WIN32
    mode_t oldmask=0;
    if(sensitive && st_protectFiles)
    {
        oldmask = umask(0600);
    }
#endif
    f.open( fullname, mode );
#ifndef WIN32
    if(sensitive && st_protectFiles)
    {
        chmod( &fullname(0), 0600 );
        umask(oldmask);
    }
#endif

    return ( f && f.good() );
}

bool tPath::Open(std::fstream& f, const char* filename) const
{
    //	std::cout << "open\n";

    tString fullname = GetWritePath(filename);
    f.open(fullname, std::ios::in | std::ios::out);

    return ( f.good() );
}

tString tPath::GetReadPath   ( const char* filename   ) const
{
    if ( !tPath::IsValidPath( filename ) )
        return tString();

    tArray< tString > paths;
    Paths( paths );

    for ( int prio = paths.Len() - 1; prio>=0; --prio )
    {
        tString fullname;
        fullname << paths( prio ) << "/" << filename;
        std::ifstream f;

        //if (fullname != "./moviepack/sky.png")
#ifdef PRINTSEARCH
        printf("Searching %s...", (const char *)fullname);
#endif
        f.open( fullname );

        if ( f && f.good() )
        {
            //if (fullname != "./moviepack/sky.png")
#ifdef PRINTSEARCH
            printf("OK\n");
#endif
            return fullname;
        }
        //if (fullname != "./moviepack/sky.png")
#ifdef PRINTSEARCH
        printf("nope\n");
#endif
    }

    return tString();
}

tString tPath::GetWritePath  ( const char* filename   ) const
{
    if ( !tPath::IsValidPath( filename ) )
        return tString();

    tArray< tString > paths;
    Paths( paths );

    tString fullname;
    fullname << paths( paths.Len() -1 ) << "/" << filename;

    {
        bool s;

        char *fpmr = strdup( static_cast< char const * > ( fullname ) );
        s = mkdir_recurse(fpmr, paths( paths.Len() -1 ).Len());
        free(fpmr);

        if (!s)
        {
            tERR_WARN( "Could not create path to " << fullname << ". Check your user's rights." );
            return tString();
        }
    }

    return fullname;
}

const tPath& tDirectories::Data()
{
    return st_Data;
}

const tPath& tDirectories::Music()
{
    return st_Music;
}

const tPath& tDirectories::Config()
{
    return st_Config;
}

const tPath& tDirectories::Scripts()
{
    return st_Scripts;
}

const tPath& tDirectories::Var()
{
    return st_Var;
}

const tPath& tDirectories::Screenshot()
{
    return st_Screenshot;
}

const tPathResource& tDirectories::Resource()
{
    return st_Resource;
}

// set location of data directory
void tDirectories::SetData( const tString& dir )
{
    st_DataDir = dir;
}

// set location of user data directory
void tDirectories::SetUserData( const tString& dir )
{
    st_UserDataDir = dir;
}

// set location of config directory
void tDirectories::SetConfig( const tString& dir )
{
    st_ConfigDir = dir;
}

// set location of user config directory
void tDirectories::SetUserConfig( const tString& dir )
{
    st_UserConfigDir = dir;
}

// set location of var directory
void tDirectories::SetVar( const tString& dir )
{
    st_VarDir = dir;
}

// set location of screenshot directory
void tDirectories::SetScreenshot( const tString& dir ) {
    st_ScreenshotDir = dir;
}

void tDirectories::SetResource( const tString& dir ) {
    st_ResourceDir = dir;
}

void tDirectories::SetAutoResource( const tString& dir ) {
    st_AutoResourceDir = dir;
}

void tDirectories::SetIncludedResource( const tString& dir ) {
    st_IncludedResourceDir = dir;
}

tString const & tDirectories::GetUserData()
{
    return st_UserDataDir;
}

tString const & tDirectories::GetData()
{
    return st_DataDir;
}

tString tDirectories::GetConfig()
{
    return st_ConfigDir.Len() > 1 ? st_ConfigDir : (st_DataDir + "/config");
}

tString tDirectories::GetUserConfig()
{
    return st_UserConfigDir.Len() > 1 ? st_UserConfigDir : (st_UserDataDir + "/config");
}

static tString st_BuildCWD()
{
#define MAX_CWD 1000

#ifdef WIN32
#define getcwd _getcwd
#endif

    char buffer[MAX_CWD+2];
    return tString( getcwd( buffer, MAX_CWD ) );
}

tString const & tDirectories::GetCWD()
{
    static tString ret = st_BuildCWD();
    return ret;
}

/*
 * robust glob pattern matcher
 * ozan s. yigit/dec 1994
 * public domain
 * http://www.cs.yorku.ca/~oz/glob.bun
 *
 * glob patterns:
 * * matches zero or more characters
 * ? matches any single character
 *
 * char matches itself except where char is '*' or '?' or '['
 * \char matches char, including any pattern character
 *
 * examples:
 * a*c  ac abc abbc ...
 * a?c  acc abc aXc ...
 *
 * Revision 1.5  2004/08/24  12:24:23  k
 * added case sensitive/insensitive option
 *
 * Revision 1.4  2004/08/17  12:24:23  k
 * removed [a-z] checking to match Windows wildcard parsing
 */
// check if a file name matches a wildcard (* and ? are valid wild cards)
bool tDirectories::FileMatchesWildcard(const char *str, const char *pattern,
                                       bool ignoreCase /* = true */)
{
    int c;

    while (*pattern)
    {
        if (!*str && *pattern != '*')
            return false;

        switch (c = *pattern++)
        {

        case '*':
            while (*pattern == '*')
                pattern++;

            if (!*pattern)
                return true;

            if (*pattern != '?' && *pattern != '\\')
            {
                if (ignoreCase)
                {
                    while (*str && tolower(*pattern) != tolower(*str) )
                        str++;
                }
                else
                {
                    while (*str && *pattern != *str )
                        str++;
                }
            }

            while (*str)
            {
                if (FileMatchesWildcard(str, pattern, ignoreCase))
                    return true;
                str++;
            }
            return false;

        case '?':
            if (*str)
                break;
            return false;

        case '\\':
            if (*pattern)
                c = *pattern++;
            [[fallthrough]];
        default:
            if (ignoreCase)
            {
                if (tolower(c) != tolower(*str))
                    return false;
            }
            else
            {
                if (c != *str)
                    return false;
            }
            break;

        }
        str++;
    }

    return !*str;
}

// get a list of files for a directory
// flag: 0=files+dirs, 1=files, 2=dirs
void tDirectories::GetFiles( const tString& dir, const tString& fileSpec,
                             tArray< tString >& files, int flag /* = eGetFilesAllFiles */ )
{
    tArray<tString> specList;
    long pos = 0;
    struct stat statbuf;
    tString temp;
    bool bDir = false;

    files.SetLen( 0 );

    // Check for multiple file specs
    GetSpecList( fileSpec, specList );

    DIR *dirp;
    struct dirent *entry;

    dirp = opendir( dir );

    if ( dirp != NULL )
    {
        while ( ( entry = readdir( dirp ) ) != NULL )
        {
            // Ignore "." and ".." entries
            if ( entry->d_name[0] != '.' )
            {
                // Build path.  Make sure dir ends with a '/' or '\'.
                temp = dir;
                if ( ( dir.Len() > 1 ) && ( dir[ dir.Len() - 2 ] != '/' && dir[ dir.Len() - 2 ] != '\\' ) )
                {
                    temp += "/";
                }
                temp += entry->d_name;

                // Is the entry a directory?
                bDir = false;
                if ( stat( temp, &statbuf ) == 0 )
                {
                    if( statbuf.st_mode & S_IFDIR )
                    {
                        bDir = true;
                        // Don't add directories when flag = 1
                        if ( flag == eGetFilesFilesOnly )
                            continue;
                    }
                    else
                    {
                        // Don't add files when flag = 2
                        if ( flag == eGetFilesDirsOnly )
                            continue;
                    }
                }

                // If the entry matches a file spec add it to the list
                for ( int i = 0; i < specList.Len(); i++ )
                {
                    if ( FileMatchesWildcard( entry->d_name, specList( i ), true ) )
                    {
                        files[ pos ] = entry->d_name;
                        if ( bDir )
                            files[ pos ] += "/";
                        pos++;
                        break;
                    }
                }
            }
        }

        closedir( dirp );
    }

    // Sort the list of files
    SortFiles( files );
}

// Convert a file name to a menu name (strip extension, replace '_' with ' ')
tString& tDirectories::FileNameToMenuName(const char* fileName, tString& menuName)
{
    char szBuf[256];
    int i = 0;

    // copy string to buffer for manipulation
    memset( szBuf, 0, sizeof( szBuf ) );
    strncpy( szBuf, fileName, sizeof( szBuf ) - 1 );

    // skip translation strings
    if ( szBuf[0] != '$' )
    {
        // strip extension
        for ( i = strlen(szBuf); i >= 0; i-- )
        {
            if ( szBuf[i] == '.' )
            {
                szBuf[i] = '\0';
            }
        }

        // replace underscores with spaces
        for ( i = 0; (unsigned)i < strlen(szBuf); i++ )
        {
            if ( szBuf[i] == '_' )
            {
                szBuf[i] = ' ';
            }
        }
    }

    menuName = szBuf;

    return menuName;
}

// split the file spec into a list
void tDirectories::GetSpecList( const tString& fileSpec, tArray< tString >& specList )
{
    char szBuf[256];
    char szSep[] = ";";
    char *token = NULL;
    long pos = 0;

    specList.SetLen( 0 );

    // Check for multiple file specs
    memset( szBuf, 0, sizeof( szBuf ) );
    strncpy( szBuf, (const char *) fileSpec, sizeof( szBuf ) - 1 );
    token = strtok( szBuf, szSep );
    while( token != NULL )
    {
        specList[ pos++ ] = token;
        token = strtok( NULL, szSep );
    }
}

// Helper function to see if tString s1 is less than s2 ignoring case
static bool tStringLessThan(const tString &s1, const tString &s2)
{
    for (int i = 0; i < s1.Len() - 1; i++)
    {
        if ( tolower( s2( i ) ) >= tolower( s1( i ) ) )
        {
            return false;
        }
    }
    return true;
}

// Sort the list of files
void tDirectories::SortFiles( tArray< tString >& files )
{
    tString temp;
    long pos = 0;

    // bubble sort for now
    for ( pos = 0; pos < files.Len() - 1; pos ++ )
    {
        for ( long pos2 = pos + 1; pos2 < files.Len(); pos2++ )
        {
            //if ( strcmp( (const char *) files( pos2 ), (const char *) files( pos ) ) < 0 )
            if ( tStringLessThan( files( pos2 ), files( pos ) ) )
            {
                temp = files( pos );
                files( pos ) = files( pos2 );
                files( pos2 ) = temp;
            }
        }
    }
}

/*
static void quitWithMessage( const char* message )
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
*/

//#define QUIT(x) { std::ostringstream s; s << x; quitWithMessage(s.str().c_str()); name_.Clear(); } exit(0)
#define QUIT(x) { std::ostringstream s; s << x; quitWithMessage(s.str().c_str()); name_.Clear(); } return false

/*
// reads a command line option
bool ReadOption( int& i, int argc, char **argv, const char* argument, tString& target )
{
    if ( !strcmp(argv[i],argument ) )
    {
        if ( i < argc - 1 )
        {
            i++;
            target = argv[i];

            return true;
        }
        else
        {
            tString name_;
            QUIT( "\n\n" << argument << " needs another argument.\n\n" );
        }
    }

    return false;
}
*/

void ReplacePath( tString & path, char const * replacement )
{
    // don't do a thing if the path is already set
    if ( path.Len() < 3 )
    {
        path = replacement;
    }
}

// tests whether <file> can be found in path <path>
bool TestPath( char const * path, char const * file )
{
    std::string testData( path );
    testData += "/";
    testData += file;
    std::ifstream f(testData.c_str());


#ifdef DEBUG_PATH
    con << "Testing existence of file " << testData << ( f.good() ? " : good.\n" : " : bad!\n" );
#endif

    return ( f.good() );
}

// tests whether the given path is a valid configuration path
bool TestConfigurationPath( char const * path )
{
    if ( TestPath( path, "settings.cfg" ) )
    {
#ifdef DEBUG_PATH
        con << "Configuration path " << path << " is good.\n";
#endif
        // replace data paths
        ReplacePath( st_ConfigDir, path );

        return true;
    }

#ifdef DEBUG_PATH
    con << "Configuration path " << path << " is bad!\n";
#endif

    return false;
}

// tests whether the given path is a valid data path
bool TestDataPath( char const * path )
{
    if ( TestPath( path, "language/english_base.txt") )
    {
#ifdef DEBUG_PATH
        con << "Data path " << path << " is good.\n";
#endif
        // replace data paths
        ReplacePath( st_DataDir, path );

        return true;
    }

#ifdef DEBUG_PATH
    con << "Data path " << path << " is bad!\n";
#endif

    return false;
}

// generates parent directories of the passed path
static tString GetParent( char const * child, int levels )
{
    tString ret( child );

// TODO Add windows support for this behavior.
#ifndef WIN32    
    if ( ret == "." )
    {
        ret = "";
    }
    // If it's a relative path, ensure it starts with "./"
    else if ( ret.compare( 0, 1, "/" ) != 0 && ret.compare( 0, 2, "./" ) != 0 )
    {
        ret = "./" + ret;
    }
    bool isRelative = ret.compare( 0, 1, "/" ) != 0;
#endif

    // strip the requested number of path segments
    int toStrip = levels;
    int stripCurrent = ret.Size()-1;
    while (stripCurrent >= 0 && toStrip > 0)
    {
        const char & c = ret[ stripCurrent ];
        // count separators
        if (c == '/' || c == '\\' || c == ':')
            --toStrip;
        --stripCurrent;
    }
    ret.erase( stripCurrent + 1 );

// TODO Add windows support for this behavior.
#ifndef WIN32    
    if ( toStrip && isRelative )
    {
        // No more path segments to strip, but we still need to go down more levels
        tString moreLevels;
        for (int i = 0; i < toStrip; i++)
        {
            // We don't want the path to end with a "/"
            if (i > 0)
                moreLevels += "/";
            moreLevels += "..";
        }
        ret = moreLevels + ret;
    }
    else if ( ret == "" )
    {
        ret = isRelative ? "." : "/";
    }
#endif

#ifdef DEBUG_PATH
    std::cout << "Parent (levels=" << levels << "): " << ret << "\n";
#endif
    return ret;
}

#if 0
struct TestGetParent
{
    TestGetParent()
    {
        std::cout << "== Testing GetParent()\n";

        Test("./path/to/file", 1, "./path/to");
        Test("./path/to/file", 2, "./path");
        Test("./path/to/file", 3, ".");
        Test("./path/to/file", 4, "..");
        Test("./path/to/file", 5, "../..");
        
        Test( "path/to/file", 1, "./path/to" );
        Test( "path/to/file", 2, "./path" );
        Test( "path/to/file", 3, "." );
        Test( "path/to/file", 4, ".." );
        Test( "path/to/file", 5, "../.." );
        
        Test("/path/to/file", 1, "/path/to");
        Test("/path/to/file", 2, "/path");
        Test("/path/to/file", 3, "/");
        Test("/path/to/file", 4, "/");
        
        Test( ".", 1, ".." );
        Test( ".", 2, "../.." ),
        Test( ".", 3, "../../.." ),

        exit(0);
    }
    void Test( char const *path, int levels, char const *expectedPath )
    {
        tString gotPath = GetParent( path, levels );
        if ( gotPath != expectedPath )
        {
            std::cout << "TEST FAILURE\npath=" << path << " levels=" << levels << '\n';
            std::cout << "Got '" << gotPath << "', but expected '" << expectedPath << "'\n\n";
        }
    }
};
static TestGetParent st_getParentTests;
#endif

//! Class holding our best guess of the path to the binary executable
class tPathToExecutable
{
public:
    //! sets a default path, for when we have no better idea
    void Set( char const * defaultPath )
    {
#ifndef WIN32
#ifdef ENABLE_BINRELOC
        // get path of executable
        char const * bestGuess = SELFPATH;
#else // binreloc
        char const * bestGuess = BINDIR "/" PROGNAME;
#endif// binreloc
#else // win32
        char const * bestGuess = "./" PROGNAME;
#endif// win32

#ifndef ENABLE_BINRELOC
        // if the passed default path is a real path, let it override the best guess
        if ( strstr( defaultPath, "/" ) || strstr( defaultPath, "\\" ) )
            bestGuess = defaultPath;
        //            bestGuess = "./armagetronad-dedicated";
#endif

        path_ = bestGuess;

#ifdef DEBUG_PATH
        con << "path to executable: " << path_ << "\n";
#endif
    }

    // returns the best bet
    char const * Get() const
    {
        tASSERT( path_.Len() > 2 );
        return path_;
    }
private:
    tString path_;
};

static tPathToExecutable st_pathToExecutable;

static tString GenerateParentOfExecutable( int levels = 2 )
{
    return GetParent( st_pathToExecutable.Get(), levels );
}

// exception to throw if the game is running from the build directory
struct tRunningInBuildDirectory
{
};

// generates real prefix from executable position and compiled in prefix and
// binary path
static tString GeneratePrefix()
{
    // fetch prefix as it was compiled in
    tString const & prefixCompiled = st_prefixCompiled;
    // the binary path as it was compiled in
    tString const & bindirCompiled = st_bindirCompiled;
    // and the current binary path
    tString bindirNow(GenerateParentOfExecutable(1));

    // the length of the bindir suffix, the part that is added below prefix
    int bindirSuffixLength=bindirCompiled.Len() - prefixCompiled.Len();

    // the end of the prefix part in binDirNow according to that
    int bindirNowPrefixEnd=bindirNow.Len() - 1 - bindirSuffixLength;
    if ( bindirNowPrefixEnd < 0 )
        bindirNowPrefixEnd = 0;

    // check that the binary path now ends the same way
    tString suffixNow      = bindirNow.SubStr( bindirNowPrefixEnd + 1 );
    tString suffixCompiled = bindirCompiled.SubStr( prefixCompiled.Len() );

#ifdef DEBUG_PATH
    con << "suffices: " << suffixNow << ", " << suffixCompiled << "\n";
#endif

    if ( suffixNow != suffixCompiled )
    {
        // may we be running inside the build directory?
        int bindirEndStart = bindirNow.Len() - 5;
        if ( bindirEndStart < 0 )
            bindirEndStart = 0;

        tString bindirEnd = bindirNow.SubStr( bindirEndStart );

#ifdef DEBUG_PATH
        con << "bindirEnd: " << bindirEnd << "\n";
#endif

        if ( TestPath( bindirNow, "Makefile" ) )
            throw tRunningInBuildDirectory();

        tERR_ERROR("Relocation error. The binary was supposed to be installed into " << bindirCompiled << " and found itself in " << bindirNow << " and could not find out what this means for the prefix " << prefixCompiled << "." );
    }

    // generate prefix
    tString prefixNow = bindirNow.SubStr( 0, bindirNowPrefixEnd );

#ifdef DEBUG_PATH
    con << "prefix: " << prefixNow << "\n";
#endif

    return prefixNow;
}

// returns the complete prefix the game was installed in (defaults to /usr/local)
static char const * GetPrefix()
{
    static tString ret( GeneratePrefix() );
    return ret;
}

/*
// appends the given suffix to the prefix and returns the result
static tString AddPrefix( const char * suffix )
{
    tString ret(GetPrefix());
    ret += suffix;

    return ret;
}
*/

#ifndef WIN32
#ifndef MACOSX_XCODE
static tString st_RelocatePath( tString const & original )
{
    // fetch prefix as it was compiled in
    tString const & prefixCompiled = st_prefixCompiled;
    // and as it is now
    static tString prefixNow(GetPrefix());

    // see if the passed string starts with it
    if ( original.StartsWith( prefixCompiled ) )
    {
        // replace the prefix with the real prefix and return the result
        return prefixNow + original.SubStr( prefixCompiled.Len()-1 );
    }
    else
    {
        // don't relocate and hope it works
        return original;
    }
}
#endif
#endif

// tries to find the path to the data files, given the location of the executable
static void FindDataPath()
{
#if defined(WIN32)
    // look for data in the same directory as the executable
    if ( TestDataPath(GetParent(st_pathToExecutable.Get(), 1) ) ) return;
#elif defined(MACOSX_XCODE)
#ifdef DEDICATED
    if ( TestDataPath( GetParent( st_pathToExecutable.Get(), 2 ) ) ) return;
#else
    if ( TestDataPath( GetParent( st_pathToExecutable.Get(), 2 ) + "/Resources" ) ) return;
#endif
#else
    // try to use path substitution
    if ( TestDataPath( st_RelocatePath( tString(AA_DATADIR ) ) ) ) return;
    // if ( TestDataPath( AddPrefix( DATASUFFIX ) ) ) return;
#endif

#ifdef DEBUG_PATH
    con << "Data sarch failed, trying debug fallback.\n";
#endif

#ifdef DEBUG_PATH
    tERR_MESSAGE("Could not determine path to data files. Using defaults or command line arguments.\n");
#endif
}

// tries to find the path to the configuration files, given the location of the executable
static void FindConfigurationPath()
{
#ifndef MACOSX_XCODE
#ifndef WIN32
    if ( TestConfigurationPath( st_RelocatePath( tString( AA_SYSCONFDIR ) ) ) ) return;
#endif
#endif

    // look for configuration where the data is
    if ( TestConfigurationPath(st_DataDir + "/config") )
    {
        return;
    }
#ifdef MACOSX_XCODE
    else if ( TestPath( st_DataDir, "Makefile" ) )
    {
        throw tRunningInBuildDirectory();
    }
#endif

    tERR_WARN("Could not determine path to configuration files. Using defaults or command line arguments.\n");
}

// tries to read a direcory type argument from the command line parser; result is written
// into target, argument is the required switch ("--userdatadir")
static bool ReadDir( tCommandLineParser & parser, tString & target, const char* argument )
{
    if ( parser.GetOption( target, argument ) )
    {
        target = expand_home_c( target );

        return true;
    }

    return false;
}

void tDirectoriesCommandLineAnalyzer::DoInitialize( tCommandLineParser & parser )
{
    // Puts the data files in the executable's bundle
    try
    {
        st_pathToExecutable.Set( parser.Executable() );
        FindDataPath();
        FindConfigurationPath();

#ifdef LEGACY_USER_DATA_DIR
        // blank out legacy user data dir if it matches the real user data dir
        if ( st_UserDataDir == st_LegacyUserDataDir )
        {
            st_LegacyUserDataDir = "";
        }
#endif

#ifdef LEGACY_USER_DATA_DIR2
        // blank out legacy user data dir if it matches the real user data dir
        if ( st_UserDataDir == st_LegacyUserDataDir2 )
        {
            st_LegacyUserDataDir2 = "";
        }
#ifdef LEGACY_USER_DATA_DIR
        // blank out legacy user data dir if it matches the other legacy user data dir
        if ( st_LegacyUserDataDir == st_LegacyUserDataDir2 )
        {
            st_LegacyUserDataDir2 = "";
        }
#endif
#endif
    }
    catch( tRunningInBuildDirectory )
    {
        // last fallback for debugging (activated only if there is data in the current directory)
        if ( TestPath( ".", "language/languages.txt") && TestDataPath(s_topSourceDir) && TestConfigurationPath(st_DataDir + "/config") )
        {
            // we must be running the game in debug mode; set user data dir to current directory.
            st_UserDataDir = ".";
            st_UserConfigDir = "./userconfig";

            // the included resources are scrambled and put into the current directory as well.
            st_IncludedResourceDir = "./resource/included";

#ifdef LEGACY_USER_DATA_DIR
            st_LegacyUserDataDir = "";
#endif
#ifdef LEGACY_USER_DATA_DIR2
            st_LegacyUserDataDir2 = "";
#endif
            return;
        }
    }
}

bool tDirectoriesCommandLineAnalyzer::DoAnalyze( tCommandLineParser & parser )
{
    if( ReadDir( parser, st_DataDir, "--datadir" ) ) return true;
    if( ReadDir( parser, st_UserDataDir, "--userdatadir" ) ) return true;
    if( ReadDir( parser, st_ConfigDir, "--configdir" ) ) return true;
    if( ReadDir( parser, st_UserConfigDir, "--userconfigdir" ) ) return true;
    if( ReadDir( parser, st_VarDir, "--vardir" ) ) return true;
    
    if ( enableExtraOptions_ )
    {
        if( ReadDir( parser, st_ResourceDir, "--resourcedir" ) ) return true;
        if( ReadDir( parser, st_AutoResourceDir, "--autoresourcedir" ) ) return true;

        if ( parser.GetSwitch( "--path-no-absolutecheck" ) )
        {
            st_checkPathAbsolute = false;
            return true;
        }

        if ( parser.GetSwitch( "--path-no-relativecheck" ) )
        {
            st_checkPathRelative = false;
            return true;
        }

        if ( parser.GetSwitch( "--path-no-hiddencheck" ) )
        {
            st_checkPathHidden = false;
            return true;
        }

        if ( parser.GetSwitch( "--prefix" ) )
        {
            std::cout << GetPrefix() << '\n';
            throw 1;
            return true;
        }
    }

    return false;
}

void tDirectoriesCommandLineAnalyzer::DoHelp( std::ostream & s )
{                                      //
    s << "--datadir <Directory>        : read game data (textures, sounds and texts)\n"
    <<   "                               from this directory\n";
    s << "--userdatadir <Directory>    : read customized game data from this directory\n";
    s << "--configdir <Directory>      : read game configuration (.cfg-files)\n"
    <<   "                               from this directory\n";
    s << "--userconfigdir <Directory>  : read user game configuration from this directory\n";
    s << "--vardir <Directory>         : save game logs and highscores in this directory\n";
    
    if ( enableExtraOptions_ )
    {
        s << "\n--resourcedir <Directory>    : look for resources in this directory\n\n";
        s << "--autoresourcedir <Directory>: download missing resources into this directory\n\n";
        s << "--path-no-absolutecheck      : disables security check against absolute paths\n";
        s << "--path-no-hiddencheck        : disables security check against hidden paths\n";
        s << "--path-no-relativecheck      : disables security check against relative paths.\n"
        <<   "                               Not recommended, this check is really important.\n\n";
        s << "--prefix                     : prints the prefix the game was installed to\n";
    }
}

static tDirectoriesCommandLineAnalyzer analyzer;

tString tPath::GetPaths(void) const {
    tString ret;
    tArray<tString> paths;
    Paths(paths);
    for (int i = 0; i < paths.Len(); ++i) {
        if(i > 0 && paths[i - 1] == paths[i]) continue;
        ret << " - " << paths[i] << "\n";
    }
    return ret;
}

tString tPath::GetPaths(char const * delimiter, char const * finalizer) const {
    tString ret;
    tArray<tString> paths;
    Paths(paths);
    for (int i = 0; i < paths.Len(); ++i) {
        ret << paths[i];
        ret << ( (i == paths.Len() - 1) ? finalizer : delimiter );
    }
    return ret;
}

extern char *st_userConfigs[];
void st_PrintPathInfo(tOutput &buf) {
    tString const hcol("0xff8888");
    const char * userCfg = "user_3_1_utf8.cfg";
    string uc = tDirectories::Config().GetReadPath(userCfg);
    if(uc.size() <= 1)
        uc = tDirectories::Var().GetReadPath(userCfg);
    buf << hcol << "$path_info_user_cfg"   << "0xRESETT\n   " << uc << "\n"
    << hcol << "$path_info_config"     << "0xRESETT\n" << tDirectories::Config().GetPaths()
    << hcol << "$path_info_resource"   << "0xRESETT\n" << tDirectories::Resource().GetPaths()
    << hcol << "$path_info_data"       << "0xRESETT\n" << tDirectories::Data().GetPaths()
    << hcol << "$path_info_screenshot" << "0xRESETT\n" << tDirectories::Screenshot().GetPaths()
    << hcol << "$path_info_var"        << "0xRESETT\n" << tDirectories::Var().GetPaths();
}
