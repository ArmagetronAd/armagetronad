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

#include "zAI.h"

#include "zZone.h"
#include "zShape.h"
#include "gCycle.h"

#include "eDebugLine.h"

// *************************
// * Zone attack/defense   *
// *************************

//!@param cycle the cycle to control
//!@param zone  the zone to protect/attack
//!@papam maxstep the maximal delay until the next thought
zZoneEvaluator::zZoneEvaluator( gCycle const & cycle, zZone const & zone, REAL maxStep ): gAINavigator::FollowEvaluator( cycle )
{
    Init( cycle, zone, maxStep );
}

zZoneEvaluator::~zZoneEvaluator(){}

void zZoneEvaluator::Init( gCycle const & cycle, zZone const & zone, REAL maxStep )
{
    zShape * shape = zone.getShape();
    if ( !shape )
    {
        return;
    }
    
    eCoord pos    = cycle_.Position();
    eCoord center = shape->findCenter();

    // REAL insideness = (pos-center).NormSquared()/(edge-center).NormSquared();
        
    eCoord tailToChase, tailToChaseVelocity;

    if( zone.Team() == cycle.Team() )
    {
        gCycle::WallInfo info;
            
        // don't get the real tail. Instead, get a bit in front and extrapolate back.
        REAL endOffset = cycle.GetTurnDelay();
        if( maxStep > endOffset )
        {
            endOffset = maxStep;
        }

        cycle.FillWallInfo( info, .5, -endOffset*cycle.Speed() );

        tailToChaseVelocity = info.tailDir * cycle.Speed();
        tailToChase = info.tailPos - tailToChaseVelocity * endOffset;

        {
            // blend in standard circular path
            REAL blend = 2 - cycle.GetDistance()/cycle.ThisWallsLength();
            if ( blend > 0 )
            {
                if( blend > 1 )
                {
                    blend = 1;
                }
                    
                eCoord rand;
                eCoord edge   = shape->findPointNear( rand );
                REAL angle = 2 * M_PI * cycle.GetDistance()/cycle.ThisWallsLength();
                REAL angularVelocity = 2 * M_PI * cycle.Speed()/cycle.ThisWallsLength();
                eCoord tailToChase2 = center + .9 * ( edge - center ).Turn( cosf(angle), sinf( angle ) );
                eCoord tailToChaseVelocity2 = angularVelocity*( edge - center ).Turn( -sinf( angle ), cosf(angle) );
                tailToChase = tailToChase * ( 1 - blend ) + tailToChase2 * blend;
                tailToChaseVelocity = tailToChaseVelocity * ( 1 - blend ) + tailToChaseVelocity2 * blend;
            }
        }

#ifdef DEBUG
        eDebugLine::SetTimeout(.5);
        eDebugLine::SetColor  (0, 1, 1);
        eDebugLine::Draw(tailToChase, .5, tailToChase, 5.5);
        eDebugLine::SetTimeout(0);
#endif

        // make sure tail pos to chase lies inside an acceptable strip around the zone border
        eCoord tailEdge = shape->findPointNear( tailToChase );
        REAL tailInsideness = (tailToChase-center).NormSquared()/(tailEdge-center).NormSquared();
        REAL minInsideness = .5, maxInsideness = .9;
        if( tailInsideness < minInsideness )
        {
            // close to center. Get opposing point and mirror it.
            gCycle::WallInfo info2;
            cycle.FillWallInfoFlexible( info2, cycle.ThisWallsLength()/2 );
            eCoord tailToChase2 = 2 * center - info2.tailPos;
            eCoord tailToChaseVelocity2 = -info2.tailDir * cycle.Speed();
            REAL blend = tailInsideness/minInsideness;

#ifdef DEBUG
            eDebugLine::SetTimeout(.5);
            eDebugLine::SetColor  (1, 0, 0);
            eDebugLine::Draw(tailToChase2, .5, tailToChase2, 5.5);
            eDebugLine::Draw(info2.tailPos, .5, info2.tailPos, 5.5);
            eDebugLine::SetTimeout(0);
#endif

            // blend chase point with mirrored opposite point.
            tailToChase = tailToChase * blend + tailToChase2 * ( 1 - blend );
            tailToChaseVelocity = tailToChaseVelocity * blend + tailToChaseVelocity2 * ( 1 - blend );

            tailInsideness = (tailToChase-center).NormSquared()/(tailEdge-center).NormSquared();

            tailToChase = center + (tailToChase-center) * sqrt( minInsideness/tailInsideness );
        }
        else if( tailInsideness > maxInsideness )
        {
            tailToChase = center + (tailToChase-center) * sqrt( maxInsideness/tailInsideness );
        }
    }
    else
    {
        tailToChase = center;
    }

#ifdef DEBUG
    eDebugLine::SetTimeout(.5);
    eDebugLine::SetColor  (0, 0, 1);
    eDebugLine::Draw(tailToChase, .5, tailToChase, 5.5);
    eDebugLine::SetTimeout(0);
#endif

    SetTarget( tailToChase, tailToChaseVelocity );

}
   
zStateDefend::zStateDefend( gAIPlayer & ai ): gAIPlayer::State( ai ){}
zStateDefend::~zStateDefend(){}

REAL zStateDefend::Think( REAL maxStep )
{ 
    gCycle & cycle = *Parent().Object();
    Navigator().UpdatePaths();
    gAINavigator::EvaluationManager manager( Navigator().GetPaths() );
    manager.Evaluate( gAINavigator::SuicideEvaluator( cycle ), 1 );
    manager.Evaluate( gAINavigator::SuicideEvaluator( cycle, maxStep ), 1 );
    manager.Evaluate( gAINavigator::TrapEvaluator( cycle ), 1 );
    manager.Reset();
    manager.Evaluate( gAINavigator::CowardEvaluator( cycle ), -.1 );
    manager.Evaluate( gAINavigator::SpaceEvaluator( cycle ), .1 );
    manager.Evaluate( gAINavigator::RandomEvaluator(), .01 );
    manager.Evaluate( gAINavigator::PlanEvaluator(), .1 );

    zZone const * zoneTarget = dynamic_cast< zZone const * >( Parent().GetTarget() );
    if ( zoneTarget )
    {
        manager.Evaluate( zZoneEvaluator( cycle, *zoneTarget, maxStep ), gAINavigator::EvaluationManager::BLEND_ADD, 2 );
    }
    else
    {
        Parent().SwitchToSurvival();
    }

    gAINavigator::CycleControllerBasic controller;
    return manager.Finish( controller, *Parent().Object(), maxStep );
}
