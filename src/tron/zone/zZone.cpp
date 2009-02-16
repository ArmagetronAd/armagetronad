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

#include "zone/zZone.h"

std::deque<zZone *> sz_Zones;

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

zZone::zZone( nMessage & m )
        :eNetGameObject( m ),
        //rotation_(1,0),
        playersInside(),
        playersOutside(),
        oldFortressAutomaticAssignmentBehavior_(false),
        name_()
{
    // read creation time
    m >> createTime_;
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


    /*
    shape = zShapePtr(new zShapeCircle(Grid(), ID()));

    tFunction asdf = tFunction();
    asdf.SetSlope(0.0);
    asdf.SetOffset(100.0);
    shape->setPosX(asdf);
    shape->setPosY(asdf);
    shape->setScale(asdf);
    shape->setRotation(asdf);
    shape->setScale(asdf);
    shape->setColor(rColor(0.7, 0.0, 0.7));
    zShapeCircle *circle = dynamic_cast<zShapeCircle *>( (zShape*)shape );

    circle->emulatingOldZone_ = true;
    */

    /*
    if (!m.End() && sz_ShapedZones.Supported() ) 
      {
	// Factory to make the shapes
	// HACK 
	// This makes them static for the life of the zone
	// rather than to re-send them all at each updates
	typedef zShape* (*shapeFactory)(nMessage &);
	std::map<tString, shapeFactory> shapes;
	// Build the list of supported shapes
	shapes[tString("circle")] = zShapeCircle::create;
	shapes[tString("polygon")] = zShapePolygon::create;

	// Get the name of the shape from the network
	tString shapeName;
	m >> shapeName;
	
	std::map<tString, shapeFactory>::const_iterator iterShapeFactory;
	// Build that shape if it is available
	if((iterShapeFactory = shapes.find(shapeName)) != shapes.end()) 
	  {
	    shape = zShapePtr((*(iterShapeFactory->second))(m));
	  }
      }
    else
      {
	// Didnt receive a shape information. Assume we are talking to a 0.2.8- server
	shape = zShapePtr(new zShapeCircle());
	shape->setPosX(posx_);
	shape->setPosY(posy_);
	shape->setScale(scale_);
	tPolynomial<nMessage> tpRotationSpeed(2);
	tpRotationSpeed[0] = rotationSpeed_.GetOffset();
	tpRotationSpeed[0] = rotationSpeed_.GetOffset();
	shape->setRotation2(tpRotationSpeed);
	shape->setScale(scale_);
	shape->setColor(color_);
	
	emulateOldZoneShape = true;
      }
*/

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

    /*
    if(!emulateOldZoneShape && sz_ShapedZones.Supported() )
      shape->WriteCreate( m );
    */
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

    /*
    // write color
    m << color_.r_;
    m << color_.g_;
    m << color_.b_;

    // write reference time and functions
    m << referenceTime_;
    m << posx_;
    m << posy_;
    m << scale_;

    // write rotation speed
    m << rotationSpeed_;
    */

    // Simply populate the message with semi-meaningful data
    m << shape->getColor().r_;
    m << shape->getColor().g_;
    m << shape->getColor().b_;

    m << referenceTime_;

    m << shape->getPosX();
    m << shape->getPosY();
    m << shape->getScale();
    // shape->getRotation no longer exist. Fake it!
    tFunction tfFakedRotation;
    m << tfFakedRotation;

    m << shape->ID();
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

    if(shape && shape->isEmulatingOldZone())
    {
        // read color
        rColor aColor(0.0, 0.7, 0.5);
        if (!m.End())
        {
            m >> aColor.r_;
            m >> aColor.g_;
            m >> aColor.b_;
        }
        se_MakeColorValid(aColor.r_, aColor.g_, aColor.b_, 1.0f);
        shape->setColor(aColor);

        // read reference time and functions
        if (!m.End())
        {
            m >> referenceTime_;
            shape->setReferenceTime(referenceTime_);
            if (!m.End())
            {
                tFunction aFunc;
                m >> aFunc;
                shape->setPosX(aFunc);
                m >> aFunc;
                shape->setPosY(aFunc);
                m >> aFunc;
                shape->setScale(aFunc);
            }
        }
        else
        {
            // Uses values from the eNetGameObject
            referenceTime_ = createTime_;
            shape->setReferenceTime(referenceTime_);

            tFunction aFunc;
            aFunc.SetOffset( pos.x );
            aFunc.SetSlope( 0 );
            shape->setPosX( aFunc );

            aFunc.SetOffset( pos.y );
            aFunc.SetSlope( 0 );
            shape->setPosY( aFunc );

            aFunc.SetOffset( sg_initialSize );
            aFunc.SetSlope( sg_expansionSpeed );
            shape->setScale( aFunc );
        }

        // read rotation speed
        tFunction rotationSpeed;
        if (!m.End())
        {
            m >> rotationSpeed;
        }
        else
        {
            // set fixed values
            rotationSpeed.SetOffset( .3f );
            rotationSpeed.SetSlope( 0.0f );
        }
	tPolynomial<nMessage> tpRotationSpeed(2);
	tpRotationSpeed[0] = rotationSpeed.GetOffset();
	tpRotationSpeed[1] = rotationSpeed.GetSlope();
        shape->setRotation2(tpRotationSpeed);
    }
    else
    {
        bool b;

        //        while(!m.End() ) { m >> b;}

        int count=44-16;
        // Discard the information
        while(!m.End() && count-->0) { m >> b;}
        unsigned short shapeID;
        m >> shapeID;
        if(sn_netObjects[shapeID]) {
            // zShape *asdf = dynamic_cast<zShape*>(&*sn_netObjects[shapeID]);
            shape = zShapePtr( dynamic_cast<zShape*>(&*sn_netObjects[shapeID]) );
        }

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
    if(0 != shape) {
        shape->TimeStep( time );
    }
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
                    // avoid the OnEnter transition. This happens at game
                    // start-up for example, when players are neither inside nor outside
                    if( playersOutside.find(prey->Player()) != playersOutside.end() )
                    {
                        // Passing from outside to inside triggers the OnEnter event
                        OnEnter( prey, time );
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
                    // avoid OnLeave transition. This happens at game
                    // start-up for example, when players are neither inside nor outside
                    if( playersInside.find(prey->Player()) != playersInside.end() )
                    {
                        // Passing from inside to outside triggers the OnLeave event
                        OnLeave( prey, time );
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
tPolynomial<nMessage> tpOne(asdf, sizeof(asdf)/sizeof(asdf[0]));
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
static nNOInitialisator<zZone> zone_init(341,"zonev2");

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
// *	Render
// *
// *******************************************************************************
//!
//!		@param	cam  the camera used for rendering
//!
// *******************************************************************************

void zZone::Render( const eCamera * cam )
{
    if (shape)
        shape->render(cam);
}

void zZone::Render2D( tCoord scale ) const {
    if (shape)
        shape->render2d(scale);
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


zZoneExtManager::FactoryList zZoneExtManager::_factories;

zZone*
zZoneExtManager::Create(std::string const & typex, eGrid*grid)
{
    std::string type = typex;
    transform (type.begin(), type.end(), type.begin(), tolower);

    FactoryList::const_iterator iterFactory;
    if ((iterFactory = _factories.find(type)) == _factories.end())
        return NULL;
    
    return iterFactory->second(grid, type);
}

void
zZoneExtManager::Register(std::string const & type, std::string const & desc, NamedFactory_t f)
{
    _factories[type] = f;
}

static zZoneExtRegistration regBasic("", "", zZone::create);

