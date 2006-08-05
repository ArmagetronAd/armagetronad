#include "zZoneInfluence.h"

zZoneInfluence::zZoneInfluence(zZone * _zone) : zone(_zone), zoneInfluenceItems() { }

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

zZoneInfluenceItem::zZoneInfluenceItem(zZone * aZone):zone(aZone) {}

zZoneInfluenceItem::~zZoneInfluenceItem() {}

void
zZoneInfluenceItemRotation::apply(REAL value) {
    zone->SetRotationSpeed(rotationSpeed*value);
    zone->SetRotationAcceleration(rotationAcceleration);
}

void
zZoneInfluenceItemRadius::apply(REAL value) {
    zone->SetRadius(radius);
}

void
zZoneInfluenceItemPosition::apply(REAL value) {
    zone->SetPosition(pos);
}

void
zZoneInfluenceItemColor::apply(REAL value) {
    zone->SetColor(color);
}

