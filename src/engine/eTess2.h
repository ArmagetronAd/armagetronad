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

#ifndef ArmageTron_TESS_H
#define ArmageTron_TESS_H

#include "tMemManager.h"
#include "tList.h"
#include "eCoord.h"
#include "tHeap.h"
#include <iostream>
#include "eWall.h"

extern int se_debugExt;

class ePoint;
//class eEdge;
class eFace;
class eWall;
class eGameObject;
class ePath;
class eHalfEdge;
class eGrid;

// **********************************************************
// * eDual: common base class for points and faces;
// *         they are in a duality relation.
// **********************************************************

class eDual{
    friend class eHalfEdge;
    friend class eGrid;
    friend class eFace;
    friend class ePoint;
    //    friend class eGameObject;

public:
    eDual();    // empty constructor

    eHalfEdge *Edge()const {return edge;}

    bool IsInGrid() const {return ID >= 0;}
protected:
    ~eDual();  // destructor

    bool Check(int type);    // basic consistency check; type is 0 for points and 1 for faces.
    void Unlink(int type);   // removes itself from the edges' pointers. type is 0 for points and 1 for edges.

    int                             ID;           // list identification
    tJUST_CONTROLLED_PTR<eHalfEdge> edge;         // one of the edges that begin at this vertex/is a border of this edge
};

// information about from where the path came in pathfinding
typedef enum { PATH_NEXT=0, PATH_PREV=1, PATH_OTHER_NEXT=2, PATH_PREV_OTHER=3,
               PATH_START=4, PATH_CLOSED=8, PATH_CLOSED_START=12, PATH_NONE=16}
ePATH_ORIGIN;

class eHalfEdge: public tHeapElement, public eWallHolder, public tReferencable< eHalfEdge > {
    friend class eDual;
    friend class ePoint;
    friend class eFace;
    friend class eGrid;
    friend class eTempEdge;
    friend class eWall;
    friend class tReferencable< eHalfEdge >;

    ~eHalfEdge();                  // destructor unlinking all pointers
    eHalfEdge(ePoint *p = NULL);            // empty constructor
public:
    eHalfEdge(ePoint *a, ePoint *b,eWall *w=NULL);   // full line constructor

    // wall management
    //  eWall *Wall()const {return wall;}
    void SetWall(eWall *w);
    void ClearWall( void );

    void SetOther(eHalfEdge *e)
    {
        tASSERT(e != this);

        if (e == other)
            return;

        if (other)
            other->other = NULL;

        if (e->other)
            e->other->other = NULL;

        e->other = this;
        other = e;
    }

    // completely delete this edge and its other half
    void Destroy()
    {
        tJUST_CONTROLLED_PTR< eHalfEdge > holder( this );
        Unlink();
    }

    // assuming c is on this line: how far is it in direction of the endpoint?
    REAL Ratio(const eCoord &c) const;

    // the vector from beginning to end
    eCoord Vec() const;

    // are we allowed to replace it?
    bool Movable() const{return !GetWall() && (!other || !other->GetWall());}

    // splitting
    bool Splittable() const;

    // split eEdge at s;
    void Split(eHalfEdge *& e1,eHalfEdge *& e2,ePoint *s);

    // does the same, but guarantees that first lies in e1.
    //  void SplitOriented(eHalfEdge *& e1,eHalfEdge *& e2,ePoint *s,ePoint *first);

    // different split along an eEdge::
    // this:   XXXXXWWWWZZZZWWWWWWWWWW
    // e:      ...........

    // after:
    // e1:                ZZWWWWWWWWWW
    // e:      XXXXXWWWWZZ
    // void Split(eHalfEdge * e,eHalfEdge *& e1);


    // get the intersection point of two edges:
    // stupid intersection without checks; returned point
    // does not allways lie within the two edges
    eCoord IntersectWithCareless(const eHalfEdge *e2) const;

    // the same, but with checks: if the two edges don't intersect,
    // NULL is returned.
    ePoint * IntersectWith(const eHalfEdge *e2) const;

    // inserts an eEdge into the grid
    void CopyIntoGrid(eGrid *grid, bool change_grid=1);

    bool Simplify(eGrid *grid);
    // try to get rid of this eEdge; return value: can we delete it?

    // I/O:
    void Print(std::ostream &s) const;

    /*
    // mark this as edge of vision
    void MarkCheckVis(int i);
    static void MarkCheckVisAll(int i);

    // checks the visibility; return value: do we need to check
    // it again in the near future?
    bool UpdateVis(int i);

    // checks all edges on visibility for viewer i
    static void UpdateVisAll();
    static void UpdateVisAll(int viewer);



    eFace * NearerToViewer(int i);    // which of the neighbours is
    // closer to viewer i?
    //static void check_vis(REAL time); // checks visibility of all edges

    static void SeethroughHasChanged(); // some eWalls changed their 
    // blocking height; check that!

    */
    // member inquiry:
    ePoint*    Point() const;
    eFace*     Face () const;
    eHalfEdge* Other() const {return other;}
    eHalfEdge* Next() const  {return next;}


    // pathfinding interface: find a path for gameobject from origin to target.
    static void FindPath(const eCoord& originPoint, const eFace* originFace,
                         const eCoord& targetPoint, const eFace* targetFace,
                         const eGameObject* gameObject,
                         ePath& path);

    static void ClearPathData();

    eWall* CrossesNewWall(const eGrid *grid) const; // check whether this edge
    // crosses any of the brand-new walls on the grid
protected:

    // pathfinding temporary data:
    ePATH_ORIGIN origin_; // the origin of the path

    REAL MinPathLength(){ return Val(); }   // the minimum length of a path through this edge
    void SetMinPathLength( REAL length, tHeapBase& heap, ePATH_ORIGIN origin );

    void PossiblePath( ePATH_ORIGIN origin, REAL minLength ); // tell the pathfinder that there might be a path of total length minLength through this HalfEdge, coming from the given origin type.

    virtual tHeapBase *Heap() const; // pathfinding heap

    void Unlink();          // remove us from all lists
    bool Check() const;     // consistency check

    // member manipulation
    void SetPoint(ePoint *p);

    void SetFace(eFace *p);

    //  eEdgeViewer *edgeViewers; // ancor for attached edgeViewers

    int                             ID;
    tJUST_CONTROLLED_PTR<ePoint>    point;     // pointer to the point this edge begins at
    tJUST_CONTROLLED_PTR<eFace>     face;     // pointer to the face this edge it is an edge of
    tJUST_CONTROLLED_PTR<eHalfEdge> next;        // the next HalfEdge around the face. Asserts: next->face == face, next->point == other->point.
    tJUST_CONTROLLED_PTR<eHalfEdge> prev;        // the previouns HalfEdge around the face. Assert: next->prev == this
    tJUST_CONTROLLED_PTR<eHalfEdge> other;       // the other half of this edge

    //  tCHECKED_PTR(eWall) wall;  // is it a eWall? what type?
};



class ePoint:public eDual, public eCoord, public tReferencable< ePoint >{
    friend class tReferencable< ePoint >;

public:
    ePoint()                         {}
    ePoint(const eCoord &c):eCoord(c){}


    bool Check()  {return eDual::Check(0);}

    // calculations
    bool operator==(const ePoint &a) const{return eCoord::operator==(a);}
    bool operator!=(const ePoint &a) const{return !eCoord::operator==(a);}
    eCoord operator-(const ePoint &a) const{return eCoord(x-a.x,y-a.y);}
    eCoord operator+(const ePoint &a) const{return eCoord(x+a.x,y+a.y);}
    REAL   operator*(const ePoint &a) const{return -x*a.y+y*a.x;}
    const eCoord &operator=(const eCoord &a){x=a.x;y=a.y;return *this;}

    bool operator==(const eCoord &a) const{return eCoord::operator==(a);}
    bool operator!=(const eCoord &a) const{return !eCoord::operator==(a);}
    eCoord operator-(const eCoord &a) const{return eCoord(x-a.x,y-a.y);}
    eCoord operator-() const{return eCoord(-x,-y);}
    eCoord operator+(const eCoord &a) const{return eCoord(x+a.x,y+a.y);}
    REAL   operator*(const eCoord &a) const{return -x*a.y+y*a.x;}
    const eCoord &operator=(const ePoint &a){x=a.x;y=a.y;return *this;}


    // replaces all known pointers to *this with pointers to *P.
    void Replace(ePoint *P);

    void Unlink();

    void Print(std::ostream &s) const;
protected:
    ~ePoint()     {Unlink();}
};

class eReplacementStorage;

class eFace:public eDual, public tReferencable< eFace >{
    friend class tReferencable< eFace >;

    ~eFace();
public:
    //  eFace(eGrid *Grid): grid(Grid)    {};
    eFace(eHalfEdge *a, eHalfEdge *b, eHalfEdge *c );
    eFace(eHalfEdge *a, eHalfEdge *b, eHalfEdge *c, tControlledPTR< eFace >& old );
    eFace(eHalfEdge *a, eHalfEdge *b, eHalfEdge *c, tControlledPTR< eFace >& old1, tControlledPTR< eFace >& old2 );

    // recreate the face surrounded by the three half edges
    void Create(eHalfEdge *a, eHalfEdge *b, eHalfEdge *c);

    //  eFace ()     {};
    //  eFace *F(int i){return e[i]->Other(this);}

    //  eCoord LeftVec(int i){return (*p[se_Left(i)])-(*p[i]);}
    // eCoord RightVec(int i){return (*p[se_Right(i)])-(*p[i]);}


    bool Check()  {return eDual::Check(1);}

    //  int FindPoint(const ePoint *P) const; // returns -1, if P is not a point
    // of this eFace, else the p[FindPoint(P)]=P.
    // int FindEdge(const eHalfEdge *E) const; // same

    void Print(std::ostream &s) const;

    void Unlink();

    // ckeck if the given point lies inside this face (edges included)
    bool IsInside( const eCoord& coord ) const;

    // ckeck if the given point lies inside this face; return value positive if it is inside and negative if outside
    REAL Insideness( const eCoord& coord, const eCoord& direction, const eCoord& lastDirection ) const;
    REAL Insideness( const eCoord& coord, const eCoord& direction ) const;
    REAL Insideness( const eCoord& coord ) const;

    // find a replacement for this face that contains the given coordinates for an object moving in the given directions
    eFace* FindReplacement( const eCoord& coord, const eCoord& direction, const eCoord& lastDirection ) const;
    eFace* FindReplacement( const eCoord& coord, const eCoord& direction ) const;
    eFace* FindReplacement( const eCoord& coord ) const;

    // check the area of this face; if it is negative, modify it to fix it and return true.
    bool CorrectArea();

    /*
    // visibility by viewer i
    void SetVisHeight(int i,REAL h);
    static void SetVisHeightAll(int i,REAL h);


    static void UpdateVisAll(int num=1);    // removes faces from the vis list
    void UpdateVis(int i);      // removes this eFace from the vis list
    */
    eFace* nextProcessed;

    // returns the array of stored replacements
    eReplacementStorage& GetReplacementStorage() const;
protected:
    /*
    REAL   visHeight[MAX_VIEWERS]; // at which height can the
    // cameras see into this eFace?
    static int s_currentCheckVis[MAX_VIEWERS];
    */

    //  eGrid *grid;
private:
    mutable eReplacementStorage* replacementStorage;
};





/*
// a base class for the next two classes: connects an eEdge and a viewer
class eEdgeViewer{

  friend class eEdge;
 protected:
  //  tCHECKED_PTR(eEdgeViewer) next;
  eEdgeViewer *next;

  tJUST_CONTROLLED_PTR(eHalfEdge) e;      // the eEdge we belong to
  int viewer;                             // and the viewer
     
  eEdgeViewer **Ref();

public:
  eEdgeViewer(eHalfEdge *E,int v);

  virtual ~eEdgeViewer();
  
  virtual void Render();
};

// tEvents for a viewer crossing an eEdge (that is, the straight
// prolongiation of the eEdge):

class eViewerCrossesEdge: public tEvent,public eEdgeViewer{
protected:
  virtual tHeapBase *Heap();
public:
  eViewerCrossesEdge(eHalfEdge *E,int v);
  virtual ~eViewerCrossesEdge();
  
  virtual bool Check(REAL dist);

  virtual void Render();
};

*/



// inline implementations:





inline eDual::eDual():ID(-1),edge(NULL){}


inline eCoord eHalfEdge::Vec() const{
    tASSERT(other);
    return *(other->Point()) - *Point();
}

// member manipulation
inline ePoint *eHalfEdge::Point() const { return point;}
inline void eHalfEdge::SetPoint(ePoint *p) {point = p;}

inline eFace *eHalfEdge::Face() const {return face;}
inline void eHalfEdge::SetFace(eFace *f) {face = f;}


inline REAL eHalfEdge::Ratio(const eCoord &c)const
{
    return eCoord::V(*Point(),c,*other->Point());
}




inline std::ostream & operator<<(std::ostream &s,const ePoint &x){x.Print(s);return s;}
inline std::ostream & operator<<(std::ostream &s,const eHalfEdge &x){x.Print(s);return s;}
inline std::ostream & operator<<(std::ostream &s,const eFace &x){x.Print(s);return s;}




#endif




