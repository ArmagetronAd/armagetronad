%{
#include "gCycle.h"
%}

// a complete lightcycle
%rename(Cycle) gCycle;
class gCycle: public gCycleMovement
{
    friend class gPlayerWall;
    friend class gNetPlayerWall;
    friend class gDestination;
    friend class gCycleWallRenderer;

public:
    //REAL	brakingReservoir; // reservoir for braking. 1 means full, 0 is empty

    gRealColor color_;
    gRealColor trailColor_;

    // smooth corrections
    // pos is always the correct simulated position; the displayed position is calculated as pos + correctPosSmooth
    // and correctPosSmooth decays with time.
    eCoord correctPosSmooth;
    eCoord predictPosition_; //!< the best guess of where the cycle is at at display time

    // every frame, a bit of this variable is taken away and added to the step the cycle makes.
    REAL correctDistanceSmooth;

protected:
    virtual ~gCycle();

    virtual void OnRemoveFromGame(); // called when the cycle is physically removed from the game

    virtual void OnRoundEnd();    //!< called when the round ends

    // virtual REAL            DoGetDistanceSinceLastTurn  (                               ) const     ;   //!< returns the distance since the last turn
public:
    virtual void Die ( REAL time )  ;  //!< dies at the specified time
    void KillAt( const eCoord& pos );  //!< kill this cycle at the given position and take care of scoring

%rename(winding_number) WindingNumber;
    int WindingNumber() const {return windingNumber_;}

%rename(vulnerable) Vulnerable;
    virtual bool            Vulnerable              ()                                    const     ;   //!< returns whether the cycle can be killed

    // bool CanMakeTurn() const { return pendingTurns <= 0 && lastTime >= nextTurn; }

    virtual void InitAfterCreation();
    gCycle(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID *p=NULL);
%rename(respawn_cycle) RespawnCycle;
    static      bool    RespawnCycle(const eCoord &pos, const eCoord &dir, ePlayerNetID *p, bool warn=true);

    static	void 	SetWallsStayUpDelay		( REAL delay );				//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	void 	SetWallsLength			( REAL length);				//!< the maximum total length of the walls
    static	void 	SetExplosionRadius		( REAL radius);				//!< the radius of the holes blewn in by an explosion

    static	REAL 	WallsStayUpDelay()	 { return wallsStayUpDelay;	}	//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	REAL	WallsLength()	 	 { return wallsLength;		}	//!< the default total length of the walls
%rename(max_walls_length) MaxWallsLength;
    REAL	        MaxWallsLength() const;                             //!< the maximum total length of the walls (including max effect of rubber growth)
%rename(this_walls_length) ThisWallsLength;
    REAL	        ThisWallsLength() const;                            //!< the maximum total length of this cycle's wall (including rubber shrink)
    REAL	        WallEndSpeed() const;                               //!< the speed the end of the trail is receeding with right now
    static	REAL	ExplosionRadius()	 { return explosionRadius;	}	//!< the radius of the holes blewn in by an explosion

    bool    IsMe( eGameObject const * other ) const;              //!< checks whether the passed pointer is logically identical with this cycle

%rename(request_sync_owner) RequestSyncOwner;
    virtual void RequestSyncOwner(); //!< requests special syncs to the owner on important points (just passed an enemy trail end safely...)
%rename(request_sync_all) RequestSyncAll;
    virtual void RequestSyncAll(); //!< requests special syncs to everyone on important points (just passed an enemy trail end safely...)

    virtual void SyncEnemy ( const eCoord& begWall );    //!< handle sync message for enemy cycles
    // virtual void SyncFriend( const eCoord& begWall );    //!< handle sync message for enemy cycles

    //! creates a netobject form sync data
    gCycle( Game::CycleSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Game::CycleSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Game::CycleSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 


//    virtual void ReceiveControl(REAL time,uActionPlayer *Act,REAL x);
    virtual void PrintName(tString &s) const;
    virtual bool ActionOnQuit();

    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;

    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Game::CycleSync const & sync, nSenderInfo const & sender );

    virtual bool Timestep(REAL currentTime);
    virtual bool TimestepCore(REAL currentTime,bool calculateAcceleration = true);

    virtual void InteractWith(eGameObject *target,REAL time,int recursion=1);

    virtual bool EdgeIsDangerous(const eWall *w, REAL time, REAL a) const;

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int recursion=1);

    virtual REAL PathfindingModifier( const eWall *w ) const;

%rename(do_turn) DoTurn;
    virtual bool DoTurn(int dir);
%rename(drop_wall) DropWall;
    void DropWall( bool buildNew=true );                                    //!< Drops the current wall and builds a new one

    // void Turbo(bool turbo);

    virtual void Kill();

    const eTempEdge* Edge();
    const gPlayerWall* CurrentWall();
    // const gPlayerWall* LastWall();

    virtual eCoord CamPos() const;
    virtual eCoord PredictPosition() const;
    virtual eCoord CamTop() const;
    virtual eCoord CamDir()  const;
    virtual eCoord Direction()  const;

    virtual void RightBeforeDeath( int numTries );

#ifdef POWERPAK_DEB
//    virtual void PPDisplay();
#endif

    static 	void	PrivateSettings();									// initiate private setting items

    //	virtual void AddRef();
    //	virtual void Release();

private:
    static	REAL	wallsStayUpDelay;			//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	REAL	wallsLength;				//!< the maximum total length of the walls
    static	REAL	explosionRadius;			//!< the radius of the holes blewn in by an explosion
    gJoystick *     joystick_;                  //!< joystick control
protected:
    virtual 	bool 			DoIsDestinationUsed		( const gDestination *	dest		) const		;	//!< returns whether the given destination is in active use
};

