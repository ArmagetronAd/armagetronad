/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
Portions Copyright (C) 2008  Luke Dashjr (luke@dashjr.org)

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

#ifndef ArmageTron_zFlag_H
#define ArmageTron_zFlag_H

#include "zone/zZone.h"

#include <vector>

class eTeam;
class gCycle;
class gParser;

//! fortress zone: belongs to a team, enemy players who manage to stay inside win the round
class zFlagZone: public zZone
{
public:
    static zZone* create(eGrid*grid, std::string const & type) { return new zFlagZone(grid); };
    zFlagZone(eGrid *grid);                                   //!< local constructor
    ~zFlagZone();                                             //!< destructor

    void setupVisuals(gParser::State_t &);
    void readXML(tXmlParser::node const &);

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnConquest();                         //!< called when the zone gets conquered
    virtual void CheckSurvivor();                      //!< checks for the only surviving zone
    virtual void OnRoundBegin();                       //!< called on the beginning of the round
    virtual void OnRoundEnd();                         //!< called on the end of the round

    void ZoneWasHeld();                                //!< call when the zone was held as long as possible with the set game rules
	void GoHome();
	
	bool init_;
	eTeam initOwnerTeam_;
    eCoord homePosition_;
    gCycle *owner_;
	float ownerTime_;
	bool flagHome_;
    float chatBlinkUpdateTime_;
    float blinkUpdateTime_;
    float blinkTrackUpdateTime_;
    gCycle *ownerDropped_;
   float ownerDroppedTime_;
    float lastHoldScoreTime_;
    bool positionUpdatePending_;
   

   
};

#endif
