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

#ifdef _MSC_VER
// disable long debug symbol warning
#pragma warning ( disable: 4786 )
#endif

#include "eGrid.h"
#include "eTess2.h"

#include "rTexture.h"
#include "tEventQueue.h"
#include "eTimer.h"
#include "eWall.h"
#include "eGameObject.h"
#include "eCamera.h"

#include "tMath.h"
#include "nConfig.h" 

#include "tRecorder.h"

#include <vector>
#include <set>

#define SMALL_FLOAT 1E-30
const REAL se_maxGridSize = 1E+15;	// this gives us way more space than OpenGL allows us to render

// #define EPS 1E-2

int se_debugExt=0;

// counts how many edges we should attempt to simplify
static int se_simplifyEdges = 0;

// after one edge has been successfully simplified, simplify that many more,
// it seems to pay
static const int se_simplifyEdgesSuccess = 20;

// after one edge has been inserted, try to simplify that many old ones
static const int se_simplifyEdgesNew = 2;

tDEFINE_REFOBJ( ePoint )
tDEFINE_REFOBJ( eHalfEdge )
tDEFINE_REFOBJ( eFace )


REAL se_EstimatedRangeOfMult( const eCoord &a,const eCoord &b )
{
    static eCoord turn(0,1);
    return fabs( b*a ) + fabs( b*( a.Turn( turn ) ) );
}

REAL st_GetDifference( const eCoord &a,const eCoord &b )
{
    return (a-b).NormSquared();
}

eCoord    eTempEdge::Vec()         const{return halfEdges[0]->Vec();}
eCoord&   eTempEdge::Coord(int i)  const{return *halfEdges[i]->Point();}
ePoint    *eTempEdge::Point(int i) const{return halfEdges[i]->Point();}
eFace     *eTempEdge::Face(int i)  const{return halfEdges[i]->Face();}
eHalfEdge *eTempEdge::Edge(int i)  const{return halfEdges[i];}
eWall     *eTempEdge::Wall()       const{return halfEdges[0]->GetWall();}

void	   eTempEdge::SetWall( eWall* W )    const
{
    halfEdges[0]->ClearWall();
    halfEdges[0]->SetWall( W );
}

short se_bugRip=false;
static nSettingItem<short> se_bugRipNet( "BUG_RIP", se_bugRip );

inline void eHalfEdge::SetWall(eWall *w){
    if (!w)
        return;

    // emulate rip bug
    if ( se_bugRip )
    {
        // delete old wall
        if ( other && other->GetWall() && other->GetWall()->Splittable() )
            other->eWallHolder::SetWall( 0 );

        // set new wall
        eWallHolder::SetWall( w );

        w->CalcLen();

        return;
    }

    // get other wall
    eWall* otherWall = 0;
    if ( other )
        otherWall = other->GetWall();

    // ask for permission
    if ( otherWall && !w->RunsParallelActive( otherWall ) )
        return;

    eWall* thisWall = GetWall();
    if ( thisWall )
    {
        // ask this wall for permission
        bool allowed = w->RunsParallelActive( thisWall );
        if ( otherWall )
        {
            // inform wall, but don't insert it ( since it is impossible )
            w->RunsParallelActive( otherWall );
            // st_Breakpoint();
        }
        else if ( allowed && other )
        {
            // simply flip the wall and attach it to the other edge :-)
            w->Flip();
            other->eWallHolder::SetWall(w);
            //            return;
        }
    }
    else
    {
        eWallHolder::SetWall( w );
    }

    w->CalcLen();
}

void eHalfEdge::ClearWall( void )
{
    eWallHolder::SetWall( NULL );
}

// destructor unlinking all pointers
eHalfEdge::~eHalfEdge()
{
    Unlink();
    RemoveFromHeap();
}

inline void eHalfEdge::Unlink()
{
    if (point && this == point->edge)
    {
        if (prev && prev->other)
            point->edge = prev->other;
        else if (other && other->next)
            point->edge = other->next;
        else
            point->edge = 0;
    }
    if (face && this == face->edge)
    {
        if (next)
            face->edge = next;
        if (prev)
            face->edge = prev;
    }

    if (next)
        next->prev = NULL;
    if (prev)
        prev->next = NULL;
    if (other)
        other->other = NULL;

    point   = NULL;
    face    = NULL;
    prev    = NULL;
    next    = NULL;
    other   = NULL;
}



// ********************************************

// consistency checks:

void eGrid::Check() const{
    //    return;
    if ( a )
    {
        tASSERT( a->Point() == B );
        tASSERT( a->other->Point() == C );
        tASSERT( b->Point() == C );
        tASSERT( b->other->Point() == A );
        tASSERT( c->Point() == A );
        tASSERT( c->other->Point() == B );
    }

#ifdef DEBUG_EXPENSIVE
    if (!doCheck)
        return;

    int i;
    for (i = points.Len()-1; i>=0; i--)
        points(i)->Check();

    for (i = faces.Len()-1; i>=0; i--)
        faces(i)->Check();

    for (i = edges.Len()-1; i>=0; i--)
        edges(i)->Check();
#endif
}


// **************************
//          eDual
// **************************

eDual::~eDual()  // destructor
{
    tASSERT(!edge);
}

bool eDual::Check(int type)
{
    if (type)
    {
        tASSERT(!edge ||  this == static_cast<eDual*>(edge->face));
    }
    else
    {
        tASSERT(!edge ||  this == static_cast<eDual*>(edge->point));
    }

    tASSERT( ID < 0 || !edge || edge->ID>=0);

    return true;
}
// basic consistency check; type is 0 for points and 1 for faces.
inline void eDual::Unlink(int type)
{

}
// removes itself from the edges' pointers. type is 0 for points and 1 for edges.




// **************************
//          ePoint
// **************************

void ePoint::Print(std::ostream &s) const
{
    s << "(" << x << ", " << y << ")";
}

// replaces all known pointers to *this with pointers to *P.
void ePoint::Replace(ePoint *P)
{
    tControlledPTR< ePoint > keep( this );

    //  if (!(*this==*P))
    //    tERR_WARN("Unequal points are going to get exchaned!");

    eHalfEdge *run = edge;
    const eHalfEdge *stop = run;

    *static_cast<eCoord *>(P) = *this;

    do
    {
        tASSERT(run && run->Point() == this);
        run->SetPoint(P);

        run = run->other->next;

    }
    while (stop != run);

    edge = NULL;
}

void ePoint::Unlink()
{
    if (!edge)
        return;

    eHalfEdge *run = edge;
    const eHalfEdge *stop = run;

    do
    {
        tASSERT(run && run->Point() == this &&
                (!run->next || !run->next->next || run == run->next->next->next)) ;
        run = run->next;
        if (run)
            run= run->next;
        if (run)
            run = run->other;
    }
    while (run && stop != run);

    edge = NULL;
}

// **************************
//          eFace
// **************************

// database of eFace replacements made during grid restructuring
typedef std::vector< tJUST_CONTROLLED_PTR<eFace> > eReplacementStorageBase;
class eReplacementStorage : public eReplacementStorageBase
{
};

void se_LinkFaces( tControlledPTR< eFace >& old, eFace* replacement )
{
    tASSERT( old != replacement );

    // we only need to store an eFace if it is referenced by something different than
    // the pointer we got it by
    if ( old->GetRefcount() > 1 )
    {
        eReplacementStorage& storage = old->GetReplacementStorage();
        storage.push_back( replacement );
    }
}

typedef std::pair< eFace*, REAL > eFaceScorePair;
typedef std::set< const eFace* > eFaceVisitedSet;

struct eFaceReplacementArgument
{
    eCoord coord, direction, lastDirection;
    eFaceVisitedSet visited;
};

eFaceScorePair se_FindBestReplacement( const eFace *old, eFaceReplacementArgument& arg )
{
    // return invalid return if the face was already visited
    if ( arg.visited.find( old ) != arg.visited.end() )
        return eFaceScorePair( 0, -se_maxGridSize*se_maxGridSize );

    // register face as visited
    arg.visited.insert( old );

    // find entry in replacement map

    {
        // real work starts here
        // get the vector of replacements
        const eReplacementStorage& storage = old->GetReplacementStorage();

        // iterate it

        // the currently best face/insideness pair
        std::pair< eFace*, REAL > best( 0, -se_maxGridSize*se_maxGridSize );
        for( eReplacementStorage::const_iterator i = storage.begin(); i != storage.end(); ++i )
        {
            // the current face/insideness pair
            eFaceScorePair current; // ( 0, -100000 );

            // the current face
            eFace* face = *i;

            // is it inside the grid?
            if ( face->IsInGrid() )
            {
                // enter it directly
                current.first = face;
                current.second = face->Insideness( arg.coord, arg.direction, arg.lastDirection );
            }
            else
            {
                // no, we have to look for a replacement recursively
                current = se_FindBestReplacement( face, arg );
            }

            if ( ( current.second > best.second && current.first ) || !best.first )
            {
                best = current;
            }
        }

        return best;
    }
}

eReplacementStorage& eFace::GetReplacementStorage() const
{
    if ( !replacementStorage )
        replacementStorage = tNEW( eReplacementStorage );

    return *replacementStorage;
}

eFace* eFace::FindReplacement( const eCoord& coord, const eCoord& direction, const eCoord& lastDirection ) const
{
    // check if we don't need an replacement
    tASSERT( !this->IsInGrid() );

    // set of already visited faces
    eFaceReplacementArgument arg;
    arg.coord = coord;
    arg.direction = direction;
    arg.lastDirection = lastDirection;

    // delegate search to recursive function
    return se_FindBestReplacement( this, arg ).first;
}

eFace* eFace::FindReplacement( const eCoord& coord, const eCoord& direction ) const
{
    // check if we don't need an replacement
    tASSERT( !this->IsInGrid() );

    // set of already visited faces
    eFaceReplacementArgument arg;
    arg.coord = coord;
    arg.direction = direction;

    // delegate search to recursive function
    return se_FindBestReplacement( this, arg ).first;
}

eFace* eFace::FindReplacement( const eCoord& coord ) const
{
    // check if we don't need an replacement
    tASSERT( !this->IsInGrid() );

    // set of already visited faces
    eFaceReplacementArgument arg;
    arg.coord = coord;

    // delegate search to recursive function
    return se_FindBestReplacement( this, arg ).first;
}

eFace::~eFace()
{
    Unlink();
    //	se_EraseFace( this );

    delete replacementStorage;
}

eFace::eFace (eHalfEdge *e1,eHalfEdge *e2,eHalfEdge *e3 )
        :replacementStorage( 0 )
{
    Create( e1, e2, e3 );
}

eFace::eFace (eHalfEdge *e1,eHalfEdge *e2,eHalfEdge *e3, tControlledPTR< eFace >& old )
        :replacementStorage( 0 )
{
    Create( e1, e2, e3 );
    se_LinkFaces( old, this );
}

eFace::eFace (eHalfEdge *e1,eHalfEdge *e2,eHalfEdge *e3, tControlledPTR< eFace >& old1, tControlledPTR< eFace >& old2 )
        :replacementStorage( 0 )
{
    Create( e1, e2, e3 );
    se_LinkFaces( old1, this );
    se_LinkFaces( old2, this );
}

void eFace::Create (eHalfEdge *e1,eHalfEdge *e2,eHalfEdge *e3)
{
#ifdef DEBUG 
    tASSERT( edge == 0 );

    tASSERT(e1->other);
    tASSERT(e2->other);
    tASSERT(e3->other);

    tASSERT(e1->Point() == e3->other->Point());
    tASSERT(e2->Point() == e1->other->Point());
    tASSERT(e3->Point() == e2->other->Point());

    //  tASSERT(*e1->Point() != *e2->Point());
    //  tASSERT(*e3->Point() != *e2->Point());
    //  tASSERT(*e1->Point() != *e3->Point());

    tASSERT((*e1->Point() - *e2->Point()).NormSquared() < se_maxGridSize*100);
    tASSERT((*e3->Point() - *e2->Point()).NormSquared() < se_maxGridSize*100);
    tASSERT((*e1->Point() - *e3->Point()).NormSquared() < se_maxGridSize*100);
#endif

    nextProcessed = NULL;

    e1->SetFace(this);
    e2->SetFace(this);
    e3->SetFace(this);

    e1->next = e2;
    e2->next = e3;
    e3->next = e1;
    e1->prev = e3;
    e2->prev = e1;
    e3->prev = e2;

    e1->Point()->edge = e1;
    e2->Point()->edge = e2;
    e3->Point()->edge = e3;

    edge = e1;

#ifdef DEBUG
    REAL area = -(*e1->Point() - *e2->Point()) * (*e1->Point() - *e3->Point());
    if( area <= -EPS )
    {
        con << "Face " << *this << " has wrong orientation (area: " << area << ").\n";
    }
#endif
}

void eFace::Unlink(){
    if (edge)
    {
        eHalfEdge *run = edge;
        const eHalfEdge *stop = run;

        do
        {
            tASSERT(run->Face() == this);
            run->SetFace(NULL);

            run = run->next;
        }
        while (stop != run);

        edge = NULL;
    }
}

// ckeck if the given point lies inside this face; return value positive if it is inside and negative if outside
REAL eFace::Insideness(const eCoord& pos, const eCoord& direction, const eCoord& lastDirection ) const
{
    // TODO: bounding box check

    eCoord min, max;

    eHalfEdge *run = edge;
    if (!run)
        return -1000000000;

    REAL ret = 1000000000;

    eCoord *last_p = run->Point();
    run = run->next;
    const eHalfEdge *stop = run;

    do
    {
        tASSERT(run && run->Face() == this);
        eCoord *p = run->Point();
        eCoord ed=*last_p - *p;
        eCoord cc= pos - *p;

        REAL val = ed*cc;
        // REAL tol = 10 * EPS/(ed.Norm() + EPS*1000);
        REAL tol = 10 * EPS;
        REAL retDir = direction*ed;
        REAL retLastDir = lastDirection*ed;
        val += tol*(retDir + tol*retLastDir);
        if ( val < ret )
        {
            ret = val;
        }

        last_p = p;
        run = run->next;
    }
    while (run && stop != run);

    return ret;
}

REAL eFace::Insideness(const eCoord& c, const eCoord& direction ) const
{
    return Insideness( c, direction, eCoord() );
}

REAL eFace::Insideness(const eCoord& c ) const
{
    return Insideness( c, eCoord(), eCoord() );
}

// ckeck if the given point lies inside this face (edges included)
bool eFace::IsInside(const eCoord &c) const
{
    // flags storing whether there is a point of the triangle with bigger/smaller coordinates
    // than the test coords
    bool xLower=false, xHigher=false, yLower=false, yHigher=false;

    eHalfEdge *run = edge;
    if (!run)
        return false;

    eCoord *last_p = run->Point();
    run = run->next;
    const eHalfEdge *stop = run;

    do
    {
        tASSERT(run && run->Face() == this);
        eCoord *p = run->Point();
        eCoord ed=*last_p - *p;
        eCoord cc= c - *p;

        if ( p->x <= c.x + EPS )
            xLower = true;
        if ( p->x >= c.x - EPS )
            xHigher = true;
        if ( p->y <= c.y + EPS )
            yLower = true;
        if ( p->y >= c.y - EPS )
            yHigher = true;

        if (ed*cc<-EPS)
            return false;

        last_p = p;
        run = run->next;
    }
    while (run && stop != run);

    // test if the point is in the bounding rectangle of the triangle
    return xLower && xHigher && yLower && yHigher;
}

void eCoord::Print(std::ostream &s)const{ s << "(" << x << "," << y << ")"; }
void eHalfEdge::Print(std::ostream &s)const{ s << "[" << *Point() << "->" << *other->Point() << "]"; }
void eFace::Print(std::ostream &s)const{ s << "[" << *edge->Point() << "->" << *edge->next->Point() << "->" << *edge->next->next->Point() << "]"; }
void eCoord::Read(std::istream &s)
{
    char c;
    s >> c;
    tASSERT( c == '(' );
    s >> x;
    s >> c;
    tASSERT( c == ',' );
    s >> y;
    s >> c;
    tASSERT( c == ')' );
}


// **************************
//          eGrid
// **************************

/*
 * Return the winding number closest to direction dir
 */
int eGrid::DirectionWinding(const eCoord&dir)
{
    return axis.NearestWinding(dir);
}

/*
 * Return the direction associated with a winding number
 */
eCoord eGrid::GetDirection(int winding)
{
    return axis.GetDirection(winding);
}

/*
 * Define the number of winding to be used on this grid
 */
void eGrid::SetWinding(int number)
{
    axis.Init(number);
}

/*
 * Define the number of winding and their values.
 * When the normalize flag is set, the values are normalized.
 */
void eGrid::SetWinding(int number, eCoord directions[], bool normalise)
{
    axis.Init(number, directions, normalise);
}

/*
 * change/turn currentWinding by 'direction' steps
 */
void eGrid::Turn(int &currentWinding, int direction)
{
    axis.Turn(currentWinding, direction);
}


static eHalfEdge *leakEdge =NULL;
static ePoint    *leakPoint=NULL;

static int se_drawLineTimeout = 10000;
static tSettingItem<int> se_drawLineTimeoutConf("DRAWLINE_TIMEOUT", se_drawLineTimeout);

ePoint * eGrid::DrawLine(ePoint *start, const eCoord &end, eWall *w, bool change_grid){
    // for every edge we add, allow to simplify
    se_simplifyEdges += se_simplifyEdgesNew;

    // prepare a teporary edge so we always know what the wall we are placing looks like
    tStackObject< eTempEdge > toBePlaced( *start, end, w );
    //tJUST_CONTROLLED_PTR< eWall > wal( w );

    // sanity check
    if ( !isfinite( end.x ) || !isfinite( end.y ) )
        return start;

    Range(end.NormSquared());
    int restart=1;
    //  eEdge *todo=tNEW(eEdge) (start,end,wal);


    // now the loop: go from source to destination,
    // "rotate" the edges we cross out of our way (if possible)
    // or cut them in half.

    tJUST_CONTROLLED_PTR<eHalfEdge> currentEdge  = NULL;
    // the edge we are currently considering.
    // the line we are to draw starts at currentEdge->Point()
    // and goes into currentEdge->Face().


    REAL left_test,right_test;
    int exactly_onLeft=0;

    tASSERT(start->edge);

#ifdef DEBUG
    bool foundCurrentEdge;
    static const int laststarts_max = 10;
    ePoint *laststarts[laststarts_max];
#endif

    eCoord dir=eCoord(1,1); // the direction we are going

    int timeout = se_drawLineTimeout;

    // distance to end to know when we are going backwards
    REAL distToEnd = ( *start - end ).NormSquared();
    ePoint * lastStart = start;

    bool goon=1;
    while (goon && dir.NormSquared()>0){
#ifdef DEBUG
        static int count = 0;
        count++;
        //    if (count == 40)
        //      st_Breakpoint();
#endif

        dir=end-*start; // the direction we are going

        // check if we got closer to the end, if not, accelerate timeout and set breakpoint
        REAL newDistToEnd = dir.NormSquared();
        // std::cout << distToEnd << ", " << newDistToEnd << "\n";
        if ( newDistToEnd > distToEnd && start != lastStart )
        {
            //st_Breakpoint();
            timeout -= 100;
        }
        distToEnd = newDistToEnd;
        lastStart = start;

#ifdef DEBUG
        for (int i = laststarts_max-1; i>=1; i--)
            laststarts[i] = laststarts[i-1];

        laststarts[0] = start;

        if (timeout < 10)
            st_Breakpoint();
#endif

        if (timeout-- < 0)
        {
            requestCleanup = true;
            return start;  // give up.
        }

#ifdef DEBUG
        Check();
#endif

        // first, we need to find the first eFace:
        //    int i;

#ifdef DEBUG
        //    if (doCheck && start->id == 4792)
        //      st_Breakpoint();
#endif

        tASSERT(restart || currentEdge);

        if (restart){
            exactly_onLeft = -1;

            eHalfEdge *run  = start->edge;
            tASSERT(run);
            eHalfEdge *stop = run;

            eCoord lvec = run->Vec();
            eCoord rvec;
            left_test = lvec * dir;

            eHalfEdge *best = NULL;
            REAL best_score = -100;

            currentEdge = NULL;

            while (run)
            {
                eHalfEdge *next = run->next->next;
                tASSERT(next->next = run);
                next = next->other;

                tASSERT(next->Point() == start);

                right_test = left_test;
                rvec = lvec;
                lvec = next->Vec();
                left_test = lvec * dir;

                if (right_test <= 0 && left_test >= 0)
                {
                    bool good = rvec * lvec < 0 && right_test < 0;
                    if (good)
                        next = NULL;

                    if (good || !currentEdge)
                    {
                        currentEdge = run;

                        // are dir and lvec exactly on one line?
                        exactly_onLeft=(left_test<=EPS * EPS * se_EstimatedRangeOfMult(lvec,dir));
                    }
                }

                REAL score = right_test * left_test;
                if (right_test < 0 && left_test < 0)
                    score *= -1;     // here, - * - still is -.....

                if (!best || ( score > best_score  && rvec * lvec < 0 )
                   )
                    best = run;

                run = next;
                if (run == stop)
                    run = NULL;
            }

#ifdef DEBUG
            foundCurrentEdge = currentEdge;
            //      tASSERT(run || currentEdge);
#endif


            if (!currentEdge && best)
            {
                if (restart >= 3)
                {
                    requestCleanup = true;
                    // give up.
                    // st_Breakpoint();
                    return start;
                }

                start = Insert(*start, best->Face());
                restart ++;
                continue;

                currentEdge = best;
            }

            if (!currentEdge)
                tERR_ERROR_INT("Point " << *start << " does not have a eFace in direction "
                               << dir << "!");
            // I know this bug happens a lot more often than"
            //  " I'd like to. Don't send a bug report!");

            restart=0;
        }

        if (start != currentEdge->Point())
            tERR_ERROR_INT("currentEdge not correct!");

        eHalfEdge *rightFromCurrent = currentEdge->next;
        eHalfEdge *leftFromCurrent  = rightFromCurrent->next;
        tASSERT (currentEdge == leftFromCurrent->next);
        // are we finished?

        if (exactly_onLeft<0)
        {
            eCoord lvec = leftFromCurrent->Vec();
            exactly_onLeft =  dir * lvec < EPS * EPS * se_EstimatedRangeOfMult(lvec, dir);
        }

        eCoord left_point_to_dest = end - *(leftFromCurrent->Point());
        eCoord lvec               = rightFromCurrent->Vec();

        REAL test = lvec * left_point_to_dest;
        if(test<0){
            // endpoint lies within current eFace; cut it into three and be finnished.

            // XXX have to check that we didnt start from a edge that jumps
            // to another field. If thats the case, then recall drawLine on the
            // other field
            goon=0;

            /*
              Should this figure get distorted, align the :

              :   C        a        B
              :   #################
              :   #\           _/#
              :   # \ cc   bb_/ #
              :   #  \     _/  #
              :   #   \  _/  ##
              : b #   D\/   #
              :   #    /  ##  c
              :   # aa/  #
              :   #  / ##
              :   # / #
              :   #/##
              :   ##
              :   A


            */

            ePoint *A=currentEdge->Point();

            if ((*A) == end){
#ifdef DEBUG
                Check();
#endif
                return A;
            }
            else{
                eHalfEdge* a=rightFromCurrent;
                eHalfEdge* b=leftFromCurrent;
                eHalfEdge* c=currentEdge;

                ePoint*    B=a->Point();
                ePoint*    C=b->Point();

                if ((*B) == end)
                {
                    c->SetWall(toBePlaced.Wall());
                    return B;
                }

                if ((*C) == end)
                {
                    tASSERT( b->other );
                    b->other->SetWall(toBePlaced.Wall());
                    return C;
                }

                if(!exactly_onLeft)
                {
                    ePoint*    D=tNEW(ePoint(end));
#ifdef DEBUG
                    Check();
#endif
                    tControlledPTR< eFace > oldFace = ZombifyFace(a->Face());

                    // no problem: D really lies within current eFace.
                    // just split it in three parts:

                    eHalfEdge *aa = tNEW(eHalfEdge) (A, D);
                    eHalfEdge *bb = tNEW(eHalfEdge) (B, D);
                    eHalfEdge *cc = tNEW(eHalfEdge) (C, D);

                    //	  aa->SetOther(   tNEW(eHalfEdge) (D));
                    //      bb->SetOther(   tNEW(eHalfEdge) (D));
                    //	  cc->SetOther(   tNEW(eHalfEdge) (D));

                    AddFaceAll(tNEW(eFace) (b,aa,cc->other, oldFace ));
                    AddFaceAll(tNEW(eFace) (bb,aa->other,c, oldFace ));
                    AddFaceAll(tNEW(eFace) (bb->other,a,cc, oldFace ));

                    aa->SetWall(toBePlaced.Wall());

#ifdef DEBUG
                    Check();
#endif
                    return D;
                }
                else // exactly_onLeft
                {
                    /*
                      Should this figure get distorted, align the ":"

                      Bad luck, D lies on eEdge b. We are going to need
                      the other triangle touching b.


                      :          C
                      :          #
                      :         ####
                      :        # #  ##
                      :       #  #    ##
                      :  e2  #   #      ##
                      :     #    #        ##  a
                      :    #   D X          ##
                      : E #      #            ##
                      :   #      # b            ##
                      :    #     #                ## B
                      :     #    #              ##
                      :     #    #            ##
                      :      #   #          ##
                      :  e1   #  #        ##
                      :       #  #      ##   c
                      :        # #    ##
                      :         ##  ##
                      :         ####
                      :          #
                      :          A

                      Make four new faces around D. 4 new half 
                      segments DA, DB, DC and DE are created.
                      :          C
                      :          #
                      :         #|##
                      :        # |  ##
                      :       #  | DC ##
                      :  e2  #   |      ##
                      :     #    | D      ##  a
                      :    #  ___X____      ##
                      : E #__/   |    \___    ##
                      :   #  DE  |     DB \___  ##
                      :    #     |            \___## B
                      :     #    | DA           ##
                      :     #    |            ##
                      :      #   |          ##
                      :  e1   #  |        ##
                      :       #  |      ##   c
                      :        # |    ##
                      :         #|  ##
                      :         #|##
                      :          #
                      :          A

                    */

                    tASSERT(b->other);

                    // eFace *G=b->other->face;

                    // tASSERT(G);

                    eHalfEdge *e1=b->other->next;
                    eHalfEdge *e2=e1->next;
                    tASSERT(e2);
                    ePoint *E=e2->point;
                    tASSERT(E);
                    // easy: edge is just right
                    if (*E == end)
                    {
                        e2->other->SetWall(toBePlaced.Wall());
                        return E;
                    }

                    // ask walls for permission to lay a parallel wall
                    //					if ( b->GetWall() && wal && !wal->RunsParallel( b->GetWall() ) )
                    //						return start;
                    //					if ( b->other->GetWall() && wal && !wal->RunsParallel( b->other->GetWall() ) )
                    //						return start;

                    // ask edge if it is splittalbe
                    if ( !b->Splittable() )
                        return start;

                    // create endpoint
                    ePoint*    D=tNEW(ePoint(end));
#ifdef DEBUG
                    Check();
#endif
                    // delete faces inside quad ABCE and connect A...E with new
                    // ePoint D.

                    tControlledPTR< eFace > oldFace_b = ZombifyFace( b->Face());
                    tControlledPTR< eFace > oldFace_bOther = ZombifyFace( b->other->Face() );

                    eHalfEdge *DC, *DA;
                    b->Split( DC, DA, D );
                    KillEdge(b);
                    DC = DC->other;

                    tASSERT( DC->Point() == D );
                    tASSERT( DA->Point() == D );
                    tASSERT( DC->other->Point() == C );
                    tASSERT( DA->other->Point() == A );

                    eHalfEdge *DE=tNEW(eHalfEdge) (D,E);
                    eHalfEdge *DB=tNEW(eHalfEdge) (D,B);
                    DA->other->SetWall(toBePlaced.Wall());

                    AddFaceAll(tNEW(eFace) (DE,e2,DA->other, oldFace_bOther ) );
                    AddFaceAll(tNEW(eFace) (DC,e1,DE->other, oldFace_bOther ) );
                    AddFaceAll(tNEW(eFace) (DB,a ,DC->other, oldFace_b ) );
                    AddFaceAll(tNEW(eFace) (DA,c ,DB->other, oldFace_b ) );

#ifdef DEBUG
                    Check();
#endif
                    return D;
                }
            }
            // finnished!
        }


        else{
            // some definitions and trivial tests:

            eHalfEdge* c = currentEdge;
            eHalfEdge* e = rightFromCurrent;
            eHalfEdge* d = leftFromCurrent;

            eFace  *F = c->Face();
            ePoint *C = c->Point();

            if (*C == end){
#ifdef DEBUG
                Check();
#endif
                return C;
            }
            //      eHalfEdge *dummy=NULL;


            ePoint *D = d->Point();

            if (*D == end){
                d->other->SetWall(toBePlaced.Wall());

#ifdef DEBUG
                Check();
#endif
                return D;
            }

            ePoint *B= e->Point();

            if (*B == end){
                c->SetWall(toBePlaced.Wall());

#ifdef DEBUG
                Check();
#endif
                return B;
            }

            // if dir is parallel with eEdge c:

            /*
              if(exactly_onLeft){
              todo->Split(d,dummy);
              delete todo;
              todo=dummy;
              start=todo->p[0];
              restart=1;
              }
               
              else
            */
            {
                if (!e->other || !e->other->Face())
                {
                    // tERR_ERROR_INT ("Left Grid!");
                    // z-man: no need for an error! We can go on!
                    // nevertheless, should you get here in the debugger, I would
                    // ask you to follow the rest of this code block carefully
                    // and send me a trace ( which lines were visited, which pointers
                    // were set ) of it.
                    st_Breakpoint();

                    // grid rift or outer rim?
                    bool rift = false;

                    if ( !e->other )
                    {
                        // all gridded edges have other correctly set. Even the outer ones.
                        rift = true;
                    }

                    // get the wall of the halfopen edge
                    eWall* wall = e->GetWall();
                    if ( !wall && e->other )
                        wall = e->other->GetWall();

                    // no wall or splittable wall? This is not the outer edge, it is a rift.
                    if ( !wall || wall->Splittable() )
                        rift = true;

                    // wall is useless when e is not complete; forgt about it.
                    if ( !e->other )
                        wall = 0;

                    if ( rift )
                    {
                        // a rift! topology police to the rescue ( after eliminating
                        // the witnesses, of course )!
                        toBePlaced.Wall()->SplitByActive( wall );
                        return start;
                    }
                    else
                    {
                        // line to be drawn hit the outer edge.
                        // should not happen either; Range() should have prevented it.
                        // grow one step anyway:
                        Grow();

                        // test again
                        if (!e->other || !e->other->Face())
                        {
                            // still no luck? a case for the topology police.
                            toBePlaced.Wall()->SplitByActive( 0 );
                            return start;
                        }
                    }
                    // z-man: Thanks for debugging, this is all I need.
                }

                {
                    eFace  *G=e->other->Face();

                    eHalfEdge* b = e->other->next;
                    eHalfEdge* a = b->next;
                    tASSERT(a->next == e->other);

                    ePoint *A = a->Point();

                    /* we have the following Situation:

                    Should this figure get distorted, align the ":"

                    :         A
                    :         #
                    :        # ##
                    :   a   #  [eFace G]
                    :      #       ##
                    :     #       X  ##  b
                    :    #   e   /     ## 
                    : D ########/######### B
                    :   #      /(new eWall)
                    :   #     /      #
                    :   #    /  [eFace F]
                    :   #   /    #
                    : d #  /   # c 
                    :   # /  #
                    :   #/ #
                    :   ##   
                    :   C
                            
                    */


                    eCoord dd=*C-*D;
                    eCoord aa=*A-*D;
                    eCoord bb=*A-*B;
                    eCoord cc=*C-*B;

                    REAL scale = aa.NormSquared() + bb.NormSquared() +
                                 bb.NormSquared() + cc.NormSquared();

                    tASSERT(scale > 0);
                    //tASSERT(aa.NormSquared() > scale *SMALL_FLOAT);
                    //tASSERT(bb.NormSquared() > scale *SMALL_FLOAT);
                    //tASSERT(dd.NormSquared() > scale *SMALL_FLOAT);
                    //tASSERT(cc.NormSquared() > scale *SMALL_FLOAT);

#ifdef DEBUG
                    Check();
#endif

                    if (change_grid && e->Movable() && aa*dd>scale*.0001 && cc*bb >scale*.0001){
                        /*

                        if the angles CDA and ABC are less than 180deg, we can 
                        just "turn" eEdge e:


                        :         A
                        :[eFace F]#
                        :        #|##  [eFace G]
                        :   a   # |  ## b
                        :      # |     ##
                        :     #  |       ##
                        :    #  |    X     ## 
                        : D #   |   |        # B
                        :   #  |   |(new eWall)
                        :   #  e  |      #
                        :   # |  |     #
                        :   # | |    #
                        : d #| |   # c 
                        :   #||  #
                        :   #| #
                        :   ##   
                        :   C

                        */
                        if (*A == *C)
                        {
                            start = A;
                            restart = 1;
                            continue;
                        }


                        tControlledPTR< eFace > oldF = ZombifyFace(F);
                        tControlledPTR< eFace > oldG = ZombifyFace(G);
                        KillEdge(e);

                        e=tNEW(eHalfEdge) (C,A);
                        F=tNEW(eFace)     (a,d,e, oldF, oldG);
                        G=tNEW(eFace)     (b,e->other,c, oldF, oldG);

                        AddFaceAll(F);
                        AddFaceAll(G);

                        eCoord ee=*A-*C;
                        REAL x_test=ee*dir;
                        if (x_test>=0){
                            exactly_onLeft=(x_test<=EPS * EPS * se_EstimatedRangeOfMult(ee,dir));
                            currentEdge = c;
                            tASSERT(currentEdge->ID >= 0);
                        }
                        else{
                            exactly_onLeft=-1;
                            currentEdge = e;
                            tASSERT(currentEdge->ID >= 0);
                        }

#ifdef DEBUG
                        Check();
#endif

                    }

                    /* if not, we hace to introduce a new ePoint E: the cut between our
                       new eWall and eEdge e.

                            
                       :        A
                       :        ##
                       :        #\##
                       :         # \###
                       :         #  \  ##
                       : [eFace G]#  \   ## [eFace GG]
                       :          #   \    ###
                       :           #   aa     ##
                       :       a   #     \      ## b
                       :            #     \       ### 
                       :            #      \   CC    ##
                       :             #      \ /        ## 
                       :           D ## dd ##E### bb #### B [BD = e]
                       :            #      /(new eWall)
                       : [eFace F] #     /      #
                       :          #   cc     # [eFace FF]
                       :         #   /    #
                       :      d #  /   # c 
                       :       # /  #
                       :      #/ #
                       :     #/#   
                       :    ##
                       :   C
                            

                    */

                    else{
                        // calculate the coordinates of the intersection ePoint E:
                        ePoint *E;
                        eHalfEdge *bb,*cc,*dd;

                        if (*C == *A)
                        {
                            start = A;
                            restart = 1;
                            continue;
                        }

                        REAL ar = .5f;

                        {
                            eHalfEdge *newtodo;
                            ePoint *CC = tNEW(ePoint(end));
                            tStackObject< eTempEdge > todo (C, CC);
                            E=tNEW(ePoint) (e->IntersectWithCareless(todo.Edge(0)));

                            ar = e->Ratio(*E);

                            //	      REAL br = todo.Edge(0)->Ratio(*E);
                            //	      tASSERT(-.1f < ar && ar < 1.1f);
                            //	      tASSERT(-.1f < br && br < 1.1f);


                            tASSERT(E->NormSquared() < se_maxGridSize*100);
                            todo.Edge(0)->Split(cc,newtodo,E);
                            KillEdge(newtodo);  // not needed. Make sure it gets deleted.
                        }

                        if (toBePlaced.Wall())
                        {
#ifdef DEBUG
                            tASSERT(toBePlaced.Wall()->Splittable());
                            Check();
#endif

                            // inform walls about the reason of their split
                            if ( !e->Movable() )
                            {
                                eWall *other = e->GetWall();
                                if ( other )
                                {
                                    toBePlaced.Wall()->SplitByActive( other );
                                }
                                other = e->other->GetWall();
                                if ( other )
                                {
                                    toBePlaced.Wall()->SplitByActive( other );
                                }
                            }

                            eWall *wa, *wb;
                            toBePlaced.Wall()->Split(wa, wb, eCoord::V(*start, *E, end));

                            // place the first bit of wall
                            cc->SetWall(wa);

                            // modify the temp edge to be placed to represent the rest of the wall
                            toBePlaced.SetWall( wb );
                            toBePlaced.Coord(0) = *E;
                        }

                        REAL arreal = ar;
                        if (ar > .99999f)
                            ar = .99999f;
                        if (ar < .00001f)
                            ar = .00001f;
                        *E = *B + (*D - *B) * ar;

                        if (*E == *D || (arreal >= 1.0f && !d->other->GetWall()))
                        {
                            d->other->SetWall(cc->GetWall());
                            KillEdge(cc);
                            start = D;
                            restart = 1;
                            continue;
                        }

                        if (*E == *B || (arreal <= 0.0f && !c->GetWall()))
                        {
                            c->SetWall(cc->GetWall());

                            KillEdge(cc);
                            start = B;
                            restart = 1;
                            continue;
                        }



                        e->other->Split(dd,bb,E);

                        F->CorrectArea();
                        G->CorrectArea();
                        tControlledPTR< eFace > oldF = ZombifyFace(F);
                        tControlledPTR< eFace > oldG = ZombifyFace(G);
                        KillEdge(e);


                        start = E;

                        bool aeq = false, ceq = false;

                        if (*A == *E)
                        {
                            if (A==leakPoint)
                                st_Breakpoint();

                            A->Replace(E);
                            KillPoint(A);

                            a->other->SetWall(dd->GetWall());
                            a->SetWall(dd->other->GetWall());
                            b->SetWall(bb->other->GetWall());
                            b->other->SetWall(bb->GetWall());
                            KillEdge(dd);
                            KillEdge(bb);
                            dd = a->other;
                            bb = b->other;
                            aeq = true;

                            se_LinkFaces( oldG, oldF );
                        }

                        if (*C == *E)
                        {
                            tASSERT(!aeq);

                            C->Replace(E);
                            KillPoint(C);

                            d->other->SetWall(dd->other->GetWall());
                            d->SetWall(dd->GetWall());
                            c->other->SetWall(bb->other->GetWall());
                            c->SetWall(bb->GetWall());

                            KillEdge(dd);
                            KillEdge(bb);
                            KillEdge(cc);
                            dd = d;
                            bb = c;
                            ceq = true;

                            se_LinkFaces( oldF, oldG );
                        }

                        if (!ceq)
                        {
                            F=tNEW(eFace) (d,cc,dd->other, oldF );
                            eFace *FF=tNEW(eFace) (c,bb->other,cc->other, oldF );

                            AddFaceAll(F);
                            AddFaceAll(FF);
                        }

                        eHalfEdge *aa=NULL;


                        if (!aeq)
                        {
                            aa = tNEW(eHalfEdge) (E,A);

                            eFace *GG=tNEW(eFace) (b,aa->other,bb, oldG );
                            G=tNEW(eFace) (a,dd,aa, oldG );
                            AddFaceAll(G);
                            AddFaceAll(GG);
                        }
                        else
                        {
                            start = E;
                            restart = true;
                            continue;
                        }

#ifdef DEBUG
                        Check();
#endif
                        if (end == *E)
                            return E;

                        //	    start = E;
                        eCoord ea=*A-*E;
                        dir   = end - *E;
                        REAL x_test=ea*dir;

                        tASSERT (aa);
                        if (x_test>=0){
                            currentEdge=bb;
                            tASSERT(currentEdge->ID >= 0);
                            exactly_onLeft=(x_test<=EPS*EPS*se_EstimatedRangeOfMult(ea,dir));
                            // if (exactly_onLeft) std::cerr << "Achtung 3!\n";
                        }

                        else{
                            currentEdge=aa;
                            tASSERT(currentEdge->ID >= 0);
                            exactly_onLeft=-1;
                        }
                    }
                }
            }
        }
    }

    tERR_ERROR_INT("We shound never get here!");
    return NULL;
}


// **************************
//          eHalfEdge
// **************************




// splitting
bool eHalfEdge::Splittable() const
{
    eWall *w = GetWall();
    if (!w && other)
        w = other->GetWall();
    return !w || w->Splittable();
}

// split eEdge at s;
void eHalfEdge::Split(eHalfEdge *& e1,eHalfEdge *& e2,ePoint *s)
{
    tASSERT(other);

    e1=tNEW(eHalfEdge) (Point(),s);
    e2=tNEW(eHalfEdge) (s,other->Point());

    if (GetWall())
    {
        if (!GetWall()->Splittable())
            tERR_ERROR_INT("I told you not to split that!");
        // first, split the eWall structure:
        eWall *w1,*w2;
        GetWall()->Split(w1,w2,Ratio(*s));

        // we use the base class' function to set walls so we don't alarm the topology police
        e1->eWallHolder::SetWall(w1);
        e2->eWallHolder::SetWall(w2);
        w1->CalcLen();
        w2->CalcLen();
    }
    if (other->GetWall())
    {
        if (!other->GetWall()->Splittable())
            tERR_ERROR_INT("I told you not to split that!");
        // first, split the eWall structure:
        eWall *w1,*w2;
        other->GetWall()->Split(w2,w1,1-Ratio(*s));

        // the topology police would crash right here.
        e1->other->eWallHolder::SetWall(w1);
        e2->other->eWallHolder::SetWall(w2);
        w1->CalcLen();
        w2->CalcLen();
    }
}


#define SIMPLIFY
#ifdef SIMPLIFY

// test if a point is movable
bool Movable( const ePoint* p )
{
    eHalfEdge* run = p->Edge();
    eHalfEdge* stop =run;

    // run through all edges from this point
    do
    {
        // one of the edges contains a wall, abort:
        if ( !run->Movable() )
            return false;

        // advance to next edge from this point
        run = run->Other();
        if ( run )
            run = run->Next();
    }
    while ( run && stop != run );

    // no obstacles found.
    return true;
}


// test how far you can move a point in a certain direction without giving the
// face adjacent to the given half-edge negative area
void ClampMovement( const ePoint* p, eCoord& dir, const eHalfEdge* e )
{
    eHalfEdge* next = e->Next();
    // get two of the side vectors of the face
    eCoord oppositeEdge = next->Vec();
    eCoord adjacentEdge = e->Vec();

    // calculate current surface area
    REAL area = oppositeEdge * adjacentEdge;

    // calculate the influence of the proposed movement on the surface area
    REAL areaChange = dir * oppositeEdge;

    // allow it if the change increases the area
    if ( areaChange >= 0 )
        return;

    // allow it if the change keeps the area positive
    if ( area + areaChange >= 0 )
        return;

    // if the current surface area is already negative, forbid the change
    if ( area < 0 )
    {
        dir = eCoord(0,0);
        return;
    }

    // clamp the desired movement
    REAL clampFactor = - area / areaChange;
    tASSERT( clampFactor >=0 && clampFactor <= 1 );

    // clamp
    dir = dir * clampFactor;
}

// test how far you can move a point in a certain direction without giving any
// of the adjacent faces a negative area
void ClampMovement( const ePoint* p, eCoord& dir )
{
    eHalfEdge* run = p->Edge();
    eHalfEdge* stop =run;

    // run through all edges from this point
    do
    {
        // get the max mobility of the edge
        ClampMovement( p, dir, run );

        // advance to next edge from this point
        run = run->Other();
        if ( run )
            run = run->Next();
    }
    while ( run && stop != run );
}

bool eFace::CorrectArea()
{
    REAL area = edge->next->Vec() * edge->Vec();
    if (area >= 0)
        return false;

    eHalfEdge *run = edge;

    // find the longest edge ( preferably with wall )
    eHalfEdge *longest = NULL;
    REAL       length  = -1;
    bool       wall    = false;

    for (int i=2; i>=0; i--)
    {
        REAL runLength   = run->Vec().NormSquared();

        // test if moving the point opposite to run would move walls
        if (!Movable( run->next->next->Point() ) )
            continue;

        bool runWall     = !run->Movable();
        if ( runLength > length || runWall )
        {
            // abort if the triangle has two walled sides
            if ( wall && runWall )
            {
                // however, this should never, ever happen.
                st_Breakpoint();
                return false;
            }

            wall    = runWall;
            length  = runLength;
            longest = run;
        }
        run = run->next;
    }

    // no luck?
    if ( !longest )
        return false;

    run = longest;
    eCoord &A = *run->Point();
    run       =  run->next;
    eCoord &B = *run->Point();
    run       =  run->next;
    eCoord &C = *run->Point();
    ePoint* pC = run->Point();

    // now, C is the point opposite to the longest edge.
    eCoord MoveDir = (B-A);
    MoveDir = MoveDir.Turn(0, -area * 1.01/MoveDir.NormSquared() );

    // move C as little as possible into the desired direction
    if ( MoveDir.NormSquared() > 0 )
    {
        eCoord Cnew = C + MoveDir;

        while ( Cnew.x == C.x && Cnew.y == C.y )
        {
            // try displacing by MoveDir and enlarge MoveDir
            Cnew = C + MoveDir;
            MoveDir = MoveDir * 2;
        }

        // clamp the movement so it does not make the situation worse
        // for other faces
        ClampMovement( pC, MoveDir );

        // store result
        C = C + MoveDir;
    }
    else // give up
        return false;

    REAL newarea = edge->next->Vec() * edge->Vec();
    tASSERT(newarea >= area);

    // check if treatment was successful
    return newarea > area;
}

bool eHalfEdge::Simplify(eGrid *grid)
{
    if (GetWall() && GetWall()->Deletable() && face)
    {
        ClearWall();
    }

    if (other && other->GetWall() && other->GetWall()->Deletable() && other->face)
    {
        other->ClearWall();
    }

    if (GetWall() || !other || other->GetWall() || !face || !other->face)
        return false;

    ePoint  *to_be_removed=point; // we'll remove this ePoint
    ePoint  *stay=other->point;   // and redirect all his edges to this ePoint

    // first, we find the faces that are going to be modified:
    // they are neighbours of the ePoint we'll remove.

    eHalfEdge *run = this->other->next;
    do{
        //    eFace *F = run->face; // eFace to be modified
        tASSERT(run->other);




        /*




          C  this
          A ------------ --------------- stays
          Next    |
          | 
          |r 
          |u   
          |n    
          |     
          |
          B




        */


        // if any of the to-be-modified edges is a eWall, abort the operation.
        if (run->GetWall() || run->other->GetWall() || !run->face)
            return false;

        eHalfEdge *Next = run->other->next;
        eCoord &A = *Next->other->point;
        eCoord &B = *run->other->point;
        eCoord &C = *stay;
#ifdef DEBUG
        //    eCoord &D = *to_be_removed;
        //    eCoord leftvec_b  = B-D;
        //    eCoord rightvec_b = A-D;
        //    REAL orientation_b=leftvec_b*rightvec_b;
#endif


        // if the eFace is to be modified (and not deleted), check
        // if it will flip orientation; if yes, abort.

        eCoord leftvec = B-C;
        eCoord rightvec= A-C;

        REAL orientation=leftvec*rightvec;

        if (orientation <= 0)
            return false;

        // test if the face contains a gameobject or is otherwise referenced; abort if that is the case
        // four references are nominal: three from the edges, one from the grid
        if ( !Next->face || Next->face->GetRefcount() > 4 )
            return false;

        //    tASSERT(orientation_b > 0);

        run = Next;
    }
    while (run && run->other &&  !(this == run->other->next));

    // dangerously close to the rim....
    if (!run || !run->other)
        return false;

    // Allright! everything is safe.
#ifdef DEBUG
    grid->Check();
#endif

    eHalfEdge *a  = next;
    eHalfEdge *aa = a->next;
    ePoint *A     = aa->Point();

    eHalfEdge *b  = other->next;
    eHalfEdge *bb = b->next;
    ePoint *B     = bb->Point();

    if (a->GetWall() || a->other->GetWall())
        return false;

    if (aa->GetWall() || aa->other->GetWall())
        return false;

    if (b->GetWall() || b->other->GetWall())
        return false;

    if (bb->GetWall() || bb->other->GetWall())
        return false;

    tControlledPTR< eHalfEdge > keep( this );

    int idrec = ID;
    tRecorderSync< int >::Archive( "_GRID_SIMPLIFY_REAL", 8, idrec );

    // the faces that will be removed
    tControlledPTR< eFace > killed1 = grid->ZombifyFace( face );
    tControlledPTR< eFace > killed2 = grid->ZombifyFace( other->face );

    // all faces that will replace them get added to their replacement list
    run = this->other->next->other->next;
    do{
        tASSERT(run->other);

        //       eHalfEdge *Next = run->other->next;
        eFace *F = run->face; // eFace to be modified
        tASSERT( F != killed1 );
        tASSERT( F != killed2 );
        if( F )
        {
            se_LinkFaces( killed1, F );
            se_LinkFaces( killed2, F );
        }

        run = run->other->next;
    }
    while (run && run->other &&  !(this == run));

    stay->Replace(to_be_removed);

    // kill everything that goes away:
    //	grid->KillFace( killed1 );
    //	grid->KillFace( killed2 );
    grid->KillPoint(stay);

    grid->KillEdge(this);

    to_be_removed->edge = aa->other;
    A            ->edge = a->other;
    B            ->edge = b->other;

    // sew the edges of the faces that are going to disappear together:
    a->other->SetOther(aa->other);
    b->other->SetOther(bb->other);

    grid->KillEdge(a);
    grid->KillEdge(aa);
    grid->KillEdge(b);
    grid->KillEdge(bb);

    return true;
}

void eGrid::SimplifyNum(int e){
#ifdef DEBUG
    Check();
#endif

    if (e>=0 && e < edges.Len()){
        eHalfEdge *E=edges(e);
#ifdef DEBUG
        eHalfEdge *Esave=E;
#endif
        if ( !E || !E->other )
            return;

        // this point will be removed. Make sure it is none of the points on the outer rim.
        ePoint * toBeRemoved = E->other->Point();
        if ( toBeRemoved == A || toBeRemoved == B || toBeRemoved == C )
            return;

        tRecorderSync< int >::Archive( "_GRID_SIMPLIFY", 8, e );

        if ( E->Simplify(this) )
        {
            se_simplifyEdges += se_simplifyEdgesSuccess;
        }

#ifdef DEBUG
        tASSERT(Esave);

        Check();
#endif
    }
}

#endif // SIMPLIFY

void eGrid::SimplifyAll(int count){
    if (requestCleanup)
    {
        requestCleanup = false;
        for (int i=faces.Len()-1; i>=0; i--)
            requestCleanup |= faces(i)->CorrectArea();
    }


    static int counter=0;

    // try to simplify at least one edge
    se_simplifyEdges++;

    for(;count>0 && se_simplifyEdges>0;count--,se_simplifyEdges--){
        counter--;

        if (counter<0 || counter>=edges.Len())
            counter=edges.Len()-1;

        SimplifyNum(counter);
    }
}



eGrid *currentGrid = NULL;

eGrid* eGrid::CurrentGrid(){return currentGrid;}

void eGrid::Create(){
    currentGrid = this;

    if (!A)
        Clear();

    base=eCoord(20,100);

#ifdef DEBUG
    doCheck = true;
#endif
    requestCleanup = false;


    A=tNEW(ePoint) (base.Turn(1,0));
    B=tNEW(ePoint) (base.Turn(-.5,.87));
    C=tNEW(ePoint) (base.Turn(-.5,-.87));

    a=tNEW(eHalfEdge) (B,C,tNEW(eWall(this)));
    b=tNEW(eHalfEdge) (C,A,tNEW(eWall(this)));
    c=tNEW(eHalfEdge) (A,B,tNEW(eWall(this)));

    a->other->next = c->other;
    a->other->prev = b->other;
    b->other->next = a->other;
    b->other->prev = c->other;
    c->other->next = b->other;
    c->other->prev = a->other;

    AddFaceAll(tNEW(eFace) (a,b,c));

    maxNormSquared=base.NormSquared();

#ifdef DEBUG
    Check();
#endif

}


void eGrid::Clear(){
    eHalfEdge::ClearPathData();

    eGameObject::DeleteAll( this );

    base=eCoord(0,100);
    maxNormSquared=0;

    int i;
    for(i=faces.Len()-1; i>=0; i--)
    {
        faces(i)->Unlink();
        RemoveFace(faces(i));
    }
    for(i=points.Len()-1; i>=0; i--)
    {
        points(i)->Unlink();
        RemovePoint(points(i));
    }
    for(i=edges.Len()-1; i>=0; i--)
    {
        edges(i)->Unlink();
        RemoveEdge(edges(i));
    }


    A=B=C=NULL;
    a=b=c=NULL;

    //	se_faceReplacements.clear();
}

void eGrid::Grow()
{
#ifdef DEBUG
    Check();
#endif

    if ( maxNormSquared > se_maxGridSize )
        return;

    base = base.Turn(-4,0);
    maxNormSquared=base.NormSquared();

    a->ClearWall();
    b->ClearWall();
    c->ClearWall();

    ePoint *newA=tNEW(ePoint) (base.Turn(1,0));
    ePoint *newB=tNEW(ePoint) (base.Turn(-.5,.87));
    ePoint *newC=tNEW(ePoint) (base.Turn(-.5,-.87));

    eHalfEdge *newa=tNEW(eHalfEdge) (newB,newC,tNEW(eWall(this)));
    eHalfEdge *newb=tNEW(eHalfEdge) (newC,newA,tNEW(eWall(this)));
    eHalfEdge *newc=tNEW(eHalfEdge) (newA,newB,tNEW(eWall(this)));

    eHalfEdge *AnewB=tNEW(eHalfEdge) (A,newB,NULL);
    eHalfEdge *AnewC=tNEW(eHalfEdge) (A,newC,NULL);
    eHalfEdge *BnewA=tNEW(eHalfEdge) (B,newA,NULL);
    eHalfEdge *BnewC=tNEW(eHalfEdge) (B,newC,NULL);
    eHalfEdge *CnewA=tNEW(eHalfEdge) (C,newA,NULL);
    eHalfEdge *CnewB=tNEW(eHalfEdge) (C,newB,NULL);

    AddFaceAll(tNEW(eFace) (a->other,BnewA,CnewA->other));
    AddFaceAll(tNEW(eFace) (b->other,CnewB,AnewB->other));
    AddFaceAll(tNEW(eFace) (c->other,AnewC,BnewC->other));

    AddFaceAll(tNEW(eFace) (newa,AnewC->other,AnewB));
    AddFaceAll(tNEW(eFace) (newb,BnewA->other,BnewC));
    AddFaceAll(tNEW(eFace) (newc,CnewB->other,CnewA));

    a=newa;
    b=newb;
    c=newc;

    A=newA;
    B=newB;
    C=newC;

    a->other->next = c->other;
    a->other->prev = b->other;
    b->other->next = a->other;
    b->other->prev = c->other;
    c->other->next = b->other;
    c->other->prev = a->other;

#ifdef DEBUG
    Check();
#endif
}

void eGrid::Range(REAL NormSquared){
    if (A==NULL)
        Create();
    //    tERR_ERROR_INT("Wanted to insert something into the nonexistant grid!");

    // the magic factor of 4 is chosen so that no points can be added outside of the
    // current tiangle abc added by Grow().
    while (NormSquared * 4 > maxNormSquared && maxNormSquared < se_maxGridSize )
        Grow();
}

void eTempEdge::CopyIntoGrid(eGrid *grid)
{
    if (Point(0) != Point(1)){
        ePoint *newp = grid->Insert(*Point(0));
        tASSERT( newp );
        eWall *w = Edge(0)->GetWall();
        w->Insert();
        tASSERT(w->Splittable());
        //    Edge(0)->GetWall() = NULL;
        //    w->edge       = NULL;
        //    Edge(0)->SetWall(NULL);
        grid->DrawLine(newp, *Point(1), w);
    }
}

eTempEdge::eTempEdge(const eCoord& a, const eCoord &b, eWall *w){
    halfEdges[0] = tNEW(eHalfEdge(tNEW(ePoint)(a),tNEW(ePoint)(b),w));
    halfEdges[1] = halfEdges[0]->Other();
}

eTempEdge::eTempEdge(ePoint *a, ePoint *b, eWall *w){
    halfEdges[0] = tNEW(eHalfEdge(a, b ,w));
    halfEdges[1] = halfEdges[0]->Other();
}



eTempEdge::~eTempEdge()
{
    halfEdges[0]->Unlink();
}




eHalfEdge::eHalfEdge(ePoint *p):ID(-1),next(NULL), prev(NULL), other(NULL)
{
    origin_ = PATH_NONE;

    point = p;
    face  = 0;
}


eHalfEdge::eHalfEdge(ePoint *a, ePoint *b, eWall *w)
        :ID(-1),next(NULL), prev(NULL), other(NULL)
{
    tASSERT( a !=  b);

    origin_ = PATH_NONE;

    SetPoint(a);
    SetOther(new eHalfEdge(b));
    SetWall(w);
}

// get the intersection point of two edges:
// stupid intersection without checks; returned point
// does not allways lie within the two edges
eCoord eHalfEdge::IntersectWithCareless(const eHalfEdge *e2) const
{
    eCoord vec  = *other->Point()     -     *Point();
    eCoord e2vec= *e2->other->Point() - *e2->Point();
    eCoord e2_t = *Point()     - *e2->Point();

    REAL inv = (vec*e2vec);
    if (fabs(inv) > EPS * se_EstimatedRangeOfMult( vec, e2vec ) )
    {
        // return intersection poin
        return (*Point()-vec*((e2_t*e2vec)/inv));
    }
    else
    {
        // lines lie almost parallel; return "inverse center of gravity" ( the swapped
        // weighting is intentional, the smaller line should have a bigger influence )
        REAL m1 = vec.Norm();
        REAL m2 = e2vec.Norm();
        if ( m1 + m2 <= 0 ) // sanity check weights
            m1=m2=1;
        return ((*Point() + *other->Point())*m2 + (*e2->Point() + *e2->other->Point())*m1)*(.5/(m1+m2));
    }
}

// the same, but with checks: if the two edges don't intersect,
// NULL is returned.
ePoint * eHalfEdge::IntersectWith(const eHalfEdge *e2) const
{
    eCoord vec  = *other->Point()     -     *Point();
    eCoord e2vec= *e2->other->Point() - *e2->Point();
    eCoord e2_t = *Point()     - *e2->Point();

    REAL div=(vec*e2vec);

    if (div==0) return NULL;

    eCoord ret=*Point()-vec*((e2_t*e2vec)/ div);


    REAL v=Ratio(ret);
    if (v<0 || v>1) return NULL;
    v=e2->Ratio(ret);
    if (v<0 || v>1) return NULL;

    return tNEW(ePoint) (ret);

}

// inserts an eEdge into the grid
void eHalfEdge::CopyIntoGrid(eGrid *grid, bool change_grid){
    tASSERT(other);
    ePoint *start = grid->Insert(*Point());
    grid->DrawLine(start, *other->Point(), GetWall(), change_grid);
}

//bool eHalfEdge::Simplify(int dir);
// try to get rid of this eEdge; return value: can we delete it?


bool eHalfEdge::Check() const{
#ifdef DEBUG
    if (other && other->next)
        tASSERT(other->next->Point() == Point());
    if (next)
        tASSERT(next->Face() == Face());
    if (next)
        tASSERT(this == next->prev);
    if (prev)
        tASSERT(this == prev->next);

    if (ID>=0)
    {
        tASSERT(!Point() || Point()->ID >=0);
        tASSERT(!Face()  || Face()->ID  >=0);
        tASSERT(!next    || next->ID    >=0);
        tASSERT(!prev    || prev->ID    >=0);
        tASSERT(!other   || other->ID   >=0);
    }
#endif

    // only the three outer edges are allowed to have missing faces
    if ( !face )
    {
        eWall* wall = GetWall();
        if ( !wall && other )
            wall = other->GetWall();
        tASSERT( wall );

        tASSERT( !wall->Splittable() );
        // the outer walls are unsplittable ( and right now are the only unsplittable
        // walls; all real walls need to be split sometimes when laid out. )
    }

    return true;
}



void eGrid::AddFace    (eFace     *f)
{
    if (f->ID >= 0)
        return;

    faces.Add(f, f->ID);

    if (f->CorrectArea())
        requestCleanup = true;
}

void eGrid::RemoveFace (eFace     *f)
{
    if (f->ID < 0)
        return;

    faces.Remove(f, f->ID);
}

void eGrid::AddEdge    (eHalfEdge *e)
{
    if (e->ID >= 0)
        return;

    tASSERT(e->Other() && *e->Point() != *e->Other()->Point());

    edges.Add(e, e->ID);

    int idrec = e->ID;
    tRecorderSync< int >::Archive( "_GRID_ADD_EDGE", 8, idrec );
}

void eGrid::RemoveEdge (eHalfEdge *e)
{
    if (e == leakEdge)
        st_Breakpoint();

    if (e->ID < 0)
        return;

    int idrec = e->ID;
    tRecorderSync< int >::Archive( "_GRID_REMOVE_EDGE", 8, idrec );

    edges.Remove(e, e->ID);
}

void eGrid::AddPoint   (ePoint    *p)
{
    if (p->ID >= 0)
        return;

    points.Add(p, p->ID);

#ifdef DEBUG_DISTORT
    // debug facility: distort grid
    static eCoord distortion(1,0);
    distortion = distortion.Turn(.6,.8);
    *p = *p + distortion;
#endif
}

void eGrid::RemovePoint(ePoint    *p)
{
    if (p == leakPoint)
        st_Breakpoint();

    if (p->ID < 0)
        return;

    points.Remove(p, p->ID);
}

void eGrid::KillFace (eFace*      f)
{
    tControlledPTR< eFace > keep( f );
    RemoveFace(f);
    f->Unlink();
}

tControlledPTR< eFace > eGrid::ZombifyFace (eFace*      f)
{
    tControlledPTR< eFace > keep( f );
    RemoveFace(f);
    f->Unlink();
    return keep;
}

void eGrid::KillEdge (eHalfEdge*  e)
{
    tControlledPTR< eHalfEdge > keep( e );
    RemoveEdge(e);
    tControlledPTR< eHalfEdge > o ( e->other );

    e->Unlink();

    if (o)
    {
        KillEdge(o);
    }
}

void eGrid::KillPoint(ePoint*     p)
{
    tControlledPTR< ePoint > keep( p );
    RemovePoint(p);
    p->Unlink();
}

void eGrid::AddFaceAll (eFace     *f)
{
    eHalfEdge *run = f->edge;
    for(int j=0;j<=2;j++){
        AddEdge(run);
        AddEdge(run->other);
        AddPoint(run->Point());
        run = run->next;
    }
    tASSERT (run == f->edge);

    AddFace(f);
}



ePoint * eGrid::Insert(const eCoord& x, eFace *start){
#ifdef DEBUG
    Check();
#endif
    Range(x.NormSquared());

    eFace *f = FindSurroundingFace(x, start);
    if (!f)
        return NULL;

    /*
      for(int i=faces.Len()-1;i>=0;i--)
      {
         eFace *f = faces(i);
         if (f->IsInside(x)){

    */

    eHalfEdge *run = f->edge;
    int j;
    for(j=0;j<=2;j++){
        if (*run->Point() == x)
            return run->Point();
        run = run->next;
    }
    tASSERT (run == f->edge);

#ifdef DEBUG
    // check again ( possibly with breakpoint )
    for(j=0;j<=2;j++){
        if (*run->Point() == x)
            return run->Point();
        run = run->next;
    }
    tASSERT (run == f->edge);
#endif

    return DrawLine(f->edge->Point(), x);
    /*
      break;
         }
      }
      return NULL; 
    */
}

const eCoord se_zeroCoord(0,0);


eFace * eGrid::FindSurroundingFace(const eCoord &pos, eFace *currentFace) const{
    if (faces.Len()<1)
        return NULL;

    if (!currentFace || currentFace->ID < 0 || currentFace->ID >= faces.Len() || faces(currentFace->ID) != currentFace)
        currentFace=faces(0);

    int timeout=faces.Len()+2;

    while (currentFace && timeout >0 && !currentFace->IsInside(pos)){
        timeout--;

        eHalfEdge *run   = currentFace->Edge(); // runs through all edges of the face
        eHalfEdge *best  = NULL;                // the best face to leave
        eHalfEdge *first = NULL;
        eHalfEdge *in    = NULL;
        REAL bestScore   = 0;


        // look for the best way out
        while(run != first)
        {
            if (run == in)
            {
                run = run->Next();
                continue;
            }

            if ( run->Other() && run->Other()->Face() )
            {
                eCoord vec=pos - (*run->Point()); // the vector to our destination

                REAL score = run->Vec() * vec;

                if (score > bestScore)
                {
                    best      = run;
                    bestScore = score;
                }
            }

            if (!first)
                first = run;

            run = run->Next();
        }

        if (best)
        {
            in = best->Other();
            tASSERT(in);

            currentFace = in->Face();
            timeout--;
        }
        else
        {
            timeout = 0;
            //			st_Breakpoint();
        }
    }

    if (timeout<=0){ // normal way failed
#ifdef DEBUG
        con << "WARNING! FindSurroundingFace failed.\n";
#endif
        // do it the hard way:
        for(int i=faces.Len()-1;i>=0;i--)
            if(faces(i)->IsInside(pos)){
                currentFace=faces(i);
                i=-1;
            }
    }

    return currentFace;
}

void eGrid::AddGameObjectInteresting    (eGameObject *o){
    gameObjectsInteresting.Add(o, o->interestingID);
}

void eGrid::RemoveGameObjectInteresting (eGameObject *o){
    gameObjectsInteresting.Remove(o, o->interestingID);
}

void eGrid::AddGameObjectInactive       (eGameObject *o){
    gameObjectsInactive.Add(o, o->inactiveID);
}

void eGrid::RemoveGameObjectInactive    (eGameObject *o){
    gameObjectsInactive.Remove(o, o->inactiveID);
}


/*

void eEdge::calc_Len() const{
  REAL len2=(*p[0] - *p[1]).NormSquared();
  *reinterpret_cast<REAL *>(reinterpret_cast<void *>(&len))=sqrt(len2);
}


void ePoint::InsertIntoGrid(){
  if (f.Len()==0){
    for(int i=eFace::faces.Len()-1;i>=0;i--)
      
      if (eFace::faces(i)->IsInside(*this)){
	for(int j=0;j<=2;j++){
	  if (*eFace::faces(i)->p[j]==*this){
	    ePoint *same=eFace::faces(i)->p[j];
	    same->Replace(this);
	    delete same;
	    return;
	  }
	}
	eFace::faces(i)->p[0]->DrawLineTo(this);
	return;
      }
  }
}



      eHalfEdge *run  = start->edge;
      tASSERT(run);
      const eHalfEdge *stop = run;

      do
	{
	  
	  run = run->other->next;
	}
      while (stop != start);

*/


eGrid::eGrid()
        :	 requestCleanup(false),
        A(NULL), B(NULL), C(NULL),
        a(NULL), b(NULL), c(NULL),
        maxNormSquared(-1),
        base(100,100)
{
    currentGrid = this;
}


eGrid::~eGrid()
{
    if (currentGrid == this)
        currentGrid = NULL;
}

static REAL s_rangeSquared;
static eFace *s_start = NULL;
static eFace *s_processed = NULL;

// call WallProcessor for all walls around pos
static void ProcessWallsRecursive		( eGrid::WallProcessor* 	proc,
                                     const eCoord&				pos	,
                                     REAL						range,
                                     eFace*					start,
                                     eFace*					from,
                                     int						depthLeft )
{
    tASSERT( start );

    if ( depthLeft < 0 )
        return;

    if ( start->nextProcessed )
        return;

    start->nextProcessed = s_processed;
    s_processed = start;


    eHalfEdge* leave = start->Edge();
    for ( int i = 2; i>=0; --i )
    {
        if ( leave->Other() && leave->Other()->Face() )
        {
            eCoord normal = leave->Vec().Conj();
            REAL normalLen = normal.Norm();
            if ( normalLen <= 0 )
                continue;
            normal *= 1/ normalLen;

            eCoord Pos1 = normal.Turn( *leave->Point() - pos );
            eCoord Pos2 = normal.Turn( *leave->Other()->Point() - pos );

            tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y ) + fabs ( Pos2.y ) + .1f * .1f );

            // start test: the line through the edge "leave" must come closer than "range" to "pos"
            // and we have to be on the correct side
            if ( Pos1.y > -range && Pos1.y <= range*.01f )
            {
                // check for real hit
                if ( Pos1.x * Pos2.x < 0 ||
                        Pos1.NormSquared() < s_rangeSquared ||
                        Pos2.NormSquared() < s_rangeSquared )
                {
                    // recurse
                    if ( NULL == leave->Other()->Face()->nextProcessed )
                        ProcessWallsRecursive( proc, pos, range, leave->Other()->Face(), start , depthLeft - 1 );

                    // process this wall
                    if ( leave->GetWall() )
                        (*proc) ( leave->GetWall() );
                    if ( leave->Other()->GetWall() )
                        (*proc) ( leave->Other()->GetWall() );
                }
            }
        }
        leave = leave->Next();
    }
}

// call WallProcessor for all walls around pos
void eGrid::ProcessWallsInRange		( WallProcessor* 	proc,
                                   const eCoord&		pos	,
                                   REAL				range,
                                   eFace*			start )
{
    int i;

    start = FindSurroundingFace( pos, start );
    if ( !start )
        return;

    s_rangeSquared = range * range;
    s_start = start;
    s_processed = start;

    // process the gridded walls
    ProcessWallsRecursive( proc, pos, range, start, NULL, 100 );

#ifdef DEBUG_OLD
    // make sure all walls were processed

    // process the not gridded walls
    for ( i = edges.Len()-1; i>=0; --i )
    {
        const eHalfEdge *leave = edges(i);

        if ( !leave->Other() || !leave->Face() || !leave->Other()->Face() )
            continue;

        eCoord normal = leave->Vec().Conj();
        REAL normalLen = normal.Norm();
        if ( normalLen <= 0 )
            continue;
        normal *= 1/ normalLen;

        eCoord Pos1 = normal.Turn( *leave->Point() - pos );
        eCoord Pos2 = normal.Turn( *leave->Other()->Point() - pos );

        tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y + Pos2.y +.1f ) * .1f );

        // start test: the line through the edge "leave" must come closer than "range" to "pos"
        if ( Pos1.y < range && Pos1.y > -range )
        {
            // check for real hit
            if ( Pos1.x * Pos2.x < 0 ||
                    Pos1.NormSquared() < s_rangeSquared ||
                    Pos2.NormSquared() < s_rangeSquared )
            {
                if ( !leave->Face()->nextProcessed || !leave->Other()->Face()->nextProcessed )
                {
                    st_Breakpoint();
                }

                if ( leave->Face()->nextProcessed &&  !leave->Other()->Face()->nextProcessed )
                {
                    while ( s_processed != start )
                    {
                        eFace* next = s_processed->nextProcessed;
                        s_processed->nextProcessed = NULL;
                        s_processed = next;
                    }

                    start->nextProcessed = NULL;
                    s_processed = leave->Face();
                    leave->Face()->nextProcessed = start;

                    st_Breakpoint();

                    ProcessWallsRecursive( proc, pos, range, leave->Face(), NULL, 100 );

                    while ( s_processed != start )
                    {
                        eFace* next = s_processed->nextProcessed;
                        s_processed->nextProcessed = NULL;
                        s_processed = next;
                    }

                    start->nextProcessed = NULL;
                    s_processed = NULL;

                }
            }
        }
    }
#endif

    while ( s_processed && s_processed != start )
    {
        eFace* next = s_processed->nextProcessed;
        s_processed->nextProcessed = NULL;
        s_processed = next;
    }

    start->nextProcessed = NULL;
    s_processed = NULL;

    // process the not gridded walls
    for ( i = wallsNotYetInserted.Len()-1; i>=0; --i )
    {
        eWall* w= wallsNotYetInserted(i);
        const eHalfEdge *leave = w->Edge();

        eCoord normal = leave->Vec().Conj();
        REAL normalLen = normal.Norm();
        if ( normalLen <= 0 )
            continue;
        normal *= 1/ normalLen;

        eCoord Pos1 = normal.Turn( *leave->Point() - pos );
        eCoord Pos2 = normal.Turn( *leave->Other()->Point() - pos );

        tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y + Pos2.y +.1f ) * .1f );

        // start test: the line through the edge "leave" must come closer than "range" to "pos"
        if ( Pos1.y < range && Pos1.y > -range )
        {
            // check for real hit
            if ( Pos1.x * Pos2.x < 0 ||
                    Pos1.NormSquared() < s_rangeSquared ||
                    Pos2.NormSquared() < s_rangeSquared )
            {
                (*proc) ( w );
            }
        }

    }
}

