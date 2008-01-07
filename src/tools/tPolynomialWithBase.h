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

#ifndef ArmageTron_tPolynomialWithBase_H
#define ArmageTron_tPolynomialWithBase_H

#include "tPolynomial.h"

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
template <typename T>
class tPolynomialWithBase : public class tPolynomial<T>
{
public:

    tPolynomialWithBase();  //!< constructor
    tPolynomialWithBase(REAL value);  //!< constructor
    tPolynomialWithBase(REAL coefs_[]);  //!< constructor
    tPolynomialWithBase(const tPolynomialWithBase<T> &tf);  //!< constructor
    tPolynomialWithBase(const tPolynomial<T> &tf);  //!< constructor

    virtual ~tPolynomialWithBase() { //!< destructor
      // Empty
    }

    virtual REAL evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator

    void setUnadjustableOffset(REAL x);
    REAL getUnadjustableOffset();

    virtual T & ReadSync( T & m );
    virtual T & WriteSync( T & m ) const;

    template<typename D> 
    friend bool operator == (const tPolynomialWithBase<D> & left, const tPolynomialWithBase<D> & right);

protected:
    REAL unadjustableOffset; //!< offset value that is not modified by an adjustSlope
};

//These templates are probably only useable with nMessage as parameter.
//The reason they're templates is that nMessage isn't available in src/tools
//and that tPolynomialWithBase might be needed to be in src/tools in the future.
//Imagine a constructor for vValue that converts from a tPolynomialWithBase as an
//example (or the other way, as far as possible). --wrtlprnft

T & operator << ( T & m, tPolynomialWithBase<T> const & f ); //! function network message writing operator
template<typename T>
T & operator >> ( T & m, tPolynomialWithBase<T> & f );       //! function network message reading operator
template<typename T>
bool operator == (const tPolynomialWithBase<T> & left, const tPolynomial<T> & right);
template<typename T>
bool operator == (const tPolynomialWithBase<T> & left, const tPolynomialWithBase<T> & right);



template <typename T>
tPolynomialWithBase<T>::tPolynomialWithBase()  //!< constructor
: tPolynomial<T>(),
  unadjustableOffset(0.0)
{
  // Empty
}

template <typename T>
tPolynomialWithBase<T>::tPolynomialWithBase(REAL value)  //!< constructor
: tPolynomial<T>(value),
  unadjustableOffset(0.0)
{ 
  // Empty
}


template <typename T>
tPolynomialWithBase<T>::tPolynomialWithBase(REAL coefs_[])  //!< constructor
: tPolynomial<T>(coefs_),
  unadjustableOffset(0.0)
{
  // Empty
}

template <typename T>
tPolynomialWithBase<T>::tPolynomialWithBase(const tPolynomialWithBase<T> &tf)  //!< constructor
: tPolynomial<T>(tf),
  unadjustableOffset(tf.unadjustableOffset)
{
  // Empty
}

template <typename T>
tPolynomialWithBase<T>::tPolynomialWithBase(const tPolynomial<T> &tf)  //!< constructor
: tPolynomial<T>(tf),
  unadjustableOffset(0.0)
{
  // Empty
}

/**
 *
 */
#define DELTA 1e-10

template<typename T>
bool operator == (const tPolynomialWithBase<T> & left, const tPolynomialWithBase<T> & right)
{
    bool res = static_cast<tPolynomial<T> >(left) == static_cast<tPolynomial<T> >(right);

    if(true == res) {
      // float/double equality doesnt exist. Accept small difference as equal.
      res = ( fabs(left.unadjustableOffset - right.unadjustableOffset) < DELTA);
    }
    
    return res;
}

template <typename T>
REAL tPolynomialWithBase<T>::evaluate( REAL argument ) const
{
  REAL res = tPolynomial<T>::evaluate( argument );

  res += unadjustableOffset;
  return res;
}

template <typename T>
T & tPolynomialWithBase<T>::WriteSync( T & m ) const
{
    tPolynomial<T>::WriteSync( m );

    // write unadjustableOffset
    m << unadjustableOffset;

    return m;
}

template <typename T>
T & tPolynomialWithBase<T>::ReadSync( T & m )
{
    tPolynomial<T>::ReadSync( m );

    // read unadjustableOffset
    m >> unadjustableOffset;

    return m;
}

#endif

