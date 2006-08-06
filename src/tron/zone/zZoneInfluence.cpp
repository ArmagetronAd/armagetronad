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
    zone->SetRotationSpeed(rotationSpeed*value);
    zone->SetRotationAcceleration(rotationAcceleration);
}

zZoneInfluenceItemRadius::zZoneInfluenceItemRadius(zZonePtr aZone):
  zZoneInfluenceItem(aZone),
  radius(0.0) 
{}

void
zZoneInfluenceItemRadius::apply(REAL value) {
    zone->SetRadius(radius);
}

zZoneInfluenceItemPosition::zZoneInfluenceItemPosition(zZonePtr aZone):
  zZoneInfluenceItem(aZone),
  pos(0.0, 0.0) 
{}

void
zZoneInfluenceItemPosition::apply(REAL value) {
    zone->SetPosition(pos);
}

zZoneInfluenceItemColor::zZoneInfluenceItemColor(zZonePtr aZone):
  zZoneInfluenceItem(aZone),
  color(0.0, 0.0, 0.0, 0.0)
{}

void
zZoneInfluenceItemColor::apply(REAL value) {
    zone->SetColor(color);
}

