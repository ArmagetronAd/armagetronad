
#ifndef ArmageTron_PARSER_H
#define ArmageTron_PARSER_H

#include "defs.h"
#include "tXmlParser.h"
#include "eCoord.h"
#include "tValue.h"

class eGrid;
class gArena;
class ePoint;
class gGame;
class gWallRim;

#include "zone/zZone.h"
#include "zone/zMisc.h"

/*
Note to the reader: In the full World idea, the parser should, 
when called, create a full world structure, from the Empire down,
and just return a pointer to it(He, whats to forbit a server from
running 2 independant games?). 

Due to the limitations of the current code, it is best to have a 
pointer to the gGame, the gArena and the gGrid.

*/

class gParser : public tXmlResource {

    gArena *theArena; /*Patch: All the world structure should be created by the parser*/
    eGrid *theGrid; /*Patch: All the world structure should be created by the parser*/

    REAL rimTexture; /* The rim wall texture coordinate */

    ePoint * DrawRim( eGrid * grid, ePoint * start, eCoord const & stop, REAL h=10000 ); /* Draws a rim wall segment */

public:
    gParser(gArena *anArena, eGrid *aGrid);
    //    gParser(const gGame *aGame, gArena *anArena, tControlledPTR<eGrid> aGrid);
    void setSizeMultiplier(REAL aSizeMultiplier);
    void Parse();

protected:
    bool trueOrFalse(char *str);
    char *myxmlGetProp(xmlNodePtr cur, const char *name);
    int myxmlGetPropInt(xmlNodePtr cur, const char *name);
    float myxmlGetPropFloat(xmlNodePtr cur, const char *name);
    bool myxmlGetPropBool(xmlNodePtr cur, const char *name);
    Triad myxmlGetPropTriad(xmlNodePtr cur, const char *name);
    void myxmlGetDirection(xmlNodePtr cur, float &x, float &y);

    //    bool isElement(const xmlChar *elementName, const xmlChar *searchedElement);
    bool isElement(const xmlChar *elementName, const xmlChar *searchedElement, const xmlChar * keyword = NULL);
    bool isValidAlternative(xmlNodePtr cur, const xmlChar * keyword = NULL);
    void processSubAlt(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword = NULL);
    bool isValidCodeName(const xmlChar *version);
    bool isValidDotNumber(const xmlChar *version);
    bool validateVersionSubRange(const xmlChar *subRange, const xmlChar *codeName, const xmlChar *dotVersion);
    bool xmlCharSearchReplace(xmlChar *&original, const xmlChar * searchPattern, const xmlChar * replace);
    int validateVersionRange(xmlChar *version, const xmlChar * codeName, const xmlChar * dotVersion);
    void endElementAlternative(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    void parseAxes(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseSpawn(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseZone(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    rColor parseColor(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    void parseShapeCircle(eGrid *grid, xmlNodePtr cur, float &x, float &y, float &radius, float &growth, rColor &color, const xmlChar * keyword);
    void                 parseMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);

    zEffectGroupPtr      parseZoneEffectGroup(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    zMonitorInfluencePtr parseZoneEffectGroupMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zEffectorPtr         parseZoneEffectGroupEffector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zSelectorPtr         parseZoneEffectGroupSelector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zValidatorPtr        parseZoneEffectGroupValidator(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zZoneInfluencePtr    parseZoneEffectGroupZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);


    void parseField(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseWorld(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword = NULL);
    void parseMap(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword = NULL);

    void parseSettings(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword = NULL);
    void parseSetting(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    void parseAlternativeContent(eGrid *grid, xmlNodePtr cur);
    /* Experimental features*/
    void parseWallLine(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseWallRect(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseObstacleWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    /* This is a hack that will bring shame to my decendants for many generations: */
    float sizeMultiplier;
};

#endif //ArmageTron_PARSER_H

