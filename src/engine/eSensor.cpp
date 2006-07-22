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

#include "eSensor.h"
#include "eWall.h"
#include "eDebugLine.h"
#include "eGrid.h"

#ifdef DEBUG
//#define DEBUGLINE
#endif

eSensor::eSensor(eGameObject *o,const eCoord &start,const eCoord &d)
        :eGameObject(o->grid, start,d,o->currentFace,0)
        ,hit(1000),ehit(NULL),lr(0), owned(o) , inverseSpeed_(0)
{
    if (owned)
    {
        currentFace=owned->currentFace;

        // find a better current face if our start postion is not really at the
        // current position of the owner
        if ( grid && !currentFace || !currentFace->IsInside( pos ) && currentFace->IsInside( owned->pos ) )
        {
            currentFace = o->grid->FindSurroundingFace( pos, currentFace );
        }
    }
    else
        currentFace=NULL;
}

void eSensor::PassEdge(const eWall *w,REAL time,REAL a,int){
    if (hit>time){
        if (!w->Massive()){
            return;
        }

        // extrapolate the hit time
        REAL hitTime = owned->LastTime() + time * inverseSpeed_;

        if (owned && !owned->EdgeIsDangerous(w, hitTime, a))
            return;

        lr=0;

        const eHalfEdge *e = w->Edge();

        eCoord eEdge_dir=w->Vec();
        eCoord collPos = w->Point( a );

        // con << dir << eEdge_dir << '\n';
        REAL dec=- eEdge_dir*dir;

        if (dec>0)
            lr=1;
        else if (dec<0)
            lr=-1;
        else
            lr=0;

        hit=time;
        ehit=e;
        before_hit=collPos-dir*.000001;

    }
}

//void eSensor::PassEdge(eEdge *e,REAL time,REAL a,int recursion){
//  PassEdge((const eEdge *)e,time,a,recursion);
//}

void eSensor::detect(REAL range){
    //  eCoord start = pos;
    //  pos=pos+dir*.01;
    before_hit=pos+dir*(range-.001);
    hit=range+.00001f;
    ehit = 0;

    /*
    {
      ePoint a(pos);
      ePoint b(pos+dir*range);
      eEdge e(&a,&b);

      
      for(int i=eGameObject::gameObjects.Len()-1;i>=0;i--){
        eGameObject *target=gameobject::gameObjects(i);
        
        if (target->type()==ArmageTron_CYCLE){
    gCycle *c=(gCycle *)target;
    if (c->Alive() && c!=owned){
     const eEdge *oe=c->Edge();
     if (oe){
       ePoint *meet=e.IntersectWith(oe);
     
       if (meet){ // whoops. Hit!
         REAL ratio=oe->Ratio(*meet);
         // gPlayerWall *w=(gPlayerWall *)oe->w;
         REAL time=e.Ratio(*meet)*range;
         PassEdge(oe,time,ratio,1);
         delete meet;
       }
     }
    }
        }
      } 
    }
    */

    Move(pos+dir*range,0,range);

#ifdef DEBUGLINE
    if (hit < range)
    {
        eDebugLine::SetColor  (0, 1, 1);
        eDebugLine::Draw(start, .1, before_hit, .1);

        eDebugLine::SetColor  (0, .5, 1);
        eDebugLine::Draw(before_hit, .1, before_hit, 2.0);
    }
    else
    {
        eDebugLine::SetColor  (1, 0, 0);
        eDebugLine::Draw(start, .5, pos, .5);
    }
#endif
}


