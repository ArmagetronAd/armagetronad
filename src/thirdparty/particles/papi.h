// papi.h
//
// Copyright 1997-2006 by David K. McAllister
// http://www.cs.unc.edu/~davemc/Particle
//
// Include this file in all applications that use the Particle System API.

#ifndef _particle_api_h
#define _particle_api_h

#include "./pDomain.h"

// Defines NULL.
#include <stdlib.h>

// This is the major and minor version number of this release of the API.
#define P_VERSION 200

#ifdef WIN32
// Define PARTICLE_MAKE_DLL in the project file to make the DLL.
#ifdef PARTICLE_MAKE_DLL
#include <windows.h>

#ifdef PARTICLEDLL_EXPORTS
#define PARTICLEDLL_API __declspec(dllexport)
#else
#define PARTICLEDLL_API __declspec(dllimport)
#endif

#else
#define PARTICLEDLL_API
#endif
#else
#define PARTICLEDLL_API
#endif

// Actually this must be < sqrt(MAXFLOAT) since we store this value squared.
#define P_MAXFLOAT 1.0e16f

#ifdef MAXINT
#define P_MAXINT MAXINT
#else
#define P_MAXINT 0x7fffffff
#endif

#define P_EPS 1e-3f

// State setting calls

PARTICLEDLL_API void pColor(float red, float green, float blue, float alpha = 1.0f);

PARTICLEDLL_API void pColorD(const pDomain &cdom);
PARTICLEDLL_API void pColorD(const pDomain &cdom, const pDomain &adom);

PARTICLEDLL_API void pSize(const pVec &size);

PARTICLEDLL_API void pSizeD(const pDomain &dom);

PARTICLEDLL_API void pMass(float mass);

PARTICLEDLL_API void pStartingAge(float age, float sigma = 1.0f);

PARTICLEDLL_API void pTimeStep(float new_dt);

PARTICLEDLL_API void pUpVec(const pVec &v);

PARTICLEDLL_API void pUpVecD(const pDomain &dom);

PARTICLEDLL_API void pVelocity(const pVec &vel);

PARTICLEDLL_API void pVelocityD(const pDomain &dom);

PARTICLEDLL_API void pRotVelocity(const pVec &v);

PARTICLEDLL_API void pRotVelocityD(const pDomain &dom);

PARTICLEDLL_API void pVertexB(const pVec &v);

PARTICLEDLL_API void pVertexBD(const pDomain &dom);

PARTICLEDLL_API void pVertexBTracks(bool track_vertex = true);


// Action List Calls

PARTICLEDLL_API void pCallActionList(int action_list_num);

PARTICLEDLL_API void pDeleteActionLists(int action_list_num, int action_list_count = 1);

PARTICLEDLL_API void pEndActionList();

PARTICLEDLL_API int pGenActionLists(int action_list_count = 1);

PARTICLEDLL_API void pNewActionList(int action_list_num);


// Particle Group Calls

PARTICLEDLL_API void pCopyGroup(int p_src_group_num, size_t index = 0, size_t copy_count = P_MAXINT);

PARTICLEDLL_API void pCurrentGroup(int p_group_num);

PARTICLEDLL_API void pDeleteParticleGroups(int p_group_num, int p_group_count = 1);

PARTICLEDLL_API int pGenParticleGroups(int p_group_count = 1, size_t max_particles = 0);

PARTICLEDLL_API size_t pGetGroupCount();

PARTICLEDLL_API size_t pGetParticles(size_t index, size_t count, float *position = NULL, float *color = NULL,
                                     float *vel = NULL, float *size = NULL, float *age = NULL);

PARTICLEDLL_API size_t pGetParticlePointer(float *&ptr, size_t &stride, size_t &pos3Ofs, size_t &posB3Ofs,
        size_t &size3Ofs, size_t &vel3Ofs, size_t &velB3Ofs,
        size_t &color3Ofs, size_t &alpha1Ofs, size_t &age1Ofs);

PARTICLEDLL_API size_t pSetMaxParticles(size_t max_count);

PARTICLEDLL_API size_t pGetMaxParticles();


// Actions

PARTICLEDLL_API void pAvoid(float magnitude, float epsilon, float look_ahead, const pDomain &dom);

PARTICLEDLL_API void pBounce(float friction, float resilience, float cutoff, const pDomain &dom);

PARTICLEDLL_API void pCopyVertexB(bool copy_pos = true, bool copy_vel = false);

PARTICLEDLL_API void pDamping(const pVec &damping, float vlow = 0.0f, float vhigh = P_MAXFLOAT);

PARTICLEDLL_API void pRotDamping(const pVec &damping, float vlow = 0.0f, float vhigh = P_MAXFLOAT);

PARTICLEDLL_API void pExplosion(const pVec &center, float velocity,
                                float magnitude, float stdev, float epsilon = P_EPS, float age = 0.0f);

PARTICLEDLL_API void pFollow(float magnitude = 1.0f, float epsilon = P_EPS, float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pFountain(); // EXPERIMENTAL. DO NOT USE!

PARTICLEDLL_API void pGravitate(float magnitude = 1.0f, float epsilon = P_EPS, float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pGravity(const pVec &dir);

PARTICLEDLL_API void pJet(const pDomain &dom, const pDomain &acc);

PARTICLEDLL_API void pKillOld(float age_limit, bool kill_less_than = false);

PARTICLEDLL_API void pMatchVelocity(float magnitude = 1.0f, float epsilon = P_EPS,
                                    float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pMatchRotVelocity(float magnitude = 1.0f, float epsilon = P_EPS,
                                       float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pMove();

PARTICLEDLL_API void pOrbitLine(const pVec &p, const pVec &axis, float magnitude = 1.0f,
                                float epsilon = P_EPS, float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pOrbitPoint(const pVec &center, float magnitude = 1.0f, float epsilon = P_EPS,
                                 float max_radius = P_MAXFLOAT);

PARTICLEDLL_API void pRandomAccel(const pDomain &dom);

PARTICLEDLL_API void pRandomDisplace(const pDomain &dom);

PARTICLEDLL_API void pRandomVelocity(const pDomain &dom);

PARTICLEDLL_API void pRandomRotVelocity(const pDomain &dom);

PARTICLEDLL_API void pRestore(float time, bool vel = true, bool rvel = true);

PARTICLEDLL_API void pSink(bool kill_inside, const pDomain &dom);

PARTICLEDLL_API void pSinkVelocity(bool kill_inside, const pDomain &dom);

PARTICLEDLL_API void pSort(const pVec &eye, const pVec &look);

PARTICLEDLL_API void pSource(float particle_rate, const pDomain &dom);

PARTICLEDLL_API void pSpeedLimit(float min_speed, float max_speed = P_MAXFLOAT);

PARTICLEDLL_API void pTargetColor(const pVec &color, float alpha, float scale);

PARTICLEDLL_API void pTargetSize(const pVec &size, const pVec &scale);

PARTICLEDLL_API void pTargetVelocity(const pVec &vel, float scale);

PARTICLEDLL_API void pTargetRotVelocity(const pVec &vel, float scale);

PARTICLEDLL_API void pVertex(const pVec &v, long data = 0);

PARTICLEDLL_API void pVortex(const pVec &center, const pVec &axis, float magnitude = 1.0f,
                             float tightnessExponent = 1.0f, float rotSpeed = 1.0f,
                             float epsilon = P_EPS, float max_radius = P_MAXFLOAT);

/////////////////////////////////////////////////////
// Other stuff

typedef void (*P_PARTICLE_CALLBACK)(struct Particle &particle, void *data);

PARTICLEDLL_API void pBirthCallback(P_PARTICLE_CALLBACK callback, void *data = NULL);
PARTICLEDLL_API void pDeathCallback(P_PARTICLE_CALLBACK callback, void *data = NULL);

PARTICLEDLL_API void pReset();
PARTICLEDLL_API void pSeed(unsigned int seed);

enum ErrorCodes {
    PERR_NO_ERROR = 0,
    PERR_NOT_IMPLEMENTED = 1,
    PERR_INTERNAL_ERROR = 2
};

// Returns the first error hit in this thread
PARTICLEDLL_API int pGetError();

#endif
