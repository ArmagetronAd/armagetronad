/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005 by the AA DevTeam (see the file AUTHORS(.txt)
in the main source directory)

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

#include "eWarmup.h"
#include "tConfiguration.h"
#include "tLocale.h"

eWarmup se_warmup;

eWarmup::eWarmup()
    :matchesToPlay_( 0 ), matchesLeft_( 0 ), wasReset_( false )
{
}

bool eWarmup::IsWarmupMode() const
{
    return matchesLeft_ < 0;
}

bool eWarmup::IsPickupGame() const
{
    return matchesToPlay_ > 0;
}

void eWarmup::DoWarmup( int matchesToPlay )
{
    // clamp to acceptable values
    if ( matchesToPlay < 0 )
        matchesToPlay = 0;

    if ( matchesToPlay > 0 )
    {
        con << tOutput( "$do_warmup_enabled", matchesToPlay ) << '\n';
        matchesLeft_ = -1;
    }
    else
    {
        con << tOutput( "$do_warmup_disabled" ) << '\n';
        matchesLeft_ = 0;
    }
    matchesToPlay_ = matchesToPlay;
}

void eWarmup::StartNewMatchUserInitiated()
{
    // Restarting the match, don't make everyone /ready up again.
    if ( IsPickupGame() )
    {
        matchesLeft_++;
    }
}

void eWarmup::MatchStarted()
{
    if ( IsWarmupMode() )
    {
        matchesLeft_ = matchesToPlay_ - 1;
    }
    else if ( IsPickupGame() )
    {

        matchesLeft_--;
    }
}

void eWarmup::MatchCanStart()
{
    wasReset_ = false;
}

void eWarmup::Reset()
{
    if ( IsPickupGame() )
    {
        matchesLeft_ = -1;
        wasReset_ = true;
    }
}
