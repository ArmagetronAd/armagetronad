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

#ifndef ARMAGETRON_WIDGET_BASE_H
#define ARMAGETRON_WIDGET_BASE_H

#include "tValue.h"

#ifndef DEDICATED

#include "tString.h"
#include "tXmlParser.h"
#define DONTDOIT
#include "rGradient.h"
#include "rTexture.h"
#include <memory>
#include "tSafePTR.h"

class cCockpit;

//! @file
//! @brief Contains the classes the actual widgets are based on
//!
//! The classes in here do all the parsing and storing of settings,
//! they don't define a Render() function and therefore can't be
//! directly used.
//!
//! This file is a candidate for being moved to src/ui or even
//! src/tools (since its classes doesn't actually render anything).

//! Namespace for all widgets
namespace cWidget {

//! Offers basic functions and keeps the camera settings
class Base : public tReferencable<Base> {
    int m_Cam; //!< The camera(s) the widget will be rendered for
    bool m_ParsingTemplate;
protected:
    void DisplayError(tXmlParser::node cur); //!< Displays a parsing error message
    bool m_Render; //!< Should this Widget be rendered?
    bool m_RenderDefault; //!< Should this Widget be rendered by default?
    bool m_Sticky; //!< Should this Widget be sticky?
    cCockpit *m_Cockpit; //!< the cockpit this widget belongs to
public:
    Base() : m_ParsingTemplate(false), m_Render(true), m_RenderDefault(true), m_Sticky(true) {}
    virtual ~Base() { }
    virtual void Render() = 0; //!< Needs to be owerwritten for all widgets that can be rendered (and therefore created)
    void SetCam(int Cam); //!< Set the camera(s) this widget will be rendered for

    int GetCam(void) { return m_Cam; } //!< Get the camera(s) this widget will be rendered for
    virtual bool Process(tXmlParser::node cur); //!< Process a node
    void SetDefaultState (bool state) { m_Render = m_RenderDefault = state; }
    void SetCockpit(cCockpit *cockpit) {m_Cockpit = cockpit;} //!< Set the cockpit this widget belongs to
    void SetSticky (bool sticky) { m_Sticky = sticky; }
    bool Active() { return m_Render; } //!< Should we render this?
    //! Toggle activity
    void Toggle(bool state) {
        if(m_Sticky) {
            if(state) {
                m_Render = !m_Render;
            }
        } else {
            m_Render = state;
        }
    }
    //! Overwrite if you need to handle custom events (other than toggling)
    //! @param state true if the key was pressed, false if it was released
    //! @param id key id of the event that was received
    virtual void HandleEvent(bool state, int id) { Toggle(state); }
    //! Call this to surpress warnings about unknown tags
    //! @param state whether to surpress warnings
    void ParseTemplate(bool state) {m_ParsingTemplate = state;}
};

typedef std::auto_ptr<Base> Base_ptr; //!< simple shortcut; used in the derived classes

//! Able to store and parse the position and size of a widget
class WithCoordinates : virtual public Base {
    tCoord m_originalPosition; //!< The position without any transformations applied
    tCoord m_originalSize; //!< The size without any transformations applied
protected:
    tCoord m_position; //!< The x- and y- coordinates of the widget
    tCoord m_size; //!< The size as width and height
public:
    WithCoordinates(); //!< Default constructor
    bool Process(tXmlParser::node cur); //!< This function will parse Size and Position nodes
    void SetFactor(float factor); //!< multiply all y- coordinates by a value
};

//! This class offers functions to parse DataSet and friends. It doesn't parse anything by itself, ProcessDataSet() has to be called by an inherited class
class WithDataFunctions : virtual public Base {
    tValue::Base *ProcessValue(tXmlParser::node cur); //!< Processes a Value node given as its parameter, parses it, and returns a tValue
    tValue::Base *ProcessMath(tXmlParser::node cur); //!< Processes a Math node given as its parameter and returns it
    tValue::Base *ProcessConditionalCore(tXmlParser::node cur); //!< Processes an IfTrue or IfFalse node and returns the resulting Value
    tValue::Base *ProcessConditional(tXmlParser::node cur); //!< Processes a Conditional node given as its parameter and returns it
    tValue::Base *ProcessAtomicData(tXmlParser::node cur); //!< Processes an AtomicData node and returns the result
    void          ProcessDataTags(tXmlParser::node cur, tValue::Base &data); //!< Processes the precision="" attributes and friends for a given node and Value
    tValue::Base *ProcessDataSource(tString const &data); //!< Processes and returns a simple data source (like lvalue and rvalue)
protected:
    tValue::Set ProcessDataSet(tXmlParser::node cur); //!< Processes a given DataSet tag and returns the resulting set
public:
    bool Process(tXmlParser::node cur);
};

//! Allows one single DataSet without id="" attribute to be parsed and stored
class WithSingleData : virtual public WithDataFunctions {
protected:
    tValue::Set m_data; //!< The parsed DataSet
public:
    bool Process(tXmlParser::node cur); //!< Passes a DataSet to WithDataFunctions::ProcessDataSet() and saves the result or calls WithDataFunctions::Process for any other tag
};

//! Stores multiple DataSets in a map, keyed by their id="" attribute
class WithIdData : virtual public WithDataFunctions {
protected:
    std::map<tString, tValue::Set> m_data; //!< The parsed data sets
public:
    bool Process(tXmlParser::node cur); //!< Passes a DataSet to WithDataFunctions::ProcessDataSet() and saves the result or calls WithDataFunctions::Process for any other tag
};

//! Implements a table, uses the previously parsed DataSets
class WithTable : virtual public WithIdData {
    void ProcessCore(tXmlParser::node cur);  //!< Processes the inside of a Face tag
    void ProcessTable(tXmlParser::node cur); //!< Processes the inside of a Table tag
    void ProcessRow(tXmlParser::node cur);   //!< Processes the inside of a Row tag
    void ProcessCell(tXmlParser::node cur);  //!< Processes the inside of a Cell tag
protected:
    //TODO: make this a real table- oriented structure

    std::deque<std::deque<std::deque<tValue::Set> > > m_table; //!< Stores the resulting table in rows, cells and contained Value sets
public:
    bool Process(tXmlParser::node cur); //!< Processes a Face tag or passes on to WithIdData::Process()
};

//! Offers basic functions to parse nodes that contain color information like Forground or Background
class WithColorFunctions : virtual public Base {
protected:
    rGradient ProcessGradient(tXmlParser::node cur); //!< Processes the inside of a nodes like Foreground or Background
    void ProcessGradientCore(tXmlParser::node cur, rGradient &gradient); //!< Processes the inside of a Solid or Gradient nodes
    void ProcessImage(tXmlParser::node cur, rGradient &gradient, int repeat); //!< Processes the inside of an Image node
public:
    bool Process(tXmlParser::node cur); //!< just passes on to Base::Process()
};

//! Able to parse and store Foreground nodes
class WithForeground : virtual public WithColorFunctions {
protected:
    rGradient m_foreground; //!< Stores the resulting foreground
public:
    bool Process(tXmlParser::node cur); //!< Calls WithColorFunctions::ProcessGradient() for Foreground nodes or passes on to WithColorFunctions::Process()
};

//! Able to parse and store Background nodes
class WithBackground : virtual public WithColorFunctions {
protected:
    rGradient m_background; //!< Stores the resulting background
public:
    bool Process(tXmlParser::node cur); //!< Calls WithColorFunctions::ProcessGradient() for Background nodes or passes on to WithColorFunctions::Process()
};

//! Able to parse and store LineColor nodes
class WithLineColor : virtual public WithColorFunctions {
protected:
    rGradient m_line_color; //!< Stores the resulting background
public:
    bool Process(tXmlParser::node cur); //!< Calls WithColorFunctions::ProcessGradient() for LineColor nodes or passes on to WithColorFunctions::Process()
};

//! Able to parse and store a Caption node
class WithCaption : virtual public Base{
    void ProcessCaption(tXmlParser::node cur); //!< Processes the inside of a Caption node
    void ProcessCaptionLocation(tXmlParser::node cur); //!< Processes the location attribute of a Caption node
protected:
    enum location { //!< the possible positions of a caption
        top,
        bottom,
        off
    };
    tString m_caption; //!< The caption text
    int m_captionloc; //!< The caption location
    WithCaption() : Base(), m_captionloc(off) {}; //!< Default constructor
public:
    bool Process(tXmlParser::node cur); //!< Processes a Caption node or passes on to Base::Process()
};

//! Able to parse and store a Reverse node
class WithReverse : virtual public Base{
protected:
    bool m_reverse; //!< Stores if the widget should be reversed at rendering time
    WithReverse() : m_reverse(false){}; //!< Default constructor
public:
    bool Process(tXmlParser::node cur); //!< Processes a Reverse node or passes on to Base::Process
};

//! Able to parse and store an Angles node
class WithAngles : virtual public Base{
protected:
    float m_angle_min; //!< Stores the angle at the minimum value
    float m_angle_max; //!< Stores the angle at the minimum value
    WithAngles() : m_angle_min(0), m_angle_max(M_PI){}; //!< Default constructor
public:
    bool Process(tXmlParser::node cur); //!< Processes an Angles node or passes on to Base::Process
};

//! Able to parse and store ShowMinimum, ShowMaximum or ShowValue nodes
class WithShowSettings : virtual public Base{
protected:
    bool m_showmin; //!< Should the minimum be rendered?
    bool m_showmax; //!< Should the maximum be rendered?
    bool m_showvalue; //!< Should the value be rendered?
    WithShowSettings() : m_showmin(true), m_showmax(true), m_showvalue(true) {}; //!< Default constructor
public:
    bool Process(tXmlParser::node cur); //!< Processes ShowMinimum, ShowMaximum or ShowValue nodes or passes on to Base::Process()

};

}

#endif
#endif
