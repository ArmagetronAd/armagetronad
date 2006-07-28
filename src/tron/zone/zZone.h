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
    zZone(nMessage &m);                    //!< network constructor
    ~zZone();                              //!< destructor

    void SetReferenceTime();               //!< sets the reference time to the current time

    zZone &         SetPosition         ( eCoord const & position );	//!< Sets the current position
    eCoord          GetPosition         ( void ) const;	                //!< Gets the current position
    zZone const &   GetPosition         ( eCoord & position ) const;	//!< Gets the current position
    zZone &         SetVelocity         ( eCoord const & velocity );	//!< Sets the current velocity
    eCoord          GetVelocity         ( void ) const;	                //!< Gets the current velocity
    zZone const &   GetVelocity         ( eCoord & velocity ) const;	//!< Gets the current velocity
    zZone &         SetRadius           ( REAL radius );	            //!< Sets the current radius
    REAL            GetRadius           ( void ) const;	                //!< Gets the current radius
    zZone const &   GetRadius           ( REAL & radius ) const;	    //!< Gets the current radius
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

    void addEffectGroupEnter  (zEffectGroupPtr anEffectGroup) {effectGroupEnter.push_back  (anEffectGroup);};
    void addEffectGroupInside (zEffectGroupPtr anEffectGroup) {effectGroupInside.push_back (anEffectGroup);};
    void addEffectGroupLeave  (zEffectGroupPtr anEffectGroup) {effectGroupLeave.push_back  (anEffectGroup);};
    void addEffectGroupOutside(zEffectGroupPtr anEffectGroup) {effectGroupOutside.push_back(anEffectGroup);};

protected:
    rColor color_;           //!< the zone's color
    REAL createTime_;            //!< the time the zone was created at

    REAL referenceTime_;         //!< reference time for function evaluations
    tFunction posx_;             //!< time dependence of x component of position
    tFunction posy_;             //!< time dependence of y component of position
    tFunction radius_;           //!< time dependence of radius
    tFunction rotationSpeed_;    //!< the zone's rotation speed
    eCoord    rotation_;         //!< the current rotation state

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnVanish();                     //!< called when the zone vanishes

    zEffectGroupPtrs effectGroupEnter;
    zEffectGroupPtrs effectGroupInside;
    zEffectGroupPtrs effectGroupLeave;
    zEffectGroupPtrs effectGroupOutside;

    std::set<ePlayerNetID *> playersInside; //!< The players that are currently inside the zone
    std::set<ePlayerNetID *> playersOutside; //!< The players that are currently outside the zone

private:
    virtual void WriteCreate(nMessage &m); //!< writes data for network constructor
    virtual void WriteSync(nMessage &m);   //!< writes sync data
    virtual void ReadSync(nMessage &m);    //!< reads sync data

    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them

    void OnEnter( gCycle *target, REAL time ); //!< reacts on objects entering the zone
    void OnLeave( gCycle *target, REAL time ); //!< reacts on objects leaving the zone
    void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    void OnOutside( gCycle *target, REAL time ); //!< reacts on objects outside the zone

    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network

    REAL Radius() const;           //!< returns the current radius

    virtual void Render(const eCamera *cam);  //!< renders the zone

    inline REAL EvaluateFunctionNow( tFunction const & f ) const;  //!< evaluates the given function with lastTime - referenceTime_ as argument
    inline void SetFunctionNow( tFunction & f, REAL value ) const; //!< makes sure EvaluateFunctionNow() returns the given value
};

#endif
