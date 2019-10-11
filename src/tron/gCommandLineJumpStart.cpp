/*

 *************************************************************************

 ArmageTron -- Just another Tron Lightcycle Game in 3D.
 Copyright (C) 2005  by Daniel Harple
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

#include "gGame.h"
#include "gCommandLineJumpStart.h"
#include "gLogo.h"

// *****************************************************************************
// *
// * gCommandLineJumpStartAnalyzer
// *
// *****************************************************************************
// *****************************************************************************

bool gCommandLineJumpStartAnalyzer::DoAnalyze( tCommandLineParser & parser )
{
    if ( parser.GetOption( _raw, "--connect" ) )
    {
        _shouldConnect = true;

        return true;
    }
    else
    {
        _shouldConnect = false;
    }

    return false;
}

void gCommandLineJumpStartAnalyzer::DoHelp( std::ostream & s )
{
    s << "--connect <server>[:<port>]  : connect directly to SERVER, skipping all menus. default PORT=4534\n";
}

void gCommandLineJumpStartAnalyzer::Connect()
{
    tString server;
    tString port;

    ExtractConnectionInformation( _raw, server, port );

    nServerInfoRedirect connectTo( server, port.ToInt() );

    ConnectToServer( &connectTo );
}

// *****************************************************************************
// *
// * ExtractConnectionInformation()
// *
// * Accepted formats
// *    - armagetronad://<server>[:<port>]
// *    - <server>[:<port>]
// *
// *****************************************************************************
//!
//! @raw            raw commandline string
//! @servername     store extracted servername here
//! @port           store port here
//!
// *****************************************************************************
void ExtractConnectionInformation( tString &raw, tString &servername, tString &port )
{
    // BUG: case sensitive
    if ( raw.StartsWith("armagetronad://") )
    {
        raw.RemoveSubStr(0, 15);
    }

    // Windows automatically adds a trailing / and we have no use for it
    if ( raw.EndsWith("/") )
    {
        raw.RemoveSubStr(-1, 1);
    }

    int portStart = strcspn( raw, ":" );
    // Is a port given?
    if ( portStart != raw.Len() - 1 )
    {
        servername = raw.SubStr( 0, portStart );
        port       = raw.SubStr( portStart + 1 );
    }
    else
    {
        servername = raw;
        port       = "4534";
    }
}

#ifdef MACOSX
void AAURLHandlerConnect( nServerInfoBase * server )
{
    gLogo::SetDisplayed(false);
    ConnectToServer( server );
}
#endif


