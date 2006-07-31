/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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

#ifndef ArmageTron_SPAM_PROTECTION_H
#define ArmageTron_SPAM_PROTECTION_H

#include "defs.h"
#include "tLocale.h"
#include "nNetwork.h"
#include "tConfiguration.h"

// spam protection settings
class nSpamProtectionSettings
{
public:
    REAL timeScale_;	//!< timescale of the protection
    tOutput silence_;	//!< message to send when someone is silenced
    tSettingItem<REAL> timeScaleSetting_; //!< setting item for timeScale_

    nSpamProtectionSettings( REAL timeScale, char const * timeScaleConfig , const tOutput& silence );
};

// spam protection
class nSpamProtection
{
public:
    enum Level					// enum describing the spam level
    {
        Level_Ok,				// no spam
        Level_Mild,				// some level of spam
        Level_Hard				// extremly annoying
    };

    Level	CheckSpam( REAL spamlevel, int UserToKick, tOutput const & message );	// check if someone is spamming
    REAL	BlockTime();									                        // time left in silenced mode

    nSpamProtection( const nSpamProtectionSettings& settings );
    ~nSpamProtection();

private:
    const nSpamProtectionSettings&	settings_;
    REAL							spamProtect_;
    nTimeRolling					spamProtectTime_;
};

#endif // !defined(ArmageTron_SPAM_PROTECTION_H)
