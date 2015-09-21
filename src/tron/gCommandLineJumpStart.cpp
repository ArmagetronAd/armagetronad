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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
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
    tString raw;
    tString server;
    tString port;

    if ( parser.GetOption( raw, "--connect" ) )
    {
        ExtractConnectionInformation( raw, server, port );
        _server.SetPort( port.ToInt() );
        _server.SetConnectionName( server );
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
    ConnectToServer( &_server );
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


