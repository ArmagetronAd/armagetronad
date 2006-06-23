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
    if( ! isInited ) {
        // Initialize the web server, giving it the path to its config file
        shttpd_init(tDirectories::Config().GetReadPath("settings_web.cfg"));

        // Set configuration parameters
#ifdef DEDICATED
        shttpd_setopt("document_root", tDirectories::Webroot().GetDirPath() );
#endif
        shttpd_register_mountpoint("/resource", tDirectories::Resource().GetDirPath());

        // Setup extra mimetypes for armagetron
        shttpd_addmimetype("xml", "text/xml");

        // Setup cgi handlers
        // path isn't currently implemented in shttpd, but the extension is
        //shttpd_addcgihandler(".py", "/usr/bin/python");

        // Setup callbacks
        // The admin callback for executing a console command on the server
        //shttpd_register_url("/admin/actions/doconsole", &uWebInterface::set_console, NULL);

        // Open port, get socket so we can poll
        mSocket = shttpd_open_port(0);
        isInited = TRUE;
    }
}

void uWebInterface::PollNetwork(unsigned milliseconds) {
    shttpd_poll(mSocket, milliseconds);
}

void uWebInterface::Shutdown() {
    shttpd_fini();
}

int uWebInterface::set_console(struct shttpd_callback_arg *arg)
{
    tString command;
    command << shttpd_get_var(arg->connection, "cmd") << " " <<
    shttpd_get_var(arg->connection, "value");
    std::stringstream s;
    s << command;
    tConfItemBase::LoadAll(s);

    /* Output HTTP headers */
    shttpd_printf(arg->connection, "%s\r\n", "HTTP/1.1 200 OK");
    shttpd_printf(arg->connection, "%s\r\n", "Content-Type: text/plain");
    shttpd_printf(arg->connection, "%s\r\n", "");

    tDelay(2);

    return 0;
}


#endif // DEDICATED

