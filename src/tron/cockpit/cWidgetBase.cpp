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

#include "cockpit/cWidgetBase.h"
#include "cockpit/cCockpit.h"

#ifndef DEDICATED

#include <typeinfo>

namespace cWidget {

void Base::SetCam(int Cam) {
    m_Cam = Cam;
}

//! Prints an error message to the console.
//!
//! This should be called by derived classes if parsing a setting fails.
//! @param cur the node that's being attempted to parse
void Base::DisplayError(tXmlParser::node cur) {
    tERR_WARN("Element of type '" + cur.GetName() + "' not processable in this context: '" + typeid(*this).name() + "'");
}

//! This needs to be overwritten if the derived class has anyting to parse or can be derived from.
//!
//! It should try to parse the given node, return true on success or return the result of the Process() function of the widget it's derived from if it fails.
//! @param cur the node that's being attempted to parse
//! @returns true on success, false on failure
bool Base::Process(tXmlParser::node cur) {
    return false;
}

WithCoordinates::WithCoordinates() : m_originalPosition(0,0), m_originalSize(1,1), m_position(0,0), m_size(1,1)
{}

bool WithCoordinates::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Position")) {
        tCoord shift;
        cur.GetProp("x", shift.x);
        cur.GetProp("y", shift.y);
        m_position += shift;
        m_originalPosition = m_position;
        return true;
    }
    if(cur.IsOfType("Size")) {
        tCoord factor;
        cur.GetProp("width", factor.x);
        cur.GetProp("height", factor.y);
        m_size *= factor;
        m_originalSize = m_size;
        return true;
    }
    return Base::Process(cur);
}

//!@arg factor the factor to multiply with
void WithCoordinates::SetFactor(float factor) {
    m_position.y = (m_originalPosition.y + 1.) * factor - 1.;
    m_size.y = m_originalSize.y * factor;
}

bool WithDataFunctions::Process(tXmlParser::node cur) {
    return Base::Process(cur);
}

//! @param cur the node to be parsed as DataSet
//! @return the resulting data set
tValue::Set WithDataFunctions::ProcessDataSet(tXmlParser::node cur) {
    std::auto_ptr<tValue::Base>
    value(new tValue::Base()),
    minimum(new tValue::Base()),
    maximum(new tValue::Base());
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        tValue::Base *newvalue = 0;
        tString name = cur.GetName();
        if(name == "AtomicData") {
            newvalue = ProcessAtomicData(cur);
        } else if(name == "Conditional") {
            newvalue = ProcessConditional(cur);
        } else if(name == "Math") {
            newvalue = ProcessMath(cur);
        }
        if(newvalue != 0) {
            tString field = cur.GetProp("field");
            if(field == "source") {
                value = std::auto_ptr<tValue::Base>(newvalue);
            } else if(field=="minimum") {
                minimum = std::auto_ptr<tValue::Base>(newvalue);
            } else if(field=="maximum") {
                maximum = std::auto_ptr<tValue::Base>(newvalue);
            }
        }
    }
    return tValue::Set(
               value.release(),
               minimum.release(),
               maximum.release());
}

tValue::Base *WithDataFunctions::ProcessMath(tXmlParser::node cur) {
    tValue::BasePtr lvalue(ProcessDataSource(cur.GetProp("lvalue")));
    tValue::BasePtr rvalue(ProcessDataSource(cur.GetProp("rvalue")));
    std::map<tString, tValue::Math::type> types;
    types[tString("sum"       )] = tValue::Math::sum;
    types[tString("difference")] = tValue::Math::difference;
    types[tString("product"   )] = tValue::Math::product;
    types[tString("quotient"  )] = tValue::Math::quotient;

    std::map<tString, tValue::Math::type>::iterator iter;
    if((iter = types.find(cur.GetProp("type"))) == types.end()) {
        tERR_WARN("Type '" + cur.GetProp("type") + "' unknown!");
        iter = types.begin();
    }

    for (tXmlParser::node child = cur.GetFirstChild(); child; ++child) {
        tString name = child.GetName();
        if(name == "RValue") {
            rvalue = tValue::BasePtr(ProcessConditionalCore(child));
        } else if(name == "LValue") {
            lvalue = tValue::BasePtr(ProcessConditionalCore(child));
        }
    }


    tValue::Base *val = new tValue::Math(lvalue.release(), rvalue.release(), iter->second);
    ProcessDataTags(cur, *val);
    return val;
}

tValue::Base *WithDataFunctions::ProcessConditional(tXmlParser::node cur) {
    tValue::Base *lvalue = ProcessDataSource(cur.GetProp("lvalue"));
    tValue::Base *rvalue = ProcessDataSource(cur.GetProp("rvalue"));
    tValue::BasePtr truevalue(new tValue::Base), falsevalue(new tValue::Base);

    std::map<tString, tValue::Condition::comparator> comparators;
    comparators[tString("gt")] = tValue::Condition::gt;
    comparators[tString("lt")] = tValue::Condition::lt;
    comparators[tString("ge")] = tValue::Condition::ge;
    comparators[tString("le")] = tValue::Condition::le;
    comparators[tString("eq")] = tValue::Condition::eq;
    comparators[tString("ne")] = tValue::Condition::ne;

    std::map<tString, tValue::Condition::comparator>::iterator iter;
    if((iter = comparators.find(cur.GetProp("operator"))) == comparators.end()) {
        tERR_WARN("Operator '" + cur.GetProp("operator") + "' unknown!");
        iter = comparators.begin();
    }

    for (cur = cur.GetFirstChild(); cur; ++cur) {
        tString name = cur.GetName();
        if(name == "IfTrue") {
            truevalue = tValue::BasePtr(ProcessConditionalCore(cur));
        } else if(name == "IfFalse") {
            falsevalue = tValue::BasePtr(ProcessConditionalCore(cur));
        }
    }
    return new tValue::Condition(lvalue, rvalue, truevalue.release(), falsevalue.release(), iter->second);
}

tValue::Base *WithDataFunctions::ProcessConditionalCore(tXmlParser::node cur) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        tString name = cur.GetName();
        if(name == "AtomicData") {
            return ProcessAtomicData(cur);
        }
        if(name == "Conditional") {
            return ProcessConditional(cur);
        }
        if(name == "Math") {
            return ProcessMath(cur);
        }
    }
    tERR_WARN("IfTrue or IfFalse node doesn't contain an AtomicData or Conditional node")
    return new tValue::Base();
}

tValue::Base *WithDataFunctions::ProcessAtomicData(tXmlParser::node cur) {
    tValue::Base *ret = ProcessDataSource(cur.GetProp("source"));
    ProcessDataTags(cur, *ret);
    return ret;
}

void WithDataFunctions::ProcessDataTags(tXmlParser::node cur, tValue::Base &data) {
    int precision, minwidth;
    cur.GetProp("precision", precision);
    cur.GetProp("minwidth", minwidth);
    tString fill = cur.GetProp("fill");
    if(fill.size() != 1) {
        tERR_WARN("Attribute 'fill' has to have a length of 1!");
        fill = "!";
    }
    data.SetPrecision(precision);
    data.SetMinsize(minwidth);
    data.SetFill(fill(0));
}

tValue::Base *WithDataFunctions::ProcessDataSource(tString const &data) {
    //is it an integer?
    int val_int;
    if(data.Convert(val_int)) return new tValue::Int(val_int);
    //is it a float
    float val_float;
    if(data.Convert(val_float)) return new tValue::Float(val_float);

    //is it one of the dynamic callbacks?
    std::map<tString, tValue::Callback<cCockpit>::cb_ptr>::const_iterator iter;
    if((iter = stc_callbacks.find(data.ToLower())) != stc_callbacks.end()) {
	if(stc_forbiddenCallbacks.count(data.ToLower())) return new tValue::Base();
        return new tValue::Callback<cCockpit>(iter->second, cCockpit::GetCockpit());
    }

    //growing desperate... is this a configuration value maybe?
    tValue::ConfItem *item = new tValue::ConfItem(data);
    if(item->Good())
        return item;
    delete item;

    //Ok, giving up... this has to be a string then.
    return new tValue::String(data);
}

bool WithSingleData::Process(tXmlParser::node cur) {
    if(cur.IsOfType("DataSet")) {
        m_data = ProcessDataSet(cur);
        return true;
    }
    return WithDataFunctions::Process(cur);
}

bool WithIdData::Process(tXmlParser::node cur) {
    if(cur.IsOfType("DataSet")) {
        tString id = cur.GetProp("id");
        if(id.empty()) {
            tERR_WARN("Empty or no id tag where needed!");
            return true;
        }
        m_data[id] = ProcessDataSet(cur);
        return true;
    }
    return WithDataFunctions::Process(cur);
}

bool WithTable::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Face")) {
        ProcessCore(cur);
        return true;
    }
    return WithIdData::Process(cur);
}

void WithTable::ProcessCore(tXmlParser::node cur) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        if(cur.IsOfType("Table")) {
            ProcessTable(cur);
            return;
        }
    }
}

void WithTable::ProcessTable(tXmlParser::node cur) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        if(cur.IsOfType("Row")) {
            m_table.push_back(std::deque<std::deque<tValue::Set> >());
            ProcessRow(cur);
        }
    }
}

void WithTable::ProcessRow(tXmlParser::node cur) {
    for (cur = cur.GetFirstChild(); ++cur; ++cur) {
        tString name = cur.GetName();
        if(name == "Cell") {
            m_table.back().push_back(std::deque<tValue::Set>());
            ProcessCell(cur);
        }
    }
}

void WithTable::ProcessCell(tXmlParser::node cur) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        if(cur.IsOfType("Text")) {
            m_table.back().back().push_back((tValue::Set(new tValue::String(cur.GetProp("value")))));
        } else if(cur.IsOfType("GameData")) {
            std::map<tString, tValue::Set>::iterator iter;
            if((iter = m_data.find(cur.GetProp("data"))) != m_data.end()) {
                m_table.back().back().push_back((tValue::Set(iter->second)));
            } else {
                tERR_WARN("Id '" + cur.GetProp("data") + "' undefined!");
            }
        }
    }
}

bool WithColorFunctions::Process(tXmlParser::node cur) {
    return Base::Process(cur);
}

rGradient WithColorFunctions::ProcessGradient(tXmlParser::node cur) {
    rGradient ret;
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        tString name=cur.GetName();
        if(name == "Solid") {
            ProcessGradientCore(cur, ret);
        } else if(name == "Gradient") {
            std::map<tString, rGradient::direction> directions;
            directions[tString("horizontal")] = rGradient::horizontal;
            directions[tString("vertical")] = rGradient::vertical;
            directions[tString("value")] = rGradient::value;
            std::map<tString, rGradient::direction>::iterator iter;
            if((iter = directions.find(cur.GetProp("orientation"))) != directions.end()) {
                ret.SetDir(iter->second);
            } else {
                tERR_WARN("Gradient orientation '" + cur.GetProp("orientation") + "' unknown!");
            }
            ProcessGradientCore(cur, ret);
        }
    }
    return ret;
}

void WithColorFunctions::ProcessGradientCore(tXmlParser::node cur, rGradient &gradient) {
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        if(cur.IsOfType("Color")) {
            float r,g,b,a,at;
            cur.GetProp("r", r);
            cur.GetProp("g", g);
            cur.GetProp("b", b);
            cur.GetProp("alpha", a);
            cur.GetProp("at", at);
            gradient[at] = rColor(r ,g ,b ,a);
        }
    }
}

bool WithForeground::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Foreground")) {
        m_foreground = ProcessGradient(cur);
        return true;
    }
    return WithColorFunctions::Process(cur);
}

bool WithBackground::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Background")) {
        m_background = ProcessGradient(cur);
        return true;
    }
    return WithColorFunctions::Process(cur);
}

bool WithCaption::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Caption")) {
        ProcessCaption(cur);
        return true;
    }
    return Base::Process(cur);
}

void WithCaption::ProcessCaption(tXmlParser::node cur) {
    ProcessCaptionLocation(cur);
    for (cur = cur.GetFirstChild(); cur; ++cur) {
        if(cur.IsOfType("Text")) {
            m_caption = cur.GetProp("value");
        }
    }
}

void WithCaption::ProcessCaptionLocation(tXmlParser::node cur) {
    std::map<tString, int> locations;
    locations[tString("top")] = top;
    locations[tString("bottom")] = bottom;
    locations[tString("off")] = off;

    std::map<tString, int>::iterator iter;
    if((iter = locations.find(cur.GetProp("location"))) != locations.end()) {
        m_captionloc = iter->second;
    } else {
        tERR_WARN("Location '" + cur.GetProp("location") + "' unknown!");
        m_captionloc = bottom;
    }
}

bool WithReverse::Process(tXmlParser::node cur) {
    if(cur.IsOfType("Reverse")) {
        cur.GetPropBool("value");
        return true;
    }
    return Base::Process(cur);
}

bool WithShowSettings::Process(tXmlParser::node cur) {
    if(cur.IsOfType("ShowMinimum")) {
        m_showmin = cur.GetPropBool("value");
        return true;
    }
    if(cur.IsOfType("ShowMaximum")) {
        m_showmax = cur.GetPropBool("value");
        return true;
    }
    if(cur.IsOfType("ShowCurrent")) {
        m_showvalue = cur.GetPropBool("value");
        return true;
    }
    return Base::Process(cur);
}

}

#endif
