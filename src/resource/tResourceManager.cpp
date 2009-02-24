/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libxml/nanohttp.h>

#include "tConfiguration.h"
#include "tDict.h"
#include "tDirectories.h"
#include "tResourceManager.h"
#include "tString.h"

/***************************************************************
 *          tResourceManager                                   *
 ***************************************************************/

// This is a little ugly, open to suggestions :)
tResourceManager::Reference tResourceManager::__inst = tResourceManager::Reference(new tResourceManager() );
tResourceTypeMap* tResourceManager::m_ResourceList = new tResourceTypeMap();

/** The constructor for tResourceManager. */
tResourceManager::tResourceManager() {
    // stub constructor for now
}

tResourceManager::~tResourceManager() { }

/** When you need to carry a local reference for the Resource Manager,
 *  use this method to do so.  It may be more convenient for you thataway.
 */
tResourceManager::Reference tResourceManager::GetResourceManager() {
    if(!__inst) {
        __inst = Reference(new tResourceManager() );
    }

    return Reference(__inst);
}

/** Used by tResourceLoader to register a file loader of some sort.
 */
void tResourceManager::RegisterLoader()
{
}

tResource* tResourceManager::GetResource(const char *file, int typeID)
{
    // stub
    return NULL;
}

/**
 *   Call RegisterResourceType when you want to register a new resource
 *   type.  While that may be obvious, make sure you use the tResourceType
 *   class to describe the resource type.
 *
 *   Ok, fine, there's nothing non-obvious about this method.
 */
int tResourceManager::RegisterResourceType(tResourceType* newType) {
    if(m_ResourceList->find(newType->GetName() ) == m_ResourceList->end() ) {
        m_ResourceList->insert( make_pair( newType->GetName(), newType->Get_reference() ) );
        return 1;
    }
    return 0;
}

//tResource* GetResource(const char *file, int typeID) {
//    // stub
//}

// server determined resource repository
tString & tResourceManager::AccessRepoServer()
{
    static tString resRepoServer("http://resource.armagetronad.net/resource/");
    return resRepoServer;
}
// the nSettingItem is in gStuff.cpp

// client determined resource repository
tString & tResourceManager::AccessRepoClient()
{
    static tString resRepoClient("http://resource.armagetronad.net/resource/");
    return resRepoClient;
}

static tSettingItem<tString> conf_res_repo("RESOURCE_REPOSITORY_CLIENT", tResourceManager::AccessRepoClient());

static int myHTTPFetch(const char *URI, const char *filename, const char *savepath)
{
    void *ctxt = NULL;
    char *buf = NULL;
    FILE* fd;
    int len, rc;

    con << tOutput( "$resource_downloading", URI );
    // con << "Downloading " << URI << "...\n";

    ctxt = xmlNanoHTTPOpen(URI, NULL);
    if (ctxt == NULL) {
        con << "ERROR: ctxt is NULL\n";
        return 1;
    }

    if ( (rc = xmlNanoHTTPReturnCode(ctxt)) != 200 ) {
        con << tOutput( rc == 404 ? "$resource_fetcherror_404" : "$resource_fetcherror", rc );
        return 2;
    }

    fd = fopen(savepath, "wb");
    if (fd < 0) {
        xmlNanoHTTPClose(ctxt);
        con << tOutput( "$resource_no_write", savepath );
        return 3;
    }

    //xmlNanoHTTPFetchContent( ctxt, &buf, &len );
    int maxlen = 10000;
    buf = (char*)malloc(maxlen);
    while( (len = xmlNanoHTTPRead(ctxt, buf, maxlen)) > 0 ) {
        Ignore( fwrite(buf, len, 1, fd) );
    }
    free(buf);

    xmlNanoHTTPClose(ctxt);
    fclose(fd);


    con << "OK\n";

    return 0;
}

static int myFetch(const char *URIs, const char *filename, const char *savepath) {
    const char *r = URIs, *p, *n;
    char *u;
    size_t len;
    int rv = -1;
    // r = unprocessed data		p = end-of-item + 1		u = item
    // n = to-be r				len = length of item	savepath = result filepath

    while (r[0] != '\0') {
        while (r[0] == ' ') ++r;			// skip spaces at the start of the item
        (p = strchr(r, ';')) ? 0 : (p = strchr(r, '\0'));
        n = (p[0] == '\0') ? p : (p + 1);	// next item starts after the semicolon
        // NOTE: skip semicolons, *NOT* nulls
        while (p[-1] == ' ') --p;			// skip spaces at the end of the item
        len = (size_t)(p - r);
        if (len > 0) {						// skip this for null-length items
            u = (char*)malloc((len + 1) * sizeof(char));
            strncpy(u, r, len);
            u[len] = '\0';					// u now contains the individual URI
            rv = myHTTPFetch(u, filename, savepath);	// TODO: handle other protocols?
            free(u);
            if (rv == 0) return 0;		// If successful, return the file retrieved
        }
        r = n;								// move onto the next item
    }

    return rv;	// last error
}

/*
Allows for the fetching and caching of ressources available on the web,
such as maps (xml), texture (jpg, gif, bmp), sound and models.
Nota: On some forums (such as guru3.sytes.net), it is possible for the
download link not give information about the filename or type, ie:
http://guru3.sytes.net/download.php?id=1191. This is why the filename
parameter is required.
Parameters:
uri: The full uri to obtain the ressource
filename: The filename to use for the local ressource
Return a file handle to the ressource
NOTE: There must be *at least* one directory level, even if it is ./
*/
tString tResourceManager::locateResource(const char *file, const char *uri) {
    tString filepath, a_uri = tString(), savepath;
    int rv;

    char * to_free = NULL; // string to delete later

    {
        char const *pos, *posb;
        char *nf;
        size_t l;

        // Step 1: If 'file' has an open paren, cut everything after it off
        if ( (pos = strchr(file, '(')) ) {
            l = (size_t)(pos - file);
            nf = (char*)malloc((l + 1) * sizeof(char));
            strncpy(nf, file, l);
            nf[l] = '\0';
            file = nf;
            to_free = nf;

            // Step 2: Extract URI, if any
            ++pos;
            if ( (posb = strchr(pos, ')')) ) {
                l = (size_t)(posb - pos);
                nf = (char*)malloc((l + 1) * sizeof(char));
                strncpy(nf, pos, l);
                nf[l] = '\0';
                a_uri << nf << ';';
                free( nf );
            }
        }
    }
    // Validate paths and determine detination savepath
    if (!file || file[0] == '\0') {
        con << tOutput( "$resource_no_filename" );
        return (tString) NULL;
    }
    if (file[0] == '/' || file[0] == '\\') {
        con << tOutput( "$resource_abs_path" );
        return (tString) NULL;
    }
    savepath = tDirectories::Resource().GetWritePath(file);
    if (savepath == "") {
        con << tOutput( "$resource_no_writepath" );
        return (tString) NULL;
    }

    // Do we have this file locally ?
    filepath = tDirectories::Resource().GetReadPath(file);

    if (filepath != "")
    {
        if ( NULL != to_free )
            free( to_free );
        return filepath;
    }

    // Some sort of File not found
    if (uri && strcmp("0", uri))
        a_uri << uri << ';';

    // add repositories to uri
    if ( AccessRepoServer().Len() > 2 )
        a_uri << AccessRepoServer() << file << ';';

    if ( AccessRepoClient().Len() > 2 && AccessRepoClient() != AccessRepoServer() )
        a_uri << AccessRepoClient() << file << ';';

    con << tOutput( "$resource_not_cached", file );

    rv = myFetch((const char *)a_uri, file, (const char *)savepath);

    if ( NULL != to_free )
        free( to_free );

    if (rv)
        return (tString) NULL;
    return savepath;
}

FILE* tResourceManager::openResource(const char *file, const char *uri) {
    tString filepath;
    filepath = locateResource(file, uri);
    if ( filepath.Len() <= 1 )
        return NULL;
    return fopen((const char *)filepath, "r");
}

static void RInclude(std::istream& s)
{
    // prevent CASACL
    tCasaclPreventer preventer;

    tString resourceID;
    s >> resourceID;

    tString filename = tResourceManager::locateResource(resourceID);

    if ( filename )
    {
        st_Include( filename, true );

        return;
    }

    con << tOutput( "$config_rinclude_not_found", resourceID );
}

static tConfItemFunc s_RInclude("RINCLUDE",  &RInclude);

static bool st_checkAuthor(tString const &Author) {
    if(Author.empty() || Author[0] < 'A' || Author[0] > 'z' || ( Author[0] > 'Z' && Author[0] < 'a' ) || Author.find('/') != tString::npos) {
        tERR_WARN("Resource authors must start with a letter and may not contain slashes");
        return false;
    }
    return true;
}
static bool st_checkCategory(tString const &Category) {
    if(Category[0] == '/' || *Category.rbegin() == '/' || Category.find("/.") != tString::npos) {
        tERR_WARN("Resource categories must not start or end with a slash or dot or contain the sequence \"./\".");
        return false;
    }
    return true;
}
static bool st_checkName(tString const &Name) {
    if(Name.empty() || Name[0] == '.' || Name.find_first_of("-/") != tString::npos) {
        tERR_WARN("Resource names must not start with a dot or contain slashes or minus signs");
        return false;
    }
    return true;
}
static bool st_checkExtension(tString const &Extension) {
    if(Extension.empty() || Extension.find_first_of("/.") != tString::npos) {
        tERR_WARN("Resource extensions must not contain slashes or dots");
        return false;
    }
    return true;
}
static bool st_checkType(tString const &Type) {
    if(Type.empty() || Type.find_first_of("/.") != tString::npos) {
        tERR_WARN("Resource types must not contain slashes or dots");
        return false;
    }
    return true;
}
static bool st_checkVersion(tString const &Version) {
    if(Version.empty() || Version.find('/') != tString::npos) {
        tERR_WARN("Resource versions must not contain slashes");
        return false;
    }
    return true;
}

/****************************************************************
 *       tResourcePath                                          *
 ****************************************************************/


tResourcePath::tResourcePath(tString const &Author,
                             tString const &Category,
                             tString const &Name,
                             tString const &Version,
                             tString const &Type,
                             tString const &Extension,
                             tString const &URI) :
    m_Author   (Author   ),
    m_Category (Category ),
    m_Name     (Name     ),
    m_Version  (Version  ),
    m_Type     (Type     ),
    m_Extension(Extension),
    m_URI      (URI      ),
    m_Valid(false) {
    m_Path << Author << '/';
    if(!st_checkAuthor(Author)) return;
    if(!Category.empty()) {
        if(!st_checkCategory(Category)) return;
        m_Path << Category << '/';
    }
    if(!st_checkName(Name)) return;
    if(!st_checkExtension(Extension)) return;
    if(!st_checkType(Extension)) return;
    if(!st_checkVersion(Version)) return;
    m_Path << Name << '-' << Version << '.' << Type << '.' << Extension;
    if(!URI.empty()) {
        m_Path << '(' << URI << ')';
    }
    m_Valid = true;
}

tResourcePath::tResourcePath(tString const &Path) : m_Path(Path), m_Valid(false) {
    // check if an URI is attached
    tString::size_type uridelim = m_Path.find('(');
    if(uridelim != tString::npos) {
        // find the corresponding opening bracket
        if(*m_Path.rbegin() != ')') {
            tERR_WARN("Incomplete URI specification");
            return;
        }
        m_URI = m_Path.substr(uridelim + 1, m_Path.size() - uridelim - 2);
        --uridelim;
    } else {
        uridelim = m_Path.size() - 1;
    }

    tString::size_type authordelim = Path.find('/');
    if(authordelim == tString::npos || authordelim >= uridelim) {
        tERR_WARN("Resource paths need to contain at least one slash");
        return;
    }
    m_Author = Path.substr(0, authordelim);
    if(!st_checkAuthor(m_Author)) return;
    tString::size_type categorydelim = Path.rfind('/', uridelim);
    if(categorydelim != authordelim) {
        m_Category = Path.substr(authordelim + 1, categorydelim - authordelim - 1);
        if(!st_checkCategory(m_Category)) return;
    }
    tString::size_type namedelim = Path.find('-', categorydelim);
    if(namedelim == tString::npos || namedelim >= uridelim) {
        tERR_WARN("Resource path is missing the version delimiter ('-')");
        return;
    }
    m_Name = Path.substr(categorydelim+1, namedelim - categorydelim - 1);
    if(!st_checkName(m_Name)) return;

    // now parse from the back to the front to find the version (which can
    // contain dots)
    tString::size_type extensiondelim = Path.rfind('.', uridelim);
    if(extensiondelim == tString::npos || extensiondelim <= namedelim || extensiondelim >= Path.size() - 1) {
        tERR_WARN("Resource path is missing the extension delimiter ('.')");
    }
    m_Extension = Path.substr(extensiondelim + 1, uridelim - extensiondelim);
    if(!st_checkExtension(m_Extension)) return;
    tString::size_type typedelim = Path.rfind('.', extensiondelim - 1);
    if(typedelim == tString::npos || typedelim <= namedelim) {
        tERR_WARN("Resource path is missing the type delimiter ('.')");
    }
    m_Type = Path.substr(typedelim + 1, extensiondelim - typedelim - 1);
    if(!st_checkType(m_Type)) return;

    // the rest is (hopefully) the version, now...
    m_Version = Path.substr(namedelim + 1, typedelim - namedelim - 1);
    if(!st_checkVersion(m_Version)) return;
    m_Valid=true;
}

bool tResourcePath::operator==(tResourcePath const &other) const {
    return m_Author    == other.m_Author    &&
           m_Category  == other.m_Category  &&
           m_Name      == other.m_Name      &&
           m_Version   == other.m_Version   &&
           m_Type      == other.m_Type      &&
           m_Extension == other.m_Extension;
}

// separate implementation to exploit lazy condition evaluation
bool tResourcePath::operator!=(tResourcePath const &other) const {
    return m_Author    != other.m_Author    ||
           m_Category  != other.m_Category  ||
           m_Name      != other.m_Name      ||
           m_Version   != other.m_Version   ||
           m_Type      != other.m_Type      ||
           m_Extension != other.m_Extension;
}
