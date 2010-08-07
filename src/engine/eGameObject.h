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

#ifndef ArmageTron_GAMEOBJECT_H
#define ArmageTron_GAMEOBJECT_H

//#include "uInput.h"
#include "tList.h"
// #include "eGrid.h"
#include "eCoord.h"
#include "tSafePTR.h"

class eGrid;
class uActionPlayer;
class eTeam;
class eWall;
class eCamera;

// a generic object for the game (cycles,explosions, particles,
// maybe AI opponents..)
class eGameObject{
    friend class eFace;
    friend class eCamera;
    friend class eSensor;
    friend class eGrid;
    friend class ePlayerNetID;

    // a list of all active gameobjects
    int id;
#define GO ((eGameObject *)NULL)
#define GO_I ((int *)NULL)

    // small wrapper of TimestepThis doing preparation and cleanup work
    static void TimestepThisWrapper(eGrid * grid, REAL currentTime, eGameObject *t, REAL minTimestep);

    //! called immediately after the object is created, either right after round beginning or mid-game creation
    virtual void OnBirth();

protected:
    // does a timestep and all interactions for this gameobject,
    // divided in many small steps
    static bool TimestepThis(REAL currentTime,eGameObject *t);

    // tells game objects how far they are allowed to exeed the given simulation time
    static REAL MaxSimulateAhead();

    // a list of all eGameObjects that are interesting to watch
    int interestingID;
    int inactiveID;

    // shall s_Timestep delete a eGameObject requesting destruction
    // completely (autodelete=1) or should it just be removed from the list
    // (autodelete=0) ?
    // (the latter may be useful if there exists other pointers to
    // the object)


    bool autodelete;
    REAL lastTime;          // the time it was last updated
    REAL deathTime;          // the time the thing died

    eCoord pos;               // current position,
    eCoord dir;               // direction
    REAL  z;									// and height (currently unused)

    tJUST_CONTROLLED_PTR< eTeam > team;       		 				// the team we belong to

    tJUST_CONTROLLED_PTR<eFace> currentFace;  // the eFace pos it is currently
    tCHECKED_PTR(eGrid) grid;         // the game grid we are on

    bool _born;

    // entry and deletion in the list of all eGameObjects
public:
    //! tells game objects what the maximum lag caused by lazy simulation of timesteps is
    static REAL GetMaxLazyLag();
    //! sets the value reported by GetMaxLazyLag()
    static void SetMaxLazyLag( REAL lag );

    eTeam* Team() const { return team; }

    static uActionPlayer se_turnLeft,se_turnRight;

    eGrid* Grid()        const { return grid;        }
    eFace* CurrentFace() const { return currentFace; }

    virtual void AddRef()  = 0;          //!< adds a reference
    virtual void Release() = 0;         //!< removes a reference

    void AddToList();
    void RemoveFromList();
    void RemoveFromListsAll();
    void RemoveFromGame(); //!< removes the object physically from the game

protected:
    virtual void OnRemoveFromGame(); //!< called on RemoveFromGame(). Call base class implementation, too, in your implementation. Must keep the object alive.
private:
    virtual void DoRemoveFromGame(); //!< called on RemoveFromGame() after OnRemoveFromGame(). Do not call base class implementation of this function, don't expect to get called from subclasses.
public:

    int GOID() const {return id;}
    int InterestingID() const {return interestingID;}
    REAL LastTime() const {return lastTime;}
    virtual REAL NextInterestingTime() const {return lastTime;} //!< the next time something interesting is going to happen with this object

    eGameObject(eGrid *grid, const eCoord &p,const eCoord &d, eFace *currentface, bool autodelete=1);
    virtual ~eGameObject();

    virtual eCoord Position()const;
    virtual eCoord Direction()const;
    virtual eCoord LastDirection()const;
    virtual REAL DeathTime()const;
    virtual REAL  Speed()const;

    //! returns a guess about which other object killed this
    virtual eGameObject const * Killer() const { return NULL; }

    // position after FPS dependant extrapolation
    virtual eCoord PredictPosition() const {return pos;}

    // makes two gameObjects interact:
    virtual void InteractWith( eGameObject *target,REAL time,int recursion=1 );

    // what happens if we pass eWall w? (at position e->p[0]*a + e->p[1]*(1-a) )
    virtual void PassEdge( const eWall *w,REAL time,REAL a,int recursion=1 );

    // what length multiplicator does driving along the given wall get when it is the given distance away?
    virtual REAL PathfindingModifier( const eWall *w ) const { return 1 ;}

    // moves the object from pos to dest during the timeinterval
    // [startTime,endTime] and issues all eWall-crossing tEvents
    void Move( const eCoord &dest, REAL startTime, REAL endTime, bool useTempWalls = true );

    // finds the eFace we are in
    void FindCurrentFace();

    // simulates behaviour up to currentTime:
    virtual bool Timestep(REAL currentTime);
    // return value: shall this object be destroyed?

    virtual bool EdgeIsDangerous(const eWall *w, REAL time, REAL a) const{
        return w;
    }

    //! called when the round begins, after all game objects have been created,
    //! before the first network sync is sent out.
    virtual void OnRoundBegin();

    //! called when the round ends
    virtual void OnRoundEnd();

    //! call to ensure the object is "born"
    void EnsureBorn();

    //! destroys the gameobject (in the game)
    virtual void Kill();

    //! tells whether the object is alive
    virtual bool Alive() const {return false;}

    //! draws object to the screen using OpenGL
    virtual void Render(const eCamera *cam);

    // draws it to the screen in two dimensions using OpenGL (ie. for the HUD map)
    virtual void Render2D(tCoord scale) const;

    //! returns whether the rendering uses alpha blending (massively, so sorting errors would show)
    virtual bool RendersAlpha() const;

    // draws the cockpit or whatever is seen from the interior
    // in fixed perspective, called before the main rendering
    virtual bool RenderCockpitFixedBefore(bool primary=true);
    // return value: draw everything else?

    // the same purpose, but called after main rendering
    virtual void RenderCockpitFixedAfter(bool primary=true);

    // virtual perspective
    virtual void RenderCockpitVirtual(bool primary=false);

    //sound output
    //virtual void SoundMix(unsigned char *dest,unsigned int len,
    //                      int viewer,REAL rvol,REAL lvol){};

    // internal camera
    virtual eCoord CamDir()  const {return dir;}
    virtual REAL  CamRise()  const {return 0;}
    virtual eCoord CamPos()  const {return pos;}
    virtual REAL  CamZ()     const {return z;}
    virtual eCoord  CamTop() const {return eCoord(0,0);}

    // sr_laggometer
    virtual REAL Lag() const{return 0;}          //!< expected average network latency
    virtual REAL LagThreshold() const{return 0;} //!< tolerated network latency variation

#ifdef POWERPAK_DEB
    virtual void PPDisplay();
#endif

    // Receives control from ePlayer
    virtual bool Act(uActionPlayer *Act,REAL x);

    // does a timestep and all interactions for every gameobject
    static void s_Timestep(eGrid *grid, REAL currentTime, REAL minTimestep);

    // displays everything:
    static void RenderAll(eGrid *grid, const eCamera *cam);
    static void PPDisplayAll();

    // kills everything:
    static void DeleteAll(eGrid *grid);
};

// game object to be created on the heap
class eReferencableGameObject: public eGameObject, public tReferencable< eReferencableGameObject >
{
public:
    eReferencableGameObject(eGrid *grid, const eCoord &p,const eCoord &d, eFace *currentface, bool autodelete=1);

    // real reference counting
    virtual void AddRef();          //!< adds a reference
    virtual void Release();         //!< removes a reference

private:
    virtual void DoRemoveFromGame(); //!< called when removed from the game
};

// game object of temporary lifetime on the stack. Don't dynamically allocate this.
class eStackGameObject: public eGameObject
{
public:
    eStackGameObject(eGrid *grid, const eCoord &p,const eCoord &d, eFace *currentface);

    // dummy reference counting
    virtual void AddRef();          //!< adds a reference
    virtual void Release();         //!< removes a reference

private:
    virtual void DoRemoveFromGame(); //!< called when removed from the game
};

//! Exception to throw when a gameobject dies during movement
class eDeath
{
public:
    eDeath(){};   //!< constructor
    ~eDeath(){};  //!< destructor
};

#endif
