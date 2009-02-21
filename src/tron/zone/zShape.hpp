#include "tValue.h"
#include "rColor.h"
#include <boost/shared_ptr.hpp>
#include "eNetGameObject.h"
#include "tSysTime.h"

#include "tFunction.h"
#include "tPolynomial.h"

#ifndef Z_SHAPE_H
#define Z_SHAPE_H

class zShape : public eNetGameObject{
public:
    zShape(eGrid* grid, unsigned short idZone);
    zShape(nMessage &m);
    virtual ~zShape() {};
    virtual void WriteCreate( nMessage & m );
    virtual void WriteSync(nMessage &m);   //!< writes sync data
    virtual void ReadSync(nMessage &m);    //!< reads sync data

    void animate( REAL time );
    virtual bool isInteracting(eGameObject * target);
    virtual void render(const eCamera * cam );
    virtual void render2d(tCoord scale) const;

    void setPosX(const tFunction &x);
    void setPosY(const tFunction &y);
    virtual
    void setRotation2(const tPolynomial<nMessage> & r);

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
    virtual bool isEmulatingOldZone() {return false;};
    void setReferenceTime(REAL time);

    virtual void setGrowth(REAL growth);  //!< similar to old zones v1 setExpansionSpeed, but generic
    virtual void collapse(REAL speed);  //!< set growth such that collapse happens in a timeframe

protected:
    tFunction posx_; //!< position need not be inside the shape.
    tFunction posy_; //!< positoin need not be inside the shape.
    tFunction scale_; //!< Used to affect the contour and not the position
    tPolynomial<nMessage> rotation2; //!< Rotate the contour around the position at this rate.
    rColor color_;

    eCoord Position() { return eCoord(posx_(lasttime_ - referencetime_), posy_(lasttime_ - referencetime_) ); };

    void networkRead(nMessage &m);
    void networkWrite(nMessage &m);

    void setCreatedTime(REAL time);

    void joinWithZone();

    REAL createdtime_; // The in-game time when this shape was first instantiated
    REAL referencetime_; // The in-game time when this shape's data was updated, used for evaluation
    REAL lasttime_; // What is to be considered as the current time for this shape

    unsigned short idZone_;
    bool newIdZone_;
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
    void WriteSync(nMessage &m);   //!< writes sync data
    void WriteSyncV1(nMessage &m);   //!< writes zones v1 sync data
    void ReadSync(nMessage &m);    //!< reads sync data

    bool isInteracting(eGameObject * target);
    void render(const eCamera * cam );
	virtual void render2d(tCoord scale) const;

    bool isEmulatingOldZone() {return emulatingOldZone_;}; 
    bool emulatingOldZone_;

    void setRotation2(const tPolynomial<nMessage> & r);
    void setScale(const tFunction &s);
    void setRadius(tFunction radius);
    
    void setGrowth(REAL growth);
protected:
    tFunction radius;

private:
    virtual nDescriptor& CreatorDescriptor() const; //!< returns the descriptor to recreate this object over the network
    tFunction * _cacheScaledRadius;
    tFunction * _cacheRotationF;
    REAL _cacheTime;
};

typedef std::pair<tFunction, tFunction> myPoint;

class zShapePolygon : public zShape {
public :
    zShapePolygon(eGrid *grid, unsigned short idZone);
    zShapePolygon(nMessage &n);
    ~zShapePolygon() {};
    void WriteCreate( nMessage & m );
  //    void WriteSync(nMessage &m);   //!< writes sync data
  //    void ReadSync(nMessage &m);    //!< reads sync data

    bool isInteracting(eGameObject * target);
    void render(const eCamera * cam );
    virtual void render2d(tCoord scale) const;
    void addPoint( myPoint const &aPoint) { points.push_back(aPoint);};

  bool isEmulatingOldZone() {return false;}; // zShapePolygon cant be used for emulation of old zones

protected:
    std::vector< myPoint > points;
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
