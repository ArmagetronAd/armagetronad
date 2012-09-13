#ifndef Z_SHAPE_H
#define Z_SHAPE_H

#include "tValue.h"
#include "rColor.h"
#include <boost/shared_ptr.hpp>
#include "eNetGameObject.h"
#include "tSysTime.h"

#include "tFunction.h"
#include "tPolynomial.h"

extern int sz_zoneAlphaToggle;
extern int sz_zoneSegments;
extern REAL sz_zoneSegLength;
extern REAL sz_zoneBottom;
extern REAL sz_zoneHeight;
extern REAL sz_expansionSpeed;
extern REAL sz_initialSize;

namespace Zone { class ShapeSync; class ShapeCircleSync; class ShapePolygonSync; }
namespace Game { class ZoneV1Sync; }

class zZone;
class gParserState;

class zShape : public eNetGameObject{
public:
    zShape(eGrid * grid, zZone * zone );
    virtual ~zShape() {};

    //! creates a netobject form sync data
    zShape( Zone::ShapeSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Zone::ShapeSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Zone::ShapeSync & sync, bool init ) const;

    virtual void applyVisuals( gParserState & );

    void animate( REAL time );
    virtual bool isInteracting(eGameObject * target);
    virtual void Render(const eCamera * cam );
    virtual void Render2D(tCoord scale) const;
    virtual bool RendersAlpha() const;

    virtual tCoord findPointNear(tCoord&) = 0;
    virtual tCoord findPointFar(tCoord&) = 0;
    virtual REAL calcDistanceNear(tCoord&);
    virtual REAL calcDistanceFar(tCoord&);

    virtual tCoord findCenter();
    //! Calculates the max(height, width) of a bounding box
    virtual REAL calcBoundSq();

    void setPosX(const tFunction &x);
    void setPosY(const tFunction &y);
    virtual
    void setRotation2(const tPolynomial & r);

    virtual
    void setScale(const tFunction &s);
    void setColor(const rColor &c);

    void setColorNow(const rColor &c);

    virtual tCoord Position() const;
    tFunction getPosX() {return posx_;};
    tFunction getPosY() {return posy_;};
    tPolynomial getRotation2() { return rotation2; };
    REAL GetCurrentScale() const;
    tFunction getScale() {return scale_;};
    rColor getColor() {return color_;};

    //! shortcut rotation functions
    tCoord GetRotation() const;
    REAL GetRotationSpeed();
    void SetRotationSpeed(REAL r);
    REAL GetRotationAcceleration();
    void SetRotationAcceleration(REAL r);

    // Get/Set visual settings
    REAL GetEffectiveBottom() const;
    REAL GetEffectiveHeight() const;
    int  GetEffectiveSegments() const;
    REAL GetEffectiveSegmentLength() const;
    int  GetEffectiveSegmentSteps() const;
    REAL GetEffectiveFloorScalePct() const;
    REAL GetEffectiveProximityDistance() const;
    REAL GetEffectiveProximityOffset() const;

    void SetBottom(const tPolynomial & r);
    void SetHeight(const tPolynomial & r);
    void SetSegments(const tPolynomial & r);
    void SetSegmentLength(const tPolynomial & r);
    void SetSegmentSteps(const tPolynomial & r);
    void SetFloorScalePct(const tPolynomial & r);
    void SetProximityDistance(const tPolynomial & r);
    void SetProximityOffset(const tPolynomial & r);

    bool Timestep( REAL time );
    virtual void setReferenceTime(REAL time);

    virtual void setGrowth(REAL growth);  //!< similar to old zones v1 setExpansionSpeed, but generic
    virtual void collapse(REAL speed);  //!< set growth such that collapse happens in a timeframe

private:
    //! called immediately after the object is created, either right after round beginning or mid-game creation
    virtual void OnBirth();

public:  // DEPRECATED -- DO NOT USE
    void __deprecated render(const eCamera*cam) { Render(cam); }
    void __deprecated render2d(tCoord&scale) { Render2D(scale); }
    void __deprecated TimeStep(REAL time) { Timestep(time); }

protected:
    tFunction posx_; //!< position need not be inside the shape.
    tFunction posy_; //!< positoin need not be inside the shape.
    tPolynomial bottom_; //!< Z position off the grid (bottom of zone)
    tFunction scale_; //!< Used to affect the contour and not the position
    tPolynomial height_; //!< Height of the zone, not affected by scale
    tPolynomial rotation2; //!< Rotate the contour around the position at this rate.
    tPolynomial segments_; //!< Number of segments to make up the zone
    tPolynomial seglength_; //!< Length of each segment making up the zone
    tPolynomial segsteps_; //!< Number of steps to draw each segment making up the zone
    tPolynomial floorscalepct_; //!< percentage of zone's size used to outline zone on the floor
    tPolynomial proximitydistance_; //!< distance from which zone's height start to shrink
    tPolynomial proximityoffset_; //!< offset distance from the zone border where the height reach 0 
    rColor color_;

    void setCreatedTime(REAL time);

    REAL createdtime_; // The in-game time when this shape was first instantiated
    REAL referencetime_; // The in-game time when this shape's data was updated, used for evaluation
};

class zShapeCircle : public zShape {
  // HACK 
  // Circle dont have a radius attribute per see.
  // They have a default radius of 1.0, and it get scaled
public :
    zShapeCircle(eGrid * grid, zZone * zone );
    ~zShapeCircle() {};

    //! creates a netobject form sync data
    zShapeCircle( Zone::ShapeCircleSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Zone::ShapeCircleSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Zone::ShapeCircleSync & sync, bool init ) const;

    void applyVisuals( gParserState & );

    bool isInteracting(eGameObject * target);
    void Render(const eCamera * cam );
	virtual void Render2D(tCoord scale) const;

    virtual void setReferenceTime(REAL time);

    tCoord findPointNear(tCoord&);
    tCoord findPointFar(tCoord&);

    //! Calculates the max(height, width) of a bounding box
    REAL calcBoundSq();

    void setRadius(tFunction radius) {this->radius = radius;};
    
    void setGrowth(REAL growth);
protected:
    tFunction radius;

private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
    
};

typedef std::pair<tFunction, tFunction> myPoint;

class zShapePolygon : public zShape {
public :
    zShapePolygon(eGrid *grid, zZone * zone );
    ~zShapePolygon() {};

    //! creates a netobject form sync data
    zShapePolygon( Zone::ShapePolygonSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Zone::ShapePolygonSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Zone::ShapePolygonSync & sync, bool init ) const;

    bool isInteracting(eGameObject * target);
    void Render(const eCamera * cam );
    virtual void Render2D(tCoord scale) const;
    void addPoint( myPoint const &aPoint) { points.push_back(aPoint);};

    tCoord findPointNear(tCoord&);
    tCoord findPointFar(tCoord&);

    //! Calculates the max(height, width) of a bounding box
    REAL calcBoundSq();

protected:
    std::vector< myPoint > points;
    bool isInside (eCoord anECoord);
private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
};

//typedef boost::shared_ptr<zShape> zShapePtr;
#include "tSafePTR.h"
typedef tJUST_CONTROLLED_PTR< zShape> zShapePtr;

#include "tSysTime.h"

#ifndef ENABLE_ZONESV1
class zShapeCircleZoneV1 : public zShapeCircle {
    zShapeCircleZoneV1( eGrid *, zZone * );
    ~zShapeCircleZoneV1() {};

public:
    //! creates a netobject form sync data
    zShapeCircleZoneV1( Game::ZoneV1Sync const &, nSenderInfo const & );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Game::ZoneV1Sync const &, nSenderInfo const & );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Game::ZoneV1Sync &, bool init ) const;

private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
};
#endif


#endif
