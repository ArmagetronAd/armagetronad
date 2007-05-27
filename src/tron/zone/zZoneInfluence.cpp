#include "zZoneInfluence.h"

zZoneInfluence::zZoneInfluence(zZonePtr _zone) : zone(_zone), zoneInfluenceItems() { }

zZoneInfluence::~zZoneInfluence() { }

void
zZoneInfluence::apply(REAL value)
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
        rotationSpeed(0.0),
        rotationAcceleration(0.0)
{}

void
zZoneInfluenceItemRotation::apply(REAL value) {
    tFunction tfRotation;
    tfRotation.SetOffset( rotationSpeed*value );
    tfRotation.SetSlope( rotationAcceleration );
    zone->getShape()->setRotation( tfRotation );
}

zZoneInfluenceItemScale::zZoneInfluenceItemScale(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        scale(0.0)
{}

void
zZoneInfluenceItemScale::apply(REAL value) {
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
zZoneInfluenceItemPosition::apply(REAL value) {
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
zZoneInfluenceItemColor::apply(REAL value) {
    zone->getShape()->setColor( color );
}

