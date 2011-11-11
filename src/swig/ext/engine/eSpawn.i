%{
#include "eSpawn.h"
%}


%extend eSpawnPoint {
    tCoord position() {
        tCoord loc, dir;
        $self->Spawn(loc, dir);
        return loc;
    }
    tCoord direction() {
        tCoord loc, dir;
        $self->Spawn(loc, dir);
        return dir;
    }
};

%rename(SpawnPoint) eSpawnPoint;
class eSpawnPoint{
    friend class gArena;

    int   id;
    eCoord location,direction;
    REAL  lastTimeUsed;
    int   numberOfUses;

public:
    eSpawnPoint(const eCoord &loc,const eCoord &dir);
    ~eSpawnPoint(){};

%rename(spawn) Spawn;
    //enters valid spawn eCoordinates and direction in loc and dir
    void Spawn(eCoord &loc,eCoord &dir);

%rename(danger) Danger;
    // estimates the danger of spawning here (0: no problem, 10: certain death)
    REAL Danger();

%rename(clear) Clear;
    // mark it unused (for match start)
    void Clear();
};

