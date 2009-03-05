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

#include "gAIIdle.h"

#include "gCycle.h"
#include "gWall.h"
#include "tRandom.h"

gAIIdle::Settings::Settings()
: newWallBlindness(-.1)
, range( 1 )
{}

gAIIdle::Wish::Wish( gAIIdle const & idler )
: turn(0)
, maxDisadvantage( 1E+30 )
{
    gCycle & owner = *idler.Owner();
    minDistance = owner.GetTurnDelay() * owner.Speed();
}


gAIIdle::Sensor::Sensor(gAIIdle & ai,const eCoord &start,const eCoord &d)
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

void gAIIdle::Sensor::PassEdge(const eWall *ww,REAL time,REAL a,int r)
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

bool gAIIdle::Sensor::DoExtraDetectionStuff()
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

        // gAIIdle & enemyChatBot = Get( hitOwner_ );

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
            hit = 1E+40;
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
REAL gAIIdle::Sensor::HitWallExtends( eCoord const & dir, eCoord const & origin )
{
    if ( !ehit || !ehit->Other() )
    {
        return 1E+30;
    }

    REAL ret = -1E+30;
    eCoord ends[2] = { *ehit->Point(), *ehit->Other()->Point() };
    for ( int i = 1; i>=0; --i )
    {
        REAL newRet = eCoord::F( dir, ends[i]-origin );
        if ( newRet > ret )
            ret = newRet;
    }

    return ret;
}


gAIIdle::gAIIdle( gCycle * owner )
: lastTurn_( 0 )
, nextTurn_ ( 0 )
, turnedRecently_ ( 0 )
, owner_ ( owner )
{
}

gAIIdle::WallHug::WallHug()
: owner_ ( NULL )
  , lastTimeSeen_ ( 0 )
{
}

// promote seen walls to possible wallhug replacements
void gAIIdle::FindHugReplacement( Sensor const & sensor )
{
    gCycle const * owner = sensor.hitOwner_;
    if (!owner)
        return;

    // store as possible replacement
    if ( !hugReplacement_.owner_ && sensor.type != gSENSOR_SELF &&
         owner != hugLeft_.owner_ &&
         owner != hugRight_.owner_ )
    {
        hugReplacement_.owner_ = sensor.hitOwner_;
        hugReplacement_.lastTimeSeen_ = owner->LastTime();
    }

    // update timestamps
    if ( owner == hugLeft_.owner_ )
        hugLeft_.lastTimeSeen_ = owner->LastTime();
    if ( owner == hugRight_.owner_ )
        hugRight_.lastTimeSeen_ = owner->LastTime();
}

// determines the distance between two sensors; the size should give the likelyhood
// to survive if you pass through a gap between the two selected walls
REAL gAIIdle::Distance( Sensor const & a, Sensor const & b )
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

    if ( a.hitOwner_ != b.hitOwner_ )
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

        // or empty space
        if ( a.type == gSENSOR_NONE || b.type == gSENSOR_NONE )
            ret *= 2;

        return ret * selfHatred;
    }
    else if ( rim )
    {
        // at least one rim wall? Take the distance between the hit positions.
        return ( a.before_hit - b.before_hit).Norm() * selfHatred;
    }
    else if ( a.type == gSENSOR_NONE && b.type == gSENSOR_NONE )
    {
        // empty space! Woo!
        return owner_->GetDistance() * 256;
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

bool gAIIdle::CanMakeTurn( uActionPlayer * action )
{
    return owner_->CanMakeTurn( ( action == &gCycle::se_turnRight ) ? 1 : -1 );
}

//! does the main thinking at the current time, knowing the next thought can't be sooner than minstep
REAL gAIIdle::Activate( REAL currentTime, REAL minstep, REAL penalty, Wish * wish )
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

        // do we have a hug replacement candiate? If so, take it.
        if ( hugReplacement_.owner_ && !hugLeft_.owner_ && !hugRight_.owner_ )
        {
            // first time hugging? let the status quo decide.
            int lr = 0;
            if ( backwardLeft.hitOwner_ == hugReplacement_.owner_ )
                lr--;
            if ( forwardLeft.hitOwner_ == hugReplacement_.owner_ )
                lr--;
            if ( backwardRight.hitOwner_ == hugReplacement_.owner_ )
                lr++;
            if ( forwardRight.hitOwner_ == hugReplacement_.owner_ )
                lr++;

            if ( lr > 0 )
                hugRight_ = hugReplacement_;
            if ( lr < 0 )
                hugLeft_ = hugReplacement_;

            hugReplacement_.owner_ = 0;
        }

        if ( hugReplacement_.owner_ )
        {
            if( hugLeft_.lastTimeSeen_ < hugRight_.lastTimeSeen_ )
            {
                if ( hugReplacement_.lastTimeSeen_ > hugLeft_.lastTimeSeen_ )
                    hugLeft_ = hugReplacement_;
            }
            else
            {
                if ( hugReplacement_.lastTimeSeen_ > hugRight_.lastTimeSeen_ )
                    hugRight_ = hugReplacement_;
            }
            hugReplacement_.owner_ = 0;
        }

        FindHugReplacement( front );
        FindHugReplacement( forwardLeft );
        FindHugReplacement( forwardRight );
        FindHugReplacement( backwardLeft );
        FindHugReplacement( backwardRight );

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
                if( !( wish->maxDisadvantage + leftOpen + rearLeftOpen < frontOpen + rearRightOpen + rightOpen || 
                       leftOpen < minDistance ) )
                {
                    bestDir = 1;
                }
                else
                {
                    wish = 0;
                }
            }
            else if( wish->turn < 0 )
            {
                if( !( wish->maxDisadvantage + rightOpen + rearRightOpen < frontOpen + leftOpen + rearLeftOpen || 
                       rightOpen < wish->minDistance ) )
                {
                    bestDir = -1;
                }
                else
                {
                    wish = 0;
                }
            }

            if( !wish && frontOpen > minDistance )
            {
                // going straigt is not too bad
                return Owner()->GetTurnDelay();
            }

        }

        uActionPlayer * bestAction = ( bestDir > 0 ) ? &gCycle::se_turnLeft : &gCycle::se_turnRight;


        if( wish )
        {
            if ( !CanMakeTurn( bestAction ) )
            {
                return -1;
            }
            
            owner_->Act( bestAction, 1 );
            return Owner()->GetTurnDelay();
        }

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
                    return -1;
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
                    return -1;
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
                            return -1;
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

        // swap hugged walls if we're in fact grinding them the other way round
        if ( hugLeft_.owner_ == backwardRight.hitOwner_ ||
             hugRight_.owner_ == backwardLeft.hitOwner_ )
        {
            WallHug swap = hugRight_;
            hugRight_ = hugLeft_;
            hugLeft_ = swap;
        }
    }

    REAL space = moveOn;
    REAL minTime = space/speed;

    if ( turnedRecently_ )
        minTime = owner_->GetTurnDelay();

    return minTime;
}
