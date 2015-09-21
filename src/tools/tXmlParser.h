/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

// The base class for xml parsers

#ifndef ArmageTron_XMLPARSER_H
#define ArmageTron_XMLPARSER_H

#include "defs.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "tString.h"
#include "tError.h"
#include "tConsole.h"

#include <map>

namespace tXmlParserNamespace {

typedef std::map<tString, tString> tAttributeList;
enum { SAX, DOM };

//! TODO: add documentation, this is just here so tXmlParser::node gets documented
class tXmlParser {
public:
    //! DOM node class; provides convienient access to libxml without dealing with its functions
    class node {
        typedef char CHAR; //!< change to wchar for future unicode support
        xmlNode *m_cur;  //!< The current node, the whole class is a wrapper around it
    public:
        node(xmlNode *cur); //!< Constructor from a libxml node
	node() : m_cur(0) {} //!< Dummy constructor, only of use for stl containers
        bool IsOfType(CHAR const *name) const; //!< Checks the type (name) of a node
        tString GetName(void) const; //!< Gets the type (name) of the node
        tString GetProp(CHAR const *prop) const; //!< Get a property of this node as a raw string
        bool GetPropBool(CHAR const *prop) const; //!< Get a boolean value out of a property
#ifndef _MSC_VER
        template<typename T> void GetProp(CHAR const *prop, T &target) const; //!< Get a property, convert to any type
#else
        void GetProp(CHAR const *prop, REAL &target) const;
        void GetProp(CHAR const *prop, bool &target) const;
        void GetProp(CHAR const *prop, int &target) const;
        void GetProp(CHAR const *prop, tString &target) const;
#endif

        bool IsOfType(tString const &name) const { return IsOfType(name.c_str());} //!< Checks the type (name) of a node
        tString GetProp(tString const &prop) const { return GetProp(prop.c_str()); } //!< Get a property of this node as a raw string
        bool GetPropBool(tString const &prop) const { return GetPropBool(prop.c_str()); } //!< Get a boolean value out of a property
        template<typename T> void GetProp(tString const &prop, T &target) const {GetProp(prop.c_str(), target);} //!< Get a property, convert to any type

        node &operator++(); //!< Move on to the next element on the same level
        node const operator++(int); //!< Move on to the next element on the same level

        operator bool () const; //!< Does this node actually exist?

        node GetFirstChild(void) const; //!< Get the first child node
    };

    node GetRoot();

    xmlDocPtr m_Doc; /* The xml document */

    // If DOM isn't what you want, make sure you override this accordingly
    tXmlParser() : m_Doc(0) { SetMode(DOM); };
    virtual ~tXmlParser();

    // Provided just in case you need it, probably will never be used
    tXmlParser(int mode) { SetMode(mode); m_Doc = 0; };

    void SetMode(int mode) { m_Mode = mode; };

    virtual bool LoadFile(const char* filename, const char *uri="");

    // Loads and parses, which is usually what you want
    bool LoadXmlFile(const char* filename, const char* uri="");

    // Loads without parsing, used if you need special parameters in your parse function
    bool LoadWithoutParsing(const char* filename, const char* uri="");
    bool LoadWithParsing(const char* filename, const char* uri="");

    // This is a generic parse.  If you're in DOM mode, it will call your ParseDom
    // method, if you're in SAX mode, it will start the sax parser.
    // If you need to parse with special parameters, you need to make your own method
    // to do it.
    bool Parse();

    // The default implementation does nothing, override this to provide your own
    // dom walker or whatever.  Return true on success, false on failure
    virtual bool ParseDom();

    // Starts the sax parser.  You should override the callbacks listed below to use
    // this and call the constructor to initialize with sax mode
    bool ParseSax();

    // todo: add arguments to these methods
    virtual void startElement(tString &element, tAttributeList &attributes);
    virtual void endElement(tString &element);
    virtual void startDocument();
    virtual void endDocument();

    // internal callbacks, used to bridge libxml2's sax data to armagetron's internal
    // data types.  subclasses can safely ignore these
    void cb_startDocument();
    void cb_endDocument();
    void cb_startElement(const xmlChar* name, const xmlChar** attrs);
    void cb_endElement(const xmlChar *name);
    tString m_Filename;
private:
    int m_Mode;
protected:
    virtual bool ValidateXml(FILE* docfd, const char* uri=NULL, const char* filepath=NULL);
};

class tXmlResource : public tXmlParser {
public:
    bool LoadFile(const char* filename, const char* uri="");
protected:
    bool ValidateXml(FILE* docfd, const char* uri, const char* filepath);
    tString m_Author;   //!< the author of the resource
    tString m_Category; //!< the category of the resource
    tString m_Name;     //!< the name of the resource
    tString m_Version;  //!< the version of the resource
    tString m_Type;     //!< the type of the resource
    node GetFileContents(void); //!< Returns the node the "real" file contents are within
};

#ifndef _MSC_VER
//! This function will print an error message if the extraction failed
//! @param prop The name of the attribute to be read
//! @param target The place where the extacted value will be stored
template<typename T> void tXmlParser::node::GetProp(CHAR const *prop, T &target) const {
    if(!(GetProp(prop).Convert(target))) {
        tERR_WARN( "Property '" + tString(prop) + "' of node of type '" + GetName() + "' is '" + GetProp(prop) + "' which isn't of type '" + typeid(T).name() + "' as needed.");
    }
}
#endif

}
using tXmlParserNamespace::tXmlParser;
using tXmlParserNamespace::tXmlResource;
#endif
