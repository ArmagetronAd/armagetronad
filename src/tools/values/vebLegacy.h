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
//! @brief Contains the class declarations for bindings to legacy vValue

#ifndef ARMAGETRON_vebLegacy_h
#define ARMAGETRON_vebLegacy_h

#include "values/vCore.h"

class tConfItemBase;

namespace vValue {
namespace Expr {
namespace Bindings {
namespace Legacy {

//! Stores a function pointer to a function within another class and offers functions to get that function's return value
template<typename T> class Callback : public Base {
public:
    typedef boost::shared_ptr<Base> (T::*cb_ptr)(void); //!< convinience typedef for a callback that can be used with this class
private:
    cb_ptr m_value; //!< Pointer to the function
    T *m_cockpit; //!< Pointer to the object that the function will be called within
public:
    Callback(cb_ptr value, T *hud); //!< Basic constructor
    Callback(Callback const &other); //!< Copy constructor

    void operator=(Callback const &other); //!< overloaded assignment operator
    Base *copy(void) const;
    virtual ~Callback() { };

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual int GetInt(void) const; //!< Returns an integer using the current value
    virtual float GetFloat(void) const; //!< Returns a float using the current value
    virtual tString GetString(Base const *other=0) const; //!< Returns a String using the current value using Output()

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Initializes the Callback object to a given callback function and object
//! @param value the callback to be used
//! @param hud the Cockpit to call the callback from
template<typename T> Callback<T>::Callback(cb_ptr value, T *hud):
        Base(),
        m_value(value),
        m_cockpit(hud)
{ }

//! Initializes the Callback object using the information from another one
//! @param other another Callback object
template<typename T> Callback<T>::Callback(Callback const &other):
        Base(other),
        m_value(other.m_value),
        m_cockpit(other.m_cockpit)
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
template<typename T> Base *Callback<T>::copy(void) const {
    return new Callback(*this);
}

//! Does nothing different than the default assignment operator, but gets rid of a compiler warning
//! @param other the Callback to be copied
template<typename T> void Callback<T>::operator=(Callback const &other) {
    this->Base::operator=(other);
    m_value=other.m_value;
    m_cockpit=other.m_cockpit;
}

//! Returns the result of calling the stored callback
//! @returns a variant containing the current value
template<typename T> Variant Callback<T>::GetValue() const {
    return (m_cockpit->*m_value)()->GetValue();
}

//! Returns the result of calling the stored callback as a string
//! @returns a string containing the current value
template<typename T> tString Callback<T>::GetString(Base const *other) const {
    return (m_cockpit->*m_value)()->GetString(other == 0 ? this : other);
}

//! Returns the result of calling the stored callback as an integer
//! @returns an integer containing the current value
template<typename T> int Callback<T>::GetInt(void) const {
    return (m_cockpit->*m_value)()->GetInt();
}

//! Returns the result of calling the stored callback as a float
//! @returns a float containing the current value
template<typename T> float Callback<T>::GetFloat(void) const {
    return (m_cockpit->*m_value)()->GetFloat();
}

template<typename T> bool Callback<T>::operator==(Base const &other) const { return *(m_cockpit->*m_value)() == other; }
template<typename T> bool Callback<T>::operator!=(Base const &other) const { return *(m_cockpit->*m_value)() != other; }
template<typename T> bool Callback<T>::operator>=(Base const &other) const { return *(m_cockpit->*m_value)() >= other; }
template<typename T> bool Callback<T>::operator<=(Base const &other) const { return *(m_cockpit->*m_value)() <= other; }
template<typename T> bool Callback<T>::operator> (Base const &other) const { return *(m_cockpit->*m_value)() >  other; }
template<typename T> bool Callback<T>::operator< (Base const &other) const { return *(m_cockpit->*m_value)() <  other; }

//TODO: Find a better solution for this, this is a bit slow.

//! stores a pointer to a configuration item and gets the value from it
class ConfItem : public Base {
    tConfItemBase *m_value; //!< pointer to the configuration item that contains the value
    tString Read() const; //!< Reads the value from the configuration item
public:
    ConfItem(tString const &value); //!< Basic constructor
    ConfItem(ConfItem const &other); //!< Copy constructor
    void operator=(ConfItem const &other); //!< overloaded assignment operator

    bool Good() const; //!< Was the configuration item found?

    Base *copy(void) const;
    virtual ~ConfItem() { };

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual tString GetString(Base const *other=0) const;
};

}
}
}
}

#endif
