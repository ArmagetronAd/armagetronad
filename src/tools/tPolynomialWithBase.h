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

namespace Tools { class PolynomialWithBaseSync; }

//! mathematical function (to be moved into tools sometime, and currently limited to linear functions)
class tPolynomialWithBase : public tPolynomial
{
public:

    tPolynomialWithBase();  //!< constructor
    tPolynomialWithBase(REAL value);  //!< constructor
    tPolynomialWithBase(tArray<REAL> const & newCoefs);  //!< constructor
    tPolynomialWithBase(const tPolynomialWithBase &tf);  //!< constructor
    tPolynomialWithBase(const tPolynomial &tf);  //!< constructor

    virtual ~tPolynomialWithBase() { //!< destructor
      // Empty
    }

    virtual REAL evaluate( REAL argument ) const; //!< evaluates the function
    inline REAL operator()( REAL argument ) const; //!< evaluation operator

    void setUnadjustableOffset(REAL x);
    REAL getUnadjustableOffset();

    void ReadSync ( Tools::PolynomialWithBaseSync const & m );
    void WriteSync( Tools::PolynomialWithBaseSync       & m ) const;

    friend bool operator == (const tPolynomialWithBase & left, const tPolynomialWithBase & right);

protected:
    REAL unadjustableOffset; //!< offset value that is not modified by an adjustSlope
};

bool operator == (const tPolynomialWithBase & left, const tPolynomial & right);
bool operator == (const tPolynomialWithBase & left, const tPolynomialWithBase & right);

#endif

