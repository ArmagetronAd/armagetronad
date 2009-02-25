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

#ifndef ArmageTron_tPolynomialMarshaler_H
#define ArmageTron_tPolynomialMarshaler_H


#include "tPolynomial.h"

/*! \brief Marshal a tPolynomial as an input value (var) for "a + b*var + (c + d*var)*t"
 *
 *
 */
class tPolynomialMarshaler 
{
 public:
    tPolynomialMarshaler(); //!< Default constructor
    tPolynomialMarshaler(std::string str); //!< Parsing constructor
    tPolynomialMarshaler(const tPolynomialMarshaler & other); //!< Copy constructor
    tPolynomialMarshaler(const tPolynomial & _constant, const tPolynomial & _variant); //!< Constructor from 2 tPolynomial

    void setConstant(REAL value, int index);
    void setVariant(REAL value, int index);
    void setConstant(const tPolynomial & tpConstant);
    void setVariant(const tPolynomial & tpVariant);
    
    void parse(std::string str);

    tPolynomial const marshal(const tPolynomial & other);

    std::string toString() const;

    friend bool operator == (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right);
    friend bool operator != (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right);
 protected:
    void parsePart(std::string str, tArray<REAL> &array);
    int getLenConstant() const {return constant.Len();};
    int getLenVariant()const {return variant.Len();};

    tPolynomial constant;
    tPolynomial variant;
};

// *******************************************************
// *******************************************************
// *******************************************************
// *******************************************************

bool operator == (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right);
bool operator != (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right);

#endif
