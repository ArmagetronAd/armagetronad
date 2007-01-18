%{
#include "gCycleMovement.h"
%}

class gCycleMovement : public eNetGameObject
{
public:
    // accessors
    static float            RubberSpeed             ()                                              ;   //!< returns the rubber speed (decay rate of the distance to the wall in front)
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
    virtual eCoord          SpawnDirection         ()                                    const     ;   //!< returns the driving direction when the cycle was last spawned

    bool                    CanMakeTurn             (int direction                      ) const     ;   //!< returns whether a turn is currently possible
    bool                    CanMakeTurn             ( REAL time, int direction          ) const     ;   //!< returns whether a turn is possible at the given time
    inline  REAL            GetDistanceSinceLastTurn(                                   ) const     ;   //!< returns the distance since the last turn
    REAL                    GetTurnDelay            (                                   ) const     ;   //!< returns the time between turns in different directions
    REAL                    GetTurnDelayDb          (                                   ) const     ;   //!< returns the time between turns in the same direcion
    REAL                    GetNextTurn             (int direction                                   ) const     ;   //!< returns the time of the next turn

    // destination handling
    void                    AddDestination          ()                                              ;   //!< adds current position as destination
    void                    AdvanceDestination      ()                                              ;   //!< proceeds to the next destination
    void                    AddDestination          ( gDestination *        dest        )           ;   //!< adds given destination
    gDestination*           GetCurrentDestination   ()                                    const     ;   //!< returns the current destination
    void                    NotifyNewDestination    ( gDestination *        dest        )           ;   //!< notifies cycle of the insertion of a new destination ( don't call manually )
    bool                    IsDestinationUsed       ( const gDestination *  dest        ) const     ;   //!< returns whether the given destination is in active use

    inline void            DropTempWall             ( gPlayerWall *         wall
            ,                                         eCoord const &        pos
            ,                                         eCoord const &        dir         )           ;   //!< called when another cycle grinds a wall; this cycle should then drop its current wall if the grinding is too close.
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

    virtual REAL            NextInterestingTime     () const                                        ;   //!< the next time something interesting is going to happen with this object

    // existence management
    virtual void            AddRef                  ()                                              ;   //!< increase reference count

    gCycleMovement    	                            ( eGrid *               grid
            ,                                         const eCoord &        pos
            ,                                         const eCoord &        dir
            ,                                         ePlayerNetID *        p=NULL
                    ,                                 bool                  autodelete=1 )          ;   //!< local constructor
    gCycleMovement                                  ( nMessage &            message      )          ;   //!< remote constructor
    virtual ~gCycleMovement                         ()                                              ;   //!< destructor
    virtual void RemoveFromGame(); // call this instead of the destructor
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
    virtual void            AccelerationDiscontinuity ()                                            ;   //!< call when you know the acceleration makes a sharp jump now
    virtual void            CalculateAcceleration   (                                   )           ;   //!< calculate acceleration to apply later
    virtual void            ApplyAcceleration       ( REAL                  dt          )           ;   //!< apply acceleration calculated earlier

    // destination handling
    REAL                    DistanceToDestination   ( gDestination &        dest        ) const     ;   //!< calculates the distance to the given destination
    virtual void            OnNotifyNewDestination  ( gDestination *        dest        )           ;   //!< notifies cycle of the insertion of a new destination
    virtual void            OnDropTempWall          ( gPlayerWall *         wall
            ,                                         eCoord const &        pos
            ,                                         eCoord const &        dir         )           ;   //!< called when another cycle grinds a wall; this cycle should then drop its current wall if the grinding is too close.
    virtual bool            DoIsDestinationUsed     ( const gDestination *  dest        ) const     ;   //!< returns whether the given destination is in active use
    static  gDestination*   GetDestinationBefore    ( const SyncData &      sync
            ,                                         gDestination*         first       ) 		    ;   //!< determine the destination from before the sync message

    virtual bool            DoTurn                  ( int                   dir         )           ;   //!< turns the cycle in the given direction
    virtual REAL            DoGetDistanceSinceLastTurn  (                               ) const     ;   //!< returns the distance since the last turn

    virtual void            RightBeforeDeath        ( int                   numTries    )           ;   //!< called when the cycle is very close to a wall and about to crash
    virtual void            Die                     ( REAL time                         )           ;  //!< dies at the specified time

    virtual bool            TimestepCore            ( REAL                  currentTime
            ,                                         bool                  calculateAcceleration = true )           ;   //!< core physics simulation routine
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
    gEnemyInfluence				enemyInfluence; //!< keeps track of enemies that influenced this cycle

    gDestination*   destinationList;            //!< the list of destinations that belong to this cycle ( for memory management )
    gDestination*   currentDestination;         //!< the destination this cycle aims for now
    gDestination*   lastDestination;            //!< the last destination that was passed

    eCoord          dirDrive;                   //!< the direction we are facing
    eCoord          dirSpawn;                   //!< the direction we were facing on the last spawn
    eCoord          lastDirDrive;               //!< the direction we were facing before the last turn
    REAL            acceleration;               //!< current acceleration

    REAL            lastTimestep_;              //!< the length of the last timestep
    REAL            verletSpeed_;               //!< object speed according to verlet (speed of half a frame ago)

    REAL            distance;                   //!< the distance traveled so far
    // REAL         wallContDistance;           //!< distance at which the walls will start to build up ( negative if the wall is already building )

    mutable bool    refreshSpaceAhead_;         //!< flag to set when maximum space in front of cycle should be recalculated
    REAL            maxSpaceMaxCast_;           //!< the maximum raycast length to determine the above value
    mutable gMaxSpaceAheadHitInfo * maxSpaceHit_; //!< detailed information about the wall in front

    unsigned short  turns;                      //!< the number of turns taken so far
    unsigned short  braking;                    //!< flag indicating status of brakes ( on/off )

    int             windingNumber_;             //!< number that gets increased on every right turn and decreased on every left turn ( used by the AI )
    int             windingNumberWrapped_;      //!< winding number wrapped to be used as an index to the axes code

    eCoord			lastTurnPos_;	            //! the location of the last turn
    REAL            lastTurnTimeRight_;         //!< the time of the last turn right
    REAL            lastTurnTimeLeft_;          //!< the time of the last turn left
    REAL            lastTimeAlive_;             //!< the time of the last timestep where we would not have been killed
    std::deque<int> pendingTurns;               //!< stores turns ordered by the user, but not yet executed

    REAL            brakingReservoir;           //!< the reservoir for braking. 1 means full, 0 is empty
    REAL            rubber;                     //!< the amount rubber used up by the cycle
    REAL            rubberMalus;                //!< additional rubber usage factor
    REAL            rubberSpeedFactor;          //!< the factor by which the speed is currently multiplied by rubber

    REAL            brakeUsage;                 //!< current brake usage
    REAL            rubberUsage;                //!< current rubber usage (not from hitting a wall, but from tunneling. Without taking efficiency into account.)

    // room for accessors
public:
    REAL GetMaxSpaceAhead( REAL maxReport ) const; //< Returns the current maximal space ahead

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
    REAL GetAcceleration(void) const  { return acceleration; };  //!< Gets the cycle's acceleration
protected:
    inline gCycleMovement & SetLastTurnPos( eCoord const & lastTurnPos );	//!< Sets the location of the last turn
    inline gCycleMovement & SetLastTurnTime( REAL const & lastTurnTime );	//!< Sets the time of the last turn
private:
    inline gCycleMovement & SetDistance( REAL distance );   //!< Sets the distance traveled so far
public:  // HACK
    //To have the zone able to influence it
    inline gCycleMovement & SetRubber( REAL rubber );   //!< Sets the amount rubber used up by the cycle
private: // END OF HACK
    inline gCycleMovement & SetTurns( unsigned short turns );   //!< Sets the number of turns taken so far
    inline gCycleMovement & SetBraking( unsigned short braking );   //!< Sets flag indicating status of brakes ( on/off )
public:  // HACK
    //To have the zone able to influence it
    inline gCycleMovement & SetBrakingReservoir( REAL brakingReservoir );	//!< Sets the reservoir for braking. 1 means full, 0 is empty
private: // END OF HACK
    inline gCycleMovement & SetRubberMalus( REAL rubberMalus );	//!< Sets additional rubber usage factor
};
