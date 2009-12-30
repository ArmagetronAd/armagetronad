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

#include "gWinZone.h"
#include "eFloor.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
#include "eTeam.h"
#include "ePlayer.h"
#include "rRender.h"
#include "nConfig.h"
#include "tString.h"
#include "rScreen.h"

#include "eAdvWall.h"
#include "eGameObject.h"
#include "uInputQueue.h"
#include "eTess2.h"
#include "eWall.h"
#include "gWall.h"
#include "gSvgOutput.h"
#include "eSound.h"
#include "uInput.h"
#include "tMath.h"
#include "nConfig.h"
#include "gSpawn.h"
#include "gArena.h"
#include "tRandom.h"

#include <time.h>
#include <algorithm>

static int sg_zoneAlphaToggle = 0;
static tSettingItem<int> sg_zoneAlphaToggleConf( "ZONE_ALPHA_TOGGLE", sg_zoneAlphaToggle );

static int sg_zoneDeath = 1;
static tSettingItem<int> sg_zoneDeathConf( "WIN_ZONE_DEATHS", sg_zoneDeath );

static REAL sg_expansionSpeed = 1.0f;
static REAL sg_initialSize = 5.0f;

static nSettingItem< REAL > sg_expansionSpeedConf( "WIN_ZONE_EXPANSION", sg_expansionSpeed );
static nSettingItem< REAL > sg_initialSizeConf( "WIN_ZONE_INITIAL_SIZE", sg_initialSize );

static int sg_ballsInteract = 0;
static tSettingItem<int> sg_ballsInteractConf( "BALLS_INTERACT", sg_ballsInteract );

static int sg_cycleWallsInteract = 0;
static tSettingItem<int> sg_cycleWallsInteractConf( "BALLS_BOUNCE_ON_CYCLE_WALLS", sg_cycleWallsInteract );

static int sg_ballTeamMode = 0;	 //0=ball score other team, 1=ball score only team owner ...
static tSettingItem<int> sg_ballTeamModeConfig( "BALL_TEAM_MODE", sg_ballTeamMode );

static int sg_ballKiller = 0;	 //1=ball kill other team players ...
static tSettingItem<int> sg_ballKillerConfig( "BALL_KILLS", sg_ballKiller );

static REAL sg_ballSpeedDecay = 0;
static tSettingItem<REAL> sg_ballSpeedDecayConf( "BALL_SPEED_DECAY", sg_ballSpeedDecay );

static REAL sg_ballCycleBoost = 0;
static tSettingItem<REAL> sg_ballCycleBoostConf( "BALL_CYCLE_ACCEL_BOOST", sg_ballCycleBoost );

static int sg_ballAutoRespawn = 1;
static tSettingItem<int> sg_ballAutoRespawnConf( "BALL_AUTORESPAWN", sg_ballAutoRespawn );

static int sg_zoneSegments = 11;
static tSettingItem<int> sg_zoneSegmentsConf( "ZONE_SEGMENTS", sg_zoneSegments );

static REAL sg_zoneSegLength = .5;
static tSettingItem<REAL> sg_zoneSegLengthConf( "ZONE_SEG_LENGTH", sg_zoneSegLength );

static REAL sg_zoneBottom = 0.0f;
static tSettingItem<REAL> sg_zoneBottomConf( "ZONE_BOTTOM", sg_zoneBottom );

static REAL sg_zoneHeight = 5.0f;
static tSettingItem<REAL> sg_zoneHeightConf( "ZONE_HEIGHT", sg_zoneHeight );

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos )
{
	gZone * ret = NULL;
	if ( sg_zoneDeath )
	{
		ret = tNEW( gDeathZoneHack( grid, pos ) );
		sn_ConsoleOut( "$instant_death_activated" );
	}
	else
	{
		ret = tNEW( gWinZoneHack( grid, pos ) );
		if ( sg_currentSettings->gameType != gFREESTYLE )
		{
			sn_ConsoleOut( "$instant_win_activated" );
		}
		else
		{
			sn_ConsoleOut( "$instant_round_end_activated" );
		}
	}

	// initialize radius and expansion speed
	static_cast<eGameObject*>(ret)->Timestep( se_GameTime() );
	ret->SetReferenceTime();
	ret->SetRadius( sg_initialSize );
	ret->SetExpansionSpeed( sg_expansionSpeed );
	ret->SetRotationSpeed( .3f );

	return ret;
}


// number of segments to render a zone with
static const int sg_segments = 11;

// *******************************************************************************
// *
// *   EvaluateFunctionNow
// *
// *******************************************************************************
//!
//!        @param  f   function to evaluate
//!        @return     the function's value at lastTime - referenceTime_
//!
// *******************************************************************************

inline REAL gZone::EvaluateFunctionNow( tFunction const & f ) const
{
	return f( lastTime - referenceTime_ );
}


// *******************************************************************************
// *
// *   SetFunctionNow
// *
// *******************************************************************************
//!
//!        @param  f      function to modify
//!        @param  value  value the function should have at lastTime - referenceTime_
//!
// *******************************************************************************

inline void gZone::SetFunctionNow( tFunction & f, REAL value ) const
{
	f.SetOffset( value + f.GetSlope() * ( referenceTime_ - lastTime ) );
}


// *******************************************************************************
// *
// *   FindFirst / FindNext
// *
// *******************************************************************************
//!
//!        @param  name      name of the zone or group of zones
//!        @param  prev_pos  value of a previous result
//!
// *******************************************************************************

int gZone::FindFirst(tString name)
{
	eGrid *grid = eGrid::CurrentGrid();
	const tList<eGameObject>& gameObjects = grid->GameObjects();
	for (int j=gameObjects.Len()-1;j>=0;j--)
	{
		gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
		// for all active base zone ...
		if ( zone )
		{
			if (zone->name_ == name)
			{
				return j;
			}
		}
	}
	// if no zone has been found ...
	return -1;
}


int gZone::FindNext(tString name, int prev_pos)
{
	eGrid *grid = eGrid::CurrentGrid();
	const tList<eGameObject>& gameObjects = grid->GameObjects();
	for (int j=prev_pos-1;j>=0;j--)
	{
		gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
		// for all active base zone ...
		if ( zone )
		{
			if (zone->name_ == name)
			{
				return j;
			}
		}
	}
	// if no zone has been found ...
	return -1;
}


// *******************************************************************************
// *
// *	gZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gZone::gZone( eGrid * grid, const eCoord & pos, bool dynamicCreation )
:eNetGameObject( grid, pos, eCoord( 0,0 ), NULL, true ), rotation_(1,0), lastCoord_(0), nextUpdate_(-1)
{
	// store creation time
	referenceTime_ = createTime_ = lastTime = 0;

	destroyed_ = false;
	dynamicCreation_ = dynamicCreation;
	wallInteract_ = false;
	wallBouncesLeft_ = 0;
	targetRadius_ = 0;
    lastImpactTime_ = 0;
	resizeRequested_ = false;
	fallSpeed_ = 0;
	lastSeekTime_ = 0;
	pOwner_ = NULL;
	pSeekingCycle_ = NULL;
	seeking_ = false;			 //??? change this to a game object, allow seeking any
	name_ = tString("");

	color_.r = color_.g = color_.b = 1.0f;

	//??? Look at doing this in the shot or wherever it is created...
	//??? or changing dynamic creation to disableAlpha or something
	if (dynamicCreation)
	{
		referenceTime_ = createTime_ = lastTime = se_mainGameTimer->Time();

		//Hack to get rid of the alpha to make zones non-transparent on creation
		//Negative creation times seem OK with current code
		createTime_ -= 3.5;
	}

	// add to game grid
	this->AddToList();

	// initialize position functions
	SetPosition( pos );

	//wrtl: Ok, this is the result of three hours of debugging, I hope it helps...
	eGameObject::pos = pos;
}


// *******************************************************************************
// *
// *	gZone
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!
// *******************************************************************************

gZone::gZone( nMessage & m )
:eNetGameObject( m ), rotation_(1,0)
{
	destroyed_ = false;
	dynamicCreation_ = false;
	wallInteract_ = false;
	wallBouncesLeft_ = 0;
    lastImpactTime_ = 0;
	targetRadius_ = 0;
	resizeRequested_ = false;
	fallSpeed_ = 0;
	lastSeekTime_ = 0;
	pOwner_ = NULL;
	pSeekingCycle_ = NULL;
	seeking_ = false;
	name_ = tString("");

	// read creation time
	m >> createTime_;
	referenceTime_ = lastTime = createTime_;

	// initialize color to white, ReadSync will fill in the true value if available
	color_.r = color_.g = color_.b = 1.0f;

	// add to game grid
	this->AddToList();

	// initialize position functions
	SetPosition( pos );
}


// *******************************************************************************
// *
// *	~gZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gZone::~gZone( void )
{
}


// *******************************************************************************
// *
// *	WriteCreate
// *
// *******************************************************************************
//!
//!		@param	m Message to write creation data to
//!
// *******************************************************************************

void gZone::WriteCreate( nMessage & m )
{
	// delegate
	eNetGameObject::WriteCreate( m );

	m << createTime_;
}


// *******************************************************************************
// *
// *	WriteSync
// *
// *******************************************************************************
//!
//!		@param	m Message to write sync data to
//!
// *******************************************************************************

void gZone::WriteSync( nMessage & m )
{
	// delegate
	eNetGameObject::WriteSync( m );

	// write color
	m << color_.r;
	m << color_.g;
	m << color_.b;

	// write reference time and functions
	m << referenceTime_;
	m << posx_;
	m << posy_;
	m << radius_;

	// write rotation speed
	m << rotationSpeed_;
}


// *******************************************************************************
// *
// *	ReadSync
// *
// *******************************************************************************
//!
//!		@param	m Message to read sync data from
//!
// *******************************************************************************

void gZone::ReadSync( nMessage & m )
{
	// delegage
	eNetGameObject::ReadSync( m );

	// read color
	if (!m.End())
	{
		m >> color_.r;
		m >> color_.g;
		m >> color_.b;
		se_MakeColorValid(color_.r, color_.g, color_.b, 1.0f);
	}

	// read reference time and functions
	if (!m.End())
	{
		m >> referenceTime_;
		m >> posx_;
		m >> posy_;
		m >> radius_;
	}
	else
	{
		referenceTime_ = createTime_;

		// take old default values
		this->radius_.SetOffset( sg_initialSize );
		this->radius_.SetSlope( sg_expansionSpeed );
		SetPosition( pos );
		SetVelocity( eCoord() );
	}

	// read rotation speed
	if (!m.End())
	{
		m >> rotationSpeed_;
	}
	else
	{
		// set fixed values
		SetRotationSpeed( .3f );
		SetRotationAcceleration( 0.0f );
	}
}


static float sg_shotSeekUpdateTime = 0.5;
static tSettingItem<float> conf_shotSeekUpdateTime ("SHOT_SEEK_UPDATE_TIME", sg_shotSeekUpdateTime);

static float sg_zombieZoneSpeed = 10;
static tSettingItem<float> conf_zombieZoneSpeed ("ZOMBIE_ZONE_SPEED", sg_zombieZoneSpeed);

static bool s_zoneWallInteractionFound;
static eCoord s_zoneWallInteractionCoord;
static eCoord s_zoneWallInteractionClosestCoord;
static REAL   s_zoneWallInteractionRadius;
static REAL   s_zoneWallInteractionImpactTime;
static eCoord s_zoneWallInteractionZoneVelocity;
static eCoord s_zoneWallInteractionImpactCoord;
static REAL   s_zoneWallInteractionZoneDeltaTime;

static void S_ZoneWallInteraction(eWall *pWall)
{
	//Ignore player walls for now
	gPlayerWall *pPlayerWall = dynamic_cast<gPlayerWall*>(pWall);
	if (pPlayerWall && !sg_cycleWallsInteract)
	{
		return;
	}

	// determine the coord (I) and time (t) of the impact
	eCoord V = s_zoneWallInteractionZoneVelocity;
	REAL R = s_zoneWallInteractionRadius;

	eCoord XY = pWall->EndPoint(1) - pWall->EndPoint(0);
	REAL norm_XY = XY.Norm();
	XY *= (1/norm_XY);
	eCoord XP = s_zoneWallInteractionCoord - pWall->EndPoint(0);

	REAL Vn = XY*V;
	if (Vn==0) return;			 // if zone is moving along the wall, no impact ...
	REAL d = XY*XP;

	REAL t1 = (R-d)/Vn;
	REAL t2 = (-R-d)/Vn;
	REAL t = 0;
	if ((t1>0) && (t2>0)) return;// can't be, again ...
	if (t1>0) t = t2;
	else if (t2>0) t = t1;
	else if (t1>t2) t = t2;
	else t = t1;

								 // coordinate of the impact on the wall's line space
	REAL i = eCoord::F(XY,XP+V*t);
	eCoord I;					 // position of the impact
	if (i>=0 && i<=norm_XY)		 // there's an impact on the wall
	{
		I = pWall->EndPoint(0) + XY * i;
	}
	else						 // the zone might hit a wall end point, let's check this
	{
		// select the wall end according to wall's line intersection coord (i)
		eCoord P;
		if (i<0) P = pWall->EndPoint(0);
		else P = pWall->EndPoint(1);

		// get the time of the impact ...
		eCoord dp = s_zoneWallInteractionCoord - P;
		eCoord dv = s_zoneWallInteractionZoneVelocity;
		REAL a = dv.NormSquared();
		if (a<EPS) return;		 // if ball has no speed, just left ...
		REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
		REAL c = dp.NormSquared()-s_zoneWallInteractionRadius*s_zoneWallInteractionRadius;
		REAL delta = b*b-4*a*c;
		if (delta<0) return;	 // no t. it can't be, an impact just occured ...
		else					 // delta is positive, it means we have 2 different solutions
		{
			// the t we are looking for is negative and as close as possible to 0 = it just happens ;)
			delta = sqrt(delta);
			t1 = (-b-delta)/(2*a);
			t2 = (-b+delta)/(2*a);
			t = 0;
								 // can't be, again ...
			if ((t1>0) && (t2>0)) return;
			if (t1>0) t = t2;
			else if (t2>0) t = t1;
			else if (t1>t2) t = t2;
			else t = t1;
			I = P;
		}
	}

	// t must be negative and close to 0 to make sence ...
	if (t>0 || t<-s_zoneWallInteractionZoneDeltaTime) return;

	// at this point, we have the intersection (I), the time of intersection (t)
	// let's check if it is the first impact ...
	if (!s_zoneWallInteractionFound || (t<s_zoneWallInteractionImpactTime))
	{
		s_zoneWallInteractionImpactTime = t;
		s_zoneWallInteractionImpactCoord = I;
		s_zoneWallInteractionFound = true;
	}
}


static void S_ZoneWallIntersect(eWall *pWall)
{
	//Ignore player walls for now
	gPlayerWall *pPlayerWall = dynamic_cast<gPlayerWall*>(pWall);
	if (pPlayerWall)
	{
		return;
	}

	// determine the point closest to s_zoneWallInteractionCoord
	eCoord normal = pWall->Vec().Conj();
	normal.Normalize();

	eCoord Pos1 = normal.Turn( pWall->EndPoint(0) - s_zoneWallInteractionCoord );
	eCoord Pos2 = normal.Turn( pWall->EndPoint(1) - s_zoneWallInteractionCoord );

	tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y + Pos2.y + .1f ) * .1f );

	REAL alpha = .5f;
	if ( Pos1.x != Pos2.x)
		alpha = Pos1.x / ( Pos1.x - Pos2.x );

	eCoord closestCoord = pWall->Point(alpha);

	if (!s_zoneWallInteractionFound)
	{
		s_zoneWallInteractionClosestCoord = closestCoord;
		s_zoneWallInteractionFound = true;
	}
	else
	{
		if ((closestCoord - s_zoneWallInteractionCoord).NormSquared() <
			(s_zoneWallInteractionClosestCoord - s_zoneWallInteractionCoord).NormSquared())
		{
			s_zoneWallInteractionClosestCoord = closestCoord;
		}
	}
}


void gZone::BounceOffPoint(eCoord dest, eCoord collide)
{
	//Use a simple angle deflection for now not even accounting for points that made too far in
	eCoord V = GetVelocity();
								 // adjust position to be like at at impact time
	eCoord P = GetPosition()+V*s_zoneWallInteractionImpactTime;
	eCoord base = collide - P;	 // get normal vector to the impact
	base.Normalize();
								 // adjust speed
	V = V - base*(eCoord::F(base,V)*2);
								 // adjust position according to new speed
	P = P-V*s_zoneWallInteractionImpactTime;
	SetReferenceTime();
	SetVelocity(V);
	SetPosition(P);
	RequestSync();
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

bool gZone::Timestep( REAL time )
{
	if ((sn_GetNetState() != nCLIENT) &&
		(destroyed_))
	{
		//Keep the zone around on the server side so clients will sync

		//??? Without this, the last update and RequestSync() DOES NOT make
		//??? it to the clients.  Investigate how to both remove the zone from
		//??? game AND sync the last update on time.  Removing from game gets to
		//??? the client much later than the sync does if not killed and looks bad.
		return (false);
	}

	bool doRequestSync = false;

	// resize
	if (resizeRequested_)
	{
		REAL r = GetRadius();
		REAL s = GetExpansionSpeed();
		if (((s>0)&&(r>expectedRadius_)) || ((s<0)&&(r<expectedRadius_)))
		{
			SetReferenceTime();
			SetRadius(expectedRadius_);
			SetExpansionSpeed(previousExpansionSpeed_);
			resizeRequested_ = false;
			doRequestSync = true;
		}
	}

	// rotate
	REAL speed = GetRotationSpeed();
	REAL angle = ( time - lastTime ) * speed;
	// angle /= ( 1 + 2 * 3.14159 * angle/sg_segments );
	rotation_ = rotation_.Turn( cos( angle ), sin( angle ) );

	// delta time
	REAL dt = time - referenceTime_;

	// move to new position
	Move( eCoord( posx_( dt ), posy_( dt ) ), lastTime, time );

	// decay speed
	eCoord V = GetVelocity();
	if (sg_ballSpeedDecay)
	{
		REAL SpeedFactor = V.Norm();
		SpeedFactor = SpeedFactor - dt*sg_ballSpeedDecay;
		SpeedFactor = SpeedFactor<0 ? 0 : SpeedFactor;
		SetVelocity(V * SpeedFactor);
		doRequestSync = true;
	}

	// update time
	lastTime = time;

	// kill this zone if it shrunk down to zero radius
	if ( GetExpansionSpeed() < 0 && GetRadius() <= 0 )
	{
		OnVanish();

		if (sn_GetNetState() == nCLIENT)
		{
			return true;
		}
		else
		{
			destroyed_ = true;
			return false;
		}
	}

	if (sn_GetNetState() != nCLIENT)
	{
		// check if wall interactions are enabled
		if (wallInteract_)
		{
			//??? We sometimes hiccup and go through a wall too far between updates,
			//and end up bouncing on the other side of it...  We should really back-calculate
			//and look for walls for time updates that went more than half the zone radius -
			//Or, just drop the whole thing and find out if using sensors would work better...
			s_zoneWallInteractionFound = false;
			s_zoneWallInteractionCoord = GetPosition();
			s_zoneWallInteractionRadius = GetRadius();
			s_zoneWallInteractionZoneVelocity = V;
			s_zoneWallInteractionZoneDeltaTime = time - lastImpactTime_;
			s_zoneWallInteractionZoneDeltaTime = (s_zoneWallInteractionZoneDeltaTime>1)?1:s_zoneWallInteractionZoneDeltaTime;
			grid->ProcessWallsInRange(&S_ZoneWallInteraction,
				s_zoneWallInteractionCoord,
				s_zoneWallInteractionRadius*1.5,
				CurrentFace());

			if (s_zoneWallInteractionFound)
			{
				if (wallBouncesLeft_ == 0)
				{
					// kill the zone as we hit a wall
					//??? make kill code a common function
					destroyed_ = true;
					OnVanish();
					SetReferenceTime();
					SetExpansionSpeed(-1);
					SetRadius(0);
					RequestSync();
					return false;
				}
				else
				{
					// bounce off wall
					BounceOffPoint(GetPosition(), s_zoneWallInteractionImpactCoord);
					lastImpactTime_ = time + s_zoneWallInteractionImpactTime;

					if (wallBouncesLeft_ > 0)
					{
						wallBouncesLeft_--;
					}

					return false;
				}
			}

			// only clip if zone is moving
			if ((posx_.GetSlope() != 0) || (posy_.GetSlope() != 0))
			{
				// clip movement to rim walls
				eCoord dest(posx_(dt), posy_(dt));
				tStackObject< ePoint > start(pos),stop(dest);

				REAL clip = eWallRim::Clip(start,stop,0);

				if (clip < 1)
				{
					gBallZoneHack *thisBallZone = dynamic_cast<gBallZoneHack *>(this);
					if (thisBallZone && sg_ballAutoRespawn)
					{
						// just respawn the ball at the original location
						thisBallZone->GoHome();
					} else
					{
						//Don't allow going outside, kill it
						destroyed_ = true;
						OnVanish();
						SetReferenceTime();
						SetExpansionSpeed(-1);
						SetRadius(0);
						RequestSync();
						return false;
					}
				}
			}
		}

		if(!route_.empty() && time >= nextUpdate_)
		{
			if(nextUpdate_ < 0) nextUpdate_ = time;
			eCoord const &lastPos = route_[lastCoord_++];
			REAL speed = GetVelocity().Norm();
			SetPosition(lastPos);
								 // no speed, no point...
			if(speed < EPS) route_.clear();
			if(lastCoord_ >= route_.size())
			{
				lastCoord_ = 0;
			}
			eCoord delta = route_[lastCoord_] - lastPos;
			REAL dist = delta.Norm();
			if(dist < EPS)		 // next point is the same. Stop the route ...
			{
				eCoord zoneDir = eCoord(0,0);
				SetVelocity(zoneDir);
				route_.clear();
			}
			else
			{
				delta.Normalize();
				SetVelocity(delta * speed);
				nextUpdate_ += dist/speed;
			}
			doRequestSync = true;
		}

		if ( targetRadius_ && (
			((GetExpansionSpeed() > 0) && (GetRadius() >= targetRadius_))
			||
			((GetExpansionSpeed() < 0) && (GetRadius() <= targetRadius_))
			))
		{
			//??? change this to allow negative fall speed and keeping the target radius
			SetReferenceTime();
			SetExpansionSpeed( -fallSpeed_ );
			targetRadius_ = 0;
			doRequestSync = true;
		}

		if (seeking_)
		{
			if ((!pSeekingCycle_) || (!pSeekingCycle_->Player()) || (!pSeekingCycle_->Alive()))
			{
				//Seeking cycle is gone, shrink
				pSeekingCycle_ = NULL;
				SetReferenceTime();
				SetExpansionSpeed( -GetRadius()*1 );
				targetRadius_ = 0;
				seeking_ = false;
				doRequestSync = true;
			}
			else
			{
				//Only run this every poll time to save on network traffic
				if (lastTime >= (lastSeekTime_ + sg_shotSeekUpdateTime))
				{
					//Calculate new direction
					eCoord newDir = pSeekingCycle_->Position() - GetPosition();
					newDir.Normalize();

					SetReferenceTime();
					SetVelocity(newDir * sg_zombieZoneSpeed);
					//??? move out all seeking variables into base zone

					lastSeekTime_ = lastTime;

					doRequestSync = true;
				}
			}
		}

		if (doRequestSync)
		{
			RequestSync();
		}
	}

	return false;
}


gZone &gZone::AddWaypoint(eCoord const &point)
{
	route_.push_back(point);
	return *this;
}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gZone::OnVanish( void )
{
}


// *******************************************************************************
// *
// *	InteractWith
// *
// *******************************************************************************
//!
//!		@param	target        the other game object
//!		@param	time          the current time
//!		@param	recursion     if set to true, don't recurse into other InteractWith functions (quite silly now that I think about it...)
//!
// *******************************************************************************

void gZone::InteractWith( eGameObject * target, REAL time, int recursion )
{
	gCycle* prey = dynamic_cast< gCycle* >( target );
	if ( prey )
	{
		REAL r = this->Radius();
		if ( ( prey->Position() - this->Position() ).NormSquared() < r*r )
		{
			if ( prey->Player() && prey->Alive() )
			{
				OnEnter( prey, time );
			}
		}
	}
	else
	{
		if (!destroyed_)
		{
			// check if the interaction target is a zone
			//gZone *pZone
			gDeathZoneHack *pThisDeathZone = dynamic_cast<gDeathZoneHack *>(this);
			if (pThisDeathZone)
			{
				gDeathZoneHack *pDeathZone = dynamic_cast<gDeathZoneHack *>(target);
				if ((pDeathZone) && (pDeathZone != pThisDeathZone) && (!pDeathZone->destroyed_))
				{
					REAL dis = this->Radius() + pDeathZone->Radius();
					if ( ( pDeathZone->Position() - this->Position() ).NormSquared() < (dis * dis) )
					{
						if ((pThisDeathZone->pLastShotCollision != pDeathZone) &&
							(pDeathZone->pLastShotCollision != pThisDeathZone))
						{
							pThisDeathZone->OnEnter(pDeathZone, time);
						}
					}
					else
					{
						if (pThisDeathZone->pLastShotCollision == pDeathZone)
						{
							pThisDeathZone->pLastShotCollision = NULL;
						}

						if (pDeathZone->pLastShotCollision == pThisDeathZone)
						{
							pDeathZone->pLastShotCollision = NULL;
						}
					}
				}
			}

			gBaseZoneHack *thisBaseZone = dynamic_cast<gBaseZoneHack *>(this);
			if (thisBaseZone)
			{
				gZone *zone = dynamic_cast<gZone *>(target);

				if ((zone) &&
					(zone != this) &&
					(!zone->destroyed_))
				{
					REAL dis = this->Radius() + zone->Radius();

					if ((zone->Position() - this->Position()).NormSquared() < (dis * dis))
					{
						OnEnter(zone, time);
					}
				}
			}

			// interaction between 2 balls
			gBallZoneHack *thisBallZone = dynamic_cast<gBallZoneHack *>(this);
			if (thisBallZone && sg_ballsInteract)
			{
				gBallZoneHack *ball = dynamic_cast<gBallZoneHack *>(target);

				s_zoneWallInteractionFound = false;
				s_zoneWallInteractionCoord = GetPosition();
				s_zoneWallInteractionRadius = GetRadius();
				grid->ProcessWallsInRange(&S_ZoneWallIntersect,
					s_zoneWallInteractionCoord,
					s_zoneWallInteractionRadius+.5,
					CurrentFace());

				if (s_zoneWallInteractionFound)
					return;

				if ((ball) &&
					(ball != this) &&
					(!ball->destroyed_))
				{
					REAL r1 = this->Radius();
					REAL r2 = ball->Radius();
					REAL R = r1 + r2;
					eCoord p1 = this->Position();
					eCoord p2 = ball->Position();
					eCoord dp = p2 - p1;
					REAL c = dp.NormSquared() - R * R;

					if (c < 0)	 // the 2 balls just hit each other ...
					{
						// first find the real time and position of the impact ...
						eCoord v1 = this->GetVelocity();
						eCoord v2 = ball->GetVelocity();
						eCoord dv = v2 - v1;
						REAL a = dv.NormSquared();
						REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
						REAL delta = b*b-4*a*c;
								 // no t. it can't be, an impact just occured ...
						if (delta<0) return;
						else	 // delta is positive, it means we have 2 different solutions
						{		 // the t we are looking for is negative and as close as possible to 0 = it just happens ;)
							delta = sqrt(delta);
							REAL t1 = (-b-delta)/(2*a);
							REAL t2 = (-b+delta)/(2*a);
							REAL t = 0;
								 // can't be, again ...
							if ((t1>0) && (t2>0)) return;
							if (t1>0) t = t2;
							else if (t2>0) t = t1;
							else if (t1>t2) t = t1;
							else t = t2;

							// if a wall impact happens too close, just skip balls interaction for now ...
							if (t > lastImpactTime_ - time) return;

							// now that we have the time, get the positions ...
							eCoord p1c = p1+v1*t;
							eCoord p2c = p2+v2*t;
							//eCoord pi = p1c + (p2c-p1c)*(R/this->Radius());
							// now compute the impact : new velocities and new correct positions ...
							eCoord base = (p2c-p1c);
							base.Normalize();
								 // the weight of the zones are set according to the area
							REAL m1 = r1*r1;
							REAL m2 = r2*r2;
							REAL M = m1+m2;
							eCoord new_v1 = v1 + base*((2*m2/M)*(eCoord::F(dv,base)));
							eCoord new_v2 = v2 - base*((2*m1/M)*(eCoord::F(dv,base)));
							eCoord new_p1 = p1c + new_v1*(-t+0.01);
							eCoord new_p2 = p2c + new_v2*(-t+0.01);
							this->SetReferenceTime();
							this->SetPosition(new_p1);
							this->SetVelocity(new_v1);
							this->RequestSync();
							ball->SetReferenceTime();
							ball->SetPosition(new_p2);
							ball->SetVelocity(new_v2);
							ball->RequestSync();
						}
					}
				}
			}
		}
	}
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gZone::OnEnter( gCycle * target, REAL time )
{
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the zone that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gZone::OnEnter( gZone * target, REAL time )
{
}


// *******************************************************************************
// *
// *	Destroy
// *
// *******************************************************************************

void gZone::Destroy()
{
	// destroy the zone by setting a negative expansion and a zero radius
	destroyed_ = true;
	SetReferenceTime();
	SetExpansionSpeed(-1);
	SetRadius(0);
	RequestSync();
}


// the zone's network initializator
static nNOInitialisator<gZone> zone_init(340,"zone");

// *******************************************************************************
// *
// *	CreatorDescriptor
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nDescriptor & gZone::CreatorDescriptor( void ) const
{
	return zone_init;
}


// *******************************************************************************
// *
// *	Radius
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

REAL gZone::Radius( void ) const
{
	return GetRadius();
}


// extra alpha blending factors
static REAL sg_zoneAlpha = 1.0, sg_zoneAlphaServer = 1.0;
static tSettingItem< REAL > sg_zoneAlphaConf( "ZONE_ALPHA", sg_zoneAlpha );
static nSettingItem< REAL > sg_zoneAlphaConfServer( "ZONE_ALPHA_SERVER", sg_zoneAlphaServer );

// *******************************************************************************
// *
// *	Render
// *
// *******************************************************************************
//!
//!		@param	cam  the camera used for rendering
//!
// *******************************************************************************

void gZone::Render( const eCamera * cam )
{
    #ifndef DEDICATED
    if ( sg_zoneSegLength <= 0 )
        sg_zoneSegLength = .5;
    if ( sg_zoneSegments < 1 )
        sg_zoneSegments = 11;

    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
    alpha = .7f;
    if ( alpha <= 0 )
        return;
    alpha *= sg_zoneAlpha * sg_zoneAlphaServer;

	    ModelMatrix();
    glPushMatrix();

    REAL seglen = 2 * M_PI / sg_zoneSegments * sg_zoneSegLength;

    REAL r = Radius();
    GLfloat m[4][4]={{r*rotation_.x,r*rotation_.y,0,0},
                     {-r*rotation_.y,r*rotation_.x,0,0},
                     {0,0,sg_zoneHeight,0},
                     {pos.x,pos.y,sg_zoneBottom,1}};

    glMultMatrixf(&m[0][0]);

    glColor4f( color_.r ,color_.g,color_.b, alpha );

	bool useAlpha = sr_alphaBlend ? !sg_zoneAlphaToggle : sg_zoneAlphaToggle;
    static bool lastAlpha = useAlpha;

    static rDisplayList zoneList;
    if ( lastAlpha != useAlpha || !zoneList.Call() )
    {
        lastAlpha = useAlpha;

        rDisplayListFiller filler( zoneList );

        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glDisable(GL_TEXTURE_2D);

        if ( useAlpha )
            BeginQuads();
        else
        {
            sr_DepthOffset(true);
            BeginLineStrip();
        }

        for ( int i = sg_zoneSegments - 1; i>=0; --i )
        {
            REAL a = i * 2 * M_PI / REAL( sg_zoneSegments );
            REAL b = a + seglen;

            REAL sa = sin(a);
            REAL ca = cos(a);
            REAL sb = sin(b);
            REAL cb = cos(b);

            glVertex3f(sa, ca, 0);
            glVertex3f(sa, ca, 1);
            glVertex3f(sb, cb, 1);
            glVertex3f(sb, cb, 0);

            if ( !useAlpha )
            {
                glVertex3f(sa, ca, 0);
                RenderEnd();
                BeginLineStrip();
            }
        }

        RenderEnd();

        sr_DepthOffset(false);
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDepthMask(GL_TRUE);
    }

    glPopMatrix();
#endif
}


// *******************************************************************************
// *
// *	DrawSvg
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************
//! draws it in a svg file
void gZone::DrawSvg(std::ofstream &f) {
    REAL r = Radius();
    REAL dash = 2 * r * M_PI / 20;

    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
    alpha = .7f;
    if ( alpha <= 0 )
        return;
    alpha *= sg_zoneAlpha * sg_zoneAlphaServer;

    f << "  <circle cx=\"0\" cy=\"0\" r=\"" << r << "\" fill=\"none\" stroke='" << gSvgColor(color_) << "' stroke-width=\"1\" stroke-dasharray=\""
      << dash << ", " << dash << "\" opacity=\"" <<  alpha << "\" transform=\"translate("
      << pos.x << " " << -pos.y << ")\">\n";
    REAL speed = GetRotationSpeed();
    if(fabs(speed) > EPS) {
        REAL t = fabs(2*M_PI/speed);
        f << "    <animateTransform attributeName=\"transform\" attributeType=\"XML\" type=\"rotate\" from=\"0\" to=\"";
        if(speed < 0) {
            f << '-';
        }
        f << "360\" dur=\""
          << t << "\" repeatCount=\"indefinite\" additive=\"sum\" />\n";
    }
    f << "  </circle>\n";

}


// *******************************************************************************
// *
// *	RendersAlpha
// *
// *******************************************************************************
//!
//!		@return	True if alpha blending is used
//!
// *******************************************************************************
bool gZone::RendersAlpha() const
{
	return sr_alphaBlend ? !sg_zoneAlphaToggle : sg_zoneAlphaToggle;
}


// *******************************************************************************
// *
// *	gWinZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gWinZoneHack::gWinZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation )
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 0.0f;
	color_.g = 1.0f;
	color_.b = 0.0f;
}


// *******************************************************************************
// *
// *	gWinZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gWinZoneHack::gWinZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *	~gWinZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gWinZoneHack::~gWinZoneHack( void )
{
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

static eLadderLogWriter sg_winzonePlayerEnterWriter("WINZONE_PLAYER_ENTER", false);

void gWinZoneHack::OnEnter( gCycle * target, REAL time )
{
	static const char* message="$player_win_instant";
	sg_DeclareWinner( target->Player()->CurrentTeam(), message );

	// let zone vanish
	if ( GetExpansionSpeed() >= 0 )
	{
		SetReferenceTime();
		SetExpansionSpeed( -GetRadius()*.5 );
		RequestSync();
	}

	// message in edlog
	if ((!target) && (!target->Player())) return;
	sg_winzonePlayerEnterWriter << this->GOID() << name_ << GetPosition().x << GetPosition().y << target->Player()->GetUserName() << target->Player()->Object()->Position().x << target->Player()->Object()->Position().y << target->Player()->Object()->Direction().x << target->Player()->Object()->Direction().y << time;
		sg_winzonePlayerEnterWriter.write();
}


// *******************************************************************************
// *
// *	gDeathZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gDeathZoneHack::gDeathZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner )
:gZone( grid, pos, dynamicCreation )
{
	deathZoneType = TYPE_NORMAL;
	pLastShotCollision = NULL;

	color_.r = 1.0f;
	color_.g = 0.0f;
	color_.b = 0.0f;

	if (teamowner!=NULL) team = teamowner;

	grid->AddGameObjectInteresting(this);
}


// *******************************************************************************
// *
// *	gDeathZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gDeathZoneHack::gDeathZoneHack( nMessage & m )
: gZone( m )
{
	deathZoneType = TYPE_NORMAL;
	pLastShotCollision = NULL;
}


// *******************************************************************************
// *
// *	~gDeathZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gDeathZoneHack::~gDeathZoneHack( void )
{
}


static int score_deathzone=-1;
static tSettingItem<int> s_dz("SCORE_DEATHZONE",score_deathzone);

void gDeathZoneHack::OnVanish( void )
{
	grid->RemoveGameObjectInteresting(this);
}


extern int score_die;
extern int score_kill;
extern int score_suicide;

static int score_shot=1;
static tSettingItem<int> s_score_shot("SCORE_SHOT",score_shot);

static int score_shot_suicide=0;
static tSettingItem<int> s_score_shot_suicide("SCORE_SHOT_SUICIDE",score_shot_suicide);

static int score_death_shot=1;
static tSettingItem<int> s_score_death_shot("SCORE_DEATH_SHOT",score_death_shot);

static int score_self_destruct=1;
static tSettingItem<int> s_score_self_destruct("SCORE_SELF_DESTRUCT",score_self_destruct);

static int score_zombie_zone_revenge=1;
static tSettingItem<int> s_score_zombie_zone_revenge("SCORE_ZOMBIE_ZONE_REVENGE",score_zombie_zone_revenge);

static int score_zombie_zone=0;
static tSettingItem<int> s_score_zombie_zone("SCORE_ZOMBIE_ZONE",score_zombie_zone);

static bool sg_shotKillSelf = false;
static tSettingItem<bool> conf_shotKillSelf ("SHOT_KILL_SELF", sg_shotKillSelf);

static bool sg_shotKillVanish = true;
static tSettingItem<bool> conf_shotKillVanish ("SHOT_KILL_VANISH", sg_shotKillVanish);

static bool sg_selfDestructVanish = true;
static tSettingItem<bool> conf_selfDestructVanish ("SELF_DESTRUCT_VANISH", sg_selfDestructVanish);

static bool sg_zombieZoneVanish = true;
static tSettingItem<bool> conf_zombieZoneVanish ("ZOMBIE_ZONE_VANISH", sg_zombieZoneVanish);

static float sg_zombieZoneShoot = true;
static tSettingItem<float> conf_zombieZoneShoot ("ZOMBIE_ZONE_SHOOT", sg_zombieZoneShoot);

//??? Warning - shot collision isn't working quite right
static bool sg_shotCollision = false;
static tSettingItem<bool> conf_shotCollision ("SHOT_COLLISION", sg_shotCollision);

static int sg_shotWallBounce = 0;
static tSettingItem<int> sg_shotWallBounceConf ("SHOT_WALL_BOUNCE", sg_shotWallBounce);

static bool sg_shotKillInvulnerable = 1;
static tSettingItem<bool> sg_shotKillInvulnerableConf( "SHOT_KILL_INVULNERABLE", sg_shotKillInvulnerable );

gZone & gDeathZoneHack::SetType(int type)
{
	if (type < NUM_DEATH_ZONE_TYPES)
	{
		deathZoneType = type;

		if ((type == TYPE_SHOT) ||
			(type == TYPE_DEATH_SHOT))
		{
			// set wall interaction to kill the zone on a wall
			wallInteract_ = true;

			wallBouncesLeft_ = sg_shotWallBounce;

			if (wallBouncesLeft_ < 0)
			{
				wallBouncesLeft_ = 0;
			}
		}
	}

	return (*this);
}


extern tList<ePlayerNetID> se_PlayerNetIDs;

ePlayerNetID * validatePlayer(ePlayerNetID *pPlayer)
{
	if (pPlayer)
	{
		int i;
		for (i = se_PlayerNetIDs.Len()-1; i>=0; --i)
		{
			if (se_PlayerNetIDs(i) == pPlayer)
			{
				break;
			}
		}

		if (i < 0)
		{
			pPlayer = NULL;
		}
	}

	return (pPlayer);
}


gCycle * gDeathZoneHack::getPlayerCycle(ePlayerNetID *pPlayer)
{
	gCycle *pPlayerCycle = NULL;
	if (pPlayer)
	{
		const tList<eGameObject>& gameObjects = Grid()->GameObjects();
		for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			gCycle *pCycle=dynamic_cast<gCycle *>(gameObjects(i));

			if (pCycle)
			{
				if (pCycle->Player() == pOwner_)
				{
					pPlayerCycle = pCycle;
					break;
				}
			}
		}
	}

	return (pPlayerCycle);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************
static eLadderLogWriter sg_deathShotFragWriter("DEATH_SHOT_FRAG", true);
static eLadderLogWriter sg_deathShotSuicideWriter("DEATH_SHOT_SUICIDE", true);
static eLadderLogWriter sg_deathShotTeamkillWriter("DEATH_SHOT_TEAMKILL", true);
static eLadderLogWriter sg_deathDeathZoneWriter("DEATH_DEATHZONE", true);
void gDeathZoneHack::OnEnter( gCycle * target, REAL time )
{
	if (!dynamicCreation_ || ( deathZoneType == TYPE_NORMAL && !team ) )
    {
		target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_deathzone");
    target->Kill();
		sg_deathDeathZoneWriter << target->Player()->GetUserName();
		sg_deathDeathZoneWriter.write();
    }
    else
    {
        if ( !target->Vulnerable() && !sg_shotKillInvulnerable ) {
            //Checks to see if their cycle is invulnerable, don't kill invulnerable players
            return;
   	}

		// check normal death zone linked to a team ...
		if (team && deathZoneType == TYPE_NORMAL)
		{
			if ((!target) || (!target->Player()))
			{
				return;
			}
			if (target->Player()->CurrentTeam() == team) return;
		}

        //Validate the owner player ID
        pOwner_ = validatePlayer(pOwner_);
        //??? Really need for the player to get cleared when they exit, it is possible for
        //a new player to enter and own a zone

        //Get the owner cycle if still around
        gCycle *pOwnerCycle = NULL;
        if (pOwner_)
        {
            pOwnerCycle = getPlayerCycle(pOwner_);

            if (target->Player() == pOwner_)
            {
                //Player entered own shot
                if (!sg_shotKillSelf)
                {
                    //Don't kill yourself
                    return;
                }
                else
                {
                    sg_deathShotSuicideWriter << *target->Player()->GetUserName();
                    sg_deathShotSuicideWriter.write();
                    if (!score_shot_suicide)
                    {
                        tColoredString playerName;
                        playerName << *target->Player() << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_free_shot_suicide", playerName ) );
                    }
                    else
                    {
                        target->Player()->AddScore(score_shot_suicide, tOutput(), "$player_shot_suicide");
                    }
                    target->Kill();
                }
            }
            else
            {
                //The cycle may have been deleted since...  I think this is OK because Player() returns a checked pointer

                //Another player entered a shot, find ownership
                ePlayerNetID *hunter = pOwner_;
                ePlayerNetID *prey = target->Player();

                tOutput lose;
                tOutput win;

                tColoredString preyName;
                preyName << *prey;
                preyName << tColoredString::ColorString(1,1,1);

                if (prey->CurrentTeam() != hunter->CurrentTeam())
                {
                    sg_deathShotFragWriter << prey->GetUserName() << hunter->GetUserName();
                    sg_deathShotFragWriter.write();
                    char const *pWinString = "$player_win_shot";
                    char const *pFreeString = "$player_free_shot";
                    int score = score_shot;
                    if (deathZoneType == TYPE_DEATH_SHOT)
                    {
                        pWinString = "$player_win_death_shot";
                        pFreeString = "$player_free_death_shot";
                        score = score_death_shot;
                    }
                    else if (deathZoneType == TYPE_SELF_DESTRUCT)
                    {
                        pWinString = "$player_win_self_destruct";
                        pFreeString = "$player_free_self_destruct";
                        score = score_self_destruct;
                    }
                    else if (deathZoneType == TYPE_ZOMBIE_ZONE)
                    {
                        if (target == pSeekingCycle_)
                        {
                            pWinString = "$player_win_zombie_zone_revenge";
                            pFreeString = "$player_free_zombie_zone_revenge";
                            score = score_zombie_zone_revenge;

                            //Zombie killed his maker - don't seek anymore
                            pSeekingCycle_ = NULL;
                        }
                        else
                        {
                            pWinString = "$player_win_zombie_zone";
                            pFreeString = "$player_free_zombie_zone";
                            score = score_zombie_zone;
                        }
                    }

                    if (score)
                    {
                        win.SetTemplateParameter(3, preyName);
                        win << pWinString;
                        hunter->AddScore(score, win, lose);
                    }
                    else
                    {
                        tColoredString hunterName;
                        hunterName << *hunter << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( pFreeString, hunterName, preyName ) );
                    }
                }
                else
                {
                    //Team member entered shot
                    if (!sg_shotKillSelf)
                    {
                        //Don't kill team
                        return;
                    }

                    sg_deathShotTeamkillWriter << prey->GetUserName() << hunter->GetUserName();
                    sg_deathShotTeamkillWriter.write();
                    tColoredString hunterName;
                    hunterName << *hunter << tColoredString::ColorString(1,1,1);
                    sn_ConsoleOut( tOutput( "$player_teamkill", hunterName, preyName ) );
                }

                target->Killed(pOwnerCycle); //???make this player so it isn't NULL if dead?

                target->Kill();
            }
        }
        else
        {
			if(deathZoneType == TYPE_ZOMBIE_ZONE)
			{
				target->Player()->AddScore(score_zombie_zone, tOutput(), "$player_lose_suicide");
			}
			else
			{
            target->Player()->AddScore(score_die, tOutput(), "$player_lose_frag");
			}
            target->Kill();
        }

        if (((deathZoneType == TYPE_ZOMBIE_ZONE) && ((!pSeekingCycle_) || (sg_zombieZoneVanish))) ||
            ((deathZoneType == TYPE_SELF_DESTRUCT) && (sg_selfDestructVanish)) ||
            ((deathZoneType < TYPE_SELF_DESTRUCT) && (sg_shotKillVanish)))
        {
#if 0
            SetReferenceTime();
            SetExpansionSpeed( -GetRadius()*1 );
            RequestSync();
#else
            //Instead of making the shot slowly dissapear, we'll get rid of it right away so it doesn't kill the zombie
            destroyed_ = true;
            SetReferenceTime();
            SetExpansionSpeed(-1);
            SetRadius(0);
            RequestSync();
#endif
        }
    }
}


void gDeathZoneHack::OnEnter( gDeathZoneHack * target, REAL time )
{
	//Process if we're a zombie
	if (deathZoneType == TYPE_ZOMBIE_ZONE)
	{
		if (target->deathZoneType == TYPE_ZOMBIE_ZONE)
		{
			//Two zombies met
			if (GetSeekingCycle() == target->GetSeekingCycle())
			{
				//Zombies had same maker, merge
			}
		}
		else if ((sg_zombieZoneShoot > 0) &&
			((target->deathZoneType == TYPE_SHOT) ||
			(target->deathZoneType == TYPE_DEATH_SHOT)))
		{
			//A shot entered the zombie

			//Validate the owner player ID
			pOwner_ = validatePlayer(pOwner_);
			//??? Really need for the player to get cleared when they exit, it is possible for
			//a new player to enter and own a zone

			//Validate the shot owner player ID
			ePlayerNetID *pShotOwner = validatePlayer(target->pOwner_);

			if ((pOwner_) && (pShotOwner))
			{
				//We have both owners
				if ((pOwner_->CurrentTeam() != pShotOwner->CurrentTeam()) ||
					(sg_shotKillSelf))
				{
					//Shoot the zombie!

					REAL zombieRadius = targetRadius_;
					REAL shotRadius = target->GetRadius() * sg_zombieZoneShoot;

					if (targetRadius_ <= 0)
					{
						zombieRadius = GetRadius();
					}

					if (shotRadius >= zombieRadius)
					{
						//Shot was bigger than the zombie, destroy zombie
						destroyed_ = true;
						SetReferenceTime();
						SetExpansionSpeed(-1);
						SetRadius(0);
						RequestSync();

						tColoredString zombieName;
						zombieName << *pOwner_;
						zombieName << tColoredString::ColorString(1,1,1);

						tColoredString shooterName;
						shooterName << *pShotOwner << tColoredString::ColorString(1,1,1);
						sn_ConsoleOut( tOutput( "$player_free_zombie_zone_die", shooterName, zombieName ) );
					}
					else
					{
						//Take the zombie and target radius down by the shot radius
						SetReferenceTime();
						if (targetRadius_ > 0)
						{
							zombieRadius = GetRadius();
							SetRadius(zombieRadius - (shotRadius * zombieRadius / targetRadius_));
							targetRadius_ -= shotRadius;
						}
						else
						{
							SetRadius(zombieRadius - shotRadius);

						}
						RequestSync();
					}

					//Destroy the shot, it is done
					target->destroyed_ = true;
					target->SetReferenceTime();
					target->SetExpansionSpeed(-1);
					target->SetRadius(0);
					target->RequestSync();
				}
			}
		}
	}
	else if ((deathZoneType == TYPE_SHOT) ||
		(deathZoneType == TYPE_DEATH_SHOT))
	{
		if ((target->deathZoneType == TYPE_SHOT) ||
			(target->deathZoneType == TYPE_DEATH_SHOT))
		{
			//??? Warning - shot collision isn't working quite right
			if (sg_shotCollision)
			{
				SetReferenceTime();
				target->SetReferenceTime();

				eCoord position1 = GetPosition();
				eCoord position2 = target->GetPosition();

				eCoord velocity1 = GetVelocity();
				eCoord velocity2 = target->GetVelocity();

				REAL radius1 = GetRadius();
				REAL radius2 = target->GetRadius();

				#if 1
				//Base the mass on a shot sphere volume
				REAL massConstant = M_PI * 4 / 3;
				REAL mass1 = radius1 * radius1 * radius1 * massConstant;
				REAL mass2 = radius2 * radius2 * radius2 * massConstant;
				#else
				//Base the mass on a shot circle area
				REAL massConstant = M_PI;
				REAL mass1 = radius1 * radius1 * massConstant;
				REAL mass2 = radius2 * radius2 * massConstant;
				#endif

				eCoord impact = velocity2 - velocity1;
				eCoord impulse = position2 - position1;
				impulse.Normalize();

				REAL impactSpeed = eCoord::F(impact, impulse);

				REAL sign = 1;
				if (impactSpeed < 0)
				{
					sign = -1;
					impactSpeed *= -1;
				}

				impulse *= sqrt(impactSpeed * mass1 * mass2);
				impulse *= sign;

				velocity1 = velocity1 + (impulse * (1 / mass1));
				velocity2 = velocity2 - (impulse * (1 / mass2));

				SetVelocity(velocity1);
				target->SetVelocity(velocity2);

				RequestSync();
				target->RequestSync();

				//Record the last collision so we don't do it again
				pLastShotCollision = target;
				target->pLastShotCollision = this;
			}
		}
	}
}

// *******************************************************************************
// *
// *	gRubberZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gRubberZoneHack::gRubberZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation)
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 1.0f;
    color_.g = 0.7f;
    color_.b = 0.2f;

	grid->AddGameObjectInteresting(this);
}


// *******************************************************************************
// *
// *	~gRubberZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gRubberZoneHack::~gRubberZoneHack( void )
{
}


static int score_rubberzone=-1;
static tSettingItem<int> s_rz("SCORE_RUBBERZONE",score_rubberzone);

void gRubberZoneHack::OnVanish( void )
{
	grid->RemoveGameObjectInteresting(this);
}

gZone & gRubberZoneHack::SetRubber(REAL rubber)
{
	rmRubber = rubber;

    color_.r = 1.0f;
    REAL p_rubber = (1-(rmRubber/sg_rubberCycle));
    if (p_rubber <0)
        p_rubber=0;
    color_.g = (p_rubber);
    color_.b = (p_rubber/3);

	return (*this);
}

gCycle * gRubberZoneHack::getPlayerCycle(ePlayerNetID *pPlayer)
{
	gCycle *pPlayerCycle = NULL;
	if (pPlayer)
	{
		const tList<eGameObject>& gameObjects = Grid()->GameObjects();
		for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			gCycle *pCycle=dynamic_cast<gCycle *>(gameObjects(i));

			if (pCycle)
			{
				if (pCycle->Player() == pOwner_)
				{
					pPlayerCycle = pCycle;
					break;
				}
			}
		}
	}

	return (pPlayerCycle);
}

// *******************************************************************************
// *
// *	sg_RubberZoneHurt
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@brief    when DZ_RUBBER is not 0: change target's rubber instead of just killing
//!
// *******************************************************************************
static eLadderLogWriter sg_deathRubberZoneWriter("DEATH_RUBBERZONE", true);
static void sg_RubberZoneHurt( gCycle * target, REAL rmRubber );
void sg_RubberZoneHurt( gCycle * target, REAL rmRubber)
{
    REAL rubber = target->GetRubber();

    if ( rubber + rmRubber >= sg_rubberCycle )       // max rubber amount reached, kill the cycle
    {
        target->Player()->AddScore( score_rubberzone, tOutput(), "$player_lose_rubberzone" );
        target->Kill();
        target->SetRubber( sg_rubberCycle );
        sg_deathRubberZoneWriter << target->Player()->GetUserName();
        sg_deathRubberZoneWriter.write();
    }
    else if ( rubber + rmRubber > 0 )            // sg_deathZoneRubberMalus might be negative, avoid setting a negative rubber!
    {
        target->SetRubber( rubber + rmRubber );
    }
    else                                                        // too low value, just set to zero
        target->SetRubber( 0 );
}


// *******************************************************************************
// *    gRubberZoneHack
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gRubberZoneHack::OnEnter( gCycle * target, REAL time )
{
    sg_RubberZoneHurt( target, rmRubber );
}


// *******************************************************************************
// *
// *	gBaseZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner )
:gZone( grid, pos, dynamicCreation), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
    touchy_ = false;

	if (teamowner!=NULL) team = teamowner;

	for (int i=0; i<MAXCLIENTS+1; i++) conquerer_[i]=0;

    color_.r = color_.g = color_.b = 0;
}


// *******************************************************************************
// *
// *	gBaseZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( nMessage & m )
: gZone( m ), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
    touchy_ = false;
	for (int i=0; i<MAXCLIENTS+1; i++) conquerer_[i]=0;
}


// *******************************************************************************
// *
// *	~gBaseZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBaseZoneHack::~gBaseZoneHack( void )
{
}


static REAL sg_conquestRate = .5;
static REAL sg_defendRate = .25;
static REAL sg_conquestDecayRate = .1;

static tSettingItem< REAL > sg_conquestRateConf( "FORTRESS_CONQUEST_RATE", sg_conquestRate );
static tSettingItem< REAL > sg_defendRateConf( "FORTRESS_DEFEND_RATE", sg_defendRate );
static tSettingItem< REAL > sg_conquestDecayRateConf( "FORTRESS_CONQUEST_DECAY_RATE", sg_conquestDecayRate );

// time with no enemy inside a zone before it collapses harmlessly
static REAL sg_conquestTimeout = 0;
static tSettingItem< REAL > sg_conquestTimeoutConf( "FORTRESS_CONQUEST_TIMEOUT", sg_conquestTimeout );

// kill at least than many players from the team that just got its zone conquered
static int sg_onConquestKillMin = 0;
static tSettingItem< int > sg_onConquestKillMinConfig( "FORTRESS_CONQUERED_KILL_MIN", sg_onConquestKillMin );

// and at least this ratio
static REAL sg_onConquestKillRatio = 0;
static tSettingItem< REAL > sg_onConquestKillRationConfig( "FORTRESS_CONQUERED_KILL_RATIO", sg_onConquestKillRatio );

// score you get for conquering a zone
static int sg_onConquestScore = 0;
static tSettingItem< int > sg_onConquestConquestScoreConfig( "FORTRESS_CONQUERED_SCORE", sg_onConquestScore );

// flag indicating whether the team conquering the first zone wins (good for one on one matches)
static int sg_onConquestWin = 1;
static tSettingItem< int > sg_onConquestConquestWinConfig( "FORTRESS_CONQUERED_WIN", sg_onConquestWin );

// maximal number of base zones ownable by a team
static int sg_baseZonesPerTeam = 0;
static tSettingItem< int > sg_baseZonesPerTeamConfig( "FORTRESS_MAX_PER_TEAM", sg_baseZonesPerTeam );
// flag indicating whether base will respawn team if a team player enters it
static bool sg_baseRespawn = false;
static tSettingItem<bool> sg_baseRespawnConfig ("BASE_RESPAWN", sg_baseRespawn);

// flag indicating whether base will respawn team if an enemy player enters it
static bool sg_baseEnemyRespawn = false;
static tSettingItem<bool> sg_baseEnemyRespawnConfig ("BASE_ENEMY_RESPAWN", sg_baseEnemyRespawn);

// flag indicating whether base will kill enemy players
static bool sg_baseEnemyKill = false;
static tSettingItem<bool> sg_baseEnemyKillConfig ("BASE_ENEMY_KILL", sg_baseEnemyKill);

// number of points a player scores on returning a captured flag to their base
static int sg_scoreFlag=1;
static tSettingItem<int> sg_scoreFlagConfig("SCORE_FLAG", sg_scoreFlag);

// number of points a player scores on kicking the ball into the enemy goal
static int sg_scoreGoal=1;
static tSettingItem<int> sg_scoreGoalConfig("SCORE_GOAL", sg_scoreGoal);

// flag indicating whether the round ends when a goal is shot
static bool sg_goalRoundEnd = true;
static tSettingItem<bool> sg_goalRoundEndConfig("GOAL_ROUND_END", sg_goalRoundEnd);

// flag indicating whether flags need to be home to score
static bool sg_flagRequiredHome = true;
static tSettingItem<bool> sg_flagRequiredHomeConfig("FLAG_REQUIRED_HOME", sg_flagRequiredHome);

// flag indicating whether dropping the flag sends it home
static bool sg_flagDropHome = false;
static tSettingItem<bool> sg_flagDropHomeConfig("FLAG_DROP_HOME", sg_flagDropHome);

// time between respawn reminders
static int sg_baseRespawnRemindTime=30;
static tSettingItem<int> sg_baseRespawnRemindTimeConfig("BASE_RESPAWN_REMIND_TIME", sg_baseRespawnRemindTime);

//number of flags that must be home in order to capture
static int sg_minFlagsHome = 0;
static tSettingItem<int> sg_minFlagsHomeConfig( "MIN_FLAGS_HOME", sg_minFlagsHome );

// count zones belonging to the given team.
// fill in count and the zone that is farthest to the team.
void gBaseZoneHack::CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest )
{
	count = 0;
	farthest = NULL;

	// check whether other zones are already registered to that team
	const tList<eGameObject>& gameObjects = grid->GameObjects();
	for (int j=gameObjects.Len()-1;j>=0;j--)
	{
		gBaseZoneHack *otherZone=dynamic_cast<gBaseZoneHack *>(gameObjects(j));

		if ( otherZone && otherTeam == otherZone->Team() )
		{
			count++;
			if ( !farthest || otherZone->teamDistance_ > farthest->teamDistance_ )
				farthest = otherZone;
		}
	}
}


static int sg_onSurviveScore = 0;
static tSettingItem< int > sg_onSurviveConquestScoreConfig( "FORTRESS_HELD_SCORE", sg_onSurviveScore );

extern gArena Arena;

static REAL sg_collapseSpeed = .5;
static tSettingItem< REAL > sg_collapseSpeedConfig( "FORTRESS_COLLAPSE_SPEED", sg_collapseSpeed );

extern gArena Arena;

// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool gBaseZoneHack::Timestep( REAL time )
{
	// no team?!? Get rid of this zone ASAP.
	if ( !team )
	{
		return true;
	}

	if ( currentState_ == State_Conquering )
	{
		// let zone vanish
		SetReferenceTime();

		// let it light up in agony
		if ( sg_collapseSpeed < .4 )
		{
			color_.r = color_.g = color_.b = 1;
		}

		SetExpansionSpeed( -GetRadius()*sg_collapseSpeed );
		SetRotationAcceleration( -GetRotationSpeed()*.4 );
		RequestSync();

		currentState_ = State_Conquered;
	}
	else if ( currentState_ == State_Conquered && GetRotationSpeed() < 0 )
	{
		// let zone vanish
		SetReferenceTime();
		SetRotationSpeed( 0 );
		SetRotationAcceleration( 0 );
		color_.r = color_.g = color_.b = .5;
		RequestSync();
	}

	REAL dt = time - lastTime;

	// conquest going on
	REAL conquest = sg_conquestRate * enemiesInside_ - sg_defendRate * ownersInside_ - sg_conquestDecayRate;
	conquered_ += dt * conquest;

	if ( touchy_ && enemiesInside_ > 0 )
	{
		conquered_ = 1.01;
	}

	// clamp
	if ( conquered_ < 0 )
	{
		conquered_ = 0;
		conquest = 0;
	}
	if ( conquered_ > 1.01 )
	{
		conquered_ = 1.01;
		conquest = 0;
	}

	// set speed according to conquest status
	if ( currentState_ == State_Safe )
	{
		REAL maxSpeed = 10 * ( 2 *  M_PI ) / sg_segments;
		REAL omega = .3 + conquered_ * conquered_ * maxSpeed;
		REAL omegaDot = 2 * conquered_ * conquest * maxSpeed;

		// determine the time since the last sync (exaggerate for smoother motion in local games)
		REAL timeStep = lastTime - lastSync_;
		if ( sn_GetNetState() != nSERVER )
			timeStep *= 100;

		if ( sn_GetNetState() != nCLIENT &&
			( ( fabs( omega - GetRotationSpeed() ) + fabs( omegaDot - GetRotationAcceleration() ) ) * timeStep > .5 ) )
		{
			SetRotationSpeed( omega );
			SetRotationAcceleration( omegaDot );
			SetReferenceTime();
			RequestSync();
			lastSync_ = lastTime;
		}

		// check for enemy contact timeout
		if ( sg_conquestTimeout > 0 && lastEnemyContact_ + sg_conquestTimeout < time )
		{
			enemies_.clear();

			// if the zone would collapse without defenders, let it collapse now. A smart defender would
			// have left the zone to let it collapse anyway.
			if ( sg_conquestDecayRate < 0 )
			{
				if ( team )
				{
					if ( sg_onSurviveScore != 0 )
					{
						// give player the survive score bonus right now, they deserve it
						ZoneWasHeld();
					}
					else
					{
						sn_ConsoleOut( tOutput( "$zone_collapse_harmless", team->GetColoredName()  ) );
					}
				}
				conquered_ = 1.0;
			}
		}

		// check whether the zone got conquered
		if ( conquered_ >= 1 )
		{
			currentState_ = State_Conquering;
			OnConquest();
		}
	}

	if ((team) &&
		(((ownersInside_ > 0) && (sg_baseRespawn)) ||
		((enemiesInside_ > 0) && (sg_baseEnemyRespawn))))
	{
		// check for respawn
		//??? change this to divide up spawns between bases, and randomly choose among those closest
		gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());

		if (pSpawn)
		{
			for (int i = team->NumPlayers() - 1; i >= 0; --i)
			{
				ePlayerNetID *pPlayer = team->Player(i);

				eGameObject *pGameObject = pPlayer->Object();

				if ((!pGameObject) ||
					((!pGameObject->Alive()) &&
					(pGameObject->DeathTime() < (time - 1))))
				{
					lastRespawnRemindTime_ = time - sg_baseRespawnRemindTime - 1;

					eCoord pos, dir;

					pSpawn->Spawn(pos, dir);

					gCycle *pCycle = new gCycle(grid, pos, dir, pPlayer);
					pPlayer->ControlObject(pCycle);

					tColoredString playerName;
					playerName << *pPlayer << tColoredString::ColorString(1,1,1);

					if ((ownersInside_ > 0) && (sg_baseRespawn))
					{
						tColoredString spawnerName;
						spawnerName << teamPlayerName_ << tColoredString::ColorString(1,1,1);
						sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );
					}
					else
					{
						tColoredString spawnerName;
						spawnerName << enemyPlayerName_ << tColoredString::ColorString(1,1,1);
						sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );
					}

					// send a console message to the player
					sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
				}
			}
		}
	}

	// reset counts
	enemiesInside_ = ownersInside_ = 0;

	if ((team) && (sg_baseRespawn) && (sg_baseRespawnRemindTime > 0) &&
		(time >= (lastRespawnRemindTime_ + sg_baseRespawnRemindTime)))
	{
		// find out how many players are waiting to be respawned
		int waiting = 0;
		int alive = 0;

		for (int i = team->NumPlayers() - 1; i >= 0; --i)
		{
			ePlayerNetID *pPlayer = team->Player(i);

			eGameObject *pGameObject = pPlayer->Object();

			if ((!pGameObject) ||
				(!pGameObject->Alive()))
			{
				waiting++;
			}
			else
			{
				alive++;
			}
		}

		if ((waiting) && (alive))
		{
			if (lastRespawnRemindWaiting_)
			{
				for (int i = team->NumPlayers() - 1; i >= 0; --i)
				{
					ePlayerNetID *pPlayer = team->Player(i);
					if (alive > 1)
					{
						sn_ConsoleOut(tOutput("$player_base_respawn_reminder", waiting), pPlayer->Owner());
					}
					else
					{
						sn_ConsoleOut(tOutput("$player_base_respawn_reminder_alone", waiting), pPlayer->Owner());
					}
				}
			}

			if ((alive > 1) || (lastRespawnRemindWaiting_ != 1))
			{
				lastRespawnRemindTime_ = time;
			}
			else
			{
				lastRespawnRemindTime_ = time - (sg_baseRespawnRemindTime / 3);
			}
		}

		lastRespawnRemindWaiting_ = waiting;
	}

	// delegate
	bool ret = gZone::Timestep( time );

	// reward survival
	if ( team && !ret && onlySurvivor_ )
	{
		const char* message= ( eTeam::teams.Len() > 2 || sg_onConquestScore ) ? "$player_win_held_fortress" : "$player_win_conquest";
		sg_DeclareWinner( team, message );
	}

	return ret;
}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter sg_deathBasezoneConqueredWriter("DEATH_BASEZONE_CONQUERED", true);

void gBaseZoneHack::OnVanish( void )
{

	if (!team)
		return;

	CheckSurvivor();

	// kill the closest owners of the zone
	if ( currentState_ != State_Safe  && ( enemies_.size() > 0 || sg_defendRate < 0 ))
	{
		int kills = int( sg_onConquestKillRatio * team->NumPlayers() );
		kills = kills > sg_onConquestKillMin ? kills : sg_onConquestKillMin;

		while ( kills > 0 )
		{
			-- kills;

			ePlayerNetID * closest = NULL;
			REAL closestDistance = 0;

			// find the closest living owner
			for ( int i = team->NumPlayers()-1; i >= 0; --i )
			{
				ePlayerNetID * player = team->Player(i);
				eNetGameObject * object = player->Object();
				if ( object && object->Alive() )
				{
					eCoord otherpos = object->Position() - pos;
					REAL distance = otherpos.NormSquared();
					if ( !closest || distance < closestDistance )
					{
						closest = player;
						closestDistance = distance;
					}
				}
			}

			if ( closest )
			{
				sg_deathBasezoneConqueredWriter << closest->GetUserName();
				if ( enemies_.size() > 0 || sg_defendRate < 0)
				{
                    tColoredString playerName;
                    playerName = closest->GetColoredName();
                    playerName << tColoredStringProxy(-1,-1,-1);
                    sn_ConsoleOut( tOutput("$player_kill_collapse", playerName ) );
					closest->Object()->Kill();
				}
				else
				{
					sg_deathBasezoneConqueredWriter << "NO_ENEMIES";
				}
				sg_deathBasezoneConqueredWriter.write();
			}
		}
	}
}


// *******************************************************************************
// *
// *	OnConquest
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter sg_basezoneConqueredWriter("BASEZONE_CONQUERED", true);
static eLadderLogWriter sg_basezoneConquererWriter("BASEZONE_CONQUERER", true);
static eLadderLogWriter sg_basezoneConquererTeamWriter("BASEZONE_CONQUERER_TEAM", true);

void gBaseZoneHack::OnConquest( void )
{
	if ( team )
	{
		sg_basezoneConqueredWriter << ePlayerNetID::FilterName(team->Name()) << GetPosition().x << GetPosition().y;
		if ( enemies_.size() == 0 ) //no enemies inside
		{
			sg_basezoneConqueredWriter << "NO_ENEMIES";
		}
		sg_basezoneConqueredWriter.write();
	}
	float rr = GetRadius();
	rr *= rr;
    int totalInZone = 0;
    for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
	{
		ePlayerNetID *player = se_PlayerNetIDs(i);
		if(!player)
		{
			continue;
		}
		gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
		if(!cycle)
		{
			continue;
		}
		if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
		{
			totalInZone++;
		}
	}
    float percentWon = 0.0;
    if (totalInZone > 0){
        percentWon = 1.0 /totalInZone;
    }
	for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
	{
		ePlayerNetID *player = se_PlayerNetIDs(i);
		if(!player)
		{
			continue;
		}
		gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
		if(!cycle)
		{
			continue;
		}
		if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
		{
			sg_basezoneConquererWriter << player->GetUserName();
            sg_basezoneConquererWriter << percentWon;
			sg_basezoneConquererWriter.write();
		}
	}

	// calculate score. If nobody really was inside the zone any more, half it.
	int totalScore = sg_onConquestScore;
	if ( 0 == enemiesInside_ )
		totalScore /= 2;

	// eliminate dead enemies
	TeamArray enemiesAlive;
	for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
	{
		eTeam* team = *iter;
		if ( team->Alive() )
			enemiesAlive.push_back( team );
	}
	enemies_ = enemiesAlive;

	// add score for successful conquest, divided equally between the teams that are
	// inside the zone
	if ( totalScore && enemies_.size() > 0 )
	{
		tOutput win;
		if ( team )
		{
			win.SetTemplateParameter( 3, team->GetColoredName() );
			win << "$player_win_conquest_specific";
		}
		else
		{
			win << "$player_win_conquest";
		}
		int score = totalScore / enemies_.size();
		for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
		{
			//sg_basezoneConquererTeamWriter << ePlayerNetID::FilterName((*iter)->Name()) << score;
			//sg_basezoneConquererTeamWriter.write();
			(*iter)->AddScore( score, win, tOutput() );
		}
	}
	//write basezoneConquererTeam msg regardless of whether a score is given
		if(  enemies_.size() > 0)
		{
			int score = totalScore / enemies_.size();
			for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
			{
				sg_basezoneConquererTeamWriter << ePlayerNetID::FilterName((*iter)->Name()) << score;
				sg_basezoneConquererTeamWriter.write();
			}
		}
	// trigger immediate win
	if ( sg_onConquestWin && enemies_.size() > 0 )
	{
		static const char* message="$player_win_conquest";
		sg_DeclareWinner( enemies_[0], message );
	}
	CheckSurvivor();
}


// if this flag is enabled, the last team with a non-conquered zone wins the round.
static int sg_onSurviveWin = 1;
static tSettingItem< int > sg_onSurviveWinConfig( "FORTRESS_SURVIVE_WIN", sg_onSurviveWin );

// *******************************************************************************
// *
// *	CheckSurvivor
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::CheckSurvivor( void )
{
	// test if there is only one team with non-conquered zones left
	if ( sg_onSurviveWin )
	{
		// find surviving team and test whether it is the only one
		gBaseZoneHack * survivor = 0;
		bool onlySurvivor = true;

		const tList<eGameObject>& gameObjects = Grid()->GameObjects();
		for (int i=gameObjects.Len()-1;i>=0 && onlySurvivor;i--)
		{
			gBaseZoneHack *other=dynamic_cast<gBaseZoneHack *>(gameObjects(i));

			if ( other && other->currentState_ == State_Safe && other->team )
			{
				if ( survivor && survivor->team != other->team )
					onlySurvivor = false;
				else
					survivor = other;
			}
		}

		// reward it later
		if ( onlySurvivor && survivor )
		{
			survivor->onlySurvivor_ = true;
		}
	}
}


// *******************************************************************************
// *
// *   ZoneWasHeld
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::ZoneWasHeld( void )
{
	// survived?
	if ( currentState_ == State_Safe && sg_onSurviveScore != 0 )
	{
		// award owning team
		if ( team && team->Alive() )
		{
			team->AddScore( sg_onSurviveScore, tOutput("$player_win_held_fortress"), tOutput("$player_lose_held_fortress") );

			currentState_ = State_Conquering;
			enemies_.clear();
		}
		else
		{
			// give a little conquering help. The round is almost over, if
			// an enemy actually made it into the zone by now, it should be his.
			touchy_ = true;
		}
	}
}


// *******************************************************************************
// *
// *   CheckTeamAssignment
// *
// *******************************************************************************
//!
//! @return true if a team is assigned, false if not
//!
// *******************************************************************************

bool gBaseZoneHack::CheckTeamAssignment()
{
	// determine the owning team: the one that has a player spawned closest
	// find the closest player
	if ( !team )
	{
		teamDistance_ = 0;
		const tList<eGameObject>& gameObjects = Grid()->GameObjects();
		gCycle * closest = NULL;
		REAL closestDistance = 0;
		for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

			if (other )
			{
				eTeam * otherTeam = other->Player()->CurrentTeam();
				eCoord otherpos = other->Position() - pos;
				REAL distance = otherpos.NormSquared();
				if ( !closest || distance < closestDistance )
				{
					// check whether other zones are already registered to that team
					gBaseZoneHack * farthest = NULL;
					int count = 0;
					if ( sg_baseZonesPerTeam > 0 )
						CountZonesOfTeam( Grid(), otherTeam, count, farthest );

					// only set team if not too many closer other zones are registered
					if ( sg_baseZonesPerTeam == 0 || count < sg_baseZonesPerTeam || farthest->teamDistance_ > distance )
					{
						closest = other;
						closestDistance = distance;
					}
				}
			}
		}

		if ( closest )
		{
			// take over team and color
			team = closest->Player()->CurrentTeam();
			color_.r = team->R()/15.0;
			color_.g = team->G()/15.0;
			color_.b = team->B()/15.0;
			teamDistance_ = closestDistance;

			RequestSync();
		}

		// if this zone does not belong to a team, return false.
		if ( !team )
		{
			return false;
		}

		// check other zones owned by the same team. Discard the one farthest away
		// if the max count is exceeded
		if ( team && sg_baseZonesPerTeam > 0 )
		{
			gBaseZoneHack * farthest = 0;
			int count = 0;
			CountZonesOfTeam( Grid(), team, count, farthest );

			// discard team of farthest zone
			if ( count > sg_baseZonesPerTeam )
			{
				farthest->team = NULL;
				farthest->RemoveFromGame();
			}
		}
	}
	return true;
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

void gBaseZoneHack::OnRoundBegin( void )
{
		// if this zone does not belong to a team, discard it.
		if ( !CheckTeamAssignment() )
		{
			RemoveFromGame();
			return;
		}
}


// *******************************************************************************
// *
// *   OnRoundEnd
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::OnRoundEnd( void )
{
	// survived?
	if ( currentState_ == State_Safe )
	{
		ZoneWasHeld();
	}
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gBaseZoneHack::OnEnter( gCycle * target, REAL time )
{
	// determine the team of the player
	tASSERT( target );
	if ( !target->Player() )
		return;
	tJUST_CONTROLLED_PTR< eTeam > otherTeam = target->Player()->CurrentTeam();
	if (!otherTeam)
		return;
	if ( currentState_ != State_Safe )
		return;

	// remember who is inside
	if ( team == otherTeam )
	{
		if ( ownersInside_ == 0 )
		{
			// store the name
			teamPlayerName_ = target->Player()->GetColoredName();
		}

		++ ownersInside_;
	}
	else if ( team )
	{
		// check if base enemy kills are enabled
		if (sg_baseEnemyKill)
		{
			tColoredString playerName;
			playerName << *target->Player() << tColoredString::ColorString(1,1,1);
			sn_ConsoleOut( tOutput( "$player_base_enemy_kill", playerName ) );

			target->Kill();

			return;
		}

		if ( enemiesInside_ == 0 )
		{
			enemies_.clear();

			// store the name
			enemyPlayerName_ = target->Player()->GetColoredName();
		}

		++ enemiesInside_;
		conquerer_[target->Player()->ListID()]+=time - lastTime;
		if ( std::find( enemies_.begin(), enemies_.end(), otherTeam ) == enemies_.end() )
			enemies_.push_back( otherTeam );

		lastEnemyContact_ = time;
	}
	else
	{
		conquerer_[target->Player()->ListID()]+=time - lastTime;
	}

	// check if player has a flag
	if (target->flag_)
	{
		// check if the player is on our team
		if (team == otherTeam)
		{
			// search for another flag owned by our team
			bool allFlagsHome = true;
            int flagsHome = 0;
            int flagsTaken = 0;
            const tList<eGameObject>& gameObjects = Grid()->GameObjects();
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                gFlagZoneHack *otherFlag=dynamic_cast<gFlagZoneHack *>(gameObjects(i));

                if ((otherFlag) &&
                    (otherFlag->Team() == team))
                {
                    // check if flag is at home (starting position)
                    if (!otherFlag->IsHome())
                    {
                        allFlagsHome = false;
                        flagsTaken++;
                    }
                    else
                    {
                        flagsHome++;
                    }
                }
            }
            if (!sg_flagRequiredHome)
			{
                if (sg_minFlagsHome > (flagsHome+flagsTaken)){
                    allFlagsHome=false;
                }else if(sg_minFlagsHome <= flagsHome){
                    allFlagsHome=true;
                }
			}

			if (!allFlagsHome)
			{
				target->flag_->WarnFlagNotHome();
			}
			else
			{
				// player has scored a flag capture
				tOutput lose;
				tOutput win;
				int score = sg_scoreFlag;

				win << "$player_flag_score";
				target->Player()->AddScore(score, win, lose);

				// tell the flag to go back home
				target->flag_->GoHome();
			}
		}
	}
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the zone that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gBaseZoneHack::OnEnter( gZone * target, REAL time )
{
								 //it already hit the zone.
	if(currentState_ == State_Conquering || currentState_ == State_Conquered) return;
	gBallZoneHack *ball = dynamic_cast<gBallZoneHack *>(target);
	if (!ball) return;

	// check ball team mode
	int toScore;
	if (!team || !target ) toScore = 1;
	else if (sg_ballTeamMode) toScore = target->Team() == team ? 1 : 0;
	else toScore = target->Team() == team ? 0 : 1;

	if (ball && toScore)
	{
		// the ball entered the goal, figure out who shot it
		ePlayerNetID *lastPlayer = ball->GetLastPlayer();

		// and respawn players ...
		if ( sg_baseRespawn || sg_baseEnemyRespawn )
		{
			// check for respawn
			//??? change this to divide up spawns between bases, and randomly choose among those closest
			gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());

			if (pSpawn)
			{
				for (int i = team->NumPlayers() - 1; i >= 0; --i)
				{
					ePlayerNetID *pPlayer = team->Player(i);

					eGameObject *pGameObject = pPlayer->Object();

					if ((!pGameObject) ||
						((!pGameObject->Alive()) &&
						(pGameObject->DeathTime() < (time - 1))))
					{
						lastRespawnRemindTime_ = time - sg_baseRespawnRemindTime - 1;

						eCoord pos, dir;

						pSpawn->Spawn(pos, dir);

						gCycle *pCycle = new gCycle(grid, pos, dir, pPlayer);
						pPlayer->ControlObject(pCycle);

						tColoredString playerName;
						playerName << *pPlayer << tColoredString::ColorString(1,1,1);

						if ((ownersInside_ > 0) && (sg_baseRespawn))
						{
							tColoredString spawnerName;
							spawnerName << teamPlayerName_ << tColoredString::ColorString(1,1,1);
							sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );
						}
						else
						{
							tColoredString spawnerName;
							spawnerName << enemyPlayerName_ << tColoredString::ColorString(1,1,1);
							sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );
						}

						// send a console message to the player
						sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
					}
				}
			}
		}

		if (!lastPlayer)
		{
			return;
		}
		else
		{
			eTeam *lastTeam = lastPlayer->CurrentTeam();

			if (lastTeam == team)
			{
				// own team hit it in
				tColoredString playerName;
				playerName << *lastPlayer << tColoredString::ColorString(1,1,1);
				sn_ConsoleOut( tOutput( "$player_score_own_goal", playerName ) );

				// search through the teams and add a point to each
				for (int i = eTeam::teams.Len() - 1; i >= 0; i--)
				{
					if (eTeam::teams(i) != team)
					{
						eTeam::teams(i)->AddScore(1);
						lastTeam = eTeam::teams(i);
					}
				}
			}
			else
			{
				// another team hit it in, give that player the point
				tOutput lose;
				tOutput win;
				int score = sg_scoreGoal;

				win << "$player_score_goal";
				lastPlayer->AddScore(score, win, lose);
			}

			// check if the round should end or we should respawn the ball
			if (sg_goalRoundEnd)
			{
				//static const char* message="$player_win_instant";
				//sg_DeclareWinner( lastTeam, message );

				//// destroy the ball
				//ball->Destroy();
				OnConquest();
				currentState_ = State_Conquering;
			}
			else
			{
				// don't end the round, just respawn the ball at the original location
				ball->GoHome();
			}
		}
	}
}


// *******************************************************************************
// *
// *	tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::tFunction( void )
: offset_(0), slope_(0)
{
}


// *******************************************************************************
// *
// *	~tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::~tFunction( void )
{
}


// *******************************************************************************
// *
// *	Evaluate
// *
// *******************************************************************************
//!
//!		@param	argument
//!		@return
//!
// *******************************************************************************

REAL tFunction::Evaluate( REAL argument ) const
{
	return offset_ + slope_ * argument;
}


// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	m	message to write to
//!		@param	f	function to write
//!		@return		reference to message for chaining
//!
// *******************************************************************************

nMessage & operator << ( nMessage & m, tFunction const & f )
{
	// write ID for compatibility with future extensions
	unsigned short ID = 1;
	m.Write( ID );

	// write values
	m << f.GetOffset();
	m << f.GetSlope();

	return m;
}


// *******************************************************************************
// *
// *	operator >>
// *
// *******************************************************************************
//!
//!     @param  m   message to read from
//!     @param  f   function to read to
//!     @return     reference to message for chaining
//!
// *******************************************************************************

nMessage & operator >> ( nMessage & m, tFunction & f )
{
	// write ID for compatibility with future extensions
	unsigned short ID;
	m.Read(ID);
	tASSERT( ID == 1 );

	// read values
	REAL slope, offset;
	m >> offset >> slope;

	// store values
	f.SetOffset( offset ).SetSlope( slope );

	return m;
}


// *******************************************************************************
// *
// *	GetPosition
// *
// *******************************************************************************
//!
//!		@return		the current position
//!
// *******************************************************************************

eCoord gZone::GetPosition( void ) const
{
	eCoord ret;
	GetPosition( ret );
	return ret;
}


// *******************************************************************************
// *
// *	GetPosition
// *
// *******************************************************************************
//!
//!		@param	position	the current position to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetPosition( eCoord & position ) const
{
	position.x = EvaluateFunctionNow( posx_ );
	position.y = EvaluateFunctionNow( posy_ );
	return *this;
}


// *******************************************************************************
// *
// *	SetPosition
// *
// *******************************************************************************
//!
//!		@param	position	the current position to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetPosition( eCoord const & position )
{
	SetFunctionNow( posx_, position.x );
	SetFunctionNow( posy_, position.y );
	return *this;
}


// *******************************************************************************
// *
// *	GetVelocity
// *
// *******************************************************************************
//!
//!		@return		the current velocity
//!
// *******************************************************************************

eCoord gZone::GetVelocity( void ) const
{
	eCoord ret;
	GetVelocity( ret );

	return ret;
}


// *******************************************************************************
// *
// *	GetVelocity
// *
// *******************************************************************************
//!
//!		@param	velocity	the current velocity to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetVelocity( eCoord & velocity ) const
{
	velocity.x = posx_.GetSlope();
	velocity.y = posy_.GetSlope();

	return *this;
}


// *******************************************************************************
// *
// *	SetVelocity
// *
// *******************************************************************************
//!
//!		@param	velocity	the current velocity to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetVelocity( eCoord const & velocity )
{
	// backup position
	eCoord pos;
	GetPosition( pos );

	posx_.SetSlope( velocity.x );
	posy_.SetSlope( velocity.y );

	// restore position
	SetPosition( pos );

	return *this;
}


// *******************************************************************************
// *
// *	GetRadius
// *
// *******************************************************************************
//!
//!		@return		the current radius
//!
// *******************************************************************************

REAL gZone::GetRadius( void ) const
{
	REAL ret = EvaluateFunctionNow( this->radius_ );
	ret = ret > 0 ? ret : 0;

	return ret;
}


// *******************************************************************************
// *
// *	GetRadius
// *
// *******************************************************************************
//!
//!		@param	radius	the current radius to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRadius( REAL & radius ) const
{
	radius = GetRadius();

	return *this;
}


// *******************************************************************************
// *
// *	SetRadius
// *
// *******************************************************************************
//!
//!		@param	radius	the current radius to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRadius( REAL radius )
{
	SetFunctionNow( this->radius_, radius );

	return *this;
}


// *******************************************************************************
// *
// *    SetRadiusSmoothly
// *
// *******************************************************************************
//!
//!             @param  radius  the current radius to set
//!             @param  expansionSpeed  the current expansion speed to set
//!             @return         A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRadiusSmoothly( REAL radius, REAL expansionSpeed )
{
	expectedRadius_ = radius;
								 // save expansion speed if not already resizing ...
	if (!resizeRequested_) previousExpansionSpeed_ = GetExpansionSpeed();
	resizeRequested_ = true;
	expansionSpeed = (expansionSpeed==0?radius*0.5:(expansionSpeed<0?-expansionSpeed:expansionSpeed));
	SetExpansionSpeed( (GetRadius()>radius?-expansionSpeed:expansionSpeed ));

	return *this;
}


// *******************************************************************************
// *
// *	GetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@return		the current expansion speed
//!
// *******************************************************************************

REAL gZone::GetExpansionSpeed( void ) const
{
	return this->radius_.GetSlope();
}


// *******************************************************************************
// *
// *	GetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@param	expansionSpeed	the current expansion speed to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetExpansionSpeed( REAL & expansionSpeed ) const
{
	expansionSpeed = this->radius_.GetSlope();

	return *this;
}


// *******************************************************************************
// *
// *	SetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@param	expansionSpeed	the current expansion speed to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetExpansionSpeed( REAL expansionSpeed )
{
	REAL r = EvaluateFunctionNow( this->radius_ );
	this->radius_.SetSlope( expansionSpeed );
	SetRadius( r );

	return *this;
}


// *******************************************************************************
// *
// *	SetReferenceTime
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gZone::SetReferenceTime( void )
{
	// set offsets to current values
	this->posx_.SetOffset( EvaluateFunctionNow( this->posx_ ) );
	this->posy_.SetOffset( EvaluateFunctionNow( this->posy_ ) );
	this->radius_.SetOffset( EvaluateFunctionNow( this->radius_ ) );
	this->rotationSpeed_.SetOffset( EvaluateFunctionNow( this->rotationSpeed_ ) );

	// reset time
	this->referenceTime_ = lastTime;
}


// *******************************************************************************
// *
// *	GetRotationSpeed
// *
// *******************************************************************************
//!
//!		@return		The current rotation speed
//!
// *******************************************************************************

REAL gZone::GetRotationSpeed( void ) const
{
	return EvaluateFunctionNow( rotationSpeed_ );
}


// *******************************************************************************
// *
// *	GetRotationSpeed
// *
// *******************************************************************************
//!
//!		@param	rotationSpeed	The current rotation speed to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationSpeed( REAL & rotationSpeed ) const
{
	rotationSpeed = this->GetRotationSpeed();
	return *this;
}


// *******************************************************************************
// *
// *	SetRotationSpeed
// *
// *******************************************************************************
//!
//!		@param	rotationSpeed	The current rotation speed to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationSpeed( REAL rotationSpeed )
{
	SetFunctionNow( this->rotationSpeed_, rotationSpeed );
	return *this;
}


// *******************************************************************************
// *
// *	GetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@return		the current acceleration of the rotation
//!
// *******************************************************************************

REAL gZone::GetRotationAcceleration( void ) const
{
	return this->rotationSpeed_.GetSlope();
}


// *******************************************************************************
// *
// *	GetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@param	rotationAcceleration	the current acceleration of the rotation to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationAcceleration( REAL & rotationAcceleration ) const
{
	rotationAcceleration = this->GetRotationAcceleration();
	return *this;
}


// *******************************************************************************
// *
// *	SetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@param	rotationAcceleration	the current acceleration of the rotation to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationAcceleration( REAL rotationAcceleration )
{
	REAL omega = this->GetRotationSpeed();
	this->rotationSpeed_.SetSlope( rotationAcceleration );
	SetRotationSpeed( omega );

	return *this;
}


// *******************************************************************************
// *
// *	gBallZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gBallZoneHack::gBallZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner )
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 1.0f;
	color_.g = 1.0f;
	color_.b = 1.0f;

	wallInteract_ = true;
	wallBouncesLeft_ = -1;
	lastPlayer_ = NULL;
	originalPosition_ = pos;
	originalVelocity_ = eCoord(0,0);
	init_ = false;
	lastImpactTime_ = 0;

	if (teamowner!=NULL) team = teamowner;

	grid->AddGameObjectInteresting(this);
}


// *******************************************************************************
// *
// *	gBallZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gBallZoneHack::gBallZoneHack( nMessage & m )
: gZone( m )
{
	lastPlayer_ = NULL;
	init_ = false;
	originalPosition_ = pos;
	originalVelocity_ = eCoord(0,0);
}


// *******************************************************************************
// *
// *	~gBallZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBallZoneHack::~gBallZoneHack( void )
{
}


static eLadderLogWriter sg_ballVanishWriter("BALL_VANISH", false);

void gBallZoneHack::OnVanish( void )
{
	grid->RemoveGameObjectInteresting(this);
	sg_ballVanishWriter << this->GOID() << name_ << GetPosition().x << GetPosition().y;
	sg_ballVanishWriter.write();
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!             @param  time    the current time
//!
// *******************************************************************************

bool gBallZoneHack::Timestep( REAL time )
{
	// initialize the zone info, this happens only once, but has to happen after construction
	if (!init_)
	{
		init_ = true;
		originalVelocity_ = GetVelocity();
	}

	// delegate
	bool returnStatus = gZone::Timestep( time );

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************
extern eLadderLogWriter sg_deathFragWriter;
extern eLadderLogWriter sg_deathSuicideWriter;
extern eLadderLogWriter sg_deathTeamkillWriter;

void gBallZoneHack::OnEnter( gCycle * target, REAL time )
{
	if ((!target) || (!target->Player()))
	{
		return;
	}

	REAL dt = time - referenceTime_;
	eCoord dest(posx_(dt), posy_(dt));
	s_zoneWallInteractionFound = false;
	s_zoneWallInteractionCoord = dest;
	s_zoneWallInteractionRadius = GetRadius();
	grid->ProcessWallsInRange(&S_ZoneWallIntersect,
		s_zoneWallInteractionCoord,
		s_zoneWallInteractionRadius,
		CurrentFace());

	// keep some data to allow to bounce even for killing balls ...
	eCoord p2 = target->Position();
	eCoord v2 = target->Direction()*target->Speed();

	// check killer balls ... ;)
	ePlayerNetID * prey = target->Player();
	if ((sg_ballKiller) && (team) && !(prey->CurrentTeam() == team))
	{
		target->Kill();
		// scoring ...
		if ( lastPlayer_ )
		{
			tOutput lose;
			tOutput win;
			ePlayerNetID * hunter = lastPlayer_;

			if ( hunter->CurrentTeam() != prey->CurrentTeam() )
			{
				tColoredString preyName;
				preyName << *prey;
				preyName << tColoredString::ColorString(1,1,1);
				if (prey->CurrentTeam() != hunter->CurrentTeam())
				{
					sg_deathFragWriter << prey->GetUserName() << nMachine::GetMachine(prey->Owner()).GetIP()
						<< hunter->GetUserName() << nMachine::GetMachine(hunter->Owner()).GetIP();
					sg_deathFragWriter.write();

					win.SetTemplateParameter(3, preyName);
					win << "$player_win_frag";
					if ( score_kill != 0 )
						hunter->AddScore(score_kill, win, lose );
					else
					{
						tColoredString hunterName;
						hunterName << *hunter << tColoredString::ColorString(1,1,1);
						sn_ConsoleOut( tOutput( "$player_free_frag", hunterName, preyName ) );
					}
				}
				else			 // this should never happens as the killed player is not kept as lastplayer but we can change it easily ;)
				{
					sg_deathTeamkillWriter << prey->GetUserName() << nMachine::GetMachine(prey->Owner()).GetIP()
						<< hunter->GetUserName() << nMachine::GetMachine(hunter->Owner()).GetIP();
					sg_deathTeamkillWriter.write();

					tColoredString hunterName;
					hunterName << *hunter << tColoredString::ColorString(1,1,1);
					sn_ConsoleOut( tOutput( "$player_teamkill", hunterName, preyName ) );
				}
			}
		}
		else
			target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_suicide");
	}
	else
	{
		// keep the last cycle to enter even if we're discarding this hit
		lastPlayer_ = target->Player();
	}

	//Only process the cycle kick if there is no wall inside the zone
	if (s_zoneWallInteractionFound)
		return;

	REAL r1 = this->GetRadius();
	REAL r2 = target->Player()->ping/.2;                             // the cycle "radius". accomidates for higher ping players
	REAL R = r1 + r2;
	eCoord p1 = this->Position();
	eCoord dp = p2 - p1;
	REAL c = dp.NormSquared() - R * R;
	// first find the real time and position of the impact ...
	eCoord v1 = this->GetVelocity();
	eCoord dv = v2 - v1;
	REAL a = dv.NormSquared();
	REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
	REAL delta = b*b-4*a*c;
	if (delta<0) return;		 // no t. it can't be, an impact just occured ...
	delta = sqrt(delta);
	REAL t1 = (-b-delta)/(2*a);
	REAL t2 = (-b+delta)/(2*a);
	REAL t = 0;
	if ((t1>0) && (t2>0)) return;// can't be, again ...
	if (t1>0) t = t2;
	else if (t2>0) t = t1;
	else if (t1>t2) t = t1;
	else t = t2;

	// if a wall impact happens too close, just skip cycle bouncing for now ...
	if (t < lastImpactTime_ - time) return;

	// now that we have the time, get the positions ...
	eCoord p1c = p1+v1*t;
	eCoord p2c = p2+v2*t;
	// now compute the impact : new velocities and new correct positions ...
	eCoord base = (p2c-p1c);
	base.Normalize();
	eCoord new_v1 = base*-target->Speed();
	eCoord new_p1 = p1c + new_v1*(-t+0.01);
	new_v1 = new_v1*(1+sg_ballCycleBoost*target->GetAcceleration()/100);

	s_zoneWallInteractionFound = false;
	s_zoneWallInteractionCoord = new_p1 + new_v1 * dt;
	s_zoneWallInteractionRadius = GetRadius();
	grid->ProcessWallsInRange(&S_ZoneWallIntersect,
		s_zoneWallInteractionCoord,
		s_zoneWallInteractionRadius,
		CurrentFace());

	if (s_zoneWallInteractionFound)
		return;

	SetReferenceTime();
	this->SetPosition(new_p1);
	this->SetVelocity(new_v1);
	RequestSync();
}


// *******************************************************************************
// *
// *	RemovePlayer
// *
// *******************************************************************************

void gBallZoneHack::RemovePlayer( ePlayerNetID *player )
{
	if (&*lastPlayer_ == player)
	{
		lastPlayer_ = NULL;
	}
}


// *******************************************************************************
// *
// *	GetLastPlayer
// *
// *******************************************************************************

ePlayerNetID * gBallZoneHack::GetLastPlayer()
{
	return lastPlayer_;
}


// *******************************************************************************
// *
// *	GoHome
// *
// *******************************************************************************

void gBallZoneHack::GoHome()
{
	// remove the last cycle
	lastPlayer_ = NULL;

	// put the ball at home
	SetReferenceTime();
	SetPosition(originalPosition_);
	SetVelocity(originalVelocity_);
	RequestSync();
}


// *******************************************************************************
// *
// *	gFlagZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gFlagZoneHack::gFlagZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner  )
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 1.0f;
	color_.g = 1.0f;
	color_.b = 1.0f;

	init_ = false;
	originalPosition_ = pos;
	homePosition_ = pos;
	owner_ = NULL;
	ownerTime_ = 0;
	ownerWarnedNotHome_ = false;
	chatBlinkUpdateTime_ = 0;
	blinkUpdateTime_ = 0;
	blinkTrackUpdateTime_ = 0;
	ownerDropped_ = NULL;
	ownerDroppedTime_ = 0;
	lastHoldScoreTime_ = -1;
	positionUpdatePending_ = false;

	SetExpansionSpeed(0);
	SetRotationSpeed( .3f );
	if (teamowner!=NULL) team = teamowner;
}


// *******************************************************************************
// *
// *	gFlagZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gFlagZoneHack::gFlagZoneHack( nMessage & m )
: gZone( m )
{
	init_ = false;
	originalPosition_ = pos;
	homePosition_ = pos;
	owner_ = NULL;
	ownerTime_ = 0;
	ownerWarnedNotHome_ = false;
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
// *	~gFlagZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gFlagZoneHack::~gFlagZoneHack( void )
{
	// remove the owner
	RemoveOwner();
}


// if positive, blinks chat box of the player with the flag at the time in this setting
static float sg_flagChatBlinkTime = -1;
static tSettingItem<float> sg_flagChatBlinkTimeConfig( "FLAG_CHAT_BLINK_TIME", sg_flagChatBlinkTime );

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

float sg_flagDropTime = 3;
static tSettingItem<float> sg_flagDropTimeConfig( "FLAG_DROP_TIME", sg_flagDropTime );

static bool sg_flagTeam = true;
static tSettingItem<bool> sg_flagTeamConfig( "FLAG_TEAM", sg_flagTeam );

static float sg_flagHoldScoreTime = -1;
static tSettingItem<float> sg_flagHoldScoreTimeConfig( "FLAG_HOLD_SCORE_TIME", sg_flagHoldScoreTime );

static int sg_flagHoldScore = 1;
static tSettingItem<int> sg_flagHoldScoreConfig( "FLAG_HOLD_SCORE", sg_flagHoldScore );

static float sg_flagHomeRandomnessX = -1;
static tSettingItem<float> sg_flagHomeRandomnessXConfig( "FLAG_HOME_RANDOMNESS_X", sg_flagHomeRandomnessX );

static float sg_flagHomeRandomnessY = -1;
static tSettingItem<float> sg_flagHomeRandomnessYConfig( "FLAG_HOME_RANDOMNESS_Y", sg_flagHomeRandomnessY );

static float sg_flagColorR = -1;
static tSettingItem<float> sg_flagColorRConfig( "FLAG_COLOR_R", sg_flagColorR );

static float sg_flagColorG = -1;
static tSettingItem<float> sg_flagColorGConfig( "FLAG_COLOR_G", sg_flagColorG );

static float sg_flagColorB = -1;
static tSettingItem<float> sg_flagColorBConfig( "FLAG_COLOR_B", sg_flagColorB );

// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool gFlagZoneHack::Timestep( REAL time )
{
	// initialize the zone info, this happens only once, but has to happen after construction
	if (!init_)
	{
		init_ = true;

		// save the original radius, can't do this at construction
		originalRadius_ = GetRadius();

		if ((sg_flagColorR >= 0) &&
			(sg_flagColorG >= 0) &&
			(sg_flagColorB >= 0))
		{
			color_.r = sg_flagColorR;
			color_.g = sg_flagColorG;
			color_.b = sg_flagColorB;

			if (color_.r > 1.0)
			{
				color_.r = 1.0;
			}
			if (color_.g > 1.0)
			{
				color_.g = 1.0;
			}
			if (color_.b > 1.0)
			{
				color_.b = 1.0;
			}

			RequestSync();
		}

		// if this is a team based flag, find the team
		//??? PIG - FIX to make sure every player has a flag?
		if (sg_flagTeam)
		{
			const tList<eGameObject>& gameObjects = Grid()->GameObjects();
			gCycle * closest = NULL;
			REAL closestDistance = 0;
			for (int i=gameObjects.Len()-1;i>=0;i--)
			{
				gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

				if (other )
				{
					// eTeam * otherTeam = other->Player()->CurrentTeam();
					eCoord otherpos = other->Position() - pos;
					REAL distance = otherpos.NormSquared();
					if ( !closest || distance < closestDistance )
					{
						closest = other;
						closestDistance = distance;
					}
				}
			}

			if ( closest )
			{
				// take over team and color
				team = closest->Player()->CurrentTeam();
				color_.r = team->R()/15.0;
				color_.g = team->G()/15.0;
				color_.b = team->B()/15.0;

				color_.r += (1.0 - color_.r) / 1.8;
				color_.g += (1.0 - color_.g) / 1.8;
				color_.b += (1.0 - color_.b) / 1.8;

				RequestSync();
			}

			// if this zone does not belong to a team, discard it.
			if ( !team )
			{
				return true;
			}
		}
	}

	// check if the flag is owned
	if (owner_)
	{
		ePlayerNetID *player = owner_->Player();

		if ((player) &&
			(sg_flagHoldTime > 0) &&
			(time >= (ownerTime_ + sg_flagHoldTime)))
		{
			// go home
			GoHome();

			tColoredString playerName;
			playerName << *player << tColoredString::ColorString(1,1,1);
			sn_ConsoleOut( tOutput( "$player_flag_timeout", playerName ) );
		}
	}

	// check if the flag is owned
	if (owner_)
	{
		ePlayerNetID *player = owner_->Player();

		if (player)
		{
			if ((sg_flagHoldScoreTime > 0) &&
				(sg_flagHoldScore) &&
				(time >= (lastHoldScoreTime_ + sg_flagHoldScoreTime)))
			{
				lastHoldScoreTime_ = time;

				tOutput win;
				tOutput lose;

				win << "$player_flag_hold_score_win";
				lose << "$player_flag_hold_score_lose";

				player->AddScore(sg_flagHoldScore, win, lose);
			}
		}

		if (player)
		{
			// check if flag chat blinking is enabled
			if (sg_flagChatBlinkTime > 0)
			{
				if (player->flagOverrideChat)
				{
					if (time > (chatBlinkUpdateTime_ + sg_flagChatBlinkTime))
					{
						chatBlinkUpdateTime_ = time;
						player->flagChatState = !player->flagChatState;
						player->RequestSync();
					}
				}
				else
				{
					// start the override
					chatBlinkUpdateTime_ = time;
					player->flagOverrideChat = true;
					player->flagChatState = false;
					player->RequestSync();
				}
			}
		}

		if (sg_flagBlinkTime > 0)
		{
			REAL startRadiusPercent = sg_flagBlinkStart;
			if (startRadiusPercent < 0)
			{
				startRadiusPercent = 0;
			}

			REAL endRadiusPercent = sg_flagBlinkEnd;
			if (endRadiusPercent < startRadiusPercent)
			{
				endRadiusPercent = startRadiusPercent;
			}

			// get the time the flag will be on
			REAL onTime = sg_flagBlinkTime;

			if ((sg_flagBlinkOnTime > 0) &&
				(sg_flagBlinkOnTime < onTime))
			{
				onTime = sg_flagBlinkOnTime;
			}

			if (time >= (blinkUpdateTime_ + sg_flagBlinkTime))
			{
				// start a new blink
				blinkUpdateTime_ = time;
				blinkTrackUpdateTime_ = time;

				REAL expansionSpeed =
					(originalRadius_ *
					(endRadiusPercent - startRadiusPercent)) /
					onTime;

				SetReferenceTime();

				if (sg_flagBlinkTrackTime > 0)
				{
					SetPosition(owner_->Position());
					SetVelocity(owner_->Direction() * owner_->Speed());
				}
				else
				{
					eCoord estimatedPosition =
						(owner_->Position() +
						(owner_->Direction() *
						(sg_flagBlinkEstimatePosition * owner_->Speed() * onTime)));

					SetPosition(estimatedPosition);
					SetVelocity(se_zeroCoord);
				}

				SetRadius(originalRadius_ * startRadiusPercent);
				SetExpansionSpeed(expansionSpeed);
				RequestSync();
			}
			else if (GetRadius() > 0)
			{
				if (time >= (blinkUpdateTime_ + onTime))
				{
					// kill the blink until the next update time
					SetReferenceTime();
					SetVelocity(se_zeroCoord);
					SetRadius(0);
					SetExpansionSpeed(0);
					RequestSync();
				}
				else if ((sg_flagBlinkTrackTime > 0) &&
					(time >= (blinkTrackUpdateTime_ + sg_flagBlinkTrackTime)))
				{
					// track the owner again
					SetReferenceTime();
					SetPosition(owner_->Position());
					SetVelocity(owner_->Direction() * owner_->Speed());
					RequestSync();
				}
			}
		}
	}

	// delegate
	bool returnStatus = gZone::Timestep( time );

	// any pending position updates have been made
	positionUpdatePending_ = false;

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gFlagZoneHack::OnEnter( gCycle * target, REAL time )
{
    // make sure target, player, and their team are OK
    if ((!target) ||
        (!target->Player()) ||
        (!target->Player()->CurrentTeam()))
    {
        return;
    }

    // don't process if not initialized yet or owned or updating
    if ((!init_) ||
        (owner_) ||
        (positionUpdatePending_))
    {
        return;
    }

    //check to see if player already has a flag
    bool playerHasFlag = false;
    const tList<eGameObject>& gameObjects = Grid()->GameObjects();
    for (int i=gameObjects.Len()-1;i>=0;i--)
    {
        gFlagZoneHack *otherFlag=dynamic_cast<gFlagZoneHack *>(gameObjects(i));
        if ((otherFlag)){
            if (otherFlag->Owner()){
                //make sure they are on the oposite team else pointless to continue
                if(otherFlag->Team() != target->Player()->CurrentTeam())
                {
                    ePlayerNetID *flagHolder = otherFlag->Owner()->Player();
                    if (flagHolder){
                        if (flagHolder ==  target->Player())
                        {
                playerHasFlag = true;
                        }
                    }
                }
            }
        }
    }

    // check if the player is on our team or not (check will fail if team not enabled)
    if (target->Player()->CurrentTeam() == team)
    {
        // player is on our team, if we're not at home, go back
        if (!IsHome())
        {
            // go home
            GoHome();

            tColoredString playerName;
            playerName << *target->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_return", playerName ) );
        }
    }
	// check if this player dropped the flag previously and ensure that player does not have another flag
    else if (((target != ownerDropped_) || (time > (ownerDroppedTime_ + sg_flagDropTime))) && (!playerHasFlag))
    {
        // take the flag
        owner_ = target;
        ownerTime_ = time;
        lastHoldScoreTime_ = time;
        ownerWarnedNotHome_ = false;
        owner_->flag_ = this;

        blinkUpdateTime_ = -1000;
        blinkTrackUpdateTime_ = -1000;

        // diminish the flag and put it at the original location
        SetReferenceTime();
        SetVelocity(se_zeroCoord);
        SetPosition(originalPosition_);
        SetRadius(0);
        SetExpansionSpeed(0);
        RequestSync();
        positionUpdatePending_ = true;

        tColoredString playerName;
        playerName << *target->Player() << tColoredString::ColorString(1,1,1);
        sn_ConsoleOut( tOutput( "$player_flag_take", playerName ) );
    }
}


// *******************************************************************************
// *
// *	WarnFlagNotHome
// *
// *******************************************************************************

void gFlagZoneHack::WarnFlagNotHome()
{
	// if we have an owner, warn them if we haven't already
	if ((owner_) &&
		(!ownerWarnedNotHome_))
	{
		ownerWarnedNotHome_ = true;

		if (owner_->Player())
		{
			tColoredString playerName;
			playerName << *owner_->Player() << tColoredString::ColorString(1,1,1);
			sn_ConsoleOut( tOutput( "$player_flag_cant_score", playerName ) );
		}
	}
}


// *******************************************************************************
// *
// *	IsHome
// *
// *******************************************************************************

bool gFlagZoneHack::IsHome()
{
	// flag is at home if at the original position and not owned
	if ((!owner_) &&
		(GetPosition() == homePosition_))
	{
		return (true);
	}
	else
	{
		return (false);
	}
}


// *******************************************************************************
// *
// *	GoHome
// *
// *******************************************************************************

void gFlagZoneHack::GoHome()
{
	// remove the owner
	RemoveOwner();

	// circle randomness
	#if 0
	// pick a new home position if randomness is on
	REAL randomness = sg_flagHomeRandomness * gArena::SizeMultiplier();
	if (randomness > originalRadius_)
	{
		REAL homeRadius = randomness - originalRadius_;

		tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();
		REAL randX = (randomizer.Get() * 2.0f) - 1.0f;
		REAL randY = (randomizer.Get() * 2.0f) - 1.0f;

		//??? Would a random turn and random radius be a better use?

		homePosition_.x = randX * homeRadius;
		homePosition_.y = randY * sqrtf((homeRadius * homeRadius) - (homePosition_.x * homePosition_.x));

		homePosition_.x += originalPosition_.x;
		homePosition_.y += originalPosition_.y;
	}
	#endif

	// pick a new home position if randomness is on
	REAL randomnessX = (sg_flagHomeRandomnessX * gArena::SizeMultiplier()) - originalRadius_;
	REAL randomnessY = (sg_flagHomeRandomnessY * gArena::SizeMultiplier()) - originalRadius_;

	if ((randomnessX > 0) || (randomnessY > 0))
	{
		if (randomnessX < 0)
		{
			randomnessX = 0;
		}
		if (randomnessY < 0)
		{
			randomnessY = 0;
		}

		tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();

		REAL randX = (randomizer.Get() * 2.0f) - 1.0f;
		REAL randY = (randomizer.Get() * 2.0f) - 1.0f;

		homePosition_.x = randX * randomnessX;
		homePosition_.y = randY * randomnessY;

		homePosition_.x += originalPosition_.x;
		homePosition_.y += originalPosition_.y;
	}

	// put the flag at home
	SetReferenceTime();
	SetVelocity(se_zeroCoord);
	SetPosition(homePosition_);
	SetRadius(originalRadius_);
	SetExpansionSpeed(0);
	RequestSync();
	positionUpdatePending_ = true;
}


// *******************************************************************************
// *
// *	RemoveOwner
// *
// *******************************************************************************

void gFlagZoneHack::RemoveOwner()
{
	// get rid of the owner
	if (owner_)
	{
		if (owner_->Player())
		{
			if (owner_->Player()->flagOverrideChat)
			{
				owner_->Player()->flagOverrideChat = false;
				owner_->Player()->RequestSync();
			}
		}

		owner_->flag_ = NULL;
		owner_ = NULL;
	}
}


// *******************************************************************************
// *
// *	OwnerDropped
// *
// *******************************************************************************

void gFlagZoneHack::OwnerDropped()
{
	if (owner_)
	{
		// save the last owner and drop time
		ownerDropped_ = owner_;
		ownerDroppedTime_ = lastTime;

		ePlayerNetID *player = owner_->Player();

		if (player)
		{
			//??? prints after round ends... can check if enemies are alive?
			//??? add flag for knowing when grid is getting deleted?
			tColoredString playerName;
			playerName << *player << tColoredString::ColorString(1,1,1);
			sn_ConsoleOut( tOutput( "$player_flag_drop", playerName ) );
		}

		if (sg_flagDropHome)
		{
			// this will take the flag home and remove the owner
			GoHome();
		}
		else
		{
			// put the flag at the owner's last position
			SetReferenceTime();
			SetVelocity(se_zeroCoord);
			SetPosition(owner_->Position());
			SetRadius(originalRadius_);
			SetExpansionSpeed(0);
			RequestSync();
			positionUpdatePending_ = true;

			// remove the owner
			RemoveOwner();
		}
	}
}


// *******************************************************************************
// *
// *	gTargetZoneHack settings
// *
// *******************************************************************************

// time in sec before the zone vanished. -1 for infinite
static int sg_targetTimeBeforeVanish = -1;
static tSettingItem<int> sg_targetTimeBeforeVanishConf( "TARGET_LIFETIME", sg_targetTimeBeforeVanish );

// time in sec before the zone vanished once a player entered. -1 for infinite
static int sg_targetTimeBeforeVanishOnceConquered = 10;
static tSettingItem<int> sg_targetTimeBeforeVanishOnceConqueredConf( "TARGET_SURVIVE_TIME", sg_targetTimeBeforeVanishOnceConquered );

// score for the first player entering the zone
static int sg_targetInitScore = 10;
static tSettingItem<int> sg_targetInitScoreConf( "TARGET_INITIAL_SCORE", sg_targetInitScore );

// score suppress to the zone score each time a player entered
static int sg_targetScoreDeplete = 2;
static tSettingItem<int> sg_targetScoreDepleteConf( "TARGET_SCORE_DEPLETE", sg_targetScoreDeplete );

// last target zone is a winzone ?;
static int sg_targetIsWinzone = 1;
static tSettingItem<int> sg_targetIsWinzoneConf( "TARGET_DECLARE_WINNER", sg_targetIsWinzone );

								 //!< count check zone on grid
int gTargetZoneHack::TargetZoneCounter_ = 0;
								 //!< game time when a winner can be declared if nothing happens soon
REAL gTargetZoneHack::winnerTime_ = 0;
								 //!< first player entering the last zone to be declare winner
ePlayerNetID *gTargetZoneHack::winner_ = 0;

static void sg_SetTargetCmd(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// first parse the line to get the param : object_id
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	tString event_str = params.ExtractNonBlankSubString(pos);
	tString mode_str("new");
	if (event_str=="add") {
		mode_str = tString("add");
		event_str = params.ExtractNonBlankSubString(pos);
	}
	tString cmd_str = params.SubStr(pos);
	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gTargetZoneHack *zone=dynamic_cast<gTargetZoneHack *>(gameObjects(zone_id));
		if (zone)
		{
			if (event_str=="onenter") {
				zone->SetOnEnterCmd(cmd_str, mode_str);
				con << "Zone " << zone->GOID() << " command onenter '" << cmd_str << "'\n";
			} else if (event_str=="onvanish") {
				zone->SetOnVanishCmd(cmd_str, mode_str);
				con << "Zone " << zone->GOID() << " command onvanish '" << cmd_str << "'\n";
			}
			zone_id=gZone::FindNext(object_id_str, zone_id);
		}
	}
}

static tConfItemFunc sg_SetTargetCmd_conf("SET_TARGET_COMMAND",&sg_SetTargetCmd);

// *******************************************************************************
// *
// *	gTargetZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gTargetZoneHack::gTargetZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation)
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 0.0f;
	color_.g = 1.0f;
	color_.b = 0.0f;

	TargetZoneCounter_++;
	winnerTime_ = -1;
	winner_ = 0;
	firstPlayer_ = 0;
	zoneInitialScore_ = sg_targetInitScore;
	zoneScore_ = sg_targetInitScore;
	zoneScoreDeplete_ = sg_targetScoreDeplete;
	timeFirstEntry_ = -1.0;
	targetEmptyTime_ = -1.0;
	currentState_ = State_Safe;
	for(int i=0; i<MAXCLIENTS; i++) playersFlags[i] = 0;

	grid->AddGameObjectInteresting(this);

	SetExpansionSpeed(0);
	SetRotationSpeed( .3f );
	RequestSync();
}


// *******************************************************************************
// *
// *	gTargetZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gTargetZoneHack::gTargetZoneHack( nMessage & m )
: gZone( m )
{
	TargetZoneCounter_++;
	winnerTime_ = -1;
	winner_ = 0;
	firstPlayer_ = 0;
	zoneInitialScore_ = sg_targetInitScore;
	zoneScore_ = sg_targetInitScore;
	zoneScoreDeplete_ = sg_targetScoreDeplete;
	timeFirstEntry_ = -1.0;
	targetEmptyTime_ = -1.0;
	currentState_ = State_Safe;
	for(int i=0; i<MAXCLIENTS; i++) playersFlags[i] = 0;
}


// *******************************************************************************
// *
// *	~gTargetZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gTargetZoneHack::~gTargetZoneHack( void )
{
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

static eLadderLogWriter sg_targetzoneTimeoutWriter("TARGETZONE_TIMEOUT", false);
static eLadderLogWriter sg_targetzoneConqueredWriter("TARGETZONE_CONQUERED", false);
static eLadderLogWriter sg_targetzonePlayerLeftWriter("TARGETZONE_PLAYER_LEFT", false);

bool gTargetZoneHack::Timestep( REAL time )
{
	// check if the zone must collapse ...
	if ( (currentState_ != State_Conquered) && (currentState_ != State_Conquering) && (sg_targetTimeBeforeVanish>0) && ((time - createTime_ - 3.5)>sg_targetTimeBeforeVanish) )
	{
		// zone is conquered, make the zone collapse ...
		currentState_ = State_Conquered;
		SetReferenceTime();
		SetExpansionSpeed( -GetRadius()*.5 );
		RequestSync();
		// send message to ladder log file ...
		sg_targetzoneTimeoutWriter << eGameObject::GOID() << name_ << GetPosition().x << GetPosition().y;
		sg_targetzoneTimeoutWriter.write();
	}

	if ( ((currentState_ == State_Conquering) && (sg_targetTimeBeforeVanishOnceConquered>0) && (time - timeFirstEntry_>sg_targetTimeBeforeVanishOnceConquered))
		||((currentState_ != State_Conquered) && (zoneScore_<=0) && (zoneScore_!=zoneInitialScore_)) )
	{
		// zone is conquered, make the zone collapse ...
		currentState_ = State_Conquered;
		SetReferenceTime();
		SetExpansionSpeed( -GetRadius()*.5 );
		RequestSync();
		// send message to ladder log file ...
		sg_targetzoneConqueredWriter << eGameObject::GOID() << name_ << GetPosition().x << GetPosition().y;
		if (firstPlayer_)
		{
			sg_targetzoneConqueredWriter << firstPlayer_->GetUserName();
			if ( firstPlayer_->CurrentTeam() )
			{
				sg_targetzoneConqueredWriter << ePlayerNetID::FilterName( firstPlayer_->CurrentTeam()->Name() );
			}
		}
		sg_targetzoneConqueredWriter.write();
	}

	for (int i=0; i<MAXCLIENTS; i++)
	{
		if (playersFlags[i]==2)
		{
			ePlayerNetID* p = se_PlayerNetIDs(i);
			if (p)
			{
				gCycle* prey = dynamic_cast< gCycle* >( p->Object() );
				if ( prey )
				{
					REAL r = this->GetRadius();
					if (!prey->Alive() || (( prey->Position() - this->Position() ).NormSquared() >= r*r))
					{
						sg_targetzonePlayerLeftWriter << this->GOID() << name_ << GetPosition().x << GetPosition().y << p->GetUserName() << p->Object()->Position().x << p->Object()->Position().y << p->Object()->Direction().x << p->Object()->Direction().y ;
						sg_targetzonePlayerLeftWriter.write();
						playersFlags[i] = 1;
					}
				} else playersFlags[i] = 1;
			} else playersFlags[i] = 1;
		}
	}

	// manage rotation speed
	REAL omega;
	REAL omegaDot;
	REAL maxSpeed = 10 * ( 2 * 3.141 ) / sg_segments;
	if (currentState_ != State_Conquered)
	{
		// set rotation speed accordingly to remaining points ...
		REAL conquered = (zoneInitialScore_==0?0:(REAL)(zoneInitialScore_ - zoneScore_) / (REAL)zoneInitialScore_);
		omega = .3 + conquered * conquered * maxSpeed;
		omegaDot = 2 * conquered * maxSpeed;
	}
	else
	{
		omega = .3 + maxSpeed;
		omegaDot = 2 * maxSpeed;
	}
	REAL timeStep = lastTime;
	if ( sn_GetNetState() != nSERVER )
		timeStep *= 100;

	if ( sn_GetNetState() != nCLIENT &&
		( ( fabs( omega - GetRotationSpeed() ) + fabs( omegaDot - GetRotationAcceleration() ) ) * timeStep > .5 ) )
	{
		SetRotationSpeed( omega );
		SetRotationAcceleration( omegaDot );
		SetReferenceTime();
		RequestSync();
	}

	// delegate
	bool returnStatus = gZone::Timestep( time );

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************
static eLadderLogWriter sg_targetzonePlayerEnterWriter("TARGETZONE_PLAYER_ENTER", false);

void gTargetZoneHack::OnEnter( gCycle * target, REAL time )
{
	// make sure target and player are OK
	if ((!target) ||
		(!target->Player()) ||
		(currentState_ == State_Conquered))
	{
		return;
	}

	// keep first player in memory, if it's the last zone, he is the winner ...
	if (!firstPlayer_)
	{
		firstPlayer_= target->Player();
		timeFirstEntry_ = time;
		currentState_ = State_Conquering;
		// keep winner ...
		winnerTime_ = timeFirstEntry_;
		winner_ = firstPlayer_;
		// send on enter commands ...
		std::istringstream stream(&OnEnterCmd(0));
		tCurrentAccessLevel elevator( sg_SetTargetCmd_conf.GetRequiredLevel(), true );
		tConfItemBase::LoadAll(stream);
	}

	// Check if player already entered this zone
	// If not ...
	if (!playersFlags[target->Player()->ListID()])
	{
		if (zoneScore_>0)
		{
			//  flag the player
			playersFlags[target->Player()->ListID()]=1;
			// grant the player
			tOutput win;
			tOutput lose;
			win << "$player_target_score_win";
			lose << "$player_target_score_lose";
			target->Player()->AddScore(zoneScore_, win, lose);
		}
		// and decrease zone score value
		zoneScore_-=zoneScoreDeplete_;
		if (zoneScore_<=0)
		{
			zoneScore_=0;
			targetEmptyTime_=time;
		}
	}

	// message in edlog
	if (playersFlags[target->Player()->ListID()] != 2)
	{
		sg_targetzonePlayerEnterWriter << this->GOID() << name_ << GetPosition().x << GetPosition().y << target->Player()->GetUserName() << target->Player()->Object()->Position().x << target->Player()->Object()->Position().y << target->Player()->Object()->Direction().x << target->Player()->Object()->Direction().y << time;
		sg_targetzonePlayerEnterWriter.write();
		playersFlags[target->Player()->ListID()] = 2;
	}
}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************

void gTargetZoneHack::OnVanish( void )
{
	// send on vanish commands ...
	std::istringstream stream(&OnVanishCmd(0));
	tCurrentAccessLevel elevator( sg_SetTargetCmd_conf.GetRequiredLevel(), true );
    tConfItemBase::LoadAll(stream);

	// check if we have a winner ...
	if (sg_targetIsWinzone && (TargetZoneCounter_==1) && firstPlayer_)
	{
		winnerTime_ = timeFirstEntry_;
		winner_ = firstPlayer_;
		static const char* message="$player_target_win_conquest";
		sg_DeclareWinner( firstPlayer_->CurrentTeam(), message );
	}
	// decrement target zone counter
	TargetZoneCounter_--;
}


// *******************************************************************************
// *
// *	gKOHZoneHack settings
// *
// *******************************************************************************

// score added to a player that has been in the zone for KOH_SCORE_TIME
static int sg_kohScore = 1;
static tSettingItem<int> sg_kohScoreConf( "KOH_SCORE", sg_kohScore );

// the interval that KOH_SCORE is added
static int sg_kohScoreTime = 5;
static tSettingItem<int> sg_kohScoreTimeConf( "KOH_SCORE_TIME", sg_kohScoreTime );

// *******************************************************************************
// *
// *	gKOHZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gKOHZoneHack::gKOHZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation)
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 1.0f;
	color_.g = 1.0f;
	color_.b = 1.0f;

	PlayersInside_=0;
	EnteredTime_ = -1;
    owner_=NULL;
	grid->AddGameObjectInteresting(this);

	SetExpansionSpeed(0);
	SetRotationSpeed( .5f );
	RequestSync();
}


// *******************************************************************************
// *
// *	gKOHZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gKOHZoneHack::gKOHZoneHack( nMessage & m )
: gZone( m )
{
	PlayersInside_=0;
	EnteredTime_ = -1;
    owner_=NULL;
}


// *******************************************************************************
// *
// *	~gKOHZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gKOHZoneHack::~gKOHZoneHack( void )
{
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

bool gKOHZoneHack::Timestep( REAL time )
{
    if (PlayersInside_ > 0){
        int totalInside=0;
        float r = GetRadius();
        float rr = r*r;
        for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
        {
            ePlayerNetID *player = se_PlayerNetIDs(i);
            if(!player)
            {
                continue;
            }
            gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
            if(!cycle)
            {
                continue;
            }
            if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
            {
                totalInside++;
            }
        }
        if (totalInside == 1){
            for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
            {
                ePlayerNetID *player = se_PlayerNetIDs(i);
                if(!player)
                {
                    continue;
                }
                gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
                if(!cycle)
                {
                    continue;
                }
                if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
                {
                    if (owner_ != cycle){
                        owner_=cycle;
                        color_.r = owner_->Player()->r/15.0;
                        color_.g = owner_->Player()->g/15.0;
                        color_.b = owner_->Player()->b/15.0;
                        RequestSync();
                        EnteredTime_=time;
                    }
                }
            }
        }else{
            owner_=NULL;
            EnteredTime_=-1;
            color_.r = 1.0;
            color_.g = 1.0;
            color_.b = 1.0;
            RequestSync();
        }
        PlayersInside_=totalInside;
    }

    // check if the flag is owned
	if (owner_ && PlayersInside_ ==1)
	{
		ePlayerNetID *player = owner_->Player();

		if ((player) &&
			(sg_kohScoreTime > 0) &&
			(time >= (EnteredTime_ + sg_kohScoreTime)))
		{
			tColoredString playerName;
			playerName << *player << tColoredString::ColorString(1,1,1);
            owner_->Player()->AddScore( sg_kohScore, "$player_koh_score", tOutput());
            EnteredTime_=time;

		}
	}


	// delegate
	bool returnStatus = gZone::Timestep( time );

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gKOHZoneHack::OnEnter( gCycle * target, REAL time )
{
	// make sure target and player are OK
	if ((!target) ||
		(!target->Player())|| (owner_== target))
	{
		return;
	}

	// keep first player in memory, if it's the last zone, he is the winner ...
	if (!owner_)
	{
		owner_= target;
		EnteredTime_ = time;
        PlayersInside_++;
        color_.r = target->Player()->r/15.0;
        color_.g = target->Player()->g/15.0;
        color_.b = target->Player()->b/15.0;
        RequestSync();
	}
    else
    {
        owner_= NULL;
        EnteredTime_ = -1;
        PlayersInside_++;
        color_.r = 1.0;
        color_.g = 1.0;
        color_.b = 1.0;
        RequestSync();
    }

}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************

void gKOHZoneHack::OnVanish( void )
{
	owner_= NULL;
    EnteredTime_ = -1;
    PlayersInside_=0;
    grid->RemoveGameObjectInteresting(this);
}


// *******************************************************************************
// *
// *	gTeleportZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gTeleportZoneHack::gTeleportZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation)
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 0.0f;
	color_.g = 1.0f;
	color_.b = 0.0f;

	grid->AddGameObjectInteresting(this);

	SetExpansionSpeed(0);
	SetRotationSpeed( .3f );
	RequestSync();
}


// *******************************************************************************
// *
// *	gTeleportZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gTeleportZoneHack::gTeleportZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *	~gTeleportZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gTeleportZoneHack::~gTeleportZoneHack( void )
{
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

bool gTeleportZoneHack::Timestep( REAL time )
{
	// delegate
	bool returnStatus = gZone::Timestep( time );

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gTeleportZoneHack::OnEnter( gCycle * target, REAL time )
{
	// make sure target and player are OK
	if ((!target) ||
		(!target->Player()))
	{
		return;
	}
	// Jump to new position without creating walls
	eCoord d = target->Direction();
	d.Normalize();
	eCoord n = eCoord(-d.y,d.x);
	eCoord p = target->Position();
	eCoord v = GetPosition() - p;
	REAL vn = v.Norm();
	v.Normalize();
	REAL l = (eCoord::F(v,d)*vn*2.0);
	// if center is behind, you might have been teleported in this zone. Don't jump !
	if (l<=0) return;
	eCoord newPos = (relative==1? p + jump : (relative==2? p + d*jump.x + n*jump.y: jump - v))+d*l*reloc;
	// check if newPos is inside this zone
	//if ((newPos-GetPosition()).Norm()<=GetRadius()) return;
	// if not, let's jump
	target->TeleportTo(newPos, ndir, time);
}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************

void gTeleportZoneHack::OnVanish( void )
{
}


// *******************************************************************************
// *
// *	gBlastZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gBlastZoneHack::gBlastZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation)
:gZone( grid, pos, dynamicCreation )
{
	color_.r = 0.0f;
	color_.g = 1.0f;
	color_.b = 0.0f;

	grid->AddGameObjectInteresting(this);

	SetExpansionSpeed(0);
	SetRotationSpeed( .3f );
	RequestSync();
}


// *******************************************************************************
// *
// *	gBlastZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gBlastZoneHack::gBlastZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *	~gBlastZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBlastZoneHack::~gBlastZoneHack( void )
{
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

bool gBlastZoneHack::Timestep( REAL time )
{
	// delegate
	bool returnStatus = gZone::Timestep( time );

	return (returnStatus);
}


// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gBlastZoneHack::OnEnter( gCycle * target, REAL time )
{
	target->SetWallBuilding(false);
}


// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************

void gBlastZoneHack::OnVanish( void )
{
}


// *******************************************************************************
// *
// *	Spawn_Zone
// *
// *******************************************************************************

static eLadderLogWriter sg_spawnzoneWriter("ZONE_SPAWNED", false);

static void sg_CreateZone_conf(std::istream &s)
{
	eGrid *grid = eGrid::CurrentGrid();
	if(!grid)
	{
		con << "Must be called while a grid exists!\n";
		return;
	}

	tString params;
	params.ReadLine( s, true );

	// first parse the line to get the params : <win|death> <x> <y> <size> <growth>
	eTeam *zoneTeam=NULL;
	int pos = 0;				 //
	tString zoneTypeStr = params.ExtractNonBlankSubString(pos);
	tString zoneNameStr;
	if ((zoneTypeStr=="n")||(zoneTypeStr=="N"))
	{
		zoneNameStr = params.ExtractNonBlankSubString(pos);
		zoneTypeStr = params.ExtractNonBlankSubString(pos);
	}
	else
	{
		zoneNameStr = tString("");
	}
	tString zoneTeamStr = tString("");
	if ((zoneTypeStr=="fortress")||(zoneTypeStr=="flag")||(zoneTypeStr=="deathTeam")||(zoneTypeStr=="ballTeam"))
	{
		zoneTeamStr = params.ExtractNonBlankSubString(pos);
		for (int i = eTeam::teams.Len() - 1; i>=0; --i)
		{
			tString teamName = eTeam::teams(i)->Name();
			if (zoneTeamStr==ePlayerNetID::FilterName(teamName))
			{
				zoneTeam = eTeam::teams(i);
				break;
			}
		}
	}

	ePlayerNetID *zonePlayer = 0;
	ePlayerNetID *zoneOwner = 0;
	if(zoneTypeStr=="zombie" || zoneTypeStr == "zombieOwner")
	{
		tString targetPlayer = params.ExtractNonBlankSubString(pos);
		zonePlayer = ePlayerNetID::FindPlayerByName(targetPlayer, NULL);

		if(!zonePlayer)
		{
			return;
		}
	}
	if(zoneTypeStr == "zombieOwner")
	{
		tString targetPlayer = params.ExtractNonBlankSubString(pos);
		zoneOwner = ePlayerNetID::FindPlayerByName(targetPlayer, NULL);
		if(zoneOwner)
		{
			zoneTeam = zoneOwner->CurrentTeam();
		}
	}

	tString zonePosXStr = params.ExtractNonBlankSubString(pos);
	tString zonePosYStr;

	std::vector<eCoord> route;
	if(zonePosXStr == "L")
	{
		tString x,y;
		while(true)
		{
			x = params.ExtractNonBlankSubString(pos);
			if(x == "Z" || x == "z" || x == "") break;
			y = params.ExtractNonBlankSubString(pos);
			route.push_back(eCoord(atof(x), atof(y)));
		}
	}
	else
	{
		zonePosYStr = params.ExtractNonBlankSubString(pos);
	}

	const tString zoneSizeStr = params.ExtractNonBlankSubString(pos);
	const tString zoneGrowthStr = params.ExtractNonBlankSubString(pos);
	const tString zoneDirXStr = params.ExtractNonBlankSubString(pos);
	const tString zoneDirYStr = params.ExtractNonBlankSubString(pos);
    tString  zoneRubberStr;
    REAL zoneRubber;
    if(zoneTypeStr == "rubber"){
        zoneRubberStr = params.ExtractNonBlankSubString(pos);
        zoneRubber = atof(zoneRubberStr);
        con << zoneRubber;
    }
    eCoord zoneJump;
	int relJump;
	eCoord ndir;
	REAL reloc;
    if(zoneTypeStr == "teleport"){
		tString  zoneJumpXStr;
		tString  zoneJumpYStr;
        zoneJumpXStr = params.ExtractNonBlankSubString(pos);
        zoneJumpYStr = params.ExtractNonBlankSubString(pos);
        zoneJump = eCoord(atof(zoneJumpXStr),atof(zoneJumpYStr));
        tString zoneRelAbsStr = params.ExtractNonBlankSubString(pos);
		if (zoneRelAbsStr=="") zoneRelAbsStr="rel";
		if (zoneRelAbsStr=="rel") relJump=1;
		else if (zoneRelAbsStr=="cycle") relJump=2;
		else relJump = 0;

		const tString xdir_str = params.ExtractNonBlankSubString(pos);
		REAL xdir = atof(xdir_str);
		const tString ydir_str = params.ExtractNonBlankSubString(pos);
		REAL ydir = atof(ydir_str);
		if (xdir_str == "") xdir = 0.0;
		if (ydir_str == "") ydir = 0.0;
		ndir = eCoord(xdir,ydir);
		const tString reloc_str = params.ExtractNonBlankSubString(pos);
		reloc = atof(reloc_str);
		if (reloc_str == "") reloc = 1.0;
    }
	const tString zoneInteractive = params.ExtractNonBlankSubString(pos);
	const tString zoneRedStr = params.ExtractNonBlankSubString(pos);
	const tString zoneGreenStr = params.ExtractNonBlankSubString(pos);
	const tString zoneBlueStr = params.ExtractNonBlankSubString(pos);
	const tString targetRadiusStr = params.ExtractNonBlankSubString(pos);

	// second convert params ( to do : check validity).
	eCoord zonePos = route.empty() ? eCoord(atof(zonePosXStr),atof(zonePosYStr)) : route.front();
	const REAL zoneSize = atof(zoneSizeStr);
	const REAL zoneGrowth = atof(zoneGrowthStr);
	//const REAL zoneRubber = atof(zoneRubberStr);
	eCoord zoneDir = eCoord(atof(zoneDirXStr),atof(zoneDirYStr));
	gRealColor zoneColor;
	bool setColorFlag = false;
	if ((zoneRedStr!="")&&(zoneGreenStr!="")&&(zoneBlueStr!=""))
	{
		zoneColor.r = atof(zoneRedStr);
		zoneColor.g = atof(zoneGreenStr);
		zoneColor.b = atof(zoneBlueStr);
		setColorFlag = true;
	}

	// if no_team was pass, assigns one: the one that has a player spawned closest
	if ( zoneTeamStr=="no_team" )
	{
		//REAL teamDistance_ = 0;
		const tList<eGameObject>& gameObjects = grid->GameObjects();
		gCycle * closest = 0;
		REAL closestDistance = 0;
		for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

			if (other )
			{
				//eTeam * otherTeam = other->Player()->CurrentTeam();
				eCoord otherpos = other->Position() - zonePos;
				REAL distance = otherpos.NormSquared();
				if ( !closest || distance < closestDistance )
				{
					closest = other;
					closestDistance = distance;
				}
			}
		}

		if ( closest )
		{
			// take over team
			zoneTeam = closest->Player()->CurrentTeam();
		}
	}
	// then create zone ...
	gZone *Zone = NULL;
	if ( zoneTypeStr=="death" || zoneTypeStr=="deathTeam" )
	{
		Zone = tNEW( gDeathZoneHack( grid, zonePos, true, zoneTeam ) );
		//sn_ConsoleOut( "$instant_death_activated" );
		if (zoneTeam!=NULL)
		{
			zoneColor.r = zoneTeam->R()/15.0;
			zoneColor.g = zoneTeam->G()/15.0;
			zoneColor.b = zoneTeam->B()/15.0;
			zoneColor.r += (1.0 - zoneColor.r) / 1.8;
			zoneColor.g += (1.0 - zoneColor.g) / 1.8;
			zoneColor.b += (1.0 - zoneColor.b) / 1.8;
			setColorFlag = true;
		}
	}
	else if ( zoneTypeStr=="win")
	{
		Zone = tNEW( gWinZoneHack( grid, zonePos, true ) );
		//sn_ConsoleOut( "$instant_win_activated" );
	}
	else if ( zoneTypeStr=="rubber")
	{
        gRubberZoneHack *rZone = tNEW(gRubberZoneHack(grid, zonePos, true));
		rZone->SetRubber(zoneRubber);
		Zone = rZone;
	}
	else if ( zoneTypeStr=="teleport")
	{
        gTeleportZoneHack *rZone = tNEW(gTeleportZoneHack(grid, zonePos, true));
		rZone->SetJump(zoneJump,relJump);
		rZone->SetNewDir(ndir);
		rZone->SetReloc(reloc);
		Zone = rZone;
	}
	else if ( zoneTypeStr=="ball" || zoneTypeStr=="ballTeam" )
	{
		Zone = tNEW( gBallZoneHack( grid, zonePos, true, zoneTeam ) );
		//sn_ConsoleOut( "$instant_win_activated" );
		if (zoneTeam!=NULL)
		{
			zoneColor.r = zoneTeam->R()/15.0;
			zoneColor.g = zoneTeam->G()/15.0;
			zoneColor.b = zoneTeam->B()/15.0;
			zoneColor.r += (1.0 - zoneColor.r) / 1.8;
			zoneColor.g += (1.0 - zoneColor.g) / 1.8;
			zoneColor.b += (1.0 - zoneColor.b) / 1.8;
			setColorFlag = true;
		}
	}
	else if ( zoneTypeStr=="flag" )
	{
		Zone = tNEW( gFlagZoneHack( grid, zonePos, true, zoneTeam ) );
		//sn_ConsoleOut( "$instant_win_activated" );
		if (zoneTeam!=NULL)
		{
			zoneColor.r = zoneTeam->R()/15.0;
			zoneColor.g = zoneTeam->G()/15.0;
			zoneColor.b = zoneTeam->B()/15.0;
			zoneColor.r += (1.0 - zoneColor.r) / 1.8;
			zoneColor.g += (1.0 - zoneColor.g) / 1.8;
			zoneColor.b += (1.0 - zoneColor.b) / 1.8;
			setColorFlag = true;
		}
	}
	else if ( zoneTypeStr=="fortress" )
	{
		gBaseZoneHack *fZone = tNEW( gBaseZoneHack( grid, zonePos, true, zoneTeam ) );
		// if this zone does not belong to a team, discard it.
		if ( !fZone->CheckTeamAssignment() )
		{
			fZone->RemoveFromGame();
			return;
		}
		Zone = fZone;
		//sn_ConsoleOut( "$instant_win_activated" );
		if (Zone->Team()!=NULL)
		{
			zoneColor.r = Zone->Team()->R()/15.0;
			zoneColor.g = Zone->Team()->G()/15.0;
			zoneColor.b = Zone->Team()->B()/15.0;
			setColorFlag = true;
		}
	}
	else if ( zoneTypeStr=="zombie" || zoneTypeStr=="zombieOwner" )
	{
		gCycle *target = dynamic_cast<gCycle *>(zonePlayer->Object());
		if(target)
		{
			gDeathZoneHack *dZone = tNEW(gDeathZoneHack(grid, zonePos, true));
			dZone->SetSeekingCycle(target);
			dZone->SetType(gDeathZoneHack::TYPE_ZOMBIE_ZONE);
			if(zoneOwner)
			{
				dZone->SetOwner(zoneOwner);
				if (zoneTeam!=NULL)
				{
					zoneColor.r = zoneTeam->R()/15.0;
					zoneColor.g = zoneTeam->G()/15.0;
					zoneColor.b = zoneTeam->B()/15.0;
					setColorFlag = true;
				}
			}
			Zone = dZone;
		}
		else
		{
			goto usage;
		}
	}
	else if ( zoneTypeStr=="target" )
	{
		Zone = tNEW( gTargetZoneHack( grid, zonePos, true ) );
	}
	else if ( zoneTypeStr=="blast" )
	{
		Zone = tNEW( gBlastZoneHack( grid, zonePos, true ) );
	}
    else if (zoneTypeStr=="koh"){
        Zone = tNEW( gKOHZoneHack( grid, zonePos, true ) );
    }
	else
	{
		usage:
		con << "Usage:\n"
			"SPAWN_ZONE <win|death|ball|target|blast|koh> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
			"SPAWN_ZONE rubber <x> <y> <size> <growth> <xdir> <ydir> <rubber> <interactive> <r> <g> <b> <target_size> \n"
			"SPAWN_ZONE teleport <x> <y> <size> <growth> <xdir> <ydir> <xjump> <yjump> <rel|abs> <interactive> <r> <g> <b> <target_size> \n"
			"SPAWN_ZONE <fortress|flag|deathTeam|ballTeam> <team> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
			"SPAWN_ZONE zombie <player> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
			"SPAWN_ZONE zombieOwner <player> <owner> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
			"instead of <x> <y> one can write: L <x1> <y1> <x2> <y2> [...] Z\n";
		return;
	}
	static_cast<eGameObject*>(Zone)->Timestep( se_GameTime() );
	Zone->SetReferenceTime();
	Zone->SetRadius( zoneSize );
	Zone->SetExpansionSpeed( zoneGrowth );
	Zone->SetRotationSpeed( .3f );
	Zone->SetVelocity(zoneDir);
	if (setColorFlag)
	{
		zoneColor.r = (zoneColor.r>1.0)?1.0:zoneColor.r;
		zoneColor.g = (zoneColor.g>1.0)?1.0:zoneColor.g;
		zoneColor.b = (zoneColor.b>1.0)?1.0:zoneColor.b;
		Zone->SetColor(zoneColor);
	}
	if (zoneInteractive=="true")
	{
		Zone->SetWallInteract(true);
		Zone->SetWallBouncesLeft(-1);
	}
	REAL targetRadius = atof(targetRadiusStr);
	if(targetRadius != 0)
	{
		Zone->SetTargetRadius(targetRadius);
	}
	if(!route.empty())
	{
		Zone->SetPosition(route.front());
		for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
		{
			Zone->AddWaypoint(*iter);
		}
	}
	Zone->SetName(zoneNameStr);
	Zone->RequestSync();

	sg_spawnzoneWriter << Zone->GOID() << zoneNameStr << Zone->GetPosition().x << Zone->GetPosition().y;
	sg_spawnzoneWriter.write();
}


static tConfItemFunc sg_CreateZone_c("SPAWN_ZONE",&sg_CreateZone_conf);

static eLadderLogWriter sg_collapsezoneWriter("ZONE_COLLAPSED", false);

static void sg_CollapseZone(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// first parse the line to get the param : object_id
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			zone->SetExpansionSpeed( -zone->GetRadius()*.5 );
			zone->RequestSync();
			sg_collapsezoneWriter << zone_id << object_id_str << zone->GetPosition().x << zone->GetPosition().y;
			sg_collapsezoneWriter.write();
			zone_id=gZone::FindNext(object_id_str, zone_id);
		}
	}
}

static tConfItemFunc sg_CollapseZone_conf("COLLAPSE_ZONE",&sg_CollapseZone);


static void sg_SetZoneRadius(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// parse the line to get the param : object_id, radius, speed
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	const tString radius_str = params.ExtractNonBlankSubString(pos);
	int radius = atoi(radius_str);
	const tString speed_str = params.ExtractNonBlankSubString(pos);
	REAL speed = atof(speed_str);

	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			// set new radius and speed to reach it ...
			if (speed==0)
				zone->SetRadiusSmoothly( radius );
			else
				zone->SetRadiusSmoothly( radius, speed );
			zone->RequestSync();
		}
		zone_id=gZone::FindNext(object_id_str, zone_id);
	}
}

static tConfItemFunc sg_SetZoneRadius_conf("SET_ZONE_RADIUS",&sg_SetZoneRadius);

static void sg_SetZoneRoute(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// parse the line to get the param : object_id, positions ...
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	//        const tString mode_str = params.ExtractNonBlankSubString(pos);
	std::vector<eCoord> route;
/*	tString speedstr;
	speedstr = params.ExtractNonBlankSubString(pos);
	REAL speed = atof(speedstr);*/
	tString x,y;
	while(true)
	{
		x = params.ExtractNonBlankSubString(pos);
		if(x == "") break;
		y = params.ExtractNonBlankSubString(pos);
		route.push_back(eCoord(atof(x), atof(y)));
	}

	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			eCoord zoneDir = eCoord(0,0);
			if(!route.empty())
			{
				eCoord curPos = zone->GetPosition();
				zone->AddWaypoint(curPos);
				zoneDir = route.front();
				for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
				{
					zone->AddWaypoint(*iter + curPos);
				}
			}
/*			REAL magnitude = zoneDir.Norm();
			if (speed<=0) speed = magnitude;
			if (magnitude!=0.0) zoneDir.Normalize();
			zoneDir*=speed;*/
			zone->SetVelocity(zoneDir);
			zone->RequestSync();
		}
		zone_id=gZone::FindNext(object_id_str, zone_id);
	}
}

static tConfItemFunc sg_SetZoneRoute_conf("SET_ZONE_POSITION",&sg_SetZoneRoute);

static void sg_SetZoneSpeed(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// parse the line to get the param : object_id, speed ...
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	//        const tString mode_str = params.ExtractNonBlankSubString(pos);
	tString speedstr;
	speedstr = params.ExtractNonBlankSubString(pos);
	REAL speed = atof(speedstr);

	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			eCoord zoneDir = zone->GetVelocity();
			REAL magnitude = zoneDir.Norm();
			if (speed<=0) speed = magnitude;
			if (magnitude!=0.0) zoneDir.Normalize();
			zoneDir*=speed;
			zone->SetVelocity(zoneDir);
			zone->RequestSync();
		}
		zone_id=gZone::FindNext(object_id_str, zone_id);
	}
}

static tConfItemFunc sg_SetZoneSpeed_conf("SET_ZONE_SPEED",&sg_SetZoneSpeed);

static void sg_SetZoneColor(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// parse the line to get the param : object_id, r, g, b ...
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	//        const tString mode_str = params.ExtractNonBlankSubString(pos);
	const tString zoneRedStr = params.ExtractNonBlankSubString(pos);
	const tString zoneGreenStr = params.ExtractNonBlankSubString(pos);
	const tString zoneBlueStr = params.ExtractNonBlankSubString(pos);
	gRealColor zoneColor;
	if ((zoneRedStr=="")||(zoneGreenStr=="")||(zoneBlueStr=="")) return;
	zoneColor.r = atof(zoneRedStr);
	zoneColor.g = atof(zoneGreenStr);
	zoneColor.b = atof(zoneBlueStr);

	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			zoneColor.r = (zoneColor.r>1.0)?1.0:zoneColor.r;
			zoneColor.g = (zoneColor.g>1.0)?1.0:zoneColor.g;
			zoneColor.b = (zoneColor.b>1.0)?1.0:zoneColor.b;
			zone->SetColor(zoneColor);
			zone->RequestSync();
		}
		zone_id=gZone::FindNext(object_id_str, zone_id);
	}
}

static tConfItemFunc sg_SetZoneColor_conf("SET_ZONE_COLOR",&sg_SetZoneColor);

static void sg_SetZoneExpansion(std::istream &s)
{
	tString params;
	params.ReadLine( s, true );

	// parse the line to get the param : object_id, expansion
	int pos = 0;				 //
	const tString object_id_str = params.ExtractNonBlankSubString(pos);
	const tString expansion_str = params.ExtractNonBlankSubString(pos);
	REAL expansion = atof(expansion_str);

	// first check for the name
	int zone_id;
	zone_id=gZone::FindFirst(object_id_str);
	if (zone_id==-1)
	{
		zone_id = atoi(object_id_str);
		if (zone_id==0 && object_id_str!="0") return;
	}

	const tList<eGameObject>& gameObjects = eGrid::CurrentGrid()->GameObjects();
	while (zone_id!=-1)
	{
		// get the zone ...
		gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
		if (zone)
		{
			zone->SetReferenceTime();
			zone->SetExpansionSpeed( expansion );
			zone->RequestSync();
		}
		zone_id=gZone::FindNext(object_id_str, zone_id);
	}
}

static tConfItemFunc sg_SetZoneExpansion_conf("SET_ZONE_EXPANSION",&sg_SetZoneExpansion);

