#include "eSoundMixer.h"
#include "rScreen.h"
#include "zShape.h"
#include "gCycle.h"
#include "gParser.h"
#include "zZone.h"

#include "zShape.pb.h"
#include "nConfig.h"
#include "nProtoBuf.h"

#ifndef ENABLE_ZONESV1
int sz_zoneAlphaToggle = 0;
static tSettingItem<int> sz_zoneAlphaToggleConf( "ZONE_ALPHA_TOGGLE", sz_zoneAlphaToggle );

int sz_zoneSegments = 11;
static tSettingItem<int> sz_zoneSegmentsConf( "ZONE_SEGMENTS", sz_zoneSegments );

REAL sz_zoneSegLength = .5;
static tSettingItem<REAL> sz_zoneSegLengthConf( "ZONE_SEG_LENGTH", sz_zoneSegLength );

REAL sz_zoneBottom = 0.0f;
static tSettingItem<REAL> sz_zoneBottomConf( "ZONE_BOTTOM", sz_zoneBottom );

REAL sz_zoneHeight = 5.0f;
static tSettingItem<REAL> sz_zoneHeightConf( "ZONE_HEIGHT", sz_zoneHeight );

REAL sz_expansionSpeed = 1.0f;
static nSettingItem<REAL> sz_expansionSpeedConf( "WIN_ZONE_EXPANSION", sz_expansionSpeed );

REAL sz_initialSize = 5.0f;
static nSettingItem<REAL> sz_initialSizeConf( "WIN_ZONE_INITIAL_SIZE", sz_initialSize );

// extra alpha blending factors, the first one under pure client control
static REAL sz_zoneAlpha = 1.0;
static tSettingItem< REAL > sg_zoneAlphaConf( "ZONE_ALPHA", sz_zoneAlpha );

// the second one under server control. Not used for all kinds of zones, only
// v1 zones synced from the server and v2 fortress zones.
REAL sz_zoneAlphaServer = 0.7;
static nSettingItem< REAL > sg_zoneAlphaConfServer( "ZONE_ALPHA_SERVER", sz_zoneAlphaServer );
#else
#include "gWinZone.h"

extern REAL sg_zoneAlpha, sg_zoneAlphaServer;

#define sz_zoneAlphaToggle sg_zoneAlphaToggle
#define sz_zoneSegments    sg_zoneSegments
#define sz_zoneSegLength   sg_zoneSegLength
#define sz_zoneBottom      sg_zoneBottom
#define sz_zoneHeight      sg_zoneHeight
#define sz_expansionSpeed  sg_expansionSpeed
#define sz_initialSize     sg_initialSize
#define sz_zoneAlpha       sg_zoneAlpha
#define sz_zoneAlphaServer sg_zoneAlphaServer
#endif

#define lasttime_ lastTime

zShape::zShape( eGrid* grid, zZone * zone )
        :eNetGameObject( grid, eCoord(0,0), eCoord(0,0), NULL, true ),
        posx_(),
        posy_(),
        bottom_(),
        scale_(),
        height_(),
        rotation2(),
        segments_(),
        seglength_(),
        color_(),
        createdtime_(0.0),
        referencetime_(0.0)
{
    tASSERT( zone );
    zone->setShape( this );
    this->AddToList();

    color_.a_ = sz_zoneAlphaServer;
}

zShape::zShape( Zone::ShapeSync const & sync, nSenderInfo const & sender )
: eNetGameObject( sync.base(), sender ),
        posx_(),
        posy_(),
        bottom_(),
        scale_(),
        height_(),
        rotation2(),
        segments_(),
        seglength_(),
        color_(),
        referencetime_(0.0)
{
    setCreatedTime( sync.creation_time() );
    this->AddToList();

    color_.a_ = sz_zoneAlphaServer;
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
    // adapt tFunction members
    posx_.SetOffset( posx_.Evaluate( time - referencetime_ ) );
    posy_.SetOffset( posy_.Evaluate( time - referencetime_ ) );
    scale_.SetOffset( scale_.Evaluate( time - referencetime_ ) );

    referencetime_ = time;
    // Do not update lasttime_, referencetime_ might be set in the future for ease of equation writing.
}


void zShape::SetVelocity( eCoord const & velocity )
{
	// backup position
	eCoord pos;
	pos = Position();
			
	posx_.SetSlope( velocity.x );
	posy_.SetSlope( velocity.y );
	// restore position
	Position() = pos;
}

void zShape::WriteSync( Zone::ShapeSync & sync, bool init ) const
{
    eNetGameObject::WriteSync( *sync.mutable_base(), init );

    if ( init )
    {
        sync.set_creation_time( createdtime_ );
    }

    sync.set_reference_time( referencetime_ );
    posx_.WriteSync( *sync.mutable_pos_x() );
    posy_.WriteSync( *sync.mutable_pos_y() );
    bottom_.WriteSync( *sync.mutable_pos_z_bottom() );
    scale_.WriteSync( *sync.mutable_scale() );
    height_.WriteSync( *sync.mutable_height() );
    rotation2.WriteSync( *sync.mutable_rotation2() );
    segments_.WriteSync( *sync.mutable_segments() );
    seglength_.WriteSync( *sync.mutable_segment_length() );
    color_.WriteSync( *sync.mutable_color() );
}

void zShape::ReadSync( Zone::ShapeSync const & sync, nSenderInfo const & sender )
{
    eNetGameObject::ReadSync( sync.base(), sender );

    setReferenceTime( sync.reference_time() );
    posx_.ReadSync( sync.pos_x() );
    posy_.ReadSync( sync.pos_y() );
    if (sync.has_pos_z_bottom())
        bottom_.ReadSync( sync.pos_z_bottom() );
    scale_.ReadSync( sync.scale() );
    if (sync.has_height())
        height_.ReadSync( sync.height() );
    rotation2.ReadSync( sync.rotation2() );
    if (sync.has_segments())
        segments_.ReadSync( sync.segments() );
    if (sync.has_segment_length())
        seglength_.ReadSync( sync.segment_length() );
    color_.ReadSync( sync.color() );

    // old servers don't send alpha values; replace them by the server controlled alpha setting
    if( !sync.color().has_a() )
    {
        color_.a_ = sz_zoneAlphaServer * .7;
    }
}

void zShape::applyVisuals( gParserState & state ) {
    if (state.istype<rColor>("color"))
        setColor(state.get<rColor>("color"));
    if (state.isset("rotation"))
        setRotation2( state.get<tPolynomial>("rotation") );
    if (state.isset("scale"))
        setScale( state.get<tFunction>("scale") );
}

void
zShape::OnBirth() {
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->PushButton(ZONE_SPAWN, Position());
}


REAL zShape::calcDistanceNear(tCoord & p) {
    return (findPointNear(p) - p).Norm();
}

REAL zShape::calcDistanceFar(tCoord & p) {
    return (findPointFar(p) - p).Norm();
}

tCoord zShape::findCenter() {
    // NOTE: Should be overridden if not actually center
    return Position();
}

REAL zShape::calcBoundSq() {
    // NOTE: Default implementation assumes a size of 1, scaled
    return scale_.Evaluate(lasttime_ - referencetime_);
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

REAL zShape::GetCurrentScale() const {
    return scale_.Evaluate(lasttime_ - referencetime_);
}

REAL zShape::GetEffectiveBottom() const {
    if (bottom_.Len())
        return bottom_.evaluate(lastTime);
    return sz_zoneBottom;
}

REAL zShape::GetEffectiveHeight() const {
    if (height_.Len())
        return height_.evaluate(lastTime);
    return sz_zoneHeight;
}

tCoord zShape::GetRotation() const {
    REAL currAngle = rotation2.evaluate(lasttime_);
    tCoord rot( cos(currAngle), sin(currAngle) );
    return rot;
}

REAL zShape::GetRotationSpeed() {
    return getRotation2().evaluateRate(1, lasttime_);
}

void zShape::SetRotationSpeed(REAL r) {
    tPolynomial r2 = getRotation2();
    r2.changeRate(r, 1, lasttime_);
    setRotation2(r2);
}

REAL zShape::GetRotationAcceleration() {
    return getRotation2().evaluateRate(2, lasttime_);
}

void zShape::SetRotationAcceleration(REAL r) {
    tPolynomial r2 = getRotation2();
    r2.changeRate(r, 2, lasttime_);
    setRotation2(r2);
}

int zShape::GetEffectiveSegments() const {
    if (segments_.Len())
        return int( segments_.evaluate(lastTime) );
    return sz_zoneSegments;
}

REAL zShape::GetEffectiveSegmentLength() const {
    if (seglength_.Len())
        return int( seglength_.evaluate(lastTime) );
    return sz_zoneSegLength;
}

void zShape::animate( REAL time ) {
    // Is this needed as the items are already animated?
}

bool zShape::Timestep( REAL time ) {
    lasttime_ = time;
    /*
    REAL scale = scale_.Evaluate(lasttime_ - referencetime_);
    if (scale < -1.0) // Allow shapes to keep existing for a while even though they are not physical
      {
        // The shape has collapsed and should be removed
      }
    */
    return false;
}

bool zShape::isInteracting(eGameObject * target) {
    return false;
}

void zShape::Render(const eCamera *cam )
{}
void zShape::Render2D(tCoord scale) const
    {}

// *******************************************************************************
// *
// *	RendersAlpha
// *
// *******************************************************************************
//!
//!		@return	True if alpha blending is used
//!
// *******************************************************************************
bool zShape::RendersAlpha() const
{
	return sr_alphaBlend ? !sz_zoneAlphaToggle : sz_zoneAlphaToggle;
}


zShapeCircle::zShapeCircle(eGrid *grid, zZone * zone ):
        zShape(grid, zone ),
        radius(sz_initialSize, sz_expansionSpeed)
{}

/*
 * to create a shape on the clients
 */

zShapeCircle::zShapeCircle( Zone::ShapeCircleSync const & sync, nSenderInfo const & sender ):
        zShape( sync.base(), sender ),
        radius(sz_initialSize, sz_expansionSpeed)
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

void zShapeCircle::applyVisuals( gParserState & state ) {
    zShape::applyVisuals(state);
    if (state.isset("radius"))
        setRadius( state.get<tFunction>("radius") );
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

void zShapeCircle::Render(const eCamera * cam )
{
#ifndef DEDICATED
    bool useAlpha = sr_alphaBlend;
    if (sz_zoneAlphaToggle)
        useAlpha = !useAlpha;

    if ( color_.a_ <= 0 )
        return;

    tCoord rot = GetRotation();

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

    if ( useAlpha ) {
        glDepthMask(GL_FALSE);
        BeginQuads();
    } else {
        glDepthMask(GL_TRUE);
        BeginLineStrip();
    }

    const int sg_segments = GetEffectiveSegments();
    const REAL seglen = 2 * M_PI / sg_segments * GetEffectiveSegmentLength();
    const REAL bot = GetEffectiveBottom();
    const REAL top = bot + GetEffectiveHeight();

    // apply color, respecting client chosen extra alpha factor
    rColor color = color_;
    color.a_ *= sz_zoneAlpha;
    color.Apply();

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

            if ( !useAlpha )
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

void zShapeCircle::Render2D(tCoord scale) const {
#ifndef DEDICATED
    if ( color_.a_ <= 0 )
        return;

    tCoord rot = GetRotation();

    GLfloat m[4][4]={{rot.x,rot.y,0,0},
                     {-rot.y,rot.x,0,0},
                     {0,0,1,0},
                     {posx_.Evaluate(lasttime_ - referencetime_), posy_.Evaluate(lasttime_ - referencetime_), 0,1}};

    ModelMatrix();
    glPushMatrix();

    glMultMatrixf(&m[0][0]);

    BeginLines();

    int sg_segments = GetEffectiveSegments();
    const REAL seglen = 2 * M_PI / sg_segments * GetEffectiveSegmentLength();

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

void zShapeCircle::setReferenceTime(REAL time)
{
    radius.SetOffset( radius.Evaluate( time - referencetime_ ) );
    zShape::setReferenceTime( time );
}

tCoord zShapeCircle::findPointNear(tCoord & p) {
    tCoord angle = p - Position();
    angle.Normalize();
    return angle * calcBoundSq();
}

tCoord zShapeCircle::findPointFar(tCoord & p) {
    tCoord angle = Position() - p;
    angle.Normalize();
    return angle * calcBoundSq();
}

REAL zShapeCircle::calcBoundSq() {
    return scale_.Evaluate(lasttime_ - referencetime_) * radius.Evaluate(lasttime_ - referencetime_);
}

void zShapeCircle::setGrowth(REAL growth) {
    REAL s = radius(lasttime_ - referencetime_);
    radius.SetSlope(0.);
    radius.SetOffset(s);
    
    zShape::setGrowth(growth);
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
    tCoord rotation = GetRotation();
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

void zShapePolygon::Render(const eCamera * cam )
{
#ifndef DEDICATED
    bool useAlpha = sr_alphaBlend;
    if (sz_zoneAlphaToggle)
        useAlpha = !useAlpha;

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

        if ( useAlpha ) {
            glDepthMask(GL_FALSE);
            BeginQuads();
        } else {
            RenderEnd();
            glDepthMask(GL_TRUE);
            BeginLineStrip();
        }

        const REAL bot = GetEffectiveBottom();
        const REAL top = bot + GetEffectiveHeight();

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

            if ( !useAlpha )
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
void zShapePolygon::Render2D(tCoord scale) const {
#ifndef DEDICATED
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

tCoord zShapePolygon::findPointNear(tCoord & p) {
    // FIXME: Write real sane code for a polygon
    tCoord angle = p - Position();
    angle.Normalize();
    return angle * calcBoundSq();
}

tCoord zShapePolygon::findPointFar(tCoord & p) {
    // FIXME: Write real sane code for a polygon
    tCoord angle = Position() - p;
    angle.Normalize();
    return angle * calcBoundSq();
}

REAL zShapePolygon::calcBoundSq() {
    // FIXME: Write real sane code for a polygon
    return zShape::calcBoundSq();
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

#include "gZone.pb.h"

#ifndef ENABLE_ZONESV1
static nNetObjectDescriptor< zShapeCircleZoneV1, Game::ZoneV1Sync > zone_init( 340 );

static
Zone::ShapeCircleSync const
v1upgrade( Game::ZoneV1Sync const & source, REAL referenceTime, REAL currentRotation ) {
    Zone::ShapeSync shape;

    shape.mutable_base()->CopyFrom( source.base() );
    shape.mutable_color()->CopyFrom( source.color() );
    shape.set_creation_time( source.create_time() );
    shape.set_reference_time( source.reference_time() );
    shape.mutable_pos_x()->CopyFrom( source.pos_x() );
    shape.mutable_pos_y()->CopyFrom( source.pos_y() );

    tFunction old_rotation;
    old_rotation.ReadSync( source.rotation_speed() );
    tPolynomial rotation;
    rotation = rotation.adaptToNewReferenceVarValue( referenceTime );
    rotation[0] = currentRotation;
    rotation[1] = old_rotation.offset_;
    rotation[2] = old_rotation.slope_;
    rotation.WriteSync( *shape.mutable_rotation2() );

    static tFunction tfOne(1., 0.);
    tfOne.WriteSync( *shape.mutable_scale() );

    Zone::ShapeCircleSync dest;

    dest.mutable_base()->CopyFrom( shape );
    dest.mutable_radius()->CopyFrom( source.radius() );

    return dest;
}

zShapeCircleZoneV1::zShapeCircleZoneV1( Game::ZoneV1Sync const & oldsync, nSenderInfo const & sender ):
    zShapeCircle( v1upgrade(oldsync, 0, 0), sender )
{
}

void zShapeCircleZoneV1::ReadSync( Game::ZoneV1Sync const & oldsync, nSenderInfo const & sender )
{
    zShapeCircle::ReadSync( v1upgrade(oldsync, lastTime, rotation2(lastTime) ), sender );
}

void zShapeCircleZoneV1::WriteSync( Game::ZoneV1Sync & oldsync, bool init ) const
{
    assert( false && "Not implemented" );
}

nNetObjectDescriptorBase const & zShapeCircleZoneV1::DoGetDescriptor() const
{
    return zone_init;
}
#endif

//! convert circle shape messages into zone v1 messages for old clients
class nZonesV1Translator: public nMessageTranslator< Zone::ShapeCircleSync >
{
public:
    //! constructor registering with the descriptor
    nZonesV1Translator(): nMessageTranslator< Zone::ShapeCircleSync >( zoneCircle_init )
    {
    }
    
    //! convert current message format to format suitable for old client
    virtual nMessageBase * Translate( Zone::ShapeCircleSync const & source, int receiver ) const
    {
        if( sn_protoBuf.Supported( receiver ) )
        {
            // no translation required for protobuf capable clients
            return NULL;
        }

        Zone::ShapeSync const & shape = source.base();

        // copy message over
        Game::ZoneV1Sync dest;
        dest.mutable_base()->CopyFrom( shape.base() );
        dest.mutable_color()->CopyFrom( shape.color() );
        dest.set_create_time( shape.creation_time() );
        dest.set_reference_time( shape.reference_time() );
        dest.mutable_pos_x()->CopyFrom( shape.pos_x() );
        dest.mutable_pos_y()->CopyFrom( shape.pos_y() );

        if (shape.color().a() < .01)
        {
            // Transmit invisible zones as a 0 radius
            static tFunction zeroRadius(.0, .0);
            zeroRadius.WriteSync( *dest.mutable_radius() );
        }
        else
        {

        // transfer radius function
        tFunction scale, radius;
        scale.ReadSync( shape.scale() );
        radius.ReadSync( source.radius() );

        // FIXME: don't ignore the quadratic term
        // FIXME: Force a new sync when it differs too much
        // FIXME: cache this conversion as much as possible
        // mend them together, ignoring the quadratic term
        tFunction mendedRadius( scale.GetOffset() * radius.GetOffset(), scale.GetOffset() * radius.GetSlope() + scale.GetSlope() * radius.GetOffset() );
        mendedRadius.WriteSync( *dest.mutable_radius() );

        }

        // FIXME: the above mostly applies here too
        // calculate rotation speed
        tPolynomial rotation;
        rotation.ReadSync( shape.rotation2() );
        rotation.adaptToNewReferenceVarValue( shape.reference_time() );
        tFunction mendedRotation( rotation[1], rotation[2] );
        mendedRotation.WriteSync( *dest.mutable_rotation_speed() );

        nProtoBufMessageBase * ret = nProtoBufDescriptor< Game::ZoneV1Sync >::TransformStatic( dest );

        if( !shape.has_creation_time() )
        {
            // make it a sync message
            ret->SetStreamer( nNetObjectDescriptorBase::SyncStreamer() );
        }
        
        return ret;
    }
};

static nZonesV1Translator sn_v1translator;
