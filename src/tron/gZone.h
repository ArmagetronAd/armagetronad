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

#ifndef ArmageTron_Zone_H
#define ArmageTron_Zone_H

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
    gZone(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false); //!< local constructor
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
    gZone &         SetRadiusSmoothly   ( REAL radius, REAL expansionSpeed = 5); //!< Gets the current radius
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
    //HACK RACE
    void            Vanish				(REAL factor = 0.5);		//!< let the zone vanish

    gZone &         SetWallInteract     (bool wallInteract) {wallInteract_=wallInteract; return *this;}
    gZone &         SetWallBouncesLeft  (int wallBouncesLeft) {wallBouncesLeft_=wallBouncesLeft; return *this;}
    bool            GetWallInteract     () { return wallInteract_; }
    int             GetBouncesLeft      () { return wallBouncesLeft_; }

    gZone &         SetColor            (gRealColor color) {color_ = color; return *this;}      //!< Sets the current color
    gRealColor &    GetColor            () {return color_;}             //!< Gets the current color
    gZone &         GetColor            (gRealColor & color) {color = color_; return *this;}    //!< Gets the current color
    gZone &         SetOwner            (ePlayerNetID *pOwner); //!< Sets the current owner
    ePlayerNetID *  GetOwner            () {return pOwner_;}  //!< Gets the current owner
    gZone &         SetSeekingCycle     (gCycle *pCycle) {if (pCycle) {seeking_ = true;} else {seeking_ = false;} pSeekingCycle_ = pCycle; return *this;}  //!< Sets the current seeking cycle
    gCycle *        GetSeekingCycle     () {return pSeekingCycle_;}  //!< Sets the current seeking cycle
    gZone &         SetTargetRadius     (REAL radius) {targetRadius_ = radius; return *this;}      //!< Sets the target radius
    gZone &         SetFallSpeed        (REAL speed) {fallSpeed_ = speed; return *this;}      //!< Sets the fall speed
    void BounceOffPoint(eCoord dest, eCoord collide);
    gZone & AddWaypoint(eCoord const &point);
    void Destroy();
    bool destroyed_;
    tString             GetName() {return name_;}
    void                SetName(tString name) {name_ = name;}
    static int          FindFirst(tString name);
    static int          FindNext(tString name, int prev_pos);

    tString             GetEffect() {return effect_;}
    void                SetEffect(tString newEffect) {effect_ = newEffect;}

    bool                GetWallPenetrate() { return wallPenetrate_; }
    void                SetWallPenetrate (bool penetrate = false) { wallPenetrate_ = penetrate; }

    static void Timesteps(REAL currentTime);
    static void AddDelay(REAL delayTime, gZone *Zone);
    static void ClearDelay();

    virtual void Collapse();    //  have the zone disappear instantly
    static void GridPosLadderLog(); //  gridpos ladderlog for the zones

protected:
    bool wallInteract_;
    int wallBouncesLeft_;
    eWall *pLastWall_; // dumb pointer is OK, it is never dereferenced.
    REAL lastImpactTime_;
    REAL newImpactTime_;
    eCoord newImpactPos_;
    eCoord newImpactVelocity_;

    bool wallPenetrate_;

    bool dynamicCreation_;  //??? remove
    bool delayCreation_;
    tJUST_CONTROLLED_PTR< ePlayerNetID > pOwner_;
    tJUST_CONTROLLED_PTR< gCycle > pSeekingCycle_;       //!< cycle owner of this zone
    bool seeking_;
    REAL targetRadius_;
    REAL expectedRadius_;
    bool resizeRequested_;
    REAL previousExpansionSpeed_;
    REAL fallSpeed_;
    REAL lastSeekTime_;
    bool zoneInit_;

    tString name_;
    tString effect_;
    gRealColor color_;           //!< the zone's color
    REAL createTime_;            //!< the time the zone was created at

    REAL referenceTime_;         //!< reference time for function evaluations
    tFunction posx_;             //!< time dependence of x component of position
    tFunction posy_;             //!< time dependence of y component of position
    tFunction radius_;           //!< time dependence of radius
    tFunction rotationSpeed_;    //!< the zone's rotation speed
    eCoord    rotation_;         //!< the current rotation state

    std::vector<eCoord> route_;
    unsigned int lastCoord_;
    REAL nextUpdate_;

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnVanish();                     //!< called when the zone vanishes

private:
    virtual void WriteCreate(nMessage &m); //!< writes data for network constructor
    virtual void WriteSync(nMessage &m);   //!< writes sync data
    virtual void ReadSync(nMessage &m);    //!< reads sync data

    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 ); //!< looks for objects inzide the zone and reacts on them

    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone
    virtual void OnEnter( gZone *target, REAL time );  //!< reacts on objects inside the zone

    virtual void OnExit( gCycle *target, REAL time );   //!< reacts to objects leaving the zone
    virtual void OnExit( gZone *target, REAL time );    //!< reacts to objects leaving the zone

    virtual void OnNear( gCycle *target, REAL time );   //!< reacts to objects near the zone
    virtual void OnNear( gZone *target, REAL time );    //!< reacts to objects near the zone

    tArray<gCycle *> cycesInside_;
    void AddPlayerInteraction(gCycle *cycle) { cycesInside_.Insert(cycle); }
    void RemovePlayerInteraction(gCycle *cycle)
    {
        for(int i=0; i < cycesInside_.Len(); i++)
        {
            gCycle *p = cycesInside_[i];
            if (p && (p == cycle))
            {
                cycesInside_.RemoveAt(i);
                break;
            }
        }
    }
    bool isInside(gCycle *cycle)
    {
        for(int i=0; i < cycesInside_.Len(); i++)
        {
            gCycle *p = cycesInside_[i];
            if (p && (p == cycle))
            {
                return true;
            }
        }
        return false;
    }

    static std::map<REAL, std::set<gZone*> > delayedZones_;

    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network

    REAL Radius() const;           //!< returns the current radius

    virtual void Render(const eCamera *cam);  //!< renders the zone

    virtual void DrawSvg(std::ofstream &f); //! draws it in a svg file

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
								 //!< local constructor
		gWinZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
								 //!< network constructor
		gWinZoneHack(nMessage &m);
		~gWinZoneHack();		 //!< destructor

		static bool WinnerPlayer() { winnerPlayer_; }
		static void SetWinnerPlayer(bool val) { winnerPlayer_ = val; }

	protected:
        static bool winnerPlayer_;
	private:
								 //!< reacts on objects inside the zone (declares them the winner)
		virtual void OnEnter( gCycle *target, REAL time );
		virtual void OnExit( gCycle *target, REAL time );   //!< reacts to players leaving the zone
};

//! death zone: kills players who enter
class gDeathZoneHack: public gZone
{
	public:

		enum
		{
			TYPE_NORMAL,
			TYPE_TEAM,
			TYPE_SHOT,
			TYPE_DEATH_SHOT,
			TYPE_SELF_DESTRUCT,
			TYPE_ZOMBIE_ZONE,

			NUM_DEATH_ZONE_TYPES
		};

								 //!< local constructor
		gDeathZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL, bool delayCreation = false );
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
        virtual bool Timestep(REAL currentTime);

								 //!< reacts on objects inside the zone (kills them)
		virtual void OnEnter( gCycle *target, REAL time );

		gCycle * getPlayerCycle(ePlayerNetID *pPlayer);
};
//! Rubber zone: Increase players rubber on enter
class gRubberZoneHack: public gZone
{
	public:
        enum
		{
			TYPE_ADJUST,
			TYPE_RUBBER,

			NUM_RUBBER_ZONES
		};
								 //!< local constructor
		gRubberZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
								 //!< network constructor
		gRubberZoneHack(nMessage &m);
		~gRubberZoneHack();		 //!< destructor

        void SetRubberType(int type) { this->rubberType_ = type; }
        int  GetRubberType()         { return this->rubberType_; }

								 //!< Sets the current rubber
		gZone &         SetRubber            (REAL rubber);
		REAL            GetRubber            () {return (rmRubber);}

								 //!< reacts on objects inside the zone
	protected:
		virtual void OnVanish(); //!< called when the zone vanishes
		REAL rmRubber;
		int rubberType_;

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
		gBaseZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL, bool delayCreation = false );
								 //!< network constructor
		gBaseZoneHack(nMessage &m);
		~gBaseZoneHack();		 //!< destructor

		bool CheckTeamAssignment(); //!< Check if this zone is assigned to a team, if not, try to assign one.
        bool CheckTeamAssignmentOnTeam(); //!< If a zone is assigned a team It will make sure any unsigned zone that is closer to a team will have this team.
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

        gCycle *enemyPlayer_;     //!< the first enemy player that was inside us

		int ownersInside_;		 //!< count of owners currently inside the zone
		gCycle *teamPlayer_;     //!< the first team player that was inside us


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
//! sumo zone: creates base zones for teams
class gSumoZoneHack: public gZone
{
	public:
								 //!< local constructor
		gSumoZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
								 //!< network constructor
		gSumoZoneHack(nMessage &m);
		~gSumoZoneHack();		 //!< destructor
        void SetUnspawnedState(){currentState_=State_Unspawned;}
    protected:
        virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    private:
        enum State
		{
            State_Parsing,      //!< This zone is still being created
			State_Unspawned,	//!< team zones have yet to be created
			State_Spawned	   //!< Team zones have been created

		};
		State currentState_;	 //!< the current state
        void SetStateParsing(){currentState_=State_Parsing;}
        virtual void OnVanish();                     //!< called when the zone vanishes

};
class gBallZoneHack: public gZone
{
	public:
								 //!< local constructor
		gBallZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL, bool delayCreation = false );
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
    gFlagZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam * teamowner = NULL, bool delayCreation = false );
    gFlagZoneHack(nMessage &m);                                  //!< network constructor
    ~gFlagZoneHack();                                            //!< destructor

    void SetTeam(tJUST_CONTROLLED_PTR< eTeam > team) { this->team = team; }
    eTeam *Team(){return team;}
    void WarnFlagNotHome();
    bool IsHome();
    void GoHome();
    void RemoveOwner();
    void OwnerDropped();
    gCycle *Owner(){return owner_;}

    void PassTheFlag(tString name = tString(""));
    void PassComplete(gCycle *target);
    void PassFailed(gCycle *target);
    void SetPassing(bool passed, gCycle *passOwner = NULL);

protected:
    bool init_;
    eCoord originalPosition_;
    eCoord homePosition_;
    REAL originalRadius_;
    tJUST_CONTROLLED_PTR<gCycle> owner_;
    REAL ownerTime_;
    bool ownerWarnedNotHome_;
    REAL chatBlinkUpdateTime_;
    REAL blinkUpdateTime_;
    REAL blinkTrackUpdateTime_;
    tJUST_CONTROLLED_PTR<gCycle> ownerDropped_;
    REAL ownerDroppedTime_;
    REAL lastHoldScoreTime_;
    bool positionUpdatePending_;

    bool passingTheFlag_;
    gCycle *passingOwner_;
    gCycle *passerOwner_;
    bool passerOwnerFree_;

private:
    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime
    virtual void OnEnter( gCycle *target, REAL time ); //!< reacts on objects inside the zone (kills them)
    virtual void OnExit( gCycle *target, REAL time ); //!< reacts on objects exits the zone (them)
};

class gTargetZoneHack: public gZone
{
	public:
								 //!< local constructor
		gTargetZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false );
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
		virtual void OnExit( gCycle *target, REAL time );   //!< reacts to objects leaving zone

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
		REAL winzoneEmptyTeam_;  //!< game time of the last points granted .
		//! possible states
		enum State
		{
			State_Safe,			 //!< not yet conquered
			State_Conquering,	 //!< conquering in this frame
			State_Conquered		 //!< conquered
		};
		State currentState_;	 //!< the current state
		tString OnEnterCmd, OnExitCmd, OnVanishCmd; //!< commands to be parse when the first player enter the target and when the target vanish
	public:
		void SetOnEnterCmd(tString &cmd, tString &mode) {if (mode=="add") OnEnterCmd << "\n" << cmd; else OnEnterCmd = cmd;};
		void SetOnExitCmd(tString &cmd, tString &mode) {if (mode=="add") OnExitCmd << "\n" << cmd; else OnExitCmd = cmd;};
		void SetOnVanishCmd(tString &cmd, tString &mode) {if (mode=="add") OnVanishCmd << "\n" << cmd; else OnVanishCmd = cmd;};
};
class gKOHZoneHack: public gZone
{
	public:
								 //!< local constructor
		gKOHZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false );
								 //!< network constructor
		gKOHZoneHack(nMessage &m);
		~gKOHZoneHack();		 //!< destructor

	protected:

	private:
								 //!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
		virtual void OnVanish(); //!< called when the zone vanishes
								 //!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );

								 //!< count check zone on grid
		int PlayersInside_;
		REAL EnteredTime_; //!< game time when a winner can be declared if nothing happens soon
								 //!< first player entering the last zone to be declare winner
		gCycle *owner_;
								 //!< first player entering the zone
	public:
};

class gTeleportZoneHack: public gZone
{
	public:
								 //!< local constructor
		gTeleportZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false );
								 //!< network constructor
		gTeleportZoneHack(nMessage &m);
		~gTeleportZoneHack();		 //!< destructor

	void SetJump(eCoord coord, int rel) { this->jump = coord; this->relative = rel; }
	eCoord GetJump() { return this->jump; }
	void SetNewDir(eCoord coord) { this->ndir = coord; }
	eCoord GetNewDir() { return this->ndir; }
	int IsRelativeJump() { return this->relative; }
	void SetReloc(REAL factor) { this->reloc = factor; }
	REAL GetReloc() { return this->reloc; }

	protected:

	private:
		//!< simulates behaviour up to currentTime
		virtual bool Timestep(REAL currentTime);
		//!< called when the zone vanishes
		virtual void OnVanish();
		//!< reacts on objects inside the zone
		virtual void OnEnter( gCycle *target, REAL time );

	eCoord jump;
	eCoord ndir;
	int relative; // 0=absolute ie map basis, 1=relative to cycle with map basis, 2=relative to cycle with cycle basis
	REAL reloc;   // add extra jump to pretend to cross the zone
};

class gBlastZoneHack: public gZone
	{
	public:

		//!< local constructor
		gBlastZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false );
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

class gSpeedZoneHack: public gZone
{
    public:
        enum
		{
			TYPE_ACCEL,
			TYPE_SPEED,

			NUM_SPEED_ZONES
		};

        gSpeedZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
        gSpeedZoneHack(nMessage &m);
        ~gSpeedZoneHack();

        void SetSpeedType(int type) { this->speedType_ = type; }
        int  GetSpeedType()         { return this->speedType_; }

        void SetSpeedVal(REAL speed){ this->setSpeed_ = speed; }
        REAL GetSpeedVal()          { return this->setSpeed_;  }

    protected:
        int speedType_;

        REAL setSpeed_;

    private:
        virtual bool Timestep(REAL currentTime);
        virtual void OnVanish();
        virtual void OnEnter(gCycle *target, REAL time);
};

class gObjectZoneHack: public gZone
{
    public:
        gObjectZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
        gObjectZoneHack(nMessage &m);
        ~gObjectZoneHack();

        virtual void OnEnter(gZone *target, REAL time);
        virtual void OnExit(gCycle *target, REAL time);

        REAL SeekUpdateTime()         { return this->seekUpdateTime_; }
        void SetSeekUpdate(REAL time) { this->seekUpdateTime_ = time; }

        REAL SeekSpeed()              { return this->seekSpeed_; }
        void SetSeekSpeed(REAL speed) { this->seekSpeed_ = speed; }

    protected:
        REAL seekUpdateTime_;
        REAL seekSpeed_;

    private:
        virtual bool Timestep(REAL currentTime);
        virtual void OnVanish();
        virtual void OnEnter(gCycle *target, REAL time);
};

class gSoccerZoneHack: public gZone
{
    public:
        enum
        {
            gSoccer_GOAL,
            gSoccer_BALL
        };

        gSoccerZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, bool delayCreation = false);
        gSoccerZoneHack(eGrid *grid, const eCoord &pos, bool dynamicCreation = false, eTeam *team = NULL, bool delayCreation = false);
        gSoccerZoneHack(nMessage &m);
        ~gSoccerZoneHack();

		void SetType(int settype) {zoneType = settype;}     //  set the type of the zone
		int GetType() {return (zoneType);}                  //  get the type of the zone

		bool CheckTeamAssignment();         //  check if the soccer goal zonehas been assigned a team owner

		virtual void OnEnter(gSoccerZoneHack *target, REAL time);

		//  if it's a ball, go home after either dying or hitting a goal
		void GoHome();

    protected:
        int zoneType;
        REAL teamDistance_;

        bool init_;
        eCoord originalPosition_;   //  the initial spawned position of the zone
        eCoord originalVelocity_;   //  the initial spawned velocity of the zone
        REAL originalRadius_;       //  the initial spawned radius of the zone

        eTeam *lastTeamIn_;     //  store the last player's team to enter the zone

        int ballShots_;     //  number of times ball has entered goals

    private:
        virtual bool Timestep(REAL currentTime);
        virtual void OnVanish();
        virtual void OnEnter(gCycle *target, REAL time);
};

class gRespawnZoneHack: public gZone
{
    public:
        gRespawnZoneHack(eGrid *grid, const eCoord &pos, ePlayerNetID *player, bool dynamicCreation = false, bool delayCreation = false);
        gRespawnZoneHack(nMessage &m);
        ~gRespawnZoneHack();

        ePlayerNetID *DeadPlayer() { return deadPlayer_; }
        void SetDeadPlayer(ePlayerNetID *player) { deadPlayer_ = player; }
        void ClearDeadPlayer() { deadPlayer_ = NULL; }

        eCoord SpawnDirection() { return this->spawnDirection_; }
        void SetSpawnDirection(eCoord dir) { this->spawnDirection_;  }

        void Finish();

    protected:
        ePlayerNetID *deadPlayer_;
        eCoord spawnDirection_;

    private:
        virtual bool Timestep(REAL currentTime);
        virtual void OnVanish();
        virtual void OnEnter(gCycle *target, REAL time);
};

class gCheckpointZoneHack: public gZone
{
    public:
        gCheckpointZoneHack(eGrid *grid, const eCoord &pos, int checkpointId, int checkpointTime = 0.0, bool dynamicCreation = false, bool delayCreation = false);
        gCheckpointZoneHack(nMessage &m);
        ~gCheckpointZoneHack();

        int CheckpointID()      { return checkpointId_; }
        int CheckpointTime()   { return checkpointTime_; }

    protected:
        int checkpointId_;
        int checkpointTime_;

        tArray<bool> wrongPlayerEntries_;

    private:
        virtual bool Timestep(REAL currentTime);
        virtual void OnVanish();
        virtual void OnEnter(gCycle *target, REAL time);    //!< reacts to players entering the zone
        virtual void OnExit( gCycle *target, REAL time );   //!< reacts to players leaving the zone
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

int sg_NumCheckpointZones();

extern bool sg_deathZoneRotation;
extern REAL sg_deathZoneRotationSpeed;

#endif
