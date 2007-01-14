%{
#include "gSensor.h"
%}

typedef enum{gSENSOR_NONE,gSENSOR_RIM, gSENSOR_ENEMY,
             gSENSOR_TEAMMATE ,gSENSOR_SELF} gSensorWallType;

class gSensor: public eSensor{
public:
    gSensorWallType type;
    
    gSensor(eGameObject *o,const eCoord &start,const eCoord &d)
        :eSensor(o,start,d), type(gSENSOR_NONE){}
    
    virtual void PassEdge(const eWall *w,REAL time,REAL,int =1);
};

