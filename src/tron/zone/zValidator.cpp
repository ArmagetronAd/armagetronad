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

#include "zValidator.h"




zValidator::zValidator(Triad _positive, Triad _marked)
        : selectors(),
        monitorInfluences(),
        zoneInfluences(),
        positive(_positive),
        marked(_marked)
{ }

zValidator::zValidator(zValidator const &other):
        selectors( other.selectors ),
        monitorInfluences( other.monitorInfluences ),
        zoneInfluences( other.zoneInfluences ),
        positive( other.getPositive() ),
        marked( other.getMarked() )
{  }


void zValidator::operator=(zValidator const &other)
{
    if(this != &other) {
        positive = other.getPositive();
        marked   = other.getMarked();
    }
}

zValidator *zValidator::copy(void) const {
    return new zValidator(*this);
}

bool
zValidator::isOwner(ePlayerNetID *possibleOwner, gVectorExtra< nNetObjectID > &owners)
{
    gVectorExtra< nNetObjectID >::iterator iter;
    for(iter = owners.begin();
            iter != owners.end();
            ++iter)
    {
        if (bool(sn_netObjects[(*iter)]) && (*iter == possibleOwner->ID()) )
            return true;
    }
    return false;
}

bool
zValidator::isTeamOwner(eTeam *possibleTeamOwner, gVectorExtra< nNetObjectID > &teamOwners)
{
    gVectorExtra< nNetObjectID >::iterator iter;
    for(iter = teamOwners.begin();
            iter != teamOwners.end();
            ++iter)
    {
        // When in local game, the team id of all players is 0, which while not a valid reference, it still ok
        bool isValidID = ( (*iter) == 0 || bool(sn_netObjects[(*iter)]));
        if ( isValidID && (*iter == possibleTeamOwner->ID()) )
            return true;
    }
    return false;
}

void
zValidator::validate(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, Triggerer possibleUser, miscDataPtr &miscData) {
    REAL value = 0.0;
    if ( miscData.get() != 0 )
        value = *miscData;

    if(isValid(owners, teamOwners, possibleUser.who) && validateTriad(possibleUser.positive, positive) && validateTriad(possibleUser.marked, marked)) {
        zSelectorPtrs::const_iterator iterSelector;
        for(iterSelector=selectors.begin();
                iterSelector!=selectors.end();
                ++iterSelector)
        {
            (*iterSelector)->apply(owners, teamOwners, possibleUser.who);
        }

        zMonitorInfluencePtrs::const_iterator iterMonitorInfluence;
        for(iterMonitorInfluence=monitorInfluences.begin();
                iterMonitorInfluence!=monitorInfluences.end();
                ++iterMonitorInfluence)
        {
            (*iterMonitorInfluence)->apply(owners, teamOwners, possibleUser.who, value);
        }

        zZoneInfluencePtrs::const_iterator iterZoneInfluence;
        for(iterZoneInfluence=zoneInfluences.begin();
                iterZoneInfluence!=zoneInfluences.end();
                ++iterZoneInfluence)
        {
            (*iterZoneInfluence)->apply(value);
        }
    }
}

// *******************
// zValidatorAll
// *******************
zValidator * zValidatorAll::create(Triad _positive, Triad _marked)
{
    return new zValidatorAll(_positive, _marked);
}

zValidatorAll::zValidatorAll(Triad _positive, Triad _marked):
        zValidator(_positive, _marked)
{ }

zValidatorAll::zValidatorAll(zValidatorAll const &other):
        zValidator(other)
{ }

void zValidatorAll::operator=(zValidatorAll const &other)
{
    this->zValidator::operator=(other);
}

zValidator *zValidatorAll::copy(void) const {
    return new zValidatorAll(*this);
}

// *******************
// zValidatorOwner
// *******************
zValidator * zValidatorOwner::create(Triad _positive, Triad _marked)
{
    return new zValidatorOwner(_positive, _marked);
}

zValidatorOwner::zValidatorOwner(Triad _positive, Triad _marked):
        zValidator(_positive, _marked)
{ }

zValidatorOwner::zValidatorOwner(zValidatorOwner const &other):
        zValidator(other)
{ }

void zValidatorOwner::operator=(zValidatorOwner const &other)
{
    this->zValidator::operator=(other);
}

zValidator *zValidatorOwner::copy(void) const {
    return new zValidatorOwner(*this);
}

bool
zValidatorOwner::isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser)
{
    return isOwner(possibleUser->Player(), owners);
}

// *******************
// zValidatorOwnerTeam
// *******************
zValidator * zValidatorOwnerTeam::create(Triad _positive, Triad _marked)
{
    return new zValidatorOwnerTeam(_positive, _marked);
}

zValidatorOwnerTeam::zValidatorOwnerTeam(Triad _positive, Triad _marked):
        zValidator(_positive, _marked)
{ }

zValidatorOwnerTeam::zValidatorOwnerTeam(zValidatorOwnerTeam const &other):
        zValidator(other)
{ }

void zValidatorOwnerTeam::operator=(zValidatorOwnerTeam const &other)
{
    this->zValidator::operator=(other);
}

zValidator *zValidatorOwnerTeam::copy(void) const {
    return new zValidatorOwnerTeam(*this);
}

bool
zValidatorOwnerTeam::isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser)
{
    return isTeamOwner(possibleUser->Player()->CurrentTeam(), teamOwners);
}

// *******************
// zValidatorAllButOwner
// *******************
zValidator * zValidatorAllButOwner::create(Triad _positive, Triad _marked)
{
    return new zValidatorAllButOwner(_positive, _marked);
}

zValidatorAllButOwner::zValidatorAllButOwner(Triad _positive, Triad _marked):
        zValidator(_positive, _marked)
{ }

zValidatorAllButOwner::zValidatorAllButOwner(zValidatorAllButOwner const &other):
        zValidator(other)
{ }

void zValidatorAllButOwner::operator=(zValidatorAllButOwner const &other)
{
    this->zValidator::operator=(other);
}

zValidator *zValidatorAllButOwner::copy(void) const {
    return new zValidatorAllButOwner(*this);
}

bool
zValidatorAllButOwner::isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser)
{
    return !isOwner(possibleUser->Player(), owners);
}

// *******************
// zValidatorAllButTeamOwner
// *******************
zValidator * zValidatorAllButTeamOwner::create(Triad _positive, Triad _marked)
{
    return new zValidatorAllButTeamOwner(_positive, _marked);
}

zValidatorAllButTeamOwner::zValidatorAllButTeamOwner(Triad _positive, Triad _marked):
        zValidator(_positive, _marked)
{ }

zValidatorAllButTeamOwner::zValidatorAllButTeamOwner(zValidatorAllButTeamOwner const &other):
        zValidator(other)
{ }

void zValidatorAllButTeamOwner::operator=(zValidatorAllButTeamOwner const &other)
{
    this->zValidator::operator=(other);
}

zValidator *zValidatorAllButTeamOwner::copy(void) const {
    return new zValidatorAllButTeamOwner(*this);
}

bool
zValidatorAllButTeamOwner::isValid(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* possibleUser)
{
    return !isTeamOwner(possibleUser->Player()->CurrentTeam(), teamOwners);
}






/*
bool
::isValidUser(gCycle *possibleUser)
{
    switch(d_userType)
    {
    case UserAll: 
// Everybody, irrevelantly of any settings, is allowed to
// trigger this Zone. This will be the default behavior.
	return true;
    break;
    case UserOwner: 
// Only a player that has its id in the owner field of the
// Zone can trigger it (revisit this when a decision is made about
// multiple owners)
	return isOwner(possibleUser->Player());
    break;
    case UserOwnerTeam: 
// Only a player member of a team that has its id in the
// ownerTeam field can trigger this Zone. Thus the Zone can be
// triggered by any of the members of a team.
	return isTeamOwner(possibleUser->Player()->CurrentTeam());
    break;
    case UserAllButOwner: 
// Only the player that has its id in the owner field is
// denied triggering the Zone. This is the opposite effect as
// Owner. It is synonymous to "everybody but the owner of the Zone".
	return !isOwner(possibleUser->Player());
    break;
    case UserAllButTeamOwner: 
// Only players that are members of the team that
// has its id in the ownerTeam field are denied triggering this
// Zone. This is the opposite of OwnerTeam. It is synonymous to
// "everybody but the team owner of the Zone".
	return !isTeamOwner(possibleUser->Player()->CurrentTeam());
    break;
    case UserAnotherTeammate: 
// This Zone is restricted to players that are
// member of the same team as the player which id is stored in the
// owner field. The triggering of the Zone is itself denied to the
// player that has its id stored in the owner field. This is a
// restricted version of OwnerTeam. It is synonymous to "only the
// teammates of the owner can use this Zone, but even the owner is
// denied".
	return !isOwner(possibleUser->Player()) && isTeamOwner(possibleUser->Player()->CurrentTeam());
    break;
    }

    return false;
}
*/
