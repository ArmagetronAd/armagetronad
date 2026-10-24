/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "gExplosion.h"
#include "rModel.h"
#include "rRender.h"
#include "rScreen.h"
#include "tInitExit.h"
#include "gWall.h"
#include "gCycle.h"
#include "gImpactAnimation.h"
#include "gStuff.h"
#include "nConfig.h"
#include "eGrid.h"
#include "eTimer.h"
#include "eTeam.h"
#include "tRandom.h"
#include "tMath.h"
#include "eLadderLog.h"
#include "eSoundMixer.h"
#include <algorithm>
#include <cmath>
#ifdef USEPARTICLES
#include "papi.h"
#endif

static tList< gExplosion > sg_Explosions;

static void clamp01(REAL &c)
{
    if (!std::isfinite(c))
        c = 0.5;

    if (c<0)
        c = 0;

    if (c>1)
        c = 1;
}

int	gExplosion::expansionSteps=5;
REAL gExplosion::expansionTime = 0.2f;

static eCoord s_explosionCoord;
static REAL   s_explosionRadius;
static REAL	  s_explosionTime;
static gExplosion * s_holer = 0;

static bool sg_deadlyExplosions = false;
static tSettingItem< bool > sg_deadlyExplosionsSettingItem( "DEADLY_EXPLOSIONS", sg_deadlyExplosions );

static int sg_scoreExplosionOwner = 0;
static tSettingItem< int > sg_scoreExplosionPreySettingItem( "SCORE_EXPLOSION_OWNER", sg_scoreExplosionOwner );

static int sg_scoreExplosion = 0;
static tSettingItem< int > sg_scoreExplosionPredatorSettingItem( "SCORE_EXPLOSION", sg_scoreExplosion );

static eLadderLogWriter sg_deathExplosionWriter( "DEATH_EXPLOSION", true, "player" );

static REAL sg_explosionSpeedFactor = 0;
static nSettingItemWatched< REAL > sg_explosionSpeedFactorSettingItem( "EXPLOSION_RADIUS_SPEED_FACTOR", sg_explosionSpeedFactor, nConfItemVersionWatcher::Group_Bumpy, 22 );

// blow a hole centered at s_explosionCoord with radius s_explosionRadius into wall w
static void S_BlowHoles( eWall * w )
{
    // determine the point closest to s_explosionCoord
    eCoord normal = w->Vec().Conj();
    normal.Normalize();

    eCoord Pos1 = normal.Turn( w->EndPoint(0) - s_explosionCoord );
    eCoord Pos2 = normal.Turn( w->EndPoint(1) - s_explosionCoord );

    tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y + Pos2.y + .1f ) * .1f );

    REAL alpha = .5f;
    if ( Pos1.x != Pos2.x)
        alpha = Pos1.x / ( Pos1.x - Pos2.x );

    REAL radius = s_explosionRadius * s_explosionRadius - Pos1.y * Pos2.y;

    // wall too far away
    if ( radius < 0 )
        return;

    radius = sqrt( radius );

    // works only for player walls
    gPlayerWall* wall = dynamic_cast<gPlayerWall*>( w );
    if ( ! wall )
        return;

    REAL closestPos = wall->Pos( alpha );

    REAL start = closestPos - radius;
    REAL end = closestPos + radius;

    // only cut away walls that were created before the explosion started
    REAL wallEnd = wall->EndPos();
    REAL wallBeg = wall->BegPos();
    if ( wallEnd <= wallBeg )
    {
        return;
    }
    REAL endAlpha = ( end - wallBeg ) / ( wallEnd - wallBeg );
    // //std::cout << wall->BegTime() << "\n";
    clamp01( endAlpha );
    REAL endHoleTime = wall->Time( endAlpha );
    if ( endHoleTime > s_explosionTime )
    {
        REAL begTime = wall->BegTime();
        REAL endTime = wall->EndTime();
        REAL timeNorm = endTime - begTime;
        if ( timeNorm != 0.0f )
        {
            endAlpha = ( s_explosionTime - begTime ) / timeNorm;
            end = wall->Pos( endAlpha );
        }
    }

    if ( end > start )
    {
        wall->BlowHole ( start, end, s_holer );
    }
}

/*
// synchronize walls affected by explosion
static void S_Sync( eWall * w )
{
	// works only for player walls
	gPlayerWall* wall = dynamic_cast<gPlayerWall*>( w );
	if ( ! wall )
		return;

	wall->RequestSync ();
}
*/

gExplosion::gExplosion(eGrid* grid, const eCoord& pos, REAL time, gRealColor& color, gCycle* owner)
    : eReferencableGameObject(grid, pos, eCoord(0, 0), NULL, true),
      createTime(time),
      visualCreateTime_(std::max(time, REAL(se_GameTime()))),
      expansion(0),
      listID(-1),
      owner_(owner)
{
    radius_ = gCycle::ExplosionRadius() + owner_->Speed() * sg_explosionSpeedFactor;
    impactDirection_ = owner_->Direction();
    if (impactDirection_.NormSquared() < .00001f)
        impactDirection_ = eCoord(1, 0);
    impactDirection_.Normalize();
    impactSpeed_ = owner_->Speed();
#ifndef DEDICATED
    if (sr_hiddenWindow && sg_blacklightVisuals)
        std::cerr << "Legacy impact: received_age=" << se_GameTime() - time
                  << " speed=" << impactSpeed_ << " presentation_start=" << visualCreateTime_ << '\n';
#endif
    eSoundMixer& mixer = eSoundMixer::GetMixer();
    mixer.PushButton(CYCLE_EXPLOSION, *this, 4.0);
    //std::cout << "explosion sound effect\n";
    holeAccountedFor_ = false;

    lastTime = time;
    explosion_r = color.r_;
    explosion_g = color.g_;
    explosion_b = color.b_;
    //std::cout << "about to do sg_Explosions.Add ### Holer: " << owner_->Player()->GetUserName() << " Size: " << radius_ << "\n";
    sg_Explosions.Add( this, listID );
    z=0;
    //std::cout << "explosion constructed\n";
#ifdef USEPARTICLES
    //std::cout << "Using particle explosion\n";
    //    particle_handle_circle = pGenParticleGroups(1, 10);
    //    particle_handle_cylinder = pGenParticleGroups(1, 1000);

    //    pCurrentGroup(particle_handle_circle);
    // Generate particles along a very small line in the nozzle.
    //    pSource(1000, PDSphere(pVec(pos.x, pos.y, 0.3), 0.2));
    //    pCurrentGroup(particle_handle_cylinder);
    // Generate particles along a very small line in the nozzle.
    //    pSource(1000, PDSphere(pVec(Position().x, Position().y, 0.3), 0.2));
#endif

    // add to game object lists
    AddToList();
    //std::cout << "Last line of constructor\n";
}

gExplosion::~gExplosion(){
    sg_Explosions.Remove( this, listID );

    if ( s_explosionRadius > 0 )
    {
        /*
        s_explosionCoord  = pos;
        s_explosionRadius = gCycle::ExplosionRadius()*1.5f;

        grid->ProcessWallsInRange( &S_Sync,
        						   s_explosionCoord,
        						   s_explosionRadius,
        						   this->CurrentFace() );
        */
    }
}

// virtual eGameObject_type type();


bool gExplosion::Timestep(REAL currentTime){
    lastTime=currentTime;
    ////std::cout << "Started timestep\n";

    int currentExpansion = int(REAL( expansionSteps )*((currentTime - createTime)/expansionTime)) + 1;
    if ( currentExpansion > expansionSteps )
        currentExpansion = expansionSteps;

    if ( currentExpansion > expansion )
    {
        expansion = currentExpansion;

        s_explosionCoord  = pos;
        REAL factor = expansion / REAL( expansionSteps );
        s_explosionRadius = this->GetRadius() * sqrt(factor);
        s_explosionTime = currentTime;
        s_holer = this;

        if ( s_explosionRadius > 0 && (currentTime < createTime+4) )
        {
            grid->ProcessWallsInRange( &S_BlowHoles,
                                       s_explosionCoord,
                                       s_explosionRadius,
                                       this->CurrentFace() );

        }

        s_holer = 0;
    }
#ifndef USEPARTICLES

    /*
    	// see every once in a while if all cycles did a turn since the expl was created
    	if ( expansion >= expansionSteps )
    	{
    		currentExpansion = expansionSteps + 1 + int( REAL( expansion ) * ( currentTime - createTime ) );
    	}
    	if ( currentExpansion > expansion )
    	{
    		expansion = currentExpansion;
    		REAL time = createTime + 5.0f;

    		const tList<eGameObject>& gameObjects = this->Grid()->GameObjects();
    		for ( int i = gameObjects.Len() - 1; i>=0; --i )
    		{
    			eGameObject* o = gameObjects( i );

    			gCycle* c = dynamic_cast< gCycle* >( o );
    			if ( c && c->Alive() )
    			{
    				const gPlayerWall* w = c->CurrentWall();

    				if ( !w	|| w->BegTime() < time || w->EndTime() < time )
    				{
    					// c has not made a turn; we need to stay around some more
    					return false;
    				}
    			}
    		}
    	}
    */
    if (currentTime>createTime+4)
        return true;
    else
        return false;
#else
    /*    pCurrentGroup(particle_handle_circle);
    // Set up the state.
    pExplosion(Position().x, Position().y, 0.3, 20.0, 20.0, 1.0);
    //pVelocityD(PDCylinder(pVec(0.0, 0.0, 0.0), pVec(0.0, 0.0, 0.01), 0.01, 0.007));
    pColorD(PDLine(pVec(1.0, 1.0, 1.0), pVec(1.0, 1.0, 1.0)));
    pSize(40.0);
    //pStartingAge(0);

    //pRandomAccel(PDSphere(pVec(Position().x, Position().y, 0.0), 100.0));

    // Bounce particles off the grid floor.
    pBounce(-0.05, 1.0, 0.1, PDDisc(pVec(0, 0, 0), pVec(0, 0, 1), 5000000));

    pKillOld(400.0);

    // Move particles to their new positions.
    pMove();

    // Finished when there are less than 1/10 of the original particles
    if (pGetGroupCount() < 100) {
    pDeleteParticleGroups(particle_handle_circle, particle_handle_circle);
    return true;
    } else
    return false;*/
    //std::cout << "returning from timestep\n";
    if (currentTime>createTime+4)
        return true;
    else
        return false;
#endif
}

void gExplosion::InteractWith( eGameObject *target, REAL time, int recursion )
{
    if ( !sg_deadlyExplosions || s_explosionRadius <= 0 || time > createTime + 0.1 )
        return;
    
    gCycle* cycle = dynamic_cast< gCycle* >( target );
    if (
            cycle &&
            cycle->Alive() &&
            ( cycle->Position() - owner_->Position() ).NormSquared() < s_explosionRadius * s_explosionRadius
        )
    {
        cycle->Kill();
        
        tColoredString cycleName, ownerName;
        cycleName << *cycle->Player() << tColoredString::ColorString(-1,-1,-1);
        ownerName << *owner_->Player() << tColoredString::ColorString(-1,-1,-1);
        
        tOutput message;
        message.SetTemplateParameter( 1, cycleName );
        message.SetTemplateParameter( 2, ownerName );
        message << "$player_dead_explosion";
        
        sn_ConsoleOut( message );
        
        sg_deathExplosionWriter << cycle->Player()->GetUserName() << owner_->Player()->GetUserName();
        sg_deathExplosionWriter.write();
        
        // Apply scoring
        if ( eTeam::Enemies( owner_->Team(), cycle->Player() ) )
        {
            owner_->Player()->AddScore( sg_scoreExplosionOwner, tOutput(), tOutput() );
            cycle->Player()->AddScore( sg_scoreExplosion, tOutput(), tOutput() );
        }
    }
        
}

eGameObject::ePassEdgeResult gExplosion::PassEdge(const eWall *,REAL ,REAL ,int){return eContinue;}

void gExplosion::Kill(){
    createTime=lastTime-100;
    visualCreateTime_ = createTime;
}

bool gExplosion::AccountForHole()
{
    bool ret = !holeAccountedFor_;
    holeAccountedFor_ = true;
    return ret;
}

void gExplosion::OnNewWall( eWall* w )
{
    for ( int i = sg_Explosions.Len() - 1; i>=0; --i )
    {
        const gExplosion* e = sg_Explosions(i);
        if ( e->expansion >= expansionSteps )
        {
            // e is a already completed explosion
            s_explosionCoord  = e->pos;
            s_explosionRadius = e->GetRadius();
            s_explosionTime = e->createTime;

            S_BlowHoles( w );
        }
    }
}

static tArray<Vec3> expvec;

static void init_exp(){
    int i=0;
    expvec[i++]=Vec3(0,0,1);
    expvec[i++]=Vec3(0,1,1);
    expvec[i++]=Vec3(0,-1,1);
    expvec[i++]=Vec3(1,0,1);
    expvec[i++]=Vec3(-1,0,1);
    expvec[i++]=Vec3(1,1,1);
    expvec[i++]=Vec3(-1,1,1);
    expvec[i++]=Vec3(1,-1,1);
    expvec[i++]=Vec3(-1,-1,1);

    const REAL fak=7;

    tRandomizer & randomizer = tRandomizer::GetInstance();

    for (int j=i;j<40;j++){
        expvec[i++]=Vec3(fak*( randomizer.Get() -.5f ),
                         fak*( randomizer.Get() -.5f ),
                         1);
        //        expvec[i++]=Vec3(fak*(rand()/static_cast<REAL>(RAND_MAX)-.5f),
        //                         fak*(rand()/static_cast<REAL>(RAND_MAX)-.5f),
        //                         1);
    }

    for (int k=expvec.Len()-1;k>=0;k--)
        expvec[k]=expvec[k]*(1/expvec[k].Norm());
}

static tInitExit ie_exp(&init_exp);

bool sg_crashExplosion = true;
bool sg_crashExplosionHud = true;

#ifndef DEDICATED
namespace
{
class ExplosionVisualState
{
public:
    explicit ExplosionVisualState(bool world)
    {
        RenderEnd();
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
                     GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_LINE_BIT |
                     GL_POLYGON_BIT | GL_LIGHTING_BIT);
#ifdef HAVE_GLEW
        if (GLEW_VERSION_2_0)
        {
            glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
            glUseProgram(0);
        }
        if (GLEW_VERSION_1_3)
        {
            glGetIntegerv(GL_ACTIVE_TEXTURE, &texture_);
            GLint units = 1;
            glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);
            for (GLint unit = 0; unit < units; ++unit)
            {
                glActiveTexture(GL_TEXTURE0 + unit);
                glDisable(GL_TEXTURE_1D);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_3D);
                glDisable(GL_TEXTURE_CUBE_MAP);
            }
            glActiveTexture(GL_TEXTURE0);
        }
        if (GLEW_VERSION_1_4)
            glBlendEquation(GL_FUNC_ADD);
#endif
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_POLYGON_OFFSET_FILL);
        if (world)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glShadeModel(GL_SMOOTH);
        glLineWidth(1.25f);
    }
    ~ExplosionVisualState()
    {
        RenderEnd();
        glPopAttrib();
#ifdef HAVE_GLEW
        if (GLEW_VERSION_2_0)
            glUseProgram(program_);
        if (GLEW_VERSION_1_3)
            glActiveTexture(texture_);
#endif
    }

private:
#ifdef HAVE_GLEW
    GLint program_ = 0, texture_ = GL_TEXTURE0;
#endif
};
} // namespace

void gExplosion::Render(const eCamera *cam){
    if (sg_blacklightVisuals && !sg_MoviePack())
    {
        REAL age = se_GameTime() - visualCreateTime_;
        if (!sg_crashExplosion || !std::isfinite(age) || age < 0 || age >= gImpactAnimation::Duration)
            return;
        ExplosionVisualState state(true);
        REAL r = explosion_r, g = explosion_g, b = explosion_b;
        clamp01(r);
        clamp01(g);
        clamp01(b);

        // A very short, local ion flash replaces the arena-spanning spokes.
        // World-space planes also work under the mirrored scene transform.
        if (age < .30f)
        {
            REAL fade = 1 - age / .30f;
            REAL radius = .18f + .95f * (1 - std::exp(-age * 17.f));
            for (int plane = 0; plane < 3; ++plane)
            {
                BeginTriangleFan();
                glColor4f(r * .32f + .68f, g * .32f + .68f, b * .32f + .68f, .95f * fade);
                glVertex3f(pos.x, pos.y, .55f);
                glColor4f(r, g, b, 0);
                for (int point = 0; point <= 16; ++point)
                {
                    REAL angle = point * (6.2831853f / 16);
                    REAL a = radius * std::cos(angle), c = radius * std::sin(angle);
                    glVertex3f(pos.x + (plane == 2 ? 0 : a), pos.y + (plane == 0 ? c : plane == 2 ? a
                                                                                                  : 0),
                               plane == 0 ? .55f : std::max(REAL(.012f), REAL(.55f + c)));
                }
                RenderEnd();
            }
        }

        // The upright contact front expands across the struck wall before the
        // shell fragments disperse. The floor pass reflects this same geometry.
        if (age < .42f)
        {
            REAL u = age / .42f, radius = .12f + 1.85f * std::sqrt(u);
            REAL fade = (1 - u) * (1 - u);
            eCoord side(-impactDirection_.y, impactDirection_.x);
            BeginTriangleStrip();
            for (int i = 0; i <= 96; ++i)
            {
                REAL a = i * (6.2831853f / 96), c = std::cos(a), s = std::sin(a);
                glColor4f(r * .5f + .5f, g * .5f + .5f, b * .5f + .5f, .60f * fade);
                glVertex3f(pos.x + side.x * radius * c, pos.y + side.y * radius * c, .55f + radius * s);
                glColor4f(r, g, b, 0);
                glVertex3f(pos.x + side.x * (radius + .16f) * c, pos.y + side.y * (radius + .16f) * c, .55f + (radius + .16f) * s);
            }
            RenderEnd();
        }

        // A shallow optical shock front spreads across the glass, then gives
        // way to the fragments. Its radius never represents collision damage.
        if (age < .52f)
        {
            REAL u = age / .52f, fade = (1 - u) * (1 - u);
            REAL radius = .16f + 2.6f * std::sqrt(u);
            BeginTriangleStrip();
            for (int i = 0; i <= 80; ++i)
            {
                REAL a = i * (6.2831853f / 80), c = std::cos(a), s = std::sin(a);
                glColor4f(r * .7f + .3f, g * .7f + .3f, b * .7f + .3f, .40f * fade);
                glVertex3f(pos.x + radius * c, pos.y + radius * s, .022f);
                glColor4f(r, g, b, 0);
                glVertex3f(pos.x + (radius + .09f + .14f * u) * c, pos.y + (radius + .09f + .14f * u) * s, .022f);
            }
            RenderEnd();
        }

        // The body breaks into solid, tumbling facets rather than long rays.
        // Hashes are local constants: adding a visual never advances game RNG.
        // Sampling from absolute age makes both render passes and all FPS agree.
        eCoord heading = impactDirection_;
        REAL modelScale = .5f * sg_LightcycleScale();
        gImpactAnimation::RimFragment rims[gImpactAnimation::RimFragmentCount];
        REAL rimRotation[gImpactAnimation::RimFragmentCount][6];
        REAL const halfSpan = .24085f;
        REAL phaseCos[7], phaseSin[7];
        for (int i = 0; i <= 6; ++i)
        {
            REAL phase = -halfSpan + 2 * halfSpan * i / 6;
            phaseCos[i] = std::cos(phase);
            phaseSin[i] = std::sin(phase);
        }
        for (unsigned i = 0; i < gImpactAnimation::RimFragmentCount; ++i)
        {
            auto& f = rims[i];
            f = gImpactAnimation::SampleRim(i, age, impactSpeed_, modelScale);
            rimRotation[i][0] = std::cos(f.angle);
            rimRotation[i][1] = std::sin(f.angle);
            rimRotation[i][2] = std::cos(f.pitch);
            rimRotation[i][3] = std::sin(f.pitch);
            rimRotation[i][4] = std::cos(f.roll);
            rimRotation[i][5] = std::sin(f.roll);
        }
        auto rimVertex = [&](unsigned index, int point, REAL radialOffset, REAL depth)
        {
            auto const& piece = rims[index];
            REAL const* rotation = rimRotation[index];
            REAL ca = rotation[0], sa = rotation[1], cp = rotation[2], sp = rotation[3], cr = rotation[4], sr = rotation[5];
            REAL radius = piece.radius + radialOffset;
            REAL x = radius * (ca * phaseCos[point] - sa * phaseSin[point]) - piece.radius * ca;
            REAL z = radius * (sa * phaseCos[point] + ca * phaseSin[point]) - piece.radius * sa;
            REAL forward = x * cp + z * sp, up = -x * sp + z * cp;
            REAL across = depth * cr - up * sr;
            up = depth * sr + up * cr;
            forward += piece.along;
            across += piece.across;
            glVertex3f(pos.x + heading.x * forward - heading.y * across,
                       pos.y + heading.y * forward + heading.x * across,
                       std::max(REAL(.012f), REAL(piece.height + up)));
        };
        struct Fragment
        {
            REAL x, y, z, sizeX, sizeY, sizeZ, cs, sn, fade;
            unsigned index;
        } fragments[gImpactAnimation::FragmentCount];
        unsigned fragmentCount = 0;
        for (unsigned i = 0; i < gImpactAnimation::FragmentCount; ++i)
        {
            gImpactAnimation::Fragment f = gImpactAnimation::Sample(i, age, impactSpeed_);
            if (!f.Active())
                continue;
            REAL x = pos.x + heading.x * f.along - heading.y * f.across;
            REAL y = pos.y + heading.y * f.along + heading.x * f.across;
            fragments[fragmentCount++] = {x, y, f.height, f.sizeX, f.sizeY, f.sizeZ,
                                          std::cos(f.spin), std::sin(f.spin), f.fade, i};
        }
        // All six faces wind outwards. The renderer already reverses its front
        // face for the mirrored floor pass, so the same solid mesh works there.
        static float const corners[8][3] = {
            {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
        static unsigned const faces[6][4] = {
            {0, 4, 7, 3}, {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2}, {4, 5, 6, 7}, {0, 3, 2, 1}};
        static float const normals[6][3] = {
            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, -1}};
        static unsigned const edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        auto rotate = [](Fragment const& piece, float a, float c, float d, REAL& x, REAL& y, REAL& z)
        {
            REAL v = c * piece.cs - d * piece.sn;
            x = a * piece.cs - v * piece.sn;
            y = a * piece.sn + v * piece.cs;
            z = c * piece.sn + d * piece.cs;
        };
        auto vertex = [&](Fragment const& piece, unsigned corner)
        {
            REAL x, y, z;
            rotate(piece, corners[corner][0] * piece.sizeX, corners[corner][1] * piece.sizeY,
                   corners[corner][2] * piece.sizeZ, x, y, z);
            glVertex3f(piece.x + x, piece.y + y, piece.z + z);
        };
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glShadeModel(GL_FLAT);
        // Bias solid faces slightly behind their edges; the luminous pass keeps
        // depth testing and cannot shine through the back of an opaque fragment.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.f, 1.f);
        BeginQuads();
        for (unsigned i = 0; i < fragmentCount; ++i)
        {
            Fragment const& piece = fragments[i];
            for (unsigned face = 0; face < 6; ++face)
            {
                REAL nx, ny, nz;
                rotate(piece, normals[face][0], normals[face][1], normals[face][2], nx, ny, nz);
                REAL key = std::max(REAL(0), REAL(nx * .45f - ny * .32f + nz * .83f));
                REAL fill = std::max(REAL(0), REAL(-nx * .64f + ny * .31f + nz * .70f));
                REAL shade = (.22f + .65f * key + .18f * fill) * (.35f + .65f * piece.fade);
                REAL ember = (piece.index % 5 == 0 ? .65f : .18f) * piece.fade;
                glColor4f((.055f + r * (.09f + ember)) * shade,
                          (.065f + g * (.09f + ember)) * shade,
                          (.080f + b * (.09f + ember)) * shade, 1);
                for (unsigned corner = 0; corner < 4; ++corner)
                    vertex(piece, faces[face][corner]);
            }
        }
        RenderEnd();

        // Curved shell pieces keep the wheel silhouette briefly recognizable.
        // Their solid bands occlude the far-side light, including in the mirror.
        glDisable(GL_CULL_FACE);
        BeginQuads();
        for (unsigned i = 0; i < gImpactAnimation::RimFragmentCount; ++i)
        {
            auto const& piece = rims[i];
            if (!piece.Active())
                continue;
            REAL width = .050f * modelScale, depth = .018f * modelScale;
            for (int section = 0; section < 6; ++section)
            {
                for (int side = -1; side <= 1; side += 2)
                {
                    REAL shade = side > 0 ? .80f : .55f;
                    glColor4f((.025f + r * .055f * piece.fade) * shade,
                              (.035f + g * .055f * piece.fade) * shade,
                              (.045f + b * .055f * piece.fade) * shade, 1);
                    rimVertex(i, section, -width, side * depth);
                    rimVertex(i, section + 1, -width, side * depth);
                    rimVertex(i, section + 1, width, side * depth);
                    rimVertex(i, section, width, side * depth);
                    glColor4f(.035f, .046f, .055f, 1);
                    rimVertex(i, section, side * width, -depth);
                    rimVertex(i, section + 1, side * width, -depth);
                    rimVertex(i, section + 1, side * width, depth);
                    rimVertex(i, section, side * width, depth);
                }
            }
            for (int end = 0; end <= 6; end += 6)
            {
                glColor4f(.025f, .033f, .040f, 1);
                rimVertex(i, end, -width, -depth);
                rimVertex(i, end, width, -depth);
                rimVertex(i, end, width, depth);
                rimVertex(i, end, -width, depth);
            }
        }
        RenderEnd();

        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glShadeModel(GL_SMOOTH);
        BeginQuads();
        for (unsigned i = 0; i < gImpactAnimation::RimFragmentCount; ++i)
        {
            auto const& piece = rims[i];
            if (!piece.Active())
                continue;
            REAL fade = piece.fade * piece.fade;
            for (int glow = 0; glow < 2; ++glow)
            {
                REAL width = (glow ? .025f : .075f) * modelScale;
                REAL alpha = (glow ? .82f : .12f) * fade;
                REAL hot = glow ? .48f : .05f;
                glColor4f(r + (1 - r) * hot, g + (1 - g) * hot, b + (1 - b) * hot, alpha);
                for (int section = 0; section < 6; ++section)
                {
                    for (int side = -1; side <= 1; side += 2)
                    {
                        REAL depth = side * (.018f * modelScale + .002f);
                        rimVertex(i, section, -width, depth);
                        rimVertex(i, section + 1, -width, depth);
                        rimVertex(i, section + 1, width, depth);
                        rimVertex(i, section, width, depth);
                    }
                }
            }
        }
        RenderEnd();
        BeginLines();
        for (unsigned i = 0; i < fragmentCount; ++i)
        {
            Fragment const& piece = fragments[i];
            REAL fade = piece.fade * piece.fade;
            for (unsigned edge = 0; edge < 12; ++edge)
            {
                bool glint = piece.index % 7 == 0 && edge == piece.index % 12;
                REAL hot = glint ? .70f : .12f;
                glColor4f(r + (1 - r) * hot, g + (1 - g) * hot, b + (1 - b) * hot,
                          (glint ? .85f : .38f) * fade);
                vertex(piece, edges[edge][0]);
                vertex(piece, edges[edge][1]);
            }
        }
        RenderEnd();

        BeginTriangles();
        int count = std::min(40, expvec.Len());
        for (int i = 0; i < count; ++i)
        {
            REAL life = .62f + .055f * (i % 7);
            if (age >= life)
                continue;
            REAL remaining = 1 - age / life;
            REAL fade = remaining * remaining;
            REAL nx = expvec[i].x[0], ny = expvec[i].x[1], nz = expvec[i].x[2];
            REAL radius = (.12f + 2.8f * (1 - std::exp(-age * 2.9f))) * (.70f + .05f * (i % 7));
            REAL x = pos.x + nx * radius, y = pos.y + ny * radius;
            REAL z = std::max(REAL(.025f), REAL(.18f + nz * (.35f + 1.6f * age) - 1.2f * age * age));
            REAL angle = i * 2.3999632f + age * (4 + i % 5);
            REAL width = (.035f + .012f * (i % 3)) * (.65f + .35f * remaining);
            REAL length = (.075f + .035f * (i % 4)) * (.65f + .35f * remaining);
            REAL sx = std::cos(angle) * width, sy = std::sin(angle) * width;
            // Small tumbling triangular pieces, not long rays from the origin.
            glColor4f(r, g, b, .90f * fade);
            glVertex3f(x + nx * length, y + ny * length, z + nz * length);
            glColor4f(r * .45f, g * .45f, b * .45f, .48f * fade);
            glVertex3f(x + sx, y + sy, z + .45f * width);
            glVertex3f(x - sx, y - sy, std::max(REAL(.012f), z - .45f * width));
        }
        RenderEnd();
        BeginLines();
        for (int i = 0; i < count; ++i)
        {
            REAL life = .62f + .055f * (i % 7);
            if (age >= life)
                continue;
            REAL fade = 1 - age / life;
            REAL radius = (.12f + 2.8f * (1 - std::exp(-age * 2.9f))) * (.70f + .05f * (i % 7));
            REAL x = pos.x + expvec[i].x[0] * radius, y = pos.y + expvec[i].x[1] * radius;
            REAL z = std::max(REAL(.025f), REAL(.18f + expvec[i].x[2] * (.35f + 1.6f * age) - 1.2f * age * age));
            REAL tail = .035f + .10f * fade;
            glColor4f(r, g, b, .80f * fade * fade);
            glVertex3f(x, y, z);
            glColor4f(r, g, b, 0);
            glVertex3f(x - expvec[i].x[0] * tail, y - expvec[i].x[1] * tail,
                       std::max(REAL(.012f), z - expvec[i].x[2] * tail));
        }
        return;
    }
    //std::cout << "Starting render\n";
#ifdef USEPARTICLES
    /*if(sg_crashExplosion){
        ModelMatrix();
        glPushMatrix();
        pCurrentGroup(particle_handle_circle);
        int cnt = (int)pGetGroupCount();
        if(cnt < 1) return;

        float *ptr;
        size_t flstride, pos3Ofs, posB3Ofs, size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs;

        cnt = (int)pGetParticlePointer(ptr, flstride, pos3Ofs, posB3Ofs,
            size3Ofs, vel3Ofs, velB3Ofs, color3Ofs, alpha1Ofs, age1Ofs);
        if(cnt < 1) return;

        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, int(flstride) * sizeof(float), ptr + color3Ofs);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, int(flstride) * sizeof(float), ptr + pos3Ofs);

        glDrawArrays(GL_POINTS, 0, cnt);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glPopMatrix();
    }*/
#else
    if (sg_crashExplosion){
        REAL a1=(lastTime-createTime)+.01f;//+.2;
        REAL e=a1-1;

        if (e<0) e=0;

        REAL fade=(2-a1);
        if (fade<0) fade=0;
        if (fade>1) fade=1;

        a1*=100;
        e*=100;

        ModelMatrix();
        glPushMatrix();
        glTranslatef(pos.x,pos.y,0);

        //glDisable(GL_TEXTURE);
        glDisable(GL_TEXTURE_2D);

        glColor4f(explosion_r,explosion_g,explosion_b,fade);
        BeginLines();
        for (int i=expvec.Len()-1;i>=0;i--){
            glVertex3f(a1*expvec[i].x[0],a1*expvec[i].x[1],a1*expvec[i].x[2]);
            glVertex3f( e*expvec[i].x[0], e*expvec[i].x[1], e*expvec[i].x[2]);
        }
        RenderEnd();
        glPopMatrix();
    }
    /*
    if(sr_alphaBlend)
    for(int i=2;i>=0;i--){
    REAL age=a1-i*.1;
    if (0<age && age<.5){
    REAL alpha=(3-i)*.5*(1-age*2)*(1-age*2);
    glColor4f(r,g,b,alpha);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(pos.x,pos.y,age*pow((age+1),3)*(age+.5)*100);
    y

    GLUquadricObj* q= gluNewQuadric();

    gluSphere(q,(age*pow((age+1),3)*(age+.5))*100,5,5);

    gluDeleteQuadric( q );

    glPopMatrix();
    }
    }
    */
#endif
    //std::cout << "Finishing render\n";
}

void gExplosion::Render2D(tCoord scale) const {
#ifndef DEDICATED
    if (sg_blacklightVisuals && !sg_MoviePack())
    {
        REAL age = lastTime - createTime;
        if (!sg_crashExplosionHud || !std::isfinite(age) || age < 0 || age >= .75f)
            return;
        ExplosionVisualState state(false);
        REAL r = explosion_r, g = explosion_g, b = explosion_b;
        clamp01(r);
        clamp01(g);
        clamp01(b);
        REAL fade = 1 - age / .75f;
        REAL pulse = .25f + .55f * (1 - fade);
        REAL sx = std::isfinite(scale.x) ? std::max(REAL(.4f), std::min(REAL(2.4f), REAL(std::fabs(scale.x)))) : 1;
        REAL sy = std::isfinite(scale.y) ? std::max(REAL(.4f), std::min(REAL(2.4f), REAL(std::fabs(scale.y)))) : 1;
        glColor4f(r, g, b, .90f * fade * fade);
        BeginLineLoop();
        for (int point = 0; point < 32; ++point)
        {
            REAL angle = point * (6.2831853f / 32);
            glVertex2f(pos.x + sx * pulse * std::cos(angle), pos.y + sy * pulse * std::sin(angle));
        }
        return;
    }
    if(sg_crashExplosionHud){
        REAL a1=(lastTime-createTime)+.01f;//+.2;
        REAL e=a1-1;

        if (e<0) e=0;

        REAL fade=(2-a1);
        if (fade<0) fade=0;
        if (fade>1) fade=1;

        a1*=100;
        e*=100;

        ModelMatrix();
        glPushMatrix();
        glTranslatef(pos.x,pos.y,0);

        glDisable(GL_TEXTURE_2D);

        glColor4f(explosion_r,explosion_g,explosion_b,fade);
        BeginLines();
        for(int i=expvec.Len()-1;i>=0;i--){
            glVertex2f(a1*expvec[i].x[0],a1*expvec[i].x[1]);
            glVertex2f( e*expvec[i].x[0], e*expvec[i].x[1]);
        }
        RenderEnd();
        glPopMatrix();
    }
#endif
    //std::cout << "Finishing render\n";
}

#if 0
void gExplosion::SoundMix(Sint16 *dest,unsigned int len,
                          int viewer,REAL rvol,REAL lvol, REAL dopplerPitch){
#ifndef HAVE_LaIBSDL_MIXER
    sound.Mix(dest,len,viewer,rvol*4,lvol*4, dopplerPitch);
#endif
}
#endif

#endif

void gExplosion::OnRemoveFromGame()
{
    // remove from list to avoid costy checks whenever a new wall appears
    sg_Explosions.Remove( this, listID );

    // delegate to base
    eReferencableGameObject::OnRemoveFromGame();
}
