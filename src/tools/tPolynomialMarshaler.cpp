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

#include "tPolynomialMarshaler.h"

// *******************************************************
// *******************************************************

tPolynomial const tPolynomialMarshaler::marshal(const tPolynomial & other)
{
    tPolynomial tf(0);
    tPolynomial res(0);

    tf.setAtSameReferenceVarValue(other);
    res.setAtSameReferenceVarValue(other);

    tf = variant.substitute(other);

    {
      REAL tData[] = {0.0, 1.0};
      tPolynomial t(tData, sizeof(tData)/sizeof(tData[0]));
      t.setAtSameReferenceVarValue(other);

      res = tf * t;
    }

    // Reset tf for further computation
    tf = tPolynomial(0);
    tf.setAtSameReferenceVarValue(other);

    tf = constant.substitute(other);

    res += tf;

    return res;
}



tPolynomialMarshaler::tPolynomialMarshaler() 
:constant(0),
  variant(0)
{
  // Empty
}

tPolynomialMarshaler::tPolynomialMarshaler(std::string str)
:constant(0),
  variant(0)
{
  parse(str);
}

tPolynomialMarshaler::tPolynomialMarshaler(const tPolynomialMarshaler & other)
:constant(other.constant),
  variant(other.variant)
{
  // Empty
}

tPolynomialMarshaler::tPolynomialMarshaler(const tPolynomial & _constant, const tPolynomial & _variant)
  :constant(_constant),
   variant(_variant)
{
  // Empty
}

// **************************************************************    
// **************************************************************    

void tPolynomialMarshaler::parse(std::string str)
{
    int bPos;
#define TPOLYNOMIAL_MARSHALER_DELIMITER ':'

    if ( (bPos = str.find(TPOLYNOMIAL_MARSHALER_DELIMITER)) != -1)
    {
        constant.parse(str.substr(0, bPos));
        variant.parse(str.substr(bPos + 1, str.length()));
    }
    else
    {
        constant.parse(str);
        variant = tPolynomial(0);
    }
}

void tPolynomialMarshaler::setConstant(REAL value, int index)
{
  constant[index] = value;
}

void tPolynomialMarshaler::setVariant(REAL value, int index)
{
  variant[index] = value;
}

void tPolynomialMarshaler::setConstant(const tPolynomial & tpConstant)
{
  constant = tpConstant;
}

void tPolynomialMarshaler::setVariant(const tPolynomial & tpVariant)
{
  variant = tpVariant;
}

// friend
bool operator == (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right)
{
  bool res = true;
  
  res = (left.constant == right.constant)
     && (left.variant == right.variant);
  
  return res;
}

// friend
bool operator != (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right)
{
  return !(left == right);
}

std::string tPolynomialMarshaler::toString() const
{
  std::ostringstream ostr("");

  ostr << "Constant :" << std::endl;
  ostr << constant.toString() ;

  ostr << std::endl << "Variant :" << std::endl;
  ostr << variant.toString() ;

  return ostr.str();
}
