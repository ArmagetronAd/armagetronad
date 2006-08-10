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
//! @brief Contains the class declarations for mathexpr bindings

#ifndef ARMAGETRON_vebMathExpr_h
#define ARMAGETRON_vebMathExpr_h

#include <map>

#include "values/vCore.h"

class ROperation;
class RVar;
class RFunction;

namespace vValue {
namespace Expr {
namespace Bindings {

//! Stores a math expression using the mathexpr library
class MathExpr : public Base {
public:
    typedef std::map<tString, float *> varmap_t; //!< map of variable names and their references
    typedef std::map<tString, float ((*)(float))> funcmap_t; //!< map of function names and their references
private:
    boost::shared_ptr<ROperation> m_operation;
    RVar **m_vararray;
    RFunction **m_funcarray;
    int m_varsSize, m_functionsSize;
public:
    MathExpr(tString const &expr); //!< Constructs a new MathExpr without variables and functions and the like
    MathExpr(tString const &expr, varmap_t const &vars, funcmap_t const &functions); //!< Constructs a new MathExpr with an array of variables and functions

    Base *copy(void) const;
    ~MathExpr(void);

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
};

}
}
}

#endif
