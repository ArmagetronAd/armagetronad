
#ifndef ArmageTron_PARSER_H
#define ArmageTron_PARSER_H

#include "defs.h"
#include "tString.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

class eGrid;
class gArena;
class eCoord;
class ePoint;
class gGame;
class gWallRim;
class gXMLCharReturn;
class gSpawnPoint;

/*
Note to the reader: In the full World idea, the parser should,
when called, create a full world structure, from the Empire down,
and just return a pointer to it(He, whats to forbit a server from
running 2 independant games?).

Due to the limitations of the current code, it is best to have a
pointer to the gGame, the gArena and the gGrid.

*/

class gParser {

    gArena *theArena; /*Patch: All the world structure should be created by the parser*/
    eGrid *theGrid; /*Patch: All the world structure should be created by the parser*/

    xmlDocPtr doc; /* The map xml document */

    REAL rimTexture; /* The rim wall texture coordinate */

    ePoint * DrawRim( eGrid * grid, ePoint * start, eCoord const & stop, REAL h=10000 ); /* Draws a rim wall segment */

public:
    gParser(gArena *anArena, eGrid *aGrid);
    //    gParser(const gGame *aGame, gArena *anArena, tControlledPTR<eGrid> aGrid);
    /*Sorry, I just cant figure when you'd want to load without
      validating, or revalidate a document. So I joined both
      toghether */
    ~gParser();

    bool LoadAndValidateMapXML(char const *uri, FILE* docfd, char const * filepath );
    void InstantiateMap(float sizeMultiplier);

protected:
    bool trueOrFalse(char *str);
    gXMLCharReturn myxmlGetProp(xmlNodePtr cur, const char *name);
    int myxmlGetPropInt(xmlNodePtr cur, const char *name);
    float myxmlGetPropFloat(xmlNodePtr cur, const char *name);
    bool myxmlGetPropBool(xmlNodePtr cur, const char *name);
    void myxmlGetDirection(xmlNodePtr cur, float &x, float &y);
    tString myxmlGetPropString(xmlNodePtr cur, const char *name);

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

    bool parseShapeCircle(eGrid *grid, xmlNodePtr cur, float &x, float &y, float &radius, float &growth, const xmlChar * keyword);

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

extern tString mapName;

#endif //ArmageTron_PARSER_H

