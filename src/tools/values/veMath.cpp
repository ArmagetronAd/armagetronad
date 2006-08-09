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
#include "vRegistry.h"
#include "vebCFunction.h"
#include "veMath.h"

using namespace vValue::Registry;

namespace vValue {
	namespace Expr {
		namespace Math {

//! Returns the result of adding the lvalue and rvalue
//! @returns the result
Variant
Add::GetValue(void) const {
    const Variant lvalue = m_lvalue->GetValue();
    const Variant rvalue = m_rvalue->GetValue();
    //return boost::apply_visitor(AddVisitor(), lvalue, rvalue);
    /*
    	if (boost::get<tString>(&lvalue) || boost::get<tString>(&rvalue))
    		return boost::lexical_cast<tString>lvalue
    		     + boost::lexical_cast<tString>rvalue;
    	else
    */
    if (boost::get<int>(&lvalue) && boost::get<int>(&rvalue))
                return boost::get<int>(lvalue) + boost::get<int>(rvalue);
    /*
    else
    if (boost::get<float>(&lvalue) || boost::get<float>(&rvalue))
    	return boost::lexical_cast<float>(lvalue)
    	     + boost::lexical_cast<float>(rvalue);
    else
    	throw(1);*/
    return m_lvalue->GetFloat() + m_rvalue->GetFloat();
}

Base *Add::copy(void) const {
    return new Add(*this);
}

//! Returns the result of subtracting rvalue from lvalue
//! @returns the result
Variant
Subtract::GetValue(void) const {
    const Variant lvalue = m_lvalue->GetValue();
    const Variant rvalue = m_rvalue->GetValue();
    if (boost::get<int>(&lvalue) && boost::get<int>(&rvalue))
                return boost::get<int>(lvalue) - boost::get<int>(rvalue);
    return m_lvalue->GetFloat() - m_rvalue->GetFloat();
    /*
    if (boost::get<float>(&lvalue) || boost::get<float>(&rvalue))
    	return boost::lexical_cast<float>(lvalue)
    	     - boost::lexical_cast<float>(rvalue);
    else
    if (boost::get<int>(&lvalue) && boost::get<int>(&rvalue))
    	return boost::get<int>(lvalue) - boost::get<int>(rvalue);
    else
    	throw(1);
    */
}

Base *Subtract::copy(void) const {
    return new Subtract(*this);
}

//! Returns the result of multiplying lvalue by rvalue
//! @returns the result
Variant
Multiply::GetValue(void) const {
    const Variant lvalue = m_lvalue->GetValue();
    const Variant rvalue = m_rvalue->GetValue();
    if (boost::get<int>(&lvalue) && boost::get<int>(&rvalue))
                return boost::get<int>(lvalue) * boost::get<int>(rvalue);
    return m_lvalue->GetFloat() * m_rvalue->GetFloat();
}

Base *Multiply::copy(void) const {
    return new Multiply(*this);
}

//! Returns the result of dividing lvalue by rvalue
//! @returns the result
Variant
Divide::GetValue(void) const {
    // Operate on both values as floats, because this is division
    return m_lvalue->GetFloat() / m_rvalue->GetFloat();
}

Base *Divide::copy(void) const {
    return new Divide(*this);
}


Registration register_sin("func\nmath", "sin", 1, (Registration::fptr)
                          ( ctor::a1* )& Creator<Trig::Sin>::create<BasePtr> );


		}
	}
}
