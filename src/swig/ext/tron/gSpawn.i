%{
#include "gSpawn.h"
%}


%extend gSpawnPoint {
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

%rename(SpawnPoint) gSpawnPoint;
class gSpawnPoint{
    friend class gArena;

    int   id;
    eCoord location,direction;
    REAL  lastTimeUsed;
    int   numberOfUses;

public:
    gSpawnPoint(const eCoord &loc,const eCoord &dir);
    ~gSpawnPoint(){};

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

