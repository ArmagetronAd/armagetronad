#include "zZoneInfluence.h"

zZoneInfluence::zZoneInfluence(zZonePtr _zone) : zone(_zone), zoneInfluenceItems() { }

zZoneInfluence::~zZoneInfluence() { }

void
zZoneInfluence::apply(const tPolynomial<nMessage> &value)
{
    zZoneInfluenceItemList::const_iterator iter;
    for(iter=zoneInfluenceItems.begin();
            iter!=zoneInfluenceItems.end();
            ++iter)
    {
        (*iter)->apply(value);
    }

    zone->RequestSync();
}

zZoneInfluenceItem::zZoneInfluenceItem(zZonePtr aZone):zone(aZone) {}

zZoneInfluenceItem::~zZoneInfluenceItem() {}


zZoneInfluenceItemRotation::zZoneInfluenceItemRotation(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        rotationAngle(),
        rotationSpeed()
{}

void
zZoneInfluenceItemRotation::apply(const tPolynomial<nMessage> &valueEq) {
    // x is the influence from the monitor value
    // t is the influence from the current time
    // tf = (a +b*x) + (c + d*x)*t

    // The following should improve readability
    REAL tData[] = {0.0, 1.0};
    tPolynomial<nMessage> t(tData, sizeof(tData)/sizeof(tData[0]));

    REAL a = rotationAngle.GetOffset();
    REAL b = rotationAngle.GetSlope();
    REAL c = rotationSpeed.GetOffset();
    REAL d = rotationSpeed.GetSlope();

    tPolynomial<nMessage> tf = 
      ( (valueEq * b) + a)
      + t * ( (valueEq * d) + c);

    zone->getShape()->setRotation2( tf );
}

zZoneInfluenceItemScale::zZoneInfluenceItemScale(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        scale(0.0)
{}

void
zZoneInfluenceItemScale::apply(const tPolynomial<nMessage> &value) {
    tFunction tfScale;
    tfScale.SetOffset( scale );
    tfScale.SetSlope( 0.0f );
    zone->getShape()->setScale( tfScale );
}

zZoneInfluenceItemPosition::zZoneInfluenceItemPosition(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        pos(0.0, 0.0)
{}

void
zZoneInfluenceItemPosition::apply(const tPolynomial<nMessage> &value) {
    tFunction tfPosition;

    tfPosition.SetOffset( pos.x );
    tfPosition.SetSlope( 0.0f );
    zone->getShape()->setPosX( tfPosition );

    tfPosition.SetOffset( pos.y );
    zone->getShape()->setPosY( tfPosition );
}

zZoneInfluenceItemColor::zZoneInfluenceItemColor(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        color(0.0, 0.0, 0.0, 0.0)
{}

void
zZoneInfluenceItemColor::apply(const tPolynomial<nMessage> &value) {
    zone->getShape()->setColorNow( color );
}

