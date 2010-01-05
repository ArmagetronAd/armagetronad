%{
#include "gArena.h"
#include "eGrid.h"
extern gArena * sg_GetArena();
%}

%extend gArena {
    static gArena *get_arena()  { return sg_GetArena(); }
    // get the number of directions a cycle can drive in on this grid
    int winding_number() const {return eGrid::CurrentGrid()->WindingNumber();}
    // get the number corresponding to a particular direction
    int direction_winding(const eCoord& dir) {return eGrid::CurrentGrid()->DirectionWinding(dir);}
    // get the direction associated with a winding number
    eCoord get_direction(int winding) {return eGrid::CurrentGrid()->GetDirection(winding);}
};

%rename(Arena) gArena;
// the fighting arena
class gArena{
    friend class gSpawnPoint;
    friend class gParser;

public:
%rename(new_spawn_point) NewSpawnPoint;
    void NewSpawnPoint(const eCoord &loc,const eCoord &dir);

    gArena();
    virtual ~gArena();

    // draw the gArena
    virtual void PrepareGrid(eGrid *grid, gParser *aParser);

    // get a random position
%rename(get_random_pos) GetRandomPos;
    virtual eCoord GetRandomPos( REAL factor ) const;

    // find the closes gSpawnPoint 
%rename(closest_spawn_point) ClosestSpawnPoint;
    gSpawnPoint * ClosestSpawnPoint(eCoord loc);

    // find the best gSpawnPoint
%rename(least_dangerous_spawn_point) LeastDangerousSpawnPoint;
    gSpawnPoint * LeastDangerousSpawnPoint();

%rename(size_multiplier) SizeMultiplier;
    // access the size multiplier
    static float	SizeMultiplier();
    static void     SetSizeMultiplier(float mult);
    static float    GetSizeMultiplier();

%rename(remove_all_spawn) RemoveAllSpawn;
    void RemoveAllSpawn();

private:
    tList<gSpawnPoint> spawnPoints; //!< the list of active spawn points
};
