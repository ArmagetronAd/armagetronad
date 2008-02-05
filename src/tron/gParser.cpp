#include "rSDL.h"

#include "aa_config.h"

#include "gParser.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <memory>

#include "eCoord.h"
#include "eGrid.h"
#include "eTess2.h"
#include "gWall.h"
#include "gArena.h"
#include "tMemManager.h"
#include "tResourceManager.h"
#include "tRecorder.h"
#include "tConfiguration.h"
#include "tPolynomial.h"

#include "gGame.h"

#include "tConfiguration.h"
#include "gGame.h"

#ifdef ENABLE_ZONESV2
#include <boost/tokenizer.hpp> // to support splitting a string on ","
#include <boost/shared_ptr.hpp>
#else
#include "gWinZone.h"
#endif

#ifdef __MINGW32__
#define xmlFree(x) free(x)
#endif

#if HAVE_LIBXML2_WO_PIBCREATE
#	include "tDirectories.h"
#endif

#ifdef ENABLE_ZONESV2
typedef std::map< string, zMonitorPtr > monitorMap;
monitorMap monitors;

typedef std::map< string, zZonePtr > zoneMap;
zoneMap mapZones;

#include "nNetObject.h"
typedef std::map< string, nNetObjectID > MapIdToGameId;
MapIdToGameId teamAsso; // mapping between map's teamId and in-game team
MapIdToGameId playerAsso; // mapping between map's playerId and in-game player

#include "nConfig.h"
#define DEFAULT_POLYGONAL_SHAPE_USED "FALSE"
static tString polygonal_shape_used(DEFAULT_POLYGONAL_SHAPE_USED);
static nSettingItemWatched<tString> safetymecanism_polygonal_shapeused("POLYGONAL_SHAPE_USED",polygonal_shape_used, nConfItemVersionWatcher::Group_Breaking, 20 );

// The following are only relevant in the case of zones from maps using version 1
static REAL sg_conquestDecayRate = .1;
static tSettingItem< REAL > sg_conquestDecayRateConf( "FORTRESS_CONQUEST_DECAY_RATE", sg_conquestDecayRate );
#endif
int mapVersion = 0; // The version of the map currently being parsed. Used to adapt parsing to support version specific features

//! Warn about deprecated map format
static void sg_Deprecated()
{
    // no use informing players on the client, they can't do anything about it anyway
    if ( sn_GetNetState() != nCLIENT )
        con << "\n\n" << tColoredString::ColorString(1,.3,.3) << "WARNING: You are loading a map that is written in a deprecated format. It should work for now, but will stop to do so in one of the next releases. Have the map upgraded to an up-to-date version as soon as possible.\n\n";
}

#ifdef DEBUG_ZONE_SYNC
// This code is to attempt to synchronize moving zones on the client and server
// It just doesnt work ATM, nor it is close to any solution.
static bool newGameRound; // Indicate that a round has just started when true (no not really, a bad approximation)

// the following crash on the server as soon as the player being monitored dies!!!!
float tSysTimeHack2(float x)
{
    int playerID = sr_viewportBelongsToPlayer[ 0 ];
    // get the player
    ePlayer* player = ePlayer::PlayerConfig( playerID );

    float asdf = player->netPlayer->ping;

    static float basePing;
    // preserve basePing only at the start of a new round
    if (newGameRound == true) {
        newGameRound = false;
        basePing = asdf;
    }

    return static_cast<float> (tSysTimeFloat() /*+ (player->netPlayer->ping*2)*/ + basePing/2);
}
#endif // DEBUG_ZONE_SYNC

gParser::gParser(gArena *anArena, eGrid *aGrid):
        theArena(anArena),
        theGrid(aGrid),
        rimTexture(0),
        sizeMultiplier(0.0)
{
    m_Doc = NULL;

    // HACK - philippeqc
    // This seems as inconvenient as any other place to load the
    // static tables of variables and functions available.
    // It's run once per map loading, between round, so its basically
    // cost less vs the real structural mess that it cause ;)

    //  vars[tString("sizeMultiplier")] = &sizeMultiplier; // BAD dont use other than as an example, non static content

    //    tValue::Expr::functions[tString("time")] = &tSysTimeHack;
#ifdef DEBUG_ZONE_SYNC
    tValue::Expr::functions[tString("time2")] = &tSysTimeHack2;
#endif //DEBUG_ZONE_SYNC
    //    tValue::Expr::functions[tString("sizeMultiplier")] = &gArena::GetSizeMultiplierHack; // static

}

bool
gParser::trueOrFalse(char *str)
{
    // This will work with true/false/yes/no/1/-1/0/etc
    return (!strncasecmp(str, "t", 1) || !strncasecmp(str, "y", 1) || atoi(str));
}

char *
gParser::myxmlGetProp(xmlNodePtr cur, const char *name) {
    return (char *)xmlGetProp(cur, (const xmlChar *)name);
}

int
gParser::myxmlGetPropInt(xmlNodePtr cur, const char *name) {
    char *v = myxmlGetProp(cur, name);
    if (v == NULL)	return 0;
    int r = atoi(v);
    xmlFree(v);
    return r;
}

#ifdef ENABLE_ZONESV2
rColor
gParser::myxmlGetPropColorFromHex(xmlNodePtr cur, const char *name) {
    char *v = myxmlGetProp(cur, name);
    if (v == NULL)	return rColor();
    int r = strtoul(v, NULL, 0);
    rColor aColor;
    if (strlen(v) >= 9) {
        aColor.a_ = ((REAL)(r & 255)) / 255.0;
        r /= 256;
        if (aColor.a_ > 0.7)
            aColor.a_ = 0.7;
    }
    else {
        aColor.a_ = 0.7;
    }
    aColor.b_ = ((REAL)(r & 255)) / 255.0;
    r /= 256;
    aColor.g_ = ((REAL)(r & 255)) / 255.0;
    r /= 256;
    aColor.r_ = ((REAL)(r & 255)) / 255.0;
    r /= 256;

    xmlFree(v);
    return aColor;
}
#endif

float
gParser::myxmlGetPropFloat(xmlNodePtr cur, const char *name) {
    char *v = myxmlGetProp(cur, name);
    if (v == NULL)	return 0.;
    float r = atof(v);
    xmlFree(v);
    return r;
}

bool
gParser::myxmlGetPropBool(xmlNodePtr cur, const char *name) {
    char *v = myxmlGetProp(cur, name);
    if (v == NULL)	return false;
    bool r = trueOrFalse(v);
    xmlFree(v);
    return r;
}

#ifdef ENABLE_ZONESV2
Triad
gParser::myxmlGetPropTriad(xmlNodePtr cur, const char *name) {
    Triad res = _ignore;
    char *v = myxmlGetProp(cur, name);
    if (v == NULL)           res = _false;
    else if (strcmp(v, "true")==0)  res = _true;
    else if (strcmp(v, "false")==0) res = _false;

    xmlFree(v);
    return res;
}
#endif

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
    xmlChar *version = xmlGetProp(cur, (const xmlChar *) "version");
    /*
     * Find non empty version and
     * Alternative element, ie those having a name starting by "Alternative" and
     * only the Alternative elements that are for our version, ie Arthemis
     */
    return ((version != NULL) && isElement(cur->name, (const xmlChar *)"Alternative", keyword) && validateVersionRange(version, (const xmlChar*)"Artemis", (const xmlChar*)"0.2.8.0") );
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
        strtol(start, &end, 10);
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
    while ( cur != NULL) {
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
        for (int i=0; i<number; i++){
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

    theArena->NewSpawnPoint(eCoord(x, y), eCoord(xdir, ydir));

    endElementAlternative(grid, cur, keyword);
}

#ifdef ENABLE_ZONESV2
/*
  Extract all the color codes and build a rColor object.
  Return: a rColor object.

 */
rColor
gParser::parseColor(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    rColor color;
    color.r_ = myxmlGetPropFloat(cur, "red");
    color.g_ = myxmlGetPropFloat(cur, "green");
    color.b_ = myxmlGetPropFloat(cur, "blue");
    color.a_ = myxmlGetPropFloat(cur, "alpha");

    if (myxmlHasProp(cur, "hexCode"))
    {
        color = myxmlGetPropColorFromHex(cur, "hexCode");
    }

    if (color.a_ > 0.7)
        color.a_ = 0.7;
    if (color.a_ < 0.0)
        color.a_ = 0.0;


    /*
    if(myxmlHasProp(cur, "name") {
      string colorName = myxmlGetProp(cur, "name");
      
    }
    */


    /*


    // Blue

    0xf0f8ff alice,
    0x007fff azure,
    0x007ba7 cerulean,
    0x0047ab cobalt
    0x6495ed cornflower
    0x0000c8 dark
    0x1560bd denim
    0x1e90ff dodger
    0x4b0082 indigo
    0x002fa7 internationalklein
    0xbdbbd7 lavender
    0x003366 midnight
    0x000080 navy
    0xccccff periwinkle
    0x32127a persian
    0x003399 powder
    0x003153 prussian
    0x4169e1 royal
    0x082567 sapphire
    0x4682b4 steel
    0x120a8f ultramarine



    // RED

    0xe32636 alizarin,
    0x800020 burgundy,
    0xc41e3a cardinal,
    0x960018 carmine,
    0xde3163 cerise
    0xcd5c5c chestnut
    0xdc143c crimson
    0x801818 falu
    0xff00ff fuchsia
    0xff0090 magenta
    0x800000 maroon
    0x993366 mauve
    0xc71585 red-violet
    0xb7410e rust
    0xcc8899 puce
    0x92000a sangria
    0xff2400 scarlet
    0xe2725b terracotta
    0xcc4e5c darkterracotta
    0xe34234 vermilion
    */

    return color;
}

zShapePtr
gParser::parseShapeCircleArthemis(eGrid *grid, xmlNodePtr cur, unsigned short idZone, const xmlChar * keyword)
{
    zShapePtr shape = zShapePtr( new zShapeCircle(grid, idZone) );

    // Build up the scale information
    {
        tFunction tfScale;
        tfScale.SetOffset( myxmlGetPropFloat(cur, "radius") * sizeMultiplier );
        tfScale.SetSlope( myxmlGetPropFloat(cur, "growth") * sizeMultiplier );
        shape->setScale( tfScale );
    }

    // Set up the default rotation speed
    {
        tPolynomial<nMessage> tpRotation(2);
        tpRotation[0] = 0.0f;
        tpRotation[1] = .3f;
        shape->setRotation2( tpRotation );
    }

    // Set up the location
    cur = cur->xmlChildrenNode;
    while ( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            REAL x = myxmlGetPropFloat(cur, "x");
            REAL y = myxmlGetPropFloat(cur, "y");

            tFunction tfPos;
            tfPos.SetOffset( x * sizeMultiplier );
            shape->setPosX( tfPos );

            tfPos.SetOffset( y * sizeMultiplier );
            shape->setPosY( tfPos );

            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
    return shape;
}

zShapePtr
gParser::parseShapeCircleBachus(eGrid *grid, xmlNodePtr cur, unsigned short idZone, const xmlChar * keyword)
{
    zShapeCircle *shapePtr = new zShapeCircle(grid, idZone) ;

    // The radius need to be handled separatly
    tFunction tfRadius;
    if (myxmlHasProp(cur, "radius")) {
        string str = string(myxmlGetProp(cur, "radius"));
        myCheapParameterSplitter(str, tfRadius, true);
    }
    else {
        tfRadius.SetOffset( 1.0 * sizeMultiplier );
        tfRadius.SetSlope( 0.0 );
    }
    shapePtr->setRadius( tfRadius );

    zShapePtr shape = zShapePtr( shapePtr );

    parseShape(grid, cur, keyword, shape);

    return shape;
}

zShapePtr
gParser::parseShapePolygon(eGrid *grid, xmlNodePtr cur, unsigned short idZone, const xmlChar * keyword)
{
    // Polygon shapes are not supported by older clients.
    std::stringstream ss;
    /* Yes it is ackward to generate a string that will be decifered on the other end*/
    ss << "POLYGONAL_SHAPE_USED TRUE";
    tConfItemBase::LoadLine(ss);

    zShapePtr shape = zShapePtr( new zShapePolygon(grid, idZone) );
    parseShape(grid, cur, keyword, shape);

    return shape;
}

// Quick stub to allow to operate on tFunction
// Remove when all that is variant has been ported to ruby
void gParser::myCheapParameterSplitter(const string &str, tFunction &tf, bool addSizeMultiplier)
{
    REAL param[2] = {0.0, 0.0};
    int bPos;
    if ( (bPos = str.find(';')) != -1)
    {
        param[0] = atof(str.substr(0, bPos).c_str());
        param[1] = atof(str.substr(bPos + 1, str.length()).c_str());
    }
    else
    {
        param[0] = atof(str.c_str());
    }

    if (addSizeMultiplier)
    {
        param[0] = param[0] * sizeMultiplier;
        param[1] = param[1] * sizeMultiplier;
    }
    tf.SetOffset(param[0]);
    tf.SetSlope(param[1]);
}

// Quick stub to allow to operate on tFunction
// Remove when all that is variant has been ported to ruby
void gParser::myCheapParameterSplitter2(const string &str, tPolynomial<nMessage> &tp, bool addSizeMultiplier)
{
    REAL param[2] = {0.0, 0.0};
    int bPos;
    if ( (bPos = str.find(';')) != -1)
    {
        param[0] = atof(str.substr(0, bPos).c_str());
        param[1] = atof(str.substr(bPos + 1, str.length()).c_str());
    }
    else
    {
        param[0] = atof(str.c_str());
    }

    if (addSizeMultiplier)
    {
        param[0] = param[0] * sizeMultiplier;
        param[1] = param[1] * sizeMultiplier;
    }

    tp[0] = param[0];
    tp[1] = param[1];
}

void
gParser::parseShape(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword, zShapePtr &shape)
{
    tValue::BasePtr xp;
    tValue::BasePtr yp;
    bool centerLocationFound = false;

    if (myxmlHasProp(cur, "scale")) {
        string str = string(myxmlGetProp(cur, "scale"));
        tFunction tfScale;

        myCheapParameterSplitter(str, tfScale, false);
        shape->setScale( tfScale );
    }

    if (myxmlHasProp(cur, "rotation")) {
        string str = string(myxmlGetProp(cur, "rotation"));
        tPolynomial<nMessage> tpRotation;

        myCheapParameterSplitter2(str, tpRotation, false);
        shape->setRotation2( tpRotation );
    }

    cur = cur->xmlChildrenNode;
    while ( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            /* We need to multipy by sizeMultiper so the item are properly placed*/
            string strX = string(myxmlGetProp(cur, "x"));
            tFunction tfX;
            myCheapParameterSplitter(strX, tfX, true);
            string strY = string(myxmlGetProp(cur, "y"));
            tFunction tfY;
            myCheapParameterSplitter(strY, tfY, true);

            if (centerLocationFound == false) {
                shape->setPosX( tfX );
                shape->setPosY( tfY );
                centerLocationFound = true;
            }
            else {
                zShapePolygon *tmpShapePolygon = dynamic_cast<zShapePolygon *>( (zShape*)shape );
                if (tmpShapePolygon)
                    tmpShapePolygon->addPoint( myPoint( tfX, tfY ) );
            }

            endElementAlternative(grid, cur, keyword);
        }

        else if (isElement(cur->name, (const xmlChar *)"Color", keyword)) {
            shape->setColor( parseColor(grid, cur, keyword) );

            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }
}

zZoneInfluencePtr
gParser::parseZoneEffectGroupZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword) {
    string zoneName(myxmlGetProp(cur, "name"));
    zZoneInfluencePtr infl;
    zZonePtr refZone;

    // Does the zone to be monitored is already registered
    zoneMap::const_iterator iterZone;
    if ((iterZone = mapZones.find(zoneName)) != mapZones.end()) {
        // load the zone
        refZone = iterZone->second;
    }
    else {
        // make an empty zone and store under the right label
        // It should be populated later

        //  refZone = zZonePtr(new zZone(grid));
        refZone = tNEW(zZone)(grid);
        if (!zoneName.empty())
            mapZones[zoneName] = refZone;
    }
    infl = zZoneInfluencePtr(new zZoneInfluence(refZone));

    cur = cur->xmlChildrenNode;
    while ( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Rotation", keyword)) {
            zZoneInfluenceItemRotation *b = new zZoneInfluenceItemRotation(refZone);

            string str = string(myxmlGetProp(cur, "rotationAngle"));
            tFunction tfRotationAngle;
            myCheapParameterSplitter(str, tfRotationAngle, false);

            str = string(myxmlGetProp(cur, "rotationSpeed"));
            tFunction tfRotationSpeed;
            myCheapParameterSplitter(str, tfRotationSpeed, false);

            b->set(tfRotationAngle, tfRotationSpeed);
            infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));
        }
        else if (isElement(cur->name, (const xmlChar *)"Scale", keyword)) {
            zZoneInfluenceItemScale *b = new zZoneInfluenceItemScale(refZone);
            b->set(myxmlGetPropFloat(cur, "scale") *sizeMultiplier);
            infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));
        }
        else if (isElement(cur->name, (const xmlChar *)"Color", keyword)) {
            zZoneInfluenceItemColor *b = new zZoneInfluenceItemColor(refZone);
            b->set(parseColor(grid, cur, keyword));
            infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));
        }
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            /* We need to multipy by sizeMultiper so the item are properly placed*/
            REAL x = myxmlGetPropFloat(cur, "x") *sizeMultiplier;
            REAL y = myxmlGetPropFloat(cur, "y") *sizeMultiplier;
            zZoneInfluenceItemPosition *b = new zZoneInfluenceItemPosition(refZone);
            b->set( eCoord(x, y) );
            infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));

            endElementAlternative(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }


    return infl;
}

zMonitorInfluencePtr
gParser::parseZoneEffectGroupMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword) {
    string monitorName(myxmlGetProp(cur, "name"));

    zMonitorPtr ref;
    // Find the associated monitor

    monitorMap::const_iterator iterMonitor;
    // associate the label to the proper effector
    if ((iterMonitor = monitors.find(monitorName)) != monitors.end()) {
        ref = iterMonitor->second;
    }
    else {
        // make an empty zone and store under the right label
        // It should be populated later
        ref = zMonitorPtr(new zMonitor(grid));
        if (!monitorName.empty()) {
            monitors[monitorName] = ref;
            ref->setName(monitorName);
        }
    }

    zMonitorInfluencePtr infl = zMonitorInfluencePtr(new zMonitorInfluence(ref));
    infl->setMarked(myxmlGetPropTriad(cur, "marked"));

    if (xmlHasProp(cur, (const xmlChar*)"influenceSlide")) {
        string str = string(myxmlGetProp(cur, "influenceSlide"));
        tPolynomial<nMessage> tpInfluenceSlide(2);
        myCheapParameterSplitter2(str, tpInfluenceSlide, false);

        infl->setInfluenceSlide( tpInfluenceSlide );
    }

    if (xmlHasProp(cur, (const xmlChar *)"influenceAdd")) {
        string str = string(myxmlGetProp(cur, "influenceAdd"));
        tFunction tfInfluence;
        myCheapParameterSplitter(str, tfInfluence, false);
        infl->setInfluenceAdd( tfInfluence );
    }

    if (xmlHasProp(cur, (const xmlChar *)"influenceSet")) {
        string str = string(myxmlGetProp(cur, "influenceSet"));
        tFunction tfInfluence;
        myCheapParameterSplitter(str, tfInfluence, false);
        infl->setInfluenceSet( tfInfluence );
    }

    return infl;
}

zEffectorPtr
gParser::parseZoneEffectGroupEffector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword) {
    zEffectorPtr effector;
    /*
      build the effect, in this case give points to the target(s)
    */
    typedef zEffector* (*effectorFactory)();
    std::map<tString, effectorFactory> effectors;
    // Build the list of supported effector
    effectors[tString("win")] = zEffectorWin::create;
    effectors[tString("death")] = zEffectorDeath::create;
    effectors[tString("point")] = zEffectorPoint::create;

    /*
      effectors[tString("event")] = zEffectorEvent::create;
      effectors[tString("cleartrace")] = zEffectorClearrace::create;
      effectors[tString("teleport")] = zEffectorTeleport::create;
    */
    effectors[tString("spawnplayer")] = zEffectorSpawnPlayer::create;
    effectors[tString("brakerecharge")] = zEffectorCycleBrake::create;
    effectors[tString("rubberrecharge")] = zEffectorCycleRubber::create;
    effectors[tString("acceleration")] = zEffectorCycleAcceleration::create;
    effectors[tString("setting")] = zEffectorSetting::create;


    // TODO: add tolower()
    // Get the label of the effector to be used
    string effectorAttribute( myxmlGetProp(cur, "effect"));
    transform (effectorAttribute.begin(), effectorAttribute.end(), effectorAttribute.begin(), tolower);
    std::map<tString, effectorFactory>::const_iterator iterEffectorFactory;
    // associate the label to the proper effector
    if ((iterEffectorFactory = effectors.find(effectorAttribute)) != effectors.end()) {

        effector = zEffectorPtr((*(iterEffectorFactory->second))());
        /*
        Save the effector for the zone effect
        */
    }

    // Should we load the score information
    zEffectorPoint *effectorPoint;
    effectorPoint = dynamic_cast<zEffectorPoint *>(effector.get());
    if (effectorPoint) {
        effectorPoint->setPoint(myxmlGetPropInt(cur, "score"));
    }

    // Should we load the acceleration
    zEffectorCycleAcceleration *effectorAcceleration;
    effectorAcceleration = dynamic_cast<zEffectorCycleAcceleration *>(effector.get());
    if (effectorAcceleration) {
        tFunction tfValue;
        string str = string(myxmlGetProp(cur, "value"));
        myCheapParameterSplitter(str, tfValue, false);
        effectorAcceleration->setValue(tfValue);
    }

    // Should we set the grid and arena for respawning
    zEffectorSpawnPlayer *effectorSpawnPlayer;
    effectorSpawnPlayer = dynamic_cast<zEffectorSpawnPlayer *>(effector.get());
    if (effectorSpawnPlayer) {
        effectorSpawnPlayer->setGrid(grid);
        effectorSpawnPlayer->setArena(sg_GetArena());
    }

    // Should we set the setting name and value
    zEffectorSetting *effectorSetting;
    effectorSetting = dynamic_cast<zEffectorSetting *>(effector.get());
    if (effectorSetting) {
        if (myxmlHasProp(cur, "settingName"))
            effectorSetting->setSettingName(tString(myxmlGetProp(cur, "settingName")));
        if (myxmlHasProp(cur, "settingValue"))
            effectorSetting->setSettingValue(tString(myxmlGetProp(cur, "settingValue")));
    }

    effector->setCount(myxmlGetPropInt(cur, "count"));

    if (myxmlHasProp(cur, "description"))
        effector->setMessage(tString(myxmlGetProp(cur, "description")));

    return effector;
}

zSelectorPtr
gParser::parseZoneEffectGroupSelector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword) {
    zSelectorPtr selector;

    /*
      Build a selector of type self, ie: only the person activating the zone is the target
    */
    typedef zSelector* (*selectorFactory)();
    std::map<tString, selectorFactory> selectors;
    // Build the list of supported selector
    selectors[tString("self")] = zSelectorSelf::create;
    selectors[tString("teammate")] = zSelectorTeammate::create;
    selectors[tString("another")] = zSelectorAnother::create;
    selectors[tString("team")] = zSelectorTeam::create;
    selectors[tString("all")] = zSelectorAll::create;
    selectors[tString("allbutself")] = zSelectorAllButSelf::create;
    //  selectors[tString("allbutteam")] = zSelectorAllButTeam::create;
    selectors[tString("another")] = zSelectorAnother::create;
    /*
      selectors[tString("anotherteam")] = zSelectorAnotherTeam::create;
      selectors[tString("anotherteammate")] = zSelectorAnotherTeammate::create;
      selectors[tString("anothernotteammate")] = zSelectorAnotherNotTeammate::create;
    */
    selectors[tString("owner")] = zSelectorOwner::create;
    selectors[tString("ownerteam")] = zSelectorOwnerTeam::create;
    selectors[tString("ownerteamteammate")] = zSelectorOwnerTeamTeammate::create;
    selectors[tString("anydead")] = zSelectorAnyDead::create;
    selectors[tString("alldead")] = zSelectorAllDead::create;
    selectors[tString("singledeadowner")] = zSelectorSingleDeadOwner::create;
    selectors[tString("anotherteammatedead")] = zSelectorAnotherTeammateDead::create;
    selectors[tString("anothernotteammatedead")] = zSelectorAnotherNotTeammateDead::create;

    // TODO: add tolower()
    // Get the label of the selector to be used
    string selectorAttribute( myxmlGetProp(cur, "target"));
    transform (selectorAttribute.begin(), selectorAttribute.end(), selectorAttribute.begin(), tolower);

    std::map<tString, selectorFactory>::const_iterator iterSelectorFactory;
    // associate the label to the proper selector
    if ((iterSelectorFactory = selectors.find(selectorAttribute)) != selectors.end()) {

        selector = zSelectorPtr((*(iterSelectorFactory->second))());

        selector->setCount(myxmlGetPropInt(cur, "count"));

        cur = cur->xmlChildrenNode;
        while ( cur != NULL) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
            else if (isElement(cur->name, (const xmlChar *)"Effect", keyword)) {
                selector->addEffector(parseZoneEffectGroupEffector(grid, cur, keyword));
            }
            else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
                if (isValidAlternative(cur, keyword)) {
                    parseAlternativeContent(grid, cur);
                }
            }
            cur = cur->next;
        }
    }
    return selector;
}

zValidatorPtr
gParser::parseZoneEffectGroupValidator(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword) {
    zValidatorPtr validator;

    Triad positive=myxmlGetPropTriad(cur, "positive");
    Triad marked  =myxmlGetPropTriad(cur, "marked"  );

    /*
     * Get the validator for this EffectGroup
     */
    typedef zValidator* (*validatorFactory)(Triad, Triad);
    std::map<tString, validatorFactory> validators;
    // Build the list of supported validator
    validators[tString("all")] = zValidatorAll::create;
    validators[tString("owner")] = zValidatorOwner::create;
    validators[tString("ownerteam")] = zValidatorOwnerTeam::create;
    validators[tString("allbutowner")] = zValidatorAllButOwner::create;
    validators[tString("allbutteamowner")] = zValidatorAllButTeamOwner::create;
    /*
      validators[tString("anotherteammate")] = zValidatorTeammate::create;
    */

    // TODO: add tolower()
    // Get the label of the validator to be used
    string validatorAttribute( myxmlGetProp(cur, "user"));
    transform (validatorAttribute.begin(),validatorAttribute.end(), validatorAttribute.begin(), tolower);

    std::map<tString, validatorFactory>::const_iterator iterValidatorFactory;
    // associate the label to the proper validator
    if ((iterValidatorFactory = validators.find(validatorAttribute)) != validators.end()) {

        validator = zValidatorPtr((*(iterValidatorFactory->second))(positive, marked));
        /*
          Save the validator for the zone effect
        */
    }


    cur = cur->xmlChildrenNode;
    while ( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Target", keyword)) {
            validator->addSelector(parseZoneEffectGroupSelector(grid, cur, keyword));
        }
        else if (isElement(cur->name, (const xmlChar *)"MonitorInfluence", keyword)) {
            validator->addMonitorInfluence(parseZoneEffectGroupMonitor(grid, cur, keyword));
        }
        else if (isElement(cur->name, (const xmlChar *)"ZoneInfluence", keyword)) {
            validator->addZoneInfluence(parseZoneEffectGroupZone(grid, cur, keyword));
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }


    return validator;
}


zEffectGroupPtr
gParser::parseZoneEffectGroup(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    /*
      Store the owner information
    */
    gVectorExtra< nNetObjectID > nidPlayerOwners;

    if (xmlHasProp(cur, (const xmlChar*)"owners"))
    {
        string ownersDesc( myxmlGetProp(cur, "owners"));
        boost::tokenizer<> tok(ownersDesc);

        // For each owner listed
        for (boost::tokenizer<>::iterator iter=tok.begin();
                iter!=tok.end();
                ++iter)
        {
            // Map from map descriptor to in-game ids
            MapIdToGameId::iterator mapOwnerToInGameOwnerPairIter = playerAsso.find(*iter);
            if (mapOwnerToInGameOwnerPairIter != playerAsso.end())
            {
                // Found a matching in-game owner
                nidPlayerOwners.push_back( (*mapOwnerToInGameOwnerPairIter).second );
            }
            else
            {
                // No in-game owner matching, pass
            }
        }
    }

    /*
     * Store the teamOwners information
     */
    gVectorExtra< nNetObjectID > nidTeamOwners;

    if (xmlHasProp(cur, (const xmlChar*)"teamOwners"))
    {
        string ownersDesc( myxmlGetProp(cur, "teamOwners"));
        boost::tokenizer<> tok(ownersDesc);

        for (boost::tokenizer<>::iterator iter=tok.begin();
                iter!=tok.end();
                ++iter)
        {
            MapIdToGameId::iterator mapTeamOwnerToInGameTeamOwnerPairIter = teamAsso.find(*iter);
            if (mapTeamOwnerToInGameTeamOwnerPairIter != teamAsso.end())
            {
                // Found a matching in-game owning team
                nidTeamOwners.push_back( (*mapTeamOwnerToInGameTeamOwnerPairIter).second );
            }
            else {
                // No in-game owning team found. pass.
            }
        }
    }

    /*
     * Prepare a new EffectGroup
     */
    zEffectGroupPtr currentZoneEffect = zEffectGroupPtr(new zEffectGroup(nidPlayerOwners, nidTeamOwners));


    cur = cur->xmlChildrenNode;
    while ( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"User", keyword)) {
            currentZoneEffect->addValidator(parseZoneEffectGroupValidator(grid, cur, keyword));
        }
        else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
            if (isValidAlternative(cur, keyword)) {
                parseAlternativeContent(grid, cur);
            }
        }
        cur = cur->next;
    }


    return currentZoneEffect;
}

void
gParser::parseZoneArthemis(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{

    if (sn_GetNetState() != nCLIENT )
    {
        rColor color;

        // Create a new zone
        zZonePtr zone = zZonePtr(new zZone(grid));

        // Insert the zone under a bogus name
        string zoneName = "";
        zoneMap::const_iterator iterZone;
        do
        {
            // Fill the zone under the shortest available series of pound.
            zoneName += "#";
            iterZone = mapZones.find(zoneName);
        }
        while (iterZone != mapZones.end());

        // If a name was assigned to it, save the zone in a map so it can be refered to
        if (!zoneName.empty())
            mapZones[zoneName] = zone;
        zone->setName(zoneName);


        enum { win, death, fortress };
        int effect = win;
        if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"win")) {
            effect = win;
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"death")) {
            effect = death;
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"fortress")) {
            effect = fortress;
        }

        if (sn_GetNetState() != nCLIENT )
        {
            if (effect != fortress)
            {
                // Create an effect group without ownership
                zEffectGroupPtr currentZoneEffect = zEffectGroupPtr(new zEffectGroup(gVectorExtra< nNetObjectID >(), gVectorExtra< nNetObjectID >()));

                // Create a validator for everybody (i.e. All)
                zValidatorPtr validator = zValidatorPtr( new zValidatorAll(_ignore, _ignore) );

                zSelectorPtr selector = zSelectorPtr( new zSelectorSelf() );
                //selector->setCount( -1 ); // Give infinite usage

                zEffectorPtr effector;
                if (effect == win)
                    effector = zEffectorPtr( new zEffectorWin() );
                else
                    effector = zEffectorPtr( new zEffectorDeath() );

                effector->setCount( -1 );

                // Store all the objects
                selector->addEffector( effector );
                validator->addSelector( selector );
                currentZoneEffect->addValidator( validator );
                zone->addEffectGroupEnter( currentZoneEffect );
            }
            else {
                zMonitorPtr monitor = zMonitorPtr(new zMonitor(grid));
                // use the same name as the associated zone
                monitors[zoneName] = monitor;
                // TODO: read data and populate t
                tPolynomial<nMessage> t;
                monitor->setInit( t );
                tPolynomial<nMessage> drift(2);
                drift[1] = -1.0f * sg_conquestDecayRate;
                monitor->setDrift( drift );
                monitor->setClampLow ( 0.0f );
                monitor->setClampHigh( 1.0f );

                zMonitorRulePtr rule;
                {
                    // All that happens once the zone is conquered
                    rule = zMonitorRulePtr( new zMonitorRuleOver( 1.0f ) );

                    zEffectGroupPtr currentZoneEffect;
                    {
                        // Create an effect group without ownership
                        currentZoneEffect = zEffectGroupPtr(new zEffectGroup(gVectorExtra< nNetObjectID >(), gVectorExtra< nNetObjectID >()));

                        // Create a validator for everybody (i.e. All)
                        zValidatorPtr validator = zValidatorPtr( new zValidatorAll(_ignore, _ignore) );

                        // Once the zone is conquered, collapse the zhape
                        zZoneInfluenceItemScale *scaler = new zZoneInfluenceItemScale(zone);
                        scaler->set( -1.5f );

                        zZoneInfluencePtr infl = zZoneInfluencePtr(new zZoneInfluence(zone));
                        infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(scaler));

                        validator->addZoneInfluence( infl );
                        currentZoneEffect->addValidator( validator );
                    }

                    rule->addEffectGroup( currentZoneEffect );
                    monitor->addRule(rule);
                }

                {
                    rule = zMonitorRulePtr( new zMonitorRuleUnder( 1.0f ) ); // i.e: Always
                    zEffectGroupPtr currentZoneEffect;
                    {
                        // Create an effect group without ownership
                        currentZoneEffect = zEffectGroupPtr(new zEffectGroup(gVectorExtra< nNetObjectID >(), gVectorExtra< nNetObjectID >()));

                        // Create a validator for everybody (i.e. All)
                        zValidatorPtr validator = zValidatorPtr( new zValidatorAll(_ignore, _ignore) );

                        zZoneInfluenceItemRotation *b = new zZoneInfluenceItemRotation(zone);
                        b->set( tFunction(0.0f, 0.0f), tFunction(0.3f, 2.0f * 3.141f / 11.0f) );

                        zZoneInfluencePtr infl = zZoneInfluencePtr(new zZoneInfluence(zone));
                        infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));

                        validator->addZoneInfluence( infl );
                        currentZoneEffect->addValidator( validator );
                    }
                    rule->addEffectGroup( currentZoneEffect );
                    monitor->addRule(rule);
                }

                zone->setOldFortressAutomaticAssignmentBehavior(true);
            }
        }

        cur = cur->xmlChildrenNode;

        while (cur) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
            else if (isElement(cur->name, (const xmlChar *)"ShapeCircle", keyword)) {
                zone->setShape( parseShapeCircleArthemis(grid, cur, zone->ID(), keyword) );
            }
            else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
                if (isValidAlternative(cur, keyword)) {
                    parseAlternativeContent(grid, cur);
                }
            }
            cur = cur->next;
        }


    }
}

void
gParser::parseZoneBachus(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
    string zoneName = "";

    if (myxmlHasProp(cur, "name"))
        zoneName = myxmlGetProp(cur, "name");

    cur = cur->xmlChildrenNode;

    if (sn_GetNetState() != nCLIENT )
    {
        rColor color;

        zZonePtr zone;
        zoneMap::const_iterator iterZone;
        // Has this zone been already registered, such as through a zoneInfluence
        if ((iterZone = mapZones.find(zoneName)) != mapZones.end()) {
            // Open the zone so we can fill in the details
            zone = iterZone->second;
        }
        else {
            // Create a new zone
            zone = zZonePtr(new zZone(grid));
            // If a name was assigned to it, save the zone in a map so it can be refered to
            if (!zoneName.empty())
                mapZones[zoneName] = zone;
            zone->setName(zoneName);
        }

        while (cur != NULL) {
            if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
            else if (isElement(cur->name, (const xmlChar *)"ShapeCircle", keyword)) {
                zone->setShape( parseShapeCircleBachus(grid, cur, zone->ID(), keyword) );
            }
            else if (isElement(cur->name, (const xmlChar *)"ShapePolygon", keyword)) {
                zone->setShape( parseShapePolygon(grid, cur, zone->ID(), keyword) );
            }
            else if (isElement(cur->name, (const xmlChar *)"Enter", keyword)) {
                xmlNodePtr cur2 = cur->xmlChildrenNode;
                while (cur2 != NULL) {
                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
                    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
                        zone->addEffectGroupEnter(parseZoneEffectGroup(grid, cur2, keyword));
                    }
                    cur2 = cur2->next;
                }
            }
            else if (isElement(cur->name, (const xmlChar *)"Inside", keyword)) {
                xmlNodePtr cur2 = cur->xmlChildrenNode;
                while (cur2 != NULL) {
                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
                    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
                        zone->addEffectGroupInside(parseZoneEffectGroup(grid, cur2, keyword));
                    }
                    cur2 = cur2->next;
                }
            }
            else if (isElement(cur->name, (const xmlChar *)"Leave", keyword)) {
                xmlNodePtr cur2 = cur->xmlChildrenNode;
                while (cur2 != NULL) {
                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
                    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
                        zone->addEffectGroupLeave(parseZoneEffectGroup(grid, cur2, keyword));
                    }
                    cur2 = cur2->next;
                }
            }
            else if (isElement(cur->name, (const xmlChar *)"Outside", keyword)) {
                xmlNodePtr cur2 = cur->xmlChildrenNode;
                while (cur2 != NULL) {
                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
                    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
                        zone->addEffectGroupOutside(parseZoneEffectGroup(grid, cur2, keyword));
                    }
                    cur2 = cur2->next;
                }
            }
            else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
                if (isValidAlternative(cur, keyword)) {
                    parseAlternativeContent(grid, cur);
                }
            }
            cur = cur->next;
        }

        // leaving zone undeleted is no memory leak here, the grid takes control of it
        if ( zone )
        {
            zone->RequestSync();
        }
    }
}
#else
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
#endif

void
gParser::parseZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
#ifdef ENABLE_ZONESV2
    switch (mapVersion)
    {
    case 1:
        parseZoneArthemis(grid, cur, keyword);
        break;
    case 2:
        parseZoneBachus(grid, cur, keyword);
        break;
    default:
        parseZoneBachus(grid, cur, keyword);
        break;
    }
#else
    float x, y, radius, growth;
    bool shapeFound = false;
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

    gZone * zone = NULL;
    if (sn_GetNetState() != nCLIENT )
    {
        if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"win")) {
            zone = tNEW( gWinZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"death")) {
            zone = tNEW( gDeathZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }
        else if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *)"effect"), (const xmlChar *)"fortress")) {
            zone = tNEW( gBaseZoneHack) ( grid, eCoord(x*sizeMultiplier,y*sizeMultiplier) );
        }

        // leaving zone undeleted is no memory leak here, the gid takes control of it
        if ( zone )
        {
            zone->SetRadius( radius*sizeMultiplier );
            zone->SetExpansionSpeed( growth*sizeMultiplier );
            zone->SetRotationSpeed( .3f );
            zone->RequestSync();
        }
    }
#endif
}

#ifdef ENABLE_ZONESV2
void
gParser::parseMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
    if (sn_GetNetState() != nCLIENT )
    {
        zMonitorPtr monitor;

        string monitorName(myxmlGetProp(cur, "name"));
        monitorMap::const_iterator iterMonitor;
        // associate the label to the proper effector
        if ((iterMonitor = monitors.find(monitorName)) != monitors.end()) {
            monitor = iterMonitor->second;
        }
        else {
            // make an empty zone and store under the right label
            // It should be populated later
            monitor = zMonitorPtr(new zMonitor(grid));
            if (!monitorName.empty()) {
                monitors[monitorName] = monitor;
                monitor->setName(monitorName);
            }
        }

        // TODO: read data and populate t
        tPolynomial<nMessage> t;
        //        monitor->setInit(myxmlGetPropFloat(cur, "init"));
        monitor->setInit( t );
        tPolynomial<nMessage> drift(2);
        drift[1] = myxmlGetPropFloat(cur, "drift");
        monitor->setDrift( drift );
        monitor->setClampLow (myxmlGetPropFloat(cur, "low"));
        monitor->setClampHigh(myxmlGetPropFloat(cur, "high"));

        cur = cur->xmlChildrenNode;

        if (sn_GetNetState() != nCLIENT )
        {
            zMonitorRulePtr rule;
            bool ruleFound = false;

            while (cur != NULL) {
                if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
                else if (isElement(cur->name, (const xmlChar *)"OnOver", keyword)) {
                    rule = zMonitorRulePtr(new zMonitorRuleOver(myxmlGetPropFloat(cur, "value")));
                    ruleFound = true;
                }
                else if (isElement(cur->name, (const xmlChar *)"OnUnder", keyword)) {
                    rule = zMonitorRulePtr(new zMonitorRuleUnder(myxmlGetPropFloat(cur, "value")));
                    ruleFound = true;
                }
                else if (isElement(cur->name, (const xmlChar *)"InRange", keyword)) {
                    rule = zMonitorRulePtr(new zMonitorRuleInRange(myxmlGetPropFloat(cur, "low"), myxmlGetPropFloat(cur, "high")));
                    ruleFound = true;
                }
                else if (isElement(cur->name, (const xmlChar *)"OutsideRange", keyword)) {
                    rule = zMonitorRulePtr(new zMonitorRuleOutsideRange(myxmlGetPropFloat(cur, "low"), myxmlGetPropFloat(cur, "high")));
                    ruleFound = true;
                }
                else if (isElement(cur->name, (const xmlChar *)"Alternative", keyword)) {
                    if (isValidAlternative(cur, keyword)) {
                        parseAlternativeContent(grid, cur);
                    }
                }
                if (ruleFound == true ) {
                    xmlNodePtr cur2 = cur->xmlChildrenNode;
                    while (cur2 != NULL) {
                        if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
                        else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
                            rule->addEffectGroup(parseZoneEffectGroup(grid, cur2, keyword));
                        }
                        else if (isElement(cur2->name, (const xmlChar *)"ZoneInfluence", keyword)) {
                            rule->addZoneInfluence(parseZoneEffectGroupZone(grid, cur2, keyword));
                        }
                        else if (isElement(cur2->name, (const xmlChar *)"MonitorInfluence", keyword)) {
                            rule->addMonitorInfluence(parseZoneEffectGroupMonitor(grid, cur2, keyword));
                        }
                        cur2 = cur2->next;
                    }
                    ruleFound = false;
                    monitor->addRule(rule);
                }

                cur = cur->next;
            }
        }
    }
}
#endif

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

    ox = myxmlGetPropFloat(cur, "startx");
    oy = myxmlGetPropFloat(cur, "starty");
    x = myxmlGetPropFloat(cur,   "endx");
    y = myxmlGetPropFloat(cur,   "endy");
    R = grid->Insert(eCoord(ox, oy) * sizeMultiplier);
    R = this->DrawRim(grid, R, eCoord(x, y) * sizeMultiplier);
    sg_Deprecated();

    endElementAlternative(grid, cur, keyword);
}

void
gParser::parseWallRect(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) {
    REAL ox, oy, x, y;
    ePoint *R;

    ox = myxmlGetPropFloat(cur, "startx");
    oy = myxmlGetPropFloat(cur, "starty");
    x = myxmlGetPropFloat(cur,   "endx");
    y = myxmlGetPropFloat(cur,   "endy");
    R = grid->Insert(eCoord(ox, oy) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord( x, oy) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord( x,  y) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord(ox,  y) * sizeMultiplier);
    R = this->DrawRim( grid, R, eCoord(ox, oy) * sizeMultiplier);
    sg_Deprecated();

    endElementAlternative(grid, cur, keyword);
}

void
gParser::parseWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    ePoint *R = NULL, *sR = NULL;
    REAL ox, oy, x, y;

    REAL height = myxmlGetPropFloat(cur, "height");
    if ( height <= 0 )
        height = 10000;

    cur = cur->xmlChildrenNode;

    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
            x = myxmlGetPropFloat(cur, "x");
            y = myxmlGetPropFloat(cur, "y");

            if (R == NULL)
                R = grid->Insert(eCoord(x, y) * sizeMultiplier);
            else
                R = this->DrawRim(grid, R, eCoord(x, y) * sizeMultiplier, height);

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
        ox = x;
        oy = y;
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
    while ( cur != NULL) {
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
    const xmlChar * keyword = xmlGetProp(cur, (const xmlChar *) "keyword");

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
#ifdef ENABLE_ZONESV2
        // Introduced in version 2, but no extra logic is required for it.
        else if (isElement(cur->name, (const xmlChar *)"Ownership", keyword)) {
            parseOwnership(grid, cur, keyword);
        }
        else if (isElement(cur->name, (const xmlChar *)"Monitor", keyword)) {
            parseMonitor(grid, cur, keyword);
        }
#endif
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

#ifdef ENABLE_ZONESV2
/**
 *
 */
const char * TEAM_ID_STR =  "teamId";
const char * PLAYER_ID_STR = "playerId";

void
gParser::parseOwnership(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    // Prepare the structures to store the ownership information
    TeamOwnershipInfo mapIdOfTeamOwners;

    // Remove previous ownership
    playerAsso.erase(playerAsso.begin(), playerAsso.end());
    teamAsso.erase(teamAsso.begin(), teamAsso.end());

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"TeamOwnership", keyword)) {
            parseTeamOwnership(grid, cur, keyword, mapIdOfTeamOwners);
        }
        cur = cur->next;
    }

    // BOP
    // The association between teamId and in-game teams should be moved to its own class

    std::cout << "###################" << std::endl;
    std::cout << "number of teams " << eTeam::teams.Len() << std::endl;

    TeamOwnershipInfo::iterator iterTeamOwnership = mapIdOfTeamOwners.begin();
    for (int index=0; index<eTeam::teams.Len() && iterTeamOwnership != mapIdOfTeamOwners.end(); ) {
        eTeam* ee = eTeam::teams[index];
        string teamId = (*iterTeamOwnership).first;
        std::cout << "associating " << teamId << " with " << ee->Name() << " net ID:" << ee->ID() << std::endl;
        MapIdToGameId::value_type asdf(teamId, ee->ID());

        // Store the association between the map id and the in-game id
        teamAsso.insert(asdf);

        std::set<string> playerIdForThisTeam = (*iterTeamOwnership).second;
        int indexPlayer=0;
        for (std::set<string>::iterator iter = playerIdForThisTeam.begin();
                    iter != playerIdForThisTeam.end() && indexPlayer<ee->NumPlayers();
                    ++iter, ++indexPlayer) {
                ePlayerNetID *aa = ee->Player(indexPlayer);
                string playerId = (*iter);
                // TODO
                // HACK
                // BOP
                // The following might produce unexpected results should the same playerId be associated in many team
                MapIdToGameId::value_type mapOwnerToInGameOwnerPair(playerId, aa->ID());
                playerAsso.insert(mapOwnerToInGameOwnerPair);
            }
        ++index;
        ++iterTeamOwnership;
    }
    // EOP
}

void
gParser::parseTeamOwnership(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword, TeamOwnershipInfo & mapIdOfTeamOwners)
{
    // Explore the teamId attribute
    if (myxmlHasProp(cur, TEAM_ID_STR )) {
        string tOwnersDesc( myxmlGetProp(cur, TEAM_ID_STR));
        boost::tokenizer<> tokTeam(tOwnersDesc);

        for (boost::tokenizer<>::iterator tokTeamIter=tokTeam.begin();
                tokTeamIter!=tokTeam.end();
                ++tokTeamIter) {
            tString aTeamId = tString(*tokTeamIter);
            TeamOwnershipInfo::iterator teamIter = mapIdOfTeamOwners.find(aTeamId);
            // Add the teamId to the list if abscent
            if (teamIter == mapIdOfTeamOwners.end()) {
                //	teamIter = team.insert(std::pair<string, std::set<string> >( aTeamId, std::set<string> ));
                std::pair<string, std::set<string> > asdf( aTeamId, std::set<string>() );
                teamIter = mapIdOfTeamOwners.insert(teamIter, asdf);
            }

            // Should the team receive playerId
            if (myxmlHasProp(cur, PLAYER_ID_STR)) {
                // Extract all the playerId for this team
                string plOwnersDesc( myxmlGetProp(cur, PLAYER_ID_STR) );
                boost::tokenizer<> tokPlayer(plOwnersDesc);
                for (boost::tokenizer<>::iterator tokPlayerIter=tokPlayer.begin();
                        tokPlayerIter!=tokPlayer.end();
                        ++tokPlayerIter) {
                    tString aPlayerId = tString(*tokPlayerIter);

                    std::set<string> aa = (*teamIter).second;
                    aa.insert(aPlayerId);
                    (*teamIter).second = aa;
                }
            }
        }
    }
    else {
        // TeamId is #REQUIRED, this should not happen
    }
}
#endif

void
gParser::parseWorld(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword)
{
    // Polygon shapes are not supported by older clients.
    // This mechanism assume that no polygonal shapes are used until one is found.
    // Hopefully someone will come with a better solution.
    std::stringstream ss;
    /* Yes it is ackward to generate a string that will be decifered on the other end*/
    ss << "POLYGONAL_SHAPE_USED FALSE";
    tConfItemBase::LoadLine(ss);

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
        if ( tRecorder::IsPlayingBack() )
            tConfItemBase::LoadPlayback( true );
        else
            tConfItemBase::LoadAll(ss);
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
    mapVersion = myxmlGetPropInt(cur, "version");

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
gParser::setSizeMultiplier(REAL aSizeMultiplier)
{
    // BOP
    sizeMultiplier = aSizeMultiplier;
    // EOP
}

void
gParser::Parse()
{
    rimTexture = 0;
    xmlNodePtr cur;
    cur = xmlDocGetRootElement(m_Doc);

#ifdef ENABLE_ZONESV2
    monitors.clear();
#endif
#ifdef DEBUG_ZONE_SYNC
    newGameRound = true;
#endif //DEBUG_ZONE_SYNC

    if (cur == NULL) {
        con << "ERROR: Map file is blank\n";
        return;
    }

    if (isElement(cur->name, (const xmlChar *) "Resource")) {
        if (xmlStrcmp((const xmlChar *) "aamap", xmlGetProp(cur, (const xmlChar *) "type"))) {
            con << "Type aamap expected, found " << xmlGetProp(cur, (const xmlChar *) "type") << " instead\n";
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

#ifdef ENABLE_ZONESV2
    mapZones.clear();
#endif

    //        fprintf(stderr,"ERROR: Map file is missing root \'Resources\' node");

}

#ifdef ENABLE_ZONESV2
zMonitorPtr
gParser::getMonitor(string monitorName)
{
    return monitors[monitorName];
}
#endif
