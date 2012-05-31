#include "rSDL.h"

#include "config.h"

#include "gParser.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include<stdarg.h>

#include "eCoord.h"
#include "eAdvWall.h"
#include "eGrid.h"
#include "eTess2.h"
#include "gWall.h"
#include "gArena.h"
#include "gWinZone.h"
#include "gSpawn.h"
#include "tMemManager.h"
#include "tResourceManager.h"
#include "tRecorder.h"
#include "tConfiguration.h"

#include "gGame.h"

#include "tConfiguration.h"
#include "gGame.h"

// This fixes stupid crappy OS-wannabe Windoze
// Someday, autoconf-ize this stuff...
#ifdef WIN32
#	define strncasecmp strnicmp
#	define vsnprintf _vsnprintf
#endif

#ifdef __MINGW32__
#define xmlFree(x) free(x)
#endif

#if HAVE_LIBXML2_WO_PIBCREATE
#	include "tDirectories.h"
#endif

// lean auto-deleting wrapper class for xmlChar * return values the user has to clean after use
class gXMLCharReturn
{
public:
    gXMLCharReturn( xmlChar * string = NULL ): string_( string ){}
    ~gXMLCharReturn(){ Destroy(); }

    //! auto_ptrish copy constructor
    gXMLCharReturn( gXMLCharReturn const & other ): string_( const_cast< gXMLCharReturn & >( other ).Release() ){}

    //! auto_ptrish assignment operator
    gXMLCharReturn & operator = ( gXMLCharReturn & other ){ string_ = other.Release(); return *this; }

    //! conversion to char *
    operator char * () const{ return reinterpret_cast< char * >( string_ ); }

    //! conversion to xmlChar *( bad idea, causes overload trouble )
    // operator xmlChar * () const{ return string_; }

    //! releases ownership of the string and returns it
    xmlChar * Release(){ xmlChar * ret = string_; string_ = 0; return ret; }

    //! returns the xml string without modifying it
    xmlChar * GetXML() const { return string_; }

    //! returns the string without modifying it
    char * Get() const { return *this; }

    //! destroys the string
    void Destroy(){ xmlFree(string_); string_ = 0; }
private:
    xmlChar * string_; //!< the string storage
};

//! Warn about deprecated map format
static void sg_Deprecated()
{
    // no use informing players on the client, they can't do anything about it anyway
    if ( sn_GetNetState() != nCLIENT )
        con << "\n\n" << tColoredString::ColorString(1,.3,.3) << "WARNING: You are loading a map that is written in a deprecated format. It should work for now, but will stop to do so in one of the next releases. Have the map upgraded to an up-to-date version as soon as possible.\n\n";
}

gParser::gParser(gArena *anArena, eGrid *aGrid)
{
    theArena = anArena;

    theGrid = aGrid;
    doc = NULL;
    rimTexture = 0;
}

gParser::~gParser()
{
    if (doc)
    {
        xmlFreeDoc(doc);
        doc=NULL;
    }
}

bool
gParser::trueOrFalse(char *str)
{
    // This will work with true/false/yes/no/1/-1/0/etc
    return (!strncasecmp(str, "t", 1) || !strncasecmp(str, "y", 1) || atoi(str));
}

gXMLCharReturn
gParser::myxmlGetProp(xmlNodePtr cur, const char *name) {
    return gXMLCharReturn( xmlGetProp(cur, (const xmlChar *)name) );
}

int
gParser::myxmlGetPropInt(xmlNodePtr cur, const char *name) {
    gXMLCharReturn v ( myxmlGetProp(cur, name) );
    if (v.Get() == NULL)	return 0;
    int r = atoi(v);
    return r;
}

float
gParser::myxmlGetPropFloat(xmlNodePtr cur, const char *name) {
    gXMLCharReturn v = myxmlGetProp(cur, name);
    if (v.Get() == NULL)	return 0.;
    float r = atof(v);
    return r;
}

bool
gParser::myxmlGetPropBool(xmlNodePtr cur, const char *name) {
    gXMLCharReturn v = myxmlGetProp(cur, name);
    if (v.Get() == NULL)	return false;
    bool r = trueOrFalse(v);
    return r;
}

tString
gParser::myxmlGetPropString(xmlNodePtr cur, const char *name) {
    gXMLCharReturn v = myxmlGetProp(cur, name);
    if (v.Get() == NULL) return tString("");
    tString r(v);
    return r;
}

#define myxmlHasProp(cur, name)	xmlHasProp(cur, reinterpret_cast<const xmlChar *>(name))

/*
 * Determine if elementName is the same as searchedElement, or any of its valid syntax.
 * Anything sharing the same start counts as a valid syntax. This allow for variation
 * on the name to reduce DTD conflicts.
 */
bool
gParser::isElement(const xmlChar *elementName, const xmlChar *searchedElement, const xmlChar * keyword) {
    bool valid = false;
    if (xmlStrcmp(elementName, searchedElement) == 0) {
        valid = true;
    }
    else {
        if (keyword != NULL) {
            xmlChar * searchedElementAndKeyword = xmlStrdup(searchedElement);
            searchedElementAndKeyword = xmlStrcat(searchedElementAndKeyword, keyword);
            if (xmlStrcmp(elementName, searchedElementAndKeyword) == 0) {
                valid = true;
            }
            xmlFree (searchedElementAndKeyword);
        }
    }

    return valid;
}

/*
 * Determine if this is an alternative for us. To be an alternative for us, the
 * current element's name must starts with Alternative, and the version attribute
 * has a version that is ours ("Arthemis", "0.2.8" or "0_2_8"). If both conditions
 * are met, it return true.
 */
bool
gParser::isValidAlternative(xmlNodePtr cur, const xmlChar * keyword) {
    gXMLCharReturn version = myxmlGetProp(cur, "version");
    /*
     * Find non empty version and
     * Alternative element, ie those having a name starting by "Alternative" and
     * only the Alternative elements that are for our version, ie Arthemis
     */
    return ((version.Get() != NULL) && isElement(cur->name, (const xmlChar *)"Alternative", keyword) && validateVersionRange(version.GetXML(), (const xmlChar*)"Artemis", (const xmlChar*)"0.2.8.0") );
}

bool
gParser::isValidCodeName(const xmlChar *version)
{
    const int NUMBER_NAMES = 24;
    xmlChar const *const names [] = { (const xmlChar*) "Artemis", (const xmlChar*) "Bachus", (const xmlChar*) "Cronus", (const xmlChar*) "Demeter", (const xmlChar*) "Epimetheus", (const xmlChar*) "Furiae", (const xmlChar*) "Gaia", (const xmlChar*) "Hades", (const xmlChar*) "Iapetos", (const xmlChar*) "Juno", (const xmlChar*) "Koios", (const xmlChar*) "Leto", (const xmlChar*) "Mars", (const xmlChar*) "Neptune", (const xmlChar*) "Okeanos", (const xmlChar*) "Prometheus", (const xmlChar*) "Rheia", (const xmlChar*) "Selene", (const xmlChar*) "Thanatos", (const xmlChar*) "Uranus", (const xmlChar*) "Vulcan", (const xmlChar*) "Wodan", (const xmlChar*) "Yggdrasil", (const xmlChar*) "Zeus"};
    /* Is is a valid codename*/
    int i;
    for (i=0; i<NUMBER_NAMES; i++)
    {
        if (xmlStrcmp(version, names[i]) == 0)
            return true;
    }

    return false;
}

bool
gParser::isValidDotNumber(const xmlChar *version)
{
    char * work = (char *) xmlStrdup(version);
    char * start = work;
    char * end = work;
    bool valid = false;
    /* Check that we have at least i.j.k.xxx */
    for (int i=0; i<3; i++)
    {
        Ignore( strtol(start, &end, 10) );
        if (start != end) {
            valid = true;
            start = end +1;
        }
        else {
            valid = false;
            break;
        }
    }
    xmlFree(work);
    return valid;
}

/* Are we within a range */
bool
gParser::validateVersionSubRange(const xmlChar *subRange, const xmlChar *codeName, const xmlChar *dotVersion)
{
    bool valid = false;
    int posDelimiter = xmlUTF8Strloc(subRange, (const xmlChar *) "-");
    if (posDelimiter != -1)
    {
        xmlChar * pre = xmlUTF8Strsub(subRange, 0, posDelimiter);
        xmlChar * post = xmlUTF8Strsub(subRange, posDelimiter + 1, xmlUTF8Strlen(subRange) - posDelimiter - 1 );

        if (xmlStrlen(pre) == 0)
            /* The range is of type <empty>-<something>, so we pass the first part */
            valid = true;
        else
        {
            if (isValidCodeName(pre))
                valid = ( xmlStrcmp(pre, codeName) <= 0 );
            else if (isValidDotNumber(pre))
                valid = ( xmlStrcmp(pre, dotVersion) <= 0 );
        }

        if (xmlStrlen(post) == 0)
            /* Reject ranges of types <something>-<empty>*/
            valid = true;
        else
        {
            if (isValidCodeName(post))
                valid &= ( xmlStrcmp(post, codeName) >= 0 );
            else if (isValidDotNumber(post))
                valid &= ( xmlStrcmp(post, dotVersion) >= 0 );
        }
    }
    else
    {
        if (isValidCodeName(subRange))
            valid = ( xmlStrcmp(subRange, codeName) == 0 );
        else if (isValidDotNumber(subRange))
            valid = ( xmlStrcmp(subRange, dotVersion) == 0 );
    }

    return valid;
}

/*
 * Substitute a xmlChar string searchPattern to a xmlChar string replacePattern
 */
bool
gParser::xmlCharSearchReplace(xmlChar *&original, const xmlChar * searchPattern, const xmlChar * replace)
{
    int pos;

    int lenSearchPattern = xmlUTF8Strlen(searchPattern); /* number of character */
    int sizeSearchPattern = xmlUTF8Strsize(searchPattern, lenSearchPattern); /* number of bytes required*/

    /* Ugly hack as many function are lacking in string manipulation for xmlChar * */
    /* xmlUTF8Strloc can only find the location of a single character, and no xmlUTF8Str... function
       can return the location of a string in character position */
    /* We are looking for the first instance searchPattern and want its position in character */
    for (pos=0; pos<xmlUTF8Strlen(original); pos++)
    {
        if ( xmlStrncmp( xmlUTF8Strpos(original, pos), searchPattern, sizeSearchPattern) == 0 )
        {
            int count = xmlUTF8Strlen(original);
            xmlChar * pre = xmlUTF8Strsub(original, 0, pos);
            xmlChar * post = xmlUTF8Strsub(original, pos + lenSearchPattern, count - pos - lenSearchPattern );

            xmlFree(original);
            pre = xmlStrcat ( pre, replace);
            original = xmlStrcat ( pre, post );
            xmlFree(post);
            return true;
        }
    }

    return false;
}


/*Separates all the comma delimited ranges*/
int
gParser::validateVersionRange(xmlChar *version, const xmlChar * codeName, const xmlChar * dotVersion)
{
    xmlChar * copy = xmlStrdup(version);

    /* We allow numerical version to be expressed with . or _, cut down one to simplify treatment */
    while ( xmlCharSearchReplace(copy, (const xmlChar *) "_", (const xmlChar *) ".") ) {} ;

    /* Eliminate all the white space */
    while ( xmlCharSearchReplace(copy, (const xmlChar *) " ", (const xmlChar *) "") ) {} ;

    xmlChar * remain = copy;
    bool valid = false;
    int pos=0;
    while (xmlStrlen(remain) && valid == false)
    {
        if ( ( pos = xmlUTF8Strloc(remain, (const xmlChar *) ",")) == -1)
        {
            /* This is the last sub-range to explore */
            pos = xmlUTF8Strlen(remain);
        }
        xmlChar * subRange = xmlUTF8Strndup(remain, pos);

        /* Is the current version in the presented range */
        valid |= validateVersionSubRange(subRange, codeName, dotVersion);
        xmlFree(subRange);

        /* Move away from the zone explored*/
        remain = const_cast<xmlChar *>( xmlUTF8Strpos(remain, pos + 1) );
    }

    xmlFree(copy);
    return valid;
}

/*
 * This method allows for elements that are at the bottom of the
 * hierarchy, such as Spawn, Point, Setting and Axis, to verify if sub element
 * havent been added in future version(s)
 */
void
gParser::endElementAlternative(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) {
    /* Verify if any sub elements are included, and if they contain any Alt
       Sub elements of Spawn arent defined in the current version*/
    cur = cur->xmlChildrenNode;
    while( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
}

void
gParser::myxmlGetDirection(xmlNodePtr cur, float &x, float &y)
{
    if (myxmlHasProp(cur, "angle")) {
        float angle = myxmlGetPropFloat(cur, "angle") * M_PI / 180.0;
        float speed = myxmlGetPropFloat(cur, "length");
        x = cosf(angle) * speed;
        y = sinf(angle) * speed;
    } else {
        x = myxmlGetPropFloat(cur, "xdir");
        y = myxmlGetPropFloat(cur, "ydir");
    }
}

void
gParser::parseAxes(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    int number;
    int normalize;

    number = myxmlGetPropInt(cur, "number");
    if (number < 1)
        return; // 1 axis is one-way. Keep it.

    normalize = myxmlGetPropBool(cur, "normalize");

    grid->SetWinding(number);

    cur = cur->xmlChildrenNode;
    if (cur != NULL)
    {
        int index = 0;
        eCoord *axisDir;
        axisDir = (eCoord *)malloc(sizeof(eCoord) * number);
        for(int i=0; i<number; i++){
            axisDir[i] = eCoord(1,0);
        }

        while (cur!= NULL) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
            else if (isElement(cur->name, (const xmlChar *)"Axis", keyword)) {
                if (index < number) {
                    myxmlGetDirection(cur, axisDir[index].x, axisDir[index].y);
                    index++;
                }
                else {
                    con << "Invalid index #" << index << "\n";
                }
                /* Verify if any sub elements are included, and if they contain any Alt
                   Sub elements of Point arent defined in the current version*/
                endElementAlternative(grid, cur, keyword);
            }
            else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
                if (index < number) {
                    axisDir[index].x = myxmlGetPropFloat(cur, "x");
                    axisDir[index].y = myxmlGetPropFloat(cur, "y");
                    index++;
                }
                else {
                    con << "Invalid index #" << index << "\n";
                }
                /* Verify if any sub elements are included, and if they contain any Alt
                   Sub elements of Point arent defined in the current version*/
                endElementAlternative(grid, cur, keyword);
            }
            else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
                if (isValidAlternative(cur, keyword)) {
                    parseAlternativeContent(grid, cur);
                }
            }
            cur = cur->next;
        }
        grid->SetWinding(number, axisDir, normalize);
        free(axisDir);
    }
}

void
gParser::parseSpawn(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    float x, y, xdir, ydir;

    x = myxmlGetPropFloat(cur, "x");
    y = myxmlGetPropFloat(cur, "y");
    myxmlGetDirection(cur, xdir, ydir);

    eCoord rootPos = eCoord(x, y);
    eCoord rootDir = eCoord(xdir, ydir);
    gSpawnPoint *sp = tNEW(gSpawnPoint)(rootPos, rootDir);
    for(xmlNodePtr child = cur->xmlChildrenNode; child; child = child->next) {
        if(!isElement(child->name, (const xmlChar *)"Spawn", keyword)) {
            continue;
        }

        x = myxmlGetPropFloat(child, "x");
        y = myxmlGetPropFloat(child, "y");
        myxmlGetDirection(child, xdir, ydir);

        eCoord pos = eCoord(x, y);
        eCoord dir = eCoord(xdir, ydir);

        if(xdir == 0 && ydir == 0) {
            // no direction given (at least no sensible one)
            // assume the same direction as the root spawn point
            dir = rootDir;
        }

        sp->AddSubSpawn(pos, dir);
    }
    theArena->NewSpawnPoint(sp);

    endElementAlternative(grid, cur, keyword);
}

bool
gParser::parseShapeCircle(eGrid *grid, xmlNodePtr cur, float &x, float &y, float &radius, float& growth, const xmlChar * keyword)
{
    radius = myxmlGetPropFloat(cur, "radius");
    growth = myxmlGetPropFloat(cur, "growth");

    cur = cur->xmlChildrenNode;
    while( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            x = myxmlGetPropFloat(cur, "x");
            y = myxmlGetPropFloat(cur, "y");

            endElementAlternative(grid, cur, keyword);
            return true;
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
    return false;
}

void
gParser::parseZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
    float x, y, radius, growth;
    bool shapeFound = false;
    gZone * zone = NULL;
    xmlNodePtr shape = cur->xmlChildrenNode;

    while(shape != NULL && shapeFound==false) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(shape->name, (const xmlChar *)"ShapeCircle", keyword)) {
            shapeFound = parseShapeCircle(grid, shape, x, y, radius, growth, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        shape = shape->next;
    }

    int pos = 0;
    tString zoneNamestr = myxmlGetPropString(cur, "name");
    gRealColor ZoneColor;

    if (sn_GetNetState() != nCLIENT )
    {
        if (!xmlStrcmp(myxmlGetProp(cur, "effect").GetXML(), (const xmlChar *)"win")) {
            zone = tNEW( gWinZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(myxmlGetProp(cur, "effect").GetXML(), (const xmlChar *)"death")) {
            tString zoneTeamStr = myxmlGetPropString(cur, "team");
            eTeam *zoneTeam=NULL;
            gRealColor zoneColor;
            for (int i = eTeam::teams.Len() - 1; i>=0; --i) {
                tString teamName = eTeam::teams(i)->Name();
                if (zoneTeamStr==ePlayerNetID::FilterName(teamName)) {
                    zoneTeam = eTeam::teams(i);
                    break;
                }
            }

            if (zoneTeam) {
                zoneColor.r = zoneTeam->R()/15.0;
                zoneColor.g = zoneTeam->G()/15.0;
                zoneColor.b = zoneTeam->B()/15.0;

                zone = tNEW( gDeathZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier), true, zoneTeam );
                zone->SetColor(zoneColor);
            }
            else {
                zone = tNEW( gDeathZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
            }
        }
        else if (!xmlStrcmp(myxmlGetProp(cur, "effect").GetXML(), (const xmlChar *)"fortress")) {
            tString zoneTeamStr = myxmlGetPropString(cur, "team").ExtractNonBlankSubString(pos);
            eTeam *zoneTeam=NULL;
            gRealColor zoneColor;
            for (int i = eTeam::teams.Len() - 1; i>=0; --i) {
                tString teamName = eTeam::teams(i)->Name();
                if (zoneTeamStr==ePlayerNetID::FilterName(teamName)) {
                    zoneTeam = eTeam::teams(i);
                    break;
                }
            }

            if (zoneTeam) {
                zoneColor.r = zoneTeam->R()/15.0;
                zoneColor.g = zoneTeam->G()/15.0;
                zoneColor.b = zoneTeam->B()/15.0;

                zone = tNEW( gBaseZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier), true, zoneTeam );
                zone->SetColor(zoneColor);
            }
            else {
                zone = tNEW( gBaseZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
            }
        }
        else if (!xmlStrcmp(myxmlGetProp(cur, "effect").GetXML(), (const xmlChar *)"sumo")) {
            zone = tNEW( gSumoZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"flag")) {
            tString zoneTeamStr = myxmlGetPropString(cur, "team").ExtractNonBlankSubString(pos);
            eTeam *zoneTeam=NULL;
            gRealColor zoneColor;
            for (int i = eTeam::teams.Len() - 1; i>=0; --i) {
                tString teamName = eTeam::teams(i)->Name();
                if (zoneTeamStr==ePlayerNetID::FilterName(teamName)) {
                    zoneTeam = eTeam::teams(i);
                    break;
                }
            }

            if (zoneTeam) {
                zoneColor.r = zoneTeam->R()/15.0;
                zoneColor.g = zoneTeam->G()/15.0;
                zoneColor.b = zoneTeam->B()/15.0;

                zone = tNEW( gFlagZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier), true, zoneTeam );
                zone->SetColor(zoneColor);
            }
            else {
                zone = tNEW( gFlagZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
            }
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"ball")) {
            tString zoneTeamStr = myxmlGetPropString(cur, "team").ExtractNonBlankSubString(pos);
            eTeam *zoneTeam=NULL;
            gRealColor zoneColor;
            for (int i = eTeam::teams.Len() - 1; i>=0; --i)
            {
                tString teamName = eTeam::teams(i)->Name();
                if (zoneTeamStr==ePlayerNetID::FilterName(teamName))
                {
                    zoneTeam = eTeam::teams(i);
                    break;
                }
            }

            if (zoneTeam)
            {
                zoneColor.r = zoneTeam->R()/15.0;
                zoneColor.g = zoneTeam->G()/15.0;
                zoneColor.b = zoneTeam->B()/15.0;

                zone = tNEW( gBallZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier), true, zoneTeam );
                zone->SetColor(zoneColor);
            }
            else
            {
                zone = tNEW( gBallZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
            }
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"target")) {
            zone = tNEW( gTargetZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"blast")) {
            zone = tNEW( gBlastZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *) "effect"), (const xmlChar *)"rubber")) {
            REAL rubberVal = myxmlGetPropFloat(cur, "rubberVal");
            if (rubberVal != 0.0 ){
                gRubberZoneHack *rZone= tNEW(gRubberZoneHack(grid, eCoord(x*sizeMultiplier,y*sizeMultiplier), false ));
                rZone->SetRubber(rubberVal);
                zone = rZone;
            }

        }

        // leaving zone undeleted is no memory leak here, the gid takes control of it
        if ( zone )
        {
            zone->SetRadius( radius*sizeMultiplier );
            zone->SetExpansionSpeed( growth*sizeMultiplier );
            zone->SetRotationSpeed( .3f );
            zone->RequestSync();
            if (zoneNamestr) zone->SetName(zoneNamestr);
            //zone->SetColor();
        }
    }
}

ePoint * gParser::DrawRim( eGrid * grid, ePoint * start, eCoord const & stop, REAL h )
{
    // calculate the wall's length and the rim wall textures
    REAL length = (stop-(*start)).Norm();
    REAL rimTextureStop = rimTexture + length;

    // create wall
    tJUST_CONTROLLED_PTR< gWallRim > newWall = tNEW( gWallRim )(grid, rimTexture, rimTextureStop, h);

    // update rim texture
    rimTexture = rimTextureStop;

    // draw line with wall
    return grid->DrawLine( start, stop, newWall, 0 );
}

void
gParser::parseWallLine(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) {
    REAL ox, oy, x, y;
    ePoint *R;

    ox = myxmlGetPropFloat(cur, "startx");	oy = myxmlGetPropFloat(cur, "starty");
    x = myxmlGetPropFloat(cur,   "endx");	 y = myxmlGetPropFloat(cur,   "endy");
    R = grid->Insert(eCoord(ox, oy) * sizeMultiplier);
    R = this->DrawRim(grid, R, eCoord(x, y) * sizeMultiplier);
    sg_Deprecated();

    endElementAlternative(grid, cur, keyword);
}

void
gParser::parseWallRect(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) {
    REAL ox, oy, x, y;
    ePoint *R;

    ox = myxmlGetPropFloat(cur, "startx");	oy = myxmlGetPropFloat(cur, "starty");
    x = myxmlGetPropFloat(cur,   "endx");	 y = myxmlGetPropFloat(cur,   "endy");
    R = grid->Insert(eCoord(ox, oy) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord( x, oy) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord( x,  y) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord(ox,  y) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord(ox, oy) * sizeMultiplier);
    sg_Deprecated();

    endElementAlternative(grid, cur, keyword);
}

extern std::vector<ePolyLine> se_unsplittedRimWalls;

void
gParser::parseWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    ePoint *R = NULL, *sR = NULL;
#ifdef DEBUG
    REAL ox, oy;
#endif
    REAL x, y;

    REAL height = myxmlGetPropFloat(cur, "height");
    if ( height <= 0 )
        height = 10000;

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            x = myxmlGetPropFloat(cur, "x");
            y = myxmlGetPropFloat(cur, "y");

            if (R == NULL) {
                R = grid->Insert(eCoord(x, y) * sizeMultiplier);
                ePolyLine P('M',eCoord(x, y) * sizeMultiplier);
                se_unsplittedRimWalls.push_back(P);
            } else {
                R = this->DrawRim(grid, R, eCoord(x, y) * sizeMultiplier, height);
                ePolyLine P('L',eCoord(x, y) * sizeMultiplier);
                se_unsplittedRimWalls.push_back(P);
			}

            // TODO-Alt:
            // if this function returns a point, use it in the wall. Otherwise, ignore what comes out.
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"SavePos", keyword)) {
            sR = R;
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"RestorePos", keyword)) {
            R = sR;
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Line", keyword)) {
            parseWallLine(grid, cur, keyword);
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Rectangle", keyword)) {
            parseWallRect(grid, cur, keyword);
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
#ifdef DEBUG
        ox = x;	oy = y;
#endif
    }
}

void
gParser::parseObstacleWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    ePoint *R = NULL;
    REAL x, y;

    REAL height = myxmlGetPropFloat(cur, "height");
    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            x = myxmlGetPropFloat(cur, "x");
            y = myxmlGetPropFloat(cur, "y");

            if (R == NULL)
                R = grid->Insert(eCoord(x, y) * sizeMultiplier);
            else
                R = this->DrawRim(grid, R, eCoord(x, y) * sizeMultiplier, height );
            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
    sg_Deprecated();
}

/* processSubAlt should be applied to all and any elements, even those that are known not to have any
   sub elements possible. This ensure maximal future compatibility.*/

// TODO-Alt:
// processSubAlt need to be altered. It need to be able to "return" a <Point>, a <ShapeCircle> or NULL.
// This will allow for imbricked elements to contribute to their parents, ie: Wall, Zone's shape and ShapeCircle's Point.
void
gParser::processSubAlt(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) {
    cur = cur->xmlChildrenNode;
    /* Quickly goes through all the sub element until a valid Alternative is found */
    while( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isValidAlternative(cur, keyword)) {
            parseAlternativeContent(grid, cur);
            return; /*We process only the first matching one*/
        }
        cur = cur->next;
    }
}

/* Present a  */
void
gParser::parseAlternativeContent(eGrid *grid, xmlNodePtr cur)
{
    gXMLCharReturn keywordStore = myxmlGetProp(cur, "keyword");
    xmlChar * keyword = keywordStore.GetXML();

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {
            /* Do nothing, but is required to eliminate all Text and Comment element */
            /* text elements are half of any other elements, drop them here rather than perform countless test */
        }
        /* The elements of Field */
        else if (isElement(cur->name, (const xmlChar *)"Axes", keyword)) {
            parseAxes(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Spawn", keyword)) {
            parseSpawn(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Zone", keyword)) {
            parseZone(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Wall", keyword)) {
            parseWall(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"ObstacleWall", keyword)) {
            parseObstacleWall(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Line", keyword)) {
            parseWallLine(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Rectangle", keyword)) {
            parseWallRect(grid, cur, keyword);
        }
        /* The settings */
        else if (isElement(cur->name, (const xmlChar *)"Settings", keyword)) {
            parseSettings(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Setting", keyword)) {
            parseSetting(grid, cur, keyword);
        }
        /* The big holders*/
        else if (isElement(cur->name, (const xmlChar *)"Map", keyword)) {
            parseMap(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"World", keyword)) {
            parseWorld(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Field", keyword)) {
            parseField(grid, cur, keyword);
        }
        /* Those that cant affect the instance directly. They should return something */
        else if (isElement(cur->name, (const xmlChar *)"Axis", keyword)) {
            // TODO-Alt2: A method to read in Axis data and return an "Axis object" to be captured at some other level.
            // The same method could be used inside of the parseAxes
            //            parseAxis(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            // TODO-Alt2: A method to read in Point data and return an "Point object" to be captured at some other level.
            // The same method could be used to read all "Point" in the code
            //            parsePoint(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"ShapeCircle", keyword)) {
            // TODO-Alt2: parseShapeCircle should be modified to return an "ShapeCircle object" to be captured at some other level.
            // The same method could be used inside of the parseZone
            //            parseShapeCircle(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }


        cur = cur->next;
    }
}

void
gParser::parseField(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Axes", keyword)) {
            parseAxes(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Spawn", keyword)) {
            parseSpawn(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Zone", keyword)) {
            parseZone(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Wall", keyword)) {
            parseWall(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"ObstacleWall", keyword)) {
            parseObstacleWall(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Line", keyword)) {
            parseWallLine(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Rectangle", keyword)) {
            parseWallRect(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
}

void
gParser::parseWorld(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Field", keyword)) {
            parseField(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
}

void
gParser::parseSetting(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    if (strlen(myxmlGetProp(cur, "name")) && strlen(myxmlGetProp(cur, "value")) && sn_GetNetState() != nCLIENT )
    {
        std::stringstream ss;
        /* Yes it is ackward to generate a string that will be decifered on the other end*/
        ss << myxmlGetProp(cur, "name")  << " " << myxmlGetProp(cur, "value");
        tConfItemBase::LoadLine(ss);
    }
    /* Verify if any sub elements are included, and if they contain any Alt
       Sub elements of Point arent defined in the current version*/
    endElementAlternative(grid, cur, keyword);
}

void
gParser::parseSettings(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Setting", keyword)) {
            parseSetting(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }

    update_settings();
    sizeMultiplier = gArena::GetSizeMultiplier();
}

void
gParser::parseMap(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Settings", keyword)) {
            parseSettings (grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"World", keyword)) {
            parseWorld (theGrid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
}

void
gParser::InstantiateMap(float aSizeMultiplier)
{
    rimTexture = 0;
    // BOP
    sizeMultiplier = aSizeMultiplier;
    // EOP
    xmlNodePtr cur;
    cur = xmlDocGetRootElement(doc);

    if (cur == NULL) {
        con << "ERROR: Map file is blank\n";
        return;
    }

    if (isElement(cur->name, (const xmlChar *) "Resource")) {
        if (xmlStrcmp((const xmlChar *) "aamap", myxmlGetProp(cur,"type").GetXML())) {
            con << "Type aamap expected, found " << myxmlGetProp(cur, "type") << " instead\n";
            con << "formalise this message\n";
        }
        else {
            cur = cur->xmlChildrenNode;
            while (cur != NULL) {
                if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {
                    /* Do nothing, but is required to eliminate all Text element */
                    /* text elements are half of any other elements, drop them here rather than perform countless test */
                }
                else if (isElement(cur->name, (const xmlChar *)"Map")) {
                    parseMap(theGrid, cur);
                }
                else if (isElement(cur->name, (const xmlChar *)"Alternative")) {
                    if (isValidAlternative(cur)) {
                        parseAlternativeContent(theGrid, cur);
                    }
                }
                cur = cur ? cur->next : NULL;
            }
        }
    }
    else if (isElement(cur->name, (const xmlChar *) "World")) {
        // Legacy code to support version 0.1 of the DTDs
        sg_Deprecated();

        cur = cur->xmlChildrenNode;
        while (cur != NULL) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
            else if (isElement(cur->name, (const xmlChar *)"Map")) {
                // Map and world got swapped in the current DTD, that's why this looks a little strange.
                parseWorld (theGrid, cur);
            }
            else if (isElement(cur->name, (const xmlChar *)"Alternative")) {
                if (isValidAlternative(cur)) {
                    parseAlternativeContent(theGrid, cur);
                }
            }
            cur = cur->next;
        }
    }

    //        fprintf(stderr,"ERROR: Map file is missing root \'Resources\' node");

}

int myxmlInputReadFILE (void *context, char *buffer, int len) {
    return fread(buffer, 1, len, (FILE *)context);
}

int myxmlInputCloseFILE (void *context) {
    return (fclose((FILE *)context) == 0) ? 0 : -1;
}

static bool sg_IgnoreRequest( tString const & URI )
{
#ifdef WIN32
    return st_StringEndsWith( URI, "../etc/catalog" );
#else
    return URI.StartsWith( "file:///" ) && strstr( URI, "xml" ) && st_StringEndsWith( URI, "catalog" );
#endif
}

#ifndef HAVE_LIBXML2_WO_PIBCREATE
xmlParserInputBufferPtr
myxmlParserInputBufferCreateFilenameFunc (const char *URI, xmlCharEncoding enc) {
    if ( sg_IgnoreRequest( tString( URI ) ) )
    {
#ifdef DEBUG
        con << "Ignoring xml request for " << URI << "\n";
#endif
        return NULL;
    }
#ifdef DEBUG
    con << "xml wants " << URI << "\n";
#endif
    FILE *f = tResourceManager::openResource(NULL, URI);
    if (f == NULL)
        return NULL;
    xmlParserInputBufferPtr ret = xmlAllocParserInputBuffer(enc);
    ret->context = f;
    ret->readcallback = myxmlInputReadFILE;
    ret->closecallback = myxmlInputCloseFILE;
    return ret;
}
#endif

#ifndef DEDICATED
static tString sg_errorLeadIn("");

static void sg_ErrorFunc( void * ctx,
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
    if ( sg_errorLeadIn.Len() > 2 )
    {
        con << sg_errorLeadIn;
#ifndef DEBUG
        std::cerr << sg_errorLeadIn;
#endif
        sg_errorLeadIn = "";
    }

#ifndef DEBUG
    std::cerr << message;
#endif

    con << message;
}
#endif

bool
gParser::LoadAndValidateMapXML(char const * uri, FILE* docfd, char const * filePath)
{
#ifndef DEDICATED
    /* register error handler */
    xmlGenericErrorFunc errorFunc = &sg_ErrorFunc;
    initGenericErrorDefaultFunc( &errorFunc );
    sg_errorLeadIn = "XML validation error in ";
    sg_errorLeadIn += filePath;
    sg_errorLeadIn += ":\n\n";
#endif

    bool validated = false;

    if (docfd == NULL) {
        con << "LoadAndValidateMapXML passed a NULL docfd (we should really trap this somewhere else!)\n";
        return false;
    }

#ifndef HAVE_LIBXML2_WO_PIBCREATE
    //xmlSetExternalEntityLoader(myxmlResourceEntityLoader);
    xmlParserInputBufferCreateFilenameDefault(myxmlParserInputBufferCreateFilenameFunc);	//should be moved to some program init area
#endif

    if (doc)
    {
        xmlFreeDoc(doc);
        doc=NULL;
    }

    /*Validate the xml*/
    xmlParserCtxtPtr ctxt; /*Parser context*/

    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        con << "Failed to allocate parser context\n";
        return false;
    }

#if HAVE_LIBXML2_WO_PIBCREATE
    // bend URI to the place our included DTDs can be found at
    tString fakeURI = tDirectories::Resource().GetIncluded() + "/aamap.xml";
    uri = fakeURI;
#endif

    /* parse the file, activating the DTD validation option */
    doc = xmlCtxtReadIO(ctxt, myxmlInputReadFILE, NULL, docfd, uri, NULL, XML_PARSE_DTDVALID);
    // NOTE: Do *not* pass myxmlInputCloseFILE; we close the file *later*

    /* check if parsing suceeded */
    if (doc == NULL) {
        con << "Failed to parse \n";
    } else {
        /* check if validation suceeded */
        if (ctxt->valid == 0) {
            con << "Failed to validate.\n";
            xmlFreeDoc(doc);
            doc=NULL;
        }
        else
        {
            validated = true;
        }
    }

    /* check filepath */
    if ( sn_GetNetState() != nCLIENT )
    {
        xmlNodePtr root;
        root = xmlDocGetRootElement(doc);

        if (root) {
            if (isElement(root->name, (const xmlChar *) "Resource"))
            {
                tString rightFilePath = tString( (char const *)myxmlGetProp(root, "author") ) + "/" +
                                        tString( (char const *)myxmlGetProp(root, "category") ) + "/" +
                                        tString( (char const *)myxmlGetProp(root, "name") ) + "-" +
                                        tString( (char const *)myxmlGetProp(root, "version") ) + "." +
                                        tString( (char const *)myxmlGetProp(root, "type") ) + ".xml";

                tString pureFilePath( filePath );
                int paren = pureFilePath.StrPos( "(" );
                if ( paren > 0 )
                {
                    pureFilePath = pureFilePath.SubStr( 0, paren );
                }
                int pos;
                while((pos = rightFilePath.StrPos("//")) != -1) {
                    rightFilePath.RemoveSubStr(pos, 1);
                }
                while((pos = pureFilePath.StrPos("//")) != -1) {
                    pureFilePath.RemoveSubStr(pos, 1);
                }
                if ( rightFilePath != pureFilePath )
                {
                    tOutput message( "$resource_file_wrong_place", pureFilePath, rightFilePath );
                    tOutput title( "$resource_file_wrong_place_title" );
                    throw tGenericException( message, title );
                }
            }
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
