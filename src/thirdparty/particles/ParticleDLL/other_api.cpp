// other_api.cpp
//
// Copyright 1998-2005 by David K. McAllister.
//
// This file implements the API calls that are not particle actions.

#include "papi.h"
#include "ParticleState.h"

#include <iostream>

// For Windows DLL.
#ifdef WIN32
#ifdef PARTICLE_MAKE_DLL
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif
#endif

////////////////////////////////////////////////////////
// State setting calls

PARTICLEDLL_API void pColor(float red, float green, float blue, float alpha)
{
    ParticleState &PS = _GetPState();

    delete PS.Color;
    delete PS.Alpha;
    PS.Color = new PDPoint(pVec(red, green, blue));
    PS.Alpha = new PDPoint(pVec(alpha));
}

PARTICLEDLL_API void pColorD(const pDomain &cdom)
{
    ParticleState &PS = _GetPState();
    delete PS.Color;
    delete PS.Alpha;
    PS.Color = cdom.copy();
    PS.Alpha = new PDPoint(pVec(1));
}

PARTICLEDLL_API void pColorD(const pDomain &cdom, const pDomain &adom)
{
    ParticleState &PS = _GetPState();

    delete PS.Color;
    delete PS.Alpha;
    PS.Color = cdom.copy();
    PS.Alpha = adom.copy();
}

PARTICLEDLL_API void pUpVec(const pVec &up)
{
    ParticleState &PS = _GetPState();

    delete PS.Up;
    PS.Up = new PDPoint(up);
}

PARTICLEDLL_API void pUpVecD(const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    delete PS.Up;
    PS.Up = dom.copy();
}

PARTICLEDLL_API void pVelocity(const pVec &v)
{
    ParticleState &PS = _GetPState();

    delete PS.Vel;
    PS.Vel = new PDPoint(v);
}

PARTICLEDLL_API void pVelocityD(const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    delete PS.Vel;
    PS.Vel = dom.copy();
}

PARTICLEDLL_API void pRotVelocity(const pVec &v)
{
    ParticleState &PS = _GetPState();

    delete PS.RotVel;
    PS.RotVel = new PDPoint(v);
}

PARTICLEDLL_API void pRotVelocityD(const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    delete PS.RotVel;
    PS.RotVel = dom.copy();
}

PARTICLEDLL_API void pVertexB(const pVec &v)
{
    ParticleState &PS = _GetPState();

    delete PS.VertexB;
    PS.VertexB = new PDPoint(v);
}

PARTICLEDLL_API void pVertexBD(const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    delete PS.VertexB;
    PS.VertexB = dom.copy();
}


PARTICLEDLL_API void pVertexBTracks(bool trackVertex)
{
    ParticleState &PS = _GetPState();

    PS.vertexB_tracks = trackVertex;
}

PARTICLEDLL_API void pSize(const pVec &size)
{
    ParticleState &PS = _GetPState();

    delete PS.Size;
    PS.Size = new PDPoint(size);
}

PARTICLEDLL_API void pSizeD(const pDomain &dom)
{
    ParticleState &PS = _GetPState();

    delete PS.Size;
    PS.Size = dom.copy();
}

PARTICLEDLL_API void pMass(float mass)
{
    ParticleState &PS = _GetPState();

    PS.Mass = mass;
}

PARTICLEDLL_API void pStartingAge(float age, float sigma)
{
    ParticleState &PS = _GetPState();

    PS.Age = age;
    PS.AgeSigma = sigma;
}

PARTICLEDLL_API void pTimeStep(float newDT)
{
    ParticleState &PS = _GetPState();

    PS.dt = newDT;
}

////////////////////////////////////////////////////////
// Action List Calls

PARTICLEDLL_API int pGenActionLists(int action_list_count)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return -1; // ERROR

    _PLock();

    int ind = PS.GenerateALists(action_list_count);

    _PUnLock();

    return ind;
}

PARTICLEDLL_API void pNewActionList(int action_list_num)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return; // ERROR

    _PLock();

    PS.alist_id = action_list_num;
    if(PS.alist_id < 0 || PS.alist_id >= (int)PS.ALists.size())
        return; // ERROR

    PS.in_new_list = true;
    PS.ALists[PS.alist_id].resize(0); // Remove any old actions
    // XXX Does that delete the actions?

    _PUnLock();
}

PARTICLEDLL_API void pEndActionList()
{
    ParticleState &PS = _GetPState();

    if(!PS.in_new_list)
        return; // ERROR

    PS.in_new_list = false;

    PS.alist_id = -1;
}

PARTICLEDLL_API void pDeleteActionLists(int action_list_num, int action_list_count)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return; // ERROR

    if(action_list_num < 0)
        return; // ERROR

    _PLock();

    if(action_list_num + action_list_count > (int)PS.ALists.size())
        return; // ERROR

    for(int i = action_list_num; i < action_list_num + action_list_count; i++) {
        PS.ALists[i].resize(0);
        // XXX Does that delete the actions?
    }

    _PUnLock();
}

PARTICLEDLL_API void pCallActionList(int action_list_num)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list) {
        // Add this call as an action to the current list.
        PACallActionList *S = new PACallActionList;
        S->action_list_num = action_list_num;

        PS.SendAction(S);
    } else {
        // Execute the specified action list.
        _PLock();

        if(action_list_num < 0 || action_list_num >= (int)PS.ALists.size())
            return; // ERROR

        ActionList &AList = PS.ALists[action_list_num];

        // Not sure it's safe to unlock here since AList can be accessed by another thread while
        // we're executing it, but we can't stay locked while doing all the actions or it's not parallel.
        _PUnLock();

        PS.ExecuteActionList(AList);
    }
}

////////////////////////////////////////////////////////
// Particle Group Calls

// Create particle groups, each with max_particles allocated.
PARTICLEDLL_API int pGenParticleGroups(int p_group_count, size_t max_particles)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return -1; // ERROR

    _PLock();

    int ind = PS.GeneratePGroups(p_group_count);

    for(int i = ind; i < ind + p_group_count; i++) {
        PS.PGroups[i].SetMaxParticles(max_particles);
    }

    _PUnLock();

    return ind;
}

PARTICLEDLL_API void pDeleteParticleGroups(int p_group_num, int p_group_count)
{
    ParticleState &PS = _GetPState();

    if(p_group_num < 0)
        return; // ERROR

    _PLock();

    if(p_group_num + p_group_count > (int)PS.ALists.size())
        return; // ERROR

    for(int i = p_group_num; i < p_group_num + p_group_count; i++) {
        PS.PGroups[i].SetMaxParticles(0);
        PS.PGroups[i].GetList().resize(0);
    }

    _PUnLock();
}

// Change which group is current.
PARTICLEDLL_API void pCurrentGroup(int p_group_num)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return; // ERROR

    _PLock();

    if(p_group_num < 0 || p_group_num >= (int)PS.PGroups.size())
        return; // ERROR

    _PUnLock();

    PS.pgroup_id = p_group_num;
}

// Change the maximum number of particles in the current group.
PARTICLEDLL_API size_t pSetMaxParticles(size_t max_count)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return 0; // ERROR

	// useless test, can never be negative
    // if(max_count < 0)
    //    return 0; // ERROR

    // This can kill them and call their death callback.
    PS.GetPGroup(PS.pgroup_id).SetMaxParticles(max_count);

    return max_count;
}

// Copy from the specified group to the current group.
PARTICLEDLL_API void pCopyGroup(int p_src_group_num, size_t index, size_t copy_count)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return; // ERROR

    _PLock();

    if(p_src_group_num < 0 || p_src_group_num >= (int)PS.PGroups.size())
        return; // ERROR

    ParticleGroup &srcgrp = PS.GetPGroup(p_src_group_num);

    ParticleGroup &destgrp = PS.GetPGroup(PS.pgroup_id);

    // Find out exactly how many to copy.
    size_t ccount = copy_count;
    if(ccount > srcgrp.size() - index)
        ccount = srcgrp.size() - index;
    if(ccount > destgrp.GetMaxParticles() - destgrp.size())
        ccount = destgrp.GetMaxParticles() - destgrp.size();

	// useless test, can never be negative
    // if(ccount<0)
    //     ccount = 0;

    // Directly copy the particles to the current list.
    for(size_t i=0; i<ccount; i++) {
        // Is it bad to call a birth callback while locked?
        destgrp.Add(srcgrp.GetList()[index+i]);
    }

    _PUnLock();
}

// Copy from the current group to application memory.
PARTICLEDLL_API size_t pGetParticles(size_t index, size_t count, float *verts,
                                     float *color, float *vel, float *size, float *age)
{
    ParticleState &PS = _GetPState();

    // XXX I should think about whether color means color3, color4, or what.
    // For now, it means color4.

    if(PS.in_new_list)
        return static_cast< size_t >( -1 ); // ERROR

    _PLock();

    if(PS.pgroup_id < 0 || PS.pgroup_id >= (int)PS.PGroups.size())
        return static_cast< size_t >( -2 ); // ERROR

	// useless test, both are never negative
    // if(index < 0 || count < 0)
    //    return static_cast< size_t >( -3 ); // ERROR

    ParticleGroup &pg = PS.PGroups[PS.pgroup_id];

    _PUnLock();

    if(index + count > pg.size()) {
        count = pg.size() - index;
        if(count <= 0)
            return static_cast< size_t >( -4 ); // ERROR index out of bounds.
    }

    int vi = 0, ci = 0, li = 0, si = 0, ai = 0;

    // This should be optimized.
    for(size_t i=index; i<index+count; i++) {
        const Particle &m = pg.GetList()[i];

        if(verts) {
            verts[vi++] = m.pos.x();
            verts[vi++] = m.pos.y();
            verts[vi++] = m.pos.z();
        }

        if(color) {
            color[ci++] = m.color.x();
            color[ci++] = m.color.y();
            color[ci++] = m.color.z();
            color[ci++] = m.alpha;
        }

        if(vel) {
            vel[li++] = m.vel.x();
            vel[li++] = m.vel.y();
            vel[li++] = m.vel.z();
        }

        if(size) {
            size[si++] = m.size.x();
            size[si++] = m.size.y();
            size[si++] = m.size.z();
        }

        if(age) {
            age[ai++] = m.age;
        }
    }

    return count;
}

// Return a pointer to the particle data, together with the stride IN FLOATS
// from one particle to the next and the offset IN FLOATS from the start of the particle
// for each attribute's data. The number in the arg name is how many floats the attribute
// consists of.
//
// WARNING: This function gives the application access to memory allocated and controlled
// by the Particle API. Don't do anything stupid with this power or you will regret it.
PARTICLEDLL_API size_t pGetParticlePointer(float *&ptr, size_t &stride, size_t &pos3Ofs, size_t &posB3Ofs,
        size_t &size3Ofs, size_t &vel3Ofs, size_t &velB3Ofs,
        size_t &color3Ofs, size_t &alpha1Ofs, size_t &age1Ofs)
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return static_cast< size_t >( -1 ); // ERROR

    ParticleGroup &pg = PS.PGroups[PS.pgroup_id];

    if(pg.size() < 1) {
        return static_cast< size_t >( -4 ); // ERROR index out of bounds.
    }

    ParticleList::iterator it = pg.begin();
    Particle *p0 = &(*it);
    ++it;
    Particle *p1 = &(*it);
    float *fp0 = (float *)p0;
    float *fp1 = (float *)p1;

    ptr = (float *)p0;
    stride = fp1 - fp0;
    pos3Ofs = (float *)&(p0->pos.x()) - fp0;
    posB3Ofs = (float *)&(p0->posB.x()) - fp0;
    size3Ofs = (float *)&(p0->size.x()) - fp0;
    vel3Ofs = (float *)&(p0->vel.x()) - fp0;
    velB3Ofs = (float *)&(p0->velB.x()) - fp0;
    color3Ofs = (float *)&(p0->color.x()) - fp0;
    alpha1Ofs = (float *)&(p0->alpha) - fp0;
    age1Ofs = (float *)&(p0->age) - fp0;

    return pg.size();
}

// Returns the number of particles currently in the group.
PARTICLEDLL_API size_t pGetGroupCount()
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return 0; // ERROR

    PS.GetPGroup(PS.pgroup_id);
    _PLock();

    if(PS.pgroup_id < 0 || PS.pgroup_id >= (int)PS.PGroups.size())
        return static_cast< size_t >( -2 ); // ERROR

    _PUnLock();

    return PS.PGroups[PS.pgroup_id].size();
}

// Returns the maximum number of allowed particles
PARTICLEDLL_API size_t pGetMaxParticles()
{
    ParticleState &PS = _GetPState();

    if(PS.in_new_list)
        return 0; // ERROR

    _PLock();

    if(PS.pgroup_id < 0 || PS.pgroup_id >= (int)PS.PGroups.size())
        return static_cast< size_t >( -2 ); // ERROR

    _PUnLock();

    return PS.PGroups[PS.pgroup_id].GetMaxParticles();
}

////////////////////////////////////////////////////////
// Other API Calls

PARTICLEDLL_API void pBirthCallback(P_PARTICLE_CALLBACK callback, void *data)
{
    ParticleState &PS = _GetPState();
    if(PS.in_new_list)
        return; // ERROR

    PS.GetPGroup(PS.pgroup_id).SetBirthCallback(callback, data);
}

PARTICLEDLL_API void pDeathCallback(P_PARTICLE_CALLBACK callback, void *data)
{
    ParticleState &PS = _GetPState();
    if(PS.in_new_list)
        return; // ERROR

    PS.GetPGroup(PS.pgroup_id).SetDeathCallback(callback, data);
}

PARTICLEDLL_API void pReset()
{
    ParticleState &PS = _GetPState();
    if(PS.in_new_list)
        return; // ERROR

    PS.GetPGroup(PS.pgroup_id).GetList().clear();
}

PARTICLEDLL_API void pSeed(unsigned int seed)
{
    pSRandf(seed);
}
