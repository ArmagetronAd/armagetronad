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
#include "tFunction.h"
#include "tPolynomial.h"
#include "zone/zZoneInfluence.h"

class zMonitorRule;
class zMonitor;

typedef tJUST_CONTROLLED_PTR<zMonitor> zMonitorPtr;

typedef boost::shared_ptr<zMonitorRule> zMonitorRulePtr;
typedef std::vector<zMonitorRulePtr> zMonitorRulePtrs;

typedef gVectorExtra<Triggerer> triggerers;

class zMonitor: public eReferencableGameObject {
public:
    zMonitor(eGrid * _grid):
            eReferencableGameObject( _grid, eCoord( 1,1 ), eCoord( 0,0 ), NULL, true ),
            totalInfluenceSlide(0),
            totalInfluenceAdd(0.0),
            totalInfluenceSet(0.0),
            contributorsSlide(),
            contributorsAdd(),
            contributorsSet(),
            rules(),
            valueEq(),
            drift(0),
            minValue(0.0),
            maxValue(1.0),
            previousTotalInfluenceSlide(0),
            previousTotalInfluenceAdd(0.0)
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

    void setName(string name) {
        this->name = name;
    };
    void addRule(zMonitorRulePtr aRule);

    void setInit(tPolynomial<nMessage> v) {
        valueEq = v;
    };
    void setDrift(tPolynomial<nMessage> d) {
        drift = d;
    };
    void setClampLow(REAL l)  {
        minValue = l;
    };
    void setClampHigh(REAL h) {
        maxValue = h;
    };

    void affectSlide(gCycle* user, tPolynomial<nMessage> triggererInfluenceSlide, Triad marked);
    void affectAdd(gCycle* user, REAL triggererInfluenceAdd, Triad marked);
    void affectSet(gCycle* user, REAL triggererInfluenceSet, Triad marked);

protected:
    tPolynomial<nMessage> totalInfluenceSlide; //!< The sum of all the individual contributions in one, to be scaled in time
    REAL totalInfluenceAdd; //!< The sum of all the individual contributions, NOT to be scaled in time
    REAL totalInfluenceSet; //!< The new value to overwrite the value of the monitor

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    triggerers contributorsSlide;  //!< All the players that contributed to the sliding (ie proportional to time)
    triggerers contributorsAdd;    //!< All the players that contributed to the unscaled chaning of the value
    triggerers contributorsSet;    //!< All the players that contributed to the resetting of the value

    zMonitorRulePtrs rules;

    tPolynomial<nMessage> valueEq; // The current value of the monitor
    tPolynomial<nMessage> drift; // How much the value of the monitor changes per second

    REAL minValue; //!< Low bound that value can take
    REAL maxValue; //!< High bound that value can take

    string name;

    // values used to reduce the update transmitted
    tPolynomial<nMessage> previousValue;
    tPolynomial<nMessage> previousTotalInfluenceSlide;
    REAL previousTotalInfluenceAdd;

};


class zMonitorRule {
public:
    zMonitorRule() { };
    virtual bool isValid(float monitorValue) {
        return true;
    }; // Should the rule be activated
    virtual ~zMonitorRule() { };

    void addEffectGroup(zEffectGroupPtr anEffectGroupPtr) {
        effectGroupList.push_back(anEffectGroupPtr);
    };
    void applyRule(triggerers &contributors, REAL time, const tPolynomial<nMessage> &valueEq) ;

    void addMonitorInfluence(zMonitorInfluencePtr newInfluence) {
        monitorInfluences.push_back( newInfluence );
    };
    void addZoneInfluence(zZoneInfluencePtr aZoneInfluencePtr) {
        zoneInfluences.push_back(aZoneInfluencePtr);
    };
protected:
    zEffectGroupPtrs effectGroupList;

    zMonitorInfluencePtrs monitorInfluences;
    zZoneInfluencePtrs zoneInfluences;
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
    tPolynomialMarshaler influence;
    tPolynomial<nMessage> influenceSlide;
    tFunction influenceAdd;
    tFunction influenceSet;

    bool influenceSlideAvailable;
    bool influenceAddAvailable;
    bool influenceSetAvailable;

    Triad marked;

public:
    zMonitorInfluence(zMonitorPtr aMonitor):
            monitor(aMonitor),
            influence(),
            influenceSlide(),
            influenceAdd(),
            influenceSet(),
            influenceSlideAvailable(false),
            influenceAddAvailable(false),
            influenceSetAvailable(false),
            marked(_ignore)
    { };
    ~zMonitorInfluence() { };

    void apply(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* triggerer, const tPolynomial<nMessage> &valueEq);

    void setMarked(Triad mark) {
        marked = mark;
    };
    void setInfluence(tPolynomialMarshaler infl) {
      influence = infl;
    }

    void setInfluenceSlide(tPolynomial<nMessage> infl) {
        influenceSlide = infl;
        influenceSlideAvailable=true;
    };
    void setInfluenceAdd  (tFunction infl) {
        influenceAdd   = infl;
        influenceAddAvailable  =true;
    };
    void setInfluenceSet  (tFunction infl) {
        influenceSet   = infl;
        influenceSetAvailable  =true;
    };

};


#endif //ArmageTron_Monitor_H
