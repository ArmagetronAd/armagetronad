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

#include "zone/zZone.h"
#include "eFloor.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
//#include "eTeam.h"
//#include "ePlayer.h"
#include "rRender.h"
#include "nConfig.h"
#include "tString.h"
#include "rScreen.h"
#include "eSoundMixer.h"


#include <time.h>
#include <algorithm>
#include <functional>
#include <deque>
#include <iterator>

std::deque<zZone *> sg_Zones;

// number of segments to render a zone with
static const int sg_segments = 11;

static REAL sg_expansionSpeed = 1.0f;
static REAL sg_initialSize = 5.0f;

static nSettingItem< REAL > sg_expansionSpeedConf( "WIN_ZONE_EXPANSION", sg_expansionSpeed );
static nSettingItem< REAL > sg_initialSizeConf( "WIN_ZONE_INITIAL_SIZE", sg_initialSize );



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

inline REAL zZone::EvaluateFunctionNow( tFunction const & f ) const
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

inline void zZone::SetFunctionNow( tFunction & f, REAL value ) const
{
    f.SetOffset( value + f.GetSlope() * ( referenceTime_ - lastTime ) );
}

// *******************************************************************************
// *
// *	zZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************
zZone::zZone( eGrid * grid )
        :eNetGameObject( grid, eCoord(0,0), eCoord(0,0), NULL, true ), 
	 rotation_(1,0),
	 effectGroupEnter(),
	 effectGroupInside(),
	 effectGroupLeave(),
	 effectGroupOutside(),
	 playersInside(),
	 playersOutside()
{
    // store creation time
    referenceTime_ = createTime_ = lastTime = 0;

    // add to game grid
    this->AddToList();

    sg_Zones.push_back(this);

    // initialize position functions
    SetPosition( pos );
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->PushButton(ZONE_SPAWN, pos);
}

// *******************************************************************************
// *
// *	zZone
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!
// *******************************************************************************

zZone::zZone( nMessage & m )
  :eNetGameObject( m ), rotation_(1,0),
   playersInside(),
   playersOutside()
{
    // read creation time
    m >> createTime_;
    referenceTime_ = lastTime = createTime_;

    // initialize color to white, ReadSync will fill in the true value if available
    color_.r_ = color_.g_ = color_.b_ = 1.0f;

    // add to game grid
    this->AddToList();

    sg_Zones.push_back(this);

    // initialize position functions
    SetPosition( pos );
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->PushButton(ZONE_SPAWN, pos);
}

// *******************************************************************************
// *
// *	~zZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

zZone::~zZone( void )
{
    sg_Zones.erase(
        std::find_if(
            sg_Zones.begin(),
            sg_Zones.end(),
            std::bind2nd(
                std::equal_to<zZone *>(),
                this)
        )
    );
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

void zZone::WriteCreate( nMessage & m )
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

void zZone::WriteSync( nMessage & m )
{
    // delegate
    eNetGameObject::WriteSync( m );

    // write color
    m << color_.r_;
    m << color_.g_;
    m << color_.b_;

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

void zZone::ReadSync( nMessage & m )
{
    // delegage
    eNetGameObject::ReadSync( m );

    // read color
    if (!m.End())
    {
        m >> color_.r_;
        m >> color_.g_;
        m >> color_.b_;
        se_MakeColorValid(color_.r_, color_.g_, color_.b_, 1.0f);
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

// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool zZone::Timestep( REAL time )
{
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
        return true;
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

void zZone::OnVanish( void )
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

void zZone::InteractWith( eGameObject * target, REAL time, int recursion )
{
    gCycle* prey = dynamic_cast< gCycle* >( target );
    if ( prey )
    {
        REAL r = this->Radius();
	if ( prey->Player() && prey->Alive() )
	  {
	    // Is the player inside or outside the zone
	    if ( ( prey->Position() - this->Position() ).NormSquared() < r*r )
	      {
		// If the player is not on the "inside" list, then he was outside
		std::set<ePlayerNetID *>::iterator iter;
		std::set<ePlayerNetID *>::iterator iterEnd;
		ePlayerNetID * aaa = prey->Player();
		iter = playersInside.find(aaa) ;
		iterEnd = playersInside.end();
		if ((iter = playersInside.find(prey->Player()) ) == playersInside.end()) {
		  playersInside.insert(prey->Player());
		  // Passing from outside to inside triggers the OnEnter event
		  OnEnter( prey, time );
		  // The player is no longer outside
		  playersOutside.erase(prey->Player());
		}
		// Being inside gives the OnInside event
		OnInside( prey, time );
            }
	    else {
		// If the player is not on the "outside" list, then he was inside
		std::set<ePlayerNetID *>::iterator iter;
		if ((iter = playersOutside.find(prey->Player())) == playersOutside.end()) {
		  playersOutside.insert(prey->Player());
		  // Passing from inside to outside triggers the OnLeave event
		  OnLeave( prey, time );
		  // The player is no longer inside
		  playersInside.erase(prey->Player());
		}
		// Being inside gives the OnOutside event
		OnOutside( prey, time );
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
void zZone::OnEnter( gCycle * target, REAL time )
{
  Triggerer triggerer;
  triggerer.who = target;
  triggerer.positive = _ignore;
  triggerer.marked = _ignore;

  zEffectGroupPtrs::const_iterator iter;
  for (iter = effectGroupEnter.begin();
       iter != effectGroupEnter.end();
       ++iter)
    {
      (*iter)->OnEnter(triggerer, time);
    }
}

void zZone::OnInside( gCycle * target, REAL time )
{
  Triggerer triggerer;
  triggerer.who = target;
  triggerer.positive = _ignore;
  triggerer.marked = _ignore;

  zEffectGroupPtrs::const_iterator iter;
  for (iter = effectGroupInside.begin();
       iter != effectGroupInside.end();
       ++iter)
    {
      (*iter)->OnInside(triggerer, time);
    }
}
void zZone::OnLeave( gCycle * target, REAL time )
{
  Triggerer triggerer;
  triggerer.who = target;
  triggerer.positive = _ignore;
  triggerer.marked = _ignore;

  zEffectGroupPtrs::const_iterator iter;
  for (iter = effectGroupLeave.begin();
       iter != effectGroupLeave.end();
       ++iter)
    {
      (*iter)->OnLeave(triggerer, time);
    }
}
void zZone::OnOutside( gCycle * target, REAL time )
{
  Triggerer triggerer;
  triggerer.who = target;
  triggerer.positive = _ignore;
  triggerer.marked = _ignore;

  zEffectGroupPtrs::const_iterator iter;
  for (iter = effectGroupOutside.begin();
       iter != effectGroupOutside.end();
       ++iter)
    {
      (*iter)->OnOutside(triggerer, time);
    }
}

// the zone's network initializator
static nNOInitialisator<zZone> zone_init(340,"zone");

// *******************************************************************************
// *
// *	CreatorDescriptor
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nDescriptor & zZone::CreatorDescriptor( void ) const
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

REAL zZone::Radius( void ) const
{
    return GetRadius();
}

// *******************************************************************************
// *
// *	Render
// *
// *******************************************************************************
//!
//!		@param	cam  the camera used for rendering
//!
// *******************************************************************************

void zZone::Render( const eCamera * cam )
{
#ifndef DEDICATED

    color_.a_ = ( lastTime - createTime_ ) * .2f;
    if ( color_.a_ > .7f )
        color_.a_ = .7f;
    if ( color_.a_ <= 0 )
        return;

    GLfloat m[4][4]={{rotation_.x,rotation_.y,0,0},
                     {-rotation_.y,rotation_.x,0,0},
                     {0,0,1,0},
                     {pos.x,pos.y,0,1}};

    ModelMatrix();
    glPushMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    //glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);

    //	glTranslatef(pos.x,pos.y,0);

    glMultMatrixf(&m[0][0]);
    //	glScalef(.5,.5,.5);

    if ( sr_alphaBlend )
        BeginQuads();
    else
        BeginLineStrip();

    const REAL seglen = .2f;
    const REAL bot = 0.0f;
    const REAL top = 5.0f; // + ( lastTime - createTime_ ) * .1f;

    color_.Apply();

    REAL r = Radius();
    for ( int i = sg_segments - 1; i>=0; --i )
    {
        REAL a = i * 2 * 3.14159 / REAL( sg_segments );
        REAL b = a + seglen;

        REAL sa = r * sin(a);
        REAL ca = r * cos(a);
        REAL sb = r * sin(b);
        REAL cb = r * cos(b);

        glVertex3f(sa, ca, bot);
        glVertex3f(sa, ca, top);
        glVertex3f(sb, cb, top);
        glVertex3f(sb, cb, bot);

        if ( !sr_alphaBlend )
        {
            glVertex3f(sa, ca, bot);
            RenderEnd();
            BeginLineStrip();
        }
    }

    RenderEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask(GL_TRUE);

    glPopMatrix();
#endif
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

eCoord zZone::GetPosition( void ) const
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

zZone const & zZone::GetPosition( eCoord & position ) const
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

zZone & zZone::SetPosition( eCoord const & position )
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

eCoord zZone::GetVelocity( void ) const
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

zZone const & zZone::GetVelocity( eCoord & velocity ) const
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

zZone & zZone::SetVelocity( eCoord const & velocity )
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

REAL zZone::GetRadius( void ) const
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

zZone const & zZone::GetRadius( REAL & radius ) const
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

zZone & zZone::SetRadius( REAL radius )
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

REAL zZone::GetExpansionSpeed( void ) const
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

zZone const & zZone::GetExpansionSpeed( REAL & expansionSpeed ) const
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

zZone & zZone::SetExpansionSpeed( REAL expansionSpeed )
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

void zZone::SetReferenceTime( void )
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

REAL zZone::GetRotationSpeed( void ) const
{
    return EvaluateFunctionNow( rotationSpeed_ );
}

// *******************************************************************************
// *
// *	GetRotation
// *
// *******************************************************************************
//!
//!		@return		The current rotation position, as normalized x and y coordinates
//!
// *******************************************************************************

tCoord const &zZone::GetRotation( void ) const
{
    return rotation_;
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

zZone const & zZone::GetRotationSpeed( REAL & rotationSpeed ) const
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

zZone & zZone::SetRotationSpeed( REAL rotationSpeed )
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

REAL zZone::GetRotationAcceleration( void ) const
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

zZone const & zZone::GetRotationAcceleration( REAL & rotationAcceleration ) const
{
    rotationAcceleration = this->GetRotationAcceleration();
    return *this;
}

// *******************************************************************************
// *
// *	GetColor
// *
// *******************************************************************************
//!
//!		@return		the current color of the zone
//!
// *******************************************************************************

rColor const & zZone::GetColor( void ) const
{
    return color_;
}

// *******************************************************************************
// *
// *	SetColor
// *
// *******************************************************************************
//!
//!		@return		the current color of the zone
//!
// *******************************************************************************

void zZone::SetColor( rColor const & color ) 
{
    color_ = color;
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

zZone & zZone::SetRotationAcceleration( REAL rotationAcceleration )
{
    REAL omega = this->GetRotationSpeed();
    this->rotationSpeed_.SetSlope( rotationAcceleration );
    SetRotationSpeed( omega );

    return *this;
}

