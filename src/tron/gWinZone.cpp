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
// *	gZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gZone::gZone( eGrid * grid, const eCoord & pos, bool dynamicCreation )
        :eNetGameObject( grid, pos, eCoord( 0,0 ), NULL, true ), rotation_(1,0)
{
    // store creation time
    referenceTime_ = createTime_ = lastTime = 0;

    destroyed_ = false;
    dynamicCreation_ = dynamicCreation;
    wallInteract_ = false;
    wallBouncesLeft_ = 0;
    targetRadius_ = 0;
    fallSpeed_ = 0;
    lastSeekTime_ = 0;
    pOwner_ = NULL;
    pSeekingCycle_ = NULL;
    seeking_ = false;  //??? change this to a game object, allow seeking any
    pLastWall_ = NULL;

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
    targetRadius_ = 0;
    fallSpeed_ = 0;
    lastSeekTime_ = 0;
    pOwner_ = NULL;
    pSeekingCycle_ = NULL;
    seeking_ = false;
    pLastWall_ = NULL;

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

static float sg_ballWallMod = 1;
static tSettingItem<float> conf_ballWallMod ("BALL_WALL_MOD", sg_ballWallMod);

static bool s_zoneWallInteractionFound;
static eCoord s_zoneWallInteractionCoord;
static eCoord s_zoneWallInteractionClosestCoord;
static REAL   s_zoneWallInteractionRadius;
static eWall *s_zoneWallInteractionWallPtr;

static void S_ZoneWallInteraction(eWall *pWall)
{
    //Ignore if we hit this wall last
    if (pWall == s_zoneWallInteractionWallPtr)
    {
        return;
    }

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

    //don't need this???
#if 0
    REAL radius = s_zoneWallInteractionRadius * s_zoneWallInteractionRadius - Pos1.y * Pos2.y;

    // wall too far away
    if ( radius < 0 )
        return;

//    radius = sqrt( radius );
#endif

    eCoord closestCoord = pWall->Point(alpha);

    if (!s_zoneWallInteractionFound)
    {
        s_zoneWallInteractionClosestCoord = closestCoord;
        s_zoneWallInteractionWallPtr = pWall;
        s_zoneWallInteractionFound = true;
    }
    else
    {
        if ((closestCoord - s_zoneWallInteractionCoord).NormSquared() <
            (s_zoneWallInteractionClosestCoord - s_zoneWallInteractionCoord).NormSquared())
        {
            s_zoneWallInteractionClosestCoord = closestCoord;
            s_zoneWallInteractionWallPtr = pWall;
        }
    }
}

void gZone::BounceOffPoint(eCoord dest, eCoord collide, REAL mod)
{
    //Use a simple angle deflection for now not even accounting for points that made too far in

    //??? This should back calculate the time and point at which we collided

    //Get the collide vector and normalize
    eCoord collideVec = collide - dest;
    collideVec.Normalize();

    eCoord velocity = GetVelocity();
    REAL speed = velocity.Norm();

    eCoord startVec = velocity;
    startVec.Normalize();
    startVec *= -1;

    REAL angleX = eCoord::F(collideVec, startVec);

    REAL angle = acos(angleX);

    REAL angleY = sin(angle);

    //Turn the collide vector by the angle to get the target vector
    eCoord endVec = collideVec.Turn(angleX, angleY);

    //con << "Zone collision: Old: " << startVec << " C: " << collideVec << " E1: " << endVec;

    if (((endVec.x * startVec.x) >= 0) &&
        ((endVec.y * startVec.y) >= 0))
    {
        endVec = collideVec.Turn(cos((2 * M_PI) - angle), sin((2 * M_PI) - angle));
        //con << " E2: " << endVec << '\n';
    }

    //con << " E2N: " << collideVec.Turn(angleX, angleY) << '\n';

    SetReferenceTime();
    //??? why does mod cause wall stick? (multiwall infinite stick)
    SetVelocity(endVec * speed * mod);
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

    // rotate
    REAL speed = GetRotationSpeed();
    REAL angle = ( time - lastTime ) * speed;
    // angle /= ( 1 + 2 * 3.14159 * angle/sg_segments );
    rotation_ = rotation_.Turn( cos( angle ), sin( angle ) );

    // move to new position
    REAL dt = time - referenceTime_;

    Move( eCoord( posx_( dt ), posy_( dt ) ), lastTime, time );

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
            eCoord dest(posx_(dt), posy_(dt));
            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCoord = dest;
            s_zoneWallInteractionRadius = GetRadius();
            s_zoneWallInteractionWallPtr = pLastWall_;
            grid->ProcessWallsInRange(&S_ZoneWallInteraction,
                                      s_zoneWallInteractionCoord,
                                      s_zoneWallInteractionRadius,
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
                    BounceOffPoint(dest, s_zoneWallInteractionClosestCoord, sg_ballWallMod);

                    // save the last wall pointer so we don't act on it again next time
                    //??? really should make the
                    pLastWall_ = s_zoneWallInteractionWallPtr;

                    if (wallBouncesLeft_ > 0)
                    {
                        wallBouncesLeft_--;
                    }

                    return false;
                }
            }
            else
            {
                pLastWall_ = NULL;
            }

            // only clip if zone is moving
            if ((posx_.GetSlope() != 0) || (posy_.GetSlope() != 0))
            {
                // clip movement to rim walls
                eCoord dest(posx_(dt), posy_(dt));
                tStackObject< ePoint > start(pos),stop(dest);
                // ePoint* pstart = &start;
                // ePoint* pstop = &stop;

                REAL clip = eWallRim::Clip(start,stop,0);

                if (clip < 1)
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

        bool doRequestSync = false;

        if ((GetExpansionSpeed() > 0) && (targetRadius_) && (GetRadius() >= targetRadius_))
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
    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
        alpha = .7f;
    if ( alpha <= 0 )
        return;

    alpha *= sg_zoneAlpha * sg_zoneAlphaServer;

    ModelMatrix();
    glPushMatrix();

    const REAL seglen = .2f;
    const REAL bot = 0.0f;
    const REAL top = 5.0f; // + ( lastTime - createTime_ ) * .1f;

    REAL r = Radius();
    GLfloat m[4][4]={{r*rotation_.x,r*rotation_.y,0,0},
                     {-r*rotation_.y,r*rotation_.x,0,0},
                     {0,0,top,0},
                     {pos.x,pos.y,bot,1}};

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

        for ( int i = sg_segments - 1; i>=0; --i )
        {
            REAL a = i * 2 * 3.14159 / REAL( sg_segments );
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

gWinZoneHack::gWinZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos )
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

gDeathZoneHack::gDeathZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation )
        :gZone( grid, pos, dynamicCreation )
{
    deathZoneType = TYPE_NORMAL;
    pLastShotCollision = NULL;

    color_.r = 1.0f;
    color_.g = 0.0f;
    color_.b = 0.0f;

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

void gDeathZoneHack::OnEnter( gCycle * target, REAL time )
{
    if (!dynamicCreation_)
    {
    target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_suicide");
    target->Kill();
    }
    else
    {
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
            target->Player()->AddScore(score_die, tOutput(), "$player_lose_frag");
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
// *	gBaseZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
    touchy_ = false;

    color_.r = color_.g = color_.b = 0;

    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
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

    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
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
        REAL maxSpeed = 10 * ( 2 * 3.141 ) / sg_segments;
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

        // if this zone does not belong to a team, discard it.
        if ( !team )
        {
            return true;
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
                farthest->team = NULL;
        }
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

void gBaseZoneHack::OnVanish( void )
{
    if (!team)
        return;

    CheckSurvivor();

    // kill the closest owners of the zone
    if ( currentState_ != State_Safe && ( enemies_.size() > 0 || sg_defendRate < 0 ) )
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
                tColoredString playerName;
                playerName = closest->GetColoredName();
                playerName << tColoredStringProxy(-1,-1,-1);
                sn_ConsoleOut( tOutput("$player_kill_collapse", playerName ) );
                closest->Object()->Kill();
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

void gBaseZoneHack::OnConquest( void )
{
    if ( team )
    {
        sg_basezoneConqueredWriter << ePlayerNetID::FilterName(team->Name()) << GetPosition().x << GetPosition().y;
        sg_basezoneConqueredWriter.write();
    }
    float rr = GetRadius();
    rr *= rr;
    for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i) {
        ePlayerNetID *player = se_PlayerNetIDs(i);
        if(!player) {
            continue;
        }
        gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
        if(!cycle) {
            continue;
        }
        if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr) {
            sg_basezoneConquererWriter << player->GetUserName();
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
            (*iter)->AddScore( score, win, tOutput() );
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
        for (int i=gameObjects.Len()-1;i>=0 && onlySurvivor;i--){
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
// *   OnRoundBegin
// *
// *******************************************************************************
//!
//! @return shall the hole process be repeated?
//!
// *******************************************************************************

void gBaseZoneHack::OnRoundBegin( void )
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

        // if this zone does not belong to a team, discard it.
        if ( !team )
        {
            RemoveFromGame();
            return;
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
        if ( std::find( enemies_.begin(), enemies_.end(), otherTeam ) == enemies_.end() )
            enemies_.push_back( otherTeam );
            
        lastEnemyContact_ = time;
    }

    // check if player has a flag
    if (target->flag_)
    {
        // check if the player is on our team
        if (team == otherTeam)
        {
            // search for another flag owned by our team
            bool allFlagsHome = true;

            if (sg_flagRequiredHome)
            {
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
                        }
                    }
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
    gBallZoneHack *ball = dynamic_cast<gBallZoneHack *>(target);

    if (ball)
    {
        if(currentState_ == State_Conquering || currentState_ == State_Conquered)
        {
            return; // the zone is already being conquered, no more points
        }
        // the ball entered the goal, figure out who shot it
        ePlayerNetID *lastPlayer_ = ball->GetLastPlayer();

        if (lastPlayer_)
        {
            eTeam *lastTeam = lastPlayer_->CurrentTeam();

            if (lastTeam == team)
            {
                // own team hit it in
                tColoredString playerName;
                playerName << *lastPlayer_ << tColoredString::ColorString(1,1,1);
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
                lastPlayer_->AddScore(score, win, lose);
            }

            // check if the round should end or we should respawn the ball
            if (sg_goalRoundEnd)
            {
                //static const char* message="$player_win_instant";
                //sg_DeclareWinner( lastTeam, message );

                // destroy the ball
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

gBallZoneHack::gBallZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos, false )
{
    color_.r = 1.0f;
    color_.g = 1.0f;
    color_.b = 1.0f;

    wallInteract_ = true;
    wallBouncesLeft_ = -1;
    lastPlayer_ = NULL;
    originalPosition_ = pos;

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
    originalPosition_ = pos;
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

void gBallZoneHack::OnVanish( void )
{
    grid->RemoveGameObjectInteresting(this);
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

void gBallZoneHack::OnEnter( gCycle * target, REAL time )
{
    if ((!target) || (!target->Player()))
    {
        return;
    }

    // save the last cycle to enter even if we're discarding this hit
    lastPlayer_ = target->Player();

    //Only process the cycle kick if there is no wall inside the zone
    s_zoneWallInteractionFound = false;
    s_zoneWallInteractionCoord = GetPosition();
    s_zoneWallInteractionRadius = GetRadius();
    s_zoneWallInteractionWallPtr = NULL;
    grid->ProcessWallsInRange(&S_ZoneWallInteraction,
                              s_zoneWallInteractionCoord,
                              s_zoneWallInteractionRadius,
                              CurrentFace());

    if (s_zoneWallInteractionFound)
    {
        return;
    }

    //Get the collide vector and normalize
    eCoord collideVec = GetPosition() - target->Position();
    collideVec.Normalize();

    SetReferenceTime();
    SetVelocity(collideVec * target->Speed());
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
    SetVelocity(eCoord(0,0));
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

gFlagZoneHack::gFlagZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos, false )
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
            if (otherFlag->Owner() == target ){
                playerHasFlag = true;
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
    // check if this player dropped the flag previously
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
