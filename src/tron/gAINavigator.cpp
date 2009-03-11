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

#include "gAINavigator.h"

#include "tRandom.h"
#include "tSysTime.h"

#include "eGrid.h"

#include "gCycle.h"
#include "gWall.h"

#ifndef HUGE
#define HUGE 1e+38
#endif

gAINavigator::Settings::Settings()
: newWallBlindness(-.1)
, range( 1 )
{}

gAINavigator::Wish::Wish( gAINavigator const & idler )
: turn(0)
, maxDisadvantage( HUGE )
{
    gCycle & owner = *idler.Owner();
    minDistance = owner.GetTurnDelay() * owner.Speed();
}

// *************************
// * Sensors               *
// *************************

gAINavigator::Sensor::Sensor(gAINavigator & ai,const eCoord &start,const eCoord &d)
: gSensor(ai.Owner(),start,d)
, ai_( ai )
, hitOwner_( 0 )
, hitTime_ ( 0 )
, hitDistance_( ai.Owner()->MaxWallsLength() )
, lrSuggestion_( 0 )
, windingNumber_( 0 )
{
    if ( hitDistance_ <= 0 )
        hitDistance_ = ai.Owner()->GetDistance();
}

/*
// do detection and additional stuff
void detect( REAL range )
{
gSensor::detect( range );
}
*/

void gAINavigator::Sensor::PassEdge(const eWall *ww,REAL time,REAL a,int r)
{
    try{
        gSensor::PassEdge(ww,time,a,r);
    }
    catch( eSensorFinished & e )
    {
        if ( DoExtraDetectionStuff() )
            throw;
    }
}

extern REAL sg_cycleRubberWallShrink;

bool gAINavigator::Sensor::DoExtraDetectionStuff()
{
    // move towards the beginning of a wall
    lrSuggestion_ = -lr;

    switch ( type )
    {
    case gSENSOR_NONE:
    case gSENSOR_RIM:
        lrSuggestion_ = 0;
        return true;
    default:
        // unless it is an enemy, follow his wall instead (uncomment for a nasty cowardy campbot)
        // lrSuggestion *= -1;
    case gSENSOR_SELF:
    {
        // determine whether we're hitting the front or back half of his wall
        if ( !ehit )
            return true;
        eWall * wall = ehit->GetWall();
        if ( !wall )
            return true;
        gPlayerWall * playerWall = dynamic_cast< gPlayerWall * >( wall );
        if ( !playerWall )
            return true;
        hitOwner_ = playerWall->Cycle();
        if ( !hitOwner_ )
            return true;

        // gAINavigator & enemyChatBot = Get( hitOwner_ );

        REAL wallAlpha = playerWall->Edge()->Ratio( before_hit );
        // that's an unreliable source
        if ( wallAlpha < 0 )
            wallAlpha = 0;
        if ( wallAlpha > 1 )
            wallAlpha = 1;
        hitDistance_   = hitOwner_->GetDistance() - playerWall->Pos( wallAlpha );
        hitTime_       = playerWall->Time( wallAlpha );
        windingNumber_ = playerWall->WindingNumber();

        // don't see new walls
        if ( hitTime_ > hitOwner_->LastTime() - ai_.settings_.newWallBlindness && hitOwner_ != owned )
        {
            ehit = false;
            hit = 1.01;
            return false;
        }

        // don't see vanishing walls
        {
            // TODO: this is a bit wrong in the face of diagonals and quantizized driving directions.
            REAL distanceToHit = dir.Norm() * hit;
            REAL rubberDistance = 0;
            
            // assume we can waste half our rubber waiting for the wall to get lost
            {
                REAL rubberGranted, rubberEffectiveness;
                sg_RubberValues( ai_.owner_->Player(), ai_.owner_->Speed(), rubberGranted, rubberEffectiveness );
                rubberDistance = ( rubberGranted - ai_.owner_->GetRubber() ) * .5;
            }
            if( type == gSENSOR_SELF )
            {
                rubberDistance *= sg_cycleRubberWallShrink;
            }

            REAL timeToHit = distanceToHit/ai_.owner_->Speed();
            if( !playerWall->IsDangerous( wallAlpha, ai_.owner_->LastTime() + timeToHit ) )
            {
                // wall will be gone until we get there. ignore.
                ehit = false;
                hit = 1.01;
                return false;
            }
        }

        // REAL cycleDistance = hitOwner_->GetDistance();

        // REAL wallStart = 0;

        /*
          if ( gCycle::WallsLength() > 0 )
          {
          wallStart = cyclePos - playerWall->Cycle()->ThisWallsLength();
          if ( wallStart < 0 )
          wallStart = 0;
          }
        */
    }
    }

    return true;
}

// check how far the hit wall extends straight into the given direction
REAL gAINavigator::Sensor::HitWallExtends( eCoord const & dir, eCoord const & origin )
{
    if ( !ehit || !ehit->Other() )
    {
        return 0;
    }

    REAL ret = -HUGE;
    eCoord ends[2] = { *ehit->Point(), *ehit->Other()->Point() };
    for ( int i = 1; i>=0; --i )
    {
        REAL newRet = eCoord::F( dir, ends[i]-origin );
        if ( newRet > ret )
            ret = newRet;
    }

    return ret;
}

// *************************
// * Controllers           *
// *************************

//!@param cycle  the cycle to execute the action
//!@param dir    direction to turn into
void gAINavigator::CycleController::Turn( gCycle & cycle , int dir    )
{
}

//!@param cycle  the cycle to execute the action
//!@param brake  whether to brake or not
void gAINavigator::CycleController::Brake( gCycle & cycle, bool brake )
{
}

gAINavigator::CycleController::~CycleController(){}

void gAINavigator::CycleControllerBasic::Turn( gCycle & cycle , int dir    )
{
    cycle.Turn( dir );
}

void gAINavigator::CycleControllerBasic::Brake( gCycle & cycle, bool brake )
{
    cycle.Act( &gCycle::s_brake, brake ? 1 : -1 );
}

gAINavigator::CycleControllerBasic::~CycleControllerBasic(){}

void gAINavigator::CycleControllerAction::Turn( gCycle & cycle , int dir    )
{
    cycle.Act( dir > 0 ? &gCycle::se_turnRight : &gCycle::se_turnLeft, 1 );
}

void gAINavigator::CycleControllerAction::Brake( gCycle & cycle, bool brake )
{
    cycle.Act( &gCycle::s_brake, brake ? 1 : -1 );
}

gAINavigator::CycleControllerAction::~CycleControllerAction(){}

// *************************
// * Path                  *
// *************************

void gAINavigator::Path::Fill( gAINavigator const & navigator, Sensor const & left, Sensor const & right, eCoord const & shortDir, eCoord const & longDir, int turn )
{
    // check orientation
    tASSERT( left.Direction() * right.Direction() >= 0 );

    this->distance = navigator.Distance( left, right );
    this->shortTermDirection = shortDir;
    this->longTermDirection = longDir;

    this->left.FillFrom( left );
    this->right.FillFrom( right );

    this->turn = turn;
    this->driveOn = 0;
}

//!@param controller the controller to use for the execution
//!@param cycle      the cycle to execute the action
//!@param maxStep    maximal timestep the caller suggests
REAL gAINavigator::Path::Take( CycleController & controller, gCycle & cycle, REAL maxStep )
{
    if( driveOn > 0 )
    {
        REAL driveOnTime = driveOn/cycle.Speed();
        if( maxStep > driveOnTime )
        {
            maxStep = driveOnTime;
        }
        driveOn = 0;
        return maxStep;
    }

    if( turn != 0 )
    {
        if( !cycle.CanMakeTurn( turn ) )
        {
            return driveOn = cycle.GetNextTurn( turn ) - cycle.LastTime();
        }

        controller.Turn( cycle, turn );
    }
    
    return maxStep;
}

gAINavigator::Path::~Path()
{
}

gAINavigator::Path::Path()
: distance(HUGE)
, immediateDistance(HUGE)
, shortTermDirection(0,0)
, longTermDirection(0,0)
, followedSince(0)
, turn(0)
, driveOn(0)
{
}

// *************************
// * PathGroup             *
// *************************


//!@return the number of paths accessible via GetPath
int gAINavigator::PathGroup::GetPathCount() const
{
    return PATH_COUNT;
}

//!@param id  the ID of the path, between 0 and GetPathCount()-1
//!@return    the path of given ID
gAINavigator::Path const & gAINavigator::PathGroup::GetPath( int id ) const
{
    tASSERT( id >= 0 && id < PATH_COUNT );
    return paths[ id ];
}

//!@param controller the controller to use for the execution
//!@param cycle      the cycle to execute the action
//!@param id         the ID of the path, between 0 and GetPathCount()-1
//!@param maxStep    maximal timestep the caller suggests
REAL gAINavigator::PathGroup::TakePath( CycleController & controller, gCycle & cycle, int id, REAL maxStep )
{
    // save plan
    last = GetPath( id );

    // execute plan
    REAL ret = last.Take( controller, cycle, maxStep );

    // clear all paths
    for( int i = GetPathCount()-1; i >= 0; --i )
    {
        Path & path = AccessPath( i );
        path.left = path.right = WallHug();
        path.distance = path.immediateDistance = HUGE;
        path.followedSince = 0;
        path.driveOn = 0;
    }

    // check whether execution was successful
    if( last.driveOn > 0 )
    {
        // turn was not executed. memorize that it was a good choilce
        paths[ id ].followedSince++;
        return ret;
    }

    // transfer last use stats
    switch( id )
    {
    case PATH_UTURN_LEFT:
        paths[ PATH_UTURN_LEFT ].followedSince = last.followedSince + 1;
        paths[ PATH_TURN_LEFT  ].followedSince = last.followedSince + 1;
        break;
    case PATH_UTURN_RIGHT:
        paths[ PATH_UTURN_RIGHT ].followedSince = last.followedSince + 1;
        paths[ PATH_TURN_RIGHT  ].followedSince = last.followedSince + 1;
        break;
    case PATH_ZIGZAG_LEFT:
        if ( last.driveOn )
        {
            paths[ PATH_TURN_LEFT  ].followedSince = last.followedSince + 1;
            paths[ PATH_ZIGZAG_LEFT].followedSince = last.followedSince + 1;
        }
        else
        {
            paths[ PATH_STRAIGHT   ].followedSince = last.followedSince + 1;
            paths[ PATH_TURN_RIGHT ].followedSince = last.followedSince + 1;
        }
        break;
    case PATH_ZIGZAG_RIGHT:
        if ( last.driveOn )
        {
            paths[ PATH_TURN_RIGHT  ].followedSince = last.followedSince + 1;
            paths[ PATH_ZIGZAG_RIGHT].followedSince = last.followedSince + 1;
        }
        else
        {
            paths[ PATH_STRAIGHT  ].followedSince = last.followedSince + 1;
            paths[ PATH_TURN_LEFT ].followedSince = last.followedSince + 1;
        }
        break;
    case PATH_TURN_LEFT:
    case PATH_STRAIGHT:
    case PATH_TURN_RIGHT:
        paths[ PATH_STRAIGHT ].followedSince = last.followedSince + 1;
        break;
    }

    return ret;
}

//!@return    the path taken last time
gAINavigator::Path const & gAINavigator::PathGroup::GetLastPath() const
{
    return last;
}

gAINavigator::PathGroup::PathGroup()
{
    // pretend we went straight
    paths[ PATH_STRAIGHT ].followedSince = 100;
}
gAINavigator::PathGroup::~PathGroup(){}

//!@param id  the ID of the path, between 0 and GetPathCount()-1
//!@return    the path of given ID
gAINavigator::Path & gAINavigator::PathGroup::AccessPath( int id )
{
    tASSERT( id >= 0 && id < PATH_COUNT );
    return paths[ id ];
}

// *************************
// * Evaluation            *
// *************************

gAINavigator::PathEvaluation::PathEvaluation() : veto( false ), score( 0 ), nextThought( HUGE ){}

//!@param path        the path to evaluate
//!@param evaluation  place to store the result
void gAINavigator::PathEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const{}
gAINavigator::PathEvaluator::~PathEvaluator(){}

//!@param path        the path to evaluate
//!@param evaluation  place to store the result
void gAINavigator::SuicideEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    REAL speed = cycle_.Speed();
    REAL referenceDistance = speed*timeFrame_;
    REAL distance = path.distance;
    if( path.immediateDistance < distance )
    {
        distance = path.immediateDistance;
    }

    // check if this is the forward path
    if( !emergency_ &&
        eCoord::F( path.shortTermDirection, cycle_.Direction() ) > .99 )
    {
        distance += cycle_.GetTurnDelay()*speed;
    }

    REAL timeToLive = distance/referenceDistance;
    evaluation.score = Adjust(timeToLive/100);
    if( timeToLive < 1 )
    {
        evaluation.veto = true;
    }
}

bool gAINavigator::SuicideEvaluator::emergency_ = false;

void gAINavigator::SuicideEvaluator::SetEmergency( bool emergency )
{
    emergency_ = emergency;
}

gAINavigator::SuicideEvaluator::SuicideEvaluator( gCycle const & cycle ): cycle_( cycle ), timeFrame_( cycle.GetTurnDelay() ){}
gAINavigator::SuicideEvaluator::SuicideEvaluator( gCycle const & cycle, REAL timeFrame ): cycle_( cycle ), timeFrame_( timeFrame ){}
gAINavigator::SuicideEvaluator::~SuicideEvaluator(){}

//!@param path        the path to evaluate
//!@param evaluation  place to store the result
void gAINavigator::TrapEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    if( space_ <= 0 )
    {
        return;
    }
    evaluation.score = Adjust( path.distance/space_ );
    REAL trapLength = cycle_.GetTurnDelay() * cycle_.Speed();
    if( path.distance < space_ && path.left.owner == path.right.owner && path.left.owner && 
        ( path.immediateDistance < trapLength || ( path.left.distance < trapLength && path.right.distance < trapLength ) || path.left.owner == &cycle_ ) )
    {
        evaluation.veto = true;
    }
}

gAINavigator::TrapEvaluator::TrapEvaluator( gCycle const & cycle )
: cycle_( cycle )
{
    space_ = .25 * cycle.ThisWallsLength();
}

gAINavigator::TrapEvaluator::TrapEvaluator( gCycle const & cycle, REAL space )
: cycle_( cycle )
, space_( space ){}
gAINavigator::TrapEvaluator::~TrapEvaluator(){}

void gAINavigator::RandomEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    static tReproducibleRandomizer randomizer;
    evaluation.score = randomizer.Get() * 100;
}

gAINavigator::RandomEvaluator::RandomEvaluator(){}
gAINavigator::RandomEvaluator::~RandomEvaluator(){}

gAINavigator::CowardEvaluator::CowardEvaluator( gCycle const & cycle ): cycle_( cycle ){}
gAINavigator::CowardEvaluator::~CowardEvaluator(){}

void gAINavigator::CowardEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    evaluation.score = 100;
    if( path.left.owner || path.right.owner )
    {
        // REAL turnDelay = cycle_.GetTurnDelay() * cycle_.Speed();
        // if( path.right.distance + path.left.distance < turnDelay * 4 )
        {
            if(  path.left.owner && path.left.owner->Alive() && path.left.owner->Team() != cycle_.Team() && path.left.lr == 1 )
            {
                evaluation.score = 0;
            }
            if( path.right.owner && path.right.owner->Alive() && path.right.owner->Team() != cycle_.Team() && path.right.lr == -1 )
            {
                evaluation.score = 0;
            }
        }
    }
}

gAINavigator::SpaceEvaluator::SpaceEvaluator( gCycle const & cycle )
: referenceDistance_( cycle.MaxWallsLength() )
{
    if( referenceDistance_ <= 0 )
    {
        referenceDistance_ = cycle.GetDistance() + cycle.Speed() * 5;
    }
}

gAINavigator::SpaceEvaluator::SpaceEvaluator( REAL referenceDistance )
: referenceDistance_( referenceDistance )
{}

void gAINavigator::SpaceEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    evaluation.score = Adjust(path.distance/referenceDistance_);
}

gAINavigator::SpaceEvaluator::~SpaceEvaluator(){}

//!@param path        the path to evaluate
//!@param evaluation  place to store the result
void gAINavigator::PlanEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    evaluation.score = Adjust( path.followedSince/3.0 );
}

gAINavigator::PlanEvaluator::~PlanEvaluator(){}

//!@param paths the path group to evaluate
gAINavigator::EvaluationManager::EvaluationManager( PathGroup & paths )
: paths_( paths )
, bestPath_( -1 )
{
    // evaluations_.reserve( paths.GetPathCount() );
    for( int i = 0; i < paths.GetPathCount(); ++i )
    {
        //evaluations_.push_back( PathEvaluation() );

        // store preliminary 'best' path
        // if( paths.GetPath( i ).followedSince )
        // {
        // bestPath_ = i;
        // }
    }
}
        
//! evaluate a path.
void gAINavigator::RubberEvaluator::Evaluate( Path const & path, PathEvaluation & evaluation ) const
{
    // rubber that is about to get burned
    REAL burn = rubberLeft_ - path.immediateDistance;
    if( burn < 0 )
    {
        burn = 0;
    }
    if( burn > maxRubber_ )
    {
        burn = maxRubber_;
    }
    evaluation.score = ( 1 - burn/maxRubber_ ) * 100;
}

gAINavigator::RubberEvaluator::RubberEvaluator( gCycle const & cycle )
{
    Init( cycle, cycle.GetTurnDelay() );
}

gAINavigator::RubberEvaluator::RubberEvaluator( gCycle const & cycle, REAL maxTime )
{
    Init( cycle, maxTime );
}

gAINavigator::RubberEvaluator::~RubberEvaluator()
{
}

void gAINavigator::RubberEvaluator::Init( gCycle const & cycle, REAL maxTime )
{
    // compensate for the addition of rubber in the stored sensor distances
    REAL rubberGranted, rubberEffectiveness;
    REAL speed = cycle.Speed();
    sg_RubberValues( cycle.Player(), speed, rubberGranted, rubberEffectiveness );
    REAL rubberLeft = ( rubberGranted - cycle.GetRubber() )*rubberEffectiveness;
    maxRubber_  = maxTime * speed;

    // account for inevitable loss
    rubberLeft_ = rubberLeft + maxRubber_;

    if( maxRubber_ > rubberLeft )
    {
        maxRubber_ = rubberLeft;
    }

    if( maxRubber_ < EPS )
    {
        maxRubber_ = EPS;
    }
}

// *************************
// * FollowEvaluator      *
// *************************

gAINavigator::FollowEvaluator::FollowEvaluator( gCycle const & cycle ): cycle_( cycle ), blocker_( 0 )
{
}

gAINavigator::FollowEvaluator::~FollowEvaluator()
{
}

//!@ param direction            direction to turn in
//!@ param targetVelocity       velocity of the target
//!@ param targetPosition       current position of the target relative to cycle
//!@ param data                 solution filled in
void gAINavigator::FollowEvaluator::SolveTurn( int direction, eCoord const & targetVelocity, eCoord const & targetPosition, SolveTurnData & data )
{
    eCoord velocityDifference = cycle_.Speed() * cycle_.Direction() - targetVelocity;

    // get the velocity after the turn
    int winding = cycle_.WindingNumber();
    cycle_.Grid()->Turn( winding, direction );
    data.turnDir = cycle_.Grid()->GetDirection( winding );
    eCoord turnVelocityDifference = data.turnDir * cycle_.Speed() - targetVelocity;
        
    // some little algebra to solve velocityDifference * turnTime + turnVelocityDifference * Quality == targetPosition
    REAL determinant = turnVelocityDifference * velocityDifference;
    if( fabs( determinant ) < EPS )
    {
        // division by zero error, problem is not well defined. approximate.
        data.turnTime = 0;

        REAL velocityDifferenceLen = velocityDifference.NormSquared();
        if( velocityDifferenceLen > EPS )
        {
            data.turnTime = eCoord::F( velocityDifference, targetPosition )/velocityDifferenceLen;
        }
    }
    else
    {
        // the better case
        data.turnTime = ( turnVelocityDifference * targetPosition )/determinant;
    }

    // but halt, clamp it, we can't rewind (we've gone too far)
    if ( data.turnTime < 0 )
    {
        data.turnTime = 0;
    }

    // fetch an alternative turn direction (the same as before with 4 axes)
    // so that we have a guaranteed nonnegative positive quality outcome among the two
    winding = cycle_.WindingNumber();
    cycle_.Grid()->Turn( winding, -direction );
    eCoord antiTurnDir = -cycle_.Grid()->GetDirection( winding );
    turnVelocityDifference = antiTurnDir * cycle_.Speed() - targetVelocity;

    // and handle the rest primitively via dot product
    data.quality = eCoord::F( turnVelocityDifference, targetPosition - velocityDifference * data.turnTime );
    REAL velocityDifferenceLen = turnVelocityDifference.NormSquared();
    if( velocityDifferenceLen > EPS )
    {
        data.quality /= velocityDifferenceLen;

        // turn right now if target is much to the side
        if( data.quality > 2 * data.turnTime )
        {
            data.turnTime = 0;
        }
    }
}

//! sensor picking up several walls between cycle and target
class gTargetSensor:public gSensor
{
public:
    int lastOwnLR; //!< the last LR value of a hit with an own wall
    tCHECKED_PTR_CONST(eHalfEdge) lastOwnEHit; //!< the edge hit there
    
    gTargetSensor(eGameObject const * o,const eCoord &start,const eCoord &d)
    :gSensor(o,start,d), lastOwnLR(0), lastOwnEHit(0) {}

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int i)
    {
        try
        {
            gSensor::PassEdge( w, time, a, i );
        }
        catch( eSensorFinished & e )
        {
            if( type == gSENSOR_SELF )
            {
                // copy the last own wall we see
                lastOwnLR = lr;
                lastOwnEHit = ehit;
                ehit = 0;
            }
            else
            {
                throw;
            }
        }
    }
};

//!@param minQuality minimal turn quality (measured in seconds)
void gAINavigator::FollowEvaluator::SetTarget( eCoord const & target, eCoord const & velocity )
{
    // determine direction to center
    toTarget_ = target - cycle_.Position();

    // check whether the path is blocked
    gTargetSensor sensor( &cycle_, cycle_.Position(), toTarget_ );
    sensor.detect( .99 );

    if( sensor.type != gSENSOR_NONE )
    {
        if( sensor.lastOwnEHit )
        {
            gPlayerWall const * ownWall = dynamic_cast< gPlayerWall const * >( sensor.lastOwnEHit->GetWall() );
            if ( ownWall )
            {
                blocker_ = ownWall->Cycle();
                tASSERT( blocker_ == &cycle_ );
                if( ownWall->Pos(.5) > blocker_->GetDistance() - blocker_->ThisWallsLength()*.9 )
                {
                    int lr = toTarget_ * ownWall->Vec() > 0 ? 1 : -1;

                    // if we block our own recent path, turn around
                    if( ownWall->Pos(.5) > blocker_->GetDistance() - blocker_->ThisWallsLength()*.75 )
                    {
                        lr *= -1;
                    }

                    int winding = cycle_.WindingNumber();
                    cycle_.Grid()->Turn( winding, lr );
                    toTarget_ = cycle_.Grid()->GetDirection( winding );
                    return;
                }
            }
        }
        else if ( sensor.ehit )
        {
            gPlayerWall const * w = dynamic_cast< gPlayerWall const * >( sensor.ehit->GetWall() );
            if ( w )
            {
                // just follow enemy wall in the inverse direction, but keep an eye on the target
                toTarget_.Normalize();
                eCoord follow = - w->Vec();
                follow.Normalize();
                toTarget_ += follow * .9;
                toTarget_.Normalize();
                toTarget_ *= .1;
                return;
            }
        }
    }

    // determine next possible left or right directions
    SolveTurnData left, right;
    SolveTurn( -1, velocity, toTarget_, left );
    SolveTurn( 1,  velocity, toTarget_, right );

    REAL quality = 0;

    // pick the best one
    if( left.quality > right.quality )
    {
        quality   = left.quality;
        toTarget_ = left.turnDir;
        turnTime_ = left.turnTime;
    }
    else
    {
        quality   = right.quality;
        toTarget_ = right.turnDir;
        turnTime_ = right.turnTime;
    }

    if ( turnTime_ > 0 )
    {
        toTarget_ = cycle_.Direction();
    }
    else
    {
        turnTime_ = HUGE;
    }
}

void gAINavigator::FollowEvaluator::Evaluate( gAINavigator::Path const & path, gAINavigator::PathEvaluation & evaluation ) const
{
    eCoord pathDir = path.shortTermDirection;
    REAL f = eCoord::F( toTarget_, pathDir );
    if( f > 0 && f*f > .99 * toTarget_.NormSquared() * pathDir.NormSquared() )
    {
        evaluation.nextThought = turnTime_;
    }

    evaluation.score = 50 * f + 50;
}

//!@param evaluator  evaluator doing the core work
//!@param scale      weight factor for that work
void gAINavigator::EvaluationManager::Evaluate( PathEvaluator const & evaluator, BlendMode mode, REAL scale, REAL offset )
{
    REAL bestScore = -HUGE;
    REAL bestDistance = HUGE;
        
    for( int i = paths_.GetPathCount()-1; i >= 0 ; --i )
    {
        PathEvaluation & store = evaluations_[i];
        if( store.veto )
        {
            continue;
        }

        PathEvaluation evaluation;
        Path const & path = paths_.GetPath(i);
        evaluator.Evaluate( path, evaluation );
        evaluation.score = evaluation.score * scale + offset;

        // update stored evaluation
        switch ( mode )
        {
        case BLEND_ADD:
            store.score += evaluation.score;
            break;
        case BLEND_MULT:
            store.score *= evaluation.score;
            break;
        case BLEND_MAX:
            if( evaluation.score > store.score )
            {
                store.score = evaluation.score;
            }
            break;
        case BLEND_MIN:
            if( evaluation.score < store.score )
            {
                store.score = evaluation.score;
            }
            break;
        }

        if ( evaluation.nextThought < store.nextThought )
        {
            store.nextThought = evaluation.nextThought;
        }

        if ( evaluation.veto )
        {
            store.veto = true;
        }
        else if ( bestPath_ < 0 || store.score > bestScore || ( store.score == bestScore && path.distance < bestDistance ) )
        {
            bestPath_ = i;
            bestScore = store.score;
            bestDistance = path.distance;
        }
    }

    if( bestPath_ < 0 )
    {
#ifdef DEBUG
        con << "PANIC!\n";
#endif

        // PANIC. No path ever was not vetoed. Oh well. Take the best anyway and be done.
        for( int i = paths_.GetPathCount()-1; i >= 0 ; --i )
        {
            PathEvaluation & store = evaluations_[i];
            Path const & path = paths_.GetPath(i);
            if ( bestPath_ < 0 || store.score > bestScore || ( store.score == bestScore && path.distance < bestDistance ) )
            {
                bestPath_ = i;
                bestScore = store.score;
                bestDistance = path.distance;
            }
        }
    }
}

//! reset scores, but don't forget veto
void gAINavigator::EvaluationManager::Reset()
{
    for( int i = paths_.GetPathCount()-1; i >= 0; --i )
    {
        evaluations_[i].score = 0;
    }
}

//!@param controller   controller issuing the commands
//!@param cycle        cycle getting controlled
//!@param maxStep      suggestion for next timestep
REAL gAINavigator::EvaluationManager::Finish( CycleController & controller, gCycle & cycle, REAL maxStep ){
    if( bestPath_ >= 0 )
    {
        REAL thisMaxStep = evaluations_[ bestPath_ ].nextThought;
        if( maxStep > thisMaxStep )
        {
            maxStep = thisMaxStep;
        }
        return paths_.TakePath( controller, cycle, bestPath_, maxStep );
    }
    else
    {
        return maxStep;
    }
}

gAINavigator::gAINavigator( gCycle * owner )
: lastTurn_( 0 )
, nextTurn_ ( 0 )
, turnedRecently_ ( 0 )
, owner_ ( owner )
{
}

gAINavigator::WallHug::WallHug()
: owner ( NULL ), lastTimeSeen ( 0 ), distance( HUGE ), lr( 0 )
{
}

void gAINavigator::WallHug::FillFrom( Sensor const & sensor )
{
    owner = sensor.hitOwner_;
    lr = sensor.lr;
    hitDistance = sensor.hitDistance_;
    distance = sensor.hit * sensor.Direction().Norm();
    if( owner )
    {
        lastTimeSeen = owner->LastTime();
    }
}

// determines the distance between two sensors; the size should give the likelyhood
// to survive if you pass through a gap between the two selected walls
REAL gAINavigator::Distance( Sensor const & a, Sensor const & b ) const
{
    // make sure a is left from b
    if ( a.Direction() * b.Direction() < 0 )
        return Distance( b, a );

    bool self = a.type == gSENSOR_SELF || b.type == gSENSOR_SELF;
    bool rim  = a.type == gSENSOR_RIM || b.type == gSENSOR_RIM;

    // some big distance to return if we don't know anything better
    REAL bigDistance = owner_->MaxWallsLength();
    if ( bigDistance <= 0 )
        bigDistance = owner_->GetDistance();

    REAL totallyFree = bigDistance * 8;
    REAL halfFree    = bigDistance * 6;
    REAL rimTunnel   = bigDistance * 4;
    REAL tunnel      = bigDistance * 2;

    if ( a.type == gSENSOR_NONE || b.type == gSENSOR_NONE )
    {
        // empty space! Woo!
        if ( a.type == gSENSOR_NONE && b.type == gSENSOR_NONE )
        {
            // totally empty space! groovy!
            return totallyFree;
        }
        else
        {
            return halfFree;
        }
    }
    else if ( a.hitOwner_ != b.hitOwner_ )
    {
        // different owners? Great, there has to be a way through!
        if ( rim )
        {
            if ( !self )
            {
                // we love going between the rim and enemies
                return rimTunnel;
            }
            else
            {
                // we don't love going between own wall and rim
                return bigDistance * .001 + ( a.before_hit - b.before_hit).Norm();
            }
        }

        if ( self )
        {
            REAL trapFactor = .5;
            if ( a.type == gSENSOR_SELF && a.lr == +1 )
            {
                // we're trapping another player and us with him. bad idea.
                return a.hitDistance_ * trapFactor;
            }
            if ( b.type == gSENSOR_SELF && b.lr == -1 )
            {
                return b.hitDistance_ * trapFactor;
            }
        }

        return tunnel;
    }
    else if ( rim )
    {
        // at least one rim wall? Take the distance between the hit positions.
        return ( a.before_hit - b.before_hit).Norm();
    }
    else if ( a.lr != b.lr )
    {
        // well, the longer the wall segment between the two points, the better.
        return fabsf( a.hitDistance_ - b.hitDistance_ ) * 2;
    }

    // default: hit distance
    return ( a.before_hit - b.before_hit).Norm();
}

bool gAINavigator::CanMakeTurn( uActionPlayer * action )
{
    return owner_->CanMakeTurn( ( action == &gCycle::se_turnRight ) ? 1 : -1 );
}

//!@return group of future paths
gAINavigator::PathGroup & gAINavigator::GetPaths()
{
    return paths_;
}

// check whether two sensors effecively hit the same wall
static inline bool sg_SameWall( gAINavigator::Sensor const & a,
                                gAINavigator::Sensor const & b )
{
    if( a.ehit == b.ehit )
    {
        return true;
    }
    if( a.hitOwner_ != b.hitOwner_ )
    {
        return false;
    }
    if( b.hit == 0 && a.hit != 0 )
    {
        return false;
    }

    REAL ratio = a.hit/b.hit;
    if( ratio < .99 || ratio > 1.01 )
    {
        return false;
    }

    return true;
}

// generates immediate distance data from sensor
static void sg_AddSensor( gAINavigator::Path & path, gAINavigator::Sensor const & sensor, REAL range, REAL rubber )
{
    REAL r = path.distance;
    if( sensor.type != gSENSOR_NONE )
    {
        // check for the direct problem that driving in that direction causes: we're about to hit a wall,
        // and rubber is going to run out eventually.
        r  = sensor.hit * range + rubber;
    }

    if( r < path.immediateDistance )
    {
        path.immediateDistance = r;
    }
}

// interpolates gaps of sensor data
static void sg_FillSensorGap( gAINavigator::Sensor const & left, gAINavigator::Sensor const & right, gAINavigator::Sensor & gap )
{
    // check orientation
    tASSERT( left.Direction() * right.Direction() >= 0 );

    if( gap.type == gSENSOR_NONE )
    {
        // do left and right sensors fit?
        if ( left.type != gSENSOR_NONE &&
             left.type == right.type &&
             left.lr  == right.lr &&
             left.hitOwner_ == right.hitOwner_ )
        {
            gap.type = left.type;
            gap.lr = left.lr;
            gap.hitOwner_ = left.hitOwner_;
            gap.hitTime_  = ( left.hitTime_ + right.hitTime_ )/2;
            gap.hitDistance_  = ( left.hitDistance_ + right.hitDistance_ )/2;
            gap.hit = 1.0;

            return;
        }
        
        // is there an enemy that may block?
        if( left.type == gSENSOR_ENEMY && left.lr == 1 )
        {
            gap.type = left.type;
            gap.lr = left.lr;
            gap.hitOwner_ = left.hitOwner_;
            gap.hitTime_  = left.hitTime_;
            gap.hitDistance_  = 0;
            gap.hit = left.hitDistance_/gap.Direction().Norm();
        }

        // is there an enemy that may block?
        if( right.type == gSENSOR_ENEMY && right.lr == -1 )
        {
            gap.type = right.type;
            gap.lr = right.lr;
            gap.hitOwner_ = right.hitOwner_;
            gap.hitTime_  = right.hitTime_;
            gap.hitDistance_  = 0;
            gap.hit = right.hitDistance_/gap.Direction().Norm();
        }
    }
}


static inline void sg_NoNeg( REAL & r )
{
    if( r < 0 )
        r = 0;
}

void gAINavigator::UpdatePaths()
{
    REAL lookahead = settings_.range;  // seconds to plan ahead

    // cylce data
    REAL speed = owner_->Speed();
    eCoord dir = owner_->Direction();
    eCoord pos = owner_->Position();

    REAL range = speed * lookahead;
    eCoord scanDir = dir * range;

    // get extra time we get through rubber usage
    REAL rubberGranted, rubberEffectiveness;
    sg_RubberValues( owner_->player, speed, rubberGranted, rubberEffectiveness );
    REAL rubberLeft = ( rubberGranted - owner_->GetRubber() )*rubberEffectiveness;
    // REAL rubberTime = rubberLeft/speed;
    // REAL rubberRatio = owner_->GetRubber()/rubberGranted;

    // this distance needs to be considered close
    REAL close = rubberLeft + speed * owner_->GetTurnDelay();

    REAL narrowFront = 1;

    // these checks can hit our last wall and fail. Temporarily set it to NULL.
    tJUST_CONTROLLED_PTR< gNetPlayerWall > lastWall = owner_->lastWall;
    owner_->lastWall = NULL;

    Sensor forward ( *this, pos, scanDir );
    forward.detect(1);

    // cast four diagonal rays
    Sensor forwardLeft ( *this, pos, scanDir.Turn(+1,+1 ) );
    Sensor left        ( *this, pos, scanDir.Turn( 0,+1 ) );
    Sensor backwardLeft( *this, pos, scanDir.Turn(-1,+narrowFront) );
    forwardLeft.detect(1);
    left.detect(1);
    backwardLeft.detect(1);
    Sensor forwardRight ( *this, pos, scanDir.Turn(+1,-1 ) );
    Sensor right        ( *this, pos, scanDir.Turn( 0,-1 ) );
    Sensor backwardRight( *this, pos, scanDir.Turn(-1,-narrowFront) );
    forwardRight.detect(1);
    right.detect(1);
    backwardRight.detect(1);

    sg_FillSensorGap( backwardLeft, forwardLeft, left );
    sg_FillSensorGap( forwardLeft, forwardRight, forward );
    sg_FillSensorGap( forwardRight, backwardRight, right );
    
    // sensor going backwards with fake entries
    Sensor self( *this, pos, scanDir.Turn(-1, 0) );
    self.before_hit = pos;
    self.windingNumber_ = owner_->windingNumber_;
    self.type = gSENSOR_SELF;
    self.hit = 0;
    self.hitDistance_ = 0;
    self.hitOwner_ = owner_;
    self.hitTime_ = owner_->LastTime();

    // get directions
    eCoord forwardDir  = dir;
    eGrid * grid = owner_->Grid();
    eCoord backwardDir = - forwardDir;
    int winding = owner_->WindingNumber();
    int wl = winding, wr = winding;
    grid->Turn( wl, -1 );
    grid->Turn( wr, 1 );
    eCoord leftDir = grid->GetDirection( wl );
    eCoord rightDir = grid->GetDirection( wr );

    // fill paths, first the simple cases
    self.lr = -1;
    {
        // U-Turn left
        Path & path = GetPaths().AccessPath( PathGroup::PATH_UTURN_LEFT  );
        path.Fill( *this, self, left, leftDir, backwardDir, -1 );
        sg_AddSensor( path, left, range, rubberLeft );

        // see if there would be more space to the right
        if( forward.hit > left.hit &&
            forwardRight.hit > left.hit &&
            right.hit > left.hit &&
            left.hit * range < close )
        {
            // calculate rubber usage of the two possibilities
            REAL rubberOffset = owner_->GetTurnDelay() * owner_->Speed()/range;
            REAL pRubberUsageA = rubberOffset - right.hit;
            REAL pRubberUsageB = pRubberUsageA - left.hit;
            REAL pRubberUsageC = rubberOffset - forward.hit;
            REAL uRubberUsage  = rubberOffset - left.hit;
            sg_NoNeg( pRubberUsageA );
            sg_NoNeg( pRubberUsageB );
            sg_NoNeg( pRubberUsageC );
            sg_NoNeg( uRubberUsage );
            if( uRubberUsage > pRubberUsageA + pRubberUsageB + pRubberUsageC )
            {
                // transform it into a P-Turn
                path.turn *= -1;
                path.immediateDistance = HUGE;
                sg_AddSensor( path, forward, range, rubberLeft );
                sg_AddSensor( path, right, range, rubberLeft );
                sg_AddSensor( path, forwardRight, range, rubberLeft );
            }
        }
    }

    self.lr = 1;
    // U-Turn right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_UTURN_RIGHT );
        path.Fill( *this, right, self, rightDir, backwardDir, 1 );
        sg_AddSensor( path, right, range, rubberLeft );

        // see if there would be more space to the left
        if( forward.hit > right.hit &&
            forwardLeft.hit > right.hit &&
            left.hit > right.hit &&
            right.hit * range < close )
        {
            // calculate rubber usage of the two possibilities
            REAL rubberOffset = owner_->GetTurnDelay() * owner_->Speed()/range;
            REAL pRubberUsageA = rubberOffset - left.hit;
            REAL pRubberUsageB = pRubberUsageA - right.hit;
            REAL pRubberUsageC = rubberOffset - forward.hit;
            REAL uRubberUsage  = rubberOffset - right.hit;
            sg_NoNeg( pRubberUsageA );
            sg_NoNeg( pRubberUsageB );
            sg_NoNeg( pRubberUsageC );
            sg_NoNeg( uRubberUsage );
            if( uRubberUsage > pRubberUsageA + pRubberUsageB + pRubberUsageC )
            {
                // transform it into a P-Turn
                path.turn *= -1;
                path.immediateDistance = HUGE;
                sg_AddSensor( path, forward, range, rubberLeft );
                sg_AddSensor( path, left, range, rubberLeft );
                sg_AddSensor( path, forwardLeft, range, rubberLeft );
            }
        }
    }

    // straight ahead
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_STRAIGHT );
        path.Fill( *this, forwardLeft, forwardRight, forwardDir, forwardDir, 0 );
        sg_AddSensor( path, forward, range, rubberLeft );

        // slighly favor going straight over anything else
        path.distance *= 1.0001;
    }

    // complicated cases

    // immediate turn right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_TURN_RIGHT );
        path.Fill( *this, forwardRight, backwardRight, rightDir, rightDir, 1 );

        REAL smallRightDistance = Distance( right, backwardRight );
        if( sg_SameWall( backwardRight, right ) || path.distance < smallRightDistance )
        {
            // we did not pass a kink, so the smaller distance applies
            path.distance = smallRightDistance;
        }

        sg_AddSensor( path, right, range, rubberLeft );
    }

    // immediate turn left
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_TURN_LEFT );
        path.Fill( *this, backwardLeft, forwardLeft, leftDir, leftDir, -1 );

        REAL smallLeftDistance = Distance( backwardLeft, left );
        if( sg_SameWall( backwardLeft, left ) || path.distance < smallLeftDistance )
        {
            path.distance = smallLeftDistance;
        }

        sg_AddSensor( path, left, range, rubberLeft );
    }

    // zigzag right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_ZIGZAG_RIGHT );
        path.Fill( *this, forward, right, rightDir, forwardDir, 1 );

        REAL driveOn = right.HitWallExtends( dir, pos );
        REAL side    = forward.HitWallExtends( rightDir, pos );
        if( driveOn > 0 && ( driveOn < forward.hit * range * ( 1-EPS ) || forward.type == gSENSOR_NONE || side > right.hit * range * ( 1+EPS ) ) )
        {
            // there is a gap waiting for us. Wait and take it.
            path.driveOn = driveOn;
            path.shortTermDirection = forwardDir;
            path.longTermDirection  = rightDir;

            sg_AddSensor( path, forward, range, rubberLeft );
        }
        else
        {
            sg_AddSensor( path, right, range, rubberLeft );
        }
    }

    // zigzag left
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_ZIGZAG_LEFT );
        path.Fill( *this, left, forward, leftDir, forwardDir, -1 );

        REAL driveOn = left.HitWallExtends( dir, pos );
        REAL side    = forward.HitWallExtends( leftDir, pos );
        if( driveOn > 0 && ( driveOn < forward.hit * range * ( 1-EPS ) || forward.type == gSENSOR_NONE || side > left.hit * range * ( 1+EPS ) ) )
        {
            // there is a gap waiting for us. Wait and take it.
            path.driveOn = driveOn;
            path.shortTermDirection = forwardDir;
            path.longTermDirection  = leftDir;

            sg_AddSensor( path, forward, range, rubberLeft );
        }
        else
        {
            sg_AddSensor( path, left, range, rubberLeft );
        }
    }
}

//! does the main thinking at the current time, knowing the next thought can't be sooner than minstep
REAL gAINavigator::Activate( REAL currentTime, REAL minstep, REAL penalty, Wish * wish )
{
    REAL lookahead = settings_.range;  // seconds to plan ahead

    // cylce data
    REAL speed = owner_->Speed();
    eCoord dir = owner_->Direction();
    eCoord pos = owner_->Position();

    REAL range= speed * lookahead;
    eCoord scanDir = dir * range;

    REAL frontFactor = .5;

    Sensor front(*this,pos,scanDir);
    front.detect(frontFactor);
    owner_->enemyInfluence.AddSensor( front, penalty, owner_ );

    REAL minMoveOn = 0, maxMoveOn = 0, moveOn = 0;

    // get extra time we get through rubber usage
    REAL rubberGranted, rubberEffectiveness;
    sg_RubberValues( owner_->player, speed, rubberGranted, rubberEffectiveness );
    REAL rubberTime = ( rubberGranted - owner_->GetRubber() )*rubberEffectiveness/speed;
    REAL rubberRatio = owner_->GetRubber()/rubberGranted;

    if ( front.ehit || wish )
    {
        turnedRecently_ = false;

        // these checks can hit our last wall and fail. Temporarily set it to NULL.
        tJUST_CONTROLLED_PTR< gNetPlayerWall > lastWall = owner_->lastWall;
        owner_->lastWall = NULL;

        REAL narrowFront = 1;

        // cast four diagonal rays
        Sensor forwardLeft ( *this, pos, scanDir.Turn(+1,+1 ) );
        Sensor backwardLeft( *this, pos, scanDir.Turn(-1,+narrowFront) );
        forwardLeft.detect(1);
        backwardLeft.detect(1);
        Sensor forwardRight ( *this, pos, scanDir.Turn(+1,-1 ) );
        Sensor backwardRight( *this, pos, scanDir.Turn(-1,-narrowFront) );
        forwardRight.detect(1);
        backwardRight.detect(1);

        // determine survival chances in the four directions
        REAL frontOpen = Distance ( forwardLeft, forwardRight );
        REAL leftOpen  = Distance ( forwardLeft, backwardLeft );
        REAL rightOpen = Distance ( forwardRight, backwardRight );
        REAL rearOpen = Distance ( backwardLeft, backwardRight );

        Sensor self( *this, pos, scanDir.Turn(-1, 0) );
        // fake entries
        self.before_hit = pos;
        self.windingNumber_ = owner_->windingNumber_;
        self.type = gSENSOR_SELF;
        self.hitDistance_ = 0;
        self.hitOwner_ = owner_;
        self.hitTime_ = currentTime;
        self.lr = -1;
        REAL rearLeftOpen = Distance( backwardLeft, self );
        self.lr = 1;
        REAL rearRightOpen = Distance( backwardRight, self );

        // override rim hugging
        if ( forwardRight.type == gSENSOR_SELF &&
             forwardLeft.type == gSENSOR_RIM &&
             backwardRight.type == gSENSOR_SELF &&
             backwardLeft.type == gSENSOR_RIM &&
             forwardRight.lr == -1 &&
             backwardRight.lr == -1 )
        {
            turnedRecently_ = true;
            if ( rightOpen > speed * ( owner_->GetTurnDelay() - rubberTime * .8 ) )
            {
                owner_->Act( &gCycle::se_turnRight, 1 );
                owner_->Act( &gCycle::se_turnRight, 1 );
            }
            else
            {
                owner_->Act( &gCycle::se_turnLeft, 1 );
                owner_->Act( &gCycle::se_turnLeft, 1 );
            }
        }

        if ( forwardLeft.type == gSENSOR_SELF &&
             forwardRight.type == gSENSOR_RIM &&
             backwardLeft.type == gSENSOR_SELF &&
             backwardRight.type == gSENSOR_RIM &&
             forwardLeft.lr == 1 &&
             backwardLeft.lr == 1 )
        {
            turnedRecently_ = true;
            if ( leftOpen > speed * ( owner_->GetTurnDelay() - rubberTime * .8 ) )
            {
                owner_->Act( &gCycle::se_turnLeft, 1 );
                owner_->Act( &gCycle::se_turnLeft, 1 );
            }
            else
            {
                owner_->Act( &gCycle::se_turnRight, 1 );
                owner_->Act( &gCycle::se_turnRight, 1 );
            }
        }

        // get the best turn direction
        int             bestDir      = ( leftOpen + rearLeftOpen > rightOpen + rearRightOpen ) ? 1 : -1;

        Sensor direct ( *this, pos, scanDir.Turn( 0, bestDir) );
        direct.detect( 1 );

        // restore last wall
        owner_->lastWall = lastWall;

        if( wish && wish->turn )
        {
            REAL minDistance = wish->minDistance;
            if( wish->turn > 0 )
            {
                if( wish->maxDisadvantage + leftOpen + rearLeftOpen < frontOpen + rightOpen + rearRightOpen )
                {
                    wish = 0;
                }
                else if ( leftOpen < wish->minDistance )
                {
                    if( leftOpen + rearLeftOpen < wish->minDistance || frontOpen < wish->minDistance || rightOpen < wish->minDistance )
                    {
                        wish = 0;
                    }
                    else if( wish->turn == -1 )
                    {
                        wish = 0;
                    }
                    else
                    {
                        // double back
                        owner_->Act( &gCycle::se_turnRight, 1 );
                        owner_->Act( &gCycle::se_turnRight, 1 );
                        owner_->Act( &gCycle::se_turnRight, 1 );
                        return owner_->GetTurnDelay() * 3;
                    }
                }
                else
                {
                    bestDir = 1;
                }
            }
            else if( wish->turn < 0 )
            {
                if( wish->maxDisadvantage + rightOpen + rearRightOpen < frontOpen + leftOpen + rearLeftOpen )
                {
                    wish = 0;
                }
                else if ( rightOpen < wish->minDistance )
                {
                    if( rightOpen + rearRightOpen < wish->minDistance || frontOpen < wish->minDistance || leftOpen < wish->minDistance )
                    {
                        wish = 0;
                    }
                    else if( wish->turn == -1 )
                    {
                        wish = 0;
                    }
                    else
                    {
                        // double back
                        owner_->Act( &gCycle::se_turnLeft, 1 );
                        owner_->Act( &gCycle::se_turnLeft, 1 );
                        owner_->Act( &gCycle::se_turnLeft, 1 );
                        return owner_->GetTurnDelay() * 3;
                    }
                }
                else
                {
                    bestDir = -1;
                }
            }

            if( !wish && frontOpen > minDistance )
            {
                // going straigt is not too bad
                return Owner()->GetTurnDelay();
            }

        }

        uActionPlayer * bestAction = ( bestDir > 0 ) ? &gCycle::se_turnLeft : &gCycle::se_turnRight;

        REAL            bestOpen     = ( bestDir > 0 ) ? leftOpen : rightOpen;
        Sensor &        bestForward  = ( bestDir > 0 ) ? forwardLeft : forwardRight;
        Sensor &        bestBackward = ( bestDir > 0 ) ? backwardLeft : backwardRight;

        // only turn if the hole has a shape that allows better entry after we do a zig-zag, or if we're past the good turning point
        // see how the survival chance is distributed between forward and backward half
        REAL forwardHalf  = Distance ( direct, bestForward );
        REAL backwardHalf = Distance ( direct, bestBackward );

        REAL forwardOverhang  = bestForward.HitWallExtends( bestForward.Direction(), pos );
        REAL backwardOverhang  = bestBackward.HitWallExtends( bestForward.Direction(), pos );

        // we have to move forward this much before we can hope to turn
        minMoveOn = bestBackward.HitWallExtends( dir, pos );

        if( wish )
        {
            if( backwardHalf < wish->minDistance )
            {
                return (minMoveOn/owner_->Speed())*1.1;
            }

            if ( !CanMakeTurn( bestAction ) )
            {
                return owner_->GetNextTurn( -bestDir ) - owner_->LastTime();
            }
            
            owner_->Act( bestAction, 1 );
            return Owner()->GetTurnDelay();
        }

        // maybe the direct to the side sensor is better?
        REAL minMoveOnOther = direct.HitWallExtends( dir, pos );

        // determine how far we can drive on
        maxMoveOn      = bestForward.HitWallExtends( dir, pos );
        REAL maxMoveOnOther = front.HitWallExtends( dir, pos );
        if ( maxMoveOn > maxMoveOnOther )
            maxMoveOn = maxMoveOnOther;

        if ( maxMoveOn > minMoveOnOther && forwardHalf > backwardHalf && direct.hitOwner_ == bestBackward.hitOwner_ )
        {
            backwardOverhang  = direct.HitWallExtends( bestForward.Direction(), pos );
            minMoveOn = minMoveOnOther;
        }

        // best place to turn
        moveOn = .5 * ( minMoveOn * ( 1 + rubberRatio ) + maxMoveOn * ( 1 - rubberRatio ) );

        // hit the brakes before you hit anything and if it's worth it
        bool brake = sg_brakeCycle > 0 &&
        front.hit * lookahead * sg_cycleBrakeDeplete < owner_->GetBrakingReservoir() &&
        sg_brakeCycle * front.hit * lookahead < 2 * speed * owner_->GetBrakingReservoir() &&
        ( maxMoveOn - minMoveOn ) > 0 &&
        owner_->GetBrakingReservoir() * ( maxMoveOn - minMoveOn ) < speed * owner_->GetTurnDelay();
        if ( frontOpen < bestOpen &&
             ( forwardOverhang <= backwardOverhang || ( minMoveOn < 0 && moveOn < minstep * speed ) ) )
        {
            turnedRecently_ = true;

            minMoveOn = maxMoveOn = moveOn = 0;

            {
                if ( !CanMakeTurn( bestAction ) )
                {
                    return owner_->GetNextTurn( -bestDir ) - owner_->LastTime();
                }

                owner_->Act( bestAction, 1 );
            }

            brake = false;
        }
        else
        {
            // the best
            REAL bestSoFar = frontOpen > bestOpen ? frontOpen : bestOpen;
            bestSoFar *= ( 10 * ( 1 - rubberRatio ) + 1 );

            if ( rearOpen > bestSoFar && ( rearLeftOpen > bestSoFar || rearRightOpen > bestSoFar ) )
            {
                brake = false;
                turnedRecently_ = true;

                bool goLeft = rearLeftOpen > rearRightOpen;

                // dead end. reverse into the opposite direction of the front wall
                uActionPlayer * bestAction = goLeft ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
                uActionPlayer * otherAction = !goLeft ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
                Sensor &        bestForward  = goLeft ? forwardLeft : forwardRight;
                Sensor &        bestBackward  = goLeft ? backwardLeft : backwardRight;
                Sensor &        otherForward  = !goLeft ? forwardLeft : forwardRight;
                Sensor &        otherBackward  = !goLeft ? backwardLeft : backwardRight;

                // space in the two directions available for turns
                REAL bestHit = bestForward.hit > bestBackward.hit ? bestBackward.hit : bestForward.hit;
                REAL otherHit = otherForward.hit > otherBackward.hit ? otherBackward.hit : otherForward.hit;

                bool wait = false;

                if ( !CanMakeTurn( bestAction ) )
                {
                    return owner_->GetNextTurn( -bestDir ) - owner_->LastTime();
                }

                // well, after a short turn to the right if space is tight
                if ( bestHit * lookahead < owner_->GetTurnDelay() + rubberTime )
                {
                    if ( otherHit < bestForward.hit * 2 && front.hit * lookahead > owner_->GetTurnDelay() * 2 )
                    {
                        // wait a bit, perhaps there will be a better spot
                        wait = true;
                    }
                    else
                    {
                        if ( !CanMakeTurn( otherAction ) )
                        {
                            return owner_->GetNextTurn( bestDir ) - owner_->LastTime();
                        }

                        owner_->Act( otherAction, 1 );

                        // there needs to be space ahead to finish the maneuver correctly
                        if ( maxMoveOn < speed * owner_->GetTurnDelay() )
                        {
                            // there isn't. oh well, turn into the wrong direction completely, see if I care
                            owner_->Act( otherAction, 1 );
                            wait = true;
                        }
                    }
                }

                if ( !wait )
                {
                    owner_->Act( bestAction, 1 );
                    owner_->Act( bestAction, 1 );
                }

                minMoveOn = maxMoveOn = moveOn = 0;
            }
        }

        // execute brake command
        owner_->Act( &gCycle::s_brake, brake ? 1 : -1 );
    }

    REAL space = moveOn;
    REAL minTime = space/speed;

    if ( turnedRecently_ )
        minTime = owner_->GetTurnDelay();

    return minTime;
}
