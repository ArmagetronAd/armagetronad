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

#include "zone/zEffectGroup.h"

zEffectGroup::zEffectGroup(gVectorExtra<ePlayerNetID *> const owners, gVectorExtra<eTeam *> const teamOwners):
        validators(),
        d_owners(owners),
        d_teamOwners(teamOwners),
        d_calculatedTargets()
{ }

zEffectGroup::zEffectGroup(zEffectGroup const &other) :
        validators(other.validators),
        d_owners(other.d_owners),
        d_teamOwners(other.d_teamOwners),
        d_calculatedTargets(other.d_calculatedTargets)
{ }

zEffectGroup::~zEffectGroup()
{ }

void zEffectGroup::operator=(zEffectGroup const &other)
{
    if(this != &other) {
        validators = other.validators;
        d_owners = other.d_owners;
        d_teamOwners = other.d_teamOwners;
        d_calculatedTargets = other.d_calculatedTargets;
    }
}

void zEffectGroup::apply( Triggerer possibleUser, REAL &time, miscDataPtr miscData )
{
    std::vector<zValidatorPtr>::const_iterator iter;
    for(iter=validators.begin();
            iter!=validators.end();
            ++iter)
    {
        (*iter)->validate(d_owners, d_teamOwners, possibleUser, miscData);
    }
}
