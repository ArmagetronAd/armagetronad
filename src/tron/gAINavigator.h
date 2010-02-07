/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_AI_NAVIGATOR_H
#define ArmageTron_AI_NAVIGATOR_H

#include "gSensor.h"
#include "eCoord.h"

class gCycle;

//! AI helper class that knows the basics of staying alive.
class gAINavigator
{
    gAINavigator();
public:
    //! settings used by the idler bot
    struct Settings
    {
        REAL newWallBlindness; //!< number of seconds new walls are invisible to the idler
        REAL range; //!< seconds to plan ahead
        
        Settings();
    };

    Settings settings_; // settings to use

    //! turn wish passed from outside
    struct Wish
    {
        int turn;             //!< direction to turn into (values >1 turn multiple times)

        REAL maxDisadvantage; //!< maximal disadvantage of the wish direction over others to have it still accepted
        REAL minDistance;     //!< minimal 'distance' value of the wish direction to have it accepted

        Wish( gAINavigator const & idler );
    private:
        Wish();
    };

    // sensor with additional data
    class Sensor: public gSensor
    {
    public:
        Sensor(gAINavigator & ai,const eCoord &start,const eCoord &d);

        virtual void PassEdge(const eWall *ww,REAL time,REAL a,int r);

        // check how far the hit wall extends straight into the given direction
        REAL HitWallExtends( eCoord const & dir, eCoord const & origin );

        gAINavigator & ai_;          // AI using this sensor
        gCycle * hitOwner_;     // the owner of the hit wall
        REAL     hitTime_;      // the time the hit wall was built at
        REAL     hitDistance_;  // the distance of the wall to the cycle that built it
        short    lrSuggestion_; // sensor's oppinon on whether moving to the left or right of the hit wall is recommended (-1 for left, +1 for right)
        int      windingNumber_; // the number of turns (with sign) the cycle has taken

    private:
        bool DoExtraDetectionStuff();
    };

    //! ways to relay controls to a cycle
    class CycleController
    {
    public:
        virtual void Turn( gCycle & cycle , int dir    ) = 0; //!< turns a cycle. dir < 0 turns left.
        virtual void Brake( gCycle & cycle, bool brake ) = 0; //!< brakes a cycle
        virtual ~CycleController();
    };

    //! use direct control
    class CycleControllerBasic: public CycleController
    {
    public:
        virtual void Turn( gCycle & cycle , int dir    ); //!< turns a cycle.
        virtual void Brake( gCycle & cycle, bool brake ); //!< brakes a cycle
        virtual ~CycleControllerBasic();
    };

    //! use actions
    class CycleControllerAction: public CycleController
    {
    public:
        virtual void Turn( gCycle & cycle , int dir    ); //!< turns a cycle.
        virtual void Brake( gCycle & cycle, bool brake ); //!< brakes a cycle
        virtual ~CycleControllerAction();
    };

    //! describes walls
    struct WallHug
    {
        gCycle const * owner;  //! the cycle the walls we like belong to
        REAL lastTimeSeen;     //! the last time we saw such a wall
        REAL hitDistance;      //! driving distance of the wall that was hit
        REAL distance;         //! distance to the hit wall
        int  lr;               //! direction the wall is running to as seen from us

        WallHug();

        // fill data from sensor
        void FillFrom( Sensor const & sensor );
    };

    class PathGroup;

    //! a possible path that can be taken
    class Path
    {
        friend class gAINavigator;
        friend class PathGroup;
    public:
        REAL   distance;           //!< expected distance possible to travel this path without getting killed
        REAL   immediateDistance;  //!< distance to next problem
        REAL   width;              //!< width of the narrowest bit of the path
        eCoord shortTermDirection; //!< direction to travel in in the short run
        eCoord longTermDirection;  //!< direction to travel in in the long run
        int    followedSince;      //!< number of turns this path is already being followed

        WallHug left, right;       //!< walls to the left and right of the path
    private:
        // fill in relevant data from sensors
        void Fill( gAINavigator const & navigator, Sensor const & left, Sensor const & right, eCoord const & shortDir, eCoord const & longDir, int turn );
        REAL Take( CycleController & controller, gCycle & cycle, REAL maxStep ); //!< take that path. Return value: time to next check
        ~Path();
        Path();

        int  turn;    //!< direction to turn to next
        REAL driveOn; //!< how long to drive straight before doing something (in distance)
    };

    class PathGroup
    {
        friend class gAINavigator;
    public:
        //! types of paths
        enum PathID
        {
            PATH_UTURN_LEFT = 0,   //!< turn around completely
            PATH_TURN_LEFT,        //!< turn left
            PATH_ZIGZAG_LEFT,      //!< turn left, then right, or wait, then turn left, looking for a hole
            PATH_STRAIGHT,         //!< go straight
            PATH_ZIGZAG_RIGHT,
            PATH_TURN_RIGHT,
            PATH_UTURN_RIGHT,
            PATH_COUNT
        };

        int GetPathCount() const;                   //!< returns the current number of paths
        Path const & GetPath( int id ) const;       //!< returns a path
        REAL TakePath( CycleController & controller, gCycle & cycle, int id, REAL maxStep = 1E+30 );    //!< takes a path
        Path const & GetLastPath() const;           //!< the last path taken, with old info

        PathGroup();
        ~PathGroup();
    private:
        Path & AccessPath( int id );          //!< returns a path

        Path last;                        //!< last path
        Path paths[ PATH_COUNT ];         //!< fixed path list. Maybe add dynamic paths later.
    };

    //! evaluation of a path
    struct PathEvaluation
    {
        bool veto;  //!< was this path vetoed?
        REAL score; //!< score of the path. 0: pointless suicide, 100: pretty good.
        REAL nextThought; //!< seconds to next thought

        PathEvaluation();
    };

    //! class evaluating pathes
    class PathEvaluator
    {
    public:
        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const = 0;
        virtual ~PathEvaluator();

        //! turns a value between 0 and infinity, where 1 would be an expected value, into a value
        //! between 0 and 100
        static REAL Adjust( REAL input )
        {
            return 100*(1-1/(1+input));
        }
    };

    //! simple evaluator: vetoes certain death moves, picks best space move
    class SuicideEvaluator: public PathEvaluator
    {
    public:
        SuicideEvaluator( gCycle const & cycle, REAL timeFrame );
        explicit SuicideEvaluator( gCycle const & cycle );

        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        ~SuicideEvaluator();
        
        static void SetEmergency( bool emergency );
    private:
        gCycle const & cycle_;
        REAL   timeFrame_;
        static bool emergency_;
    };

    //! simple evaluator: vetoes moves that trap self
    class TrapEvaluator: public PathEvaluator
    {
    public:
        TrapEvaluator( gCycle const & cycle, REAL space );
        explicit TrapEvaluator( gCycle const & cycle );

        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        ~TrapEvaluator();
        
        static void SetEmergency( bool emergency );
    private:
        gCycle const & cycle_;
        REAL   space_;
    };

    //! simple evaluator: random noise
    class RandomEvaluator: public PathEvaluator
    {
    public:
        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        RandomEvaluator();
        ~RandomEvaluator();
    };

    //! cowardly evaluator: try to move backwards on enemy walls
    class CowardEvaluator: public PathEvaluator
    {
    public:
        explicit CowardEvaluator( gCycle const & cycle );
        ~CowardEvaluator();

        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
    private:
        gCycle const & cycle_;
    };

    //! tunnel evaluator: avoids tunneling between walls of different cycles
    class TunnelEvaluator: public PathEvaluator
    {
    public:
        explicit TunnelEvaluator( gCycle const & cycle );
        ~TunnelEvaluator();

        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
    private:
        gCycle const & cycle_;
    };

    //! simple evaluator: measures available space compared to a passed-in value
    class SpaceEvaluator: public PathEvaluator
    {
    public:
        explicit SpaceEvaluator( gCycle const & cycle );
        explicit SpaceEvaluator( REAL referenceDistance );

        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        ~SpaceEvaluator();
    private:
        REAL referenceDistance_;
    };

    //! simple evaluator: likes to follow through with the plan
    class PlanEvaluator: public PathEvaluator
    {
    public:
        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        ~PlanEvaluator();
    };

    //! simple evaluator: does not like it if rubber gets burned
    class RubberEvaluator: public PathEvaluator
    {
    public:
        //! evaluate a path.
        virtual void Evaluate( Path const & path, PathEvaluation & evaluation ) const;
        explicit RubberEvaluator( gCycle const & cylce );
        RubberEvaluator( gCycle const & cycle, REAL maxTime );
        ~RubberEvaluator();
    private:
        void Init( gCycle const & cycle, REAL maxTime );
        REAL rubberLeft_; //!< amount of rubber left to burn with inevitable loss due to turn delay factored in
        REAL maxRubber_;  //!< maximal rubber possible to burn
    };

    //! evaluator for following a moving target
    class FollowEvaluator: public PathEvaluator
    {
    public:
        FollowEvaluator( gCycle const & cycle );
        ~FollowEvaluator();
        
        //! return data of SolveTurn
        struct SolveTurnData
        {
            REAL turnTime;  //!< seconds to wait before we turn
            REAL quality;   //!< quality of the turn
            eCoord turnDir; //!< direction to drive in
            
            SolveTurnData(): turnTime(0), quality(0){}
        };
        
        //! determine when we need to turn in order to catch the target.
        void SolveTurn( int direction, eCoord const & targetVelocity, eCoord const & targetPosition, SolveTurnData & data );
        
        //! set the target to follow
        void SetTarget( eCoord const & target, eCoord const & velocity );

        virtual void Evaluate( gAINavigator::Path const & path, gAINavigator::PathEvaluation & evaluation ) const;

        gCycle * GetBlocker() const { return blocker_; }
    protected:
        gCycle const & cycle_; //!< the owning cycle
        gCycle * blocker_;     //!< other cycle blocking the path to the target
        bool   blockedBySelf_; //!< flag indicating the path is blocked by the cycle itself
        eCoord toTarget_;      //!< direction to target, roughly normalized
        REAL   turnTime_;      //!< time to make the next turn
    };


    class EvaluationManager
    {
    public:
        EvaluationManager( PathGroup & paths );
        
        //! ways to combine new evaluation with previous results
        enum BlendMode
        {
            BLEND_ADD,
            BLEND_MULT,
            BLEND_MAX,
            BLEND_MIN
        };
        
        //! evaluate all paths using the evaluator
        void Evaluate( PathEvaluator const & evaluator, BlendMode mode = BLEND_ADD, REAL scale = 1.0, REAL offset = 0.0 );

        //! evaluate all paths using the evaluator, adding results
        void Evaluate( PathEvaluator const & evaluator, REAL scale )
        {
            Evaluate( evaluator, BLEND_ADD, scale );
        }

        //! reset scores, but don't forget veto
        void Reset();

        //! execute
        REAL Finish( CycleController & controller, gCycle & cycle, REAL maxStep = 1E+30 );
    private:
        PathGroup & paths_; //!< the path group

        int  bestPath_;  //!< best path

        //! list of evaluations fitting to those in the 
        PathEvaluation evaluations_[ PathGroup::PATH_COUNT ];
    };

    PathGroup & GetPaths(); //!< returns the paths the navigator knows about
    void UpdatePaths();     //!< updates the paths to the new cycle position

    gAINavigator( gCycle * owner );

    // returns the controlled cycle
    gCycle * Owner() const
    {
        return owner_;
    }

    // determines the distance between two sensors; the size should give the likelyhood
    // to survive if you pass through a gap between the two selected walls
    REAL Distance( Sensor const & a, Sensor const & b ) const;

    bool CanMakeTurn( uActionPlayer * action );

    //! does the main thinking at the current time, knowing the next thought can't be sooner than minstep
    REAL Activate( REAL currentTime, REAL minstep, REAL penalty = 0, Wish * wish = 0 );

private:
    short lastTurn_;         //!< the last turn the chat AI made
    REAL nextTurn_;          //!< the next turn if one is planned
    bool turnedRecently_;    //!< whether the cycle was turned or almost turned recently
    gCycle * owner_;         //!< owner of chatbot
    PathGroup paths_;        //!< possible future paths
};

#endif
