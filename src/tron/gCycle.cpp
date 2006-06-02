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
#include "gWall.h"
#include "nKrawall.h"
#include "gAIBase.h"
#include "eDebugLine.h"
#include "gArena.h"

#include "tMath.h"
#include <stdlib.h>
#include <fstream>

#ifndef DEDICATED
#define DONTDOIT
#include "rRender.h"
#endif

// TODO: get rid of this
#include "tDirectories.h"

bool sg_gnuplotDebug = false;

#ifdef DEBUG
static tSettingItem<bool> sg_("DEBUG_GNUPLOT",sg_gnuplotDebug);
#endif

//  *****************************************************************

static nNOInitialisator<gCycle> cycle_init(320,"cycle");

//  *****************************************************************

// static nVersionFeature sg_DoubleSpeed( 1 );

//tCONTROLLED_PTR(ePlayerNetID)   lastEnemyInfluence;  	// the last enemy wall we encountered
//REAL							lastTime;				// the time it was drawn at
bool headlights=1;
bool cycleprograminited=0;

// enemy influence settings
static REAL sg_enemyChatbotTimePenalty = 30.0f;   //!< penalty for victim in chatbot mode
static REAL sg_enemyFriendTimePenalty = 2500.0f;  //!< penalty for teammate influence
// REAL sg_enemySelfTimePenalty = 5000.0f;    //!< penalty for self influence
static REAL sg_enemyDeadTimePenalty = 0.0f;       //!< penalty for influence from dead players
static REAL sg_suicideTimeout = 10000.0f;         //!< influences older than this don't count as kill cause
static REAL sg_enemyCurrentTimeInfluence = 0.0f;  //!< blends in the current time into the relevant time

static tSettingItem<REAL> sg_enemyChatbotTimePenaltyConf( "ENEMY_CHATBOT_PENALTY", sg_enemyChatbotTimePenalty );
static tSettingItem<REAL> sg_enemyFriendTimePenaltyConf( "ENEMY_TEAMMATE_PENALTY", sg_enemyFriendTimePenalty );
static tSettingItem<REAL> sg_enemyDeadTimePenaltyConf( "ENEMY_DEAD_PENALTY", sg_enemyDeadTimePenalty );
static tSettingItem<REAL> sg_suicideTimeoutConf( "ENEMY_SUICIDE_TIMEOUT", sg_suicideTimeout );
static tSettingItem<REAL> sg_enemyCurrentTimeInfluenceConf( "ENEMY_CURRENTTIME_INFLUENCE", sg_enemyCurrentTimeInfluence );

// the last enemy possibly responsible for our death
const ePlayerNetID* gEnemyInfluence::GetEnemy() const
{
    return lastEnemyInfluence.GetPointer();
}

REAL gEnemyInfluence::GetTime() const
{
    return lastTime;
}

gEnemyInfluence::gEnemyInfluence()
{
    lastTime = -sg_suicideTimeout;
}

// add the result of the sensor scan to our data
void gEnemyInfluence::AddSensor( const gSensor& sensor, REAL timePenalty, gCycle * thisCycle )
{
    // the client has no need for this, it does not execute AI code
    if ( sn_GetNetState() == nCLIENT )
        return;

    // check if the sensor hit an enemy wall
    // if ( sensor.type != gSENSOR_ENEMY )
    //    return;

    // get the wall
    if ( !sensor.ehit )
        return;

    eWall* wall = sensor.ehit->GetWall();
    if ( !wall )
        return;

    AddWall( wall, sensor.before_hit, timePenalty, thisCycle );
}

// add the interaction with a wall to our data
void gEnemyInfluence::AddWall( const eWall * wall, eCoord const & pos, REAL timePenalty, gCycle * thisCycle )
{
    // the client has no need for this, it does not execute AI code
    if ( sn_GetNetState() == nCLIENT )
        return;

    // see if it is a player wall
    gPlayerWall const * playerWall = dynamic_cast<gPlayerWall const *>( wall );
    if ( !playerWall )
        return;

    // get the approximate time the wall was drawn
    REAL alpha = .5f;
    // try to get a more accurate value
    if ( playerWall->Edge() )
    {
        // get the position of the collision point
        alpha = playerWall->Edge()->Ratio( pos );
    }
    REAL timeBuilt = playerWall->Time( 0.5f );

    AddWall( playerWall, timeBuilt - timePenalty, thisCycle );
}

// add the interaction with a wall to our data
void gEnemyInfluence::AddWall( const gPlayerWall * wall, REAL timeBuilt, gCycle * thisCycle )
{
    // the client has no need for this, it does not execute AI code
    if ( sn_GetNetState() == nCLIENT )
        return;

    if ( !wall )
        return;

    // get the cycle
    gCycle *cycle = wall->Cycle();
    if ( !cycle )
        return;

    // don't count self influence
    if ( thisCycle == cycle )
        return;

    REAL time = timeBuilt;
    if ( thisCycle )
    {
        REAL currentTime = thisCycle->LastTime();
        time += ( currentTime - time ) * sg_enemyCurrentTimeInfluence;
    }

    // get the player
    ePlayerNetID* player = cycle->Player();
    if ( !player )
        return;

    // don't accept milkers.
    if ( thisCycle && !ePlayerNetID::Enemies( thisCycle->Player(), player ) )
    {
        return;
    }

    // if the player is not our enemy, add extra time penalty
    if ( thisCycle->Player() && player->CurrentTeam() == thisCycle->Player()->CurrentTeam() )
    {
        // the time shall be at most the time of the last turn, it should count as suicide if
        // I drive into your three mile long wall
        if ( time > cycle->GetLastTurnTime() )
            time = cycle->GetLastTurnTime();
        time -= sg_enemyFriendTimePenalty;
    }
    const ePlayerNetID* pInfluence = this->lastEnemyInfluence.GetPointer();

    // calculate effective last time. Add malus if the player is dead.
    REAL lastEffectiveTime = lastTime;
    if ( !pInfluence  || !pInfluence->Object() ||  !pInfluence->Object()->Alive() )
    {
        lastEffectiveTime -= sg_enemyDeadTimePenalty;
    }

    // same for the current influence
    REAL effectiveTime = time;
    if ( !cycle->Alive() )
    {
        effectiveTime -= sg_enemyDeadTimePenalty;
    }

    // if the new influence is newer, take it.
    if ( effectiveTime > lastEffectiveTime || !bool(lastEnemyInfluence) )
    {
        lastEnemyInfluence = player;
        lastTime		   = time;
    }
}

static float sg_cycleSyncSmoothTime = .1f;
static tSettingItem<float> conf_smoothTime ("CYCLE_SMOOTH_TIME", sg_cycleSyncSmoothTime);

static float sg_cycleSyncSmoothMinSpeed = .2f;
static tSettingItem<float> conf_smoothMinSpeed ("CYCLE_SMOOTH_MIN_SPEED", sg_cycleSyncSmoothMinSpeed);

static float sg_cycleSyncSmoothThreshold = .2f;
static tSettingItem<float> conf_smoothThreshold ("CYCLE_SMOOTH_THRESHOLD", sg_cycleSyncSmoothThreshold);

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



static int score_die=-2;
static tSettingItem<int> s_d("SCORE_DIE",score_die);

static int score_kill=3;
static tSettingItem<int> s_k("SCORE_KILL",score_kill);

static int score_suicide=-4;
static tSettingItem<int> s_s("SCORE_SUICIDE",score_suicide);

// input control

uActionPlayer gCycle::s_brake("CYCLE_BRAKE", -5);
static uActionPlayer s_brakeToggle("CYCLE_BRAKE_TOGGLE", -5);

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
gDestination::gDestination(nMessage &m)
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
void gDestination::WriteCreate(nMessage &m){
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
    gDestination *dest=new gDestination(m);

    // and the ID of the cycle the destination is added to
    unsigned short cycle_id;
    m.Read(cycle_id);
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
                    c->AddDestination(dest);
                    if ( c->Player() && !dest->Chatting() )
                        c->Player()->Activity();

                    // read game time from message
                    REAL gameTime = se_GameTime()+c->Lag()*3;
                    if ( !m.End() )
                        m >> gameTime;

                    // uncomment to test sync error compensation code on client :)
                    // gameTime += .2;

                    dest->SetGameTime( gameTime );
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
    dest->WriteCreate(*m);
    m->Write(c->ID());
    *m << dest->GetGameTime();
    m->BroadCast();
}

void gCycle::OnNotifyNewDestination( gDestination* dest )
{
#ifdef DEBUG
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
}


// *******************************************************************************************
// *
// *	OnDropTempWall
// *
// *******************************************************************************************
//!
//!		@param	wall	   the wall the other cycle is grinding
//!
// *******************************************************************************************

void gCycle::OnDropTempWall( gPlayerWall * wall )
{
    tASSERT( wall );

    // drop the current wall if eiter this or the last wall is grinded
    gNetPlayerWall * nw = wall->NetWall();
    if ( nw == currentWall || ( nw == lastWall && currentWall && currentWall->Vec().NormSquared() > EPS ) )
    {
        // just request the drop, Timestep() will execute it later
        dropWallRequested_ = true;
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

    // apply rubber shortening
    return len - GetRubber() * sg_cycleRubberWallShrink;
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
}

gCycleExtrapolator::gCycleExtrapolator(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID *p,bool autodelete)
        :gCycleMovement( grid, pos, dir, p, autodelete )
        ,trueDistance_( 0 )
        ,parent_( 0 )
{
    // an extrapolator should not be visible as a gameobject from the outside
    RemoveFromList();
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
    // ignore temporary walls, they may not be real
    const gPlayerWall *w = dynamic_cast<const gPlayerWall*>(ww);
    if (w)
    {
        gNetPlayerWall* nw = w->NetWall();
        if ( nw && nw->Preliminary() && w->Cycle() == parent_ )
            return false;

        // get time the wall was built
        REAL builtTime = w->Time(alpha);

        // is the wall built in the future ( happens during extrapolation )
        if ( builtTime > time )
            return false;

        // ignore recent walls of parent cycle
        gCycle *otherPlayer=w->Cycle();
        if (otherPlayer==parent_ &&
                ( time < builtTime + 4 * GetTurnDelay() || GetDistance() < w->Pos( alpha ) + .01 * sg_delayCycle*Speed()/SpeedMultiplier()  )
           )
            return false;
    }

    // delegate
    return parent_->EdgeIsDangerous( ww, time, alpha ) && gCycleMovement::EdgeIsDangerous( ww, time, alpha );
}

bool gCycleExtrapolator::TimestepCore(REAL currentTime)
{
    // determine a suitable next destination
    gDestination destDefault( *parent_ ), *dest=&destDefault;
    if ( GetCurrentDestination() )
    {
        dest = GetCurrentDestination();
    }

    // correct distance
    distance = dest->distance - DistanceToDestination( *dest );
    REAL distanceBefore = GetDistance();

    // delegate
    bool ret = gCycleMovement::TimestepCore( currentTime );

    // update true distance
    trueDistance_ += GetDistance() - distanceBefore;

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
        delete customModel;
        delete bodyModel;
        delete frontModel;
        delete rearModel;
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
        std::ifstream in;
        if ( tDirectories::Data().Open( in, filename ) )
        {
            return tNEW(rModel( filename ));
        }
        return 0;
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

void gCycle::MyInitAfterCreation(){
    dropWallRequested_ = false;
    lastGoodPosition_ = pos;

#ifdef DEBUG
    // con << "creating cycle.\n";
#endif
    engine  = tNEW(eSoundPlayer)(cycle_run,true);
    turning = tNEW(eSoundPlayer)(turn_wav);
    spark   = tNEW(eSoundPlayer)(scrap);

    //correctDistSmooth=correctTimeSmooth=correctSpeedSmooth=0;
    correctDistanceSmooth = 0;

    resimulate_ = false;

    deathTime=0;

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

    // dir=dirDrive;

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
                    "cycle.ase and bike.png or body.mod, front.mod, rear.mod, body.png and wheel.png." );
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

    nextChatAI=lastTime;

    // add to game grid
    this->AddToList();

#ifdef DEBUG
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

gCycle::gCycle(eGrid *grid, const eCoord &pos,const eCoord &d,ePlayerNetID *p,bool autodelete)
        :gCycleMovement(grid, pos,d,p,autodelete),
        engine(NULL),
        turning(NULL),
        skew(0),skewDot(0),
        rotationFrontWheel(1,0),rotationRearWheel(1,0),heightFrontWheel(0),heightRearWheel(0),
        currentWall(NULL),
        lastWall(NULL)
{
    windingNumberWrapped_ = windingNumber_ = Grid()->DirectionWinding(dirDrive);
    dirDrive = Grid()->GetDirection(windingNumberWrapped_);
    dir = dirDrive;

    lastNetWall=lastWall=currentWall=NULL;

    MyInitAfterCreation();

    sg_ArchiveCoord( this->dirDrive, 1 );
    sg_ArchiveCoord( this->dir, 1 );
    sg_ArchiveCoord( this->pos, 1 );
    sg_ArchiveReal( this->verletSpeed_, 1 );
}

gCycle::~gCycle(){
#ifdef DEBUG
    //  con << "deleting cylce...\n";
#endif
    // clear the destination list

    tDESTROY(engine);
    tDESTROY(turning);
    tDESTROY(spark);

    this->RemoveFromGame();

    if (mp){
        delete customModel;
        delete customTexture;
    }
    else{
        delete body;
        delete front;
        delete rear;
        delete wheelTex;
        delete bodyTex;
    }
#ifdef DEBUG
    //con << "Deleted cycle.\n";
#endif
    /*
      delete currentPos;
      delete last_turn;
    */
}

void gCycle::RemoveFromGame()
{
    // keep this cycle alive
    tJUST_CONTROLLED_PTR< gCycle > keep;

    if ( this->GetRefcount() > 0 )
    {
        keep = this;

        this->Turn(0);
        this->Kill();

        // really kill the cycle even on the client
        if ( this->Alive() )
        {
            Die( lastTime );
            tNEW(gExplosion)(grid, pos, lastTime, color_);
        }
    }

    if (currentWall)
        currentWall->CopyIntoGrid( grid );
    currentWall=NULL;
    lastWall=NULL;

    gCycleMovement::RemoveFromGame();
}

static inline void rotate(eCoord &r,REAL angle){
    REAL x=r.x;
    r.x+=r.y*angle;
    r.y-=x*angle;
    r=r*(1/sqrt(r.NormSquared()));
}

#ifdef MACOSX
// Sparks have a large performance problem on Macs. See http://guru3.sytes.net/viewtopic.php?t=2167
bool crash_sparks=false;
#else
bool crash_sparks=true;
#endif

//static bool forceTime=false;

// from nNetwork.C
extern REAL planned_rate_control[MAXCLIENTS+2];

static REAL sg_minDropInterval=0.05;
static tSettingItem< REAL > sg_minDropIntervalConf( "CYCLE_MIN_WALLDROP_INTERVAL", sg_minDropInterval );

bool gCycle::Timestep(REAL currentTime){
    // drop current wall if it was requested
    if ( dropWallRequested_ )
    {
        // but don't do so too often globally (performance)
        static double nextDrop = 0;
        double time = tSysTimeFloat();
        if ( time >= nextDrop )
        {
            nextDrop = time + sg_minDropInterval;
            this->DropWall();
        }
    }

#ifdef DEBUG
    if ( sg_gnuplotDebug && Player() )
    {
        std::ofstream f( Player()->GetUserName() + "_step", std::ios::app );
        f << pos.x << " " << pos.y << "\n";
    }
#endif
    // timewarp test debug code
    //if ( Player() && Player()->IsHuman() )
    //    currentTime -= .1;

    // don't timestep when you're dead
    if ( !Alive() )
    {
        if ( sn_GetNetState() == nSERVER )
            RequestSync();

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

    // nothing special if simulating backwards
    if (currentTime < lastTime)
        return gCycleMovement::Timestep(currentTime);

    // no targets are given: activate chat AI
    if (!currentDestination && pendingTurns.empty()){
        if (currentTime>nextChatAI && bool(player) &&
                ((player->IsChatting()    && player->Owner() == sn_myNetID) ||
                 (!player->IsActive() && sn_GetNetState() == nSERVER) )
           ){
            nextChatAI=currentTime+1;

            gSensor front(this,pos,dir);
            gSensor left(this,pos,dir.Turn(eCoord(0,1)));
            gSensor right(this,pos,dir.Turn(eCoord(0,-1)));

            REAL range=verletSpeed_*4;

            front.detect(range);
            left.detect(range);
            right.detect(range);

            enemyInfluence.AddSensor( front, sg_enemyChatbotTimePenalty, this );
            enemyInfluence.AddSensor( right, sg_enemyChatbotTimePenalty, this );
            enemyInfluence.AddSensor( left, sg_enemyChatbotTimePenalty, this );

#ifdef DEBUG_X
            if ( rand() > RAND_MAX / 4 )
                left.hit *= .5f;
            if ( rand() > RAND_MAX / 4 )
                right.hit *= .5f;
            if ( rand() > RAND_MAX / 4 )
                front.hit *= .5f;
#endif

            if (front.hit<left.hit*.9 || front.hit<right.hit*.9){
                REAL lr=front.lr;
                if (front.type==gSENSOR_SELF) // NEVER close yourself in.
                    lr*=-100;
                lr-=left.hit-right.hit;
                if (lr>0)
                    Act(&se_turnRight,1);
                else
                    Act(&se_turnLeft,1);
            }
        }


        bool simulate=Alive();

        if ( !pendingTurns.empty() || currentDestination )
            simulate=true;

        if (simulate)
        {
            try
            {
                return gCycleMovement::Timestep(currentTime);
            }
            catch ( gCycleDeath const & death )
            {
                KillAt( death.pos_ );
                return false;
            }
        }
        else
            return !Alive();
    }
    else
    {
        // just basic movement to do: let base class handle that.
        gCycleMovement::Timestep( currentTime );
    }

    // do the rest of the timestep
    return gCycleMovement::Timestep( currentTime );
}

static void blocks(const gSensor &s, const gCycle *c, int lr)
{
    if ( nCLIENT == sn_GetNetState() )
        return;

    if (s.type == gSENSOR_RIM)
        gAIPlayer::CycleBlocksRim(c, lr);
    else if (s.type == gSENSOR_TEAMMATE || s.type == gSENSOR_ENEMY && s.ehit)
    {
        gPlayerWall *w = dynamic_cast<gPlayerWall*>(s.ehit->GetWall());
        if (w)
        {
            // int turn     = c->Grid()->WindingNumber();
            //	  int halfTurn = turn >> 1;

            // calculate the winding number.
            int windingBefore = c->WindingNumber();  // we start driving in c's direction
            // we need to make a sharp turn in the lr-direction
            //	  windingBefore   += lr * halfTurn;

            // after the transfer, we need to drive in the direction of the other
            // wall:
            int windingAfter = w->WindingNumber();

            // if the other wall drives in the opposite direction, we
            // need to turn around again:
            //	  if (s.lr == lr)
            // windingAfter -= lr * halfTurn;

            // make the winding difference a multiple of the winding number
            /*
              int compensation = ((windingAfter - windingBefore - halfTurn) % turn)
              + halfTurn;
              while (compensation < -halfTurn)
              compensation += turn;
            */

            // only if the two walls are parallel/antiparallel, there is true blocking.
            if (((windingBefore - windingAfter) & 1) == 0)
                gAIPlayer::CycleBlocksWay(c, w->Cycle(),
                                          lr, s.lr,
                                          w->Pos(s.ehit->Ratio(s.before_hit)),
                                          - windingAfter + windingBefore);
        }
    }
}

// lets a value decay smoothly
static void DecaySmooth( REAL& smooth, REAL relSpeed, REAL minSpeed, REAL clamp )
{
    if ( fabs(smooth) > .01 )
    {
        int x;
        x = 1;
    }

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

bool gCycle::TimestepCore(REAL currentTime){
    if (!finite(skew))
        skew=0;
    if (!finite(skewDot))
        skewDot=0;

    eCoord oldpos=pos;

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

    if ( smooth > 0)
    {
        REAL scd = correctDistanceSmooth * smooth;
        distance += scd;
        correctDistanceSmooth -= scd;
    }

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
            eCoord lookahead = dirDrive * ( 10 * correctPosSmooth.Norm() );
            ClampDisplacement( this, correctPosSmooth, lookahead + correctPosSmooth, pos );
            ClampDisplacement( this, correctPosSmooth, lookahead - correctPosSmooth, pos );
            lookahead = dirDrive * Speed();
            ClampDisplacement( this, correctPosSmooth, correctPosSmooth, pos + lookahead );
            ClampDisplacement( this, correctPosSmooth, - correctPosSmooth, pos + lookahead );
        }
    }

    if (animts>.2)
        animts=.2;

    rotate(rotationFrontWheel,2*verletSpeed_*animts/.43);
    rotate(rotationRearWheel,2*verletSpeed_*animts/.73);

    const REAL extension=.25;

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
                        if ( gCycleMovement::TimestepCore( startBuildWallAt ) )
                            return true;

                    // build the wall, modifying the spawn time to make sure it happens
                    REAL lastSpawn = spawnTime_;
                    spawnTime_ += -1E+20;
                    DropWall();
                    spawnTime_ = lastSpawn;
                }

                // simulate rest of frame
                if ( gCycleMovement::TimestepCore( currentTime ) )
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
        if (currentWall)
        {
            eCoord wallEndPos = pos;

            // z-man: the next two lines are a very bad idea. This lets walls stick out on the other side while you're using up your rubber.
            //if ( sn_GetNetState() != nSERVER )
            //    wallEndPos = pos + dirDrive * ( verletSpeed_ * se_PredictTime() );
            currentWall->Update(currentTime, wallEndPos );
        }

        // animate skew
        gSensor fl(this,pos,dirDrive.Turn(1,1));
        gSensor fr(this,pos,dirDrive.Turn(1,-1));

        fl.detect(extension*4);
        fr.detect(extension*4);

        enemyInfluence.AddSensor( fr, 0, this );
        enemyInfluence.AddSensor( fl, 0, this );

        if (fl.ehit)
            blocks(fl, this, -1);

        if (fr.ehit)
            blocks(fr, this,  1);

#ifndef DEDICATED
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

        if (fl.ehit && fl.hit<extension){
            sparkpos=pos+dirDrive.Turn(1,1)*fl.hit;
            sparkdir=dirDrive.Turn(0,-1);
        }
        if (fr.ehit && fr.hit<extension){
            //      blocks(fr, this, -1);
            sparkpos=pos+dirDrive.Turn(1,-1)*fr.hit;
            sparkdir=dirDrive.Turn(0,1);
        }

        /*
          if (crash_sparks && animts>0)
          new gSpark(pos,dirDrive,currentTime);
        */
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
            if ( !sg_avoidBadOldClientSync || sg_NoLocalTunnelOnSync.Supported( Owner() ) || MaxSpaceAhead( this, ts, lookahead, lookahead ) >= lookahead )
            {
                RequestSync(false);
            }

            nextSync=tSysTimeFloat()+sg_syncIntervalEnemy;
            nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
        }
        else if ( nextSyncOwner < tSysTimeFloat() &&
                  Owner() != 0 &&
                  sn_Connections[Owner()].bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) > 200 )
        {
            // sync only to the owner (provided there is enough bandwidth available)
            RequestSync(Owner(), false);
            nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
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

void gCycle::KillAt( const eCoord& deathPos){
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

#ifdef KRAWALL_SERVER
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
            tString ladderLog;
            ladderLog << "DEATH_SUICIDE " << hunter->GetUserName() << "\n";
            se_SaveToLadderLog( ladderLog );

            if ( score_suicide )
                hunter->AddScore(score_suicide, tOutput(), "$player_lose_suicide" );
            else
            {
                tColoredString hunterName;
                hunterName << *hunter << tColoredString::ColorString(1,1,1);
                sn_ConsoleOut( tOutput( "$player_free_suicide", hunterName ) );
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
                    tString ladderLog;
                    ladderLog << "DEATH_FRAG " << Player()->GetUserName() << " " << hunter->GetUserName()  << "\n";
                    se_SaveToLadderLog( ladderLog );

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
                    tString ladderLog;
                    ladderLog << "DEATH_TEAMKILL " << Player()->GetUserName() << " " << hunter->GetUserName()  << "\n";
                    se_SaveToLadderLog( ladderLog );

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
        if( nw == currentWall || nw == lastWall || nw == lastNetWall )
            return false;

        // see if the wall is from another player and from the future.
        // if it is, it's not dangerous, this cycle was here first.
        // on passing the wall later, the other cycle will be pushed back
        // to the collision point or killed if that is no longer possible.
        // z-man notes: I've got the vaque feeling something like this was
        // here before, but got thrown out again for making problems.
        if ( gJustChecking::justChecking && w->CycleMovement() != static_cast< const gCycleMovement * >( this ) && w->Time(a) > time )
            return false;
    }

    return gCycleMovement::EdgeIsDangerous( ww, time, a );
}

void gCycle::PassEdge(const eWall *ww,REAL time,REAL a,int){
    {
        // deactivate time check
        gJustChecking thisIsSerious;

        if (!EdgeIsDangerous(ww,time,a) || !Alive() )
            return;

#ifdef DEBUG
        if (!EdgeIsDangerous(ww,time,a) || !Alive() )
            return;
#endif
    }

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
        if(time < otherTime*(1-EPS))
        {
            // we were first!
            static bool tryToSaveFutureWallOwner = false;
            if ( tryToSaveFutureWallOwner && sn_GetNetState() != nCLIENT && w->NetWall() == otherPlayer->currentWall && otherPlayer->LastTime() < time + .5f )
            {
                // teleport the other cycle back to the point before the collision; its next timestep
                // will simulate the collision again from the right viewpoint
                // determine the distance of the pushback
                REAL d = otherPlayer->GetDistanceSinceLastTurn() * .001;
                if ( d < .01 )
                    d = .01;
                REAL maxd = eCoord::F( otherPlayer->dirDrive, collPos - otherPlayer->GetLastTurnPos() ) * .5;
                if ( d > maxd )
                    d = maxd;
                if ( d < 0 )
                {
                    // err, trouble. Better kill the other player.
                    if ( currentWall )
                        otherPlayer->enemyInfluence.AddWall( currentWall->Wall(), lastTime, otherPlayer );
                    otherPlayer->KillAt( collPos );
                }

                // do the move
                otherPlayer->MoveSafely( collPos-otherPlayer->dirDrive*d, otherPlayer->LastTime(), otherTime - d/otherPlayer->Speed() );

                // drop our wall so collisions are more accurate
                dropWallRequested_ = true;
            }
            else
            {
                // the unfortunate other player made a turn in the meantime or too much time has passed. We cannot unroll its movement,
                // so we have to destroy it.
                if ( currentWall )
                    otherPlayer->enemyInfluence.AddWall( currentWall->Wall(), lastTime, otherPlayer );

                otherPlayer->KillAt( collPos );
            }
        }
        else // sad but true
        {
            // this cycle has to die here unless it has rubber left
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
            braking = newBraking;
            AddDestination();
        }
        return true;
    }
    else if(s_brakeToggle==*Act){
        if ( x > 0 )
        {
            braking = !braking;
            AddDestination();
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
#ifdef DEBUG
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

void gCycle::Kill(){
    // keep this cycle alive
    tJUST_CONTROLLED_PTR< gCycle > keep( this->GetRefcount()>0 ? this : 0 );

    if (sn_GetNetState()!=nCLIENT){
        RequestSync(true);
        if (Alive()){
            Die( lastTime );
            tNEW(gExplosion)(grid, pos,lastTime, color_);
            //	 eEdge::SeethroughHasChanged();

            if ( currentWall )
            {
                // z-man: updating the wall so it reflects exactly the position of death looks like
                // a good idea, but unfortunately, the collision position reported from above
                // is inaccurate. It's better not to use it at all, or the cycle's wall will stick out
                // a bit on the other side of the wall it crashed into.
                // reason: the wall

                // currentWall->Update( lastTime, pos );

                // copy the wall into the grid, but not directly; the grid datastructures are probably currently traversed. Kill() is called from eGameObject::Move().
                currentWall->CopyIntoGrid( 0 );

                currentWall = NULL;
            }

            // request a new sync
            RequestSync();
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

REAL sg_laggometerScale=1;
static tSettingItem< REAL > sg_laggometerScaleConf( "LAG_O_METER_SCALE", sg_laggometerScale );

int sg_blinkFrequency=10;
static tSettingItem< int > sg_blinkFrequencyConf( "CYCLE_BLINK_FREQUENCY", sg_blinkFrequency );

#ifndef DEDICATED
void gCycle::Render(const eCamera *cam){
    if ( lastTime > spawnTime_ && !Vulnerable() )
    {
        double time = tSysTimeFloat();
        double wrap = time - floor(time);
        int pulse = int ( 2 * wrap * sg_blinkFrequency );
        if ( ( pulse & 1 ) == 0 )
            return;
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
            glPushMatrix();
            /*
              GLfloat sk[4][4]={{0,.1,0,0},
              {-.1,0,0,0},
              {0,0,.1,0},
              {1,.2,-1.05,1}};

              if (moviepack_hack)
              glMultMatrixf(&sk[0][0]);
            */

            customTexture->Select();
            //glDisable(GL_TEXTURE_2D);
            //glDisable(GL_TEXTURE);
            glColor3f(1,1,1);

            //glPolygonMode(GL_FRONT, GL_FILL);
            //glDepthFunc(GL_LESS);
            //glCullFace(GL_BACK);
            customModel->Render();
            //glLineWidth(2);
            //glPolygonMode(GL_BACK,GL_LINE);
            //glDepthFunc(GL_LEQUAL);
            //glCullFace(GL_FRONT);
            //customModel->Render();
            //sr_ResetRenderState();

            glPopMatrix();
            glPopMatrix();
            glTranslatef(-1.5,0,0);
        }
        else{
            /*
              glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
              glEnable(GL_TEXTURE_GEN_S);
                
              glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
              glEnable(GL_TEXTURE_GEN_T);
                
              glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
              glEnable(GL_TEXTURE_GEN_R);
                
              glTexGeni(GL_Q,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
              glEnable(GL_TEXTURE_GEN_Q);
            */

            glEnable(GL_TEXTURE_2D);

            /*
            		 static    GLfloat tswap[4][4]={{1,0,0,0},
            		 {0,0,1,0},
            		 {0,-1,0,0},
            		 {.5,.5,0,1}};
                
            		 static    GLfloat tswapb[4][4]={{1,0,0,0},
            		 {0,0,1,0},
            		 {0,-1,0,0},
            		 {.2,1.2,0,1}};
            */

            //       TexMatrix();
            //       glLoadMatrixf(&tswapb[0][0]);
            //       glScalef(.4,.4,.8);
            ModelMatrix();


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


            //       TexMatrix();
            //       glLoadMatrixf(&tswap[0][0]);
            //       glScalef(.65,.65,.65);
            //       ModelMatrix();

            rear->Render();
            glPopMatrix();

            glPushMatrix();
            glTranslatef(1.84,0,.43);

            GLfloat mf[4][4]={{rotationFrontWheel.x,0,rotationFrontWheel.y,0},
                              {0,1,0,0},
                              {-rotationFrontWheel.y,0,rotationFrontWheel.x,0},
                              {0,0,0,1}};

            glMultMatrixf(&mf[0][0]);


            //       TexMatrix();
            //       glLoadMatrixf(&tswap[0][0]);
            //       glScalef(1.2,1.2,1.2);
            //       ModelMatrix();

            front->Render();
            glPopMatrix();
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

        if(sr_floorDetail>rFLOOR_GRID && rTextureGroups::TextureMode[rTextureGroups::TEX_FLOOR]>0 && sr_alphaBlend){
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

        if (sn_GetNetState() != nSTANDALONE && sr_laggometer && f*l>.5){
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
            if (turn_wav.alt)
                turning->Mix(dest,len,viewer,rvol,lvol,5);
            else
                turning->Mix(dest,len,viewer,rvol,lvol,1);

        if (spark)
            spark->Mix(dest,len,viewer,rvol*.5,lvol*.5,4);
    }
}
#endif

eCoord gCycle::PredictPosition(){
    gSensor s(this,pos, dir * (verletSpeed_ * se_PredictTime()) + correctPosSmooth );
    s.detect(1);
    return s.before_hit;

    //    eCoord p = pos + dir * (speed * se_PredictTime());
    //    return p + correctPosSmooth;
}

eCoord gCycle::CamPos()
{
    return PredictPosition() + dir.Turn(0 ,-skew*z);

    //    gSensor s(this,pos, PredictPosition() - pos );
    //    s.detect(1);

    //    return s.before_hit + dir.Turn(0 ,-skew*z);

    // return pos + dir.Turn(0 ,-skew*z);
}

eCoord  gCycle::CamTop()
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





// cycle network routines:
gCycle::gCycle(nMessage &m)
        :gCycleMovement(m),
        engine(NULL),
        turning(NULL),
        spark(NULL),
        skew(0),skewDot(0),
        rotationFrontWheel(1,0),rotationRearWheel(1,0),heightFrontWheel(0),heightRearWheel(0),
        currentWall(NULL),
        lastWall(NULL)
{
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
    nextSync=tSysTimeFloat()+sg_syncIntervalEnemy;
    nextSyncOwner=tSysTimeFloat()+sg_GetSyncIntervalSelf( this );
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

// resets the extrapolator to the last known state
void gCycle::ResetExtrapolator()
{
    resimulate_ = false;
    if (!extrapolator_)
    {
        extrapolator_ = tNEW( gCycleExtrapolator )(grid, pos, dir );
    }

    extrapolator_->CopyFrom( lastSyncMessage_, *this );
}

// simulate the extrapolator at higher speed
bool gCycle::Extrapolate( REAL dt )
{
    tASSERT( extrapolator_ );

    eCoord posBefore = extrapolator_->Position();

    // calculate target time
    REAL newTime = extrapolator_->LastTime() + dt;

    bool ret = false;

    // clamp: don't simulate further than our current time
    if ( newTime >= lastTime )
    {
        // simulate extrapolator until now
        eGameObject::TimestepThis( lastTime, extrapolator_ );

        // test if there are real (the check for list does that) destinations left; we cannot call it finished if there are.
        gDestination* unhandledDestination = extrapolator_->GetCurrentDestination();
        ret = !unhandledDestination || !unhandledDestination->list;

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
    eCoord reachable_pos = base->Position() + displacement;

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
    // REAL correctDist = eCoord::F( correctPosSmooth, dirDrive );
    // correctDistanceSmooth += correctDist;
    // correctPosSmooth = correctPosSmooth - dirDrive * correctDist;
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
        currentWall->beg = extrapolator_->GetLastTurnPos();

    // smooth position correction
    correctPosSmooth = correctPosSmooth + oldPos - pos;

#ifdef DEBUG
    if ( correctPosSmooth.NormSquared() > .1f )
    {
        std::cout << "Lag slide! " << correctPosSmooth << "\n";
        int x;
        x = 0;
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
    REAL forward = eCoord::F( newPos - startPos, dir );
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

    eCoord new_pos = pos;	// the extrapolated position

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
    if ( eCoord::F( dirDrive, sync.dir ) > .99f )
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
    if (Alive() && sync_alive!=1)
    {
        Die( lastSyncMessage_.time );
        MoveSafely( lastSyncMessage_.pos, lastTime, deathTime );
        distance=lastSyncMessage_.distance;

        DropWall( false );

        tNEW(gExplosion)( grid, lastSyncMessage_.pos, lastSyncMessage_.time ,color_ );

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
        distance += eCoord::F( newPos - pos, Direction() );

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
            braking = lastSyncMessage_.braking;
        }
        else
        {
            // same algorithm, but update smooth position correction so that there is no immediate visual change
            eCoord oldPos = pos + correctPosSmooth;
            SyncEnemy( lastSyncMessage_.lastTurn );
            correctPosSmooth = oldPos - pos;

#ifdef DEBUG
            if ( correctPosSmooth.NormSquared() > .1f && lastTime > 0.0 )
            {
                std::cout << "Lag slide! " << correctPosSmooth << "\n";
                int x;
                x = 0;
            }
#endif
        }

        // restore rubber meter
        if ( !rubberSent )
        {
            rubber = lastSyncMessage_.rubber;
        }
    }

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
    REAL notTurned=eCoord::F( dirDrive, lastSyncMessage_.dir );

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
        currentWall=new gNetPlayerWall
                    (this,crossPos,lastSyncMessage_.dir,crossTime,crossDist);

        turned = true;

        // save last driving direction
        lastDirDrive = dirDrive;
    }

    // side bending
    skewDot+=4*turnDirection;

    // calculate timestep
    // REAL ts = lastSyncMessage_.time - lastTime;

    // update position, speed, distance and direction
    MoveSafely( lastSyncMessage_.pos, lastTime, lastTime );
    verletSpeed_  = lastSyncMessage_.speed;
    lastTimestep_ = 0;
    distance = lastSyncMessage_.distance;
    dirDrive = lastSyncMessage_.dir;
    rubber = lastSyncMessage_.rubber;
    brakingReservoir = lastSyncMessage_.brakingReservoir;

    // update time to values from sync
    REAL oldTime = lastTime;
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
    return lastTime > spawnTime_ + sg_cycleInvulnerableTime;
}
