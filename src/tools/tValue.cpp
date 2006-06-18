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

myCol BaseExt::GetCol(void) const {
  myCol col;
  BasePtr node(this->copy());
  col.insert(node);
  return col;
}

//! Sets 
//! @param val the value
Set::Set() : //!< Constructor using BasePtr
        m_min(new Base()),
        m_max(new Base()),
        m_val(new Base())
{}

//! Sets the stored values to the given arguments 
//! @param val the value
Set::Set(BasePtr &val) : //!< Constructor using BasePtr
        m_min(new Base()),
        m_max(new Base()),
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
        BaseExt(other),
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
Condition::Condition(BasePtr lvalue, BasePtr rvalue, BasePtr truevalue, BasePtr falsevalue, comparator comp) :
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
        BaseExt     (other             ),
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
Math::Math(BasePtr lvalue, BasePtr rvalue, type comp) :
        m_lvalue(lvalue    ),
        m_rvalue(rvalue    ),
        m_type  (comp      )
{ }

//! Initializes the Math object using the information from another one
//! @param other another Math object
Math::Math(Math const &other) :
        BaseExt (other         ),
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
        BaseExt(),
        m_value(tConfItemBase::FindConfigItem(value))
{ }

//! Initializes the ConfItem object using the information from another one
//! @param other another ConfItem object
ConfItem::ConfItem(ConfItem const &other):
        BaseExt(other),
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

int ColNode::GetInt(void) const               { return (*m_col.begin())->GetInt(); }
float ColNode::GetFloat(void) const           { return (*m_col.begin())->GetFloat(); }
myCol ColNode::GetCol(void) const { return m_col; }

myCol getCol(BasePtr base) {
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(base->copy());
  myCol ijk(baseExt->GetCol());
  return ijk;
}


bool ColNode::operator==(Base const &other) const { 
  //    return m_col == static_cast<myCol >(other);  
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col == static_cast<myCol >(*baseExt) ;
}
bool ColNode::operator!=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col != static_cast<myCol >(*baseExt); 
}
bool ColNode::operator>=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col >= static_cast<myCol >(*baseExt); 
}
bool ColNode::operator<=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col <= static_cast<myCol >(*baseExt); 
  }
bool ColNode::operator> (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col >  static_cast<myCol >(*baseExt); 
}
bool ColNode::operator< (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return m_col <  static_cast<myCol >(*baseExt); 
}


int ColUnary::GetInt(void) const               { return (*_operation().begin())->GetInt(); }
float ColUnary::GetFloat(void) const           { return (*_operation().begin())->GetFloat(); }
myCol ColUnary::GetCol(void) const { return _operation(); }

bool ColUnary::operator==(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() == static_cast<myCol >(*baseExt); 
}
bool ColUnary::operator!=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() != static_cast<myCol >(*baseExt); 
}
bool ColUnary::operator>=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() >= static_cast<myCol >(*baseExt); 
}
bool ColUnary::operator<=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() <= static_cast<myCol >(*baseExt); 
}
bool ColUnary::operator> (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() >  static_cast<myCol >(*baseExt); 
}
bool ColUnary::operator< (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() <  static_cast<myCol >(*baseExt); 
}


myCol ColPickOne::_operation() const
{
  myCol ijk = getCol(ColUnary::m_child);
  
  myCol tempCol;
  int number = ijk.size();
  if(number != 0) {
    int ran = number / 2;
    
    myCol::iterator iter = ijk.begin();
    int i=0;
    for (i=0; i<ran; i++)
      ++iter;
    BasePtr asdf((*iter)->copy());
    tempCol.insert(asdf);
  }
  return tempCol;
}


myCol ColBinary::_operation(void) const {
  myCol res;
  
  myCol a = getCol(ColBinary::r_child);
  myCol b = getCol(ColBinary::l_child);
  
  set_union(a.begin(),
	    a.end(),
	    b.begin(),
	    b.end(),
	    inserter(res, res.begin()),
	    FooPtrOps());
  
  return res;
}

bool ColBinary::operator==(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() == static_cast<myCol >(*baseExt); 
}
bool ColBinary::operator!=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() != static_cast<myCol >(*baseExt); 
}
bool ColBinary::operator>=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() >= static_cast<myCol >(*baseExt); 
}
bool ColBinary::operator<=(Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() <= static_cast<myCol >(*baseExt); 
}
bool ColBinary::operator> (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() >  static_cast<myCol >(*baseExt); 
}
bool ColBinary::operator< (Base const &other) const { 
  BaseExt const *baseExt = dynamic_cast<BaseExt*>(const_cast<Base*>(&other));
  return _operation() <  static_cast<myCol >(*baseExt); 
}

int ColBinary::GetInt(void) const               { return (*_operation().begin())->GetInt(); }
float ColBinary::GetFloat(void) const           { return (*_operation().begin())->GetFloat(); }
myCol ColBinary::GetCol(void) const             { return _operation(); }

void displayCol(myCol res) {
  myCol::iterator iter;
  for(iter = res.begin();
      iter != res.end();
      ++iter) {
    std::cout << (*iter)->GetInt() << " ";
  }
  std::cout << std::endl;
}

myCol ColUnion::_operation(void) const {
  myCol res;
  
  myCol a = getCol(ColBinary::r_child);
  myCol b = getCol(ColBinary::l_child);
  
  set_union(a.begin(),
	    a.end(),
	    b.begin(),
	    b.end(),
	    inserter(res, res.begin()),
	    FooPtrOps());
  
  return res;
}


myCol ColIntersection::_operation(void) const {
  myCol res;
  myCol a = getCol(ColBinary::r_child);
  myCol b = getCol(ColBinary::l_child);
  
  set_intersection(a.begin(),
		   a.end(),
		   b.begin(),
		   b.end(),
		   inserter(res, res.begin()),
		   FooPtrOps());
  
  return res;
}




myCol ColDifference::_operation(void) const {
  myCol res;
  myCol a = getCol(ColBinary::r_child);
  myCol b = getCol(ColBinary::l_child);
  
  set_difference(a.begin(),
		 a.end(),
		 b.begin(),
		 b.end(),
		 inserter(res, res.begin()),
		 FooPtrOps());
  
  return res;
}

}
