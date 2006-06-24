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

#ifndef ArmageTron_WEBINTERFACE_H
#define ArmageTron_WEBINTERFACE_H

#ifdef DEDICATED

class uWebInterface {

public:
    // Initialize initializes the class and starts the server
    static void Initialize();
    // PollNetwork is the polling method, should be called
    // at least once per game loop and also when the server is idling
    static void PollNetwork(unsigned milliseconds);
    // Shutdown shuts down the web server
    static void Shutdown();

    // Callbacks for the web server
    static int set_console(struct shttpd_callback_arg *arg);
    static int admin_html(struct shttpd_callback_arg *arg);
    static int admin_disabled(struct shttpd_callback_arg *arg);

    static int mSocket;
private:
    static bool isInited;
    uWebInterface();
    ~uWebInterface();
};

#endif // DEDICATED

#endif // ArmageTron_WEBINTERFACE_H





