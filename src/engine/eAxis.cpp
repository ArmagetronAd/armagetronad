#include "eAxis.h"

eAxis::eAxis(int number)
{
    windings = NULL;
    windingAngles = NULL;
    Init(number);
}

void eAxis::Init(int number)
{
    numberWinding = (number < 1) ? 4 : number;
    if (windings)
        delete [] windings;
    if (windingAngles)
        delete [] windingAngles; // [] not really required as we are dealing with floats, not objects.

    windings = new eCoord[numberWinding]();
    windingAngles = new REAL[numberWinding];

    ComputeWinding();
    SnapWinding();
    ComputeAngles();
}

void eAxis::Init(int number, eCoord predefinedAxis[], bool normalize)
{
    if ( number < 1 || predefinedAxis == NULL)
    {
        Init(4);
        return;
    }

    numberWinding = number;
    if (windings)
        delete [] windings;
    if (windingAngles)
        delete [] windingAngles; // [] not really required as we are dealing with floats, not objects.

    windings = new eCoord[numberWinding]();
    windingAngles = new REAL[numberWinding];

    for(int i=0; i<number; i++)
    {
        windings[i] = predefinedAxis[i];
        if (normalize) {
            windings[i].Normalize();
        }
    }

    SnapWinding();
    ComputeAngles();
}


eAxis::~eAxis()
{
    if (windings)
        delete [] windings;
    if (windingAngles)
        delete [] windingAngles; // [] not really required as we are dealing with floats, not objects.
}

void eAxis::Turn(int &currentWinding, int direction)
{
    if (direction > 1) direction = 1;
    if (direction < -1) direction = -1;
    currentWinding += direction + numberWinding; // + numberWinding to avoid any negative. They dont modulate well!
    currentWinding %= numberWinding;
}

void eAxis::TurnRight(int &windingDirection)
{
    windingDirection--;
    windingDirection += numberWinding; // + numberWinding to fix negative modulos
    windingDirection %= numberWinding;
}

void eAxis::TurnLeft(int &windingDirection)
{
    windingDirection++;
    windingDirection %= numberWinding;
}

/*
 * Compute evenly spaced windings 
 */
void eAxis::ComputeWinding()
{
    unsigned int i;
    REAL tetha;
    for(i=0; i<numberWinding; i++)
    {
        tetha = M_PI*2*(numberWinding - i - 1)/numberWinding;
        windings[i].x = (cosf(tetha));
        windings[i].y = (sinf(tetha));
    }
}

void eAxis::SnapWinding()
{
    unsigned int i;
    for(i=0; i<numberWinding; i++)
    {
        eCoord & winding = windings[i];
        if ( fabs(winding.x) < EPS )
            winding.x = 0;
        if ( fabs(winding.y) < EPS )
            winding.y = 0;
    }
}

void eAxis::ComputeAngles()
{
    unsigned int i;
    for(i=0; i<numberWinding; i++)
    {
        windingAngles[i] = eCoordToRad(windings[i]);
    }
}

REAL remf(REAL x, REAL y) {
    return x - y*floorf(x/y);
}

REAL angledifff(REAL x, REAL y) {
    return fabsf(remf(x - y + M_PI, 2*M_PI) - M_PI);
}

/*
 * Given the direction described by (0,0) to pos, return the winding number
 * describing the closest axis/direction 
 * 
 */
int eAxis::NearestWinding (eCoord pos)
{
    REAL posAngle = eCoordToRad(pos);

    REAL closestval = angledifff(windingAngles[0], posAngle);
    unsigned int i, closestid = 0;
    for(i=1; i<numberWinding; i++) {
        REAL diff = angledifff(windingAngles[i], posAngle);
        if(diff < closestval) {
            closestid = i;
            closestval = diff;
        }
    }
    return closestid;
}

eCoord eAxis::GetDirection (int winding)
{
    winding %= numberWinding;
    return windings[winding];
}
