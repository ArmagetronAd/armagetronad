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

#include "rSDL.h"

#include "eFloor.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
#include "gParser.h"
#include "eTeam.h"
#include "ePlayer.h"
#include "rRender.h"
#include "nConfig.h"
#include "tString.h"
#include "tPolynomial.h"
#include "rScreen.h"
#include "eSoundMixer.h"

#include "zone/zFlag.h"
#include "zone/zFortress.h"
#include "zone/zZone.h"

#include <time.h>
#include <algorithm>
#include <functional>
#include <deque>


//#define sg_segments sz_zoneSegments


// *******************************************************************************
// *
// *	zFlagZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!
// *******************************************************************************

zFlagZone::zFlagZone( eGrid * grid )
        :zZone( grid )
{
    init_ = false;
    owner_ = NULL;
    ownerTime_ = 0;
	flagHome_ = true;
    chatBlinkUpdateTime_ = 0;
    blinkUpdateTime_ = 0;
    blinkTrackUpdateTime_ = 0;
    ownerDropped_ = NULL;
    ownerDroppedTime_ = 0;
    lastHoldScoreTime_ = -1;
    positionUpdatePending_ = false;

}

// *******************************************************************************
// *
// *	~zFlagZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

zFlagZone::~zFlagZone( void )
{
}


static float sg_flagBlinkTime = -1;
static tSettingItem<float> sg_flagBlinkTimeConfig( "FLAG_BLINK_TIME", sg_flagBlinkTime );

static float sg_flagBlinkStart = 0.1;
static tSettingItem<float> sg_flagBlinkStartConfig( "FLAG_BLINK_START", sg_flagBlinkStart );

static float sg_flagBlinkEnd = 1.0;
static tSettingItem<float> sg_flagBlinkEndConfig( "FLAG_BLINK_END", sg_flagBlinkEnd );

static float sg_flagBlinkOnTime = -1;
static tSettingItem<float> sg_flagBlinkOnTimeConfig( "FLAG_BLINK_ON_TIME", sg_flagBlinkOnTime );

static float sg_flagBlinkEstimatePosition = 0.5;
static tSettingItem<float> sg_flagBlinkEstimatePositionConfig( "FLAG_BLINK_ESTIMATE_POSITION", sg_flagBlinkEstimatePosition );

static float sg_flagBlinkTrackTime = -1;
static tSettingItem<float> sg_flagBlinkTrackTimeConfig( "FLAG_BLINK_TRACK_TIME", sg_flagBlinkTrackTime );

static float sg_flagHoldTime = -1;
static tSettingItem<float> sg_flagHoldTimeConfig( "FLAG_HOLD_TIME", sg_flagHoldTime );

static float sg_flagDropTime = 3;
static tSettingItem<float> sg_flagDropTimeConfig( "FLAG_DROP_TIME", sg_flagDropTime );

static bool sg_flagTeam = true;
static tSettingItem<bool> sg_flagTeamConfig( "FLAG_TEAM", sg_flagTeam );

static float sg_flagHoldScoreTime = -1;
static tSettingItem<float> sg_flagHoldScoreTimeConfig( "FLAG_HOLD_SCORE_TIME", sg_flagHoldScoreTime );

static int sg_flagHoldScore = 1;
static tSettingItem<int> sg_flagHoldScoreConfig( "FLAG_HOLD_SCORE", sg_flagHoldScore );


void zFlagZone::setupVisuals(gParser::State_t & state)
{
    REAL tpR[] = {.0f, .3f};
    tPolynomial tpr(tpR, 2);
    state.set("rotation", tpr);
}

void zFlagZone::readXML(tXmlParser::node const & node)
{
}

// *******************************************************************************
// *
// *	FlagGoHome
// *
// *******************************************************************************

void zFlagZone::GoHome()
{
	owner_ = NULL;
	flagHome_ = true;
	shape->Position() = homePosition_;
}


// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool zFlagZone::Timestep( REAL time )
{
    if (!init_)
		 {
         homePosition_ = shape->Position();
			init_ = true;
			 
			 const tList<eGameObject>& gameObjects = Grid()->GameObjects();
			 gCycle * closest = NULL;
			 REAL closestDistance = 0;
			 for (int i=gameObjects.Len()-1;i>=0;i--)
			 {
				 gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));
				 
				 if (other )
				 {
					 // eTeam * otherTeam = other->Player()->CurrentTeam();
					 eCoord otherpos = other->Position() - Position();
					 REAL distance = otherpos.NormSquared();
					 if ( !closest || distance < closestDistance )
					 {
						 closest = other;
						 closestDistance = distance;
					 }
				 }
			 
			initOwnerTeam_ = closest->Player()->CurrentTeam();
		 }
		 }
    return zZone::Timestep(time);
}

// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFlagZone::OnVanish( void )
{

}



// *******************************************************************************
// *
// *	CheckSurvivor
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFlagZone::CheckSurvivor( void )
{

}



// *******************************************************************************
// *
// *   OnRoundBegin
// *
// *******************************************************************************
//!
//! @return shall the hole process be repeated?
//!
// *******************************************************************************

void zFlagZone::OnRoundBegin( void )
{

}

// *******************************************************************************
// *
// *   OnRoundEnd
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFlagZone::OnRoundEnd( void )
{

}

// *******************************************************************************
// *
// *	OnInside
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void zFlagZone::OnInside( gCycle * target, REAL time )
{
	if(sg_flagTeam)
	{
	//CTF
    // make sure target, player, and their team are OK
    if ((!target) ||
        (!target->Player()) ||
        (!target->Player()->CurrentTeam()))
    {
        return;
    }

    // don't process if owned or updating
    if ((owner_) || (positionUpdatePending_))
    {
        return;
    }
    
	eTeam * TheTeam = target->Player()->CurrentTeam();
		
	if(TheTeam == initOwnerTeam_)
	{
		//Return the Flag
		GoHome();
		tColoredString playerName;
		playerName << *target->Player() << tColoredString::ColorString(1,1,1);
		sn_ConsoleOut( tOutput( "$player_flag_return", playerName ) );
		return;
	}
		
	//check to see if player already has a flag
	bool playerHasFlag = false;
	const tList<eGameObject>& gameObjects = Grid()->GameObjects();
	for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			zFlagZone* otherFlag=dynamic_cast<zFlagZone *>(gameObjects(i));
			if ((otherFlag)){
				if (otherFlag->initOwnerTeam_ == target->Player()->CurrentTeam()){
					playerHasFlag = true;
					return;
				}
			}
		}
		
		//take the flag
		owner_ = target;
	    flagHome_ = false;
		
		tColoredString playerName;
		playerName << *target->Player() << tColoredString::ColorString(1,1,1);
		sn_ConsoleOut( tOutput( "$player_flag_take", playerName ) );
		//TODO - kill the flag at its home
	}
	else
	{
	//HTF
		
	}
}
		
static zZoneExtRegistration regFlag("flag", "", zFlagZone::create);
