
#ifndef ArmageTron_PARSER_H
#define ArmageTron_PARSER_H

#include "defs.h"
#include "tResource.h"
#include "eCoord.h"
#include "tValue.h"
#include <map>
#include <string>

class eGrid;
class gArena;
class ePoint;
class gGame;
class gWallRim;
class gXMLCharReturn;

#ifdef ENABLE_ZONESV2
#include <boost/any.hpp>
#include "zone/zShape.h"
#include "zone/zZone.h"
#include "zone/zMisc.h"

class gParserState {
private:
    typedef std::map<std::string, boost::shared_ptr<boost::any> > my_map_t;
    std::deque< my_map_t > _varstack;
public:
    gParserState();

    bool exists(std::string const & var) const;
    bool isset(std::string const & var) const;
    template<typename T> bool istype(std::string const & var) const {
        if (!isset(var))
            return false;
        try {
            boost::any_cast<T>(getAny(var));
            return true;
        }
        catch (const boost::bad_any_cast &)
        {
            return false;
        }
    }
    boost::any getAny(std::string const & var) const;
    template<typename T> T get(std::string const & var) const {
        return boost::any_cast<T>(getAny(var));
    }
    void setAny(std::string const & var, boost::any val);
    template<typename T> void set(std::string const & var, T val) {
        setAny(var, boost::any(val));
    }
    void unset(std::string const & var);
    void inherit(std::string const & var);

    void push();
    void pop();
};
#endif

/*
Note to the reader: In the full World idea, the parser should, 
when called, create a full world structure, from the Empire down,
and just return a pointer to it(He, whats to forbit a server from
running 2 independant games?). 

Due to the limitations of the current code, it is best to have a 
pointer to the gGame, the gArena and the gGrid.

*/

class gParser : public tResource {

    gArena *theArena; /*Patch: All the world structure should be created by the parser*/
    eGrid *theGrid; /*Patch: All the world structure should be created by the parser*/

    REAL rimTexture; /* The rim wall texture coordinate */

    ePoint * DrawRim( eGrid * grid, ePoint * start, eCoord const & stop, REAL h=10000 ); /* Draws a rim wall segment */

public:
    gParser(gArena *anArena, eGrid *aGrid);
    //    gParser(const gGame *aGame, gArena *anArena, tControlledPTR<eGrid> aGrid);
    void setSizeMultiplier(REAL aSizeMultiplier);
    void Parse();
#ifdef ENABLE_ZONESV2
    zMonitorPtr getMonitor(string monitorName);
#endif

protected:
    bool trueOrFalse(char *str);
    gXMLCharReturn myxmlGetProp(xmlNodePtr cur, const char *name);
    int myxmlGetPropInt(xmlNodePtr cur, const char *name);
    float myxmlGetPropFloat(xmlNodePtr cur, const char *name);
    bool myxmlGetPropBool(xmlNodePtr cur, const char *name);
#ifdef ENABLE_ZONESV2
    Triad myxmlGetPropTriad(xmlNodePtr cur, const char *name);
    rColor myxmlGetPropColorFromHex(xmlNodePtr cur, const char *name);
#endif
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
    bool parseZoneArthemis_v1(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
#ifdef ENABLE_ZONESV2
    void parseZoneArthemis_v2(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    void parseZoneBachus(eGrid * grid, xmlNodePtr cur, const xmlChar * keyword);
    rColor parseColor(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);
#endif

    void parseWall(eGrid *grid, xmlNodePtr cur, const xmlChar * keyword);

#ifdef ENABLE_ZONESV2
    zShapePtr parseShapeCircleArthemis(eGrid *grid, xmlNodePtr cur, zZone * zone, const xmlChar * keyword);
    zShapePtr parseShapeCircleBachus(eGrid *grid, xmlNodePtr cur, zZone * zone, const xmlChar * keyword);
    zShapePtr parseShapePolygon(eGrid *grid, xmlNodePtr cur, zZone * zone, const xmlChar * keyword);
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
#endif
	bool parseShapeCircle(eGrid *grid, xmlNodePtr cur, float &x, float &y, float &radius, float &growth, const xmlChar *keyword);

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

#ifdef ENABLE_ZONESV2
    void __deprecated myCheapParameterSplitter(const string &str, tFunction &tf, bool addSizeMultiplier=false);
    void __deprecated myCheapParameterSplitter2(const string &str, tPolynomial &tp, bool addSizeMultiplier=false);
#endif

    /* This is a hack that will bring shame to my decendants for many generations: */
    float sizeMultiplier;
#ifdef ENABLE_ZONESV2
    int currentFormat; // Store the format version of the map currently being parsed. Used to support different format.
public:
    tValue::Expr::varmap_t vars;
    tValue::Expr::funcmap_t functions;

    typedef gParserState State_t;
    State_t state;
    
    std::map< std::string, std::vector< zZoneInfluencePtr > > ZIPtoMap;
    
    gArena * __deprecated contextArena(tXmlParser::node const &);
    eGrid * __deprecated contextGrid(tXmlParser::node const &);
#endif
};

#endif //ArmageTron_PARSER_H

