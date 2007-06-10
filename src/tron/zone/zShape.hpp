#include "tValue.h"
#include "rColor.h"
#include <boost/shared_ptr.hpp>
#include "eNetGameObject.h"
#include "tSysTime.h"

#ifdef DADA
using namespace tValue;
#else
#include "tFunction.h"
#endif

#ifndef Z_SHAPE_H
#define Z_SHAPE_H

class zShape : public eNetGameObject{
public:
    zShape(eGrid* grid, unsigned short idZone);
    zShape(nMessage &m);
    virtual ~zShape() {};
    virtual void WriteCreate( nMessage & m );

    void animate( REAL time );
    virtual bool isInteracting(eGameObject * target);
    virtual void render(const eCamera * cam );
	virtual void render2d(tCoord scale) const;

#ifdef DADA
    void setPosX(const BasePtr & x, tString &exprStr);
    void setPosY(const BasePtr & y, tString &exprStr);
    void setRotation(const BasePtr & r, tString &exprStr);
    void setScale(const BasePtr & s, tString &exprStr);
    void setColor(const rColor &c);
#else
    void setPosX(const tFunction &x);
    void setPosY(const tFunction &y);
    void setRotation(const tFunction &r);
    void setScale(const tFunction &s);
    void setColor(const rColor &c);

  tFunction getPosX() {return posx_;};
  tFunction getPosY() {return posy_;};
  tFunction getScale() {return scale_;};
  tFunction getRotation() {return rotation_;};
  rColor getColor() {return color_;};
#endif

    void TimeStep( REAL time );
    virtual bool isEmulatingOldZone() {return false;};
    void setReferenceTime(REAL time);

protected:
#ifdef DADA
    BasePtr posx_; //!< position need not be inside the shape.
    BasePtr posy_; //!< positoin need not be inside the shape.
    BasePtr scale_; //!< Used to affect the contour and not the position
    BasePtr rotation_; //!< Rotate the contour around the position at this rate.
    rColor color_;

    tString posxExpr;
    tString posyExpr;
    tString scaleExpr;
    tString rotationExpr;
#else
    tFunction posx_; //!< position need not be inside the shape.
    tFunction posy_; //!< positoin need not be inside the shape.
    tFunction scale_; //!< Used to affect the contour and not the position
    tFunction rotation_; //!< Rotate the contour around the position at this rate.
    rColor color_;
#endif

#ifdef DADA
    eCoord Position() { return eCoord(posx_->GetFloat(), posy_->GetFloat()); };
#else
    eCoord Position() { return eCoord(posx_(lasttime_ - referencetime_), posy_(lasttime_ - referencetime_) ); };
#endif

    static void networkRead(nMessage &m, zShape *aShape);
    void networkWrite(nMessage &m);

    void setCreatedTime(REAL time);

    REAL createdtime_; // The in-game time when this shape was first instantiated
    REAL referencetime_; // The in-game time when this shape's data was updated, used for evaluation
    REAL lasttime_; // What is to be considered as the current time for this shape

    unsigned short idZone_;
};

class zShapeCircle : public zShape {
  // HACK 
  // Circle dont have a radius attribute per see.
  // They have a default radius of 1.0, and it get scaled
public :
    zShapeCircle(eGrid *grid, unsigned short idZone);
    zShapeCircle(nMessage &m);
    ~zShapeCircle() {};
    void WriteCreate( nMessage & m );

    bool isInteracting(eGameObject * target);
    void render(const eCamera * cam );
	virtual void render2d(tCoord scale) const;

    bool isEmulatingOldZone() {return emulatingOldZone_;}; 
    bool emulatingOldZone_;

    void setRadius(tFunction radius) {this->radius = radius;};
protected:
    tFunction radius;

private:
    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network
};

#ifdef DADA
typedef std::pair<BasePtr, BasePtr> myPoint;
#else
typedef std::pair<tFunction, tFunction> myPoint;
#endif

class zShapePolygon : public zShape {
public :
    zShapePolygon(eGrid *grid, unsigned short idZone);
    zShapePolygon(nMessage &n);
    ~zShapePolygon() {};
    void WriteCreate( nMessage & m );

    bool isInteracting(eGameObject * target);
    void render(const eCamera * cam );
	virtual void render2d(tCoord scale) const;
#ifdef DADA
    void addPoint( myPoint const &aPoint, std::pair<tString, tString> const & exprPair) { points.push_back(aPoint); exprs.push_back(exprPair);};
#else
    void addPoint( myPoint const &aPoint) { points.push_back(aPoint);};
#endif

  bool isEmulatingOldZone() {return false;}; // zShapePolygon cant be used for emulation of old zones

protected:
#ifdef DADA
    std::vector< myPoint > points;
    std::vector< std::pair<tString, tString> > exprs;
#else
    std::vector< myPoint > points;
#endif
    bool isInside (eCoord anECoord);
    static void networkRead(nMessage &m, zShape *aShape);
private:
    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network
};

//typedef boost::shared_ptr<zShape> zShapePtr;
#include "tSafePTR.h"
typedef tJUST_CONTROLLED_PTR< zShape> zShapePtr;

#include "tSysTime.h"


#endif
