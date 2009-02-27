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
#include "tXmlParser.h"
#include <set>
#include <vector>
#include "zone/zShape.hpp"

namespace Zone { class ZoneSync; }

class gParserState;

class zZone: public eNetGameObject
{
private:
    void*pos;  //!< pos is not valid for zones
public:  // DEPRECATED methods: please do NOT use in new code, and REPLACE in old code
    REAL __deprecated GetRotationSpeed();
    void __deprecated SetRotationSpeed(REAL r);
    REAL __deprecated GetRotationAcceleration();
    void __deprecated SetRotationAcceleration(REAL r);
    void __deprecated SetReferenceTime();

public:
    static zZone* create(eGrid*grid, std::string const & type) { return new zZone(grid); };
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

    virtual void setupVisuals(gParserState &);
    virtual void readXML(tXmlParser::node const &);

    eCoord         __deprecated GetPosition         ( void ) const;	                //!< Gets the current position
    zZone const &  __deprecated GetPosition         ( eCoord & position ) const;	//!< Gets the current position
    tCoord const   __deprecated GetRotation         ( void ) const;	                //!< Gets the current rotation state
    REAL           __deprecated GetScale           ( void ) const;	                //!< Gets the current scale
    rColor const   __deprecated GetColor( void ) const;	//!< Gets the current color

    void addEffectGroupEnter  (zEffectGroupPtr anEffectGroup) {effectGroupEnter.push_back  (anEffectGroup);};
    void addEffectGroupInside (zEffectGroupPtr anEffectGroup) {effectGroupInside.push_back (anEffectGroup);};
    void addEffectGroupLeave  (zEffectGroupPtr anEffectGroup) {effectGroupLeave.push_back  (anEffectGroup);};
    void addEffectGroupOutside(zEffectGroupPtr anEffectGroup) {effectGroupOutside.push_back(anEffectGroup);};

    void setShape (zShapePtr aShape) { shape = aShape; };
    zShapePtr getShape() { return shape; };

    void setName(string name) {name_ = name;};
    string getName() { return name_; };

protected:
    REAL createTime_;            //!< the time the zone was created at
    zShapePtr shape; //!< the shape(s) of this zone

    REAL referenceTime_;         //!< reference time for function evaluations

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

#define OnEnter(tgt, time) ;REPLACED__OnEnter_is_now_OnInside_or_OnEntry
    virtual void OnEntry( gCycle *target, REAL time ); //!< reacts on objects entering the zone
    virtual void OnExit( gCycle *target, REAL time ); //!< reacts on objects leaving the zone
    virtual void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnOutside( gCycle *target, REAL time ); //!< reacts on objects outside the zone

    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;

    inline REAL __deprecated EvaluateFunctionNow( tFunction const & f ) const;  //!< evaluates the given function with lastTime - referenceTime_ as argument
    inline void __deprecated SetFunctionNow( tFunction & f, REAL value ) const; //!< makes sure EvaluateFunctionNow() returns the given value

    void RemoveFromZoneList(void); //!< Removes the zone from the sg_Zones list if it's there

    string name_;

};


//! zone extension manager: put summary here
/**
 *   put detailed docs here
 */
class zZoneExtManager {
public:
    static zZone* Create(std::string const & type, eGrid*);

    typedef zZone* (*NamedFactory_t)(eGrid*, std::string const & type);
    
    //! Register an extension type.
    /**
            @param name the name of the extension type
            @param description a human readable description of the extension
            @param func a function that returns a new instance of the zone
     */
    static void Register(std::string const & type, std::string const & desc, NamedFactory_t);

    ~zZoneExtManager();
private:
    typedef std::map<std::string, NamedFactory_t> FactoryList;
    static FactoryList & _factories();

    //! We make the constructor private so that nobody else can
    /// instantiate this class
    zZoneExtManager();
};

class zZoneExtRegistration {
public:
    template<typename T>
    zZoneExtRegistration(std::string const & type, std::string const & desc, T f) {
        zZoneExtManager::Register(type, desc, f);
    }
};


#endif
