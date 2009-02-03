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

#ifndef ArmageTron_Zone_H
#define ArmageTron_Zone_H

#include "eNetGameObject.h"

#include "gCycle.h"
#include "rColor.h"
#include <boost/shared_ptr.hpp>
#include "zone/zEffectGroup.h"
#include "tFunction.h"
#include <set>
#include <vector>
#include "zone/zShape.hpp"

namespace Zone { class ZoneSync; }

/*
class zZone: public eNetGameObject
{
    zZone(eGrid *grid); //!< local constructor
    zZone(nMessage &m);                    //!< network constructor
    ~zZone();                              //!< destructor

    void SetReferenceTime();               //!< sets the reference time to the current time

 protected:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnVanish();                     //!< called when the zone vanishes
private:
    virtual void WriteCreate(nMessage &m); //!< writes data for network constructor
    virtual void WriteSync(nMessage &m);   //!< writes sync data
    virtual void ReadSync(nMessage &m);    //!< reads sync data

    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them
    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network

    virtual void Render(const eCamera *cam);  //!< renders the zone
}
*/

class zZone: public eNetGameObject
{
public:
    zZone(eGrid *grid); //!< local constructor
    ~zZone();                              //!< destructor
    void RemoveFromGame();		   //!< call this instead of the destructor

    //! creates a netobject form sync data
    zZone( Zone::ZoneSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Zone::ZoneSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Zone::ZoneSync & sync, bool init ) const;

    // we must not transmit an object that contains pointers
    // to non-transmitted objects. this function is supposed to check that.
    virtual bool ClearToTransmit( int user ) const;

    void SetReferenceTime();               //!< sets the reference time to the current time

    eCoord          GetPosition         ( void ) const;	                //!< Gets the current position
    zZone const &   GetPosition         ( eCoord & position ) const;	//!< Gets the current position
    tCoord const    GetRotation         ( void ) const;	                //!< Gets the current rotation state
    REAL            GetScale           ( void ) const;	                //!< Gets the current scale
    rColor const    GetColor( void ) const;	//!< Gets the current color
    /*
    zZone &         SetPosition         ( eCoord const & position );	//!< Sets the current position
    zZone &         SetVelocity         ( eCoord const & velocity );	//!< Sets the current velocity
    eCoord          GetVelocity         ( void ) const;	                //!< Gets the current velocity
    zZone const &   GetVelocity         ( eCoord & velocity ) const;	//!< Gets the current velocity
    zZone &         SetScale           ( REAL scale );	            //!< Sets the current scale
    REAL            GetScale           ( void ) const;	                //!< Gets the current scale
    zZone const &   GetScale           ( REAL & scale ) const;	    //!< Gets the current scale
    zZone &         SetExpansionSpeed   ( REAL expansionSpeed );	    //!< Sets the current expansion speed
    REAL            GetExpansionSpeed   ( void ) const;	                //!< Gets the current expansion speed
    zZone const &   GetExpansionSpeed   ( REAL & expansionSpeed ) const;//!< Gets the current expansion speed
    zZone &         SetRotationSpeed    ( REAL rotationSpeed );	        //!< Sets the current rotation speed
    REAL            GetRotationSpeed    ( void ) const;	                //!< Gets the current rotation speed
    tCoord const &  GetRotation         ( void ) const;	                //!< Gets the current rotation state
    zZone const &   GetRotationSpeed    ( REAL & rotationSpeed ) const;	//!< Gets the current rotation speed
    zZone &         SetRotationAcceleration( REAL rotationAcceleration );	        //!< Sets the current acceleration of the rotation
    REAL            GetRotationAcceleration( void ) const;	                        //!< Gets the current acceleration of the rotation
    zZone const &   GetRotationAcceleration( REAL & rotationAcceleration ) const;	//!< Gets the current acceleration of the rotation
    rColor const &  GetColor( void ) const;	//!< Gets the current color
    void            SetColor( rColor const & color ); //!< Sets the current color
    */

    void addEffectGroupEnter  (zEffectGroupPtr anEffectGroup) {effectGroupEnter.push_back  (anEffectGroup);};
    void addEffectGroupInside (zEffectGroupPtr anEffectGroup) {effectGroupInside.push_back (anEffectGroup);};
    void addEffectGroupLeave  (zEffectGroupPtr anEffectGroup) {effectGroupLeave.push_back  (anEffectGroup);};
    void addEffectGroupOutside(zEffectGroupPtr anEffectGroup) {effectGroupOutside.push_back(anEffectGroup);};

    void setShape (zShapePtr aShape) { shape = aShape; };
    zShapePtr getShape() { return shape; };

    // HACK
    // Enables fortress described in maps from format 1 to be assigned to a team according to the old behavior
    void setOldFortressAutomaticAssignmentBehavior(bool oldFortressAutomaticAssignmentBehavior)
    {
        oldFortressAutomaticAssignmentBehavior_ = oldFortressAutomaticAssignmentBehavior;
    };
    bool getOldFortressAutomaticAssignmentBehavior() { return oldFortressAutomaticAssignmentBehavior_; };

    void setName(string name) {name_ = name;};
    string getName() { return name_; };

protected:
    //    rColor color_;           //!< the zone's color
    REAL createTime_;            //!< the time the zone was created at
    zShapePtr shape; //!< the shape(s) of this zone

    REAL referenceTime_;         //!< reference time for function evaluations
    /*
    tFunction posx_;             //!< time dependence of x component of position
    tFunction posy_;             //!< time dependence of y component of position
    tFunction scale_;           //!< time dependence of scale
    tFunction rotationSpeed_;    //!< the zone's rotation speed
    eCoord    rotation_;         //!< the current rotation state
    */

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnVanish();                     //!< called when the zone vanishes

    zEffectGroupPtrs effectGroupEnter;
    zEffectGroupPtrs effectGroupInside;
    zEffectGroupPtrs effectGroupLeave;
    zEffectGroupPtrs effectGroupOutside;

    std::set<ePlayerNetID *> playersInside; //!< The players that are currently inside the zone
    std::set<ePlayerNetID *> playersOutside; //!< The players that are currently outside the zone

private:
    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them

    void OnEnter( gCycle *target, REAL time ); //!< reacts on objects entering the zone
    void OnLeave( gCycle *target, REAL time ); //!< reacts on objects leaving the zone
    void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    void OnOutside( gCycle *target, REAL time ); //!< reacts on objects outside the zone

    //! returns the descriptor responsible for this class
    virtual nOProtoBufDescriptorBase const * DoGetDescriptor() const;

    //    REAL Scale() const;           //!< returns the current scale

    virtual void Render(const eCamera *cam);  //!< renders the zone
    virtual void Render2D(tCoord scale) const;  //!< renders the zone

    inline REAL EvaluateFunctionNow( tFunction const & f ) const;  //!< evaluates the given function with lastTime - referenceTime_ as argument
    inline void SetFunctionNow( tFunction & f, REAL value ) const; //!< makes sure EvaluateFunctionNow() returns the given value

    void RemoveFromZoneList(void); //!< Removes the zone from the sg_Zones list if it's there

    bool oldFortressAutomaticAssignmentBehavior_;

    string name_;

};

#endif
