%{
#include "zone/zZone.h"
#include "zone/zShape.h"
%}

%extend zZone {
    static zZone* find(string name) {
        zZone::zoneMap::const_iterator iterZone;
        if ((iterZone = zZone::MapZones().find(name)) != zZone::MapZones().end()) {
            return iterZone->second;
        } else {
            return NULL;
        }
    }

    zShape* get_shape() {
        return $self->getShape();
    }
};

%rename(Zone) zZone;
class zZone: public eNetGameObject
{
%rename(get_shape) getShape;
    zShapePtr getShape();
%rename(get_name) getName;
    string getName();
};

%rename(Shape) zShape;
class zShape : public eNetGameObject {
public:
    zShape(eGrid * grid, zZone * zone );
    virtual ~zShape() {};

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
    virtual void setRotation2(const tPolynomial & r);

    virtual void setScale(const tFunction &s);
    void setColor(const rColor &c);

    void setColorNow(const rColor &c);

    virtual tCoord Position() const;
    tFunction getPosX() {return posx_;};
    tFunction getPosY() {return posy_;};
    tPolynomial getRotation2() { return rotation2; };
    REAL GetCurrentScale() const;
    tFunction getScale() {return scale_;};
    rColor getColor() {return color_;};

    REAL GetEffectiveBottom() const;
    REAL GetEffectiveHeight() const;

    //! shortcut rotation functions
    tCoord GetRotation() const;
    REAL GetRotationSpeed();
    void SetRotationSpeed(REAL r);
    REAL GetRotationAcceleration();
    void SetRotationAcceleration(REAL r);
    int GetEffectiveSegments() const;
    REAL GetEffectiveSegmentLength() const;

    bool Timestep( REAL time );
    virtual void setReferenceTime(REAL time);

    virtual void setGrowth(REAL growth);  //!< similar to old zones v1 setExpansionSpeed, but generic
    virtual void collapse(REAL speed);  //!< set growth such that collapse happens in a timeframe

private:
    //! called immediately after the object is created, either right after round beginning or mid-game creation
    virtual void OnBirth();
};

