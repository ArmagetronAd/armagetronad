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
#include "tFunction.h"
#include <math.h>
#include "tArray.h"

#include <iostream>
#include <string>

namespace Tools { class PolynomialSync; }

#define REAL float
#define MAX(a, b) ((a>b)?a:b)
#define MIN(a, b) ((a<b)?a:b)


//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
class tPolynomial
{
public:

    tPolynomial();  //!< constructor
    explicit tPolynomial(int count);  //!< constructor
    tPolynomial(REAL newCoefs[], int count);  //!< constructor
    tPolynomial(REAL value);  //!< constructor for constant polynomial
    tPolynomial(tArray<REAL> const & newCoefs);  //!< constructor
    tPolynomial(const tPolynomial &tf);  //!< constructor
    tPolynomial(const tFunction &tf);  //!< constructor
    tPolynomial(std::string str);  //!< constructor

    virtual ~tPolynomial() //!< destructor
    {
    }

    void parse(std::string str);

    virtual REAL evaluate( REAL currentVarValue ) const; //!< evaluates the function
    inline REAL operator()( REAL currentVarValue ) const; //!< evaluation operator
    
    virtual tFunction simplify( REAL currentVarValue ) const; //!< evaluates the function only so far as to simplify it to a tFunction

    tPolynomial const operator*( REAL constant ) const;
    tPolynomial const operator*( const tPolynomial & tfRight ) const ;
    tPolynomial const operator+( REAL constant ) const ;
    tPolynomial const operator+( const tPolynomial &tfRight ) const ;
    tPolynomial & operator+=( const tPolynomial &tfRight ) ;

    tPolynomial const substitute( const tPolynomial &other ) const;

    REAL &operator[](int index); // Allow both reading and writing of element
    REAL const &operator[](int index) const; // Allow reading of element even when the object is const
    void operator=(tPolynomial const &other);

    void addConstant(REAL constant) {
        if (coefs.Len()==0) {
            coefs.SetLen(1);
            coefs[0] = 0;
        } 
	coefs[0] += constant;
    }

    tPolynomial adaptToNewReferenceVarValue(REAL currentVarValue) const;
    tPolynomial translate(REAL currentVarValue) const;
    void changeRate(REAL newRate, int newRateLength, REAL currentVarValue);

    void setRates(REAL newValues[], int newValuesLength, REAL currentVarValue);
    void setRates(tArray<REAL> newValues, REAL currentVarValue);

    void ReadSync ( Tools::PolynomialSync const & m );
    void WriteSync( Tools::PolynomialSync       & m ) const;

    friend bool operator == (const tPolynomial & left, const tPolynomial & right);
    friend bool operator != (const tPolynomial & left, const tPolynomial & right);

    virtual std::string toString() const;
    tPolynomial const clamp(REAL min, REAL max, REAL currentVarValue);

    int Len() const{
        return coefs.Len();
    };

    void setAtSameReferenceVarValue(const tPolynomial & other);
protected:
    void setReferenceVarValue(REAL newReferenceVarValue) {
        referenceVarValue = newReferenceVarValue;
    }
    void growCoefsArray(int newLength);

    // Variables
    REAL referenceVarValue; //!< the evaluation is always done on (currentVarValue - referenceVarValue) rather than (currentVarValue)
    tArray<REAL> coefs;
};

bool operator == (const tPolynomial & left, const tPolynomial & right);
bool operator != (const tPolynomial & left, const tPolynomial & right);

#endif

