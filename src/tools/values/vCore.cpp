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

#include <boost/lexical_cast.hpp>

#include "vCore.h"

namespace vValue {
namespace Expr {
namespace Core {

//! @param precision the number of digits after the decimal to be used when outputting a string
//! @param minsize   the minimum width when outputting a string
//! @param fill      the character used to fill when outputting a string and the size of the output is smaller than minsize
Base::Base(int precision, int minsize, char fill):
        m_precision(precision),
        m_minsize(minsize),
        m_fill(fill)
{}

//! This will drop all value information of other and create a dummy- class (except if it's called from the parameter initialisation list of a derived class)
//! @param other another Base or derived class
Base::Base(Base const &other) :
        m_precision(other.m_precision),
        m_minsize(other.m_minsize),
        m_fill(other.m_fill)
{ }

//! Sets m_precision to the given value
//! @param precision the number of digits after the decimal to be displayed
void Base::SetPrecision(int precision) {
    m_precision = precision;
}

//! Sets m_minsize to the given value
//! @param minsize the minimal width of the outputted string in # of characters
void Base::SetMinsize(int minsize) {
    m_minsize = minsize;
}

//! Sets m_fill to the given value
//! @param fill the character to be used to fill
void Base::SetFill(char fill) {
    m_fill = fill;
}

Variant
Base::GetValue(void) const {
    //std::cerr << "WARNING: Base::GetValue called!" << std::endl;
    return std::string("");
}
template<> Variant Base::Get<Variant>() const { return GetValue(); }

//! This should be overwritten in a derived class if it's sensible to convert the value to an integer
//! @returns 0
int Base::GetInt(void) const {
    try {
        return boost::lexical_cast<int>(GetValue());
    }
    catch(boost::bad_lexical_cast &)
    {
        return 0;
    }
}
template<> int     Base::Get<int    >() const { return GetInt(); }

//! This should be overwritten in a derived class if it's sensible to convert the value to a floating- point number
//! @returns a stream conversion of the value
float Base::GetFloat(void) const {
    try {
        return boost::lexical_cast<float>(GetValue());
    }
    catch(boost::bad_lexical_cast &)
    {
        return 0.0;
    }
}
template<> float   Base::Get<float  >() const { return GetFloat(); }

//! This should be overwritten in a derived class if it's sensible to convert the value to a string
//! @returns GetValue streamed to a tString
tString Base::GetString(Base const *other) const {
    Variant v = GetValue();
    if (tString *iptr = boost::get<tString>(&v))
            return *iptr;
    return Output(v, other);
}
template<> tString Base::Get<tString>() const { return GetString(); }

//! This should be overwritten in any derived class or they will mysteriously vanish if you use the tValue::Set container, for example
//! @returns a pointer to the newly created copy
Base *Base::copy(void) const {
    return new Base();
}

bool Base::operator==(Base const &other) const { return false; }
bool Base::operator!=(Base const &other) const { return true;  }
bool Base::operator>=(Base const &other) const { return false; }
bool Base::operator<=(Base const &other) const { return false; }
bool Base::operator> (Base const &other) const { return false; }
bool Base::operator< (Base const &other) const { return false; }

}
}
namespace Type {

//! Sets
//! @param val the value
Set::Set() : //!< Constructor using BasePtr
        m_min(new Expr::Base()),
        m_max(new Expr::Base()),
        m_val(new Expr::Base())
{}

//! Sets the stored values to the given arguments
//! @param val the value
Set::Set(BasePtr &val) : //!< Constructor using BasePtr
        m_min(new Expr::Base()),
        m_max(new Expr::Base()),
        m_val(val)
{}

//! Sets the stored values to the given arguments
//! @param val the value
//! @param min the minimum value
//! @param max the maximum value
Set::Set(BasePtr &val, BasePtr &min, BasePtr &max) : //!< Constructor using BasePtr
        m_min(min),
        m_max(max),
        m_val(val)
{}

//! Copies the values from another class into the newly created one
//! @param other the object to be copied from
Set::Set(Set const &other) :
        m_min(other.m_min),
        m_max(other.m_max),
        m_val(other.m_val)
{}

Set::~Set() {
}

//! Erases the current contents and copies the contents of another set into itself.
//! @param other the object to be copied from
void Set::operator=(Set const &other) {
    if(this != &other) {
        m_min = BasePtr(other.GetMin().copy());
        m_max = BasePtr(other.GetMax().copy());
        m_val = BasePtr(other.GetVal().copy());
    }
}

}
namespace Expr {
namespace Core {

//! Stores the given string inside the new class
//! @param value the string to be used
String::String(tString value):
        m_value(value)
{ }

//! Stores the given string inside the new class
//! @param value the string to be used
String::String(char const *value):
        m_value(tString(value))
{ }

//! If the other value is not a String object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
String::String(Base const &other):
        Base(other),
        m_value(other.GetString())
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *String::copy(void) const {
    return new String(*this);
}

Variant String::GetValue() const {
    return GetString();
}

//! Returns the string stored inside the object
//! @returns a string containing the current value
tString String::GetString(Base const *other) const {
    return Output(m_value, other);
}

bool String::operator==(Base const &other) const { return m_value == static_cast<tString>(other); }
bool String::operator!=(Base const &other) const { return m_value !=  static_cast<tString>(other); }
bool String::operator>=(Base const &other) const { return m_value >=  static_cast<tString>(other); }
bool String::operator<=(Base const &other) const { return m_value <=  static_cast<tString>(other); }
bool String::operator> (Base const &other) const { return m_value >   static_cast<tString>(other); }
bool String::operator< (Base const &other) const { return m_value <   static_cast<tString>(other); }


//! Constructs a new binary operator object with the given parameters
//! @param value      the value for the operation
UnaryOp::UnaryOp(BasePtr value) :
        m_value(value    )
{ }

//! Initializes the binary operator object using the information from another one
//! @param other another binary operator object
UnaryOp::UnaryOp(UnaryOp const &other) :
        Base (other         ),
        m_value(other.m_value->copy())
{ }

Base *UnaryOp::copy(void) const {
    return new UnaryOp(*this);
}

//! Constructs a new binary operator object with the given parameters
//! @param lvalue     the value to be on the left  side of the operation
//! @param rvalue     the value to be on the right side of the operation
BinaryOp::BinaryOp(BasePtr lvalue, BasePtr rvalue) :
        m_lvalue(lvalue    ),
        m_rvalue(rvalue    )
{ }

//! Initializes the binary operator object using the information from another one
//! @param other another binary operator object
BinaryOp::BinaryOp(BinaryOp const &other) :
        Base (other         ),
        m_lvalue(other.m_lvalue->copy()),
        m_rvalue(other.m_rvalue->copy())
{ }

Base *BinaryOp::copy(void) const {
    return new BinaryOp(*this);
}

}
}
}


