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

#ifndef ArmageTron_gCommandLineJumpStart_H
#define ArmageTron_gCommandLineJumpStart_H

#include "tCommandLine.h"
#include "tString.h"
#include "gServerFavorites.h"
#include "nServerInfo.h"
#include "aa_config.h"

// *****************************************************************************
// *
// * gCommandLineJumpStart
// *
// *****************************************************************************
//!
//! Allow a user to connect to an internet game directly from the command line
//!
//! You should register the armagetronad:// protocol with your OS to allow greater functionality.
//!
// *****************************************************************************

class gCommandLineJumpStartAnalyzer : public tCommandLineAnalyzer
{
public:
    bool ShouldConnect() { return _shouldConnect; }
    void Connect();
private:
    bool _shouldConnect;
    tString _raw;
    virtual bool DoAnalyze( tCommandLineParser & parser );
    virtual void DoHelp( std::ostream & s );
};

void ExtractConnectionInformation( tString &raw, tString &servername, tString &port );

#ifdef MACOSX
// nemo TOFIX: I can not include "gGame.h" in AAURLHandler, because one of the file
// it includes conflicts with objective-c, most notabley using +id+ as a variable name
void AAURLHandlerConnect( nServerInfoBase *server );
#endif

#endif // ArmageTron_gCommandLineJumpStart_H
