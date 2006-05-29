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

#ifndef ArmageTron_WALL_H
#define ArmageTron_WALL_H

// #include "eGrid.h"
#include "tHeap.h"
#include "eCoord.h"

class eHalfEdge;

// ******************************************************************


// ******************************************************************

// a class for the different types of eWalls or similar objects
// (enery barriers, fault lines...) that may appear in the game

//#ifdef WIN32
// disable a hack that apparently only works with gcc 2.95.x
//#define CAUTION_WALL
//#endif

// uncomment to disable evil hack for all architectures
//#define CAUTION_WALL



class eWall;
class eWallHolder;
class eGameObject;
class eCamera;
class eGrid;

class eWallView:public tHeapElement{
    friend class eWall;
protected:
    virtual tHeapBase *Heap() const;

#ifdef CAUTION_WALL
    eWall *wall;
#endif

    int  viewer;
public:
    eWallView(){}
    ~eWallView();

    void Set(int view,eWall *mw){
        viewer=view;
#ifdef CAUTION_WALL
        wall=mw;
#endif
    }

    REAL Value(){return tHeapElement::Val();}
    void SetValue(REAL v);

    eWall *Belongs();  // to wich wall do we belong?

};

class eWall: public tReferencable< eWall >{
    friend class tReferencable< eWall >;
    //friend class eHalfEdge;
    //friend class eTempEdge;
    friend class eWallView;
    friend class eWallHolder;

    eWallView view[MAX_VIEWERS];
protected:
    tCHECKED_PTR(eWallHolder)   holder_;
    tJUST_CONTROLLED_PTR<eGrid> grid;
    REAL                        len;
    int                         flipped;
public:
    int id;

    eWall(eGrid *grid);

    /*
    	virtual void AddRef();
    	virtual void Release();
    	int GetRefcount();
    */

    eHalfEdge* Edge()const;

    REAL Len() const {return len;}

    void CalcLen();

    const eCoord& EndPoint(int i) const; // returns the coordinates of the beginning (i=0) or end (i=1) of this wall

    eCoord Point( REAL a ) const; // returns the coordinates somewhere between beginning and end

    eCoord Vec() const; // returns the vector from the beginning to the end of the wall

    // flip the fall
    virtual void Flip(){flipped = 1-flipped;}

    // may we split the eWall in two?
    virtual bool Splittable() const;

    // can we delete the eWall now?
    virtual bool Deletable() const;

    // split the eWall in two with ratio a : 1-a
    virtual void Split(eWall *& w1,eWall *& w2,REAL a);

    // split the wall absolutely correctly; update the grid accordingly.
    void SplitComplete(eWall *& w1,eWall *& w2,REAL a);

    // is it still massive? (i.e. is the player it belongs to alive?)
    virtual bool Massive() const{return true;}

    // what happens to a gameobject that passes here?
    virtual void PassingGameObject(eGameObject *pass,REAL time,REAL pos,int recursion=1);

    // what happens if this wall, when layed out, turns out to cross another wall that was already there?
    virtual void SplitByActive( eWall * oldWall );

    // the same question for the old wall.
    virtual void SplitByPassive( eWall * newWall );

    // what happens if this wall, when layed out, turns out to be exactly parallel to another wall? The return value indicates if this operation is allowed.
    virtual bool RunsParallelActive( eWall * oldWall );

    // the same seen from the other wall
    virtual bool RunsParallelPassive( eWall * newWall );
#ifndef DEDICATED
    //  static void Render_helper(eWall *w,REAL tBeg,REAL tEnd,REAL h=4,REAL hfrac=1,REAL bot=0);

    // draws it to the screen using OpenGL
    virtual void Render(const eCamera *cam){};
#endif

    // can you see through it?
    virtual REAL Height() const{return 1;}
    virtual REAL BlockHeight() const{return 1;}
    virtual REAL SeeHeight() const{return 1;}

    inline void BlocksCamera( eCamera * cam, REAL height ) const; //!< called by the camera code when this wall is between the cycle and the camera

    void Insert();
    void Remove();

    void SetVisHeight(int v,REAL h);

protected:

    virtual void OnBlocksCamera( eCamera * cam, REAL height ) const; //!< called by the camera code when this wall is between the cycle and the camera

    virtual ~eWall();
private:
};

class eWallHolder
{
    friend class eWall;
public:
    void SetWall( eWall* wall );
    eWall* GetWall( void ) const;

protected:
    eWallHolder();
    ~eWallHolder();

private:
    tCONTROLLED_PTR( eWall ) wall_;
};

// TODO: get rid of these
extern tHeap<eWallView>  se_wallsVisible[MAX_VIEWERS];

// *******************************************************************************************
// *
// *	BlocksCamera
// *
// *******************************************************************************************
//!
//! @param cam the camera that is blocked
//! @param height the maximal height the wall would be allowed to have without blocking the view
//!
// *******************************************************************************************

void eWall::BlocksCamera( eCamera * cam, REAL height ) const
{
    OnBlocksCamera( cam, height );
}

#endif

