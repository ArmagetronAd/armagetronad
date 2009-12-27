%{
#include "eSensor.h"
%}

class eSensor: public eGameObject{
public:
    REAL            hit;            // where is the eWall?
    eHalfEdge *ehit;     // the eWall we sense
    int             lr;         // and direction it goes to (left/right)
    eCoord           before_hit; // a point shortly before that eWall
    
    eSensor(eGameObject *o,const eCoord &start,const eCoord &d);
    
    virtual void PassEdge(const eWall *w,REAL time,REAL,int =1);
    //  virtual void PassEdge(eEdge *e,REAL time,REAL a,int recursion=1);
    void detect(REAL range);
    
    void SetCurrentFace(eFace *f) { currentFace = f ; }
    
    inline eSensor & SetInverseSpeed( REAL inverseSpeed );	//!< Sets the inverse speed of the sensor
    inline REAL GetInverseSpeed( void ) const;	//!< Gets the inverse speed of the sensor
    inline eSensor const & GetInverseSpeed( REAL & inverseSpeed ) const;	//!< Gets the inverse speed of the sensor
};
