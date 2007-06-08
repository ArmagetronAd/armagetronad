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

#ifndef ARMAGETRONAD_H_VALIDATOR
#define ARMAGETRONAD_H_VALIDATOR


#include <vector>
#include <boost/shared_ptr.hpp>
#include "gVectorExtra.h"
#include "ePlayer.h"
#include "gCycle.h"
#include "eTeam.h"
#include "zone/zMisc.h"
#include <boost/shared_ptr.hpp>


class zSelector;
typedef boost::shared_ptr<zSelector> zSelectorPtr;
typedef std::vector< zSelectorPtr > zSelectorPtrs;

class zMonitorInfluence;
typedef boost::shared_ptr<zMonitorInfluence> zMonitorInfluencePtr;
typedef std::vector< zMonitorInfluencePtr > zMonitorInfluencePtrs;

class zZoneInfluence;
typedef boost::shared_ptr<zZoneInfluence> zZoneInfluencePtr;
typedef std::vector< zZoneInfluencePtr > zZoneInfluencePtrs;

class zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidator(Triad _positive, Triad _marked);
    zValidator(zValidator const &other);
    void operator=(zValidator const &other); //!< overloaded assignment operator
    virtual zValidator *copy(void) const;
    virtual ~zValidator() {};

    void validate(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, Triggerer possibleUser, miscDataPtr &miscData);

    void addSelector(zSelectorPtr _selector) {selectors.push_back(_selector);};
    void addMonitorInfluence(zMonitorInfluencePtr newInfluence) {monitorInfluences.push_back( newInfluence );};
    void addZoneInfluence(zZoneInfluencePtr newInfluence) {zoneInfluences.push_back( newInfluence );};

    Triad getPositive(void) const {return positive;};
    Triad getMarked(void) const {return marked;};

protected:
    zSelectorPtrs selectors;
    zMonitorInfluencePtrs monitorInfluences;
    zZoneInfluencePtrs zoneInfluences;

    Triad positive; // Should the possible user make a positive contribution. ATM this is only for monitors. Anything else should use _ignore
    Triad marked; // Should the possible user be "marked". ATM this is only for monitors. Anything else should use _ignore


    bool isOwner(ePlayerNetID *possibleOwner, gVectorExtra< nNetObjectID > &owners);
    bool isTeamOwner(eTeam *possibleTeamOwner, gVectorExtra< nNetObjectID > &teamOwners);
    virtual bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser) {return false;};
};


class zValidatorAll : public zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidatorAll(Triad _positive, Triad _marked);
    zValidatorAll(zValidatorAll const &other);
    void operator=(zValidatorAll const &other); //!< overloaded assignment operator
    virtual ~zValidatorAll() {};
    zValidator *copy(void) const;
protected:
    bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser) { return true ;};
};

class zValidatorOwner : public zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidatorOwner(Triad _positive, Triad _marked);
    zValidatorOwner(zValidatorOwner const &other);
    void operator=(zValidatorOwner const &other); //!< overloaded assignment operator
    virtual ~zValidatorOwner() {};
    zValidator *copy(void) const;
protected:
    bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser);
};

class zValidatorOwnerTeam : public zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidatorOwnerTeam(Triad _positive, Triad _marked);
    zValidatorOwnerTeam(zValidatorOwnerTeam const &other);
    void operator=(zValidatorOwnerTeam const &other); //!< overloaded assignment operator
    virtual ~zValidatorOwnerTeam() {};
    zValidator *copy(void) const;
protected:
    bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser);
};

class zValidatorAllButOwner : public zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidatorAllButOwner(Triad _positive, Triad _marked);
    zValidatorAllButOwner(zValidatorAllButOwner const &other);
    void operator=(zValidatorAllButOwner const &other); //!< overloaded assignment operator
    virtual ~zValidatorAllButOwner() {};
    zValidator *copy(void) const;
protected:
    bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser);
};

class zValidatorAllButTeamOwner : public zValidator
{
public:
    static zValidator* create(Triad _positive, Triad _marked);
    zValidatorAllButTeamOwner(Triad _positive, Triad _marked);
    zValidatorAllButTeamOwner(zValidatorAllButTeamOwner const &other);
    void operator=(zValidatorAllButTeamOwner const &other); //!< overloaded assignment operator
    virtual ~zValidatorAllButTeamOwner() {};
    zValidator *copy(void) const;
protected:
    bool isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser);
};



/*
 * ******** WARNING *********
 * This include has to be at the end of this file to break a cyclic referencing.
 * The classes involved are, in order:
 * 
 * zMonitorRule
 * zEffectGroup
 * zValidator
 * zMonitorInfluence
 * wich points back to zMonitorRule
 * ******** WARNING *********
 */
#include "zone/zMonitor.h"
#include "zone/zSelector.h"
#include "zone/zZoneInfluence.h"

#endif
