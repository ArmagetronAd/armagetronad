// Particle.h
//
// Copyright 1998-2005 by David K. McAllister.
//
// This file contains the definition of a particle.
// It is only included by API implementation files, not by applications.

#ifndef _Particle_h
#define _Particle_h

#include "pVec.h"

// A single particle
struct Particle
{
    pVec pos;
    pVec posB;
    pVec up;
    pVec upB;
    pVec vel;
    pVec velB;	    // Used to compute binormal, normal, etc.
    pVec rvel;
    pVec rvelB;
    pVec size;
    pVec color;	    // Color must be next to alpha so glColor4fv works.
    float alpha;	// This is both cunning and scary.
    float age;
    float mass;
    long data;		// arbitrary data for user
    float tmp0;		// These temporaries are used as padding and for sorting.

    inline Particle(const pVec &pos, const pVec &posB,
                    const pVec &up, const pVec &upB,
                    const pVec &vel, const pVec &velB,
                    const pVec &rvel, const pVec &rvelB,
                    const pVec &size, const pVec &color,
                    float alpha, float age, float mass, long data, float tmp0) :
            pos(pos), posB(posB),
            up(up), upB(upB),
            vel(vel), velB(velB),
            rvel(rvel), rvelB(rvelB),
            size(size), color(color),
            alpha(alpha), age(age), mass(mass), data(data), tmp0(0)
    {
    }

    inline Particle(const Particle &rhs) :
            pos(rhs.pos), posB(rhs.posB),
            up(rhs.up), upB(rhs.upB),
            vel(rhs.vel), velB(rhs.velB),
            rvel(rhs.rvel), rvelB(rhs.rvelB),
            size(rhs.size), color(rhs.color),
            alpha(rhs.alpha), age(rhs.age), mass(rhs.mass), data(rhs.data), tmp0(rhs.tmp0)
    {
    }

    Particle() : data(0)
    {
    }

    // For sorting.
    bool operator<(const Particle &P) const
    {
        return tmp0 < P.tmp0;
    }
};

#endif
