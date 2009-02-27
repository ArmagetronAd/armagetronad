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

#ifndef ArmageTron_WinZone_H
#define ArmageTron_WinZone_H

#include "eNetGameObject.h"

#include <vector>

#include "rColor.h"
#include "tFunction.h"

#ifdef OnEnter
// Undo Zones V2 safety catch here
#undef OnEnter
#endif

namespace Game { class ZoneV1Sync; }

// misc zone-related settings
extern int sg_zoneAlphaToggle;
extern int sg_zoneSegments;
extern REAL sg_zoneSegLength;
extern REAL sg_zoneBottom;
extern REAL sg_zoneHeight;

// zone expansion speed and size
extern REAL sg_expansionSpeed;
extern REAL sg_initialSize;

// fortress conquest rates
extern REAL sg_conquestDecayRate;
extern REAL sg_conquestRate;
extern REAL sg_defendRate;

class eTeam;
class gCycle;

//! basic zone class: handles rendering and entwork syncing
class gZone: public eNetGameObject
{
public:
    gZone(eGrid *grid, const eCoord &pos); //!< local constructor
    ~gZone();                              //!< destructor

    void SetReferenceTime();               //!< sets the reference time to the current time

    gZone &         SetPosition         ( eCoord const & position );	//!< Sets the current position
    eCoord          GetPosition         ( void ) const;	                //!< Gets the current position
    gZone const &   GetPosition         ( eCoord & position ) const;	//!< Gets the current position
    gZone &         SetVelocity         ( eCoord const & velocity );	//!< Sets the current velocity
    eCoord          GetVelocity         ( void ) const;	                //!< Gets the current velocity
    gZone const &   GetVelocity         ( eCoord & velocity ) const;	//!< Gets the current velocity
    gZone &         SetRadius           ( REAL radius );	            //!< Sets the current radius
    REAL            GetRadius           ( void ) const;	                //!< Gets the current radius
    gZone const &   GetRadius           ( REAL & radius ) const;	    //!< Gets the current radius
    gZone &         SetExpansionSpeed   ( REAL expansionSpeed );	    //!< Sets the current expansion speed
    REAL            GetExpansionSpeed   ( void ) const;	                //!< Gets the current expansion speed
    gZone const &   GetExpansionSpeed   ( REAL & expansionSpeed ) const;//!< Gets the current expansion speed
    gZone &         SetRotationSpeed    ( REAL rotationSpeed );	        //!< Sets the current rotation speed
    REAL            GetRotationSpeed    ( void ) const;	                //!< Gets the current rotation speed
    tCoord const &  GetRotation    ( void ) const;	                //!< Gets the current rotation state
    gZone const &   GetRotationSpeed    ( REAL & rotationSpeed ) const;	//!< Gets the current rotation speed
    gZone &         SetRotationAcceleration( REAL rotationAcceleration );	        //!< Sets the current acceleration of the rotation
    REAL            GetRotationAcceleration( void ) const;	                        //!< Gets the current acceleration of the rotation
    gZone const &   GetRotationAcceleration( REAL & rotationAcceleration ) const;	//!< Gets the current acceleration of the rotation
    rColor const &  GetColor( void ) const;	//!< Gets the current color

    gZone( Game::ZoneV1Sync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Game::ZoneV1Sync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Game::ZoneV1Sync & sync, bool init ) const;

protected:
    rColor color_;               //!< the zone's color
    REAL createTime_;            //!< the time the zone was created at

    REAL referenceTime_;         //!< reference time for function evaluations
    tFunction posx_;             //!< time dependence of x component of position
    tFunction posy_;             //!< time dependence of y component of position
    tFunction radius_;           //!< time dependence of radius
    tFunction rotationSpeed_;    //!< the zone's rotation speed
    eCoord    rotation_;         //!< the current rotation state

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnVanish();                     //!< called when the zone vanishes

private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;

    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone

    REAL Radius() const;           //!< returns the current radius

    virtual void Render(const eCamera *cam);  //!< renders the zone
    virtual void Render2D(tCoord scale) const;  //!< renders the zone

    //! returns whether the rendering uses alpha blending (massively, so sorting errors would show)
    virtual bool RendersAlpha() const;

    inline REAL EvaluateFunctionNow( tFunction const & f ) const;  //!< evaluates the given function with lastTime - referenceTime_ as argument
    inline void SetFunctionNow( tFunction & f, REAL value ) const; //!< makes sure EvaluateFunctionNow() returns the given value
};

// all the following zones are hacks until the full zone system is in place

//! win zone: lets players who enter win the round
class gWinZoneHack: public gZone
{
public:
    gWinZoneHack(eGrid *grid, const eCoord &pos); //!< local constructor
    ~gWinZoneHack();                              //!< destructor

protected:
private:
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (declares them the winner)
};

//! death zone: kills players who enter
class gDeathZoneHack: public gZone
{
public:
    gDeathZoneHack(eGrid *grid, const eCoord &pos );              //!< local constructor
    ~gDeathZoneHack();                                            //!< destructor

protected:
private:
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (kills them)
};

//! base zone: belongs to a team, enemy players who manage to stay inside win the round (will be replaced
class gBaseZoneHack: public gZone
{
public:
    gBaseZoneHack(eGrid *grid, const eCoord &pos );               //!< local constructor
    ~gBaseZoneHack();                                             //!< destructor

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnConquest();                         //!< called when the zone gets conquered
    virtual void CheckSurvivor();                      //!< checks for the only surviving zone
    virtual void OnRoundBegin();                       //!< called on the beginning of the round
    virtual void OnRoundEnd();                         //!< called on the end of the round

    void ZoneWasHeld();                                //!< call when the zone was held as long as possible with the set game rules

    static void CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest ); //!< counts the zones belonging to the given team.

    REAL conquered_;                       //!< conquest status; zero if it is free, 1 if it has been completely conquered by the enemy
    int enemiesInside_;                     //!< count of enemies currently inside the zone
    int ownersInside_;                      //!< count of owners currently inside the zone

    bool onlySurvivor_;                     //!< flag set if this zone is the only survivor

    typedef std::vector< tJUST_CONTROLLED_PTR< eTeam > > TeamArray;
    TeamArray enemies_;                     //!< list of teams that currently have a player in the zone
    REAL lastEnemyContact_;                 //!< last time an enemy player was in the zone

    REAL teamDistance_;                     //!< distance to the closest member of the owning team

    bool touchy_;                           //!< flag set when the zone is "touchy", which makes it get conquered on the slightest enemy touch

    //! possible states
    enum State
    {
        State_Safe,        //!< not yet conquered
        State_Conquering,  //!< conquering in this frame
        State_Conquered    //!< conquered
    };
    State currentState_;   //!< the current state

    REAL lastSync_;        //!< time of the last sync request
};

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos );

#endif
