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


#include "rSDL.h"

#include "gWall.h"
#include "gStuff.h"
#include "eGrid.h"
#include "eWall.h"
#include "math.h"
#include "gCycle.h"
#include "rTexture.h"
#include "eTimer.h"
#include "gGame.h"
#include "rScreen.h"
#include "rRender.h"
#include "eCamera.h"
#include "tConfiguration.h"
#include "gExplosion.h"
#include "tMath.h"
#include "ePlayer.h"
#include "eTess2.h"
#include "nConfig.h"
#include "nProtoBuf.h"

#include <fstream>

#include "gWall.pb.h"

/* **********************************************
   Wall
   ********************************************** */

#ifndef DEDICATED

// setting items to enable the old style semi-transparency of rim walls
static bool sg_bugTransparency;
static bool sg_bugTransparencyDemand;
static tSettingItem< bool > sgc_bugTransparency( "BUG_TRANSPARENCY", sg_bugTransparency );
static tSettingItem< bool > sgc_bugTransparencyDemand( "BUG_TRANSPARENCY_DEMAND", sg_bugTransparencyDemand );

//texture ArmageTron_invis_eWall("eWall2.png",1,0);
/*
static tString lala_wallRim("Anonymous/original/textures/rim_wall.png");
static nSettingItem<tString> lalala_wallRim("TEXTURE_WALLRIM", lala_wallRim);
rFileTexture gWallRim_text(rTextureGroups::TEX_WALL, lala_wallRim, 1,1);

static tString lala_mp_wallRimA("Anonymous/original/moviepack/rim_wall_a.png");
static nSettingItem<tString> lalala_mp_wallRimA("TEXTURE_MP_WALLRIM_A", lala_mp_wallRimA);
rFileTexture gWallRim_a(rTextureGroups::TEX_WALL, lala_mp_wallRimA, 0,0);

static tString lala_mp_wallRimB("Anonymous/original/moviepack/rim_wall_b.png");
static nSettingItem<tString> lalala_mp_wallRimB("TEXTURE_MP_WALLRIM_B", lala_mp_wallRimB);
rFileTexture gWallRim_b(rTextureGroups::TEX_WALL, lala_mp_wallRimB, 0,0);

static tString lala_mp_wallRimC("Anonymous/original/moviepack/rim_wall_c.png");
static nSettingItem<tString> lalala_mp_wallRimC("TEXTURE_MP_WALLRIM_C", lala_mp_wallRimC);
rFileTexture gWallRim_c(rTextureGroups::TEX_WALL, lala_mp_wallRimC, 0,0);

static tString lala_mp_wallRimD("Anonymous/original/moviepack/rim_wall_d.png");
static nSettingItem<tString> lalala_mp_wallRimD("TEXTURE_MP_WALLRIM_D", lala_mp_wallRimD);
rFileTexture gWallRim_d(rTextureGroups::TEX_WALL, lala_mp_wallRimD, 0,0);
*/

//static rTexture gWallRim_text_moviepack(rTEX_WALL,"moviepack/gWallRim2.png",1,0);
static rFileTexture gWallRim_a(rTextureGroups::TEX_WALL,"moviepack/rim_wall_a.png",0,0);
static rFileTexture gWallRim_b(rTextureGroups::TEX_WALL,"moviepack/rim_wall_b.png",0,0);
static rFileTexture gWallRim_c(rTextureGroups::TEX_WALL,"moviepack/rim_wall_c.png",0,0);
static rFileTexture gWallRim_d(rTextureGroups::TEX_WALL,"moviepack/rim_wall_d.png",0,0);

static rITexture *gWallRim_mp[4]={&gWallRim_a,&gWallRim_b,
                                  &gWallRim_c,&gWallRim_d};

/*
static tString lala_dir_eWall("Anonymous/original/textures/dir_wall.png");
static nSettingItem<tString> lalala_dir_eWall("TEXTURE_DIR_WALL", lala_dir_eWall);
rFileTexture dir_eWall(rTextureGroups::TEX_WALL, lala_dir_eWall, 1,0);

static tString lala_mp_dir_eWall("Anonymous/original/moviepack/dir_wall.png");
static nSettingItem<tString> lalala_mp_dir_eWall("TEXTURE_MP_DIR_WALL", lala_mp_dir_eWall);
rFileTexture dir_eWall_moviepack(rTextureGroups::TEX_WALL, lala_mp_dir_eWall, 1,0);
*/
#endif

static REAL sg_RimStretchX=100;
static tSettingItem<REAL> sg_RimStretchXConf
("RIM_WALL_STRETCH_X",sg_RimStretchX);
static REAL sg_RimStretchY=100;
static tSettingItem<REAL> sg_RimStretchYConf
("RIM_WALL_STRETCH_Y",sg_RimStretchY);

static REAL sg_MPRimStretchX=50;
static tSettingItem<REAL> sg_MPRimStretchXConf
("MOVIEPACK_RIM_WALL_STRETCH_X",sg_MPRimStretchX);
static REAL sg_MPRimStretchY=50;
static tSettingItem<REAL> sg_MPRimStretchYConf
("MOVIEPACK_RIM_WALL_STRETCH_Y",sg_MPRimStretchY);

/* **********************************************
   RimWall
   ********************************************** */

gWallRim::gWallRim(eGrid *grid, REAL h)
        :eWallRim(grid, false, h), renderHeight_(h), lastUpdate_(-100), tBeg_( 0 ), tEnd_( 0 )
{
    // std::cout << "create " << this << "\n";
}

gWallRim::gWallRim(eGrid *grid, REAL tBeg, REAL tEnd, REAL h)
        :eWallRim(grid, false, h), renderHeight_(h), lastUpdate_(-100), tBeg_( tBeg ), tEnd_( tEnd )
{
    // std::cout << "create " << this << "\n";
}

gWallRim::~gWallRim()
{
    // std::cout << "destroy " << this << "\n";
}

bool gWallRim::Splittable() const{return 1;}
void gWallRim::Split(eWall *& w1,eWall *& w2,REAL ratio)
{
    // std::cout << "split " << this << "\n";

    REAL tMid = tEnd_ * ratio + tBeg_ * ( 1 - ratio );
    w1=tNEW(gWallRim(grid, tBeg_, tMid, height));
    w2=tNEW(gWallRim(grid, tMid, tEnd_, height));
}

// do not allow walls to run parallel
bool gWallRim::RunsParallelPassive( eWall* newWall )
{
    return false;
}

// from display.C
extern REAL lower_height,upper_height;

#ifndef DEDICATED


static void gWallRim_helper(eCoord p1,eCoord p2,REAL tBeg,REAL tEnd,REAL h,
                            REAL Z_SCALE,bool sw){

    // draw additional upper line
    /*
    sr_DepthOffset(true);
    glPolygonOffset(-100,10000000);
    glDisable(GL_TEXTURE_2D);
    BeginLines();
    Color(1,1,1);
    Vertex(p1.x,p1.y,1);
    Vertex(p2.x,p2.y,1);
    RenderEnd();
    sr_DepthOffset(false);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_TEXTURE_2D);
    */

    if (sg_MoviePack()){
        int t=int(floor((tBeg+tEnd)/2));
        tBeg-=t;
        tEnd-=t;
        t=t%4;
        while (t<0)
            t+=4;
        gWallRim_mp[t]->Select();
    }

    if (sw){
        Swap(p1,p2);
        Swap(tBeg,tEnd);
    }


    if (h>lower_height){
        if (sr_upperSky && !sg_MoviePack() && h>upper_height) h=upper_height;
        else if (sr_lowerSky || sg_MoviePack()) h=lower_height;
    }

    BeginQuads();

    // NOTE: display lists on nvidia cards don't like infinite points
    if (h<9000 || !sr_infinityPlane || rDisplayList::IsRecording() ){
        TexVertex(p1.x, p1.y, 0,
                  tBeg      , 1);

        TexVertex(p1.x, p1.y, h,
                  tBeg,       1-h/Z_SCALE);

        TexVertex(p2.x, p2.y, h,
                  tEnd,       1-h/Z_SCALE);

        TexVertex(p2.x, p2.y, 0,
                  tEnd      , 1);
    }

    else{
        TexVertex(p1.x, p1.y, 0,
                  tBeg,       1);

        TexCoord(0,-1/REAL(Z_SCALE),0,0);

#ifndef WIN32
        Vertex(0,0,1,0);
        Vertex(0,0,1,0);
#else
        Vertex(0.001f,0.001f,1,0); // Windows OpenGL has problems with
        // infitite points perpenticular to the viewing direction
        Vertex(0.001f,0.001f,1,0);
#endif

        TexVertex(p2.x, p2.y, 0,
                  tEnd,       1);
    }
}

// maximal size of the arena wall shadow compared to the camera height
static REAL sg_arenaWallShadowSize = 0.1;
static tSettingItem<REAL> sg_arenaWallShadowSizeConf("ARENA_WALL_SHADOW_SIZE",sg_arenaWallShadowSize);

// shadows are drawn when the cycle gets closer to the line the wall follows than this
static REAL sg_arenaWallShadowSideDist = 10.0;
static tSettingItem<REAL> sg_arenaWallShadowSideDistConf("ARENA_WALL_SHADOW_SIDEDIST",sg_arenaWallShadowSideDist);

// getting closer to the wall than this distance does not increase the shadow much
static REAL sg_arenaWallShadowNear = 1.0;
static tSettingItem<REAL> sg_arenaWallShadowNearConf("ARENA_WALL_SHADOW_NEAR",sg_arenaWallShadowNear);

// shadows are drawn when the cycle gets closer to the wall than this
static REAL sg_arenaWallShadowDist = 100.0;
static tSettingItem<REAL> sg_arenaWallShadowDistConf("ARENA_WALL_SHADOW_DIST",sg_arenaWallShadowDist);

void gWallRim::RenderReal(const eCamera *cam){
    if ( Edge() ){
        const eCoord *p1=&EndPoint(0);
        const eCoord *p2=&EndPoint(1);

        REAL X_SCALE=sg_RimStretchX;
        REAL Z_SCALE=sg_RimStretchY;

        // determine height and transparency
        bool transparency = sg_bugTransparency || ( sg_bugTransparencyDemand && renderHeight_ < height );
        REAL h = transparency ? height : renderHeight_;
        if ( transparency )
            glDisable( GL_DEPTH_TEST );

      if (sg_MoviePack()){
            X_SCALE=sg_MPRimStretchX;
            Z_SCALE=sg_MPRimStretchY;
        }

        if ( tBeg_ == tEnd_ )
        {
            tBeg_=(p1->x+p1->y);
            tEnd_=(p2->x+p2->y);
        }
        REAL tBeg = tBeg_/X_SCALE;
        REAL tEnd = tEnd_/X_SCALE;
        eCoord P1=*p1;
        eCoord P2=*p2;

        // draw "shadow" away from camera
        if ( cam )
        {
            // determine relevant position
            eCoord pos = cam->CenterPos();

            // alternative that does not look so good
            // eCoord pos = cam->CameraGlancePos();

            // calculate normal on wall
            eCoord normal = (P1 - P2).Turn(0,1);
            normal.Normalize();

            // determine distance of the position to the wall's line
            REAL side = -eCoord::F(normal, pos - P1);

            // length scale
            REAL scale = pos.Norm() + 10 + (P1-P2).Norm();

            // add a tiny contribution from the direction to avoid side==zero
            // side += (  20 * EPS * eCoord::F( normal, cam->CenterCamDir() )
            //          +10 * EPS * eCoord::F( normal, cam->CameraDir() ) ) * scale;

            // fallback to camera position in case of doubt
            // if ( fabs(side) < EPS*scale )
            //    side = -eCoord::F( normal, cam->CameraGlancePos() );

            // get absolute value and sign of side
            REAL abs = fabs(side);
            REAL sign = side/abs;

            // if abs is low, override the sign by considering the direction
            if ( abs < EPS*scale*10 )
            {
                // the direction object and camera are facing
                eCoord facing = cam->CenterCamDir() * 2 + cam->CameraDir();

                sign = eCoord::F( normal, facing );
                abs  = fabs( sign );
                if ( abs > EPS )
                {
                    sign/= fabs( sign );
                    abs  = EPS*scale;
                }
                else
                {
                    abs  = 0;
                    sign = 0;
                }
            }

            // driving direction
            eCoord dir = cam->CenterCamDir();

            // the side the camera is on
            REAL camSide = -eCoord::F(normal, cam->CameraGlancePos() - P1);

            // no shadow if camera and vehicle are on different sides
            if ( camSide * sign < -EPS*scale )
                sign = 0;

            // if the camera is closer to the wall than the vehicle, take the abs value from that
            if ( sign * eCoord::F( normal, dir ) > -EPS )
            {
                if ( camSide * sign >= 0 )
                {
                    REAL camAbs = fabs( camSide );
                    if ( camAbs < abs )
                        abs = camAbs;
                }
                else
                {
                    // wall lies between camera and cycle
                    abs = 0;
                }
            }

            // add scaled down distance of the wall and the projected path
            {
                REAL d1 = dir * (pos - P1);
                REAL d2 = dir * (pos - P2);
                if ( d1 * d2 >= 0 )
                {
                    REAL dist = fabs( d1 );
                    d2 = fabs( d2 );
                    if ( d2 < dist )
                        dist = d2;

                    abs += dist * sg_arenaWallShadowSideDist/sg_arenaWallShadowDist;
                }
            }

            // add scaled down distance along the wall's direction
            /*
            {
                REAL a1 = -(pos - P1)*normal;
                REAL a2 =  (pos - P2)*normal;
                if (a1 > 0)
                    abs += a1 * sg_arenaWallShadowSideDist/sg_arenaWallShadowDist;
                if (a2 > 0)
                    abs += a2 * sg_arenaWallShadowSideDist/sg_arenaWallShadowDist;
            }
            */

            if ( sign != 0 && abs < sg_arenaWallShadowSideDist )
            {
                // determine height scale
                REAL heightForShadow = cam->CameraZ()+10;
                if ( this->renderHeight_*4 < heightForShadow )
                    heightForShadow = this->renderHeight_*4;

                // determine shadow extension
                REAL extension = heightForShadow*sg_arenaWallShadowSize * sign/( 1 + abs/sg_arenaWallShadowNear );
                extension *= ( 1 - abs/sg_arenaWallShadowSideDist );

                // get two more vertices for teh shadow
                eCoord P3=P1+normal*extension;
                eCoord P4=P2+normal*extension;

                // render shadow
                Color(0,0,0);
                BeginQuads();
                Vertex(P1.x, P1.y, 0);
                Vertex(P2.x, P2.y, 0);
                Vertex(P4.x, P4.y, 0);
                Vertex(P3.x, P3.y, 0);
            }
        }

        {
            eCoord vec = P1-P2;
            REAL xs = vec.x*vec.x;
            REAL ys = vec.y*vec.y;
            REAL intensity = .7 + .3 * xs/(xs+ys);
            RenderEnd( true );
            Color(intensity, intensity, intensity);
        }

        if (sg_MoviePack()){
            bool sw=false;

            if (tBeg>tEnd){
                Swap(P1,P2);
                Swap(tBeg,tEnd);
                //sw=true;
            }

            REAL ta=tBeg;
            eCoord ca=P1;
            for (int i=int(ceil(tBeg));i<tEnd;i++){
                eCoord cb=P1+(P2-P1)*((i-tBeg)/(tEnd-tBeg));
                gWallRim_helper(ca,cb,ta,i,h,Z_SCALE,sw);
                ca=cb;
                ta=i;
            }
            gWallRim_helper(ca,P2,ta,tEnd,h,Z_SCALE,sw);
        }
        else{
            // wrap manually in y-direction, some graphics card are bad at it
            REAL offset = 0;
            if (tBeg>tEnd)
                offset = -floor(tEnd);
            else
                offset = -floor(tBeg);

            tBeg += offset;
            tEnd += offset;

            gWallRim_helper(*p1,*p2,tBeg,tEnd,h,Z_SCALE,false);
        }

        //eWall::Render_helper(edge,(p1->x+p1->y)/SCALE,(p2->x+p2->y)/SCALE,40,height);
            
        if ( transparency )
            glEnable( GL_DEPTH_TEST );
    }

    // grow the wall again
    if ( se_mainGameTimer )
    {
        REAL time = se_mainGameTimer->Time();
        REAL ts = time - lastUpdate_;
        if ( ts > 0 )
        {
            lastUpdate_ = time;

            if ( renderHeight_ < .25 )
            {
                renderHeight_ = .25;
            }
            renderHeight_ *= 1 + 10 * ts;
            renderHeight_ += 5 * ts;
            if ( renderHeight_ > height )
            {
                renderHeight_ = height;
            }
        }

        if ( renderHeight_ < height )
        {
            DestroyDisplayList();
        }
    }
}

// *******************************************************************************************
// *
// *   OnBlocksCamera
// *
// *******************************************************************************************
//!
//! @param cam the camera that is blocked
//! @param height the maximal height the wall would be allowed to have without blocking the view
//!
// *******************************************************************************************

void gWallRim::OnBlocksCamera( eCamera * camera, REAL height ) const
{
    DestroyDisplayList();

    // lower the wall so it now longer blocks the view
    if ( height < renderHeight_ )
    {
        renderHeight_ = height;
   }
    if ( renderHeight_ < .25 )
        renderHeight_ = .25;
}

#endif

// *******************************************************************************************
// *
// *   Height
// *
// *******************************************************************************************
//!
//!        @return
//!
// *******************************************************************************************

REAL gWallRim::Height( void )
{
    return renderHeight_;
}

// *******************************************************************************************
// *
// *   SeeHeight
// *
// *******************************************************************************************
//!
//!        @return
//!
// *******************************************************************************************

REAL gWallRim::SeeHeight( void )
{
    return renderHeight_ * 2;
}

/* **********************************************
   PlayerWall
   ********************************************** */

#ifdef DEBUG
#define CHECKWALL this->Check();
#else
#define CHECKWALL
#endif

gPlayerWall::gPlayerWall(gNetPlayerWall*w, gCycle *p)
        :eWall(p->grid),cycle_(p),netWall_(w),begDist_(w->Pos(0)),endDist_(w->Pos(1))
{
    CHECKWALL;

    if (cycle_)
        windingNumber_ = cycle_->WindingNumber();

#ifdef DEBUG
    if (!cycle_)
    {
        //		st_Breakpoint();
    }
#endif
}

gPlayerWall::~gPlayerWall(){
    CHECKWALL;
}

//ArmageTron_eWalltype gPlayerWall::type(){return ArmageTron_PLAYERWALL;}

void gPlayerWall::Flip(){
    CHECKWALL;

    eWall::Flip();
    Swap( this->begDist_, this->endDist_ );

    CHECKWALL;
}

static void clamp01(REAL &c){
    if (!finite(c))
        c = 0.5;

    if (c<0)
        c = 0;

    if (c>1)
        c = 1;
}

// execute cycles that violate the rules of topology. To hell with them!
void sg_TopologyPoliceKill( gCycle* cycle )
{
    if ( sn_GetNetState() != nCLIENT && cycle->Alive() )
    {
        tOutput message;
        tString name;
        if ( cycle->Player() )
            name = cycle->Player()->GetName();
        else
            cycle->PrintName( name );
        message.SetTemplateParameter(1, name );
        message << "$player_topologypolice";
        sn_ConsoleOut( message );

        cycle->Kill();
    }
}

static short sg_topologyPolice = false;
static tSettingItem< short > sg_topologyPoliceCofig( "TOPOLOGY_POLICE", sg_topologyPolice );

static short sg_topologyPoliceParallel = true;
static tSettingItem< short > sg_topologyPoliceParallelCofig( "TOPOLOGY_POLICE_PARALLEL", sg_topologyPoliceParallel );

class gTopologyPoliceConsoleFiler: public tConsoleFilter
{
    virtual void DoFilterLine( tString& line )
    {
        tString oldLine = line;
        tOutput message;
        message.SetTemplateParameter(1, oldLine );
        message << "$player_topologypolice";
        line = message;
    }
};

extern bool sg_gnuplotDebug; // from gCycle.cpp

// called on a post-insert collision of two walls
// oldWall is the wall that was in place first, newWall is the wall just drawn, and point
// is a point of collision between the two.
void sg_TopologyPoliceCheck( gCycle* cycle, eWall* oldWall, gPlayerWall* newWall, const eCoord& point, bool split )
{
    // test if topology police is enabled
    if ( !sg_topologyPolice && !sg_gnuplotDebug )
        return;

    // locate point on both edges
    REAL oldAlpha = oldWall->Edge()->Ratio( point );
    REAL newAlpha = newWall->Edge()->Ratio( point );
    clamp01( newAlpha );
    clamp01( oldAlpha );

    // calculate time of passage
    REAL time = newWall->Time( newAlpha );

    // if the old wall does not lie in a hole of the new wall, don't go on.
    // this test overestimates the time ( instead of "se_GameTime()", it should
    // read just "time" ) to test wheter the wall is dangerous NOW
    // to avoid innocent victims. A wall never gets more dangerous with time.
    if ( !newWall->IsDangerous( newAlpha, se_GameTime() ) )
        return;

    // the same logic for the old wall
    gPlayerWall* oldPlayerWall = dynamic_cast< gPlayerWall* >( oldWall );
    if ( oldPlayerWall && !oldPlayerWall->IsDangerous( oldAlpha, se_GameTime() ) )
        return;

    // log collision for gnuplot
#ifdef DEBUG
    if ( sg_gnuplotDebug )
    {
        std::stringstream filename;
        if ( cycle && cycle->Player() )
        {
            filename << cycle->Player()->GetUserName() << "_";
        }
        filename << "topology";
        std::ofstream f( filename.str().c_str(), std::ios::app );
        f << point.x << " " << point.y << "\n";
    }
#endif

    // last chance to exit
    if ( !sg_topologyPolice || ( !split && !sg_topologyPoliceParallel ) )
        return;

    gTopologyPoliceConsoleFiler filter;

    // treat the crossing as if the cycle just went through the old wall.
    try
    {
        cycle->PassEdge( oldWall, time, oldAlpha );
    }
    catch ( gCycleDeath const & death )
    {
        cycle->KillAt( death.pos_ );
    }
}

// collision hooks
void gPlayerWall::SplitByActive( eWall * oldWall )
{
    if ( oldWall )
    {
        // pretend our cycle crossed the old wall just now
        eCoord intersection = oldWall->Edge()->IntersectWithCareless( Edge() );
        sg_TopologyPoliceCheck( Cycle(), oldWall, this, intersection, true );
    }
    else
    {
        sg_TopologyPoliceKill( Cycle() );
    }

    eWall::SplitByActive( oldWall );
}

// hook called when walls are really parallel on the grid
bool gPlayerWall::RunsParallelActive( eWall* oldWall )
{
    if ( oldWall )
    {

        // collision point: center of gravity
        eCoord collision = ( oldWall->Point(.5f) + this->Point(.5f) ) *.5f;

        sg_TopologyPoliceCheck( Cycle(), oldWall, this, collision, false );
    }
    else
    {
        sg_TopologyPoliceKill( Cycle() );
    }

    return eWall::RunsParallelPassive( oldWall );
}

bool gPlayerWall::Splittable() const{return 1;}

bool gPlayerWall::Deletable() const{
    CHECKWALL;

    // a wall without a cycle can clearly be deleted
    if ( !cycle_ )
        return true;

    return !IsDangerousAnywhere( se_GameTime() - 1.0f );
}

/*
static void S_Mix( const gPlayerWallCoord source[2], REAL alpha, gPlayerWallCoord& target )
{
	REAL diff  = ( source[1].Alpha - source[0].Alpha );
	REAL ralpha = 0;
	if ( diff > 0 )
		ralpha     = ( alpha - source[0].Alpha ) / diff;

	target.Alpha = alpha;
	target.Pos   = source[0].Pos  + ralpha * ( source[1].Pos  - source[0].Pos  );
	target.Pos   = source[0].Time + ralpha * ( source[1].Time - source[0].Time );
}
*/

void gPlayerWall::Split(eWall *& w1,eWall *& w2,REAL a){
    CHECKWALL;

    gPlayerWall *W1, *W2;

    W1=tNEW(gPlayerWall(netWall_,cycle_));
    W2=tNEW(gPlayerWall(netWall_,cycle_));
    W1->windingNumber_ = windingNumber_;
    W2->windingNumber_ = windingNumber_;
    W1->begDist_ = begDist_;
    W2->endDist_ = endDist_;
    W1->endDist_ = W2->begDist_ = begDist_ + ( endDist_ - begDist_ ) * a;

    /*
    	int divindex = IndexPos( mp );
    	int i;

    	// transfer front points
    	W1->coords_.SetLen( divindex + 2 );
    	for ( i = divindex+1; i>=0; --i )
    		W1->coords_(i) = coords_(i);

    	W1->coords_(divindex+1).Pos  = mp;
    	W1->coords_(divindex+1).Time = mt;

    	// transfer rear points
    	W2->coords_.SetLen( coords_.Len() - divindex );
    	for ( i = coords_.Len() - divindex - 1 ; i>=0; --i )
    		W2->coords_(i) = coords_( divindex + i );

    	W2->coords_(0).Pos  = mp;
    	W2->coords_(0).Time = mt;

    	if ( flipped )
    		Swap( W1, W2 );
    */

    // store wall pointers
    w1 = W1;
    w2 = W2;

#ifdef DEBUG
    W1->Check();
    W2->Check();
#endif

    CHECKWALL;
}

//#define gBEG_LEN 2
//#define gBEG_LEN .25
//#define gCYCLE_LEN .95
//#define gCYCLE_LEN 1.6
#define gCYCLE_LEN 1.5
#define gBEG_OFFSET .25
#define gBEG_LEN 2
//#define gCYCLE_LEN 3.8
//#define gBEG_OFFSET 1

#ifndef DEDICATED
void gPlayerWall::Render(const eCamera *cam){
    if (!cycle_)
        return;
    RenderList(true);
}

void gNetPlayerWall::Render(const eCamera *cam ){
    if (!cycle_)
        return;
    RenderList(true);
}

/*
class gPerformanceCounter
{
public:
    gPerformanceCounter(): count_(0){ tRealSysTimeFloat(); }
    unsigned int Count(){ return count_++; }
    ~gPerformanceCounter()
    {
        double time = tRealSysTimeFloat();
        std::stringstream s;
        s << count_ << " walls in " << time << " seconds: " << count_ / time << " wps.\n";
#ifdef WIN32
        MessageBox (NULL, s.str().c_str() , "Performance", MB_OK);
#else
        std::cout << s.str();
#endif
    }
private:
    unsigned int count_;
};
*/

void gPlayerWall::RenderList(bool list)
{
    netWall_->RenderList( list );
}

void gNetPlayerWall::RenderList(bool list, gWallRenderMode renderMode ){
    if ( !cycle_ )
    {
        return;
    }

#ifdef DEBUG_X
    if ( cycle_->Player()->GetName().StartsWith("B") && dbegin < .1 )
    {
        int x;
        x = 1;
    }
#endif
    // clear list if walls are vanishing
    // or if the wall end was reached
    // or this is the cycle's first wall
    if ( gCycleWallsDisplayListManager::CannotHaveList( dbegin, cycle_ ) ||
         this == cycle_->currentWall )
    {
        ClearDisplayList(2);
    }

    if ( !displayList_.Call() )
    {   
        //static gPerformanceCounter counter;
        //counter.Count();

        rDisplayListFiller filler( displayList_ );

        REAL r,g,b;
        if (cycle_){
            r=cycle_->trailColor_.r_;
            g=cycle_->trailColor_.g_;
            b=cycle_->trailColor_.b_;
        }
        else
            r=g=b=1;

        eCoord P1=EndPoint(0);
        eCoord P2=EndPoint(1);

        {
            eCoord vec = P2-P1;
            REAL xs = vec.x*vec.x;
            REAL ys = vec.y*vec.y;
            REAL intensity = .7 + .3 * xs/(xs+ys);
            r *= intensity;
            g *= intensity;
            b *= intensity;
        }

        REAL a=1;

#define SEGLEN 2.5
        //REAL ta=startTime*3;
        //REAL te=endTime*3;
        for ( int i = coords_.Len()-2; i>=0; --i )
        {
            const gPlayerWallCoord* coord = &coords_(i);

            if ( !coord[0].IsDangerous )
                continue;

            REAL pa = coord[0].Pos;
            REAL pe = coord[1].Pos;

            REAL aa = Alpha( pa );
            REAL ae = Alpha( pe );

            eCoord p1 = P1 + ( P2 - P1 ) * aa;
            eCoord p2 = P1 + ( P2 - P1 ) * ae;

            REAL ta=pa/SEGLEN;
            REAL te=pe/SEGLEN;
            //REAL shift=REAL(floor((ta+te)/20)*10);

            //REAL time=ArmageTronTimer*3;
            REAL time;
            if (cycle_)
            {
                if ( cycle_->currentWall )
                    time = cycle_->currentWall->EndPos()/SEGLEN;
                else
                    time=cycle_->GetDistance()/SEGLEN;
                if ( !cycle_->Alive() )
                    time += se_GameTime() - cycle_->deathTime;
            }
            else
                time=0;

            //ta-=shift;
            //te-=shift;
            //time-=shift;

            if (ta>te){
                Swap(ta,te);
                Swap(p1,p2);
                Swap(pa,pe);
            }

            // cut the end of the wall
            if ( bool(cycle_) && gCycle::WallsLength() > 0 )
            {
                REAL cut = (cycle_->GetDistance() - cycle_->ThisWallsLength() - pe) / ( pa - pe );
                if ( cut < 0 )
                    continue;
                if ( cut < 1 )
                {
                    p1 = p2 + (p1-p2)*cut;
                    ta = te + (ta-te)*cut;
                }
            }

            if (te+gBEG_LEN<=time){
                RenderNormal(p1,p2,ta,te,r,g,b,a,renderMode);
                sr_CheckGLError();
            }

            else{ // complicated
                // can't squeeze that into a display list
                ClearDisplayList();

                if (ta+gBEG_LEN>=time){
                    sr_CheckGLError();
                    RenderBegin(p1,p2,ta,te,
                                1+(ta-time)/gBEG_LEN,
                                1+(te-time)/gBEG_LEN,
                                r,g,b,a);
                    sr_CheckGLError();
                }
                else{
                    sr_CheckGLError();
                    REAL s=((time-gBEG_LEN)-ta)/(te-ta);
                    eCoord pm=p1+(p2-p1)*s;
                    RenderBegin(pm,p2,
                                ta+(te-ta)*s,te,0,
                                1+(te-time)/gBEG_LEN,
                                r,g,b,a);
                    RenderNormal(p1,pm,ta,ta+(te-ta)*s,r,g,b,a, gWallRenderMode_All );
                    sr_CheckGLError();
                }
            }
        }
    }
}


inline bool upperlinecolor(REAL r,REAL g,REAL b, REAL a){
    if (rTextureGroups::TextureMode[rTextureGroups::TEX_WALL]<0)
        glColor4f(1,1,1,a);
    else{
        /*
          REAL upperline_alpha=fabs(se_cameraRise*2);
          upperline_alpha-=1;
          if (upperline_alpha>.5)
          upperline_alpha=1;
          if (upperline_alpha<=.5)
          return false;
          glColor4f(r,g,b,upperline_alpha);
        */
        //glDisable(GL_TEXTURE);
        glColor4f(r,g,b,a);
    }

    return true;
}

void gNetPlayerWall::RenderNormal(const eCoord &p1,const eCoord &p2,REAL ta,REAL te,REAL r,REAL g,REAL b,REAL a, gWallRenderMode mode ){
    REAL hfrac=1;

    if (bool(cycle_) && !cycle_->Alive() && gCycle::WallsStayUpDelay() >= 0 ){
        REAL dt=(se_GameTime()-cycle_->deathTime-gCycle::WallsStayUpDelay())*2;

        if (dt>1)
        {
            // remove from rendering lists
            Remove();
            return;
        }

        if (dt>=0)
        {
            REAL ca=REAL(.5/(dt+.5));
            REAL alpha=1-dt;
            if (alpha>1) alpha=1;
            hfrac=1-dt;

            r+=ca;
            b+=ca;
            g+=ca;

            a*=alpha;
        }
    }
    REAL h=1;


    if (hfrac>0){
        if ( ( mode & gWallRenderMode_Lines ) ){

            // draw additional upper line
            if ( mode == gWallRenderMode_All )
            {
                sr_DepthOffset(true);
                if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
                {
                    RenderEnd();
                    glDisable(GL_TEXTURE_2D);
                }
            }

            BeginLines();
            upperlinecolor(r,g,b,a);
            glVertex3f(p1.x,p1.y,h*hfrac);
            upperlinecolor(r,g,b,a);
            glVertex3f(p2.x,p2.y,h*hfrac);

            // in the other modes, the caller is responsible for
            // calling RenderEnd() and resetting the states.
            if ( mode == gWallRenderMode_All )
            {
                RenderEnd();
                sr_DepthOffset(false);
                if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
                    glEnable(GL_TEXTURE_2D);
            }
        }

        //glColor4f(r,g,b,a);

#ifdef XDEBUG
        REAL extrarise = 0;
        if ( this->id >= 0 )
        {
            extrarise = 1;
        }
#else
        static const REAL extrarise = 0;
#endif
        if ( mode & gWallRenderMode_Quads )
        {
            BeginQuads();

            glColor3f(r,g,b);
            glTexCoord2f(ta,hfrac);
            glVertex3f(p1.x,p1.y,extrarise);
            
            glColor3f(r,g,b);
            glTexCoord2f(ta,0);
            glVertex3f(p1.x,p1.y,extrarise + h*hfrac);
            
            glColor3f(r,g,b);
            glTexCoord2f(te,0);
            glVertex3f(p2.x,p2.y,extrarise + h*hfrac);
            
            glColor3f(r,g,b);
            glTexCoord2f(te,hfrac);
            glVertex3f(p2.x,p2.y,extrarise);

			// in the other modes, the caller is responsible for
			// calling RenderEnd().
			if ( mode == gWallRenderMode_All )
			{
				RenderEnd();
			}
        }
    }
}

static inline REAL hfunc(REAL x){return 1-(x*x)/2;}
//static inline REAL hfunc(REAL x){return 1-(x*x);}
static inline REAL cfunc(REAL x){return (x*x);}
//static inline REAL afunc(REAL x){return 1-(x*x)/2;}
static inline REAL afunc(REAL x){return 1-(x*x);}
static inline REAL sfunc(REAL x){return (x*x);}
//static inline REAL xfunc(REAL x){return (x+x*x)/2;}
static inline REAL xfunc(REAL x){return REAL((x*.2+x*x)/2);}

void gNetPlayerWall::RenderBegin(const eCoord &p1,const eCoord &pp2,REAL ta,REAL te,REAL ra,REAL re,REAL r,REAL g,REAL b,REAL a){
    if ( !cycle_ )
    {
        return;
    }

    REAL hfrac=1;

    eCoord p2 = pp2;

    if (re > 1){
        if (re > 2)
            return;

        REAL ratio = (1-ra)/(re-ra);
        p2 = p1 + (pp2-p1)*ratio;
        te = ta + (te-ta)*ratio;
        re= 1;
    }

    if (bool(cycle_) && !cycle_->Alive()){
        REAL dt=(se_GameTime()-cycle_->deathTime-gCycle::WallsStayUpDelay())*2;
        if (dt>1) dt=1;
        if (dt>0)
        {
            REAL ca=REAL(.5/(dt+.5));
            REAL alpha=1-dt;
            if (alpha>1) alpha=1;
            hfrac=1-dt;

            r+=ca;
            b+=ca;
            g+=ca;
        }
        //a*=alpha;
    }

    REAL h=1;

    eCoord ppos=cycle_->PredictPosition() - cycle_->dir*REAL(gCYCLE_LEN);

    if ( hfrac>0 ){
        sr_DepthOffset(true);  
        //REAL H=h*hfrac;
#define segs 5
        upperlinecolor(r,g,b,a);//a*afunc(rat));

        if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
            glDisable(GL_TEXTURE_2D);
        
        BeginLineStrip();

        for (int i=0;i<=segs;i++){
            REAL frag=i/float(segs);
            REAL rat=ra+frag*(re-ra);
            REAL x=(p1.x+frag*(p2.x-p1.x))*(1-xfunc(rat))+ppos.x*xfunc(rat);
            REAL y=(p1.y+frag*(p2.y-p1.y))*(1-xfunc(rat))+ppos.y*xfunc(rat);

            REAL H=h*hfrac*hfunc(rat);
            upperlinecolor(r,g,b,a*afunc(rat));
            glVertex3f(x+H*cycle_->skew*sfunc(rat)*cycle_->dir.y,
                       y-H*cycle_->skew*sfunc(rat)*cycle_->dir.x,
                       H);//+se_cameraZ*.005);
        }
        RenderEnd();

        sr_DepthOffset(false);
        if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
            glEnable(GL_TEXTURE_2D);
    }

    sr_CheckGLError();
    BeginQuadStrip();

    //REAL H=h*hfrac;

    //ppos=ePlayer->pos-ePlayer->dir*gCYCLE__LEN;

    for (int i=0;i<=segs;i++){
        REAL frag=i/float(segs);
        REAL rat=ra+frag*(re-ra);
        REAL x=(p1.x+frag*(p2.x-p1.x))*(1-xfunc(rat))+ppos.x*xfunc(rat);
        REAL y=(p1.y+frag*(p2.y-p1.y))*(1-xfunc(rat))+ppos.y*xfunc(rat);

        // bottom
        glColor4f(r+cfunc(rat),g+cfunc(rat),b+cfunc(rat),a*afunc(rat));
        glTexCoord2f(ta+(te-ta)*frag,hfrac);
        glVertex3f(x,y,0);

        // top

        //glTexCoord2f(ta+(te-ta)*frag,hfrac*(1-hfunc(rat)));
        glTexCoord2f(ta+(te-ta)*frag,0);
        REAL H=h*hfrac*hfunc(rat);
        glVertex3f(x+H*cycle_->skew*sfunc(rat)*cycle_->dir.y,
                   y-H*cycle_->skew*sfunc(rat)*cycle_->dir.x,
                   H);
    }
    RenderEnd();
    sr_CheckGLError();
}
#endif

void gNetPlayerWall::SetEndTime(REAL t){
    CHECKWALL;

    REAL BegTime = coords_( coords_.Len() -2 ).Time;
    if ( t < BegTime )
    {
        t = BegTime;
    }

    coords_(coords_.Len()-1).Time = t;

    CHECKWALL;

}

void gNetPlayerWall::SetEndPos(REAL ep){
    CHECKWALL;

    REAL BegPos = coords_( coords_.Len() -2 ).Pos;
    if ( ep < BegPos )
    {
        ep = BegPos;
    }

    coords_(coords_.Len()-1).Pos = ep;

    CHECKWALL;
}

REAL gPlayerWall::BlockHeight() const{
    if (bool(cycle_) && cycle_->Alive()==1)
        return 1;
    else
        return 0;
}

REAL gPlayerWall::SeeHeight() const{
    return BlockHeight();
}


gCycle *gPlayerWall::Cycle() const {return cycle_;}
gCycleMovement *gPlayerWall::CycleMovement() const {return cycle_;}
gNetPlayerWall *gPlayerWall::NetWall() const {return netWall_;}

void gPlayerWall::Insert()
{
    CHECKWALL;

    eWall::Insert();
}

void gPlayerWall::Check() const
{
    netWall_->Check();
#ifdef DEBUG
    REAL range = 5 * fabs(begDist_) + fabs(endDist_) * EPS;
    tASSERT( begDist_ <= endDist_ + range );
    tASSERT( begDist_ >= netWall_->Pos( 0 ) - range );
    tASSERT( endDist_ <= netWall_->Pos( 1 ) + range );
#endif
}

REAL gPlayerWall::LocalToGlobal( REAL a ) const
{
    CHECKWALL;

    tASSERT( good( a ) );

    REAL dist = begDist_ + a * ( endDist_ - begDist_ );
    REAL ret = netWall_->Alpha( dist );

    tASSERT( good( ret ) );

    return ret;
}

void gNetPlayerWall::ClearDisplayList( int inhibitThis, int inhibitCycle )
{
#ifndef DEDICATED
    if ( CanHaveDisplayList() && cycle_ && inhibitCycle >= 0 )
    {
        cycle_->displayList_.Clear( inhibitCycle );
    }
    displayList_.Clear( inhibitThis );
#endif
}

REAL gPlayerWall::GlobalToLocal( REAL a ) const
{
    CHECKWALL;

    tASSERT( good( a ) );

    REAL dist = netWall_->Pos( a );

    REAL div = ( endDist_ - begDist_ );
    if ( div == 0 )
    {
        return .5f;
    }
    else
    {
        REAL ret = ( dist - begDist_ ) / div;

        tASSERT( good( ret ) );

        return ret;
    }
}

REAL gPlayerWall::Time(REAL a) const
{
    tASSERT( good( a ) );

    return netWall_->Time( LocalToGlobal( a ) );
}

REAL gPlayerWall::Pos(REAL a) const
{
    CHECKWALL;

    tASSERT( good( a ) );

    return begDist_ + ( endDist_ - begDist_ ) * a;
}

REAL gPlayerWall::Alpha(REAL pos) const
{
    CHECKWALL;

    REAL diff = ( endDist_  - begDist_ );
    REAL a = pos - begDist_;

    if ( diff > 0 )
        a /= diff;

    tASSERT ( -.001 < a );
    tASSERT ( 1.001 > a );

    return a;
}

bool gPlayerWall::IsDangerousAnywhere( REAL time ) const
{
    CHECKWALL;

    return netWall_->IsDangerousAnywhere( time );
}

bool gPlayerWall::IsDangerous( REAL a, REAL time ) const
{
    CHECKWALL;

    return netWall_->IsDangerous( LocalToGlobal( a ), time );
}

// returns the guy who holed here
gExplosion * gPlayerWall::Holer( REAL a, REAL time ) const
{
    CHECKWALL;

    return netWall_->Holer( LocalToGlobal( a ), time );
}

REAL gPlayerWall::EndPos() const
{
    CHECKWALL;

    return this->endDist_;
}

REAL gPlayerWall::BegPos() const
{
    CHECKWALL;

    return this->begDist_;
}

REAL gPlayerWall::EndTime() const
{
    CHECKWALL;

    return netWall_->Time( netWall_->Alpha( this->endDist_ ) );
}

REAL gPlayerWall::BegTime() const
{
    CHECKWALL;

    return netWall_->Time( netWall_->Alpha( this->begDist_ ) );
}

void gPlayerWall::BlowHole	( REAL beg, REAL end, gExplosion * holer )
{
    CHECKWALL;

    this->netWall_->BlowHole( beg, end, holer );
}

/*
void gPlayerWall::Clamp	(  )
{
	tASSERT( coords.Len() >= 2 );

	// clamp beginning
	int begin = IndexAlpha(0.0f);
	gPlayerWallCoord* bcoord = &coords[begin];
	S_Mix( bcoord, 0.0f, bcoord[0] );

	// clamp end
	int end = IndexAlpha(1.0f);
	gPlayerWallCoord* ecoord = &coords[end];
	S_Mix( ecoord, 1.0f, ecoord[1] );

	// useful coordinates now lie between begin and end+1

	// throw away junk at the beginning
	if ( begin > 0 )
	{
		for ( int i = 0; i <= end + 1 - begin; ++i )
			coords[i] = coords[ i + begin ];
	}

	// throw away junk at the end
	coords.SetLen( end - begin + 2 );
}
*/

// ************************************************
// ************************************************


tList<gNetPlayerWall> sg_netPlayerWalls;
tList<gNetPlayerWall> sg_netPlayerWallsGridded;


void gNetPlayerWall::CreateEdge()
{
    if ( this->edge_ )
        return;

    if (this->cycle_)
    {
        gPlayerWall* w = tNEW(gPlayerWall)(this,
                                           this->cycle_);

        this->edge_=tNEW(eTempEdge)(beg,
                                    end,
                                    w );
    }
    else
    {
        this->edge_ = NULL;
        return;
    }
}

void gNetPlayerWall::InitAfterCreation()
{
    nNetObject::InitAfterCreation();
    MyInitAfterCreation();
}

void gNetPlayerWall::InitArray()
{
    REAL ep = dbegin+sqrt((beg-end).NormSquared());
    REAL sp = dbegin;

    if ( ep < sp )
    {
        ep = sp;
    }

    if ( tEnd < tBeg )
    {
        tEnd = tBeg;
    }

    coords_.SetLen(2);
    coords_[0].Pos   		= sp;
    coords_[0].Time  		= tBeg;
    coords_[0].IsDangerous   = true;
    coords_[1].Pos   		= ep;
    coords_[1].Time  		= tEnd;
    coords_[1].IsDangerous   = true;
}

void gNetPlayerWall::MyInitAfterCreation()
{
#ifndef DEDICATED
    // put yourself into rendering list
    if ( cycle_ )
    {
        Insert( cycle_->displayList_.wallList_ );
    }
#endif

    //w=
#ifdef DEBUG
    if (!finite(end.x) || !finite(end.y))
        st_Breakpoint();

    if (!finite(beg.x) || !finite(beg.y))
        st_Breakpoint();
#endif

    if ( coords_.Len() < 2 )
    {
        InitArray();
    }

    CreateEdge();

    id=-1;
    griddedid=-1;
    sg_netPlayerWalls.Add(this,id);

    if ( !Wall() )
        return;
    tASSERT( Wall()->Splittable() );

    for (int i=MAX_VIEWERS-1;i>=0;i--)
        Wall()->SetVisHeight(i,0);

    Wall()->Remove();

    displayList_.Clear(2);
}



gNetPlayerWall::gNetPlayerWall(gCycle *cyc,
                               const eCoord &begi,const eCoord &d,
                               REAL tBegi, REAL dbeg)
        :nNetObject(cyc->Owner()),
        id(-1),griddedid(-1),
        cycle_(cyc),lastWall_(NULL),dir(d),dbegin(dbeg),
        beg(begi),end(begi),tBeg(tBegi),tEnd(tBegi),
        inGrid(false){
    dir=dir; // Don't normalize: *REAL(1/sqrt(dir.NormSquared()));
    preliminary=(sn_GetNetState()==nCLIENT);
    obsoleted_=-100;
    gridding=1E+20;
    MyInitAfterCreation();
}

/*
void gNetPlayerWall::Update(REAL Tend,REAL dend){
	if (!inGrid){
		tEnd=Tend;
		end=beg + dir*(dend-dbegin);

#ifdef DEBUG
		if (!finite(end.x) || !finite(end.y))
			st_Breakpoint();
#endif

		if (e)
			e->Coord(1) = end;

		gPlayerWall *w = Wall();
		if (w){
			w->SetEndTime(tEnd);
			w->SetEndPos(dend);
			w->CalcLen();
		}
	}
}
*/

void gNetPlayerWall::Update(REAL Tend,const eCoord &pend)
{
    CHECKWALL;

    if (!inGrid && ( preliminary || sn_GetNetState() != nCLIENT ) )
    {
        real_Update( Tend, pend, false );
    }

    CHECKWALL;
}

void gNetPlayerWall::real_Update(REAL Tend,const eCoord &pend, bool force )
{
    // duplicate last coords entry if its dangerousness disagrees with the previous entry.
    if ( coords_.Len() >= 2 && coords_[ coords_.Len()-2 ].IsDangerous != coords_[ coords_.Len()-1 ].IsDangerous )
    {
        Checkpoint();
    }

    tEnd=Tend;
    end=pend;

    // make sure the wall points forward
    REAL forward = eCoord::F( end-beg, dir )/dir.NormSquared();
    if ( forward < 0 )
    {
        end = beg;
        tEnd = tBeg;
    }

#ifdef DEBUG
    if (!finite(end.x) || !finite(end.y))
        st_Breakpoint();
#endif

    eCoord odir=dir.Turn(0,1);
    REAL x=eCoord::F(odir,(end-beg))/dir.NormSquared();
    beg=beg+odir*x;

    if (bool( this->edge_ ) && this->edge_->Point(0) && this->edge_->Point(1)){
        this->edge_->Coord(1) = end;
        if ( !lastWall_ )
            this->edge_->Coord(0) = beg;
    }

    // determine the correct end position
    REAL endPos = 0;
    //if ( bool( this->cycle_ ) && !force )
    //{
    //    endPos =  this->cycle_->GetDistance();
    //}
    //else
    {
        endPos = dbegin + eCoord::F(dir, end - beg )/dir.NormSquared();
    }

    // delete coords_ entries that lie after the last one according to their distance; they're invalidated.
    {
        int len = coords_.Len();
        while ( len >= 3 && coords_[len-2].IsDangerous == coords_[len-1].IsDangerous && coords_[len-2].Pos > endPos )
        {
            coords_[len-2] = coords_[len-1];
            coords_.SetLen(len - 1);
            len = coords_.Len();
        }
    }

    // set end position and time
    SetEndTime(tEnd);
    SetEndPos(endPos);


    gPlayerWall *w = Wall();

    if ( w )
    {
        w->CalcLen();
        if ( !lastWall_ )
            w->begDist_ = dbegin;
        w->endDist_ = EndPos();
#ifdef DEBUG
        w->Check();
#endif
    }
}

void gNetPlayerWall::Checkpoint()
{
    CHECKWALL;

    // copy the last coordinate entry
    int len = coords_.Len();

#ifdef DEBUG
    if ( len > 100 )
    {
        st_Breakpoint();
    }
#endif

    // temporary is required to compensate for growing array nightmare
    coords_[len] = gPlayerWallCoord( coords_[len-1] );

    CHECKWALL;
}

void gNetPlayerWall::CopyIntoGrid(eGrid * grid, bool force){
    tJUST_CONTROLLED_PTR< gNetPlayerWall > keep( this );

    if (!inGrid && (force ||
                    (sn_GetNetState()!=nCLIENT || preliminary))){
        inGrid=true;
        gridding=REAL(se_GameTime()+1.0);
        if (sn_GetNetState()==nCLIENT)
        {
            // leave the wall lingering around for some time on the client
            gridding=se_GameTime()+40*sn_Connections[0].ping+10;

            // unless it is already obsoleted by a final wall or IS a final wall. Delete/grid it immediately then.
            if ( obsoleted_ > tEnd - .003f || !preliminary )
            {
                if ( grid )
                    real_CopyIntoGrid( grid );
                else
                    gridding=REAL(se_GameTime()+.000001);
            }
        }
        else
        {
            // copy it into the grid at the next opportunity for server/standalone mode
            RequestSync();
            if ( grid )
                real_CopyIntoGrid( grid );
            else
                gridding=REAL(se_GameTime()+.000001);
        }
    }
}

void gNetPlayerWall::real_CopyIntoGrid(eGrid *grid){
    //  con << "Gridding " << ID() << " : ";
    //con << "from " << *e->Point(0) << " to " << *e->Point(1) << '\n';

    tJUST_CONTROLLED_PTR< gNetPlayerWall > keep( this );

#ifdef DEBUG
    grid->Check();
#endif

    if (griddedid<0){
        if ( this->cycle_ )
        {
            tASSERT( static_cast< bool >(this->edge_) );
            tASSERT(Wall());
            tASSERT(Wall()->Splittable());

            if (preliminary){
                //delete this; // get rid of it
                tControlledPTR< nNetObject > bounce( this );

                sg_netPlayerWalls.Remove(this,id);
                sg_netPlayerWallsGridded.Add(this,griddedid);
                Wall()->Insert();
                this->ReleaseData();
                ClearDisplayList();
            }
            else{
                sg_netPlayerWallsGridded.Add(this,griddedid);
                sg_netPlayerWalls.Remove(this,id);
                if ( this->edge_ ){
                    Wall()->Insert();
                    this->edge_->CopyIntoGrid(this->cycle_->Grid());
                    this->edge_ = NULL;
                }
            }
        }
    }

#ifdef DEBUG
    grid->Check();
#endif

}

void gNetPlayerWall::PartialCopyIntoGrid(eGrid *grid){
    //  con << "Gridding " << ID() << " : ";
    //con << "from " << *e->Point(0) << " to " << *e->Point(1) << '\n';

    tJUST_CONTROLLED_PTR< gNetPlayerWall > keep( this );

#ifdef DEBUG
    grid->Check();
#endif

    if (griddedid<0 && bool(this->cycle_) && !preliminary ){

        // just copy the current edge into the grid
        if ( this->edge_ ){
            lastWall_ = Wall();
            Wall()->Insert();
            this->edge_->CopyIntoGrid(grid);
            this->edge_ = NULL;
        }

        // and create a new one just at the end bit
        gPlayerWall* w = tNEW(gPlayerWall)(this,
                                           this->cycle_);
        this->edge_=tNEW(eTempEdge)(end,
                                    end,
                                    w );

        // insert it into the list of not yet gridded walls
        w->Remove();

        // hack the beginning distance to be the same as the starting distance
        w->begDist_ = w->endDist_;
    }

    // add a new segment as a copy of the current one
    // int newCoord = coords_.Len();
    // coords_[newCoord]=coords_[newCoord-1];

#ifdef DEBUG
    grid->Check();
#endif

}

void gNetPlayerWall::s_CopyIntoGrid()
{
#ifdef DEBUG
    static int maxw=20;
    if (sg_netPlayerWalls.Len()>maxw)
        con << "Many walls: " << (maxw=sg_netPlayerWalls.Len()) << '\n';
#endif

    for (int i=sg_netPlayerWalls.Len()-1;i>=0;i--){
        gNetPlayerWall *w=sg_netPlayerWalls(i);
        if (w->inGrid && w->griddedid<0 && se_GameTime()>w->gridding)
            w->real_CopyIntoGrid(w->cycle_->Grid());
    }
}

void gNetPlayerWall::RealWallReceived( gNetPlayerWall* realWall )
{
    if (this->cycle_ )
    {
        tASSERT( realWall );
        tASSERT( preliminary && !realWall->preliminary );

        // accelerate gridding if the real wall is newer than this
        if ( tBeg + tEnd < 2 * realWall->tEnd )
        {
            REAL maxGridding=se_GameTime() + 2*sn_Connections[0].ping;
            if ( gridding > maxGridding )
                gridding = maxGridding;
        }

        // calculate the overlap between the real wall and this wall
        REAL overlap = 0;
        {
            REAL tEndThis = tEnd;
            // cut from the end if we're in prediction mode and this is an enemy wall
            if ( sr_predictObjects && this->cycle_->currentWall == this && Owner() != sn_myNetID )
            {
                tEndThis -= this->cycle_->Lag();
            }

            REAL tBegMin = tBeg;
            REAL tBegMax = realWall->tBeg;
            if ( tBegMin > tBegMax )
            {
                tBegMin = realWall->tBeg;
                tBegMax = tBeg;
            }

            REAL tEndMin = tEndThis;

            REAL tEndMax = realWall->tEnd;
            if ( tEndMin > tEndMax )
            {
                tEndMin = realWall->tEnd;
                tEndMax = tEndThis;
            }

            REAL denominator = tEndMax - tBegMin;
            if ( denominator > 0 )
                overlap = ( tEndMin - tBegMax ) / denominator;
        }

        // no overlap if directions don't match
        if ( overlap > 0 && fabs( dir * realWall->dir ) > 10 * EPS )
            overlap = 0;

        // no good overlap? Go home.
        if ( overlap < .8 )
            return;

        // mark current walls as to be deleted immediately after the cycle does no longer need it
        obsoleted_ = realWall->tEnd;

        // copy non-current walls into the grid immediately
        if (this->cycle_->currentWall!=this )
        {
            // replace pointer in cycle
            if (this->cycle_->lastWall==this)
            {
                this->cycle_->lastWall=realWall;

                /*
                // close seams (does not help, deactivated)
                if ( this->cycle_->currentWall && this->cycle_->currentWall->preliminary )
                {
                    this->cycle_->currentWall->tBeg   = realWall->tEnd;
                    this->cycle_->currentWall->coords_[0].Time   = realWall->tEnd;
                    this->cycle_->currentWall->dbegin = realWall->EndPos();
                }
                */
            }

            // delete this wall, it is no longer needed
            this->real_CopyIntoGrid( this->cycle_->Grid() );
        }
    }
}


gNetPlayerWall::gNetPlayerWall( Game::PlayerWallSync const & sync, nSenderInfo const & sender )
        :nNetObject( sync.base(), sender ),
        id(-1),griddedid(-1),
        cycle_(NULL),edge_(NULL), lastWall_(NULL),
        dir(0,0),dbegin(0),
        beg(0,0),end(0,0),
        tBeg(0),tEnd(0),
        inGrid(0)
{
    gridding=1E+20;
    IDToPointer( sync.cycle_id(), cycle_ );

    beg.ReadSync( sync.begin_pos() );
    end=beg;
    dir.ReadSync( sync.direction() );
    dbegin = sync.begin_distance();

    tBeg = sync.begin_time();
    preliminary = sync.preliminary();

    obsoleted_=-100;

    this->InitArray();
}

eCoord gNetPlayerWall::Vec()
{
    if ( edge_ ) return edge_->Vec();
    else return eCoord();
}

gPlayerWall *gNetPlayerWall::Wall(){
    if (this->edge_)
    {
        eWall *w = this->edge_->Wall();

        return reinterpret_cast<gPlayerWall *>(w);
    }
    else
        return NULL;
}

void gNetPlayerWall::ReleaseData()
{
    if (this->cycle_){
        if (this->cycle_->currentWall==this)
            this->cycle_->currentWall=NULL;
        if (this->cycle_->lastWall==this)
            this->cycle_->lastWall=NULL;
        if (this->cycle_->lastNetWall==this)
            this->cycle_->lastNetWall=NULL;
    }

    // tDESTROY(w);

    if (this->edge_)
    {
        if ( this->edge_->Wall() )
            this->edge_->Wall()->Insert();

        this->edge_ = NULL;  // w will be deleted with e
        //    tDESTROY_PTR(p1);
        //    tDESTROY_PTR(p2);
    }

    this->cycle_=NULL;
    this->edge_=NULL;
    //w=NULL;

    sg_netPlayerWalls.Remove(this,id);
    sg_netPlayerWallsGridded.Remove(this,griddedid);
}

gNetPlayerWall::~gNetPlayerWall()
{
    ReleaseData();
    ClearDisplayList();
}

bool gNetPlayerWall::ActionOnQuit()
{
    if ( sn_GetNetState() == nSERVER )
    {
        TakeOwnership();
        return false;
    }
    else
    {
        ReleaseData();

        return true;
    }
}

bool gNetPlayerWall::ClearToTransmit(int user) const{
#ifdef DEBUG
    if (nNetObject::DoDebugPrint() && bool( this->cycle_ ) )
    {
        if (!GridIsReady(user))
            con << "Not transfering gNetPlayerWall " << ID()
            << " for user " << user << " because the grid is not ready yet.\n";
        else if (!this->cycle_)
            con << "Not transfering gNetPlayerWall " << ID()
            << " for user " << user << " because it has no cycle!\n";
        else if (!this->cycle_->HasBeenTransmitted(user))
        {
            tString s;
            s << "No transfering gNetPlayerWall " << ID()
            << " for user " << user << " because ";
            this->cycle_->PrintName(s);
            s << " has not been transmitted.\n";
            con << s;
        }
    }
#endif

    return GridIsReady(user) && nNetObject::ClearToTransmit(user)
           && bool(this->cycle_) && this->cycle_->HasBeenTransmitted(user) && inGrid;
}

void gNetPlayerWall::WriteSync( Game::PlayerWallSync & sync, bool init ) const
{
    nNetObject::WriteSync( *sync.mutable_base(), init );

    if( init )
    {
        sync.set_cycle_id( PointerToID( cycle_ ) );
        beg.WriteSync( *sync.mutable_begin_pos() );
        dir.WriteSync( *sync.mutable_direction() );
        sync.set_begin_distance( dbegin );
        sync.set_begin_time( tBeg );
        sync.set_preliminary( preliminary );
    }

    if (inGrid){
        end.WriteSync( *sync.mutable_end_pos() ); // the far end of the eWall
        sync.set_end_time( tEnd ); // the endTime
    }
    else{
        // write start data instead
        beg.WriteSync( *sync.mutable_end_pos() );
        sync.set_end_time( tBeg );
    }
    sync.set_in_grid( inGrid );

    if ( coords_.Len() > 2 || !coords_(0).IsDangerous || !coords_(1).IsDangerous )
    {
        unsigned short len = coords_.Len();
        sync.clear_segments();
        for ( int i = 0; i < len; ++i )
        {
            const gPlayerWallCoord& coord = coords_(i);
            Game::WallCoordSync * s = sync.add_segments();
            s->set_dangerous( coord.IsDangerous );
            s->set_distance( coord.Pos );
            s->set_time( coord.Time );
        }
    }
}

static bool sg_ServerSentHoles = false;

void gNetPlayerWall::ReadSync( Game::PlayerWallSync const & sync, nSenderInfo const & sender )
{
    nNetObject::ReadSync( sync.base(), sender );

    ClearDisplayList();

    REAL tEnd_new = sync.end_time();
    eCoord end_new;
    end_new.ReadSync( sync.end_pos() );

    if ( tEnd_new < tBeg )
    {
        tEnd_new = tBeg;
    }

    unsigned short new_inGrid = sync.in_grid();

    if ( griddedid < 0 )
        CreateEdge();

    if ( sync.segments_size() )
    {
        unsigned short len = sync.segments_size();

        coords_.SetLen( len );

        for ( int i = len-1; i>=0; --i )
        {
            gPlayerWallCoord& coord = coords_(i);
            Game::WallCoordSync const & s = sync.segments(i);
            coord.IsDangerous = s.dangerous();
            coord.Pos         = s.distance();
            coord.Time        = s.time();
        }

        sg_ServerSentHoles = true;
    }

    real_Update(tEnd_new,end_new, true);

    if (Wall() && new_inGrid && !inGrid)
    {
        /*
        		if ( ( beg - end ).NormSquared() > 0.01f )
        		{
        			gExplosion::OnNewWall( Wall() );
        		}
        */

        CopyIntoGrid( NULL, true );

        if (!preliminary)
        {
            // inform preliminary walls
            for (int i=sg_netPlayerWalls.Len()-1;i>=0;i--)
            {
                gNetPlayerWall *o=sg_netPlayerWalls[i];
                if ( o != this && o->preliminary && o->cycle_ == this->cycle_ )
                {
                    o->RealWallReceived( this );
                }
            }

            // test whether this wall is newer than the last received wall in the cycle
            if ( ( 0 != this->cycle_ ) && ( !this->cycle_->lastNetWall || this->cycle_->lastNetWall->tBeg < this->tBeg ) )
            {
                this->cycle_->lastNetWall = this;
            }
        }
    }
    else
    {
        //		st_Breakpoint();
    }

#ifdef DEBUG
    if ( Wall() )
        Wall()->Check();
#endif
}

static nNetObjectDescriptor< gNetPlayerWall, Game::PlayerWallSync > gNetPlayerWall_init(300);

nNetObjectDescriptorBase const & gNetPlayerWall::DoGetDescriptor() const
{
    return gNetPlayerWall_init;
}

void gNetPlayerWall::PrintName(tString &s) const
{
    s << "gNetPlayerWall nr. " << id;
    if ( this->cycle_ )
    {
        s	<< " owned by ";
        this->cycle_->PrintName( s );
    }
}

gCycleMovement *gNetPlayerWall::CycleMovement() const {
    return cycle_;
}

void gNetPlayerWall::Clear()
{
    //	if( nCLIENT == sn_GetNetState() )
    //	return;

    int i;
    for (i=sg_netPlayerWalls.Len()-1;i>=0;i--){
        // sg_netPlayerWalls(i)->owner=sn_myNetID;
        //delete sg_netPlayerWalls(i);
        gNetPlayerWall* w = sg_netPlayerWalls(i);
        tControlledPTR< nNetObject > bounce( w );
        w->ReleaseData();

        sg_netPlayerWalls.Remove( w, w->id );

        if ( w->edge_ )
            w->edge_->Wall()->Insert();

    }
    for (i=sg_netPlayerWallsGridded.Len()-1;i>=0;i--){
        // sg_netPlayerWallsGridded(i)->owner=sn_myNetID;
        gNetPlayerWall* w = sg_netPlayerWallsGridded(i);
        tControlledPTR< nNetObject > bounce( w );
        w->ReleaseData();

        sg_netPlayerWallsGridded.Remove( w, w->griddedid );
    }
}


void gNetPlayerWall::Check() const
{
#ifdef DEBUG
    int i;
    for ( i = coords_.Len() -2 ; i>=0; --i )
    {
        gPlayerWallCoord* coords = &( coords_( i ) );
        tASSERT( coords[0].Pos <= coords[1].Pos );
        tASSERT( coords[0].Time <= coords[1].Time );
    }

    for ( i = coords_.Len() -1 ; i>=0; --i )
    {
        gPlayerWallCoord* coords = &( coords_( i ) );
        tASSERT( finite( coords[0].Pos ) );
        tASSERT( finite( coords[0].Time ) );
    }
#endif
}

int gNetPlayerWall::IndexPos(REAL d) const
{
    CHECKWALL;


    // get the first coord with smaller alpha than a
    int i = coords_.Len() - 2;
    while ( i >= 1 && coords_(i).Pos >= d)
        --i;

#ifdef DEBUG
    if (!( i >= 0 && i < coords_.Len() - 1 ))
    {
        st_Breakpoint();
    }
#endif

    return i;
}

int gNetPlayerWall::IndexAlpha(REAL a) const
{
    CHECKWALL;

    REAL d = Pos( a );

    return IndexPos( d );
}

REAL gNetPlayerWall::Time(REAL a) const
{
    tASSERT( good( a ) );

    CHECKWALL;

    const gPlayerWallCoord* coord = &coords_(IndexAlpha(a));
    REAL div = ( coord[1].Pos - coord[0].Pos );
    REAL alpha = 0.0f;
    if ( div > 0 )
    {
        alpha = ( Pos(a) - coord[0].Pos ) / div;
    }

    REAL ret = coord[0].Time + alpha*(coord[1].Time-coord[0].Time);

    tASSERT( good( ret ) );

    return ret;
}

REAL gNetPlayerWall::Pos(REAL a) const
{
    CHECKWALL;

    tASSERT( good( a ) );

    REAL ret = BegPos() + a * ( EndPos()  - BegPos() );

    tASSERT( good( ret ) );

    return ret;
}

REAL gNetPlayerWall::Alpha(REAL pos) const
{
    CHECKWALL;

    REAL diff = ( EndPos()  - BegPos() );
    REAL a = pos - BegPos();

    if ( diff > 0 )
        a /= diff;

    tASSERT ( -.001 < a );
    tASSERT ( 1.001 > a );

    return a;
}

bool gNetPlayerWall::IsDangerousAnywhere( REAL time ) const
{
    CHECKWALL;

    if ( !cycle_ )
        return false;

    // is the player dead?
    if ( gCycle::WallsStayUpDelay() >= 0 )
    {
        if ( !cycle_->Alive() && time - cycle_->deathTime > .2f + gCycle::WallsStayUpDelay() )
            return false;
    }

    // is the wall behind the wall end?
    if ( gCycle::WallsLength() > 0 )
    {
        tASSERT( cycle_->MaxWallsLength() >= cycle_->ThisWallsLength() );
        REAL maxDist = cycle_->GetDistance() - cycle_->MaxWallsLength();
        if ( maxDist > EndPos()  && maxDist > BegPos() )
        {
            return false;
        }
    }

    return true;
}

bool gNetPlayerWall::IsDangerousApartFromHoles( REAL a, REAL time ) const
{
    CHECKWALL;

    // test for disappearing after death
    if ( gCycle::WallsStayUpDelay() >= 0.0f )
    {
        // walls disappeear after death
        if (!cycle_ || ( !cycle_->Alive() && cycle_->deathTime + gCycle::WallsStayUpDelay()+0.2f <= time ) )
            return false;
    }

    // the time from the last simulation to the time the query shall be made;
    // cycleDistance is valid at cycle_->lastTime, we need it at time.
    REAL dt = ( time - cycle_->lastTime );

    // the distance value at the spot we hit
    REAL wallDistance = Pos( a );

    // test for finite wall lenght
    if ( gCycle::WallsLength() > 0 )
    {
        // the distance the cycle traveled so far
        REAL cycleDistance = cycle_->GetDistance();

        // extrapolate it, taking rubber slowdown into account
        if ( cycle_->Alive() )
        {
            // cycle movement
            cycleDistance += cycle_->WallEndSpeed() * dt;
        }

        if ( wallDistance + cycle_->ThisWallsLength() < cycleDistance )
            return false;	// hit was after the wall length
    }

    // check whether it is an extrapolated bit
    {
        // the distance the cycle traveled so far
        REAL cycleDistance = cycle_->GetDistance();

        // extrapolate it if the test time lies in the cycle's future.
        if ( cycle_->Alive() && dt > 0 )
        {
            // cycle movement
            cycleDistance += cycle_->Speed() * cycle_->rubberSpeedFactor * dt;
        }

        // is the wall ahead of the cycle?
        if ( wallDistance > cycleDistance )
            return false;
    }

    return true;
}

bool gNetPlayerWall::IsDangerous( REAL a, REAL time ) const
{
    CHECKWALL;

    if ( !IsDangerousApartFromHoles( a, time ) )
    {
        return false;
    }

    const gPlayerWallCoord* coord = &coords_(IndexAlpha(a));
    return coord->IsDangerous;
}

// returns the guy who holed here
gExplosion * gNetPlayerWall::Holer( REAL a, REAL time ) const
{
    CHECKWALL;

    // it does not count as a hole if the wall has expired already
    // for other reasons
    if ( !IsDangerousApartFromHoles( a, time ) )
    {
        return false;
    }

    const gPlayerWallCoord* coord = &coords_(IndexAlpha(a));
    return coord->holer;
}

REAL gNetPlayerWall::EndPos() const
{
    CHECKWALL;

    return coords_(coords_.Len()-1).Pos;
}

REAL gNetPlayerWall::BegPos() const
{
    CHECKWALL;

    return coords_(0).Pos;
}

REAL gNetPlayerWall::EndTime() const
{
    CHECKWALL;

    return coords_(coords_.Len()-1).Time;
}

REAL gNetPlayerWall::BegTime() const
{
    CHECKWALL;

    return coords_(0).Time;
}

void gNetPlayerWall::BlowHole	( REAL beg, REAL end, gExplosion * holer )
{
    CHECKWALL;

#ifndef DEDICATED
    ClearDisplayList(60);
#endif

#ifdef DEBUG
    /*
    for ( int i = 0; i < coords_.Len(); ++i )
    {
        std::cout << "[" << coords_(i).IsDangerous << ',' << coords_(i).Pos << "]";
    }

    static int count=0;
    ++count;

    std::cout << " hole " << count << " : " << beg << ',' << end << '(' << BegPos() << ',' << EndPos() << ")\n";
    */
#endif

    // don't touch anything if the server concluded it is his business
    if ( sn_GetNetState() != nSERVER && sg_ServerSentHoles && !preliminary )
    {
        return;
    }

#ifdef DEBUG
    tASSERT (coords_.Len() < 1000 );
#endif

    // find the last index that will stay before the hole:
    int begind = IndexPos( beg );

    // skip ahead if the holing would create redunant non-dangerous blocks
    while ( begind >= 1 && !coords_[begind].IsDangerous )
    {
        beg = coords_[begind].Pos;
        begind--;
    }

    // find the last index in the hole:
    int endind = IndexPos( end );

    // skip ahead if the holing would create redunant non-dangerous blocks
    while ( endind < coords_.Len() - 2 && !coords_[endind].IsDangerous )
    {
        endind++;
        end = coords_[endind].Pos;
    }

    if ( beg < BegPos() )
    {
        begind = -1;

        beg = BegPos();
    }

    if ( end > EndPos() )
    {
        if ( bool(cycle_) && ( EndPos() < cycle_->GetDistance()-10 || this != cycle_->currentWall ) )
            endind = coords_.Len() - 1;

        end = EndPos();
    }

    // out of range
    if ( end < beg )
    {
        return;
    }

    if ( sn_GetNetState() != nCLIENT )
    {
        this->RequestSync();
    }

    // find the alpha at the hole begin and end:
    REAL begalph = Alpha( beg );
    REAL endalph = Alpha( end );

    // find the time at the hole begin and end:
    REAL begtime = Time( begalph );
    REAL endtime = Time( endalph );

    int insert = begind + 2 - endind;

#ifdef DEBUG
    tASSERT (insert < 40 );
#endif

    // remove positions inside the hole:
    if ( insert < 0 )
    {
        for ( int i = begind+1; i - insert < coords_.Len(); ++i )
            coords_(i) = coords_( i - insert );
        coords_.SetLen( coords_.Len() + insert );
    }

    // make room for the new points of the hole:
    else if ( insert > 0 )
    {
        coords_.SetLen( coords_.Len() + insert );

        for ( int i = coords_.Len() - 1; i >= begind + insert && i >= insert ; --i )
            coords_( i ) = coords_( i - insert );
    }

    // clamp times
    {
        if ( begind >= 0 )
        {
            REAL beforetime = coords_(begind).Time;
            if ( begtime < beforetime )
            {
                begtime = beforetime;
            }
        }

        if ( begind +3 < coords_.Len() )
        {
            REAL afttime = coords_(begind + 3).Time;
            if ( endtime > afttime )
            {
                endtime = afttime;
            }
        }
    }

    // enter the hole
    coords_(begind+1).IsDangerous = false;
    coords_(begind+1).Time        = begtime;
    coords_(begind+1).holer       = holer;
    coords_(begind+1).Pos         = beg;
    coords_(begind+2).Time        = endtime;
    coords_(begind+2).Pos         = end;

#ifdef DEBUG
    /*
    for ( int i = 0; i < coords_.Len(); ++i )
    {
        std::cout << "[" << coords_(i).IsDangerous << ',' << coords_(i).Pos << "]";
    }
    std::cout << "\n";
    */
#endif

    CHECKWALL;
}

static void login_callback(){
    sg_ServerSentHoles = false;
}

static nCallbackLoginLogout sg_LoginLogout(&login_callback);


