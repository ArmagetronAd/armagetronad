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
#include "tArray.h"

#define REAL float

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
template <typename T>
class tPolynomial
{
public:

    tPolynomial();  //!< constructor
    tPolynomial(int count);  //!< constructor
    tPolynomial(REAL newCoefs[], int count);  //!< constructor
    tPolynomial(tArray<REAL> newCoefs);  //!< constructor
    tPolynomial(const tPolynomial<T> &tf);  //!< constructor

    virtual ~tPolynomial() //!< destructor
    {
    }

    virtual REAL evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator
    tPolynomial<T> const operator*( REAL argument ) const;
    tPolynomial<T> const operator*( const tPolynomial<T> & tfRight ) const ;
    tPolynomial<T> const operator+( REAL value ) const ;
    tPolynomial<T> const operator+( const tPolynomial<T> &tfRight ) const ;
    REAL &operator[](int index); // Allow both reading and writing of element
    REAL const &operator[](int index) const; // Allow reading of element even when the object is const
    void operator=(tPolynomial<T> const &other);

    void addConstant(REAL value) {if(coefs.Len()==0) {coefs.SetLen(1); coefs[0] = 0;} coefs[0] += value;}

    void reevaluateCoefsAt(REAL argument);
    void changeRate(REAL newRate, int newRateLength, REAL argument);

    void setRates(REAL newValues[], int newValuesLength, REAL argument);
    void setRates(tArray<REAL> newValues, REAL argument);

    virtual T & ReadSync( T & m );
    virtual T & WriteSync( T & m ) const;

    template<typename D> 
    friend bool operator == (const tPolynomial<D> & left, const tPolynomial<D> & right);

    virtual void toString();
 protected:
    void setBaseArgument(REAL value) {baseArgument = value;}
    void growCoefsArray(int newLength);

    // Variables
    REAL baseArgument; //!< the evaluation is always done on (argument - baseArgument) rather than (argument)
    tArray<REAL> coefs;
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
tPolynomial<T>::tPolynomial(int count)  //!< constructor
: baseArgument(0.0),
  coefs(count)
{
    // Initialise to all 0's
    for(int i=0; i<coefs.Len(); i++)
        coefs[i] = 0.0f;
}

template <typename T>
tPolynomial<T>::tPolynomial()  //!< constructor
: baseArgument(0.0),
  coefs(0)
{
  // Empty
}

template <typename T>
tPolynomial<T>::tPolynomial(REAL newCoefs[], int count)  //!< constructor
: baseArgument(0.0),
  coefs(count)
{
    for(int i=0; i<coefs.Len(); i++)
        coefs[i] = newCoefs[i];
}

template <typename T>
tPolynomial<T>::tPolynomial(tArray<REAL> newCoefs)  //!< constructor
: baseArgument(0.0),
  coefs(newCoefs)
{
  // Empty
}

template <typename T>
tPolynomial<T>::tPolynomial(const tPolynomial<T> &tf)  //!< constructor
: baseArgument(tf.baseArgument),
  coefs(tf.coefs)
{
  // Empty
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
    bool res = ( fabs(left.baseArgument - right.baseArgument) < DELTA);

    // If the length of the coefs array differ, then the extra elements should be 0
    int maxLength = left.coefs.Len()>right.coefs.Len()?left.coefs.Len():right.coefs.Len();
    int minLength = left.coefs.Len()<right.coefs.Len()?left.coefs.Len():right.coefs.Len();

    // Inspect the common coefficients (ie defined for both polynomial)

    for(int i=0; i<minLength; i++) {
        if ( fabs(left[i] - right[i]) >= DELTA ) {
            res = false;
            break;
        }
    }

    for(int i=minLength; i<maxLength; i++) {
        // The polynomial that is defined up to that length should have its elements set to 0.0
        if(left.coefs.Len()>right.coefs.Len()) {
            if(fabs(left[i]) >= DELTA) {
                res = false;
                break;
            }
        }
        else {
            if(fabs(right[i]) >= DELTA) {
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
    for(int coefIndex=0; coefIndex<coefs.Len(); coefIndex++) {
        REAL newCoefValue = 0.0;
        for(int j=coefs.Len()-1; j>coefIndex; j--) {
            newCoefValue = (newCoefValue + coefs[j]/(j - coefIndex) ) * arg;
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
    if(coefs.Len() <= newRateIndex) {
        coefs.SetLen(newRateIndex + 1);
    }

    reevaluateCoefsAt(argument);

    coefs[newRateIndex] = newRate;
}

/**
 * This perform a hard setting of all the coefficients
 */
template <typename T>
void tPolynomial<T>::setRates(REAL newValues[], int newValuesLength, REAL argument)
{
    if(coefs.Len() < newValuesLength) {
        coefs.SetLen(newValuesLength + 1);
    }

    setBaseArgument(argument);

    for (int i=0; i<newValuesLength; i++) {
        coefs[i] = newValues[i];
    }
}

template <typename T>
void tPolynomial<T>::setRates(tArray<REAL> newValues, REAL argument)
{
    coefs = newValues;
    setBaseArgument(argument);
}

template <typename T>
REAL tPolynomial<T>::evaluate( REAL argument ) const
{
    REAL arg = (argument - baseArgument);

    REAL res = 0.0;

    // Compute res = c[0] + c[1]*arg + (c[2]/2)*arg^2 + ... + (c[N]/N)*arg^N
    for (int i=coefs.Len()-1; i>0; i--) {
        res = (res + coefs[i]/i) * arg;
    }
    if(coefs.Len()!=0)
      res += (coefs[0]);

    return res;

    return res;

}

template <typename T>
T & tPolynomial<T>::WriteSync( T & m ) const
{
    m << baseArgument;
    // write length
    m << coefs.Len();

    for(int i=0; i<coefs.Len(); i++)
    {
        m << coefs[i];
    }

    return m;
}

template <typename T>
T & tPolynomial<T>::ReadSync( T & m )
{
    m >> baseArgument;

    // Read the length
    int newLength = 0;
    m >> newLength;
    coefs.SetLen(newLength);

    for(int i=0; i<coefs.Len(); i++)
    {
        m >> coefs[i];
    }

    return m;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator*( REAL value ) const {
    tPolynomial<T> tf(*this);
    for(int i=0; i<coefs.Len(); i++) {
        tf[i] *= value;
    }
    return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator*( const tPolynomial<T> & tfRight ) const {
    tPolynomial<T> tf;

    // If any Polygonial is of size 0, then the resulting one is too
    // Otherwise, it is the sum of both lenght, minus 1.
    int newLength = 
      (0 == this->coefs.Len() || 0 == tfRight.coefs.Len())
      ? 0 
      : (this->coefs.Len() + tfRight.coefs.Len() - 1);

    tf.coefs.SetLen(newLength);

    if(0 == newLength) {
        // Special case, nothing needs to be done
    }
    else {
        for(int i=0; i<this->coefs.Len(); i++) {
            for(int j=0; j<tfRight.coefs.Len(); j++) {
	      tf[i+j] += (this->coefs[i]) * tfRight[j];
            }
        }
    }
    return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( REAL value ) const {
    tPolynomial<T> tf(*this);
    tf[0] += value;
    return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( const tPolynomial<T> &tfRight ) const {
    tPolynomial<T> tf(*this);
    int maxLength = this->coefs.Len()>tfRight.coefs.Len()?this->coefs.Len():tfRight.coefs.Len();
    // Set the lenght to the longest member of the addition
    tf.coefs.SetLen(maxLength);

    for(int i=0; i<maxLength; i++) {
        tf[i] += tfRight[i];
    }
    return tf;
}

template<typename T>
REAL &tPolynomial<T>::operator[](int index) // Allow both reading and writing of element
{
  // Manually growing the array to set all new elements to 0
  if(index >= coefs.Len()) {
    int previousLength = coefs.Len();
    coefs.SetLen(index + 1);
    for(int i=previousLength; i<coefs.Len(); i++) {
      coefs[i] = 0.0;
    }
  }

  return coefs[index];
}

template <typename T>
REAL const &tPolynomial<T>::operator[](int index) const // Allow reading of element even when the object is const
{
  return coefs[index];
}

template <typename T>
void tPolynomial<T>::operator=(tPolynomial<T> const &other) 
{
    coefs = other.coefs;
    baseArgument = other.baseArgument;
}

template <typename T>
void tPolynomial<T>::toString() {
    std::cout << "base :" << baseArgument << " lenght:" << coefs.Len();

    for(int i=0; i<coefs.Len(); i++) {
      std::cout << " c[" << i << "]:" << coefs[i];
    }
    std::cout << std::endl;
}

#endif

