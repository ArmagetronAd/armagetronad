#include <iostream>
#include <vector>

#include "zMonitor.h"
#include "tMath.h"

/*
 * Keep track of everybody contributing to the monitor (for a tic atm)
 */
void
zMonitor::affectSlide(gCycle* user, REAL triggererInfluenceSlide, Triad marked) {
    Triggerer triggerer;
    triggerer.who = user;
    triggerer.positive = triggererInfluenceSlide > 0.0 ? _true : _false;
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

    totalInfluenceSet = triggererInfluenceSet;
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
    // Do we need to reset the value?
    if (contributorsSet.size()!=0)
        value = totalInfluenceSet;

    // Compute the sliding influence, ie: proportional to time
    totalInfluenceSlide += drift;
    value += totalInfluenceSlide * (time - lastTime);

    // Computer the non-sliding influence
    value += totalInfluenceAdd;

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

    totalInfluenceSlide = 0.0;
    totalInfluenceAdd   = 0.0;
    totalInfluenceSet   = 0.0;

    // go through all the rules and find the ones to apply
    zMonitorRulePtrs::const_iterator iter;
    for(iter = rules.begin();
            iter != rules.end();
            ++iter)
    {
        // Go through all the rules of the monitor and see wich need to be activated
        if ((*iter)->isValid(value)) {
            (*iter)->applyRule(contributors, time, value);
        }
    }

    // bound the value
    clamp(value, minValue, maxValue);


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




void zMonitorRule::applyRule(triggerers &contributors, REAL time, REAL _value) {
    /* We take all the contributors */
    /* And apply the proper effect */
    miscDataPtr value = miscDataPtr(new REAL(_value));
    
    std::vector<zEffectGroupPtr>::iterator iter;
    for (iter = effectGroupList.begin();
            iter != effectGroupList.end();
            ++iter)
    {
        triggerers::const_iterator iter2;
        for (iter2 = contributors.begin();
                iter2 != contributors.end();
                ++iter2)
        {
            (*iter)->apply(*iter2, time, value);
        }
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
zMonitorInfluence::apply(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * user) {
    // Currently, we discard ownership information
    if (influenceSlideAvailable == true)
        monitor->affectSlide(user, influenceSlide, marked);
    if (influenceAddAvailable == true)
        monitor->affectAdd(user, influenceAdd, marked);
    if (influenceSetAvailable == true)
        monitor->affectSet(user, influenceSet, marked);
}
