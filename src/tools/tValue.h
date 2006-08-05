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
//! @brief Contains the class declarations for all tValue members

#ifndef ARMAGETRON_VALUE_H
#define ARMAGETRON_VALUE_H

#include "tString.h"
#include <memory>
#include <map>
#include <iomanip>
#include <set>
#include <iterator>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
class tConfItemBase;

//! Namespace for all Value classes for use by the cockpit
namespace tValue {

class Base;
class FooPtrOps;
typedef boost::shared_ptr<Base> BasePtr; //!< convinience definition for the use in derived classes

typedef boost::variant<int, float, std::string> Variant;
typedef std::set<BasePtr, FooPtrOps> myCol;

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

struct FooPtrOps
{
    bool operator()( const BasePtr & a, const BasePtr & b )
    {
        return (*a < *b);
    }
    /*
     * Usefull for debug purpose
     */
    /*
      void operator()( const BasePtr & a )
      { std::cout << a->GetInt() << "\n"; }
    */
};

class BaseExt: public Base {
public:
    BaseExt() :Base() {};
    BaseExt(Base const &other): Base(other) {};
    BaseExt(BaseExt const &other): Base(other) {};
    virtual ~BaseExt() { };

    virtual Base *copy(void) const {return new BaseExt();};

    //! Convert the stored data into a collection.
    //! @returns a collection of at least 1.
    virtual myCol GetCol(void) const ;
    operator myCol() const { return GetCol(); }
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
    Base const &GetMin(void) const {return *m_min;} //!< Returns a reference to the stored minimum value
    Base const &GetMax(void) const {return *m_max;} //!< Returns a reference to the stored maximum value
    Base const &GetVal(void) const {return *m_val;} //!< Returns a reference to the stored current value
};

//! Basic number class
//!
//! This should be used for types that can be converted to int and float and inserted into an istream
template<typename T> class Number : public BaseExt {
    T m_value; //!< The stored value
public:
    Number(T value); //!< Constructor
    Number(Base const &other); //!< Copy constructor
    Number(BaseExt const &other); //!< Copy constructor
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
        BaseExt(),
        m_value(value)
{ }

//! If the other value is not a Number object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
template<typename T> Number<T>::Number(Base const &other):
        BaseExt(other), // TODO: check if Base or BaseExt goes here
        m_value(static_cast<T>(other))
{ }

//! If the other value is not a Number object this constructor will only copy the current value and drop all other information.
//! @param other another Base or derived class to copy from
template<typename T> Number<T>::Number(BaseExt const &other):
        BaseExt(other),
        m_value(static_cast<T>(other))
{ }

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
class String : public BaseExt {
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

}
class ROperation;
namespace tValue {

//! Stores a math expression using the mathexpr library
class Expr : public BaseExt {
public:
    typedef std::map<tString, float *> varmap_t; //!< map of variable names and their references
    typedef std::map<tString, float ((*)(float))> funcmap_t; //!< map of function names and their references
private:
    boost::shared_ptr<ROperation> m_operation;
public:
    Expr(tString const &expr); //!< Constructs a new Expr without variables and functions and the like
    Expr(tString const &expr, varmap_t const &vars, funcmap_t const &functions); //!< Constructs a new Expr with an array of variables and functions

    Base *copy(void) const;

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
};

//! Stores a function pointer to a function within another class and offers functions to get that function's return value
template<typename T> class Callback : public BaseExt {
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

//! Stores two other values and returns one of them based on a specific condition
class Condition : public BaseExt {
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

//! Base class for unary operators
class UnaryOp : public BaseExt {
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

DeclStdUnaryOp(Not)

//! Base class for binary operators
class BinaryOp : public BaseExt {
protected:
    BasePtr m_lvalue; //!< The lvalue for the operation
    BasePtr m_rvalue; //!< The rvalue for the operation
public:
    BinaryOp(BasePtr lvalue, BasePtr rvalue); //!< Basic constructor
    BinaryOp(BinaryOp const &other); //!< Copy constructor
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};

class Add : public BinaryOp {
public:
    Add(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Add(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Subtract : public BinaryOp {
public:
    Subtract(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Subtract(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Multiply : public BinaryOp {
public:
    Multiply(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Multiply(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual Base *copy(void) const; //!< Returns an exact copy of this object
};
class Divide : public BinaryOp {
public:
    Divide(BasePtr lvalue, BasePtr rvalue) : BinaryOp(lvalue, rvalue) {}; //!< Basic constructor
    Divide(BinaryOp const &other) : BinaryOp(other) {}; //!< Copy constructor

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
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

DeclStdBinaryOp(GreaterThan    )
DeclStdBinaryOp(GreaterOrEquals)
DeclStdBinaryOp(         Equals)
DeclStdBinaryOp(   LessOrEquals)
DeclStdBinaryOp(   LessThan    )
DeclStdBinaryOp(Compare)

//TODO: Find a better solution for this, this is a bit slow.

//! stores a pointer to a configuration item and gets the value from it
class ConfItem : public BaseExt {
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

//! Initializes the Callback object to a given callback function and object
//! @param value the callback to be used
//! @param hud the Cockpit to call the callback from
template<typename T> Callback<T>::Callback(cb_ptr value, T *hud):
        BaseExt(),
        m_value(value),
        m_cockpit(hud)
{ }

//! Initializes the Callback object using the information from another one
//! @param other another Callback object
template<typename T> Callback<T>::Callback(Callback const &other):
        BaseExt(other),
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
    this->BaseExt::operator=(other);
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


class ColNode : public BaseExt {
protected:
    myCol m_col;

public:
    template <typename InputIterator>
    ColNode(InputIterator begin, InputIterator end): BaseExt(), m_col(begin, end) { }

    //!< Specialised constructor from iterators

    ColNode(BaseExt const &other): BaseExt(other), m_col() { };
    ColNode(ColNode const &other): BaseExt(other), m_col(other.m_col) { };
    virtual ~ColNode() { };

    Base *copy(void) const { return new ColNode(*this); };
    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual int GetInt(void) const;
    virtual float GetFloat(void) const;
    myCol GetCol(void) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
};


/*
  ColUnary holds one BaseExt object.
  Any query toward the underlying BaseExt object are done only as collection.
  
  For each operation, ColUnary and its childs will query the BaseExt object for 
  its collection of tValue, possibly do a manipulation on the collection and
  return the associated result.
  
  It is to be noted that GetInt and GetFloat will return the value of the 
  first element of the resulting collection, if not otherly overridden.
  
  It is to be noted that there are no guaranties that 2 consecutive queries 
  will provide the same results. Any subsequent query should will be considered 
  as a new one, possibly affecting the state of the data. It is recommended to 
  store the data returned for as long as its current state is relevant.
*/
class ColUnary : public BaseExt {
protected:
    BasePtr m_child;
public:
    ColUnary(BasePtr child) : BaseExt(), m_child(child) { };

    ColUnary(const ColUnary &other): BaseExt(), m_child(other.m_child->copy()) { };
    virtual ~ColUnary() { };

    Base *copy(void) const { return new ColUnary(*this); };

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual int GetInt(void) const;
    virtual float GetFloat(void) const;
    myCol GetCol(void) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
protected:
    virtual myCol _operation(void) const {
        //      return m_child->GetCol();
        Base *base = m_child->copy();
        BaseExt const *baseExt = dynamic_cast<BaseExt*>(base);
        return baseExt->GetCol();
    };
};


/*
  After querying the child BaseExt object contained for its collection, 
  PickOneCol will select a random element and return only this one
*/
class ColPickOne : public ColUnary {
public:
    ColPickOne(BasePtr child = BasePtr( new Base() ) ) : ColUnary(child) { };

    ColPickOne(const ColPickOne &other): ColUnary(other) { };
    virtual ~ColPickOne() { };

    Base *copy(void) const { return new ColPickOne(*this); };

protected:
    virtual myCol _operation(void) const ;
};


/*
   Set
   Intersection: Elements that are both on A and B
   Difference  : Elements that are in A and not B
   Union       :
   Symetric Difference: Elements that are either in A or B, but not in both
*/

/*
  The ColBinary and its children class operates on 2 tValue. The exact manipulation is determined by the child class
  
  It is to be noted that there are no guaranties that 2 consecutive queries 
  will provide the same results. Any subsequent query should will be considered 
  as a new one, possibly affecting the state of the data. It is recommended to 
  store the data returned for as long as its current state is relevant.
*/
class ColBinary : public BaseExt {
protected:
    BasePtr r_child;
    BasePtr l_child;

public:
    ColBinary(BaseExt *r_value = new BaseExt,
              BaseExt *l_value = new BaseExt ) :
            r_child(r_value),
            l_child(l_value)
    { };
    ColBinary(BasePtr &r_value,
              BasePtr &l_value):
            r_child(r_value),
            l_child(l_value)
    { };
    ColBinary(const ColBinary &other) :
            BaseExt(other),
            r_child(other.r_child->copy()),
            l_child(other.l_child->copy())
    { };
    virtual ~ColBinary() { };

    Base *copy(void) const { return new ColBinary(*this); };

    virtual Variant GetValue(void) const; //!< Returns the value in its native format
    virtual int GetInt(void) const;
    virtual float GetFloat(void) const;
    myCol GetCol(void) const;

    bool operator==(Base const &other) const;
    bool operator!=(Base const &other) const;
    bool operator>=(Base const &other) const;
    bool operator<=(Base const &other) const;
    bool operator> (Base const &other) const;
    bool operator< (Base const &other) const;
protected:
    virtual myCol _operation(void) const;
};



class ColUnion : public ColBinary {
public:
    ColUnion(BaseExt *r_value = new BaseExt,
             BaseExt *l_value = new BaseExt ) :
            ColBinary(r_value, l_value)
    { };
    ColUnion(BasePtr &r_value,
             BasePtr &l_value):
    ColBinary(r_value, l_value) { };
    ColUnion(const ColBinary &other) :
    ColBinary(other) { };
    virtual ~ColUnion() { };

    Base *copy(void) const { return new ColUnion(*this); };

protected:
    myCol _operation(void) const;
};


class ColIntersection : public ColBinary {

public:
    ColIntersection(BaseExt *r_value = new BaseExt,
                    BaseExt *l_value = new BaseExt ) :
            ColBinary(r_value, l_value)
    { };
    ColIntersection(BasePtr &r_value,
                    BasePtr &l_value):
    ColBinary(r_value, l_value) { };
    ColIntersection(const ColBinary &other) :
    ColBinary(other) { };
    virtual ~ColIntersection() { };

    Base *copy(void) const { return new ColIntersection(*this); };

protected:
    myCol _operation(void) const;
};



class ColDifference : public ColBinary {
public:
    ColDifference(BaseExt *r_value = new BaseExt,
                  BaseExt *l_value = new BaseExt ) :
            ColBinary(r_value, l_value)
    { };
    ColDifference(BasePtr &r_value,
                  BasePtr &l_value):
    ColBinary(r_value, l_value) { };
    ColDifference(const ColBinary &other) :
    ColBinary(other) { };
    virtual ~ColDifference() { };

    Base *copy(void) const { return new ColDifference(*this); };

protected:
    myCol _operation(void) const;
};



}

#endif
