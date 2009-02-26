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
#include "tPolynomial.h"


#include <time.h>
#include <map>
#include <algorithm>
#include <functional>
#include <deque>
#include <iterator>

#include "gWinZone.h"

#include "nProtoBuf.h"
#include "zZone.pb.h"

#include "zone/zZone.h"

std::deque<zZone *> sz_Zones;

// number of segments to render a zone with
static const int sg_segments = 11;

#ifndef ENABLE_ZONESV1
REAL sg_expansionSpeed = 1.0f;
REAL sg_initialSize = 5.0f;
#endif

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
        //        rotation_(1,0),
        effectGroupEnter(),
        effectGroupInside(),
        effectGroupLeave(),
        effectGroupOutside(),
        playersInside(),
        playersOutside(),
        oldFortressAutomaticAssignmentBehavior_(false),
        name_()
{
    // store creation time
    referenceTime_ = createTime_ = lastTime = 0;

    // add to game grid
    this->AddToList();

    sz_Zones.push_back(this);

    // initialize position functions
    //    SetPosition( pos );
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->PushButton(ZONE_SPAWN, pos);
}

static nVersionFeature sz_ShapedZones(20);

// *******************************************************************************
// *
// *	zZone
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!
// *******************************************************************************

zZone::zZone( Zone::ZoneSync const & sync, nSenderInfo const & sender )
        :eNetGameObject( sync.base(), sender ),
        //rotation_(1,0),
        playersInside(),
        playersOutside(),
        oldFortressAutomaticAssignmentBehavior_(false),
        name_()
{
    // read creation time
    createTime_ = sync.create_time();
    referenceTime_ = lastTime = createTime_;

    // initialize color to white, ReadSync will fill in the true value if available
    //    color_.r_ = color_.g_ = color_.b_ = 1.0f;

    // add to game grid
    this->AddToList();

    sz_Zones.push_back(this);

    // initialize position functions
    //    SetPosition( pos );
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
    RemoveFromZoneList();
}

void zZone::RemoveFromGame(void) {
    RemoveFromZoneList();
    eNetGameObject::RemoveFromGame();
}

void zZone::RemoveFromZoneList(void) {
    std::deque<zZone *>::iterator pos_found =
        std::find_if(
            sz_Zones.begin(),
            sz_Zones.end(),
            std::bind2nd(
                std::equal_to<zZone *>(),
                this)
        );
    if(pos_found != sz_Zones.end())
        sz_Zones.erase(pos_found);
}

void
zZone::setupVisuals(gParser &)
{
}
void
zZone::readXML(tXmlParser::node const &)
{
    // FIXME: Maybe move the entire zone parsing code in here? :D
}


// BEGIN DEPRECATED METHODS

REAL zZone::GetRotationSpeed() {
    tASSERT(shape);
    return shape->GetRotationSpeed();
}

void zZone::SetRotationSpeed(REAL r) {
    tASSERT(shape);
    shape->SetRotationSpeed(r);
}

REAL zZone::GetRotationAcceleration() {
    tASSERT(shape);
    return shape->GetRotationAcceleration();
}

void zZone::SetRotationAcceleration(REAL r) {
    tASSERT(shape);
    shape->SetRotationAcceleration(r);
}

// SetReferenceTime BELOW....

// END OF DEPRECATED METHODS


// *******************************************************************************
// *
// *	ClearToTransmit
// *
// *******************************************************************************
//!
//!		@param	user the sync message is intended for
//!
// *******************************************************************************

bool zZone::ClearToTransmit( int user ) const
{
    tASSERT( shape );
    if( !shape->HasBeenTransmitted( user ) )
    {
        return false;
    }

    // falback
    return eNetGameObject::ClearToTransmit( user );
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

void zZone::WriteSync(  Zone::ZoneSync & sync, bool init ) const
{
    eNetGameObject::WriteSync( *sync.mutable_base(), init );

    if ( init )
    {
        sync.set_create_time( createTime_ );

        sync.set_shape_id( nNetObject::PointerToID( shape ) );
    }
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

void zZone::ReadSync( Zone::ZoneSync const & sync, nSenderInfo const & sender )
{
    // delegage
    eNetGameObject::ReadSync( sync.base(), sender );

    if( sync.has_shape_id() )
    {
        IDToPointer( sync.shape_id(), shape );
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
    /*
       if(!emulateOldZoneShape) {
           shape->TimeStep( time );
       }
       else { // Old representation of zone
       // rotate
       REAL speed = GetRotationSpeed();
       REAL angle = ( time - lastTime ) * speed;
       // angle /= ( 1 + 2 * 3.14159 * angle/sg_segments );
       rotation_ = rotation_.Turn( cos( angle ), sin( angle ) );

       // move to new position
       REAL dt = time - referenceTime_;
       Move( eCoord( posx_( dt ), posy_( dt ) ), lastTime, time );
       

       // kill this zone if it shrunk down to zero scale
       if ( GetExpansionSpeed() < 0 && GetScale() <= 0 )
       {
           OnVanish();
           return true;
       }
       }
       // update time
       */
    lastTime = time;

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
    if ( prey && 0 != shape)
    {
        if ( prey->Player() && prey->Alive() )
        {
            // Is the player inside or outside the zone
            if ( shape->isInteracting(target) == true )
            {
                // If the player is not on the "inside" list, then he was outside
                std::set<ePlayerNetID *>::iterator iter;
                if ((iter = playersInside.find(prey->Player()) ) == playersInside.end()) {
                    playersInside.insert(prey->Player());

                    // Should the player not be marked as being outside
                    // avoid the OnEntry transition. This happens at game
                    // start-up for example, when players are neither inside nor outside
                    if( playersOutside.find(prey->Player()) != playersOutside.end() )
                    {
                        // Passing from outside to inside triggers the OnEntry event
                        OnEntry( prey, time );
                    }
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

                    // Should the player not be marked as being inside
                    // avoid OnExit transition. This happens at game
                    // start-up for example, when players are neither inside nor outside
                    if( playersInside.find(prey->Player()) != playersInside.end() )
                    {
                        // Passing from inside to outside triggers the OnExit event
                        OnExit( prey, time );
                    }
                    // The player is no longer inside
                    playersInside.erase(prey->Player());
                }
                // Being inside gives the OnOutside event
                OnOutside( prey, time );
            }
        }
    }
}

REAL asdf[] = {0, 1};
tPolynomial tpOne(asdf, sizeof(asdf)/sizeof(asdf[0]));
// *******************************************************************************
// *
// *	OnEntry
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************
void zZone::OnEntry( gCycle * target, REAL time )
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
        (*iter)->apply(triggerer, time, tpOne);
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
        (*iter)->apply(triggerer, time, tpOne);
    }
}
void zZone::OnExit( gCycle * target, REAL time )
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
        (*iter)->apply(triggerer, time, tpOne);
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
        (*iter)->apply(triggerer, time, tpOne);
    }
}

// the zone's network initializator
static nNetObjectDescriptor< zZone, Zone::ZoneSync > zone_init( 341 );

// *******************************************************************************
// *
// *	DoGetDescriptor
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nNetObjectDescriptorBase const & zZone::DoGetDescriptor() const
{
    return zone_init;
}

/*
// *******************************************************************************
// *
// *	Scale
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

REAL zZone::Scale( void ) const
{
    //    return GetScale();
    return shape->getScale();
}
*/


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
    // HACK, to be implemented later and differently
    // Should get this info from the shape, not the zone
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
    if(0 != shape) {
        position.x = EvaluateFunctionNow( shape->getPosX() );
	position.y = EvaluateFunctionNow( shape->getPosY() );
    }
    return *this;
}

// *******************************************************************************
// *
// *	GetScale
// *
// *******************************************************************************
//!
//!		@return		the current scale
//!
// *******************************************************************************

REAL zZone::GetScale( void ) const
{
    //    REAL ret = EvaluateFunctionNow( this->scale_ );
    //    ret = ret > 0 ? ret : 0;


    // HACK, to be implemented later and differently
    // Should get this info from the shape, not the zone
    REAL scale = 0.0;
    if(0 != shape) {
        scale = EvaluateFunctionNow( shape->getScale() ) ;
    }
    return scale;
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
    /*
      this->posx_.SetOffset( EvaluateFunctionNow( this->posx_ ) );
      this->posy_.SetOffset( EvaluateFunctionNow( this->posy_ ) );
      this->scale_.SetOffset( EvaluateFunctionNow( this->scale_ ) );
      this->rotationSpeed_.SetOffset( EvaluateFunctionNow( this->rotationSpeed_ ) );
    */

    // FIXME: zZone didn't originally do this, but it is added for compat w/
    //         Zones v1 porting; nothing in zones v2 seems to actually use this
    //         function
    if (shape)
        shape->setReferenceTime(lastTime);

    // reset time
    this->referenceTime_ = lastTime;
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

tCoord const zZone::GetRotation( void ) const
{
    // HACK, to be implemented later and differently
    // Should get this info from the shape, not the zone
    return tCoord(0.0, 0.0);
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

rColor const  zZone::GetColor( void ) const
{
    rColor color;
    if(0 != shape) {
        color = shape->getColor();
    }
    return color;
}


zZoneExtManager::FactoryList &
zZoneExtManager::_factories()
{
    static FactoryList _ifl;
    return _ifl;
}

zZone*
zZoneExtManager::Create(std::string const & typex, eGrid*grid)
{
    std::string type = typex;
    transform (type.begin(), type.end(), type.begin(), tolower);

    FactoryList::const_iterator iterFactory;
    if ((iterFactory = _factories().find(type)) == _factories().end())
        return NULL;
    
    return iterFactory->second(grid, type);
}

void
zZoneExtManager::Register(std::string const & type, std::string const & desc, NamedFactory_t f)
{
    _factories().insert(std::make_pair(type, f));
}

static zZoneExtRegistration regBasic("", "", zZone::create);

