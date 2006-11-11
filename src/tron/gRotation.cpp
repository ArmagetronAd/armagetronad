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

#include "rConsole.h"
#include "gRotation.h"
#include "tConfiguration.h"
#include "tError.h"
#include "tAutoDeque.h"
#include "tXmlParser.h"
#include <typeinfo>

// static void cfcb(void) {
//     gRotation::GetRotator().Parse();
// }
// 
// static tString rotation_file("");
// static tConfItem<tString> cf("ROTATION_FILE",rotation_file, &cfcb);
// 
// class gRotationEventNewRound {
// public:
//     void Print();
// };
// class gRotationEventNewMatch {
// public:
//     void Print();
// };
// void gRotationEventNewRound::Print() {
//     con << "Event: New Round\n";
// }
// void gRotationEventNewMatch::Print() {
//     con << "Event: New Match\n";
// }
// 
// //! This is the base class for rotation tags.
// //!
// //! It takes care of parsing child elements and of handling events. Child classes need to override functions they want to change
// class gRotationTag {
// protected:
//     tAutoDeque<gRotationTag> m_childs;
//     void ParseChilds(tXmlParser::node cur); //!< Parse all childs and store them in m_childs
// public:
//     virtual bool HandleEvent(gRotationEvent const &event); //!< Handle an event
//     gRotationTag(tXmlParser::node cur); //!< Parse this element and all childs
//     virtual ~gRotationTag() {}
//     virtual void Apply(); //! Recursively apply all settings in this tag and all tags below
// };
// 
// //! Do- nothing container: just pass to all childs
// void gRotationTag::Apply() {
//     for(tAutoDeque<gRotationTag>::iterator iter = m_childs.begin(); iter != m_childs.end(); ++iter) {
//         (*iter)->Apply();
//     }
// }
// 
// gRotationTag::gRotationTag(tXmlParser::node cur) {
//     ParseChilds(cur);
// }
// 
// //! This should do whatever it needs to do with an event, ie check if typeid(event) == typeid(the_event_type_it_needs)
// //!
// //! It should also delegate events, if appropiate.
// //! @param event the received event
// //! @return true if it did anything, false if not
// bool gRotationTag::HandleEvent(gRotationEvent const &event) {
//     bool ret = false;
//     for(tAutoDeque<gRotationTag>::iterator iter = m_childs.begin(); iter != m_childs.end(); ++iter) {
//         if((*iter)->HandleEvent(event)) ret = true;
//     }
//     return ret;
// }
// class gRotationTagRotate : public gRotationTag {
//     //TODO: This should keep a pointer to the currently used child and only delegate to it.
// 
//     enum rotationType { match, round, fixed };
// rotationType m_rotationType;
// enum rotationMode { random, shuffle, ordered };
// rotationMode m_rotationMode;
// 
// public:
// gRotationTagRotate(tXmlParser::node cur);
//     bool HandleEvent(gRotationEvent const &event); //!< Handle an event
// };
// 
// gRotationTagRotate::gRotationTagRotate(tXmlParser::node cur)
//         : gRotationTag(cur)
// {
//     tString type = cur.GetProp("type");
//     if (type == "match") m_rotationType = match;
//     else if (type == "round") m_rotationType = round;
//     else if (type == "fixed") m_rotationType = fixed;
// else {
//     tERR_WARN(tString("Rotation type '") + type + "' unknown.");
//         m_rotationType = fixed;
// }
// tString mode = cur.GetProp("mode");
//     if (mode == "random" ) m_rotationMode = random ;
//     else if (mode == "shuffle") m_rotationMode = shuffle;
//     else if (mode == "ordered") m_rotationMode = ordered;
//     else {
//         tERR_WARN(tString("Rotation mode '") + mode + "' unknown.");
//         m_rotationMode = random;
//     }
// }
// 
// bool gRotationTagRotate::HandleEvent(gRotationEvent const &event) {
//     if(m_rotationType == match && typeid(event) == typeid(gRotationEventNewMatch)) {
//         //TODO: Insert code to handle a match here
//         con << "Received a new match event and ready to handle it!\n";
//         return true;
//     }
//     if(m_rotationType == round && typeid(event) == typeid(gRotationEventNewRound)) {
//         //TODO: Insert code to handle a round here
//         con << "Received a new round event and ready to handle it!\n";
//         return true;
//     }
//     // default: delegate to all childs (this should only delegate to the child that's active, so this is a TODO)
//     return gRotationTag::HandleEvent(event);
// }
// 
// gRotation &gRotation::GetRotator() {
//     static gRotation theRotation;
//     return theRotation;
// }

void gRotation::HandleNewRound() {
    std::cerr << "round!\n";
	gRoundEventRuby::DoRoundEvents();
}
void gRotation::HandleNewMatch() {
    std::cerr << "match!\n";
	gMatchEventRuby::DoMatchEvents();
}

// gRotation::gRotation() : m_mainRotation(0) {
//     Parse();
// }
// void gRotation::Parse() {
//     if(m_mainRotation != 0) delete m_mainRotation;
//     if(rotation_file.empty()) return; //nothing to do
// 
//     std::cerr << "Opening: " << rotation_file << std::endl;
//     if (!LoadWithParsing(rotation_file)) return;
//     node cur = GetFileContents();
//     if(!cur) {
//         tERR_WARN("No Rotation node found!");
//     }
//     if (m_Type != "aarotate") {
//         tERR_WARN("Type 'aarotate' expected, found '" << cur.GetProp("type") << "' instead");
//         return;
//     }
//     m_mainRotation = new gRotationTag(cur);
// }
// gRotation::~gRotation() {
//     if(m_mainRotation != 0) delete m_mainRotation;
// }
// 
// void gRotationTag::ParseChilds(tXmlParser::node cur) {
//     std::cerr << "Parsing childs of: " << cur.GetName() << ".\n";
//     for(cur = cur.GetFirstChild(); cur; ++cur) {
//         tString name(cur.GetName());
//         std::cerr << "Parsing: " << name << ".\n";
//         if(name == "Rotate") {
//             m_childs.push_back(new gRotationTagRotate(cur));
//         } else if(name == "Possibility") {
//             //<Possibility> doesn't actually do anything, so we can as well use the default tag...
//             m_childs.push_back(new gRotationTag(cur));
//             //} else if(name == "Settings") {
//             //    m_childs.push_back(new gRotationTagSettings(cur));
//         } else {
//             tERR_WARN(tString("Unknown tag for a Rotation file: ") + name);
//         }
//     }
// }


static tCallbackRuby *roundEventRuby_anchor;
gRoundEventRuby::gRoundEventRuby()
	:tCallbackRuby(roundEventRuby_anchor)
{	
}

void gRoundEventRuby::DoRoundEvents()
{
	Exec(roundEventRuby_anchor);
}


static tCallbackRuby *matchEventRuby_anchor;
gMatchEventRuby::gMatchEventRuby()
	:tCallbackRuby(matchEventRuby_anchor)
{	
}

void gMatchEventRuby::DoMatchEvents()
{
	Exec(matchEventRuby_anchor);
}



