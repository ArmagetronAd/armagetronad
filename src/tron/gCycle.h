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

#ifndef ArmageTron_CYCLE_H
#define ArmageTron_CYCLE_H

//#define USE_HEADLIGHT

#include "gStuff.h"
//#include "rTexture.h"
//#include "rModel.h"
#include "eNetGameObject.h"
#include "tList.h"
#include "nObserver.h"

#include "gCycleMovement.h"

class rModel;
class gTextureCycle;
class gSensor;
class gNetPlayerWall;
class gPlayerWall;
class eTempEdge;
struct gPredictPositionData;

// minimum time between two cycle turns
extern REAL sg_delayCycle;

// Render the headlight effect?
extern bool headlights;

// steering help
extern REAL sg_rubberCycle;


// this class set is responsible for remembering which walls are too
// close together to pass through safely. The AI uses this information,
// so the real declaration of gCylceMemoryEntry can be found in gAIBase.cpp.
class gCycleMemoryEntry;

class gCycleMemory{
    friend class gCycleMemoryEntry;

    tList<gCycleMemoryEntry>     memory;  // memory about other cylces

public:
    // memory functions: access the memory for a cylce
    gCycleMemoryEntry* Remember(const gCycle *cycle);
    int Len() const {return memory.Len();}
    gCycleMemoryEntry* operator() (int i)  const;
    gCycleMemoryEntry* Latest   (int side)  const;
    gCycleMemoryEntry* Earliest (int side)  const;

    void Clear();

    gCycleMemory();
    ~gCycleMemory();
};

// class used to extrapolate the movement of a lightcycle
class gCycleExtrapolator: public gCycleMovement
{
public:
    void CopyFrom( const gCycleMovement& other );							// copies relevant info from other cylce
    void CopyFrom( const SyncData& sync, const gCycle& other );	        	// copies relevant info from sync data and everything else from other cycle

    gCycleExtrapolator(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID *p=NULL,bool autodelete=1);
    // gCycleExtrapolator(nMessage &m);
    virtual ~gCycleExtrapolator();

    // virtual gDestination* GetCurrentDestination() const;			// returns the current destination

    virtual bool EdgeIsDangerous(const eWall *w, REAL time, REAL a) const;

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int recursion=1);

    virtual bool TimestepCore(REAL currentTime, bool calculateAcceleration = true );

    // virtual bool DoTurn(int dir);

    REAL			  trueDistance_;										// distance predicted as best as we can
private:
    // virtual REAL            DoGetDistanceSinceLastTurn  (                               ) const     ;   //!< returns the distance since the last turn

    virtual nDescriptor& CreatorDescriptor() const;

    const gCycleMovement* parent_;												// the cycle that is extrapolated
};

// a complete lightcycle
class gCycle: public gCycleMovement
{
    friend class gPlayerWall;
    friend class gNetPlayerWall;
    friend class gDestination;

    REAL spawnTime_;    //!< time the cycle spawned at
    REAL lastTimeAnim;  //!< last time animation was simulated at

    REAL timeCameIntoView;

    REAL nextChatAI;
    bool dropWallRequested_; //!< flag indicating that someone requested a wall drop

public:
    eCoord            lastGoodPosition_;    // the location of the last known good position

    REAL skew,skewDot;						// leaning to the side

    bool 				mp; 				// use moviepack or not?

    rModel *body,*front,*rear,
    *customModel;

    gTextureCycle  *wheelTex,*bodyTex;
    gTextureCycle  *customTexture;

    eCoord rotationFrontWheel,rotationRearWheel; 	// wheel position (rotation)
    REAL   heightFrontWheel,heightRearWheel;  		// wheel (suspension)
public:
    //REAL	brakingReservoir; // reservoir for braking. 1 means full, 0 is empty

    static uActionPlayer s_brake;
    gCycleMemory memory;

    gRealColor color_;
    gRealColor trailColor_;

    // smooth corrections
    // pos is always the correct simulated position; the displayed position is calculated as pos + correctPosSmooth
    // and correctPosSmooth decays with time.
    eCoord correctPosSmooth;
    eCoord predictPosition_; //!< the best guess of where the cycle is at at display time

    // every frame, a bit of this variable is taken away and added to the step the cycle makes.
    REAL correctDistanceSmooth;

private:
    void TransferPositionCorrectionToDistanceCorrection();

    tCHECKED_PTR(gNetPlayerWall)	currentWall;                    //!< the wall that currenly is attached to the cycle
    tCHECKED_PTR(gNetPlayerWall)	lastWall;                       //!< the last wall that was attached to this cycle
    tCHECKED_PTR(gNetPlayerWall)	lastNetWall;                    //!< the last wall received over the network

    // for network prediction
    SyncData									lastSyncMessage_;	// the last sync message the cycle received
    tJUST_CONTROLLED_PTR<gCycleExtrapolator>	extrapolator_;		// the cycle copy used for extrapolation
    bool										resimulate_;		// flag indicating that a new extrapolation should be started

    void	ResetExtrapolator();							// resets the extrapolator to the last known state
    bool	Extrapolate( REAL dt );							// simulate the extrapolator at higher speed
    void	SyncFromExtrapolator();							// take over the extrapolator's data

    virtual void OnNotifyNewDestination(gDestination *dest);   //!< called when a destination is successfully inserted into the destination list
    virtual void OnDropTempWall        ( gPlayerWall * wall, eCoord const & position, eCoord const & direction );   //!< called when another cycle grinds a wall; this cycle should then drop its current wall if the grinding is too close.

    //	unsigned short currentWallID;

    nTimeRolling nextSync, nextSyncOwner;

    void MyInitAfterCreation();

    void SetCurrentWall(gNetPlayerWall *w);

    void PreparePredictPosition( gPredictPositionData & data ); //!< prepares CalculatePredictPosition() call, requesting a raycast to the front
    REAL CalculatePredictPosition( gPredictPositionData & data ); //!< Calculates predictPosition_
protected:
    virtual ~gCycle();

    virtual void RemoveFromGame(); // call this instead of the destructor

    // virtual REAL            DoGetDistanceSinceLastTurn  (                               ) const     ;   //!< returns the distance since the last turn
public:
    virtual void Die ( REAL time )  ;  //!< dies at the specified time
    void KillAt( const eCoord& pos );  //!< kill this cycle at the given position and take care of scoring

    int WindingNumber() const {return windingNumber_;}

    virtual bool            Vulnerable              ()                                    const     ;   //!< returns whether the cycle can be killed

    // bool CanMakeTurn() const { return pendingTurns <= 0 && lastTime >= nextTurn; }

    virtual void InitAfterCreation();
    gCycle(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID *p=NULL,bool autodelete=1);

    static	void 	SetWallsStayUpDelay		( REAL delay );				//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	void 	SetWallsLength			( REAL length);				//!< the maximum total length of the walls
    static	void 	SetExplosionRadius		( REAL radius);				//!< the radius of the holes blewn in by an explosion

    static	REAL 	WallsStayUpDelay()	 { return wallsStayUpDelay;	}	//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	REAL	WallsLength()	 	 { return wallsLength;		}	//!< the default total length of the walls
    REAL	        MaxWallsLength() const;                             //!< the maximum total length of the walls (including max effect of rubber growth)
    REAL	        ThisWallsLength() const;                            //!< the maximum total length of this cycle's wall (including rubber shrink)
    REAL	        WallEndSpeed() const;                               //!< the speed the end of the trail is receeding with right now
    static	REAL	ExplosionRadius()	 { return explosionRadius;	}	//!< the radius of the holes blewn in by an explosion

    bool    IsMe( eGameObject const * other ) const;              //!< checks whether the passed pointer is logically identical with this cycle

    // the network routines:
    gCycle(nMessage &m);
    virtual void WriteCreate(nMessage &m);
    virtual void WriteSync(nMessage &m);
    virtual void ReadSync(nMessage &m);
    virtual void RequestSyncOwner(); //!< requests special syncs to the owner on important points (just passed an enemy trail end safely...)
    virtual void RequestSyncAll(); //!< requests special syncs to everyone on important points (just passed an enemy trail end safely...)

    virtual void SyncEnemy ( const eCoord& begWall );    //!< handle sync message for enemy cycles
    // virtual void SyncFriend( const eCoord& begWall );    //!< handle sync message for enemy cycles

    virtual void ReceiveControl(REAL time,uActionPlayer *Act,REAL x);
    virtual void PrintName(tString &s) const;
    virtual bool ActionOnQuit();

    virtual nDescriptor &CreatorDescriptor() const;
    virtual bool SyncIsNew(nMessage &m);
    //virtual bool ClearToTransmit(int user) const;

    virtual bool Timestep(REAL currentTime);
    virtual bool TimestepCore(REAL currentTime,bool calculateAcceleration = true);

    virtual void InteractWith(eGameObject *target,REAL time,int recursion=1);

    virtual bool EdgeIsDangerous(const eWall *w, REAL time, REAL a) const;

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int recursion=1);

    virtual REAL PathfindingModifier( const eWall *w ) const;

    virtual bool Act(uActionPlayer *Act,REAL x);

    virtual bool DoTurn(int dir);
    void DropWall( bool buildNew=true );                                    //!< Drops the current wall and builds a new one

    // void Turbo(bool turbo);

    virtual void Kill();

    const eTempEdge* Edge();
    const gPlayerWall* CurrentWall();
    // const gPlayerWall* LastWall();

#ifndef DEDICATED
    virtual void Render(const eCamera *cam);

    virtual void RenderName( const eCamera *cam );

    virtual bool RenderCockpitFixedBefore(bool primary=true);

    //virtual void SoundMix(unsigned char *dest,unsigned int len,
    //                      int viewer,REAL rvol,REAL lvol);
#endif

    virtual eCoord CamPos();
    virtual eCoord PredictPosition();
    virtual eCoord  CamTop();

    virtual void RightBeforeDeath( int numTries );

#ifdef POWERPAK_DEB
    virtual void PPDisplay();
#endif

    static 	void	PrivateSettings();									// initiate private setting items

    //	virtual void AddRef();
    //	virtual void Release();

private:
    static	REAL	wallsStayUpDelay;			//!< the time the cycle walls stay up ( negative values: they stay up forever )
    static	REAL	wallsLength;				//!< the maximum total length of the walls
    static	REAL	explosionRadius;			//!< the radius of the holes blewn in by an explosion

protected:
    virtual 	bool 			DoIsDestinationUsed		( const gDestination *	dest		) const		;	//!< returns whether the given destination is in active use
};

#endif

