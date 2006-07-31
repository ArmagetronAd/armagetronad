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

#include "eGrid.h"
#include "eTess2.h"
#include "ePath.h"
#include "eGameObject.h"
#include "tConsole.h"
#include "eWall.h"
#include "eSensor.h"

// heap of HalfEdges that are considered to be included in a possible path
static tHeap<eHalfEdge> openEdges;
static tHeap<eHalfEdge> closedEdges;



static REAL Distance(const eHalfEdge* a, const eCoord& b)
{
    tASSERT( a );
    tASSERT( a->Point() );

    return sqrt(((*a->Point()) - b).NormSquared());
}



static REAL Modifier(const eGameObject *gameObject,
                     const eHalfEdge   *edge)
{
    tASSERT(edge);
    tASSERT(gameObject);

    eWall* w = edge->GetWall();
    if (!w && edge->Other())
        w = edge->Other()->GetWall();

    return gameObject->PathfindingModifier(w);
}

static bool CanPass(const eGameObject *gameObject,
                    const eHalfEdge   *edge)
{
    tASSERT(edge);
    tASSERT(gameObject);

    eWall* w = edge->GetWall();
    if (!w && edge->Other())
        w = edge->Other()->GetWall();

    return !gameObject->EdgeIsDangerous(w, gameObject->LastTime(), 0.5f);
}


// check whether this edge
// crosses any of the brand-new walls on the grid:
eWall* eHalfEdge::CrossesNewWall(const eGrid *grid) const
{
    // check all the currently drawn eWalls:
    for(int i=grid->wallsNotYetInserted.Len()-1;i>=0;i--)
    {
        const eHalfEdge *other_e=grid->wallsNotYetInserted(i)->Edge();
        if (//!sg_netPlayerWalls(i)->Preliminary() &&
            other_e->Point() && other_e->Other() && other_e->Other()->Point())
        {
            tJUST_CONTROLLED_PTR< ePoint > new_cross_p=IntersectWith(other_e);
            if (new_cross_p)
            {
                REAL e_ratio =Ratio(*new_cross_p);
                REAL o_ratio =other_e->Ratio(*new_cross_p);
                if (0<=e_ratio && 1>=e_ratio &&
                        0<=o_ratio && 1>=o_ratio)
                { // find the fall
                    eWall *w = other_e->GetWall();
                    if (!w)
                        w = other_e->Other()->GetWall();

                    if (w)
                        return w;
                }
            }
        }
    }
    return NULL;
}

void eHalfEdge::ClearPathData()
{
    while (openEdges.Len())
    {
        eHalfEdge *e = openEdges.Remove(0);
        e->origin_ = PATH_NONE;
    }
    while (closedEdges.Len())
    {
        eHalfEdge *e = closedEdges.Remove(0);
        e->origin_ = PATH_NONE;
    }
}


REAL EdgePenalty(const eHalfEdge *e, const eGameObject *g)
{
    eCoord probePre = e->Vec().Turn(0, 1);
    eCoord probe    = probePre * (1/sqrt(probePre.NormSquared()));

    eSensor s (const_cast<eGameObject * >( g ), *e->Point() + e->Vec() * .5f + probePre * .0001f, probe);
    s.SetCurrentFace(e->Face());
    s.detect(1);
    if (s.hit > 1)
        s.hit = 1;

    return 100 * (1/(s.hit + .1) - (1/1.1));
}

void eHalfEdge::SetMinPathLength( REAL length, tHeapBase& heap, ePATH_ORIGIN origin )
{
    origin_ = origin;
    tHeapElement::SetVal( length, heap );

    tASSERT( &heap == Heap() );
}

// pathfinding interface: find a path for gameobject from origin to target.
void eHalfEdge::FindPath(const eCoord& startPoint, const eFace* startFace,
                         const eCoord& stopPoint , const eFace* stopFace,
                         const eGameObject* gameObject,
                         ePath& path)
{
    tASSERT( startFace );
    tASSERT( stopFace );
    tASSERT( gameObject );

    // cleanup previous path data (we'll keep it around for debugging)
    ClearPathData();
    path.Clear();

    // start: add the three vertices around the origin to the open list
    {
        eHalfEdge *run = startFace->Edge();
        for (int i=2; i>=0; i--)
        {
            run->SetMinPathLength( Distance(run, startPoint) + Distance(run, stopPoint), openEdges, PATH_START );

            run = run->Next();
        }
    }

    // search for a path until one of the edges of the goal face is in the closed list or we have to give up
    eHalfEdge* stopEdge = NULL;

    while (openEdges.Len() > 0 && !stopEdge)
    {
        // take the most promising HalfEdge out of the open list, put it
        // into the closed list
        // and add all possible ways from there to the open list.

        eHalfEdge *e = openEdges.Remove(0);

        int origin = e->origin_;
        origin |= PATH_CLOSED;
        e->origin_ = static_cast<ePATH_ORIGIN>(origin);

        closedEdges.Insert(e);

        // check if we are at the goal
        if (e->Face() == stopFace)
            stopEdge = e;

#ifdef DEBUG
        //      con << "examining " << *e->Point() << ", " << e->Vec() << "\n";
#endif

        eHalfEdge *next = e->Next();
        eHalfEdge *prev = NULL;

        eHalfEdge *other_next = e->Other()->Next();
        eHalfEdge *prev_other = NULL;

        // try the next HalfEdge in this triangle
        {
            if (next)
            {
                prev = next->Next();

                // check if we cross a new wall on the way
                if (!e->CrossesNewWall(gameObject->Grid()))
                {
                    // we get this much closer to the target by going this way:
                    REAL closer  = Distance(e, stopPoint) - Distance(next, stopPoint);

                    // but have to go this much to get there:
                    REAL way     = sqrt(e->Vec().NormSquared()) * Modifier(gameObject, e);

                    // tell n that there is a possible path to it
                    next->PossiblePath(PATH_PREV, e->MinPathLength() + way - closer + EdgePenalty(e, gameObject) );
                }
            }
        }


        // try the previous HalfEdge in this triangle
        if (prev)
        {
            tASSERT(prev->Next() == e);
            prev_other = prev->Other();

            // check if we cross a new wall on the way
            if (!prev->CrossesNewWall(gameObject->Grid()))
            {
                // we get this much closer to the target by going this way:
                REAL closer  = Distance(e, stopPoint) - Distance(prev, stopPoint);

                // but have to go this much to get there:
                REAL way     = sqrt(prev->Vec().NormSquared()) * Modifier(gameObject, e);

                // tell n that there is a possible path to it
                prev->PossiblePath(PATH_NEXT, e->MinPathLength() + way - closer  + EdgePenalty(e, gameObject));
            }
        }


        // now the edges that are just around the corner. They do not
        // make the way longer, but require the passed edge to be harmless
        // to the object.

        if (other_next && CanPass(gameObject, e))
            other_next->PossiblePath(PATH_PREV_OTHER, e->MinPathLength() + EdgePenalty(e, gameObject) + .000001f);
        if (prev_other && CanPass(gameObject, prev))
            prev_other->PossiblePath(PATH_OTHER_NEXT, e->MinPathLength() + EdgePenalty(e, gameObject) + .000001f);
    }

    // no path found...
    if (!stopEdge)
    {
        ClearPathData();
        return;
    }

    // allright! Now we just go back from the goal to the origin,
    // following the crumbs of bread we left behind.

#ifdef DEBUG
    con << "Found path.\n";
    //  con << startPoint << "\n";
#endif

    path.Add(stopPoint);

    eHalfEdge* run = stopEdge;
    while (run && run->Face() != startFace)
    {
        path.Add(run);

#ifdef DEBUG
        //      con << *run->Point() << ", " << run->Vec() << "\n";
#endif

        tASSERT(run->origin_ >= PATH_CLOSED);

        switch (run->origin_-PATH_CLOSED)
        {
        case (PATH_NEXT):
                        run = run->Next();
            break;
        case (PATH_PREV):
                        run = run->Next()->Next();
            break;
        case (PATH_OTHER_NEXT):
                        run = run->Other()->Next();
            break;
        case (PATH_PREV_OTHER):
                        run = run->Next()->Next()->Other();
            break;
        default:
            tERR_WARN("Pathfinding error!\n");
            run = startFace->Edge();
        }
    }

    //  path.Add(run);
    //  path.Add(startPoint);

#ifdef DEBUG
    //  con << stopPoint << "\n";
#endif

    ClearPathData();
}


void eHalfEdge::PossiblePath( ePATH_ORIGIN newOrigin, REAL minLength ) // tell the pathfinder that there might be a path of total length minLength through this HalfEdge, coming from the given origin type.
{
    // nothing needs to be done if there is already a shorter path known
    if( origin_ == PATH_NONE )
    {
        // completely new entry.

        SetMinPathLength( minLength, openEdges, newOrigin );

#ifdef DEBUG
        //      con << "adding " << *Point() << ", " << Vec() << ", origin "
        //	  << origin << " to open list.\n";
#endif
    }
    else if( minLength <  MinPathLength() && origin_ < PATH_CLOSED)
    {
        // just update our info; the path got shorter.
        SetMinPathLength( minLength, openEdges, newOrigin );

#ifdef DEBUG
        //      con << "updating " << *Point() << ", " << Vec() << ", origin "
        //	  << origin << " to open list.\n";
#endif

    }
}


tHeapBase *eHalfEdge::Heap() const
{
    if (origin_ >= PATH_NONE)
        return NULL;
    if (origin_ >= PATH_CLOSED)
        return &closedEdges;

    return &openEdges;
}


static ePath* lastPath = NULL;

#ifdef DEBUG
#include "rRender.h"

void ePath::RenderLast()  // renders the last found path
{
#ifndef DEDICATED
    if (!lastPath)
        return;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glColor4f(1,0,0,1);

    BeginLineStrip();
    for (int i = lastPath->positions.Len()-1; i>=0; i--)
    {
        eCoord c = lastPath->positions(i) + lastPath->offsets(i);
        Vertex(c.x, c.y, 0.1f);
    }
    RenderEnd();

    glColor4f(1,1,0,1);

    BeginLineStrip();
    if (lastPath->current >= 0 && lastPath->positions.Len() > 0)
    {
        eCoord c = lastPath->CurrentPosition();
        Vertex(c.x, c.y, 0);
        Vertex(c.x, c.y, 50);
    }
    RenderEnd();
#endif
}
#endif

bool ePath::Proceed()
{
    if (current > 0)
    {
        current--;
        return true;
    }
    else
        return false;
}

bool ePath::GoBack()
{
    if (current < positions.Len()-1)
    {
        current++;
        return true;
    }
    else
        return false;
}

ePath::ePath(){
    current=-1;
}

ePath::~ePath(){
    if ( lastPath == this )
        lastPath = NULL;
}

void ePath::Add(eHalfEdge *e)
{
    tASSERT (e->Point() && e->Next() && e->Next()->Point() && e->Next()->Next() && e->Next()->Next()->Point());

    current++;

    eCoord pos    = *e->Point();
    //  eCoord dir    = e->Vec();
    eCoord offset = ((*e->Next()->Next()->Point() + *e->Next()->Point()) * .5f - pos) * .5f;
    //  offset = offset - dir * (eCoord::F(dir, offset) / dir.NormSquared());

    REAL ol = sqrt(offset.NormSquared());
    if (ol > 1)
        offset = offset * (1/ol);

    positions[current] = pos;
    offsets[current]   = offset;
}

void ePath::Add(const eCoord& point)
{
    current++;

    positions[current] = point;
    offsets[current]   = eCoord(0, 0);
}


void ePath::Clear()
{
    lastPath = this;

    current=-1;

    offsets.SetLen(0);
    positions.SetLen(0);
}




