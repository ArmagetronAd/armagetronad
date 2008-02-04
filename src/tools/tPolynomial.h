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
    explicit tPolynomial(int count);  //!< constructor
    tPolynomial(REAL newCoefs[], int count);  //!< constructor
    tPolynomial(REAL value);  //!< constructor for constant polynomial
    tPolynomial(tArray<REAL> newCoefs);  //!< constructor
    tPolynomial(const tPolynomial<T> &tf);  //!< constructor

    virtual ~tPolynomial() //!< destructor
    {
    }

    virtual REAL evaluate( REAL valueOfVariable ) const; //!< evaluates the function
    inline REAL operator()( REAL valueOfVariable ) const; //!< evaluation operator
    tPolynomial<T> const operator*( REAL constant ) const;
    tPolynomial<T> const operator*( const tPolynomial<T> & tfRight ) const ;
    tPolynomial<T> const operator+( REAL constant ) const ;
    tPolynomial<T> const operator+( const tPolynomial<T> &tfRight ) const ;
    tPolynomial<T> & operator+=( const tPolynomial<T> &tfRight ) ;

    REAL &operator[](int index); // Allow both reading and writing of element
    REAL const &operator[](int index) const; // Allow reading of element even when the object is const
    void operator=(tPolynomial<T> const &other);

    void addConstant(REAL constant) {
        if (coefs.Len()==0) {
            coefs.SetLen(1);
            coefs[0] = 0;
        } coefs[0] += constant;
    }

    tPolynomial<T> evaluateCoefsAt(REAL valueOfVariable) const;
    void changeRate(REAL newRate, int newRateLength, REAL valueOfVariable);

    void setRates(REAL newValues[], int newValuesLength, REAL valueOfVariable);
    void setRates(tArray<REAL> newValues, REAL valueOfVariable);

    virtual T & ReadSync( T & m );
    virtual T & WriteSync( T & m ) const;

    template<typename D>
    friend bool operator == (const tPolynomial<D> & left, const tPolynomial<D> & right);
    template<typename D>
    friend bool operator != (const tPolynomial<D> & left, const tPolynomial<D> & right);

    virtual void toString() const;
    tPolynomial<T> clamp(REAL min, REAL max, REAL valueOfVariable);

    int Len() const{
        return coefs.Len();
    };
protected:
    void setBaseValueOfVariable(REAL newBaseValueOfVariable) {
        baseValueOfVariable = newBaseValueOfVariable;
    }
    void growCoefsArray(int newLength);

    // Variables
    REAL baseValueOfVariable; //!< the evaluation is always done on (valueOfVariable - baseValueOfVariable) rather than (valueOfVariable)
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
template<typename T>
bool operator != (const tPolynomial<T> & left, const tPolynomial<T> & right);

template <typename T>
tPolynomial<T>::tPolynomial(int count)  //!< constructor
        : baseValueOfVariable(0.0),
        coefs(count)
{
    // Initialise to all 0's
    for (int i=0; i<coefs.Len(); i++)
        coefs[i] = 0.0f;
}

template <typename T>
tPolynomial<T>::tPolynomial()  //!< constructor
        : baseValueOfVariable(0.0),
        coefs(0)
{
    // Empty
}

template <typename T>
tPolynomial<T>::tPolynomial(REAL newCoefs[], int count)  //!< constructor
        : baseValueOfVariable(0.0),
        coefs(count)
{
    for (int i=0; i<coefs.Len(); i++)
        coefs[i] = newCoefs[i];
}

template <typename T>
tPolynomial<T>::tPolynomial(REAL value)  //!< constructor for constant polynomial
        : baseValueOfVariable(0.0),
        coefs(1)
{
    coefs[0] = value;
}

template <typename T>
tPolynomial<T>::tPolynomial(tArray<REAL> newCoefs)  //!< constructor
        : baseValueOfVariable(0.0),
        coefs(newCoefs)
{
    // Empty
}

template <typename T>
tPolynomial<T>::tPolynomial(const tPolynomial<T> &tf)  //!< constructor
        : baseValueOfVariable(tf.baseValueOfVariable),
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
REAL tPolynomial<T>::operator ( )( REAL valueOfVariable ) const
{
    return evaluate( valueOfVariable );
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
#define DELTA 1e-3

template<typename T>
bool operator == (const tPolynomial<T> & left, const tPolynomial<T> & right)
{
    tPolynomial<T> tRebasedRight;

    // Do both polynomial have the same baseValue?
    if (false == ( fabs(left.baseValueOfVariable - right.baseValueOfVariable) < DELTA)) {
        // Bring back both polynomial to the same baseValue for easy comparision
        tRebasedRight = right.evaluateCoefsAt(left.baseValueOfVariable);
    }
    else {
        // They have similar baseValue, no need to adjust.
        tRebasedRight = right;
    }


    // If the length of the coefs array differ, then the extra elements should be 0
    int maxLength = left.coefs.Len()>right.coefs.Len()?left.coefs.Len():right.coefs.Len();
    int minLength = left.coefs.Len()<right.coefs.Len()?left.coefs.Len():right.coefs.Len();

    bool res = true;

    // Inspect the common coefficients (ie defined for both polynomial)
    for (int i=0; i<minLength; i++) {
        if ( fabs(left[i] - tRebasedRight[i]) >= DELTA ) {
            res = false;
            break;
        }
    }

    for (int i=minLength; i<maxLength; i++) {
        // The polynomial that is defined up to that length should have its elements set to 0.0
        if (left.coefs.Len()>tRebasedRight.coefs.Len()) {
            if (fabs(left[i]) >= DELTA) {
                res = false;
                break;
            }
        }
        else {
            if (fabs(tRebasedRight[i]) >= DELTA) {
                res = false;
                break;
            }
        }
    }
    return res;
}

template<typename T>
bool operator != (const tPolynomial<T> & left, const tPolynomial<T> & right)
{
    return !(left == right);
}


/**
 *
 */
template <typename T>
tPolynomial<T> tPolynomial<T>::evaluateCoefsAt(REAL valueOfVariable) const
{
    tPolynomial<T> tf(*this);

    // Compute the coefficients at "valueOfVariable" (var for short)
    // c0' = c0 + (c1/sum(1))*var + (c2/sum(2))*var^2 + ... + (c[N]/sum(N))*var^N
    // c1' = c1 + (c2/sum(1))*var + ... + (c[N]/sum(N-1))*var^(N-1)
    // c2' = c2 + ... + (c[N]/sum(N-2))*var^(N-2)
    // ...
    // c[N-1] = c[N-1] + (c[N]/sum(1))*var
    // c[N] = c[N]
    //
    // so:
    // [0   , 0 , a] at valueOfVariable=0 will become
    // [9a/2, 3a, a] at valueOfVariable=3

    REAL deltaVariableValue = valueOfVariable - baseValueOfVariable;

    // Compute for each coefficient their new value
    for (int coefIndex=0; coefIndex<coefs.Len(); coefIndex++) {
        REAL newCoefValue = 0.0;
        for (int j=coefs.Len()-1; j>coefIndex; j--) {
            newCoefValue = (newCoefValue + coefs[j] ) * deltaVariableValue /(j - coefIndex);
        }
        tf.coefs[coefIndex] += newCoefValue;
    }

    tf.setBaseValueOfVariable(valueOfVariable);
    return tf;
}

/**
 *
 */
template <typename T>
void tPolynomial<T>::changeRate(REAL newRate, int newRateIndex, REAL valueOfVariable)
{
    if (coefs.Len() <= newRateIndex) {
        int oldLength = coefs.Len();
        coefs.SetLen(newRateIndex + 1);
        for (int i=oldLength; i<newRateIndex; i++) {
            coefs[i] = 0.0;
        }
    }

    *this = evaluateCoefsAt(valueOfVariable);

    coefs[newRateIndex] = newRate;
}

/**
 * This perform a hard setting of all the coefficients
 */
template <typename T>
void tPolynomial<T>::setRates(REAL newValues[], int newValuesLength, REAL valueOfVariable)
{
    if (coefs.Len() < newValuesLength) {
        int oldLength = coefs.Len();
        coefs.SetLen(newValuesLength + 1);
        for (int i=oldLength; i<newValuesLength; i++) {
            coefs[i] = 0.0;
        }
    }

    setBaseValueOfVariable(valueOfVariable);

    for (int i=0; i<newValuesLength; i++) {
        coefs[i] = newValues[i];
    }
}

template <typename T>
void tPolynomial<T>::setRates(tArray<REAL> newValues, REAL valueOfVariable)
{
    coefs = newValues;
    setBaseValueOfVariable(valueOfVariable);
}

template <typename T>
REAL tPolynomial<T>::evaluate( REAL valueOfVariable ) const
{
    REAL deltaVariableValue = (valueOfVariable - baseValueOfVariable);

    REAL res = 0.0;

    // Compute res = c[0] + c[1]*var + (c[2]/2)*var^2 + ... + (c[N]/N)*var^N
    for (int i=coefs.Len()-1; i>0; i--) {
        res = (res + coefs[i]/i) * deltaVariableValue;
    }
    if (coefs.Len()!=0)
        res += (coefs[0]);

    return res;

}

template <typename T>
T & tPolynomial<T>::WriteSync( T & m ) const
{
    m << baseValueOfVariable;
    // write length
    m << coefs.Len();

    for (int i=0; i<coefs.Len(); i++)
    {
        m << coefs[i];
    }

    return m;
}

template <typename T>
T & tPolynomial<T>::ReadSync( T & m )
{
    m >> baseValueOfVariable;

    // Read the length
    int newLength = 0;
    m >> newLength;
    coefs.SetLen(newLength);

    for (int i=0; i<coefs.Len(); i++)
    {
        m >> coefs[i];
    }

    return m;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator*( REAL constant ) const {
    tPolynomial<T> tf(*this);

    for (int i=0; i<coefs.Len(); i++) {
        tf[i] *= constant;
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

    int oldLength = tf.coefs.Len();
    tf.coefs.SetLen(newLength);
    for (int i=oldLength; i<newLength; i++) {
        tf[i] = 0.0;
    }

    if (0 == newLength) {
        // Special case, nothing needs to be done
    }
    else {
        for (int i=0; i<this->coefs.Len(); i++) {
            for (int j=0; j<tfRight.coefs.Len(); j++) {
                tf[i+j] += (this->coefs[i]) * tfRight[j];
            }
        }
    }
    return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( REAL constant ) const {
    tPolynomial<T> tf(*this);
    tf[0] += constant;
    return tf;
}

template <typename T>
tPolynomial<T> const tPolynomial<T>::operator+( const tPolynomial<T> &tfRight ) const {
    // Bring the polynomial to the same baseValue, so that the terms mean the same thing
    tPolynomial<T> tRebasedRight(tfRight.evaluateCoefsAt(this->baseValueOfVariable));

    int maxLength = this->coefs.Len()>tfRight.coefs.Len()?this->coefs.Len():tfRight.coefs.Len();
    int minLength = this->coefs.Len()<tfRight.coefs.Len()?this->coefs.Len():tfRight.coefs.Len();

    // Set the lenght to the longest member of the addition
    tRebasedRight.coefs.SetLen(maxLength);
    for (int i=tfRight.Len(); i<maxLength; i++) {
        tRebasedRight[i] = 0.0;
    }

    for (int i=0; i<minLength; i++) {
        tRebasedRight[i] += coefs[i];
    }

    return tRebasedRight;
}

template <typename T>
tPolynomial<T> & tPolynomial<T>::operator+=( const tPolynomial<T> &tfRight ) {
    // Bring the polynomial to the same baseValue, so that the terms mean the same thing
    tPolynomial<T> tRebasedRight = tfRight.evaluateCoefsAt(this->baseValueOfVariable);

    int maxLength = this->coefs.Len()>tfRight.coefs.Len()?this->coefs.Len():tfRight.coefs.Len();
    // Set the lenght to the longest member of the addition
    coefs.SetLen(maxLength);

    for (int i=0; i<maxLength; i++) {
        coefs[i] += tRebasedRight[i];
    }

    return *this;
}

template<typename T>
REAL &tPolynomial<T>::operator[](int index) // Allow both reading and writing of element
{
    // Manually growing the array to set all new elements to 0
    if (index >= coefs.Len()) {
        int previousLength = coefs.Len();
        coefs.SetLen(index + 1);
        for (int i=previousLength; i<coefs.Len(); i++) {
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
    baseValueOfVariable = other.baseValueOfVariable;
}

template <typename T>
void tPolynomial<T>::toString() const {
    std::cout << "base :" << baseValueOfVariable << " lenght:" << coefs.Len();

    for (int i=0; i<coefs.Len(); i++) {
        std::cout << " c[" << i << "]:" << coefs[i];
    }
    std::cout << std::endl;
}

/**
 * If the current polynomial is within the range, return a copy of itself
 * Otherwise, return a new polynomial that is bound by the range
 */
template <typename T>
tPolynomial<T> tPolynomial<T>::clamp(REAL minValue, REAL maxValue, REAL valueOfVariable)
{
    tPolynomial<T> tf(*this);

    REAL valueAt = evaluate(valueOfVariable);
    if (valueAt < minValue) {
        tf[0] = minValue;
        for (int i=1; i<coefs.Len(); i++) {
            if (tf[i] < 0) {
                tf[i] = 0.0;
            }
        }
    }
    if (maxValue < valueAt) {
        tf[0] = maxValue;
        for (int i=1; i<coefs.Len(); i++) {
            if (tf[i] > 0) {
                tf[i] = 0.0;
            }
        }
    }

    return tf;
}

#endif

