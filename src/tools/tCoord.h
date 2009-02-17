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
#include "tError.h"
#include <iterator>
#include <algorithm>
#include <map>
#include <vector>

namespace Tools { class Direction; class Position; }

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

    //! returns the cross product of the three given points, whatever that is--code taken from http://en.wikipedia.org/wiki/Graham_scan
    static REAL CrossProduct(tCoord const &a, tCoord const &b, tCoord const &c) {
        return (b.x - a.x)*(c.y - a.y) - (c.x - a.x)*(b.y - a.y);
    }

    //! returns delta y over delta x
    static REAL Tangent(tCoord const &a, tCoord const &b) {
        return (b.y - a.y)/(b.x-a.x);
    }

    // network syncing functions (separate for direction and position coords,
    // different optimizations may apply to them)
    void WriteSync( Tools::Position       & position  ) const;
    void WriteSync( Tools::Direction      & direction ) const;
    void ReadSync( Tools::Position  const & position  )      ;
    void ReadSync( Tools::Direction const & direction )      ;
private:
    class GrahamComparator {
        tCoord const &m_reference;
    public:
        GrahamComparator(tCoord const &reference) : m_reference(reference) {}
        bool operator()(tCoord const &a, tCoord const &b) {
            REAL ta = Tangent(m_reference, a), tb = Tangent(m_reference, b);

            //check for 90 degree angles...
            if(ta == NAN && tb == NAN) return fabs((m_reference-a).NormSquared()) < fabs((m_reference-b).NormSquared());
            if(ta == NAN) return tb<0;
            if(tb == NAN) return ta>0;

            //check for opposite sides
            if(ta>0 && tb<0) return true;
            if(tb>0 && ta<0) return false;

            return (ta<tb-EPS) || ((fabs(ta-tb)<EPS) && fabs((m_reference-a).NormSquared()) < fabs((m_reference-b).NormSquared()));
        }
    };
public:

    //! finds the smallest convex polygon completely surrounding all points in in and stores them in out. See also http://en.wikipedia.org/wiki/Graham_scan
    template<typename T> static void GrahamScan(T &in, T &out);

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

//! stretches a coordinate by a factor from the center
inline tCoord operator*(REAL a, tCoord const &c) {
    return c*a;
}

//! @param in the standard container containing the tCoords to wrap into a rubber band. In will be modified!
//! @param out the standard container to put the rubber band into, its contents will be erased
template<typename T> void tCoord::GrahamScan(T &in, T &out) {
    tASSERT(!in.empty());
    out.clear();

    // find the point that's closest to the bottom, preferring points to the left
    tCoord P=in.front();
    typename T::iterator iter(in.begin()+1);
    for(; iter != in.end(); ++iter) {
        if(iter->y < P.y || ( iter->y == P.y && iter->x < P.x ) ) {
            P = *iter;
        }
    }

    std::sort(in.begin(), in.end(), GrahamComparator(P));

    //std::cerr << "P: " << P << std::endl;

    out.push_back(P);
    for(typename T::iterator iter = in.begin()+1; iter != in.end(); ++iter) {
        if(*iter != P) {
            out.push_back(*iter);
            break;
        }
    }
    if(out.size()<2) return; //all points equeal P, kinda boring...

    //std::copy(in.begin(), in.end(), std::ostream_iterator<tCoord>(std::cerr, " "));
    //std::cerr << std::endl;
    for(typename T::iterator iter = in.begin(); iter != in.end(); ++iter) {
        //std::cerr << "New point: " << *iter << "; angle=" << (atanf(Tangent(P, *iter))*180/M_PI) << std::endl;
    }

    for(typename T::iterator iter = in.begin()+2; iter != in.end(); ++iter) {
        if(*iter == P) continue;
        if(*iter == *(iter - 1)) continue; //duplicate or near- duplicate point; We don't want that mess.
        //std::cerr << "New point: " << *iter << "; angle=" << Tangent(P, *iter) << std::endl;
        //if(iter->y > 200) std::cerr << "==================\n";
        float product = CrossProduct(out[out.size()-2], out.back(), *iter);
        if (product < 0 || fabs(product) < EPS) {
            //the last three points are concave, remove points until they're not
            while((product < 0 || fabs(product) < EPS) && out.size() > 2) {
                //std::cerr << "concave: " << *(iter-2) << " " << *(iter-1) << " " << *iter << std::endl;
                //std::copy(out.begin(), out.end(), std::ostream_iterator<tCoord>(std::cerr, " "));
                //std::cerr << std::endl;
                out.pop_back();
                product = CrossProduct(out[out.size()-2], out.back(), *iter);
            }
        } else {
            //std::cerr << "Passed point: " << *iter << std::endl;
        }
        out.push_back(*iter);
        //std::copy(out.begin(), out.end(), std::ostream_iterator<tCoord>(std::cerr, " "));
        //std::cerr << std::endl;
    }
    float product = CrossProduct(out[out.size()-2], out.back(), P);
    while((product < 0 || fabs(product) < EPS) && out.size() > 2) {
        //std::cerr << "concave: " << *(iter-2) << " " << *(iter-1) << " " << *iter << std::endl;
        //std::copy(out.begin(), out.end(), std::ostream_iterator<tCoord>(std::cerr, " "));
        //std::cerr << std::endl;
        out.pop_back();
        product = CrossProduct(out[out.size()-2], out.back(), P);
    }
    //std::cerr << "=====\n";
    //for(typename T::iterator iter = out.begin(); iter != out.end(); ++iter) {
    //	std::cerr << *iter << std::endl;
    //}

}

//! ?
extern const tCoord se_zeroCoord;
#endif
