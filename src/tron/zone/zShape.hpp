#ifndef Z_SHAPE_H
#define Z_SHAPE_H

#include "tValue.h"
#include "rColor.h"
#include <boost/shared_ptr.hpp>
#include "eNetGameObject.h"
#include "tSysTime.h"

#include "tFunction.h"
#include "tPolynomial.h"

namespace Zone { class ShapeSync; class ShapeCircleSync; class ShapePolygonSync; }

class zZone;

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

    void animate( REAL time );
    virtual bool isInteracting(eGameObject * target);
    virtual void render(const eCamera * cam );
    virtual void render2d(tCoord scale) const;

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
    tPolynomial<nMessage> getRotation2() { return rotation2; };
  tFunction getScale() {return scale_;};
  rColor getColor() {return color_;};

    void TimeStep( REAL time );
    void setReferenceTime(REAL time);

    virtual void setGrowth(REAL growth);  //!< similar to old zones v1 setExpansionSpeed, but generic
    virtual void collapse(REAL speed);  //!< set growth such that collapse happens in a timeframe

protected:
    tFunction posx_; //!< position need not be inside the shape.
    tFunction posy_; //!< positoin need not be inside the shape.
    tFunction scale_; //!< Used to affect the contour and not the position
    tPolynomial rotation2; //!< Rotate the contour around the position at this rate.
    rColor color_;

    eCoord Position() { return eCoord(posx_(lasttime_ - referencetime_), posy_(lasttime_ - referencetime_) ); };

    void setCreatedTime(REAL time);

    REAL createdtime_; // The in-game time when this shape was first instantiated
    REAL referencetime_; // The in-game time when this shape's data was updated, used for evaluation
    REAL lasttime_; // What is to be considered as the current time for this shape
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

    bool isInteracting(eGameObject * target);
    void render(const eCamera * cam );
	virtual void render2d(tCoord scale) const;

    void setRotation2(const tPolynomial<nMessage> & r);
    void setScale(const tFunction &s);
    void setRadius(tFunction radius) {this->radius = radius;};
    
    void setGrowth(REAL growth);
protected:
    tFunction radius;

private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;

    tFunction * _cacheScaledRadius;
    tFunction * _cacheRotationF;
    REAL _cacheTime;
    
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
    void render(const eCamera * cam );
    virtual void render2d(tCoord scale) const;
    void addPoint( myPoint const &aPoint) { points.push_back(aPoint);};

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


#endif
