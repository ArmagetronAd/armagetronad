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

#ifndef ArmageTron_explosion_H
#define ArmageTron_explosion_H

#include "eGameObject.h"

class gCycle;
struct gRealColor;

class gExplosion: virtual public eGameObject, public tReferencable< gExplosion >
{ // Boom!
public:
    gExplosion(eGrid *grid, const eCoord &pos,REAL time, gRealColor& color, gCycle * owner );
    virtual ~gExplosion();

    virtual bool Timestep(REAL currentTime);

    virtual void InteractWith(eGameObject *target,REAL time,int recursion=1);

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int recursion=1);

    virtual void Kill();
#ifndef DEDICATED
    virtual void Render(const eCamera *cam);
    virtual void Render2D(tCoord scale) const;

    //virtual void SoundMix(Uint8 *dest,unsigned int len,
    //                      int viewer,REAL rvol,REAL lvol);
#endif

    bool AccountForHole(); // will return true exactly once per explosion; to be used to make the holing score only count once.

    static void OnNewWall( eWall* w );	// blow holes into a new wall

    //#ifdef USEPARTICLES
    int particle_handle_circle;
    int particle_handle_cylinder;
    //#endif

    // returns the owner
    gCycle * GetOwner() const
    {
        return owner_;
    }

    virtual void AddRef(){tReferencable< gExplosion >::AddRef();}
    virtual void Release(){tReferencable< gExplosion >::Release();}
    virtual void RemoveFromGame();

private:
    REAL        createTime;

    //	gRealColor	color_;
    REAL		explosion_r;
    REAL		explosion_g;
    REAL		explosion_b;

    int			expansion;

    static int	expansionSteps;
    static REAL expansionTime;

    bool holeAccountedFor_;  //!< flag indicating whether we already gave the player credit for making a hole for his teammates

    int 		listID;

    tJUST_CONTROLLED_PTR< gCycle > owner_; // the owner/victim of the explosion
};
#endif
