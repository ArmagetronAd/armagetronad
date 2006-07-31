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

#if 0
#ifndef ArmageTron_PARTICLES_H
#define ArmageTron_PARTICLES_H

// Uncomment the following line to use the particle system
//#define USE_PARTICLES

#include "defs.h"
#include "eCamera.h"
#include "rModel.h"
#include "rTexture.h"
#include "eGameObject.h"
#include "eCoord.h"
#include <vector>

#define gNUM_PARTICLES 1000

// The struct that defines the particle system.  Pass an instance of it to the
//   gParticles constructor.
typedef struct {
    float particle_size;  // Obviously, the size of the particles
    int numParticles;     // Total number of particles in the system
    float initialSpread;    // Initial spread of particles
    float gravity;        // (Gravity)
    bool generateNewParticles; // Flag, if false don't generate new particles
    int numStartParticles;  // Number of active particles to initialize with
    float life;           // Average lifetime of a particle
    float lifeRand;       // Factor to use to randomize particle lifetime
    float expansion;      // The rate at which the system will run
    float flow;           // The rate at which new particles will be generated
    //   New particle generation happens when a particles dies
    //      and also when the system is not full
    //      If the system is full of active particles,
    //      new particles will not be generated
} ParticleSystem;

typedef struct {
    float x;
    float y;
    float z;
} glCoord;

// Particle structure
typedef struct {
    glCoord pos;
    glCoord vec;
    float life;
    REAL startTime;
    float decay;
} ParticleInfo;

class gParticles {
public:
    gParticles(const eCoord &pos,const glCoord &vec,REAL time, ParticleSystem &param);
    virtual ~gParticles(){}

    virtual bool Timestep(REAL currentTime);
    virtual void GiveBirth(REAL currentTime = 0.0f);   // creates a new particle in the system
#ifndef DEDICATED
    virtual void Render(const eCamera *cam);
#endif

private:
    std::vector<ParticleInfo> particles; // Array of particles
    ParticleSystem psystem;  // Info for the particle system

    glCoord focus;
    glCoord svector;

    REAL startTime;
    REAL lastTime;
    int thisSystem;
};

#endif // ArmageTron_PARTICLES_H

#endif

