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

#include "tXmlParser.h"
#include "tResourceManager.h"
#include "tConsole.h"
#include "tArray.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#ifdef __MINGW32__
#define xmlFree(x) free(x)
#endif

// We'll use this macro to call callbacks
#define CALL_MEMBER_FN(object,ptrToMember)  ((object)->*(ptrToMember))

namespace tXmlParserNamespace {

// These are the global callbacks

void cb_startDocument(void *userData) {
    CALL_MEMBER_FN((tXmlParser*)userData,&tXmlParser::cb_startDocument)();
}

void cb_endDocument(void *userData) {
    CALL_MEMBER_FN((tXmlParser*)userData,&tXmlParser::cb_endDocument)();
}

void cb_startElement(void *userData, const xmlChar *name, const xmlChar **attrs) {
    CALL_MEMBER_FN((tXmlParser*)userData,&tXmlParser::cb_startElement)(name, attrs);
}

void cb_endElement(void *userData, const xmlChar *name) {
    CALL_MEMBER_FN((tXmlParser*)userData,&tXmlParser::cb_endElement)(name);
}


// These next 3 just ripped from libxml2
void cb_warning(void *ctx , const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stdout, "SAX.warning: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}

void cb_error(void *ctx , const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stdout, "SAX.error: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}

void cb_fatalError(void *ctx , const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stdout, "SAX.fatalError: ");
    vfprintf(stdout, msg, args);
    va_end(args);
}


/*
    internalSubsetSAXFunc internalSubset;
    isStandaloneSAXFunc isStandalone;
    hasInternalSubsetSAXFunc hasInternalSubset;
    hasExternalSubsetSAXFunc hasExternalSubset;
    resolveEntitySAXFunc resolveEntity;
    getEntitySAXFunc getEntity;
    entityDeclSAXFunc entityDecl;
    notationDeclSAXFunc notationDecl;
    attributeDeclSAXFunc attributeDecl;
    elementDeclSAXFunc elementDecl;
    unparsedEntityDeclSAXFunc unparsedEntityDecl;
    setDocumentLocatorSAXFunc setDocumentLocator;
    startDocumentSAXFunc startDocument;
    endDocumentSAXFunc endDocument;
    startElementSAXFunc startElement;
    endElementSAXFunc endElement;
    referenceSAXFunc reference;
    charactersSAXFunc characters;
    ignorableWhitespaceSAXFunc ignorableWhitespace;
    processingInstructionSAXFunc processingInstruction;
    commentSAXFunc comment;
    warningSAXFunc warning;
    errorSAXFunc error;
    fatalErrorSAXFunc fatalError;

*/

xmlSAXHandler aaSaxCallsback = {
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   cb_startDocument,
                                   cb_endDocument,
                                   cb_startElement,
                                   cb_endElement,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   cb_warning,
                                   cb_error,
                                   cb_fatalError,
                                   NULL,
                                   NULL,
                                   NULL,
                                   1,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL
                               };

int myxmlInputReadFILE (void *context, char *buffer, int len) {
    return fread(buffer, 1, len, (FILE *)context);
}

int myxmlInputCloseFILE (void *context) {
    return (fclose((FILE *)context) == 0) ? 0 : -1;
}

static bool sg_IgnoreRequest( tString const & URI )
{
#ifdef WIN32
    return URI.EndsWith( "../etc/catalog" );
#else
    return URI.StartsWith( "file:///" ) && strstr( URI, "xml" ) && URI.EndsWith( "catalog" );
#endif
}

#ifndef HAVE_LIBXML2_WO_PIBCREATE
xmlParserInputBufferPtr myxmlParserInputBufferCreateFilenameFunc (const char *URI, xmlCharEncoding enc) {
    if ( sg_IgnoreRequest( tString( URI ) ) )
    {
#ifdef DEBUG
        printf("Ignoring xml request for %s\n", URI);
#endif
        return NULL;
    }
#ifdef DEBUG
    //  con << "xml wants " << URI << "\n";
#endif
    FILE *f = tResourceManager::openResource(URI, NULL);
    if (f == NULL)
        return NULL;
    xmlParserInputBufferPtr ret = xmlAllocParserInputBuffer(enc);
    ret->context = f;
    ret->readcallback = myxmlInputReadFILE;
    ret->closecallback = myxmlInputCloseFILE;
    return ret;
}
#endif

void tXmlParser::cb_startDocument() {
    startDocument();
}

void tXmlParser::startDocument() {
    // Do nothing
}

void tXmlParser::cb_endDocument() {
    endDocument();
}

void tXmlParser::endDocument() {
    // Do nothing
}

void tXmlParser::cb_startElement(const xmlChar* name, const xmlChar** attrs) {
    tAttributeList attributes;
    if (attrs != NULL) {
        for (int i = 0; (attrs[i] != NULL); i++) {
            tString attributeName( (const char*) attrs[i] );
            tString attributeValue;
            i++;

            if (attrs[i] != NULL) {
                attributeValue = tString( (const char*)attrs[i] );
            } else {
                attributeValue = tString();
            }
            attributes[attributeName] = attributeValue;
        }
        tString elementName((const char*)name);
        startElement(elementName, attributes );
    }
}

void tXmlParser::startElement(tString &element, tAttributeList &attributes) {
    // Do nothing
}

tXmlParser::~tXmlParser() {
    // do nothing
}

void tXmlParser::cb_endElement(const xmlChar *name) {
    tString elementName((const char*)name);
    endElement(elementName);
}

void tXmlParser::endElement(tString &element) {
    // Do nothing
}

bool tXmlParser::LoadWithoutParsing(const char* filename, const char* uri) {
    bool success=false;
    FILE* docfd;

    docfd = tResourceManager::openResource(filename, uri);
    m_Filename = tResourceManager::locateResource(filename, uri);

    if ( docfd )
    {
        success = ValidateXml(docfd, uri, filename);
        fclose(docfd);
    }

    return success;
}

bool tXmlParser::LoadWithParsing(const char* filename, const char *uri) {
    bool success=false;
    FILE* docfd;

    if(!(docfd = tResourceManager::openResource(filename, uri))) {
        con << "Loading XML file '" << filename << "' failed!\n";
        return false;
    }
    m_Filename = tResourceManager::locateResource(filename, uri);

    success = ValidateXml(docfd, uri, filename);
    fclose(docfd);
    if(success) {
        return Parse();
    } else {
        return false;
    }
}

bool tXmlResource::LoadFile(const char* filename, const char* uri) {
    m_Filename = tResourceManager::locateResource(filename, uri);
    return LoadXmlFile(m_Filename, uri);
}

bool tXmlParser::LoadFile(const char* filename, const char* uri) {
    return LoadXmlFile(filename, uri);
}

bool tXmlParser::LoadXmlFile(const char* filename, const char* uri) {
    bool goOn;

    FILE* docfd;

    docfd = fopen(filename, "r");

    goOn = ValidateXml(docfd, uri, filename);

    if(goOn) {
        return Parse();
    } else {
        return false;
    }
}

bool tXmlParser::Parse() {
    if(m_Mode == DOM) {
        return ParseDom();
    } else {
        return ParseSax();
    }
}

// Subclasses need to override this, default method provided that does nothing
bool tXmlParser::ParseDom() {
    return true;
}

bool tXmlParser::ParseSax() {
    if (xmlSAXUserParseFile(&aaSaxCallsback, this, m_Filename) < 0) {
        return false;
    } else
        return true;
}

// gets a path segment from a string, appending a "/" if none is there and the path is not empty
static tString st_GetPathSegment( tString const &segment )
{
    tString ret(segment);

    if ( ret.Size() > 0 && ret[ ret.Size()-1 ] != '/' )
        ret += '/';

    return ret;
}

#ifndef DEDICATED
static tString st_errorLeadIn("");

static void st_ErrorFunc( void * ctx,
                          const char * msg,
                          ... )
{
    // print formatted message into buffer
    static int maxlen = 100;
    tArray<char> buffer;
    bool retry = true;
    while ( retry )
    {
        buffer.SetLen( maxlen );
        va_list ap;
        va_start(ap, msg);
        retry = vsnprintf(&buffer[0], maxlen, msg, ap) >= maxlen;
        va_end(ap);

        if ( retry )
            maxlen *= 2;
    }
    char * message = &buffer[0];

    // print buffer to stderr and console
    if ( st_errorLeadIn.Len() > 2 )
    {
        con << st_errorLeadIn;
#ifndef DEBUG
        std::cerr << st_errorLeadIn;
#endif
        st_errorLeadIn = "";
    }

#ifndef DEBUG
    std::cerr << message;
#endif

    con << message;
}
#endif

bool tXmlParser::ValidateXml(FILE* docfd, const char* uri, const char* filepath)
{
#ifndef DEDICATED
    /* register error handler */
    xmlGenericErrorFunc errorFunc = &st_ErrorFunc;
    initGenericErrorDefaultFunc( &errorFunc );
    st_errorLeadIn = "XML validation error in ";
    st_errorLeadIn += filepath;
    st_errorLeadIn += ":\n\n";
#endif

    bool validated = false;

    if (docfd == NULL) {
        printf("LoadAndValidateMapXML passed a NULL docfd (we should really trap this somewhere else!)\n");
        return false;
    }

#ifndef HAVE_LIBXML2_WO_PIBCREATE
    //xmlSetExternalEntityLoader(myxmlResourceEntityLoader);
    xmlParserInputBufferCreateFilenameDefault(myxmlParserInputBufferCreateFilenameFunc);    //should be moved to some program init area
#endif

    if (m_Doc)
    {
        xmlFreeDoc(m_Doc);
        m_Doc=0;
    }

    /*Validate the xml*/
    xmlParserCtxtPtr ctxt; /*Parser context*/

    ctxt = xmlNewParserCtxt();

    if (ctxt == 0) {
        fprintf(stderr, "Failed to allocate parser context\n");
        return false;
    }

    /* parse the file, activating the DTD validation option */
    m_Doc = xmlCtxtReadIO(ctxt, myxmlInputReadFILE, 0, docfd,
#if HAVE_LIBXML2_WO_PIBCREATE
                          (const char *)tDirectories::Resource().GetReadPath("map-0.1.dtd")
                          // TODO: don't hardcode the file
#else
                          uri
#endif
                          , NULL, XML_PARSE_DTDVALID);
    // NOTE: Do *not* pass myxmlInputCloseFILE; we close the file *later*

    /* check if parsing suceeded */
    if (m_Doc == NULL) {
        fprintf(stderr, "Failed to parse \n");
    } else {
        /* check if validation suceeded */
        if (ctxt->valid == 0) {
            fprintf(stderr, "Failed to validate \n");
            xmlFreeDoc(m_Doc);
            m_Doc=NULL;
        }
        else
        {
            validated = true;
        }
    }

    /* free up the parser context */
    xmlFreeParserCtxt(ctxt);

#ifndef DEDICATED
    /* reset error handler */
    initGenericErrorDefaultFunc( NULL );
#endif
    return validated;
}

bool tXmlResource::ValidateXml(FILE* docfd, const char* uri, const char* filepath) {
    bool validated = tXmlParser::ValidateXml(docfd, uri, filepath);

    /* check filepath */
    if ( validated && filepath )
    {
        node root = GetRoot();

        if (!root) {
            con << "Empty document\n";
            return false;
        } else if (root.IsOfType("Resource")) {
            m_Author   = root.GetProp("author");
            m_Category = root.GetProp("category");
            m_Name     = root.GetProp("name");
            m_Version  = root.GetProp("version");
            m_Type     = root.GetProp("type");
            tString rightFilepath = st_GetPathSegment(m_Author);
            rightFilepath << st_GetPathSegment(m_Category)
            << m_Name << "-"
            << m_Version << "."
            << m_Type << ".xml";

            /* remove extra double // */

            tString pureFilepath( filepath );
            int paren = pureFilepath.StrPos( "(" );
            if ( paren > 0 )
            {
                pureFilepath = pureFilepath.SubStr( 0, paren );
            }
            if ( rightFilepath != pureFilepath )
            {
                con << "\nWARNING: incorrect filepath. The resource wants to be at \"" << rightFilepath << "\", but was loaded from \"" << filepath << "\".\n\n";
            }
        }
        else {
            con << "Root node is not of type 'Resource' but '" << root.GetName() << "'.\n";
            return false;
        }
    }

    return validated;
}

tXmlParser::node tXmlResource::GetFileContents(void) {
    for(node cur = GetRoot().GetFirstChild(); cur; ++cur) {
        if(!cur.IsOfType("comment") && !cur.IsOfType("text")) {
            return cur;
        }
    }
    return 0;
}

tXmlParser::node tXmlParser::GetRoot() {
    return node(xmlDocGetRootElement(m_Doc));
}

//! @param cur The node this object should be constructed around
tXmlParser::node::node(xmlNode *cur) : m_cur(cur) {
    //tASSERT(m_cur);
}

//! @param name The name to be checked
//! @returns true if the name matches, false if it doesn't. The conversion is case sensitive.
bool tXmlParser::node::IsOfType(CHAR const *name) const {
    tASSERT(m_cur);
    return(!xmlStrcmp(m_cur->name, reinterpret_cast<xmlChar const *>(name)));
}

//! @returns the name (type) of the node
tString tXmlParser::node::GetName(void) const {
    tASSERT(m_cur);
    return tString(reinterpret_cast<const char *>(m_cur->name));
}

//! This function prints a warning if the attribute doesn't exist
//! @param prop The name of the attribute to be read
//! @returns The property as a string
tString tXmlParser::node::GetProp(CHAR const *prop) const {
    tASSERT(m_cur);
    xmlChar *val = xmlGetProp(m_cur,
                              reinterpret_cast<const xmlChar *>
                              (prop)
                             );
    if(val == 0) {
        tERR_WARN(tString("Call for non- existent Attribute '") + tString(prop) + "' of element of type '" + GetName() + '"');
        st_Breakpoint();
        return tString();
    }
    tString ret(reinterpret_cast<const char *>(val));
    xmlFree(val);
    return(ret);
}

//! This function prints a warning if the attribute doesn't exist
//! @param prop The name of the attribute to be read
//! @returns true if the attribute starts with one of [tTyY] (true, True, yes, Yes) or if it equeals "on" or if it converts completely to an integer that in not 0
bool tXmlParser::node::GetPropBool(CHAR const *prop) const {
    tString string(GetProp(prop));
    if (string.empty()) return false;
    switch(string[0]) {
case 't': case 'T':
case 'y': case 'Y':
        return true;
    default:
        if(string == "on") return true;
        int i;
        return string.Convert(i) && i;
    }
}

//! @returns The node as it was before the incrementation
tXmlParser::node &tXmlParser::node::operator++() {
    tASSERT(m_cur);
    m_cur=m_cur->next;
    return *this;
}
//! @returns The node as it is after the incrementation
tXmlParser::node const tXmlParser::node::operator++(int) {
    tASSERT(m_cur);
    xmlNode *old = m_cur;
    m_cur=m_cur->next;
    return old;
}

//! @returns A node pointing to the first child
tXmlParser::node tXmlParser::node::GetFirstChild(void) const {
    tASSERT(m_cur);
    return m_cur->xmlChildrenNode;
}

//! @returns true if the object exists, false if it doesn't (any operations on this node will segfault)
tXmlParser::node::operator bool() const {
    return(m_cur != 0);
}

}

#ifdef _MSC_VER
void tXmlParser::node::GetProp(CHAR const *prop, int &target) const {
    if(!(GetProp(prop).Convert(target))) {
        tERR_WARN( "Property '" + tString(prop) + "' of node of type '" + GetName() + "' is '" + GetProp(prop) + "' which isn't of type '" + typeid(int).name() + "' as needed.");
    }
}
void tXmlParser::node::GetProp(CHAR const *prop, REAL &target) const {
    if(!(GetProp(prop).Convert(target))) {
        tERR_WARN( "Property '" + tString(prop) + "' of node of type '" + GetName() + "' is '" + GetProp(prop) + "' which isn't of type '" + typeid(REAL).name() + "' as needed.");
    }
}
#endif

//! @class tXmlParserNamespace::tXmlParser::node
//! Basic example of use (within a child of tXmlResource):
//!
//! <pre>
//! node cur = GetFileContents(); // node now points to the first child of the Resource tag, which is most likely a text node
//! for (; cur; ++cur) { // iterate throgh the following nodes to find the one we're looking for (ignoring all comments and text nodes)
//!     if(cur.IsOfType("whateveryouwant")) { // check if this is the right node
//!       &nbsp;// ok, it is the right node, now we want to parse all children and get their "height" property as floats, their "show" property as booleans, and their "name" property as a plain string
//!         for(node child = cur.GetFirstChild; child; ++child) {
//!             float height;
//!             cur.GetProp("height", height);
//!             bool show = cur.GetPropBool("show");
//!             tString name = cur.GetProp("name");
//!            &nbsp;// do something with that info...
//!         }
//!         break; // break out of the loop, we found what we were looking for
//!     }
//! }
//! </pre>
