#include <iostream>
#include <vector>

#include "zMonitor.h"
#include "tMath.h"
#include "defs.h"
#include "nNetwork.h"

// *******************************************************************************
// *
// *   Monitors
// *
// *******************************************************************************
//!
//!        @param      none
//!        @return     reference to static std::map of monitor pointers
//!
// *******************************************************************************
zMonitor::monitorMap& zMonitor::Monitors() {
    static zMonitor::monitorMap monitors;
    return monitors;
}

/*
 * Keep track of everybody contributing to the monitor (for a tic atm)
 */
void
zMonitor::affectSlide(gCycle* user, tPolynomial triggererInfluence, Triad marked) {
    Triggerer triggerer;
    triggerer.who = user;
    // TODO:
    //triggerer.positive = triggererInfluence > 0.0 ? _true : _false;
    triggerer.marked = marked;

    contributors.push_back(triggerer);
    totalInfluence += triggererInfluence;

}


// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool zMonitor::Timestep( REAL time )
{
    tPolynomial prevValueEq = valueEq;

    if ( totalInfluence != previousTotalInfluenceSlide ) {

        for (int i=1; i<totalInfluence.Len(); i++) {
            valueEq.changeRate(totalInfluence[i], i, time);
        }
        // Set to null any remainding elements not reassigned in the previous loop
        for (int i=totalInfluence.Len(); i<valueEq.Len(); i++) {
            valueEq.changeRate(0.0, i, time);
        }

        previousTotalInfluenceSlide = totalInfluence;
    }

    valueEq = valueEq.clamp(minValue, maxValue, time);

    // Assemble all the contributors in a single list
    // Nota: this also protect the list for further modification
    // by recursives rules. Should all list be merged in a later version
    // still do make a copy at this step


    totalInfluence = drift;

    // Only update if value has changed enough
    // TODO:
    //    if( value < previousValue - EPS || previousValue + EPS < value) {
    // go through all the rules and find the ones to apply
    zMonitorRulePtrs::const_iterator iter;
    for (iter = rules.begin();
            iter != rules.end();
            ++iter)
    {
        // Go through all the rules of the monitor and see wich need to be activated
        if ((*iter)->isValid(valueEq.evaluate(time))) {
            (*iter)->applyRule(contributors, time, valueEq);
        }
    }
    //    }

    // Remove the information about who contributed in the last tic
    contributors.erase(contributors.begin(), contributors.end());

    // update time
    lastTime = time;

    return false;
}


void
zMonitor::addRule(zMonitorRulePtr aRule) {
    /*
     * HACK
     * This only works for a single rule! fix this
     */
    rules.push_back(aRule);
}




void zMonitorRule::applyRule(triggerers &contributors, REAL time, const tPolynomial &valueEq) {
    /* We take all the contributors */
    /* And apply the proper effect */

    // Go through all effectgroups (owners of some action)
    std::vector<zEffectGroupPtr>::iterator iter;
    for (iter = effectGroupList.begin();
            iter != effectGroupList.end();
            ++iter)
    {
        // Go through all the categories of triggerer (ie: people who have right to trigger an action)
        triggerers::const_iterator iter2;
        for (iter2 = contributors.begin();
                iter2 != contributors.end();
                ++iter2)
        {
            (*iter)->apply(*iter2, time, valueEq);
        }
    }

    gVectorExtra< nNetObjectID > owners;
    gVectorExtra< nNetObjectID > teamOwners;

    zMonitorInfluencePtrs::const_iterator iterMonitorInfluence;
    for (iterMonitorInfluence=monitorInfluences.begin();
            iterMonitorInfluence!=monitorInfluences.end();
            ++iterMonitorInfluence)
    {
        // TODO: pass the whole valueEq
        (*iterMonitorInfluence)->apply(owners, teamOwners, (gCycle*)0, valueEq);
    }

    zZoneInfluencePtrs::const_iterator iterZoneInfluence;
    for (iterZoneInfluence=zoneInfluences.begin();
            iterZoneInfluence!=zoneInfluences.end();
            ++iterZoneInfluence)
    {
        (*iterZoneInfluence)->apply(valueEq);
    }
}

/*
 * Triggers when the monitor is over a value
 */
bool
zMonitorRuleOver::isValid(float monitorValue) {
    return (monitorValue>=limit);
}

/*
 * Triggers when the monitor is under a value
 */
bool
zMonitorRuleUnder::isValid(float monitorValue) {
    return (monitorValue<=limit);
}


/*
 * Triggers only in a range
 */
bool
zMonitorRuleInRange::isValid(float monitorValue) {
    return (lowLimit<= monitorValue && monitorValue<=highLimit);
}

/*
 * Triggers only when the monitor is outside of a range
 */
bool
zMonitorRuleOutsideRange::isValid(float monitorValue) {
    return (monitorValue <= lowLimit  || highLimit <= monitorValue);
}

void
zMonitorInfluence::apply(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle * user, const tPolynomial &valueEq) {
    // Currently, we discard ownership information

    tPolynomial tf = influence.marshal(valueEq);
    monitor->affectSlide(user, tf, marked);

}
