#ifndef ArmageTron_ZoneInfluence_H
#define ArmageTron_ZoneInfluence_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "defs.h"
#include "eCoord.h"
#include "tSafePTR.h"
#include "rColor.h"

class zZone;
//typedef boost::shared_ptr< zZone > zZonePtr;
typedef tJUST_CONTROLLED_PTR< zZone> zZonePtr;

class zZoneInfluenceItem;
typedef boost::shared_ptr<zZoneInfluenceItem> zZoneInfluenceItemPtr;
typedef std::vector<zZoneInfluenceItemPtr> zZoneInfluenceItemList;

class zZoneInfluence {
protected:
    zZonePtr zone;
    zZoneInfluenceItemList zoneInfluenceItems;

public:
    zZoneInfluence(zZonePtr _zone);
    ~zZoneInfluence();
    void apply(REAL value);

    void addZoneInfluenceRule(zZoneInfluenceItemPtr aRule) {zoneInfluenceItems.push_back(aRule);};
};

class zZoneInfluenceItem {
protected:
    zZonePtr zone;
public:
    zZoneInfluenceItem(zZonePtr aZone);
    virtual ~zZoneInfluenceItem();

    virtual void apply(REAL value) {};
};

class zZoneInfluenceItemRotation : public zZoneInfluenceItem {
protected:
    REAL rotationBaseAngle; // The base component of the rotation speed
    REAL rotationValueAngle; // This component of the speed is affected by "value"
    REAL rotationBaseSpeed;
    REAL rotationValueSpeed;
public:
    zZoneInfluenceItemRotation(zZonePtr aZone);
    virtual ~zZoneInfluenceItemRotation() {};

    void set(REAL rotBaseAn, REAL rotValueAn, REAL rotBaseSp, REAL rotValueSp) {
      rotationBaseAngle = rotBaseAn; 
      rotationValueAngle = rotValueAn; 
      rotationBaseSpeed = rotBaseSp;
      rotationValueSpeed = rotValueSp;
    };
    virtual void apply(REAL value);
};

class zZoneInfluenceItemScale : public zZoneInfluenceItem {
protected:
    REAL scale;
public:
    zZoneInfluenceItemScale(zZonePtr aZone);
    virtual ~zZoneInfluenceItemScale() {};

    void set(REAL sca) { scale = sca; };
    virtual void apply(REAL value);
};

class zZoneInfluenceItemPosition : public zZoneInfluenceItem {
protected:
    eCoord pos;
public:
    zZoneInfluenceItemPosition(zZonePtr aZone);
    virtual ~zZoneInfluenceItemPosition() {};

    void set(eCoord const & p) {pos = p;};
    virtual void apply(REAL value);
};

class zZoneInfluenceItemColor : public zZoneInfluenceItem {
protected:
    rColor color;
public:
    zZoneInfluenceItemColor(zZonePtr aZone);
    virtual ~zZoneInfluenceItemColor() {};

    void set(rColor const & col) {color = col;};
    virtual void apply(REAL value);
};

#include "zZone.h"

#endif
