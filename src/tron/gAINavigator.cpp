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
            hit = HUGE;
            return false;
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

    this->left.owner = left.hitOwner_;
    this->right.owner = right.hitOwner_;
    this->left.lr = left.lr;
    this->right.lr = right.lr;

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
    case PATH_PTURN_LEFT:
        paths[ PATH_UTURN_LEFT ].followedSince = last.followedSince + 1;
        break;
    case PATH_PTURN_RIGHT:
        paths[ PATH_UTURN_RIGHT ].followedSince = last.followedSince + 1;
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

gAINavigator::PathEvaluation::PathEvaluation() : veto( false ), score( 0 ){}

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
        
//!@param evaluator  evaluator doing the core work
//!@param scale      weight factor for that work
void gAINavigator::EvaluationManager::Evaluate( PathEvaluator const & evaluator, REAL scale )
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

        // update stored evaluation
        store.score += scale * evaluation.score;
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
: owner ( NULL ), lastTimeSeen ( 0 ), lr( 0 )
{
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

    // avoid. own. walls.
    REAL selfHatred = 1;
    if ( a.type == gSENSOR_SELF )
    {
        selfHatred *= .5;
        if ( a.lr > 0 )
        {
            selfHatred *= .5;
            if ( b.type == gSENSOR_RIM )
                selfHatred *= .25;
        }
    }
    if ( b.type == gSENSOR_SELF )
    {
        selfHatred *= .5;
        if ( b.lr < 0 )
        {
            selfHatred *= .5;
            if ( a.type == gSENSOR_RIM )
                selfHatred *= .25;
        }
    }

    // some big distance to return if we don't know anything better
    REAL bigDistance = owner_->MaxWallsLength();
    if ( bigDistance <= 0 )
        bigDistance = owner_->GetDistance();

    if ( a.type == gSENSOR_NONE || b.type == gSENSOR_NONE )
    {
        // empty space! Woo!
        if ( a.type == gSENSOR_NONE && b.type == gSENSOR_NONE )
        {
            // totally empty space! groovy!
            return bigDistance * 512;
        }
        else
        {
            return bigDistance * 256;
        }
    }
    else if ( a.hitOwner_ != b.hitOwner_ )
    {
        // different owners? Great, there has to be a way through!
        REAL ret =
        a.hitDistance_ + b.hitDistance_;

        if ( rim )
        {
            ret = bigDistance * .001 + ret * .01 + ( a.before_hit - b.before_hit).Norm();

            // we love going between the rim and enemies
            if ( !self )
                ret = bigDistance * 2;
        }

        // minimal factor should be 1, this path should never return something smaller than the
        // paths where only one cycle's walls are hit
        ret *= 16;

        return ret * selfHatred;
    }
    else if ( rim )
    {
        if( a.type != gSENSOR_RIM || b.type != gSENSOR_RIM )
        {
            // not too bad if one of the walls is not the rim
            selfHatred *= 4;
        }

        // at least one rim wall? Take the distance between the hit positions.
        return ( a.before_hit - b.before_hit).Norm() * selfHatred;
    }
    else if ( a.lr != b.lr )
    {
        // different directions? Also great!
        return ( fabsf( a.hitDistance_ - b.hitDistance_ ) + .25 * bigDistance ) * selfHatred;
    }
    /*
      else if ( - 2 * a.lr * (a.windingNumber_ - b.windingNumber_ ) > owner_->Grid()->WindingNumber() )
      {
      // this looks like a way out to me
      return fabsf( a.hitDistance_ - b.hitDistance_ ) * 10 * selfHatred;
      }
    */
    else
    {
        // well, the longer the wall segment between the two points, the better.
        return fabsf( a.hitDistance_ - b.hitDistance_ ) * selfHatred;
    }

    // default: hit distance
    return ( a.before_hit - b.before_hit).Norm() * selfHatred;
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

    // do left and right sensors fit?
    if( gap.type == gSENSOR_NONE &&
        left.type != gSENSOR_NONE &&
        left.type == right.type &&
        left.lr  == right.lr &&
        left.hitOwner_ == right.hitOwner_ )
    {
        gap.type = left.type;
        gap.lr = left.lr;
        gap.hitOwner_ = left.hitOwner_;
        gap.hitTime_  = ( left.hitTime_ + right.hitTime_ )/2;
        gap.hitDistance_  = ( left.hitDistance_ + right.hitDistance_ )/2;
    }
}

void gAINavigator::UpdatePaths()
{
#ifdef DEBUG
    // to debug specific situations on playback
    static int count = 0;
    count++;
    if( count == 0 )
    {
        st_Breakpoint();
    }
#endif


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
    self.hitDistance_ = 0;
    self.hitOwner_ = owner_;
    self.hitTime_ = owner_->LastTime();

    // get directions
    eCoord forwardDir  = dir;
    eGrid * grid = owner_->Grid();
    eCoord backwardDir = - forwardDir;
    int winding = grid->DirectionWinding( forwardDir );
    int wl = winding, wr = winding;
    grid->Turn( wl, -1 );
    grid->Turn( wr, 1 );
    eCoord leftDir = grid->GetDirection( wl );
    eCoord rightDir = grid->GetDirection( wr );

    // fill paths, first the simple cases
    self.lr = -1;
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_UTURN_LEFT  );
        path.Fill( *this, self, backwardLeft, leftDir, backwardDir, -1 );
        sg_AddSensor( path, left, range, rubberLeft );
    }
    // P-Turn left
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_PTURN_LEFT );
        path.Fill( *this, self, backwardLeft, rightDir, backwardDir, 1 );

        // P-Turning takes a bit of space on the other side
        sg_AddSensor( path, forward, range, rubberLeft );
        sg_AddSensor( path, right, range, rubberLeft );
        sg_AddSensor( path, forwardRight, range, rubberLeft );

        // slighly favor U-Turns
        path.distance *= .999;
    }

    self.lr = 1;
    // U-Turn right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_UTURN_RIGHT );
        path.Fill( *this, backwardRight, self, rightDir, backwardDir, 1 );
        sg_AddSensor( path, right, range, rubberLeft );
    }
    // P-Turn right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_PTURN_RIGHT );
        path.Fill( *this, backwardRight, self, leftDir, backwardDir, -1 );

        // P-Turning takes a bit of space on the other side
        sg_AddSensor( path, forward, range, rubberLeft );
        sg_AddSensor( path, left, range, rubberLeft );
        sg_AddSensor( path, forwardLeft, range, rubberLeft );

        // slighly favor U-Turns
        path.distance *= .999;
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
        path.Fill( *this, right, backwardRight, rightDir, rightDir, 1 );

        if( !sg_SameWall( backwardRight, right ) )
        {
            // if we just passed a kink, see if we can get a better value for immediate turns
            REAL totalRightDistance = Distance( forwardRight, backwardRight );
            if ( path.distance < totalRightDistance )
            {
                path.distance = totalRightDistance;
            }
        }

        sg_AddSensor( path, right, range, rubberLeft );
    }

    // immediate turn left
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_TURN_LEFT );
        path.Fill( *this, backwardLeft, left, leftDir, leftDir, -1 );

        if( !sg_SameWall( backwardLeft, left ) )
        {
            REAL totalLeftDistance = Distance( backwardLeft, forwardLeft );
            if ( path.distance < totalLeftDistance )
            {
                path.distance = totalLeftDistance;
            }
        }

        sg_AddSensor( path, left, range, rubberLeft );
    }

    // zigzag right
    {
        Path & path = GetPaths().AccessPath( PathGroup::PATH_ZIGZAG_RIGHT );
        path.Fill( *this, forward, right, rightDir, forwardDir, 1 );

        REAL driveOn = right.HitWallExtends( dir, pos );
        REAL side    = forward.HitWallExtends( rightDir, pos );
        if( driveOn < forward.hit * range * ( 1-EPS ) || forward.type == gSENSOR_NONE || side > right.hit * range * ( 1+EPS ) )
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
        if( driveOn < forward.hit * range * ( 1-EPS ) || forward.type == gSENSOR_NONE || side > left.hit * range * ( 1+EPS ) )
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
