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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_ADV_WALL_H
#define ArmageTron_ADV_WALL_H

#include "eWall.h"
#include "nNetObject.h"

class eRectangle;

class eWallRim:public eWall{
protected:
    int rim_id;
    bool bf_cull;
    REAL height;
public:
    eWallRim(eGrid *grid, bool backface_cull=true,REAL h=10000000);
    virtual ~eWallRim();

    virtual bool Splittable() const;

    //  virtual void Split(eWall *& w1,eWall *& w2,REAL a);

#ifndef DEDICATED
    virtual void RenderReal(const eCamera *cam)=0;

    virtual void Render(const eCamera *cam){
        if (rim_id<0 && Edge() )
            RenderReal(cam);
    }
#endif

    virtual REAL Height() const{return height;}

    virtual REAL SeeHeight() const{return height*40;}

    virtual REAL BlockHeight() const{
        if (bf_cull)
            return 0;
        else
            return SeeHeight();
    }

    //! is x by offset inside the bounds of the rim eWalls?
    static bool IsBound(const eCoord &x,REAL offset=0);

    //! renders all rim walls
    static void RenderAll( eCamera * camera ); 
    //! destroys the display list (call when geometry is updated)
    static void DestroyDisplayList();
    //! brings x into the bounds of the rim eWalls with a min distance of offset
    static REAL Bound(eCoord &x,REAL offset=0);
    //! brings out inside the bounds of the rim walls, moving it closer to in.
    static REAL Clip(const eCoord& in, eCoord &out,REAL offset=0);

    //! returns the bounding rectangle enclosing all rim walls
    static eRectangle const & GetBounds();
    //! updates the bounding rectangle enclosing all rim walls (call after arena buildup)
    static void UpdateBounds();
};

extern tList<eWallRim> se_rimWalls;




#endif
