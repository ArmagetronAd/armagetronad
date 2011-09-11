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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#include "gCycle.h"
#include "nConfig.h"
#include "rModel.h"
//#include "eTess.h"
#include "eGrid.h"
#include "rTexture.h"
#include "eTimer.h"
#include "tInitExit.h"
#include "tRecorder.h"
#include "rScreen.h"
#include "rFont.h"
#include "gSensor.h"
#include "ePlayer.h"
#include "eSound.h"
#include "eGrid.h"
#include "eFloor.h"
#include "gSparks.h"
#include "gExplosion.h"
#include "gSvgOutput.h"
#include "gWall.h"
#include "nKrawall.h"
#include "gAIBase.h"
#include "eDebugLine.h"
#include "eLagCompensation.h"
#include "gArena.h"
#include "gWinZone.h"

#include "tMath.h"
#include <stdlib.h>
#include <fstream>
#include <memory>
#include <functional>
#include <algorithm>

#ifndef DEDICATED
#define DONTDOIT
#include "rRender.h"
#endif

// TODO: get rid of this
#include "tDirectories.h"

// also used in gWall.cpp
bool sg_gnuplotDebug = false;

#define GNUPLOT_DEBUG
// #define DELAYEDTURN_DEBUG

#ifdef GNUPLOT_DEBUG
static tSettingItem<bool> sg_("DEBUG_GNUPLOT",sg_gnuplotDebug);
#endif

static REAL sg_minDropInterval=0.05;
static tSettingItem< REAL > sg_minDropIntervalConf( "CYCLE_MIN_WALLDROP_INTERVAL", sg_minDropInterval );

#ifdef DEDICATED
static bool sg_predictWalls=true;
static tSettingItem< bool > sg_predictWallsConf( "PREDICT_WALLS", sg_predictWalls );
#endif

//  *****************************************************************

static nNOInitialisator<gCycle> cycle_init(320,"cycle");

//  *****************************************************************

std::vector<gCycle *> gCycle::cycles;

// static nVersionFeature sg_DoubleSpeed( 1 );

//tCONTROLLED_PTR(ePlayerNetID)   lastEnemyInfluence;  	// the last enemy wall we encountered
//REAL							lastTime;				// the time it was drawn at
bool headlights=1;
extern bool cycleprograminited;

static float sg_cycleSyncSmoothTime = .1f;
static tSettingItem<float> conf_smoothTime ("CYCLE_SMOOTH_TIME", sg_cycleSyncSmoothTime);

static float sg_cycleSyncSmoothMinSpeed = .2f;
static tSettingItem<float> conf_smoothMinSpeed ("CYCLE_SMOOTH_MIN_SPEED", sg_cycleSyncSmoothMinSpeed);

static float sg_cycleSyncSmoothThreshold = .2f;
static tSettingItem<float> conf_smoothThreshold ("CYCLE_SMOOTH_THRESHOLD", sg_cycleSyncSmoothThreshold);

static REAL sg_enemyChatbotTimePenalty = 30.0f;   //!< penalty for victim in chatbot mode
static tSettingItem<REAL> sg_enemyChatbotTimePenaltyConf( "ENEMY_CHATBOT_PENALTY", sg_enemyChatbotTimePenalty );
extern REAL sg_suicideTimeout;

static inline void clamp(REAL &c, REAL min, REAL max){
    tASSERT(min < max);

    if (!finite(c))
        c = 0;

    if (c<min)
        c = min;

    if (c>max)
        c = max;
}


REAL		gCycle::wallsStayUpDelay=8.0f;	// the time the cycle walls stay up ( negative values: they stay up forever )

REAL		gCycle::wallsLength=-1.0f;		// the maximum total length of the walls
REAL		gCycle::explosionRadius=4.0f;	// the radius of the holes blewn in by an explosion

static		nSettingItem<REAL> *c_pwsud = NULL, *c_pwl = NULL, *c_per = NULL;

void gCycle::PrivateSettings()
{
    static nSettingItem<REAL> c_wsud("CYCLE_WALLS_STAY_UP_DELAY",wallsStayUpDelay);
    static nSettingItem<REAL> c_wl("CYCLE_WALLS_LENGTH",wallsLength);
    static nSettingItem<REAL> c_er("CYCLE_EXPLOSION_RADIUS",explosionRadius);

    c_pwsud=&c_wsud;
    c_pwl  =&c_wl;
    c_per  =&c_er;
}

// sound speed divisor
static REAL sg_speedCycleSound=15;
static nSettingItem<REAL> c_ss("CYCLE_SOUND_SPEED",
                               sg_speedCycleSound);

// time after spawning it takes the cycle to start building a wall
static REAL sg_cycleWallTime=0.0;
static nSettingItemWatched<REAL>
sg_cycleWallTimeConf("CYCLE_WALL_TIME",
                     sg_cycleWallTime,
                     nConfItemVersionWatcher::Group_Bumpy,
                     12);

// time after spawning during which a cycle can't be killed
static REAL sg_cycleInvulnerableTime=0.0;
static nSettingItemWatched<REAL>
sg_cycleInvulnerableTimeConf("CYCLE_INVULNERABLE_TIME",
                             sg_cycleInvulnerableTime,
                             nConfItemVersionWatcher::Group_Bumpy,
                             12);

// time after spawning during which a cycle can't be killed
static bool sg_cycleFirstSpawnProtection=false;
static nSettingItemWatched<bool>
sg_cycleFirstSpawnProtectionConf("CYCLE_FIRST_SPAWN_PROTECTION",
                                 sg_cycleFirstSpawnProtection,
                                 nConfItemVersionWatcher::Group_Bumpy,
                                 12);

// time intevals between server-client syncs
static REAL sg_syncIntervalEnemy=1;
static tSettingItem<REAL> c_sie( "CYCLE_SYNC_INTERVAL_ENEMY",
                                 sg_syncIntervalEnemy );

static REAL sg_syncIntervalSelf=.1;
static tSettingItem<REAL> c_sis( "CYCLE_SYNC_INTERVAL_SELF",
                                 sg_syncIntervalSelf );

// flag controlling case-by-case-niceness for older clients
static bool sg_avoidBadOldClientSync=true;
static tSettingItem<bool> c_sbs( "CYCLE_AVOID_OLDCLIENT_BAD_SYNC",
                                 sg_avoidBadOldClientSync );

// fast forward factor for extrapolating sync
static REAL sg_syncFF=10;
static tSettingItem<REAL> c_sff( "CYCLE_SYNC_FF",
                                 sg_syncFF );

static int sg_syncFFSteps=1;
static tSettingItem<int> c_sffs( "CYCLE_SYNC_FF_STEPS",
                                 sg_syncFFSteps );

// client side bugfix: local tunneling is avoided on syncs
static nVersionFeature sg_NoLocalTunnelOnSync( 4 );

static REAL sg_GetSyncIntervalSelf( gCycle* cycle )
{
    if ( sg_NoLocalTunnelOnSync.Supported( cycle->Owner() ) )
        return sg_syncIntervalSelf;
    else
        return sg_syncIntervalEnemy * 10;
}

// moviepack hack
//static bool moviepack_hack=false;       // do we use it?
//static tSettingItem<bool> ump("MOVIEPACK_HACK",moviepack_hack);

static int score_hole=0;
static tSettingItem<int> s_h("SCORE_HOLE",score_hole);

static int score_survive=0;
static tSettingItem<int> s_sur("SCORE_SURVIVE",score_survive);

int score_die=-2;
static tSettingItem<int> s_d("SCORE_DIE",score_die);

int score_kill=3;
static tSettingItem<int> s_k("SCORE_KILL",score_kill);

int score_suicide=-4;
static tSettingItem<int> s_s("SCORE_SUICIDE",score_suicide);

// input control

uActionPlayer gCycle::s_brake("CYCLE_BRAKE", -5);
static uActionPlayer s_brakeToggle("CYCLE_BRAKE_TOGGLE", -5);
static uActionTooltip sg_brakeTooltip( gCycle::s_brake, 1, &ePlayer::VetoActiveTooltip );

static eWavData cycle_run("moviesounds/engine.wav","sound/cyclrun.wav");
static eWavData turn_wav("moviesounds/cycturn.wav","sound/expl.wav");
static eWavData scrap("sound/expl.wav");

// a class of textures where the transparent part of the
// image is replaced by the player color
class gTextureCycle: public rSurfaceTexture
{
    gRealColor color_; // player color
    bool wheel; // wheel or body
public:
    gTextureCycle(rSurface const & surface, const gRealColor& color,bool repx=0,bool repy=0,bool wheel=false);

    virtual void ProcessImage(SDL_Surface *im);

    virtual void OnSelect(bool enforce);
};

gTextureCycle::gTextureCycle(rSurface const & surface, const gRealColor& color,bool repx,bool repy,bool w)
        :rSurfaceTexture(rTextureGroups::TEX_OBJ,surface,repx,repy),
        color_(color),wheel(w)
{
    Select();
}

void gTextureCycle::ProcessImage(SDL_Surface *im)
{
#ifndef DEDICATED
    // blend transparent texture parts with cycle color
    tVERIFY(im->format->BytesPerPixel == 4);
    GLubyte R=int(color_.r*255);
    GLubyte G=int(color_.g*255);
    GLubyte B=int(color_.b*255);

    GLubyte *pixels =reinterpret_cast<GLubyte *>(im->pixels);

    for(int i=im->w*im->h-1;i>=0;i--){
        GLubyte alpha=pixels[4*i+3];
        pixels[4*i  ] = (alpha * pixels[4*i  ] + (255-alpha)*R) >> 8;
        pixels[4*i+1] = (alpha * pixels[4*i+1] + (255-alpha)*G) >> 8;
        pixels[4*i+2] = (alpha * pixels[4*i+2] + (255-alpha)*B) >> 8;
        pixels[4*i+3] = 255;
    }
#endif
}

void gTextureCycle::OnSelect(bool enforce){
#ifndef DEDICATED
    rISurfaceTexture::OnSelect(enforce);

    if(rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ]<0){
        REAL R=color_.r,G=color_.g,B=color_.b;
        if(wheel){
            R*=.7;
            G*=.7;
            B*=.7;
        }
        glColor3f(R,G,B);
        GLfloat color[4]={R,G,B,1};

        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,color);
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);
    }
#endif
}

// from gCycleMovement.cpp
extern void sg_RubberValues( ePlayerNetID const * player, REAL speed, REAL & max, REAL & effectiveness );
extern REAL sg_brakeCycle;
extern REAL sg_cycleBrakeDeplete;

// in release mode, default values should always be used. in debug mode, we want to experiment :)
#ifdef DEBUG
#ifndef DEDICATED
#define DEBUGCHATBOT
#endif
#endif

#ifdef DEBUGCHATBOT
typedef tSettingItem<REAL> gChatBotSetting;
typedef tSettingItem<bool> gChatBotSwitch;
#else
typedef nSettingItem<REAL> gChatBotSetting;
typedef nSettingItem<bool> gChatBotSwitch;
#endif

static bool sg_chatBotAlwaysActive = false;
static gChatBotSwitch sg_chatBotAlwaysActiveConf( "CHATBOT_ALWAYS_ACTIVE", sg_chatBotAlwaysActive );

static REAL sg_chatBotNewWallBlindness = .3;
static gChatBotSetting sg_chatBotNewWallBlindnessConf( "CHATBOT_NEW_WALL_BLINDNESS",
        sg_chatBotNewWallBlindness );

static REAL sg_chatBotMinTimestep = .3;
static gChatBotSetting sg_chatBotMinTimestepConf( "CHATBOT_MIN_TIMESTEP",
        sg_chatBotMinTimestep );

static REAL sg_chatBotDelay = .5;
static gChatBotSetting sg_chatBotDelayConf( "CHATBOT_DELAY",
        sg_chatBotDelay );

static REAL sg_chatBotRange = 1;
static gChatBotSetting sg_chatBotRangeConf( "CHATBOT_RANGE",
        sg_chatBotRange );

static REAL sg_chatBotDecay = .02;
static gChatBotSetting sg_chatBotDecayConf( "CHATBOT_DECAY",
        sg_chatBotDecay );

class gCycleChatBot
{
    gCycleChatBot();
public:
class Sensor: public gSensor
    {
    public:
        Sensor(gCycle *o,const eCoord &start,const eCoord &d)
                : gSensor(o,start,d)
                , hitOwner_( 0 )
                , hitTime_ ( 0 )
                , hitDistance_( o->MaxWallsLength() )
                , lrSuggestion_( 0 )
                , windingNumber_( 0 )
        {
            if ( hitDistance_ <= 0 )
                hitDistance_ = o->GetDistance();
        }

        /*
        // do detection and additional stuff
        void detect( REAL range )
        {
            gSensor::detect( range );
        }
        */

        virtual void PassEdge(const eWall *ww,REAL time,REAL a,int r)
        {
            try{
                gSensor::PassEdge(ww,time,a,r);
            }
            catch( eSensorFinished & e )
            {
                if ( DoExtraDetectionStuff() )
                    throw;
            }
        }

        bool DoExtraDetectionStuff()
        {
            // move towards the beginning of a wall
            lrSuggestion_ = -lr;

            switch ( type )
            {
            case gSENSOR_NONE:
            case gSENSOR_RIM:
                lrSuggestion_ = 0;
                return true;
            default:
                // unless it is an enemy, follow his wall instead (uncomment for a nasty cowardy campbot)
                // lrSuggestion *= -1;
            case gSENSOR_SELF:
                {
                    // determine whether we're hitting the front or back half of his wall
                    if ( !ehit )
                        return true;
                    eWall * wall = ehit->GetWall();
                    if ( !wall )
                        return true;
                    gPlayerWall * playerWall = dynamic_cast< gPlayerWall * >( wall );
                    if ( !playerWall )
                        return true;
                    hitOwner_ = playerWall->Cycle();
                    if ( !hitOwner_ )
                        return true;

                    // gCycleChatBot & enemyChatBot = Get( hitOwner_ );

                    REAL wallAlpha = playerWall->Edge()->Ratio( before_hit );
                    // that's an unreliable source
                    if ( wallAlpha < 0 )
                        wallAlpha = 0;
                    if ( wallAlpha > 1 )
                        wallAlpha = 1;
                    hitDistance_   = hitOwner_->GetDistance() - playerWall->Pos( wallAlpha );
                    hitTime_       = playerWall->Time( wallAlpha );
                    windingNumber_ = playerWall->WindingNumber();

                    // don't see new walls
                    if ( hitTime_ > hitOwner_->LastTime() - sg_chatBotNewWallBlindness && hitOwner_ != owned )
                    {
                        ehit = NULL;
                        hit = 1E+40;
                        return false;
                    }

                    // REAL cycleDistance = hitOwner_->GetDistance();

                    // REAL wallStart = 0;

                    /*
                    if ( gCycle::WallsLength() > 0 )
                    {
                        wallStart = cyclePos - playerWall->Cycle()->ThisWallsLength();
                        if ( wallStart < 0 )
                            wallStart = 0;
                    }
                    */
                }
            }

            return true;
        }

        // check how far the hit wall extends straight into the given direction
        REAL HitWallExtends( eCoord const & dir, eCoord const & origin )
        {
            if ( !ehit || !ehit->Other() )
            {
                return 1E+30;
            }

            REAL ret = -1E+30;
            eCoord ends[2] = { *ehit->Point(), *ehit->Other()->Point() };
            for ( int i = 1; i>=0; --i )
            {
                REAL newRet = eCoord::F( dir, ends[i]-origin );
                if ( newRet > ret )
                    ret = newRet;
            }

            return ret;
        }

        gCycle * hitOwner_;     // the owner of the hit wall
        REAL     hitTime_;      // the time the hit wall was built at
        REAL     hitDistance_;  // the distance of the wall to the cycle that built it
        short    lrSuggestion_; // sensor's oppinon on whether moving to the left or right of the hit wall is recommended (-1 for left, +1 for right)
        int      windingNumber_; // the number of turns (with sign) the cycle has taken
    };

    gCycleChatBot( gCycle * owner )
            : nextChatAI_( 0 )
            , timeOnChatAI_( 0 )
            , lastTurn_( 0 )
            , nextTurn_ ( 0 )
            , turnedRecently_ ( 0 )
            , owner_ ( owner )
    {
#ifdef RLBOT
        rlDir = 1;
        rlLastTime = -100;
#endif
    }

    // describes walls we like. We like enemy walls. We like to go between them.
    class WallHug
    {
    public:
        gCycle const * owner_;  // the cycle the walls we like belong to
        REAL lastTimeSeen_;    // the last time we saw such a wall

        WallHug()
                : owner_ ( NULL )
                , lastTimeSeen_ ( 0 )
        {
        }
    };

    // promote seen walls to possible wallhug replacements
    void FindHugReplacement( Sensor const & sensor )
    {
        gCycle const * owner = sensor.hitOwner_;
        if (!owner)
            return;

        // store as possible replacement
        if ( !hugReplacement_.owner_ && sensor.type != gSENSOR_SELF &&
                owner != hugLeft_.owner_ &&
                owner != hugRight_.owner_ )
        {
            hugReplacement_.owner_ = sensor.hitOwner_;
            hugReplacement_.lastTimeSeen_ = nextChatAI_;
        }

        // update timestamps
        if ( owner == hugLeft_.owner_ )
            hugLeft_.lastTimeSeen_ = nextChatAI_;
        if ( owner == hugRight_.owner_ )
            hugRight_.lastTimeSeen_ = nextChatAI_;
    }

    // determines the distance between two sensors; the size should give the likelyhood
    // to survive if you pass through a gap between the two selected walls
    REAL Distance( Sensor const & a, Sensor const & b )
    {
        // make sure a is left from b
        if ( a.Direction() * b.Direction() < 0 )
            return Distance( b, a );

        bool self = a.type == gSENSOR_SELF || b.type == gSENSOR_SELF;
        bool rim  = a.type == gSENSOR_RIM || b.type == gSENSOR_RIM;

        // avoid. own. walls.
        REAL selfHatred = 1;
        if ( a.type == gSENSOR_SELF )
        {
            selfHatred *= .5;
            if ( a.lr > 0 )
            {
                selfHatred *= .5;
                if ( b.type == gSENSOR_RIM )
                    selfHatred *= .25;
            }
        }
        if ( b.type == gSENSOR_SELF )
        {
            selfHatred *= .5;
            if ( b.lr < 0 )
            {
                selfHatred *= .5;
                if ( a.type == gSENSOR_RIM )
                    selfHatred *= .25;
            }
        }

        // some big distance to return if we don't know anything better
        REAL bigDistance = owner_->MaxWallsLength();
        if ( bigDistance <= 0 )
            bigDistance = owner_->GetDistance();

        if ( a.hitOwner_ != b.hitOwner_ )
        {
            // different owners? Great, there has to be a way through!
            REAL ret =
                a.hitDistance_ + b.hitDistance_;

            if ( rim )
            {
                ret = bigDistance * .001 + ret * .01 + ( a.before_hit - b.before_hit).Norm();

                // we love going between the rim and enemies
                if ( !self )
                    ret = bigDistance * 2;
            }

            // minimal factor should be 1, this path should never return something smaller than the
            // paths where only one cycle's walls are hit
            ret *= 16;

            // or empty space
            if ( a.type == gSENSOR_NONE || b.type == gSENSOR_NONE )
                ret *= 2;

            return ret * selfHatred;
        }
        else if ( rim )
        {
            // at least one rim wall? Take the distance between the hit positions.
            return ( a.before_hit - b.before_hit).Norm() * selfHatred;
        }
        else if ( a.type == gSENSOR_NONE && b.type == gSENSOR_NONE )
        {
            // empty space! Woo!
            return owner_->GetDistance() * 256;
        }
        else if ( a.lr != b.lr )
        {
            // different directions? Also great!
            return ( fabsf( a.hitDistance_ - b.hitDistance_ ) + .25 * bigDistance ) * selfHatred;
        }
        /*
        else if ( - 2 * a.lr * (a.windingNumber_ - b.windingNumber_ ) > owner_->Grid()->WindingNumber() )
        {
            // this looks like a way out to me
            return fabsf( a.hitDistance_ - b.hitDistance_ ) * 10 * selfHatred;
        }
        */
        else
        {
            // well, the longer the wall segment between the two points, the better.
            return fabsf( a.hitDistance_ - b.hitDistance_ ) * selfHatred;
        }

        // default: hit distance
        return ( a.before_hit - b.before_hit).Norm() * selfHatred;
    }

    static gCycleChatBot & Get( gCycle * cycle )
    {
        tASSERT( cycle );

        // create
        if ( &(*cycle->chatBot_) == 0 )
            cycle->chatBot_ = std::auto_ptr< gCycleChatBot >( new gCycleChatBot( cycle ) );

        return *cycle->chatBot_;
    }

    bool CanMakeTurn( uActionPlayer * action )
    {
        return owner_->CanMakeTurn( ( action == &gCycle::se_turnRight ) ? 1 : -1 );
    }

#ifdef RLBOT
    int rlDir;
    REAL rlLastTime;
#endif

    // does the main thinking
    void Activate( REAL currentTime )
    {
#ifdef RLBOT
        // hack chatbot for crazy turning
        {
            if (!owner_->Alive() || !owner_->Vulnerable() )
            {
                return;
            }
            if( fabs( rlLastTime - currentTime) > 1 )
            {
                owner_->Act( &gCycle::se_turnRight, 1 );
                rlDir = -1;
            }
            else  if ( rlDir > 0 )
            {
                if( CanMakeTurn( &gCycle::se_turnRight ) )
                {
                    owner_->Act( &gCycle::se_turnRight, 1 );
                    owner_->Act( &gCycle::se_turnRight, 1 );
                    owner_->Act( &gCycle::se_turnRight, 1 );
                    rlDir = -1;
                }
            }
            else
            {
                if( CanMakeTurn( &gCycle::se_turnLeft ) )
                {
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                    rlDir = 1;
                }
            }
            rlLastTime = currentTime;
            return;
        }
#endif

        // is it already time for activation?
        if ( currentTime < nextChatAI_ )
            return;

        REAL lookahead = sg_chatBotRange;  // seconds to plan ahead
        REAL minstep   = sg_chatBotMinTimestep; // minimum timestep between thoughts in seconds
        REAL maxstep   = sg_chatBotMinTimestep * 4 * ( 1 + .1 * tReproducibleRandomizer::GetInstance().Get() );  // maximum timestep between thoughts in seconds

        // chat AI wasn't active yet, so don't start immediately
        if ( nextChatAI_ <= EPS )
        {
            nextChatAI_ = sg_chatBotDelay + currentTime;
            return;
        }

        timeOnChatAI_ += currentTime - nextChatAI_;

        // cylce data
        REAL speed = owner_->Speed();
        eCoord dir = owner_->Direction();
        eCoord pos = owner_->Position();

        // make chat AI worse over time
        if ( sn_GetNetState() != nSTANDALONE )
        {
            REAL qualityFactor = ( timeOnChatAI_ * sg_chatBotDecay );
            if ( qualityFactor > 1 )
            {
                minstep *= qualityFactor;
                // maxstep *= qualityFactor;
            }
        }

        REAL range= speed * lookahead;
        eCoord scanDir = dir * range;

        REAL frontFactor = .5;

        Sensor front(owner_,pos,scanDir);
        front.detect(frontFactor);
        owner_->enemyInfluence.AddSensor( front, sg_enemyChatbotTimePenalty, owner_ );

        REAL minMoveOn = 0, maxMoveOn = 0, moveOn = 0;

        // get extra time we get through rubber usage
        REAL rubberGranted, rubberEffectiveness;
        sg_RubberValues( owner_->player, speed, rubberGranted, rubberEffectiveness );
        REAL rubberTime = ( rubberGranted - owner_->GetRubber() )*rubberEffectiveness/speed;
        REAL rubberRatio = owner_->GetRubber()/rubberGranted;

        if ( front.ehit )
        {
            turnedRecently_ = false;

            // these checks can hit our last wall and fail. Temporarily set it to NULL.
            tJUST_CONTROLLED_PTR< gNetPlayerWall > lastWall = owner_->lastWall;
            owner_->lastWall = NULL;

            REAL narrowFront = 1;

            // cast four diagonal rays
            Sensor forwardLeft ( owner_, pos, scanDir.Turn(+1,+1 ) );
            Sensor backwardLeft( owner_, pos, scanDir.Turn(-1,+narrowFront) );
            forwardLeft.detect(1);
            backwardLeft.detect(1);
            Sensor forwardRight ( owner_, pos, scanDir.Turn(+1,-1 ) );
            Sensor backwardRight( owner_, pos, scanDir.Turn(-1,-narrowFront) );
            forwardRight.detect(1);
            backwardRight.detect(1);

            // do we have a hug replacement candiate? If so, take it.
            if ( hugReplacement_.owner_ && !hugLeft_.owner_ && !hugRight_.owner_ )
            {
                // first time hugging? let the status quo decide.
                int lr = 0;
                if ( backwardLeft.hitOwner_ == hugReplacement_.owner_ )
                    lr--;
                if ( forwardLeft.hitOwner_ == hugReplacement_.owner_ )
                    lr--;
                if ( backwardRight.hitOwner_ == hugReplacement_.owner_ )
                    lr++;
                if ( forwardRight.hitOwner_ == hugReplacement_.owner_ )
                    lr++;

                if ( lr > 0 )
                    hugRight_ = hugReplacement_;
                if ( lr < 0 )
                    hugLeft_ = hugReplacement_;

                hugReplacement_.owner_ = 0;
            }

            if ( hugReplacement_.owner_ )
            {
                if( hugLeft_.lastTimeSeen_ < hugRight_.lastTimeSeen_ )
                {
                    if ( hugReplacement_.lastTimeSeen_ > hugLeft_.lastTimeSeen_ )
                        hugLeft_ = hugReplacement_;
                }
                else
                {
                    if ( hugReplacement_.lastTimeSeen_ > hugRight_.lastTimeSeen_ )
                        hugRight_ = hugReplacement_;
                }
                hugReplacement_.owner_ = 0;
            }

            FindHugReplacement( front );
            FindHugReplacement( forwardLeft );
            FindHugReplacement( forwardRight );
            FindHugReplacement( backwardLeft );
            FindHugReplacement( backwardRight );

            // determine survival chances in the four directions
            REAL frontOpen = Distance ( forwardLeft, forwardRight );
            REAL leftOpen  = Distance ( forwardLeft, backwardLeft );
            REAL rightOpen = Distance ( forwardRight, backwardRight );
            REAL rearOpen = Distance ( backwardLeft, backwardRight );

            Sensor self( owner_, pos, scanDir.Turn(-1, 0) );
            // fake entries
            self.before_hit = pos;
            self.windingNumber_ = owner_->windingNumber_;
            self.type = gSENSOR_SELF;
            self.hitDistance_ = 0;
            self.hitOwner_ = owner_;
            self.hitTime_ = currentTime;
            self.lr = -1;
            REAL rearLeftOpen = Distance( backwardLeft, self );
            self.lr = 1;
            REAL rearRightOpen = Distance( backwardRight, self );

            /*
            // override: don't camp (too much)
            if ( forwardRight.type == gSENSOR_SELF &&
                    forwardLeft.type == gSENSOR_SELF &&
                    backwardRight.type == gSENSOR_SELF &&
                    backwardLeft.type == gSENSOR_SELF &&
                    front.type == gSENSOR_SELF &&
                    forwardRight.lr == front.lr &&
                    forwardLeft.lr == front.lr &&
                    backwardRight.lr == front.lr &&
                    backwardLeft.lr == front.lr &&
                    frontOpen + leftOpen + rightOpen < owner_->GetDistance() * .5 )
            {
                turnedRecently_ = true;
                if ( front.lr > 0 )
                {
                    if ( leftOpen > minstep * speed )
                        // force a turn to the left
                        rightOpen = 0;
                    else if ( front.hit * range < 2 * minstep )
                        // force a preliminary turn to the right that will allow us to reverse
                        frontOpen = 0;
                }
                else
                {
                    if ( rightOpen > minstep * speed )
                        // force a turn to the right
                        leftOpen = 0;
                    else if ( front.hit * range < 2 * minstep )
                        // force a preliminary turn to the left that will allow us to reverse
                        frontOpen = 0;
                }
            }
            */

            // override rim hugging
            if ( forwardRight.type == gSENSOR_SELF &&
                    forwardLeft.type == gSENSOR_RIM &&
                    backwardRight.type == gSENSOR_SELF &&
                    backwardLeft.type == gSENSOR_RIM &&
                    // backwardLeft.hit < .1 &&
                    forwardRight.lr == -1 &&
                    backwardRight.lr == -1 )
            {
                turnedRecently_ = true;
                if ( rightOpen > speed * ( owner_->GetTurnDelay() - rubberTime * .8 ) )
                {
                    owner_->Act( &gCycle::se_turnRight, 1 );
                    owner_->Act( &gCycle::se_turnRight, 1 );
                }
                else
                {
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                }
            }

            if ( forwardLeft.type == gSENSOR_SELF &&
                    forwardRight.type == gSENSOR_RIM &&
                    backwardLeft.type == gSENSOR_SELF &&
                    backwardRight.type == gSENSOR_RIM &&
                    // backwardRight.hit < .1 &&
                    forwardLeft.lr == 1 &&
                    backwardLeft.lr == 1 )
            {
                turnedRecently_ = true;
                if ( leftOpen > speed * ( owner_->GetTurnDelay() - rubberTime * .8 ) )
                {
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                    owner_->Act( &gCycle::se_turnLeft, 1 );
                }
                else
                {
                    owner_->Act( &gCycle::se_turnRight, 1 );
                    owner_->Act( &gCycle::se_turnRight, 1 );
                }
            }

            // get the best turn direction
            uActionPlayer * bestAction = ( leftOpen > rightOpen ) ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
            int             bestDir      = ( leftOpen > rightOpen ) ? 1 : -1;
            REAL            bestOpen     = ( leftOpen > rightOpen ) ? leftOpen : rightOpen;
            Sensor &        bestForward  = ( leftOpen > rightOpen ) ? forwardLeft : forwardRight;
            Sensor &        bestBackward = ( leftOpen > rightOpen ) ? backwardLeft : backwardRight;

            Sensor direct ( owner_, pos, scanDir.Turn( 0, bestDir) );
            direct.detect( 1 );

            // restore last wall
            owner_->lastWall = lastWall;

            // only turn if the hole has a shape that allows better entry after we do a zig-zag, or if we're past the good turning point
            // see how the survival chance is distributed between forward and backward half
            REAL forwardHalf  = Distance ( direct, bestForward );
            REAL backwardHalf = Distance ( direct, bestBackward );

            REAL forwardOverhang  = bestForward.HitWallExtends( bestForward.Direction(), pos );
            REAL backwardOverhang  = bestBackward.HitWallExtends( bestForward.Direction(), pos );

            // we have to move forward this much before we can hope to turn
            minMoveOn = bestBackward.HitWallExtends( dir, pos );

            // maybe the direct to the side sensor is better?
            REAL minMoveOnOther = direct.HitWallExtends( dir, pos );

            // determine how far we can drive on
            maxMoveOn      = bestForward.HitWallExtends( dir, pos );
            REAL maxMoveOnOther = front.HitWallExtends( dir, pos );
            if ( maxMoveOn > maxMoveOnOther )
                maxMoveOn = maxMoveOnOther;

            if ( maxMoveOn > minMoveOnOther && forwardHalf > backwardHalf && direct.hitOwner_ == bestBackward.hitOwner_ )
            {
                backwardOverhang  = direct.HitWallExtends( bestForward.Direction(), pos );
                minMoveOn = minMoveOnOther;
            }

            // best place to turn
            moveOn = .5 * ( minMoveOn * ( 1 + rubberRatio ) + maxMoveOn * ( 1 - rubberRatio ) );

            // hit the brakes before you hit anything and if it's worth it
            bool brake = sg_brakeCycle > 0 &&
                         front.hit * lookahead * sg_cycleBrakeDeplete < owner_->GetBrakingReservoir() &&
                         sg_brakeCycle * front.hit * lookahead < 2 * speed * owner_->GetBrakingReservoir() &&
                         ( maxMoveOn - minMoveOn ) > 0 &&
                         owner_->GetBrakingReservoir() * ( maxMoveOn - minMoveOn ) < speed * owner_->GetTurnDelay();
            if ( frontOpen < bestOpen &&
                    ( forwardOverhang <= backwardOverhang || ( minMoveOn < 0 && moveOn < minstep * speed ) ) )
            {
                // FindHugReplacement( direct );
                // REAL expectedBackwardHalf = ( direct.before_hit - bestBackward.before_hit ).Norm();

                // if ( ( ( forwardHalf + backwardHalf > bestOpen * 2 || backwardHalf > frontOpen * 10 || backwardHalf > expectedBackwardHalf * 1.01 ) && frontOpen < bestOpen ) ||
                // rubberTime * .5 + minspace * lookahead < minstep )
                //                {
                turnedRecently_ = true;

                minMoveOn = maxMoveOn = moveOn = 0;

                /*
                if (
                    ( ( ( bestBackward.type == gSENSOR_ENEMY || bestBackward.type == gSENSOR_TEAMMATE ) && bestBackward.hitDistance_ < bestBackward.hit * lookahead * speed ) ||
                      direct.hit * lookahead + rubberTime < owner_->GetTurnDelay() ) &&
                    ( bestBackward.hit * lookahead + rubberTime < owner_->GetTurnDelay() ||
                      bestForward.hit * lookahead + rubberTime < owner_->GetTurnDelay() )
                )
                {
                    // override: stupid turn into certain death, turn it around if that makes it less stupid
                    uActionPlayer * newBestAction = ( leftOpen > rightOpen ) ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
                    Sensor newDirect ( owner_, pos, scanDir.Turn( 0, -bestDir) );
                    newDirect.detect( 1 );
                    if ( newDirect.hit > direct.hit ||
                            newDirect.hit * lookahead + rubberTime > owner_->GetTurnDelay() )
                        owner_->Act( newBestAction, 1 );
                }
                else
                */
                {
                    if ( !CanMakeTurn( bestAction ) )
                    {
                        nextChatAI_ = currentTime;
                        return;
                    }

                    owner_->Act( bestAction, 1 );
                }

                brake = false;
            }
            else
            {
                // the best
                REAL bestSoFar = frontOpen > bestOpen ? frontOpen : bestOpen;
                bestSoFar *= ( 10 * ( 1 - rubberRatio ) + 1 );

                if ( rearOpen > bestSoFar && ( rearLeftOpen > bestSoFar || rearRightOpen > bestSoFar ) )
                {
                    brake = false;
                    turnedRecently_ = true;

                    bool goLeft = rearLeftOpen > rearRightOpen;

                    // dead end. reverse into the opposite direction of the front wall
                    uActionPlayer * bestAction = goLeft ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
                    uActionPlayer * otherAction = !goLeft ? &gCycle::se_turnLeft : &gCycle::se_turnRight;
                    Sensor &        bestForward  = goLeft ? forwardLeft : forwardRight;
                    Sensor &        bestBackward  = goLeft ? backwardLeft : backwardRight;
                    Sensor &        otherForward  = !goLeft ? forwardLeft : forwardRight;
                    Sensor &        otherBackward  = !goLeft ? backwardLeft : backwardRight;

                    // space in the two directions available for turns
                    REAL bestHit = bestForward.hit > bestBackward.hit ? bestBackward.hit : bestForward.hit;
                    REAL otherHit = otherForward.hit > otherBackward.hit ? otherBackward.hit : otherForward.hit;

                    bool wait = false;

                    if ( !CanMakeTurn( bestAction ) )
                    {
                        nextChatAI_ = currentTime;
                        return;
                    }

                    // well, after a short turn to the right if space is tight
                    if ( bestHit * lookahead < owner_->GetTurnDelay() + rubberTime )
                    {
                        if ( otherHit < bestForward.hit * 2 && front.hit * lookahead > owner_->GetTurnDelay() * 2 )
                        {
                            // wait a bit, perhaps there will be a better spot
                            wait = true;
                        }
                        else
                        {
                            if ( !CanMakeTurn( otherAction ) )
                            {
                                nextChatAI_ = currentTime;
                                return;
                            }

                            owner_->Act( otherAction, 1 );

                            // there needs to be space ahead to finish the maneuver correctly
                            if ( maxMoveOn < speed * owner_->GetTurnDelay() )
                            {
                                // there isn't. oh well, turn into the wrong direction completely, see if I care
                                owner_->Act( otherAction, 1 );
                                wait = true;
                            }
                        }
                    }

                    if ( !wait )
                    {
                        owner_->Act( bestAction, 1 );
                        owner_->Act( bestAction, 1 );
                    }

                    minMoveOn = maxMoveOn = moveOn = 0;
                }
            }

            // execute brake command
            owner_->Act( &gCycle::s_brake, brake ? 1 : -1 );

            // swap hugged walls if we're in fact grinding them the other way round
            if ( hugLeft_.owner_ == backwardRight.hitOwner_ ||
                    hugRight_.owner_ == backwardLeft.hitOwner_ )
            {
                WallHug swap = hugRight_;
                hugRight_ = hugLeft_;
                hugLeft_ = swap;
            }
        }

        // REAL mintime = minspace * lookahead;

        // try again soon
        //        REAL newmintime = mintime * .5 - minstep * .2 * tReproducibleRandomizer::GetInstance().Get();

        // clamp
        // if ( newmintime < minstep )
        // newmintime = minstep;

        // add slack, acceleration and rubber
        // if ( owner_->acceleration > 0 )
        // mintime -= owner_->acceleration * mintime * mintime / speed;
        // mintime -= .1 * minstep - rubberTime * .3;

        // if the next step gets us too close to the wall to do anything useful,
        // bring us really close right away.
        // if ( mintime - newmintime > minstep )
        // {
        // mintime = newmintime;
        // }

        REAL space = moveOn;
        REAL minTime = space/speed;

        if ( turnedRecently_ )
            minTime = owner_->GetTurnDelay();

        if ( minTime < minstep )
            minTime = minstep;
        if ( minTime > maxstep + minstep * 1.5 )
        {
            minTime = maxstep;
        }
        // minTime = 0;

        nextChatAI_ = currentTime + minTime;
        timeOnChatAI_ += minTime;
    }

    REAL nextChatAI_;        //!< the next time the chat AI can be active
private:
    REAL timeOnChatAI_;      //!< the total time the player was on chat AI this round
    short lastTurn_;         //!< the last turn the chat AI made
    REAL nextTurn_;          //!< the next turn if one is planned
    bool turnedRecently_;    //!< whether the cycle was turned or almost turned recently
    gCycle * owner_;         //!< owner of chatbot

    WallHug hugLeft_;              //!< the wall we like to have on our left side
    WallHug hugRight_;             //!< the wall we like to have on our right side
    WallHug hugReplacement_;       //!< a possible replacement candidate for one of the hugged walls
};

//  *****************************************************************

static void sg_ArchiveCoord( eCoord & coord, int level )
{
    static char const * section = "_COORD";
    tRecorderSync< eCoord >::Archive( section, level, coord );
}

static void sg_ArchiveReal( REAL & real, int level )
{
    static char const * section = "_REAL";
    tRecorderSync< REAL >::Archive( section, level, real );
}

//  *****************************************************************



//  *****************************************************************


// take pos,dir and time from a cycle
gDestination::gDestination(const gCycleMovement &c)
        :chatting(false)
        ,turns(0)
        ,hasBeenUsed(false)
        ,messageID(1)
        ,missable(true)
        ,next(NULL)
        ,list(NULL)
{
    CopyFrom( c );
}

gDestination::gDestination(const gCycle &c)
        :chatting(false)
        ,turns(0)
        ,hasBeenUsed(false)
        ,messageID(1)
        ,missable(true)
        ,next(NULL)
        ,list(NULL)
{
    CopyFrom( c );
}

// or from a message
gDestination::gDestination(nMessage &m, unsigned short & cycle_id )
        :gameTime(0),distance(0),speed(0),
        hasBeenUsed(false),
        messageID(1),
        missable(true),
next(NULL),list(NULL){
    m >> position;
    m >> direction;
    m >> distance;

    unsigned short flags;
    m >> flags;
    braking  = flags & 0x01;
    chatting = flags & 0x02;

    messageID = m.MessageID();

    turns = 0;

    m.Read( cycle_id );

    if ( !m.End() )
        m >> gameTime;
    else
        gameTime = -1000;

    if ( !m.End() )
    {
        m.Read( turns );
    }
}

void gDestination::CopyFrom(const gCycleMovement &other)
{
    position 	= other.Position();
    direction 	= other.Direction();
    gameTime 	= other.LastTime();
    distance 	= other.GetDistance();
    speed 		= other.Speed();
    braking 	= other.GetBraking();
    turns 		= other.GetTurns();

#ifdef DEBUG
    if (!finite(gameTime) || !finite(speed) || !finite(distance))
        st_Breakpoint();
#endif
    if ( other.Owner() && other.Player() )
        chatting = other.Player()->IsChatting();

    // cheat. If rubber ran out, backdate the time.
    if ( other.RubberDepleteTime() > 0 )
    {
        gameTime = other.RubberDepleteTime();
    }
}

void gDestination::CopyFrom(const gCycle &other)
{
    CopyFrom( static_cast<const gCycleMovement&>(other) );

    // correct distance
    distance 	+= other.correctDistanceSmooth;
}

// *******************************************************************************************
// *
// *	CompareWith
// *
// *******************************************************************************************
//!
//!		@param	other	the destination to compare with
//!		@return			-1 if this destination came earler, +1 if other was earler, 0 if no difference can be found
//!
// *******************************************************************************************

int gDestination::CompareWith( const gDestination & other ) const
{
    // compare distances, but use the network message ID ( they are always increasing, the distance may stagnate or in extreme cases run backwards ) as main sort criterion

    // compare message IDs with overflow ( if both are at good values )
    if ( messageID > 1 && other.messageID > 1 )
    {
        short messageIDDifference = messageID - other.messageID;
        if ( messageIDDifference < 0 )
            return -1;
        if ( messageIDDifference > 0 )
            return 1;
    }

    // compare travelled distance
    if ( distance < other.distance )
        return -1;
    else if ( distance > other.distance )
        return 1;

    // no relevant difference found
    return 0;
}

class gFloatCompressor
{
public:
    gFloatCompressor( REAL min, REAL max )
            : min_( min ), max_( max ){}

    // write compressed float to message
    void Write( nMessage& m, REAL value ) const
    {
        clamp( value, min_, max_ );
        unsigned short compressed = static_cast< unsigned short > ( maxShort_ * ( value - min_ )/( max_ - min_ ) );
        m.Write( compressed );
    }

    REAL Read( nMessage& m ) const
    {
        unsigned short compressed;
        m.Read( compressed );

        return  min_ + compressed * ( max_ - min_ )/maxShort_;
    }
private:
    REAL min_, max_;  // minimal and maximal expected value

    static const unsigned short maxShort_;

    gFloatCompressor();
};

const unsigned short gFloatCompressor::maxShort_ = 0xFFFF;

static gFloatCompressor compressZeroOne( 0, 1 );

// write all the data into a nMessage
void gDestination::WriteCreate(nMessage &m, unsigned short cycle_id ){
    m << position;
    m << direction;
    m << distance;

    unsigned short flags = 0;
    if ( braking )
        flags |= 0x01;
    if ( chatting )
        flags |= 0x02;
    m << flags;

    // store message ID for later reference
    messageID = m.MessageID();

    m.Write( cycle_id );
    m << gameTime;
    m.Write( turns );
}

gDestination *gDestination::RightBefore(gDestination *list, REAL dist){
    if (!list || list->distance > dist)
        return NULL;

    gDestination *ret=list;
    while (ret && ret->next && ret->next->distance < dist)
        ret=ret->next;

    return ret;
}

gDestination *gDestination::RightAfter(gDestination *list, REAL dist){
    if (!list)
        return NULL;

    gDestination *ret=list;
    while (ret && ret->distance < dist)
        ret=ret->next;

    return ret;
}

// insert yourself into a list ordered by gameTime
void gDestination::InsertIntoList(gDestination **l){
    if (!l)
        return;

    RemoveFromList();

    // let message find its place
    while (l && *l && CompareWith( **l ) > 0 )
        l = &((*l)->next);

    list=l;

    tASSERT(l);

    next=*l;
    *l=this;
}



void gDestination::RemoveFromList(){
    /*
       if (!list)
           return;

       while (list && *list && *list != this)
           list = &((*list)->next);

       tASSERT(list);
       tASSERT(*list);

       (*list) = next;
       next=NULL;
       list=NULL;

      */

    // z-man: HUH? I don't understand the code above any more and it seems to be leaky.
    // This simple alternative sounds better:

    if(list)
        *list = next;
    if(next)
        next->list = list;

    next = 0;
    list = 0;
}

// *******************************************************************************************
// *
// *	GetGameTime
// *
// *******************************************************************************************
//!
//!		@return		game time of the command
//!
// *******************************************************************************************

REAL gDestination::GetGameTime( void ) const
{
    return this->gameTime;
}

// *******************************************************************************************
// *
// *	GetGameTime
// *
// *******************************************************************************************
//!
//!		@param	gameTime	game time of the command to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gDestination const & gDestination::GetGameTime( REAL & gameTime ) const
{
    gameTime = this->gameTime;
    return *this;
}

// *******************************************************************************************
// *
// *	SetGameTime
// *
// *******************************************************************************************
//!
//!		@param	gameTime	game time of the command to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

gDestination & gDestination::SetGameTime( REAL gameTime )
{
    this->gameTime = gameTime;
    return *this;
}

// ********************************************************

static void new_destination_handler(nMessage &m)
{
    // read the destination
    unsigned short cycle_id;
    gDestination *dest=new gDestination(m, cycle_id );

    // and the ID of the cycle the destination is added to
    nNetObject *o=nNetObject::ObjectDangerous(cycle_id);
    if (o && &o->CreatorDescriptor() == &cycle_init){
        if ((sn_GetNetState() == nSERVER) && (m.SenderID() != o->Owner()))
        {
            Cheater(m.SenderID());
        }
        else
            if(o->Owner() != ::sn_myNetID)
            {
                gCycle* c = dynamic_cast<gCycle *>(o);
                if (c)
                {
                    if ( c->Player() && !dest->Chatting() )
                        c->Player()->Activity();

                    // fill default gametime
                    if ( dest->GetGameTime() < -100 )
                        dest->SetGameTime( se_GameTime()+c->Lag()*3 );

                    c->AddDestination(dest);
                    dest = 0;
                }
            }
    }

    // delete the destination if it has not been used
    // TODO: exception safety!
    delete dest;
}

static nDescriptor destination_descriptor(321,&new_destination_handler,"destinaton");

static void BroadCastNewDestination(gCycleMovement *c, gDestination *dest){
    nMessage *m=new nMessage(destination_descriptor);
    dest->WriteCreate(*m, c->ID() );
    m->BroadCast();
}

bool gCycle::IsMe( eGameObject const * other ) const
{
    return other == this || other == extrapolator_;
}

#ifdef DELAYEDTURN_DEBUG
double sg_turnReceivedTime = 0;
#endif

void gCycle::OnNotifyNewDestination( gDestination* dest )
{
#ifdef DELAYEDTURN_DEBUG
    sg_turnReceivedTime = tSysTimeFloat();
#endif

#ifdef GNUPLOT_DEBUG
    if ( sg_gnuplotDebug && Player() )
    {
        std::ofstream f( Player()->GetUserName() + "_sync", std::ios::app );
        f << dest->position.x << " " << dest->position.y << "\n";
    }
#endif

    gCycleMovement::OnNotifyNewDestination( dest );

    //  if (sn_GetNetState()==nSERVER || ::sn_myNetID == owner)
    if (sn_GetNetState()==nCLIENT && ::sn_myNetID == Owner())
        BroadCastNewDestination(this,dest);

    if ( extrapolator_ )
    {
        // add destination and try to squeeze it into the schedule.
        extrapolator_->NotifyNewDestination( dest );
    }

    // start a new simulation in any case. The server may simulate the movement a bit differently at the turn.
    // resimulate_ = ( sn_GetNetState() == nCLIENT );

    // detect lag slides
    if( sn_GetNetState() == nSERVER )
    {
        // see how far we should be simulated in an ideal world
        REAL simTime=se_GameTime() - Lag();

        REAL lag = simTime - dest->gameTime;  // the real lag
        REAL lagOffset = simTime - lastTime;  // difference between real lag and practical lag (what we need to compensate)
        if ( sn_GetNetState() == nSERVER )
        {
            eLag::Report( Owner(), lag );
            if ( currentWall && currentWall->Wall() && rubberSpeedFactor >= 1-EPS )
            {
                lag -= lagOffset; // switch to practical lag

                // no compensation? Just quit.
                if ( lag < 0 )
                    return;

                // see how much we can go back
                REAL minDist   = currentWall->Wall()->Pos(0);
                REAL maxGoBack = ( distance - minDist ) * .8;

                // see how much we should go back
                REAL stepBack = distance - dest->distance;

                if ( rubberSpeedFactor < 1-EPS )
                {
                    // make the correction distance based so we don't loosen a grind
                    maxGoBack = stepBack;
                }

                // clamp so we don't go back too far
                if ( stepBack > maxGoBack )
                {
                    stepBack = maxGoBack;
                    lag = rubberSpeedFactor*stepBack/verletSpeed_;
                }

                // ask lag compensation how much we are allowed to go back; switch to real lag
                // for the credit
                lag = eLag::TakeCredit( Owner(), lag + lagOffset ) - lagOffset;

                // don't go back further than last sync to the owner.
                // old clients get doubly confused and produce an extra
                // evil lag slide.
                static nVersionFeature noConfusionFromMoveBack( 16 );
                if( !noConfusionFromMoveBack.Supported( Owner() ) &&
                    lastTime - lag < lastSyncOwnerGameTime_ )
                {
                    lag = lastTime - lastSyncOwnerGameTime_;
                }

                // no compensation? Just quit.
                if ( lag < 0 )
                    return;

                // go back in time
                if ( rubberSpeedFactor >= 1-EPS )
                {
                    // rubber is inactive, basic timestep is enough
                    TimestepCore( lastTime - lag );
                }
                else if ( 0 )
                {
                    // rubber is active. Take care!

                    // just extrapolate the movement backwards
                    REAL step = lag * verletSpeed_;
                    lastTime -= lag;

                    // rubber is a bit tricker, we need the effectiveness
                    REAL rubberGranted, rubberEffectiveness;
                    sg_RubberValues( player, verletSpeed_, rubberGranted, rubberEffectiveness );
                    if ( rubberEffectiveness > 0 )
                        rubber -= step/rubberEffectiveness;

                    if ( rubber < 0 )
                        rubber = 0;

                    // position and distance are easy
                    step *= rubberSpeedFactor;
                    distance -= step;
                    pos = pos - dirDrive * step;

                    // undo acceleration
                    verletSpeed_ -= acceleration * lag;
                }

                // see if we went back too far (should almost never happen)
                if ( distance < minDist )
                {
                    // st_Breakpoint();
                    TimestepCore( lastTime + ( minDist - distance )/verletSpeed_ );
                }
            }
        }
    }
}


// *******************************************************************************************
// *
// *	OnDropTempWall
// *
// *******************************************************************************************
//!
//!		@param	wall	   the wall the other cycle is grinding
//!		@param	pos	       the position of the grind
//!     @param  dir        the direction the raycast triggering the gridding comes from
//!
// *******************************************************************************************

void gCycle::OnDropTempWall( gPlayerWall * wall, eCoord const & position, eCoord const & dir )
{
    tASSERT( wall );

    unsigned short idrec = ID();
    tRecorderSync< unsigned short >::Archive( "_ON_DROP_WALL", 8, idrec );

    // determine if the grinded wall is current enough
    bool wallRight = ( currentWall && ( wall->NetWall() == currentWall || wall->NetWall() == currentWall ) );

    // don't drop if we already dropped a short time ago
    if ( wallRight && currentWall->Edge()->Vec().NormSquared() < verletSpeed_ * verletSpeed_ * sg_minDropInterval * sg_minDropInterval )
        wallRight = false;

    tRecorderSync< bool >::Archive( "_ON_DROP_WALL_RIGHT", 8, wallRight );

    // drop the current wall if eiter this or the last wall is grinded
    // gNetPlayerWall * nw = wall->NetWall();
    if ( wallRight )
    {
        // calculate relative position of grinding in wall; if alpha is positive, it's already too late
        REAL alpha = currentWall->Edge()->Edge(0)->Ratio( position );
        tRecorderSync< REAL >::Archive( "_ON_DROP_WALL_ALPHA", 8, alpha );
        if ( alpha > -.5 )
        {
            unsigned short idrec = ID();
            tRecorderSync< unsigned short >::Archive( "_ON_DROP_WALL_DROP", 8, idrec );

            // just request the drop, Timestep() will execute it later
            dropWallRequested_ = true;

            // bend last driving direction to -dir. That way, should the grinder overtake this cycle,
            // it will end up on the right side of his wall.
            lastDirDrive = -dir;
        }
    }
}

// ************************************************************




// ************************************************************


// the time the cycle walls stay up ( negative values: they stay up forever )
void 	gCycle::SetWallsStayUpDelay	( REAL delay )
{
    c_pwsud->Set( delay );
}

// how much rubber usage shortens the walls
static REAL sg_cycleRubberWallShrink = 0;
static nSettingItemWatched<REAL>
sg_cycleRubberWallShrinkConf("CYCLE_RUBBER_WALL_SHRINK",
                             sg_cycleRubberWallShrink,
                             nConfItemVersionWatcher::Group_Bumpy,
                             12);

// make walls grow with distance traveled
static REAL sg_cycleDistWallShrink = 0;
static nSettingItemWatched<REAL>
sg_cycleDistWallShrinkConf("CYCLE_DIST_WALL_SHRINK",
                           sg_cycleDistWallShrink,
                           nConfItemVersionWatcher::Group_Bumpy,
                           12);

static REAL sg_cycleDistWallShrinkOffset = 0;
static nSettingItemWatched<REAL>
sg_cycleDistWallShrinkOffsetConf("CYCLE_DIST_WALL_SHRINK_OFFSET",
                                 sg_cycleDistWallShrinkOffset,
                                 nConfItemVersionWatcher::Group_Bumpy,
                                 12);

// calculates the effect of driving distance to wall length
static REAL sg_CycleWallLengthFromDist( REAL distance )
{
    REAL len = gCycle::WallsLength();
    if ( len <= 0 )
    {
        return len;
    }

    // make base length longer or shorter, depending on the sign of sg_cycleDistWallShrink
    REAL d = sg_cycleDistWallShrinkOffset - distance;
    if ( d > 0 )
        len -= sg_cycleDistWallShrink * d;

    return len;
}

// the current length of the walls of this cycle
REAL gCycle::ThisWallsLength() const
{
    // get distance influence
    REAL len = sg_CycleWallLengthFromDist( distance );
    if ( len <= 0 )
    {
        return len;
    }

    // apply rubber shortening
    return len - GetRubber() * sg_cycleRubberWallShrink;
}


// the speed the end of the trail currently receeds with
REAL gCycle::WallEndSpeed() const
{
    REAL rubberMax, rubberEffectiveness;
    sg_RubberValues( player, Speed(), rubberMax, rubberEffectiveness );

    // basic speed from cycle movement
    REAL speed = rubberSpeedFactor * Speed();

    // take distance shrinking into account
    REAL d = sg_cycleDistWallShrinkOffset - distance;
    if ( d > 0 )
        speed *= ( 1 - sg_cycleDistWallShrink );

    // speed from rubber usage and shringing
    if ( rubberEffectiveness > 0 )
        speed += Speed() * ( 1 - rubberSpeedFactor ) * sg_cycleRubberWallShrink / rubberEffectiveness;

    return speed;
}

// the maximum total length of the walls
REAL gCycle::MaxWallsLength() const
{
    REAL len = sg_CycleWallLengthFromDist( distance );

    // exception: if the wall grows faster than it receeds, take the maximum, because the wall will
    // grow backwards
    if ( sg_cycleDistWallShrink > 1 )
    {
        len = wallsLength;
    }

    // if the wall grows from rubber use, add the maximal growth
    if ( sg_cycleRubberWallShrink >= 0 || sg_rubberCycle < 0 )
        return len;
    else
        return len - sg_cycleRubberWallShrink * sg_rubberCycle;
}

// the maximum total length of the walls
void 	gCycle::SetWallsLength			( REAL length)
{
    c_pwl->Set( length );
}

// the radius of the holes blewn in by an explosion
void 	gCycle::SetExplosionRadius		( REAL radius)
{
    c_per->Set( radius );
}

//  *****************************************************************

// copies relevant info from other cylce
void gCycleExtrapolator::CopyFrom( const gCycleMovement& other )
{
    // delegate
    gCycleMovement::CopyFrom( other );

    // set parent
    parent_ = &other;

    // copy distance
    trueDistance_ = GetDistance();
}

// copies relevant info from sync data and everything else from other cycle
void gCycleExtrapolator::CopyFrom( const SyncData& sync, const gCycle& other )
{
    // delegate
    gCycleMovement::CopyFrom( sync, other );

    //eFace* face1 = currentFace;
    MoveSafely( other.lastGoodPosition_, sync.time, sync.time );
    //eFace* face2 = currentFace;
    MoveSafely( sync.pos, sync.time, sync.time );

#ifdef DEBUG_X
    if ( face1 != face2 || face1 != currentFace )
    {
        currentFace = face1;
        MoveSafely( sync.lastTurn *.01 + sync.pos * .99, sync.time, sync.time );
        MoveSafely( sync.pos, sync.time, sync.time );
    }
#endif

    // set parent
    parent_ = &other;

    // set last turn
    lastTurnPos_ = sync.lastTurn;

    // copy distance
    trueDistance_ = GetDistance();

    // make a small timestep backwards if we passed the next
    // destination. While this makes us react a tad later to lag slides,
    // it avoids lag slide false positives, which later cause real
    // lag slides.
    if( eLag::Feature().Supported(0) )
    {
        gDestination *dest = GetCurrentDestination();
        if ( dest )
        {
            REAL distToDest = eCoord::F( dest->position - pos, dirDrive );
            if( distToDest < 0 )
            {
                // instead of doing a full simulation, just trust the data from the
                // destination.
                pos = dest->position;
                lastTime = dest->gameTime;
                distance = dest->distance;
                verletSpeed_ = dest->speed;
            }
        }
    }
}

gCycleExtrapolator::gCycleExtrapolator(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID *p,bool autodelete)
        :gCycleMovement( grid, pos, dir, p, autodelete )
        ,trueDistance_( 0 )
        ,parent_( 0 )
{
    // an extrapolator should not be visible as a gameobject from the outside
    eFace * currentFaceBack = currentFace;
    RemoveFromList();
    currentFace = currentFaceBack;
    if ( !currentFace )
        currentFace = grid->FindSurroundingFace( pos, currentFace );
}

// gCycleExtrapolator::gCycleExtrapolator(nMessage &m);
gCycleExtrapolator::~gCycleExtrapolator()
{
}

/*
// returns the current destination
gDestination* gCycleExtrapolator::GetCurrentDestination() const
{
    return currentDestination;

    // the code below does not appear to be such a good idea after all...
    if ( currentDestination )
    {
        return currentDestination;
    }
    else
    {
        tASSERT( parent_ );

        // return a destination with the current parent position
        static gDestination parentPos( *parent_ );
        parentPos.missable = false;
        parentPos.CopyFrom( *parent_ );
        return &parentPos;
    }
}
*/

bool gCycleExtrapolator::EdgeIsDangerous(const eWall *ww, REAL time, REAL alpha ) const
{
    const gPlayerWall *w = dynamic_cast<const gPlayerWall*>(ww);
    if (w)
    {
        gNetPlayerWall* nw = w->NetWall();

        // get time the wall was built
        REAL builtTime = w->Time(alpha);

        // is the wall built in the future ( happens during extrapolation )
        if ( builtTime > time )
            return false;

        // ignore temporary walls in some cases, they may not be real
        if ( nw && nw->Preliminary() && w->Cycle() == parent_ && fabs( dirDrive * w->Vec() ) < EPS )
            return false;

        // ignore recent walls of parent cycle
        gCycle *otherPlayer=w->Cycle();
        if (otherPlayer==parent_ &&
                ( time < builtTime + 2 * GetTurnDelay() || GetDistance() < w->Pos( alpha ) + .01 * sg_delayCycle*Speed()/SpeedMultiplier()  )
           )
            return false;
    }

    // delegate
    return bool(parent_) && parent_->EdgeIsDangerous( ww, time, alpha ) && gCycleMovement::EdgeIsDangerous( ww, time, alpha );
}

void gCycleExtrapolator::PassEdge(const eWall *ww,REAL time,REAL a,int){
    {
        if (!EdgeIsDangerous(ww,time,a) || !Alive() )
        {
            return;
        }
        else
        {
            eCoord collPos = ww->Point( a );
            throw gCycleDeath( collPos );
        }
    }
}

bool gCycleExtrapolator::TimestepCore(REAL currentTime, bool calculateAcceleration)
{
    // determine a suitable next destination
    gDestination destDefault( *parent_ ), *dest=GetCurrentDestination();
    if ( !dest )
    {
        dest = &destDefault;
    }

    // correct distance
    // distance = dest->distance - DistanceToDestination( *dest );
    // REAL distanceBefore = GetDistance();
    tASSERT(finite(distance));

    // delegate
    bool ret = false;
    try{
        ret = gCycleMovement::TimestepCore( currentTime, calculateAcceleration );
    }
    catch( gCycleDeath & )
    {
        return false;
    }

    // update true distance
    // trueDistance_ += GetDistance() - distanceBefore;
    trueDistance_ = distance;

    return ret;
}

/*
bool gCycleExtrapolator::DoTurn( int dir )
{
    // delegate
    return gCycleMovement::DoTurn( dir );
}
*/

nDescriptor &gCycleExtrapolator::CreatorDescriptor() const{
    // should never be called
    tASSERT( 0 );
    return cycle_init;
}

//  *****************************************************************

const eTempEdge* gCycle::Edge(){
    if (currentWall)
        return currentWall->Edge();
    else
        return NULL;
}

const gPlayerWall* gCycle::CurrentWall(){
    if (currentWall)
        return currentWall->Wall();
    else
        return NULL;
}

/*
const gPlayerWall* gCycle::LastWall(){
    if (lastWall)
        return lastWall->Wall();
    else
        return NULL;
}
*/

/*
static tString lala_bodyTexture("Anonymous/original/textures/cycle_body.png");
static nSettingItem<tString> gg_tex("TEXTURE_CYCLE_BODY", lala_bodyTexture);
//static tConfItemLine g_tex("CYCLE_BODY", sg_bodyTexture);
//static tSettingItem<tString> gg_tex("TEXTURE_CYCLE_BODY", sg_bodyTexture);
static tString lala_wheelTexture("Anonymous/original/textures/cycle_wheel.png");
static nSettingItem<tString> gg_wheelTexture("TEXTURE_CYCLE_WHEEL", lala_wheelTexture);

static tString lala_bikeTexture("Anonymous/original/moviepack/bike.png");
static nSettingItem<tString> lalala_bikeTexture("TEXTURE_MP_BIKE", lala_bikeTexture);
*/

// HACK! Flexible model loader that eats anything a moviepack currently can throw at it.
#ifndef DEDICATED
struct gCycleVisuals
{
    rModel *customModel, *bodyModel, *frontModel, *rearModel; // cycle models
    gTextureCycle *customTexture, *bodyTexture, *wheelTexture; // cycle textures
    gRealColor color; // cycle color
    bool mpType;      // true if moviepack type model/texture can be used
    int  mpPreference; // the user preference for the texture/model search path

    gCycleVisuals( gRealColor const & a_color )
    {
        customModel = bodyModel = frontModel = rearModel = 0;
        customTexture = bodyTexture = wheelTexture = 0;

        color = a_color;

        mpType = false;
        mpPreference = 0;
    }

    ~gCycleVisuals()
    {
        delete customTexture;
        delete bodyTexture;
        delete wheelTexture;
    }

    enum Slot{ SLOT_CUSTOM=0, SLOT_BODY=1, SLOT_WHEEL=2, SLOT_MAX=3 };

    // loads a specific texture from a specific folder
    static rSurface * LoadTextureSafe2( Slot slot, int mp )
    {
        static std::auto_ptr<rSurface> cache[SLOT_MAX][2];
        std::auto_ptr<rSurface> & surface = cache[slot][mp];
        if ( surface.get() == NULL )
        {
            static char const * names[SLOT_MAX]={"bike.png","cycle_body.png", "cycle_wheel.png"};
            char const * name = names[slot];

            char const * folder = mp ? "moviepack" : "textures";
            tString file = tString(folder) + "/" + name;

            surface = std::auto_ptr<rSurface> ( tNEW( rSurface( file ) ) );
        }

        if ( surface->GetSurface() )
            return surface.get();
        else
            return NULL;
    }

    // load one texture from the prefered folder (or the other one as fallback)
    gTextureCycle * LoadTextureSafe( Slot slot, bool wheel )
    {
        rSurface * surface = LoadTextureSafe2( slot, mpPreference );
        if ( !surface )
            surface = LoadTextureSafe2( slot, 1-mpPreference );

        if ( surface )
            return tNEW( gTextureCycle )( *surface, color, 0, 0, wheel );

        return NULL;
    }

    // load textures from the specified folder (or the other one) and the format the model has just been read for
    bool LoadTextures()
    {
        if ( mpType )
        {
            if ( !customTexture )
                customTexture = LoadTextureSafe( SLOT_CUSTOM, false );

            return customTexture;
        }
        else
        {
            if ( !bodyTexture )
                bodyTexture = LoadTextureSafe( SLOT_BODY, false );
            if ( !wheelTexture )
                wheelTexture = LoadTextureSafe( SLOT_WHEEL, true );

            return bodyTexture && wheelTexture;
        }
    }

    // loads a model, checking before if the file exists
    static rModel * LoadModelSafe( char const * filename )
    {
        return rModel::GetModel(filename);
    }

    // load a model of specified type from a specified directory
    bool LoadModel( bool a_mpType, bool mpFolder )
    {
        mpType = a_mpType;
        char const * folder = mpFolder ? "moviepack" : "models";

        if ( mpType )
        {
            tString base = tString(folder);
            base << "/cycle";
            if (!customModel) customModel = LoadModelSafe( base + ".ASE" );
            if (!customModel) customModel = LoadModelSafe( base + ".ase" );

            return customModel && LoadTextures();
        }
        else
        {
            tString base = tString(folder) + "/cycle_";

            if (!bodyModel) bodyModel = LoadModelSafe( base + "body.mod" );
            if (!frontModel) frontModel = LoadModelSafe( base + "front.mod" );
            if (!rearModel) rearModel = LoadModelSafe( base + "rear.mod" );

            return bodyModel && frontModel && rearModel && LoadTextures();
        }
    }

    // tries to load everything from the given data folder
    bool LoadModel2( bool mp )
    {
        // first, try the right type, then the 'unnatural' choice
        return LoadModel( mp, mp ) || LoadModel( !mp, mp );
    }

    // top level load function: tries to load all variations, starting with passed moviepack folder flag
    bool LoadModel( bool mp )
    {
        mpPreference = mp ? 1 : 0;

        // delegate to try loading both formats from both directories
        return LoadModel2( mp ) || LoadModel2( !mp );
    }
};
#endif

#ifndef DEDICATED
// renders a cycle even after it died
class gCycleWallRenderer: public eReferencableGameObject
{
public:
    gCycleWallRenderer( gCycle * cycle )
    : eReferencableGameObject( cycle->Grid(), cycle->Position(), cycle->Direction(), cycle->CurrentFace(), true )
    , cycle_( cycle )
    {
        AddToList();
    }

#if 0 // not required
    virtual ~gCycleWallRenderer()
    {
    }

    virtual void OnRemoveFromGame()
    {
        eReferencableGameObject::OnRemoveFromGame();
    }
#endif
private:
    virtual void Render( eCamera const * camera )
    {
        cycle_->displayList_.RenderAll( camera, cycle_ );
    }

    virtual bool Timestep( REAL currentTime )
    {
        if ( !cycle_ )
        {
            return true;
        }

        Move( cycle_->Position(), lastTime, currentTime );

        return !cycle_->Alive() && !cycle_->displayList_.Walls();
    }

    tJUST_CONTROLLED_PTR< gCycle > cycle_;
};
#endif

void gCycle::MyInitAfterCreation(){
// create wall renderer
#ifndef DEDICATED
    new gCycleWallRenderer( this );
#endif

    dropWallRequested_ = false;
    lastGoodPosition_ = pos;

    lastShotTime = 0;
    shotStarted = 0;
    shotReservoir = 1.0;

#ifdef DEBUG
    // con << "creating cycle.\n";
#endif
    engine  = tNEW(eSoundPlayer)(cycle_run,true);
    turning = tNEW(eSoundPlayer)(turn_wav);
    spark   = tNEW(eSoundPlayer)(scrap);

    //correctDistSmooth=correctTimeSmooth=correctSpeedSmooth=0;
    correctDistanceSmooth = 0;

    resimulate_ = false;

    mp=sg_MoviePack();

    lastTimeAnim = 0;
    timeCameIntoView = 0;

    customModel = NULL;
    body = NULL;
    front = NULL;
    rear = NULL;
    wheelTex = NULL;
    bodyTex = NULL;
    customTexture = NULL;

    correctPosSmooth=eCoord(0,0);

    rotationFrontWheel=rotationRearWheel=eCoord(1,0);

    skew=skewDot=0;

    {
        dir=dirDrive;
        REAL dirLen=dir.NormSquared();
        if ( dirLen > 0 )
        {
            dir = dir * (1/sqrt( dirLen ));
        }
    }

    if (sn_GetNetState()!=nCLIENT){
        if(!player)
        { // distribute AI colors
            // con << current_ai << ':' << take_ai << ':' << maxmindist <<  "\n\n\n";
            color_.r=1;
            color_.g=1;
            color_.b=1;

            trailColor_.r=1;
            trailColor_.g=1;
            trailColor_.b=1;
        }
        else
        {
            player->Color(color_.r,color_.g,color_.b);
            player->TrailColor(trailColor_.r,trailColor_.g,trailColor_.b);
        }

        se_MakeColorValid( color_.r, color_.g, color_.b, 1.0f );
        se_MakeColorValid( trailColor_.r, trailColor_.g, trailColor_.b, .5f );
    }

    // load model and texture
#ifndef DEDICATED
    gCycleVisuals visuals( color_ );
    if ( !visuals.LoadModel( mp ) )
    {
        tERR_ERROR( "Neither classic style nor moviepack style model and textures found. "
                    "The folders \"textures\" and \"moviepack\" need to contain either "
                    "cycle.ase and bike.png or body.mod, front.mod, rear.mod, cycle_body.png and cycle_wheel.png." );
    }

    mp = visuals.mpType;

    // transfer models and textures
    if ( mp )
    {
        // use moviepack style body and texture
        customModel = visuals.customModel;
        visuals.customModel = 0;
        customTexture = visuals.customTexture;
        visuals.customTexture = 0;
    }
    else
    {
        // use classic style body and texture
        body = visuals.bodyModel;
        visuals.bodyModel = 0;
        front = visuals.frontModel;
        visuals.frontModel = 0;
        rear = visuals.rearModel;
        visuals.rearModel = 0;
        bodyTex = visuals.bodyTexture;
        visuals.bodyTexture = 0;
        wheelTex = visuals.wheelTexture;
        visuals.wheelTexture = 0;

        tASSERT ( body && front && rear && bodyTex && wheelTex );

        mp = false;
    }
#endif // DEDICATED

    /*
      the old, less flexible, loading code

    if (mp)
    {
        customModel=new rModel("moviepack/cycle.ASE","moviepack/cycle.ase");
    }
    else{
        body=new rModel("models/cycle_body.mod");
        front=new rModel("models/cycle_front.mod");
        rear=new rModel("models/cycle_rear.mod");
    }

    if (mp)
    {
        // static rSurface bike(lala_bikeTexture);
        static rSurface bike("moviepack/bike.png");
        customTexture=new gTextureCycle(bike,color_,0,0);
    }
    else{
        static rSurface body("textures/cycle_body.png");
        static rSurface wheel("textures/cycle_wheel.png");
        //static rSurface body((const char*) lala_bodyTexture);
        //static rSurface wheel((const char*) lala_wheelTexture);
        wheelTex=new gTextureCycle(wheel,color_,0,0,true);
        bodyTex=new gTextureCycle(body,color_,0,0);
    }
    */

#ifdef DEBUG
    //con << "Created cycle.\n";
#endif
    nextSyncOwner=nextSync=tSysTimeFloat()-1;
    lastSyncOwnerGameTime_ = 0;

    if (sn_GetNetState()!=nCLIENT)
        RequestSync();

    grid->AddGameObjectInteresting(this);

    spawnTime_=lastTimeAnim=lastTime;
    // set spawn time to infinite past if this is the first spawn
    if ( !sg_cycleFirstSpawnProtection && spawnTime_ <= 1.0 )
    {
        spawnTime_ = -1E+20;
    }

    if ( engine )
        engine->Reset(10000);

    if ( turning )
        turning->End();

    // add to game grid
    this->AddToList();

    predictPosition_ = pos;

#ifdef GNUPLOT_DEBUG
    if ( sg_gnuplotDebug && Player() )
    {
        std::ofstream f( Player()->GetUserName() + "_step" );
        f << pos.x << " " << pos.y << "\n";
        std::ofstream g( Player()->GetUserName() + "_sync" );
        g << pos.x << " " << pos.y << "\n";
        std::ofstream h( Player()->GetUserName() + "_turn" );
        h << pos.x << " " << pos.y << "\n";
    }
#endif
}

void gCycle::InitAfterCreation(){
#ifdef DEBUG
    if (!finite(Speed()))
        st_Breakpoint();
#endif
    gCycleMovement::InitAfterCreation();
#ifdef DEBUG
    if (!finite(Speed()))
        st_Breakpoint();
#endif
    MyInitAfterCreation();
}

static eLadderLogWriter se_cycleCreatedWriter("CYCLE_CREATED", false);

gCycle::gCycle(eGrid *grid, const eCoord &pos,const eCoord &d,ePlayerNetID *p)
        :gCycleMovement(grid, pos,d,p,false),
        engine(NULL),
        turning(NULL),
        skew(0),skewDot(0),
        rotationFrontWheel(1,0),rotationRearWheel(1,0),heightFrontWheel(0),heightRearWheel(0),
        m_target_mgr(this),
        currentWall(NULL),
        lastWall(NULL)
{
	cycles.push_back(this);

    se_cycleCreatedWriter << p->GetLogName() << pos.x << pos.y << d.x << d.y;
    se_cycleCreatedWriter.write();

    windingNumberWrapped_ = windingNumber_ = Grid()->DirectionWinding(dirDrive);
    dirDrive = Grid()->GetDirection(windingNumberWrapped_);
    dir = dirDrive;

    deathTime=0;

    lastNetWall=lastWall=currentWall=NULL;

    MyInitAfterCreation();

    sg_ArchiveCoord( this->dirDrive, 1 );
    sg_ArchiveCoord( this->dir, 1 );
    sg_ArchiveCoord( this->pos, 1 );
    sg_ArchiveReal( this->verletSpeed_, 1 );

    flag_ = NULL;
}

gCycle::~gCycle(){
#ifdef DEBUG
    con << "deleting cycle...\n";
#endif
    // clear the destination list

    //Search the list for zones that could be referencing us
    const tList<eGameObject>& gameObjects = Grid()->GameObjects();
    gZone *zone = NULL;
    for (int i=gameObjects.Len()-1;i>=0;i--)
    {
        zone = dynamic_cast<gZone *>(gameObjects(i));

        if (zone)
        {
            if (zone->GetSeekingCycle() == this)
            {
                zone->SetSeekingCycle(NULL);
            }
        }
    }

    tDESTROY(engine);
    tDESTROY(turning);
    tDESTROY(spark);

    cycles.erase(std::find(cycles.begin(),cycles.end(),this));

    this->RemoveFromGame();

    if (mp){
        delete customTexture;
    }
    else{
        delete wheelTex;
        delete bodyTex;
    }
#ifdef DEBUG
    con << "Deleted cycle.\n";
#endif
    /*
      delete currentPos;
      delete last_turn;
    */
}

void gCycle::OnRemoveFromGame()
{
    // keep this cycle alive during this function
    tJUST_CONTROLLED_PTR< gCycle > keep;

    if ( this->GetRefcount() > 0 )
    {
        keep = this;

        this->Turn(0);

        if ( sn_GetNetState() == nSERVER )
            RequestSync();
    }

    // make sure we're dead, so our walls know they need to time out.
    if ( Alive() )
    {
        Die( lastTime );
    }

    if (currentWall)
        currentWall->CopyIntoGrid(0);
    currentWall=NULL;
    lastWall=NULL;

    gCycleMovement::OnRemoveFromGame();
}

// called when the round ends
void gCycle::OnRoundEnd()
{
    // give survival bonus
    if ( Alive() && player )
    {
        Player()->AddScore( score_survive, tOutput("$player_win_survive"), tOutput() );
    }
}


static inline void rotate(eCoord &r,REAL angle){
    REAL x=r.x;
    r.x+=r.y*angle;
    r.y-=x*angle;
    r=r*(1/sqrt(r.NormSquared()));
}

#ifdef MACOSX
// Sparks have a large performance problem on Macs. See http://forums.armagetronad.net/viewtopic.php?t=2167
bool crash_sparks=false;
#else
bool crash_sparks=true;
#endif

//static bool forceTime=false;

// from nNetwork.C
extern REAL planned_rate_control[MAXCLIENTS+2];

//! intermediate data for position prediction
struct gPredictPositionData
{
    REAL maxSpaceReport; //!< maximum space to report in front
#ifdef DEDICATED
    REAL rubberStart;    //!< distance from a wall rubber starts to get activated
#endif
};

// *******************************************************************************************
// *
// *	PreparePredictPosition()
// *
// *******************************************************************************************
//!
//!     @param  data data to be passed to CalculatePredictPosition() later
//!
// *******************************************************************************************
void gCycle::PreparePredictPosition( gPredictPositionData & data )
{
    // don't cast a ray by default
    data.maxSpaceReport = 0;
#ifdef DEDICATED
    if ( sg_predictWalls )
    {
        // predict to the maximum time anyone else may be simulated up to
        REAL maxTime = lastTime + MaxSimulateAhead() + GetMaxLazyLag();
        REAL predictDT = maxTime - lastTime;
        REAL lookAhead = predictDT * verletSpeed_;

        // see how far we can go before rubber kicks in
        data.rubberStart = verletSpeed_ / RubberSpeed();

        // store max lookahead plus rubber safety
        data.maxSpaceReport = lookAhead + data.rubberStart;
    }
#else
    data.maxSpaceReport = verletSpeed_ * se_PredictTime() * rubberSpeedFactor;
#endif

    // request a raycast of the right length
    maxSpaceMaxCast_ = data.maxSpaceReport;
}

// *******************************************************************************************
// *
// *	CalculatePredictPosition()
// *
// *******************************************************************************************
//!
//!     @param  data data from PreparePredictPosition()
//!		@return the time up to which the cycle's position was predicted
//!
// *******************************************************************************************

REAL gCycle::CalculatePredictPosition( gPredictPositionData & data )
{
    // predict position
    REAL predictTime = lastTime;
    {
#ifdef DEDICATED
        predictPosition_ = pos;

        if ( sg_predictWalls )
        {
            REAL spaceAhead = GetMaxSpaceAhead( data.maxSpaceReport );
            spaceAhead -= data.rubberStart;

            if ( spaceAhead > 0 )
            {
                // store consistent prediction position and  time
                predictPosition_ = pos + dirDrive * spaceAhead;
                predictTime = lastTime + spaceAhead/verletSpeed_;
            }
        }
#else
        // predict half a frame time
        predictTime += se_PredictTime();
        predictPosition_ = pos+correctPosSmooth + dirDrive * GetMaxSpaceAhead( data.maxSpaceReport );
#endif
    }

    return predictTime;
}

bool gCycle::Timestep(REAL currentTime){
    // clear out dangerous info when we're done
    gMaxSpaceAheadHitInfoClearer hitInfoClearer( maxSpaceHit_ );

    // archive rubber speed for later comparison
    REAL rubberSpeedFactorBack = rubberSpeedFactor;

    // if ( Owner() == sn_myNetID )
    //    con << pos << ',' << distance << ',' << eCoord::F( dirDrive, pos ) - distance << '\n';

    // request the right space ahead for wall extrapolation
    gPredictPositionData predictPositionData;
    PreparePredictPosition( predictPositionData );

    // drop current wall if it was requested
    if ( dropWallRequested_ )
    {
        // but don't do so too often globally (performance)
        static double nextDrop = 0;
        double time = tSysTimeFloat();
        if ( time >= nextDrop )
        {
            unsigned short idrec = ID();
            tRecorderSync< unsigned short >::Archive( "_PARTIAL_COPY_GRID", 8, idrec );

            nextDrop = time + sg_minDropInterval;
            if ( currentWall )
            {
                currentWall->Update(lastTime,pos);
                currentWall->PartialCopyIntoGrid( grid );
            }
            dropWallRequested_ = false;
        }
    }

#ifdef GNUPLOT_DEBUG
    if ( sg_gnuplotDebug && Player() )
    {
        std::ofstream f( Player()->GetUserName() + "_step", std::ios::app );
        f << pos.x << " " << pos.y << "\n";
    }
#endif

    // save old braking status
    short oldBraking = braking;

    // timewarp test debug code
    //if ( Player() && Player()->IsHuman() )
    //    currentTime -= .1;

    // don't timestep when you're dead
    if ( !Alive() )
    {
        // die completely
        Die( lastTime );

        // and let yourself be removed from the lists so we don't have to go
        // through this again.
        return true;
    }

    // Debug archive position and speed
    sg_ArchiveCoord( pos, 7 );
    sg_ArchiveReal( verletSpeed_, 7 );

    if ( !destinationList && sn_GetNetState() == nCLIENT )
    {
        // insert emergency first destination without broadcasting it
        gDestination* dest = tNEW(gDestination)(*this);
        dest->messageID = 0;
        dest->distance = -.001;
        dest->InsertIntoList(&destinationList);
    }

    // start new extrapolation
    if ( !extrapolator_ && resimulate_ )
        ResetExtrapolator();

    // extrapolate state from server and copy state when finished
    if ( extrapolator_ )
    {
        REAL dt = ( currentTime - lastTime ) * sg_syncFF / sg_syncFFSteps;
#ifdef DEBUG
        // dt *= 10.101;
        //if ( !resimulate_ )
        //	dt *= .1;
#endif
        for ( int i = sg_syncFFSteps - 1; i>= 0; --i )
        {
            if ( Extrapolate( dt ) )
            {
                SyncFromExtrapolator();
                break;
            }
        }
    }

    bool ret = false;

    // nothing special if simulating backwards
    if (currentTime < lastTime)
    {
        ret = gCycleMovement::Timestep(currentTime);
    }
    // no targets are given
    else if ( !currentDestination && pendingTurns.empty() )
    {
        // chatting? activate chatbot
        if ( bool(player) &&
                player->IsHuman() &&
                ( sg_chatBotAlwaysActive || player->IsChatting() ) &&
                player->Owner() == sn_myNetID )
        {
            gCycleChatBot & bot = gCycleChatBot::Get( this );
            bot.Activate( currentTime );
        }
        else if ( &(*chatBot_) )
        {
            chatBot_->nextChatAI_ = 0;
        }

        bool simulate=Alive();

        if ( !pendingTurns.empty() || currentDestination )
            simulate=true;

        if (simulate)
        {
            try
            {
                ret = gCycleMovement::Timestep(currentTime);
            }
            catch ( gCycleDeath const & death )
            {
                KillAt( death.pos_ );
                return false;
            }
        }
        else
            ret = !Alive();
    }
    else
    {
        // just basic movement to do: let base class handle that.
        try
        {
            gCycleMovement::Timestep(currentTime);
        }
        catch ( gCycleDeath const & death )
        {
            KillAt( death.pos_ );
            return false;
        }
    }

    // do the rest of the timestep
    try
    {
        if ( currentTime > lastTime )
            ret = gCycleMovement::Timestep(currentTime);
    }
    catch ( gCycleDeath const & death )
    {
        KillAt( death.pos_ );
        return false;
    }

    REAL predictTime = CalculatePredictPosition( predictPositionData );

    if ( Alive() && currentWall )
    {
        // z-man: the next two lines are a very bad idea. This lets walls stick out on the other side while you're using up your rubber.
        //if ( sn_GetNetState() != nSERVER )
        //    wallEndPos = pos + dirDrive * ( verletSpeed_ * se_PredictTime() );

        // but using the predicted position which halts at walls may work
        currentWall->Update(predictTime, PredictPosition() );
    }

    //check for brake change (probably not the best place to do this)
    if (sn_GetNetState()!=nCLIENT)
    {
        if (braking != oldBraking)
        {
            ProcessShoot(false);
        }
    }

    // checkpoint wall when rubber starts to get used
    if ( currentWall )
    {
        if ( rubberSpeedFactor >= .99 && rubberSpeedFactorBack < .99 )
        {
            currentWall->Checkpoint();
        }
        else if ( rubberSpeedFactor < .99 && rubberSpeedFactorBack >= .99 )
        {
            currentWall->Checkpoint();
        }
        else if ( rubberSpeedFactor < .1 && rubberSpeedFactorBack >= .1 )
        {
            currentWall->Checkpoint();
        }
        else if ( rubberSpeedFactor < .01 && rubberSpeedFactorBack >= .01 )
        {
            currentWall->Checkpoint();
        }
    }

    if ( sn_GetNetState()==nSERVER )
    {
        // do an emergency sync when rubber starts to get used, it may come unexpected to clients
        if ( rubberSpeedFactor < .99 && rubberSpeedFactorBack >= .99 )
        {
            RequestSyncOwner();
        }
    }

    //check for brake change (probably not the best place to do this)
    if (sn_GetNetState()!=nCLIENT)
    {
        if (braking != oldBraking)
        {
            ProcessShoot(false);
        }
    }

    // check target assignment timeout
    if (currentTime > gTarget::start_time)
    {
        if (gTarget::assignment_mode && CheckTargetPtr() && !Target().Started())
            gTarget::AutoSetCycles(gCycle::cycles);
        if (CheckTargetPtr()) Target().Timestep(currentTime);
    }

    // check whether simulation has fallen too far behind the requested time
#ifdef DEDICATED
    if ( Alive() && currentTime > lastTime + Lag() + 1 )
    {
        sn_ConsoleOut( "0xff7777Admin : 0xffff77BUG had to kill a cycle because it lagged behind in the simulation. Probably the invulnerability bug. Investigate!\n" );
        st_Breakpoint();
        KillAt( pos );
        ret = false;
    }
#endif

    return ret;
}

// lets a value decay smoothly
static void DecaySmooth( REAL& smooth, REAL relSpeed, REAL minSpeed, REAL clamp )
{
#ifdef DEBUG
    if ( fabs(smooth) > .01 )
    {
        int x;
        x = 1;
    }
#endif

    // increase correction speed if the value is out of bounds
    if ( clamp > 0 )
        relSpeed *= ( 1 + smooth * smooth / ( clamp * clamp ) );

    // nothing to do
    if ( smooth == 0 )
    {
        return;
    }

    // calculate speed from relative speed
    REAL speed = smooth * relSpeed;

    // apply minimal correction
    if ( fabs( speed ) < minSpeed )
        speed = copysign ( minSpeed , smooth );

    // don't overshoot
    if ( fabs( speed ) > fabs( smooth ) )
        smooth = 0;
    else
        smooth -= speed;
}

// clamps a cycle displacement to non-confusing values
static REAL ClampDisplacement( gCycle* cycle, eCoord& displacement, const eCoord& lookout, const eCoord& pos )
{
    gSensor sensor( cycle, pos, lookout );
    sensor.detect(1);
    if ( sensor.ehit && sensor.hit >= 0 && sensor.hit < 1 )
    {
#ifdef DEBUG_X
        // repeat sensor
        gSensor sensor( cycle, pos, lookout );
        sensor.detect(1);
#endif
        displacement = displacement * sensor.hit;
    }
    return sensor.hit;
}

// from gCycleMovement.cpp
REAL sg_GetSparksDistance();


bool gCycle::TimestepCore(REAL currentTime, bool calculateAcceleration ){
    if (!finite(skew))
        skew=0;
    if (!finite(skewDot))
        skewDot=0;

    // eCoord oldpos=pos;

    REAL ts=(currentTime-lastTime);

    clamp(ts, -10, 10);
    //clamp(correctTimeSmooth, -100, 100);
    clamp(correctDistanceSmooth, -100, 100);
    //clamp(correctSpeedSmooth, -100, 100);

    // scale factor for smoothing. It is always used as
    // value += smooth * smooth_correction;
    // smooth_correction -= smooth * smooth_correction;

    REAL smooth = 0;

    if ( ts > 0 )
    {
        // go a bit of the way
        smooth = 1 - 1/( 1 + ts / sg_cycleSyncSmoothTime );
    }

    //if ( smooth > 0)
    //{
    //    REAL scd = correctDistanceSmooth * smooth;
    //    distance += scd;
    //    correctDistanceSmooth -= scd;
    // }

    // handle distance correction
    TransferPositionCorrectionToDistanceCorrection();

    // apply smooth position correction
    // smooth = .5f;
    //correctPosSmooth = eCoord(0,0);

    // correctPosSmooth = correctPosSmooth * ( 1 - smooth );

    //if ( 0 )

    REAL animts=currentTime-lastTimeAnim;
    if (animts<0 || !finite(animts))
        animts=0;
    else
        lastTimeAnim=currentTime;

    // handle decaying of smooth position correction
    {
        // let components of smooth position correction decay
        REAL minSpeed = sg_cycleSyncSmoothMinSpeed * Speed() * animts;
        REAL clamp    = sg_cycleSyncSmoothThreshold * Speed();

        DecaySmooth( correctPosSmooth.x, smooth, minSpeed, clamp );
        DecaySmooth( correctPosSmooth.y, smooth, minSpeed, clamp );

        // do sanity checks
        if ( correctPosSmooth.NormSquared() > EPS )
        {
            // cast ray to make sure corrected position lies on the right side of walls
            ClampDisplacement( this, correctPosSmooth, correctPosSmooth * 2, pos );

            // same for negative direction, players should see it when they are close to a wall
            ClampDisplacement( this, correctPosSmooth, -correctPosSmooth, pos );

            // cast another some rays into the future
            eCoord lookahead = dirDrive * ( 2 * correctPosSmooth.Norm() );
            ClampDisplacement( this, correctPosSmooth, lookahead, pos );
        }
    }

    if (animts>.2)
        animts=.2;

    rotate(rotationFrontWheel,2*verletSpeed_*animts/.43);
    rotate(rotationRearWheel,2*verletSpeed_*animts/.73);

    REAL sparksDistance = sg_GetSparksDistance();
    REAL extension = .25;
    if ( extension < sparksDistance )
        extension = sparksDistance;

    //    REAL step=speed*ts; // +.5*acceleration*ts*ts;

    // animate cycle direction
    {
        // move it a bit closer to dirDrive
        dir=dir+dirDrive*animts*verletSpeed_*3;

        // if it's too far away from dirDrive, clamp it
        eCoord dirDistance = dir - dirDrive;
        REAL dist = dirDistance.NormSquared();
        const REAL maxDist = .8;
        if (dirDistance.NormSquared() > maxDist)
            dir = dirDrive + dirDistance* (1/sqrt(dist/maxDist));

        // normalize
        dir=dir*(1/sqrt(dir.NormSquared()));
    }

    {
        eCoord oldPos = pos;

        if ( Alive() ){
            // delegate core work to base class
            try
            {
                // start building wall
                REAL startBuildWallAt = spawnTime_ + sg_cycleWallTime;
                if ( !currentWall && currentTime > startBuildWallAt  )
                {
                    // simulate right to the spot where the wall should begin
                    if ( currentTime < startBuildWallAt )
                        if ( gCycleMovement::TimestepCore( startBuildWallAt, calculateAcceleration ) )
                            return true;
                    calculateAcceleration = true;

                    // build the wall, modifying the spawn time to make sure it happens
                    REAL lastSpawn = spawnTime_;
                    spawnTime_ += -1E+20;
                    DropWall();
                    spawnTime_ = lastSpawn;
                    lastTurnPos_ = pos; // hack last turn position to generate good wall
                }

                // simulate rest of frame
                if ( gCycleMovement::TimestepCore( currentTime, calculateAcceleration ) )
                    return true;
            }
            catch ( gCycleDeath const & death )
            {
                KillAt( death.pos_ );

                // death exceptions are precise; we can safely take over the position from it
                oldPos = death.pos_;
            }
        }

        // die where you started
        if ( !Alive() )
        {
            MoveSafely(oldPos,currentTime,currentTime);
            Die( currentTime );
        }
    }

    // Debug archive position and speed
    sg_ArchiveCoord( pos, 7 );
    sg_ArchiveReal( verletSpeed_, 7 );

    if (Alive()){
#ifndef DEDICATED
        // animate skew
        gSensor fl(this,pos,dirDrive.Turn(1,1));
        gSensor fr(this,pos,dirDrive.Turn(1,-1));

        fl.detect(extension*4);
        fr.detect(extension*4);

        if (fl.hit > extension)
            fl.hit = extension;

        if (fr.hit > extension)
            fr.hit = extension;

        REAL lr=(fl.hit-fr.hit)/extension;

        // ODE for skew
        const REAL skewrelax=24;
        skewDot-=128*(skew+lr/2)*animts;
        skewDot/=1+skewrelax*animts;
        if (skew*skewDot>4) skewDot=4/skew;
        skew+=skewDot*animts;

        REAL fac = 0.5f;
        if ( skew > fr.hit * fac )
        {
            skew = fr.hit * fac;
        }
        if ( skew < -fl.hit * fac )
        {
            skew = -fl.hit * fac;
        }

        // generate sparks
        eCoord sparkpos,sparkdir;

        if (fl.ehit && fl.hit<=sparksDistance){
            sparkpos=pos+dirDrive.Turn(1,1)*fl.hit;
            sparkdir=dirDrive.Turn(0,-1);
        }
        if (fr.ehit && fr.hit<=sparksDistance){
            sparkpos=pos+dirDrive.Turn(1,-1)*fr.hit;
            sparkdir=dirDrive.Turn(0,1);
        }

        if (fabs(skew)<fabs(lr*.8) ){
            skewDot-=lr*1000*animts;
            if (crash_sparks && animts>0)
            {
                gPlayerWall *tmpplayerWall=0;

                if(fl.ehit) tmpplayerWall=dynamic_cast<gPlayerWall*>(fl.ehit->GetWall());
                if(fr.ehit) tmpplayerWall=dynamic_cast<gPlayerWall*>(fr.ehit->GetWall());

                if(tmpplayerWall) {
                    gCycle *tmpcycle = tmpplayerWall->Cycle();

                    if( tmpcycle )
                        new gSpark(grid, sparkpos-dirDrive*.1,sparkdir,currentTime,color_.r,color_.g,color_.b,tmpcycle->color_.r,tmpcycle->color_.g,tmpcycle->color_.b);
                }
                else
                    new gSpark(grid, sparkpos-dirDrive*.1,sparkdir,currentTime,color_.r,color_.g,color_.b,1,1,1);

                if ( spark )
                    spark->Reset();
            }
        }

        if (fabs(skew)<fabs(lr*.9) ){
            skewDot-=lr*100*animts;
            if (crash_sparks && animts>0)
            {
                gPlayerWall *tmpplayerWall=0;

                if(fl.ehit) tmpplayerWall=dynamic_cast<gPlayerWall*>(fl.ehit->GetWall());
                if(fr.ehit) tmpplayerWall=dynamic_cast<gPlayerWall*>(fr.ehit->GetWall());

                if(tmpplayerWall) {
                    gCycle *tmpcycle = tmpplayerWall->Cycle();

                    if( tmpcycle )
                        new gSpark(grid, sparkpos-dirDrive*.1,sparkdir,currentTime,color_.r,color_.g,color_.b,tmpcycle->color_.r,tmpcycle->color_.g,tmpcycle->color_.b);
                }
                else
                    new gSpark(grid, sparkpos-dirDrive*.1,sparkdir,currentTime,color_.r,color_.g,color_.b,1,1,1);
            }
        }
#endif
        /*
          if (fl.hit+fr.hit<extension*.4)
          Kill();
        */
    }

    // clamp skew
    if (skew>.5){
        skew=.5;
        skewDot=0;
    }

    if (skew<-.5){
        skew=-.5;
        skewDot=0;
    }

    if ( sn_GetNetState()==nSERVER )
    {
        if (nextSync < tSysTimeFloat() )
        {
            // delay syncs for old clients when there is a wall ahead; they would tunnel locally
            REAL lookahead = Speed() * sg_syncIntervalEnemy*.5;
            if ( !sg_avoidBadOldClientSync || sg_NoLocalTunnelOnSync.Supported( Owner() ) || GetMaxSpaceAhead( lookahead ) >= lookahead )
            {
                RequestSync(false);

                // checkpoint wall for better collision accuracy, only required on the server
                if ( currentWall )
                    currentWall->Checkpoint();
            }

            nextSync=tSysTimeFloat()+sg_syncIntervalEnemy;
            nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
        }
        else if ( nextSyncOwner < tSysTimeFloat() &&
                  Owner() != 0 &&
                  sn_Connections[Owner()].bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) > 200 )
        {
            // sync only to the owner (provided there is enough bandwidth available)
            RequestSyncOwner();
        }
    }

    return (!Alive());
}


void gCycle::InteractWith(eGameObject *target,REAL,int){
    /*
      if (alive && target->type()==ArmageTron_CYCLE){
         gCycle *c=(gCycle *)target;
         if (c->alive){
      const eEdge *current_eEdge=Edge();
      const eEdge *other_eEdge=c->Edge();
      if(current_eEdge && other_eEdge){
      ePoint *meet=current_eEdge->IntersectWith(other_eEdge);
      if (meet){ // somebody is going to die!
      REAL ratio=current_eEdge->Ratio(*meet);
      REAL time=CurrentWall()->Time(ratio);
      PassEdge(other_eEdge,time,other_eEdge->Ratio(*meet),0);
      }
      }
         }
      }
    */
}

// *******************************************************************************************
// *
// *	Die
// *
// *******************************************************************************************
//!
//!		@param	time	the time of death
//!
// *******************************************************************************************

void gCycle::Die( REAL time )
{
    if ( sn_GetNetState() == nSERVER )
    {
        // request one last sync
        RequestSync( true );
    }

    if( player && Alive() )
    {
        // death is hardly good timing.
        player->AnalyzeTiming( -1 );
    }

    gCycleMovement::Die( time );

    // handle cycle's target and hunters
    if (CheckTargetPtr())
    {
        // keep hunters list locally
        vec_cycle_ptr l_hunters(Target().m_hunters);

        // 1st, unset this cycle target
        Target().Unset(false);

        // 2nd, unset all cycles from hunter list silently, they will be reaffected below if needed ...
        for (gCycleItr itr = l_hunters.begin(); itr != l_hunters.end(); ++itr)
        {
            (*itr)->Target().Unset(false);
        }

        // check if there's still enough cycles to set target
        if (AliveCounter()<gTarget::min_cycles)
        {
            // we've reached min_cycles so all targets must be disabled, unset all cycle's targets
            for (gCycleItr itr = cycles.begin(); itr != cycles.end(); ++itr)
            {
                if (((*itr)->CheckTargetPtr()) && ((*itr)->Target().HasTarget()))
                {
                	(*itr)->Target().Unset(true);
                }
            }
        } else {
            // set this cycle's hunters new targets
            gTarget::AutoSetCycles(l_hunters);
        }
    }

    // reset smoothing
    correctPosSmooth = eCoord();
    TransferPositionCorrectionToDistanceCorrection();
    predictPosition_ = pos;

    // delete all temporary walls of this cycle
    for ( int i = sg_netPlayerWalls.Len()-1; i >= 0; --i )
    {
        gNetPlayerWall * wall = sg_netPlayerWalls(i);
        if ( wall->Cycle() == this && wall->Preliminary() )
        {
            wall->real_CopyIntoGrid(grid);
        }
    }
}

eLadderLogWriter sg_deathFragWriter("DEATH_FRAG", true);
eLadderLogWriter sg_deathSuicideWriter("DEATH_SUICIDE", true);
eLadderLogWriter sg_deathTeamkillWriter("DEATH_TEAMKILL", true);

static bool sg_suicideMessage = true;
static tSettingItem< bool > sg_suicideMessageConf( "SUICIDE_MESSAGE", sg_suicideMessage );

void gCycle::KillAt( const eCoord& deathPos){
    // don't kill invulnerable cycles
    if ( !Vulnerable() )
    {
        MoveSafely( deathPos, lastTime, lastTime );
        return;
    }

    // find the killer from the enemy influence storage
    ePlayerNetID const * constHunter = enemyInfluence.GetEnemy();
    ePlayerNetID * hunter = Player();

    // cast away const the safe way
    if ( constHunter && constHunter->Object() )
        hunter = constHunter->Object()->Player();

    // only take it if it is not too old
    if ( LastTime() - enemyInfluence.GetTime() > sg_suicideTimeout )
        hunter = NULL;

    // suicide?
    if ( !hunter )
        hunter = Player();


    if (!Alive() || sn_GetNetState()==nCLIENT)
        return;

#ifdef KRAWALL_SERVER_LEAGUE
    if (    hunter           && Player()          &&
            !dynamic_cast<gAIPlayer*>(hunter)     &&
            !dynamic_cast<gAIPlayer*>(Player())             &&
            hunter->IsAuth() && Player()->IsAuth())
        nKrawall::ServerFrag(hunter->GetUserName(), Player()->GetUserName());
#endif

    if (hunter==Player())
    {
        if (hunter)
        {
            sg_deathSuicideWriter << hunter->GetUserName();
            sg_deathSuicideWriter.write();

            if ( score_suicide )
                hunter->AddScore(score_suicide, tOutput(), "$player_lose_suicide", sg_suicideMessage );
            else
            {
                if (sg_suicideMessage)
                {
                    tColoredString hunterName;
                    hunterName << *hunter << tColoredString::ColorString(1,1,1);
                    sn_ConsoleOut( tOutput( "$player_free_suicide", hunterName ) );
                }
            }
        }
    }
    else{
        if (hunter)
        {
            tOutput lose;
            tOutput win;
            if (Player())
            {
                tColoredString preyName;
                preyName << *Player();
                preyName << tColoredString::ColorString(1,1,1);
                if (Player()->CurrentTeam() != hunter->CurrentTeam()) {
                    sg_deathFragWriter << Player()->GetUserName() << hunter->GetUserName();
                    sg_deathFragWriter.write();

                    win.SetTemplateParameter(3, preyName);
                    win << "$player_win_frag";
                    if ( score_kill != 0 )
                        hunter->AddScore(score_kill, win, lose );
                    else
                    {
                        tColoredString hunterName;
                        hunterName << *hunter << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_free_frag", hunterName, preyName ) );
                    }
                }
                else {
                    sg_deathTeamkillWriter << Player()->GetUserName() << hunter->GetUserName();
                    sg_deathTeamkillWriter.write();

                    tColoredString hunterName;
                    hunterName << *hunter << tColoredString::ColorString(1,1,1);
                    sn_ConsoleOut( tOutput( "$player_teamkill", hunterName, preyName ) );
                }
            }
            else
            {
                win << "$player_win_frag_ai";
                hunter->AddScore(score_kill, win, lose);
            }

            // find the cycle we were killed by
            const tList<eGameObject>& gameObjects = Grid()->GameObjects();
            gCycle * pHunterCycle = NULL;
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                gCycle *pCycle=dynamic_cast<gCycle *>(gameObjects(i));

                if (pCycle)
                {
                    if (pCycle->Player() == hunter)
                    {
                        pHunterCycle = pCycle;
                        break;
                    }
                }
            }

            if (pHunterCycle)
            {
                if (CheckTargetPtr())
                {
            	    if ((pHunterCycle->CheckTargetPtr()) && (pHunterCycle->Target().HasTarget(this)))
            	    {
            	        // handle target scoring
            		    pHunterCycle->Target().AddScore();
            		    // Assign pHunter a new target
            	        if (!pHunterCycle->Target().AutoSet(gTarget::EXCLUDE, this))
            	            pHunterCycle->Target().Unset();
            	    }
            	    // unset this cycle target silently
            	    Target().Unset(false);
            	    // set this cycle's hunters new targets
                    // _assignment_mode: 0=disable, 1/2 is enable
                    //   1 affects the player killing your target while 2 randomly affects new target
            	    if (gTarget::assignment_mode==1)
            	    {
            	        gTarget::AutoSetCycles(Target().m_hunters, gTarget::FORCE, pHunterCycle);
            	    } else {
            	    	gTarget::AutoSetCycles(Target().m_hunters, gTarget::EXCLUDE, this);
            	    }
                }

                Killed(pHunterCycle);

            }
        }
        //	if (prey->player && (prey->player->CurrentTeam() != hunter->player->CurrentTeam()))
        if (Player())
            Player()->AddScore(score_die,tOutput(),"$player_lose_frag");
    }

    // set position to death position so the explosion is at the right place (I dimly remember this caused problems in an earlier attempt)
    this->pos = deathPos;

    Kill();
}

class gJustChecking
{
public:
    static bool justChecking;

    gJustChecking(){ justChecking = false; }
    ~gJustChecking(){ justChecking = true; }
};

bool gJustChecking::justChecking = true;

bool gCycle::EdgeIsDangerous(const eWall* ww, REAL time, REAL a) const{
    gPlayerWall const * w = dynamic_cast< gPlayerWall const * >( ww );
    if ( w )
    {
        if ( !Vulnerable() )
            return false;

        gNetPlayerWall *nw = w->NetWall();

        // check whether the wall is one of the walls no real collision is
        // possible with. The lastNetWall is only checked for enemy cycles,
        // because it can be an arbitrary wall for your own cycle. But for
        // enemy cycles, it may be what lastWall should be, and the test
        // prevents enemy cycles from getting stuck right after a turn.
        if( nw == currentWall || nw == lastWall || ( nw == lastNetWall && Owner() != sn_myNetID ) )
            return false;

        // see if the wall is from another player and from the future.
        // if it is, it's not dangerous, this cycle was here first.
        // on passing the wall later, the other cycle will be pushed back
        // to the collision point or killed if that is no longer possible.
        // z-man notes: I've got the vaque feeling something like this was
        // here before, but got thrown out again for making problems.
        if ( gJustChecking::justChecking && w->CycleMovement() != static_cast< const gCycleMovement * >( this ) && w->Time(a) > time )
        {
            // One problem with this is that you kill teammates if they can't
            // be pushed back, whereas we'd just use our rubber to survive.
            // So we should first check whether the other player should be
            // protected.
            gCycle const * otherPlayer = w->Cycle();
            if ( !otherPlayer || // valididy
                    otherPlayer->Team() != this->Team() || // team protection
                    !otherPlayer->currentWall || w == otherPlayer->currentWall->Wall() // pushback protection, if the other player can be pushed back, it's all right as well
               )
                return false;
        }
    }

    return gCycleMovement::EdgeIsDangerous( ww, time, a );
}

// turn future walls of a cycle into gaping holes of nothingness if it indeed belongs to the cycle
static void sg_KillFutureWall( gCycle * cycle, gNetPlayerWall * wall )
{
    if ( cycle && wall && wall->Cycle() == cycle && wall->Pos(1) > cycle->GetDistance() )
    {
        wall->BlowHole( cycle->GetDistance(), wall->Pos(1) + 100, 0 );
    }
}

// turn future walls of a cycle into gaping holes of nothingness
static void sg_KillFutureWalls( gCycle * cycle )
{
#ifdef DEBUG_X
    con << "Removing future walls of the cylce that just got killed mercilessly.\n";
#endif

    // handle future walls that won't be drawn after all. Just make them a big hole.
    if ( sn_GetNetState() != nCLIENT )
    {
        int i;
        for ( i = sg_netPlayerWalls.Len()-1; i >= 0; --i )
            sg_KillFutureWall( cycle, sg_netPlayerWalls(i) );

        for ( i = sg_netPlayerWallsGridded.Len()-1; i >= 0; --i )
            sg_KillFutureWall( cycle, sg_netPlayerWallsGridded(i) );
    }
}

static void sg_HoleScore( gCycle & cycle )
{
    cycle.Player()->AddScore( score_hole, tOutput("$player_win_hole"), tOutput("$player_lose_hole") );
}

static eLadderLogWriter sg_sacrificeWriter("SACRIFICE", true);

void gCycle::PassEdge(const eWall *ww,REAL time,REAL a,int){
    {
        // deactivate time check
        gJustChecking thisIsSerious;

        if (!EdgeIsDangerous(ww,time,a) || !Alive() )
        {
            // request a sync for everyone if this is a non-bogus wall passage, maybe not all clients know the wall is passable
            if ( ( !currentWall || ww != currentWall->Wall() ) && ( !lastWall || ww != lastWall->Wall() ) )
                RequestSyncAll();

            // check whether we drove through a hole in an enemy wall made by a teammate
            gPlayerWall const * w = dynamic_cast< gPlayerWall const * >( ww );
            if ( Alive() && w && score_hole )
            {
                gExplosion * explosion = w->Holer( a, time );
                if ( explosion )
                {
                    gCycle * holer = explosion->GetOwner();
                    if ( holer && holer != this && holer->Player() &&
                         Player() &&
                         w->Cycle() && w->Cycle()->Player() &&
                         holer->Player()->CurrentTeam() == Player()->CurrentTeam() &&       // holer must have been a teammate
                         w->Cycle()->Player()->CurrentTeam() != Player()->CurrentTeam()  // wall must have been an enemy
                        )
                    {
                        // this test must come last, it resets the flag.
                        if ( explosion->AccountForHole() )
                        {
                            sg_sacrificeWriter << Player()->GetUserName() << holer->Player()->GetUserName() << w->Cycle()->Player()->GetUserName();
                            sg_sacrificeWriter.write();
                            if ( score_hole > 0 )
                            {
                                // positive hole score goes to the holer
                                sg_HoleScore( *holer );
                            }
                            else
                            {
                                // negative hole score to the driver
                                sg_HoleScore( *this );

                            }
                       }
                    }
                }
            }

            return;
        }

#ifdef DEBUG
        if (!EdgeIsDangerous(ww,time,a) || !Alive() )
            return;
#endif
    }

    // request a sync to bring everyone up to date about the cycle passing/getting stuck on this wall
    RequestSyncOwner();

#ifdef DEBUG_X
    // keep other cycle around
    tJUST_CONTROLLED_PTR<gCycleExtrapolator> keepOther( extrapolator_ );

    ResetExtrapolator();

    // extrapolate state from server and copy state when finished
    REAL dt = 1;
    for ( int i = 9; i>= 0; --i )
    {
        Extrapolate( dt );
    }

    extrapolator_ = keepOther;
#endif

    eCoord collPos = ww->Point( a );

    const gPlayerWall *w = dynamic_cast<const gPlayerWall*>(ww);

    enemyInfluence.AddWall( ww, collPos, 0, this );

    if (w)
    {
        gCycle *otherPlayer=w->Cycle();

        REAL otherTime = w->Time(a);
        if(otherPlayer && time < otherTime*(1-EPS) )
        {
            // also send updates about the other cylce
            otherPlayer->RequestSyncOwner();

            // get the distance of the wall
            REAL wallDist = w->Pos(a);
            // get the distance the cycle is simulated up to
            REAL cycleDist = w->Cycle()->distance;
            // comparing these two gives an accurate criterion whether the wall is extrapolated
            if ( wallDist > cycleDist * (1 + EPS ) )
            {
                static bool fix = false;
                // it's an extrapolation wall, don't simulate further.
                if ( fix && lastTime > se_GameTime() - 2 * Lag() - GetMaxLazyLag() )
                    throw gCycleStop();
                else
                    return;
            }

            // we were first!
            if ( otherPlayer->Vulnerable() )
            {
                static bool tryToSaveFutureWallOwner = true;
                bool saved = false;

                if ( tryToSaveFutureWallOwner && otherPlayer->currentWall && w == otherPlayer->currentWall->Wall() )
                {
                    // teleport the other cycle back to the point before the collision; its next timestep
                    // will simulate the collision again from the right viewpoint
                    // determine the distance of the pushback
                    REAL d = otherPlayer->GetDistanceSinceLastTurn() * .001;
                    if ( d < .01 )
                        d = .01;
                    REAL maxd = eCoord::F( otherPlayer->dirDrive, collPos - otherPlayer->GetLastTurnPos() ) * .5/otherPlayer->dirDrive.NormSquared();
                    if ( d > maxd )
                        d = maxd;
                    if ( d > 0 )
                    {
                        saved = true;

                        // do the move
                        otherPlayer->MoveSafely( collPos-otherPlayer->dirDrive*d, otherPlayer->LastTime(), otherTime - d/otherPlayer->Speed() );
                        otherPlayer->currentWall->Update( otherPlayer->lastTime, otherPlayer->pos );
                        otherPlayer->dropWallRequested_ = false;

                        // drop our wall so collisions are more accurate
                        dropWallRequested_ = true;
                    }
                }

                // another possibility: if the walls are very short compared to rubber, we could
                // get away with just accounding for some rubber on the cycle that we'd need to kill
                // otherwise.
                if ( !saved && verletSpeed_ >= 0 && this->ThisWallsLength() > 0 )
                {
                    REAL dt = otherTime - time;

                    // this long would the other cycle have to sit in front of our wall
                    // before he's released by the end
                    REAL wallTimeLeft = this->ThisWallsLength()/verletSpeed_ - dt;

                    if ( wallTimeLeft < 0 )
                    {
                        // isn't hit at all
                        return;
                    }

                    // check how much rubber would be used
                    REAL max, effectiveness;
                    sg_RubberValues( otherPlayer->Player(), otherPlayer->verletSpeed_, max, effectiveness );
                    if ( effectiveness > 0 )
                    {
                        REAL rubberToEat = wallTimeLeft * otherPlayer->Speed()/effectiveness;

                        otherPlayer->rubber += rubberToEat;
                        if ( otherPlayer->rubber > max )
                            otherPlayer->rubber = max; // too much rubber used
                        else
                            saved = true;              // within bounds, he may survive
                    }
                }

                if ( !saved && sn_GetNetState() != nCLIENT )
                {
                    // err, trouble. Can't push the other guy back far enough. Better kill him.
                    if ( currentWall )
                        otherPlayer->enemyInfluence.AddWall( currentWall->Wall(), lastTime, 0, otherPlayer );
                    otherPlayer->distance = wallDist;
                    otherPlayer->DropWall();
                    otherPlayer->KillAt( collPos );

                    // get rid of future walls
                    sg_KillFutureWalls( otherPlayer );
                }
            }
        }
        else // sad but true
        {
            // this cycle has to die here unless it has rubber left or is invulnerable (checked on catching the exception, and besides, this code path isn't called for invulnerable cycles)
            throw gCycleDeath( collPos );

            //			REAL dist = w->Pos( a );
            //			const_cast<gPlayerWall*>(w)->BlowHole( dist - explosionRadius, dist + explosionRadius );
        }
    }
    else
    {
        if (bool(player) && sn_GetNetState()!=nCLIENT && Alive() )
        {
            throw gCycleDeath( collPos );
        }

    }
}

REAL gCycle::PathfindingModifier( const eWall *w ) const
{
    if (!w)
        return 1;
    if (dynamic_cast<const gPlayerWall*>(w))
        return .9f;
    else
        return 1;
}


bool gCycle::Act(uActionPlayer *Act, REAL x){
    // don't accept premature input
    if (se_mainGameTimer && ( se_mainGameTimer->speed <= 0 || se_mainGameTimer->Time() < -1 ) )
        return false;

    if (!Alive() && sn_GetNetState()==nSERVER)
        RequestSync(false);

    if(se_turnLeft==*Act && x>.5){
        //SendControl(lastTime,&se_turnLeft,1);
        Turn(-1);
        return true;
    }
    else if(se_turnRight==*Act && x>.5){
        //SendControl(lastTime,&se_turnRight,1);
        Turn(1);
        return true;
    }
    else if(s_brake==*Act){
        //SendControl(lastTime,&brake,x);
        unsigned short newBraking=(x>0);
        if ( braking != newBraking )
        {
            AccelerationDiscontinuity();
            braking = newBraking;
            AddDestination();

            // brake changed, process shooting
            if (sn_GetNetState()!=nCLIENT)
            {
                ProcessShoot(false);
            }
        }
        return true;
    }
    else if(s_brakeToggle==*Act){
        if ( x > 0 )
        {
            AccelerationDiscontinuity();
            braking = !braking;
            AddDestination();

            // brake changed, process shooting
            if (sn_GetNetState()!=nCLIENT)
            {
                ProcessShoot(false);
            }
        }
        return true;
    }
    return false;
}

// client side bugfix: network sync messages get actually used
static nVersionFeature sg_SyncsAreUsed( 5 );

// temporarily override driving directions on wall drops
static eCoord const * sg_fakeDirDrive = NULL;
class gFakeDirDriveSetter
{
public:
    gFakeDirDriveSetter( eCoord const & dir )
            : lastFakeDir_( sg_fakeDirDrive )
    {
        sg_fakeDirDrive = &dir;
    }

    ~gFakeDirDriveSetter()
    {
        sg_fakeDirDrive = lastFakeDir_;
    }
private:
    eCoord const * lastFakeDir_;
};

bool gCycle::DoTurn(int d)
{
#ifdef DELAYEDTURN_DEBUG
    REAL delay = tSysTimeFloat() - sg_turnReceivedTime;
    if ( delay > EPS && sn_GetNetState() == nSERVER && Owner() != 0 )
    {
        con << "Delayed turn execution! " << turns << "\n";
    }
#endif

#ifdef GNUPLOT_DEBUG
    if ( sg_gnuplotDebug && Player() )
    {
        std::ofstream f( Player()->GetUserName() + "_turn", std::ios::app );
        f << pos.x << " " << pos.y << "\n";
    }
#endif

    if (d >  1) d =  1;
    if (d < -1) d = -1;

    if (Alive()){
        if ( turning )
            turning->Reset();

        clientside_action();

        if ( gCycleMovement::DoTurn( d ) )
        {
            sg_ArchiveCoord( pos, 1 );

            skewDot+=4*d;

            if (sn_GetNetState() == nCLIENT && Owner() == ::sn_myNetID)
                AddDestination();

            if (sn_GetNetState()!=nCLIENT)
            {
                RequestSync();
            }

            // hack: while dropping the wall, turn around dirDrive.
            // this makes FindCurrentFace work better.
            {
                FindCurrentFace();
                REAL factor = -16;
                eCoord dirDriveFake = dirDrive * factor;
                eCoord lastDirDriveBack = lastDirDrive;
                lastDirDrive = lastDirDrive * factor;
                gFakeDirDriveSetter fakeSetter( dirDriveFake );
                DropWall();
                lastDirDrive = lastDirDriveBack;
            }

            return true;
        }
    }

    return false;
}

void gCycle::DropWall( bool buildNew )
{
    // keep this cycle alive
    tJUST_CONTROLLED_PTR< gCycle > keep( this->GetRefcount()>0 ? this : 0 );

    // drop last net wall if it is outdated
    if ( lastWall && lastNetWall && lastWall->Time(.5) > lastNetWall->Time(0) )
        lastNetWall = 0;

    // update and drop current wall
    if(currentWall)
    {
        lastWall=currentWall;
        currentWall->Update(lastTime,pos);
        currentWall->CopyIntoGrid( grid );
        currentWall=NULL;
    }

    if ( buildNew && lastTime >= spawnTime_ + sg_cycleWallTime )
        currentWall=new gNetPlayerWall(this,pos,dirDrive,lastTime,distance);

    // grid datastructures change on inserting a wall, better recheck
    // all game objects. Temporarily override this cycle's driving direction.
    eCoord dirDriveBack = dirDrive;
    if ( sg_fakeDirDrive )
        dirDrive = *sg_fakeDirDrive;

    if ( grid )
    {
        for(int i=grid->GameObjects().Len()-1;i>=0;i--)
        {
            eGameObject * c = grid->GameObjects()(i);
            if (c->CurrentFace() && !c->CurrentFace()->IsInGrid() )
                c->FindCurrentFace();
        }
    }
    dirDrive = dirDriveBack;

    // reset flag
    dropWallRequested_ = false;
}

static bool sg_deathShot = true;
static tSettingItem<bool> conf_deathShot ("DEATH_SHOT", sg_deathShot);

static bool sg_selfDestruct = false;
static tSettingItem<bool> conf_selfDestruct ("SELF_DESTRUCT", sg_selfDestruct);

static float sg_selfDestructRadius = 1;
static tSettingItem<float> conf_selfDestructRadius ("SELF_DESTRUCT_RADIUS", sg_selfDestructRadius);

static float sg_selfDestructRotation = 5;
static tSettingItem<float> conf_selfDestructRotation ("SELF_DESTRUCT_ROT", sg_selfDestructRotation);

static float sg_selfDestructRise = 1;
static tSettingItem<float> conf_selfDestructRise ("SELF_DESTRUCT_RISE", sg_selfDestructRise);

static float sg_selfDestructFall = 1;
static tSettingItem<float> conf_selfDestructFall ("SELF_DESTRUCT_FALL", sg_selfDestructFall);

static bool sg_zombieZone = false;
static tSettingItem<bool> conf_zombieZone ("ZOMBIE_ZONE", sg_zombieZone);

static float sg_zombieZoneRadius = 1;
static tSettingItem<float> conf_zombieZoneRadius ("ZOMBIE_ZONE_RADIUS", sg_zombieZoneRadius);

static float sg_zombieZoneRotation = 5;
static tSettingItem<float> conf_zombieZoneRotation ("ZOMBIE_ZONE_ROT", sg_zombieZoneRotation);

static float sg_zombieZoneRise = 1;
static tSettingItem<float> conf_zombieZoneRise ("ZOMBIE_ZONE_RISE", sg_zombieZoneRise);

static float sg_zombieZoneFall = 1;
static tSettingItem<float> conf_zombieZoneFall ("ZOMBIE_ZONE_FALL", sg_zombieZoneFall);

void gCycle::Killed(gCycle *pKiller, int type)
{
    if ((sg_zombieZone) &&
        (pKiller) &&
        (pKiller->Player()) &&
        (pKiller->Alive()) &&
        (pKiller != this) &&
        (pKiller->Player()->CurrentTeam() != Player()->CurrentTeam()))
    {
        //Someone killed us - rise from the dead and eat their brains
        gDeathZoneHack *pZone = new gDeathZoneHack(grid, pos, true);
        if (pZone)
        {
            if (sg_zombieZoneRise > 0)
            {
                pZone->SetRadius(0);
                pZone->SetExpansionSpeed(sg_zombieZoneRise);
            }
            else
            {
                pZone->SetRadius(sg_zombieZoneRadius);

                if (sg_zombieZoneFall > 0)
                {
                    pZone->SetExpansionSpeed(-sg_zombieZoneFall);
                }
            }

            pZone->SetRotationSpeed(sg_zombieZoneRotation);
            pZone->SetColor(color_);

            pZone->SetOwner(Player());
            pZone->SetSeekingCycle(pKiller);
            pZone->SetType(gDeathZoneHack::TYPE_ZOMBIE_ZONE);
            pZone->SetTargetRadius(sg_zombieZoneRadius);
            pZone->SetFallSpeed(sg_zombieZoneFall);
            pZone->RequestSync();
        }
    }

    if ((sg_selfDestruct) &&
        (sg_selfDestructRadius > 0) &&
        (sg_selfDestructRise >= 0) &&
        (sg_selfDestructFall >= 0))
    {
        gDeathZoneHack *pZone = new gDeathZoneHack(grid, pos, true);
        if (pZone)
        {
            if (sg_selfDestructRise > 0)
            {
                pZone->SetRadius(0);
                pZone->SetExpansionSpeed(sg_selfDestructRise);
            }
            else
            {
                pZone->SetRadius(sg_selfDestructRadius);

                if (sg_selfDestructFall > 0)
                {
                    pZone->SetExpansionSpeed(-sg_selfDestructFall);
                }
            }

            pZone->SetRotationSpeed(sg_selfDestructRotation);
            pZone->SetColor(color_);
            pZone->SetOwner(Player());
            pZone->SetType(gDeathZoneHack::TYPE_SELF_DESTRUCT);
            pZone->SetTargetRadius(sg_selfDestructRadius);
            pZone->SetFallSpeed(sg_selfDestructFall);
            pZone->RequestSync();
        }
    }
}

void gCycle::Kill(){
    // keep this cycle alive
    tJUST_CONTROLLED_PTR< gCycle > keep( this->GetRefcount()>0 ? this : 0 );

    if (sn_GetNetState()!=nCLIENT){
        RequestSync(true);

        // check if we own a flag
        if (flag_)
        {
            // tell the flag we dropped it
            flag_->OwnerDropped();
        }

        if (Alive() && grid && GOID() >= 0 ){
            if (sg_deathShot)
            {
                if (braking)
                {
                    braking = 0;
                    ProcessShoot(true);
                }
            }
            lastShotTime = 0;
            shotStarted = 0;
            shotReservoir = 1.0;

            Die( lastTime );
            tNEW(gExplosion)(grid, pos,lastTime, color_, this );
            //	 eEdge::SeethroughHasChanged();

            if ( currentWall )
            {
                // z-man: updating the wall so it reflects exactly the position of death looks like
                // a good idea, but unfortunately, the collision position reported from above
                // is inaccurate. It's better not to use it at all, or the cycle's wall will stick out
                // a bit on the other side of the wall it crashed into.

                // but if prediction was active, do it anyway
                if ( currentWall->Pos(1) > distance || currentWall->Time(1) > lastTime )
                    currentWall->Update( lastTime, pos );

                // copy the wall into the grid, but not directly; the grid datastructures are probably currently traversed. Kill() is called from eGameObject::Move().
                currentWall->CopyIntoGrid( 0 );

                currentWall = NULL;
            }
        }
    }
    // z-man: another stupid idea. Why would we need a destination when we're dead?
    //    else if (Owner() == ::sn_myNetID)
    //        AddDestination();
    /*
      else if (owner!=::sn_myNetID)
      speed=-.01;
    */
}

/*
void gCycle::Turbo(bool turbo){
    if (turbo && speed<30){
        speed=40;
        Turn(0);
    }

    if (!turbo && speed>=30){
        speed=20;
        Turn(0);
    }
}
*/

static rFileTexture cycle_shad(rTextureGroups::TEX_FLOOR,"textures/shadow.png",0,0,true);
/*
static tString lala_cycle_shad("Anonymous/original/textures/shadow.png");
static nSettingItem<tString> lalala_cycle_shad("TEXTURE_CYCLE_SHADOW", lala_cycle_shad);
rFileTexture cycle_shad(rTextureGroups::TEX_FLOOR, lala_cycle_shad, 0,0,true);
*/

#define ENABLE_OLD_LAG_O_METER

REAL sg_laggometerScale=1;
static tSettingItem< REAL > sg_laggometerScaleConf( "LAG_O_METER_SCALE", sg_laggometerScale );
REAL sg_laggometerThreshold=.5;
static tSettingItem< REAL > sg_laggometerThresholdConf( "LAG_O_METER_THRESHOLD", sg_laggometerThreshold );
REAL sg_laggometerBlend=.5;
static tSettingItem< REAL > sg_laggometerBlendConf( "LAG_O_METER_BLEND", sg_laggometerBlend );
#ifdef ENABLE_OLD_LAG_O_METER
bool sg_laggometerUseOld=false;
static tSettingItem< bool > sg_laggometerUseOldConf( "LAG_O_METER_USE_OLD", sg_laggometerUseOld );
#endif
bool sg_axesIndicator=false;

int sg_blinkFrequency=10;
static tSettingItem< int > sg_blinkFrequencyConf( "CYCLE_BLINK_FREQUENCY", sg_blinkFrequency );

#ifndef DEDICATED
// put meriton's classes into a namespace so they can't possibly conflict with other code, especially the Colour class. --wrtl
namespace gLaggometer {
class Colour {
public:
    REAL cp[3];
    Colour(REAL r, REAL g, REAL b) {
        cp[0]=r;
        cp[1]=g;
        cp[2]=b;
    }
    Colour(ePlayerNetID* player) {
        if ( player )
        {
            player->Color(cp[0], cp[1], cp[2]);
        }
        else
        {
            cp[0]=cp[1]=cp[2]=1;
        }
    }
    void blend(REAL factor, const Colour& target) {
        for (int i=0; i<3; i++) {
            cp[i] = (1 - factor) * cp[i] + factor * target.cp[i];
        }
    }
    void toGl() const { glColor3f(cp[0], cp[1], cp[2]); }

    static const Colour white;
    static const Colour black;
};

const Colour Colour::white(1,1,1);
const Colour Colour::black(0,0,0);

class DirectionTransformer {
private:
    eGrid* grid;
    eCoord factor;
public:
    DirectionTransformer(eGrid* theGrid) : grid(theGrid), factor(theGrid->GetDirection(0).Conj()) { }
    eCoord get(int i) {
        return grid->GetDirection(i).Turn(factor);

    }
    int ahead() { return 2 * (grid->WindingNumber()); }
};

class LagOMeterRenderer {
private:
    DirectionTransformer directions;
    REAL delay;
    Colour color;
protected:
    bool drawTriangle(eCoord loc, int winding, REAL lag, int inc);
public:
    LagOMeterRenderer(gCycle* cycle) :
            directions(cycle->Grid()),
            delay(cycle->GetTurnDelay()),
            color(cycle->Player())
    {
        color.blend(sg_laggometerBlend, Colour::white);
    }
    void render(REAL lag);
};

//! returns whether the sprial intersects its counterpart
bool LagOMeterRenderer::drawTriangle(eCoord loc, int winding, REAL lag, int inc) {
    eCoord outer = loc + directions.get(winding) * lag;
    if (outer.y * inc > 0.01f) {
        eCoord oldOuter = loc + directions.get(winding - inc) * lag;
        eCoord d = outer - oldOuter;
        outer = oldOuter + d * (-oldOuter.y / d.y);
        glVertex2f(outer.x, outer.y);
        return true;
    } else {
        glVertex2f(outer.x, outer.y);
        if (lag > delay) {
            if (drawTriangle(loc + directions.get(winding + inc) * delay, winding + inc, lag - delay, inc)) return true;
        } else {
            outer = loc + directions.get(winding + inc) * lag;
            glVertex2f(outer.x, outer.y);
        }
        glVertex2f(loc.x, loc.y);
        return false;
    }
}

void LagOMeterRenderer::render(REAL lag) {
    color.toGl();
    BeginLineStrip();
    drawTriangle(eCoord(0,0), directions.ahead(), lag, 1);
    RenderEnd();

    BeginLineStrip();
    drawTriangle(eCoord(0,0), directions.ahead(), lag, -1);
    RenderEnd();
}


class AxesIndicator {
private:
    DirectionTransformer directions;
    Colour color;
public:
    AxesIndicator(gCycle* cycle) :
            directions(cycle->Grid()),
            color(cycle->Player())
    {
        color.blend(.5f, Colour::white);
    }
    void line(int i) {
        eCoord midle = directions.get(directions.ahead() + i) * .1f;
        eCoord inner = midle * .5f;
        eCoord outer = midle + directions.get(directions.ahead() + 2 * i) * .05f;

        BeginLineStrip();
        //Colour::black.toGl();
        color.toGl();
        glVertex2f(inner.x, inner.y);


        glVertex2f(midle.x, midle.y);

        Colour::black.toGl();
        glVertex2f(outer.x, outer.y);
        RenderEnd();
    }
    void render() {
        //return; // disable, for now
        glShadeModel(GL_SMOOTH);
        line(-1);
        line(0);
        line(1);
    }
};

}

static REAL mp_eWall_stretch=4;
static tSettingItem<REAL> mpws
("MOVIEPACK_WALL_STRETCH",mp_eWall_stretch);

static rFileTexture dir_eWall(rTextureGroups::TEX_WALL,"textures/dir_wall.png",1,0,1);
static rFileTexture dir_eWall_moviepack(rTextureGroups::TEX_WALL,"moviepack/dir_wall.png",1,0,1);

static void dir_eWall_select()
{
    if (sg_MoviePack()){
        TexMatrix();
        IdentityMatrix();
        ScaleMatrix(1/mp_eWall_stretch,1,1);
        dir_eWall_moviepack.Select();
    }
    else
    {
        dir_eWall.Select();
    }
}

gCycleWallsDisplayListManager::gCycleWallsDisplayListManager()
    : wallList_(0)
    , wallsWithDisplayList_(0)
    , wallsWithDisplayListMinDistance_(0)
    , wallsInDisplayList_(0)
{
}

bool gCycleWallsDisplayListManager::CannotHaveList( REAL distance, gCycle const * cycle )
{
    return
            ( !cycle->Alive() && gCycle::WallsStayUpDelay() >= 0 && se_GameTime()-cycle->DeathTime()-gCycle::WallsStayUpDelay() > 0 )

            ||

            ( cycle->ThisWallsLength() > 0 && cycle->GetDistance() - cycle->ThisWallsLength() > distance );
}

void gCycleWallsDisplayListManager::RenderAllWithDisplayList( eCamera const * camera, gCycle * cycle )
{
    dir_eWall_select();

    glDisable(GL_CULL_FACE);

    gNetPlayerWall * run = 0;
    // transfer walls with display list into their list

    int wallsWithPossibleDisplayList = 0;
    run = wallList_;
    while( run )
    {
        gNetPlayerWall * next = run->Next();
        if ( run->CanHaveDisplayList() )
        {
            wallsWithPossibleDisplayList++;
        }
        else
        {
            // wall has expired, remove it
            if ( cycle->ThisWallsLength() > 0 && cycle->GetDistance() - cycle->MaxWallsLength() > run->EndPos() )

            {
                run->Remove();
            }
        }
        run = next;
    }

    // clear display list if needed
    bool tailExpired=false;
    if ( CannotHaveList( wallsWithDisplayListMinDistance_, cycle ) )
    {
        tailExpired=true;
        displayList_.Clear(0);
    }
    // check if enough new walls are present to warrant altering the display list
    else if ( wallsWithPossibleDisplayList >= 3 ||
         wallsWithPossibleDisplayList * 5 > wallsInDisplayList_ )
    {
        // yes? Ok, rebuild the list in this case, too
        displayList_.Clear(0);
    }

    // call display list
    if ( displayList_.Call() )
    {
        return;
    }

    // remove and render walls without display list
    run = wallsWithDisplayList_;
    while( run )
    {
        gNetPlayerWall * next = run->Next();
        if ( !run->CanHaveDisplayList() || ( tailExpired && wallsWithDisplayListMinDistance_ >= run->BegPos() ) )
        {
            run->Insert( wallList_ );
        }
        run = next;
    }

    if ( wallsWithPossibleDisplayList > 0 )
    {
        run = wallList_;
        while( run )
        {
            gNetPlayerWall * next = run->Next();
            if ( run->CanHaveDisplayList() )
            {
                run->Insert( wallsWithDisplayList_ );

                // clear the wall's own display list, it will no longer be needed
                run->ClearDisplayList(0, -1);
            }

            run = next;
        }
    }

    if ( !wallsWithDisplayList_ )
    {
        return;
    }

    // fill display list
    rDisplayListFiller filler( displayList_ );

    if ( rDisplayList::IsRecording() )
    {
        wallsWithDisplayListMinDistance_ = 1E+30;
        wallsInDisplayList_ = 0;

        // bookkeeping of walls in the display list
        run = wallsWithDisplayList_;
        while( run )
        {
            gNetPlayerWall * next = run->Next();
            if ( run->BegPos() < wallsWithDisplayListMinDistance_ )
            {
                wallsWithDisplayListMinDistance_ = run->BegPos();
            }
            wallsInDisplayList_++;
            run = next;
        }
    }

    // render walls with display list
    RenderAll( camera, cycle, wallsWithDisplayList_ );
}

void gCycleWallsDisplayListManager::RenderAll( eCamera const * camera, gCycle * cycle, gNetPlayerWall * list )
{
    if( !list )
    {
        return;
    }

    // first, render all lines
    sr_DepthOffset(true);
    if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
        glDisable(GL_TEXTURE_2D);
    
    gNetPlayerWall * run = list;
    while( run )
    {
        gNetPlayerWall * next = run->Next();
        run->RenderList( true, gNetPlayerWall::gWallRenderMode_Lines );
        run = next;
    }

    RenderEnd();
    sr_DepthOffset(false);
    if ( rTextureGroups::TextureMode[rTextureGroups::TEX_WALL] != 0 )
        glEnable(GL_TEXTURE_2D);
    
    run = list;
    while( run )
    {
        gNetPlayerWall * next = run->Next();
        run->RenderList( true, gNetPlayerWall::gWallRenderMode_Quads );
        run = next;
    }

    RenderEnd();
}

void gCycleWallsDisplayListManager::RenderAll( eCamera const * camera, gCycle * cycle )
{
    // render everything you can with a display list
    RenderAllWithDisplayList( camera, cycle );

    // then, render the rest
    RenderAll( camera, cycle, wallList_ );
}

void gCycle::Render(const eCamera *cam){
    /*
    // for use when there's rendering problems on one specific occasion
    static int counter = 0;
    ++ counter;
    if ( counter == -1 )
    {
        st_Breakpoint();
    }
    */

    // are we blinking from invulnerability?
    bool blinking = false;
    if ( lastTime > spawnTime_ && !Vulnerable() )
    {
        double time = tSysTimeFloat();
        double wrap = time - floor(time);
        int pulse = int ( 2 * wrap * sg_blinkFrequency );
        blinking = pulse & 1;
    }

#ifdef USE_HEADLIGHT
#ifdef LINUX
    typedef void (*glProgramStringARB_Func)(GLenum, GLenum, GLsizei, const void*);
    glProgramStringARB_Func glProgramStringARB_ptr = 0;

    typedef void (*glProgramLocalParameter4fARB_Func)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    glProgramLocalParameter4fARB_Func glProgramLocalParameter4fARB_ptr = 0;

    glProgramStringARB_ptr = (glProgramStringARB_Func) SDL_GL_GetProcAddress("glProgramStringARB");
    glProgramLocalParameter4fARB_ptr = (glProgramLocalParameter4fARB_Func) SDL_GL_GetProcAddress("glProgramLocalParameter4fARB");
#endif
#endif
    if (!finite(z) || !finite(pos.x) ||!finite(pos.y)||!finite(dir.x)||!finite(dir.y)
            || !finite(skew))
        st_Breakpoint();
    if (Alive()){
        //con << "Drawing cycle at " << pos << '\n';

#ifdef DEBUG
        /*     {
        	   gDestination *l = destinationList;
        	   glDisable(GL_LIGHTING);
        	   glColor3f(1,1,1);
        	   while(l){
        	   if (l == currentDestination)
        	   glColor3f(0,1,0);

        	   glBegin(GL_LINES);
        	   glVertex3f(l->position.x, l->position.y, 0);
        	   glVertex3f(l->position.x, l->position.y, 100);
        	   glEnd();

        	   if (l == currentDestination)
        	   glColor3f(0,1,1);

        	   l=l->next;
        	   }
        	   } */
#endif

        GLfloat color[4]={1,1,1,1};
        static GLfloat lposa[4] = { 320, 240, 200,0};
        static GLfloat lposb[4] = { -240, -100, 200,0};
        static GLfloat lighta[4] = { 1, .7, .7, 1 };
        static GLfloat lightb[4] = { .7, .7, 1, 1 };

        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,color);
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,color);

        glLightfv(GL_LIGHT0, GL_DIFFUSE, lighta);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lighta);
        glLightfv(GL_LIGHT0, GL_POSITION, lposa);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, lightb);
        glLightfv(GL_LIGHT1, GL_SPECULAR, lightb);
        glLightfv(GL_LIGHT1, GL_POSITION, lposb);


        ModelMatrix();
        glPushMatrix();
        eCoord p = PredictPosition();
        glTranslatef(p.x,p.y,0);
        glScalef(.5f,.5f,.5f);


        eCoord ske(1,skew);
        ske=ske*(1/sqrt(ske.NormSquared()));

        GLfloat m[4][4]={{dir.x,dir.y,0,0},
                         {-dir.y,dir.x,0,0},
                         {0,0,1,0},
                         {0,0,0,1}};
        glMultMatrixf(&m[0][0]);

        glPushMatrix();
        //glTranslatef(-1.84,0,0);
        if (!mp)
            glTranslatef(-1.5,0,0);

        glPushMatrix();

        GLfloat sk[4][4]={{1,0,0,0},
                          {0,ske.x,ske.y,0},
                          {0,-ske.y,ske.x,0},
                          {0,0,0,1}};

        glMultMatrixf(&sk[0][0]);


        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHTING);



        TexMatrix();
        IdentityMatrix();

        if (mp){

            ModelMatrix();
            if ( !blinking )
            {
                glPushMatrix();
                customTexture->Select();
                glColor3f(1,1,1);
                customModel->Render();
                glPopMatrix();
            }

            glPopMatrix();
            glTranslatef(-1.5,0,0);
        }
        else{
            glEnable(GL_TEXTURE_2D);

            ModelMatrix();

            if ( !blinking )
            {
                bodyTex->Select();
                body->Render();

                wheelTex->Select();

                glPushMatrix();
                glTranslatef(0,0,.73);

                GLfloat mr[4][4]={{rotationRearWheel.x,0,rotationRearWheel.y,0},
                                  {0,1,0,0},
                                  {-rotationRearWheel.y,0,rotationRearWheel.x,0},
                                  {0,0,0,1}};


                glMultMatrixf(&mr[0][0]);

                rear->Render();
                glPopMatrix();

                glPushMatrix();
                glTranslatef(1.84,0,.43);

                GLfloat mf[4][4]={{rotationFrontWheel.x,0,rotationFrontWheel.y,0},
                                  {0,1,0,0},
                                  {-rotationFrontWheel.y,0,rotationFrontWheel.x,0},
                                  {0,0,0,1}};

                glMultMatrixf(&mf[0][0]);

                front->Render();
                glPopMatrix();
            }

            glPopMatrix();
        }


        //     TexMatrix();
        //     IdentityMatrix();
        ModelMatrix();

        /*
          glDisable(GL_TEXTURE_GEN_S);
          glDisable(GL_TEXTURE_GEN_T);
          glDisable(GL_TEXTURE_GEN_Q);
          glDisable(GL_TEXTURE_GEN_R);
        */

        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHTING);

        //glDisable(GL_TEXTURE);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1,1,1);

        {
            bool renderPyramid = false;
            gRealColor colorPyramid;
            REAL alpha = 1;
            const REAL timeout = .5f;

            if ( bool(player) )
            {
                if ( player->IsChatting() )
                {
                    renderPyramid = true;
                    colorPyramid.b = 0.0f;
                }
                else if ( !player->IsActive() )
                {
                    renderPyramid = true;
                    colorPyramid.b = 0.0f;
                    colorPyramid.g = 0.0f;
                }
                else if ( cam && cam->Center() == this && se_GameTime() < timeout && player->CurrentTeam() && player->CurrentTeam()->NumPlayers() > 1 )
                {
                    renderPyramid = true;
                    alpha = timeout - se_GameTime();
                }
            }

            if ( renderPyramid )
            {
                GLfloat s=sin(lastTime);
                GLfloat c=cos(lastTime);

                GLfloat m[4][4]={{c,s,0,0},
                                 {-s,c,0,0},
                                 {0,0,1,0},
                                 {0,0,1,1}};

                glPushMatrix();

                glMultMatrixf(&m[0][0]);
                glScalef(.5,.5,.5);


                BeginTriangles();

                glColor4f( colorPyramid.r,colorPyramid.g,colorPyramid.b, alpha );
                glVertex3f(0,0,3);
                glVertex3f(0,1,4.5);
                glVertex3f(0,-1,4.5);

                glColor4f( colorPyramid.r * .7f,colorPyramid.g * .7f,colorPyramid.b * .7f, alpha );
                glVertex3f(0,0,3);
                glVertex3f(1,0,4.5);
                glVertex3f(-1,0,4.5);

                RenderEnd();

                glPopMatrix();
            }
        }

#ifdef USE_HEADLIGHT
        // Headlight contributed by Jonathan
        if(headlights) {
            if(!cycleprograminited) { // set to false on every sr_InitDisplay, without it I lost my program when I switched to windowed
                const char *program =
                    "!!ARBfp1.0\
                    \
                    PARAM normal = program.local[0];\
                    ATTRIB texcoord = fragment.texcoord;\
                    TEMP final, diffuse, distance;\
                    \
                    DP3 distance, texcoord, texcoord;\
                    RSQ diffuse, distance.w;\
                    RCP distance, distance.w;\
                    MUL diffuse, texcoord, diffuse;\
                    DP3 diffuse, diffuse, normal;\
                    MUL final, diffuse, distance;\
                    MOV result.color.w, fragment.color;\
                    MUL result.color.xyz, fragment.color, final;\
                    \
                    END";
#ifdef LINUX
                glProgramStringARB_ptr(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program), program);
#else
                glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program), program);
#endif
                cycleprograminited = true;
            }
#ifdef LINUX
            glProgramLocalParameter4fARB_ptr(GL_FRAGMENT_PROGRAM_ARB, 0, 0, 0, verletSpeed_ * verletSpeed_, 0);
#else
            glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, 0, 0, verletSpeed_ * verletSpeed_, 0);
#endif
            glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // blend func and depth mask. Efficient or not, glPushAttrib/glPopAttrib is a quick way to manage state.
            glEnable(GL_FRAGMENT_PROGRAM_ARB); // doesn't check if it exists...

            const unsigned sensors = 32; // actually one more
            const double mul = 0.25 * M_PI / sensors;
            const double add = -0.125 * M_PI;

            double size = gArena::SizeMultiplier() * 500 * M_SQRT2; // is M_SQRT2 in your math.h?
            GLfloat array[sensors+2][5];

            array[0][0] = 0;
            array[0][1] = 0;
            array[0][2] = p.x;
            array[0][3] = p.y;
            array[0][4] = 0.125;

            for(unsigned i=0; i<=sensors; i++) {
                gSensor sensor(this, p, dir.Turn(cos(i * mul + add), sin(i * mul + add)));
                sensor.detect(size);
                array[i][5] = sensor.before_hit.x - p.x;
                array[i][6] = sensor.before_hit.y - p.y;
                array[i][7] = sensor.before_hit.x;
                array[i][8] = sensor.before_hit.y;
                array[i][9] = 0.125;
            }

            glPushMatrix();
            glLoadIdentity();

            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glTranslatef(0, 0, 1);

            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);

            glColor3fv(reinterpret_cast<GLfloat *>(&color_)); // 8-)
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);

            glInterleavedArrays(GL_T2F_V3F, 0, array);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sensors+2);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);

            glDisable(GL_FRAGMENT_PROGRAM_ARB);

            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);

            glPopMatrix();
            glPopAttrib();
        }
#endif // USE_HEADLIGHT
        // Name
        RenderName( cam );


        // shadow:

        sr_DepthOffset(true);


        REAL h=0;//se_cameraZ*.005+.03;

        glEnable(GL_CULL_FACE);

        if(!blinking && sr_floorDetail>rFLOOR_GRID && rTextureGroups::TextureMode[rTextureGroups::TEX_FLOOR]>0 && sr_alphaBlend){
            glColor3f(0,0,0);
            cycle_shad.Select();
            BeginQuads();
            glTexCoord2f(0,1);
            glVertex3f(-.6,.4,h);

            glTexCoord2f(1,1);
            glVertex3f(-.6,-.4,h);

            glTexCoord2f(1,0);
            glVertex3f(2.1,-.4,h);

            glTexCoord2f(0,0);
            glVertex3f(2.1,.4,h);

            RenderEnd();
        }

        glDisable(GL_CULL_FACE);

        // sr_laggometer;


        REAL f=verletSpeed_;

        REAL l=Lag();

        glPopMatrix();

        h=cam->CameraZ()*.005+.03;

#ifdef ENABLE_OLD_LAG_O_METER
        if(sg_laggometerUseOld) {
            if (sn_GetNetState() != nSTANDALONE && sr_laggometer && f*l>.5) {
                //&& owner!=::sn_myNetID){
                glPushMatrix();

                glColor3f(1,1,1);
                //glDisable(GL_TEXTURE);
                glDisable(GL_TEXTURE_2D);

                glTranslatef(0,0,h);
                //glScalef(.5*f,.5*f,.5*f);

                // compensate for the .5 scaling further up
                f *= 2 * sg_laggometerScale;

                glScalef(f,f,f);

                // move the sr_laggometer ahead a bit
                if (!sr_predictObjects || sn_GetNetState()==nSERVER)
                    glTranslatef(l,0,0);


                BeginLineLoop();


                glVertex2f(-l,-l);
                glVertex2f(0,0);
                glVertex2f(-l,l);
                REAL delay = GetTurnDelay();
                if(l> 2*delay){
                    glVertex2f(-2*l+delay,delay);
                    glVertex2f(-2*l+2*delay,0);
                    glVertex2f(-2*l+delay,-delay);
                }
                else if (l>delay){
                    glVertex2f(-2*l+delay,delay);
                    glVertex2f(-l,2*delay-l);
                    glVertex2f(-l,-(2*delay-l));
                    glVertex2f(-2*l+delay,-delay);
                }

                RenderEnd();
                glPopMatrix();
            }
        } else
#endif
        {
            glPushMatrix();

            //glDisable(GL_TEXTURE);
            glDisable(GL_TEXTURE_2D);

            glTranslatef(0,0,h);
            //glScalef(.5*f,.5*f,.5*f);

            // compensate for the .5 scaling further up
            f *= 2 * sg_laggometerScale;

            glScalef(f,f,f);

            // move the sr_laggometer back a bit
            if (sr_predictObjects || sn_GetNetState()==nSERVER) {
                glTranslatef(-l,0,0);
            }

            if (f*l>sg_laggometerThreshold) {
                if (sr_laggometer) {
                    gLaggometer::LagOMeterRenderer(this).render(l);
                }
            } else if(sg_axesIndicator) {
                gLaggometer::AxesIndicator(this).render();
            }

            glPopMatrix();
        }
        sr_DepthOffset(false);

        glPopMatrix();

    }
}

static REAL fadeOutNameAfter = 5.0f;	/* 0: never show, < 0 always show */
//static int fadeOutNameMode = 1;			// 0: never show, 1: show for fadeOutNameAfter, 2: always show
static bool showOwnName = 0;			// show name on own cycle?

static tSettingItem< bool > sg_showOwnName( "SHOW_OWN_NAME", showOwnName );
//static tSettingItem< int > sg_fadeOutNameMode( "FADEOUT_NAME_MODE", showOwnName )
static tSettingItem< REAL > sg_fadeOutNameAfter( "FADEOUT_NAME_DELAY", fadeOutNameAfter );


void gCycle::RenderName( const eCamera* cam ) {
    if ( !this->Player() )
        return;

    float modelviewMatrix[16], projectionMatrix[16];
    float x, y, z, w;
    float xp, yp, wp;
    float alpha = 0.75;

    if (fadeOutNameAfter == 0) return; /* XXX put that in ::Render() */
    if ( !cam->RenderingMain() ) return; // no name in mirrored image
    if ( !showOwnName && cam->Player() == this->player ) return; // don't show own name

    glPushMatrix();
    /* position sign above center of cycle */
    glTranslatef(0.8, 0, 2.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
    glPopMatrix();

    /* get coordinates of sign */
    x = modelviewMatrix[12];
    y = modelviewMatrix[13];
    z = modelviewMatrix[14];
    w = modelviewMatrix[15];

    /* multiply by projection matrix */
    xp = projectionMatrix[0] * x + projectionMatrix[4] * y +
         projectionMatrix[8] * z + projectionMatrix[12] * w;
    yp = projectionMatrix[1] * x + projectionMatrix[5] * y +
         projectionMatrix[9] * z + projectionMatrix[13] * w;
    wp = projectionMatrix[3] * x + projectionMatrix[7] * y +
         projectionMatrix[11] * z + projectionMatrix[15] * w;

    if (wp <= 0) {
        /* behind camera */
        timeCameIntoView = 0;
        return;
    }

    xp /= wp;
    yp /= wp;
    yp += rCHEIGHT_NORMAL;// * 2.0;

    if (xp <= -1 || xp >= 1 || yp <= -1 || yp >= 1) {
        /* out of screen */
        timeCameIntoView = 0;
        return;
    }

    /* it is visible */

    if (fadeOutNameAfter > 0) {
        REAL now = tSysTimeFloat();
        if (timeCameIntoView == 0)
            timeCameIntoView = now;

        if (now - timeCameIntoView > fadeOutNameAfter) {
            return;
        } else if (now - timeCameIntoView > fadeOutNameAfter - 1) {
            /* start to fade out */
            alpha = 0.75 - (now - timeCameIntoView -
                            (fadeOutNameAfter - 1)) * 0.75;
        }
    }

    ModelMatrix();
    glPushMatrix();
    glLoadIdentity();

    ProjMatrix();
    glPushMatrix();
    glLoadIdentity();

    glColor4f(1, 1, 1, alpha);
    DisplayText(xp, yp, rCWIDTH_NORMAL, rCHEIGHT_NORMAL, this->player->GetName(), 0, 0);

    ProjMatrix();
    glPopMatrix();

    ModelMatrix();
    glPopMatrix();
}


bool gCycle::RenderCockpitFixedBefore(bool){
    /*
      if (alive)
      return true;
      else{
      REAL rd=se_GameTime()-deathTime;
      if (rd<1)
         return true;
      else{
         REAL d=1.25-rd;
         d*=8;
         if (d<0) d=0;
         glColor3f(d,d/2,d/4);
         glDisable(GL_TEXTURE_2D);
         glDisable(GL_TEXTURE);
         glDisable(GL_DEPTH_TEST);
         glRectf(-1,-1,1,1);
         glColor4f(1,1,1,rd*(2-rd/2));
         DisplayText(0,0,.05,.15,"You have been deleted.");
         return false;
      }
      }
    */
    return true;
}

void gCycle::SoundMix(Uint8 *dest,unsigned int len,
                      int viewer,REAL rvol,REAL lvol){
    if (Alive()){
        /*
          if (!cycle_run.alt){
          rvol*=4;
          lvol*=4;
          }
        */

        if (engine)
            engine->Mix(dest,len,viewer,rvol,lvol,verletSpeed_/(sg_speedCycleSound * SpeedMultiplier()));

        if (turning)
        {
            if (turn_wav.alt)
                turning->Mix(dest,len,viewer,rvol,lvol,5);
            else
                turning->Mix(dest,len,viewer,rvol,lvol,1);
        }

        if (spark)
            spark->Mix(dest,len,viewer,rvol*.5,lvol*.5,4);
    }
}
#endif

eCoord gCycle::PredictPosition() const {
    return predictPosition_;

    //    eCoord p = pos + dir * (speed * se_PredictTime());
    //    return p + correctPosSmooth;
}

eCoord gCycle::CamPos() const
{
    return PredictPosition() + dir.Turn(0 ,-skew*z);

    //    gSensor s(this,pos, PredictPosition() - pos );
    //    s.detect(1);

    //    return s.before_hit + dir.Turn(0 ,-skew*z);

    // return pos + dir.Turn(0 ,-skew*z);
}

eCoord  gCycle::CamTop() const
{
    return dir.Turn(0,-skew);
}


#ifdef POWERPAK_DEB
void gCycle::PPDisplay(){
    int R=int(r*255);
    int G=int(g*255);
    int B=int(b*255);
    PD_PutPixel(DoubleBuffer,
                se_X_ToScreen(pos.x),
                se_Y_ToScreen(pos.y),
                PD_CreateColor(DoubleBuffer,R,G,B));
    /*
      PD_PutPixel(DoubleBuffer,
      se_X_ToScreen(pos.x+1),
      se_Y_ToScreen(pos.y),
      PD_CreateColor(DoubleBuffer,R,G,B));
      PD_PutPixel(DoubleBuffer,
      se_X_ToScreen(pos.x-1),
      se_Y_ToScreen(pos.y),
      PD_CreateColor(DoubleBuffer,R,G,B));
      PD_PutPixel(DoubleBuffer,
      se_X_ToScreen(pos.x),
      se_Y_ToScreen(pos.y+1),
      PD_CreateColor(DoubleBuffer,R,G,B));
      PD_PutPixel(DoubleBuffer,
      se_X_ToScreen(pos.x),
      se_Y_ToScreen(pos.y-1),
      PD_CreateColor(DoubleBuffer,R,G,B));
    */
}
#endif

//! draws it in a svg file
void gCycle::DrawSvg(std::ofstream &f) {
    double alpha = 1;
    if(!Alive()) {
        alpha -= 2 * (se_GameTime() - DeathTime());
        if(alpha <= 0) return;
    }
    eCoord p = PredictPosition(), dir = Direction();
    float a = Grid()->GetWindingAngle(WindingNumber())*180/M_PI;
    f << "  <use xlink:href='#cycle' fill='" << gSvgColor(color_) << "' transform=\"translate(" << pos.x << " " << -pos.y
      << ") rotate(" << -a << ")\" opacity=\"" << alpha << "\" />\n";
    if (Player()) {
        eCoord v = p - eCoord(SvgOutput::cx,SvgOutput::cy);
        v.Normalize();
        float tx = p.x + v.x * 2;
        float ty = p.y + v.y * 3;

        f << "  <text x='" << tx << "' y='" << -ty << "' stroke='#333333' stroke-width='.2' fill='" << gSvgColor(color_) << "' text-anchor='"
          << (tx<SvgOutput::cx?"end":"start") << "' opacity='" << alpha << "' font-size='8' font-family = 'fantasy'>";
        tString const &n = Player()->GetName();
        for( char *c = &n(0); *c != 0; ++c )
        {
            unsigned char u = *c;
            if ( u=='<' ) f << "&lt;";
            else if ( u=='>' ) f << "&gt;";
            else if( u < ' ' );
            else if( u > '~' && u < 0xa0 );
            else f << *c;
        }
        f << "</text>\n";
    }
}


// cycle network routines:
gCycle::gCycle(nMessage &m)
        :gCycleMovement(m),
        engine(NULL),
        turning(NULL),
        spark(NULL),
        skew(0),skewDot(0),
        rotationFrontWheel(1,0),rotationRearWheel(1,0),heightFrontWheel(0),heightRearWheel(0),
        m_target_mgr(this),
        currentWall(NULL),
        lastWall(NULL)
{
    cycles.push_back(this);

    deathTime=0;
    lastNetWall=lastWall=currentWall=NULL;
    windingNumberWrapped_ = windingNumber_ = Grid()->DirectionWinding(dirDrive);
    dirDrive = Grid()->GetDirection(windingNumberWrapped_);
    dir = dirDrive;

    rubber=0;
    //correctTimeSmooth =0;
    correctDistanceSmooth =0;
    //correctSpeedSmooth =0;

    deathTime = 0;
    spawnTime_ = se_GameTime() + 100;

    m >> color_.r;
    m >> color_.g;
    m >> color_.b;

    trailColor_ = color_;

    se_MakeColorValid( color_.r, color_.g, color_.b, 1.0f );
    se_MakeColorValid( trailColor_.r, trailColor_.g, trailColor_.b, .5f );

    // set last time so that the first read_sync will not think this is old
    lastTimeAnim = lastTime = -EPS;

    nextSync = nextSyncOwner = -1;
    flag_ = NULL;
    lastSyncOwnerGameTime_ = 0;
}


void gCycle::WriteCreate(nMessage &m){
    eNetGameObject::WriteCreate(m);
    m << color_.r;
    m << color_.g;
    m << color_.b;
}

static nVersionFeature sg_verletIntegration( 7 );

void gCycle::WriteSync(nMessage &m){
    //	eNetGameObject::WriteSync(m);

    if( SyncedUser() == Owner() )
    {
        lastSyncOwnerGameTime_ = lastTime;
    }

    if ( Alive() )
    {
        m << lastTime;
    }
    else
    {
        m << deathTime;
    }
    m << Direction();
    m << Position();

    REAL speed = verletSpeed_;
    // if the clients understand it, send them the real current speed
    if ( sg_verletIntegration.Supported() )
        speed = Speed();

#ifdef DEBUG
    if ( speed > 15 )
    {
        int x;
        x = 0;
    }
#endif

    m << speed;
    m << short( Alive() ? 1 : 0 );
    m << distance;
    if (!currentWall || currentWall->preliminary)
        m.Write(0);
    else
        m.Write(currentWall->ID());

    m.Write(turns);
    m.Write(braking);

    // write last turn position
    m << GetLastTurnPos();

    // write rubber
    compressZeroOne.Write( m, rubber/( sg_rubberCycle + .1 ) );
    compressZeroOne.Write( m, 1/( 1 + rubberMalus ) );

    // write last clientside sync message ID
    unsigned short lastMessageID = 0;
    if ( lastDestination )
        lastMessageID = lastDestination->messageID;
    m.Write(lastMessageID);

    // write brake
    compressZeroOne.Write( m, brakingReservoir );

    // set new sync times
    // nextSync=tSysTimeFloat()+sg_syncIntervalEnemy;
    // nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
}

bool gCycle::SyncIsNew(nMessage &m){
    bool ret=eNetGameObject::SyncIsNew(m);


    REAL dummy2;
    short al;
    unsigned short Turns;

    m >> dummy2;
    m >> al;
    m >> dummy2;
    m.Read(Turns);

#ifdef DEBUG_X
    con << "received sync for player " << player->GetName() << "  ";
    if (ret || al!=1){
        con << "accepting..\n";
        return true;
    }
    else
    {
        con << "rejecting..\n";
        return false;
    }
#endif

    return ret || al!=1;
}

void gCycle::RequestSyncOwner()
{
    // no more syncs when you're dead
    if ( !Alive() )
    {
        return;
    }

    // nothing to do on the client or if the cycle belongs to an AI
    if ( sn_GetNetState() != nSERVER || Owner() == 0 )
        return;

    REAL syncInterval = sg_GetSyncIntervalSelf( this );
    if ( nextSyncOwner < tSysTimeFloat() + syncInterval * 2.0 )
    {
        // postpone next sync so the notal number of syncs stays the same
        RequestSync( Owner(), false );
        nextSyncOwner += syncInterval;
    }
}

void gCycle::RequestSyncAll()
{
    // no more syncs when you're dead
    if ( !Alive() )
    {
        return;
    }

    // nothing to do on the client or if the cycle belongs to an AI
    if ( sn_GetNetState() != nSERVER || Owner() == 0 )
        return;

    REAL syncInterval = sg_syncIntervalEnemy;
    if ( nextSync < tSysTimeFloat() + syncInterval * 2.0 )
    {
        // postpone next sync so the notal number of syncs stays the same
        RequestSync( false );
        nextSync += syncInterval;
    }
}

// resets the extrapolator to the last known state
void gCycle::ResetExtrapolator()
{
    resimulate_ = false;
    if (!extrapolator_)
    {
        extrapolator_ = tNEW( gCycleExtrapolator )(grid, pos, dir );
    }

    extrapolator_->CopyFrom( lastSyncMessage_, *this );

    // simulate a bit, only to get current rubberSpeedFactor and acceleration
    extrapolator_->TimestepCore( extrapolator_->LastTime(), true );
}

// simulate the extrapolator at higher speed
bool gCycle::Extrapolate( REAL dt )
{
    tASSERT( extrapolator_ );

#ifdef DEBUG
    eCoord posBefore = extrapolator_->Position();
#endif

    // calculate target time
    REAL newTime = extrapolator_->LastTime() + dt;

    bool ret = false;

    // clamp: don't simulate further than our current time
    if ( newTime >= lastTime )
    {
        // simulate extrapolator until now
        if( lastTime > extrapolator_->LastTime() )
        {
            eGameObject::TimestepThis( lastTime, extrapolator_ );
        }

        // test if there are real (the check for list does that) destinations left; we cannot call it finished if there are.
        gDestination* unhandledDestination = extrapolator_->GetCurrentDestination();
        ret = !unhandledDestination || !unhandledDestination->list;

        if( !ret )
        {
            if ( unhandledDestination->gameTime < newTime - Lag() * 2 - sn_Connections[0].ping.GetPing()*2 - GetTurnDelay()*4 )
            {
                // emergency reset.
                extrapolator_ = 0;
                resimulate_ = true;
            }
        }

        newTime = lastTime;
    }
    else
    {
        // simulate extrapolator as requested
        eGameObject::TimestepThis( newTime, extrapolator_ );
    }

    //eCoord posAfter = extrapolator_->Position();
    //eDebugLine::SetTimeout( 1.0 );
    //eDebugLine::SetColor( 1,0,0 );
    //eDebugLine::Draw( posBefore, 8, posAfter, 4 );
    //eDebugLine::Draw( posBefore, 4, posAfter, 4 );

    return ret;
}

// makes sure the given displacement does not cross walls
void se_SanifyDisplacement( eGameObject* base, eCoord& displacement )
{
    eCoord base_pos = base->Position();
    // eCoord reachable_pos = base->Position() + displacement;

    int timeout = 5;
    while( timeout > 0 )
    {
        --timeout;

        // cast ray fron sync_pos to reachable_pos
        gSensor test( base, base_pos, displacement );
        test.detect(1);

        if ( timeout == 0 )
        {
            // emergency exit; take something closely before the wall
            displacement = ( displacement ) * ( test.hit * .99 );
            break;
        }

        if ( !test.ehit )
        {
            // path is clear
            break;
        }
        else
        {
#ifdef DEBUG
            // see if the wall that was hit is nearly parallel to the desired move. maybe we should add
            // special code for that. However, the projection idea seems to be universally robust.
            float p = test.ehit->Vec() * displacement;
            float m = se_EstimatedRangeOfMult( test.ehit->Vec(), displacement );
            if ( fabs(p) < m * .001 )
            {
                con << "Almost missed wall during gCycle::ReadSync positon update";
            }
#endif

            // project reachable_pos to the line that was hit
            REAL alpha = test.ehit->Ratio( base_pos + displacement );
            displacement = *test.ehit->Point() + test.ehit->Vec() * alpha - base_pos;

            // move it a bit closer to the known good position
            displacement = displacement * .99;
        }
    }
}

void gCycle::TransferPositionCorrectionToDistanceCorrection()
{
    REAL newCorrectDist = eCoord::F( correctPosSmooth, dirDrive );
    distance += newCorrectDist - correctDistanceSmooth;
    correctDistanceSmooth = newCorrectDist;
}

// take over the extrapolator's data
void gCycle::SyncFromExtrapolator()
{
    // store old position
    eCoord oldPos = pos;

    // con << "Copy: " << LastTime() << ", " << extrapolator_->LastTime() << ", " << extrapolator_->Position() << ", " << pos << "\n";

    tASSERT( extrapolator_ );

    // delegate
    CopyFrom( *extrapolator_ );

    // adjust current wall (not essential, don't do it for the first wall)
    if ( currentWall && currentWall->tBeg > spawnTime_ + sg_cycleWallTime + .01f )
    {
        // update start position
        currentWall->beg = extrapolator_->GetLastTurnPos();

        // set begin distance as well
        REAL dBeg = extrapolator_->GetDistance() - eCoord::F( extrapolator_->Direction(), extrapolator_->Position() - extrapolator_->GetLastTurnPos() );

        currentWall->dbegin = dBeg;
        currentWall->coords_[0].Pos = dBeg;

        // and care for consistency
        int i;
        for ( i = currentWall->coords_.Len() -1 ; i>=0; --i )
        {
            gPlayerWallCoord & coord = currentWall->coords_( i );
            if ( coord.Pos <= dBeg )
                coord.Pos = dBeg;
        }
    }

    // transfer last turn position
    lastTurnPos_ = extrapolator_->GetLastTurnPos();

    // smooth position correction
    correctPosSmooth = correctPosSmooth + oldPos - pos;

#ifdef DEBUG
    if ( correctPosSmooth.NormSquared() > .1f )
    {
        std::cout << "Lag slide! " << correctPosSmooth << "\n";
//        resimulate_ = true;
    }
#endif

    // calculate time difference between this cycle and extrapolator
    REAL dt = this->LastTime() - extrapolator_->LastTime();

    // extrapolate true distance ( the best estimate we have for the distance on the server )
    REAL trueDistance = extrapolator_->trueDistance_ + extrapolator_->Speed() * dt;

    // update distance correction
    // con << 	correctDistanceSmooth << "," << trueDistance << "," << distance << "\n";
    // correctDistanceSmooth = trueDistance - distance;
    distance = trueDistance;
    correctDistanceSmooth=0;

    // split away part in driving direction
    TransferPositionCorrectionToDistanceCorrection();

    // make sure correction does not bring us to the wrong side of a wall
    // se_SanifyDisplacement( this, correctPosSmooth );

    //eDebugLine::SetTimeout( 1.0 );
    //eDebugLine::SetColor( 0,1,0 );
    //eDebugLine::Draw( pos, 4, pos + dirDrive * 10, 14 );

    // delete extrapolator
    extrapolator_ = 0;
}

static int sg_useExtrapolatorSync=1;
static tSettingItem<int> sg_useExtrapolatorSyncConf("EXTRAPOLATOR_SYNC",sg_useExtrapolatorSync);

// make sure no correction moves the cycle backwards beyond the beginning of the last wall
void ClampForward( eCoord& newPos, const eCoord& startPos, const eCoord& dir )
{
    REAL forward = eCoord::F( newPos - startPos, dir )/dir.NormSquared();
    if ( forward < 0 )
        newPos = newPos - dir * forward;
}

extern REAL sg_cycleBrakeRefill;
extern REAL sg_cycleBrakeDeplete;

void gCycle::ReadSync( nMessage &m )
{
    // data from sync message
    SyncData sync;

    short sync_alive;               // is this cycle alive?
    unsigned short sync_wall=0;     // ID of wall

    // eCoord new_pos = pos;	// the extrapolated position

    // warning: depends on the implementation of eNetGameObject::WriteSync
    // since we don't call eNetGameObject::ReadSync.
    m >> sync.time;

    // reset values not sent with old protocol messages
    sync.rubber = rubber;
    sync.turns = turns;
    sync.braking = braking;
    sync.messageID = 1;

    m >> sync.dir;
    m >> sync.pos;

    //eDebugLine::SetTimeout( 1.0 );
    //eDebugLine::SetColor( 1,1,1 );
    //eDebugLine::Draw( lastSyncMessage_.pos, 0, lastSyncMessage_.pos, 20 );

    m >> sync.speed;
    m >> sync_alive;
    m >> sync.distance;
    m.Read(sync_wall);
    if (!m.End())
        m.Read(sync.turns);
    if (!m.End())
        m.Read(sync.braking);

    if ( !m.End() )
    {
        m >> sync.lastTurn;
    }
    else if ( currentWall )
    {
        sync.lastTurn = currentWall->beg;
    }

    bool canUseExtrapolatorMethod = false;

    bool rubberSent = false;
    if ( !m.End() )
    {
        rubberSent = true;

        // read rubber
        REAL preRubber, preRubberMalus;
        preRubber = compressZeroOne.Read( m );
        preRubberMalus = compressZeroOne.Read( m );

        // read last message ID
        m.Read(sync.messageID);

        // read braking reservoir
        sync.brakingReservoir = compressZeroOne.Read( m );
        // std::cout << "sync: " << sync.brakingReservoir << ":" << sync.braking << "\n";

        // undo skewing
        sync.rubber = preRubber * ( sg_rubberCycle + .1 );
        sync.rubberMalus = 1/preRubberMalus - 1;

        // extrapolation is probably safe
        canUseExtrapolatorMethod = sg_useExtrapolatorSync && lastTime > 0;
    }
    else
    {
        // try to extrapolate brake status backwards in time
        sync.brakingReservoir = brakingReservoir;
        if ( brakingReservoir > 0 && sync.braking )
            sync.brakingReservoir += ( lastTime - sync.time ) * sg_cycleBrakeDeplete;
        else if ( brakingReservoir < 1 && !sync.braking )
            sync.brakingReservoir -= ( lastTime - sync.time ) * sg_cycleBrakeRefill;

        if ( sync.brakingReservoir < 0 )
            sync.brakingReservoir = 0;
        else if ( sync.brakingReservoir > 1 )
            sync.brakingReservoir = 1;
    }

    // abort if sync is not new
    if ( lastSyncMessage_.time >= sync.time && lastSyncMessage_.turns >= sync.turns && sync_alive > 0 )
    {
        //eDebugLine::SetTimeout( 5 );
        //eDebugLine::SetColor( 1,1,0);
        //eDebugLine::Draw( lastSyncMessage_.pos, 1.5, lastSyncMessage_.pos, 5.0 );
        return;
    }
    lastSyncMessage_ = sync;

    // store last known good position: a bit before the last position confirmed by the server
    lastGoodPosition_ = sync.pos + ( sync.lastTurn - sync.pos ) *.01;
    // offset it a tiny bit by our last driving direction
    if ( eCoord::F( dirDrive, sync.dir ) > .99f*dirDrive.NormSquared() )
        lastGoodPosition_ = lastGoodPosition_ - this->lastDirDrive * .0001;

    //eDebugLine::SetTimeout( 2 );
    //eDebugLine::SetColor( 0,1,0);
    //eDebugLine::Draw( lastSyncMessage_.pos, 1.5, lastSyncMessage_.pos, 5.0 );

    // con << "Sync: " << lastTime << ", " << lastSyncMessage_.time << ", " << lastSyncMessage_.pos << ", " << pos << "\n";

    if ( canUseExtrapolatorMethod )
    {
        // reset extrapolator if information from server is more up to date
        if ( extrapolator_ && extrapolator_->LastTime() < lastSyncMessage_.time )
        {
            extrapolator_ = 0;
        }
    }

    // killed?
    if (Alive() && sync_alive!=1 && GOID() >= 0 && grid )
    {
        Die( lastSyncMessage_.time );
        MoveSafely( lastSyncMessage_.pos, lastTime, deathTime );
        distance=lastSyncMessage_.distance;
        correctDistanceSmooth=0;
        DropWall( false );

        tNEW(gExplosion)( grid, lastSyncMessage_.pos, lastSyncMessage_.time ,color_, this );

        return;
    }

    // no point going on if you're not alive
    if (!Alive())
    {
#ifdef DEBUG
        con << "Received duplicate death sync message; those things confuse old clients!\n";
#endif
        return;
    }

    gDestination emergency_aft(*this);

    // all the data was read. check where it fits in our destination list:
    gDestination *bef= GetDestinationBefore( lastSyncMessage_, destinationList );
    gDestination *aft=NULL;
    if (bef){
        aft=bef->next;
        if (!aft)
            aft=&emergency_aft;

        //eDebugLine::SetTimeout(1);
        //eDebugLine::SetColor( 1,0,0 );
        //eDebugLine::Draw( bef->position, 1.5, lastSyncMessage_.pos, 3.0 );
        //eDebugLine::SetColor( 0,1,0 );
        //eDebugLine::Draw( lastSyncMessage_.pos, 3.0, aft->position, 5.0 );

    }


    // detect local tunneling by casting a ray from the server certified position
    // to the next known client position
    if ( lastTime > 0 )
    {
        eCoord position = pos;
        if ( bef )
            position = bef->position;
        eSensor tunnel( this, lastSyncMessage_.pos, position - lastSyncMessage_.pos );
        tunnel.detect( 1 );

        // use extrapolation to undo local tunneling
        if ( tunnel.ehit )
        {
            canUseExtrapolatorMethod = true;
            if ( 0 )
            {
                con << "possible local tunneling detected\n";
                eSensor tunnel( this, lastSyncMessage_.pos, position - lastSyncMessage_.pos );
                tunnel.detect( 1 );
            }
        }
    }
    else
    {
        // first sync. Accept the position without questioning it.
        pos = sync.pos;
        FindCurrentFace();
    }

    // determine whether we can use the distance based interpolating sync method here
    bool distanceBased = aft && aft != &emergency_aft && Owner() == sn_myNetID;

    if ( canUseExtrapolatorMethod && Owner()==sn_myNetID )
    {
        // exactlu resimulate from sync position for cycles controlled by this client
        resimulate_ = true;

        return;
    }

    rubber = lastSyncMessage_.rubber;

    // bool fullSync = false;

    // try to get a distance value closer to the client's data by calculating the distance from the sync position to the next destination
    REAL interpolatedDistance = lastSyncMessage_.distance;
    if ( aft )
    {
        interpolatedDistance = aft->distance - sqrt((lastSyncMessage_.pos-aft->position).NormSquared());
    }

    // determine true lag
    // REAL lag = 1;
    //if ( player )
    //    lag = player->ping;

    if ( distanceBased )
    {
        // new way: correct our time and speed

#ifdef DEBUG
        // destination *save = bef;
#endif

        REAL ratio = (interpolatedDistance - bef->distance)/
                     (aft->distance - bef->distance);

        if (!finite(ratio))
            ratio = 0;

        // interpolate when the cycle was at the position the sync message was sent
        REAL interpolatedTime = bef->gameTime * (1-ratio) + aft->gameTime * ratio;
        // REAL interpolatedSpeed = bef->speed   * (1-ratio) + aft->speed    * ratio;

        // calculate deltas
        REAL correctTime  = ( lastSyncMessage_.time     - interpolatedTime     );
        // REAL correctSpeed = ( lastSyncMessage_.speed    - interpolatedSpeed    );
        REAL correctDist  = ( lastSyncMessage_.distance - interpolatedDistance );

        // don't trust high ratios too much; they may be skewed by rubber application
        {
            REAL factor = (1 - ratio) * 5;
            if ( factor < 1 )
            {
                if ( factor < 0 )
                    factor = 0;
                correctTime *= factor;
                correctDist *= factor;
            }
        }

        //if (correctTime > 0)
        //{
        //    correctTime*=.5;
        //    correctSpeed*=.5;
        //    correctDist*=.5;
        //}

        //correctTimeSmooth       += correctTime;
        //correctSpeedSmooth      += correctSpeed;
        // correctDistanceSmooth   += correctDist;

        // correctDistanceSmooth   += correctDist;

        // con << ratio << ", " << correctDist << ", " << correctTime << "\n";

        // correct distances according to sync
        {
            distance += correctDist;
            gDestination * run = bef;
            while ( run )
            {
                run->distance += correctDist;
                run = run->next;
            }
        }

        // correct time by adapting position and distance
        eCoord newPos = pos - Direction() * Speed() * correctTime;
        if ( currentWall )
        {
            ClampForward( newPos, currentWall->beg, Direction() );
        }

        // don't tunnel through walls
        {
            const eCoord & safePos = pos; // aft->position
            gSensor test( this, safePos , newPos - safePos );
            test.detect(1);
            if ( test.ehit )
            {
                newPos = test.before_hit;

                // something bad must be going on, better recheck with accurate extrapolation
                resimulate_ = true;
            }
        }

        correctPosSmooth = correctPosSmooth + pos - newPos;
        distance += eCoord::F( newPos - pos, Direction() )/Direction().NormSquared();

        MoveSafely( newPos, lastTime, lastTime );

        /*
        REAL ts = lastSyncMessage_.time - lastTime;

        //        eCoord intPos = pos + dirDrive * (ts * speed + .5 * ts*ts*acceleration);
        eCoord intPos = pos + dirDrive * ( ts * ( speed + lastSyncMessage_.speed ) * .5 );
        REAL  int_speed = speed + acceleration*ts;

        dirDrive = lastSyncMessage_.dir;

        correctPosSmooth = lastSyncMessage_.pos - intPos;

        distance = lastSyncMessage_.distance - speed * ts - acceleration * ts*ts*.5;

        //correctTimeSmooth = 0;
        */
    }
    else
    {
        // direct sync
        if ( Owner() != sn_myNetID )
        {
            // direct extrapolation for cycles of other clients or if no turn is newer than the sync
            SyncEnemy( lastSyncMessage_.lastTurn );

            // update beginning of current wall
            if ( currentWall )
            {
                currentWall->beg = lastSyncMessage_.lastTurn;
            }

            // update brake status
            AccelerationDiscontinuity();
            braking = lastSyncMessage_.braking;

            // store last turn
            lastTurnPos_ = lastSyncMessage_.lastTurn;
        }
        else
        {
            // same algorithm, but update smooth position correction so that there is no immediate visual change
            eCoord oldPos = pos + correctPosSmooth;
            SyncEnemy( lastSyncMessage_.lastTurn );
            correctPosSmooth = oldPos - pos;
        }

        // restore rubber meter
        if ( !rubberSent )
        {
            rubber = lastSyncMessage_.rubber;
        }
    }

    // if this happens during creation, ignore position correction
    if ( this->ID() == 0 )
    {
        correctPosSmooth = eCoord();

        // some other stuff that should happen on the first sync

        // estimate time of spawning (HACK)
        spawnTime_=lastTime;
        if ( verletSpeed_ > 0 )
            spawnTime_ -= distance/verletSpeed_;

        // set spawn time to infinite past if this is the first spawn
        if ( !sg_cycleFirstSpawnProtection && spawnTime_ <= 1.0 )
        {
            spawnTime_ = -1E+20;
        }

        // reset position and direction
        predictPosition_ = pos;
        dir = dirDrive;
        skew = skewDot = 0;
        lastDirDrive=dirDrive;
        lastTurnPos_=pos;
    }
#ifdef DEBUG
    else
        if ( correctPosSmooth.NormSquared() > .1f && lastTime > 0.0 )
        {
            std::cout << "Lag slide! " << correctPosSmooth << "\n";
            int x;
            x = 0;
        }
#endif

    sn_Update(turns,lastSyncMessage_.turns);

    //if (fabs(correctTimeSmooth > 5))
    //    st_Breakpoint();

    // calculate new winding number. Try to change it as little as possible.
    this->SetWindingNumberWrapped( Grid()->DirectionWinding(dirDrive) );

    // Resnap to the axis
    dirDrive = Grid()->GetDirection(windingNumberWrapped_);
}

void gCycle::SyncEnemy ( const eCoord& )
{
    // keep this cycle alive
    tJUST_CONTROLLED_PTR< gCycle > keep( this->GetRefcount()>0 ? this : 0 );

    resimulate_ = false;

    // calculate turning
    bool turned = false;
    REAL turnDirection=( dirDrive*lastSyncMessage_.dir );
    REAL notTurned=eCoord::F( dirDrive, lastSyncMessage_.dir )/dirDrive.NormSquared();

    // the last known time
    REAL lastKnownTime = lastTime;

    // calculate the position of the last turn from the sync data
    if ( distance > 0 && ( notTurned < .99 || this->turns < lastSyncMessage_.turns ) )
    {
        // reset sound
        if (turning)
            turning->Reset();

        // update old wall as good as we can
        eCoord crossPos = lastSyncMessage_.pos;
        REAL crossDist = lastSyncMessage_.distance;
        REAL crossTime = lastSyncMessage_.time;

        // calculate intersection of old and new trajectory (if only one turn was made)
        // the second condition is for old servers that don't send the turn count; they
        // don't support multiple axes, so we can possibly detect missed turns if
        // the dot product of the current and last direction is negative.
        if (this->turns+1 >= lastSyncMessage_.turns && ( lastSyncMessage_.turns > 0 || notTurned > -.5 ) )
        {
            if ( fabs( turnDirection ) > .01 )
            {
                REAL b = ( crossPos - pos ) * dirDrive;
                REAL distplace = b/turnDirection;
                crossPos = crossPos + lastSyncMessage_.dir * distplace;
                crossDist += distplace;
                if ( lastSyncMessage_.speed > 0 )
                    crossTime += distplace / lastSyncMessage_.speed;

                tASSERT( fabs ( ( crossPos - pos ) * dirDrive ) < 1 );
                tASSERT( fabs ( ( crossPos - lastSyncMessage_.pos ) * lastSyncMessage_.dir ) < 1 );

                // update the old wall
                if (currentWall)
                    currentWall->Update(crossTime,crossPos);
            }
        }
        else
        {
            // a turn sync was dropped for whatever reason. The last wall is not reliable.
            // make it disappear immediately.
            if (currentWall)
            {
                currentWall->real_CopyIntoGrid(grid);
            }
        }

        eDebugLine::SetTimeout(5);
        eDebugLine::SetColor( 1,0,0 );
        eDebugLine::Draw( crossPos, 0, crossPos, 8 );

        // drop old wall
        if(currentWall){
            lastWall=currentWall;
            currentWall->CopyIntoGrid( grid );
            tControlledPTR< gNetPlayerWall > bounce( currentWall );
            currentWall=NULL;
        }

        // create new wall at sync location
        distance = lastSyncMessage_.distance;
        correctDistanceSmooth=0;

        REAL startBuildWallAt = spawnTime_ + sg_cycleWallTime;
        if ( crossTime > startBuildWallAt )
            currentWall=new gNetPlayerWall
                        (this,crossPos,lastSyncMessage_.dir,crossTime,crossDist);

        turned = true;

        // save last driving direction
        lastDirDrive = dirDrive;

        // move to cross position
        MoveSafely( crossPos, lastTime, crossTime );
        lastKnownTime = crossTime;
    }

    // side bending
    skewDot+=4*turnDirection;

    // calculate timestep
    // REAL ts = lastSyncMessage_.time - lastTime;

    // backup current time
    REAL oldTime = lastTime;

    // update position, speed, distance and direction
    MoveSafely( lastSyncMessage_.pos, lastKnownTime, lastSyncMessage_.time );
    verletSpeed_  = lastSyncMessage_.speed;
    lastTimestep_ = 0;
    distance = lastSyncMessage_.distance;
    correctDistanceSmooth=0;
    dirDrive = lastSyncMessage_.dir;
    rubber = lastSyncMessage_.rubber;
    brakingReservoir = lastSyncMessage_.brakingReservoir;

    // update time to values from sync
    lastTime = lastSyncMessage_.time;
    if ( oldTime < 0 )
        oldTime = lastTime;
    clientside_action();

    // bend last driving direction if it is antiparallel to the current one
    if ( lastDirDrive * dirDrive < EPS && eCoord::F( lastDirDrive, dirDrive ) < 0 )
    {
        lastDirDrive = dirDrive.Turn(0,1);
    }

    // correct laggometer to actual facts
    if (Owner() != sn_myNetID )
    {
        // calculate lag from sync delay
        REAL lag = se_GameTime() - lastSyncMessage_.time;
        if ( lag < 0 )
            lag = 0;

        // try not to let the lag jump
        REAL maxLag = laggometer * 1.2;
        REAL minLag = laggometer * .8;

        // store it
        laggometer = lag;

        // clamp smooth laggometer to it (or set it if a turn was made)
        //if ( laggometerSmooth > lag )
        //    laggometerSmooth = lag;

        // For the client and without prediction:
        // do not simulate if a turn was made, just accept sync as it is
        // (so turns don't look awkward)
        if (
            sn_GetNetState()==nCLIENT && turned )
        {
            laggometerSmooth = lag;

            // but at least update the current wall
            if ( currentWall )
                currentWall->Update(lastTime,pos);

            // and reset the animation time
            lastTimeAnim=lastTime;

            return;
        }
        else
        {
            // sanity check: the lag should not jump up suddenly
            // (and if it does, we'll catch up at the next turn)
            // REAL maxLag = 1.2 * laggometerSmooth;
            if ( laggometer > maxLag )
                laggometer = maxLag;
            if ( laggometer < minLag )
                laggometer = minLag;
        }
    }

    // simulate to extrapolate, but don't touch the smooth laggometer
    {
        REAL laggometerSmoothBackup = this->laggometerSmooth;
        TimestepThis(oldTime, this);
        this->laggometerSmooth = laggometerSmoothBackup;
    }
}

/*
void gCycle::old_ReadSync(nMessage &m){
  REAL oldTime=lastTime;
  eCoord oldpos=pos;
  //+correctPosSmooth;
  //correctPosSmooth=0;
  eCoord olddir=dir;
  eNetGameObject::ReadSync(m);

  REAL t=(dirDrive*dir);
  if (fabs(t)>.2){
#ifdef DEBUG
    if (owner==sn_myNetID)
      con << "Warning! Turned cycle!\n";
#endif
    turning.Reset();
  }

  // side bending
  skewDot+=4*t;


  dirDrive=dir;
  dir=olddir;

  m >> speed;
  short new_alive;
  m >> new_alive;
  if (alive && new_alive!=1){
    new gExplosion(pos,oldTime,r,g,b);
    deathTime=oldTime;
    eEdge::SeethroughHasChanged();
  }
  alive=new_alive;

  m >> distance;

  // go to old time frame

  eCoord realpos=pos;
  REAL realtime=lastTime;
  REAL realdist=distance;

  if (currentWall)
    lastWall=currentWall;

  m.Read(currentWallID);

  unsigned short Turns;
  if (!m.End())
    m.Read(Turns);
  else
    Turns=turns;

  if (!m.End())
    m.Read(braking);
  else
    braking=false;

  TimestepThis(oldTime,this);

  if (!currentWall || fabs(t)>.3 || currentWall->inGrid){
    //REAL d=eCoord::F(pos-realpos,olddir);
    //eCoord crosspos=realpos+olddir*d;
    //d=eCoord::F(oldpos-crosspos,dir);
    //crosspos=crosspos+dir*d;

    eCoord crosspos=realpos;

    if (currentWall){
      currentWall->Update(realtime,crosspos);
      //currentWall->Update(realtime,realpos);
      currentWall->CopyIntoGrid();
    }
    //con << "NEW\n";
    currentWall=new gNetPlayerWall
      (this,crosspos,dirDrive,realtime,realdist);
  }


  // smooth correction
  if ((oldpos-pos).NormSquared()<4 && fabs(t)<.5){
    correctPosSmooth=pos-oldpos;
    pos=oldpos;
  }



#ifdef DEBUG
  //int old_t=turns;
  //if(sn_Update(turns,Turns))
  //con << "Updated turns form " << old_t << " to " << turns << "\n";
#endif
  sn_Update(turns,Turns);
}
*/

void gCycle::ReceiveControl(REAL time,uActionPlayer *act,REAL x){
    //forceTime=true;
    TimestepThis(time,this);
    Act(act,x);
    //forceTime=false;
}

void gCycle::PrintName(tString &s) const
{
    s << "gCycle nr. " << ID();
    if ( this->player )
    {
        s << " owned by ";
        this->player->PrintName( s );
    }
}

bool gCycle::ActionOnQuit()
{
    //  currentWall = NULL;
    //  lastWall = NULL;
    if ( sn_GetNetState() == nSERVER )
    {
        TakeOwnership();
        Kill();
        return false;
    }
    else
    {
        return true;
    }
}


nDescriptor &gCycle::CreatorDescriptor() const{
    return cycle_init;
}

/*
void gCycle::AddDestination(gDestination *dest){
    //  con << "got new dest " << dest->position << "," << dest->direction
    // << "," << dest->distance << "\n";

    dest->InsertIntoList(&destinationList);

    // if the new destination was inserted at the end of the list
    //if (!currentDestination && !dest->next &&
    //if ((dest->distance >= distance || (!currentDestination && !dest->next)) &&

    if (dest->next && dest->next->hasBeenUsed){
        delete dest;
        return;
    }

    // go back one destination if the new destination appears to be older than the current one
    if ((!currentDestination || currentDestination == dest->next ) &&
            sn_GetNetState()!=nSTANDALONE && Owner()!=::sn_myNetID){
        currentDestination=dest;
        // con << "setting new cd\n";
	}
}
*/

void gCycle::RightBeforeDeath( int numTries )
{
    if ( player )
    {
        player->RightBeforeDeath( numTries );
    }

    // delay syncs for old clients; they would tunnel locally
    if ( sg_avoidBadOldClientSync && !sg_NoLocalTunnelOnSync.Supported( Owner() ) )
    {
        nextSync=tSysTimeFloat()+sg_syncIntervalEnemy;
        nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
    }

    correctPosSmooth = correctPosSmooth * .5;
}

// *******************************************************************************************
// *
// *	DoIsDestinationUsed
// *
// *******************************************************************************************
//!
//!		@param	dest	the destination to test
//!		@return			true if the destination is still in active use
//!
// *******************************************************************************************

bool gCycle::DoIsDestinationUsed( const gDestination * dest ) const
{
    return ( extrapolator_ && extrapolator_->IsDestinationUsed( dest ) ) || gCycleMovement::DoIsDestinationUsed( dest );
}

// *******************************************************************************************
// *
// *   DoGetDistanceSinceLastTurn
// *
// *******************************************************************************************
//!
//!        @return     the distance driven since the last turn
//!
// *******************************************************************************************

/*
REAL gCycle::DoGetDistanceSinceLastTurn( void ) const
{
    if ( currentWall )
    {
        return ( currentWall->Pos(1) - currentWall->Pos(0) );
    }

    return 0;
}
*/

// *******************************************************************************************
// *
// *   DoGetDistanceSinceLastTurn
// *
// *******************************************************************************************
//!
//!        @return     the distance driven since the last turn
//!
// *******************************************************************************************

/*
REAL gCycleExtrapolator::DoGetDistanceSinceLastTurn( void ) const
{
    return eCoord::F( pos - lastTurn_, dirDrive );
}
*/

// *******************************************************************************************
// *
// *	Vulnerable
// *
// *******************************************************************************************
//!
//!		@return		true if the cycle is vulnerable
//!
// *******************************************************************************************

bool gCycle::Vulnerable() const
{
    return Alive() && lastTime > spawnTime_ + sg_cycleInvulnerableTime;
}

static float sg_shotThreshold = 2.0f;
static tSettingItem<float> conf_shotThreshold ("SHOT_THRESH", sg_shotThreshold);
//static nSettingItemWatched<float> conf_shotThreshold("SHOT_THRESH", sg_shotThreshold, nConfItemVersionWatcher::Group_Breaking, 8);

static float sg_shotDiscardTime = 0.0f;
static tSettingItem<float> conf_shotDiscardTime ("SHOT_DISCARD_TIME", sg_shotDiscardTime);

static float sg_shotStartDistance = 1.0f;
static tSettingItem<float> conf_shotStartDistance ("SHOT_START_DIST", sg_shotStartDistance);

static float sg_shotVelocityMultiplier = 1.4f;
static tSettingItem<float> conf_shotVelocityMultiplier ("SHOT_VELOCITY_MULT", sg_shotVelocityMultiplier);

static float sg_shotRadiusMin = 0.75f;
static tSettingItem<float> conf_shotRadiusMin ("SHOT_RADIUS_MIN", sg_shotRadiusMin);

static float sg_shotRadiusMax = 6.0f;
static tSettingItem<float> conf_shotRadiusMax ("SHOT_RADIUS_MAX", sg_shotRadiusMax);

static float sg_shotRotationMin = 1.0f;
static tSettingItem<float> conf_shotRotationMin ("SHOT_ROT_MIN", sg_shotRotationMin);

static float sg_shotRotationMax = 5.0f;
static tSettingItem<float> conf_shotRotationMax ("SHOT_ROT_MAX", sg_shotRotationMax);

static int sg_shotExplosion = 0;
static tSettingItem<int> conf_shotExplosion ("SHOT_EXPLOSION", sg_shotExplosion);

static float sg_megashotThreshold = 0.95f;
static tSettingItem<float> conf_megashotThreshold ("MEGA_SHOT_THRESH", sg_megashotThreshold);

static float sg_megashotMultiplier = 0.5f;
static tSettingItem<float> conf_megashotMultiplier ("MEGA_SHOT_MULT", sg_megashotMultiplier);

static int sg_megashotDirections = 3;
static tSettingItem<int> conf_megashotDirections ("MEGA_SHOT_DIR", sg_megashotDirections);

static int sg_megashotExplosion = 1;
static tSettingItem<int> conf_megashotExplosion ("MEGA_SHOT_EXPLOSION", sg_megashotExplosion);

#define FIX_BRAKE_BUG

void gCycle::ProcessShoot(bool deathShot)
{
    if (Alive())
    {
        if (braking)
        {
#ifndef FIX_BRAKE_BUG
            //Shoot pressed, save the reservoir
            shotStarted = brakingReservoir;
#else
            //Shoot pressed, save the time
            shotStarted = se_mainGameTimer->Time();
#endif
        }
        else
        {
            //Shoot let go

            //Check if the shot should be discarded
            REAL newTime = se_mainGameTimer->Time();

            if ((lastShotTime) &&
                (newTime >= lastShotTime) &&
                (newTime < (lastShotTime + sg_shotDiscardTime)))
            {
                return;
            }

#ifndef FIX_BRAKE_BUG
            //Check the strength from the reservoir
            REAL shotStrength = shotStarted - brakingReservoir;
#else
            //Refill the reservoir
            if (lastShotTime)
            {
                REAL refillTime = shotStarted - lastShotTime;
                if (refillTime < 0.0)
                {
                    return;
                }
                shotReservoir += refillTime * sg_cycleBrakeRefill;
            }
            if (shotReservoir > 1.0)
            {
                shotReservoir = 1.0;
            }

            //Get the deplete time
            REAL depleteTime = newTime - shotStarted;
            if (depleteTime <= 0.0)
            {
                return;
            }

            REAL shotStrength = depleteTime * sg_cycleBrakeDeplete;
            if ((shotReservoir - shotStrength) < 0.0)
            {
                shotStrength = shotReservoir;
                shotReservoir = 0.0;
            }
            else
            {
                shotReservoir -= shotStrength;
            }
#endif

            //Save the new shot time
            lastShotTime = newTime;

            //Get the type of shot
            int type = gDeathZoneHack::TYPE_SHOT;
            if (deathShot)
            {
                type = gDeathZoneHack::TYPE_DEATH_SHOT;
            }

            //Check if color_ is too dark
            gRealColor shotColor = color_;
            REAL colorTotal = shotColor.r + shotColor.g + shotColor.b;
            REAL colorLimit = 0.3;

            if (colorTotal < colorLimit)
            {
                //This could be much improved....
                REAL toAdd = (colorLimit - colorTotal) / 3;
                shotColor.r += toAdd;
                shotColor.g += toAdd;
                shotColor.b += toAdd;
            }

            if ((shotStrength < 0.0) ||
                (shotStrength < sg_shotThreshold))
            {
                return;
            }
            else if (shotStrength < sg_megashotThreshold)
            {
                REAL shotRadius = (shotStrength * (sg_shotRadiusMax - sg_shotRadiusMin)) + sg_shotRadiusMin;
                REAL shotRotation = (shotStrength * (sg_shotRotationMax - sg_shotRotationMin)) + sg_shotRotationMin;

                eCoord shotPos = pos + (dirDrive * (shotRadius + sg_shotStartDistance));
                eCoord shotVelocity = dirDrive * Speed() * sg_shotVelocityMultiplier;

                gDeathZoneHack *pZone = new gDeathZoneHack(grid, shotPos, true);
                pZone->SetRadius(shotRadius);
                pZone->SetVelocity(shotVelocity);
                pZone->SetRotationSpeed(shotRotation);
                pZone->SetColor(shotColor);
                pZone->SetOwner(Player());
                pZone->SetType(type);

                if (sg_shotExplosion == 1)
                {
                    new gExplosion(grid, pos, lastTime, color_, 0 );
                }
                else if (sg_shotExplosion >= 2)
                {
                    new gExplosion(grid, shotPos, lastTime, color_, 0 );
                }
            }
            else
            {
                //Megashot
                REAL shotRadius = (sg_megashotMultiplier * shotStrength *
                                   (sg_shotRadiusMax - sg_shotRadiusMin)) + sg_shotRadiusMin;
                REAL shotRotation = (sg_megashotMultiplier * shotStrength *
                                     (sg_shotRotationMax - sg_shotRotationMin)) + sg_shotRotationMin;

                eCoord tempDirection = dirDrive;

                //Don't shoot if directions are 0
                if (sg_megashotDirections > 0)
                {
                    REAL a,b;
                    REAL radians = 2 * 3.14159265 / sg_megashotDirections;
                    a = cos(radians);
                    b = sin(radians);

                    int i;
                    for (i = 0; i < sg_megashotDirections; i++)
                    {
                        eCoord shotPos = pos + (tempDirection * (shotRadius + sg_shotStartDistance));
                        eCoord shotVelocity = tempDirection * Speed() * sg_shotVelocityMultiplier;

                        gDeathZoneHack *pZone = new gDeathZoneHack(grid, shotPos, true);
                        pZone->SetRadius(shotRadius);
                        pZone->SetVelocity(shotVelocity);
                        pZone->SetRotationSpeed(shotRotation);
                        pZone->SetColor(shotColor);
                        pZone->SetOwner(Player());
                        pZone->SetType(type);

                        if (sg_megashotExplosion >= 2)
                        {
                            new gExplosion(grid, shotPos, lastTime, color_, 0 );
                        }

                        tempDirection = tempDirection.Turn(a, b);
                    }
                }

                if ((sg_megashotExplosion == 1) ||
                    (sg_megashotExplosion >= 3))
                {
                    new gExplosion(grid, pos, lastTime, color_, 0 );
                }
            }
        }
    }
}

void gCycle::SetWallBuilding(bool build) {
	// Cycle must be alive !
	if (!Alive()) return;
	// Get current wall if exists ...
	gNetPlayerWall *nwall = ((CurrentWall())?CurrentWall()->NetWall():0);
	if (build&&!nwall) DropWall(true);
	else if (nwall&&!build) DropWall(false);
	if (nwall) nwall->RequestSync();
	if ((CurrentWall()) && (CurrentWall()->NetWall())) CurrentWall()->NetWall()->RequestSync();
}

void gCycle::TeleportTo(eCoord dest, eCoord dir, REAL time) {
	// Cycle must be alive !
	if (!Alive()) return;
	// Drop current wall if exists, without building a new one ...
	gNetPlayerWall *nwall = ((CurrentWall())?CurrentWall()->NetWall():0);
	if (nwall) {
		DropWall(false);
		nwall->RequestSync();
	}
	// Do a safe move to destination
	MoveSafely(dest,time,time);
	SetLastTurnPos(dest); // hacky way to ensure to build the wall properly
	SetLastTurnTime(time);
	// if dir is not (0,0), do a turn ...
	if ((dir.x != 0.0) || (dir.y != 0.0)) {
		int d = Grid()->DirectionWinding(dir);
        eCoord nextDirDrive = Grid()->GetDirection(d);
		SetWindingNumberWrapped( d );
		lastDirDrive = dirDrive;
		dirDrive = nextDirDrive;
	}
	// If cycle was linked to a wall at start position, build a new wall at new position
	if (!nwall) {
		DropWall();
		if ((CurrentWall()) && (CurrentWall()->NetWall())) CurrentWall()->NetWall()->RequestSync();
	}
	RequestSync();
}

// ***************************
// *** target mode (begin) ***
/*
At start, assigned every players a target
If playerA kills his target, he wins extra points and a new target might be assigned to him
If playerB kills playerA's target, a new target must be assigned to playerA
If playerA's target leave, a new target must be assigned to playerA
wap's idea: make target assignment for limited time, then reassign a new target ...
*/

// _assignment_mode: 0=disable, 1/2 is enable, 1 affects the player killing your target while 2 randomly affects new target
int gTarget::assignment_mode = 0;
int gTarget::base_score = 10;
int gTarget::base_score_deplete = 2;
int gTarget::max_target = 2;
int gTarget::min_cycles = 3;
REAL gTarget::max_distance = .0;
REAL gTarget::timeout_delay = .0;
REAL gTarget::start_time = -.5;

static tSettingItem< int > sg_TargetAssignmentModeConf( "CYCLE_TARGET_MODE", gTarget::assignment_mode );
static tSettingItem<int> sg_TargetScoreConf("CYCLE_TARGET_SCORE", gTarget::base_score);
static tSettingItem<int> sg_TargetScoreDepleteConf("CYCLE_TARGET_SCORE_DEPLETE", gTarget::base_score_deplete);
static tSettingItem<int> sg_TargetMaxConf("CYCLE_TARGET_MAX", gTarget::max_target);
static tSettingItem<int> sg_TargetMinCyclesConf("CYCLE_TARGET_MIN_CYCLES", gTarget::min_cycles);
static nSettingItem<REAL> sg_TargetMaxDistanceConf("CYCLE_TARGET_MAX_DISTANCE", gTarget::max_distance);
static nSettingItem<REAL> sg_TargetTimeoutConf("CYCLE_TARGET_TIMEOUT", gTarget::timeout_delay);
static nSettingItem<REAL> sg_StartTimeConf("CYCLE_TARGET_START_TIME", gTarget::start_time);

eLadderLogWriter sg_targetKilledWriter("CYCLE_TARGET_KILLED", true);

struct gCycle::LessHuntersCount : public std::binary_function<gCycle*, gCycle*, bool> {
    bool operator()(gCycle* lhs, gCycle* rhs) const
    { return lhs->Target().HuntersCount() < rhs->Target().HuntersCount(); }
};

bool gTarget::Set(gCycle *p_cycle)
{
#ifdef DEBUG
    con << "Trying to assign target " << (p_cycle && p_cycle->Player()?static_cast<const char *>(p_cycle->Player()->GetName()):"INVALID") << " to " << m_this->Player()->GetName() << "\n";
#endif
	if ((!assignment_mode) ||
	    (!p_cycle) ||
	    (!p_cycle->Alive()) ||
	    (p_cycle==m_this) ||
	    (p_cycle==m_target) ||
	    (!p_cycle->Player()) ||
	    (p_cycle->Player()->CurrentTeam() == m_this->Player()->CurrentTeam()) ||
//	    (!m_this->Player()->IsHuman()) ||
	    (m_killed_counter >= gTarget::max_target)) return false;

    Unset(false); // this unsure that if a target is already set, it is properly removed from hunters'list
    m_target = p_cycle;
    p_cycle->Target().m_hunters.push_back(m_this);
    m_assignment_time = se_GameTime();
    tOutput out( tOutput("$cycle_target_assignment", m_target->Player()->GetColoredName()) );
    sn_ConsoleOut( out, m_this->Player()->Owner() );
    // send a console message to the player
    sn_CenterMessage(tOutput("$cycle_target_assignment"), m_this->Player()->Owner() );
    con << m_this->Player()->GetName() << ": " << out;
    return true;
}

bool gTarget::Set(ePlayerNetID *p_player)
{
    if ((!p_player) || (!p_player->Object())) return false;
    gCycle *cycle(static_cast<gCycle *>(p_player->Object()));
    return Set(cycle);
}

void gTarget::Unset(bool warn)
{
    if ((!m_target) || (!m_this)) return;
#ifdef DEBUG
    con << "Trying to unset target from " << m_this->Player()->GetName() << "\n";
#endif
    // first remove m_this from m_target's hunters' list
    if (m_target->CheckTargetPtr())
    {
        vec_cycle_ptr &v(m_target->Target().m_hunters);
        gCycleItr pos(std::find(v.begin(), v.end(), m_this));
        if (pos!=v.end()) v.erase(pos);
    }
    // send a message to user if requested
    if (warn)
    {
        tOutput out( tOutput("$cycle_target_cancel", m_target->Player()->GetName()) );
        sn_ConsoleOut( out, m_this->Owner() );
        con << m_this->Player()->GetName() << ": " << out;
    }
    // then clear m_target
    m_target = NULL;
}

void gTarget::Timestep(REAL p_gametime)
{
    // if no target is set and score to be granted is still positive, try to reassign
    // if ((!m_target) && (m_this->Alive()) && (base_score > base_score_deplete*m_killed_counter)) AutoSet();

    // if a target is set, check for timeout
    if ((m_target) && (gTarget::timeout_delay<=0?false:(p_gametime >= (m_assignment_time + gTarget::timeout_delay))))
    {
        tOutput out( tOutput("$cycle_target_timeout", m_target->Player()->GetName()) );
        sn_ConsoleOut( out, m_this->Owner() );
        con << m_this->Player()->GetName() << ": " << out;

        if (!AutoSet(EXCLUDE, m_this))
            if (!AutoSet()) Unset();
    }
}

void gTarget::AddScore()
{
	// target mode must be enable + cycle needs a target to score
	if ((!assignment_mode) || (!m_target)) return;

    tOutput lose;
    tOutput win;
    if (m_this->Player())
    {
        tColoredString preyName;
        preyName << m_target->Player()->GetName();
        preyName << tColoredString::ColorString(1,1,1);
        sg_targetKilledWriter << m_target->Player()->GetUserName() << m_this->Player()->GetUserName();
        sg_targetKilledWriter.write();

        win.SetTemplateParameter(3, preyName);
        win << "$cycle_target_score";
        int score = base_score-base_score_deplete*m_killed_counter;
        if ( score > 0 )
        {
            m_this->Player()->AddScore(score, win, lose );
            ++m_killed_counter;
        }
    }
}

// some predicates
struct IsTargetable : public std::binary_function<gCycle*, gCycle*, bool> {
    bool operator()(gCycle* lhs, gCycle* rhs) const
    {
        if ((!lhs) || (!rhs) || (!lhs->Player()) || (!rhs->Player())) return true;
        return lhs->Player()->CurrentTeam()==rhs->Player()->CurrentTeam();
    }
};

struct IsLowestCounter : public std::binary_function<gCycle*, int, bool> {
    bool operator()(gCycle* lhs, int rhs) const
    {
        return lhs->Target().HuntersCount()==rhs;
    }
};

struct IsCloseEnough : public std::binary_function<gCycle*, gCycle*, bool> {
    bool operator()(gCycle* lhs, gCycle* rhs) const
    {
        if ((!lhs) || (!rhs) || (!lhs->Player()) || (!rhs->Player())) return true;
        return (lhs->Position()-rhs->Position()).NormSquared() <= gTarget::max_distance;
    }
};

// Try to set target automatically
// hint: RANDOM = look for a "random" cycle, FORCE = force p_cycle as target (if possible), EXCLUDE = exclude p_cycle as suitable target
bool gTarget::AutoSet(t_hint p_hint, gCycle *p_cycle)
{
#ifdef DEBUG
    con << "Trying to assign target to " << m_this->Player()->GetName() << "\n";
#endif
//    if ((!assignment_mode) || (!m_this->Player()->IsHuman())) return false;
    if ((!assignment_mode) || (!m_this->Alive())) return false;

	if ((p_hint==FORCE) && (p_cycle)) return Set(p_cycle);

    std::vector<gCycle *> cycles(gCycle::cycles.begin(),gCycle::cycles.end());

    // exclude p_cycle is requested
    if ((p_hint==EXCLUDE) && (p_cycle))
    {
        gCycleItr pos(std::find(cycles.begin(), cycles.end(), p_cycle));
        if (pos!=cycles.end()) cycles.erase(pos);
    }
    // remove any cycle of this cycle's team
    cycles.erase( std::remove_if(cycles.begin(), cycles.end(), std::bind2nd(IsTargetable(), m_this)), cycles.end());

    // no suitable target, every cycles are in the same team.
    if (!cycles.size()) return false;

    // take into account distance if required
    if (max_distance>0)
    {
        gCycleItr new_end = std::partition(cycles.begin(), cycles.end(), std::bind2nd(IsCloseEnough(), m_this));
        if (new_end != cycles.begin())
            cycles.erase(new_end, cycles.end());
    }

    // sort by targeted counter ascendant and keep only less targeted cycles
    std::sort(cycles.begin(), cycles.end(), gCycle::LessHuntersCount());
    gCycle *first = *(cycles.begin());
    int lowest_counter = first->Target().HuntersCount();
    cycles.erase( std::remove_if(cycles.begin(), cycles.end(), not1(std::bind2nd(IsLowestCounter(), lowest_counter))), cycles.end());

    if (cycles.size())
    {
        // finally, scramble vector and get the first one
        std::random_shuffle( cycles.begin(), cycles.end() );
        first = *(cycles.begin());
    }
    return Set(first);
}

// static version of autoset, to address a list of target. This one also states targeting as started.
void gTarget::AutoSetCycles(vec_cycle_ptr &p_cycles, t_hint p_hint, gCycle *p_cycle)
{
#ifdef DEBUG
    con << "Trying to assign target to each cycles...\n";
#endif
    // check if there's still enough cycles to set target
    if (gCycle::AliveCounter()<gTarget::min_cycles) return;

	// well, p_cycles might be changed in this loop so let's start by a local copy
	vec_cycle_ptr l_cycles(p_cycles);
    for (gCycleItr itr = l_cycles.begin(); itr != l_cycles.end(); ++itr)
    {
    	(*itr)->Target().started = true;
        if ((*itr)->Target().AutoSet(p_hint, p_cycle))
            // target assignment failed. take care to unset target to avoid any issue
            (*itr)->Target().Unset();
    }
}

static void sg_targetReset(std::istream &s)
{
	if (!gTarget::assignment_mode) return;

    // First, reset all targets. In this case, we explicitly wants to allocate gTarget
    for (gCycleItr itr = gCycle::cycles.begin(); itr != gCycle::cycles.end(); ++itr)
    {
        (*itr)->Target().Reset();
    }
    // Then, assigns targets to all cycles.
    gTarget::AutoSetCycles(gCycle::cycles);
}

static tConfItemFunc sg_targetResetConf("CYCLE_TARGET_RESET",&sg_targetReset);

static void sg_setTarget(std::istream &s)
{
	if (!gTarget::assignment_mode) return;

	eGrid *grid = eGrid::CurrentGrid();
	if(!grid) {
		con << "Must be called while a grid exists!\n";
		return;
	}

    tString strHunter, strTarget;
    s >> strHunter;
    s >> strTarget;
    ePlayerNetID *pHunter = ePlayerNetID::FindPlayerByName(strHunter, NULL);
    ePlayerNetID *pTarget = ePlayerNetID::FindPlayerByName(strTarget, NULL);
    if (!pHunter) return;
    gCycle *pHunterCycle = dynamic_cast<gCycle *>(pHunter->Object());
    if (!pHunterCycle) return;
    gCycle *pTargetCycle = NULL;
    if (pTarget)
    	pTargetCycle = dynamic_cast<gCycle *>(pTarget->Object());
    if (!pTargetCycle)
    {
    	if (!pHunterCycle->Target().AutoSet())
    	    pHunterCycle->Target().Unset();
    }
    pHunterCycle->Target().Set(pTargetCycle);
}

static tConfItemFunc sg_setTargetConf("CYCLE_TARGET_SET",&sg_setTarget);

// *** target mode (end) ***
// *************************

static void sg_RespawnPlayer(std::istream &s)
{
        eGrid *grid = eGrid::CurrentGrid();
        if(!grid) {
                con << "Must be called while a grid exists!\n";
                return;
        }

        tString params;
        params.ReadLine( s, true );

        // first parse the line to get the params : <player name> <message flag> <x> <y> <dirx> <diry>
        int pos = 0; //
        tString PlayerName = ePlayerNetID::FilterName(params.ExtractNonBlankSubString(pos));
        ePlayerNetID *pPlayer = 0;
        pPlayer = ePlayerNetID::FindPlayerByName(PlayerName, NULL);
        if(!pPlayer) {
            return;
        }
        const tString message_str = params.ExtractNonBlankSubString(pos);
        int message = atoi(message_str);
        const tString x_str = params.ExtractNonBlankSubString(pos);
        REAL x = atof(x_str);
        const tString y_str = params.ExtractNonBlankSubString(pos);
        REAL y = atof(y_str);
        const tString dirx_str = params.ExtractNonBlankSubString(pos);
        REAL dirx = atof(dirx_str);
        const tString diry_str = params.ExtractNonBlankSubString(pos);
        REAL diry = atof(diry_str);
        // prepare coord and direction ...
        eCoord ppos, pdir;
        if (((x_str == "") && (y_str == "")) || ((dirx ==0) && (diry == 0))) {
            return;
        }
        ppos = eCoord(x,y);
        pdir = eCoord(dirx,diry);

        // let's respawn now ...
        eGameObject *pGameObject = pPlayer->Object();
        if ((!pGameObject) ||
            (!(pGameObject->Alive()) ))
        {
            gCycle *pCycle = new gCycle(grid, ppos, pdir, pPlayer);
            pPlayer->ControlObject(pCycle);

            if (message) {
                tColoredString playerName;
                playerName << *pPlayer << tColoredString::ColorString(1,1,1);

                // send a console message to the player
                sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
            }
        }
}

static tConfItemFunc sg_RespawnPlayer_conf("RESPAWN_PLAYER",&sg_RespawnPlayer);


static void sg_TeleportPlayer(std::istream &s)
{
	eGrid *grid = eGrid::CurrentGrid();
	if(!grid) {
		con << "Must be called while a grid exists!\n";
		return;
	}

	tString params;
	params.ReadLine( s, true );

	// first parse the line to get the params : <player name> <message flag> <x> <y> <dirx> <diry>
	int pos = 0; //
	tString PlayerName = ePlayerNetID::FilterName(params.ExtractNonBlankSubString(pos));
	ePlayerNetID *pPlayer = 0;
	pPlayer = ePlayerNetID::FindPlayerByName(PlayerName, NULL);
	if(!pPlayer) {
		return;
	}
	const tString x_str = params.ExtractNonBlankSubString(pos);
	REAL x = atof(x_str);
	const tString y_str = params.ExtractNonBlankSubString(pos);
	REAL y = atof(y_str);
	tString relabs = params.ExtractNonBlankSubString(pos);
	if (relabs=="") relabs="rel";

	const tString xdir_str = params.ExtractNonBlankSubString(pos);
	REAL xdir = atof(xdir_str);
	const tString ydir_str = params.ExtractNonBlankSubString(pos);
	REAL ydir = atof(ydir_str);
	if ((xdir_str == "") && (ydir_str == "")) {
		xdir = ydir = 0.0;
	}
    eCoord dir = eCoord(xdir,ydir);

	// prepare new coord ...
	eCoord ppos;
	if ((x_str == "") && (y_str == "")) {
		return;
	}
	ppos = eCoord(x,y);

	// let's teleport now ...
	gCycle *pGameObject = dynamic_cast<gCycle *>(pPlayer->Object());
	if ((pGameObject) &&
		(pGameObject->Alive()))
	{
		if (relabs=="rel") ppos = ppos + pGameObject->Position();
		// Jump to new position without creating walls
		REAL time = pGameObject->LastTime();
		pGameObject->TeleportTo(ppos, dir, time);
	}
}

static tConfItemFunc sg_TeleportPlayer_conf("TELEPORT_PLAYER",&sg_TeleportPlayer);


