#include "zZoneInfluence.h"

zZoneInfluence::zZoneInfluence() : zone(), zoneInfluenceItems() { }

zZoneInfluence::zZoneInfluence(zZonePtr _zone) : zone(_zone), zoneInfluenceItems() { }

zZoneInfluence::~zZoneInfluence() { }

void
zZoneInfluence::bindZone(zZonePtr _zone)
{
    zone = _zone;
}

void
zZoneInfluence::apply(const tPolynomial &value)
{
    if (!zone)
        return;

    zZoneInfluenceItemList::const_iterator iter;
    for (iter=zoneInfluenceItems.begin();
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
        rotation()
{}

void
zZoneInfluenceItemRotation::apply(const tPolynomial &valueEq) {

    tPolynomial tf = rotation.marshal(valueEq);
    
    zone->getShape()->setRotation2( tf );
}

zZoneInfluenceItemScale::zZoneInfluenceItemScale(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        scale(0.0)
{}

void
zZoneInfluenceItemScale::apply(const tPolynomial &value) {
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
zZoneInfluenceItemPosition::apply(const tPolynomial &value) {
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
zZoneInfluenceItemColor::apply(const tPolynomial &value) {
    zone->getShape()->setColorNow( color );
}

