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

#include "gAIBase.h"
#include "gArena.h"
#include "eGrid.h"
#include "ePath.h"
#include "eGameObject.h"
#include "gWall.h"
#include "gSensor.h"
#include "gCycle.h"
#include "tConsole.h"
#include "rFont.h"
#include "rScreen.h"
#include "eFloor.h"
#include "eDebugLine.h"
#include "gAICharacter.h"
#include "gAINavigator.h"
#include "tReferenceHolder.h"
#include "tRandom.h"
#include "tRecorder.h"
#include "zone/zZone.h"
#include <stdlib.h>
#include <cstdlib>
#include <memory>

#include "nProtoBuf.h"
#include "gAIBase.pb.h"

#define AI_REACTION          0 
#define AI_EMERGENCY         1 
#define AI_RANGE             2 
#define AI_STATE_TRACE       3 
#define AI_STATE_CLOSECOMBAT 4 
#define AI_STATE_PATH        5 
#define AI_LOOP              6 
#define AI_ENEMY             7 
#define AI_TUNNEL            8 
#define AI_DETECTTRACE       9 
#define AI_STARTSTATE        10
#define AI_STARTSTRAIGHT     11
#define AI_STATECHANGE       12

static tReferenceHolder< gAIPlayer > sg_AIReferences;

static tCONTROLLED_PTR(gAITeam) sg_AITeam = NULL;

gSimpleAIFactory *gSimpleAIFactory::factory_ = NULL;

gSimpleAI::~gSimpleAI()
{
    con << "simple AI destroyed.\n";
}

gSimpleAI * gSimpleAIFactory::Create( gCycle * object ) const
{
    gSimpleAI * ai = DoCreate();
    if ( ai )
        ai->SetObject( object );
    return ai;
}

gSimpleAIFactory * gSimpleAIFactory::Get()
{
    return factory_;
}

void gSimpleAIFactory::Set( gSimpleAIFactory * factory )
{
    factory_ = factory;
}

static gAITeam* AITeam()
{
    if ( !sg_AITeam )
    {
        sg_AITeam = tNEW( gAITeam );
    }

    return sg_AITeam;
}

static void ClearAITeam()
{
    sg_AITeam = NULL;
}

// an instance of this class will prevent deterministic random lookups
class gRandomController
{
public:
    static gRandomController * random_;
    gRandomController * lastRandom_;
    tRandomizer & randomizer_;

    gRandomController( tRandomizer & randomizer = tRandomizer::GetInstance() )
            : lastRandom_( random_ ), randomizer_( randomizer )
    {
        random_ = this;
    }

    ~gRandomController()
    {
        random_ = lastRandom_;
    }
};

gRandomController * gRandomController::random_ = 0;

REAL Random()
{
    if ( gRandomController::random_ )
    {
        tRandomizer & randomizer = gRandomController::random_->randomizer_;
        return randomizer.Get();
    }
    else
    {
        tRandomizer & randomizer = tRandomizer::GetInstance();
        return randomizer.Get();
    }
}

static REAL Delay()
{
    REAL delay = sg_delayCycle * .9f;

    REAL fd    = se_AverageFrameTime()*1.5f;
    if ( fd > delay)
        delay = fd;

    return delay;
}

static gAICharacter* BestIQ( int iq )
{
    int i;

    static tArray<bool> inGame(gAICharacter::s_Characters.Len());
    for (i = gAICharacter::s_Characters.Len()-1; i>=0; i--)
        inGame(i) = false;

    for (i = se_PlayerNetIDs.Len()-1; i>=0; i--)
    {
        // count the active AIs
        ePlayerNetID *p = se_PlayerNetIDs(i);
        gAIPlayer  *ai  = dynamic_cast<gAIPlayer*>(p);
        if (ai && ai->Character() )
        {
            int index = ai->Character() - &(gAICharacter::s_Characters(0));
            inGame(index) = true;
        }
    }

    // find the best fitting IQ that could be inserted:
    gAICharacter* bestIQ = 0;
    for (i = gAICharacter::s_Characters.Len()-1; i>=0; i--)
        if (!inGame(i))
            if (!bestIQ || fabs(bestIQ->iq - iq) > fabs(gAICharacter::s_Characters(i).iq - iq))
                bestIQ = &gAICharacter::s_Characters(i);

    return bestIQ;
}

gAITeam::gAITeam( Game::AITeamSync const & sync, nSenderInfo const & sender )
  : eTeam( sync.base(), sender )
{
    //	teams.Remove( this, listID );
}

gAITeam::gAITeam()
{
    //	teams.Remove( this, listID );
}

static nNetObjectDescriptor<gAITeam,Game::AITeamSync> gAITeam_init(331);

nNetObjectDescriptorBase const & gAITeam::DoGetDescriptor() const
{
    return gAITeam_init;
}

// fill empty team positions with AI players
void gAITeam::BalanceWithAIs(bool balanceWithAIs)
{
    // set correct team number
    EnforceConstraints();

    int numTeams = 0, numTeamsWithPlayers = 0;

    // determine the maximum number of human players on a team
    int i;
    int maxP = eTeam::minPlayers;
    for ( i = teams.Len()-1; i>=0; --i )
    {
        eTeam *t = teams(i);

        t->UpdateProperties();

        if ( t->BalanceThisTeam() )
            numTeams++;

        if ( t->NumHumanPlayers() > 0 )
            numTeamsWithPlayers++;

        int humans = t->NumHumanPlayers();

        if ( humans > maxP )
        {
            maxP = humans;
        }
    }

    // make sure all teams have equal number
    for ( i = teams.Len()-1; i>=0; --i )
    {
        eTeam *t = teams(i);

        if ( t->BalanceThisTeam() )
        {
            int wishAIs = maxP - t->NumHumanPlayers();

            // don't add AI players to human team if it is not requested
            if ( !balanceWithAIs && t->NumHumanPlayers() > 0 )
                wishAIs = 0;

            // don't genereate AI only teams if it is not requested by the min team count
            if ( numTeamsWithPlayers >= minTeams && t->NumHumanPlayers() == 0 )
                wishAIs = 0;

            // add AI only teams to team count
            if ( wishAIs > 0 && t->NumHumanPlayers() == 0 )
                numTeamsWithPlayers++;

            int AIs = t->NumAIPlayers();

            while ( AIs > wishAIs )
            {
                int j;
                gAIPlayer* throwOut = NULL;
                for ( j=t->NumPlayers()-1; j>=0; --j )
                {
                    gAIPlayer* player = dynamic_cast<gAIPlayer*>( t->Player( j ) );
                    if ( player && !player->IsHuman() && ( !throwOut || !throwOut->Character() || !player->Character() || throwOut->Character()->iq > player->Character()->iq ) )
                    {
                        throwOut = player;
                    }
                }

                if ( throwOut )
                {
                    throwOut->RemoveFromGame();
                    //					throwOut->ReleaseOwnership();

#ifdef DEBUG
                    if ( throwOut->GetRefcount() > 1 )
                    {
                        st_Breakpoint();
                    }
#endif

                    sg_AIReferences.Remove( throwOut );
                }

                --AIs;
            }

            while ( AIs < wishAIs )
            {
                // add the smartest AI player you can get
                gAICharacter* best = BestIQ( 1000 );

                if ( best )
                {
                    gAIPlayer *ai  = tNEW( gAIPlayer ) ();
                    ai->character_ = best;
                    ai->SetName( best->name );
                    ai->SetTeam( t );
                    ai->UpdateTeam();

                    sg_AIReferences.Add( ai );
                }

                ++AIs;
            }
        }
    }

    // get rid of deleted netobjects (teams, mostly)
    nNetObject::ClearAllDeleted();
}

// may player join this team?
bool gAITeam::PlayerMayJoin(const ePlayerNetID* player) const
{
    return !player->IsHuman();
}


// this class describes a single wall close event
class gCycleTouchEvent{
public:
    REAL dist;      // the position on this cycle's wall the touching happened
    REAL otherDist; // the position on the other cylce's wall
    int  otherSide; // the side of the other cylce this wall touches
    int  winding;   // winding number to add if we cross here

    gCycleTouchEvent()
    {
        dist      = 0;
        otherDist = 0;
        otherSide = -100;
    }
};

class gCycleMemoryEntry{
public:
    gCycleMemory *memory;
    int id;
    nObserverPtr< gCycle > cycle;

    gCycleMemoryEntry(gCycleMemory* m, const gCycle* c)
            :memory(m),id(-1), cycle(c)
    {
        memory->memory.Add(this, id);

        max[0].dist = -1E+30;
        max[1].dist = -1E+30;
        min[0].dist =  1E+30;
        min[1].dist =  1E+30;
    }

    ~gCycleMemoryEntry()
    {
        memory->memory.Remove(this, id);
    }

    // usable data
    gCycleTouchEvent max[2]; // latest touch event (with the given cylce)
    gCycleTouchEvent min[2]; // earliest touch event
};

#define TOL 4

// look for a closed loop in the walls if cycle a hits cycle b's wall at
// distance bDist and on side bSide.
// Look for the loop in driving direction of b if dir is 1 or to the other side of dir is 0.
// the end of the loop is reached when the wall of cycle a is driven along in
// direction aEndDir, passing the distance aEndDist and the side's bit is set
// in aEndSides.
// Cycles that will be closed in the loop are stored in the array
// closedIn.
// return value: the number of open points this loop contains
// (if this is >0, that usually means the space is wide open)
//  or -1 if there is no loop.
static bool CheckLoop(const gCycle *a, const gCycle *b,
                      REAL bDist, int bSide, int dir,
                      tArray<const gCycle*>& closedIn, int& winding,
                      REAL aEndDist = 0, int aEndSides = 3, int aEndDir = 1 )
{
    tASSERT(0<= bSide && 1 >= bSide);
    tASSERT(0<= dir && 1 >= dir);

    int tries = 10;       // so long until we give up
    int ends  = 0;

    bool bClosedIn    = false;

    const gCycle *run = b;      // we run along this cycle's wall
    int end           = dir;    // and move towards this end its wall wall
    int side          = bSide;  // we are on this side of the cycle
    REAL dist         = bDist;  // and are at this distance.
    winding           = 0;      // the winding number we collected

    int turn     = a->Grid()->WindingNumber();
    int halfTurn = turn >> 1;


#ifdef DEBUG
    //  con << "\n";
#endif

    while(tries-- > 0 && run &&
            !(run == a &&
              end == aEndDir &&
              aEndSides & (1 << side) &&
              (end > 0 ? dist >= aEndDist : dist <= aEndDist ) ) )
    {
#ifdef DEBUG
        //      con << "end = " << end << ", side = " << side << ", dist = " << dist
        //	  << ", winding = " << winding << "\n";
#endif
        if (end > 0)
        {

            // find the last connection
            gCycleMemoryEntry* last = run->memory.Latest(side);
            if (!last || last->max[side].dist <= dist + TOL)
            {
                // no interference. We can move directly around the cylce
                // and close it in.
#ifdef DEBUG
                //	      con << "Turning around...\n";
#endif

                winding += halfTurn *
                           ( side > 0 ? -1 : 1);

                end  = 0;
                side = 1-side;
                closedIn[closedIn.Len()] = run;
                dist = run->GetDistance();

                // detect early loop
                if (run == b)
                {
                    if (bClosedIn)
                    {
                        winding = 0;
                        return false;
                    }
                    else
                        bClosedIn = true;
                }
            }
            else
            {
#ifdef DEBUG
                //	      con << "Crossing...\n";
#endif

                // find the first connection
                gCycleMemoryEntry* first = run->memory.Earliest(side);
                if (first && first->min[side].dist >= dist + TOL)
                {
                    // we cross the connection:
                    winding += first->min[side].winding;
                    run      = first->cycle;
                    end      = (side == first->min[side].otherSide) ? 1 : 0;

                    //		  if (end == 0)
                    // we need to turn around to follow
                    winding += halfTurn * (side > 0 ? 1 : -1);

                    dist     = first->min[side].otherDist;
                    side     = first->min[side].otherSide;
                }
                else
                {
                    winding = 0;
                    return false;
                }
            }
        }
        else // dir = -1, we move towards the end
        {
            // find the first connection
            gCycleMemoryEntry* first = run->memory.Earliest(side);
            if (!first || first->min[side].dist >= dist - TOL)
            {
#ifdef DEBUG
                //	      con << "Turning around...\n";
#endif


                // no interference. We can move directly around the cylce's end.
                winding += halfTurn * ( side > 0 ? 1 : -1);

                end  = 1;
                side = 1-side;
                ends++;
                dist = -2 * TOL;
            }
            else
            {
#ifdef DEBUG
                //	      con << "Crossing...\n";
#endif


                // find the latest connection
                gCycleMemoryEntry* last = run->memory.Latest(side);
                if (last && last->max[side].dist <= dist - TOL)
                {
                    // we cross the connection:
                    winding += last->max[side].winding;

                    // we need to turn around to start:
                    winding += halfTurn * (side > 0 ? -1 : 1);

                    run      = last->cycle;
                    end      = (side == last->max[side].otherSide) ? 0 : 1;

                    //		  if (end == 1)
                    // we need to turn around to follow
                    //		  winding -= halfTurn * (side > 0 ? 1 : -1);

                    dist     = last->max[side].otherDist;
                    side     = last->max[side].otherSide;
                }
                else
                    // uh oh. we are already closed in. No chance...
                {
                    winding = 0;
                    return false;
                }
            }
        }
    }

#ifdef DEBUG
    //      con << "end = " << end << ", side = " << side << ", dist = " << dist
    //	  << ", winding = " << winding << "\n\n";
#endif

    if (tries >= 0)
    {
        return true;
    }
    else
    {
        winding = 0;
        return false;
    }
}


// see if the given Cycle is trapped currently
static bool IsTrapped(const gCycle *trapped, const gCycle *other)
{
    tArray<const gCycle*> closedIn;
    int winding = 0;
    if (CheckLoop(trapped, trapped, trapped->GetDistance(), 1, 0, closedIn, winding, trapped->GetDistance() - 1))
    {
        if (winding + 2 < 0)
        {
            // see if the other cylce is trapped with him
            for (int i = closedIn.Len()-1; i>=0; i--)
                if (other == closedIn(i))
                    return false;  // we can get him!

            // no. trapped is trapped allone.
            return true;
        }
    }

    return false;
}

// see if the given Cycle is trapped currently
bool IsTrapped(const eGameObject *trapped, const gCycle *other)
{
    gCycle const * cycle = dynamic_cast< gCycle const * >( trapped );
    if( cycle )
        return IsTrapped( cycle, other );
    return false;
}


// data about a loop
class gLoopData{
public:
    bool loop;                       // is there a loop?
    int  winding;                    // in what direction does it go?
    tArray<const gCycle*>closedIn;   // which cycles are closed in?

    gLoopData():loop(false), winding(0){}
    void AddCycle(const gCycle* c){closedIn[closedIn.Len()] = c;}
};

#ifdef OBSOLETES

// hit data
class gHitData{
public:
    const eHalfEdge*  edge;      // edge we hit
    gSensorWallType   wallType;  // type of the wall hit
    int               lr;        // does the wall go left or right?
    REAL              distance;  // distance to the wall

    // additional info if the wall that got hit is a cycle wall
    gCycle         *otherCycle;    // the cylce that the hit wall belongs to
    REAL            driveDistance; // the distance it had travelled when it was at the place we hit
    int             windingNumber; // the winding number at the place hit

    gHitData():edge(NULL), wallType(gSENSOR_NONE), otherCycle(NULL){}

    bool Hit() const {return edge;}

    void AddHit(const eCoord& origin, const eCoord& dir, const gSensor& sensor, int winding)
    {
        if (!sensor.ehit)
            return; // no hit, nothing to do

        /*
        REAL otherDist = eCoord::F(*sensor.ehit->Point(), dir)
                         - eCoord::F(origin, dir);

        {
            REAL otherDist2 = eCoord::F(*sensor.ehit->Other()->Point(), dir)
                              - eCoord::F(origin, dir);
            if (otherDist2 < otherDist)
                otherDist = otherDist2;
        }
        */
        REAL otherDist = eCoord::F( dir, sensor.before_hit - origin );

        if (otherDist < EPS)
            return; // the new hit is a wall we apparently drive along; nothing to do

        if (Hit() && distance < otherDist)
            return; // the hit that is already stored is more relevant. Ignore the new hit.

        // copy the relevant data
        edge     = sensor.ehit;
        wallType = sensor.type;
        lr       = sensor.lr;
        distance = otherDist;

        REAL alpha = edge->Ratio(sensor.before_hit);

        // get the extra information from the wall we hit:
        gPlayerWall *w = dynamic_cast<gPlayerWall*>(edge->GetWall());
        if (!w)
        {
            alpha = 1-alpha;
            w = dynamic_cast<gPlayerWall*>(edge->Other()->GetWall());
        }

        if (w)
        {
            otherCycle     = w->Cycle();
            driveDistance  = w->Pos(alpha);
            windingNumber  = w->WindingNumber() - winding;
        }
    }
};

// special sensor that scans a broader area (not just a raycast)
class gAISensor{
public:
    // the raw data we collect:

    const gCycle* cycle; // the cycle that sent out this sensor

    bool     hit;       // whether a dangerous spot is hit

    gHitData front;      // what happens in our front
    gHitData sides[2];   // and on our sides

    // what we make of it:
    gLoopData frontLoop[2];   // does the front wall we hit cause us to be trapped if we turn left or right?
    gLoopData sideLoop[2][2]; // do the two side walls cause us to be trapped if we turn/drive straight on?

    REAL distance;            // distance to the closest wall

    bool Hit() const
    {
        return hit;
    }

    void DetectLoop(const gHitData& hit, gLoopData loopData[2])
    {
        // avoid loops:
        gCycle *other = hit.otherCycle;

        if (other)
        {
            //	  if (other!= Object())
            {
                REAL    dist  = hit.driveDistance;
                int otherSide = hit.lr < 0 ? 0 : 1;
                for (int i=1; i>=0; i--)
                {
                    loopData[i].closedIn.SetLen(0);
                    loopData[i].loop = false;

                    int lr   = i+i-1;
                    int dir  = hit.lr * lr > 0 ? 1 : 0;
                    int winding = 0;
                    bool loop = CheckLoop(cycle, other,
                                          dist, otherSide, dir,
                                          loopData[i].closedIn, winding);

                    if (loop)
                    {
                        // complete the winding calculation: the target winding
                        // is the one of this cycle:
                        winding += cycle->WindingNumber();
                        // and the source is the winding of the wall we hit
                        winding -= hit.windingNumber;

                        //		      if (dir == 0)
                        //			winding -= lr * (Object()->Grid()->WindingNumber() >> 1);

                        winding += lr;

#ifdef DEBUG_X
                        if (winding != 4 && winding != -4)
                        {
                            gRandomController noRandom;
                            if (winding != 0)
                                //			st_Breakpoint();

                                winding = 0;
                            CheckLoop(cycle, other,
                                      dist, otherSide, dir,
                                      loopData[i].closedIn, winding);

                            winding += cycle->WindingNumber();
                            winding -= hit.windingNumber;
                            winding += lr;
                        }
#endif

#ifdef DEBUG
                        //		      con << "winding = " << winding << " ,direction = " << lr << "\n";
#endif
                        // if the winding continues the direction we would turn in,
                        // we're trapped.
                        if (winding * lr > 0)
                            loopData[i].loop = true;

                        loopData[i].winding = winding;
                    }
                }
            }
        }
    }

    gAISensor(const gCycle* c,
              const eCoord& start, const eCoord& dir,
              REAL sideScan, // the amout of space we should scan left and right
              REAL frontScan, // the total front scanning range
              REAL corridorScan, // the corridor scanning range
              int winding        // direction relative to the cycle's driving direction
             )
            :cycle(c), distance(frontScan*2)
    {
        gAIPlayer* ai = dynamic_cast<gAIPlayer*>(c->Player());
        tASSERT(ai);
        gAICharacter* character = ai->Character();
        tASSERT(character);

        hit = false;

        eDebugLine::SetTimeout(.5);

        gCycle* cycle = const_cast<gCycle*>( this->cycle );

        // detect straight ahead
        gSensor ahead(cycle, start, dir);

        int count = 0;

        do{
            ahead.detect(frontScan);
            front.AddHit(start, dir, ahead, winding);
            if (front.Hit())
            {
                hit = true;
                distance = front.distance;
                if (character->properties[AI_LOOP] > 3 + fabsf(winding) * 3)
                    DetectLoop(front, frontLoop);
            }
        } while (!front.Hit() && count++ < character->properties[AI_RANGE]);

        // adapt the corridor distance so the corridor is not looked for too far away
        if (distance*.99f < corridorScan)
            corridorScan = distance * .99f;
        corridorScan -= sideScan * .02;
        if (corridorScan < 0.1f)
            corridorScan = 0.1f;

        if (Random() * 10 < character->properties[AI_TUNNEL])
        {
            // check the sides
            eCoord lookTunnel = start + dir * corridorScan;

            int i;

            for (i = 1; i>=0; i--)
            {
                int lr       = i+i-1;
                eCoord lrDir = dir.Turn(eCoord(.01f, -lr));

                gSensor side(cycle, start, lrDir);
                side.detect(sideScan*1.01f);
                REAL thisSideScan = side.hit*.99f;

                gSensor tunnel(cycle, lookTunnel, lrDir);
                tunnel.detect(thisSideScan);
                sides[i].AddHit(start, dir, tunnel, winding + lr);

                gSensor parallel(cycle, start + lrDir * thisSideScan, dir);
                parallel.detect(corridorScan);
                sides[i].AddHit(start, dir, parallel, winding);

                if (sides[i].Hit())
                {
                    if (character->properties[AI_LOOP] > 6 + fabsf(winding) * 3)
                        DetectLoop(sides[i], sideLoop[i]);

                    if (sideLoop[i][1-i].loop ||
                            (character->properties[AI_TUNNEL] >= 10 &&
                             sides[i].otherCycle &&
                             sides[i].otherCycle->Team() != cycle->Team() &&
                             sides[i].lr * (i+i-1) < 0 &&
                             sides[i].otherCycle->GetDistance() < sides[i].driveDistance + sides[i].otherCycle->Speed() * 20)
                       )
                    {
                        if (sides[i].distance < distance)
                            distance = sides[i].distance;

                        hit = true;
                    }
                }

#ifdef DEBUG_X
                {
                    gRandomController noRandom;
                    gSensor ahead(cycle, start, dir);
                    ahead.detect(frontScan);

                    gSensor side(cycle, start, lrDir);
                    side.detect(sideScan*1.01f);
                    REAL thisSideScan = side.hit*.99f;

                    gSensor tunnel(cycle, lookTunnel, lrDir);
                    tunnel.detect(thisSideScan);
                    //	  sides[i].AddHit(start, dir, tunnel, winding + lr);

                    gSensor parallel(cycle, start + lrDir * thisSideScan, dir);
                    parallel.detect(corridorScan);
                    // sides[i].AddHit(start, dir, parallel, winding);
                }
#endif

            }
        }
        if (sides[1].otherCycle == sides[0].otherCycle && sides[0].otherCycle)
            hit = true;

#ifdef DEBUGLINE
        if (Hit())
        {
            eDebugLine::SetColor  (1, 1, 1);
            eDebugLine::SetTimeout(1);
            eDebugLine::Draw(start, .5, start + dir*distance, .5);

            eDebugLine::SetColor  (1, .5, 1);
            eDebugLine::Draw(start + dir*distance, .5, start + dir*distance, 1.5);
        }
#endif

        eDebugLine::SetTimeout(0);

    }

};

#endif // OBSOLETES






gCycleMemoryEntry* gCycleMemory::Latest (int side)  const
{
    side = (side > 0 ? 1 : 0);
    gCycleMemoryEntry* ret = NULL;
    for (int i=memory.Len()-1; i>=0; i--)
    {
        gCycleMemoryEntry* m = memory(i);
        if ((!ret || ( (m->max[side].dist > ret->max[side].dist)
                       && bool( m->cycle ) && m->cycle->Alive() ) ) )
            ret = memory(i);
    }

    return ret;
}

gCycleMemoryEntry* gCycleMemory::Earliest (int side)  const
{
    side = (side > 0 ? 1 : 0);
    gCycleMemoryEntry* ret = NULL;
    for (int i=memory.Len()-1; i>=0; i--)
    {
        gCycleMemoryEntry* m = memory(i);
        if ((!ret || ( (m->min[side].dist < ret->min[side].dist)
                       && bool( m->cycle ) && m->cycle->Alive() ) ) )
            ret = memory(i);
    }
    return ret;
}


gCycleMemoryEntry* gCycleMemory::Remember(const gCycle *cycle)
{
    for (int i=memory.Len()-1; i>=0; i--)
        if (memory(i)->cycle == cycle)
            return memory(i);

    return tNEW(gCycleMemoryEntry)(this, cycle);
}

gCycleMemory::gCycleMemory()
{
}


gCycleMemory::~gCycleMemory()
{
    Clear();
}

void gCycleMemory::Clear()
{
    for (int i = memory.Len()-1; i>=0; i--)
        delete memory(i);
}

gCycleMemoryEntry* gCycleMemory::operator()(int i) const
{
    tASSERT(0 <= i && i < Len());
    return memory(i);
}


// deeper analysis functions:

// helper function for gAIPlayer::CycleBlocksWay()
static void CycleBlocksWayHelper(const gCycle *a, const gCycle *b,
                                 int aDir, int bDir, REAL aDist, REAL bDist, int winding)
{
    gCycleMemoryEntry* aEntry = (const_cast<gCycleMemory&>(a->memory)).Remember(b);

    if (aDist > aEntry->max[aDir].dist)
    {
        aEntry->max[aDir].dist      = aDist;
        aEntry->max[aDir].otherSide = bDir;
        aEntry->max[aDir].otherDist = bDist;
        aEntry->max[aDir].winding   = winding;
    }

    if (aDist < aEntry->min[aDir].dist)
    {
        aEntry->min[aDir].dist      = aDist;
        aEntry->min[aDir].otherSide = bDir;
        aEntry->min[aDir].otherDist = bDist;
        aEntry->min[aDir].winding   = winding;
    }
}

// *******************
// * Tracing state   *
// *******************

class gStateTrace: public gAIPlayer::State
{
public:
    gStateTrace( gAIPlayer & ai, int dir ): gAIPlayer::State( ai ), dir_( dir ){}

    // evaluator for tracing
    class Evaluator: public gAINavigator::PathEvaluator
    {
    public:
        Evaluator( gCycle & cycle, int dir ): cycle_( cycle ), dir_( dir ){}

        virtual void Evaluate( gAINavigator::Path const & path, gAINavigator::PathEvaluation & evaluation ) const
        {
            gAINavigator::WallHug const * hug = 0, * noHug = 0;
            if( dir_ > 0 )
            {
                hug   = &path.left;
                noHug = &path.right;
            }
            else
            {
                hug   = &path.right;
                noHug = &path.left;
            }

            bool good = hug->owner && hug->owner->Team() != cycle_.Team();
            if( good )
            {
                if( hug->owner != noHug->owner )
                {
                    evaluation.score = 50;
                }
                else if( ( path.shortTermDirection - path.longTermDirection ).NormSquared() < .01 )
                {
                    evaluation.score = 50 + .5*Adjust( path.immediateDistance/(cycle_.Speed()*cycle_.GetTurnDelay() ) );
                }
            }
        }
    private:
        gCycle & cycle_;
        int dir_;
    };
    
    virtual REAL Think( REAL maxStep )
    { 
        Navigator().UpdatePaths();
        gAINavigator::EvaluationManager manager( Navigator().GetPaths() );
        manager.Evaluate( gAINavigator::SuicideEvaluator( *Parent().Object() ), 1 );
        manager.Reset();
        manager.Evaluate( Evaluator( *Parent().Object(), dir_ ), 1 );
        manager.Evaluate( gAINavigator::SpaceEvaluator( *Parent().Object() ), .5 );
        manager.Evaluate( gAINavigator::PlanEvaluator(), .1 );

        gAINavigator::CycleControllerBasic controller;
        return manager.Finish( controller, *Parent().Object(), maxStep );
    }
private:
    int dir_;
};

// *************************
// * following a target    *
// *************************

// evaluator for zone defense
class gFollowEvaluator: public gAINavigator::PathEvaluator
{
public:
    gFollowEvaluator( gCycle const & cycle ): cycle_( cycle ), blocker_( 0 )
    {
    }

    void SetTarget( eCoord const & target )
    {
        // determine direction to center
        toTarget_ = target - cycle_.Position();

        // normalize direction
        REAL typicalLen = cycle_.Speed() * cycle_.GetTurnDelay();
        toTarget_ *= 1/typicalLen;
        REAL toTargetLen = toTarget_.Norm();
        if ( toTargetLen > 1 )
        {
            toTarget_ *= 1/toTargetLen;
        }

        return;

        // junk code. makes more trouble than it's worth.

        // check whether the path is blocked
        gSensor sensor( &cycle_, cycle_.Position(), toTarget_ );
        sensor.detect( .99 );

        if( sensor.type != gSENSOR_NONE )
        {
            gPlayerWall const * w = dynamic_cast< gPlayerWall const * >( sensor.ehit->GetWall() );
            if ( w )
            {
                blocker_ = w->Cycle();

                // if we block our own recent path, turn around
                if( blocker_ == &cycle_ && w->Pos(0) > cycle_.GetDistance() - cycle_.ThisWallsLength()/2 )
                {
                    int windingDifference = (cycle_.WindingNumber() - w->WindingNumber())*2;
                    int fullTurn = cycle_.Grid()->WindingNumber();
                    if( windingDifference >= fullTurn || windingDifference <= -fullTurn )
                    {
                        toTarget_ = cycle_.Direction().Turn( -1, windingDifference/2 );
                    }
                }
            }
        }

    }

    virtual void Evaluate( gAINavigator::Path const & path, gAINavigator::PathEvaluation & evaluation ) const
    {
        eCoord pathDir = path.longTermDirection + path.shortTermDirection*.5;
        evaluation.score = 50 + 50 * eCoord::F( toTarget_, pathDir );
    }
protected:
    gCycle const & cycle_; //!< the owning cycle
    gCycle * blocker_;     //!< other cycle blocking the path to the target
    eCoord toTarget_;      //!< direction to target, roughly normalized
};

// ********************
// * tailchase helper *
// ********************
   
// likes to chase its own tail
class gTailChaseEvaluator: public gAINavigator::PathEvaluator
{
public:
    gTailChaseEvaluator( gCycle const & cycle ): cycle_( cycle )
    {
    }

    void Evaluate( gAINavigator::Path const & path, gAINavigator::PathEvaluation & evaluation ) const
    {
        evaluation.score = 0;
        
        // don't do anything if we're tunneling. Danger affot.
        // if( path.left.owner == path.right.owner )
        // {
        // return;
        // }

        // total wall length
        REAL len  = cycle_.ThisWallsLength();
        if( len < 0 )
        {
            return;
        }

        if( path.left.owner == &cycle_ ) // && path.left.lr == 1 )
        {
            evaluation.score += 200 * path.left.hitDistance/len - 100;
        }
        if( path.right.owner == &cycle_ ) // && path.right.lr == -1 )
        {
            evaluation.score +=  200 * path.right.hitDistance/len - 100;
        }
        if ( evaluation.score < 0 )
        {
            evaluation.score = 0;
        }
    }
private:
    gCycle const & cycle_; //!< the owning cycle
    
};

// *************************
// * Zone attack/defense   *
// *************************

// evaluator for zone defense
class gZoneEvaluator: public gFollowEvaluator
{
public:
    gZoneEvaluator( gCycle const & cycle, zZone const & zone ): gFollowEvaluator( cycle )
    {
        Init( cycle, zone );
    }

    void Init( gCycle const & cycle, zZone const & zone )
    {
        zShape * shape = zone.getShape();
        if ( !shape )
        {
            return;
        }

        eCoord pos    = cycle_.Position();
        eCoord center = shape->findCenter();

        // REAL insideness = (pos-center).NormSquared()/(edge-center).NormSquared();

        eCoord tailToChase;

        if( zone.Team() == cycle.Team() )
        {
            gCycle::WallInfo info;
            cycle.FillWallInfo( info, .5 );
            tailToChase = info.tailPos;
            
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
                    eCoord tailToChase2 = center + .9 * ( edge - center ).Turn( cosf(angle), sinf( angle ) );
                    tailToChase = tailToChase * ( 1 - blend ) + tailToChase2 * blend;
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

        SetTarget( tailToChase );

    }
private:
};
   

// *******************
// * Survival state  *
// *******************

class gStateSurvive: public gAIPlayer::State
{
public:
    gStateSurvive( gAIPlayer & ai ): gAIPlayer::State( ai ){}

    virtual REAL Think( REAL maxStep )
    { 
        gCycle & cycle = *Parent().Object();
        Navigator().UpdatePaths();
        gAINavigator::EvaluationManager manager( Navigator().GetPaths() );
        manager.Evaluate( gAINavigator::SuicideEvaluator( cycle ), 1 );
        manager.Evaluate( gAINavigator::SuicideEvaluator( cycle, maxStep ), 1 );
        manager.Reset();
        manager.Evaluate( gAINavigator::CowardEvaluator( cycle ), 5 );
        manager.Evaluate( gAINavigator::SpaceEvaluator( cycle ), 1 );
        manager.Evaluate( gAINavigator::RandomEvaluator(), .01 );
        manager.Evaluate( gAINavigator::PlanEvaluator(), .1 );
        manager.Evaluate( gTailChaseEvaluator( cycle ), 1 );

        zZone const * zoneTarget = dynamic_cast< zZone const * >( Parent().GetTarget() );
        if ( zoneTarget )
        {
            manager.Evaluate( gZoneEvaluator( cycle, *zoneTarget ), gAINavigator::EvaluationManager::BLEND_ADD, 1 );
        }

        gAINavigator::CycleControllerBasic controller;
        return manager.Finish( controller, *Parent().Object(), maxStep );
    }
};

void gAIPlayer::SetTarget( eNetGameObject * target )
{
    this->target_ = target;

    // let's see how well this works:
    // state = AI_CLOSECOMBAT;

    // start with it right now
    nextTime_ = -1;
}

// called whenever cylce a drives close to the wall of cylce b.
// directions: aDir tells whether the wall is to the left (-1) or right(1)
// of a
// bDir tells the direction the wall of b is going (-1: to the left, 1:...)
// bDist is the distance of b's wall to its start.
void gAIPlayer::CycleBlocksWay(const gCycleMovement *aa, const gCycleMovement *bb,
                               int aDir, int bDir, REAL bDist, int winding)
{

    return;


    tASSERT(aa && bb);
    gCycle * a = dynamic_cast< gCycle * >( const_cast< gCycleMovement * > ( aa ) );
    gCycle * b = dynamic_cast< gCycle * >( const_cast< gCycleMovement * > ( bb ) );

    REAL aDist = a->GetDistance();
    aDir = (aDir > 0 ? 1 : 0);
    bDir = (bDir > 0 ? 1 : 0);

    int w = winding + aDir * 2 + bDir * 2;
    while (w < 0)
        w+=400;
    if (w % 4 != 2)
        return;

    CycleBlocksWayHelper(a,b,aDir,bDir,aDist,bDist,  winding);
    CycleBlocksWayHelper(b,a,bDir,aDir,bDist,aDist, -winding);

    // what to do if cylce a tries to trace cylce b?
    if (a->Team() != b->Team())
    {
        gAIPlayer* ai = dynamic_cast<gAIPlayer*>(b->Player());
        if( ai && dynamic_cast< zZone const * >( ai->GetTarget() ) )
        {
            // ai is attacking or defending a zone, don't disturb it
            return;
        }

        if (ai && ai->Character() && ai->Character()->properties[AI_DETECTTRACE] > 5 )
            if(aDir != bDir )
            {
                REAL behind = b->GetDistance() - bDist;
                if (a->Speed() > b->Speed() * 1.2f && behind < (a->Speed() - b->Speed()) * 10)
                { // a is faster. Try to escape.
                    ai->SwitchToState( tNEW(gStateTrace)( *ai, aDir > 0 ? 1 : -1 ) );
                    ai->target_ = const_cast< gCycle * >( a );
                }
                else// if (a->Speed() < b->Speed() * 1.1f)
                { // b is faster. Attack.
                    ai->SwitchToState( tNEW(gStateTrace)( *ai, aDir > 0 ? -1 : 1 ) );
                    ai->target_ = const_cast< gCycle * >( a );
                }
            }

        // what to do if the AI player traces his opponent by accident? Trace On!
        ai = dynamic_cast<gAIPlayer*>(a->Player());

        if( ai && dynamic_cast< zZone const * >( ai->GetTarget() ) )
        {
            // ai is attacking or defending a zone, don't disturb it
            return;
        }

        if (ai && ai->Character() && ai->Character()->properties[AI_DETECTTRACE] > 0)
            if(aDir != bDir )
            {
                ai->SwitchToState( tNEW(gStateTrace)( *ai, aDir > 0 ? 1 : -1 ) );
                ai->target_ = const_cast< gCycle * >( b );
            }
    }
}

// called whenever a cylce blocks the rim wall.
void gAIPlayer::CycleBlocksRim(const gCycleMovement *a, int aDir)
{
}

// called whenever a hole is ripped in a's wall at distance aDist.
void gAIPlayer::BreakWall(const gCycleMovement *a, REAL aDist)
{}








//#define DEBUG

// which eWall is detected?

#define MAXAI_COLOR 13

static int current_ai;
static REAL rgb_ai[MAXAI_COLOR][3]={
                                       {1,.2,.2},
                                       {.2,1,.2},
                                       {.2,.2,1},
                                       {1,1,.2},
                                       {1,.2,1},
                                       {.2,1,1},
                                       {1,.6,.2},
                                       {1,.2,.6},
                                       {.6,.2,1},
                                       {.2,.6,1},
                                       {1,1,1},
                                       {.2,.2,.2},
                                       {.5,.5,.5}
                                   };

static nNetObjectDescriptor<gAIPlayer,Game::AIPlayerSync> gAIPlayer_init(330);

nNetObjectDescriptorBase const & gAIPlayer::DoGetDescriptor() const
{
    return gAIPlayer_init;
}

//! creates a netobject form sync data
gAIPlayer::gAIPlayer( Game::AIPlayerSync const & sync, nSenderInfo const & sender ):
        ePlayerNetID(sync.base(), sender ),
        character_(NULL),
        lastTime_(se_GameTime()),
        nextTime_(0),
        concentration_(1)
{
}


gAIPlayer::gAIPlayer():
        simpleAI_(NULL),
        character_(NULL),
        //	target(NULL),
        lastTime_(se_GameTime()),
        nextTime_(0),
        concentration_(1)
{
    ClearTarget();

    // find a good color
    current_ai=(current_ai+1) % MAXAI_COLOR;
    int take_ai=current_ai;
    int try_ai=current_ai;

    REAL maxmindist=-10000;

    for(int i=MAXAI_COLOR-1;i>=0;i--){
        REAL mindist=4;
        REAL score=0;
        for (int j=se_PlayerNetIDs.Len()-1;j>=-1;j--){
            REAL R, G, B; // the color we want to avoid

            if (j >= 0)
            {
                ePlayerNetID *p=se_PlayerNetIDs(j);
                if (p && p != this)
                {
                    R = p->color.r_/15.0;
                    G = p->color.g_/15.0;
                    B = p->color.b_/15.0;
                }
                else
                    continue;
            }
            else // last case: j = -1. Test against floor color
            {
                se_FloorColor(R, G, B);
            }
            REAL dist=
                fabs(R - rgb_ai[try_ai][0])+
                fabs(G - rgb_ai[try_ai][1])+
                fabs(B - rgb_ai[try_ai][2]);

            score+=exp(-dist*dist*4);

            if (dist<mindist){
                mindist=dist;
                /*	   con << c->r << ":" << rgb_ai[try_ai][0] << '\t'
                	   << c->g << ":" << rgb_ai[try_ai][1] << '\t'
                	   << c->b << ":" << rgb_ai[try_ai][2] << '\t' << dist << '\n'; */
            }
        }
        //con << "md=" << mindist << "\n\n";
        if (mindist>2)
            mindist=2;

        mindist=-score;

        if (mindist>maxmindist){
            maxmindist=mindist;
            take_ai=try_ai;
        }



        try_ai = ((try_ai+1) % MAXAI_COLOR);
    }


    color.r_ = static_cast<int>(rgb_ai[take_ai][0] * 15);
    color.g_ = static_cast<int>(rgb_ai[take_ai][1] * 15);
    color.b_ = static_cast<int>(rgb_ai[take_ai][2] * 15);


    ping = 0;
    pingCharity = 300;

    NewObject();
}

void gAIPlayer::ConfigureAIs()  // ai configuration menu
{

}


// make sure this many AI players are in the game
void gAIPlayer::SetNumberOfAIs(int num, int minPlayers, int iq, int tries)
{
    // balance the human teams with AI players
    gAITeam::BalanceWithAIs();

    // remove AI players that got kicked out of their team
    for (int i = se_PlayerNetIDs.Len()-1; i>=0; i--)
    {
        // count the active AIs
        ePlayerNetID *p = se_PlayerNetIDs(i);
        gAIPlayer  *ai  = dynamic_cast<gAIPlayer*>(p);
        if ( ai && !bool( ai->NextTeam() ) )
        {
            ai->RemoveFromGame();

#ifdef DEBUG
            if ( ai->GetRefcount() > 1 )
            {
                st_Breakpoint();
            }
#endif

            sg_AIReferences.Remove( ai );
        }
    }

    int count = 0;

    bool iqperfect = false;

    // repeat until we run out of tries or the total amount of AIs is correct
    do
    {
        count = 0;

        int i;
        gAIPlayer* worstIQ = NULL; // worst fitting AI player that is in the game

        //		static tArray<bool> inGame(gAICharacter::s_Characters.Len());
        //		for (i = gAICharacter::s_Characters.Len()-1; i>=0; i--)
        //			inGame(i) = false;

        for (i = se_PlayerNetIDs.Len()-1; i>=0; i--)
        {
            // count the active AIs
            ePlayerNetID *p = se_PlayerNetIDs(i);
            gAIPlayer  *ai  = dynamic_cast<gAIPlayer*>(p);
            if (ai && ai->NextTeam() == sg_AITeam )
            {
                //				int index = ((int)ai->character - (int)&gAICharacter::s_Characters(0))/sizeof(gAICharacter);
                //				inGame(index) = true;

                if (!worstIQ || !worstIQ->character_ || fabs(worstIQ->character_->iq - iq) < fabs(ai->character_->iq - iq) )
                    worstIQ = ai;

                count++;
            }
        }

        gAICharacter* bestIQ = BestIQ( iq );

        // count = numberOfAIs to add / remove(numberofAIs - requestedNumAIs)
        count -= num;

        // pcount = numberPlayers above minPlayers
        int pcount = - minPlayers;
        // check if more AIs are required (because there are less than minPlayers players)
        for (i = se_PlayerNetIDs.Len()-1; i>=0; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            if ( p->CurrentTeam() )
                ++pcount;
        }

        if (pcount < count)
            count = pcount;

        iqperfect = true;
        if (bestIQ && worstIQ && worstIQ->character_ )
            iqperfect = (fabs(bestIQ->iq - iq) > fabs(worstIQ->character_->iq - iq) * .9f);


        // count complete. Do something!
        if (worstIQ && (count > 0 || (count >= 0 && !iqperfect)))
        {
            // too many AIs. Delete the one with the least fitting intelligence.
            worstIQ->RemoveFromGame();
            //			worstIQ->ReleaseOwnership();

#ifdef DEBUG
            if ( worstIQ->GetRefcount() > 1 )
            {
                st_Breakpoint();
            }
#endif

            sg_AIReferences.Remove( worstIQ );
            count--;

            // remove empty AI team
            if ( 0 == AITeam()->NumPlayers() )
            {
                ClearAITeam();
            }
        }
        if (bestIQ && count < 0)
        {
            // too litte AIs. Create one.
            gAIPlayer *ai = tNEW(gAIPlayer)();
            ai->SetName( bestIQ->name );
            ai->character_ = bestIQ;

            sg_AIReferences.Add( ai );

            ai->SetTeam ( AITeam() );
            ai->UpdateTeam();

            /*
            {
                tOutput mess;
                tColoredString printname;
                printname << *(ePlayerNetID*)ai << tColoredString::ColorString(.5,1,.5);

                mess.SetTemplateParameter(1, printname);
                mess << "$player_entered_game";

                sn_ConsoleOut( mess );
            }
            */

            count++;
        }

    }
    while ((count != 0 ||
            !iqperfect) &&
            tries-- != 0);

}


// state change:
void gAIPlayer::SwitchToState( State * state )
{
    state_           = state;
}

/*

void gAIPlayer::ThinkCloseCombat( ThinkData & data )
{
    int lr=0;

    REAL nextThought = 0;

    const gAISensor* sides[2];
    sides[0] = &data.left;
    sides[1] = &data.right;

    eCoord dir = Object()->Direction();
    REAL fs=data.front.distance;
    //  REAL ls=left.hit;
    //  REAL rs=right.hit;

    zZone const * zoneTarget = dynamic_cast< zZone const * >( GetTarget() );
    if ( bool( target ) && !zoneTarget && !IsTrapped(target, Object()) && nextStateChange < se_GameTime() )
    {
        gSensor p(Object(),Object()->Position(),target->Position() - Object()->Position());
        p.detect(REAL(1));
        if (p.hit <=  .999999)  // no free line of sight to victim. Switch to path mode.
        {
            SwitchToState(AI_PATH, 5);
            EmergencySurvive( data );

            return;
        }
    }

    REAL ed = 0;

    const REAL fear=REAL(.01);
    const REAL caution=.001;
    const REAL evasive=100;
    const REAL attack=100;
    const REAL seek=REAL(1);
    const REAL trap=REAL(.01);
    const REAL ffar=20;
    //    const REAL close=1000;

    REAL random=10*Random()*Random();

    if ( bool( target ) && target->Alive()){

        eCoord enemypos=target->Position()-Object()->Position();
        eCoord enemydir=target->Direction();
        REAL enemyspeed=target->Speed();

        ed=REAL(fabs(enemypos.x)+fabs(enemypos.y));
        ed/=(enemyspeed+Object()->Speed());

        // transform coordinates relative to us:
        enemypos=enemypos.Turn(dir.Conj()).Turn(0,1);
        enemydir=enemydir.Turn(dir.Conj()).Turn(0,1);

        // now we are at the center of the coordinate system facing
        // in direction (0,1).

        // rules are symmetrical: exploit that.
        int side=1;
        if (enemypos.x<0){
            side*=-1;
            enemypos.x*=-1;
            enemydir.x*=-1;

            sides[1] = &data.left;
            sides[0] = &data.right;
            //      Swap(ls,rs);
        }
        // now we can even assume the enemy is on our right side.

        if ( zoneTarget && idler.get() )
        {
            if ( enemypos.y < 0 || ( data.front.front.wallType != gSENSOR_SELF && data.front.front.distance < Object()->Speed() * Object()->GetTurnDelay() ) )
            {
                gAINavigator::Wish wish( *idler );
                wish.turn = -side;

                // extra strong turn wish if wall is not mine
                if( sides[1]->front.wallType != gSENSOR_SELF &&  sides[0]->front.wallType != gSENSOR_SELF )
                {
                    wish.turn *= 2;
                }

                if ( enemypos.x > -enemypos.y * 2 )
                {
                    wish.minDistance = Object()->ThisWallsLength()/2;
                }
                data.thinkAgain = idler->Activate( Object()->LastTime(), 0, 0, &wish );
            }
            else
            {
                data.thinkAgain = idler->Activate( Object()->LastTime(), 0, 0 );
            }
            return;
        }

        // consider his ping and our reaction time
#define REACTION .2


        //REAL enemyspeed=target->speed;
        REAL ourspeed=Object()->Speed();

        REAL enemydist=target->Lag()*enemyspeed;

        // redo the prediction
#ifndef DEDICATED
        if (sn_GetNetState()==nCLIENT && !sr_predictObjects)
#endif
            enemypos=enemypos-enemydir*enemydist;
        enemydist+=2*REACTION *enemyspeed;

        REAL ourdist=REACTION*ourspeed;;

        

        // now we consider the worst case: we drive straight on,
        enemypos.y-=ourdist;
        // while the enemy cuts us: he goes in front of us
        REAL forward=-enemypos.y+.01;
        if (forward<0) // no need to go to much ahead
            forward=0;
        if (forward>enemydist)
            forward=enemydist;

        enemypos.y+=forward;
        enemydist-=forward;

        // and then he turns left.
        enemypos.x-=enemydist;

        if (enemypos.y*enemyspeed>enemypos.x*ourspeed){ // he is right ahead of us.
            if (random<fear){ // evade him
#ifdef DEBUG
                con << "fear!\n";
#endif
                lr+=side;

                nextThought += 1;
            }
            if (enemypos.y<=ffar &&
                    ((enemydir.x<0 && random<evasive) ||
                     (enemydir.y>0 && random<caution) ||
                     (enemydir.y<0 && random<attack))){
#ifdef DEBUG
                con << "caution!\n";
#endif
                lr+=side;

                nextThought += 1;

                if (enemyspeed > ourspeed)
                {
                    SetTraceSide(-side);
                    SwitchToState(AI_TRACE, 10);
                }
            }
        }
        else if (enemypos.y*ourspeed<-enemypos.x*enemyspeed){

            REAL canCutIfDriveOn = enemypos.x*ourspeed - fs * (enemyspeed - ourspeed);
            canCutIfDriveOn -= enemypos.y*enemyspeed;

            REAL canCutIfAttack  = - sides[1]->distance * enemyspeed
                                   - (sides[1]->distance - enemypos.x -enemypos.y*ourspeed) * ourspeed;

            if (random<attack && (!(data.front.Hit() && data.front.distance < 20) || canCutIfAttack > canCutIfDriveOn)){
#ifdef DEBUG
                con << "attack!\n";
#endif
                lr-=side;

                nextThought += 1;
            }
        }
        else if(enemypos.x>ffar*4){
            if(random<seek){
#ifdef DEBUG
                con << "seek!\n";
#endif
                lr-=side;

                nextThought += 1;
            }
        }
        else if (enemypos.x<ffar*2 && fabs(enemypos.y)<ffar){
            if(random<trap){
#ifdef DEBUG
                con << "trap!\n";
#endif
                lr+=side;

                nextThought += 1;
            }
        }
    }

    if (!EmergencySurvive(data, 1, -lr))
        nextThought = 0;

    data.thinkAgain = ed + nextThought;
}
*/

// **********
// * States *
// **********

//!@param player   the AI player the state belongs to
gAIPlayer::State::State( gAIPlayer & player )
: parent_( player )
{
}

gAIPlayer::State::~State()
{
}

//!@return time in seconds until a new thought is needed
REAL gAIPlayer::State::Think( REAL maxStep )
{
    tASSERT(0);
    return 0;
}

//!@return the navigator to use
gAINavigator & gAIPlayer::State::Navigator()
{
    tASSERT( Parent().navigator_.get() );
    return *(Parent().navigator_.get());
}

//!@return the navigator to use
gAINavigator const & gAIPlayer::State::Navigator() const
{
    tASSERT( Parent().navigator_.get() );
    return *( Parent().navigator_.get() );
}

//!@return the character
gAICharacter const & gAIPlayer::State::Character() const
{
    tASSERT( Parent().character_ );
    return *Parent().character_;
}

gAIPlayer::StateGrind::StateGrind( gAIPlayer & player )
: State( player )
{
}

gAIPlayer::StateGrind::~StateGrind(){}

REAL gAIPlayer::StateGrind::Think( REAL maxStep )
{
    /*

    REAL range = Delay() * Object()->Speed() * .5;

    if( data.front.front.wallType == gSENSOR_TEAMMATE )
    {
        if ( substate == AI_GRIND_GRIND )
        {
            if ( data.front.front.distance > range * .1 )
            {
                data.thinkAgain = data.front.front.distance/Object()->Speed();
                return;
            }
            else
            {
                data.turn = data.front.front.lr;
                data.thinkAgain = Object()->GetTurnDelay();
                return;
            }
        }
        else
        {
            data.turn = data.front.front.lr;
            data.thinkAgain = Object()->GetTurnDelay();
            state = AI_SURVIVE;
            return;
        }
    }

    if( data.left.front.wallType == gSENSOR_TEAMMATE )
    {
        if ( data.left.front.distance > range )
        {
            substate = AI_GRIND_GRIND;
            data.turn = -1;
            data.thinkAgain = data.left.front.distance/Object()->Speed();
            return;
        }
        else if ( state == AI_GRIND )
        {
            substate = AI_GRIND_SPLIT_RIGHT;
            return;
        }
    }

    if( data.right.front.wallType == gSENSOR_TEAMMATE )
    {
        if ( data.right.front.distance > range )
        {
            substate = AI_GRIND_GRIND;
            data.turn = 1;
            data.thinkAgain = data.right.front.distance/Object()->Speed();
            return;
        }
        else if ( state == AI_GRIND )
        {
            substate = AI_GRIND_SPLIT_LEFT;
            return;
        }
    }

    // SPLIT!
    if( substate == AI_GRIND_SPLIT_LEFT )
    { 
        if ( data.right.front.wallType != gSENSOR_TEAMMATE )
        {
            data.thinkAgain = data.left.front.distance/Object()->Speed();
            data.turn = -1;
        }
        else
        {
            data.thinkAgain = Delay() * 2;
            return;
        }
    }

    if( substate == AI_GRIND_SPLIT_RIGHT )
    {
        if( data.left.front.wallType != gSENSOR_TEAMMATE )
        {
            data.thinkAgain = data.right.front.distance/Object()->Speed();
            data.turn = 1;
        }
        else
        {
            data.thinkAgain = Delay() * 2;
            return;
        }
    }

    SwitchToState( AI_SURVIVE );
    data.thinkAgain = .5;

    */

    return 0;
}

void gAIPlayer::RightBeforeDeath(int triesLeft) // is called right before the vehicle gets destroyed.
{
    if ( nCLIENT == sn_GetNetState() )
        return;

    if ( simpleAI_ )
        return;

    if ( !character_ )
    {
        st_Breakpoint();
        return;
    }

    CreateNavigator();

    gRandomController random( randomizer_ );

    // think again immediately after this
    nextTime_ = lastTime_;

#ifdef DEBUG_X
    if (log && !Object()->Alive())
    {
        log->Print();
        //      st_Breakpoint();
        delete log;
        log = NULL;
    }
#endif

    if (!Object()->Alive() || ( character_ && Random() * 10 > character_->properties[AI_EMERGENCY] ) )
        return;

    if( triesLeft <= 0 )
    {
        gAINavigator::SuicideEvaluator::SetEmergency( true );
    }

    Think( 0 );

    gAINavigator::SuicideEvaluator::SetEmergency( false );
}

void gAIPlayer::NewObject()         // called when we control a new object
{
    lastTime_ = 0;
    
    navigator_.reset();

    if ( character_ )
    {
        nextTime_       = character_->properties[AI_STARTSTRAIGHT] * gArena::SizeMultiplier()/gCycleMovement::SpeedMultiplier();
        switch ( character_->properties[AI_STARTSTATE] )
        {
        default:
            state_          = tNEW(gStateSurvive)(*this);
            break;
        }
    }
    else
    {
        nextTime_        = 10;
        state_          = tNEW(gStateSurvive)(*this);
    }

    if( TeamListID() > 0 )
    {
        state_ = tNEW(StateGrind)(*this);
        nextTime_ = Delay() * 2 + .1;
    }

    ClearTarget();
}

/*
static gAISensor * sg_GetSensor( int currentDirectionNumber, gCycle const & object, int turn, REAL side, REAL range, REAL corridor, REAL & mindist )
{
    // determine the current direction
    eGrid & grid = *object.Grid();
    eCoord origDir = grid.GetDirection( currentDirectionNumber );

    // determine sensors
    gAISensor * ret = 0;

    // turn direction
    int direction = currentDirectionNumber;
    int turns = 1;
    grid.Turn( direction, turn );
    eCoord dir = grid.GetDirection( direction );
    while ( !ret || ( turn * ( origDir * dir ) > .01 && currentDirectionNumber != direction ) )
    {
        // cast rays
        gAISensor * sensor = tNEW(gAISensor)(&object,object.Position(),dir, side, range, corridor*.5f, turn );
        if ( !ret || sensor->distance > ret->distance )
        {
            if ( ret )
            {
                // calculate effective distance required to turn around in time
                REAL dist = ret->distance - 1.2 * object.Speed() * Delay() * turns;
                if ( mindist > dist )
                    mindist = dist;
            }

            delete ret;
            ret = sensor;
        }
        else
        {
            delete sensor;
        }

        // go on turning
        grid.Turn( direction, turn );
        dir = grid.GetDirection( direction );
        ++turns;
    }

    return ret;
}
*/

void gAIPlayer::CreateNavigator()
{
    gCycle * cycle = Object();
    if( cycle && !navigator_.get() )
    {
        navigator_ = std::auto_ptr< gAINavigator >( tNEW( gAINavigator( cycle ) ) );
    }
}

REAL gAIPlayer::Think( REAL maxStep ){
    if ( !simpleAI_ )
    {
        gSimpleAIFactory * factory = gSimpleAIFactory::Get();
        if ( factory )
        {
            simpleAI_ = factory->Create( Object() );
        }
    }

    CreateNavigator();

    if ( simpleAI_ )
    {
        return simpleAI_->Think( maxStep );
    }

    if( GetTarget() && ( !GetTarget()->Alive() || IsTrapped( GetTarget(), Object() ) ) )
    {
        // no longer makes sense to chase
        ClearTarget();
    }

    if (!Object()->Alive())
        return 100;

#ifdef DEBUG
    {
        eDebugLine::SetTimeout(.5);
        eDebugLine::SetColor  (0, 1, 0);
        eCoord p = Object()->Position();
        eDebugLine::Draw(p, .5, p, 5.5);
        eDebugLine::SetTimeout(0);

        // to debug specific situations on playback
        static int count = 0;
        count++;
        if( count == 1528 )
        {
            st_Breakpoint();
        }
    }
#endif

    if( state_ )
    {
        return state_->Think( maxStep );
    }

    return 10;
}

void gAIPlayer::Timestep(REAL time){
    if (!character_)
    {
        st_Breakpoint();
        return;
    }

    const REAL relax=25;

    // don't think if the object is not up to date
    if ( Object() && Object()->LastTime() < time - EPS )
        return;

    REAL ts=time-lastTime_;
    lastTime_=time;

    if (concentration_ < 0)
        concentration_ = 0;

    concentration_ += 4*(character_->properties[AI_REACTION]+1) * ts/relax;
    concentration_=concentration_/(1+ts/relax);

    if (bool(Object()) && Object()->Alive() && nextTime_<time){
        gRandomController random( randomizer_ );

        REAL target = 4;
        REAL safethought = ( target/concentration_ -.1 )/4;
        if( safethought > .3 )
        {
            safethought = .3;
        }

        REAL nextthought=Think( safethought );
        //    if (nextthought>.9) nextthought=REAL(.9);

        if (nextthought<REAL(.6-concentration_)) nextthought=REAL(.6-concentration_);

        nextTime_=nextTime_+nextthought;

        //con << concentration_ << "\t" << nextthought << '\t' << ts << '\n';

        if(.1+4*nextthought<1)
            concentration_*=REAL(.1+4*nextthought);
    }
}


void gAIPlayer::Color( REAL&a_r, REAL&a_g, REAL&a_b ) const
{
    ePlayerNetID::Color( a_r, a_g, a_b );
}

gAIPlayer::~gAIPlayer()
{
    target_=NULL;
    ClearObject();
    tCHECK_DEST;
}

void gAIPlayer::ClearAll()
{
    sg_AIReferences.ReleaseAll();

    // remove empty AI team
    if ( 0 == AITeam()->NumPlayers() )
    {
        ClearAITeam();
    }
}
/*
void gAIPlayer::AddRef()
{
	ePlayerNetID::AddRef();
}

void gAIPlayer::Release()
{
	ePlayerNetID::Release();
}
*/
