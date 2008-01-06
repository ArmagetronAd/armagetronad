
#ifndef ArmageTron_PARSER_H
#define ArmageTron_PARSER_H

#include "defs.h"
#include "tXmlParser.h"
#include "eCoord.h"
#include "tValue.h"
#include <map>
#include <string>

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
    zMonitorPtr getMonitor(string monitorName);

protected:
    bool trueOrFalse(char *str);
    char *myxmlGetProp(xmlNodePtr cur, const char *name);
    int myxmlGetPropInt(xmlNodePtr cur, const char *name);
    float myxmlGetPropFloat(xmlNodePtr cur, const char *name);
    bool myxmlGetPropBool(xmlNodePtr cur, const char *name);
    Triad myxmlGetPropTriad(xmlNodePtr cur, const char *name);
    void myxmlGetDirection(xmlNodePtr cur, float &x, float &y);
    rColor myxmlGetPropColorFromHex(xmlNodePtr cur, const char *name);

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
    void parseZoneArthemis(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseZoneBachus(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);

    void parseWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    rColor parseColor(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

    zShapePtr parseShapeCircleArthemis(eGrid *grid, xmlNodePtr cur, short unsigned int idZone, const xmlChar * keyword);
    zShapePtr parseShapeCircleBachus(eGrid *grid, xmlNodePtr cur, short unsigned int idZone, const xmlChar * keyword);
    zShapePtr parseShapePolygon(eGrid *grid, xmlNodePtr cur, short unsigned int idZone, const xmlChar * keyword);
    void      parseShape(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword, zShapePtr &shape);
    void                 parseMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);

    zEffectGroupPtr      parseZoneEffectGroup(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    zMonitorInfluencePtr parseZoneEffectGroupMonitor(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zEffectorPtr         parseZoneEffectGroupEffector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zSelectorPtr         parseZoneEffectGroupSelector(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zValidatorPtr        parseZoneEffectGroupValidator(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    zZoneInfluencePtr    parseZoneEffectGroupZone(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);


    typedef std::map<string, std::set<string> > TeamOwnershipInfo;
    typedef std::map<string, std::set<string> >::value_type TeamOwnershipInfoType;
    void parseOwnership(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseTeamOwnership(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword, TeamOwnershipInfo & team);
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

    void myCheapParameterSplitter(const string &str, tFunction &tf, bool addSizeMultiplier=false);
    void myCheapParameterSplitter2(const string &str, tPolynomial<nMessage> &tp, bool addSizeMultiplier=false);

    /* This is a hack that will bring shame to my decendants for many generations: */
    float sizeMultiplier;
    int currentFormat; // Store the format version of the map currently being parsed. Used to support different format.
public:
    tValue::Expr::varmap_t vars;
    tValue::Expr::funcmap_t functions;

};

#endif //ArmageTron_PARSER_H

