/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_SENSOR_H
#define ArmageTron_SENSOR_H

#include "eGameObject.h"
#include "eTess2.h"
//#include "eGrid.h"

// exception that is thrown when the sensor hit something
class eSensorFinished{};

// sensor sent out to detect near eWalls
class eSensor: public eGameObject{
public:
    REAL            hit;            // where is the eWall?
    tCHECKED_PTR_CONST(eHalfEdge) ehit;     // the eWall we sense
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
protected:
    tCHECKED_PTR(eGameObject) owned;
private:
    REAL inverseSpeed_; //! the inverse speed of the sensor; walls far away will be checked for opacity a bit in the future if this is set.
};

// *******************************************************************************
// *
// *	GetInverseSpeed
// *
// *******************************************************************************
//!
//!		@return		the inverse speed of the sensor
//!
// *******************************************************************************

REAL eSensor::GetInverseSpeed( void ) const
{
    return this->inverseSpeed_;
}

// *******************************************************************************
// *
// *	GetInverseSpeed
// *
// *******************************************************************************
//!
//!		@param	inverseSpeed	the inverse speed of the sensor to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

eSensor const & eSensor::GetInverseSpeed( REAL & inverseSpeed ) const
{
    inverseSpeed = this->inverseSpeed_;
    return *this;
}

// *******************************************************************************
// *
// *	SetInverseSpeed
// *
// *******************************************************************************
//!
//!		@param	inverseSpeed	the inverse speed of the sensor to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

eSensor & eSensor::SetInverseSpeed( REAL inverseSpeed )
{
    this->inverseSpeed_ = inverseSpeed;
    return *this;
}

#endif
