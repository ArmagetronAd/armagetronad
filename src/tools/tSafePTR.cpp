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

#include "tSafePTR.h"
#include "tList.h"

/*
class tPTRList:public tArray<tCheckedPTRBase *> {
  friend tCheckedPTRBase;

  // Array<T *> list;
public:
  tPTRList():tArray<tCheckedPTRBase *>(10000){}

  void Add(tCheckedPTRBase *t){
    if (t->id<0){    // tEventQueue relies on the fact that we put t in
      t->id=Len();   // the last place.

      operator[](t->id)=t;
      if ((t->id)==710)
	st_Breakpoint();
    }
  }
  void Remove(tCheckedPTRBase *t){
    // con << "offset=" << offset << '\n';
    if (t->id>=0 && Len()>0){
#ifdef DEBUG
      if ((t->id)==710)
	st_Breakpoint();

      if (t->id>=Len())
	tERR_ERROR("Scr. list structure!");
      
      tCheckedPTRBase *test=operator()(t->id);
      if (test!=t)
	tERR_ERROR("Screwed list structure!");
#endif
    // the check for Len() is done, since this may be
      // called on an allready descructed list.
      tCheckedPTRBase *other=operator()(Len()-1);
      operator()(t->id)=other;

      if ((other->id)==710)
	st_Breakpoint();

      other->id=t->id;
      SetLen(Len()-1);
    }
    t->id=-1;
  }
};
*/

tList<tCheckedPTRBase> tCheckedPTRs;

tCheckedPTRBase::tCheckedPTRBase(void *x):id(-1),target(x){
    tCheckedPTRs.Add(this,id);
}

tCheckedPTRBase::tCheckedPTRBase(const tCheckedPTRBase& x)
        :id(-1),target(x.target){
    tCheckedPTRs.Add(this,id);
}

tCheckedPTRBase::tCheckedPTRBase():id(-1),target(NULL){
    tCheckedPTRs.Add(this,id);
}


tCheckedPTRBase::~tCheckedPTRBase(){
    tCheckedPTRs.Remove(this,id);
}

#ifdef DEBUG
static void * watchobject = NULL;

// functions to watch reference counting of specific objects
void st_AddRefBreakpint( void const * object )
{
    if ( object == watchobject )
    {
        int x;
        x = 0;
    }
}

void st_ReleaseBreakpint( void const * object )
{
    if ( object == watchobject )
    {
        int x;
        x = 0;
    }
}
#endif


/*

class gWallRim;
class gNetPlayerWall;
class eCamera;
class tConfItemBase;
class ePlayer;
class ePlayerNetID;
class uAction;
class eGameObject;
class ePoint;
class eEdge;
class eFace;

extern List<gWallRim> se_rimWalls;
extern List<gNetPlayerWall> sg_netPlayerWalls;
extern List<gNetPlayerWall> gridded_sg_netPlayerWalls;
extern List<eCamera> se_cameras;
extern List<tConfItemBase> tConfItemBase::tConfItems;
extern List<eGameObject> eGameObject::gameObjects;
extern List<ePlayer> PlayerConfig;
extern List<ePlayerNetID> se_PlayerNetIDs;
extern List<uAction> s_playerActions;
extern List<uAction> s_cameraActions;
extern List<uAction> s_globalActions;
extern List<uAction> su_allActions;
extern List<ePoint> ePoint::points;
extern List<eEdge> eEdge::edges;
extern List<eFace> eFace::faces;


template<class T> void suspicious(const List<T> &l,tCheckedPTRBase *adr){
  T *best=NULL;
  int best_ind=-1;
  int i;
  for (i=l.Len()-1;i>=0;i--){
    T *t=l(i);
    int dist=int(adr)-int(t);
    if (dist>=0 && dist < 100 && int(t)>int(best)){
      best=t;
      best_ind=i;
    }
  }
  if (best){
    tERR_ERROR("Suspicious object " << l(best_ind) << " has index " << best_ind << " and is at adress " 
	  << best << ".");
  }
}





void tCheckedPTRBase::CheckDestructed(void *test){
  // return;

  for(int i=tCheckedPTRs.Len()-1;i>=0;i--){
    tCheckedPTRBase * adr=tCheckedPTRs(i);
    if (adr->target==test){
      suspicious(se_rimWalls,adr);
      suspicious(sg_netPlayerWalls,adr);
      suspicious(gridded_sg_netPlayerWalls,adr);
      suspicious(se_cameras,adr);
      suspicious(tConfItemBase::tConfItems,adr);
      suspicious(eGameObject::gameObjects,adr);
      suspicious(eGameObject::gameObjectsInactive,adr);
      suspicious(PlayerConfig,adr);
      suspicious(se_PlayerNetIDs,adr);
      suspicious(s_playerActions,adr);
      suspicious(s_cameraActions,adr);
      suspicious(s_globalActions,adr);
      suspicious(su_allActions,adr);
      suspicious(ePoint::points,adr);
      suspicious(eEdge::edges,adr);
      suspicious(eFace::faces,adr);
      
      
      tERR_ERROR("Object destroyed, but pointer at " << adr << 
	    " still points to it!");
    }
  }

}

*/

