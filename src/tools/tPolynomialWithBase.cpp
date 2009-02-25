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

#include "tPolynomialWithBase.h"

#include "tPolynomial.pb.h"

tPolynomialWithBase::tPolynomialWithBase()  //!< constructor
: tPolynomial(),
  unadjustableOffset(0.0)
{
  // Empty
}

tPolynomialWithBase::tPolynomialWithBase(REAL value)  //!< constructor
: tPolynomial(value),
  unadjustableOffset(0.0)
{ 
  // Empty
}


tPolynomialWithBase::tPolynomialWithBase(tArray<REAL> const & coefs_)  //!< constructor
: tPolynomial(coefs_),
  unadjustableOffset(0.0)
{
  // Empty
}

tPolynomialWithBase::tPolynomialWithBase(const tPolynomialWithBase &tf)  //!< constructor
: tPolynomial(tf),
  unadjustableOffset(tf.unadjustableOffset)
{
  // Empty
}

tPolynomialWithBase::tPolynomialWithBase(const tPolynomial &tf)  //!< constructor
: tPolynomial(tf),
  unadjustableOffset(0.0)
{
  // Empty
}

/**
 *
 */
#define DELTA 1e-10

bool operator == (const tPolynomialWithBase & left, const tPolynomialWithBase & right)
{
    bool res = static_cast<tPolynomial >(left) == static_cast<tPolynomial >(right);

    if(true == res) {
      // float/double equality doesnt exist. Accept small difference as equal.
      res = ( fabs(left.unadjustableOffset - right.unadjustableOffset) < DELTA);
    }
    
    return res;
}

REAL tPolynomialWithBase::evaluate( REAL argument ) const
{
  REAL res = tPolynomial::evaluate( argument );

  res += unadjustableOffset;
  return res;
}

void tPolynomialWithBase::WriteSync( Tools::PolynomialWithBaseSync       & m ) const
{
    tPolynomial::WriteSync( *m.mutable_base() );

    // write unadjustableOffset
    m.set_offset( unadjustableOffset );
}

void tPolynomialWithBase::ReadSync ( Tools::PolynomialWithBaseSync const & m )
{
    tPolynomial::ReadSync( m.base() );

    // read unadjustableOffset
    unadjustableOffset = m.offset();
}
