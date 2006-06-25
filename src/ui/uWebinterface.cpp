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

uWebInterface by Dave Fancella

*/

#include "config.h"

#ifdef DEDICATED

#include "uWebinterface.h"
#include "tSysTime.h"
#ifdef _WIN32
#include <winsock.h>
#ifndef __MINGW32__
#define snprintf            _snprintf
#endif
// #pragma comment(lib,"ws2_32")
#else
#include <sys/types.h>
#include <sys/select.h>
#endif

// Needed so the shttpd header will know we're using multi-threaded
#define MT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "shttpd.h"
#include <sstream>

#define TRUE 1
#define FALSE 0

#include "tConfiguration.h"
#include "tString.h"
#include "tDirectories.h"

int allowRemoteAdmin=0;
static tSettingItem<int> wira("WEB_REMOTE_ADMIN",allowRemoteAdmin);

int useWebInterface=0;
static tSettingItem<int> wiwa("WEB_USE_INTERNAL",useWebInterface);

int webPort=4550;
static tSettingItem<int> wip("WEB_PORT",webPort);

tString webDNSname("mydomain.com");
static tSettingItem<tString> widns("WEB_SERVER_DNS", webDNSname);

tString webIndexFiles("index.html,index.cgi");
static tConfItemLine wiif("WEB_INDEX_FILES", webIndexFiles);

extern tString sn_serverName;

int uWebInterface::mSocket;
bool uWebInterface::isInited = FALSE;
#if 0
// Reference callback from MT example
static int
index_html(struct shttpd_callback_arg *arg)
{
    int i;

    /* Output HTTP headers */
    shttpd_printf(arg->connection, "%s\r\n", "HTTP/1.1 200 OK");
    shttpd_printf(arg->connection, "%s\r\n", "Content-Type: text/plain");
    shttpd_printf(arg->connection, "%s\r\n", "");

    sleep(2);

    /* Output large chunk of binary data */
    for (i = 0; i < 100000; i++)
        shttpd_printf(arg->connection, "%s\n", "ABCDEFGHIJKLMNOP");

    return (0); /* Not used */
}
#endif

uWebInterface::uWebInterface() {
}

void uWebInterface::Initialize() {
    if( ! isInited && useWebInterface == 1 ) {
        // Initialize the web server, giving it the path to its config file
        //shttpd_init(tDirectories::Config().GetReadPath("settings_web.cfg"));
        shttpd_init("");

        // Set configuration parameters
#ifdef DEDICATED
        std::cout << tDirectories::Webroot().GetDirPath() << std::endl;
        shttpd_setopt("document_root", tDirectories::Webroot().GetDirPath() );
        shttpd_setopt("server_name", webDNSname );
        shttpd_setopt("index_files", webIndexFiles );
        shttpd_setopt("debug", "0" );
        shttpd_setopt("list_directories", "1" );
        // access and error log name changes don't work for some reason
        shttpd_setopt("access_log", "armagetronad-web.log" );
        shttpd_setopt("error_log", "armagetronad-web-error.log" );
#endif
        shttpd_register_mountpoint("/resource", tDirectories::Resource().GetDirPath());

        // Setup extra mimetypes for armagetron
        shttpd_addmimetype("xml", "text/xml");

        // Setup cgi handlers
        // path isn't currently implemented in shttpd, but the extension is
        //shttpd_addcgihandler(".py", "/usr/bin/python");

        // Setup callbacks
        // The front of the admin interface
        shttpd_register_url("/admin", &uWebInterface::admin_html, NULL);
        // The admin callback for executing a console command on the server
        shttpd_register_url("/actions/doconsole", &uWebInterface::set_console, NULL);
        // First protect the admin directory
        shttpd_protect_url("/admin", tDirectories::Config().GetReadPath("web_password.cfg") );
        shttpd_protect_url("/actions", tDirectories::Config().GetReadPath("web_password.cfg") );

        // Open port, get socket so we can poll
        mSocket = shttpd_open_port(webPort);
        isInited = TRUE;
    }
}

void uWebInterface::PollNetwork(unsigned milliseconds) {
    if( isInited && useWebInterface == 1 )
        shttpd_poll(mSocket, milliseconds);
}

void uWebInterface::Shutdown() {
    if( isInited  )
        shttpd_fini();
}

int uWebInterface::admin_disabled(struct shttpd_callback_arg *arg)
{
    shttpd_printf(arg->connection, GetHeaders(), "" );
    shttpd_printf(arg->connection, "%s\r\n", "");

    shttpd_printf(arg->connection, "%s\r\n", "<html><head><title>Armagetron Advanced Server Administration</title></head>");
    shttpd_printf(arg->connection, "%s\r\n", "<body><h1>Administration of this server through the web interface is disabled!</h1></body></html>");

    return 0;
}

tString uWebInterface::GetHeaders() {
    tString returnHeaders;

    returnHeaders << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
    return returnHeaders;
}

bool uWebInterface::AllowAdmin() {
    if( allowRemoteAdmin == 1 ) {
        return true;
    } else {
        return false;
    }
}

int uWebInterface::admin_html(struct shttpd_callback_arg *arg)
{
    if( AllowAdmin() ) {
        shttpd_printf(arg->connection, GetHeaders(), "");
        shttpd_printf(arg->connection, "\r\n", "");
        shttpd_printf(arg->connection, GetPageHeader(sn_serverName) );
        tString body;

        body << "<h1>Armagetron Advanced Server Administration</h1>\n";
        body << "<form method=\"get\" action=\"/actions/doconsole\">\n";
        body << "<div class=\"mainContent\" />\n";
        body << "<p>Command: <input type=\"text\" class=\"consoleName\" name=\"cmd\" />";
        body << " &nbsp; Value: <input type=\"text\" class=\"consoleValue\" name=\"value\" />";
        body << "</p>\n";
        body << "<p><input type=\"submit\" /></p>\n";
        body << "</div>\n";
        body << "</form>\n";
        shttpd_printf(arg->connection, body);

        shttpd_printf(arg->connection, GetPageFooter() );
    } else {
        return admin_disabled(arg);
    }

    return 0;
}

int uWebInterface::set_console(struct shttpd_callback_arg *arg)
{
    if( AllowAdmin() ) {
        tString command;
        command << shttpd_get_var(arg->connection, "cmd") << " " <<
        shttpd_get_var(arg->connection, "value");
        std::stringstream s;
        s << command;
        std::cout << command << std::endl;
        tConfItemBase::LoadAll(s);

        /* Output HTTP headers */
        shttpd_printf(arg->connection, "HTTP/1.1 200 OK\r\n" );
        shttpd_printf(arg->connection, "%s: %s", "Location", "/admin");
        shttpd_printf(arg->connection, "\r\n\r\n");
        tDelay(2);
    } else {
        admin_disabled(arg);
    }

    return 0;
}

tString uWebInterface::GetPageHeader(const char* title) {
    tString header;

    header << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";
    header << "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\" dir=\"ltr\">\n";
    header << "\n<head>\n";
    header << "<link rel=\"shortcut icon\" href=\"/favicon.ico\" />\n";
    header << "<title>" << title << " Administration</title>\n";
    header << "<link href=\"/css/admin.css\" type=\"text/css\" />\n";
    header << "</head>\n<body>\n";
    header << "<p><a href=\"/admin\">Administration Home</a></p>\n";

    return header;
}

tString uWebInterface::GetPageFooter() {
    tString footer;

    footer << "</body>\n</html>\n";

    return footer;
}

#endif // DEDICATED

