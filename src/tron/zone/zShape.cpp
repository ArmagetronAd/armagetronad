#include "zShape.hpp"
#include "gCycle.h"
#include "zZone.h"

#include "zShape.pb.h"
#include "nProtoBuf.h"

zShape::zShape( eGrid* grid, zZone * zone )
        :eNetGameObject( grid, eCoord(0,0), eCoord(0,0), NULL, true ),
        posx_(),
        posy_(),
        scale_(),
        rotation2(),
        color_(),
        createdtime_(0.0),
        referencetime_(0.0),
        lasttime_(0.0)
{
    tASSERT( zone );
    zone->setShape( this );
}

zShape::zShape( Zone::ShapeSync const & sync, nSenderInfo const & sender )
: eNetGameObject( sync.base(), sender )
{
    setCreatedTime( sync.creation_time() );
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

void zShape::WriteSync( Zone::ShapeSync & sync, bool init ) const
{
    eNetGameObject::WriteSync( *sync.mutable_base(), init );

    sync.set_reference_time( referencetime_ );
    posx_.WriteSync( *sync.mutable_pos_x() );
    posy_.WriteSync( *sync.mutable_pos_y() );
    scale_.WriteSync( *sync.mutable_scale() );
    rotation2.WriteSync( *sync.mutable_rotation2() );
    color_.WriteSync( *sync.mutable_color() );
}

void zShape::ReadSync( Zone::ShapeSync const & sync, nSenderInfo const & sender )
{
    eNetGameObject::ReadSync( sync.base(), sender );

    setReferenceTime( sync.reference_time() );
    posx_.ReadSync( sync.pos_x() );
    posy_.ReadSync( sync.pos_y() );
    scale_.ReadSync( sync.scale() );
    rotation2.ReadSync( sync.rotation2() );
    color_.ReadSync( sync.color() );
}

void zShape::setPosX(const tFunction & x){
    posx_ = x;
}

void zShape::setPosY(const tFunction & y){
    posy_ = y;
}

void zShape::setRotation2(const tPolynomial & r) {
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
}

bool zShape::isInteracting(eGameObject * target) {
    return false;
}

void zShape::render(const eCamera *cam )
{}
void zShape::render2d(tCoord scale) const
    {}

zShapeCircle::zShapeCircle(eGrid *grid, zZone * zone ):
        zShape(grid, zone ),
        radius(1.0, 0.0)
{}

/*
 * to create a shape on the clients
 */

zShapeCircle::zShapeCircle( Zone::ShapeCircleSync const & sync, nSenderInfo const & sender ):
        zShape( sync.base(), sender ),
        radius(1.0, 0.0)
{
}

void zShapeCircle::WriteSync( Zone::ShapeCircleSync & sync, bool init ) const
{
    zShape::WriteSync( *sync.mutable_base(), init );

    radius.WriteSync( *sync.mutable_radius() );
}

void zShapeCircle::ReadSync( Zone::ShapeCircleSync const & sync, nSenderInfo const & sender )
{
    zShape::ReadSync( sync.base(), sender );

    if ( sync.has_radius() )
    {
        radius.ReadSync( sync.radius() );
    }
}

bool zShapeCircle::isInteracting(eGameObject * target)
{
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

//
zShapePolygon::zShapePolygon(  Zone::ShapePolygonSync const & sync, nSenderInfo const & sender )
: zShape( sync.base(), sender ),
 points()
{
    int numPoints = sync.points_size();
    
    // read the polygon shape
    for ( int i = 0; i < numPoints; ++i )
    {
        tFunction tfX, tfY;
        Zone::FunctionPointSync const & point = sync.points(i);
        tfX.ReadSync( point.x() );
        tfY.ReadSync( point.y() );
        addPoint( myPoint( tfX, tfY ) );
    }
}

zShapePolygon::zShapePolygon(eGrid *grid, zZone * zone ):
        zShape( grid, zone ),
        points()
{}

/*
 * to create a shape on the clients
 */
void zShapePolygon::WriteSync( Zone::ShapePolygonSync & sync, bool init ) const
{
    zShape::WriteSync( *sync.mutable_base(), init );

    if ( !init )
    {
        return;
    }

    sync.clear_points();

    std::vector< myPoint >::const_iterator iter;
    for(iter = points.begin();
            iter != points.end();
            ++iter)
    {
        Zone::FunctionPointSync & point = *sync.add_points();

        (*iter).first.WriteSync( *point.mutable_x() );
        (*iter).second.WriteSync( *point.mutable_y() );
    }

    //    WriteSync( m );
}

//! reads sync data, returns false if sync was old or otherwise invalid
void zShapePolygon::ReadSync( Zone::ShapePolygonSync const & sync, nSenderInfo const & sender )
{
    zShape::ReadSync( sync.base(), sender );
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
static nNetObjectDescriptor< zShapeCircle, Zone::ShapeCircleSync > zoneCircle_init( 350 );
static nNetObjectDescriptor< zShapePolygon, Zone::ShapePolygonSync > zonePolygon_init( 360 );

nNetObjectDescriptorBase const & zShapeCircle::DoGetDescriptor() const
{
    return zoneCircle_init;
}

nNetObjectDescriptorBase const & zShapePolygon::DoGetDescriptor() const
{
    return zonePolygon_init;
}
