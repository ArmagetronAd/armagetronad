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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#include "gPingPong.h"
#include "gZone.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
#include "eTeam.h"

#include "gSpawn.h"
#include "gArena.h"

gPingPong::gPingPong()
{
    this->enabled = false;

    this->points = 1;
    this->team_balls = 1;

    this->collapse_kill = false;

    this->bounce_speed = 30;
    this->launch_speed = 30;
}

gPingPong *sg_PingPong = new gPingPong();

static tSettingItem<bool> sg_PingPongEnabled("PINGPONG_ENABLED", sg_PingPong->enabled);
static tSettingItem<int> sg_PingPongPoints("PINGPONG_POINTS", sg_PingPong->points);
static tSettingItem<int> sg_PingPongTeamBalls("PINGPONG_TEAM_BALLS", sg_PingPong->team_balls);
static tSettingItem<bool> sg_PingPongDeathKill("PINGPONG_DEATH_KILL", sg_PingPong->collapse_kill);
static tSettingItem<REAL> sg_PingPongBounceSpeed("PINGPONG_BOUNCE_SPEED", sg_PingPong->bounce_speed);
