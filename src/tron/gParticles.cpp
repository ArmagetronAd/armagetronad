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

#if 0
#include "gParticles.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
// #include "papi.h"

#ifndef DEDICATED
void gParticles::Render(const eCamera *cam) {
#ifdef USE_PARTICLES
    // Select this particle system in the PS API
    pCurrentGroup(thisSystem);

    pDrawGroupp(GL_POINTS, true);
#endif
}
#endif

bool gParticles::Timestep(REAL currentTime) {
#ifdef USE_PARTICLES
    REAL ticks = currentTime - lastTime;

    // Store the current time as the lastTime, for next time
    lastTime = currentTime;

    // Select this particle system in the PS API
    pCurrentGroup(thisSystem);

    // Set the timestep interval for the system
    pTimeStep( (float) ticks);

    // Bounce particles off a disc of radius 5.
    //	pBounce(-0.05, 0.35, 0, PDDisc, 0, 0, 0,  0, 0, 1,  5);

    // Kill particles below Z=-3.
    //pSink(false, PDPlane, 0, 0, 0, 0,0,1);

    // Move particles to their new positions.
    pMove();

    // Now we check to see if the system is over
    //if( pGetGroupCount() == 0 )
    //return true;


#endif
    return false;
}

void gParticles::GiveBirth(REAL currentTime) {
#ifdef USE_PARTICLES
    ParticleInfo newParticle;

    glCoord tmpFocus;
    glCoord tmpVec;

    tmpFocus = focus;

    float randomaxis;

    randomaxis = 10.0f * rand()/(RAND_MAX+1.0f);
    tmpFocus.x -= randomaxis/2.0f;
    tmpFocus.x += randomaxis;

    tmpFocus.y -= randomaxis/2.0f;
    tmpFocus.y += randomaxis;

    tmpVec = svector;

    //tmpVec.x -= randomaxis/2.0f;
    //tmpVec.x += randomaxis;

    //tmpVec.y -= randomaxis/2.0f;
    //tmpVec.y += randomaxis;

    newParticle.pos = tmpFocus;
    newParticle.vec = tmpVec;
    newParticle.life = psystem.life;
    newParticle.startTime = currentTime;

    particles.push_back(newParticle);
#endif
}

gParticles::gParticles(const eCoord &pos,const glCoord &vec,REAL time, ParticleSystem &param) {
#ifdef USE_PARTICLES
    // Grab the incoming data and store it
    startTime = time;

    // focus is the focus of the particle system
    focus.x = pos.x;
    focus.y = pos.y;
    focus.z = 0;

    // vector stores a vector that shows where the particle system points
    svector.x = vec.x;
    svector.y = vec.y;
    svector.z = vec.z;

    // psystem stores the parameters of the whole system
    psystem = param;

    // Initialize the system
    thisSystem = pGenParticleGroups(1, psystem.numParticles);
    // Select this particle system in the PS API
    pCurrentGroup(thisSystem);

    // Set up the state.
    pVelocity(svector.x, svector.y, svector.z);
    pColorD(1.0, PDLine, 0.8, 0.9, 1.0, 1.0, 1.0, 1.0);
    pSize(1.5);

    // Generate particles along a very small line in the nozzel.
    pSource(psystem.numParticles, PDSphere, focus.x, focus.y, focus.z, 20);

    // Gravity.
    pGravity(0.0, 0.0, -0.01);
#endif
}

#endif

