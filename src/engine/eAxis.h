#include "defs.h"

#include <math.h>
#include <stdio.h>

#include "eCoord.h"

/*
 * Find the angle described by the vector from (0,0) to POS 
 */
// YES its y then x! Why did you think I made a define? :)
#define eCoordToRad(POS) atan2f(POS.y, POS.x)

class eCoord;

class eAxis {
    friend class eGrid;

public:
    eAxis(int number=4);
    ~eAxis();

    int WindingNumber() const {return numberWinding;}
    int NearestWinding (eCoord pos);
    eCoord GetDirection (int winding);
	float GetWindingAngle (int winding) {return windingAngles?windingAngles[winding]:0;}
	
    void Turn(int &currentWinding, int direction);
    void TurnRight(int &direction);
    void TurnLeft(int &direction);

protected:
    void Init(int number);
    void Init(int number, eCoord predefinedAxis[], bool normalize=true);

    unsigned int numberWinding; // How many winding are possible
    eCoord *windings; // The different axis possible.
    float *windingAngles; // The angles (in radiant) associated with the winding axis

    void ComputeWinding();
    void ComputeAngles();
    void SnapWinding(); //!< push axes that are really close to coordinate directions to them
};

