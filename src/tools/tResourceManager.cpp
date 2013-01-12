#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libxml/nanohttp.h>

//#include <windows.h>
//#include <wininet.h>

#include "tDirectories.h"
#include "tResourceManager.h"
#include "tString.h"

//#include "../engine/ePlayer.h"

//  stores the list of resource repository hosts
static tSettingResource sg_resourceRepositoryBackups("RESOURCE_REPOSITORY_BACKUPS");

// server determined resource repository
tString tResourceManager::resRepoServer("http://resource.armagetronad.net/resource/");
// the nSettingItem is in gStuff.cpp

// client determined resource repository
tString tResourceManager::resRepoClient("http://resource.armagetronad.net/resource/");
static tSettingItem<tString> conf_res_repo("RESOURCE_REPOSITORY_CLIENT", tResourceManager::resRepoClient);

/*static void st_resourceRepositoryCheck(std::istream &s)
{
    tString filename;
    filename.ReadLine(s);

    //  don't do it if the check string is blank
    if (filename.Filter() == "") return;

    char c;
    tString outputLine;
    ePlayerNetID *receiver = 0;

    if (sg_resourceRepositoryBackups.Size() > 0)
    {
        int maxlen = 0;
        for(int a = 0; a < sg_resourceRepositoryBackups.Size(); a++)
        {
            tString strString = sg_resourceRepositoryBackups.Get(a);
            if (maxlen < strString.Len())
                maxlen = strString.Len();
        }

        outputLine << tOutput("$resource_check", filename);
        for(int i = 0; i < sg_resourceRepositoryBackups.Size(); i++)
        {
            tString output;
            tString resourceHost = sg_resourceRepositoryBackups.Get(i);

            output << tOutput("$resource_check_host", resourceHost);
            output.SetPos(maxlen + 1, false);
            output << tOutput("$resource_check_seperator") << " ";

            //  gotta remove the following or won't work
            if (resourceHost.Contains("http://"))
                resourceHost = resourceHost.RemoveWord("http://");
            else if (resourceHost.Contains("https://"))
                resourceHost = resourceHost.RemoveWord("ftp://");
            else if (resourceHost = resourceHost.RemoveWord("ftp://"))
                resourceHost = resourceHost.RemoveWord("ftp://");

            //  get the list of paths in url
            tArray<tString> resourcePaths = resourceHost.Split("/");
            bool first = false;
            tString strUrlPath;
            for(int j = 0; j < resourcePaths.Len(); j++)
            {
                if (!first)
                {
                    resourceHost = resourcePaths[j];
                    first = true;
                }
                else
                    strUrlPath << "/" << resourcePaths[j];
            }
            strUrlPath << "/" << filename;

            HINTERNET hSession = InternetOpen("MyAgent",INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            HINTERNET hConnect = InternetConnect(hSession, resourcePaths[0], INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            if (hConnect)
            {
                HINTERNET hFile = HttpOpenRequest(hConnect, NULL, strUrlPath, NULL, NULL, NULL, 0, 0);
                if (HttpSendRequest(hFile, NULL, 0, NULL, 0))
                {
                    output << tOutput("$resource_check_filefound");
                }
                else
                {
                    output << tOutput("$resource_check_filenotfound");
                }
                InternetCloseHandle(hFile);
            }
            else
            {
                output << tOutput("$resource_check_hostfail");
            }
            InternetCloseHandle(hSession);
            InternetCloseHandle(hConnect);

            outputLine << output;
        }

        //  send output of the data to file located in .../var/
        std::ofstream o;
        if (tDirectories::Var().Open(o, "repositorylog.txt"))
            o << outputLine;

        sn_ConsoleOut(outputLine, receiver->Owner());   //  set it to the one calling this function
    }


    //! Below is the old method, above is the new method.
    //! New method requires "-lwininet" included in linker options
    /*void *ctxt = NULL;
    tString outputLine;
    ePlayerNetID *receiver = 0;

    int maxlen = 0;
    for(int a = 0; a < sg_resourceRepositoryBackups.Size(); a++)
    {
        tString strString = sg_resourceRepositoryBackups.Get(a);
        if (maxlen < strString.Len())
            maxlen = strString.Len();
    }

    if (sg_resourceRepositoryBackups.Size() > 0)
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, "repositorylog.txt"))
        {
            outputLine << tOutput("$resource_check", filename);
            for(int i = 0; i < sg_resourceRepositoryBackups.Size(); i++)
            {
                tString output;
                tString resourceHost = sg_resourceRepositoryBackups.Get(i);

                output << tOutput("$resource_check_host", resourceHost);
                output.SetPos(maxlen + 1, false);
                output << tOutput("$resource_check_seperator") << " ";

                tString strUrl;
                strUrl << resourceHost << filename;

                const char *newURL = strUrl;
                ctxt = xmlNanoHTTPOpen(newURL, NULL);
                if (ctxt)
                {
                    int rc;
                    if ( (rc = xmlNanoHTTPReturnCode(ctxt)) != 200 )
                    {
                        output << tOutput("$resource_check_filenotfound");
                    }
                    else
                    {
                        output << tOutput("$resource_check_filefound");
                    }
                }
                else output << tOutput("$resource_check_hostfail");
                outputLine << output;
            }
            sg_resourceRepositoryBackups.Reset();           //  reset current key of the list back to 0

            o << outputLine;
            sn_ConsoleOut(outputLine, receiver->Owner());   //  set it to the one calling this function
        }
    }
}
static tConfItemFunc st_resourceRepositoryCheckConf("RESOURCE_REPOSITORY_CHECK", &st_resourceRepositoryCheck);*/

tString resourceErrorLogFile("errors/resource_error.txt");

static bool resourceBackupHTTPFetch(std::ofstream &o, const char *filename, const char *savepath)
{
    void *ctxt = NULL;
    char *buf = NULL;
    FILE* fd;
    int rc;

    if (sg_resourceRepositoryBackups.Size() > 0)
    {
        con << tOutput ("$resource_resorting_backup");
        for(int res = 0; res < sg_resourceRepositoryBackups.Size(); res++)
        {
            tString resourceHost = sg_resourceRepositoryBackups.Get(res);
            con << tOutput("$resource_backup_trying", resourceHost);

            tString strURL;
            strURL << resourceHost << filename;

            const char *newURL = strURL;
            ctxt = xmlNanoHTTPOpen(newURL, NULL);
            if (ctxt)
            {
                if ( (rc = xmlNanoHTTPReturnCode(ctxt)) != 200 )
                {
                    o << tOutput("$resource_backup_file404", resourceHost, filename);
                    con << tOutput("$resource_backup_file404", resourceHost, filename);
                }
                else
                {
                    int maxlength = 10000;
                    buf = (char*)malloc(maxlength);
                    int sLen;
                    int testCounter = 0;
                    tString fileType;
                    fileType << filename;

                    if (fileType.Contains("xml") && fileType.EndsWith(".aamap.xml"))
                    {
                        while( (sLen = xmlNanoHTTPRead(ctxt, buf, maxlength)) > 0 )
                        {
                            //testing to check this is a valid map
                            tString strString;
                            strString << buf;
                            if (strString.Contains(tString("<?xml")))
                                testCounter++;
                            if (strString.Contains(tString("<Resource")))
                                testCounter++;
                            if (strString.Contains(tString("<World")))
                                testCounter++;
                            if (strString.Contains(tString("<Field")))
                                testCounter++;

                            if ((testCounter > 0) && (testCounter == 4))
                                break;
                        }
                    }
                    //  others can download without checking
                    else testCounter = 4;

                    //  close network link
                    xmlNanoHTTPClose(ctxt);

                    //  if those four elements exist
                    if ((testCounter > 0) && (testCounter == 4))
                    {
                        fd = fopen(savepath, "w");
                        if (fd)
                        {
                            //  reconnect for fresh start
                            ctxt = xmlNanoHTTPOpen(newURL, NULL);
                            con << tOutput( "$resource_downloading", newURL );

                            int maxlen = 10000;
                            buf = (char*)malloc(maxlen);
                            int rLen;

                            while( (rLen = xmlNanoHTTPRead(ctxt, buf, maxlen)) > 0 )
                            {
                                Ignore( fwrite(buf, rLen, 1, fd) );
                            }
                            free(buf);

                            xmlNanoHTTPClose(ctxt);
                            fclose(fd);

                            con << "OK\n";
                            return true;
                        }
                        else
                        {
                            xmlNanoHTTPClose(ctxt);
                            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
                            o << tOutput( "$resource_no_write", savepath ) << "\n";
                            con << tOutput( "$resource_no_write", savepath );
                        }
                    }
                }
            }
            else
            {
                o << tOutput("$resource_backup_notconnect", resourceHost);
                con << tOutput("$resource_backup_notconnect", resourceHost);
            }
        }
        sg_resourceRepositoryBackups.Reset();
    }
    return false;
}

static bool resourceHTTPFetch(const char *URI, const char *filename, const char *savepath)
{
    void *ctxt = NULL;
    char *buf = NULL;
    FILE* fd;
    int rc;

    std::ofstream o;
    if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
    {
        ctxt = xmlNanoHTTPOpen(URI, NULL);
        if (ctxt)
        {
            if ( (rc = xmlNanoHTTPReturnCode(ctxt)) != 200 )
            {
                xmlNanoHTTPClose(ctxt);
                o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
                o << tOutput( rc == 404 ? "$resource_fetcherror_404" : "$resource_fetcherror", rc );
                con << tOutput( rc == 404 ? "$resource_fetcherror_404" : "$resource_fetcherror", rc );

                //  try back up resource to try and download file
                if (!resourceBackupHTTPFetch(o, filename, savepath))
                {
                    o << tOutput("$resource_download_failed", filename);
                    con << tOutput("$resource_download_failed", filename);
                }
                else return true;
            }
            else
            {
                int maxlength = 10000;
                buf = (char*)malloc(maxlength);
                int sLen;
                int testCounter = 0;
                tString fileType;
                fileType << filename;

                if (fileType.Contains("xml") && fileType.EndsWith(".aamap.xml"))
                {
                    while( (sLen = xmlNanoHTTPRead(ctxt, buf, maxlength)) > 0 )
                    {
                        //testing to check this is a valid map
                        tString strString;
                        strString << buf;
                        if (strString.Contains(tString("<?xml")))
                            testCounter++;
                        if (strString.Contains(tString("<Resource")))
                            testCounter++;
                        if (strString.Contains(tString("<World")))
                            testCounter++;
                        if (strString.Contains(tString("<Field")))
                            testCounter++;

                        if ((testCounter > 0) && (testCounter == 4))
                            break;
                    }
                }
                //  others can download without checking
                else testCounter = 4;

                //  close network link
                xmlNanoHTTPClose(ctxt);

                //  if those four elements exist
                if ((testCounter > 0) && (testCounter == 4))
                {
                    fd = fopen(savepath, "w");
                    if (fd)
                    {
                        //  reconnect for fresh start
                        ctxt = xmlNanoHTTPOpen(URI, NULL);
                        con << tOutput( "$resource_downloading", URI );

                        int maxlen = 10000;
                        buf = (char*)malloc(maxlen);
                        int rLen;

                        while( (rLen = xmlNanoHTTPRead(ctxt, buf, maxlen)) > 0 )
                        {
                            Ignore( fwrite(buf, rLen, 1, fd) );
                        }
                        free(buf);

                        xmlNanoHTTPClose(ctxt);
                        fclose(fd);

                        con << "OK\n";
                        return true;
                    }
                    else
                    {
                        xmlNanoHTTPClose(ctxt);
                        o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
                        o << tOutput( "$resource_no_write", savepath ) << "\n";
                        con << tOutput( "$resource_no_write", savepath );
                    }
                }
            }
        }
        else
        {
            xmlNanoHTTPClose(ctxt);
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_fetcherror_noconnect", URI );
            con << tOutput( "$resource_fetcherror_noconnect", URI );

                //  try back up resource to try and download file
                if (!resourceBackupHTTPFetch(o, filename, savepath))
                {
                    o << tOutput("$resource_download_failed", filename);
                    con << tOutput("$resource_download_failed", filename);
                }
                else return true;
        }
        o << "===========================================================\n\n";
        return false;
    }
}

static int myHTTPFetch(const char *URI, const char *filename, const char *savepath)
{
    void *ctxt = NULL;
    char *buf = NULL;
    FILE* fd;
    int len, rc;

    //con << tOutput( "$resource_downloading", URI );
    // con << "Downloading " << URI << "...\n";

    ctxt = xmlNanoHTTPOpen(URI, NULL);
    if (ctxt == NULL) {
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_fetcherror_noconnect", URI ) << "\n";
            o << "===========================================================\n\n";
        }
        con << tOutput( "$resource_fetcherror_noconnect", URI );
        return 1;
    }

    if ( (rc = xmlNanoHTTPReturnCode(ctxt)) != 200 )
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( rc == 404 ? "$resource_fetcherror_404" : "$resource_fetcherror", rc ) << "\n";
            o << "===========================================================\n\n";
        }
        con << tOutput( rc == 404 ? "$resource_fetcherror_404" : "$resource_fetcherror", rc );
        return 2;
    }

    fd = fopen(savepath, "w");
    if (fd < 0)
    {
        xmlNanoHTTPClose(ctxt);
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_no_write", savepath ) << "\n";
            o << "===========================================================\n\n";
        }
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
    bool rv = false;
    // r = unprocessed data		p = end-of-item + 1		u = item
    // n = to-be r				len = length of item	savepath = result filepath

    while (r[0] != '\0')
    {
        while (r[0] == ' ') ++r;			// skip spaces at the start of the item

        p = strchr(r, ';');
        if (!p)
            p = strchr(r, '\0');

        n = (p[0] == '\0') ? p : (p + 1);	// next item starts after the semicolon

        // NOTE: skip semicolons, *NOT* nulls
        while (p[-1] == ' ') --p;			// skip spaces at the end of the item

        len = (size_t)(p - r);
        if (len > 0)
        {						// skip this for null-length items
            u = (char*)malloc((len + 1) * sizeof(char));
            strncpy(u, r, len);
            u[len] = '\0';					// u now contains the individual URI

            //rv = myHTTPFetch(u, filename, savepath);	// TODO: handle other protocols?
            rv = resourceHTTPFetch(u, filename, savepath);

            free(u);

            if (rv) return true;		// If successful, return the file retrieved
        }
        r = n;								// move onto the next item
    }

    return rv;	// last error
}

/*
Allows for the fetching and caching of ressources available on the web,
such as maps (xml), texture (jpg, gif, bmp), sound and models.
Nota: On some forums (such as forums.armagetronad.net), it is possible
for the download link not give information about the filename or type,
ie: http://forums.armagetronad.net/download/file.php?id=1191. This is
why the filename parameter is required.
Parameters:
uri: The full uri to obtain the ressource
filename: The filename to use for the local ressource
Return a file handle to the ressource
NOTE: There must be *at least* one directory level, even if it is ./
*/
tString tResourceManager::locateResource(const char *uri, const char *file) {
    tString filepath, a_uri = tString(), savepath;
    bool rv = false;

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
    if (!file || file[0] == '\0')
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_no_filename" ) << "\n";
            o << "===========================================================\n\n";
        }
        con << tOutput( "$resource_no_filename" );
        return (tString) NULL;
    }
    if (file[0] == '/' || file[0] == '\\')
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_abs_path" ) << "\n";
            o << "===========================================================\n\n";
        }
        con << tOutput( "$resource_abs_path" );
        return (tString) NULL;
    }
    savepath = tDirectories::Resource().GetWritePath(file);
    if (savepath == "")
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, resourceErrorLogFile, std::ios::app))
        {
            o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << " | Error: \n";
            o << tOutput( "$resource_abs_path" ) << "\n";
            o << "===========================================================\n\n";
        }
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
    if ( resRepoServer.Len() > 2 )
        a_uri << resRepoServer << file << ';';

    if ( resRepoClient.Len() > 2 && resRepoClient != resRepoServer )
        a_uri << resRepoClient << file << ';';

    con << tOutput( "$resource_not_cached", file );

    rv = myFetch((const char *)a_uri, file, (const char *)savepath);

    if ( NULL != to_free )
        free( to_free );

    if (!rv)
        return (tString) NULL;
    return savepath;
}

FILE* tResourceManager::openResource(const char *uri, const char *file) {
    tString filepath;
    filepath = locateResource(uri, file);
    if ( filepath.Len() <= 1 )
        return NULL;
    return fopen((const char *)filepath, "r");
}

static void RInclude(std::istream& s)
{
    // forbid CASACL
    tCasaclPreventer preventer;

    tString file;
    s >> file;

    tString rclcl = tResourceManager::locateResource(NULL, file);
    if ( rclcl ) {
        std::ifstream rc(rclcl);
        tConfItemBase::LoadAll(rc, false );
        return;
    }

    con << tOutput( "$config_rinclude_not_found", file );
}

static tConfItemFunc s_RInclude("RINCLUDE",  &RInclude);

