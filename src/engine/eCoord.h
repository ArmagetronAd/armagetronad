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

#ifndef ArmageTron_COORD_H
#define ArmageTron_COORD_H

//#include <iostream>
#include "defs.h"

class eCoord{
public:
    REAL x,y;
    explicit eCoord(REAL X=0,REAL Y=0):x(X),y(Y){}

    // Calculations:
    inline bool operator==(const eCoord &a) const;
    bool operator!=(const eCoord &a) const{return !operator==(a);}
    eCoord operator-(const eCoord &a) const{return eCoord(x-a.x,y-a.y);}
    eCoord operator-() const{return eCoord(-x,-y);}
    eCoord operator+(const eCoord &a) const{return eCoord(x+a.x,y+a.y);}
    eCoord operator*(REAL a) const 		{return eCoord(x*a,y*a);}
    eCoord operator/(REAL a) const 		{return eCoord(x/a,y/a);}
    const eCoord& operator*=(REAL a)  	{ x*=a; y*=a; return *this;}
    REAL NormSquared() 			const 	{return x*x+y*y;}
    REAL Norm() 				const 	{return sqrt(NormSquared());}
    void Normalize()					{ *this *= 1/Norm(); }
    const eCoord &operator=(const eCoord &a){ x=a.x;y=a.y; return *this;  }

    //scalar product:
    static REAL F(const eCoord &a,const eCoord &b){
        return(a.x*b.x+a.y*b.y);
    }

    // change coordinates, so that a is at zero and c is at one.
    // gives the coordinate of b.
    static REAL V(const eCoord &a,const eCoord &b,const eCoord &c){
        eCoord ab=b-a;
        eCoord ac=c-a;
        return(F(ab,ac)/F(ac,ac));
    }


    // returns a positive number if (a,*this) form a right-handed
    // coordinate system, a negative number, if ...
    // and 0, if a and *this are parallel.
    REAL   operator*(const eCoord &a) const{return -x*a.y+y*a.x;}


    // complex multiplication: turns this by angle given in a
    eCoord Turn(const eCoord &a) const{return eCoord(x*a.x-y*a.y,y*a.x+x*a.y);}
    eCoord Turn(REAL a,REAL b) const{return Turn(eCoord(a,b));}
    // complex conjugation
    eCoord Conj() const{return eCoord(x,-y);}

    // I/O:
    void Print(std::ostream &s) const;
    void Read(std::istream &s);
};

// handy function used to decide whether a*b is small enough to
// consider a and b lineary dependant (criterium: |a*b|<EPS*estim....)
REAL se_EstimatedRangeOfMult(const eCoord &a,const eCoord &b);

// returns a measure for the equality of two coords
REAL st_GetDifference( const eCoord & a, const eCoord & b );

inline bool eCoord::operator==(const eCoord &a) const{
    return ((*this-a).NormSquared()<=(EPS*EPS)*se_EstimatedRangeOfMult(*this,a));
}

inline std::ostream &operator<< (std::ostream &s,const eCoord &c){
    c.Print(s);
    return s;
}

inline std::istream &operator>> (std::istream &s,eCoord &c){
    c.Read(s);
    return s;
}

extern const eCoord se_zeroCoord;
#endif
