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
template<typename T>
class tPolynomialMarshaler 
{
 public:
    tPolynomialMarshaler(); //!< Default constructor
    tPolynomialMarshaler(std::string str); //!< Parsing constructor
    tPolynomialMarshaler(const tPolynomialMarshaler & other); //!< Copy constructor
    tPolynomialMarshaler(const tPolynomial<T> & _constant, const tPolynomial<T> & _variant); //!< Constructor from 2 tPolynomial

    void setConstant(REAL value, int index);
    void setVariant(REAL value, int index);
    
    void parse(std::string str);

    tPolynomial<T> const marshal(const tPolynomial<T> & other);

    std::string toString() const;

    template<typename D>
    friend bool operator == (const tPolynomialMarshaler<D> & left, const tPolynomialMarshaler<D> & right);
    template<typename D>
    friend bool operator != (const tPolynomialMarshaler<D> & left, const tPolynomialMarshaler<D> & right);
 protected:
    void parsePart(std::string str, tArray<REAL> &array);
    int getLenConstant() const {return constant.Len();};
    int getLenVariant()const {return variant.Len();};

    tPolynomial<T> constant;
    tPolynomial<T> variant;
};

// *******************************************************
// *******************************************************
// *******************************************************
// *******************************************************

template<typename T>
bool operator == (const tPolynomialMarshaler<T> & left, const tPolynomialMarshaler<T> & right);
template<typename T>
bool operator != (const tPolynomialMarshaler<T> & left, const tPolynomialMarshaler<T> & right);

// *******************************************************
// *******************************************************

template<typename T>
tPolynomial<T> const tPolynomialMarshaler<T>::marshal(const tPolynomial<T> & other)
{
    tPolynomial<T> tf(0);
    tPolynomial<T> res(0);

    tf.setAtSameReferenceVarValue(other);
    res.setAtSameReferenceVarValue(other);

    tf = variant.substitute(other);

    {
      REAL tData[] = {0.0, 1.0};
      tPolynomial<T> t(tData, sizeof(tData)/sizeof(tData[0]));
      t.setAtSameReferenceVarValue(other);

      res = tf * t;
    }

    // Reset tf for further computation
    tf = tPolynomial<T>(0);
    tf.setAtSameReferenceVarValue(other);

    tf = constant.substitute(other);

    res += tf;

    return res;
}



template<typename T>
tPolynomialMarshaler<T>::tPolynomialMarshaler() 
:constant(0),
  variant(0)
{
  // Empty
}

template<typename T>
tPolynomialMarshaler<T>::tPolynomialMarshaler(std::string str)
:constant(0),
  variant(0)
{
  parse(str);
}

template<typename T>
tPolynomialMarshaler<T>::tPolynomialMarshaler(const tPolynomialMarshaler & other)
:constant(other.constant),
  variant(other.variant)
{
  // Empty
}

template<typename T>
tPolynomialMarshaler<T>::tPolynomialMarshaler(const tPolynomial<T> & _constant, const tPolynomial<T> & _variant)
  :constant(_constant),
   variant(_variant)
{
  // Empty
}

// **************************************************************    
// **************************************************************    

template<typename T>
void tPolynomialMarshaler<T>::parse(std::string str)
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
        variant = tPolynomial<T>(0);
    }
}

template<typename T>
void tPolynomialMarshaler<T>::setConstant(REAL value, int index)
{
  constant[index] = value;
}

template<typename T>
void tPolynomialMarshaler<T>::setVariant(REAL value, int index)
{
  variant[index] = value;
}

// friend
template<typename T>
bool operator == (const tPolynomialMarshaler<T> & left, const tPolynomialMarshaler<T> & right)
{
  bool res = true;
  
  res = (left.constant == right.constant)
     && (left.variant == right.variant);
  
  return res;
}

// friend
template<typename T>
bool operator != (const tPolynomialMarshaler<T> & left, const tPolynomialMarshaler<T> & right)
{
  return !(left == right);
}

template<typename T>
std::string tPolynomialMarshaler<T>::toString() const
{
  std::ostringstream ostr("");

  ostr << "Constant :" << std::endl;
  ostr << constant.toString() ;

  ostr << std::endl << "Variant :" << std::endl;
  ostr << variant.toString() ;

  return ostr.str();
}

#endif
