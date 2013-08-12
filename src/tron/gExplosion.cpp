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
#include "tInitExit.h"
#include "gWall.h"
#include "gCycle.h"
#include "nConfig.h"
#include "eGrid.h"
#include "eTeam.h"
#include "tRandom.h"
#include "tMath.h"
#include "eLadderLog.h"
#include "eSoundMixer.h"
#ifdef USEPARTICLES
#include "papi.h"
#endif

static tList< gExplosion > sg_Explosions;

static void clamp01(REAL &c)
{
    if (!finite(c))
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

gExplosion::gExplosion(eGrid *grid, const eCoord &pos,REAL time, gRealColor& color, gCycle * owner )
        :eReferencableGameObject(grid, pos, eCoord(0,0), NULL, true),
        createTime(time),
        expansion(0),
        listID(-1),
        owner_(owner)
{
    radius_ = gCycle::ExplosionRadius() + owner_->Speed() * sg_explosionSpeedFactor;
    eSoundMixer* mixer = eSoundMixer::GetMixer();
    mixer->PushButton(CYCLE_EXPLOSION, pos);
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

void gExplosion::PassEdge(const eWall *,REAL ,REAL ,int){}

void gExplosion::Kill(){
    createTime=lastTime-100;
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
void gExplosion::Render(const eCamera *cam){
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
void gExplosion::SoundMix(Uint8 *dest,unsigned int len,
                          int viewer,REAL rvol,REAL lvol){
#ifndef HAVE_LaIBSDL_MIXER
    sound.Mix(dest,len,viewer,rvol*4,lvol*4);
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
