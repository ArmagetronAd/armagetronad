// action_api.cpp
//
// Copyright 1997-2006 by David K. McAllister
//
// This file implements the action API calls by creating
// action class instances, which are either executed or
// added to an action list.

#include "papi.h"
#include "ParticleState.h"

PARTICLEDLL_API void pAvoid(float magnitude, float epsilon, float look_ahead, const pDomain &dom)
{
    PAAvoid *S = new PAAvoid();

    S->position = dom.copy();
    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->look_ahead = look_ahead;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pBounce(float friction, float resilience, float cutoff, const pDomain &dom)
{
    PABounce *S = new PABounce();

    S->position = dom.copy();
    S->oneMinusFriction = 1.0f - friction;
    S->resilience = resilience;
    S->cutoffSqr = fsqr(cutoff);

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pCopyVertexB(bool copy_pos, bool copy_vel)
{
    PACopyVertexB *S = new PACopyVertexB;

    S->copy_pos = copy_pos;
    S->copy_vel = copy_vel;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pDamping(const pVec &damping,
                              float vlow, float vhigh)
{
    PADamping *S = new PADamping;

    S->damping = damping;
    S->vlowSqr = fsqr(vlow);
    S->vhighSqr = fsqr(vhigh);

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRotDamping(const pVec &damping,
                                 float vlow, float vhigh)
{
    PADamping *S = new PADamping;

    S->damping = damping;
    S->vlowSqr = fsqr(vlow);
    S->vhighSqr = fsqr(vhigh);

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pExplosion(const pVec &center, float velocity,
                                float magnitude, float stdev, float epsilon, float age)
{
    PAExplosion *S = new PAExplosion;

    S->center = center;
    S->velocity = velocity;
    S->magnitude = magnitude;
    S->stdev = stdev;
    S->epsilon = epsilon;
    S->age = age;

    if(S->epsilon < 0.0f)
        S->epsilon = P_EPS;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pFollow(float magnitude, float epsilon, float max_radius)
{
    PAFollow *S = new PAFollow;

    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pFountain()
{
    PAFollow *S = new PAFollow;

    S->SetKillsParticles(true);
    S->SetDoNotSegment(true);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pGravitate(float magnitude, float epsilon, float max_radius)
{
    PAGravitate *S = new PAGravitate;

    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pGravity(const pVec &dir)
{
    PAGravity *S = new PAGravity;

    S->direction = dir;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pJet(const pDomain &dom, const pDomain &accel)
{
    ParticleState &PS = _GetPState();

    PAJet *S = new PAJet();

    S->dom = dom.copy();
    S->acc = accel.copy();

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pKillOld(float age_limit, bool kill_less_than)
{
    PAKillOld *S = new PAKillOld;

    S->age_limit = age_limit;
    S->kill_less_than = kill_less_than;

    S->SetKillsParticles(true);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pMatchVelocity(float magnitude, float epsilon, float max_radius)
{
    PAMatchVelocity *S = new PAMatchVelocity;

    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pMatchRotVelocity(float magnitude, float epsilon, float max_radius)
{
    PAMatchRotVelocity *S = new PAMatchRotVelocity;

    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pMove()
{
    PAMove *S = new PAMove;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pOrbitLine(const pVec &p, const pVec &axis,
                                float magnitude, float epsilon, float max_radius)
{
    PAOrbitLine *S = new PAOrbitLine;

    S->p = p;
    S->axis = axis;
    S->axis.normalize();
    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pOrbitPoint(const pVec &center, float magnitude, float epsilon, float max_radius)
{
    PAOrbitPoint *S = new PAOrbitPoint;

    S->center = center;
    S->magnitude = magnitude;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRandomAccel(const pDomain &dom)
{
    PARandomAccel *S = new PARandomAccel();

    S->gen_acc = dom.copy();
    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRandomDisplace(const pDomain &dom)
{
    PARandomDisplace *S = new PARandomDisplace();

    S->gen_disp = dom.copy();
    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRandomVelocity(const pDomain &dom)
{
    PARandomVelocity *S = new PARandomVelocity();

    S->gen_vel = dom.copy();
    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRandomRotVelocity(const pDomain &dom)
{
    PARandomRotVelocity *S = new PARandomRotVelocity();

    S->gen_vel = dom.copy();
    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pRestore(float time_left, bool vel, bool rvel)
{
    PARestore *S = new PARestore;

    S->time_left = time_left;
    S->restore_velocity = vel;
    S->restore_rvelocity = rvel;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pSink(bool kill_inside, const pDomain &dom)
{
    PASink *S = new PASink();

    S->position = dom.copy();
    S->kill_inside = kill_inside;

    S->SetKillsParticles(true);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pSinkVelocity(bool kill_inside, const pDomain &dom)
{
    PASinkVelocity *S = new PASinkVelocity();

    S->velocity = dom.copy();
    S->kill_inside = kill_inside;

    S->SetKillsParticles(true);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pSort(const pVec &eye, const pVec &look)
{
    PASort *S = new PASort;

    S->Eye = eye;
    S->Look= look;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true); // WARNING: Particles aren't a function of other particles, but since it can screw up the working set thing, I'm setting it true.

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pSource(float particle_rate, const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    PASource *S = new PASource();

    S->position = dom.copy();
    S->positionB = PS.VertexB->copy();
    S->upVec = PS.Up->copy();
    S->velocity = PS.Vel->copy();
    S->rvelocity = PS.RotVel->copy();
    S->size = PS.Size->copy();
    S->color = PS.Color->copy();
    S->alpha = PS.Alpha->copy();
    S->particle_rate = particle_rate;
    S->age = PS.Age;
    S->age_sigma = PS.AgeSigma;
    S->vertexB_tracks = PS.vertexB_tracks;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(true); // WARNING: Particles aren't a function of other particles, but does affect the working sets optimizations

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pSpeedLimit(float min_speed, float max_speed)
{
    PASpeedLimit *S = new PASpeedLimit;

    S->min_speed = min_speed;
    S->max_speed = max_speed;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pTargetColor(const pVec &color, float alpha, float scale)
{
    PATargetColor *S = new PATargetColor;

    S->color = color;
    S->alpha = alpha;
    S->scale = scale;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pTargetSize(const pVec &size, const pVec &scale)
{
    PATargetSize *S = new PATargetSize;

    S->size = size;
    S->scale = scale;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pTargetVelocity(const pVec &vel, float scale)
{
    PATargetVelocity *S = new PATargetVelocity;

    S->velocity = vel;
    S->scale = scale;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

PARTICLEDLL_API void pTargetRotVelocity(const pVec &vel, float scale)
{
    PATargetRotVelocity *S = new PATargetRotVelocity;

    S->velocity = vel;
    S->scale = scale;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}

// If in immediate mode, quickly add a vertex.
// If building an action list, call pSource.
PARTICLEDLL_API void pVertex(const pVec &pos, long data)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list) {
        pSource(1, PDPoint(pos));
        return;
    }

    // Immediate mode. Quickly add the vertex.
    pVec siz, up, vel, rvel, col, alpha, posB;
    if(PS.vertexB_tracks)
        posB = pos;
    else
        posB = PS.VertexB->Generate();
    siz = PS.Size->Generate();
    up = PS.Up->Generate();
    vel = PS.Vel->Generate();
    rvel = PS.RotVel->Generate();
    col = PS.Color->Generate();
    alpha = PS.Alpha->Generate();
    PS.GetPGroup(PS.pgroup_id).Add(pos, posB, up, vel, rvel, siz, col, alpha.x(), PS.Age, PS.Mass, data);
}

PARTICLEDLL_API void pVortex(const pVec &tip, const pVec &axis,
                             float magnitude, float tightnessExponent, float rotSpeed,
                             float epsilon, float max_radius)
{
    PAVortex *S = new PAVortex;

    S->tip = tip;
    S->axis = axis;
    S->magnitude = magnitude;
    S->tightnessExponent = tightnessExponent;
    S->rotSpeed = rotSpeed;
    S->epsilon = epsilon;
    S->max_radius = max_radius;

    S->SetKillsParticles(false);
    S->SetDoNotSegment(false);

    _GetPState().SendAction(S);
}
