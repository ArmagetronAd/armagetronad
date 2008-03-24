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

// get rid of this include (it's needed for gColor)
#include "gCycle.h"
#include "eTeam.h"


class eTeam;

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
class tFunction
{
public:
    tFunction();  //!< constructor
    ~tFunction(); //!< destructor

    REAL Evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator

    // function parameters: currently limited to offset_ + slope_ * argument
    REAL offset_; //!< offset value
    REAL slope_;  //!< function slope

public:
    inline tFunction & SetOffset( REAL const & offset );	//!< Sets offset value
    inline REAL const & GetOffset( void ) const;	//!< Gets offset value
    inline tFunction const & GetOffset( REAL & offset ) const;	//!< Gets offset value
    inline tFunction & SetSlope( REAL const & slope );	//!< Sets function slope
    inline REAL const & GetSlope( void ) const;	//!< Gets function slope
    inline tFunction const & GetSlope( REAL & slope ) const;	//!< Gets function slope
protected:
private:
};

nMessage & operator << ( nMessage & m, tFunction const & f ); //! function network message writing operator
nMessage & operator >> ( nMessage & m, tFunction & f );       //! function network message reading operator

//! basic zone class: handles rendering and entwork syncing
class gZone: public eNetGameObject
{
public:
    gZone(eGrid *grid, const eCoord &pos, bool dynamicCreation = false); //!< local constructor
    gZone(nMessage &m);                    //!< network constructor
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
    gZone const &   GetRotationSpeed    ( REAL & rotationSpeed ) const;	//!< Gets the current rotation speed
    gZone &         SetRotationAcceleration( REAL rotationAcceleration );	        //!< Sets the current acceleration of the rotation
    REAL            GetRotationAcceleration( void ) const;	                        //!< Gets the current acceleration of the rotation
    gZone const &   GetRotationAcceleration( REAL & rotationAcceleration ) const;	//!< Gets the current acceleration of the rotation

    gZone &         SetColor            (gRealColor color) {color_ = color; return *this;}      //!< Sets the current color
    gRealColor &    GetColor            () {return color_;}             //!< Gets the current color
    gZone &         GetColor            (gRealColor & color) {color = color_; return *this;}    //!< Gets the current color
    gZone &         SetOwner            (ePlayerNetID *pOwner) {pOwner_ = pOwner; return *this;}  //!< Sets the current owner
    ePlayerNetID *  GetOwner            () {return pOwner_;}  //!< Sets the current owner
    gZone &         SetSeekingCycle     (gCycle *pCycle) {if (pCycle) {seeking_ = true;} else {seeking_ = false;} pSeekingCycle_ = pCycle; return *this;}  //!< Sets the current seeking cycle
    gCycle *        GetSeekingCycle     () {return pSeekingCycle_;}  //!< Sets the current seeking cycle
    gZone &         SetTargetRadius     (REAL radius) {targetRadius_ = radius; return *this;}      //!< Sets the target radius
    gZone &         SetFallSpeed        (REAL speed) {fallSpeed_ = speed; return *this;}      //!< Sets the fall speed
    void BounceOffPoint(eCoord dest, eCoord collide, REAL mod);

    void Destroy();
    bool destroyed_;

protected:
    bool wallInteract_;
    int wallBouncesLeft_;
    eWall *pLastWall_;

    bool dynamicCreation_;  //??? remove
    ePlayerNetID *pOwner_;
    gCycle *pSeekingCycle_;       //!< cycle owner of this zone
    bool seeking_;
    REAL targetRadius_;
    REAL fallSpeed_;
    REAL lastSeekTime_;

    gRealColor color_;           //!< the zone's color
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
    virtual void WriteCreate(nMessage &m); //!< writes data for network constructor
    virtual void WriteSync(nMessage &m);   //!< writes sync data
    virtual void ReadSync(nMessage &m);    //!< reads sync data

    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnEnter( gZone *target, REAL time );  //!< reacts on objects inside the zone

    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network

    REAL Radius() const;           //!< returns the current radius

    virtual void Render(const eCamera *cam);  //!< renders the zone

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
    gWinZoneHack(nMessage &m);                    //!< network constructor
    ~gWinZoneHack();                              //!< destructor

protected:
private:
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (declares them the winner)
};

//! death zone: kills players who enter
class gDeathZoneHack: public gZone
{
public:

    enum
    {
        TYPE_NORMAL,
        TYPE_SHOT,
        TYPE_DEATH_SHOT,
        TYPE_SELF_DESTRUCT,
        TYPE_ZOMBIE_ZONE,

        NUM_DEATH_ZONE_TYPES
    };

    gDeathZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false );              //!< local constructor
    gDeathZoneHack(nMessage &m);                                  //!< network constructor
    ~gDeathZoneHack();                                            //!< destructor

    gDeathZoneHack *pLastShotCollision;

    gZone &         SetType            (int type);                //!< Sets the current type
    int             GetType            () {return (deathZoneType);}

    virtual void OnEnter( gDeathZoneHack *target, REAL time ); //!< reacts on objects inside the zone

protected:
    virtual void OnVanish();                     //!< called when the zone vanishes

    int deathZoneType;

private:
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (kills them)

    gCycle * getPlayerCycle(ePlayerNetID *pPlayer);
};

//! base zone: belongs to a team, enemy players who manage to stay inside win the round (will be replaced
class gBaseZoneHack: public gZone
{
public:
    gBaseZoneHack(eGrid *grid, const eCoord &pos );               //!< local constructor
    gBaseZoneHack(nMessage &m);                                   //!< network constructor
    ~gBaseZoneHack();                                             //!< destructor

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnEnter( gZone *target, REAL time );  //!< reacts on objects inside the zone
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnConquest();                         //!< called when the zone gets conquered
    virtual void CheckSurvivor();                      //!< checks for the only surviving zone
    virtual void OnRoundBegin();                       //!< called on the beginning of the round
    virtual void OnRoundEnd();                         //!< called on the end of the round

    void ZoneWasHeld();                                //!< call when the zone was held as long as possible with the set game rules

    static void CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest ); //!< counts the zones belonging to the given team.

    REAL conquered_;                       //!< conquest status; zero if it is free, 1 if it has been completely conquered by the enemy
    int enemiesInside_;                     //!< count of enemies currently inside the zone
    tColoredString enemyPlayerName_;        //!< name of the first enemy player that was inside us

    int ownersInside_;                      //!< count of owners currently inside the zone
    tColoredString teamPlayerName_;         //!< name of the first team player that was inside us

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

    REAL lastRespawnRemindTime_;
    int lastRespawnRemindWaiting_;
};

class gBallZoneHack: public gZone
{
public:
    gBallZoneHack(eGrid *grid, const eCoord &pos );              //!< local constructor
    gBallZoneHack(nMessage &m);                                  //!< network constructor
    ~gBallZoneHack();                                            //!< destructor

    void RemovePlayer(ePlayerNetID *player);
    ePlayerNetID * GetLastPlayer();
    void GoHome();

protected:
    eCoord originalPosition_;

private:
    virtual void OnVanish();                           //!< called when the zone vanishes
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (kills them)

    tJUST_CONTROLLED_PTR<ePlayerNetID> lastPlayer_;
};

class gFlagZoneHack: public gZone
{
public:
    gFlagZoneHack(eGrid *grid, const eCoord &pos );              //!< local constructor
    gFlagZoneHack(nMessage &m);                                  //!< network constructor
    ~gFlagZoneHack();                                            //!< destructor

    void SetTeam(tJUST_CONTROLLED_PTR< eTeam > team) { this->team = team; }

    void WarnFlagNotHome();
    bool IsHome();
    void GoHome();
    void RemoveOwner();
    void OwnerDropped();

protected:
    bool init_;
    eCoord originalPosition_;
    eCoord homePosition_;
    REAL originalRadius_;
    gCycle *owner_;
    REAL ownerTime_;
    bool ownerWarnedNotHome_;
    REAL chatBlinkUpdateTime_;
    REAL blinkUpdateTime_;
    REAL blinkTrackUpdateTime_;
    gCycle *ownerDropped_;
    REAL ownerDroppedTime_;
    REAL lastHoldScoreTime_;
    bool positionUpdatePending_;

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (kills them)
};

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos );

// *******************************************************************************
// *
// *	operator ( )
// *
// *******************************************************************************
//!
//!		@return		the function value
//!
// *******************************************************************************

REAL tFunction::operator ( )( REAL argument ) const
{
    return Evaluate( argument );
}

// *******************************************************************************
// *
// *	GetOffset
// *
// *******************************************************************************
//!
//!		@return		offset value
//!
// *******************************************************************************

REAL const & tFunction::GetOffset( void ) const
{
    return this->offset_;
}

// *******************************************************************************
// *
// *	GetOffset
// *
// *******************************************************************************
//!
//!		@param	offset	offset value to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction const & tFunction::GetOffset( REAL & offset ) const
{
    offset = this->offset_;
    return *this;
}

// *******************************************************************************
// *
// *	SetOffset
// *
// *******************************************************************************
//!
//!		@param	offset	offset value to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction & tFunction::SetOffset( REAL const & offset )
{
    this->offset_ = offset;
    return *this;
}

// *******************************************************************************
// *
// *	GetSlope
// *
// *******************************************************************************
//!
//!		@return		function slope
//!
// *******************************************************************************

REAL const & tFunction::GetSlope( void ) const
{
    return this->slope_;
}

// *******************************************************************************
// *
// *	GetSlope
// *
// *******************************************************************************
//!
//!		@param	slope	function slope to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction const & tFunction::GetSlope( REAL & slope ) const
{
    slope = this->slope_;
    return *this;
}

// *******************************************************************************
// *
// *	SetSlope
// *
// *******************************************************************************
//!
//!		@param	slope	function slope to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

tFunction & tFunction::SetSlope( REAL const & slope )
{
    this->slope_ = slope;
    return *this;
}

#endif
