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

#ifndef EWARMUP_H_EAIQX02N
#define EWARMUP_H_EAIQX02N

class eWarmup
{
public:
    eWarmup();

    bool IsWarmupMode() const;

    // Was the game initiated from warmup mode?
    bool IsPickupGame() const;

    void DoWarmup( int matchesToPlay );
    void StartNewMatchUserInitiated();
    void MatchStarted();
    void Reset();

    int MatchesLeft() const
    {
        return matchesLeft_;
    }

    void SetMatchesLeft( int newMatches )
    {
        matchesLeft_ = newMatches;
    }
    
    int MatchesToPlay() const
    {
        return matchesToPlay_;
    }    
private:
    int matchesToPlay_;
    int matchesLeft_;
};

extern eWarmup se_warmup;

#endif /* end of include guard: EWARMUP_H_EAIQX02N */
