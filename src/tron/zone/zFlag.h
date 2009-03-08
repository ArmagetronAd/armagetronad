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
    void SetTeam(tJUST_CONTROLLED_PTR< eTeam > team) { this->team = team; }
    void setupVisuals(gParser::State_t &);
    void readXML(tXmlParser::node const &);
    bool IsHome();
    gCycle* Owner(){return owner_;}
	
private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnInside( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnEntry( gCycle * target, REAL time  ); //!< reacts on objects entering the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
	virtual void CheckSurvivor();                      //!< checks for the only surviving zone
    virtual void OnRoundBegin();                       //!< called on the beginning of the round
    virtual void OnRoundEnd();                         //!< called on the end of the round

    void ZoneWasHeld();                                //!< call when the zone was held as long as possible with the set game rules
	void GoHome();
	void WarnFlagNotHome();
	void RemoveOwner();
	void OwnerDropped();
	
	bool init_;
    REAL teamDistance_;                     //!< distance to the closest member of the owning team
	eTeam *initOwnerTeam_;
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
    eCoord originalPosition_;
    REAL originalRadius_;
	bool ownerWarnedNotHome_;

   
};

#endif
