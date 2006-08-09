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
//! @brief Contains the class declarations for direct C binding templates

#ifndef ARMAGETRON_vBindings_CFunction_h
#define ARMAGETRON_vBindings_CFunction_h

#include "values/vCore.h"

namespace vValue {
	namespace Expr {
		namespace Bindings {
			namespace CFunction {

template<typename T, T F(void)> class fZeroary : public Base {
public:
    fZeroary() { };
    virtual Variant GetValue() const { return F(); };
};
template<typename T, typename Aa, /* Aa (Base::*GAa)(void) const, */ T F(Aa)> class fUnary : public Base {
    BasePtr m_ArgA;
public:
    fUnary(BasePtr ArgA): m_ArgA(ArgA) { }
    virtual Variant GetValue() const { return F(m_ArgA->Get<Aa>()); };
};
template<typename T, typename Aa, typename Ab, T F(Aa, Ab)> class fBinary : public Base {
    BasePtr m_ArgA, m_ArgB;
public:
    fBinary(BasePtr ArgA, BasePtr ArgB): m_ArgA(ArgA), m_ArgB(ArgB) { }
    virtual Variant GetValue() const { return F(m_ArgA->Get<Aa>(), m_ArgB->Get<Ab>()); };
};

			}
		}
	}
}

#endif
