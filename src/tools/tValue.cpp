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

#include "tValue.h"
#include "tConfiguration.h"

#include "mathexpr.h"

namespace tValue {


Registration::Registration(std::vector<tString> flags, tString fname, int argc, void *ctor):
	m_flags(flags),
	m_fname(fname),
	m_argc(argc),
	m_ctor(ctor)
{
	theRegistry.reg(this);
}

Registration::Registration(const char *flags, const char *fname, int argc, void *ctor):
	m_fname(fname),
	m_argc(argc),
	m_ctor(ctor)
{
	m_flags.clear();
	for (
		const char *ptr = flags, *ep;
		ptr;
		ptr = (ep[0] == '\0') ? 0 : (ep + 1)
	)
	{
		ep = strchr(ptr, '\n');
		if (!ep)
			ep = ptr + strlen(ptr);
		std::string s(ptr, ep - ptr);
		m_flags.push_back(s);
	}
	theRegistry.reg(this);
}

Base *
Registration::use(arglist args) {
	switch (args.size()) {
	case 0:	return ((ctor0*)m_ctor)();
	case 1:	return ((ctor1*)m_ctor)(args[0]);
	case 2:	return ((ctor2*)m_ctor)(args[0], args[1]);
	case 3:	return ((ctor3*)m_ctor)(args[0], args[1], args[2]);
	case 4:	return ((ctor4*)m_ctor)(args[0], args[1], args[2], args[3]);
	case 5:	return ((ctor5*)m_ctor)(args[0], args[1], args[2], args[3], args[4]);
	}
	throw("Unimplemented # of args");
}

bool
Registration::match(std::vector<tString> flags, tString fname, int argc)
{
	if (fname.Compare(m_fname, true))
		return false;
	for (std::vector<tString>::iterator i = flags.begin(); i != flags.end(); ++i)
	{
		bool flagmatch = (flags[0] == "!");
		tString flag = *i;
		if (flagmatch)
			flag = flag.SubStr(1);
		if (!flag.Compare(*i, true))
			flagmatch = !flagmatch;
		if (!flagmatch)
			return false;
	}
	if (argc != m_argc)
		return false;
	return true;
}

void
Registry::reg(Registration *it)
{
	registrations.push_back(it);
}

Base *
Registry::create(std::vector<tString> flags, tString fname, arglist args)
{
	for (std::vector<Registration *>::iterator i = registrations.begin(); i != registrations.end(); ++i)
		if ((*i)->match(flags, fname, args.size()))
			return (*i)->use(args);
	return NULL;
}

Registry theRegistry;


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

//! @param expr The expression to be parsed
Expr::Expr(tString const &expr) : m_operation(new ROperation(expr.c_str())) {}

//! @param expr The expression to be parsed
//! @param vars A map of variable names and their references
//! @param functions A map of function names and their references
Expr::Expr(tString const &expr, varmap_t const &vars, funcmap_t const &functions) {
    // what a mess. why can't this darn library just use stl functions? :s
    RVar **vararray;
    vararray = new RVar*[vars.size()];
    unsigned int i = 0;
    for(varmap_t::const_iterator iter = vars.begin(); iter != vars.end(); ++iter, ++i) {
        vararray[i] = new RVar(iter->first.c_str(), iter->second);
    }
    RFunction **funcarray;
    funcarray = new RFunction*[functions.size()];
    unsigned int j = 0;
    for(funcmap_t::const_iterator iter = functions.begin(); iter != functions.end(); ++iter, ++j) {
        funcarray[j] = new RFunction(iter->second);
        funcarray[j]->SetName(iter->first.c_str());
    }
    m_operation = boost::shared_ptr<ROperation>(new ROperation(expr.c_str(), vars.size(), vararray, functions.size(), funcarray));
    for(i = 0; i < vars.size(); i++) {
        delete vararray[i];
    }
    delete[] vararray;
    for(i = 0; i < functions.size(); i++) {
        delete funcarray[i];
    }
    delete[] funcarray;
}

Base *Expr::copy(void) const {
    return new Expr(*this);
}

Variant Expr::GetValue() const {
    return m_operation->Val();
}

//class blah {
//public:
//    blah() {
//        float x=4,y=3;
//		Expr::varmap_t vars;
//		vars[tString("x")] = &x;
//		vars[tString("y")] = &y;
//		Expr::funcmap_t functions;
//		functions[tString("f")] = &sinf;
//		Expr expr(tString("x+f(y)"), vars, functions);
//        std::cerr << expr.GetFloat() << std::endl;
//    }
//};
//blah asdfgsf;

Registration register_iff("func\nlogic", "iff", 3, (void *)
	( Registration::ctor3* )& Creator<Condition>::create<Base*,Base*,Base*> );

Condition::Condition(Base  * condvalue, Base  * truevalue, Base  * falsevalue) :
        m_condvalue (condvalue ),
        m_truevalue (truevalue ),
        m_falsevalue(falsevalue)
{
}

//! Constructs a new Condition object with the given parameters
//! @param condvalue  the value to be used as the condition
//! @param truevalue  the value to be used if the condition is true
//! @param falsevalue the value to be used if the condition is false
Condition::Condition(BasePtr condvalue, BasePtr truevalue, BasePtr falsevalue) :
        m_condvalue (condvalue ),
        m_truevalue (truevalue ),
        m_falsevalue(falsevalue)
{
}

//! Initializes the Condition object using the information from another one
//! @param other another Condition object
Condition::Condition(Condition const &other) :
        BaseExt     (other             ),
        m_condvalue (other.m_condvalue ->copy()),
        m_truevalue (other.m_truevalue ->copy()),
        m_falsevalue(other.m_falsevalue->copy())
{ }

//! Overwritten copy function
//! @returns a new copy of the current object
Base *Condition::copy(void) const {
    return new Condition(*this);
}

//! Uses a specific type (usually int or float) to perform the comparison
//! @param fun the return value of this function is used for the comparison
//! @returns a reference to either m_lvalue or m_rvalue based on if the condition is true or false
Base const &Condition::GetExpr() const {
    bool truth = false;
    // In the future, we might want to define some kind of rules for truth
    try {
        truth = boost::lexical_cast<bool>(m_condvalue->GetValue());
    }
    catch(boost::bad_lexical_cast &) { }
    return truth ? *m_truevalue : *m_falsevalue;
}

//! Performs the comparison and returns the resulting value
//! @returns the result as a variant
Variant Condition::GetValue(void) const {
    return GetExpr().GetValue();
}

//! Performs the comparison and returns the resulting value
//! @returns the result as an integer
int Condition::GetInt(void) const {
    return GetExpr().GetInt();
}

//! Performs the comparison and returns the resulting value
//! @returns the result as a float
float Condition::GetFloat(void) const {
    return GetExpr().GetFloat();
}

//! Performs the comparison and returns the resulting value
//! @returns the result as a string
tString Condition::GetString(tValue::Base const *other) const {
    return GetExpr().GetString(other);
}

bool Condition::operator==(Base const &other) const { return GetExpr() == other; }
bool Condition::operator!=(Base const &other) const { return GetExpr() != other; }
bool Condition::operator>=(Base const &other) const { return GetExpr() >= other; }
bool Condition::operator<=(Base const &other) const { return GetExpr() <= other; }
bool Condition::operator> (Base const &other) const { return GetExpr() >  other; }
bool Condition::operator< (Base const &other) const { return GetExpr() <  other; }

//! Constructs a new binary operator object with the given parameters
//! @param value      the value for the operation
UnaryOp::UnaryOp(BasePtr value) :
        m_value(value    )
{ }

//! Initializes the binary operator object using the information from another one
//! @param other another binary operator object
UnaryOp::UnaryOp(UnaryOp const &other) :
        BaseExt (other         ),
        m_value(other.m_value->copy())
{ }

Base *UnaryOp::copy(void) const {
    return new UnaryOp(*this);
}

//! Returns the value negated
//! @returns the result
Variant
Not::GetValue(void) const {
    bool truth = false;
    try {
        truth = boost::lexical_cast<bool>(m_value->GetValue());
    }
    catch(boost::bad_lexical_cast &) { }
    return (int)(!truth);
}

Base *Not::copy(void) const {
    return new Not(*this);
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
        BaseExt (other         ),
        m_lvalue(other.m_lvalue->copy()),
        m_rvalue(other.m_rvalue->copy())
{ }

Base *BinaryOp::copy(void) const {
    return new BinaryOp(*this);
}

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

#define CodeStdBinOp(classname, op)       \
Variant                                   \
classname::GetValue(void) const {         \
    return (int)(*m_lvalue op *m_rvalue); \
}                                         \
Base *classname::copy(void) const {       \
    return new classname(*this);          \
}                                         \

CodeStdBinOp(GreaterThan    , >  )
CodeStdBinOp(GreaterOrEquals, >= )
CodeStdBinOp(         Equals, == )
CodeStdBinOp(   LessOrEquals, <= )
CodeStdBinOp(   LessThan    , <  )

Variant
Compare::GetValue(void) const {
    // This could probably be optimized
    if (*m_lvalue == *m_rvalue)
        return 0;
    else
        if (*m_lvalue < *m_rvalue)
            return -1;
        else
            return 1;
}

Base *Compare::copy(void) const {
    return new Compare(*this);
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

Variant ConfItem::GetValue() const {
    return GetString();
}

//! Reads from the configuration item
//! @returns the result as a string
tString ConfItem::GetString(Base const *other) const {
    return Output(Read(), other);
}

Variant ColNode::GetValue(void) const         { return (*m_col.begin())->GetValue(); }
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


Variant ColUnary::GetValue(void) const         { return (*_operation().begin())->GetValue(); }
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

Variant ColBinary::GetValue(void) const         { return (*_operation().begin())->GetValue(); }
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


Registration register_sin("func\nmath", "sin", 1, (void *)
	( Registration::ctor1* )& Creator<Func::Sin>::create<Base*> );


}


