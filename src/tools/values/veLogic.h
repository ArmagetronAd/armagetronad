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

#ifndef ARMAGETRON_veLogic_h
#define ARMAGETRON_veLogic_h

#include "values/vCore.h"

namespace vValue {
	namespace Expr {
		namespace Logic {

//! Stores two other values and returns one of them based on a specific condition
class Condition : public Core::Base {
    BasePtr m_condvalue; //!< The conditional value
    BasePtr m_truevalue; //!< The value that is used if the condition is true
    BasePtr m_falsevalue; //!< The value that is used if the condition is false
protected:
    Base const &GetExpr() const; //!< Performs the comparison
public:
    Condition(BasePtr condvalue, BasePtr truevalue, BasePtr falsevalue); //!< Basic constructor
    Condition(Condition const &other); //!< Copy constructor, will not work with any class except another Condition object

    virtual ~Condition() { };
    Base *copy(void) const;

    virtual Variant GetValue() const;
    virtual tString GetString(Base const *other=0) const;
    virtual int GetInt(void) const;
    virtual float GetFloat(void) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

DeclStdUnaryOp(Not)

		}
	}
}

#endif
