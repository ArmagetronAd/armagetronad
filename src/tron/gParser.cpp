#include "rSDL.h"

#include "config.h"

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

#include "gGame.h"

#include "tConfiguration.h"
#include "gGame.h"
#include <boost/tokenizer.hpp> // to support splitting a string on "," 
#include <boost/shared_ptr.hpp>

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

typedef std::map< string, zMonitorPtr > monitorMap;
monitorMap monitors;

typedef std::map< string, zZonePtr > zoneMap;
zoneMap mapZones;


//! Warn about deprecated map format
static void sg_Deprecated()
{
    // no use informing players on the client, they can't do anything about it anyway
    if ( sn_GetNetState() != nCLIENT )
        con << "\n\n" << tColoredString::ColorString(1,.3,.3) << "WARNING: You are loading a map that is written in a deprecated format. It should work for now, but will stop to do so in one of the next releases. Have the map upgraded to an up-to-date version as soon as possible.\n\n";
}

gParser::gParser(gArena *anArena, eGrid *aGrid):
  theArena(anArena),
  theGrid(aGrid),
  rimTexture(0),
  sizeMultiplier(0.0)
{
    m_Doc = NULL;
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

    theArena->NewSpawnPoint(eCoord(x, y), eCoord(xdir, ydir));

    endElementAlternative(grid, cur, keyword);
}

rColor
gParser::parseColor(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword) 
{
    rColor color;
    color.r_ = myxmlGetPropFloat(cur, "red");
    color.g_ = myxmlGetPropFloat(cur, "green");
    color.b_ = myxmlGetPropFloat(cur, "blue");
    color.a_ = myxmlGetPropFloat(cur, "alpha");
  
    return color;
}

void
gParser::parseShapeCircle(eGrid *grid, xmlNodePtr cur, float &x, float &y, float &radius, float &growth, rColor & color, const xmlChar * keyword)
{
    radius = myxmlGetPropFloat(cur, "radius");
    growth = myxmlGetPropFloat(cur, "growth");

    cur = cur->xmlChildrenNode;
    while( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"Point", keyword)) {
	  /* We need to multipy by sizeMultiper so the item are properly placed*/
            x = myxmlGetPropFloat(cur, "x") *sizeMultiplier;
            y = myxmlGetPropFloat(cur, "y") *sizeMultiplier;

            endElementAlternative(grid, cur, keyword);
        }
	else if (isElement(cur->name, (const xmlChar *)"Color", keyword)) {
            color = parseColor(grid, cur, keyword);

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
  if((iterZone = mapZones.find(zoneName)) != mapZones.end()) {
    // load the zone
    refZone = iterZone->second;
  }
  else {
    // make an empty zone and store under the right label
    // It should be populated later
    refZone = zZonePtr(new zZone(grid));
    if (zoneName.size()!=0)
      mapZones[zoneName] = refZone;
  }
  infl = zZoneInfluencePtr(new zZoneInfluence(refZone));

  cur = cur->xmlChildrenNode;
  while( cur != NULL) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
    else if (isElement(cur->name, (const xmlChar *)"Rotation", keyword)) {
      zZoneInfluenceItemRotation *b = new zZoneInfluenceItemRotation(refZone);
      b->set(myxmlGetPropFloat(cur, "rotationSpeed"), myxmlGetPropFloat(cur, "rotationAcceleration"));
      infl->addZoneInfluenceRule(zZoneInfluenceItemPtr(b));
    }
    else if (isElement(cur->name, (const xmlChar *)"Radius", keyword)) {
      zZoneInfluenceItemRadius *b = new zZoneInfluenceItemRadius(refZone);
      b->set(myxmlGetPropFloat(cur, "radius") *sizeMultiplier);
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
    if((iterMonitor = monitors.find(monitorName)) != monitors.end()) {
      ref = iterMonitor->second;
    }
    else {
      // make an empty zone and store under the right label
      // It should be populated later
      ref = zMonitorPtr(new zMonitor(grid));
      if (monitorName.size()!=0)
	monitors[monitorName] = ref;
    }

    zMonitorInfluencePtr infl = zMonitorInfluencePtr(new zMonitorInfluence(ref));
    infl->setMarked(myxmlGetPropTriad(cur, "marked"));

    if(xmlHasProp(cur, (const xmlChar*)"influenceSlide"))
      infl->setInfluenceSlide(myxmlGetPropFloat(cur, "influenceSlide"));
    
    if(xmlHasProp(cur, (const xmlChar *)"influenceAdd"))
      infl->setInfluenceAdd(myxmlGetPropFloat(cur, "influenceAdd"));
    
    if(xmlHasProp(cur, (const xmlChar *)"influenceSet"))
      infl->setInfluenceSet(myxmlGetPropFloat(cur, "influenceSet"));
    


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
      effectors[tString("cleartrace")] = zEffectorClearTrace::create;
      effectors[tString("teleport")] = zEffectorTeleport::create;
    */
    effectors[tString("spawnplayer")] = zEffectorSpawnPlayer::create;
    effectors[tString("brakerecharge")] = zEffectorCycleBrake::create;
    effectors[tString("rubberrecharge")] = zEffectorCycleRubber::create;


    // TODO: add tolower()
    // Get the label of the effector to be used
    string effectorAttribute( myxmlGetProp(cur, "effect"));
    transform (effectorAttribute.begin(), effectorAttribute.end(), effectorAttribute.begin(), tolower);
    std::map<tString, effectorFactory>::const_iterator iterEffectorFactory;
    // associate the label to the proper effector
    if((iterEffectorFactory = effectors.find(effectorAttribute)) != effectors.end()) {
    
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

    // Should we set the grid and arena for respawning
    zEffectorSpawnPlayer *effectorSpawnPlayer;
    effectorSpawnPlayer = dynamic_cast<zEffectorSpawnPlayer *>(effector.get());
    if (effectorSpawnPlayer) {
      effectorSpawnPlayer->setGrid(grid);
      effectorSpawnPlayer->setArena(sg_GetArena());
    }

    effector->setCount(myxmlGetPropInt(cur, "count"));

    if (xmlHasProp(cur, (const xmlChar*) "description"))
      effector->setMessage(myxmlGetProp(cur, "description"));

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
    if((iterSelectorFactory = selectors.find(selectorAttribute)) != selectors.end()) {
    
      selector = zSelectorPtr((*(iterSelectorFactory->second))());

      selector->setCount(myxmlGetPropInt(cur, "count"));

      cur = cur->xmlChildrenNode;
      while( cur != NULL) {
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
  /*
    validators[tString("allbutteamowner")] = zValidatorAllButTeamOwner::create;
    validators[tString("anotherteammate")] = zValidatorTeammate::create;
  */

  // TODO: add tolower()
  // Get the label of the validator to be used
  string validatorAttribute( myxmlGetProp(cur, "user"));
  transform (validatorAttribute.begin(),validatorAttribute.end(), validatorAttribute.begin(), tolower);

  std::map<tString, validatorFactory>::const_iterator iterValidatorFactory;
  // associate the label to the proper validator
  if((iterValidatorFactory = validators.find(validatorAttribute)) != validators.end()) {
    
    validator = zValidatorPtr((*(iterValidatorFactory->second))(positive, marked));
    /*
      Save the validator for the zone effect
    */
  }


    cur = cur->xmlChildrenNode;
    while( cur != NULL) {
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
  gVectorExtra< ePlayerNetID * > owners;

  if(xmlHasProp(cur, (const xmlChar*)"owners"))
    {
      string ownersDesc( myxmlGetProp(cur, "owners"));
      boost::tokenizer<> tok(ownersDesc);

      for(boost::tokenizer<>::iterator iter=tok.begin();
	  iter!=tok.end();
	  ++iter) 
	{
	  /*
	   * Map from map descriptor to in-game ids
	   */
	  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
	    // TODO: change this to a call to Joda's code
	    if ( se_PlayerNetIDs(i)->GetName() == tString(*iter) ) {
	      owners.push_back( se_PlayerNetIDs(i) );
	    }
	  }
	}
    }

  /*
   * Prepare a new EffectGroup
   */
  zEffectGroupPtr currentZoneEffect = zEffectGroupPtr(new zEffectGroup(owners, gVectorExtra<eTeam *>()));


    cur = cur->xmlChildrenNode;
    while( cur != NULL) {
        if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
        else if (isElement(cur->name, (const xmlChar *)"User", keyword)) {
	    currentZoneEffect->setValidator(parseZoneEffectGroupValidator(grid, cur, keyword));
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
gParser::parseZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
  float x=0.0, y=0.0, radius=0.0, growth=0.0;
  string zoneName = "";

  if(myxmlHasProp(cur, "name")) 
    zoneName = myxmlGetProp(cur, "name");

  cur = cur->xmlChildrenNode;
    
  if (sn_GetNetState() != nCLIENT )
    {
      rColor color;

      zZonePtr zone;
      zoneMap::const_iterator iterZone;
      // Has this zone been already registered, such as through a zoneInfluence
      if((iterZone = mapZones.find(zoneName)) != mapZones.end()) {
	// Open the zone so we can fill in the details
	zone = iterZone->second;
      }
      else {
	// Create a new zone 
	zone = zZonePtr(new zZone(grid));
	// If a name was assigned to it, save the zone in a map so it can be refered to
	if (zoneName.size()!=0)
	  mapZones[zoneName] = zone;
      }

      while(cur != NULL) {
	if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
	else if (isElement(cur->name, (const xmlChar *)"ShapeCircle", keyword)) {
	  parseShapeCircle(grid, cur, x, y, radius, growth, color, keyword);
	}
	else if (isElement(cur->name, (const xmlChar *)"Enter", keyword)) {
	  xmlNodePtr cur2 = cur->xmlChildrenNode;
	  while(cur2 != NULL) {
	    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
	    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
	      zone->addEffectGroupEnter(parseZoneEffectGroup(grid, cur2, keyword));
	    }
	    cur2 = cur2->next;
	  }
	}
	else if (isElement(cur->name, (const xmlChar *)"Inside", keyword)) {
	  xmlNodePtr cur2 = cur->xmlChildrenNode;
	  while(cur2 != NULL) {
	    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
	    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
	      zone->addEffectGroupInside(parseZoneEffectGroup(grid, cur2, keyword));
	    }
	    cur2 = cur2->next;
	  }
	}
	else if (isElement(cur->name, (const xmlChar *)"Leave", keyword)) {
	  xmlNodePtr cur2 = cur->xmlChildrenNode;
	  while(cur2 != NULL) {
	    if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
	    else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
	      zone->addEffectGroupLeave(parseZoneEffectGroup(grid, cur2, keyword));
	    }
	    cur2 = cur2->next;
	  }
	}
	else if (isElement(cur->name, (const xmlChar *)"Outside", keyword)) {
	  xmlNodePtr cur2 = cur->xmlChildrenNode;
	  while(cur2 != NULL) {
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
	  zone->SetColor(color);
	  zone->SetPosition( eCoord(x, y) );
	  zone->SetRadius( radius * sizeMultiplier );
	  zone->SetExpansionSpeed( growth*sizeMultiplier );
	  zone->SetRotationSpeed( .3f );
	  zone->RequestSync();
	}
    }
}

void
gParser::parseMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword)
{
  if (sn_GetNetState() != nCLIENT )
    {
      zMonitorPtr monitor;
      
      string monitorName(myxmlGetProp(cur, "name"));
      monitorMap::const_iterator iterMonitor;
      // associate the label to the proper effector
      if((iterMonitor = monitors.find(monitorName)) != monitors.end()) {
	monitor = iterMonitor->second;
      }
      else {
	// make an empty zone and store under the right label
	// It should be populated later
	monitor = zMonitorPtr(new zMonitor(grid));
	if (monitorName.size()!=0)
	  monitors[monitorName] = monitor;
      }

      monitor->setInit(myxmlGetPropFloat(cur, "init"));
      monitor->setDrift(myxmlGetPropFloat(cur, "drift"));
      monitor->setClampLow (myxmlGetPropFloat(cur, "low"));
      monitor->setClampHigh(myxmlGetPropFloat(cur, "high"));

      cur = cur->xmlChildrenNode;
    
      if (sn_GetNetState() != nCLIENT )
	{
	  //std::auto_ptr<zMonitor> monitor = std::auto_ptr<zMonitor>();
	  //zMonitor *monitor = tNEW(zMonitor) ();


	  zMonitorRulePtr rule;

	  while(cur != NULL) {
	    if (!xmlStrcmp(cur->name, (const xmlChar *)"text") || !xmlStrcmp(cur->name, (const xmlChar *)"comment")) {}
	    else if (isElement(cur->name, (const xmlChar *)"OnOver", keyword)) {
	      rule = zMonitorRulePtr(new zMonitorRuleOver(myxmlGetPropFloat(cur, "value")));
	      xmlNodePtr cur2 = cur->xmlChildrenNode;
	      while(cur2 != NULL) {
		if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
		else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
		  rule->addEffectGroup(parseZoneEffectGroup(grid, cur2, keyword));
		}
		cur2 = cur2->next;
	      }
	      monitor->addRule(rule);
	    }
	    else if (isElement(cur->name, (const xmlChar *)"OnUnder", keyword)) {
	      rule = zMonitorRulePtr(new zMonitorRuleUnder(myxmlGetPropFloat(cur, "value")));
	      xmlNodePtr cur2 = cur->xmlChildrenNode;
	      while(cur2 != NULL) {
		if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
		else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
		  rule->addEffectGroup(parseZoneEffectGroup(grid, cur2, keyword));
		}
		cur2 = cur2->next;
	      }
	      monitor->addRule(rule);
	    }
	    else if (isElement(cur->name, (const xmlChar *)"InRange", keyword)) {
	      rule = zMonitorRulePtr(new zMonitorRuleInRange(myxmlGetPropFloat(cur, "low"), myxmlGetPropFloat(cur, "high")));
	      xmlNodePtr cur2 = cur->xmlChildrenNode;
	      while(cur2 != NULL) {
		if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
		else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
		  rule->addEffectGroup(parseZoneEffectGroup(grid, cur2, keyword));
		}
		cur2 = cur2->next;
	      }
	      monitor->addRule(rule);
	    }
	    else if (isElement(cur->name, (const xmlChar *)"OutsideRange", keyword)) {
	      rule = zMonitorRulePtr(new zMonitorRuleOutsideRange(myxmlGetPropFloat(cur, "low"), myxmlGetPropFloat(cur, "high")));
	      xmlNodePtr cur2 = cur->xmlChildrenNode;
	      while(cur2 != NULL) {
		if (!xmlStrcmp(cur2->name, (const xmlChar *)"text") || !xmlStrcmp(cur2->name, (const xmlChar *)"comment")) {}
		else if (isElement(cur2->name, (const xmlChar *)"EffectGroup", keyword)) {
		  rule->addEffectGroup(parseZoneEffectGroup(grid, cur2, keyword));
		}
		cur2 = cur2->next;
	      }
	      monitor->addRule(rule);
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
        cur = cur->next;	ox = x;	oy = y;
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
	else if (isElement(cur->name, (const xmlChar *)"Monitor", keyword)) {
            parseMonitor(grid, cur, keyword);
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

    monitors.clear();
    mapZones.clear();

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

    //        fprintf(stderr,"ERROR: Map file is missing root \'Resources\' node");

}
