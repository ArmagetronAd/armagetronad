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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#include "tValue.h"
#include "tConfiguration.h"

namespace tValue {

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

//! This should be overwritten in a derived class if it's sensible to convert the value to an integer
//! @returns 0
int Base::GetInt(void) const {
    return 0;
}

//! This should be overwritten in a derived class if it's sensible to convert the value to a floating- point number
//! @returns 0.0
float Base::GetFloat(void) const {
    return 0.0;
}

//! This should be overwritten in a derived class if it's sensible to convert the value to a string
//! @returns ""
tString Base::GetString(Base const *other) const {
    return tString();
}

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

//! Sets the stored values to the given arguments or dummy values if ommitted
//! @param val the value
//! @param min the minimum value
//! @param max the maximum value
Set::Set(Base *val, Base *min, Base *max) :
        m_min(min),
        m_max(max),
        m_val(val)
{}

//! Copies the values from another class into the newly created one
//! @param other the object to be copied from
Set::Set(Set const &other) :
        m_min(other.m_min->copy()),
        m_max(other.m_max->copy()),
        m_val(other.m_val->copy())
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

//! Constructs a new Condition object with the given parameters
//! @param lvalue     the value to be on the left  side of the comparison
//! @param rvalue     the value to be on the right side of the comparison
//! @param truevalue  the value to be used if the condition is true
//! @param falsevalue the value to be used if the condition is false
//! @param comp       the comparison operator to be used
Condition::Condition(Base *lvalue, Base *rvalue, Base *truevalue, Base *falsevalue, comparator comp) :
        m_lvalue    (lvalue    ),
        m_rvalue    (rvalue    ),
        m_truevalue (truevalue ),
        m_falsevalue(falsevalue)
{
    switch(comp) {
    case lt: m_comparator=&Base::operator< ; break;
    case gt: m_comparator=&Base::operator> ; break;
    case le: m_comparator=&Base::operator<=; break;
    case ge: m_comparator=&Base::operator>=; break;
    case eq: m_comparator=&Base::operator==; break;
    case ne: m_comparator=&Base::operator!=; break;
    default: m_comparator=&Base::operator==; //should never happen
    }
}

//! Initializes the Condition object using the information from another one
//! @param other another Condition object
Condition::Condition(Condition const &other) :
        Base        (other             ),
        m_lvalue    (other.m_lvalue    ->copy()),
        m_rvalue    (other.m_rvalue    ->copy()),
        m_truevalue (other.m_truevalue ->copy()),
        m_falsevalue(other.m_falsevalue->copy()),
        m_comparator(other.m_comparator)
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *Condition::copy(void) const {
    return new Condition(*this);
}

//! Uses a specific type (usually int or float) to perform the comparison
//! @param fun the return value of this function is used for the comparison
//! @returns a reference to either m_lvalue or m_rvalue based on if the condition is true or false
Base const &Condition::GetValue() const {
    return ((*m_lvalue).*m_comparator)(*m_rvalue) ? *m_truevalue : *m_falsevalue;
}

//! Performs the comparison and returns the resulting value
//! @returns the result as an integer
int Condition::GetInt(void) const {
    return GetValue().GetInt();
}

//! Performs the comparison and returns the resulting value
//! @returns the result as a float
float Condition::GetFloat(void) const {
    return GetValue().GetFloat();
}

//! Performs the comparison and returns the resulting value
//! @returns the result as a string
tString Condition::GetString(tValue::Base const *other) const {
    return GetValue().GetString(other);
}

bool Condition::operator==(Base const &other) const { return GetValue() == other; }
bool Condition::operator!=(Base const &other) const { return GetValue() != other; }
bool Condition::operator>=(Base const &other) const { return GetValue() >= other; }
bool Condition::operator<=(Base const &other) const { return GetValue() <= other; }
bool Condition::operator> (Base const &other) const { return GetValue() >  other; }
bool Condition::operator< (Base const &other) const { return GetValue() <  other; }

//! Constructs a new Math object with the given parameters
//! @param lvalue     the value to be on the left  side of the operation
//! @param rvalue     the value to be on the right side of the operation
//! @param comp       the arithmethic operator to be used
Math::Math(Base *lvalue, Base *rvalue, type comp) :
        m_lvalue(lvalue    ),
        m_rvalue(rvalue    ),
        m_type  (comp      )
{ }

//! Initializes the Math object using the information from another one
//! @param other another Math object
Math::Math(Math const &other) :
        Base    (other         ),
        m_lvalue(other.m_lvalue->copy()),
        m_rvalue(other.m_rvalue->copy()),
        m_type  (other.m_type  )
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *Math::copy(void) const {
    return new Math(*this);
}

//! Uses a specific type (usually int or float) to perform the operation
//! @param fun the return value of this function is used for the operation
//! @returns the result
template <typename T> T Math::GetValue(T (Base::*fun)(void) const) const {
    const T lvalue = (m_lvalue.get()->*fun)();
    const T rvalue = (m_rvalue.get()->*fun)();
    switch(m_type) {
    case sum       : return lvalue + rvalue;
    case difference: return lvalue - rvalue;
    case product   : return lvalue * rvalue;
    case quotient  : return rvalue != 0 ? lvalue / rvalue : lvalue;
    default: return lvalue; //should never happen
    }
}

//! Performs the operation and returns the resulting value
//! @returns the result as an integer
int Math::GetInt(void) const {
    return GetValue(&Base::GetInt);
}

//! Performs the operation and returns the resulting value
//! @returns the result as a float
float Math::GetFloat(void) const {
    return GetValue(&Base::GetFloat);
}

//! Performs the operation and returns the resulting value
//! @returns the result as a string
tString Math::GetString(tValue::Base const *other) const {
    return Output(GetValue(&Base::GetFloat));
}

//! Reads from the Configuration item
//! @returns the result as a string
tString ConfItem::Read() const {
    tASSERT(m_value);
    std::ostringstream ret("");
    m_value->WriteVal(ret);
    return(ret.str());
}

//! Searches the right configuration item and uses it if it finds it
//! @param value the name of the configuration item to be searched for
ConfItem::ConfItem(tString const &value):
        Base(),
        m_value(tConfItemBase::FindConfigItem(value))
{ }

//! Initializes the ConfItem object using the information from another one
//! @param other another ConfItem object
ConfItem::ConfItem(ConfItem const &other):
        Base(other),
        m_value(other.m_value)
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *ConfItem::copy(void) const {
    return new ConfItem(*this);
}

//! Does nothing different than the default assignment operator, but gets rid of a compiler warning
//! @param other the ConfItem to be copied
void ConfItem::operator=(ConfItem const &other) {
    this->Base::operator=(other);
    m_value=other.m_value;
}

//! Tests if the search for the right configuration item at construction time was successful
//! @returns true on success, false on failure (read operations will segfault)
bool ConfItem::Good() const {
    return m_value != 0;
}

//! Reads from the configuration item
//! @returns the result as a string
tString ConfItem::GetString(Base const *other) const {
    return Output(Read(), other);
}

//! Reads from the configuration item and attempts conversion to an integer every 100 times it is called
//! @returns the result as an integer
int ConfItem::GetInt(void) const {
    static int lastupdate = 0;
    static int lastvalue;
    if(lastupdate == 0) {
        lastupdate = 100;
        Read().Convert(lastvalue);
    } else {
        lastupdate--;
    }
    return lastvalue;
}

//! Reads from the configuration item and attempts conversion to a float every 100 times it is called
//! @returns the result as a float
float ConfItem::GetFloat(void) const {
    static int lastupdate = 0;
    static float lastvalue;
    if(lastupdate == 0) {
        lastupdate = 100;
        Read().Convert(lastvalue);
    } else {
        lastupdate--;
    }
    return lastvalue;
}

}
