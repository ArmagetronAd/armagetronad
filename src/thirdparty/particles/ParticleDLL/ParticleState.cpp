// ParticleState.cpp
//
// Copyright 1998-2005 by David K. McAllister.
//
// This file implements the under-the-hood stuff that is not part of an API call.

// To optimize action lists, at execute time check whether the first action can be combined with
// the next action. If so, combine them. Then try again. When the current one can't be combined
// with the next one anymore, execute it.

// Doing this at run time instead of compile time should make it easier to store the state of
// compound actions.

#include "ParticleState.h"
#include "papi.h"

#include <iostream>
#include <typeinfo>

// This is global just as a cheezy way to change dt for all actions easily.
float PActionBase::dt;

std::vector<ParticleGroup> ParticleState::PGroups;
std::vector<ActionList> ParticleState::ALists;

// This is the global state. It is used for single-threaded apps.
// Multithreaded apps should get a different ParticleState per thread from a hash table.
ParticleState __ps;

const pVec P010(0.0f, 1.0f, 0.0f);
const pVec P000(0.0f, 0.0f, 0.0f);
const pVec P111(1.0f, 1.0f, 1.0f);

ParticleState::ParticleState() : Up(new PDPoint(P010)), Vel(new PDPoint(P000)), RotVel(new PDPoint(P000)),
        VertexB(new PDPoint(P000)), Size(new PDPoint(P111)), Color(new PDPoint(P111)), Alpha(new PDPoint(P111))
{
    in_call_list = false;
    in_new_list = false;
    vertexB_tracks = true;

    dt = 1.0f;

    pgroup_id = -1;
    alist_id = -1;
    tid = 0; // This must be filled in if we're multi-threaded.
    ErrorCode = PERR_NO_ERROR;

    Age = 0.0f;
    AgeSigma = 0.0f;
    Mass = 1.0f;
}

ParticleState::~ParticleState()
{
    delete Up;
    delete Vel;
    delete RotVel;
    delete VertexB;
    delete Size;
    delete Color;
    delete Alpha;
}

void ParticleState::SetError(const int err, const std::string &Str)
{
    std::cerr << Str << std::endl;

    if(ErrorCode == PERR_NO_ERROR) // Keep the first error.
        ErrorCode = err;

    abort();
}

ParticleGroup &ParticleState::GetPGroup(int p_group_num)
{
    if(p_group_num < 0) {
        std::cerr << "ERROR: Negative particle group number\n";
        abort(); // IERROR
    }

    if(p_group_num >= (int)PGroups.size()) {
        std::cerr << "ERROR: Bad particle group number\n";
        abort(); // IERROR
    }

    return PGroups[p_group_num];
}

ActionList &ParticleState::GetAList(int a_list_num)
{
    if(a_list_num < 0) {
        std::cerr << "ERROR: Negative action list number\n";
        abort(); // IERROR
    }

    if(a_list_num >= (int)ALists.size()) {
        std::cerr << "ERROR: Bad action list number\n";
        abort(); // IERROR
    }

    return ALists[a_list_num];
}

// Return an index into the list of particle groups where
// p_group_count groups can be added.
int ParticleState::GeneratePGroups(int pgroups_requested)
{
    int old_size = (int)PGroups.size();
    PGroups.resize(old_size + pgroups_requested);

    return old_size;
}

// Return an index into the list of action lists where
// alists_requested lists can be added.
int ParticleState::GenerateALists(int alists_requested)
{
    int old_size = (int)ALists.size();
    ALists.resize(old_size + alists_requested);

    return old_size;
}

// Action API entry points call this to either store the action in a list or execute and delete it.
void ParticleState::SendAction(PActionBase *S)
{
    if(in_new_list) {
        // Add action S to the end of the current action list.
        ActionList &AList = GetAList(alist_id);
        AList.push_back(S);
    } else {
        // Immediate mode. Execute it.
        S->dt = dt; // This is a hack to provide local access to dt.
        ParticleGroup &pg = GetPGroup(pgroup_id);
        S->Execute(pg, pg.begin(), pg.end());
        delete S;
    }
}

// Execute an action list
void ParticleState::ExecuteActionList(ActionList &AList)
{
    ParticleGroup &pg = GetPGroup(pgroup_id);
    in_call_list = true;

    ActionList::iterator it = AList.begin();
    while(it != AList.end()) {
        // Make an action segment
        ActionList::iterator abeg = it;
        ActionList::iterator aend = it+1;

        // If the first one is connectable, try to connect some more.
        if(!(*abeg)->GetKillsParticles() && !(*abeg)->GetDoNotSegment())
            while(aend != AList.end() && !(*aend)->GetKillsParticles() && !(*aend)->GetDoNotSegment())
                aend++;

        // Found a sub-list that can be done together. Now do them.
        ParticleList::iterator pbeg = pg.begin();
        ParticleList::iterator pend = min(pbeg + PWorkingSetSize, pg.end());
        bool one_pass = false;
        if(aend - abeg == 1) {
            pend = pg.end(); // If a single action, do the whole thing in one whack.
            one_pass = true;
        }

        ActionList::iterator ait = abeg;
        do {
            // For each chunk of particles, do all the actions in this sub-list
            ait = abeg;
            while(ait < aend) {
                (*ait)->dt = dt; // This is a hack to provide local access to dt.
                (*ait)->Execute(pg, pbeg, pend);

                // This is a hack to handle our compound object experiment.
                if(typeid(ait) == typeid(PAFountain))
                    ait += 6;
                else
                    ait++;
            }
            pbeg = pend;
            pend = min(pend + PWorkingSetSize, pg.end());
        } while (!one_pass && pbeg != pg.end());
        it = ait;
    }
    in_call_list = false;
}
