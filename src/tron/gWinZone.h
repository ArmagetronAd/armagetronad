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
		tFunction();			 //!< constructor
		~tFunction();			 //!< destructor

								 //!< evaluates the function
		REAL Evaluate( REAL argument ) const;
								 //!< evaluation operator
		inline REAL operator()( REAL argument ) const;

		// function parameters: currently limited to offset_ + slope_ * argument
		REAL offset_;			 //!< offset value
		REAL slope_;			 //!< function slope

	public:
								 //!< Sets offset value
		inline tFunction & SetOffset( REAL const & offset );
								 //!< Gets offset value
		inline REAL const & GetOffset( void ) const;
								 //!< Gets offset value
		inline tFunction const & GetOffset( REAL & offset ) const;
								 //!< Sets function slope
		inline tFunction & SetSlope( REAL const & slope );
								 //!< Gets function slope
		inline REAL const & GetSlope( void ) const;
								 //!< Gets function slope
		inline tFunction const & GetSlope( REAL & slope ) const;
	protected:
	private:
};

								 //! function network message writing operator
nMessage & operator << ( nMessage & m, tFunction const & f );
								 //! function network message reading operator
nMessage & operator >> ( nMessage & m, tFunction & f );

//! basic zone class: handles rendering and entwork syncing
class gZone: public eNetGameObject
{
	public:
								 //!< local constructor
		gZone(eGrid *grid, const eCoord &pos, bool dynamicCreation = false);
		gZone(nMessage &m);		 //!< network constructor
		~gZone();				 //!< destructor

		void SetReferenceTime(); //!< sets the reference time to the current time

								 //!< Sets the current position
		gZone &         SetPosition         ( eCoord const & position );
								 //!< Gets the current position
		eCoord          GetPosition         ( void ) const;
								 //!< Gets the current position
		gZone const &   GetPosition         ( eCoord & position ) const;
								 //!< Sets the current velocity
		gZone &         SetVelocity         ( eCoord const & velocity );
								 //!< Gets the current velocity
		eCoord          GetVelocity         ( void ) const;
								 //!< Gets the current velocity
		gZone const &   GetVelocity         ( eCoord & velocity ) const;
								 //!< Sets the current radius
		gZone &         SetRadius           ( REAL radius );
								 //!< Sets the radius to go to smoothly
		gZone &         SetRadiusSmoothly   ( REAL radius, REAL expansionSpeed = 5);
								 //!< Gets the current radius
		REAL            GetRadius           ( void ) const;
								 //!< Gets the current radius
		gZone const &   GetRadius           ( REAL & radius ) const;
								 //!< Sets the current expansion speed
		gZone &         SetExpansionSpeed   ( REAL expansionSpeed );
								 //!< Gets the current expansion speed
		REAL            GetExpansionSpeed   ( void ) const;
								 //!< Gets the current expansion speed
		gZone const &   GetExpansionSpeed   ( REAL & expansionSpeed ) const;
								 //!< Sets the current rotation speed
		gZone &         SetRotationSpeed    ( REAL rotationSpeed );
								 //!< Gets the current rotation speed
		REAL            GetRotationSpeed    ( void ) const;
								 //!< Gets the current rotation speed
		gZone const &   GetRotationSpeed    ( REAL & rotationSpeed ) const;
								 //!< Sets the current acceleration of the rotation
		gZone &         SetRotationAcceleration( REAL rotationAcceleration );
								 //!< Gets the current acceleration of the rotation
		REAL            GetRotationAcceleration( void ) const;
								 //!< Gets the current acceleration of the rotation
		gZone const &   GetRotationAcceleration( REAL & rotationAcceleration ) const;

		gZone &         SetWallInteract     (bool wallInteract) {wallInteract_=wallInteract; return *this;}
		gZone &         SetWallBouncesLeft  (int wallBouncesLeft) {wallBouncesLeft_=wallBouncesLeft; return *this;}

								 //!< Sets the current color
		gZone &         SetColor            (gRealColor color)
		{
			color_ = color; return *this;
		}
								 //!< Gets the current color
		gRealColor &    GetColor            ()
		{
			return color_;
		}
								 //!< Gets the current color
		gZone &         GetColor            (gRealColor & color)
		{
			color = color_; return *this;
		}
								 //!< Sets the current owner
		gZone &         SetOwner            (ePlayerNetID *pOwner)
		{
			pOwner_ = pOwner; return *this;
		}
								 //!< Sets the current owner
		ePlayerNetID *  GetOwner            ()
		{
			return pOwner_;
		}
								 //!< Sets the current seeking cycle
		gZone &         SetSeekingCycle     (gCycle *pCycle)
		{
			if (pCycle)
			{
				seeking_ = true;
			}
			else
			{
				seeking_ = false;
			}
			pSeekingCycle_ = pCycle; return *this;
		}
								 //!< Sets the current seeking cycle
		gCycle *        GetSeekingCycle     ()
		{
			return pSeekingCycle_;
		}
								 //!< Sets the target radius
		gZone &         SetTargetRadius     (REAL radius)
		{
			targetRadius_ = radius; return *this;
		}
								 //!< Sets the fall speed
		gZone &         SetFallSpeed        (REAL speed)
		{
			fallSpeed_ = speed; return *this;
		}
		void BounceOffPoint(eCoord dest, eCoord collide);

		gZone & AddWaypoint(eCoord const &point);
		void Destroy();
		bool destroyed_;

		tString             GetName() {return name_;}
		void                SetName(tString name) {name_ = name;}
		static int          FindFirst(tString name);
		static int          FindNext(tString name, int prev_pos);
	protected:
		bool wallInteract_;
		int wallBouncesLeft_;
		REAL lastImpactTime_;
		REAL newImpactTime_;
		eCoord newImpactPos_;
		eCoord newImpactVelocity_;

		bool dynamicCreation_;	 //??? remove
		ePlayerNetID *pOwner_;
		gCycle *pSeekingCycle_;	 //!< cycle owner of this zone
		bool seeking_;
		REAL targetRadius_;
		REAL expectedRadius_;
		bool resizeRequested_;
		REAL previousExpansionSpeed_;
		REAL fallSpeed_;
		REAL lastSeekTime_;

		tString         name_;
		gRealColor color_;		 //!< the zone's color
		REAL createTime_;		 //!< the time the zone was created at

		REAL referenceTime_;	 //!< reference time for function evaluations
		tFunction posx_;		 //!< time dependence of x component of position
		tFunction posy_;		 //!< time dependence of y component of position
		tFunction radius_;		 //!< time dependence of radius
		tFunction rotationSpeed_;//!< the zone's rotation speed
		eCoord    rotation_;	 //!< the current rotation state

		std::vector<eCoord> route_;
		unsigned int lastCoord_;
		REAL nextUpdate_;

								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
		virtual void OnVanish(); //!< called when the zone vanishes

	private:
								 //!< writes data for network constructor
		virtual void WriteCreate(nMessage &m);
								 //!< writes sync data
		virtual void WriteSync(nMessage &m);
								 //!< reads sync data
		virtual void ReadSync(nMessage &m);

								 //!< looks for objects inzide the zone and reacts on them
		virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 );

								 //!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );
								 //!< reacts on objects inside the zone
		virtual void OnEnter( gZone *target, REAL time );

								 //!< returns the descriptor to recreate this object over the network
		virtual nDescriptor& CreatorDescriptor() const;

		REAL Radius() const;	 //!< returns the current radius

								 //!< renders the zone
		virtual void Render(const eCamera *cam);

		//! draws it in a svg file
		virtual void DrawSvg(std::ofstream &f);

		//! returns whether the rendering uses alpha blending (massively, so sorting errors would show)
		virtual bool RendersAlpha() const;

								 //!< evaluates the given function with lastTime - referenceTime_ as argument
		inline REAL EvaluateFunctionNow( tFunction const & f ) const;
								 //!< makes sure EvaluateFunctionNow() returns the given value
		inline void SetFunctionNow( tFunction & f, REAL value ) const;
};

// all the following zones are hacks until the full zone system is in place

//! win zone: lets players who enter win the round
class gWinZoneHack: public gZone
{
	public:
								 //!< local constructor
		gWinZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false);
								 //!< network constructor
		gWinZoneHack(nMessage &m);
		~gWinZoneHack();		 //!< destructor

	protected:
	private:
								 //!< reacts on objects inside the zone (declares them the winner)
		virtual void OnEnter( gCycle *target, REAL time );
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

								 //!< local constructor
		gDeathZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL );
								 //!< network constructor
		gDeathZoneHack(nMessage &m);
		~gDeathZoneHack();		 //!< destructor

		gDeathZoneHack *pLastShotCollision;

								 //!< Sets the current type
		gZone &         SetType            (int type);
		int             GetType            () {return (deathZoneType);}

								 //!< reacts on objects inside the zone
		virtual void OnEnter( gDeathZoneHack *target, REAL time );

	protected:
		virtual void OnVanish(); //!< called when the zone vanishes

		int deathZoneType;

	private:
								 //!< reacts on objects inside the zone (kills them)
		virtual void OnEnter( gCycle *target, REAL time );

		gCycle * getPlayerCycle(ePlayerNetID *pPlayer);
};
//! Rubber zone: Increase players rubber on enter
class gRubberZoneHack: public gZone
{
	public:

								 //!< local constructor
		gRubberZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false);
								 //!< network constructor
		gRubberZoneHack(nMessage &m);
		~gRubberZoneHack();		 //!< destructor


								 //!< Sets the current rubber
		gZone &         SetRubber            (REAL rubber);
		REAL            GetRubber            () {return (rmRubber);}

								 //!< reacts on objects inside the zone
	protected:
		virtual void OnVanish(); //!< called when the zone vanishes
		REAL rmRubber;

	private:
								 //!< reacts on objects inside the zone (kills them)
		virtual void OnEnter( gCycle *target, REAL time );

		gCycle * getPlayerCycle(ePlayerNetID *pPlayer);
};

//! base zone: belongs to a team, enemy players who manage to stay inside win the round (will be replaced
class gBaseZoneHack: public gZone
{
	public:
								 //!< local constructor
		gBaseZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL );
								 //!< network constructor
		gBaseZoneHack(nMessage &m);
		~gBaseZoneHack();		 //!< destructor

		bool CheckTeamAssignment(); //!< Check if this zone is assigned to a team, if not, try to assign one.
	private:
								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);

								 //!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );
								 //!< reacts on objects inside the zone
		virtual void OnEnter( gZone *target, REAL time );
		virtual void OnVanish(); //!< called when the zone vanishes
								 //!< called when the zone gets conquered
		virtual void OnConquest();
								 //!< checks for the only surviving zone
		virtual void CheckSurvivor();
								 //!< called on the beginning of the round
		virtual void OnRoundBegin();
								 //!< called on the end of the round
		virtual void OnRoundEnd();

		void ZoneWasHeld();		 //!< call when the zone was held as long as possible with the set game rules

								 //!< counts the zones belonging to the given team.
		static void CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest );

		REAL conquered_;		 //!< conquest status; zero if it is free, 1 if it has been completely conquered by the enemy
								 //!< time spend in the zone
		REAL conquerer_[MAXCLIENTS+1];
		int enemiesInside_;		 //!< count of enemies currently inside the zone
								 //!< name of the first enemy player that was inside us
		tColoredString enemyPlayerName_;

		int ownersInside_;		 //!< count of owners currently inside the zone
								 //!< name of the first team player that was inside us
		tColoredString teamPlayerName_;

		bool onlySurvivor_;		 //!< flag set if this zone is the only survivor

		typedef std::vector< tJUST_CONTROLLED_PTR< eTeam > > TeamArray;
		TeamArray enemies_;		 //!< list of teams that currently have a player in the zone
		REAL lastEnemyContact_;	 //!< last time an enemy player was in the zone

		REAL teamDistance_;		 //!< distance to the closest member of the owning team

		bool touchy_;			 //!< flag set when the zone is "touchy", which makes it get conquered on the slightest enemy touch

		//! possible states
		enum State
		{
			State_Safe,			 //!< not yet conquered
			State_Conquering,	 //!< conquering in this frame
			State_Conquered		 //!< conquered
		};
		State currentState_;	 //!< the current state

		REAL lastSync_;			 //!< time of the last sync request

		REAL lastRespawnRemindTime_;
		int lastRespawnRemindWaiting_;
};

class gBallZoneHack: public gZone
{
	public:
								 //!< local constructor
		gBallZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL );
								 //!< network constructor
		gBallZoneHack(nMessage &m);
		~gBallZoneHack();		 //!< destructor

		void RemovePlayer(ePlayerNetID *player);
		ePlayerNetID * GetLastPlayer();
		void GoHome();

	protected:
		bool init_;
		eCoord originalPosition_;
		eCoord originalVelocity_;

	private:
		virtual void OnVanish(); //!< called when the zone vanishes
								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
								 //!< reacts on objects inside the zone (kills them)
		virtual void OnEnter( gCycle *target, REAL time );

		tJUST_CONTROLLED_PTR<ePlayerNetID> lastPlayer_;
};

class gFlagZoneHack: public gZone
{
public:
								 //!< local constructor
		gFlagZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL );
    gFlagZoneHack(nMessage &m);                                  //!< network constructor
    ~gFlagZoneHack();                                            //!< destructor

    void SetTeam(tJUST_CONTROLLED_PTR< eTeam > team) { this->team = team; }

    void WarnFlagNotHome();
    bool IsHome();
    void GoHome();
    void RemoveOwner();
    void OwnerDropped();
    gCycle* Owner(){return owner_;}

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

class gTargetZoneHack: public gZone
{
	public:
								 //!< local constructor
		gTargetZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false );
								 //!< network constructor
		gTargetZoneHack(nMessage &m);
		~gTargetZoneHack();		 //!< destructor

	protected:

	private:
								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
		virtual void OnVanish(); //!< called when the zone vanishes
								 //!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );

								 //!< count check zone on grid
		static int TargetZoneCounter_;
		static REAL winnerTime_; //!< game time when a winner can be declared if nothing happens soon
								 //!< first player entering the last zone to be declare winner
		static ePlayerNetID *winner_;
								 //!< first player entering the zone
		ePlayerNetID *firstPlayer_;
								 //!< flags for players who already use the zone
		int playersFlags[MAXCLIENTS+1];
		int zoneInitialScore_;	 //!< score to give to the first player entering the zone
		int zoneScore_;			 //!< score to give to the next player entering the zone
		int zoneScoreDeplete_;	 //!< value to substract from score each time a player enter the zone
		REAL timeFirstEntry_;	 //!< game time of the first player entering the zone
		REAL targetEmptyTime_;	 //!< game time of the last points granted ...
		//! possible states
		enum State
		{
			State_Safe,			 //!< not yet conquered
			State_Conquering,	 //!< conquering in this frame
			State_Conquered		 //!< conquered
		};
		State currentState_;	 //!< the current state
		tString OnEnterCmd, OnVanishCmd; //!< commands to be parse when the first player enter the target and when the target vanish
	public:
		void SetOnEnterCmd(tString &cmd, tString &mode) {if (mode=="add") OnEnterCmd << "\n" << cmd; else OnEnterCmd = cmd;};
		void SetOnVanishCmd(tString &cmd, tString &mode) {if (mode=="add") OnVanishCmd << "\n" << cmd; else OnVanishCmd = cmd;};
};

class gBlastZoneHack: public gZone
{
	public:
								 //!< local constructor
		gBlastZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false );
								 //!< network constructor
		gBlastZoneHack(nMessage &m);
		~gBlastZoneHack();		 //!< destructor

	protected:

	private:
								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
		virtual void OnVanish(); //!< called when the zone vanishes
								 //!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );
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
