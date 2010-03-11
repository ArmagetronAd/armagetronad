%{
#include "zone/zZone.h"
#include "zone/zShape.h"
%}

%extend zZone {
    static zZone* find(string name) {
        zZone::zoneMap::const_iterator iterZone;
        if ((iterZone = zZone::MapZones().find(name)) != zZone::MapZones().end()) {
            return iterZone->second;
        } else {
            return NULL;
        }
    }

    zShape* get_shape() {
        return $self->getShape();
    }
};

%rename(Zone) zZone;
class zZone: public eNetGameObject
{
%rename(get_shape) getShape;
    zShapePtr getShape();
%rename(get_name) getName;
    string getName();
};



