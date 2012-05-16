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

#ifndef ArmageTron_explosion_H
#define ArmageTron_explosion_H

#include "eSound.h"
#include "eGameObject.h"
#include "gParticles.h"

class gCycle;
struct gRealColor;

class gExplosion: virtual public eReferencableGameObject
{ // Boom!
public:
    gExplosion(eGrid *grid, const eCoord &pos,REAL time, gRealColor& color, gCycle * owner, REAL radius = 0 );
    virtual ~gExplosion();

    virtual bool Timestep(REAL currentTime);

    virtual void InteractWith(eGameObject *target,REAL time,int recursion=1);

    virtual void PassEdge(const eWall *w,REAL time,REAL a,int recursion=1);

    virtual void Kill();
#ifndef DEDICATED
    virtual void Render(const eCamera *cam);

    virtual void SoundMix(Uint8 *dest,unsigned int len,
                          int viewer,REAL rvol,REAL lvol);
#endif

	//! draws it in a svg file
	virtual void DrawSvg(std::ofstream &f);

    bool AccountForHole(); // will return true exactly once per explosion; to be used to make the holing score only count once.

    static void OnNewWall( eWall* w );	// blow holes into a new wall

    // returns the owner
    gCycle * GetOwner() const
    {
        return owner_;
    }
    REAL GetRadius() const
    {
        return radius_;
    }

protected:
    virtual void OnRemoveFromGame(); // called last when the object is removed from the game
private:
    eSoundPlayer sound;

    gParticles *theExplosion;

    REAL        createTime;
    REAL 	radius_;

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
