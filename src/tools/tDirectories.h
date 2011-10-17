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

#ifndef ArmageTron_tDirectories_H
#define ArmageTron_tDirectories_H

#include <fstream>
#include "tArray.h"
#include "tString.h"

class tPath
{
public:
    bool Open    ( std::ifstream& f,
                   const char* filename   ) const; // opens a file to read

    bool Open    ( std::ofstream& f,
                   const char* filename,
                   std::ios::openmode mode = std::ios::out,
                   bool sensitive = false ) const; // opens a file to write

    bool Open    ( std::fstream& f,
                   const char* filename   ) const; // let's read and write at the same time shall we?

    tString GetReadPath   ( const char* filename   ) const; // finds the full path to a file to read
    tString GetWritePath  ( const char* filename   ) const; // finds the full path to a file to write
    static bool IsValidPath( char const * filename ); //!< checks whether filename is valid, i.e. does not endanger system security.

    tString GetPaths(void) const; //!< Puts all paths into a tString for outputting to the user
    tString GetPaths(char const * delimiter, char const * finalizer) const; //!< Puts all paths into a tString for outputting to the user

    tPath(){}
    virtual ~tPath(){}
protected:
    virtual void    Paths ( tArray< tString >& paths ) const = 0;  // maximum priority is given to paths[0]
};

class tPathResource: public tPath {
public:
    tPathResource() {}
    tString GetWritePath(const char *filename) const;
    static tString GetDirPath(); //!< returns the path to the included resources
private:
    void Paths(tArray< tString >& paths) const;
};

class tPathScripts: public tPath {
public:
    tPathScripts() {}
    static tString GetDirPath(); //!< returns the path to the included scripts
private:
    void Paths ( tArray< tString >& paths ) const;
};

#ifdef DEDICATED
class tPathWebroot: public tPath
{
public:
    tPathWebroot () {}
    static tString GetDirPath();
private:
    void Paths ( tArray< tString >& paths ) const;
};
#endif

class tDirectories
{
public:
    enum { eGetFilesAllFiles = 0, eGetFilesFilesOnly = 1, eGetFilesDirsOnly = 2 };

    static const tPath& Data();              // directory for game data
    static const tPath& Music();             // directory for game music
    static const tPath& Config();            // directory for static configuration files
    static const tPath& Var();               // directory for dynamic logs and highscores
    static const tPath& Screenshot();        // directory for screenshots
    static const tPathResource& Resource();  // directory for resources
    static const tPath& Scripts();

#ifdef DEDICATED    
    static const tPathWebroot& Webroot();    // directory for webroot of embedded web server
#endif

    static void SetData( const tString& dir );       // set location of data directory
    static void SetUserData( const tString& dir );   // set location of user data directory
    static void SetConfig( const tString& dir );     // set location of config directory
    static void SetUserConfig( const tString& dir ); // set location of user config directory
    static void SetVar( const tString& dir );        // set location of var directory
    static void SetScreenshot( const tString& dir ); // set location of screenshot directory
    static void SetResource( const tString& dir );
    static void SetAutoResource( const tString& dir );
    static void SetIncludedResource( const tString& dir );

    static tString const & GetData(); //!< returns the system data directory
    static tString const & GetUserData(); //!< returns the user data directory
    static tString GetConfig(); //!< returns the system configuration directory

    static tString const & GetCWD(); //!< returns the current working directory

    // get a list of files for a directory
    // flag: 0=files+dirs, 1=files, 2=dirs
    static void GetFiles( const tString& dir, const tString& fileSpec,
                          tArray< tString >& files, int flag = eGetFilesAllFiles );

    // check if a file name matches a wildcard (* and ? are valid wild cards)
    static bool FileMatchesWildcard( const char *str, const char *pattern,
                                     bool ignoreCase = true );

    // convert a file name to a menu name (strip extension, replace '_' with ' ')
    static tString& FileNameToMenuName( const char* fileName, tString& menuName );

    // split the file spec into a list
    static void GetSpecList( const tString& fileSpec, tArray< tString >& specList );

    // sort the list of files
    static void SortFiles( tArray< tString >& files );
};

#endif
