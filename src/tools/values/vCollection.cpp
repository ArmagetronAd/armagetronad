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

#include "vCore.h"
#include "vCollection.h"

namespace vValue {
	namespace Expr {
		namespace Collection {

myCol BaseExt::GetCol(void) const {
    myCol col;
    BasePtr node(this->copy());
    col.insert(node);
    return col;
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

		}
	}
}


