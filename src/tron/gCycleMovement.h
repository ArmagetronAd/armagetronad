/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2004  Armagetron Advanced Team (http://sourceforge.net/projects/armagetronad/) 

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

#ifndef     ARMAGETRONAD_SRC_TRON_GCYCLEMOVEMENT_H_INCLUDED
#define     ARMAGETRONAD_SRC_TRON_GCYCLEMOVEMENT_H_INCLUDED

#include "eNetGameObject.h"
#include <deque>

class gCycle;
class gDestination;
class gPlayerWall;

REAL GetTurnSpeedFactor(void);

//! class handling lightcycle movement aspects ( not networking beyond construction, no rendering, no wall building )
class gCycleMovement : public eNetGameObject
{
public:
    // accessors
    static float            SpeedMultiplier         ()                                              ;   //!< returns the current speed multiplier
    static void             SetSpeedMultiplier      ( REAL                  mult        )           ;   //!< sets the current speed multiplier
    static float            MaximalSpeed            ()                                              ;   //!< returns the maximal speed a cycle can reach on its own

    // AI info
    int    	                WindingNumber           ()                                    const     ;   //!< returns the current winding number
    void   	                SetWindingNumberWrapped ( int newWindingNumberWrapped )                 ;   //!< sets the new wrapped winding number

    // information about physics state
    virtual eCoord          Direction               ()                                    const     ;   //!< returns the current driving direction
    virtual eCoord          LastDirection           ()                                    const     ;   //!< returns the last driving direction
    virtual REAL            Speed                   ()                                    const     ;   //!< returns the current speed
    virtual bool            Alive                   ()                                    const     ;   //!< returns whether the cycle is still alive
    virtual bool            Vulnerable              ()                                    const     ;   //!< returns whether the cycle can be killed

    bool                    CanMakeTurn             ()                                    const     ;   //!< returns whether a turn is currently possible
    bool                    CanMakeTurn             ( REAL time                         ) const     ;   //!< returns whether a turn is possible at the given time
    inline  REAL            GetDistanceSinceLastTurn(                                   ) const     ;   //!< returns the distance since the last turn
    REAL                    GetTurnDelay            (                                   ) const     ;   //!< returns the time between turns
    REAL                    GetNextTurn             (                                   ) const     ;   //!< returns the time of the next turn

    // destination handling
    void                    AddDestination          ()                                              ;   //!< adds current position as destination
    void                    AdvanceDestination      ()                                              ;   //!< proceeds to the next destination
    void                    AddDestination          ( gDestination *        dest        )           ;   //!< adds given destination
    gDestination*           GetCurrentDestination   ()                                    const     ;   //!< returns the current destination
    void                    NotifyNewDestination    ( gDestination *        dest        )           ;   //!< notifies cycle of the insertion of a new destination ( don't call manually )
    bool                    IsDestinationUsed       ( const gDestination *  dest        ) const     ;   //!< returns whether the given destination is in active use

    inline void            DropTempWall             ( gPlayerWall *         wall        )           ;   //!< called when another cycle grinds a wall; this cycle should then drop its current wall if the grinding is too close.
    // information query
    virtual bool            EdgeIsDangerous         ( const eWall *         wall
            ,                                         REAL                  time
            ,                                         REAL                  alpha       ) const     ;   //!< returns whether a given wall is dangerous to this cycle

    // movement commands
    bool                    Turn                    ( REAL                  dir         )           ;   //!< Turn left for positive argument, right for negative argument
    bool                    Turn                    ( int                   dir         )           ;   //!< Turn left for positive argument, right for negative argument

    void                    MoveSafely              (const eCoord &         dest
            ,                                        REAL                   startTime
            ,                                        REAL                   endTime     )           ;   //!< move without throwing exceptions on passing a wall

    virtual bool            Timestep                ( REAL                  currentTime )           ;   //!< advance to the given time

    // existence management
    virtual void            AddRef                  ()                                              ;   //!< increase reference count

    gCycleMovement    	                            ( eGrid *               grid
            ,                                         const eCoord &        pos
            ,                                         const eCoord &        dir
            ,                                         ePlayerNetID *        p=NULL
                    ,                                 bool                  autodelete=1 )          ;   //!< local constructor
    gCycleMovement                                  ( nMessage &            message      )          ;   //!< remote constructor
    virtual ~gCycleMovement                         ()                                              ;   //!< destructor

protected:
    //! data from sync message
    struct SyncData
    {
        eCoord pos, dir, lastTurn;
        REAL distance, speed, time, rubber, rubberMalus, brakingReservoir;
        unsigned short turns,braking,messageID;

        SyncData()
                :distance(0), speed(0), time(-10000), rubber(0), rubberMalus(0), brakingReservoir(0)
                ,turns(0),braking(0),messageID(0)
        {}
    };

    void            CopyFrom                        ( const gCycleMovement & other      )           ;   //!< copies relevant info from other cylce

    void            CopyFrom                        ( const SyncData &       sync
            ,                                         const gCycleMovement & other      )           ;   //!< copies relevant info from sync data and everything else from other cycle

    virtual void            InitAfterCreation       ()                                              ;   //!< shared initialization routine

    // acceleration handling
    virtual void            CalculateAcceleration   ( REAL                  dt          )           ;   //!< calculate acceleration to apply later
    virtual void            ApplyAcceleration       ( REAL                  dt          )           ;   //!< apply acceleration calculated earlier

    // destination handling
    REAL                    DistanceToDestination   ( gDestination &        dest        ) const     ;   //!< calculates the distance to the given destination
    virtual void            OnNotifyNewDestination  ( gDestination *        dest        )           ;   //!< notifies cycle of the insertion of a new destination
    virtual void            OnDropTempWall          ( gPlayerWall *         wall        )           ;   //!< called when another cycle grinds a wall; this cycle should then drop its current wall if the grinding is too close.
    virtual bool            DoIsDestinationUsed     ( const gDestination *  dest        ) const     ;   //!< returns whether the given destination is in active use
    static  gDestination*   GetDestinationBefore    ( const SyncData &      sync
            ,                                         gDestination*         first       ) 		    ;   //!< determine the destination from before the sync message

    virtual bool            DoTurn                  ( int                   dir         )           ;   //!< turns the cycle in the given direction
    virtual REAL            DoGetDistanceSinceLastTurn  (                               ) const     ;   //!< returns the distance since the last turn

    virtual void            RightBeforeDeath        ( int                   numTries    )           ;   //!< called when the cycle is very close to a wall and about to crash
    void                    Die                     ( REAL time                         )           ;  //!< dies at the specified time

    virtual bool            TimestepCore            ( REAL                  currentTime )           ;   //!< core physics simulation routine
private:
    void                    MyInitAfterCreation     ()                                              ;   //!< private shared initialization code

    //      void            Init_gCycleCore         ()                                              ;   //!< initialisation function
    //      void            Finit_gCycleCore        ()                                              ;   //!< finalisation function

    gCycleMovement                                  ()                                              ;   //!< default constructor
    gCycleMovement                                  ( gCycleMovement const & other      )           ;   //!< copy constructor
    gCycleMovement& operator =                      ( gCycleMovement const & other      )           ;   //!< copy operator

private:
    short           alive_;                     //!< status: 1: cycle is alive, -1: cycle just died, 0: cycle is dead

protected:
    gDestination*   destinationList;            //!< the list of destinations that belong to this cycle ( for memory management )
    gDestination*   currentDestination;         //!< the destination this cycle aims for now
    gDestination*   lastDestination;            //!< the last destination that was passed

    eCoord          dirDrive;                   //!< the direction we are facing
    eCoord          lastDirDrive;               //!< the direction we were facing before the last turn
    REAL            acceleration;               //!< current acceleration

    REAL            lastTimestep_;              //!< the length of the last timestep
    REAL            verletSpeed_;               //!< object speed according to verlet (speed of half a frame ago)

    REAL            distance;                   //!< the distance traveled so far
    // REAL         wallContDistance;           //!< distance at which the walls will start to build up ( negative if the wall is already building )

    unsigned short  turns;                      //!< the number of turns taken so far
    unsigned short  braking;                    //!< flag indicating status of brakes ( on/off )

    int             windingNumber_;             //!< number that gets increased on every right turn and decreased on every left turn ( used by the AI )
    int             windingNumberWrapped_;      //!< winding number wrapped to be used as an index to the axes code

    eCoord			lastTurnPos_;	            //! the location of the last turn
    REAL            lastTurnTime_;              //!< the time of the last turn
    REAL            lastTimeAlive_;             //!< the time of the last timestep where we would not have been killed
    std::deque<int> pendingTurns;               //!< stores turns ordered by the user, but not yet executed

    REAL            brakingReservoir;           //!< the reservoir for braking. 1 means full, 0 is empty
    REAL            rubber;                     //!< the amount rubber used up by the cycle
    REAL            rubberMalus;                //!< additional rubber usage factor
    REAL            rubberSpeedFactor;          //!< the factor by which the speed is currently multiplied by rubber

    // room for accessors
public:
    inline REAL GetDistance( void ) const;  //!< Gets the distance traveled so far
    inline gCycleMovement const & GetDistance( REAL & distance ) const; //!< Gets the distance traveled so far
    inline REAL GetRubber( void ) const;    //!< Gets the amount rubber used up by the cycle
    inline gCycleMovement const & GetRubber( REAL & rubber ) const; //!< Gets the amount rubber used up by the cycle
    inline unsigned short GetTurns( void ) const;   //!< Gets the number of turns taken so far
    inline gCycleMovement const & GetTurns( unsigned short & turns ) const; //!< Gets the number of turns taken so far
    inline unsigned short GetBraking( void ) const; //!< Gets flag indicating status of brakes ( on/off )
    inline gCycleMovement const & GetBraking( unsigned short & braking ) const; //!< Gets flag indicating status of brakes ( on/off )
    inline REAL GetBrakingReservoir( void ) const;	//!< Gets the reservoir for braking. 1 means full, 0 is empty
    inline gCycleMovement const & GetBrakingReservoir( REAL & brakingReservoir ) const;	//!< Gets the reservoir for braking. 1 means full, 0 is empty
    inline REAL GetRubberMalus( void ) const;	//!< Gets additional rubber usage factor
    inline gCycleMovement const & GetRubberMalus( REAL & rubberMalus ) const;	//!< Gets additional rubber usage factor
    static bool RubberMalusActive( void ) ; //!< Returns whether rubber malus code is active
    inline eCoord const & GetLastTurnPos( void ) const;	//!< Gets the location of the last turn
    inline gCycleMovement const & GetLastTurnPos( eCoord & lastTurnPos ) const;	//!< Gets the location of the last turn
    inline REAL const & GetLastTurnTime( void ) const;	//!< Gets the time of the last turn
    inline gCycleMovement const & GetLastTurnTime( REAL & lastTurnTime ) const;	//!< Gets the time of the last turn
protected:
    inline gCycleMovement & SetLastTurnPos( eCoord const & lastTurnPos );	//!< Sets the location of the last turn
    inline gCycleMovement & SetLastTurnTime( REAL const & lastTurnTime );	//!< Sets the time of the last turn
private:
    inline gCycleMovement & SetDistance( REAL distance );   //!< Sets the distance traveled so far
    inline gCycleMovement & SetRubber( REAL rubber );   //!< Sets the amount rubber used up by the cycle
    inline gCycleMovement & SetTurns( unsigned short turns );   //!< Sets the number of turns taken so far
    inline gCycleMovement & SetBraking( unsigned short braking );   //!< Sets flag indicating status of brakes ( on/off )
    inline gCycleMovement & SetBrakingReservoir( REAL brakingReservoir );	//!< Sets the reservoir for braking. 1 means full, 0 is empty
    inline gCycleMovement & SetRubberMalus( REAL rubberMalus );	//!< Sets additional rubber usage factor
};

//! Determines the maximum space ahead of a cycle
float MaxSpaceAhead( const gCycleMovement* cycle, float ts, float lookAhead, float maxReport );

//! Exception to throw when cycle dies in a simulation frame
class gCycleDeath: public eDeath
{
public:
    gCycleDeath( eCoord const & pos )
            : pos_(pos)
    {}

    eCoord pos_;
};

// this class describes a point on the map the cycle on another
// computer of the game IS at. The copies of the cycle on the
// other computers try to reach this position by making the right
// turns.
class gDestination{
    friend class gCycleMovement;
    friend class gCycle;				// todo: remove me
    friend class gCycleExtrapolator;	// todo: remove me

    eCoord position;			// position of turn/brake command
    eCoord direction;			// driving direction after the command
    REAL  gameTime;				// game time of the command
    REAL  distance;				// distance travelled so far
    REAL  speed;				// speed at the time of the command
    bool  braking;				// flag telling whether the brake was active
    bool  chatting;				// flag indicating chat status

    unsigned short turns;	// the number of turns taken by the cycle so far
    bool hasBeenUsed;			// flag indicating whether the sync code has already used this command
    unsigned short messageID;	// ID of the message this command came from
    bool missable;		// flag indicating that this destination is not to be treated as the one after a missed destination

    gDestination *next; // so they can form a list
    gDestination **list; // the list we are in
public:
    // take pos,dir and time from a cycle
    explicit gDestination(const gCycleMovement &takeitfrom);
    explicit gDestination(const gCycle &takeitfrom);

    // or from a message
    explicit gDestination(nMessage &m);

    // take pos,dir and time from a cycle
    void CopyFrom(const gCycleMovement &other);
    void CopyFrom(const gCycle &other);

    //! compare two destinations
    int CompareWith( const gDestination& other ) const;

    // write all the data into a nMessage
    void WriteCreate(nMessage &m);

    // insert yourself into a list ordered by distance
    void InsertIntoList(gDestination **list);

    // find the latest entry in a list before the given distance
    static gDestination *RightBefore(gDestination *list, REAL dist);

    // find the earliest entry in a list after the given distance
    static gDestination *RightAfter(gDestination *list, REAL dist);

    // remove yourself again
    void RemoveFromList();

    bool Chatting(){ return chatting; }

    ~gDestination(){RemoveFromList();}

    gDestination & SetGameTime( REAL gameTime );	//!< Sets game time of the command
    REAL GetGameTime( void ) const;	//!< Gets game time of the command
    gDestination const & GetGameTime( REAL & gameTime ) const;	//!< Gets game time of the command
};

// *******************************************************************************************
// *
// *    IsDestinationUsed
// *
// *******************************************************************************************
//!
//!     @param  dest    the destination to test
//!     @return         true if the destination is still in active use
//!
// *******************************************************************************************

inline bool gCycleMovement::IsDestinationUsed( const gDestination * dest ) const
{
    // delegate to virtual function
    return DoIsDestinationUsed( dest );
}

// *******************************************************************************************
// *
// *	DropTempWall
// *
// *******************************************************************************************
//!
//!		@param	wall	   the wall the other cycle is grinding
//!
// *******************************************************************************************

inline void gCycleMovement::DropTempWall( gPlayerWall * wall )
{
    this->OnDropTempWall( wall );
}

// *******************************************************************************************
// *
// *    GetDistance
// *
// *******************************************************************************************
//!
//!     @return     the distance traveled so far
//!
// *******************************************************************************************

inline REAL gCycleMovement::GetDistance( void ) const
{
    return this->distance;
}

// *******************************************************************************************
// *
// *    GetDistance
// *
// *******************************************************************************************
//!
//!     @param  distance    the distance traveled so far to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetDistance( REAL & distance ) const
{
    distance = this->distance;
    return *this;
}

// *******************************************************************************************
// *
// *    SetDistance
// *
// *******************************************************************************************
//!
//!     @param  distance    the distance traveled so far to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetDistance( REAL distance )
{
    this->distance = distance;
    return *this;
}

// *******************************************************************************************
// *
// *    GetRubber
// *
// *******************************************************************************************
//!
//!     @return     the amount rubber used up by the cycle
//!
// *******************************************************************************************

inline REAL gCycleMovement::GetRubber( void ) const
{
    return this->rubber;
}

// *******************************************************************************************
// *
// *    GetRubber
// *
// *******************************************************************************************
//!
//!     @param  rubber  the amount rubber used up by the cycle to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetRubber( REAL & rubber ) const
{
    rubber = this->rubber;
    return *this;
}

// *******************************************************************************************
// *
// *    SetRubber
// *
// *******************************************************************************************
//!
//!     @param  rubber  the amount rubber used up by the cycle to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetRubber( REAL rubber )
{
    this->rubber = rubber;
    return *this;
}

// *******************************************************************************************
// *
// *    GetTurns
// *
// *******************************************************************************************
//!
//!     @return     the number of turns taken so far
//!
// *******************************************************************************************

inline unsigned short gCycleMovement::GetTurns( void ) const
{
    return this->turns;
}

// *******************************************************************************************
// *
// *    GetTurns
// *
// *******************************************************************************************
//!
//!     @param  turns   the number of turns taken so far to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetTurns( unsigned short & turns ) const
{
    turns = this->turns;
    return *this;
}

// *******************************************************************************************
// *
// *    SetTurns
// *
// *******************************************************************************************
//!
//!     @param  turns   the number of turns taken so far to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetTurns( unsigned short turns )
{
    this->turns = turns;
    return *this;
}

// *******************************************************************************************
// *
// *    GetBraking
// *
// *******************************************************************************************
//!
//!     @return     flag indicating status of brakes ( on/off )
//!
// *******************************************************************************************

inline unsigned short gCycleMovement::GetBraking( void ) const
{
    return this->braking;
}

// *******************************************************************************************
// *
// *    GetBraking
// *
// *******************************************************************************************
//!
//!     @param  braking flag indicating status of brakes ( on/off ) to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetBraking( unsigned short & braking ) const
{
    braking = this->braking;
    return *this;
}

// *******************************************************************************************
// *
// *    SetBraking
// *
// *******************************************************************************************
//!
//!     @param  braking flag indicating status of brakes ( on/off ) to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetBraking( unsigned short braking )
{
    this->braking = braking;
    return *this;
}

// *******************************************************************************************
// *
// *	GetBrakingReservoir
// *
// *******************************************************************************************
//!
//!		@return		the reservoir for braking. 1 means full, 0 is empty
//!
// *******************************************************************************************

inline REAL gCycleMovement::GetBrakingReservoir( void ) const
{
    return this->brakingReservoir;
}

// *******************************************************************************************
// *
// *	GetBrakingReservoir
// *
// *******************************************************************************************
//!
//!		@param	brakingReservoir	the reservoir for braking. 1 means full, 0 is empty to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetBrakingReservoir( REAL & brakingReservoir ) const
{
    brakingReservoir = this->brakingReservoir;
    return *this;
}

// *******************************************************************************************
// *
// *	SetBrakingReservoir
// *
// *******************************************************************************************
//!
//!		@param	brakingReservoir	the reservoir for braking. 1 means full, 0 is empty to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetBrakingReservoir( REAL brakingReservoir )
{
    this->brakingReservoir = brakingReservoir;
    return *this;
}

// *******************************************************************************************
// *
// *	GetDistanceSinceLastTurn
// *
// *******************************************************************************************
//!
//!		@return		the distance driven since the last turn
//!
// *******************************************************************************************

inline REAL gCycleMovement::GetDistanceSinceLastTurn( void ) const
{
    return this->DoGetDistanceSinceLastTurn();
}

// *******************************************************************************************
// *
// *	getRubberMalus
// *
// *******************************************************************************************
//!
//!		@return		additional rubber usage factor
//!
// *******************************************************************************************

inline REAL gCycleMovement::GetRubberMalus( void ) const
{
    return this->rubberMalus;
}

// *******************************************************************************************
// *
// *	getRubberMalus
// *
// *******************************************************************************************
//!
//!		@param	rubberMalus	additional rubber usage factor to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement const & gCycleMovement::GetRubberMalus( REAL & rubberMalus ) const
{
    rubberMalus = this->rubberMalus;
    return *this;
}

// *******************************************************************************************
// *
// *	setRubberMalus
// *
// *******************************************************************************************
//!
//!		@param	rubberMalus	additional rubber usage factor to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

inline gCycleMovement & gCycleMovement::SetRubberMalus( REAL rubberMalus )
{
    this->rubberMalus = rubberMalus;
    return *this;
}

// *******************************************************************************************
// *
// *	GetLastTurnPos
// *
// *******************************************************************************************
//!
//!		@return		the location of the last turn
//!
// *******************************************************************************************

eCoord const & gCycleMovement::GetLastTurnPos( void ) const
{
    return this->lastTurnPos_;
}

// *******************************************************************************************
// *
// *	GetLastTurnPos
// *
// *******************************************************************************************
//!
//!		@param	lastTurnPos	the location of the last turn to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gCycleMovement const & gCycleMovement::GetLastTurnPos( eCoord & lastTurnPos ) const
{
    lastTurnPos = this->lastTurnPos_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetLastTurnPos
// *
// *******************************************************************************************
//!
//!		@param	lastTurnPos	the location of the last turn to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gCycleMovement & gCycleMovement::SetLastTurnPos( eCoord const & lastTurnPos )
{
    this->lastTurnPos_ = lastTurnPos;
    return *this;
}

// *******************************************************************************************
// *
// *	GetLastTurnTime
// *
// *******************************************************************************************
//!
//!		@return		the time of the last turn
//!
// *******************************************************************************************

REAL const & gCycleMovement::GetLastTurnTime( void ) const
{
    return this->lastTurnTime_;
}

// *******************************************************************************************
// *
// *	GetLastTurnTime
// *
// *******************************************************************************************
//!
//!		@param	lastTurnTime	the time of the last turn to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gCycleMovement const & gCycleMovement::GetLastTurnTime( REAL & lastTurnTime ) const
{
    lastTurnTime = this->lastTurnTime_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetLastTurnTime
// *
// *******************************************************************************************
//!
//!		@param	lastTurnTime	the time of the last turn to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gCycleMovement & gCycleMovement::SetLastTurnTime( REAL const & lastTurnTime )
{
    this->lastTurnTime_ = lastTurnTime;
    return *this;
}

#endif // ARMAGETRONAD_SRC_TRON_GCYCLEMOVEMENT_H_INCLUDED
