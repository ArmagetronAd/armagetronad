/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "tPolynomial.h"
#include "tPolynomial.pb.h"

tPolynomial::tPolynomial(int count)  //!< constructor
        : referenceVarValue(0.0),
        coefs(count)
{
    // Initialise to all 0's
    for (int i=0; i<coefs.Len(); i++)
        coefs[i] = 0.0f;
}

tPolynomial::tPolynomial()  //!< constructor
        : referenceVarValue(0.0),
        coefs(0)
{
    // Empty
}

tPolynomial::tPolynomial(REAL newCoefs[], int count)  //!< constructor
        : referenceVarValue(0.0),
        coefs(count)
{
    for (int i=0; i<coefs.Len(); i++)
        coefs[i] = newCoefs[i];
}

tPolynomial::tPolynomial(REAL value)  //!< constructor for constant polynomial
        : referenceVarValue(0.0),
        coefs(1)
{
    coefs[0] = value;
}

tPolynomial::tPolynomial(tArray<REAL> const & newCoefs)  //!< constructor
        : referenceVarValue(0.0),
        coefs(newCoefs)
{
    // Empty
}

tPolynomial::tPolynomial(const tPolynomial &tf)  //!< constructor
        : referenceVarValue(tf.referenceVarValue),
        coefs(tf.coefs)
{
    // Empty
}

tPolynomial::tPolynomial(const tFunction &tf, REAL refValue)  //!< constructor
        : referenceVarValue(refValue),
        coefs(2)
{
    coefs[0] = tf.offset_;
    coefs[1] = tf.slope_;
}

tPolynomial::tPolynomial(const tFunction &tf)  //!< constructor
        : referenceVarValue(0.0),
        coefs(2)
{
    coefs[0] = tf.offset_;
    coefs[1] = tf.slope_;
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

template<typename T> T & operator << ( T & m, tPolynomial const & f )
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

template<typename T> T & operator >> ( T & m, tPolynomial & f )
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

bool operator == (const tPolynomial & left, const tPolynomial & right)
{
    tPolynomial tRebasedRight;

    // Do both polynomial have the same baseValue?
    if (false == ( fabs(left.referenceVarValue - right.referenceVarValue) < DELTA)) {
        // Bring back both polynomial to the same baseValue for easy comparision
        tRebasedRight = right.adaptToNewReferenceVarValue(left.referenceVarValue);
    }
    else {
        // They have similar baseValue, no need to adjust.
        tRebasedRight = right;
    }


    // If the length of the coefs array differ, then the extra elements should be 0
    int maxLength = MAX(left.coefs.Len(), right.coefs.Len());
    int minLength = MIN(left.coefs.Len(), right.coefs.Len());

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

bool operator != (const tPolynomial & left, const tPolynomial & right)
{
    return !(left == right);
}


/**
 *
 */
tPolynomial tPolynomial::adaptToNewReferenceVarValue(REAL currentVarValue) const
{
    tPolynomial tf(*this);

    // Compute the coefficients at "currentVarValue" (var for short)
    // c0' = c0 + (c1/sum(1))*var + (c2/sum(2))*var^2 + ... + (c[N]/sum(N))*var^N
    // c1' = c1 + (c2/sum(1))*var + ... + (c[N]/sum(N-1))*var^(N-1)
    // c2' = c2 + ... + (c[N]/sum(N-2))*var^(N-2)
    // ...
    // c[N-1] = c[N-1] + (c[N]/sum(1))*var
    // c[N] = c[N]
    //
    // so:
    // [0   , 0 , a] at currentVarValue=0 will become
    // [9a/2, 3a, a] at currentVarValue=3

    REAL deltaVariableValue = currentVarValue - referenceVarValue;

    // Compute for each coefficient their new value
    for (int coefIndex=0; coefIndex<coefs.Len(); coefIndex++) {
        REAL newCoefValue = 0.0;
        for (int j=coefs.Len()-1; j>coefIndex; j--) {
            newCoefValue = (newCoefValue + coefs[j] ) * deltaVariableValue /(j - coefIndex);
        }
        tf.coefs[coefIndex] += newCoefValue;
    }

    tf.setReferenceVarValue(currentVarValue);
    return tf;
}

/**
 *
 */
tPolynomial tPolynomial::translate(REAL currentVarValue) const
{
    tPolynomial tf(*this);

    // Compute the coefficients at "currentVarValue" (var for short)
    // c0' = c0 + (c1)*var + (c2)*var^2 + ... + (c[N])*var^N
    // c1' = c1 + (c2)*var + ... + (c[N])*var^(N-1)
    // c2' = c2 + ... + (c[N])*var^(N-2)
    // ...
    // c[N-1] = c[N-1] + (c[N])*var
    // c[N] = c[N]
    //
    // so:
    // [0   , 0 , a] at currentVarValue=0 will become
    // [9a, 6a, a] at currentVarValue=3

    REAL deltaVariableValue = currentVarValue - referenceVarValue;

    // Compute for each coefficient their new value
    for (int coefIndex=0; coefIndex<coefs.Len(); coefIndex++) {
        REAL newCoefValue = 0.0;
        for (int j=coefs.Len()-1; j>coefIndex; j--) {
            newCoefValue = (newCoefValue + coefs[j] ) * deltaVariableValue;
        }
        tf.coefs[coefIndex] += newCoefValue;
    }

    tf.setReferenceVarValue(currentVarValue);
    return tf;
}

/**
 *
 */
REAL tPolynomial::evaluateRate(int index, REAL currentVarValue)
{
    if (coefs.Len() <= index)
        return 0.0;

    *this = adaptToNewReferenceVarValue(currentVarValue);
    return coefs[index];
}

/**
 *
 */
void tPolynomial::changeRate(REAL newRate, int newRateIndex, REAL currentVarValue)
{
    if (coefs.Len() <= newRateIndex) {
        int oldLength = coefs.Len();
        coefs.SetLen(newRateIndex + 1);
        for (int i=oldLength; i<newRateIndex; i++) {
            coefs[i] = 0.0;
        }
    }

    *this = adaptToNewReferenceVarValue(currentVarValue);

    coefs[newRateIndex] = newRate;
}

/**
 * This perform a hard setting of all the coefficients
 */
void tPolynomial::setRates(REAL newValues[], int newValuesLength, REAL currentVarValue)
{
    if (coefs.Len() < newValuesLength) {
        int oldLength = coefs.Len();
        coefs.SetLen(newValuesLength + 1);
        for (int i=oldLength; i<newValuesLength; i++) {
            coefs[i] = 0.0;
        }
    }

    setReferenceVarValue(currentVarValue);

    for (int i=0; i<newValuesLength; i++) {
        coefs[i] = newValues[i];
    }
}

void tPolynomial::setRates(tArray<REAL> newValues, REAL currentVarValue)
{
    coefs = newValues;
    setReferenceVarValue(currentVarValue);
}

REAL tPolynomial::evaluate( REAL currentVarValue ) const
{
    REAL deltaVariableValue = (currentVarValue - referenceVarValue);

    REAL res = 0.0;

    // Compute res = c[0] + c[1]*var + (c[2]/2)*var^2 + ... + (c[N]/N)*var^N
    for (int i=coefs.Len()-1; i>0; i--) {
        res = (res + coefs[i]/i) * deltaVariableValue;
    }
    if (coefs.Len()!=0)
        res += (coefs[0]);

    return res;

}

tFunction tPolynomial::simplify( REAL currentVarValue ) const
{
    switch (coefs.Len())
    {
    case 0:
        return tFunction(0, 0);
    case 1:
        return tFunction(coefs[0], 0);
    case 2:
        return tFunction(coefs[0], coefs[1]);
    }
    
    REAL deltaVariableValue = (currentVarValue - referenceVarValue);

    REAL res = 0.0;

    // Compute res = c[1] + (c[2]/2)*var + (c[3]/3)*var^2 + ... + (c[N]/N)*var^(N-1)
    // FIXME: TODO: HACK: Someone who has a clue how this works, please check it. Thanks, Luke
    for (int i=coefs.Len()-1; i>1; i--) {
        res = (res + coefs[i]/i) * deltaVariableValue;
    }
        res += (coefs[1]);

    return tFunction(coefs[0], res);
}

void tPolynomial::WriteSync(  Tools::PolynomialSync & m ) const
{
    m.set_reference( referenceVarValue );

    m.clear_coefficients();
    for (int i=0; i<coefs.Len(); i++)
    {
        m.add_coefficients( coefs[i] );
    }
}

void tPolynomial::ReadSync( Tools::PolynomialSync const & m )
{
    referenceVarValue = m.reference();

    // Read the length
    int newLength = m.coefficients_size();
    coefs.SetLen(newLength);

    for (int i=0; i<coefs.Len(); i++)
    {
        coefs[i] = m.coefficients(i);
    }
}

tPolynomial const tPolynomial::operator*( REAL constant ) const {
    tPolynomial tf(*this);

    for (int i=0; i<coefs.Len(); i++) {
        tf[i] *= constant;
    }
    return tf;
}

tPolynomial const tPolynomial::operator*( const tPolynomial & tfRight ) const {
    tPolynomial tf;
    tf.setAtSameReferenceVarValue(tfRight);

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

tPolynomial const tPolynomial::operator+( REAL constant ) const {
    tPolynomial tf(*this);
    tf[0] += constant;
    return tf;
}

tPolynomial const tPolynomial::operator+( const tPolynomial &tfRight ) const {
    // Bring the polynomial to the same baseValue, so that the terms mean the same thing
    tPolynomial tRebasedRight(tfRight.adaptToNewReferenceVarValue(this->referenceVarValue));

    int maxLength = MAX(this->coefs.Len(), tfRight.coefs.Len());

    // Set the lenght to the longest member of the addition
    tRebasedRight.coefs.SetLen(maxLength);

    for (int i=0; i<this->coefs.Len(); i++) {
        tRebasedRight[i] += coefs[i];
    }

    return tRebasedRight;
}

tPolynomial & tPolynomial::operator+=( const tPolynomial &tfRight ) {
    // Bring the polynomial to the same baseValue, so that the terms mean the same thing
    tPolynomial tRebasedRight = tfRight.adaptToNewReferenceVarValue(this->referenceVarValue);

    int maxLength = MAX(this->coefs.Len(), tfRight.coefs.Len());
    // Set the lenght to the longest member of the addition
    coefs.SetLen(maxLength);

    for (int i=0; i<maxLength; i++) {
        coefs[i] += tRebasedRight[i];
    }

    return *this;
}

/*! \brief Evaluate this(x) where x is another polynomial
 *
 */
tPolynomial const tPolynomial::substitute( const tPolynomial &other ) const {
  tPolynomial tf(0);
  tf.setAtSameReferenceVarValue(other);
  for(int i=this->Len()-1; i>0; i--) {
    tf = (tf + (*this)[i]) * other;
  }
  if(0 != this->Len()) {
    tf = tf + (*this)[0];
  }

  return tf;
}

REAL &tPolynomial::operator[](int index) // Allow both reading and writing of element
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

REAL const &tPolynomial::operator[](int index) const // Allow reading of element even when the object is const
{
    return coefs[index];
}

void tPolynomial::operator=(tPolynomial const &other)
{
    coefs = other.coefs;
    referenceVarValue = other.referenceVarValue;
}

tPolynomial::tPolynomial(std::string str)
  : referenceVarValue(0.0),
     coefs(0)
{
  parse(str);
}

void tPolynomial::parse(std::string str)
{
    int pos;
    int prevPos = 0;
    int index = 0;

#define TPOLYNOMIAL_DELIMITER ';'

    pos = str.find(TPOLYNOMIAL_DELIMITER, 0);
    if(-1 != pos) {
      do{
	REAL value = atof(str.substr(prevPos, pos).c_str());
	coefs.SetLen(index + 2); // +1 because to write at index n, the len must be n+1. +1 to allocate a place for the element after the last ':'
	coefs[index] = value;
	
	prevPos = pos + 1;
	index ++;
      }
      while ( (pos = str.find(TPOLYNOMIAL_DELIMITER, prevPos)) != -1) ;

      coefs[index] = atof(str.substr(prevPos, pos).c_str());

    }
    else {
      coefs.SetLen(1);
      coefs[0] = atof(str.c_str());
    }
}

std::string tPolynomial::toString() const {
    std::ostringstream ostr("");

    ostr << "base :" << referenceVarValue << " lenght:" << coefs.Len();

    for (int i=0; i<coefs.Len(); i++) {
        ostr << " c[" << i << "]:" << coefs[i];
    }
    return ostr.str();
}


/**
 * If the current polynomial is within the range, return a copy of itself
 * Otherwise, return a new polynomial that is bound by the range
 */
tPolynomial const tPolynomial::clamp(REAL minValue, REAL maxValue, REAL currentVarValue)
{
    tPolynomial tf(*this);

    REAL valueAt = evaluate(currentVarValue);
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

/**
 * Will change our referenceVarValue to that of the designated object.
 */
void tPolynomial::setAtSameReferenceVarValue(tPolynomial const &other)
{
  REAL a = other.referenceVarValue;
  referenceVarValue = a;
  //  referenceVarValue = other.referenceVarValue;
}

// *******************************************************
// *******************************************************
// *******************************************************
// *******************************************************


