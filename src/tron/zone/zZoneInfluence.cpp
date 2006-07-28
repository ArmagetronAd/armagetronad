#include "zZoneInfluence.h"

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

void
zZoneInfluenceItemRotation::apply(REAL value) {
  zone->SetRotationSpeed(rotationSpeed);
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

