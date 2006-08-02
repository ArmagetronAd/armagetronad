#ifndef ArmageTron_Monitor_H
#define ArmageTron_Monitor_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "defs.h"
#include "eGameObject.h"
#include "gVectorExtra.h"
#include "gCycle.h"
#include "zone/zMisc.h"
#include "zone/zEffectGroup.h"


class zMonitorRule;
class zMonitor;

typedef boost::shared_ptr<zMonitor> zMonitorPtr;

typedef boost::shared_ptr<zMonitorRule> zMonitorRulePtr;
typedef std::vector<zMonitorRulePtr> zMonitorRulePtrs;

typedef gVectorExtra<Triggerer> triggerers;

class zMonitor: public eGameObject {
public:
    zMonitor(eGrid * _grid):
            eGameObject( _grid, eCoord( 1,1 ), eCoord( 0,0 ), NULL, true ),
            totalInfluenceSlide(0.0),
            totalInfluenceAdd(0.0),
            totalInfluenceSet(0.0),
            contributorsSlide(),
            contributorsAdd(),
            contributorsSet(),
            rules(),
            value(0.0),
            initialValue(0.0),
            drift(0.0),
            minValue(0.0),
            maxValue(1.0)
    {
        // add to game grid
        this->AddToList();
    };

    ~zMonitor() {
        //    contributors();
        //    rules();
    };

    virtual void RemoveFromGame(){
        RemoveFromListsAll();
    };

    void addRule(zMonitorRulePtr aRule);

    void setInit(REAL v) {initialValue = v;};
    void setDrift(REAL d) {drift = d;};
    void setClampLow(REAL l)  {minValue = l;};
    void setClampHigh(REAL h) {maxValue = h;};

    void affectSlide(gCycle* user, REAL triggererInfluenceSlide, Triad marked);
    void affectAdd(gCycle* user, REAL triggererInfluenceAdd, Triad marked);
    void affectSet(gCycle* user, REAL triggererInfluenceSet, Triad marked);

protected:
    REAL totalInfluenceSlide; //!< The sum of all the individual contributions in one, to be scaled in time
    REAL totalInfluenceAdd; //!< The sum of all the individual contributions, NOT to be scaled in time
    REAL totalInfluenceSet; //!< The new value to overwrite the value of the monitor

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    triggerers contributorsSlide;  //!< All the players that contributed to the sliding (ie proportional to time)
    triggerers contributorsAdd;    //!< All the players that contributed to the unscaled chaning of the value
    triggerers contributorsSet;    //!< All the players that contributed to the resetting of the value

    zMonitorRulePtrs rules;

    REAL value; // The current value of the monitor
    REAL initialValue; // When the monitor is instantiated, it should start at this value
    REAL drift; // How much the value of the monitor changes per second

    REAL minValue; //!< Low bound that value can take
    REAL maxValue; //!< High bound that value can take
};


class zMonitorRule {
public:
    zMonitorRule() { };
    virtual bool isValid(float monitorValue) {return true;}; // Should the rule be activated
    virtual ~zMonitorRule() { };

    void addEffectGroup(zEffectGroupPtr anEffectGroupPtr) {effectGroupList.push_back(anEffectGroupPtr);};
    void applyRule(triggerers &contributors, REAL time) ;

protected:
    zEffectGroupPtrs effectGroupList;

};

/*
 * Triggers when the monitor is over a value
 */
class zMonitorRuleOver : public zMonitorRule {
    REAL limit;
public:
    zMonitorRuleOver(REAL _limit):zMonitorRule(),limit(_limit) {};
    virtual ~zMonitorRuleOver() {};
    bool isValid(float monitorValue);
};

/*
 * Triggers when the monitor is under a value
 */
class zMonitorRuleUnder : public zMonitorRule {
    REAL limit;
public:
    zMonitorRuleUnder(REAL _limit):zMonitorRule(),limit(_limit) {};
    virtual ~zMonitorRuleUnder() {};
    bool isValid(float monitorValue);
};


/*
 * Triggers only in a range
 */
class zMonitorRuleInRange : public zMonitorRule {
    REAL lowLimit;
    REAL highLimit;
public:
    zMonitorRuleInRange(REAL _lowLimit, REAL _highLimit):zMonitorRule(),lowLimit(_lowLimit),highLimit(_highLimit) {};
    virtual ~zMonitorRuleInRange() {};
    bool isValid(float monitorValue);
};

/*
 * Triggers only when the monitor is outside of a range
 */
class zMonitorRuleOutsideRange : public zMonitorRule {
    REAL lowLimit;
    REAL highLimit;
public:
    zMonitorRuleOutsideRange(REAL _lowLimit, REAL _highLimit):zMonitorRule(),lowLimit(_lowLimit),highLimit(_highLimit) {};
    virtual ~zMonitorRuleOutsideRange() {};
    bool isValid(float monitorValue);
};


/*
 * The effectGroup object that influence a monitor
 */
class zMonitorInfluence {
    zMonitorPtr monitor;
    REAL influenceSlide;
    REAL influenceAdd;
    REAL influenceSet;

    bool influenceSlideAvailable;
    bool influenceAddAvailable;
    bool influenceSetAvailable;

    Triad marked;

public:
    zMonitorInfluence(zMonitorPtr aMonitor):
            monitor(aMonitor),
            influenceSlide(0),
            influenceAdd(0),
            influenceSet(0),
            influenceSlideAvailable(false),
            influenceAddAvailable(false),
            influenceSetAvailable(false),
            marked(_ignore)
    { };
    ~zMonitorInfluence() { };

    void apply(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle* triggerer);

    void setMarked(Triad mark) {marked = mark;};
    void setInfluenceSlide(REAL infl) {influenceSlide = infl; influenceSlideAvailable=true;};
    void setInfluenceAdd  (REAL infl) {influenceAdd   = infl; influenceAddAvailable  =true;};
    void setInfluenceSet  (REAL infl) {influenceSet   = infl; influenceSetAvailable  =true;};

};


#endif //ArmageTron_Monitor_H
