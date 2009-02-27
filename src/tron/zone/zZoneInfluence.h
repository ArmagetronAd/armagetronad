#ifndef ArmageTron_ZoneInfluence_H
#define ArmageTron_ZoneInfluence_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "defs.h"
#include "eCoord.h"
#include "tSafePTR.h"
#include "rColor.h"
#include "tFunction.h"
#include "tPolynomial.h"
#include "tPolynomialMarshaler.h"
#include "nNetwork.h"

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
    zZoneInfluence();
    zZoneInfluence(zZonePtr _zone);
    ~zZoneInfluence();

    void bindZone(zZonePtr _zone);

    void apply(const tPolynomial &value);

    void addZoneInfluenceRule(zZoneInfluenceItemPtr aRule) {
        zoneInfluenceItems.push_back(aRule);
    };
};

class zZoneInfluenceItem {
protected:
    zZonePtr zone;
public:
    zZoneInfluenceItem(zZonePtr aZone);
    virtual ~zZoneInfluenceItem();

    virtual void apply(const tPolynomial &value) {};
};

class zZoneInfluenceItemRotation : public zZoneInfluenceItem {
protected:
    tPolynomialMarshaler rotation;
public:
    zZoneInfluenceItemRotation(zZonePtr aZone);
    virtual ~zZoneInfluenceItemRotation() {};

    void set(const tPolynomialMarshaler & other)
    {
	rotation = other;
    }

    virtual void apply(const tPolynomial &value);
};

class zZoneInfluenceItemScale : public zZoneInfluenceItem {
protected:
    REAL scale;
public:
    zZoneInfluenceItemScale(zZonePtr aZone);
    virtual ~zZoneInfluenceItemScale() {};

    void set(REAL sca) {
        scale = sca;
    };
    virtual void apply(const tPolynomial &value);
};

class zZoneInfluenceItemPosition : public zZoneInfluenceItem {
protected:
    eCoord pos;
public:
    zZoneInfluenceItemPosition(zZonePtr aZone);
    virtual ~zZoneInfluenceItemPosition() {};

    void set(eCoord const & p) {
        pos = p;
    };
    virtual void apply(const tPolynomial &value);
};

class zZoneInfluenceItemColor : public zZoneInfluenceItem {
protected:
    rColor color;
public:
    zZoneInfluenceItemColor(zZonePtr aZone);
    virtual ~zZoneInfluenceItemColor() {};

    void set(rColor const & col) {
        color = col;
        color.a_ = color.a_ < 0.0?0.0:(color.a_>0.7?0.7:color.a_);
    };
    virtual void apply(const tPolynomial &value);
};

#include "zZone.h"

#endif
