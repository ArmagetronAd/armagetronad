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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_tCOORD_H
#define ArmageTron_tCOORD_H

//#include <iostream>
#include "defs.h"

//! Stores any kind of x and y coordinates
class tCoord{
public:
    REAL x,y; //!< the stored coordinates
    explicit tCoord(REAL X=0,REAL Y=0):x(X),y(Y){}; //!< Default constructor

    // Calculations:
    inline bool operator==(const tCoord &a) const; //!< Are the two coordinates close enough to each other to be considered equeal?
    bool operator!=(const tCoord &a) const{return !operator==(a);} //!< Are the two coordinates far apart from each other to be considered different?
    tCoord operator-(const tCoord &a) const{return tCoord(x-a.x,y-a.y);} //!< substracts two coordinates
    tCoord operator-() const{return tCoord(-x,-y);} //!< mirrors a coordinate around the x and y axis
    tCoord operator+(const tCoord &a) const{return tCoord(x+a.x,y+a.y);} //!< adds two coordinates
    const tCoord& operator+=(tCoord const &other)    { x+=other.x; y+=other.y; return *this;} //!< adds two coordinates
    const tCoord& operator-=(tCoord const &other)    { x-=other.x; y-=other.y; return *this;} //!< substracts two coordinates
    tCoord operator*(REAL a) const      {return tCoord(x*a,y*a);} //!< stretches a coordinate by a factor from the center
    const tCoord& operator*=(REAL a)    { x*=a; y*=a; return *this;} //!< stretches a coordinate by a factor from the center
    const tCoord& operator*=(tCoord const &other)    { x*=other.x; y*=other.y; return *this;} //!< stretches a coordinate by a factor from the center
    REAL NormSquared()          const   {return x*x+y*y;} //!< returns the square of the distance to the origin
    REAL Norm()                 const   {return sqrt(NormSquared());} //!< returns the distance to the orign
    void Normalize()                    { *this *= 1/Norm(); } //!< normalizes the coordinate by setting the distance to the orign to 1 and keeping the ratio of the x and y coordinates
    const tCoord &operator=(const tCoord &a){ x=a.x;y=a.y; return *this;  } //!< overwrites a coordinate with another one

    //! scalar product
    static REAL F(const tCoord &a,const tCoord &b){
        return(a.x*b.x+a.y*b.y);
    }

    //! change coordinates, so that a is at zero and c is at one.
    //! gives the coordinate of b.
    static REAL V(const tCoord &a,const tCoord &b,const tCoord &c){
        tCoord ab=b-a;
        tCoord ac=c-a;
        return(F(ab,ac)/F(ac,ac));
    }


    //! returns a positive number if (a,*this) form a right-handed
    //! coordinate system, a negative number, if ...
    //! and 0, if a and *this are parallel.
    REAL   operator*(const tCoord &a) const{return -x*a.y+y*a.x;}


    //! complex multiplication: turns this by angle given in a
    tCoord Turn(const tCoord &a) const{return tCoord(x*a.x-y*a.y,y*a.x+x*a.y);}
    //! complex multiplication: turns this by angle given in x and y
    tCoord Turn(REAL x,REAL y) const{return Turn(tCoord(x,y));}
    //! complex conjugation
    tCoord Conj() const{return tCoord(x,-y);}

    // I/O:
    void Print(std::ostream &s) const; //!< Prints the coordinate into a stream (format: "(x,y)")
    void Read(std::istream &s); //!< Reads the coordinate from a stream (format: "(x,y)")
};

//! handy function used to decide whether a*b is small enough to
//! consider a and b lineary dependant (criterium: |a*b|<EPS*estim....)
REAL se_EstimatedRangeOfMult(const tCoord &a,const tCoord &b);

//! returns a measure for the equality of two coords
REAL st_GetDifference( const tCoord & a, const tCoord & b );

//! @param a the other coordinate
//! @returns are the coordinates close to each other to count as equeal?
inline bool tCoord::operator==(const tCoord &a) const{
    return ((*this-a).NormSquared()<=(EPS*EPS)*se_EstimatedRangeOfMult(*this,a));
}

//! @param s the stream for the coordinate to be inserted into
//! @param c the coordinate to be inserted
//! @returns the stream given in s
inline std::ostream &operator<< (std::ostream &s,const tCoord &c){
    c.Print(s);
    return s;
}

//! @param s the stream for the coordinate to be read from
//! @param c the coordinate to be read
//! @returns the stream given in s
inline std::istream &operator>> (std::istream &s,tCoord &c){
    c.Read(s);
    return s;
}

//! ?
extern const tCoord se_zeroCoord;
#endif
