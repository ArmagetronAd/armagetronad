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

#include "gHud.h"

// These variables don't have any effect anymore, they're just here to get this file and all of the cockpit linked by being used in gGame.cpp.
REAL subby_SpeedGaugeSize=.150, subby_SpeedGaugeLocX=-0.165, subby_SpeedGaugeLocY=-0.9;
REAL subby_BrakeGaugeSize=.150, subby_BrakeGaugeLocX=0.25, subby_BrakeGaugeLocY=-0.9;
REAL subby_RubberGaugeSize=.150, subby_RubberGaugeLocX=-0.55, subby_RubberGaugeLocY=-0.9;
bool subby_SpeedGaugeBar=true, subby_BrakeGaugeBar=true, subby_RubberGaugeBar=true;
bool subby_SpeedGaugeHighIsBetter=false, subby_BrakeGaugeHighIsBetter=true, subby_RubberGaugeHighIsBetter=false;
bool subby_SpeedGaugeReverse=false, subby_BrakeGaugeReverse=false, subby_RubberGaugeReverse=false;
bool subby_ShowHUD=true, subby_ShowSpeedFastest=true, subby_ShowScore=true, subby_ShowAlivePeople=true, subby_ShowPing=true, subby_ShowSpeedMeter=true, subby_ShowBrakeMeter=true, subby_ShowRubberMeter=true;
bool showTime=false;
bool show24hour=false;
REAL subby_ScoreLocX=-0.95, subby_ScoreLocY=-0.80, subby_ScoreSize =.13;
REAL subby_FastestLocX=-0.68, subby_FastestLocY=-0.95, subby_FastestSize =.12;
REAL subby_AlivePeopleLocX=-0.16, subby_AlivePeopleLocY=-0.95, subby_AlivePeopleSize =.13;
REAL subby_PingLocX = 0.30, subby_PingLocY = -0.95, subby_PingSize = 0.13;

#ifndef DEDICATED

#include "cockpit/cCockpit.h"

//needed or it won't link anything in src/tron/cockpit...
//TODO: someone who knows what's wrong find a better solution, really.
void stupid_unnnecessary_function_that_makes_the_linker_happy() {
    cCockpit::GetCockpit()->ProcessCockpit();
}

#endif
