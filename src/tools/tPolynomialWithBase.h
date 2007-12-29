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

#ifndef ArmageTron_tPolynomial_H
#define ArmageTron_tPolynomial_H

#include "tError.h"
#include <math.h>

#define REAL float

#define MAX_LENGTH 4

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
template <typename T>
class tPolynomial
{
public:

    tPolynomial();  //!< constructor
    tPolynomial(REAL value);  //!< constructor
    tPolynomial(REAL coefs_[]);  //!< constructor
    tPolynomial(const tPolynomial<T> &tf);  //!< constructor

    ~tPolynomial() //!< destructor
    {
        delete [] coefs;
    }
      

    REAL evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator
    tPolynomial<T> const operator*( REAL argument ) const;
    tPolynomial<T> const operator*( const tPolynomial<T> & tfRight ) const ;
    tPolynomial<T> const operator+( REAL value ) const ;
    tPolynomial<T> const operator+( const tPolynomial<T> &tfRight ) const ;

    void setBaseArgument(REAL value) {baseArgument = value;}
    void addConstant(REAL value) {coefs[0] += value;}
    void setUnadjustableOffset(REAL value) {unadjustableOffset = value;}


    void reevaluateCoefsAt(REAL argument);
    void changeRate(REAL newRate, int newRateLength, REAL argument);


    void setRates(REAL newValues[], REAL argument);

    //    template<typename T> T & ReadSync( T & m );
    //    template<typename T> T & WriteSync( T & m );
    T & ReadSync( T & m );
    T & WriteSync( T & m ) const;

 protected:
    int length;
    REAL baseArgument; //!< the evaluation is always done on (argument - baseArgument) rather than (argument)
    REAL *coefs;
    REAL unadjustableOffset; //!< offset value that is not modified by an adjustSlope
};

//These templates are probably only useable with nMessage as parameter.
//The reason they're templates is that nMessage isn't available in src/tools
//and that tPolynomial might be needed to be in src/tools in the future.
//Imagine a constructor for vValue that converts from a tPolynomial as an
//example (or the other way, as far as possible). --wrtlprnft

template<typename T> 
T & operator << ( T & m, tPolynomial<T> const & f ); //! function network message writing operator
template<typename T> 
T & operator >> ( T & m, tPolynomial<T> & f );       //! function network message reading operator
template<typename T> 
bool operator == (const tPolynomial<T> & left, const tPolynomial<T> & right);


template <typename T>
tPolynomial<T>::tPolynomial()  //!< constructor
: length(0),
  baseArgument(0.0),
  unadjustableOffset(0.0)
{ 
  coefs = new REAL[MAX_LENGTH];
  for(int i=0; i<=MAX_LENGTH; i++)
    coefs[i] = 0.0f;
}

template <typename T>
tPolynomial<T>::tPolynomial(REAL value)  //!< constructor
: length(0),
  baseArgument(0.0),
  unadjustableOffset(0.0)
{ 
  coefs = new REAL[MAX_LENGTH];
  for(int i=0; i<=MAX_LENGTH; i++)
    coefs[i] = 0.0f;
  coefs[0] = value;
}


template <typename T>
tPolynomial<T>::tPolynomial(REAL coefs_[])  //!< constructor
: length(sizeof(coefs_)/sizeof(REAL)),
  baseArgument(0.0),
  unadjustableOffset(0.0)
{
  coefs = new REAL[MAX_LENGTH];
  for(int i=0; i<=MAX_LENGTH; i++)
    coefs[i] = coefs_[i];

}

template <typename T>
tPolynomial<T>::tPolynomial(const tPolynomial<T> &tf)  //!< constructor
: length(tf.length),
  baseArgument(tf.baseArgument),
  unadjustableOffset(tf.unadjustableOffset)
{
  coefs = new REAL[MAX_LENGTH];
  for(int i=0; i<=MAX_LENGTH; i++)
    coefs[i] = tf.coefs[i];


}

// *******************************************************************************
// *
// *	operator ( )
// *
// *******************************************************************************
//!
//!		@return		the function value
//!
// *******************************************************************************

template <typename T>
REAL tPolynomial<T>::operator ( )( REAL argument ) const
{
    return evaluate( argument );
}

// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	m	message to write to
//!		@param	f	function to write
//!		@return		reference to message for chaining
//!
// *******************************************************************************

template<typename T> T & operator << ( T & m, tPolynomial<T> const & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID = 1;
    m.Write( ID );

    return f.WriteSync(m);
}

// *******************************************************************************
// *
// *	operator >>
// *
// *******************************************************************************
//!
//!     @param  m   message to read from
//!     @param  f   function to read to
//!     @return     reference to message for chaining
//!
// *******************************************************************************

template<typename T> T & operator >> ( T & m, tPolynomial<T> & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID;
    m.Read(ID);
    tASSERT( ID == 1 ) ;

    return f.ReadSync(m);
}

/**
 *
 */
#define DELTA 1e-10

template<typename T> 
bool operator == (const tPolynomial<T> & left, const tPolynomial<T> & right)
{
  // float/double equality doesnt exist. Accept small difference as equal.
    bool res = ( 
	      (left.length == right.length)
	      && ( fabs(left.baseArgument       - right.baseArgument) < DELTA)
	      && ( fabs(left.unadjustableOffset - right.unadjustableOffset) < DELTA)
	       );

    // If we havent found a difference yet, inspect the coefficients
    if(true == res) {
      for(int i=0; i<left.length; i++) {
	if ( fabs(left.coefs - right.coefs) >= DELTA ) {
	  res = false;
	  break;
	}
      }
    }

    return res;
}


/**
 *
 */
template <typename T>
void tPolynomial<T>::reevaluateCoefsAt(REAL argument)
{
  // Compute the coefficients at "argument"
  // c0' = c0 + c1*arg + (c2/2)*arg^2 + ... + (c[N]/N)*arg^N
  // c1' = c1 + c2*arg + ... + (c[N]/(N-1))*arg^(N-1)
  // c2' = c2 + ... + (c[N]/(N-2))*arg^(N-2)
  // ...
  // c[N-1] = c[N-1] + c[N]*arg
  // c[N] = c[N]
  //
  // so:
  // [0   , 0 , a] at argument=0 will become
  // [9a/2, 3a, a] at argument=3

  REAL arg = argument - baseArgument; 

  // Compute for each coefficient their new value
  for(int coefIndex=0; coefIndex<length; coefIndex++) {
    REAL newCoefValue = 0.0;
    for(int j=length; j>coefIndex; j--) {
      newCoefValue = (newCoefValue + coefs[j]/j) * arg;
    }
    coefs[coefIndex] += newCoefValue;
  }

  setBaseArgument(argument);
}

/**
 *
 */
template <typename T>
void tPolynomial<T>::changeRate(REAL newRate, int newRateIndex, REAL argument)
{
  assert(MAX_LENGTH >= newRateIndex); // While order N has N+1 element, we do not receive an element for order 0.

  reevaluateCoefsAt(argument);

  coefs[newRateIndex] = newRate;

  // Grow the polynomial if required
  if(newRateIndex > length) {
    for(int i=length; i<newRateIndex; i++) {
      coefs[i] = 0.0;
    }
    length = newRateIndex;
  }
}

/**
 * This perform a hard setting of all the coefficients
 */
template <typename T>
void tPolynomial<T>::setRates(REAL newValues[], REAL argument)
{
  int newLength = sizeof(newValues)/sizeof(REAL);
  assert(MAX_LENGTH >= newLength);

  setBaseArgument(argument);

  for (int i=0; i<newLength; i++) {
      coefs[i] = newValues[i];
    }
  length = newLength;
}

template <typename T>
REAL tPolynomial<T>::evaluate( REAL argument ) const
{
  REAL arg = (argument - baseArgument);

  REAL res = 0.0;

  // Compute res = unadjustableOffset + c[0] + c[1]*arg + (c[2]/2)*arg^2 + ... + (c[N]/N)*arg^N
  for (int i=length; 0<i; i--) {
      res = (res + coefs[i]/i) * arg;
    }
  res += (coefs[0] + unadjustableOffset); // length 0
  return res;

}

template <typename T>
T & tPolynomial<T>::WriteSync( T & m ) const
{
    // write unadjustableOffset
    m << unadjustableOffset;

    m << baseArgument;
    // write length
    m << length;

    for(int i=0; i<MAX_LENGTH; i++)
      {
	m << coefs[i];
      }

    return m;
}

template <typename T>
T & tPolynomial<T>::ReadSync( T & m )
{
    // read values
    REAL value;
    // read unadjustableOffset
    m >> unadjustableOffset;
    
    m >> baseArgument;
    // write length
    m >> length;

    for(int i=0; i<MAX_LENGTH; i++)
      {
	m >> coefs[i];
      }

    return m;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator*( REAL value ) const {
  tPolynomial<T> tf(*this);
  for(int i=0; i<length; i++) {
    tf.coefs[i] *= value;
  }
  return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator*( const tPolynomial<T> & tfRight ) const {
  tPolynomial<T> tf;
  for(int i=0; i<this->length; i++) {
    for(int j=0; j<tfRight.length; j++) {
      tf.coefs[i+j] += (this->coefs[i]) * tfRight.coefs[j];
    }
  }
  return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( REAL value ) const {
  tPolynomial<T> tf(*this);
  tf.coefs[0] += value;
  return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( const tPolynomial<T> &tfRight ) const {
  tPolynomial<T> tf(*this);
  for(int i=0; i<length; i++) {
    tf.coefs[i] += tfRight.coefs[i];
  }
  return tf;
}


#endif

