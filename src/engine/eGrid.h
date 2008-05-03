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

#ifndef ArmageTron_GRID_H
#define ArmageTron_GRID_H

#include "defs.h"
#include "eCoord.h"
#include "tList.h"
#include "eAxis.h"
//#include "eGameObject.h"
//#include "eWall.h"
//#include "eCamera.h"

class ePoint;
class eFace;
class eHalfEdge;
class eWall;
class eGrid;
class eWallView;
class eCamera;
class eAxis;
class eGameObject;

class nNetObject;

// edge class for temporary variables;  automatically creates two halfeges.
class eTempEdge: public tReferencable< eTempEdge >{
    friend class tReferencable< eTempEdge >;
public:
    eTempEdge(ePoint *p1,ePoint *p2,eWall *W=NULL);
    eTempEdge(const eCoord &c1,const eCoord &c2,eWall *W=NULL);

    eCoord     Vec()         const;
    eCoord&    Coord(int i)  const;
    ePoint*    Point(int i)  const;
    eFace*     Face(int i)   const;
    eHalfEdge* Edge(int i)   const;
    eWall*     Wall()        const;

    void     SetWall( eWall* W )       const;

    void CopyIntoGrid(eGrid *grid);
protected:
    ~eTempEdge();
private:
    tControlledPTR< eHalfEdge > halfEdges[2];
private:
};


class eGrid: public tReferencable< eGrid >{
    friend class eCamera;
    friend class eFace;
    friend class eHalfEdge;
    friend class ePoint;
    friend class eGameObject;
    friend class eWall;
    friend class tReferencable< eGrid >;

protected:
    ~eGrid();
    eAxis axis;

public:
    eGrid();

    // get the number of directions a cycle can drive in on this grid
    int WindingNumber() const {return axis.WindingNumber();}

    // get the number corresponding to a particular direction
    int DirectionWinding(const eCoord& dir);

    // get the direction associated with a winding number
    eCoord GetDirection(int winding);

    void SetWinding(int number);
    void SetWinding(int number, eCoord directions[], bool normalise=true);
	float GetWindingAngle(int winding);
	 
    // Ask the grid to turn a winding
    void Turn(int &currentWinding, int direction);

    // try to get rid of eEdge number e
    void SimplifyNum(int e);

    // try to get rid of count edges
    void SimplifyAll(int count=1);

    // consistency check
    void Check() const;

    // create a new grid with a basic topology
    void Create();

    // clear all data
    void Clear();

    // make sure the circle with given radius lies inside the grid
    void Range(REAL range_squared);

    // grows the grid one step
    void Grow();

    // displays the grid, eWalls and gameobjects
    void Render( eCamera* cam, int viewer, REAL& zNear );

    // get the currently active grid (OBSOLETE)
    static eGrid *CurrentGrid();

    /*
      void ResetVisibles(int viewer);  // reset the visibility information
    */

    ePoint* Insert(const eCoord& coord, eFace *guessFace=NULL); // inserts a point at the given coordinates

    eFace *FindSurroundingFace(const eCoord& coord, eFace *start=NULL) const;

    // adds a new Point end, adds an eEdge from start to end with
    // type wall. Modifies other faces and non-eWall-edges;
    // if change_grid is set to 0, no edges will be flipped.
    // start must already be part of the grid.

    ePoint *DrawLine(ePoint* start, const eCoord& end, eWall *wal=NULL,bool change_grid=1);


    const tList<eCamera>&     Cameras()     const{return cameras;}
    const tList<eGameObject>& GameObjects() const{return gameObjects;}
    const tList<eGameObject>& GameObjectsInteresting() const{return gameObjectsInteresting;}
    const tList<eGameObject>& GameObjectsInactive() const{return gameObjectsInactive;}


    int    NumberOfCameras();
    const eCoord& CameraPos(int i);
    eCoord CameraGlancePos(int i);
    const eCoord& CameraDir(int i);
    REAL CameraHeight(int i);


    //  int    NumberOfCameras(){return eCamera::Number();}
    //  const eCoord& CameraPos(int i){return eCamera::PosNum(i);}
    //  const eCoord& CameraDir(int i){return eCamera::DirNum(i);}
    //  REAL CameraHeight(int i){return eCamera::HeightNum(i);}


    void AddGameObjectInteresting    (eGameObject *o);
    void RemoveGameObjectInteresting (eGameObject *o);
    void AddGameObjectInactive       (eGameObject *o);
    void RemoveGameObjectInactive    (eGameObject *o);

    typedef void WallProcessor		( eWall* 			w 	);	// function prototype for wall query functions
    void ProcessWallsInRange		( WallProcessor* 	proc,
                                const eCoord&		pos	,
                                REAL				range,
                                eFace*			startFace );	// call WallProcessor for all walls closer than range to pos

protected:
    // render helper
    void display_simple( int viewer,bool floor,
                         bool sr_upperSky,bool sr_lowerSky,
                         REAL flooralpha,
                         bool eWalls,bool gameObjects,
                         REAL& zNear );


    // normal list management
    void AddFace    (eFace     *f);
    void RemoveFace (eFace     *f);
    void AddEdge    (eHalfEdge *e);
    void RemoveEdge (eHalfEdge *e);
    void AddPoint   (ePoint    *p);
    void RemovePoint(ePoint    *p);

    // completely unlink:
    void KillFace (eFace*      f);
    void KillEdge (eHalfEdge*  e);
    void KillPoint(ePoint*     p);

    // unlink face, but keep it alive for recycling ( so all gameobjects that live on it won't have to find a new face )
    tControlledPTR< eFace > ZombifyFace (eFace*      f);

    // adds the face, its edges and vertives to the grid
    void AddFaceAll (eFace     *f);

    bool       requestCleanup; // triggered when the data structures have gone bonkers

    // for the grid growth
    tJUST_CONTROLLED_PTR< ePoint > A,B,C;
    tJUST_CONTROLLED_PTR< eHalfEdge >  a,b,c;
    REAL       maxNormSquared;
    eCoord     base;

    // grid data
    tList<eHalfEdge, false, true>   edges;
    tList<ePoint, false, true>      points;
    tList<eFace, false, true>       faces;

    // objects
    tList<eGameObject> gameObjects;
    tList<eGameObject> gameObjectsInactive;
    tList<eGameObject> gameObjectsInteresting;

    // cameras
    tList<eCamera>     cameras;

    // walls
    // tHeap<eWallView>  wallsVisible[MAX_VIEWERS];
    tList<eWall>       wallsNotYetInserted;

#ifdef DEBUG
public:
    bool doCheck;
#endif
};



#endif




