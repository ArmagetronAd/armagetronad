#include <iostream>
#include <vector>

#include "zMonitor.h"
#include "tMath.h"
#include "defs.h"
#include "nNetwork.h"

/*
 * Keep track of everybody contributing to the monitor (for a tic atm)
 */
void
zMonitor::affectSlide(gCycle* user, tPolynomial<nMessage> triggererInfluenceSlide, Triad marked) {
    Triggerer triggerer;
    triggerer.who = user;
    // TODO:
    //triggerer.positive = triggererInfluenceSlide > 0.0 ? _true : _false;
    triggerer.marked = marked;

    contributorsSlide.push_back(triggerer);
    totalInfluenceSlide += triggererInfluenceSlide;

}

void
zMonitor::affectAdd(gCycle* user, REAL triggererInfluenceAdd, Triad marked) {
    Triggerer triggerer;
    triggerer.who = user;
    triggerer.positive = triggererInfluenceAdd > 0.0 ? _true : _false;
    triggerer.marked = marked;

    contributorsAdd.push_back(triggerer);
    totalInfluenceAdd += triggererInfluenceAdd;
}

void
zMonitor::affectSet(gCycle* user, REAL triggererInfluenceSet, Triad marked) {
    Triggerer triggerer;
    triggerer.who = user;
    triggerer.positive = triggererInfluenceSet > 0.0 ? _true : _false;
    triggerer.marked = marked;

    contributorsSet.push_back(triggerer);
    totalInfluenceSet += triggererInfluenceSet;
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
    tPolynomial<nMessage> prevValueEq = valueEq;

    // Do we need to reset the value?
    // TODO: Re-enable those 2 lines
    //    if (contributorsSet.size()!=0)
    //        valueEq.setUnadjustableOffset(totalInfluenceSet);

    // Computer the non-sliding influence
    if ( fabs(totalInfluenceAdd - previousTotalInfluenceAdd) > 0.01) {
        valueEq.addConstant(totalInfluenceAdd);

        previousTotalInfluenceAdd = totalInfluenceAdd;
    }

    // TODO:
    //    if( fabs(totalInfluenceSlide - previousTotalInfluenceSlide) > 1e-10) {
    if ( totalInfluenceSlide != previousTotalInfluenceSlide ) {
        //      valueEq.addConstant(totalInfluenceSlide[0]);

        for (int i=1; i<totalInfluenceSlide.Len(); i++) {
            valueEq.changeRate(totalInfluenceSlide[i], i, time);
        }
        // Set to null any remainding elements not reassigned in the previous loop
        for (int i=totalInfluenceSlide.Len(); i<valueEq.Len(); i++) {
            valueEq.changeRate(0.0, i, time);
        }

        previousTotalInfluenceSlide = totalInfluenceSlide;
    }

    valueEq = valueEq.clamp(minValue, maxValue, time);

    // Assemble all the contributors in a single list
    // Nota: this also protect the list for further modification
    // by recursives rules. Should all list be merged in a later version
    // still do make a copy at this step
    triggerers contributors = contributorsSlide;
    contributors.insertAll(contributorsAdd);
    contributors.insertAll(contributorsSet);


    // Remove the information about who contributed in the last tic
    contributorsSlide.erase(contributorsSlide.begin(), contributorsSlide.end());
    contributorsAdd.erase(contributorsAdd.begin(), contributorsAdd.end());
    contributorsSet.erase(contributorsSet.begin(), contributorsSet.end());

    totalInfluenceSlide = drift;
    totalInfluenceAdd   = 0.0;
    totalInfluenceSet   = 0.0;

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




void zMonitorRule::applyRule(triggerers &contributors, REAL time, const tPolynomial<nMessage> &valueEq) {
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
zMonitorInfluence::apply(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle * user, const tPolynomial<nMessage> &valueEq) {
    // Currently, we discard ownership information

    tPolynomial<nMessage> tf = influence.marshal(valueEq);
    monitor->affectSlide(user, tf, marked);


    // TODO:
    //    if (influenceAddAvailable == true)
    //        monitor->affectAdd(user, influenceAdd(value), marked);
    //    if (influenceSetAvailable == true)
    //        monitor->affectSet(user, influenceSet(value), marked);
}
