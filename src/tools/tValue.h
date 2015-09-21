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

//! @file
//! @brief Contains the class declarations for all tValue members

#ifndef ARMAGETRON_VALUE_H
#define ARMAGETRON_VALUE_H

#include "tString.h"
#include <memory>
#include <iomanip>
class tConfItemBase;

//! Namespace for all Value classes for use by the cockpit
namespace tValue {

//! Offers basic functions to set the precision etc.
class Base {
    int m_precision; //!< The precision when returning a string
    int m_minsize;   //!< The minimum width when returning a string
    char m_fill;     //!< The fill character that's used when the contents are too small to fill m_minsize
protected:
    //! Converts a value to a string using the settings like precision etc.
    template<typename T> inline tString Output(T value, Base const *other=0) const;
public:
    Base(int precision=1, int minsize=0, char fill='0'); //!< Default constructor
    Base(Base const &other); //!< Copy constructor
    virtual ~Base() { };

    virtual Base *copy(void) const; //!< Returns an exact copy of this object

    virtual int GetInt(void) const; //!< Returns an integer using the current value
    virtual float GetFloat(void) const; //!< Returns a float using the current value
    virtual tString GetString(Base const *other=0) const; //!< Returns a String using the current value using Output()

    void SetPrecision(int precision); //!< Sets the precision when outputting a string
    void SetMinsize(int size); //!< Sets the minimal width when outputting a string
    void SetFill(char fill); //!< Sets the fill character when outputting a string shorter than the minimal width

    operator int() const { return GetInt(); }
    operator float() const { return GetFloat(); }
    operator tString() const { return GetString(); }

    virtual bool operator==(Base const &other) const; //!< compares two values
    virtual bool operator!=(Base const &other) const; //!< compares two values
    virtual bool operator>=(Base const &other) const; //!< compares two values
    virtual bool operator<=(Base const &other) const; //!< compares two values
    virtual bool operator> (Base const &other) const; //!< compares two values
    virtual bool operator< (Base const &other) const; //!< compares two values
};

//! Returns a string using the settings from this or another object.
//!
//! This function is inline even though its size since it gets called extemely often from the derived classes.
//!
//! @param value the value to be converted and returned
//! @param other pointer to the class that contains the settings, if it is 0 the settings will be taken from the class the function was called for
//! @returns the resulting string
template<typename T> inline tString Base::Output(T value, Base const *other) const {
    if (other==0) other=this;
    std::ostringstream buf("");
    buf << std::setfill(other->m_fill) << std::setw(other->m_minsize) << std::fixed << std::setprecision(other->m_precision) << value; return buf.str();
}

typedef std::auto_ptr<Base> BasePtr; //!< convinience definition for the use in derived classes

//! min, max and value in one class which handles the pointer buisness
class Set {
    BasePtr m_min; //!< The minimum value
    BasePtr m_max; //!< The maxumim value
    BasePtr m_val; //!< The current value
public:
    Set(Base *val = new Base,
        Base *min = new Base,
        Base *max = new Base); //!< Default constructor
    Set(BasePtr &val, BasePtr &min, BasePtr &max); //!< Constructor using std::auto_ptr
    Set(const Set &other); //!< Copy constructor
    ~Set(); //!< Destructor
    void operator=(Set const &other); //!< Overloaded assignment operator
    Base const &GetMin(void) const {return *m_min;} //!< Returns a reference to the stored minimum value
    Base const &GetMax(void) const {return *m_max;} //!< Returns a reference to the stored maximum value
    Base const &GetVal(void) const {return *m_val;} //!< Returns a reference to the stored current value
};

//! Basic number class
//!
//! This should be used for types that can be converted to int and float and inserted into an istream
template<typename T> class Number : public Base {
    T m_value; //!< The stored value
public:
    Number(T value); //!< Constructor
    Number(Base const &other); //!< Copy constructor
    virtual ~Number() { };

    Number *copy(void) const;

    int GetInt(void) const;
    float GetFloat(void) const;
    tString GetString(Base const *other=0) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Stores the given value inside the new class.
//! @param value the value to be used
template<typename T> Number<T>::Number(T value):
        Base(),
        m_value(value)
{ }

//! If the other value is not a Number object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
template<typename T> Number<T>::Number(Base const &other):
        Base(other),
        m_value(static_cast<T>(other))
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
template<typename T> Number<T> *Number<T>::copy(void) const {
    return new Number(*this);
}

//! Returns the value stored inside the object
//! @returns an integer containing the current value
template<typename T> int Number<T>::GetInt(void) const {
    return static_cast<int>(m_value);
}

//! Returns the value stored inside the object, converted to a float
//! @returns a float containing the current value
template<typename T> float Number<T>::GetFloat(void) const {
    return static_cast<float>(m_value);
}

//! Calls Output() to convert the stored value to a string
//! @returns the resulting string
template<typename T> tString Number<T>::GetString(Base const *other) const {
    return Output(m_value, other);
}

template<typename T> bool Number<T>::operator==(Base const &other) const {
    return m_value == static_cast<T>(other);
}
template<typename T> bool Number<T>::operator!=(Base const &other) const {
    return m_value != static_cast<T>(other);
}
template<typename T> bool Number<T>::operator>=(Base const &other) const {
    return m_value >= static_cast<T>(other);
}
template<typename T> bool Number<T>::operator<=(Base const &other) const {
    return m_value <= static_cast<T>(other);
}
template<typename T> bool Number<T>::operator> (Base const &other) const {
    return m_value > static_cast<T>(other);
}
template<typename T> bool Number<T>::operator< (Base const &other) const {
    return m_value < static_cast<T>(other);
}

typedef Number<float> Float;
typedef Number<int> Int;

//! Stores a string (tString) value
class String : public Base {
    tString m_value; //!< The stored value
public:
    String(tString value); //!< Constructor
    String(char const *value); //!< Constructor
    String(Base const &other); //!< Copy constructor

    Base *copy(void) const;
    virtual ~String() { };

    tString GetString(Base const *other=0) const;
    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Stores a function pointer to a function within another class and offers functions to get that function's return value
template<typename T> class Callback : public Base {
public:
    typedef std::auto_ptr<Base> (T::*cb_ptr)(void); //!< convinience typedef for a callback that can be used with this class
private:
    cb_ptr m_value; //!< Pointer to the function
    T *m_cockpit; //!< Pointer to the object that the function will be called within
public:
    Callback(cb_ptr value, T *hud); //!< Basic constructor
    Callback(Callback const &other); //!< Copy constructor

    void operator=(Callback const &other); //!< overloaded assignment operator
    Base *copy(void) const;
    virtual ~Callback() { };

    tString GetString(Base const *other=0) const;
    int GetInt(void) const;
    float GetFloat(void) const;
    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Stores two other values and returns one of them based on a specific condition
class Condition : public Base {
    BasePtr m_lvalue; //!< The lvalue for the comparison
    BasePtr m_rvalue; //!< The rvalue for the comparison
    BasePtr m_truevalue; //!< The value that is used if the condition is true
    BasePtr m_falsevalue; //!< The value that is used if the condition is false
    bool (Base::*m_comparator)(Base const &) const; //!< The comparison operator used
public:
    //! The different binary comparison operators available
    enum comparator {
        gt, lt, ge, le, eq, ne
    };
private:
    Base const &GetValue() const; //!< Performs the comparison
public:
    Condition(Base *lvalue, Base *rvalue, Base *truevalue, Base *falsevalue, comparator comp); //!< Basic constructor
    Condition(Condition const &other); //!< Copy constructor, will not work with any class except another Condition object

    virtual ~Condition() { };
    Base *copy(void) const;

    tString GetString(Base const *other=0) const;
    int GetInt(void) const;
    float GetFloat(void) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Supports basic arithmetical operations between two values and returns the result
class Math : public Base {
    BasePtr m_lvalue; //!< The lvalue for the operation
    BasePtr m_rvalue; //!< The rvalue for the operation
public:
    enum type { //!< The operators available
        sum, difference, product, quotient
    };
private:
    type m_type; //!< The operation to be performed
    template<typename T> T GetValue(T (Base::*fun)(void) const) const; //!< Performs the operation
public:
    Math(Base *lvalue, Base *rvalue, type comp); //!< Basic constructor
    Math(Math const &other); //!< Copy constructor

    virtual ~Math() { };
    Base *copy(void) const;

    tString GetString(Base const *other=0) const;
    int GetInt(void) const;
    float GetFloat(void) const;
};

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

    tString GetString(Base const *other=0) const;
    int GetInt(void) const;
    float GetFloat(void) const;
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

}

#endif
