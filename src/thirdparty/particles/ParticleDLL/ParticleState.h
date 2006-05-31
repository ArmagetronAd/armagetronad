// ParticleState.h
//
// Copyright 1998-2005 by David K. McAllister.
//
// Defines these classes: ParticleState

#ifndef PARTICLESTATE_H
#define PARTICLESTATE_H

#include "papi.h"
#include "actions.h"
#include "ParticleGroup.h"

#include <vector>
#include <string>

typedef std::vector<PActionBase *> ActionList;

// This is the per-thread state of the API.
// All API calls get their data from here.
// In the non-multithreaded case there is one global instance of this class.
struct ParticleState
{
    // Any particles created will get their attributes from here.
    pDomain *Up;
    pDomain *Vel;
    pDomain *RotVel;
    pDomain *VertexB;
    pDomain *Size;
    pDomain *Color;
    pDomain *Alpha;
    float Age;
    float AgeSigma;
    float Mass;

    float dt;
    bool in_call_list;
    bool in_new_list;
    bool vertexB_tracks;
    int tid;			// Only used in the multi-threaded case, but always define it.
    int ErrorCode;		// Stores the code of the first error this thread has hit since its latest pGetError() call.

    static std::vector<ParticleGroup> PGroups; // Static since all threads access the same action lists. Lock all accesses.
    int pgroup_id;
    int GeneratePGroups(int pgroups_requested);
    ParticleGroup &GetPGroup(int p_group_num);

    static std::vector<ActionList> ALists; // Static since all threads access the same action lists. Lock all accesses.
    int alist_id;
    int GenerateALists(int alists_requested);
    ActionList &GetAList(int a_list_num);

    ParticleState();
    ~ParticleState();

    // Action API entry points call this to either store the action in a list or execute and delete it.
    void SendAction(PActionBase *S);

    // Execute an action list
    void ExecuteActionList(ActionList &AList);

    // Internal functions call this to declare an error
    void SetError(const int err, const std::string &str);
};

#ifdef PARTICLE_MP
// All entry points call this to get their particle state.
inline ParticleState &_GetPState()
{
    // Returns a reference to the appropriate particle state.
    extern ParticleState &_GetPStateWithTID();

    return _GetPStateWithTID();
}

inline void _PLock() { /* Do lock stuff here. */ }
inline void _PUnLock() { /* Do unlock stuff here. */ }

#else

// All entry points call this to get their particle state.
// For the non-multi-threaded case this is practically a no-op.
inline ParticleState &_GetPState()
{
    // This is the global state.
    extern ParticleState __ps;

    return __ps;
}

inline void _PLock() {}
inline void _PUnLock() {}

#endif

#define PASSERT(x,y) {if(!(x)) { _GetPState().SetError(PERR_INTERNAL_ERROR, (y)); }}

#endif // PARTICLESTATE_H
