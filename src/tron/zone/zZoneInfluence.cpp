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
  // HACK:
  // Need a solution that influence the shape, not the zone
  //    zone->SetRotationSpeed(rotationSpeed*value);
  //    zone->SetRotationAcceleration(rotationAcceleration);
  std::cout << "This has not been implemented yet! Tell Philippeqc he forgot to do it!" << std::endl;
}

zZoneInfluenceItemScale::zZoneInfluenceItemScale(zZonePtr aZone):
  zZoneInfluenceItem(aZone),
  scale(0.0) 
{}

void
zZoneInfluenceItemScale::apply(REAL value) {
  // HACK:
  // Need a solution that influence the shape, not the zone
  //    zone->SetScale(scale);
  std::cout << "This has not been implemented yet! Tell Philippeqc he forgot to do it!" << std::endl;
}

zZoneInfluenceItemPosition::zZoneInfluenceItemPosition(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        pos(0.0, 0.0)
{}

void
zZoneInfluenceItemPosition::apply(REAL value) {
  // HACK:
  // Need a solution that influence the shape, not the zone
  //    zone->SetPosition(pos);
  std::cout << "This has not been implemented yet! Tell Philippeqc he forgot to do it!" << std::endl;
}

zZoneInfluenceItemColor::zZoneInfluenceItemColor(zZonePtr aZone):
        zZoneInfluenceItem(aZone),
        color(0.0, 0.0, 0.0, 0.0)
{}

void
zZoneInfluenceItemColor::apply(REAL value) {
  // HACK:
  // Need a solution that influence the shape, not the zone
  //    zone->SetColor(color);
  std::cout << "This has not been implemented yet! Tell Philippeqc he forgot to do it!" << std::endl;
}

