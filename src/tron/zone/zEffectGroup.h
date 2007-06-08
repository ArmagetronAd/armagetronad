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

#ifndef ArmageTron_EffectGroup_H
#define ArmageTron_EffectGroup_H

#include "gVectorExtra.h"
#include "gCycle.h"
#include <boost/shared_ptr.hpp>
#include "zone/zMisc.h"

class zValidator;
typedef boost::shared_ptr<zValidator> zValidatorPtr;


class zEffectGroup
{
public:
    zEffectGroup();
    zEffectGroup(gVectorExtra< nNetObjectID > const owners, gVectorExtra< nNetObjectID > const teamOwners);
    zEffectGroup(zEffectGroup const &other);
    ~zEffectGroup();
    void operator=(zEffectGroup const &other);

    void addValidator(zValidatorPtr _validator) {validators.push_back( _validator );};
    /*
    void addMonitorInfluence(zMonitorInfluencePtr newInfluence) {monitorInfluences.push_back( newInfluence );};
    void addZoneInfluence(zZoneInfluencePtr newInfluence) {zoneInfluences.push_back( newInfluence );};
    */
    bool isValidUser(gCycle *possibleUser);
    gVectorExtra <ePlayerNetID *> getCalculatedTarget( gCycle * triggerer );

    void apply( Triggerer target, REAL &time, miscDataPtr miscData = miscDataPtr() ); //!< reacts on objects interacting with  the zone

    //callback functions
    gCycle * cb_PossibleUser(void);           //!< Gets the used rubber for the currently watched cycle
    gVectorExtra< nNetObjectID > cb_Owners(void);
    gVectorExtra< nNetObjectID > cb_TeamOwners(void);
    gVectorExtra<ePlayerNetID *> cb_Targets(void);

protected:
    std::vector<zValidatorPtr> validators;
    //    zMonitorInfluencePtrs monitorInfluences;
    //    zZoneInfluencePtrs zoneInfluences;

    gVectorExtra< nNetObjectID > d_owners;
    gVectorExtra< nNetObjectID > d_teamOwners;

    gVectorExtra<ePlayerNetID *> d_calculatedTargets;

};

typedef boost::shared_ptr<zEffectGroup> zEffectGroupPtr;
typedef std::vector< zEffectGroupPtr > zEffectGroupPtrs;



#include "zone/zValidator.h"

#endif
