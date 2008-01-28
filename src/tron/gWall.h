/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)
#include <stdio>
#include <stdlib.h>
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

#ifndef ArmageTron_gWALL_H
#define ArmageTron_gWALL_H

#include "eAdvWall.h"
#include "nNetObject.h"
//#include "nObserver.h"
class gExplosion;
class gCycle;
class gCycleMovement;
class gNetPlayerWall;
class eTempEdge;

class gWallRim:public eWallRim{
public:
    gWallRim(eGrid *grid, REAL h=10000);
    gWallRim(eGrid *grid, REAL tBeg, REAL tEnd, REAL h=10000);
    virtual ~gWallRim();

    virtual bool Splittable() const;

    virtual void Split(eWall *& w1,eWall *& w2,REAL a);
    virtual bool RunsParallelPassive( eWall * newWall );

#ifndef DEDICATED
    void RenderReal(const eCamera *cam);
protected:
    virtual void OnBlocksCamera( eCamera * cam, REAL height ) const; //!< called by the camera code when this wall is between the cycle and the camera
#endif
    virtual REAL Height();
    virtual REAL SeeHeight();

private:
    mutable REAL renderHeight_; //!< the height with which this wall is really rendered
    mutable REAL lastUpdate_;   //!< time of the last render height update

    REAL tBeg_, tEnd_;          //!< begin and end texture coordinates
};

//! a coordinate entry for wall points, where walls turn into holes
//! or vice versa
class gPlayerWallCoord{
public:
    REAL Pos;             //!< the start position, measured relative to the point where the cycle started driving
    REAL Time;            //!< the time this point was created
    bool IsDangerous;     //!< true iff the segment AFTER this point is a true wall (and not a hole)
    tJUST_CONTROLLED_PTR< gExplosion > holer; //< if it is a hole, store who made it here.
};

class gPlayerWall:public eWall{
public:
    friend class gNetPlayerWall;

    gPlayerWall(gNetPlayerWall*w, gCycle *p);
    virtual ~gPlayerWall();

    //  virtual ArmageTron_eWalltype type();

    virtual void Flip();
    virtual void SplitByActive( eWall* oldWall );
    virtual bool RunsParallelActive( eWall* oldWall );

    virtual bool Splittable() const;
    virtual bool Deletable() const;

    virtual void Split(eWall *& w1,eWall *& w2,REAL a);

#ifndef DEDICATED
    virtual void Render(const eCamera *cam);
    void RenderList(bool list);
#endif

    virtual REAL BlockHeight() const;
    virtual REAL SeeHeight() const;

    int WindingNumber() const {return windingNumber_;}

    REAL LocalToGlobal( REAL a ) const; //!< transform alpha value of this wall into alpha value of underlying net wall
    REAL GlobalToLocal( REAL a ) const; //!< transform alpha value of underlying net wall to value of this wall

    REAL Time(REAL a) const;
    REAL Pos(REAL a) const;
    REAL Alpha(REAL pos) const;
    bool IsDangerousAnywhere( REAL time ) const;
    bool IsDangerous( REAL a, REAL time ) const;
    gExplosion * Holer( REAL a, REAL time ) const; // returns the guy who holed here

    void BlowHole	( REAL dbeg, REAL dend, gExplosion * holer ); // blow a hole into the wall form distance dbeg to dend, created by holer

    REAL BegPos() const;
    REAL EndPos() const;
    REAL BegTime() const;
    REAL EndTime() const;

    gCycle *Cycle() const;
    gCycleMovement *CycleMovement() const;
    gNetPlayerWall* NetWall() const;

    void Insert();

    //  void SetNetWall( const gNetPlayerWall* netWall );
    //  void SetCoords( const tArray< gPlayerWallCoord >& coords );
    //  const tArray< gPlayerWallCoord >&  GetCoords() const;
private:
    void Check() const;
    tCONTROLLED_PTR(gCycle) cycle_;
    tCONTROLLED_PTR(gNetPlayerWall) netWall_;

    //  nObserverPtr< gNetPlayerWall > netWall_;

    int windingNumber_;
    // REAL begAlpha_, endAlpha_;	// relative position i gNetPlayerWall
    REAL begDist_, endDist_;	// cycle driving distance at beginning and end

    gPlayerWall();
};


// the sn_netObjects that represents eWalls across the network.
class gNetPlayerWall: public nNetObject{
    friend class gCycle;
    int id,griddedid;

    tCONTROLLED_PTR(gCycle) cycle_;       // our cycle
    tCONTROLLED_PTR(eTempEdge) edge_;       // the eEdge we are representing
    gPlayerWall * lastWall_;                //! the last wall that was dropped into the grid by PartialCopyIntoGrid()

    eCoord dir;      // the direction from start to end
    REAL dbegin;    // the start position
    eCoord beg,end;  // start and end points
    REAL tBeg,tEnd; // start and end time

    unsigned short inGrid;   // are we planned to be insite the grid?
    REAL           gridding; // when are we going to enter the grid?
    bool           preliminary:1; // is it a eWall preliminary installed?
    REAL           obsoleted_;    // the game time this preliminary wall got obsoleted by a final wall (negative if it is not yet obsolete)
    // by the client while it is waiting for the real eWall from the server?

    void CreateEdge();
    void InitArray();
    void MyInitAfterCreation();
    void real_CopyIntoGrid(eGrid *grid);
    void PartialCopyIntoGrid(eGrid *grid);
    void real_Update(REAL tEnd,const eCoord &pend, bool force );
public:
    virtual void InitAfterCreation();
    gNetPlayerWall(gCycle *c,
                   const eCoord &beg,const eCoord &dir,
                   REAL tBeg, REAL dbeg);

    //  void Update(REAL tEnd,REAL dend);
    void Update(REAL tEnd,const eCoord &pend);
    void CopyIntoGrid(eGrid *grid,bool force=false);
    static void s_CopyIntoGrid();
    void RealWallReceived( gNetPlayerWall* realwall );
    void Checkpoint(); //!< marks the current distance and time for more accurate interpolation

    gNetPlayerWall(nMessage &m);

    //eCoord Vec(){return w->Vec();}
    eCoord Vec();  //!< returns the vector from the beginning to the end of the wall
protected:
    virtual ~gNetPlayerWall();
    void ReleaseData(); // release all references
public:
    void SetEndTime(REAL et);
    void SetEndPos(REAL ep);

    int  IndexAlpha(REAL a) const;
    int  IndexPos(REAL d) const;
    REAL Time(REAL a) const;
    REAL Pos(REAL a) const;
    REAL Alpha(REAL pos) const;
    bool IsDangerousAnywhere( REAL time ) const;
    bool IsDangerousApartFromHoles( REAL a, REAL time ) const; // checks all danger signs, except hooles
    bool IsDangerous( REAL a, REAL time ) const;               // checks all danger signs
    gExplosion * Holer( REAL a, REAL time ) const;                 // returns the cycle responsible for a hole

    void BlowHole	( REAL dbeg, REAL dend, gExplosion * holer ); // blow a hole into the wall form distance dbeg to dend

    REAL BegPos() const;
    REAL EndPos() const;
    REAL BegTime() const;
    REAL EndTime() const;

    const eCoord& EndPoint(int i) const
    {
        switch ( i )
        {
        case 0:
            return beg;
        case 1:
        default:
            return end;
        }
    }


#ifndef DEDICATED
    virtual void Render(const eCamera *cam);
    void RenderList(bool list);
    virtual void RenderNormal(const eCoord &x1,const eCoord &x2,REAL ta,REAL te,REAL r,REAL g,REAL b,REAL a);
    virtual void RenderBegin(const eCoord &x1,const eCoord &x2,REAL ta,REAL te,REAL ra,REAL rb,REAL r,REAL g,REAL b,REAL a);
#endif

    virtual bool ActionOnQuit();

    virtual bool ClearToTransmit(int user) const;

    virtual void WriteSync(nMessage &m);
    virtual void ReadSync(nMessage &m);

    virtual void WriteCreate(nMessage &m);
    virtual nDescriptor &CreatorDescriptor() const;
    virtual void PrintName(tString &s) const;

    virtual bool SyncIsNew(nMessage &m);


    eTempEdge   *Edge(){return this->edge_;}
    gPlayerWall *Wall();
    gCycle *Cycle() const {return this->cycle_;}
    gCycleMovement *CycleMovement() const;

    bool Preliminary()const{return preliminary;}
    bool InGrid()const{return inGrid;}

    static void Clear(); // delete all sg_netPlayerWalls.

    void Check() const;
private:
    tArray<gPlayerWallCoord> coords_;

    unsigned int displayList_;
};

extern tList<gNetPlayerWall> sg_netPlayerWalls;
extern tList<gNetPlayerWall> sg_netPlayerWallsGridded;

#endif
