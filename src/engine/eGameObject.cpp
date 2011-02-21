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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "eGameObject.h"
#include "uInputQueue.h"
#include "eTimer.h"
#include "eTess2.h"
#include "eWall.h"
#include "tConsole.h"
#include "rScreen.h"
#include "eSound.h"
#include "eAdvWall.h"
#include "eGrid.h"
#include "uInput.h"
#include "tMath.h"
#include "nConfig.h"
#include "eTeam.h"

#include <map>

uActionPlayer eGameObject::se_turnRight("CYCLE_TURN_RIGHT", -10);
static uActionTooltip se_turnRightTooltip( eGameObject::se_turnRight, 11, &ePlayer::VetoActiveTooltip );

uActionPlayer eGameObject::se_turnLeft("CYCLE_TURN_LEFT", -10);
static uActionTooltip se_turnLeftTooltip( eGameObject::se_turnLeft, 10, &ePlayer::VetoActiveTooltip );


// entry and deletion in the list of all gameObjects
void eGameObject::AddToList(){
    eSoundLocker locker;

    if ( id < 0 )
        AddRef();

    grid->gameObjectsInactive.Remove(this,inactiveID);
    grid->gameObjects.Add(this,id);
}
void eGameObject::RemoveFromList(){
    eSoundLocker locker;

    int oldID = id;

    currentFace = 0;

    grid->gameObjects.Remove(this,id);
    grid->gameObjectsInactive.Add(this,inactiveID);

    if ( oldID >= 0 )
        Release();
}

void eGameObject::RemoveFromListsAll(){
    eSoundLocker locker;

    int oldID = id;

    currentFace = 0;

    grid->gameObjects.Remove(this,id);
    grid->gameObjectsInactive.Remove(this,inactiveID);
    grid->gameObjectsInteresting.Remove(this,interestingID);

    if ( oldID >= 0 )
        Release();
}

void eGameObject::RemoveFromGame()
{
    tJUST_CONTROLLED_PTR< eGameObject > keepAlive;
    if ( id >= 0 )
        keepAlive = this;

    OnRemoveFromGame();
    DoRemoveFromGame();
}


// called on RemoveFromGame(). Call base class implementation, too, in your implementation.
void eGameObject::OnRemoveFromGame()
{
    // remove from grid
    currentFace = 0;

    // remove from lists
    RemoveFromListsAll();
}


// called on RemoveFromGame(). Do not call base class implementation of this function, don't expect to get called from subclasses.
void eGameObject::DoRemoveFromGame()
{
    // simply delete
    delete this;
}


eGameObject::eGameObject(eGrid *g,const eCoord &p,const eCoord &d,eFace *currentface,bool autodel)
        :autodelete(autodel),pos(p),dir(d),z(0),grid(g){
    tASSERT(g);
    urgentSimulationRequested_=false;
    currentFace=currentface;
    lastTime=se_GameTime();
    id=-1;
    interestingID=-1;
    inactiveID=-1;
    if ( lastTime < 0 )
        lastTime=0;
    team = 0;
}

eGameObject::~eGameObject(){
    currentFace = 0;
    RemoveFromListsAll();
    tCHECK_DEST;
}

// returns the type of this object (important for interaction of
// two gameObjects)
//gameobject_type gameobject::type(){return ArmageTron_GENERIC;}

// makes two gameObjects interact:
void eGameObject::InteractWith(eGameObject *,REAL,int){}

// what happens if we pass eWall w?
void eGameObject::PassEdge(const eWall *w,REAL,REAL,int){
    if (w) Kill();
}

static int se_moveTimeout = 100;
static tSettingItem<int> se_moveTimeoutC("GAMEOBJECT_MOVE_TIMEOUT", se_moveTimeout);

// data structures for storing temp wall collisions
struct eTempEdgePassing
{
    eWall *wall; //!< the wall the object collides with
    REAL ratio;  //!< the location of the collision point on the wall
};
typedef std::multimap< REAL, eTempEdgePassing > eTempEdgeMap;


// moves
void eGameObject::Move( const eCoord &dest, REAL startTime, REAL endTime, bool useTempWalls )
{
#ifdef DEBUG
    grid->Check();
#endif
    if (!finite(dest.x) || !finite(dest.y))
    {
        st_Breakpoint();
        return;
    }

    tStackObject< ePoint > start(pos),stop(dest);
    ePoint* pstart = &start;
    ePoint* pstop = &stop;

    // clip movement to rim walls
    REAL clip = eWallRim::Clip(start,stop,-10);
    endTime = startTime + ( endTime - startTime ) * clip;

    grid->Range(stop.NormSquared());

#ifdef DEBUG
    if (!finite(stop.x) || !finite(stop.y))
    {
        st_Breakpoint();

        static_cast<eCoord&>(stop) = dest;
        eWallRim::Bound(stop,-10);

        return;
    }
#endif

    //  se_GridRange(dest.Norm_squared());
    eTempEdgeMap tempCollisions;

    tStackObject< eTempEdge >  te( pstart, pstop );
    eHalfEdge  &e=*te.Edge(0);

    // check all the currently drawn eWalls:
    if ( useTempWalls )
    {
        for(int i=grid->wallsNotYetInserted.Len()-1;i>=0;i--){
            const eHalfEdge *other_e=grid->wallsNotYetInserted[i]->Edge();
            if (//!sg_netPlayerWalls(i)->Preliminary() &&
                other_e->Point() && other_e->Other() && other_e->Other()->Point()){
                tJUST_CONTROLLED_PTR< ePoint > new_cross_p=e.IntersectWith(other_e);
                if (new_cross_p){
                    REAL e_ratio =e.Ratio(*new_cross_p);
                    REAL o_ratio =other_e->Ratio(*new_cross_p);
                    if (0<=e_ratio && 1>=e_ratio &&
                            0<=o_ratio && 1>=o_ratio)
                    { // find the fall
                        eWall *w = other_e->GetWall();
                        if (!w)
                        {
                            w = other_e->Other()->GetWall();
                            o_ratio = 1-o_ratio;
                        }
                        if (w)
                        {
                            // insert data into map structure for later processing
                            eTempEdgePassing passing;
                            passing.wall = w;
                            passing.ratio = o_ratio;
                            tempCollisions.insert( std::pair< REAL, eTempEdgePassing >( e_ratio, passing) );
                        }
                    }
                }
            }
        }
    }

    // find a replacement face if required
    FindCurrentFace();

    // the total distance to travel
    REAL totalDistance = ( stop - pos ).Norm();

    if (currentFace){
        // start iterator for collisions with temporary walls
        eTempEdgeMap::const_iterator currentTempCollision = tempCollisions.begin();

        int timeout = se_moveTimeout;

        REAL lastDistance = 1E+30; // the distance of pos and stop in the last step
        eHalfEdge *in    = NULL;   // incoming edge to prevent entdless loop

        while (currentFace && timeout >0 && !currentFace->IsInside(stop)){
            // the vector to our destination:
            eCoord vec=stop - pos;

            // count down timeout if we're moving into the wrong direction
            REAL distance = vec.Norm();
            if ( distance >= lastDistance )
            {
                timeout--;
            }
            else
            {
                timeout = se_moveTimeout;
                if ( lastDistance > 1E+29 )
                    lastDistance = distance * 1.1;
                lastDistance = .1 * lastDistance + distance * (.9 - EPS);

                // check if the target has been reached within tolerance; it can only make matters
                // worse then to continue, even if the current face claims we're not part of it.
                if ( distance <= EPS * totalDistance )
                {
                    // st_Breakpoint();
                    break;
                }
            }
#ifdef DEBUG_X
rerun:
#endif

            eHalfEdge *run   = currentFace->Edge(); // runs through all edges of the face
            eHalfEdge *best  = NULL;                // the best face to leave
            eHalfEdge *end   = run;
            REAL bestScore   = -10.0;
            REAL bestERatio  = .5;
            REAL bestRRatio  = .5;
            eCoord bestCross   (0,0);

            // look for the best way out
            do
            {
                run = run->Next();

                if (run == in) // never leave through the edge we entered
                    continue;

                eCoord runVec = run->Vec();

                REAL score = runVec * vec / ( se_EstimatedRangeOfMult( runVec, vec ) + EPS );
                static const REAL smallBias = .01;

                // keep a bit of the score, but not too much. We want to
                // sort out exactly parallel walls here.
                if ( score > smallBias || ( score > 0 && !run->GetWall() ) )
                    score = smallBias;

                eCoord cross = e.IntersectWithCareless(run);

                // project crossing to face edge without score penalty
                REAL run_ratio = run->Ratio(cross);
                if ( !good( run_ratio ) )
                {
                    // score -= 100;
                    run_ratio = .5;
                }

                if (run_ratio < 0)
                {
                    // score += run_ratio;
                    run_ratio = 0;
                }
                else if (run_ratio > 1)
                {
                    // score += (1-run_ratio);
                    run_ratio = 1;
                }
                cross = *run->Point() + run->Vec() * run_ratio;

                // determine how far off the movement edge the modified intersection lies
                REAL e_side  = vec * ( cross - pos ) / distance;
                score -= fabs( e_side );
                // REAL run_side  = runVec * ( cross - *run->Point() ) / runVec.NormSquared();

                REAL e_ratio   = e.Ratio(cross);

                // see whether the intersection is beyond the end points of the movement vector
                if ( !good( e_ratio ) )
                {
                    score -= 100;
                    e_ratio = .5;
                }

                if (e_ratio < 0)
                {
                    score += e_ratio;
                    e_ratio = 0;
                }
                else if (e_ratio > 1)
                {
                    score += (1-e_ratio);
                    e_ratio = 1;
                }

                if (score > bestScore)
                {
                    best       = run;
                    bestScore  = score;
                    bestERatio = e_ratio;
                    bestRRatio = run_ratio;
                    bestCross  = cross;
                }

            }
            while (run != end);

#ifdef DEBUG_X
            if ( !good( bestScore ) || bestScore < -50 )
            {
                st_Breakpoint();
                goto rerun;
            }
#endif

#define TIME( ratio ) ( startTime+(endTime-startTime)*( ratio ) )

            if (best)
            {
                // handle stored temp collisions
                while ( currentTempCollision != tempCollisions.end() && (*currentTempCollision).first < bestERatio )
                {
                    eTempEdgePassing const & passing = (*currentTempCollision).second;
                    PassEdge( passing.wall, TIME( (*currentTempCollision).first ), passing.ratio, 0 );
                    ++ currentTempCollision;
                }

                REAL time=TIME( bestERatio );

                // move to the collision point
                pos = bestCross;

                // leave this face (through a wall)
                eWall*     w     = best->GetWall();
                if (w)
                    PassEdge(w,time,bestRRatio,0);

                // set next incoming edge
                tASSERT(best->Other());
                in          = best->Other();

                // enter the next face (through a wall)
                if (in)
                {
                    bestRRatio = 1-bestRRatio;
                    w = in->GetWall();

                    if (w)
                        PassEdge(w,time,bestRRatio,0);
                }

                // switch to the next face
                if (in)
                    currentFace=in->Face();
                else
                    currentFace=NULL;
            }
            else
            {
                timeout = 0;
            }
        }

        if (timeout <= 0)
            grid->requestCleanup = true;
        else
            pos=stop;

        // handle stored temp collisions
        while ( currentTempCollision != tempCollisions.end() )
        {
            eTempEdgePassing const & passing = (*currentTempCollision).second;
            PassEdge( passing.wall, TIME( (*currentTempCollision).first ), passing.ratio, 0 );
            ++ currentTempCollision;
        }
    }
    else // !currentFace
    {
        // just move.
        pos = dest;
    }

    // not if the movement timed out
    // pos=stop;

    // find a replacement face if required
    FindCurrentFace();

    //#ifdef DEBUG
    //se_CheckGrid();
    //#endif

    //if (id<0)
    //    currentFace = NULL;

    lastTime = endTime;
}

// emulate old bug allowing objects to tunnel through walls
static short se_bugTunnel = false;
static nSettingItem<short> se_bugTunnelConfig("BUG_TUNNEL",
        se_bugTunnel );

class eFaceFindFilter: public tConsoleFilter
{
    virtual void DoFilterLine( tString& line )
    {
        line = tString( "FindCurrentFace() is running, so this message probably means there is a BUG: " ) + line;
    }
};

void eGameObject::FindCurrentFace(){
    // find a replacement for a removed face
    if ( currentFace && !currentFace->IsInGrid() )
    {
        if ( !se_bugTunnel )
        {
            currentFace = currentFace->FindReplacement( pos, Direction(), LastDirection() );
            if ( !currentFace && sn_GetNetState() != nCLIENT )
            {
                static bool warn = true;
                if (warn)
                {
                    tERR_WARN("Possible phase bug!\n");
                }

                warn = false;
            }
        }
        else
        {
            // allow tunneling through walls
            currentFace = NULL;
        }
    }

    // don't fetch a new current face if you're out of the game
    if ( !currentFace && GOID() < 0 )
    {
#ifdef DEBUG
        con << "Attempting to get a current face, but object is not in game.\n";
        st_Breakpoint();
#endif        
        return;
    }

    // did that do the trick? If no, use brute force.
    if ( !currentFace )
        currentFace = grid->FindSurroundingFace(pos);

    if ( currentFace )
    {
        // check if the position lies inside the current triangle
        REAL insideness = currentFace->Insideness(pos);
        if ( insideness < 0 )
        {
            eFaceFindFilter filter;
            // if ( sn_GetNetState() != nCLIENT )
            //    con << "insideness = " << insideness << "\n";

            // no. Find the center of the current face.
            int i;
            eCoord center;
            eHalfEdge * run = currentFace->Edge();
            for ( i = 2; i >= 0; --i )
            {
                run = run->Next();
                center = center + ( *run->Point() - pos );
            }
            eCoord centerToPos = -center*(1/3.0);
            center = pos - centerToPos;
  
            // find a position that lies just inside the current triange
            REAL centerInsideness = currentFace->Insideness(center);
            eCoord inside;
            
            if( centerInsideness < 0 )
            {
                // this should not happen! but will, if the triangle has wrong orientation.
                inside = center;
            }
            else
            {
                REAL factor = ( -insideness/( centerInsideness - insideness ) );
                if ( factor > 1 )
                {
                    factor = 1;
                }
                inside = pos - centerToPos * factor;
            }

            static bool recurse = true;
            if ( recurse )
            {
                class RecursionGuard
                {
                public:
                    RecursionGuard( bool& recursion )
                            :recursion_( recursion )
                    {
                        recursion = false;
                    }

                    ~RecursionGuard()
                    {
                        recursion_ = true;
                    }

                private:
                    bool& recursion_;
                };

                RecursionGuard guard( recurse );

                // warp to the known good position and move back to where the
                // object should be
                eCoord oldPos = pos;
                pos = inside;
#ifdef DEBUG
                eFace * lastFace = currentFace;
#endif
                try
                {
                    Move( oldPos, lastTime, lastTime, false );
                }
                catch( eDeath & ) // ignore death exceptions and leave object where it would have died
                {
#ifdef DEBUG
                    // try again (yeah, this looks like a WTF, but it really helps in some cases because the situation has changed since the last try. /me blames floating points)
                    // besides, (now, this was changed) the start position changed.
                    try
                    {
                        pos = center;
                        currentFace = lastFace;
                        Move( oldPos, lastTime, lastTime, false );
                    }
                    catch( eDeath & ){}
#endif
                }

                recurse = true;
            }
            else
            {
                // alternative if true movement is not possible:
                // project current position into triangle
                run = currentFace->Edge();
                for ( i = 2; i >= 0; --i )
                {
                    run = run->Next();
                    eCoord centerToPoint = *run->Point() - center;
                    eCoord runVec = run->Vec();
                    REAL prod = centerToPoint * runVec;
                    if (prod < 0)
                    {
                        REAL toClamp = (centerToPos * runVec) / prod;
                        if ( toClamp > 1 )
                        {
                            centerToPos = centerToPos * (1/toClamp);
                        }
                    }
                }
                pos = center + centerToPos;
            }
        }
    }
}

// simulates behaviour up to currentTime:
bool eGameObject::Timestep(REAL t){
    lastTime = t;
    return 0;
}
// return value: shall this object be destroyed?

void eGameObject::OnRoundBegin(){}
void eGameObject::OnRoundEnd(){}

void eGameObject::Kill(){}

// draws it to the screen using OpenGL
void eGameObject::Render(const eCamera *){}

//! draws it in a svg file
void eGameObject::DrawSvg(std::ofstream &f) {}
    
// *******************************************************************************
// *
// *	RendersAlpha
// *
// *******************************************************************************
//!
//!		@return	True if alpha blending is used
//!
// *******************************************************************************
bool eGameObject::RendersAlpha() const{return false;}

// Cockpit
bool eGameObject::RenderCockpitFixedBefore(bool){return true;}
// return value: draw everything else?

// the same purpose, but called after main rendering
void eGameObject::RenderCockpitFixedAfter(bool){}
// virtual perspective
void eGameObject::RenderCockpitVirtual(bool){}


#ifdef POWERPAK_DEB
void eGameObject::PPDisplay(){
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x),
                se_Y_ToScreen(pos.y),
                PD_CreateColor(DoubleBuffer,255,0,100));
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x+1),
                se_Y_ToScreen(pos.y),
                PD_CreateColor(DoubleBuffer,255,0,100));
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x-1),
                se_Y_ToScreen(pos.y),
                PD_CreateColor(DoubleBuffer,255,0,100));
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x),
                se_Y_ToScreen(pos.y+1),
                PD_CreateColor(DoubleBuffer,255,0,100));
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x),
                se_Y_ToScreen(pos.y-1),
                PD_CreateColor(DoubleBuffer,255,0,100));

}
#endif

// Receives control from player; nothing to do here
bool eGameObject::Act(uActionPlayer *,REAL){return false;}


bool eGameObject::TimestepThis(REAL currentTime,eGameObject *c){
#ifdef DEBUG
    c->grid->Check();
#endif

    tJUST_CONTROLLED_PTR< eGameObject > keep( c ); // keep object alive

    REAL maxstep=.2;

    // be more careful when going back
    if (currentTime<c->lastTime)
        maxstep=.1;

    int number_of_steps=int(fabs((currentTime-c->lastTime)/maxstep));
    if (number_of_steps<1)
        number_of_steps=1;
    if ( number_of_steps > 10 )
    {
        number_of_steps = 10;
    }

    REAL lastTime=c->lastTime;

    bool ret=false;

    for(int i=1;i<=number_of_steps;i++)
    {
        // make current face valid
        c->FindCurrentFace();

        if (sn_GetNetState()!=nCLIENT)
            for(int j=c->grid->gameObjectsInteresting.Len()-1;j>=0;j--)
                c->InteractWith(c->grid->gameObjectsInteresting(j),currentTime,0);

        REAL timeThisStep = lastTime+i*(currentTime-lastTime)/number_of_steps;

        c->urgentSimulationRequested_ = false;
        ret = ret || c->Timestep(timeThisStep);
        c->FindCurrentFace();

        // see if the object refused to get simulated, if yes, give up
        if ( 2 * c->lastTime < timeThisStep + lastTime )
            break;
    }
    for(int timeout = 10; timeout >= 0 && c->urgentSimulationRequested_; --timeout )
    {
        // simulate on while events are pending
        c->urgentSimulationRequested_ = false;
        ret = ret || c->Timestep(currentTime);
        c->FindCurrentFace();
    }

#ifdef DEBUG
    c->grid->Check();
#endif

    return ret;
}

#ifdef DEDICATED
static REAL se_maxSimulateAhead = .1f;
static tSettingItem<REAL> se_maxSimulateAheadConf( "MAX_SIMULATE_AHEAD", se_maxSimulateAhead );
#endif

// what is left of this time for the gameobject to use up
static REAL se_maxSimulateAheadLeft = 0.0f;
REAL eGameObject::MaxSimulateAhead()
{
    return se_maxSimulateAheadLeft;
}

static REAL se_lazyLag = 0;
//! @return the maximum extra simulation time difference, on top of regular lag, caused by lazy simulation
REAL eGameObject::GetMaxLazyLag()
{
    return se_lazyLag;
}

//! @param lag the maximum extra simulation time difference, on top of regular lag, caused by lazy simulation
void eGameObject::SetMaxLazyLag( REAL lag )
{
    se_lazyLag = lag;
}

void eGameObject::TimestepThisWrapper(eGrid * grid, REAL currentTime, eGameObject *c, REAL minTimestep )
{
    su_FetchAndStoreSDLInput();

    REAL simTime=currentTime;
    // backdate the object a bit
#ifndef DEDICATED
    if (sn_GetNetState()==nCLIENT && !sr_predictObjects)
#endif
        simTime -= c->Lag();

    REAL maxSimTime = simTime;
#ifdef DEDICATED
    se_maxSimulateAheadLeft = se_maxSimulateAhead+se_lazyLag;

    REAL lagThreshold = c->LagThreshold();
    if( !c->urgentSimulationRequested_ )
    {
        // nothing interesting happening. add an extra portion of lag compensation
        simTime -= lagThreshold;

        if ( simTime < c->LastTime() + minTimestep )
        {
            // don't waste your time on too small timesteps
            return;
        }
    }
    else
    {
        maxSimTime += se_maxSimulateAheadLeft;
    }

#endif

    // check for teleports out of arena bounds
    if (!eWallRim::IsBound(c->pos,-20))
    {
        se_maxSimulateAheadLeft = 0;

        c->Kill();
        return;
    }

    // only simulate forward here
    if ( maxSimTime > c->lastTime )
    {
        if (TimestepThis(simTime,c))
        {
            if (c->autodelete)
                c->RemoveFromGame();
            else
            {
                c->currentFace=NULL;
                c->RemoveFromList();
            }
        }
    }

    se_maxSimulateAheadLeft = 0.0;
}

// does a timestep and all interactions for every eGameObject
void eGameObject::s_Timestep(eGrid *grid, REAL currentTime, REAL minTimestep)
{
#ifdef DEBUG
    grid->Check();
#endif

    // simulate game objects
    for(int i=grid->gameObjects.Len()-1;i>=0;i--)
    {
        eGameObject * c = grid->gameObjects(i);
        TimestepThisWrapper( grid, currentTime, c, minTimestep );
    }

#ifdef DEBUG
    grid->Check();
#endif
}

#ifdef DEBUG
eGameObject *displayed_gameobject = 0;
#endif

void eGameObject::RenderAll(eGrid *grid, const eCamera *cam){
    //if (!sr_glOut)
    //    return;
    
    // first, we need to render all the non-alpha blended objects.
    // if we encounter non-alpha blended objects after alpha blended objects
    // have already been rendered, we need to re-sort them to the back.
    eGameObject * firstAlpha = NULL;
    for(int i=grid->gameObjects.Len()-1;i>=0;i--){
        su_FetchAndStoreSDLInput();
        if (sr_glOut){
            eGameObject * object = grid->gameObjects(i);
#ifdef DEBUG
            displayed_gameobject = object;
#endif
            object->Render(cam);

            bool thisAlpha = object->RendersAlpha();
            if ( !thisAlpha && firstAlpha )
            {
                // resort the object, switch places with the first alpha blended one.
                // This will only have an effect in the next frame,
                // but the small flickering error is to be tolerated, especially
                // since alpha blended game objects tend to gently fade in.
                int firstAlphaID = firstAlpha->id;
                grid->gameObjects.Remove(firstAlpha,firstAlpha->id);
                grid->gameObjects.Add(firstAlpha,firstAlpha->id);
                grid->gameObjects.Remove(object,object->id);
                grid->gameObjects.Add(object,object->id);
                
                // the first alpha blended object no longer is the first. Look for
                // a replacement, only one object is a candidate.
                firstAlpha = 0;
                if ( firstAlphaID > 0 )
                {   
                    firstAlpha = grid->gameObjects(firstAlphaID - 1);
                    tASSERT( firstAlpha->RendersAlpha() );
                }
            }
            if ( thisAlpha && !firstAlpha )
            {
                // store first known alpha blending object
                firstAlpha = object;
            }
#ifdef DEBUG
            displayed_gameobject = 0;
#endif
        }
    }
}

#ifdef POWERPAK_DEB
void eGameObject::PPDisplayAll(){
    for(int i=gameObjects.Len()-1;i>=0;i--){
        if (pp_out) gameObjects(i)->PPDisplay();
    }
}
#endif


void eGameObject::DeleteAll(eGrid *grid){
    int i;
    for(i=grid->gameObjects.Len()-1;i>=0;i--)
    {
        eGameObject* o = grid->gameObjects(i);
        o->RemoveFromGame();
#ifdef POWERPAK_DEB
        if (pp_out) o->PPDisplay();
#endif
    }
}

eReferencableGameObject::eReferencableGameObject(eGrid *grid, const eCoord &p,const eCoord &d, eFace *currentface, bool autodelete)
: eGameObject( grid, p, d, currentface, autodelete )
{
}

// delegate real reference counting
void eReferencableGameObject::AddRef()
{
    tReferencable< eReferencableGameObject >::AddRef();
}

void eReferencableGameObject::Release()
{
    tReferencable< eReferencableGameObject >::Release();
}

void eReferencableGameObject::DoRemoveFromGame()
{
    // nothing needs to be done, the reference counting takes care of the destruction
}

eStackGameObject::eStackGameObject(eGrid *grid, const eCoord &p,const eCoord &d, eFace *currentface)
: eGameObject( grid, p, d, currentface, false )
{
}

void eStackGameObject::AddRef()
{
}

void eStackGameObject::Release()
{
}

void eStackGameObject::DoRemoveFromGame()
{
    // must not get called
    tERR_ERROR("Stack game object removed from game.");
}


