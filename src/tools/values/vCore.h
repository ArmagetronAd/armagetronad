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
//! @brief Contains the class declarations for core vValue members

#ifndef ARMAGETRON_vCore_h
#define ARMAGETRON_vCore_h

#include "tString.h"
#include <deque>
#include <iomanip>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

namespace vValue {
	namespace Expr {
		namespace Core {
class Base;
		}
	}
	namespace Type {
		typedef boost::shared_ptr<Expr::Core::Base> BasePtr; //!< convinience definition for the use in derived classes

typedef boost::variant<int, float, std::string> Variant;
typedef std::deque<BasePtr> arglist;
	}
	using namespace Type;

	namespace Expr {
		namespace Core {
//! Offers basic functions to set the precision etc.
class Base {
    int m_precision; //!< The precision when returning a string
    int m_minsize;   //!< The minimum width when returning a string
    char m_fill;     //!< The fill character that's used when the contents are too small to fill m_minsize
protected:
    //! Converts a value to a string using the settings like precision etc.
    template<typename T> inline tString Output(T value, Base const *other=0) const;
    //template<typename T> inline T VariantConvert(Variant const) const;
public:
    Base(int precision=1, int minsize=0, char fill='0'); //!< Default constructor
    Base(Base const &other); //!< Copy constructor
    virtual ~Base() { };

    virtual Base *copy(void) const; //!< Returns an exact copy of this object

    virtual int GetInt(void) const; //!< Returns an integer using the current value
    virtual float GetFloat(void) const; //!< Returns a float using the current value
    virtual tString GetString(Base const *other=0) const; //!< Returns a String using the current value using Output()
    virtual Variant GetValue(void) const; //!< Returns the value in its native format

    template<class T> T Get() const;

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

/*
template<typename T> inline T Base::VariantConvert(Variant v) const {
	if (T *ptr = boost::get<T>(v))
		return *ptr;
	std::stringstream stream("");
	stream << v;
	T i;
	stream >> i;
    return i;
}
*/

		}
	}
	namespace Type {

//! min, max and value in one class which handles the pointer buisness
class Set {
    BasePtr m_min; //!< The minimum value
    BasePtr m_max; //!< The maxumim value
    BasePtr m_val; //!< The current value
public:
    /*
    Set(Base *val = new Base,
        Base *min = new Base,
        Base *max = new Base); //!< Default constructor
    */
    Set(); //!< Constructor using std::auto_ptr
    Set(BasePtr &val); //!< Constructor using std::auto_ptr
    Set(BasePtr &val, BasePtr &min, BasePtr &max); //!< Constructor using std::auto_ptr
    //    Set(BasePtr &val = BasePtr(), BasePtr &min = BasePtr(), BasePtr &max = BasePtr()); //!< Constructor using std::auto_ptr
    Set(const Set &other); //!< Copy constructor
    ~Set(); //!< Destructor
    void operator=(Set const &other); //!< Overloaded assignment operator
    Expr::Core::
    Base const &GetMin(void) const {return *m_min;} //!< Returns a reference to the stored minimum value
    Expr::Core::
    Base const &GetMax(void) const {return *m_max;} //!< Returns a reference to the stored maximum value
    Expr::Core::
    Base const &GetVal(void) const {return *m_val;} //!< Returns a reference to the stored current value
};
	}
	namespace Expr {
		namespace Core {

//! Basic number class
//!
//! This should be used for types that can be converted to int and float and inserted into an istream
template<typename T> class Number : public Base {
    T m_value; //!< The stored value
public:
    Number(T value); //!< Constructor
    Number(Base const &other); //!< Copy constructor
    /*
    Number(BaseExt const &other); //!< Copy constructor
    */
    virtual ~Number() { };

    Base *copy(void) const;

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

//! Stores the given value inside the new class.
//! @param value the value to be used
template<typename T> Number<T>::Number(T value):
        Base(),
        m_value(value)
{ }

//! If the other value is not a Number object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
template<typename T> Number<T>::Number(Base const &other):
        Base(other), // TODO: check if Base or BaseExt goes here
        m_value(static_cast<T>(other))
{ }

/*
//! If the other value is not a Number object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
template<typename T> Number<T>::Number(BaseExt const &other):
        BaseExt(other),
        m_value(static_cast<T>(other))
{ }
*/

//! Overwritten copy function
//! @returns a new copy of the current object
// TODO: Changed from Number<T> * to Base*
template<typename T> Base *Number<T>::copy(void) const {
    return new Number(*this);
}

//! Returns the value stored inside the object
//! @returns a variant containing the current value
template<typename T> Variant Number<T>::GetValue(void) const {
    return static_cast<T>(m_value);
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

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual tString GetString(Base const *other=0) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};

//! Base class for unary operators
class UnaryOp : public Base {
protected:
    BasePtr m_value; //!< The value for the operation
public:
    UnaryOp(BasePtr value); //!< Basic constructor
    UnaryOp(UnaryOp const &other); //!< Copy constructor
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};

#define DeclStdUnaryOp(classname)	\
class classname : public UnaryOp {	\
public:	\
    classname(BasePtr value) : UnaryOp(value) {}; \
    classname(UnaryOp const &other) : UnaryOp(other) {}; \
    virtual Variant GetValue(void) const; \
    virtual Base *copy(void) const; \
};

//! Base class for binary operators
class BinaryOp : public Base {
protected:
    BasePtr m_lvalue; //!< The lvalue for the operation
    BasePtr m_rvalue; //!< The rvalue for the operation
public:
    BinaryOp(BasePtr lvalue, BasePtr rvalue); //!< Basic constructor
    BinaryOp(BinaryOp const &other); //!< Copy constructor
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};

#define DeclStdBinaryOp(classname)	\
class classname : public BinaryOp {	\
public:	\
    classname(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; \
    classname(BinaryOp const &other) : BinaryOp(other) {}; \
    virtual Variant GetValue(void) const; \
    virtual Base *copy(void) const; \
};

		}
		using namespace Core;
	}

template<typename C> class Creator {
public:
    static Expr::Base *create() {
        return new C();
    }
    template<typename T> static Expr::Base *create(T t) {
        return new C(t);
    }
    template<typename T, typename U> static Expr::Base *create(T t, U u) {
        return new C(t, u);
    }
    template<typename T, typename U, typename V> static Expr::Base *create(T t, U u, V v) {
        return new C(t, u, v);
    }
    template<typename T, typename U, typename V, typename W> static Expr::Base *create(T t, U u, V v, W w) {
        return new C(t, u, v, w);
    }
    template<typename T, typename U, typename V, typename W, typename X> static Expr::Base *create(T t, U u, V v, W w, X x) {
        return new C(t, u, v, w, x);
    }
};


	using namespace Type;
}

#endif
