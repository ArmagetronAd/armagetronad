#include "zShape.hpp"
#include "gCycle.h"
#include "zZone.h"

static nVersionFeature sz_ZonesV2(20);

zShape::zShape(eGrid* grid, unsigned short idZone)
        :eNetGameObject( grid, eCoord(0,0), eCoord(0,0), NULL, true ),
        posx_(),
        posy_(),
        scale_(),
        rotation2(),
        color_(),
        createdtime_(0.0),
        referencetime_(0.0),
        lasttime_(0.0),
        idZone_(idZone),
        newIdZone_(false)
{
    joinWithZone();
}

zShape::zShape(nMessage &m):eNetGameObject(m)
{
    REAL time;
    m >> time;
    setCreatedTime(time);

    networkRead(m);

    unsigned short anIdZone;
    m >> anIdZone;
    if(anIdZone != idZone_) {
        idZone_ = anIdZone;
        newIdZone_ = true;
        joinWithZone();
    }
}


void zShape::setCreatedTime(REAL time)
{
    createdtime_ = time;

    // Usefull when the nMessage creates the zone in the middle of a round
    if(lasttime_ < createdtime_)
        lasttime_ = createdtime_;
}

void zShape::setReferenceTime(REAL time)
{
    referencetime_ = time;
    // Do not update lasttime_, referencetime_ might be set in the future for ease of equation writing.
}

void zShape::networkWrite(nMessage &m)
{

    m << referencetime_;
    m << posx_;
    m << posy_;
    m << scale_;
    m << rotation2;
    m << color_.r_;
    m << color_.g_;
    m << color_.b_;
    m << color_.a_;
}

void zShape::networkRead(nMessage &m)
{
    REAL time;
    m >> time;
    setReferenceTime(time);
    m >> posx_;
    m >> posy_;
    m >> scale_;
    m >> rotation2;

    m >> color_.r_;
    m >> color_.g_;
    m >> color_.b_;
    m >> color_.a_;
}

/*
 * to create a shape on the clients
 */
void zShape::WriteCreate( nMessage & m )
{
    eNetGameObject::WriteCreate(m);

    m << createdtime_;

    networkWrite(m);

    m << idZone_;
}

void zShape::WriteSync(nMessage &m)
{
    networkWrite(m);
}

void zShape::ReadSync(nMessage &m)
{
    networkRead(m);
}

void zShape::setPosX(const tFunction & x){
    posx_ = x;
}

void zShape::setPosY(const tFunction & y){
    posy_ = y;
}

void zShape::setRotation2(const tPolynomial<nMessage> & r) {
  if(rotation2 == r) {
    // Empty: Nothing to do, no need to send an update
  }
  else {
    rotation2 = r;
    if (sn_GetNetState()!=nCLIENT)
        RequestSync();
  }
}

void zShape::setScale(const tFunction & s){
    scale_ = s;
}

void zShape::setGrowth(REAL growth) {
    REAL s = scale_(lasttime_ - referencetime_);
    scale_.SetSlope(growth);
    scale_.SetOffset( s + growth * ( referencetime_ - lasttime_ ) );
    setScale(scale_);
}

void zShape::collapse(REAL speed) {
    REAL s = scale_(lasttime_ - referencetime_);
    setGrowth( - s * speed );
}

void zShape::setColor(const rColor &c){
    if(color_ != c) {
        color_ = c;
        if (sn_GetNetState()!=nCLIENT)
            RequestSync();
    }
}

void zShape::setColorNow(const rColor &c){
    // TODO: make color slide with some fancy tFunc or something.
    setColor(c);
}

tCoord zShape::Position() const {
    return tCoord(
        posx_.Evaluate(lasttime_ - referencetime_),
        posy_.Evaluate(lasttime_ - referencetime_)
    );
}

void zShape::animate( REAL time ) {
    // Is this needed as the items are already animated?
}

void zShape::TimeStep( REAL time ) {
    lasttime_ = time;
    /*
    REAL scale = scale_.Evaluate(lasttime_ - referencetime_);
    if (scale < -1.0) // Allow shapes to keep existing for a while even though they are not physical
      {
        // The shape has collapsed and should be removed
      }
    */

    if(newIdZone_) {
        joinWithZone();
    }

}

bool zShape::isInteracting(eGameObject * target) {
    return false;
}

void zShape::render(const eCamera *cam )
{}
void zShape::render2d(tCoord scale) const
    {}

void zShape::joinWithZone() {
    if(sn_netObjects[idZone_]) {
        zZone *asdf = dynamic_cast<zZone*>(&*sn_netObjects[idZone_]);
        asdf->setShape(zShapePtr(this));
        newIdZone_ = false;
    }
}

zShapeCircle::zShapeCircle(eGrid *grid, unsigned short idZone):
        zShape(grid, idZone),
        emulatingOldZone_(false),
        _cacheScaledRadius(NULL),
        _cacheRotationF(NULL),
        radius(1.0, 0.0)
{}

zShapeCircle::zShapeCircle(nMessage &m):
        zShape(m),
        emulatingOldZone_(false),
        _cacheScaledRadius(NULL),
        _cacheRotationF(NULL),
        radius(1.0, 0.0)
{
    m >> radius;
}

/*
 * to create a shape on the clients
 */
void zShapeCircle::WriteCreate( nMessage & m )
{
    if (!sz_ZonesV2.Supported(SyncedUser()))
    {
        eNetGameObject::WriteCreate(m);
        m << createdtime_;
        return;
    }
    
    zShape::WriteCreate(m);

    m << radius;
}

void zShapeCircle::WriteSync(nMessage &m)
{
    if (!sz_ZonesV2.Supported(SyncedUser()))
    {
        WriteSyncV1(m);
        return;
    }
    zShape::WriteSync(m);
    m << radius;
}

void zShapeCircle::WriteSyncV1(nMessage &m)
{
    // Write a zones-v1 sync message
    eNetGameObject::WriteSync(m);

    m << color_.r_;
    m << color_.g_;
    m << color_.b_;

    m << referencetime_;
    m << posx_;
    m << posy_;
    if (!_cacheScaledRadius || (_cacheTime < lasttime_ && scale_.slope_ > 0. && radius.slope_ > 0.))
    {
        delete _cacheScaledRadius;
        tPolynomial<nMessage> effectiveRadius;
        effectiveRadius[0] = scale_.offset_ * radius.offset_;
        effectiveRadius[1] = (scale_.slope_ * radius.offset_) + (radius.slope_ * scale_.offset_);
        effectiveRadius[2] = (scale_.slope_ * radius.slope_) * 2;
        // FIXME: Force a new sync when it differs too much?
        _cacheScaledRadius = new tFunction(effectiveRadius.simplify(lasttime_));
    }
    m << *_cacheScaledRadius;

    // FIXME: .Len isn't truncated when there are 0s
    if (!_cacheRotationF || (_cacheTime < lasttime_ && rotation2.Len() > 3))
    {
        delete _cacheRotationF;
        tPolynomial<nMessage> & rp = rotation2;
        REAL sp = rp.evaluateRate(1, lasttime_);
        REAL ac = rp.evaluateRate(2, lasttime_) / 2.;
        _cacheRotationF = new tFunction(sp, ac);
    }
    m << *_cacheRotationF;

    _cacheTime = lasttime_;
}

void zShapeCircle::ReadSync(nMessage &m)
{
    zShape::ReadSync(m);
    m >> radius;
}

bool zShapeCircle::isInteracting(eGameObject * target)
{
    // NOTE: FIXME: TODO: cache effectiveRadius*effectiveRadius for speed comparable to zones v1

    bool interact = false;
    gCycle* prey = dynamic_cast< gCycle* >( target );
    if ( prey )
    {
        if ( prey->Player() && prey->Alive() )
        {
            REAL effectiveRadius;
            effectiveRadius = scale_.Evaluate(lasttime_ - referencetime_) * radius.Evaluate(lasttime_ - referencetime_);
            // Is the player inside or outside the zone
            if ( (effectiveRadius >= 0.0) && ( prey->Position() - Position() ).NormSquared() < effectiveRadius*effectiveRadius )
            {
                interact = true;
            }
        }
    }
    return interact;
}

void zShapeCircle::render(const eCamera * cam )
{
#ifndef DEDICATED

    // HACK
    int sg_segments = 11;
    bool sr_alphaBlend = true;
    // HACK

    if ( color_.a_ > .7f )
        color_.a_ = .7f;
    if ( color_.a_ <= 0 )
        return;

    REAL currAngle = rotation2.evaluate(lasttime_);
    eCoord rot( cos(currAngle), sin(currAngle) );

    GLfloat m[4][4]={{rot.x,rot.y,0,0},
                     {-rot.y,rot.x,0,0},
                     {0,0,1,0},
                     {posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_), 0,1}};

    ModelMatrix();
    glPushMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    //glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);

    //	glTranslatef(pos.x,pos.y,0);

    glMultMatrixf(&m[0][0]);
    //	glScalef(.5,.5,.5);

    if ( sr_alphaBlend ) {
        glDepthMask(GL_FALSE);
        BeginQuads();
    } else {
        glDepthMask(GL_TRUE);
        BeginLineStrip();
    }

    const REAL seglen = .2f;
    const REAL bot = 0.0f;
    const REAL top = 5.0f; // + ( lastTime - referenceTime_ ) * .1f;

    color_.Apply();

    REAL effectiveRadius;
    effectiveRadius = scale_.Evaluate(lasttime_ - referencetime_) * radius.Evaluate(lasttime_ - referencetime_);
    if (effectiveRadius >= 0.0)
    {
        for ( int i = sg_segments - 1; i>=0; --i )
        {
            REAL a = i * 2 * 3.14159 / REAL( sg_segments );
            REAL b = a + seglen;

            REAL sa = effectiveRadius * sin(a);
            REAL ca = effectiveRadius * cos(a);
            REAL sb = effectiveRadius * sin(b);
            REAL cb = effectiveRadius * cos(b);

            glVertex3f(sa, ca, bot);
            glVertex3f(sa, ca, top);
            glVertex3f(sb, cb, top);
            glVertex3f(sb, cb, bot);

            if ( !sr_alphaBlend )
            {
                glVertex3f(sa, ca, bot);
                RenderEnd();
                BeginLineStrip();
            }
        }
    }

    RenderEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask(GL_TRUE);

    glPopMatrix();
#endif

}

//HACK: render2d and render should probably be merged somehow, too much copy and paste here

void zShapeCircle::render2d(tCoord scale) const {
#ifndef DEDICATED

    // HACK
    int sg_segments = 8;
    // HACK

    //if ( color_.a_ > .7f )
    //    color_.a_ = .7f;
    if ( color_.a_ <= 0 )
        return;

    REAL currAngle = rotation2.evaluate(lasttime_);
    eCoord rot( cos(currAngle), sin(currAngle) );

    GLfloat m[4][4]={{rot.x,rot.y,0,0},
                     {-rot.y,rot.x,0,0},
                     {0,0,1,0},
                     {posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_), 0,1}};

    ModelMatrix();
    glPushMatrix();

    glMultMatrixf(&m[0][0]);

    BeginLines();

    const REAL seglen = M_PI / sg_segments;

    color_.Apply();

    REAL effectiveRadius;
    effectiveRadius = scale_.Evaluate(lasttime_ - referencetime_) * radius.Evaluate(lasttime_ - referencetime_);
    if (effectiveRadius >= 0.0)
    {
        for ( int i = sg_segments - 1; i>=0; --i )
        {
            REAL a = i * 2 * 3.14159 / REAL( sg_segments );
            REAL b = a + seglen;

            REAL sa = effectiveRadius * sin(a);
            REAL ca = effectiveRadius * cos(a);
            REAL sb = effectiveRadius * sin(b);
            REAL cb = effectiveRadius * cos(b);

            glVertex2f(sa, ca);
            glVertex2f(sb, cb);
        }
    }
    RenderEnd();
    glPopMatrix();
#endif
}

void zShapeCircle::setRotation2(const tPolynomial<nMessage> & r) {
    zShape::setRotation2(r);
    delete _cacheRotationF;
    _cacheRotationF = NULL;
}

void zShapeCircle::setScale(const tFunction & s){
    zShape::setScale(s);
    delete _cacheScaledRadius;
    _cacheScaledRadius = NULL;
}

void zShapeCircle::setRadius(tFunction radius) {
    this->radius = radius;
    _cacheScaledRadius = NULL;
}

void zShapeCircle::setGrowth(REAL growth) {
    REAL s = radius(lasttime_ - referencetime_);
    radius.SetSlope(0.);
    radius.SetOffset(s);
    
    zShape::setGrowth(growth);
}

//
zShapePolygon::zShapePolygon(nMessage &m):zShape(m),
        points()
{
    int numPoints;
    m >> numPoints;

    // read the polygon shape
    for( ; numPoints>0 && !m.End(); numPoints-- )
    {
        tFunction tfX, tfY;
        m >> tfX;
        m >> tfY;

        addPoint( myPoint( tfX, tfY ) );
    }
}

zShapePolygon::zShapePolygon(eGrid *grid, unsigned short idZone):
        zShape(grid, idZone),
        points()
{}

/*
 * to create a shape on the clients
 */
void zShapePolygon::WriteCreate( nMessage & m )
{
    zShape::WriteCreate(m);

    int numPoints;
    numPoints = points.size();
    m << numPoints;

    std::vector< myPoint >::const_iterator iter;
    for(iter = points.begin();
            iter != points.end();
            ++iter)
    {
        m << (*iter).first;
        m << (*iter).second;
    }

    //    WriteSync( m );
}

bool zShapePolygon::isInside(eCoord anECoord) {
    // The following is a modified version of code by Randolph Franklin,
    // Found at http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/

    // Hack!!!!
    // All the poinst need to be moved around the x,y position
    // then scaled
    //    REAL r = scale_->GetFloat();
    REAL x = anECoord.x;
    REAL y = anECoord.y;
    int c = 0;

    std::vector< myPoint >::const_iterator iter = points.end() - 1;
    // We need to position the prevIter to the last point.
    REAL currentScale = 0.0;
    REAL x_ = (*iter).first.Evaluate(lasttime_ - referencetime_);
    REAL y_ = (*iter).second.Evaluate(lasttime_ - referencetime_);
    tCoord centerPos = tCoord(posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_));
    //    tCoord rotation = tCoord( cosf(rotation_.Evaluate(lasttime_ - referencetime_)), sinf(rotation_.Evaluate(lasttime_ - referencetime_)) );
    tCoord rotation = tCoord( cosf(rotation2.evaluate(lasttime_)), sinf(rotation2.evaluate(lasttime_)) );
    currentScale = scale_.Evaluate(lasttime_ - referencetime_);
    tCoord previous = tCoord(x_, y_).Turn( rotation )*currentScale + centerPos;

    REAL xpp = previous.x;
    REAL ypp = previous.y;

    if(currentScale > 0.0)
    {
        for(iter = points.begin();
                iter != points.end();
                ++iter)
        {
            x_ = (*iter).first.Evaluate(lasttime_ - referencetime_);
            y_ = (*iter).second.Evaluate(lasttime_ - referencetime_);
            tCoord current = tCoord(x_, y_).Turn( rotation )*currentScale + centerPos;

            REAL xp = current.x;
            REAL yp = current.y;

            if ((((yp <= y) && (y < ypp)) || ((ypp <= y) && (y < yp))) &&
                    (x < (xpp - xp) * (y - yp) / (ypp - yp) + xp))
                c = !c;

            xpp = xp; ypp = yp;
        }
    }

    return c;
}
#include "eNetGameObject.h"
#include "ePlayer.h"
bool zShapePolygon::isInteracting(eGameObject * target)
{
    bool interact = false;
    gCycle* prey = dynamic_cast< gCycle* >( target );
    if ( prey )
    {
        if ( prey->Player() && prey->Alive() )
        {
            // Is the player inside or outside the zone
            if ( isInside( prey->Position() ) )
            {
                interact = true;
            }
        }
    }
    return interact;
}

void zShapePolygon::render(const eCamera * cam )
{
#ifndef DEDICATED

    // HACK
    //  int sg_segments = 11;
    bool sr_alphaBlend = true;
    // HACK

    if ( color_.a_ > .7f )
        color_.a_ = .7f;
    if ( color_.a_ <= 0 )
        return;


    ModelMatrix();
    glPushMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    //glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);

    //    glMultMatrixf(&m[0][0]);

    REAL currentScale = 0.0;
    glTranslatef(posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_), 0);
    currentScale = scale_.Evaluate(lasttime_ - referencetime_);

    if(currentScale > 0.0)
    {
        glScalef(currentScale, currentScale, 1.0);

        glRotatef(rotation2.evaluate(lasttime_)*180/M_PI, 0.0, 0.0, 1.0);

        if ( sr_alphaBlend ) {
            glDepthMask(GL_FALSE);
            BeginQuads();
        } else {
            RenderEnd();
            glDepthMask(GL_TRUE);
            BeginLineStrip();
        }

        //    const REAL seglen = .2f;
        const REAL bot = 0.0f;
        const REAL top = 5.0f; // + ( lastTime - createTime_ ) * .1f;

        color_.Apply();




        std::vector< myPoint >::const_iterator iter;
        std::vector< myPoint >::const_iterator prevIter = points.end() - 1;

        for(iter = points.begin();
                iter != points.end();
                prevIter = iter++)
        {
            REAL xp = (*iter).first.Evaluate( lasttime_ - referencetime_ ) ;
            REAL yp = (*iter).second.Evaluate( lasttime_ - referencetime_ ) ;
            REAL xpp = (*prevIter).first.Evaluate( lasttime_ - referencetime_ ) ;
            REAL ypp = (*prevIter).second.Evaluate( lasttime_ - referencetime_ ) ;

            glVertex3f(xp, yp, bot);
            glVertex3f(xp, yp, top);
            glVertex3f(xpp, ypp, top);
            glVertex3f(xpp, ypp, bot);

            if ( !sr_alphaBlend )
            {
                glVertex3f(xp, yp, bot);
                RenderEnd();
                BeginLineStrip();
            }
        }

        RenderEnd();

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDepthMask(GL_TRUE);
    }
    glPopMatrix();
#endif
}
void zShapePolygon::render2d(tCoord scale) const {
#ifndef DEDICATED

    //if ( color_.a_ > .7f )
    //    color_.a_ = .7f;
    if ( color_.a_ <= 0 )
        return;


    ModelMatrix();
    glPushMatrix();

    REAL currentScale = 0.0;
    glTranslatef(posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_), 0);

    currentScale = scale_.Evaluate(lasttime_ - referencetime_);

    if(currentScale > 0.0)
    {
        glScalef(currentScale, currentScale, 1.0);

        glRotatef(rotation2.evaluate(lasttime_)*180/M_PI, 0.0, 0.0, 1.0);

        BeginLines();

        //    const REAL seglen = .2f;

        color_.Apply();

        std::vector< myPoint >::const_iterator iter;
        std::vector< myPoint >::const_iterator prevIter = points.end() - 1;

        for(iter = points.begin();
                iter != points.end();
                prevIter = iter++)
        {
            REAL xp = (*iter).first.Evaluate( lasttime_ - referencetime_ ) ;
            REAL yp = (*iter).second.Evaluate( lasttime_ - referencetime_ ) ;
            REAL xpp = (*prevIter).first.Evaluate( lasttime_ - referencetime_ ) ;
            REAL ypp = (*prevIter).second.Evaluate( lasttime_ - referencetime_ ) ;

            glVertex2f(xp, yp);
            glVertex2f(xpp, ypp);
        }

        RenderEnd();
    }
    glPopMatrix();
#endif
}

// the shapes's network initializator
static nNOInitialisator<zShapeCircle> zoneCircle_init(350,"shapeCircle");
static nNOInitialisator<zShapePolygon> zonePolygon_init(360,"shapePolygon");

class gZone;
#ifdef ENABLE_ZONESV1
extern nNOInitialisator<gZone> zone_init;
#else
static nNOInitialisator<zShapeCircle> zone_init(340,"zone");
#endif

nDescriptor & zShapeCircle::CreatorDescriptor( void ) const
{
    if (!sz_ZonesV2.Supported(SyncedUser()))
        return zone_init;
    return zoneCircle_init;
}

nDescriptor & zShapePolygon::CreatorDescriptor( void ) const
{
    return zonePolygon_init;
}
