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

//! @file
//! @brief Contains the class declarations for all vValue members

#ifndef ARMAGETRON_veMath_h
#define ARMAGETRON_veMath_h

#include <math.h>
#include "tRandom.h"

long int ve_math_random();
#include "values/vCore.h"
#include "values/vebCFunction.h"


namespace vValue {
namespace Expr {
namespace Math {

class Add : public BinaryOp {
public:
    Add(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Add(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Subtract : public BinaryOp {
public:
    Subtract(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Subtract(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Multiply : public BinaryOp {
public:
    Multiply(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Multiply(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Divide : public BinaryOp {
public:
    Divide(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Divide(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Power : public BinaryOp {
public:
    Power(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Power(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Root : public BinaryOp {
public:
    Root(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Root(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};

typedef Bindings::CFunction::fZeroary<long int, ve_math_random> Random;

namespace Trig {

typedef Bindings::CFunction::fUnary<float, float, sinf> Sin;

}
}
}
}

#endif
