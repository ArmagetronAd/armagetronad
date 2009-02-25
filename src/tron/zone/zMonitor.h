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
#include "tPolynomialMarshaler.h"
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
            totalInfluence(0),
            contributors(),
            rules(),
            valueEq(),
            drift(0),
            minValue(0.0),
            maxValue(1.0),
            previousTotalInfluenceSlide(0)
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

    void setInit(tPolynomial v) {
        valueEq = v;
    };
    void setDrift(tPolynomial d) {
        drift = d;
    };
    void setClampLow(REAL l)  {
        minValue = l;
    };
    void setClampHigh(REAL h) {
        maxValue = h;
    };

    void affectSlide(gCycle* user, tPolynomial triggererInfluenceSlide, Triad marked);

protected:
    tPolynomial totalInfluence; //!< The sum of all the individual contributions in one, to be scaled in time

    virtual bool Timestep(REAL currentTime);     //!< simulates behaviour up to currentTime

    triggerers contributors;  //!< All the players that contributed to the sliding (ie proportional to time)

    zMonitorRulePtrs rules;

    tPolynomial valueEq; // The current value of the monitor
    tPolynomial drift; // How much the value of the monitor changes per second

    REAL minValue; //!< Low bound that value can take
    REAL maxValue; //!< High bound that value can take

    string name;

    // values used to reduce the update transmitted
    tPolynomial previousValue;
    tPolynomial previousTotalInfluenceSlide;


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
    void applyRule(triggerers &contributors, REAL time, const tPolynomial &valueEq) ;

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
    tPolynomial influenceSlide;

    bool influenceSlideAvailable;

    Triad marked;

public:
    zMonitorInfluence(zMonitorPtr aMonitor):
            monitor(aMonitor),
            influence(),
            influenceSlide(),
            influenceSlideAvailable(false),
            marked(_ignore)
    { };
    ~zMonitorInfluence() { };

    void apply(gVectorExtra< nNetObjectID > &owners, gVectorExtra< nNetObjectID > &teamOwners, gCycle* triggerer, const tPolynomial &valueEq);

    void setMarked(Triad mark) {
        marked = mark;
    };
    void setInfluence(tPolynomialMarshaler infl) {
      influence = infl;
    }

    void setInfluenceSlide(tPolynomial infl) {
        influenceSlide = infl;
        influenceSlideAvailable=true;
    };

};


#endif //ArmageTron_Monitor_H
