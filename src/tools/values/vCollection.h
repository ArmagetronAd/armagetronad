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
//! @brief Contains the class declarations for collection vValue members

#ifndef ARMAGETRON_vCollection_h
#define ARMAGETRON_vCollection_h

#include <set>

#include "values/vCore.h"

namespace vValue {
namespace MiscWTF {

struct FooPtrOps;

typedef std::set<BasePtr, FooPtrOps> myCol;

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

}
using namespace MiscWTF;
namespace Expr {
namespace Collection {

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
  its collection of vValue, possibly do a manipulation on the collection and
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
  The ColBinary and its children class operates on 2 vValue. The exact manipulation is determined by the child class
  
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
}
}

#endif
