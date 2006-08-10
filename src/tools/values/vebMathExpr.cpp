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

#include "vCore.h"
#include "vebMathExpr.h"

#include "mathexpr.h"

namespace vValue {
namespace Expr {
namespace Bindings {

//! @param expr The expression to be parsed
MathExpr::MathExpr(tString const &expr) : m_operation(new ROperation(expr.c_str())), m_vararray(0), m_funcarray(0), m_varsSize(0), m_functionsSize(0) {}

//! @param expr The expression to be parsed
//! @param vars A map of variable names and their references
//! @param functions A map of function names and their references
MathExpr::MathExpr(tString const &expr, varmap_t const &vars, funcmap_t const &functions) {
    // what a mess. why can't this darn library just use stl functions? :s
    m_vararray = new RVar*[vars.size()];
    unsigned int i = 0;
    for(varmap_t::const_iterator iter = vars.begin(); iter != vars.end(); ++iter, ++i) {
        m_vararray[i] = new RVar(iter->first.c_str(), iter->second);
    }
    m_funcarray = new RFunction*[functions.size()];
    unsigned int j = 0;
    m_varsSize = vars.size();
    m_functionsSize = functions.size();
    for(funcmap_t::const_iterator iter = functions.begin(); iter != functions.end(); ++iter, ++j) {
        m_funcarray[j] = new RFunction(iter->second);
        m_funcarray[j]->SetName(iter->first.c_str());
    }
    m_operation = boost::shared_ptr<ROperation>(new ROperation(expr.c_str(), vars.size(), m_vararray, functions.size(), m_funcarray));
}

MathExpr::~MathExpr() {
    if(m_vararray != 0) {
        for(int i = 0; i < m_varsSize; i++) {
            delete m_vararray[i];
        }
        delete[] m_vararray;
    }
    if(m_funcarray != 0) {
        for(int i = 0; i < m_functionsSize; i++) {
            delete m_funcarray[i];
        }
        delete[] m_funcarray;
    }
}

Base *MathExpr::copy(void) const {
    return new MathExpr(*this);
}

Variant MathExpr::GetValue() const {
    return m_operation->Val();
}

//class blah {
//public:
//    blah() {
//        float x=4,y=3;
//		vValue::Bindings::MathExpr::varmap_t vars;
//		vars[tString("x")] = &x;
//		vars[tString("y")] = &y;
//		vValue::Bindings::MathExpr::funcmap_t functions;
//		functions[tString("f")] = &sinf;
//		vValue::Bindings::MathExpr expr(tString("x+f(y)"), vars, functions);
//        std::cerr << expr.GetFloat() << std::endl;
//    }
//};
//blah asdfgsf;

}
}
}
