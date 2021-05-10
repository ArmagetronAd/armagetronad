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

#include "rSDL.h"

#include "gZone.h"
#include "eFloor.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
#include "eTeam.h"
#include "ePlayer.h"
#include "rRender.h"
#include "nConfig.h"
#include "tString.h"
#include "rScreen.h"

#include "eAdvWall.h"
#include "eGameObject.h"
#include "uInputQueue.h"
#include "eTess2.h"
#include "eWall.h"
#include "gWall.h"
#include "gSensor.h"
#include "eSensor.h"
#include "gSvgOutput.h"
#include "eSound.h"
#include "uInput.h"
#include "tMath.h"
#include "nConfig.h"
#include "gSpawn.h"
#include "gArena.h"
#include "tRandom.h"

//HACK RACE
#include "gRace.h"

#include "gParser.h"

#include <time.h>
#include <algorithm>

std::deque<gZone *> sg_Zones;


static bool sg_SwapWinDeath = false;
static tSettingItem<bool> sg_SwitchWinDeathColorCONF("SWAP_WINZONE_DEATHZONE_COLORS", sg_SwapWinDeath);

static int sg_zoneAlphaToggle = 0;
static tSettingItem<int> sg_zoneAlphaToggleConf( "ZONE_ALPHA_TOGGLE", sg_zoneAlphaToggle );

static int sg_zoneDeath = 1;
static tSettingItem<int> sg_zoneDeathConf( "WIN_ZONE_DEATHS", sg_zoneDeath );

static REAL sg_expansionSpeed = 1.0f;
static REAL sg_initialSize = 5.0f;

static nSettingItem< REAL > sg_expansionSpeedConf( "WIN_ZONE_EXPANSION", sg_expansionSpeed );
static nSettingItem< REAL > sg_initialSizeConf( "WIN_ZONE_INITIAL_SIZE", sg_initialSize );

static int sg_ballsInteract = 0;
static tSettingItem<int> sg_ballsInteractConf( "BALLS_INTERACT", sg_ballsInteract );

static bool sg_cycleWallsInteract = 0;
static tSettingItem<bool> sg_cycleWallsInteractConf( "ZONES_BOUNCE_ON_CYCLE_WALLS", sg_cycleWallsInteract );

static bool sg_ballTeamMode = 0;  //0=ball score other team, 1=ball score only team owner ...
static tSettingItem<bool> sg_ballTeamModeConfig( "BALL_TEAM_MODE", sg_ballTeamMode );

static bool sg_ballKiller = 0;    //1=ball kill other team players ...
static tSettingItem<bool> sg_ballKillerConfig( "BALL_KILLS", sg_ballKiller );

static REAL sg_ballSpeedDecay = 0;
static tSettingItem<REAL> sg_ballSpeedDecayConf( "BALL_SPEED_DECAY", sg_ballSpeedDecay );
static tSettingItem<REAL> sg_zoneSpeedDecayConf( "ZONE_SPEED_DECAY", sg_ballSpeedDecay );

static REAL sg_ballSpeedHitDecay = 0;
static tSettingItem<REAL> sg_ballSpeedHitDecayConf( "BALL_SPEED_HIT_DECAY", sg_ballSpeedHitDecay );
static tSettingItem<REAL> sg_zoneSpeedHitDecayConf( "ZONE_SPEED_HIT_DECAY", sg_ballSpeedHitDecay );

static bool sg_ballRimStop = false;
static tSettingItem<bool> sg_ballRimStopConf( "BALL_STOP_IF_RIM_AND_CYCLE", sg_ballRimStop );

static REAL sg_ballCycleBoost = 0;
static tSettingItem<REAL> sg_ballCycleBoostConf( "BALL_CYCLE_ACCEL_BOOST", sg_ballCycleBoost );

static bool sg_ballAutoRespawn = 1;
static tSettingItem<bool> sg_ballAutoRespawnConf( "BALL_AUTORESPAWN", sg_ballAutoRespawn );

static int sg_zoneSegments = 11;
static tSettingItem<int> sg_zoneSegmentsConf( "ZONE_SEGMENTS", sg_zoneSegments );

static REAL sg_zoneSegLength = .5;
static tSettingItem<REAL> sg_zoneSegLengthConf( "ZONE_SEG_LENGTH", sg_zoneSegLength );

static REAL sg_zoneBottom = 0.0f;
static tSettingItem<REAL> sg_zoneBottomConf( "ZONE_BOTTOM", sg_zoneBottom );

static REAL sg_zoneHeight = 5.0f;
static tSettingItem<REAL> sg_zoneHeightConf( "ZONE_HEIGHT", sg_zoneHeight );

static bool sg_zoneNoFadeInSvr = false;
static tSettingItem<bool> sg_zoneNoFadeInSvrConf( "ZONE_NO_FADE_IN_SERVER", sg_zoneNoFadeInSvr );


static eLadderLogWriter sg_flagConquestRoundWinWriter("FLAG_CONQUEST_ROUND_WIN", true);
static eLadderLogWriter sg_flagTimeoutWriter("FLAG_TIMEOUT", true);
static eLadderLogWriter sg_flagReturnWriter("FLAG_RETURN", true);
static eLadderLogWriter sg_flagTakeWriter("FLAG_TAKE", true);
static eLadderLogWriter sg_flagDropWriter("FLAG_DROP", true);
static eLadderLogWriter sg_flagScoreWriter("FLAG_SCORE", true);
static eLadderLogWriter sg_flagHeldWriter("FLAG_HELD", true);
static eLadderLogWriter sg_baseRespawnWriter("BASE_RESPAWN", true);
static eLadderLogWriter sg_baseEnemyRespawnWriter("BASE_ENEMY_RESPAWN", true);

static eLadderLogWriter sg_deathZoneActivated("DEATHZONE_ACTIVATED", true);
static eLadderLogWriter sg_winZoneActivated("WINZONE_ACTIVATED", true);

bool gWinZoneHack::winnerPlayer_ = false;

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos )
{
    gZone * ret = NULL;
    if ( sg_zoneDeath )
    {
        ret = tNEW( gDeathZoneHack( grid, pos ) );
        sn_ConsoleOut( "$instant_death_activated" );

        sg_deathZoneActivated << ret->GetID() << ret->GetName() << ret->MapPosition().x << ret->MapPosition().y;
        sg_deathZoneActivated.write();
    }
    else
    {
        ret = tNEW( gWinZoneHack( grid, pos ) );
        if ( sg_currentSettings->gameType != gFREESTYLE )
        {
            sn_ConsoleOut( "$instant_win_activated" );
        }
        else
        {
            sn_ConsoleOut( "$instant_round_end_activated" );
        }

        sg_winZoneActivated << ret->GetID() << ret->GetName() << ret->MapPosition().x << ret->MapPosition().y;
        sg_winZoneActivated.write();
    }

    // initialize radius and expansion speed
    static_cast<eGameObject*>(ret)->Timestep( se_GameTime() );
    ret->SetReferenceTime();
    ret->SetRadius( sg_initialSize );
    ret->SetExpansionSpeed( sg_expansionSpeed );
    ret->SetRotationSpeed( .3f );

    return ret;
}


// number of segments to render a zone with
static const int sg_segments = 11;

// *******************************************************************************
// *
// *   EvaluateFunctionNow
// *
// *******************************************************************************
//!
//!        @param  f   function to evaluate
//!        @return     the function's value at lastTime - referenceTime_
//!
// *******************************************************************************

inline REAL gZone::EvaluateFunctionNow( tFunction const & f ) const
{
    return f( lastTime - referenceTime_ );
}


// *******************************************************************************
// *
// *   SetFunctionNow
// *
// *******************************************************************************
//!
//!        @param  f      function to modify
//!        @param  value  value the function should have at lastTime - referenceTime_
//!
// *******************************************************************************

inline void gZone::SetFunctionNow( tFunction & f, REAL value ) const
{
    f.SetOffset( value + f.GetSlope() * ( referenceTime_ - lastTime ) );
}


// *******************************************************************************
// *
// *   FindFirst / FindNext
// *
// *******************************************************************************
//!
//!        @param  name      name of the zone or group of zones
//!        @param  prev_pos  value of a previous result
//!
// *******************************************************************************

int gZone::FindFirst(tString name)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return -1;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=gameObjects.Len()-1;j>=0;j--)
    {
        gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
        // for all active base zone ...
        if ( zone && !zone->destroyed_ )
        {
            if (zone->name_ == name)
            {
                return j;
            }
        }
    }
    // if no zone has been found ...
    return -1;
}


int gZone::FindNext(tString name, int prev_pos)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return -1;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=prev_pos-1;j>=0;j--)
    {
        gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
        // for all active base zone ...
        if ( zone && !zone->destroyed_ )
        {
            if (zone->name_ == name)
            {
                return j;
            }
        }
    }
    // if no zone has been found ...
    return -1;
}

// *******************************************************************************
// *
// *   FindIdFirst / FindIdNext
// *
// *******************************************************************************
//!
//!        @param  id        ID of the zone or group of zones
//!        @param  prev_pos  value of a previous result
//!
// *******************************************************************************

int gZone::FindIdFirst(int id)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return -1;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=gameObjects.Len()-1;j>=0;j--)
    {
        gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
        // for all active base zone ...
        if ( zone && !zone->destroyed_ )
        {
            if (zone->GetID() == id)
            {
                return j;
            }
        }
    }
    // if no zone has been found ...
    return -1;
}


int gZone::FindIdNext(int id, int prev_pos)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return -1;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=prev_pos-1;j>=0;j--)
    {
        gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
        // for all active base zone ...
        if ( zone && !zone->destroyed_ )
        {
            if (zone->GetID() == id)
            {
                return j;
            }
        }
    }
    // if no zone has been found ...
    return -1;
}


// *******************************************************************************
// *
// *    gZone
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gZone::gZone( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:eNetGameObject( grid, pos, eCoord( 0,0 ), NULL, true ), rotation_(1,0), lastCoord_(0), nextUpdate_(-1)
{
    // store creation time
    referenceTime_ = createTime_ = lastTime = 0;

    destroyed_ = false;
    zoneInit_ = false;
    dynamicCreation_ = dynamicCreation;
    delayCreation_ = delayCreation;
    wallInteract_ = false;
    wallBouncesLeft_ = 0;
    wallPenetrate_ = false;
    newImpactWall_ = nullptr;
    interactWithCycle_ = true;
    interactWithZone_ = false;
    targetRadius_ = 0;
    lastImpactTime_ = 0;
    resizeRequested_ = false;
    fallSpeed_ = 0;
    lastSeekTime_ = 0;
    pOwner_ = NULL;
    pSeekingCycle_ = NULL;
    seeking_ = false;            //??? change this to a game object, allow seeking any
    name_ = tString("");
    effect_ = tString("");

    color_.r = color_.g = color_.b = 1.0f;

    // set fixed values
    SetRotationSpeed( .3f );
    SetRotationAcceleration( 0.0f );

    // find and assign unused ID
    id_ = 1;
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    int len = gameObjects.Len();
    for(int j=len-1;j>=0;j--)
    {
        gZone *zone=dynamic_cast<gZone *>(gameObjects(j));
        if(zone && !zone->destroyed_)
        {
            if(zone->GetID() == id_){ id_++; j=len; }
        }
    }

    //??? Look at doing this in the shot or wherever it is created...
    //??? or changing dynamic creation to disableAlpha or something
    if (dynamicCreation || sg_zoneNoFadeInSvr)
    {
        referenceTime_ = createTime_ = lastTime = se_mainGameTimer->Time();

        //Hack to get rid of the alpha to make zones non-transparent on creation
        //Negative creation times seem OK with current code
        createTime_ -= 3.5;
    }

    // add to game grid
    if (!delayCreation)
    {
        this->AddToList();
        sg_Zones.push_back(this);
    }

    // initialize position functions
    SetPosition( pos );

    //wrtl: Ok, this is the result of three hours of debugging, I hope it helps...
    eGameObject::pos = pos;
}


// *******************************************************************************
// *
// *    gZone
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!
// *******************************************************************************

gZone::gZone( nMessage & m )
:eNetGameObject( m ), rotation_(1,0)
{
    destroyed_ = false;
    zoneInit_ = false;
    dynamicCreation_ = false;
    delayCreation_ = false;
    wallInteract_ = false;
    wallBouncesLeft_ = 0;
    wallPenetrate_ = false;
    newImpactWall_ = nullptr;
    interactWithCycle_ = false;
    interactWithZone_ = false;
    lastImpactTime_ = 0;
    targetRadius_ = 0;
    resizeRequested_ = false;
    fallSpeed_ = 0;
    lastSeekTime_ = 0;
    pOwner_ = NULL;
    pSeekingCycle_ = NULL;
    seeking_ = false;
    name_ = tString("");
    effect_ = tString("");

    // read creation time
    m >> createTime_;
    referenceTime_ = lastTime = createTime_;

    // initialize color to white, ReadSync will fill in the true value if available
    color_.r = color_.g = color_.b = 1.0f;

    // set fixed values
    SetRotationSpeed( .3f );
    SetRotationAcceleration( 0.0f );

    // add to game grid
    if(!delayCreation_)
    {
        this->AddToList();
        sg_Zones.push_back(this);
    }

    // initialize position functions
    SetPosition( pos );
}


// *******************************************************************************
// *
// *    ~gZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gZone::~gZone( void )
{
    RemoveFromZoneList();
}

void gZone::RemoveFromGame(void) {
    RemoveFromZoneList();
    eNetGameObject::RemoveFromGame();
}

void gZone::RemoveFromZoneList(void) {
    std::deque<gZone *>::iterator pos_found =
        std::find_if(
            sg_Zones.begin(),
            sg_Zones.end(),
            std::bind2nd(
                std::equal_to<gZone *>(),
                this)
        );
    if(pos_found != sg_Zones.end())
        sg_Zones.erase(pos_found);
}


// *******************************************************************************
// *
// *    WriteCreate
// *
// *******************************************************************************
//!
//!     @param  m Message to write creation data to
//!
// *******************************************************************************

void gZone::WriteCreate( nMessage & m )
{
    // delegate
    eNetGameObject::WriteCreate( m );

    m << createTime_;
}


// *******************************************************************************
// *
// *    WriteSync
// *
// *******************************************************************************
//!
//!     @param  m Message to write sync data to
//!
// *******************************************************************************

void gZone::WriteSync( nMessage & m )
{
    // delegate
    eNetGameObject::WriteSync( m );

    // write color
    m << color_.r;
    m << color_.g;
    m << color_.b;

    // write reference time and functions
    m << referenceTime_;
    m << posx_;
    m << posy_;
    m << radius_;

    // write rotation speed
    m << rotationSpeed_;
}


// *******************************************************************************
// *
// *    ReadSync
// *
// *******************************************************************************
//!
//!     @param  m Message to read sync data from
//!
// *******************************************************************************

void gZone::ReadSync( nMessage & m )
{
    // delegage
    eNetGameObject::ReadSync( m );

    // read color
    if (!m.End())
    {
        m >> color_.r;
        m >> color_.g;
        m >> color_.b;
        se_MakeColorValid(color_.r, color_.g, color_.b, 1.0f);
    }

    // read reference time and functions
    if (!m.End())
    {
        m >> referenceTime_;

        m >> posx_;
        m >> posy_;
        m >> radius_;
    }
    else
    {
        referenceTime_ = createTime_;

        // take old default values
        this->radius_.SetOffset( sg_initialSize );
        this->radius_.SetSlope( sg_expansionSpeed );
        SetPosition( pos );
        SetVelocity( eCoord() );
    }

    // read rotation speed
    if (!m.End())
    {
        m >> rotationSpeed_;
    }
    else
    {
        // set fixed values
        SetRotationSpeed( .3f );
        SetRotationAcceleration( 0.0f );
    }
}

// *******************************************************************************
// *
// *    CreateZone
// *
// *******************************************************************************

static eLadderLogWriter sg_spawnzoneWriter("ZONE_SPAWNED", false);

static void CreateZone(tString zoneEffect, gZone *Zone, const REAL zoneSize, const REAL zoneGrowth, eCoord zoneDir, bool setColorFlag, gRealColor zoneColor, bool zoneInteractive, REAL targetRadius, std::vector<eCoord> route, tString zoneNameStr,  bool zonePenetrate = false){
    static_cast<eGameObject*>(Zone)->Timestep( se_GameTime() );
    Zone->SetReferenceTime();
    Zone->SetRadius( zoneSize );
    Zone->SetExpansionSpeed( zoneGrowth );
    Zone->SetRotationSpeed( .3f );
    Zone->SetVelocity(zoneDir);
    if (setColorFlag)
    {
        zoneColor.r = (zoneColor.r>1.0)?1.0:zoneColor.r;
        zoneColor.g = (zoneColor.g>1.0)?1.0:zoneColor.g;
        zoneColor.b = (zoneColor.b>1.0)?1.0:zoneColor.b;
        Zone->SetColor(zoneColor);
    }
    if (zoneInteractive)
    {
        Zone->SetWallInteract(true);
        Zone->SetWallBouncesLeft(-1);
    }
    if (zonePenetrate)
    {
        Zone->SetWallPenetrate(true);
    }
    if(targetRadius != 0)
    {
        Zone->SetTargetRadius(targetRadius);
    }
    if(!route.empty())
    {
        Zone->SetPosition(route.front());
        for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
        {
            Zone->AddWaypoint(*iter);
        }
    }
    Zone->SetName(zoneNameStr);
    gSumoZoneHack *thisSumoZone = dynamic_cast<gSumoZoneHack *>(Zone);
    if(thisSumoZone){
        thisSumoZone->SetUnspawnedState();
    }

    Zone->SetEffect(zoneEffect);

    Zone->RequestSync();

    sg_spawnzoneWriter << zoneEffect << Zone->GetID() << Zone->GetName() << Zone->MapPosition().x << Zone->MapPosition().y << Zone->GetVelocity().x << Zone->GetVelocity().y;
    sg_spawnzoneWriter.write();
}


static float sg_shotSeekUpdateTime = 0.5;
static tSettingItem<float> conf_shotSeekUpdateTime ("SHOT_SEEK_UPDATE_TIME", sg_shotSeekUpdateTime);

static float sg_zombieZoneSpeed = 10;
static tSettingItem<float> conf_zombieZoneSpeed ("ZOMBIE_ZONE_SPEED", sg_zombieZoneSpeed);


// BEGIN: zone bouncing base
static bool s_zoneWallInteractionFound;
static eCoord s_zoneWallInteractionCoord;
static eCoord s_zoneWallInteractionClosestCoord;
static REAL   s_zoneWallInteractionRadius;
static REAL   s_zoneWallInteractionImpactTime;
static eCoord s_zoneWallInteractionZoneVelocity;
static eCoord s_zoneWallInteractionImpactCoord;
static REAL   s_zoneWallInteractionZoneDeltaTime;
static bool   s_zoneWallInteractionCycleIgnored = false;

class ZoneBounceSensor: public eSensor
{
    public:
        gSensorWallType type;
        eWall * wall;

        ZoneBounceSensor(eGameObject *o,const eCoord &start,const eCoord &d)
                :eSensor(o,start,d), type(gSENSOR_NONE), wall(nullptr){}

        virtual void PassEdge(const eWall * wall, REAL time, REAL a, int r=1)
        {
            if(!wall) return;

            try
            {
                eSensor::PassEdge(wall, time, a, r);
            }
            catch( eSensorFinished & e )
            {
                if(dynamic_cast<const gWallRim *>(wall))
                {
                    this->type = gSENSOR_RIM;
                    this->wall = const_cast<eWall *>(wall);
                    throw;
                }
#if 0
                // Yes, it works
                // but I'm not entirely convinced that we really need
                // to treat cycle walls to the same must-not-pass as
                // rim walls for now
                else if(sg_cycleWallsInteract)
                {
                    const gPlayerWall * playerWall = dynamic_cast<const gPlayerWall *>(wall);
                    if(playerWall && playerWall->IsDangerous(a, se_GameTime()))
                    {
                        this->type = gSENSOR_ENEMY;
                        this->wall = const_cast<eWall *>(wall);

                        // Not entirely sure what this does
                        if( playerWall->EndTime() < playerWall->BegTime() ) this->lr = -(this->lr);

                        throw;
                    }
                }
#endif
                // Otherwise, we won't throw any exceptions
                // ignore the wall and keep looking
                this->ehit = NULL;
            }
        }
};

static void S_ZoneWallInteraction(eWall *pWall)
{
    //Ignore player walls for now
    gPlayerWall *pPlayerWall = dynamic_cast<gPlayerWall*>(pWall);
    if (pPlayerWall && ( !sg_cycleWallsInteract || s_zoneWallInteractionCycleIgnored ) )
    {
        return;
    }

    // determine the coord (I) and time (t) of the impact
    eCoord V = s_zoneWallInteractionZoneVelocity;
    REAL R = s_zoneWallInteractionRadius;

    eCoord XY = pWall->EndPoint(1) - pWall->EndPoint(0);
    REAL norm_XY = XY.Norm();
    XY *= (1/norm_XY);
    eCoord XP = s_zoneWallInteractionCoord - pWall->EndPoint(0);

    REAL Vn = XY*V;
    if (Vn==0) return;           // if zone is moving along the wall, no impact ...
    REAL d = XY*XP;

    REAL t1 = (R-d)/Vn;
    REAL t2 = (-R-d)/Vn;
    REAL t = 0;
    if ((t1>0) && (t2>0)) return;// can't be, again ...
    if (t1>0) t = t2;
    else if (t2>0) t = t1;
    else if (t1>t2) t = t2;
    else t = t1;

                                 // coordinate of the impact on the wall's line space
    REAL i = eCoord::F(XY,XP+V*t);
    
    // Hey, now we won't bounce on invisible walls!
    // It took way too long to figure this out...
    if(pPlayerWall && !pPlayerWall->IsDangerous(i/norm_XY, se_GameTime()))
    {
        return;
    }
    
    eCoord I;                    // position of the impact
    if (i>=0 && i<=norm_XY)      // there's an impact on the wall
    {
        I = pWall->EndPoint(0) + XY * i;
    }
    else                         // the zone might hit a wall end point, let's check this
    {
        // select the wall end according to wall's line intersection coord (i)
        eCoord P;
        if (i<0) P = pWall->EndPoint(0);
        else P = pWall->EndPoint(1);

        // get the time of the impact ...
        eCoord dp = s_zoneWallInteractionCoord - P;
        eCoord dv = s_zoneWallInteractionZoneVelocity;
        REAL a = dv.NormSquared();
        if (a<EPS) return;       // if ball has no speed, just left ...
        REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
        REAL c = dp.NormSquared()-s_zoneWallInteractionRadius*s_zoneWallInteractionRadius;
        REAL delta = b*b-4*a*c;
        if (delta<0) return;     // no t. it can't be, an impact just occured ...
        else                     // delta is positive, it means we have 2 different solutions
        {
            // the t we are looking for is negative and as close as possible to 0 = it just happens ;)
            delta = sqrt(delta);
            t1 = (-b-delta)/(2*a);
            t2 = (-b+delta)/(2*a);
            t = 0;
                                 // can't be, again ...
            if ((t1>0) && (t2>0)) return;
            if (t1>0) t = t2;
            else if (t2>0) t = t1;
            else if (t1>t2) t = t2;
            else t = t1;
            I = P;
        }
    }

    // t must be negative and close to 0 to make sence ...
    if (t>0 || t<-s_zoneWallInteractionZoneDeltaTime) return;

    // at this point, we have the intersection (I), the time of intersection (t)
    // let's check if it is the first impact ...
    if (!s_zoneWallInteractionFound || (t<s_zoneWallInteractionImpactTime))
    {
        s_zoneWallInteractionImpactTime = t;
        s_zoneWallInteractionImpactCoord = I;
        s_zoneWallInteractionFound = true;
    }
}


static void S_ZoneWallIntersect(eWall *pWall)
{
    //Ignore player walls for now
    gPlayerWall *pPlayerWall = dynamic_cast<gPlayerWall*>(pWall);
    if (pPlayerWall)
    {
        return;
    }

    // determine the point closest to s_zoneWallInteractionCoord
    eCoord normal = pWall->Vec().Conj();
    normal.Normalize();

    eCoord Pos1 = normal.Turn( pWall->EndPoint(0) - s_zoneWallInteractionCoord );
    eCoord Pos2 = normal.Turn( pWall->EndPoint(1) - s_zoneWallInteractionCoord );

    tASSERT( fabs( Pos1.y - Pos2.y ) <= fabs( Pos1.y + Pos2.y + .1f ) * .1f );

    REAL alpha = .5f;
    if ( Pos1.x != Pos2.x)
        alpha = Pos1.x / ( Pos1.x - Pos2.x );

    eCoord closestCoord = pWall->Point(alpha);

    if (!s_zoneWallInteractionFound)
    {
        s_zoneWallInteractionClosestCoord = closestCoord;
        s_zoneWallInteractionFound = true;
    }
    else
    {
        if ((closestCoord - s_zoneWallInteractionCoord).NormSquared() <
            (s_zoneWallInteractionClosestCoord - s_zoneWallInteractionCoord).NormSquared())
        {
            s_zoneWallInteractionClosestCoord = closestCoord;
        }
    }
}

// END: zone bouncing base

gZone & gZone::SetOwner(ePlayerNetID *pOwner)
{
    pOwner_ = pOwner;
    if (pOwner)
        team = pOwner->CurrentTeam();
    else
        team = NULL;
    return *this;
}

void gZone::BounceOffPoint(eCoord dest, eCoord collide)
{
    //Use a simple angle deflection for now not even accounting for points that made too far in
    eCoord V = GetVelocity();
                                 // adjust position to be like at at impact time
    eCoord P = GetPosition()+V*s_zoneWallInteractionImpactTime;
    eCoord base = collide - P;   // get normal vector to the impact
    base.Normalize();
                                 // adjust speed
    V = V - base*(eCoord::F(base,V)*2);
                                 // adjust position according to new speed
    P = P-V*s_zoneWallInteractionImpactTime;
    SetReferenceTime();
    SetVelocity(V);
    SetPosition(P);
    RequestSync();
}

// *******************************************************************************
// *
// *    Collapse
// *
// *******************************************************************************

void gZone::Collapse()
{
    Vanish(-1);
    destroyed_ = true;
    SetReferenceTime();
    SetExpansionSpeed(-GetRadius());
    SetRadius(0);
    RequestSync();
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

static bool sg_zoneWallDeath = false;
static tSettingItem<bool> sg_zoneWallDeathConf("ZONE_WALL_DEATH", sg_zoneWallDeath);

static bool sg_zoneWallBoundaryRestrict = true;
static tSettingItem<bool> sg_zoneWallBoundaryRestrictedConf("ZONE_WALL_BOUNDARY_VALUE_RESTRICTED", sg_zoneWallBoundaryRestrict);

static REAL sg_zoneWallBoundary = -20.0;
bool restrictZoneBoundry(const REAL &newValue)
{
    //  we cannot have the boundry limit be greater than -1
    if(sg_zoneWallBoundaryRestrict)
    if (newValue > 0) return false;
    return true;
}
static tSettingItem<REAL> sg_zoneWallBoundaryConf("ZONE_WALL_BOUNDARY", sg_zoneWallBoundary, &restrictZoneBoundry);

static REAL sg_objectZoneZoneEnteredRate = 0.2;
static tSettingItem<REAL> sg_objectZoneZoneEnteredRateConf("LADDERLOG_OBJECTZONE_ZONE_ENTERED_POLLRATE", sg_objectZoneZoneEnteredRate);

static eLadderLogWriter sg_OnlineZoneWriter("ONLINE_ZONE", false);

static eLadderLogWriter sg_zoneRouteStopWriter("ZONE_ROUTE_STOPPED", true);

bool gZone::Timestep( REAL time )
{
    if ((sn_GetNetState() != nCLIENT) && destroyed_)
    {
        //Keep the zone around on the server side so clients will sync

        //??? Without this, the last update and RequestSync() DOES NOT make
        //??? it to the clients.  Investigate how to both remove the zone from
        //??? game AND sync the last update on time.  Removing from game gets to
        //??? the client much later than the sync does if not killed and looks bad.
        //return false;
        
        //nelg: I couldn't reproduce what the above message is referring to. 
        //nelg: From what I've seen, the only result is additional CPU usage
        //nelg: (and subsequently, lag) as more zones are spawned and destroyed.
        return true;
    }

    if (!zoneInit_)
    {
        this->newImpactTime_ = time+5;

        /*
        sg_OnlineZoneWriter << GetID() << name_ << effect_ << MapPosition().x << MapPosition().y << GetVelocity().x << GetVelocity().y
                            << GetRadius() << GetExpansionSpeed();

        */

        zoneInit_ = true;
    }
    
    //HACK to have objectzones interact with other zones.
    if(interactWithZone_ && dynamic_cast<gObjectZoneHack*>(this))
    {
        if(sg_objectZoneZoneEnteredRate >= 0)
        {
            if(time > lastPollTime_+sg_objectZoneZoneEnteredRate)
            {
                for(std::deque<gZone *>::const_iterator i = sg_Zones.begin(); i != sg_Zones.end(); ++i)
                {
                    InteractWith(dynamic_cast<eGameObject*>(*i),time,0);
                }
                lastPollTime_ = time;
            }
        }
    }

    bool doRequestSync = false;

    // resize
    if (resizeRequested_)
    {
        REAL r = GetRadius();
        REAL s = GetExpansionSpeed();
        if (((s>0)&&(r>expectedRadius_)) || ((s<0)&&(r<expectedRadius_)))
        {
            SetReferenceTime();
            SetRadius(expectedRadius_);
            SetExpansionSpeed(previousExpansionSpeed_);
            resizeRequested_ = false;
            doRequestSync = true;
        }
    }

    // rotate
    REAL speed = GetRotationSpeed();
    REAL angle = ( time - lastTime ) * speed;
    // angle /= ( 1 + 2 * 3.14159 * angle/sg_segments );
    rotation_ = rotation_.Turn( cos( angle ), sin( angle ) );

    // delta time
    REAL dt = time - referenceTime_;

    // move to new position
    Move( eCoord( posx_( dt ), posy_( dt ) ), lastTime, time );

    // decay speed
    eCoord V = GetVelocity();
    if (sg_ballSpeedDecay)
    {
            REAL SpeedFactor = V.Norm();
            if( SpeedFactor > 0 )
            {
                SpeedFactor = SpeedFactor - dt*sg_ballSpeedDecay;
                SpeedFactor = SpeedFactor<0 ? 0 : SpeedFactor;
                V.Normalize();
                SetVelocity(V * SpeedFactor);
                doRequestSync = true;
            }
    }

    // update time
    lastTime = time;

    // kill this zone if it shrunk down to zero radius
    if ( GetExpansionSpeed() < 0 && GetRadius() <= 0 )
    {
        OnVanish();

        destroyed_ = true;
        return false;
    }

    if (sn_GetNetState() != nCLIENT)
    {
        // check if wall interactions are enabled
        if (wallInteract_)
        {
            if(((posx_.GetSlope() != 0) || (posy_.GetSlope() != 0)) && this->newImpactTime_ < time)
            {
                // Oops, we've overstepped the wall! Well, put the zone where it would have been...
                SetPosition( GetPosition() - ( V * ( time - this->newImpactTime_ ) ) );
            }

            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCoord = GetPosition();
            s_zoneWallInteractionRadius = GetRadius();
            s_zoneWallInteractionZoneVelocity = V;
            s_zoneWallInteractionZoneDeltaTime = time - lastImpactTime_;
            s_zoneWallInteractionZoneDeltaTime = (s_zoneWallInteractionZoneDeltaTime>1)?1:s_zoneWallInteractionZoneDeltaTime;

            // First, check against the wall we're supposed to collide with
            if(this->newImpactWall_)
            {
                S_ZoneWallInteraction(this->newImpactWall_);
            }

            // And, how sensors are currently integrated,
            // it is very much possible to not see walls
            // simply because they are not the ones right in front of us
            if(!s_zoneWallInteractionFound)
            {
            grid->ProcessWallsInRange(&S_ZoneWallInteraction,
                s_zoneWallInteractionCoord,
                s_zoneWallInteractionRadius*1.5,
                CurrentFace());
            }

            if (s_zoneWallInteractionFound)
            {
                if (wallBouncesLeft_ == 0)
                {
                    if ((wallPenetrate_ && !eWallRim::IsBound(GetPosition(), 0)) || (!wallPenetrate_))
                    {
                        // kill the zone as we hit a wall
                        //??? make kill code a common function
                        Collapse();
                        return false;
                    }
                }
                else
                {
                    if (wallPenetrate_ && !eWallRim::IsBound(GetPosition(), 0))
                    {
                        // bounce off wall
                        BounceOffPoint(GetPosition(), s_zoneWallInteractionImpactCoord);
                        lastImpactTime_ = time + s_zoneWallInteractionImpactTime;

                        if (wallBouncesLeft_ > 0)
                        {
                            wallBouncesLeft_--;
                        }
                    }
                    else
                    {
                        // bounce off wall
                        BounceOffPoint(GetPosition(), s_zoneWallInteractionImpactCoord);
                        lastImpactTime_ = time + s_zoneWallInteractionImpactTime;

                        if (wallBouncesLeft_ > 0)
                        {
                            wallBouncesLeft_--;
                        }
                    }
                    
                    if(sg_ballSpeedHitDecay > 0)
                    {
                        eCoord V = GetVelocity();
                        REAL speed = V.Norm();
                        if(speed > 0)
                        {
                            speed -= sg_ballSpeedHitDecay;
                            if(speed < 0) speed = 0;
                            V.Normalize();
                            SetVelocity(V*speed);
                            RequestSync();
                        }
                    }
                }
            }

            // Go ahead and run sensors...
            ZoneBounceSensor test(this,GetPosition(),GetVelocity());
            test.detect(5); // How many seconds into the future to check?

            // Are we close enough to a wall?
            if(test.ehit)
            {
                // Determine when we're hitting it, taking radius into consideration.
                this->newImpactTime_ = time+(test.hit-(GetRadius()/V.Norm()));
                this->newImpactWall_ = test.wall;
            }
            else
            {
                // We won't hit anything within the timeframe
                this->newImpactTime_ = time+5;
                this->newImpactWall_ = nullptr;
            }

            // Wait until next timestep to do anything else.
            if(s_zoneWallInteractionFound) return false;

            // only clip if zone is moving
            if ((posx_.GetSlope() != 0) || (posy_.GetSlope() != 0))
            {
                // clip movement to rim walls
                eCoord dest(posx_(dt), posy_(dt));
                tStackObject< ePoint > start(pos),stop(dest);

                REAL clip = eWallRim::Clip(start,stop,0);

                if (clip < 1 || !eWallRim::IsBound(GetPosition(), sg_zoneWallBoundary))
                {
                    gBallZoneHack *thisBallZone = dynamic_cast<gBallZoneHack *>(this);
                    if (thisBallZone && sg_ballAutoRespawn)
                    {
                        // just respawn the ball at the original location
                        thisBallZone->GoHome();
                    }
                    else
                    {
                        gSoccerZoneHack *thisSoccerBall = dynamic_cast<gSoccerZoneHack *>(this);
                        if (thisSoccerBall && (thisSoccerBall->GetType() == gSoccerZoneHack::gSoccer_BALL))
                        {
                            thisSoccerBall->GoHome();
                        }
                        else
                        {
                            Collapse(); //Don't allow going outside, destroy it
                            return false;
                        }
                    }
                }
            }
        }
        else
        {
            if (sg_zoneWallDeath && !eWallRim::IsBound(GetPosition(), sg_zoneWallBoundary))
            {
                Collapse();
                return false;
            }
        }

        if(!route_.empty() && time >= nextUpdate_)
        {
            if(nextUpdate_ < 0) nextUpdate_ = time;
            eCoord const &lastPos = route_[lastCoord_++];
            REAL speed = GetVelocity().Norm();
            SetPosition(lastPos);
                                 // no speed, no point...
            if(speed < EPS) route_.clear();
            if(lastCoord_ >= route_.size())
            {
                lastCoord_ = 0;
            }
            eCoord delta = route_[lastCoord_] - lastPos;
            REAL dist = delta.Norm();
            if(dist < EPS)       // next point is the same. Stop the route ...
            {
                eCoord zoneDir = eCoord(0,0);
                SetVelocity(zoneDir);
                route_.clear();
                lastCoord_ = 0;
                nextUpdate_ = -1;
                
                sg_zoneRouteStopWriter << GetEffect() << GetID() << GetName();
                sg_zoneRouteStopWriter << lastPos.x/gArena::SizeMultiplier() << lastPos.y/gArena::SizeMultiplier();
                sg_zoneRouteStopWriter << GetVelocity().x << GetVelocity().y;

                sg_zoneRouteStopWriter.write();
            }
            else
            {
                delta.Normalize();
                SetVelocity(delta * speed);
                nextUpdate_ += dist/speed;
            }
            doRequestSync = true;
        }

        if ( targetRadius_ && (
            ((GetExpansionSpeed() > 0) && (GetRadius() >= targetRadius_))
            ||
            ((GetExpansionSpeed() < 0) && (GetRadius() <= targetRadius_))
            ))
        {
            //??? change this to allow negative fall speed and keeping the target radius
            SetReferenceTime();
            SetExpansionSpeed( -fallSpeed_ );
            targetRadius_ = 0;
            doRequestSync = true;
        }

        if (seeking_)
        {
            if ((!pSeekingCycle_) || (!pSeekingCycle_->Player()) || (!pSeekingCycle_->Alive()))
            {
                //Seeking cycle is gone, shrink
                pSeekingCycle_ = NULL;
                SetReferenceTime();
                SetExpansionSpeed( -GetRadius()*1 );
                targetRadius_ = 0;
                seeking_ = false;

                doRequestSync = true;
            }
        }

        if (doRequestSync)
        {
            RequestSync();
        }
    }

    return false;
}


// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gZone::OnVanish( void )
{
}

// *******************************************************************************
// *
// *    TriggerAvoidZone
// *
// *******************************************************************************
//!
//!     @param  target        the other game object
//!     @param  zone          the zone player is getting near
//!     @param  time          the current time
//!
// *******************************************************************************

bool sg_cyclesZonesAvoid = true;
static tSettingItem<bool> sg_cyclesZonesAvoidConf("CYCLE_ZONES_AVOID", sg_cyclesZonesAvoid);

bool sg_chatbotAvoidZones = false;
static tSettingItem<bool> sg_chatbotAvoidZonesConf("CYCLE_ZONES_AVOID_CHATBOT", sg_chatbotAvoidZones);

REAL sg_cycleZonesApproch = 100.0;
static tSettingItem<REAL>sg_cycleZoneApprochConf("CYCLE_ZONES_APPROCH", sg_cycleZonesApproch);

static void TriggerAvoidZone(gCycle *target, gZone *Zone, REAL currentTime)
{
    //  don't execute if players won't want to avoid zones
    if (!sg_cyclesZonesAvoid)
        return;

    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return;

    if(!target->Alive() || target->GetLastTurnTime()+target->GetTurnDelay() > currentTime) return;
    
    ePlayerNetID *player = target->Player();
    if ((!player->IsHuman()) || (sg_chatbotAvoidZones && player->IsChatting() && player->IsActive()))
    {
        REAL tarX = target->Position().x;
        REAL tarY = target->Position().y;
        REAL zonX = Zone->Position().x;
        REAL zonY = Zone->Position().y;

        REAL tarDirX = target->Direction().x;
        REAL tarDirY = target->Direction().y;
        
        bool shouldTurn = false;
        //A bad way to do this: loop through the zone's perimeter and determine if they intersect with the cycle direction
        REAL radi = Zone->GetRadius();
        REAL adjPosX = tarX+(tarDirX*sg_cycleZonesApproch), adjPosY = tarY+(tarDirY*sg_cycleZonesApproch);
        REAL pX, pY, s1_x, s1_y, s2_x, s2_y, s, t, lpX = radi, lpY = 0;
        for(int i=41;i>0;--i)
        {
            pX = (radi*cos(M_PI*2*(i/42.f)))+zonX; pY = (radi*sin(M_PI*2*(i/42.f)))+zonY;
            
            s1_x = adjPosX - tarX; s1_y = adjPosY - tarY; s2_x = pX - lpX; s2_y = pY - lpY;
            s = (-s1_y * (tarX - lpX) + s1_x * (tarY - lpY)) / (-s2_x * s1_y + s1_x * s2_y);
            t = ( s2_x * (tarY - lpY) - s2_y * (tarX - lpX)) / (-s2_x * s1_y + s1_x * s2_y);
            
            if(s >= 0 && s <= 1 && t >= 0 && t <= 1) { shouldTurn = true; break; };
            
            lpX = pX; lpY = pY;
        }
        
        if(!shouldTurn)
            return;
        
#ifdef DEBUG
        con << player->GetName() << " turning away from zone...\n";
#endif

        if ((tarX == zonX) || (tarY == zonY))
        {
            target->Act(&gCycle::se_turnRight, 1);
        }

        if (tarDirX >= 0)
        {
            if (tarX < zonX)
            {
                if (tarY < zonY)
                    target->Act(&gCycle::se_turnRight, 1);
                else
                    target->Act(&gCycle::se_turnLeft, 1);
            }
        }
        else if (tarDirX <= -0.01)
        {
            if (tarX > zonX)
            {
                if (tarY < zonY)
                    target->Act(&gCycle::se_turnLeft, 1);
                else
                    target->Act(&gCycle::se_turnRight, 1);
            }
        }
        if (tarDirY >= 0)
        {
            if (tarY < zonY)
            {
                if (tarX < zonX)
                    target->Act(&gCycle::se_turnLeft, 1);
                else
                    target->Act(&gCycle::se_turnRight, 1);
            }
        }
        else if (tarDirY <= -0.01)
        {
            if (tarY > zonY)
            {
                if (tarX < zonX)
                    target->Act(&gCycle::se_turnRight, 1);
                else
                    target->Act(&gCycle::se_turnLeft, 1);
            }
        }
    }

    //  if all fails, cycle will enter the zone
}


// *******************************************************************************
// *
// *    InteractWith
// *
// *******************************************************************************
//!
//!     @param  target        the other game object
//!     @param  time          the current time
//!     @param  recursion     if set to true, don't recurse into other InteractWith functions (quite silly now that I think about it...)
//!
// *******************************************************************************

void gZone::InteractWith( eGameObject * target, REAL time, int recursion )
{
    if(target == this || destroyed_) return;
    
    gCycle* prey = dynamic_cast< gCycle* >( target );
    if ( InteractsWithCycle() && prey )
    {
        if(prey->Player() && prey->Alive())
        {
            REAL r = GetRadius(), dist = ( prey->Position() - this->Position() ).NormSquared();
            r *= r;
            if ( dist <= r )
            {
                OnEnter( prey, time );
                if (!isInside(prey)) // make sure this cycle isn't in zone already
                {
                    //  if not, then add them to the list
                    AddPlayerInteraction(prey);
                }
                return;
            }
            else if ( dist < (r + (sg_cycleZonesApproch * 2)) )
            {
                TriggerAvoidZone(prey, this, time);
                OnNear(prey, time);
            }
        }
        if (isInside(prey)) // make sure this cycle already is within the zone
        {
            OnExit(prey, time); // call the OnExit event
            RemovePlayerInteraction(prey);  // remove them from the list
        }
        return;
    }
    
    gZone *zone = dynamic_cast<gZone *>(target);
    if( InteractsWithZone() && zone )
    {
        if(zone->destroyed_)
        {
            if(isInside(zone))
            {
                OnExit(zone, time);
                RemoveZoneInteraction(zone);
            }
        }
        else
        {
            REAL dis = this->Radius() + zone->Radius();
            if ( ( zone->Position() - this->Position() ).NormSquared() < (dis * dis) )
            {
                if(!isInside(zone))
                {
                    OnEnter(zone, time);
                    AddZoneInteraction(zone);
                }
            }
            else if(isInside(zone))
            {
                OnExit(zone, time);
                RemoveZoneInteraction(zone);
            }
        }
        return;
    }
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnEnter( gCycle * target, REAL time )
{
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the zone that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnEnter( gZone * target, REAL time )
{
}

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has left the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnExit( gCycle *target, REAL time )
{
}

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the zone that has left the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnExit( gZone *target, REAL time )
{
}

// *******************************************************************************
// *
// *    OnCycleDestroyed
// *
// *******************************************************************************
//!
//!     @param  cycle   the cycle that has been destroyed
//!     @param  time    the current time
//!
// *******************************************************************************
void gZone::OnCycleDestroyed(gCycle *cycle,REAL time)
{
    if(isInside(cycle))
    {
        OnExit(cycle,time);
        RemovePlayerInteraction(cycle);
    }
}

// *******************************************************************************
// *
// *    OnNear
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that is near the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnNear( gCycle *target, REAL time )
{
}

// *******************************************************************************
// *
// *    OnNear
// *
// *******************************************************************************
//!
//!     @param  target  the zone that is near the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gZone::OnNear( gZone *target, REAL time )
{
}

// *******************************************************************************
// *
// *    Destroy
// *
// *******************************************************************************

void gZone::Destroy()
{
    // destroy the zone by setting a negative expansion and a zero radius
    destroyed_ = true;
    SetReferenceTime();
    SetExpansionSpeed(-1);
    SetRadius(0);
    RequestSync();
}


// the zone's network initializator
static nNOInitialisator<gZone> zone_init(340,"zone");

// *******************************************************************************
// *
// *    CreatorDescriptor
// *
// *******************************************************************************
//!
//!     @return
//!
// *******************************************************************************

nDescriptor & gZone::CreatorDescriptor( void ) const
{
    return zone_init;
}


// *******************************************************************************
// *
// *    Radius
// *
// *******************************************************************************
//!
//!     @return
//!
// *******************************************************************************

REAL gZone::Radius( void ) const
{
    return GetRadius();
}


// extra alpha blending factors
static REAL sg_zoneAlpha = 1.0, sg_zoneAlphaServer = 1.0;
static tSettingItem< REAL > sg_zoneAlphaConf( "ZONE_ALPHA", sg_zoneAlpha );
static nSettingItem< REAL > sg_zoneAlphaConfServer( "ZONE_ALPHA_SERVER", sg_zoneAlphaServer );

// *******************************************************************************
// *
// *    Render
// *
// *******************************************************************************
//!
//!     @param  cam  the camera used for rendering
//!
// *******************************************************************************

void gZone::Render( const eCamera * cam )
{
    #ifndef DEDICATED
    if ( sg_zoneSegLength <= 0 )
        sg_zoneSegLength = .5;
    if ( sg_zoneSegments < 1 )
        sg_zoneSegments = 11;
    
    if(lastTime == 0)
    {
        static REAL fakeTime = 0;
        if(createTime_ == 0)
        {
            fakeTime = tSysTimeFloat();
        }
        createTime_ -= (tSysTimeFloat()-fakeTime)/20;
    }

    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
    alpha = .7f;
    if ( alpha <= 0 )
        return;
    alpha *= sg_zoneAlpha * sg_zoneAlphaServer;

        ModelMatrix();
    glPushMatrix();

    REAL seglen = 2 * M_PI / sg_zoneSegments * sg_zoneSegLength;

    REAL r = Radius();
    GLfloat m[4][4]={{r*rotation_.x,r*rotation_.y,0,0},
                     {-r*rotation_.y,r*rotation_.x,0,0},
                     {0,0,sg_zoneHeight,0},
                     {pos.x,pos.y,sg_zoneBottom,1}};

    glMultMatrixf(&m[0][0]);

    glColor4f( color_.r ,color_.g,color_.b, alpha );

    bool useAlpha = sr_alphaBlend ? !sg_zoneAlphaToggle : sg_zoneAlphaToggle;
    static bool lastAlpha = useAlpha;

    static rDisplayList zoneList;
    if ( lastAlpha != useAlpha || !zoneList.Call() )
    {
        lastAlpha = useAlpha;

        rDisplayListFiller filler( zoneList );

        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glDisable(GL_TEXTURE_2D);

        if ( useAlpha )
            BeginQuads();
        else
        {
            sr_DepthOffset(true);
            BeginLineStrip();
        }

        for ( int i = sg_zoneSegments - 1; i>=0; --i )
        {
            REAL a = i * 2 * M_PI / REAL( sg_zoneSegments );
            REAL b = a + seglen;

            REAL sa = sin(a);
            REAL ca = cos(a);
            REAL sb = sin(b);
            REAL cb = cos(b);

            glVertex3f(sa, ca, 0);
            glVertex3f(sa, ca, 1);
            glVertex3f(sb, cb, 1);
            glVertex3f(sb, cb, 0);

            if ( !useAlpha )
            {
                glVertex3f(sa, ca, 0);
                RenderEnd();
                BeginLineStrip();
            }
        }

        RenderEnd();

        sr_DepthOffset(false);
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDepthMask(GL_TRUE);
    }

    glPopMatrix();
#endif
}


// *******************************************************************************
// *
// *    DrawSvg
// *
// *******************************************************************************
//!
//!     @return
//!
// *******************************************************************************
//! draws it in a svg file
void gZone::DrawSvg(std::ofstream &f) {
    REAL r = Radius();
    REAL dash = 2 * r * M_PI / 20;

    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
    alpha = .7f;
    if ( alpha <= 0 )
        return;
    alpha *= sg_zoneAlpha * sg_zoneAlphaServer;

    f << "  <circle cx=\"0\" cy=\"0\" r=\"" << r << "\" fill=\"none\" stroke='" << gSvgColor(color_) << "' stroke-width=\"1\" stroke-dasharray=\""
      << dash << ", " << dash << "\" opacity=\"" <<  alpha << "\" transform=\"translate("
      << pos.x << " " << -pos.y << ")\">\n";
    REAL speed = GetRotationSpeed();
    if(fabs(speed) > EPS) {
        REAL t = fabs(2*M_PI/speed);
        f << "    <animateTransform attributeName=\"transform\" attributeType=\"XML\" type=\"rotate\" from=\"0\" to=\"";
        if(speed < 0) {
            f << '-';
        }
        f << "360\" dur=\""
          << t << "\" repeatCount=\"indefinite\" additive=\"sum\" />\n";
    }
    f << "  </circle>\n";

}


// *******************************************************************************
// *
// *    RendersAlpha
// *
// *******************************************************************************
//!
//!     @return True if alpha blending is used
//!
// *******************************************************************************
bool gZone::RendersAlpha() const
{
    return sr_alphaBlend ? !sg_zoneAlphaToggle : sg_zoneAlphaToggle;
}

// *******************************************************************************
// *
// *	Vanish
// *
// *******************************************************************************
//!
//!		@param	factor vanishing speed factor
//!
// *******************************************************************************

void gZone::Vanish( REAL factor )
{
    if ( targetRadius_ > 0 || GetExpansionSpeed() >= -GetRadius() || (resizeRequested_ && expectedRadius_ > 0) )
    {
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius() * factor );
        SetRadiusSmoothly( 0, GetExpansionSpeed() );
        SetTargetRadius(0);
        RequestSync();
    }
}

// *******************************************************************************
// *
// *    Win Zone Color Commands
// *    Death Zone Color Commands
// *
// *******************************************************************************
//!
//!     @return True if alpha blending is used
//!
// *******************************************************************************
static int sg_ColorWinZoneRed = 0;
static tSettingItem<int> sg_ColorWinZoneRedCONF("COLOR_WINZONE_RED", sg_ColorWinZoneRed);

static int sg_ColorWinZoneBlue = 0;
static tSettingItem<int> sg_ColorWinZoneBlueCONF("COLOR_WINZONE_BLUE", sg_ColorWinZoneBlue);

static int sg_ColorWinZoneGreen = 15;
static tSettingItem<int> sg_ColorWinZoneGreenCONF("COLOR_WINZONE_GREEN", sg_ColorWinZoneGreen);

static int sg_ColorDeathZoneRed = 15;
static tSettingItem<int> sg_ColorDeathZoneRedCONF("COLOR_DEATHZONE_RED", sg_ColorDeathZoneRed);

static int sg_ColorDeathZoneBlue = 0;
static tSettingItem<int> sg_ColorDeathZoneBlueCONF("COLOR_DEATHZONE_BLUE", sg_ColorDeathZoneBlue);

static int sg_ColorDeathZoneGreen = 0;
static tSettingItem<int> sg_ColorDeathZoneGreenCONF("COLOR_DEATHZONE_GREEN", sg_ColorDeathZoneGreen);

// *******************************************************************************
// *
// *    gWinZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************


gWinZoneHack::gWinZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    if (sg_SwapWinDeath == false)
    {
        color_.r = sg_ColorWinZoneRed / 15.0f;//0.0f;
        color_.b = sg_ColorWinZoneBlue / 15.0f;//1.0f;
        color_.g = sg_ColorWinZoneGreen / 15.0f;//0.0f;
    }
    else
    {
        color_.r = sg_ColorDeathZoneRed / 15.0f;//1.0f;
        color_.b = sg_ColorDeathZoneBlue / 15.0f;//0.0f;
        color_.g = sg_ColorDeathZoneGreen / 15.0f;//0.0f;
    }
}


// *******************************************************************************
// *
// *    gWinZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gWinZoneHack::gWinZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gWinZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gWinZoneHack::~gWinZoneHack( void )
{
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

static eLadderLogWriter sg_winzonePlayerEnterWriter("WINZONE_PLAYER_ENTER", true);

bool sg_winZonePlayerEnteredWin = true;
static tSettingItem<bool> sg_winZonePlayerEnteredWinConf("WINZONE_PLAYER_ENTER_WIN", sg_winZonePlayerEnteredWin);

void gWinZoneHack::OnEnter( gCycle * target, REAL time )
{
    sg_winzonePlayerEnterWriter << this->GetID() << name_ << MapPosition().x << MapPosition().y << target->Player()->GetUserName() << target->MapPosition().x << target->MapPosition().y << target->Direction().x << target->Direction().y << time;
    sg_winzonePlayerEnterWriter.write();

    //HACK RACE begin
    if ( sg_RaceTimerEnabled )
    {
        gRace::ZoneHit( target->Player(), time );
    }
    else
    {
        if (sg_winZonePlayerEnteredWin)
        {
            if (!winnerPlayer_)
            {
                LogWinnerCycleTurns(target);
                winnerPlayer_ = true;
            }

            static const char* message="$player_win_instant";
            sg_DeclareWinner( target->Player()->CurrentTeam(), message );
            destroyed_ = true;
            Vanish( 0.5 );
        }
    }
    //HACK RACE end
}

void gWinZoneHack::OnExit( gCycle *target, REAL time )
{
    if (sg_RaceTimerEnabled)
    {
        gRace::ZoneOut( target->Player(), time );
    }
}


// *******************************************************************************
// *
// *    gDeathZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

bool sg_deathZoneRandomColors = false;
static tSettingItem<bool> sg_deathZoneRandomColorsConf("DEATHZONE_RANDOM_COLORS", sg_deathZoneRandomColors);

bool sg_deathZoneRotation = false;
static tSettingItem<bool> sg_deathZoneRotationConf("DEATHZONE_ROTATION", sg_deathZoneRotation);

REAL sg_deathZoneRotationSpeed = 0.3f;
static tSettingItem<REAL> sg_deathZoneRotationSpeedConf("DEATHZONE_ROTATION_SPEED", sg_deathZoneRotationSpeed);

gDeathZoneHack::gDeathZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner, bool delayCreation )
:gZone( grid, pos, dynamicCreation, delayCreation )
{
    pLastShotCollision = NULL;

    if (!sg_deathZoneRandomColors)
    {
        if (sg_SwapWinDeath == false)
        {
            color_.r = sg_ColorDeathZoneRed / 15.0f;//1.0f;
            color_.b = sg_ColorDeathZoneBlue / 15.0f;//0.0f;
            color_.g = sg_ColorDeathZoneGreen / 15.0f;//0.0f;
        }
        else
        {
            color_.r = sg_ColorWinZoneRed / 15.0f;//0.0f;
            color_.b = sg_ColorWinZoneBlue / 15.0f;//1.0f;
            color_.g = sg_ColorWinZoneGreen / 15.0f;//0.0f;
        }
    }
    else
    {
        tRandomizer &randomizer = tRandomizer::GetInstance();
        REAL colorR = randomizer.Get(16) / 15.0f;
        REAL colorG = randomizer.Get(16) / 15.0f;
        REAL colorB = randomizer.Get(16) / 15.0f;

        color_.r = colorR;
        color_.b = colorG;
        color_.g = colorB;
    }

    if (sg_deathZoneRotation)
        SetRotationSpeed(sg_deathZoneRotationSpeed);

    if (teamowner!=NULL)
    {
        team = teamowner;
        deathZoneType = TYPE_TEAM;
    }
    else
    {
        deathZoneType = TYPE_NORMAL;
    }
}


// *******************************************************************************
// *
// *    gDeathZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************



gDeathZoneHack::gDeathZoneHack( nMessage & m )
: gZone( m )
{
    deathZoneType = TYPE_NORMAL;
    pLastShotCollision = NULL;

    if (sg_deathZoneRotation)
        SetRotationSpeed(sg_deathZoneRotationSpeed);
}


// *******************************************************************************
// *
// *    ~gDeathZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gDeathZoneHack::~gDeathZoneHack( void )
{
    if( pLastShotCollision )
    {
        pLastShotCollision->pLastShotCollision = NULL;
        pLastShotCollision = NULL;
    }
}

bool gDeathZoneHack::Timestep(REAL time)
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    if (seeking_)
    {
        //Only run this every poll time to save on network traffic
        if (lastTime >= (lastSeekTime_ + sg_shotSeekUpdateTime))
        {
            //Calculate new direction
            eCoord newDir = pSeekingCycle_->Position() - GetPosition();
            newDir.Normalize();

            SetReferenceTime();
            SetVelocity(newDir * sg_zombieZoneSpeed);
            //??? move out all seeking variables into base zone

            lastSeekTime_ = lastTime;

            RequestSync();
        }
    }

    return returnStatus;
}


static int score_deathzone=-1;
static tSettingItem<int> s_dz("SCORE_DEATHZONE",score_deathzone);
static int score_deathzone_team=-1;
static tSettingItem<int> s_dz_team("SCORE_DEATHZONE_TEAM",score_deathzone_team);

void gDeathZoneHack::OnVanish( void )
{
    if(interactWithZone_)
        grid->RemoveGameObjectInteresting(this);
}


extern int score_die;
extern int score_kill;
extern int score_suicide;

static int score_shot=1;
static tSettingItem<int> s_score_shot("SCORE_SHOT",score_shot);

static int score_shot_suicide=0;
static tSettingItem<int> s_score_shot_suicide("SCORE_SHOT_SUICIDE",score_shot_suicide);

static int score_death_shot=1;
static tSettingItem<int> s_score_death_shot("SCORE_DEATH_SHOT",score_death_shot);

static int score_self_destruct=1;
static tSettingItem<int> s_score_self_destruct("SCORE_SELF_DESTRUCT",score_self_destruct);

static int score_zombie_zone_revenge=1;
static tSettingItem<int> s_score_zombie_zone_revenge("SCORE_ZOMBIE_ZONE_REVENGE",score_zombie_zone_revenge);

static int score_zombie_zone=0;
static tSettingItem<int> s_score_zombie_zone("SCORE_ZOMBIE_ZONE",score_zombie_zone);

static bool sg_shotKillSelf = false;
static tSettingItem<bool> conf_shotKillSelf ("SHOT_KILL_SELF", sg_shotKillSelf);

static bool sg_shotKillEnemies = true;
static tSettingItem<bool> conf_shotKillEnemies ("SHOT_KILL_ENEMIES", sg_shotKillEnemies);

static bool sg_shotKillVanish = true;
static tSettingItem<bool> conf_shotKillVanish ("SHOT_KILL_VANISH", sg_shotKillVanish);

static bool sg_selfDestructVanish = true;
static tSettingItem<bool> conf_selfDestructVanish ("SELF_DESTRUCT_VANISH", sg_selfDestructVanish);

static bool sg_zombieZoneVanish = true;
static tSettingItem<bool> conf_zombieZoneVanish ("ZOMBIE_ZONE_VANISH", sg_zombieZoneVanish);

static float sg_zombieZoneShoot = true;
static tSettingItem<float> conf_zombieZoneShoot ("ZOMBIE_ZONE_SHOOT", sg_zombieZoneShoot);

//??? Warning - shot collision isn't working quite right
static bool sg_shotCollision = false;
static tSettingItem<bool> conf_shotCollision ("SHOT_COLLISION", sg_shotCollision);

static int sg_shotWallBounce = 0;
static tSettingItem<int> sg_shotWallBounceConf ("SHOT_WALL_BOUNCE", sg_shotWallBounce);

static bool sg_shotKillInvulnerable = 1;
static tSettingItem<bool> sg_shotKillInvulnerableConf( "SHOT_KILL_INVULNERABLE", sg_shotKillInvulnerable );

gZone & gDeathZoneHack::SetType(int type)
{
    if (type < NUM_DEATH_ZONE_TYPES)
    {
        deathZoneType = type;

        if ((type == TYPE_SHOT) ||
            (type == TYPE_DEATH_SHOT))
        {
            // set wall interaction to kill the zone on a wall
            wallInteract_ = true;

            wallBouncesLeft_ = sg_shotWallBounce;

            if (wallBouncesLeft_ < 0)
            {
                wallBouncesLeft_ = 0;
            }
        }
        
        switch(type)
        {
            case TYPE_SHOT: case TYPE_DEATH_SHOT:
                if(!sg_shotCollision)
                {
                    // Make it so we aren't seen by other objects.
                    // We'll send the signal to zombie zones ourselves later.
                    interactWithZone_ = true;
                    if(interestingID != -1)
                    {
                        grid->RemoveGameObjectInteresting(this);
                        interestingID=-1;
                    }
                    break;
                }
                // Otherwise, we have to be visible to ourselves for shot collisions to "work".
                // falls through
            case TYPE_ZOMBIE_ZONE: 
                if(!interactWithZone_)
                {
                    interactWithZone_ = true;
                    grid->AddGameObjectInteresting(this);
                }
                break;
            default:
                // No zone to zone interaction required for other deathzone types.
                if(interactWithZone_)
                    interactWithZone_ = false;
                if(interestingID != -1)
                {
                    grid->RemoveGameObjectInteresting(this);
                    interestingID=-1;
                }
                break;
        }
    }

    return (*this);
}


extern tList<ePlayerNetID> se_PlayerNetIDs;

ePlayerNetID * validatePlayer(ePlayerNetID *pPlayer)
{
    // with smart pointers, all players are now valid.
    return pPlayer;

    // old code when player pointers were dumb pointers, before dereferencing them,
    // it was required to check whether they were still alive (not very safe, by the way.)
    /*
    if (pPlayer)
    {
        int i;
        for (i = se_PlayerNetIDs.Len()-1; i>=0; --i)
        {
            if (se_PlayerNetIDs(i) == pPlayer)
            {
                break;
            }
        }

        if (i < 0)
        {
            pPlayer = NULL;
        }
    }

    return (pPlayer);
    */
}


gCycle * gDeathZoneHack::getPlayerCycle(ePlayerNetID *pPlayer)
{
    gCycle *pPlayerCycle = NULL;
    if (pPlayer)
    {
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *pCycle=dynamic_cast<gCycle *>(gameObjects(i));

            if (pCycle)
            {
                if (pCycle->Player() == pOwner_)
                {
                    pPlayerCycle = pCycle;
                    break;
                }
            }
        }
    }

    return (pPlayerCycle);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************
static eLadderLogWriter sg_deathShotFragWriter("DEATH_SHOT_FRAG", true);
static eLadderLogWriter sg_deathShotSuicideWriter("DEATH_SHOT_SUICIDE", true);
static eLadderLogWriter sg_deathShotTeamkillWriter("DEATH_SHOT_TEAMKILL", true);
static eLadderLogWriter sg_deathDeathZoneWriter("DEATH_DEATHZONE", true);
static eLadderLogWriter sg_deathDeathZoneTeamWriter("DEATH_DEATHZONE_TEAM", false);
static eLadderLogWriter sg_deathZombieZoneWriter("DEATH_ZOMBIEZONE", true);
static eLadderLogWriter sg_deathDeathShotWriter("DEATH_DEATHSHOT", true);
static eLadderLogWriter sg_deathDeathSelfDestructWriter("DEATH_SELF_DESTRUCT", true);

void gDeathZoneHack::OnEnter( gCycle * target, REAL time )
{
    if (!dynamicCreation_ || ( deathZoneType == TYPE_NORMAL && !team ) )
    {
        target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_deathzone");
        target->Kill("DEATHZONE");
        sg_deathDeathZoneWriter << target->Player()->GetUserName();
        sg_deathDeathZoneWriter.write();
    }
    // check normal death zone linked to a team ...
    else if (team && deathZoneType == TYPE_TEAM)
    {
        if ((!target) || (!target->Player()))
        {
            return;
        }
        if (target->Player()->CurrentTeam() == team)
        {
            return;
        }
        else
        {
            target->Player()->AddScore(score_deathzone_team, "", "player_lose_deathzone_team");
            target->deathReason_ << "DEATHZONE_TEAM " << ePlayerNetID::FilterName(team->Name());
            target->Kill();
            sg_deathDeathZoneTeamWriter << ePlayerNetID::FilterName( team->Name() ) << target->Player()->GetUserName();
            sg_deathDeathZoneTeamWriter.write();
        }
    }
    else
    {
        if ( !target->Vulnerable() && !sg_shotKillInvulnerable ) {
            //Checks to see if their cycle is invulnerable, don't kill invulnerable players
            return;
        }

        //Validate the owner player ID
        pOwner_ = validatePlayer(pOwner_);
        //??? Really need for the player to get cleared when they exit, it is possible for
        //a new player to enter and own a zone

        //Get the owner cycle if still around
        gCycle *pOwnerCycle = NULL;
        if (pOwner_)
        {
            pOwnerCycle = getPlayerCycle(pOwner_);

            if (target->Player() == pOwner_)
            {
                //Player entered own shot
                if (!sg_shotKillSelf)
                {
                    //Don't kill yourself
                    return;
                }
                else
                {
                    sg_deathShotSuicideWriter << target->Player()->GetUserName();
                    sg_deathShotSuicideWriter.write();
                    if (!score_shot_suicide)
                    {
                        tColoredString playerName;
                        playerName << *target->Player() << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_free_shot_suicide", playerName ) );
                    }
                    else
                    {
                        target->Player()->AddScore(score_shot_suicide, tOutput(), "$player_shot_suicide");
                    }
                    target->deathReason_ << "SHOT " << target->Player()->GetUserName();
                    target->Kill();
                }
            }
            else
            {
                if (!sg_shotKillEnemies)
                    return;

                //The cycle may have been deleted since...  I think this is OK because Player() returns a checked pointer

                //Another player entered a shot, find ownership
                ePlayerNetID *hunter = pOwner_;
                ePlayerNetID *prey = target->Player();

                tOutput lose;
                tOutput win;

                tColoredString preyName;
                preyName << *prey;
                preyName << tColoredString::ColorString(1,1,1);

                if (prey->CurrentTeam() != team)
                {
                    char const *pWinString = NULL;
                    char const *pFreeString = NULL;
                    int score = 0;
                    if (deathZoneType == TYPE_SHOT)
                    {
                        sg_deathShotFragWriter << prey->GetUserName() << hunter->GetUserName();
                        sg_deathShotFragWriter.write();
                        target->deathReason_ << "SHOT " << hunter->GetUserName();
                        pWinString = "$player_win_shot";
                        pFreeString = "$player_free_shot";
                        score = score_shot;
                    }
                    else if (deathZoneType == TYPE_DEATH_SHOT)
                    {
                        sg_deathDeathShotWriter << prey->GetUserName() << hunter->GetUserName();
                        sg_deathDeathShotWriter.write();
                        target->deathReason_ << "DEATH_SHOT " << hunter->GetUserName();
                        pWinString = "$player_win_death_shot";
                        pFreeString = "$player_free_death_shot";
                        score = score_death_shot;
                    }
                    else if (deathZoneType == TYPE_SELF_DESTRUCT)
                    {
                        sg_deathDeathSelfDestructWriter << prey->GetUserName() << hunter->GetUserName();
                        sg_deathDeathSelfDestructWriter.write();
                        target->deathReason_ << "SELF_DESTRUCT_ZONE " << hunter->GetUserName();
                        pWinString = "$player_win_self_destruct";
                        pFreeString = "$player_free_self_destruct";
                        score = score_self_destruct;
                    }
                    else if (deathZoneType == TYPE_ZOMBIE_ZONE)
                    {
                        sg_deathZombieZoneWriter << prey->GetUserName() << hunter->GetUserName();
                        sg_deathZombieZoneWriter.write();
                        target->deathReason_ << "ZOMBIE_ZONE " << hunter->GetUserName();
                        if (target == pSeekingCycle_)
                        {
                            pWinString = "$player_win_zombie_zone_revenge";
                            pFreeString = "$player_free_zombie_zone_revenge";
                            score = score_zombie_zone_revenge;

                            //Zombie killed his maker - don't seek anymore
                            pSeekingCycle_ = NULL;
                        }
                        else
                        {
                            pWinString = "$player_win_zombie_zone";
                            pFreeString = "$player_free_zombie_zone";
                            score = score_zombie_zone;
                        }
                    }

                    if (score)
                    {
                        win.SetTemplateParameter(3, preyName);
                        win << pWinString;
                        hunter->AddScore(score, win, lose);
                    }
                    else if (pFreeString != NULL)
                    {
                        tColoredString hunterName;
                        hunterName << *hunter << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( pFreeString, hunterName, preyName ) );
                    }
                }
                else
                {
                    //Team member entered shot
                    if (!sg_shotKillSelf)
                    {
                        //Don't kill team
                        return;
                    }
                    if (deathZoneType == TYPE_SHOT)
                    {
                        sg_deathShotTeamkillWriter << prey->GetUserName() << hunter->GetUserName();
                        sg_deathShotTeamkillWriter.write();
                        target->deathReason_ << "SHOT " << hunter->GetUserName();
                        tColoredString hunterName;
                        hunterName << *hunter << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_teamkill", hunterName, preyName ) );
                    }
                }

                target->Killed(pOwnerCycle); //???make this player so it isn't NULL if dead?

                target->Kill();
            }
        }
        else
        {
            if(deathZoneType == TYPE_ZOMBIE_ZONE)
            {
                sg_deathZombieZoneWriter << target->Player()->GetUserName();
                sg_deathZombieZoneWriter.write();
                target->Player()->AddScore(score_zombie_zone, tOutput(), "$player_lose_suicide");
            }
        }

        if (((deathZoneType == TYPE_ZOMBIE_ZONE) && ((!pSeekingCycle_) || (sg_zombieZoneVanish))) ||
            ((deathZoneType == TYPE_SELF_DESTRUCT) && (sg_selfDestructVanish)) ||
            ((deathZoneType < TYPE_SELF_DESTRUCT) && (sg_shotKillVanish)) ||
            ((deathZoneType == TYPE_SHOT && sg_shotKillEnemies)))
        {
#if 0
            SetReferenceTime();
            SetExpansionSpeed( -GetRadius()*1 );
            RequestSync();
#else
            //Instead of making the shot slowly dissapear, we'll get rid of it right away so it doesn't kill the zombie
            destroyed_ = true;
            SetReferenceTime();
            SetExpansionSpeed(-1);
            SetRadius(0);
            RequestSync();
#endif
        }
    }
}


void gDeathZoneHack::OnEnter( gDeathZoneHack * target, REAL time )
{
 //Process if we're a zombie
    if (deathZoneType == TYPE_ZOMBIE_ZONE)
    {
        if (target->deathZoneType == TYPE_ZOMBIE_ZONE)
        {
            //Two zombies met
            if (GetSeekingCycle() == target->GetSeekingCycle())
            {
                //Zombies had same maker, merge
            }
        }
        else if ((sg_zombieZoneShoot > 0) &&
                 ((target->deathZoneType == TYPE_SHOT) ||
                  (target->deathZoneType == TYPE_DEATH_SHOT)))
        {
            //A shot entered the zombie

            //Validate the owner player ID
            pOwner_ = validatePlayer(pOwner_);
            //??? Really need for the player to get cleared when they exit, it is possible for
            //a new player to enter and own a zone

            //Validate the shot owner player ID
            ePlayerNetID *pShotOwner = validatePlayer(target->pOwner_);

            if ((pOwner_) && (pShotOwner))
            {
                //We have both owners
                if ((team != target->team) ||
                    (sg_shotKillSelf))
                {
                    //Shoot the zombie!

                    REAL zombieRadius = targetRadius_;
                    REAL shotRadius = target->GetRadius() * sg_zombieZoneShoot;

                    if (targetRadius_ <= 0)
                    {
                        zombieRadius = GetRadius();
                    }

                    if (shotRadius >= zombieRadius)
                    {
                        //Shot was bigger than the zombie, destroy zombie
                        destroyed_ = true;
                        SetReferenceTime();
                        SetExpansionSpeed(-1);
                        SetRadius(0);
                        RequestSync();

                        tColoredString zombieName;
                        zombieName << *pOwner_;
                        zombieName << tColoredString::ColorString(1,1,1);

                        tColoredString shooterName;
                        shooterName << *pShotOwner << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_free_zombie_zone_die", shooterName, zombieName ) );
                    }
                    else
                    {
                        //Take the zombie and target radius down by the shot radius
                        SetReferenceTime();
                        if (targetRadius_ > 0)
                        {
                            zombieRadius = GetRadius();
                            SetRadius(zombieRadius - (shotRadius * zombieRadius / targetRadius_));
                            targetRadius_ -= shotRadius;
                        }
                        else
                        {
                            SetRadius(zombieRadius - shotRadius);

                        }
                        RequestSync();
                    }

                    //Destroy the shot, it is done
                    target->destroyed_ = true;
                    target->SetReferenceTime();
                    target->SetExpansionSpeed(-1);
                    target->SetRadius(0);
                    target->RequestSync();
                }
            }
        }
    }
    else if ((deathZoneType == TYPE_SHOT) ||
             (deathZoneType == TYPE_DEATH_SHOT))
    {
        if ((target->deathZoneType == TYPE_SHOT) ||
            (target->deathZoneType == TYPE_DEATH_SHOT))
        {
            //??? Warning - shot collision isn't working quite right
            if (sg_shotCollision)
            {
                SetReferenceTime();
                target->SetReferenceTime();

                eCoord position1 = GetPosition();
                eCoord position2 = target->GetPosition();

                eCoord velocity1 = GetVelocity();
                eCoord velocity2 = target->GetVelocity();

                REAL radius1 = GetRadius();
                REAL radius2 = target->GetRadius();

#if 1
                //Base the mass on a shot sphere volume
                REAL massConstant = M_PI * 4 / 3;
                REAL mass1 = radius1 * radius1 * radius1 * massConstant;
                REAL mass2 = radius2 * radius2 * radius2 * massConstant;
#else
                //Base the mass on a shot circle area
                REAL massConstant = M_PI;
                REAL mass1 = radius1 * radius1 * massConstant;
                REAL mass2 = radius2 * radius2 * massConstant;
#endif

                eCoord impact = velocity2 - velocity1;
                eCoord impulse = position2 - position1;
                impulse.Normalize();

                REAL impactSpeed = eCoord::F(impact, impulse);

                REAL sign = 1;
                if (impactSpeed < 0)
                {
                    sign = -1;
                    impactSpeed *= -1;
                }

                impulse *= sqrt(impactSpeed * mass1 * mass2);
                impulse *= sign;

                velocity1 = velocity1 + (impulse * (1 / mass1));
                velocity2 = velocity2 - (impulse * (1 / mass2));

                SetVelocity(velocity1);
                target->SetVelocity(velocity2);

                RequestSync();
                target->RequestSync();

                //Record the last collision so we don't do it again
                pLastShotCollision = target;
                target->pLastShotCollision = this;
            }
        }
        else if (target->deathZoneType == TYPE_ZOMBIE_ZONE)
        {
            if(interestingID == -1)
            {
                // We won't be seen by the zombie; send the signal ourselves
                target->OnEnter(this,time);
            }
        }
    }
}

void gDeathZoneHack::OnEnter( gZone * target, REAL time )
{
    gDeathZoneHack *pDeathZone = dynamic_cast<gDeathZoneHack *>(target);
    if(pDeathZone) OnEnter(pDeathZone, time);
}

void gDeathZoneHack::OnExit( gZone * target, REAL time )
{
    if(dynamic_cast<gDeathZoneHack *>(target))
    {
        if (this->pLastShotCollision == target)
        {
            this->pLastShotCollision = NULL;
        }
    }
    
    if (this->pLastShotCollision == this)
    {
        this->pLastShotCollision = NULL;
    }
}


// *******************************************************************************
// *
// *    gRubberZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

static REAL sg_colorRubberZoneR = 15;
static REAL sg_colorRubberZoneG = 10.5;
static REAL sg_colorRubberZoneB = 3;
static tSettingItem<REAL> sg_colorRubberZoneRConf("COLOR_RUBBERZONE_RED", sg_colorRubberZoneR);
static tSettingItem<REAL> sg_colorRubberZoneGConf("COLOR_RUBBERZONE_GREEN", sg_colorRubberZoneG);
static tSettingItem<REAL> sg_colorRubberZoneBConf("COLOR_RUBBERZONE_BLUE", sg_colorRubberZoneB);

gRubberZoneHack::gRubberZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = sg_colorRubberZoneR / 15.0;
    color_.g = sg_colorRubberZoneG / 15.0;
    color_.b = sg_colorRubberZoneB / 15.0;

    rmRubber = 1.0;
}


// *******************************************************************************
// *
// *    ~gRubberZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gRubberZoneHack::~gRubberZoneHack( void )
{
    SetReferenceTime();
}


static int score_rubberzone=-1;
static tSettingItem<int> s_rz("SCORE_RUBBERZONE",score_rubberzone);

static REAL sg_rubberZoneRate=40;
static tSettingItem<REAL> sg_rubberZoneRateConf("RUBBERZONE_RATE",sg_rubberZoneRate);

void gRubberZoneHack::OnVanish( void )
{
}

gZone & gRubberZoneHack::SetRubber(REAL rubber)
{
    rmRubber = rubber;

    if (rubberType_ == TYPE_RUBBER)
    {
        color_.r = 1.0f;
        REAL p_rubber = (1-(rmRubber/sg_rubberCycle));
        if (p_rubber <0)
            p_rubber=0;
        color_.g = (p_rubber);
        color_.b = (p_rubber/3);
    }

    return (*this);
}

gCycle * gRubberZoneHack::getPlayerCycle(ePlayerNetID *pPlayer)
{
    gCycle *pPlayerCycle = NULL;
    if (pPlayer)
    {
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *pCycle=dynamic_cast<gCycle *>(gameObjects(i));

            if (pCycle)
            {
                if (pCycle->Player() == pOwner_)
                {
                    pPlayerCycle = pCycle;
                    break;
                }
            }
        }
    }

    return (pPlayerCycle);
}

bool gRubberZoneHack::Timestep(REAL time)
{
    bool ret = gZone::Timestep(time);
    SetReferenceTime();
    return ret;
}

// *******************************************************************************
// *
// *    sg_RubberZoneHurt
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @brief    when DZ_RUBBER is not 0: change target's rubber instead of just killing
//!
// *******************************************************************************
static eLadderLogWriter sg_deathRubberZoneWriter("DEATH_RUBBERZONE", true);

void sg_RubberZoneHurt( gCycle * target, gRubberZoneHack *rubberZone,REAL timestep=1)
{
    REAL rubber = target->GetRubber();
    if (rubberZone->GetRubberType() == gRubberZoneHack::TYPE_RUBBER)
    {
        REAL newRubber = rubberZone->GetRubber();
        if(sg_rubberZoneRate >= 0)
            newRubber *= timestep*sg_rubberZoneRate;

        if ( rubber + newRubber >= sg_rubberCycle )       // max rubber amount reached, kill the cycle
        {
            target->Player()->AddScore( score_rubberzone, tOutput(), "$player_lose_rubberzone" );
            target->Kill("RUBBERZONE");
            target->SetRubber( sg_rubberCycle );
            sg_deathRubberZoneWriter << target->Player()->GetUserName();
            sg_deathRubberZoneWriter.write();
        }
        else if ( rubber + rubberZone->GetRubber() > 0 )            // sg_deathZoneRubberMalus might be negative, avoid setting a negative rubber!
        {
            target->SetRubber( rubber + newRubber );
        }
        else                                                        // too low value, just set to zero
            target->SetRubber( 0 );
    }
    else if (rubberZone->GetRubberType() == gRubberZoneHack::TYPE_ADJUST)
    {
        if (rubberZone->GetRubber() >= sg_rubberCycle)
            rubberZone->SetRubber(sg_rubberCycle);

        target->SetRubber(rubberZone->GetRubber());
    }
}


// *******************************************************************************
// *    gRubberZoneHack
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gRubberZoneHack::OnEnter( gCycle * target, REAL time )
{
    REAL timestep = time-referenceTime_;
    sg_RubberZoneHurt(target,this,timestep);
}

// *******************************************************************************
// *
// *    gBaseZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner, bool delayCreation )
:gZone( grid, pos, dynamicCreation, delayCreation), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
    touchy_ = false;
    
    interactWithZone_ = true;

    if (teamowner!=NULL) team = teamowner;

    for (int i=0; i<MAXCLIENTS+1; i++) conquerer_[i]=0;

    color_.r = color_.g = color_.b = 0;
}


// *******************************************************************************
// *
// *    gBaseZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( nMessage & m )
: gZone( m ), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    lastRespawnRemindTime_ = -500;
    lastRespawnRemindWaiting_ = 0;
    touchy_ = false;
    for (int i=0; i<MAXCLIENTS+1; i++) conquerer_[i]=0;
}


// *******************************************************************************
// *
// *    ~gBaseZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBaseZoneHack::~gBaseZoneHack( void )
{
}


static REAL sg_conquestRate = .5;
static REAL sg_defendRate = .25;
static REAL sg_conquestDecayRate = .1;

static tSettingItem< REAL > sg_conquestRateConf( "FORTRESS_CONQUEST_RATE", sg_conquestRate );
static tSettingItem< REAL > sg_defendRateConf( "FORTRESS_DEFEND_RATE", sg_defendRate );
static tSettingItem< REAL > sg_conquestDecayRateConf( "FORTRESS_CONQUEST_DECAY_RATE", sg_conquestDecayRate );

// time with no enemy inside a zone before it collapses harmlessly
static REAL sg_conquestTimeout = 0;
static tSettingItem< REAL > sg_conquestTimeoutConf( "FORTRESS_CONQUEST_TIMEOUT", sg_conquestTimeout );

// kill at least than many players from the team that just got its zone conquered
static int sg_onConquestKillMin = 0;
static tSettingItem< int > sg_onConquestKillMinConfig( "FORTRESS_CONQUERED_KILL_MIN", sg_onConquestKillMin );

// and at least this ratio
static REAL sg_onConquestKillRatio = 0;
static tSettingItem< REAL > sg_onConquestKillRationConfig( "FORTRESS_CONQUERED_KILL_RATIO", sg_onConquestKillRatio );

// score you get for conquering a zone
static int sg_onConquestScore = 0;
static tSettingItem< int > sg_onConquestConquestScoreConfig( "FORTRESS_CONQUERED_SCORE", sg_onConquestScore );

// flag indicating whether the team conquering the first zone wins (good for one on one matches)
static bool sg_onConquestWin = true;
static tSettingItem< bool > sg_onConquestConquestWinConfig( "FORTRESS_CONQUERED_WIN", sg_onConquestWin );

// maximal number of base zones ownable by a team
static int sg_baseZonesPerTeam = 0;
static tSettingItem< int > sg_baseZonesPerTeamConfig( "FORTRESS_MAX_PER_TEAM", sg_baseZonesPerTeam );
// flag indicating whether base will respawn team if a team player enters it
static bool sg_baseRespawn = false;
static tSettingItem<bool> sg_baseRespawnConfig ("BASE_RESPAWN", sg_baseRespawn);

// flag indicating whether base will respawn team if an enemy player enters it
static bool sg_baseEnemyRespawn = false;
static tSettingItem<bool> sg_baseEnemyRespawnConfig ("BASE_ENEMY_RESPAWN", sg_baseEnemyRespawn);

// flag indicating whether base will kill enemy players
static bool sg_baseEnemyKill = false;
static tSettingItem<bool> sg_baseEnemyKillConfig ("BASE_ENEMY_KILL", sg_baseEnemyKill);

// number of points a player scores on returning a captured flag to their base
static int sg_scoreFlag=1;
static tSettingItem<int> sg_scoreFlagConfig("SCORE_FLAG", sg_scoreFlag);

// flag indicating whether the goal is to bring flag in player home base or enemy base to score. true=home base / false=enemy base
static bool sg_scoreFlagHomeBase=true;
static tSettingItem<bool> sg_scoreFlagHomeBaseConfig("SCORE_FLAG_HOME_BASE", sg_scoreFlagHomeBase);

// number of points a player scores on kicking the ball into the enemy goal
static int sg_scoreGoal=1;
static tSettingItem<int> sg_scoreGoalConfig("SCORE_GOAL", sg_scoreGoal);

// flag indicating whether the round ends when a goal is shot
static bool sg_goalRoundEnd = true;
static tSettingItem<bool> sg_goalRoundEndConfig("GOAL_ROUND_END", sg_goalRoundEnd);

// flag indicating whether flags need to be home to score
static bool sg_flagRequiredHome = true;
static tSettingItem<bool> sg_flagRequiredHomeConfig("FLAG_REQUIRED_HOME", sg_flagRequiredHome);

// flag indicating whether dropping the flag sends it home
static bool sg_flagDropHome = false;
static tSettingItem<bool> sg_flagDropHomeConfig("FLAG_DROP_HOME", sg_flagDropHome);

// time between respawn reminders
static int sg_baseRespawnRemindTime=30;
static tSettingItem<int> sg_baseRespawnRemindTimeConfig("BASE_RESPAWN_REMIND_TIME", sg_baseRespawnRemindTime);

//number of flags that must be home in order to capture
static int sg_minFlagsHome = 0;
static tSettingItem<int> sg_minFlagsHomeConfig( "MIN_FLAGS_HOME", sg_minFlagsHome );

// count zones belonging to the given team.
// fill in count and the zone that is farthest to the team.
void gBaseZoneHack::CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, gBaseZoneHack * & farthest )
{
    count = 0;
    farthest = NULL;

    // check whether other zones are already registered to that team
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=gameObjects.Len()-1;j>=0;j--)
    {
        gBaseZoneHack *otherZone=dynamic_cast<gBaseZoneHack *>(gameObjects(j));

        if ( otherZone && otherTeam == otherZone->Team() )
        {
            count++;
            if ( !farthest || otherZone->teamDistance_ > farthest->teamDistance_ )
                farthest = otherZone;
        }
    }
}


static int sg_onSurviveScore = 0;
static tSettingItem< int > sg_onSurviveConquestScoreConfig( "FORTRESS_HELD_SCORE", sg_onSurviveScore );

static REAL sg_collapseSpeed = .5;
static tSettingItem< REAL > sg_collapseSpeedConfig( "FORTRESS_COLLAPSE_SPEED", sg_collapseSpeed );

// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gBaseZoneHack::Timestep( REAL time )
{
    // no team?!? Get rid of this zone ASAP.
    if ( !team )
    {
        return true;
    }

    if ( currentState_ == State_Conquering )
    {
        // let zone vanish
        SetReferenceTime();

        // let it light up in agony
        if ( sg_collapseSpeed < .4 )
        {
            color_.r = color_.g = color_.b = 1;
        }

        SetExpansionSpeed( -GetRadius()*sg_collapseSpeed );
        SetRotationAcceleration( -GetRotationSpeed()*.4 );
        RequestSync();

        currentState_ = State_Conquered;
    }
    else if ( currentState_ == State_Conquered && GetRotationSpeed() < 0 )
    {
        // let zone vanish
        SetReferenceTime();
        SetRotationSpeed( 0 );
        SetRotationAcceleration( 0 );
        color_.r = color_.g = color_.b = .5;
        RequestSync();
    }

    REAL dt = time - lastTime;

    // conquest going on
    REAL conquest = sg_conquestRate * enemiesInside_ - sg_defendRate * ownersInside_ - sg_conquestDecayRate;
    conquered_ += dt * conquest;

    if ( touchy_ && enemiesInside_ > 0 )
    {
        conquered_ = 1.01;
    }

    // clamp
    if ( conquered_ < 0 )
    {
        conquered_ = 0;
        conquest = 0;
    }
    if ( conquered_ > 1.01 )
    {
        conquered_ = 1.01;
        conquest = 0;
    }

    // set speed according to conquest status
    if ( currentState_ == State_Safe )
    {
        REAL maxSpeed = 10 * ( 2 *  M_PI ) / sg_segments;
        REAL omega = .3 + conquered_ * conquered_ * maxSpeed;
        REAL omegaDot = 2 * conquered_ * conquest * maxSpeed;

        // determine the time since the last sync (exaggerate for smoother motion in local games)
        REAL timeStep = lastTime - lastSync_;
        if ( sn_GetNetState() != nSERVER )
            timeStep *= 100;

        if ( sn_GetNetState() != nCLIENT &&
            ( ( fabs( omega - GetRotationSpeed() ) + fabs( omegaDot - GetRotationAcceleration() ) ) * timeStep > .5 ) )
        {
            SetRotationSpeed( omega );
            SetRotationAcceleration( omegaDot );
            SetReferenceTime();
            RequestSync();
            lastSync_ = lastTime;
        }

        // check for enemy contact timeout
        if ( sg_conquestTimeout > 0 && lastEnemyContact_ + sg_conquestTimeout < time )
        {
            enemies_.clear();

            // if the zone would collapse without defenders, let it collapse now. A smart defender would
            // have left the zone to let it collapse anyway.
            if ( sg_conquestDecayRate < 0 )
            {
                if ( team )
                {
                    if ( sg_onSurviveScore != 0 )
                    {
                        // give player the survive score bonus right now, they deserve it
                        ZoneWasHeld();
                    }
                    else
                    {
                        sn_ConsoleOut( tOutput( "$zone_collapse_harmless", team->GetColoredName()  ) );
                    }
                }
                conquered_ = 1.0;
            }
        }

        // check whether the zone got conquered
        if ( conquered_ >= 1 )
        {
            currentState_ = State_Conquering;
            OnConquest();
        }
    }

    if ((team) &&
        (((ownersInside_ > 0) && (sg_baseRespawn)) ||
        ((enemiesInside_ > 0) && (sg_baseEnemyRespawn))))
    {
        // check for respawn
        //??? change this to divide up spawns between bases, and randomly choose among those closest
        gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());

        if (pSpawn)
        {
            for (int i = team->NumPlayers() - 1; i >= 0; --i)
            {
                ePlayerNetID *pPlayer = team->Player(i);

                eGameObject *pGameObject = pPlayer->Object();

                if ((!pGameObject) ||
                    ((pPlayer->CanRespawn() && !pGameObject->Alive()) &&
                    (pGameObject->DeathTime() < (time - 1))))
                {
                    lastRespawnRemindTime_ = time - sg_baseRespawnRemindTime - 1;

                    eCoord pos, dir;

                    pSpawn->Spawn(pos, dir);

                    gCycle *pCycle = new gCycle(grid, pos, dir, pPlayer);
                    pPlayer->ControlObject(pCycle);

                    tColoredString playerName;
                    playerName << *pPlayer << tColoredString::ColorString(1,1,1);

                    if ((ownersInside_ > 0) && (sg_baseRespawn))
                    {
                        tColoredString spawnerName;
                        spawnerName << teamPlayer_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );

                        sg_baseRespawnWriter << teamPlayer_->Player()->GetLogName() << pPlayer->GetLogName();
                        sg_baseRespawnWriter.write();
                    }
                    else
                    {
                        tColoredString spawnerName;
                        spawnerName << enemyPlayer_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                        sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );

                        sg_baseEnemyRespawnWriter << enemyPlayer_->Player()->GetLogName() << pPlayer->GetLogName();
                        sg_baseEnemyRespawnWriter.write();
                    }

                    // send a console message to the player
                    sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
                }
            }
        }
    }

    // reset counts
    enemiesInside_ = ownersInside_ = 0;

    if ((team) && (sg_baseRespawn) && (sg_baseRespawnRemindTime > 0) &&
        (time >= (lastRespawnRemindTime_ + sg_baseRespawnRemindTime)))
    {
        // find out how many players are waiting to be respawned
        int waiting = 0;
        int alive = 0;

        for (int i = team->NumPlayers() - 1; i >= 0; --i)
        {
            ePlayerNetID *pPlayer = team->Player(i);

            eGameObject *pGameObject = pPlayer->Object();

            if ((!pGameObject) ||
                (pPlayer->CanRespawn() && !pGameObject->Alive()))
            {
                waiting++;
            }
            else
            {
                alive++;
            }
        }

        if ((waiting) && (alive))
        {
            if (lastRespawnRemindWaiting_)
            {
                for (int i = team->NumPlayers() - 1; i >= 0; --i)
                {
                    ePlayerNetID *pPlayer = team->Player(i);
                    if (alive > 1)
                    {
                        sn_ConsoleOut(tOutput("$player_base_respawn_reminder", waiting), pPlayer->Owner());
                    }
                    else
                    {
                        sn_ConsoleOut(tOutput("$player_base_respawn_reminder_alone", waiting), pPlayer->Owner());
                    }
                }
            }

            if ((alive > 1) || (lastRespawnRemindWaiting_ != 1))
            {
                lastRespawnRemindTime_ = time;
            }
            else
            {
                lastRespawnRemindTime_ = time - (sg_baseRespawnRemindTime / 3);
            }
        }

        lastRespawnRemindWaiting_ = waiting;
    }

    // delegate
    bool ret = gZone::Timestep( time );

    // reward survival
    if ( team && !ret && onlySurvivor_ )
    {
        const char* message= ( eTeam::teams.Len() > 2 || sg_onConquestScore ) ? "$player_win_held_fortress" : "$player_win_conquest";
        sg_DeclareWinner( team, message );
    }

    return ret;
}


// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter sg_deathBasezoneConqueredWriter("DEATH_BASEZONE_CONQUERED", true);

void gBaseZoneHack::OnVanish( void )
{

    if (!team)
        return;

    CheckSurvivor();

    // kill the closest owners of the zone
    if ( currentState_ != State_Safe  && ( enemies_.size() > 0 || sg_defendRate < 0 ))
    {
        int kills = int( sg_onConquestKillRatio * team->NumPlayers() );
        kills = kills > sg_onConquestKillMin ? kills : sg_onConquestKillMin;

        while ( kills > 0 )
        {
            -- kills;

            ePlayerNetID * closest = NULL;
            REAL closestDistance = 0;

            // find the closest living owner
            for ( int i = team->NumPlayers()-1; i >= 0; --i )
            {
                ePlayerNetID * player = team->Player(i);
                eNetGameObject * object = player->Object();
                if ( object && object->Alive() )
                {
                    eCoord otherpos = object->Position() - pos;
                    REAL distance = otherpos.NormSquared();
                    if ( !closest || distance < closestDistance )
                    {
                        closest = player;
                        closestDistance = distance;
                    }
                }
            }

            if ( closest )
            {
                sg_deathBasezoneConqueredWriter << closest->GetUserName();
                if ( enemies_.size() > 0 || sg_defendRate < 0)
                {
                    tColoredString playerName;
                    playerName = closest->GetColoredName();
                    playerName << tColoredStringProxy(-1,-1,-1);
                    sn_ConsoleOut( tOutput("$player_kill_collapse", playerName ) );

                    gCycle *c = dynamic_cast<gCycle *>(closest->Object());
                    if(c) c->Kill("BASEZONE_CONQUERED");
                }
                else
                {
                    sg_deathBasezoneConqueredWriter << "NO_ENEMIES";
                }
                sg_deathBasezoneConqueredWriter.write();
            }
        }
    }
}


// *******************************************************************************
// *
// *    OnConquest
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter sg_basezoneConqueredWriter("BASEZONE_CONQUERED", true);
static eLadderLogWriter sg_basezoneConquererWriter("BASEZONE_CONQUERER", true);
static eLadderLogWriter sg_basezoneConquererTeamWriter("BASEZONE_CONQUERER_TEAM", true);

//condense fort zone conquered output into one line for multiple wiiners.
static bool sg_condenseConquestOutput = false;
static tSettingItem<bool> sg_condenseConquestOutputConfig("CONDENSE_CONQUEST_OUTPUT", sg_condenseConquestOutput);

void gBaseZoneHack::OnConquest( void )
{
    if ( team )
    {
        sg_basezoneConqueredWriter << ePlayerNetID::FilterName(team->Name()) << MapPosition().x << MapPosition().y;
        if ( enemies_.size() == 0 ) //no enemies inside
        {
            sg_basezoneConqueredWriter << "NO_ENEMIES";
        }
        sg_basezoneConqueredWriter.write();
    }
    float rr = GetRadius();
    rr *= rr;
    int totalInZone = 0;
    for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
    {
        ePlayerNetID *player = se_PlayerNetIDs(i);
        if(!player)
        {
            continue;
        }
        gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
        if(!cycle)
        {
            continue;
        }
        if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
        {
            totalInZone++;
        }
    }
    float percentWon = 0.0;
    if (totalInZone > 0){
        percentWon = 1.0 /totalInZone;
    }
    for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
    {
        ePlayerNetID *player = se_PlayerNetIDs(i);
        if(!player)
        {
            continue;
        }
        gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
        if(!cycle)
        {
            continue;
        }
        if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
        {
            sg_basezoneConquererWriter << player->GetUserName();
            sg_basezoneConquererWriter << percentWon;
            sg_basezoneConquererWriter.write();
        }
    }

    // calculate score. If nobody really was inside the zone any more, half it.
    int totalScore = sg_onConquestScore;
    if ( 0 == enemiesInside_ )
        totalScore /= 2;

    // eliminate dead enemies
    TeamArray enemiesAlive;
    for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
    {
        eTeam* team = *iter;
        if ( team->Alive() )
            enemiesAlive.push_back( team );
    }
    enemies_ = enemiesAlive;

    // add score for successful conquest, divided equally between the teams that are
    // inside the zone
    if ( totalScore && enemies_.size() > 0 )
    {
        tOutput win;
        if ( team )
        {
            win.SetTemplateParameter( 3, team->GetColoredName() );
            win << "$player_win_conquest_specific";
        }
        else
        {
            win << "$player_win_conquest";
        }
        int score = totalScore / enemies_.size();
        int tCount=0;
        tColoredString TeamOutputNames;
        tColoredString lastTeam;
        for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
        {
            //sg_basezoneConquererTeamWriter << ePlayerNetID::FilterName((*iter)->Name()) << score;
            //sg_basezoneConquererTeamWriter.write();

            if (sg_condenseConquestOutput){
                if((*iter)->maxPlayers == 1 && (*iter)->players.Len() == 1)
                    (*iter)->players[0]->AddScore(score,tString("\n"),tString(),false);
                else
                    (*iter)->AddScore(score);
                if(tCount==0){
                    lastTeam = (*iter)->GetColoredName();
                }
                else if(tCount==1){
                    TeamOutputNames << lastTeam;
                    lastTeam = (*iter)->GetColoredName();
                }else{
                    TeamOutputNames << tColoredString::ColorString(1,1,1) << ", " << lastTeam;
                    lastTeam = (*iter)->GetColoredName();
                }
                tCount++;
            }else{
                (*iter)->AddScore( score, win, tOutput() );
            }
        }
        if (sg_condenseConquestOutput){
			if(tCount==1){
                TeamOutputNames << tColoredString::ColorString(1,1,1) << lastTeam;
            }else{
                TeamOutputNames << tColoredString::ColorString(1,1,1) << " and " << lastTeam;
            }
            tOutput message;
            message.SetTemplateParameter(1, TeamOutputNames);
            message.SetTemplateParameter(2, score > 0 ? score : -score);
            message.SetTemplateParameter(3, team->GetColoredName());
            if(tCount==1){
                message << "$player_win_conquest2";
            }else{
                message << "$players_win_conquest";
            }
            sn_ConsoleOut(message);
            RequestSync(true);

            se_SaveToScoreFile(message);
        }
    }

    //write basezoneConquererTeam msg regardless of whether a score is given
        if(  enemies_.size() > 0)
        {
            int score = totalScore / enemies_.size();
            for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
            {
                sg_basezoneConquererTeamWriter << ePlayerNetID::FilterName((*iter)->Name()) << score;
                sg_basezoneConquererTeamWriter.write();
            }
        }
    // trigger immediate win
    if ( sg_onConquestWin && enemies_.size() > 0 )
    {
        static const char* message="$player_win_conquest";
        sg_DeclareWinner( enemies_[0], message );
    }
    CheckSurvivor();
}


// if this flag is enabled, the last team with a non-conquered zone wins the round.
static bool sg_onSurviveWin = true;
static tSettingItem< bool > sg_onSurviveWinConfig( "FORTRESS_SURVIVE_WIN", sg_onSurviveWin );

// *******************************************************************************
// *
// *    CheckSurvivor
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::CheckSurvivor( void )
{
    // test if there is only one team with non-conquered zones left
    if ( sg_onSurviveWin )
    {
        // find surviving team and test whether it is the only one
        gBaseZoneHack * survivor = 0;
        bool onlySurvivor = true;

        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        for (int i=gameObjects.Len()-1;i>=0 && onlySurvivor;i--)
        {
            gBaseZoneHack *other=dynamic_cast<gBaseZoneHack *>(gameObjects(i));

            if ( other && other->currentState_ == State_Safe && other->team )
            {
                if ( survivor && survivor->team != other->team )
                    onlySurvivor = false;
                else
                    survivor = other;
            }
        }

        // reward it later
        if ( onlySurvivor && survivor )
        {
            survivor->onlySurvivor_ = true;
        }
    }
}


// *******************************************************************************
// *
// *   ZoneWasHeld
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::ZoneWasHeld( void )
{
    // survived?
    if ( currentState_ == State_Safe && sg_onSurviveScore != 0 )
    {
        // award owning team
        if ( team && team->Alive() )
        {
            team->AddScore( sg_onSurviveScore, tOutput("$player_win_held_fortress"), tOutput("$player_lose_held_fortress") );

            currentState_ = State_Conquering;
            enemies_.clear();
        }
        else
        {
            // give a little conquering help. The round is almost over, if
            // an enemy actually made it into the zone by now, it should be his.
            touchy_ = true;
        }
    }
}


// *******************************************************************************
// *
// *   CheckTeamAssignment
// *
// *******************************************************************************
//!
//! @return true if a team is assigned, false if not
//!
// *******************************************************************************

bool gBaseZoneHack::CheckTeamAssignment()
{
    // determine the owning team: the one that has a player spawned closest
    // find the closest player
    if ( !team )
    {
        teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        gCycle * closest = NULL;
        REAL closestDistance = 0;
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

            if (other )
            {
                eTeam * otherTeam = other->Player()->CurrentTeam();
                eCoord otherpos = other->Position() - pos;
                REAL distance = otherpos.NormSquared();
                if ( !closest || distance < closestDistance )
                {
                    // check whether other zones are already registered to that team
                    gBaseZoneHack * farthest = NULL;
                    int count = 0;
                    if ( sg_baseZonesPerTeam > 0 )
                        CountZonesOfTeam( Grid(), otherTeam, count, farthest );

                    // only set team if not too many closer other zones are registered
                    if ( sg_baseZonesPerTeam == 0 || count < sg_baseZonesPerTeam || (farthest && farthest->teamDistance_ > distance ) )
                    {
                        closest = other;
                        closestDistance = distance;
                    }
                }
            }
        }

        if ( closest )
        {
            // take over team and color
            team = closest->Player()->CurrentTeam();
            color_.r = team->R()/15.0;
            color_.g = team->G()/15.0;
            color_.b = team->B()/15.0;
            teamDistance_ = closestDistance;

            RequestSync();
        }

        // if this zone does not belong to a team, return false.
        if ( !team )
        {
            return false;
        }

        // check other zones owned by the same team. Discard the one farthest away
        // if the max count is exceeded
        if ( team && sg_baseZonesPerTeam > 0 )
        {
            gBaseZoneHack * farthest = 0;
            int count = 0;
            CountZonesOfTeam( Grid(), team, count, farthest );

            // discard team of farthest zone
            // No NULL check is required here for farthest, since count is greater than 0 which implies farthest was set.
            if ( count > sg_baseZonesPerTeam )
            {
                farthest->team = NULL;
                farthest->RemoveFromGame();
            }
        }
    }
    return true;
}
bool gBaseZoneHack::CheckTeamAssignmentOnTeam()
{
    // determine the owning team: the one that has a player spawned closest
    // find the closest player
    if ( team )
    {
        teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        gCycle * closest = NULL;
        REAL closestDistance = 0;
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

            if (other && (other->Player()->CurrentTeam()==team))
            {
                eCoord otherpos = other->Position() - pos;
                REAL distance = otherpos.NormSquared();
                if ( !closest)
                {
                    closest = other;
                    closestDistance = distance;
                }
                else if (distance < closestDistance )
                {
                    // check whether other zones are already registered to that team
                    gBaseZoneHack * farthest = NULL;
                    int count = 0;
                    if ( sg_baseZonesPerTeam > 0 )
                        CountZonesOfTeam( Grid(), team, count, farthest );
                    // only set team if not too many closer other zones are registered
                    if ( sg_baseZonesPerTeam == 0 || count < sg_baseZonesPerTeam || (farthest && farthest->teamDistance_ > distance ))
                    {
                        closest = other;
                        closestDistance = distance;
                    }
                }
            }
        }
        if ( closest )
        {
            teamDistance_ = closestDistance;
            RequestSync();
        }else if (!closest){
            return false;
        }
        // check other zones owned by the same team. Discard the one farthest away
        // if the max count is exceeded
        if ( team && sg_baseZonesPerTeam > 0 )
        {
            gBaseZoneHack * farthest = 0;
            int count = 0;
            CountZonesOfTeam( Grid(), team, count, farthest );
            // discard team of farthest zone
            // No NULL check is required here for farthest, since count is greater than 0 which implies farthest was set.
            if ( count > sg_baseZonesPerTeam )
            {
                farthest->team = NULL;
                farthest->RemoveFromGame();
            }
        }
    }
    return true;
}


// *******************************************************************************
// *
// *   OnRoundBegin
// *
// *******************************************************************************
//!
//! @return shall the hole process be repeated?
//!
// *******************************************************************************

void gBaseZoneHack::OnRoundBegin( void )
{
        // if this zone does not belong to a team, discard it.
        if ( !CheckTeamAssignment() )
        {
            RemoveFromGame();
            return;
        }
}


// *******************************************************************************
// *
// *   OnRoundEnd
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::OnRoundEnd( void )
{
    // survived?
    if ( currentState_ == State_Safe )
    {
        ZoneWasHeld();
    }
}


static bool sg_flagConquestWinsRound = false;
static tSettingItem< bool > sg_flagConquestWinsRoundConfig( "FLAG_CONQUEST_WINS_ROUND", sg_flagConquestWinsRound );

// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gBaseZoneHack::OnEnter( gCycle * target, REAL time )
{
    // determine the team of the player
    tASSERT( target );
    if ( !target->Player() )
        return;
    tJUST_CONTROLLED_PTR< eTeam > otherTeam = target->Player()->CurrentTeam();
    if (!otherTeam)
        return;
    if ( currentState_ != State_Safe )
        return;

    // remember who is inside
    if ( team == otherTeam )
    {
        if ( ownersInside_ == 0 )
        {
            // store the name
            teamPlayer_ = target;
        }

        ++ ownersInside_;
    }
    else if ( team )
    {
        // check if base enemy kills are enabled
        if (sg_baseEnemyKill)
        {
            tColoredString playerName;
            playerName << *target->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_base_enemy_kill", playerName ) );

            target->deathReason_ << "BASEZONE_ENEMY_KILL " << ePlayerNetID::FilterName(team->Name());
            target->Kill();

            return;
        }

        if ( enemiesInside_ == 0 )
        {
            enemies_.clear();

            // store the name
            enemyPlayer_ = target;
        }

        ++ enemiesInside_;
        conquerer_[target->Player()->ListID()]+=time - lastTime;
        if ( std::find( enemies_.begin(), enemies_.end(), otherTeam ) == enemies_.end() )
            enemies_.push_back( otherTeam );

        lastEnemyContact_ = time;
    }
    else
    {
        conquerer_[target->Player()->ListID()]+=time - lastTime;
    }

    // check if player has a flag
    if (target->flag_)
    {
        // check if the player is on our team (or not in zone team according to setting score_flag_home_base)
        if ((team == otherTeam)==sg_scoreFlagHomeBase)
        {
            // search for another flag owned by our team
            bool allFlagsHome = true;
            int flagsHome = 0;
            int flagsTaken = 0;
            const tList<eGameObject>& gameObjects = Grid()->GameObjects();
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                gFlagZoneHack *otherFlag=dynamic_cast<gFlagZoneHack *>(gameObjects(i));

                if ((otherFlag) && (otherFlag->Team() == team))
                {
                    // check if flag is at home (starting position)
                    if (!otherFlag->IsHome())
                    {
                        allFlagsHome = false;
                        flagsTaken++;
                    }
                    else
                    {
                        flagsHome++;
                    }
                }
            }
            if (!sg_flagRequiredHome)
            {
                if (sg_minFlagsHome > (flagsHome+flagsTaken)){
                    allFlagsHome=false;
                }else if(sg_minFlagsHome <= flagsHome){
                    allFlagsHome=true;
                }
            }

            if (!allFlagsHome)
            {
                target->flag_->WarnFlagNotHome();
            }
            else
            {
                // player has scored a flag capture
                sg_flagScoreWriter << target->Player()->GetUserName() << ePlayerNetID::FilterName( otherTeam->Name() );
                sg_flagScoreWriter.write();

                tOutput lose;
                tOutput win;
                int score = sg_scoreFlag;

                win << "$player_flag_score";
                target->Player()->AddScore(score, win, lose);

                // tell the flag to go back home
                target->flag_->GoHome();
                if (sg_flagConquestWinsRound)
                {

                    sg_flagConquestRoundWinWriter << target->Player()->GetUserName() << ePlayerNetID::FilterName( otherTeam->Name() );
                    sg_flagConquestRoundWinWriter.write();

                    static const char*message="$player_win_flag";
                    sg_DeclareWinner( otherTeam, message );
                }
            }
        }
    }
}

// flag indicating whether base will respawn team if a team player's shot enters it
static bool sg_shotbaseRespawn = false;
static tSettingItem<bool> sg_shotBaseRespawnConfig ("SHOT_BASE_RESPAWN", sg_shotbaseRespawn);

// flag indicating whether base will respawn team if an enemy player's shot enters it
static bool sg_shotbaseEnemyRespawn = false;
static tSettingItem<bool> sg_shotBaseEnemyRespawnConfig ("SHOT_BASE_ENEMY_RESPAWN", sg_shotbaseEnemyRespawn);

static int sg_scoreShotBase = 0;
static tSettingItem<int> sg_scoreShotBaseConf("SCORE_SHOT_BASE", sg_scoreShotBase);


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the zone that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gBaseZoneHack::OnEnter( gZone * target, REAL time )
{
                                 //it already hit the zone.
    if(currentState_ == State_Conquering || currentState_ == State_Conquered) return;
    gBallZoneHack *ball = dynamic_cast<gBallZoneHack *>(target);

    // check ball team mode
    bool toScore;
    if (!team || !target ) toScore = true;
    else if (sg_ballTeamMode) toScore = target->Team() == team ? 1 : 0;
    else toScore = target->Team() == team ? 0 : 1;

    if (ball && toScore)
    {
        // the ball entered the goal, figure out who shot it
        ePlayerNetID *lastPlayer = ball->GetLastPlayer();

        // and respawn players ...
        if ( sg_baseRespawn || sg_baseEnemyRespawn )
        {
            // check for respawn
            //??? change this to divide up spawns between bases, and randomly choose among those closest
            gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());

            if (pSpawn)
            {
                for (int i = team->NumPlayers() - 1; i >= 0; --i)
                {
                    ePlayerNetID *pPlayer = team->Player(i);

                    eGameObject *pGameObject = pPlayer->Object();

                    if ((!pGameObject) ||
                        (((pPlayer->CanRespawn() && !pGameObject->Alive())) &&
                        (pGameObject->DeathTime() < (time - 1))))
                    {
                        lastRespawnRemindTime_ = time - sg_baseRespawnRemindTime - 1;

                        eCoord pos, dir;

                        pSpawn->Spawn(pos, dir);

                        gCycle *pCycle = new gCycle(grid, pos, dir, pPlayer);
                        pPlayer->ControlObject(pCycle);

                        tColoredString playerName;
                        playerName << *pPlayer << tColoredString::ColorString(1,1,1);

                        if ((ownersInside_ > 0) && (sg_baseRespawn))
                        {
                            tColoredString spawnerName;
                            spawnerName << teamPlayer_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                            sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );

                            sg_baseRespawnWriter << teamPlayer_->Player()->GetLogName() << pPlayer->GetLogName();
                            sg_baseRespawnWriter.write();
                        }
                        else
                        {
                            tColoredString spawnerName;
                            spawnerName << enemyPlayer_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                            sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );

                            sg_baseEnemyRespawnWriter << enemyPlayer_->Player()->GetLogName() << pPlayer->GetLogName();
                            sg_baseEnemyRespawnWriter.write();
                        }

                        // send a console message to the player
                        sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
                    }
                }
            }
        }

        if (!lastPlayer)
        {
            return;
        }
        else
        {
            eTeam *lastTeam = lastPlayer->CurrentTeam();

            if (lastTeam == team)
            {
                // own team hit it in
                tColoredString playerName;
                playerName << *lastPlayer << tColoredString::ColorString(1,1,1);
                sn_ConsoleOut( tOutput( "$player_score_own_goal", playerName ) );

                // search through the teams and add a point to each
                for (int i = eTeam::teams.Len() - 1; i >= 0; i--)
                {
                    if (eTeam::teams(i) != team)
                    {
                        eTeam::teams(i)->AddScore(1);
                        lastTeam = eTeam::teams(i);
                    }
                }
            }
            else
            {
                // another team hit it in, give that player the point
                tOutput lose;
                tOutput win;
                int score = sg_scoreGoal;

                win << "$player_score_goal";
                lastPlayer->AddScore(score, win, lose);
            }

            // check if the round should end or we should respawn the ball
            if (sg_goalRoundEnd)
            {
                //static const char* message="$player_win_instant";
                //sg_DeclareWinner( lastTeam, message );

                //// destroy the ball
                //ball->Destroy();
                OnConquest();
                currentState_ = State_Conquering;
            }
            else
            {
                // don't end the round, just respawn the ball at the original location
                ball->GoHome();
            }
        }
    }

    gDeathZoneHack *deathZone = dynamic_cast<gDeathZoneHack *>(target);
    if (deathZone)
    {
        if (deathZone->GetType() == gDeathZoneHack::TYPE_SHOT)
        {
            ePlayerNetID *shotOwner = deathZone->GetOwner();
            if (shotOwner && (((shotOwner->CurrentTeam() == team) && sg_shotbaseRespawn) || ((shotOwner->CurrentTeam() != team) && sg_shotbaseEnemyRespawn)))
            {
                gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());

                if (pSpawn)
                {
                    for (int i = team->NumPlayers() - 1; i >= 0; --i)
                    {
                        ePlayerNetID *pPlayer = team->Player(i);

                        eGameObject *pGameObject = pPlayer->Object();

                        if ((!pGameObject) ||
                            ((pPlayer->CanRespawn() && !pGameObject->Alive()) &&
                            (pGameObject->DeathTime() < (time - 1))))
                        {
                            lastRespawnRemindTime_ = time - sg_baseRespawnRemindTime - 1;

                            eCoord pos, dir;

                            pSpawn->Spawn(pos, dir);

                            gCycle *pCycle = new gCycle(grid, pos, dir, pPlayer);
                            pPlayer->ControlObject(pCycle);

                            tColoredString playerName;
                            playerName << *pPlayer << tColoredString::ColorString(1,1,1);

                            if (shotOwner->CurrentTeam() == team)
                            {
                                tColoredString spawnerName;
                                spawnerName << shotOwner->GetColoredName() << tColoredString::ColorString(1,1,1);
                                sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );
                            }
                            else
                            {
                                tColoredString spawnerName;
                                spawnerName << shotOwner->GetColoredName() << tColoredString::ColorString(1,1,1);
                                sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );
                            }
                        }
                    }
                }
            }
            tOutput win;
            win.SetTemplateParameter(1, shotOwner->GetName());
            win.SetTemplateParameter(3, team->Name());
            win << "$player_shot_base";
            shotOwner->AddScore(sg_scoreShotBase, win, "");
        }
    }
}

// *******************************************************************************
// *
// *    gSumoZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gSumoZoneHack::gSumoZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation), currentState_( State_Unspawned )
{
    if (dynamicCreation){
        SetStateParsing();
    }
}


// *******************************************************************************
// *
// *    gSumoZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gSumoZoneHack::gSumoZoneHack( nMessage & m )
: gZone( m )
{
}

void gSumoZoneHack::OnVanish( void )
{
    grid->RemoveGameObjectInteresting(this);
}
// *******************************************************************************
// *
// *    ~gSumoZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gSumoZoneHack::~gSumoZoneHack( void )
{
}

bool gSumoZoneHack::Timestep( REAL time )
{
    if (currentState_ == State_Unspawned){
        currentState_ = State_Spawned;
        const REAL zoneSize = GetRadius();
        const REAL zoneGrowth = GetExpansionSpeed();
        eCoord zoneDir = GetVelocity();
        tString zoneNameStr = GetName();
        std::vector<eCoord> route = route_;
        bool zoneInteractive = wallInteract_;
        REAL targetRadius=targetRadius_;
        eCoord zonePos = GetPosition();
        for(int i=eTeam::teams.Len()-1;i>=0;i--){
            gRealColor zoneColor;
            bool setColorFlag = false;
            eTeam *zoneTeam=(eTeam::teams(i));
            gZone *Zone = NULL;
            gBaseZoneHack *fZone = tNEW( gBaseZoneHack( grid, zonePos, true, zoneTeam ) );
            // if this zone does not belong to a team, discard it.
            bool teamAssign=fZone->CheckTeamAssignmentOnTeam();
            if ( !teamAssign )
            {
                grid->RemoveGameObjectInteresting(fZone);
            }
            else if (teamAssign)
            {
                Zone = fZone;
                if (Zone->Team()!=NULL)
                {
                    zoneColor.r = Zone->Team()->R()/15.0;
                    zoneColor.g = Zone->Team()->G()/15.0;
                    zoneColor.b = Zone->Team()->B()/15.0;
                    setColorFlag = true;

                    CreateZone(tString("fortress"), Zone, zoneSize, zoneGrowth, zoneDir, setColorFlag, zoneColor, zoneInteractive, targetRadius, route, zoneNameStr);
                }
            }
        }
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius()*.5 );
        SetRadius(0);
        RequestSync();
    }
    bool returnStatus = gZone::Timestep( time );
    return (returnStatus);
}
// *******************************************************************************
// *
// *    tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::tFunction( void )
: offset_(0), slope_(0)
{
}


// *******************************************************************************
// *
// *    ~tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::~tFunction( void )
{
}


// *******************************************************************************
// *
// *    Evaluate
// *
// *******************************************************************************
//!
//!     @param  argument
//!     @return
//!
// *******************************************************************************

REAL tFunction::Evaluate( REAL argument ) const
{
    return offset_ + slope_ * argument;
}


// *******************************************************************************
// *
// *    operator <<
// *
// *******************************************************************************
//!
//!     @param  m   message to write to
//!     @param  f   function to write
//!     @return     reference to message for chaining
//!
// *******************************************************************************

nMessage & operator << ( nMessage & m, tFunction const & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID = 1;
    m.Write( ID );

    // write values
    m << f.GetOffset();
    m << f.GetSlope();

    return m;
}


// *******************************************************************************
// *
// *    operator >>
// *
// *******************************************************************************
//!
//!     @param  m   message to read from
//!     @param  f   function to read to
//!     @return     reference to message for chaining
//!
// *******************************************************************************

nMessage & operator >> ( nMessage & m, tFunction & f )
{
    // write ID for compatibility with future extensions
    unsigned short ID;
    m.Read(ID);
    tASSERT( ID == 1 );

    // read values
    REAL slope, offset;
    m >> offset >> slope;

    // store values
    f.SetOffset( offset ).SetSlope( slope );

    return m;
}


// *******************************************************************************
// *
// *    GetPosition
// *
// *******************************************************************************
//!
//!     @return     the current position
//!
// *******************************************************************************

eCoord gZone::GetPosition( void ) const
{
    eCoord ret;
    GetPosition( ret );
    return ret;
}


// *******************************************************************************
// *
// *    GetPosition
// *
// *******************************************************************************
//!
//!     @param  position    the current position to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetPosition( eCoord & position ) const
{
    position.x = EvaluateFunctionNow( posx_ );
    position.y = EvaluateFunctionNow( posy_ );
    return *this;
}


// *******************************************************************************
// *
// *    SetPosition
// *
// *******************************************************************************
//!
//!     @param  position    the current position to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetPosition( eCoord const & position )
{
    SetFunctionNow( posx_, position.x );
    SetFunctionNow( posy_, position.y );
    return *this;
}


// *******************************************************************************
// *
// *    GetVelocity
// *
// *******************************************************************************
//!
//!     @return     the current velocity
//!
// *******************************************************************************

eCoord gZone::GetVelocity( void ) const
{
    eCoord ret;
    GetVelocity( ret );

    return ret;
}


// *******************************************************************************
// *
// *    GetVelocity
// *
// *******************************************************************************
//!
//!     @param  velocity    the current velocity to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetVelocity( eCoord & velocity ) const
{
    velocity.x = posx_.GetSlope();
    velocity.y = posy_.GetSlope();

    return *this;
}


// *******************************************************************************
// *
// *    SetVelocity
// *
// *******************************************************************************
//!
//!     @param  velocity    the current velocity to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetVelocity( eCoord const & velocity )
{
    // backup position
    eCoord pos;
    GetPosition( pos );

    posx_.SetSlope( velocity.x );
    posy_.SetSlope( velocity.y );

    // restore position
    SetPosition( pos );

    return *this;
}


// *******************************************************************************
// *
// *    GetRadius
// *
// *******************************************************************************
//!
//!     @return     the current radius
//!
// *******************************************************************************

REAL gZone::GetRadius( void ) const
{
    REAL ret = EvaluateFunctionNow( this->radius_ );
    ret = ret > 0 ? ret : 0;

    return ret;
}


// *******************************************************************************
// *
// *    GetRadius
// *
// *******************************************************************************
//!
//!     @param  radius  the current radius to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRadius( REAL & radius ) const
{
    radius = GetRadius();

    return *this;
}


// *******************************************************************************
// *
// *    SetRadius
// *
// *******************************************************************************
//!
//!     @param  radius  the current radius to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRadius( REAL radius )
{
    SetFunctionNow( this->radius_, radius );

    return *this;
}


// *******************************************************************************
// *
// *    SetRadiusSmoothly
// *
// *******************************************************************************
//!
//!             @param  radius  the current radius to set
//!             @param  expansionSpeed  the current expansion speed to set
//!             @return         A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRadiusSmoothly( REAL radius, REAL expansionSpeed )
{
    expectedRadius_ = radius;
                                 // save expansion speed if not already resizing ...
    if (!resizeRequested_) previousExpansionSpeed_ = GetExpansionSpeed();
    resizeRequested_ = true;
    expansionSpeed = (expansionSpeed==0?radius*0.5:(expansionSpeed<0?-expansionSpeed:expansionSpeed));
    SetExpansionSpeed( (GetRadius()>radius?-expansionSpeed:expansionSpeed ));

    return *this;
}


// *******************************************************************************
// *
// *    GetExpansionSpeed
// *
// *******************************************************************************
//!
//!     @return     the current expansion speed
//!
// *******************************************************************************

REAL gZone::GetExpansionSpeed( void ) const
{
    return this->radius_.GetSlope();
}


// *******************************************************************************
// *
// *    GetExpansionSpeed
// *
// *******************************************************************************
//!
//!     @param  expansionSpeed  the current expansion speed to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetExpansionSpeed( REAL & expansionSpeed ) const
{
    expansionSpeed = this->radius_.GetSlope();

    return *this;
}


// *******************************************************************************
// *
// *    SetExpansionSpeed
// *
// *******************************************************************************
//!
//!     @param  expansionSpeed  the current expansion speed to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetExpansionSpeed( REAL expansionSpeed )
{
    REAL r = EvaluateFunctionNow( this->radius_ );
    this->radius_.SetSlope( expansionSpeed );
    SetRadius( r );

    return *this;
}


// *******************************************************************************
// *
// *    SetReferenceTime
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gZone::SetReferenceTime( void )
{
    // set offsets to current values
    this->posx_.SetOffset( EvaluateFunctionNow( this->posx_ ) );
    this->posy_.SetOffset( EvaluateFunctionNow( this->posy_ ) );
    this->radius_.SetOffset( EvaluateFunctionNow( this->radius_ ) );
    this->rotationSpeed_.SetOffset( EvaluateFunctionNow( this->rotationSpeed_ ) );

    // reset time
    this->referenceTime_ = lastTime;
}


// *******************************************************************************
// *
// *    GetRotationSpeed
// *
// *******************************************************************************
//!
//!     @return     The current rotation speed
//!
// *******************************************************************************

REAL gZone::GetRotationSpeed( void ) const
{
    return EvaluateFunctionNow( rotationSpeed_ );
}


// *******************************************************************************
// *
// *    GetRotationSpeed
// *
// *******************************************************************************
//!
//!     @param  rotationSpeed   The current rotation speed to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationSpeed( REAL & rotationSpeed ) const
{
    rotationSpeed = this->GetRotationSpeed();
    return *this;
}


// *******************************************************************************
// *
// *    SetRotationSpeed
// *
// *******************************************************************************
//!
//!     @param  rotationSpeed   The current rotation speed to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationSpeed( REAL rotationSpeed )
{
    SetFunctionNow( this->rotationSpeed_, rotationSpeed );
    return *this;
}


// *******************************************************************************
// *
// *    GetRotationAcceleration
// *
// *******************************************************************************
//!
//!     @return     the current acceleration of the rotation
//!
// *******************************************************************************

REAL gZone::GetRotationAcceleration( void ) const
{
    return this->rotationSpeed_.GetSlope();
}


// *******************************************************************************
// *
// *    GetRotationAcceleration
// *
// *******************************************************************************
//!
//!     @param  rotationAcceleration    the current acceleration of the rotation to fill
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationAcceleration( REAL & rotationAcceleration ) const
{
    rotationAcceleration = this->GetRotationAcceleration();
    return *this;
}


// *******************************************************************************
// *
// *    SetRotationAcceleration
// *
// *******************************************************************************
//!
//!     @param  rotationAcceleration    the current acceleration of the rotation to set
//!     @return     A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationAcceleration( REAL rotationAcceleration )
{
    REAL omega = this->GetRotationSpeed();
    this->rotationSpeed_.SetSlope( rotationAcceleration );
    SetRotationSpeed( omega );

    return *this;
}


// *******************************************************************************
// *
// *    gBallZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gBallZoneHack::gBallZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner, bool delayCreation )
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 1.0f;
    color_.b = 1.0f;

    interactWithZone_ = true;

    wallInteract_ = true;
    wallBouncesLeft_ = -1;
    lastPlayer_ = NULL;
    originalPosition_ = pos;
    originalVelocity_ = eCoord(0,0);
    init_ = false;
    lastImpactTime_ = 0;

    if (teamowner!=NULL) team = teamowner;

    //NOTE: Ball needs to be "interesting" not only for itself but so basezones can detect it
    grid->AddGameObjectInteresting(this);
}


// *******************************************************************************
// *
// *    gBallZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gBallZoneHack::gBallZoneHack( nMessage & m )
: gZone( m )
{
    lastPlayer_ = NULL;
    init_ = false;
    originalPosition_ = pos;
    originalVelocity_ = eCoord(0,0);
}


// *******************************************************************************
// *
// *    ~gBallZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBallZoneHack::~gBallZoneHack( void )
{
}


static eLadderLogWriter sg_ballVanishWriter("BALL_VANISH", false);

void gBallZoneHack::OnVanish( void )
{
    grid->RemoveGameObjectInteresting(this);
    sg_ballVanishWriter << this->GetID() << name_ << MapPosition().x << MapPosition().y;
    sg_ballVanishWriter.write();
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!             @param  time    the current time
//!
// *******************************************************************************

bool gBallZoneHack::Timestep( REAL time )
{
    // initialize the zone info, this happens only once, but has to happen after construction
    if (!init_)
    {
        init_ = true;
        originalVelocity_ = GetVelocity();
    }

    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************
extern eLadderLogWriter sg_deathFragWriter;
extern eLadderLogWriter sg_deathSuicideWriter;
extern eLadderLogWriter sg_deathTeamkillWriter;

void gBallZoneHack::OnEnter( gCycle * target, REAL time )
{
    if ((!target) || (!target->Player()))
    {
        return;
    }

    REAL dt = time - referenceTime_;
    eCoord dest(posx_(dt), posy_(dt));
    s_zoneWallInteractionFound = false;
    s_zoneWallInteractionCoord = dest;
    s_zoneWallInteractionRadius = GetRadius();
    grid->ProcessWallsInRange(&S_ZoneWallIntersect,
        s_zoneWallInteractionCoord,
        s_zoneWallInteractionRadius,
        CurrentFace());

    // keep some data to allow to bounce even for killing balls ...
    eCoord p2 = target->Position();
    eCoord v2 = target->Direction()*target->Speed();

    // check killer balls ... ;)
    ePlayerNetID * prey = target->Player();
    if ((sg_ballKiller) && (team) && !(prey->CurrentTeam() == team))
    {
        target->deathReason_ << "BALL_ZONE" << ePlayerNetID::FilterName(team->Name());
        target->Kill();
        // scoring ...
        if ( lastPlayer_ )
        {
            tOutput lose;
            tOutput win;
            ePlayerNetID * hunter = lastPlayer_;

            if ( hunter->CurrentTeam() != prey->CurrentTeam() )
            {
                tColoredString preyName;
                preyName << *prey;
                preyName << tColoredString::ColorString(1,1,1);
                if (prey->CurrentTeam() != hunter->CurrentTeam())
                {
                    sg_deathFragWriter << prey->GetUserName() << nMachine::GetMachine(prey->Owner()).GetIP()
                        << hunter->GetUserName() << nMachine::GetMachine(hunter->Owner()).GetIP();
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
                else             // this should never happens as the killed player is not kept as lastplayer but we can change it easily ;)
                {
                    sg_deathTeamkillWriter << prey->GetUserName() << nMachine::GetMachine(prey->Owner()).GetIP()
                        << hunter->GetUserName() << nMachine::GetMachine(hunter->Owner()).GetIP();
                    sg_deathTeamkillWriter.write();

                    tColoredString hunterName;
                    hunterName << *hunter << tColoredString::ColorString(1,1,1);
                    sn_ConsoleOut( tOutput( "$player_teamkill", hunterName, preyName ) );
                }
            }
        }
        else
            target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_suicide");
    }
    else
    {
        // keep the last cycle to enter even if we're discarding this hit
        lastPlayer_ = target->Player();
    }

    //Only process the cycle kick if there is no wall inside the zone
    if (s_zoneWallInteractionFound)
    {
        if(sg_ballRimStop)
        {
            this->SetVelocity(eCoord(0,0));
            RequestSync();
        }
        return;
    }

    REAL r1 = this->GetRadius();
    REAL r2 = target->Player()->ping/.2;                             // the cycle "radius". accomidates for higher ping players
    REAL R = r1 + r2;
    eCoord p1 = this->Position();
    eCoord dp = p2 - p1;
    REAL c = dp.NormSquared() - R * R;
    // first find the real time and position of the impact ...
    eCoord v1 = this->GetVelocity();
    eCoord dv = v2 - v1;
    REAL a = dv.NormSquared();
    REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
    REAL delta = b*b-4*a*c;
    if (delta<0) return;         // no t. it can't be, an impact just occured ...
    delta = sqrt(delta);
    REAL t1 = (-b-delta)/(2*a);
    REAL t2 = (-b+delta)/(2*a);
    REAL t = 0;
    if ((t1>0) && (t2>0)) return;// can't be, again ...
    if (t1>0) t = t2;
    else if (t2>0) t = t1;
    else if (t1>t2) t = t1;
    else t = t2;

    // if a wall impact happens too close, just skip cycle bouncing for now ...
    if (t < lastImpactTime_ - time) return;

    // now that we have the time, get the positions ...
    eCoord p1c = p1+v1*t;
    eCoord p2c = p2+v2*t;
    // now compute the impact : new velocities and new correct positions ...
    eCoord base = (p2c-p1c);
    base.Normalize();
    eCoord new_v1 = base*-target->Speed();
    eCoord new_p1 = p1c + new_v1*(-t+0.01);
    new_v1 = new_v1*(1+sg_ballCycleBoost*target->GetAcceleration()/100);

    s_zoneWallInteractionFound = false;
    s_zoneWallInteractionCoord = new_p1 + new_v1 * dt;
    s_zoneWallInteractionRadius = GetRadius();
    grid->ProcessWallsInRange(&S_ZoneWallIntersect,
        s_zoneWallInteractionCoord,
        s_zoneWallInteractionRadius,
        CurrentFace());

    if (s_zoneWallInteractionFound)
    {
        if(sg_ballRimStop)
        {
            this->SetVelocity(eCoord(0,0));
            RequestSync();
        }
        return;
    }

    SetReferenceTime();
    this->SetPosition(new_p1);
    this->SetVelocity(new_v1);
    RequestSync();
}

void gBallZoneHack::OnEnter( gZone * target, REAL time )
{
    if(sg_ballsInteract)
    {
        gBallZoneHack *ball = dynamic_cast<gBallZoneHack *>(target);
        
        if(ball)
        {
            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCoord = GetPosition();
            s_zoneWallInteractionRadius = GetRadius();
            grid->ProcessWallsInRange(&S_ZoneWallIntersect,
                s_zoneWallInteractionCoord,
                s_zoneWallInteractionRadius+.5,
                CurrentFace());
            
            if (!s_zoneWallInteractionFound)
            {
                REAL r1 = this->GetRadius();
                REAL r2 = ball->GetRadius();
                REAL R = r1 + r2;
                eCoord p1 = this->Position();
                eCoord p2 = ball->Position();
                eCoord dp = p2 - p1;
                REAL c = dp.NormSquared() - R * R;

                if (c < 0)   // the 2 balls just hit each other ...
                {
                    // first find the real time and position of the impact ...
                    eCoord v1 = this->GetVelocity();
                    eCoord v2 = ball->GetVelocity();
                    eCoord dv = v2 - v1;
                    REAL a = dv.NormSquared();
                    REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
                    REAL delta = b*b-4*a*c;
                            // no t. it can't be, an impact just occured ...
                    if (delta >= 0) // delta is positive, it means we have 2 different solutions
                    {        // the t we are looking for is negative and as close as possible to 0 = it just happens ;)
                        delta = sqrt(delta);
                        REAL t1 = (-b-delta)/(2*a);
                        REAL t2 = (-b+delta)/(2*a);
                        REAL t = 0;
                            // can't be, again ...
                        if ((t1>0) && (t2>0)) return;
                        if (t1>0) t = t2;
                        else if (t2>0) t = t1;
                        else if (t1>t2) t = t1;
                        else t = t2;

                        // if a wall impact happens too close, just skip balls interaction for now ...
                        if (t <= lastImpactTime_ - time)
                        {
                            // now that we have the time, get the positions ...
                            eCoord p1c = p1+v1*t;
                            eCoord p2c = p2+v2*t;
                            //eCoord pi = p1c + (p2c-p1c)*(R/this->Radius());
                            // now compute the impact : new velocities and new correct positions ...
                            eCoord base = (p2c-p1c);
                            base.Normalize();
                                // the weight of the zones are set according to the area
                            REAL m1 = r1*r1;
                            REAL m2 = r2*r2;
                            REAL M = m1+m2;
                            eCoord new_v1 = v1 + base*((2*m2/M)*(eCoord::F(dv,base)));
                            eCoord new_v2 = v2 - base*((2*m1/M)*(eCoord::F(dv,base)));
                            eCoord new_p1 = p1c + new_v1*(-t+0.01);
                            eCoord new_p2 = p2c + new_v2*(-t+0.01);
                            this->SetReferenceTime();
                            this->SetPosition(new_p1);
                            this->SetVelocity(new_v1);
                            this->RequestSync();
                            ball->SetReferenceTime();
                            ball->SetPosition(new_p2);
                            ball->SetVelocity(new_v2);
                            ball->RequestSync();
                        }
                    }
                }
            }
            
        }
    }
}


// *******************************************************************************
// *
// *    RemovePlayer
// *
// *******************************************************************************

void gBallZoneHack::RemovePlayer( ePlayerNetID *player )
{
    if (&*lastPlayer_ == player)
    {
        lastPlayer_ = NULL;
    }
}


// *******************************************************************************
// *
// *    GetLastPlayer
// *
// *******************************************************************************

ePlayerNetID * gBallZoneHack::GetLastPlayer()
{
    return lastPlayer_;
}


// *******************************************************************************
// *
// *    GoHome
// *
// *******************************************************************************

void gBallZoneHack::GoHome()
{
    // remove the last cycle
    lastPlayer_ = NULL;

    // put the ball at home
    SetReferenceTime();
    SetPosition(originalPosition_);
    SetVelocity(originalVelocity_);
    RequestSync();
}


// *******************************************************************************
// *
// *    gFlagZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gFlagZoneHack::gFlagZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam * teamowner, bool delayCreation  )
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 1.0f;
    color_.b = 1.0f;

    init_ = false;
    originalPosition_ = pos;
    homePosition_ = pos;
    owner_ = NULL;
    ownerTime_ = 0;
    ownerWarnedNotHome_ = false;
    chatBlinkUpdateTime_ = 0;
    blinkUpdateTime_ = 0;
    blinkTrackUpdateTime_ = 0;
    ownerDropped_ = NULL;
    ownerDroppedTime_ = 0;
    lastHoldScoreTime_ = -1;
    positionUpdatePending_ = false;

    passingTheFlag_ = false;
    passingOwner_ = NULL;
    passerOwnerFree_ = false;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    if (teamowner!=NULL) team = teamowner;
}


// *******************************************************************************
// *
// *    gFlagZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gFlagZoneHack::gFlagZoneHack( nMessage & m )
: gZone( m )
{
    init_ = false;
    originalPosition_ = pos;
    homePosition_ = pos;
    owner_ = NULL;
    ownerTime_ = 0;
    ownerWarnedNotHome_ = false;
    chatBlinkUpdateTime_ = 0;
    blinkUpdateTime_ = 0;
    blinkTrackUpdateTime_ = 0;
    ownerDropped_ = NULL;
    ownerDroppedTime_ = 0;
    lastHoldScoreTime_ = -1;
    positionUpdatePending_ = false;

    passingTheFlag_ = false;
    passingOwner_ = NULL;
    passerOwnerFree_ = false;
}


// *******************************************************************************
// *
// *    ~gFlagZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gFlagZoneHack::~gFlagZoneHack( void )
{
    // remove the owner
    RemoveOwner();
}


// if positive, blinks chat box of the player with the flag at the time in this setting
static float sg_flagChatBlinkTime = -1;
static tSettingItem<float> sg_flagChatBlinkTimeConfig( "FLAG_CHAT_BLINK_TIME", sg_flagChatBlinkTime );

static float sg_flagBlinkTime = -1;
static tSettingItem<float> sg_flagBlinkTimeConfig( "FLAG_BLINK_TIME", sg_flagBlinkTime );

static float sg_flagBlinkStart = 0.1;
static tSettingItem<float> sg_flagBlinkStartConfig( "FLAG_BLINK_START", sg_flagBlinkStart );

static float sg_flagBlinkEnd = 1.0;
static tSettingItem<float> sg_flagBlinkEndConfig( "FLAG_BLINK_END", sg_flagBlinkEnd );

static float sg_flagBlinkOnTime = -1;
static tSettingItem<float> sg_flagBlinkOnTimeConfig( "FLAG_BLINK_ON_TIME", sg_flagBlinkOnTime );

static float sg_flagBlinkEstimatePosition = 0.5;
static tSettingItem<float> sg_flagBlinkEstimatePositionConfig( "FLAG_BLINK_ESTIMATE_POSITION", sg_flagBlinkEstimatePosition );

static float sg_flagBlinkTrackTime = -1;
static tSettingItem<float> sg_flagBlinkTrackTimeConfig( "FLAG_BLINK_TRACK_TIME", sg_flagBlinkTrackTime );

static float sg_flagHoldTime = -1;
static tSettingItem<float> sg_flagHoldTimeConfig( "FLAG_HOLD_TIME", sg_flagHoldTime );

float sg_flagDropTime = 3;
static tSettingItem<float> sg_flagDropTimeConfig( "FLAG_DROP_TIME", sg_flagDropTime );

static bool sg_flagTeam = true;
static tSettingItem<bool> sg_flagTeamConfig( "FLAG_TEAM", sg_flagTeam );

static float sg_flagHoldScoreTime = -1;
static tSettingItem<float> sg_flagHoldScoreTimeConfig( "FLAG_HOLD_SCORE_TIME", sg_flagHoldScoreTime );

static int sg_flagHoldScore = 1;
static tSettingItem<int> sg_flagHoldScoreConfig( "FLAG_HOLD_SCORE", sg_flagHoldScore );

static float sg_flagHomeRandomnessX = -1;
static tSettingItem<float> sg_flagHomeRandomnessXConfig( "FLAG_HOME_RANDOMNESS_X", sg_flagHomeRandomnessX );

static float sg_flagHomeRandomnessY = -1;
static tSettingItem<float> sg_flagHomeRandomnessYConfig( "FLAG_HOME_RANDOMNESS_Y", sg_flagHomeRandomnessY );

static float sg_flagColorR = -1;
static tSettingItem<float> sg_flagColorRConfig( "FLAG_COLOR_R", sg_flagColorR );

static float sg_flagColorG = -1;
static tSettingItem<float> sg_flagColorGConfig( "FLAG_COLOR_G", sg_flagColorG );

static float sg_flagColorB = -1;
static tSettingItem<float> sg_flagColorBConfig( "FLAG_COLOR_B", sg_flagColorB );

REAL sg_flagPassSpeed = 30;
static tSettingItem<REAL> sg_flagPassSpeedConf("FLAG_PASS_SPEED", sg_flagPassSpeed);

// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gFlagZoneHack::Timestep( REAL time )
{
    // initialize the zone info, this happens only once, but has to happen after construction
    if (!init_)
    {
        init_ = true;

        // save the original radius, can't do this at construction
        originalRadius_ = GetRadius();

        if ((sg_flagColorR >= 0) &&
            (sg_flagColorG >= 0) &&
            (sg_flagColorB >= 0))
        {
            color_.r = sg_flagColorR;
            color_.g = sg_flagColorG;
            color_.b = sg_flagColorB;

            if (color_.r > 1.0)
            {
                color_.r = 1.0;
            }
            if (color_.g > 1.0)
            {
                color_.g = 1.0;
            }
            if (color_.b > 1.0)
            {
                color_.b = 1.0;
            }

            RequestSync();
        }

        // if this is a team based flag, find the team
        //??? PIG - FIX to make sure every player has a flag?
        if (sg_flagTeam)
        {
            const tList<eGameObject>& gameObjects = Grid()->GameObjects();
            gCycle * closest = NULL;
            REAL closestDistance = 0;
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

                if (other )
                {
                    // eTeam * otherTeam = other->Player()->CurrentTeam();
                    eCoord otherpos = other->Position() - pos;
                    REAL distance = otherpos.NormSquared();
                    if ( !closest || distance < closestDistance )
                    {
                        closest = other;
                        closestDistance = distance;
                    }
                }
            }

            if ( closest )
            {
                // take over team and color
                team = closest->Player()->CurrentTeam();
                color_.r = team->R()/15.0;
                color_.g = team->G()/15.0;
                color_.b = team->B()/15.0;

                color_.r += (1.0 - color_.r) / 1.8;
                color_.g += (1.0 - color_.g) / 1.8;
                color_.b += (1.0 - color_.b) / 1.8;

                RequestSync();
            }

            // if this zone does not belong to a team, discard it.
            if ( !team )
            {
                return true;
            }
        }
    }

    // check if the flag is owned
    if (owner_)
    {
        ePlayerNetID *player = owner_->Player();

        if ((player) &&
            (sg_flagHoldTime > 0) &&
            (time >= (ownerTime_ + sg_flagHoldTime)))
        {
            // go home
            GoHome();

            sg_flagTimeoutWriter << player->GetUserName();
            if(team){
                sg_flagTimeoutWriter << ePlayerNetID::FilterName( team->Name() );
            }
            sg_flagTimeoutWriter.write();

            tColoredString playerName;
            playerName << *player << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_timeout", playerName ) );
        }
    }

    // check if the flag is owned
    if (owner_)
    {
        ePlayerNetID *player = owner_->Player();

        if (player)
        {
            if ((sg_flagHoldScoreTime > 0) &&
                (sg_flagHoldScore) &&
                (time >= (lastHoldScoreTime_ + sg_flagHoldScoreTime)))
            {
                lastHoldScoreTime_ = time;

                sg_flagHeldWriter << player->GetUserName();
                sg_flagHeldWriter.write();

                tOutput win;
                tOutput lose;

                win << "$player_flag_hold_score_win";
                lose << "$player_flag_hold_score_lose";

                player->AddScore(sg_flagHoldScore, win, lose);
            }
        }

        if (player)
        {
            // check if flag chat blinking is enabled
            if (sg_flagChatBlinkTime > 0)
            {
                if (player->flagOverrideChat)
                {
                    if (time > (chatBlinkUpdateTime_ + sg_flagChatBlinkTime))
                    {
                        chatBlinkUpdateTime_ = time;
                        player->flagChatState = !player->flagChatState;
                        player->RequestSync();
                    }
                }
                else
                {
                    // start the override
                    chatBlinkUpdateTime_ = time;
                    player->flagOverrideChat = true;
                    player->flagChatState = false;
                    player->RequestSync();
                }
            }
        }

        if (sg_flagBlinkTime > 0)
        {
            REAL startRadiusPercent = sg_flagBlinkStart;
            if (startRadiusPercent < 0)
            {
                startRadiusPercent = 0;
            }

            REAL endRadiusPercent = sg_flagBlinkEnd;
            if (endRadiusPercent < startRadiusPercent)
            {
                endRadiusPercent = startRadiusPercent;
            }

            // get the time the flag will be on
            REAL onTime = sg_flagBlinkTime;

            if ((sg_flagBlinkOnTime > 0) &&
                (sg_flagBlinkOnTime < onTime))
            {
                onTime = sg_flagBlinkOnTime;
            }

            if (time >= (blinkUpdateTime_ + sg_flagBlinkTime))
            {
                // start a new blink
                blinkUpdateTime_ = time;
                blinkTrackUpdateTime_ = time;

                REAL expansionSpeed =
                    (originalRadius_ *
                    (endRadiusPercent - startRadiusPercent)) /
                    onTime;

                SetReferenceTime();

                if (sg_flagBlinkTrackTime > 0)
                {
                    SetPosition(owner_->Position());
                    SetVelocity(owner_->Direction() * owner_->Speed());
                }
                else
                {
                    eCoord estimatedPosition =
                        (owner_->Position() +
                        (owner_->Direction() *
                        (sg_flagBlinkEstimatePosition * owner_->Speed() * onTime)));

                    SetPosition(estimatedPosition);
                    SetVelocity(se_zeroCoord);
                }

                SetRadius(originalRadius_ * startRadiusPercent);
                SetExpansionSpeed(expansionSpeed);
                RequestSync();
            }
            else if (GetRadius() > 0)
            {
                if (time >= (blinkUpdateTime_ + onTime))
                {
                    // kill the blink until the next update time
                    SetReferenceTime();
                    SetVelocity(se_zeroCoord);
                    SetRadius(0);
                    SetExpansionSpeed(0);
                    RequestSync();
                }
                else if ((sg_flagBlinkTrackTime > 0) &&
                    (time >= (blinkTrackUpdateTime_ + sg_flagBlinkTrackTime)))
                {
                    // track the owner again
                    SetReferenceTime();
                    SetPosition(owner_->Position());
                    SetVelocity(owner_->Direction() * owner_->Speed());
                    RequestSync();
                }
            }
        }
    }

    if (passingTheFlag_)
    {
        if (passingOwner_ && !owner_)
        {
            if (passingOwner_->Alive())
            {
                //Calculate new direction
                eCoord newDir = passingOwner_->Position() - GetPosition();
                newDir.Normalize();

                SetReferenceTime();

                //  take in the speed of the flag speed command + the person who is going to receive the flag
                int newSpeed = sg_flagPassSpeed + (passingOwner_->verletSpeed_);
                SetVelocity(newDir * newSpeed);

                RequestSync();
            }
            else
            {
                SetReferenceTime();
                SetVelocity(se_zeroCoord);
                SetRadius(originalRadius_);
                SetExpansionSpeed(0);
                RequestSync();
                positionUpdatePending_ = true;

                SetPassing(false);
            }
        }
        else if(!passingOwner_)
        {
            SetReferenceTime();
            SetVelocity(se_zeroCoord);
            SetRadius(originalRadius_);
            SetExpansionSpeed(0);
            RequestSync();
            positionUpdatePending_ = true;

            SetPassing(false);
        }
    }

    // delegate
    bool returnStatus = gZone::Timestep( time );

    // any pending position updates have been made
    positionUpdatePending_ = false;

    return (returnStatus);
}

// *******************************************************************************
// *
// *    PassTheFlag
// *
// *******************************************************************************

void gFlagZoneHack::PassComplete(gCycle *target)
{
    //  finish passing the flag
    if (passingTheFlag_ && passingOwner_ && passerOwner_)
    {
        tOutput message;

        tColoredString playerName, passingName;
        passingName << passerOwner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
        playerName << target->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

        if (target == passingOwner_)
        {
            message.SetTemplateParameter(1, playerName);
            message.SetTemplateParameter(2, passingName);
            message << "$flag_pass_complete_same";
        }
        else
        {
            message.SetTemplateParameter(1, passingName);
            message.SetTemplateParameter(2, playerName);
            message << "$flag_pass_complete_diff";
        }
        sn_ConsoleOut(message);
    }

    SetPassing(false);
}

void gFlagZoneHack::PassFailed(gCycle *target)
{
    //  finish passing the flag
    if (passingTheFlag_ && passingOwner_ && passerOwner_)
    {
        tOutput message;

        tColoredString playerName, passingName;
        passingName << passerOwner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
        playerName << target->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

        message.SetTemplateParameter(1, passingName);
        message.SetTemplateParameter(2, playerName);
        message << "$flag_pass_failed";
        sn_ConsoleOut(message);
    }

    SetPassing(false);
}

void gFlagZoneHack::SetPassing(bool passed, gCycle *passOwner)
{
    if (passed)
    {
        passingTheFlag_ = true;
        passingOwner_ = passOwner;
        passerOwner_ = owner_;
        passerOwnerFree_ = false;
    }
    else
    {
        passingTheFlag_ = false;
        passingOwner_ = NULL;
        passerOwner_ = NULL;
        passerOwnerFree_ = false;
    }
}

enum gFlagPassMode
{
    gFLAGPASS_DISABLE = 0,  //! Disable passing the flag to team members
    gFLAGPASS_CLOSEST = 1,  //! Picks a team member close to the passing player
    gFLAGPASS_FURTHEST = 2, //! Picks a team member furthur to the passing player
    gFLAGPASS_DISTANCE = 3, //! Picks a team member within the distance specified
    gFLAGPASS_PERSON = 4    //! Picks a team member by the given name
};
tCONFIG_ENUM( gFlagPassMode );

gFlagPassMode sg_flagPassMode = gFLAGPASS_DISABLE;
bool restrictFlagPassMode(const gFlagPassMode &newValue)
{
    if ((newValue < gFLAGPASS_DISABLE) || (newValue > gFLAGPASS_PERSON)) return false;
    return true;
}
static tSettingItem<gFlagPassMode> sg_flagPassModeConf("FLAG_PASS_MODE", sg_flagPassMode, &restrictFlagPassMode);

REAL sg_flagPassDistance = 5.0;
bool restrictFlagPassDistance(const REAL &newValue)
{
    if (newValue < 0) return false;
    return true;
}
static tSettingItem<REAL> sg_flagPassDistanceConf("FLAG_PASS_DISTANCE", sg_flagPassDistance, &restrictFlagPassDistance);

void gFlagZoneHack::PassTheFlag(tString name)
{
    if (owner_)
    {
        if (sg_flagPassMode == gFLAGPASS_DISABLE) return;

        int alive = 0;
        for (int i = 0; i < owner_->Team()->players.Len(); i++)
        {
            ePlayerNetID *p = owner_->Team()->players[i];
            if (p && (p != owner_->Player()))
            {
                gCycle *pCycle = dynamic_cast<gCycle *>(p->Object());

                if (pCycle && pCycle->Alive())
                {
                    alive++;
                }
            }
        }

        //  if no member is alive other than yourself, don't pass the flag
        if (alive == 0) return;

        //  check the modes of each
        gCycle *passOwner = NULL;
        if (sg_flagPassMode == gFLAGPASS_CLOSEST)
        {
            REAL closestDistance = 0;
            for (int i = 0; i < owner_->Team()->players.Len(); i++)
            {
                ePlayerNetID *p = owner_->Team()->players[i];
                if (p && (p != owner_->Player()))
                {
                    gCycle *pCycle = dynamic_cast<gCycle *>(p->Object());

                    if (pCycle && pCycle->Alive())
                    {
                        eCoord otherpos = pCycle->Position() - owner_->Position();
                        REAL distance = otherpos.NormSquared();
                        if (!passOwner || (distance < closestDistance))
                        {
                            passOwner = pCycle;
                            closestDistance = distance;
                        }
                    }
                }
            }

            if (passOwner)
            {
                // put the flag to move towards new owner
                SetReferenceTime();
                SetVelocity(passOwner->Direction() * owner_->verletSpeed_);
                SetPosition(owner_->Position());
                SetRadius(originalRadius_);
                SetExpansionSpeed(0);
                RequestSync();
                positionUpdatePending_ = true;

                //  announce the passing activation
                tOutput message;

                tColoredString playerName, passingName;
                passingName << owner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                playerName << passOwner->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

                message.SetTemplateParameter(1, passingName);
                message.SetTemplateParameter(2, playerName);
                message << "$flag_pass_active";
                sn_ConsoleOut(message);

                //  set the passing owner (one to receive the flag)
                SetPassing(true, passOwner);

                // remove the current owner
                RemoveOwner();
            }
        }
        else if (sg_flagPassMode == gFLAGPASS_FURTHEST)
        {
            REAL furthestDistance = 0;
            for (int i = 0; i < owner_->Team()->players.Len(); i++)
            {
                ePlayerNetID *p = owner_->Team()->players[i];
                if (p && (p != owner_->Player()))
                {
                    gCycle *pCycle = dynamic_cast<gCycle *>(p->Object());

                    if (pCycle && pCycle->Alive())
                    {
                        eCoord otherpos = pCycle->Position() - owner_->Position();
                        REAL distance = otherpos.NormSquared();
                        if (!passOwner || (distance > furthestDistance))
                        {
                            passOwner = pCycle;
                            furthestDistance = distance;
                        }
                    }
                }
            }

            if (passOwner)
            {
                // put the flag to move towards new owner
                SetReferenceTime();
                SetVelocity(passOwner->Direction() * owner_->verletSpeed_);
                SetPosition(owner_->Position());
                SetRadius(originalRadius_);
                SetExpansionSpeed(0);
                RequestSync();
                positionUpdatePending_ = true;

                //  announce the passing activation
                tOutput message;

                tColoredString playerName, passingName;
                passingName << owner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                playerName << passOwner->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

                message.SetTemplateParameter(1, passingName);
                message.SetTemplateParameter(2, playerName);
                message << "$flag_pass_active";
                sn_ConsoleOut(message);

                //  set the passing owner
                SetPassing(true, passOwner);

                // remove the current owner
                RemoveOwner();
            }
        }
        else if (sg_flagPassMode == gFLAGPASS_DISTANCE)
        {
            REAL furthestDistance = 0;
            for (int i = 0; i < owner_->Team()->players.Len(); i++)
            {
                ePlayerNetID *p = owner_->Team()->players[i];
                if (p && (p != owner_->Player()))
                {
                    gCycle *pCycle = dynamic_cast<gCycle *>(p->Object());

                    if (pCycle && pCycle->Alive())
                    {
                        eCoord otherpos = pCycle->Position() - owner_->Position();
                        REAL distance = otherpos.NormSquared();
                        if (!passOwner || (distance > furthestDistance))
                        {
                            passOwner = pCycle;
                            furthestDistance = distance;
                        }

                        //  if distance is over the specified flag distance, break out of the function
                        if (furthestDistance >= sg_flagPassDistance) break;
                    }
                }
            }

            if (passOwner)
            {
                // put the flag to move towards new owner
                SetReferenceTime();
                SetVelocity(passOwner->Direction() * owner_->verletSpeed_);
                SetPosition(owner_->Position());
                SetRadius(originalRadius_);
                SetExpansionSpeed(0);
                RequestSync();
                positionUpdatePending_ = true;

                //  announce the passing activation
                tOutput message;

                tColoredString playerName, passingName;
                passingName << owner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                playerName << passOwner->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

                message.SetTemplateParameter(1, passingName);
                message.SetTemplateParameter(2, playerName);
                message << "$flag_pass_active";
                sn_ConsoleOut(message);

                //  set the passing owner
                SetPassing(true, passOwner);

                // remove the current owner
                RemoveOwner();
            }
        }
        else if (sg_flagPassMode == gFLAGPASS_PERSON)
        {
            if (name.Filter() == "") return;

            ePlayerNetID *p = ePlayerNetID::FindPlayerByName(name);
            if (!p) return;

            //  make sure that only the same team member will be allowed to get the pass of the flag
            if (p->CurrentTeam() != owner_->Team()) return;

            passOwner = dynamic_cast<gCycle *>(p->Object());
            if (passOwner)
            {
                // put the flag to move towards new owner
                SetReferenceTime();
                SetVelocity(passOwner->Direction() * owner_->verletSpeed_);
                SetPosition(owner_->Position());
                SetRadius(originalRadius_);
                SetExpansionSpeed(0);
                RequestSync();
                positionUpdatePending_ = true;

                //  announce the passing activation
                tOutput message;

                tColoredString playerName, passingName;
                passingName << owner_->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);
                playerName << passOwner->Player()->GetColoredName() << tColoredString::ColorString(1,1,1);

                message.SetTemplateParameter(1, passingName);
                message.SetTemplateParameter(2, playerName);
                message << "$flag_pass_active";
                sn_ConsoleOut(message);

                //  set the passing owner
                SetPassing(true, passOwner);

                // remove the current owner
                RemoveOwner();
            }
        }
    }
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gFlagZoneHack::OnEnter( gCycle * target, REAL time )
{
    // make sure target, player, and their team are OK
    if ((!target) ||
        (!target->Player()) ||
        (!target->Player()->CurrentTeam()))
    {
        return;
    }

    // don't process if not initialized yet or owned or updating
    if ((!init_) ||
        (owner_) ||
        (positionUpdatePending_))
    {
        return;
    }

    //check to see if player already has a flag
    bool playerHasFlag = false;
    const tList<eGameObject>& gameObjects = Grid()->GameObjects();
    for (int i=gameObjects.Len()-1;i>=0;i--)
    {
        gFlagZoneHack *otherFlag=dynamic_cast<gFlagZoneHack *>(gameObjects(i));
        if ((otherFlag)){
            if (otherFlag->Owner()){
                //make sure they are on the oposite team else pointless to continue
                if(otherFlag->Team() != target->Player()->CurrentTeam())
                {
                    ePlayerNetID *flagHolder = otherFlag->Owner()->Player();
                    if (flagHolder){
                        if (flagHolder ==  target->Player())
                        {
                            playerHasFlag = true;
                        }
                    }
                }
            }
        }
    }

    // check if the player is on our team or not (check will fail if team not enabled)
    if (target->Player()->CurrentTeam() == team)
    {
        PassFailed(target);

        // player is on our team, if we're not at home, go back
        if (!IsHome())
        {
            // go home
            GoHome();

            sg_flagReturnWriter << target->Player()->GetUserName();
            sg_flagReturnWriter.write();

            tColoredString playerName;
            playerName << *target->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_return", playerName ) );
        }
    }
    // check if this player dropped the flag previously and ensure that player does not have another flag
    else if (((target != ownerDropped_) || (time > (ownerDroppedTime_ + sg_flagDropTime))) && (!playerHasFlag))
    {
        bool process = true;
        if ((sg_flagPassMode > gFLAGPASS_DISABLE) && passingTheFlag_ && !passerOwnerFree_)
            process = false;

        if (process)
        {
            // take the flag
            owner_ = target;
            ownerTime_ = time;
            lastHoldScoreTime_ = time;
            ownerWarnedNotHome_ = false;
            owner_->flag_ = this;

            blinkUpdateTime_ = -1000;
            blinkTrackUpdateTime_ = -1000;

            PassComplete(target);

            // diminish the flag and put it at the original location
            SetReferenceTime();
            SetVelocity(se_zeroCoord);
            SetPosition(originalPosition_);
            SetRadius(0);
            SetExpansionSpeed(0);
            RequestSync();
            positionUpdatePending_ = true;

            sg_flagTakeWriter << target->Player()->GetUserName();
            if(team){
               sg_flagTakeWriter << ePlayerNetID::FilterName( team->Name() );
            }
            sg_flagTakeWriter.write();

            tColoredString playerName;
            playerName << *target->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_take", playerName ) );
        }
    }
}

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gFlagZoneHack::OnExit( gCycle * target, REAL time )
{
    if ((sg_flagPassMode > gFLAGPASS_DISABLE) && passingTheFlag_ && !passerOwnerFree_)
    {
        if (passerOwner_ == target)
            passerOwnerFree_ = true;
    }
}


// *******************************************************************************
// *
// *    WarnFlagNotHome
// *
// *******************************************************************************

void gFlagZoneHack::WarnFlagNotHome()
{
    // if we have an owner, warn them if we haven't already
    if ((owner_) &&
        (!ownerWarnedNotHome_))
    {
        ownerWarnedNotHome_ = true;

        if (owner_->Player())
        {
            tColoredString playerName;
            playerName << *owner_->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_cant_score", playerName ) );
        }
    }
}


// *******************************************************************************
// *
// *    IsHome
// *
// *******************************************************************************

bool gFlagZoneHack::IsHome()
{
    // flag is at home if at the original position and not owned
    if ((!owner_) &&
        (GetPosition() == homePosition_))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}


// *******************************************************************************
// *
// *    GoHome
// *
// *******************************************************************************

void gFlagZoneHack::GoHome()
{
    // remove the owner
    RemoveOwner();

    // circle randomness
    #if 0
    // pick a new home position if randomness is on
    REAL randomness = sg_flagHomeRandomness * gArena::SizeMultiplier();
    if (randomness > originalRadius_)
    {
        REAL homeRadius = randomness - originalRadius_;

        tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();
        REAL randX = (randomizer.Get() * 2.0f) - 1.0f;
        REAL randY = (randomizer.Get() * 2.0f) - 1.0f;

        //??? Would a random turn and random radius be a better use?

        homePosition_.x = randX * homeRadius;
        homePosition_.y = randY * sqrtf((homeRadius * homeRadius) - (homePosition_.x * homePosition_.x));

        homePosition_.x += originalPosition_.x;
        homePosition_.y += originalPosition_.y;
    }
    #endif

    // pick a new home position if randomness is on
    REAL randomnessX = (sg_flagHomeRandomnessX * gArena::SizeMultiplier()) - originalRadius_;
    REAL randomnessY = (sg_flagHomeRandomnessY * gArena::SizeMultiplier()) - originalRadius_;

    if ((randomnessX > 0) || (randomnessY > 0))
    {
        if (randomnessX < 0)
        {
            randomnessX = 0;
        }
        if (randomnessY < 0)
        {
            randomnessY = 0;
        }

        tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();

        REAL randX = (randomizer.Get() * 2.0f) - 1.0f;
        REAL randY = (randomizer.Get() * 2.0f) - 1.0f;

        homePosition_.x = randX * randomnessX;
        homePosition_.y = randY * randomnessY;

        homePosition_.x += originalPosition_.x;
        homePosition_.y += originalPosition_.y;
    }

    // put the flag at home
    SetReferenceTime();
    SetVelocity(se_zeroCoord);
    SetPosition(homePosition_);
    SetRadius(originalRadius_);
    SetExpansionSpeed(0);
    RequestSync();
    positionUpdatePending_ = true;
}


// *******************************************************************************
// *
// *    RemoveOwner
// *
// *******************************************************************************

void gFlagZoneHack::RemoveOwner()
{
    // get rid of the owner
    if (owner_)
    {
        if (owner_->Player())
        {
            if (owner_->Player()->flagOverrideChat)
            {
                owner_->Player()->flagOverrideChat = false;
                owner_->Player()->RequestSync();
            }
        }

        owner_->flag_ = NULL;
        owner_ = NULL;
    }
}


// *******************************************************************************
// *
// *    OwnerDropped
// *
// *******************************************************************************

void gFlagZoneHack::OwnerDropped()
{
    if (owner_)
    {
        // save the last owner and drop time
        ownerDropped_ = owner_;
        ownerDroppedTime_ = lastTime;

        ePlayerNetID *player = owner_->Player();

        if (player)
        {
            //??? prints after round ends... can check if enemies are alive?
            //??? add flag for knowing when grid is getting deleted?
            sg_flagDropWriter << player->GetUserName();
            if(team){
                sg_flagDropWriter << ePlayerNetID::FilterName( team->Name() );
            }
            sg_flagDropWriter.write();

            tColoredString playerName;
            playerName << *player << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_drop", playerName ) );
        }

        if (sg_flagDropHome)
        {
            // this will take the flag home and remove the owner
            GoHome();
        }
        else
        {
            // put the flag at the owner's last position
            SetReferenceTime();
            SetVelocity(se_zeroCoord);
            SetPosition(owner_->Position());
            SetRadius(originalRadius_);
            SetExpansionSpeed(0);
            RequestSync();
            positionUpdatePending_ = true;

            // remove the owner
            RemoveOwner();
        }
    }
}

// *******************************************************************************
// *
// *    gTargetZoneHack settings
// *
// *******************************************************************************

// time in sec before the zone vanished. -1 for infinite
static int sg_targetTimeBeforeVanish = -1;
static tSettingItem<int> sg_targetTimeBeforeVanishConf( "TARGET_LIFETIME", sg_targetTimeBeforeVanish );

// time in sec before the zone vanished once a player entered. -1 for infinite
static int sg_targetTimeBeforeVanishOnceConquered = 10;
static tSettingItem<int> sg_targetTimeBeforeVanishOnceConqueredConf( "TARGET_SURVIVE_TIME", sg_targetTimeBeforeVanishOnceConquered );

// score for the first player entering the zone
static int sg_targetInitScore = 10;
static tSettingItem<int> sg_targetInitScoreConf( "TARGET_INITIAL_SCORE", sg_targetInitScore );

// score suppress to the zone score each time a player entered
static int sg_targetScoreDeplete = 2;
static tSettingItem<int> sg_targetScoreDepleteConf( "TARGET_SCORE_DEPLETE", sg_targetScoreDeplete );

// last target zone is a winzone ?;
static int sg_targetIsWinzone = 1;
static tSettingItem<int> sg_targetIsWinzoneConf( "TARGET_DECLARE_WINNER", sg_targetIsWinzone );

                                 //!< count check zone on grid
int gTargetZoneHack::TargetZoneCounter_ = 0;
                                 //!< game time when a winner can be declared if nothing happens soon
REAL gTargetZoneHack::winnerTime_ = 0;
                                 //!< first player entering the last zone to be declare winner
ePlayerNetID *gTargetZoneHack::winner_ = 0;

static void sg_SetTargetCmd(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if ( !grid )
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    tString params;
    params.ReadLine( s, true );

    // first parse the line to get the param : object_id
    int pos = 0;                 //
    const tString object_id_str = params.ExtractNonBlankSubString(pos);
    tString event_str = params.ExtractNonBlankSubString(pos);
    tString mode_str("new");
    if (event_str=="add") {
        mode_str = tString("add");
        event_str = params.ExtractNonBlankSubString(pos);
    }
    tString cmd_str = params.SubStr(pos);
    // first check for the name
    int zone_id;
    zone_id=gZone::FindFirst(object_id_str);
    if (zone_id==-1)
    {
        zone_id = atoi(object_id_str);
        if (zone_id==0 && object_id_str!="0") return;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    while (zone_id!=-1)
    {
        // get the zone ...
        gTargetZoneHack *zone=dynamic_cast<gTargetZoneHack *>(gameObjects(zone_id));
        if (zone)
        {
            if (event_str == "onenter")
            {
                zone->SetOnEnterCmd(cmd_str, mode_str);
                con << "Zone " << zone->GetID() << " command onenter '" << cmd_str << "'\n";
            }
            else if (event_str == "onvanish")
            {
                zone->SetOnVanishCmd(cmd_str, mode_str);
                con << "Zone " << zone->GetID() << " command onvanish '" << cmd_str << "'\n";
            }
            else if (event_str == "onexit")
            {
                zone->SetOnExitCmd(cmd_str, mode_str);
                con << "Zone " << zone->GetID() << " command onexit '" << cmd_str << "'\n";
            }
            zone_id=gZone::FindNext(object_id_str, zone_id);
        }
    }
}

static tConfItemFunc sg_SetTargetCmd_conf("SET_TARGET_COMMAND",&sg_SetTargetCmd);

// *******************************************************************************
// *
// *    gTargetZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

//  target zone color settings
REAL sg_targetZoneColorR = 0;
REAL sg_targetZoneColorG = 15;
REAL sg_targetZoneColorB = 0;
static tSettingItem<REAL> sg_targetZoneColorRConf("TARGETZONE_COLOR_R", sg_targetZoneColorR);
static tSettingItem<REAL> sg_targetZoneColorGConf("TARGETZONE_COLOR_G", sg_targetZoneColorG);
static tSettingItem<REAL> sg_targetZoneColorBConf("TARGETZONE_COLOR_B", sg_targetZoneColorB);

gTargetZoneHack::gTargetZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    //color_.r = 0.0f;
    //color_.g = 1.0f;
    //color_.b = 0.0f;

    color_.r = sg_targetZoneColorR / 15.0f;
    color_.g = sg_targetZoneColorG / 15.0f;
    color_.b = sg_targetZoneColorB / 15.0f;

    TargetZoneCounter_++;
    winnerTime_ = -1;
    winner_ = 0;
    firstPlayer_ = 0;
    zoneInitialScore_ = sg_targetInitScore;
    zoneScore_ = sg_targetInitScore;
    zoneScoreDeplete_ = sg_targetScoreDeplete;
    timeFirstEntry_ = -1.0;
    targetEmptyTime_ = -1.0;
    currentState_ = State_Safe;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gTargetZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gTargetZoneHack::gTargetZoneHack( nMessage & m )
: gZone( m )
{
    TargetZoneCounter_++;
    winnerTime_ = -1;
    winner_ = 0;
    firstPlayer_ = 0;
    zoneInitialScore_ = sg_targetInitScore;
    zoneScore_ = sg_targetInitScore;
    zoneScoreDeplete_ = sg_targetScoreDeplete;
    timeFirstEntry_ = -1.0;
    targetEmptyTime_ = -1.0;
    currentState_ = State_Safe;
}


// *******************************************************************************
// *
// *    ~gTargetZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gTargetZoneHack::~gTargetZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

static eLadderLogWriter sg_targetzoneTimeoutWriter("TARGETZONE_TIMEOUT", false);
static eLadderLogWriter sg_targetzoneConqueredWriter("TARGETZONE_CONQUERED", false);

bool gTargetZoneHack::Timestep( REAL time )
{
    // check if the zone must collapse ...
    if ( (currentState_ != State_Conquered) && (currentState_ != State_Conquering) && (sg_targetTimeBeforeVanish>0) && ((time - createTime_ - 3.5)>sg_targetTimeBeforeVanish) )
    {
        // zone is conquered, make the zone collapse ...
        currentState_ = State_Conquered;
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius()*.5 );
        RequestSync();
        // send message to ladder log file ...
        sg_targetzoneTimeoutWriter << GetID() << name_ << MapPosition().x << MapPosition().y;
        sg_targetzoneTimeoutWriter.write();
    }

    if ( ((currentState_ == State_Conquering) && (sg_targetTimeBeforeVanishOnceConquered>0) && (time - timeFirstEntry_>sg_targetTimeBeforeVanishOnceConquered))
        ||((currentState_ != State_Conquered) && (zoneScore_<=0) && (zoneScore_!=zoneInitialScore_)) )
    {
        // zone is conquered, make the zone collapse ...
        currentState_ = State_Conquered;
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius()*.5 );
        RequestSync();
        // send message to ladder log file ...
        sg_targetzoneConqueredWriter << GetID() << name_ << MapPosition().x << MapPosition().y;
        if (firstPlayer_)
        {
            sg_targetzoneConqueredWriter << firstPlayer_->GetUserName();
            if ( firstPlayer_->CurrentTeam() )
            {
                sg_targetzoneConqueredWriter << ePlayerNetID::FilterName( firstPlayer_->CurrentTeam()->Name() );
            }
        }
        sg_targetzoneConqueredWriter.write();
    }

    // manage rotation speed
    REAL omega;
    REAL omegaDot;
    REAL maxSpeed = 10 * ( 2 * 3.141 ) / sg_segments;
    if (currentState_ != State_Conquered)
    {
        // set rotation speed accordingly to remaining points ...
        REAL conquered = (zoneInitialScore_==0?0:(REAL)(zoneInitialScore_ - zoneScore_) / (REAL)zoneInitialScore_);
        omega = .3 + conquered * conquered * maxSpeed;
        omegaDot = 2 * conquered * maxSpeed;
    }
    else
    {
        omega = .3 + maxSpeed;
        omegaDot = 2 * maxSpeed;
    }
    REAL timeStep = lastTime;
    if ( sn_GetNetState() != nSERVER )
        timeStep *= 100;

    if ( sn_GetNetState() != nCLIENT &&
        ( ( fabs( omega - GetRotationSpeed() ) + fabs( omegaDot - GetRotationAcceleration() ) ) * timeStep > .5 ) )
    {
        SetRotationSpeed( omega );
        SetRotationAcceleration( omegaDot );
        SetReferenceTime();
        RequestSync();
    }

    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************
static eLadderLogWriter sg_targetzonePlayerEnterWriter("TARGETZONE_PLAYER_ENTER", false);

void gTargetZoneHack::OnEnter( gCycle * target, REAL time )
{
    // make sure target and player are OK
    if ((!target) ||
        (!target->Player()) ||
        (currentState_ == State_Conquered))
    {
        return;
    }

    // RACE HACK begin
    if ( sg_RaceTimerEnabled )
    {
        gRace::ZoneHit( target->Player(), time );
    }
    // RACE HACK end

    // keep first player in memory, if it's the last zone, he is the winner ...
    if (!firstPlayer_)
    {
        firstPlayer_= target->Player();
        timeFirstEntry_ = time;
        currentState_ = State_Conquering;
        // keep winner ...
        winnerTime_ = timeFirstEntry_;
        winner_ = firstPlayer_;
        // send on enter commands ...
        std::istringstream stream(&OnEnterCmd(0));
        tCurrentAccessLevel elevator( sg_SetTargetCmd_conf.GetRequiredLevel(), true );
        tConfItemBase::LoadAll(stream);
    }

    // Check if player already entered this zone
    // If not ...
    if(!isInside(target))
    {
        if (zoneScore_>0)
        {
            // grant the player
            tOutput win;
            tOutput lose;
            win << "$player_target_score_win";
            lose << "$player_target_score_lose";
            target->Player()->AddScore(zoneScore_, win, lose);
        }
        // and decrease zone score value
        zoneScore_-=zoneScoreDeplete_;
        if (zoneScore_<=0)
        {
            zoneScore_=0;
            targetEmptyTime_=time;
        }

        sg_targetzonePlayerEnterWriter << this->GetID() << name_ << MapPosition().x << MapPosition().y << target->Player()->GetUserName() << target->Player()->Object()->MapPosition().x << target->Player()->Object()->MapPosition().y << target->Player()->Object()->Direction().x << target->Player()->Object()->Direction().y << time;
        sg_targetzonePlayerEnterWriter.write();
    }
}

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has left the zone
//!     @param  time    the current time
//!
// *******************************************************************************

static eLadderLogWriter sg_targetzonePlayerLeftWriter("TARGETZONE_PLAYER_LEFT", false);

void gTargetZoneHack::OnExit(gCycle *target, REAL time)
{
    //send on exit commands
    std::istringstream stream(&OnExitCmd(0));
    tCurrentAccessLevel elevator( sg_SetTargetCmd_conf.GetRequiredLevel(), true );
    tConfItemBase::LoadAll(stream);

    sg_targetzonePlayerLeftWriter << this->GetID() << name_ << MapPosition().x << MapPosition().y << target->Player()->GetUserName() << target->Player()->Object()->MapPosition().x << target->Player()->Object()->MapPosition().y << target->Player()->Object()->Direction().x << target->Player()->Object()->Direction().y << time;
    sg_targetzonePlayerLeftWriter.write();

    // RACE HACK begin
    if ( sg_RaceTimerEnabled )
    {
        gRace::ZoneOut( target->Player(), time );
    }
    // RACE HACK end
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gTargetZoneHack::OnVanish( void )
{
    // send on vanish commands ...
    std::istringstream stream(&OnVanishCmd(0));
    tCurrentAccessLevel elevator( sg_SetTargetCmd_conf.GetRequiredLevel(), true );
    tConfItemBase::LoadAll(stream);

    // check if we have a winner ...
    if (sg_targetIsWinzone && (TargetZoneCounter_==1) && firstPlayer_)
    {
        winnerTime_ = timeFirstEntry_;
        winner_ = firstPlayer_;
        static const char* message="$player_target_win_conquest";
        sg_DeclareWinner( firstPlayer_->CurrentTeam(), message );
    }
    // decrement target zone counter
    TargetZoneCounter_--;
}


// *******************************************************************************
// *
// *    gKOHZoneHack settings
// *
// *******************************************************************************

// score added to a player that has been in the zone for KOH_SCORE_TIME
static int sg_kohScore = 1;
static tSettingItem<int> sg_kohScoreConf( "KOH_SCORE", sg_kohScore );

// the interval that KOH_SCORE is added
static int sg_kohScoreTime = 5;
static tSettingItem<int> sg_kohScoreTimeConf( "KOH_SCORE_TIME", sg_kohScoreTime );

// *******************************************************************************
// *
// *    gKOHZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gKOHZoneHack::gKOHZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 1.0f;
    color_.b = 1.0f;

    PlayersInside_=0;
    EnteredTime_ = -1;
    owner_=NULL;

    SetExpansionSpeed(0);
    SetRotationSpeed( .5f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gKOHZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gKOHZoneHack::gKOHZoneHack( nMessage & m )
: gZone( m )
{
    PlayersInside_=0;
    EnteredTime_ = -1;
    owner_=NULL;
}


// *******************************************************************************
// *
// *    ~gKOHZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gKOHZoneHack::~gKOHZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gKOHZoneHack::Timestep( REAL time )
{
    if (PlayersInside_ > 0){
        int totalInside=0;
        float r = GetRadius();
        float rr = r*r;
        for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
        {
            ePlayerNetID *player = se_PlayerNetIDs(i);
            if(!player)
            {
                continue;
            }
            gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
            if(!cycle)
            {
                continue;
            }
            if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
            {
                totalInside++;
            }
        }
        if (totalInside == 1){
            for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i)
            {
                ePlayerNetID *player = se_PlayerNetIDs(i);
                if(!player)
                {
                    continue;
                }
                gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
                if(!cycle)
                {
                    continue;
                }
                if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr)
                {
                    if (owner_ != cycle){
                        owner_=cycle;
                        color_.r = owner_->Player()->r/15.0;
                        color_.g = owner_->Player()->g/15.0;
                        color_.b = owner_->Player()->b/15.0;
                        RequestSync();
                        EnteredTime_=time;
                    }
                }
            }
        }else{
            owner_=NULL;
            EnteredTime_=-1;
            color_.r = 1.0;
            color_.g = 1.0;
            color_.b = 1.0;
            RequestSync();
        }
        PlayersInside_=totalInside;
    }

    // check if the flag is owned
    if (owner_ && PlayersInside_ ==1)
    {
        ePlayerNetID *player = owner_->Player();

        if ((player) &&
            (sg_kohScoreTime > 0) &&
            (time >= (EnteredTime_ + sg_kohScoreTime)))
        {
            tColoredString playerName;
            playerName << *player << tColoredString::ColorString(1,1,1);
            owner_->Player()->AddScore( sg_kohScore, "$player_koh_score", tOutput());
            EnteredTime_=time;

        }
    }


    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gKOHZoneHack::OnEnter( gCycle * target, REAL time )
{
    // make sure target and player are OK
    if ((!target) ||
        (!target->Player())|| (owner_== target))
    {
        return;
    }

    // keep first player in memory, if it's the last zone, he is the winner ...
    if (!owner_)
    {
        owner_= target;
        EnteredTime_ = time;
        PlayersInside_++;
        color_.r = target->Player()->r/15.0;
        color_.g = target->Player()->g/15.0;
        color_.b = target->Player()->b/15.0;
        RequestSync();
    }
    else
    {
        owner_= NULL;
        EnteredTime_ = -1;
        PlayersInside_++;
        color_.r = 1.0;
        color_.g = 1.0;
        color_.b = 1.0;
        RequestSync();
    }

}


// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gKOHZoneHack::OnVanish( void )
{
    owner_= NULL;
    EnteredTime_ = -1;
    PlayersInside_=0;
}


// *******************************************************************************
// *
// *    gTeleportZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

static int sg_ColorTeleZoneRed = 0;
static tSettingItem<int> sg_ColorTeleZoneRedCONF("COLOR_TELEPORTZONE_RED", sg_ColorTeleZoneRed);

static int sg_ColorTeleZoneGreen = 15;
static tSettingItem<int> sg_ColorTeleZoneGreenCONF("COLOR_TELEPORTZONE_GREEN", sg_ColorTeleZoneGreen);

static int sg_ColorTeleZoneBlue = 0;
static tSettingItem<int> sg_ColorTeleZoneBlueCONF("COLOR_TELEPORTZONE_BLUE", sg_ColorTeleZoneBlue);

gTeleportZoneHack::gTeleportZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = sg_ColorTeleZoneRed / 15.0f;//0.0f
    color_.g = sg_ColorTeleZoneGreen / 15.0f;//1.0f
    color_.b = sg_ColorTeleZoneBlue / 15.0f;//0.0f

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gTeleportZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gTeleportZoneHack::gTeleportZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gTeleportZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gTeleportZoneHack::~gTeleportZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gTeleportZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gTeleportZoneHack::OnEnter( gCycle * target, REAL time )
{
    // make sure target and player are OK
    if ((!target) ||
        (!target->Player()))
    {
        return;
    }
    // Jump to new position without creating walls
    eCoord d = target->Direction();
    d.Normalize();
    eCoord n = eCoord(-d.y,d.x);
    eCoord p = target->Position();
    eCoord v = GetPosition() - p;
    REAL vn = v.Norm();
    v.Normalize();
    REAL l = (eCoord::F(v,d)*vn*2.0);
    // if center is behind, you might have been teleported in this zone. Don't jump !
    if (l<=0) return;
    eCoord newPos = (relative==1? p + jump : (relative==2? p + d*jump.x + n*jump.y: jump - v))+d*l*reloc;
    // check if newPos is inside this zone
    //if ((newPos-GetPosition()).Norm()<=GetRadius()) return;
    // if not, let's jump
    target->TeleportTo(newPos, ndir, time);
}


// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gTeleportZoneHack::OnVanish( void )
{
}


// *******************************************************************************
// *
// *    gBlastZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gBlastZoneHack::gBlastZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 0.0f;
    color_.g = 0.5f;
    color_.b = 1.0f;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gBlastZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gBlastZoneHack::gBlastZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gBlastZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gBlastZoneHack::~gBlastZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gBlastZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

static int sg_BlastZoneScore = -1;
static tSettingItem<int> sg_BlastZoneScoreConf("SCORE_BLASTZONE", sg_BlastZoneScore);
static eLadderLogWriter sg_deathDeathBlastZoneWriter("BLASTZONE_PLAYER_ENTER", false);

void gBlastZoneHack::OnEnter( gCycle * target, REAL time )
{
    target->SetWallBuilding(false);

    gCycle *c = dynamic_cast<gCycle *>(target->Player()->Object());
    if(c) c->Kill("BLASTZONE");;

    tOutput lose;
    lose << "$player_blastzone_score";
    target->Player()->AddScore( sg_BlastZoneScore, "", lose);

    sg_deathDeathBlastZoneWriter << target->Player()->GetUserName();
    sg_deathDeathBlastZoneWriter.write();
}


// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gBlastZoneHack::OnVanish( void )
{
}

// *******************************************************************************
// *
// *    gSpeedZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gSpeedZoneHack::gSpeedZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 0.0f;
    color_.g = 1.5f;
    color_.b = 1.0f;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gSpeedZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gSpeedZoneHack::gSpeedZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gSpeedZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gSpeedZoneHack::~gSpeedZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gSpeedZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gSpeedZoneHack::OnEnter( gCycle * target, REAL time )
{
    if (speedType_ == TYPE_ACCEL)
    {
        target->verletSpeed_ += setSpeed_;
    }
    else if (speedType_ == TYPE_SPEED)
    {
        target->verletSpeed_ = setSpeed_;
    }
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gSpeedZoneHack::OnVanish( void )
{
}

// *******************************************************************************
// *
// *    gObjectZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gObjectZoneHack::gObjectZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 0.0f;
    color_.b = 0.5f;

    seekSpeed_ = 1.0;
    
    interactWithZone_ = true;
    lastPollTime_ = 0;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gObjectZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gObjectZoneHack::gObjectZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gObjectZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gObjectZoneHack::~gObjectZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gObjectZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    if (seeking_)
    {
        //Only run this every poll time to save on network traffic
        if (lastTime >= (lastSeekTime_ + seekUpdateTime_))
        {
            //Calculate new direction
            eCoord newDir = pSeekingCycle_->Position() - GetPosition();
            newDir.Normalize();

            SetReferenceTime();
            SetVelocity(newDir * pSeekingCycle_->Speed() * seekSpeed_);
            //??? move out all seeking variables into base zone

            lastSeekTime_ = lastTime;

            RequestSync();
        }
    }

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

static bool sg_objectZonePlayerEnteredInside = false;
static tSettingItem<bool> sg_sg_objectZonePlayerEnteredInsideCONF("LADDERLOG_OBJECTZONE_PLAYER_ENTERED_INSIDE", sg_objectZonePlayerEnteredInside);

static eLadderLogWriter sg_objectZonePlayerEntered("OBJECTZONE_PLAYER_ENTERED", false);
static eLadderLogWriter sg_objectZoneZoneEntered("OBJECTZONE_ZONE_ENTERED", false);
void gObjectZoneHack::OnEnter( gCycle * target, REAL time )
{
    ePlayerNetID *p = target->Player();
    if (p && (sg_objectZonePlayerEnteredInside || !isInside(target)))
    {
        sg_objectZonePlayerEntered << GetID() << name_ << MapPosition().x << MapPosition().y << p->GetUserName() << target->MapPosition().x << target->MapPosition().y << target->Direction().x << target->Direction().y << time;
        sg_objectZonePlayerEntered.write();
    }
}

//  for when zones enter this object zone
void gObjectZoneHack::OnEnter(gZone *target, REAL time)
{
    if (target && !target->destroyed_)
    {
        sg_objectZoneZoneEntered << GetID() << name_ << MapPosition().x << MapPosition().y << target->GetID() << target->GetName() << target->MapPosition().x << target->MapPosition().y << target->GetVelocity().x << target->GetVelocity().y << time;
        sg_objectZoneZoneEntered.write();
    }
}

static eLadderLogWriter sg_ObjectZoneSpawned("OBJECTZONE_SPAWNED", false);

static void sg_SpawnObjectZone(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if(!grid)
        return;

    tString params;
    params.ReadLine(s, true);

    gObjectZoneHack *Zone = NULL;

    if (params.Filter() == "")
    {
        goto usage;
        return;
    }
    else
    {
        float sizeMultiplier = gArena::SizeMultiplier();
        tString name;
        std::vector<eCoord> route;
        int pos = 0;

        name = params.ExtractNonBlankSubString(pos);

        tString zonePosXStr;
        tString zonePosYStr;

        if (name.ToLower() == "n")
        {
            name = params.ExtractNonBlankSubString(pos);
            zonePosXStr = params.ExtractNonBlankSubString(pos);
        }
        else
        {
            zonePosXStr = name;
            name  = "";
        }

        //  handle x position and get y position
        if(zonePosXStr == "L")
        {
            tString x,y;
            while(true)
            {
                x = params.ExtractNonBlankSubString(pos);
                if(x == "Z" || x == "z" || x == "") break;
                y = params.ExtractNonBlankSubString(pos);
                route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
            }
        }
        else
        {
            zonePosYStr = params.ExtractNonBlankSubString(pos);
        }

        tString zoneSizeStr       = params.ExtractNonBlankSubString(pos);
        tString zoneGrowthStr     = params.ExtractNonBlankSubString(pos);
        tString zoneDirXStr       = params.ExtractNonBlankSubString(pos);
        tString zoneDirYStr       = params.ExtractNonBlankSubString(pos);
        tString zoneInteractive   = params.ExtractNonBlankSubString(pos);
        tString zoneRedStr        = params.ExtractNonBlankSubString(pos);
        tString zoneGreenStr      = params.ExtractNonBlankSubString(pos);
        tString zoneBlueStr       = params.ExtractNonBlankSubString(pos);
        tString targetRadiusStr   = params.ExtractNonBlankSubString(pos);
        tString seekOwnerStr      = params.ExtractNonBlankSubString(pos);
        tString seekSpeedStr      = params.ExtractNonBlankSubString(pos);
        tString seekUpdateTimeStr = params.ExtractNonBlankSubString(pos);

        eCoord zonePos    = route.empty() ? eCoord(atof(zonePosXStr)*sizeMultiplier,atof(zonePosYStr)*sizeMultiplier) : route.front();
        REAL zoneSize     = atof(zoneSizeStr)*sizeMultiplier;
        REAL zoneGrowth   = atof(zoneGrowthStr)*sizeMultiplier;

        eCoord zoneDir = eCoord(atof(zoneDirXStr)*sizeMultiplier,atof(zoneDirYStr)*sizeMultiplier);
        gRealColor zoneColor;
        bool setColorFlag = false;
        if ((zoneRedStr.Filter() != "") && (zoneGreenStr.Filter() != "") && (zoneBlueStr.Filter() != ""))
        {
            if (zoneRedStr == "r_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.r = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.r = atof(zoneRedStr) / 15.0;
            }

            if (zoneGreenStr == "g_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.g = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.g = atof(zoneGreenStr) / 15.0;
            }

            if (zoneBlueStr == "b_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.b = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.b = atof(zoneBlueStr) / 15.0;
            }
            setColorFlag = true;
        }

        bool zoneInteractiveBool = false;
        if (zoneInteractive.ToLower() == "true" || zoneInteractive.ToLower() == "1")
            zoneInteractiveBool = true;

        Zone = tNEW(gObjectZoneHack(grid, zonePos, true));
        Zone->SetRadius(zoneSize);
        Zone->SetExpansionSpeed(zoneGrowth);
        Zone->SetVelocity(zoneDir);

        if (setColorFlag)
        {
            zoneColor.r = (zoneColor.r>1.0)?1.0:zoneColor.r;
            zoneColor.g = (zoneColor.g>1.0)?1.0:zoneColor.g;
            zoneColor.b = (zoneColor.b>1.0)?1.0:zoneColor.b;
            Zone->SetColor(zoneColor);
        }

        if (zoneInteractiveBool)
        {
            Zone->SetWallBouncesLeft(-1);
            Zone->SetWallInteract(zoneInteractiveBool);
        }

        REAL targetRadius = atof(targetRadiusStr)*sizeMultiplier;
        if(targetRadius != 0)
            Zone->SetTargetRadius(targetRadius);

        if (seekOwnerStr.Filter() != "")
        {
            ePlayerNetID *seekOwnerPlayer = ePlayerNetID::FindPlayerByName(seekOwnerStr);
            if (seekOwnerPlayer && seekOwnerPlayer->Object() && seekOwnerPlayer->Object()->Alive())
                Zone->SetSeekingCycle(dynamic_cast<gCycle *>(seekOwnerPlayer->Object()));
        }

        REAL seekSpeed = atof(seekSpeedStr);
        if (seekSpeed >= 0)
            Zone->SetSeekSpeed(seekSpeed);
        else
            Zone->SetSeekSpeed(1.0);

        REAL seekingUpdateTime = atof(seekUpdateTimeStr);
        if (seekingUpdateTime >= 0)
            Zone->SetSeekUpdate(seekingUpdateTime);
        else
            Zone->SetSeekUpdate(0.5);

        Zone->SetReferenceTime();
        Zone->SetRotationSpeed( .3f );

        if(!route.empty())
        {
            Zone->SetPosition(route.front());
            for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
            {
                Zone->AddWaypoint(*iter);
            }
        }
        Zone->SetName(name);
        Zone->SetEffect(tString("object"));
        Zone->RequestSync();

#ifdef DEBUG
        con << Zone->GetEffect() << " " << Zone->GetID() << " " << Zone->GetName() << " " << Zone->MapPosition().x << " " << Zone->MapPosition().y << " " << Zone->GetVelocity().x << " " << Zone->GetVelocity().y << " " << (Zone->GetWallInteract()?"true":"false") << " " << Zone->GetColor().r << " " << Zone->GetColor().g << " " << Zone->GetColor().b << "\n";
#endif

        sg_ObjectZoneSpawned << Zone->GetID() << Zone->GetName() << Zone->MapPosition().x << Zone->MapPosition().y << Zone->GetVelocity().x << Zone->GetVelocity().y << se_GameTime();
        sg_ObjectZoneSpawned.write();

        return;
    }

    usage:
    {
        con << "Usage:\n"
                    "SPAWN_OBJECTZONE <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <seek_owner> <seek_speed> <seek_update_time>\n"
                    "Instead of <x> <y> one can write: L <x1> <y1> <x2> <y2> [...] Z\n"
                    "To give the zone a name, SPAWN_OBJECTZONE n <name> ...\n";
    }
}
static tConfItemFunc sg_SpawnObjectZoneConf("SPAWN_OBJECTZONE", &sg_SpawnObjectZone);

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has left the zone
//!     @param  time    the current time
//!
// *******************************************************************************

static eLadderLogWriter sg_objectZonePlayerLeft("OBJECTZONE_PLAYER_LEFT", false);
void gObjectZoneHack::OnExit( gCycle * target, REAL time )
{
    ePlayerNetID *p = target->Player();
    if (p)
    {
        sg_objectZonePlayerLeft << GetID() << name_ << MapPosition().x << MapPosition().y << p->GetUserName() << target->MapPosition().x << target->MapPosition().y << target->Direction().x << target->Direction().y << se_GameTime();
        sg_objectZonePlayerLeft.write();
    }
}

static eLadderLogWriter sg_objectZoneZoneLeft("OBJECTZONE_ZONE_LEFT", false);
void gObjectZoneHack::OnExit( gZone * target, REAL time )
{
    //if (target && !target->destroyed_)
    {
        sg_objectZoneZoneLeft << GetID() << name_ << MapPosition().x << MapPosition().y << target->GetID() << target->GetName() << target->MapPosition().x << target->MapPosition().y << target->GetVelocity().x << target->GetVelocity().y << time;
        sg_objectZoneZoneLeft.write();
    }
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gObjectZoneHack::OnVanish( void )
{
}

// *******************************************************************************
// *
// *    gSoccerZoneHack::CheckTeamAssignment
// *
// *******************************************************************************

bool gSoccerZoneHack::CheckTeamAssignment()
{
    // find the closest player
    if ( !team )
    {
        teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        gCycle *closest = NULL;
        REAL closestDistance = 0;
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other = dynamic_cast<gCycle *>(gameObjects[i]);

            if (other)
            {
                eTeam *otherTeam = other->Player()->CurrentTeam();
                eCoord otherpos = other->Position() - Position();
                REAL distance = otherpos.NormSquared();
                if (!closest || (distance < closestDistance))
                {
                    closest = other;
                    closestDistance = distance;
                }
            }
        }

        if (closest)
        {
            // take over team and color
            team = closest->Player()->CurrentTeam();
            color_.r = team->R()/15.0;
            color_.g = team->G()/15.0;
            color_.b = team->B()/15.0;
            teamDistance_ = closestDistance;

            RequestSync();
        }

        // if this zone does not belong to a team, return false.
        if (!team)
        {
            return false;
        }
    }
    return true;
}

// *******************************************************************************
// *
// *    gSoccerZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

//  this is for the soccer ball
gSoccerZoneHack::gSoccerZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 0.5f;
    color_.b = 0.5f;

    originalPosition_ = pos;
    originalVelocity_ = eCoord(0,0);

    init_ = false;
    ballShots_ = 0;

    teamDistance_ = 0;

    // NOTE: the soccer ball needs to be "interesting" so the soccer goal can detect it.
    grid->AddGameObjectInteresting(this);

    zoneType = 1;
    team = NULL;

    SetWallInteract(true);
    SetWallBouncesLeft(-1);

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}

//  this is for the soccer goal
gSoccerZoneHack::gSoccerZoneHack( eGrid * grid, const eCoord & pos, bool dynamicCreation, eTeam *teamowner, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 0.5f;
    color_.b = 0.5f;

    originalPosition_ = pos;
    originalVelocity_ = eCoord(0,0);

    init_ = false;
    ballShots_ = 0;

    teamDistance_ = 0;
    
    interactWithZone_ = true;

    if (teamowner)
    {
        team = teamowner;
        color_.r = team->R() / 15;
        color_.g = team->G() / 15;
        color_.b = team->B() / 15;
    }
    else
        CheckTeamAssignment();

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gSoccerZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gSoccerZoneHack::gSoccerZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gSoccerZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gSoccerZoneHack::~gSoccerZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool sg_soccerBallSlowdown = true;
static tSettingItem<bool> sg_soccerBallSlowdownConf("SOCCER_BALL_SLOWDOWN", sg_soccerBallSlowdown);

bool sg_soccerBallSlowdownHack = false;
static tSettingItem<bool> sg_soccerBallSlowdownHackConf("SOCCER_BALL_SLOWDOWN_HACKYMETHOD", sg_soccerBallSlowdownHack);

REAL sg_soccerBallSlowdownSpeed = 0.07;
bool restrictBallSlowdownSpeed(const REAL &newValue)
{
    //  if values are less than 0 or greater than 1, no good!
    if ((newValue <= 0) || (newValue >= 1))
            return false;

    //  if values are in between 0 and 1, good!
    return true;
}
static tSettingItem<REAL> sg_soccerBallSlowdownSpeedConf("SOCCER_BALL_SLOWDOWN_SPEED", sg_soccerBallSlowdownSpeed, &restrictBallSlowdownSpeed);

REAL sg_soccerBallSlowdownSync = 0.05;
static tSettingItem<REAL> sg_soccerBallSlowdownSyncConf("SOCCER_BALL_SLOWDOWN_SYNC_INTERVAL", sg_soccerBallSlowdownSync);

bool gSoccerZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    if ((zoneType == gSoccer_GOAL) && !team)
        if (!CheckTeamAssignment()) return true;;

    if (!init_)
    {
        originalVelocity_ = GetVelocity();

        //  less than 0.1 seconds due to unknown reasons...
        //  making sure original radius is found to be greater than 0
        if ((time < 0.1) && (GetRadius() > 0))
            originalRadius_ = GetRadius();
        else
            //  installazation success!
            init_ = true;
    }

    //  for slowing down the soccer ball
    if ((zoneType == gSoccer_BALL) && sg_soccerBallSlowdown && (GetVelocity() != eCoord(0,0)))
    {
        //  store the current speed for setup
        eCoord currentVelocity = GetVelocity();

        if(sg_soccerBallSlowdownHack)
        {
        //  get the values working with decreasing the velocity speed
        if (currentVelocity.x < 0)
            currentVelocity.x += sg_soccerBallSlowdownSpeed;
        else if (currentVelocity.x > 0)
            currentVelocity.x -= sg_soccerBallSlowdownSpeed;

        if (currentVelocity.y < 0)
            currentVelocity.y += sg_soccerBallSlowdownSpeed;
        else if (currentVelocity.y > 0)
            currentVelocity.y -= sg_soccerBallSlowdownSpeed;

        if (((currentVelocity.x > -1) && (currentVelocity.x < 0)) || ((currentVelocity.x > 0) && (currentVelocity.x < 1)))
            currentVelocity.x = 0;

        if (((currentVelocity.y > -1) && (currentVelocity.y < 0)) || ((currentVelocity.y > 0) && (currentVelocity.y < 1)))
            currentVelocity.y = 0;
        }
        else
        {
            REAL dt = time - referenceTime_;
            REAL speed = currentVelocity.Norm();
            if(speed > 0)
            {
                speed -= sg_soccerBallSlowdownSpeed*100*dt;
                if(speed < 0) speed = 0;
                currentVelocity.Normalize();
                currentVelocity *= speed;
            }
            SetReferenceTime();
        }

        //  set new velocity
        SetVelocity(currentVelocity);

        static REAL lasttime = 0;
        REAL systime = tSysTimeFloat();
        if(systime > lasttime || currentVelocity == eCoord(0,0))
        {
            lasttime = systime+sg_soccerBallSlowdownSync;
            RequestSync( currentVelocity == eCoord(0,0) );
        }
    }
    
    if(zoneType == gSoccer_BALL_SCORED && (GetVelocity() != eCoord(0,0)))
    {
        REAL dt = time - referenceTime_;
        eCoord dir = GetVelocity();
        dir.x *= 1-dt; dir.y *= 1-dt;
        SetReferenceTime();
        SetVelocity(dir);
        RequestSync(false);
    }

    return (returnStatus);
}

// *******************************************************************************
// *
// *    GoHome
// *
// *******************************************************************************

void gSoccerZoneHack::GoHome()
{
    //  clear last team to hit the soccer ball
    lastTeamIn_ = NULL;
    lastPlayerIn_ = NULL;

    //  send soccer ball back to it's oriignal place on the grid
    SetReferenceTime();
    SetPosition(originalPosition_);
    SetVelocity(originalVelocity_);
    SetRadius(originalRadius_);
    RequestSync();
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

bool sg_soccerGoalKillEnemies = false;
static tSettingItem<bool> sg_soccerGoalKillEnemiesConf("SOCCER_GOAL_KILL_ENEMIES", sg_soccerGoalKillEnemies);

bool sg_soccerGoalRespawnAllies = true;
static tSettingItem<bool> sg_soccerGoalRespawnAlliesConf("SOCCER_GOAL_RESPAWN_ALLIES", sg_soccerGoalRespawnAllies);

bool sg_soccerGoalRespawnEnemies = true;
static tSettingItem<bool> sg_soccerGoalRespawnEnemiesConf("SOCCER_GOAL_RESPAWN_ENEMIES", sg_soccerGoalRespawnEnemies);

static eLadderLogWriter sg_soccerBallPlayerEntered("SOCCER_BALL_PLAYER_ENTERED", false);
static eLadderLogWriter sg_soccerGoalPlayerEntered("SOCCER_GOAL_PLAYER_ENTERED", false);

void gSoccerZoneHack::OnEnter( gCycle *target, REAL time )
{
    if (!team && (zoneType == gSoccer_BALL || zoneType == gSoccer_BALL_SCORED))
    {
        
        REAL dt = time - referenceTime_;
        
        eCoord dest(posx_(dt), posy_(dt));
        
        //if(sg_ballRimStop)
        {
            // HACK: don't process cycle walls this time...
            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCycleIgnored = true;
            s_zoneWallInteractionCoord = GetPosition();
            s_zoneWallInteractionRadius = GetRadius();
            s_zoneWallInteractionZoneVelocity = GetVelocity();
            s_zoneWallInteractionZoneDeltaTime = time - lastImpactTime_;
            s_zoneWallInteractionZoneDeltaTime = (s_zoneWallInteractionZoneDeltaTime>1)?1:s_zoneWallInteractionZoneDeltaTime;
            grid->ProcessWallsInRange(&S_ZoneWallInteraction,
                s_zoneWallInteractionCoord,
                s_zoneWallInteractionRadius*1.5,
                CurrentFace());
            s_zoneWallInteractionCycleIgnored = false;

            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCoord = dest;
            s_zoneWallInteractionRadius = GetRadius();
            grid->ProcessWallsInRange(&S_ZoneWallIntersect,
                s_zoneWallInteractionCoord,
                s_zoneWallInteractionRadius,
                CurrentFace());
            if (s_zoneWallInteractionFound)
            {
                if(GetVelocity().Norm() == 0)
                {
                    // Attempt to move the ball away from the wall when stuck
                    SetVelocity( (-target->Direction()) * target->Speed() );
                    RequestSync();
                }
                return;
            }
        }
        
        //  calculate the bounce off. Source: gBallZoneHack
        eCoord p2 = target->Position();
        eCoord v2 = target->Direction()*target->Speed();

        REAL r1 = this->GetRadius();
        REAL r2 = target->Player()->ping/.2;                             // the cycle "radius". accomidates for higher ping players
        REAL R = r1 + r2;
        eCoord p1 = this->Position();
        eCoord dp = p2 - p1;
        REAL c = dp.NormSquared() - R * R;
        // first find the real time and position of the impact ...
        eCoord v1 = this->GetVelocity();
        eCoord dv = v2 - v1;
        REAL a = dv.NormSquared();
        REAL b = 2*(dp.x*dv.x+dp.y*dv.y);
        REAL delta = b*b-4*a*c;
        if (delta<0) return;         // no t. it can't be, an impact just occured ...
        delta = sqrt(delta);
        REAL t1 = (-b-delta)/(2*a);
        REAL t2 = (-b+delta)/(2*a);
        REAL t = 0;
        if ((t1>0) && (t2>0)) return;// can't be, again ...
        if (t1>0) t = t2;
        else if (t2>0) t = t1;
        else if (t1>t2) t = t1;
        else t = t2;

        // now that we have the time, get the positions ...
        eCoord p1c = p1+v1*t;
        eCoord p2c = p2+v2*t;
        // now compute the impact : new velocities and new correct positions ...
        eCoord base = (p2c-p1c);
        base.Normalize();
        eCoord new_v1 = base*-target->Speed();
        eCoord new_p1 = p1c + new_v1*(-t+0.01);
        new_v1 = new_v1*(1+sg_ballCycleBoost*target->GetAcceleration()/100);
        
        //if(sg_ballRimStop)
        {
            s_zoneWallInteractionFound = false;
            s_zoneWallInteractionCoord = new_p1 + new_v1 * dt;
            s_zoneWallInteractionRadius = GetRadius();
            grid->ProcessWallsInRange(&S_ZoneWallIntersect,
                s_zoneWallInteractionCoord,
                s_zoneWallInteractionRadius,
                CurrentFace());
            if (s_zoneWallInteractionFound || (t < lastImpactTime_ - time))
            {
                // Try to determine which way the wall we hit was
                if(s_zoneWallInteractionImpactCoord.x == s_zoneWallInteractionClosestCoord.x)
                {
                    new_p1.x = dest.x;
                }
                else if(s_zoneWallInteractionImpactCoord.y == s_zoneWallInteractionClosestCoord.y)
                {
                    new_p1.y = dest.y;
                }
                else
                {
                    // Just set our velocity and let the wall bounce handle it
                    SetVelocity(new_v1);                    
                    RequestSync();
                    return;
                }
            }
        }

        SetPosition(new_p1);
        SetVelocity(new_v1);

        RequestSync();

        //  set last player to hit the zone
        lastTeamIn_ = target->Team();
        lastPlayerIn_ = target->Player();

        sg_soccerBallPlayerEntered << target->Player()->GetUserName() << target->Team()->Name().Filter();
        sg_soccerBallPlayerEntered.write();

    }
    else if (team && (zoneType == gSoccer_GOAL))
    {
        if (sg_soccerGoalKillEnemies)
        {
            if (target && (Team() != target->Team()))
            {
                sn_ConsoleOut(tOutput("$soccer_goal_enemy_entered", target->Player()->GetColoredName(), Team()->GetColoredName()));
                target->deathReason_ << "ENEMY_GOAL_ZONE" << ePlayerNetID::FilterName(team->Name());
                target->Kill();
            }
        }
        else
        {
            if (sg_soccerGoalRespawnAllies && (Team() == target->Team()))
            {
                gSpawnPoint *pSpawn = Arena.ClosestSpawnPoint(GetPosition());
                if (pSpawn)
                {
                    for(int i = 0; i < Team()->NumPlayers(); i++)
                    {
                        ePlayerNetID *p = Team()->Player(i);
                        if (p)
                        {
                            gCycle *cycle = dynamic_cast<gCycle *>(p->Object());
                            if (!cycle || (!cycle->Alive()))
                            if(p->CanRespawn())
                            {
                                eCoord cyclePos, cycleDir;
                                pSpawn->Spawn(cyclePos, cycleDir);

                                gCycle *newCycle = new gCycle(Grid(), cyclePos, cycleDir, p);
                                p->ControlObject(newCycle);
                            }
                        }
                    }
                }
            }
        }

        sg_soccerGoalPlayerEntered << target->Player()->GetUserName() << target->Team()->Name().Filter() << Team()->Name().Filter();
        sg_soccerGoalPlayerEntered.write();
    }
}

int sg_soccerGoalScore = 1;
static tSettingItem<int> sg_soccerGoalScoreConf("SOCCER_GOAL_SCORE", sg_soccerGoalScore);

bool sg_soccerBallFirstWin = false;
static tSettingItem<bool> sg_soccerBallFirstWinConf("SOCCER_BALL_FIRST_WIN", sg_soccerBallFirstWin);

int sg_soccerBallShotsWin = 0;
static tSettingItem<int> sg_soccerBallShotsWinConf("SOCCER_BALL_SHOTS_WIN", sg_soccerBallShotsWin);

bool sg_soccerBallOwnGoalNR = false;
static tSettingItem<bool> sg_soccerBallOwnGoalNRConf("SOCCER_BALL_SCORE_OWN_GOAL", sg_soccerBallOwnGoalNR);

static eLadderLogWriter sg_soccerGoalScored("SOCCER_GOAL_SCORED",true);

void gSoccerZoneHack::OnEnter( gSoccerZoneHack *target, REAL time )
{
    //  check if the zone entering the goal is actually a soccer ball
    if ((GetType() == gSoccer_GOAL) && (target->GetType() == gSoccer_BALL) && target->lastTeamIn_)
    {
        sg_soccerGoalScored << Team()->Name().Filter() << target->lastTeamIn_->Name().Filter() << target->lastPlayerIn_->GetUserName() << time;
        sg_soccerGoalScored.write();
        
        bool ownGoal = (target->lastTeamIn_ == team);
        if(ownGoal && !sg_soccerBallOwnGoalNR)
        {
            sn_ConsoleOut(tOutput("$soccer_goal_self", Team()->GetColoredName()));

            //  time to return ball to home
            target->GoHome();
        }
        else
        {
            //  increase the number of times the ball entered other team's goal
            target->ballShots_++;

            if(ownGoal)
            {
                for(int i=eTeam::teams.Len()-1;i>=0;--i)
                {
                    if(eTeam::teams(i) != Team())
                        (eTeam::teams(i))->AddScore(sg_soccerGoalScore);
                }
                sn_ConsoleOut(tOutput("$player_score_own_goal", Team()->GetColoredName()));
            }
            else
                target->lastTeamIn_->AddScore(sg_soccerGoalScore, tOutput("$soccer_goal_score", target->lastTeamIn_->GetColoredName(), Team()->GetColoredName(), sg_soccerGoalScore), tOutput());
            
            if (sg_soccerBallFirstWin && (target->ballShots_ == 1))
            {
                if(ownGoal)
                {
                    bool win = false;
                    for(int i=eTeam::teams.Len()-1;i>=0;--i)
                    {
                        if(eTeam::teams(i) != Team())
                        {
                            sg_DeclareWinner(eTeam::teams(i));
                            win = true;
                        }
                    }
                    if(!win) sg_DeclareWinner(target->lastTeamIn_);
                }
                else 
                    sg_DeclareWinner(target->lastTeamIn_, tOutput("$soccer_winner"));
                
                //  ball must vanish after having a winner
                target->SetType(gSoccer_BALL_SCORED);
                target->SetReferenceTime();
            }
            else if ((sg_soccerBallShotsWin > 0) && (target->ballShots_ >= sg_soccerBallShotsWin))
            {
                eTeam *thisTeam = team;                     //  the team owner of the goal
                eTeam *shotTeam = target->lastTeamIn_;      //  the last team to hit the ball

                if (thisTeam && shotTeam)
                {
                    //  ensure the team score's are compared
                    if (shotTeam->Score() < thisTeam->Score())
                        sg_DeclareWinner(thisTeam, tOutput("$soccer_winner"));
                    else
                        sg_DeclareWinner(shotTeam, tOutput("$soccer_winner"));

                    //  ball must vanish after having a winner
                    target->SetType(gSoccer_BALL_SCORED);
                    target->SetReferenceTime();
                }
            }
            else
            {
                //  return soccer ball home
                target->GoHome();
            }
        }
    }
}

void gSoccerZoneHack::OnEnter( gZone *target, REAL time )
{
    gSoccerZoneHack *soccerZone = dynamic_cast<gSoccerZoneHack *>(target);
    if(soccerZone) OnEnter(soccerZone, time);
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gSoccerZoneHack::OnVanish( void )
{
    //  respawn soccer ball that suddenly disappears
    if (zoneType == gSoccer_BALL)
    {
        GoHome();
    }
}

// *******************************************************************************
// *
// *    SpawnSoccer
// *
// *******************************************************************************

static void sg_SpawnSoccer(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if(!grid)
        return;

    tString params;
    params.ReadLine(s, true);

    if (params.Filter() == "")
    {
        goto usage;
        return;
    }
    else
    {
        float sizeMultiplier = gArena::SizeMultiplier();
        tString name, type, team;
        std::vector<eCoord> route;
        int pos = 0;
        gZone *Zone = NULL;

        name = params.ExtractNonBlankSubString(pos);
        if (name.ToLower() == "n")
        {
            name = params.ExtractNonBlankSubString(pos);
            type = params.ExtractNonBlankSubString(pos);
        }
        else
        {
            type = name;
            name = "";
        }
        
        type = type.ToLower();
        
        if(type == "ball" || type == "soccerball")
        {
            type = "soccerball";
            //nothing additional to be done.
        }
        else if(type == "goal" || type == "soccergoal")
        {
            type = "soccergoal";
            team = params.ExtractNonBlankSubString(pos);
        }
        else
        {
            goto usage;
            return;
        }

        tString zonePosXStr = params.ExtractNonBlankSubString(pos);
        tString zonePosYStr;

        //  handle x position and get y position
        if(zonePosXStr == "L")
        {
            tString x,y;
            while(true)
            {
                x = params.ExtractNonBlankSubString(pos);
                if(x == "Z" || x == "z" || x == "") break;
                y = params.ExtractNonBlankSubString(pos);
                route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
            }
        }
        else
        {
            zonePosYStr = params.ExtractNonBlankSubString(pos);
        }

        const tString zoneSizeStr       = params.ExtractNonBlankSubString(pos);
        const tString zoneGrowthStr     = params.ExtractNonBlankSubString(pos);
        const tString zoneDirXStr       = params.ExtractNonBlankSubString(pos);
        const tString zoneDirYStr       = params.ExtractNonBlankSubString(pos);
        const tString zoneInteractive   = params.ExtractNonBlankSubString(pos);
        const tString zoneRedStr        = params.ExtractNonBlankSubString(pos);
        const tString zoneGreenStr      = params.ExtractNonBlankSubString(pos);
        const tString zoneBlueStr       = params.ExtractNonBlankSubString(pos);
        const tString targetRadiusStr   = params.ExtractNonBlankSubString(pos);

        eCoord zonePos          = route.empty() ? eCoord(atof(zonePosXStr)*sizeMultiplier,atof(zonePosYStr)*sizeMultiplier) : route.front();
        const REAL zoneSize     = atof(zoneSizeStr)*sizeMultiplier;
        const REAL zoneGrowth   = atof(zoneGrowthStr)*sizeMultiplier;

        eCoord zoneDir = eCoord(atof(zoneDirXStr)*sizeMultiplier,atof(zoneDirYStr)*sizeMultiplier);
        gRealColor zoneColor;
        bool setColorFlag = false;
        if ((zoneRedStr.Filter() != "") && (zoneGreenStr.Filter() != "") && (zoneBlueStr.Filter() != ""))
        {
            if (zoneRedStr == "r_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.r = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.r = atof(zoneRedStr) / 15.0;
            }

            if (zoneGreenStr == "g_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.g = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.g = atof(zoneGreenStr) / 15.0;
            }

            if (zoneBlueStr == "b_rand")
            {
                tRandomizer &randomizer = tRandomizer::GetInstance();
                zoneColor.b = randomizer.Get(0, 15.0) / 15.0;
            }
            else
            {
                zoneColor.b = atof(zoneBlueStr) / 15.0;
            }

            setColorFlag = true;
        }

        eTeam *zoneTeam = NULL;
        if (team.ToLower() == "no_team")
        {
            //REAL teamDistance_ = 0;
            const tList<eGameObject>& gameObjects = grid->GameObjects();
            gCycle *closest = 0;
            REAL closestDistance = 0;
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                gCycle *other=dynamic_cast<gCycle *>(gameObjects[i]);

                if (other)
                {
                    //eTeam * otherTeam = other->Player()->CurrentTeam();
                    eCoord otherpos = other->Position() - zonePos;
                    REAL distance = otherpos.NormSquared();
                    if ( !closest || distance < closestDistance )
                    {
                        closest = other;
                        closestDistance = distance;
                    }
                }
            }

            if ( closest )
            {
                // take over team
                zoneTeam = closest->Player()->CurrentTeam();
            }
        }
        else if (team.Filter() != "")
        {
            zoneTeam = eTeam::FindTeamByName(team);
        }

        //  time to create the zone
        if(type == "soccerball")
        {
            gSoccerZoneHack *sZone = new gSoccerZoneHack(grid, zonePos, true, false);
            sZone->SetType(gSoccerZoneHack::gSoccer_BALL);

            Zone = sZone;
        }
        else if(type == "soccergoal")
        {
            gSoccerZoneHack *sZone = new gSoccerZoneHack(grid, zonePos, true, zoneTeam, false);
            sZone->SetType(gSoccerZoneHack::gSoccer_GOAL);

            Zone = sZone;
        }

        REAL targetRadius = atof(targetRadiusStr)*sizeMultiplier;
        bool zoneInteractiveBool = false;
        if (zoneInteractive.ToLower() == "true" || zoneInteractive.ToLower() == "1")
            zoneInteractiveBool = true;

        CreateZone(type, Zone, zoneSize, zoneGrowth, zoneDir, setColorFlag, zoneColor, zoneInteractiveBool, targetRadius, route, name);
        return;
    }

    usage:
    {
        con << "Usage:\n"
                    "SPAWN_SOCCER ball <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
                    "SPAWN_SOCCER goal <team> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> \n"
                    "Instead of <x> <y> one can write: L <x1> <y1> <x2> <y2> [...] Z\n"
                    "To give the zone a name, SPAWN_SOCCER n <name> ...\n";
    }
}
static tConfItemFunc sg_SpawnSoccerConf("SPAWN_SOCCER", &sg_SpawnSoccer);

// *******************************************************************************
// *
// *    gRespawnZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gRespawnZoneHack::gRespawnZoneHack( eGrid * grid, const eCoord & pos, ePlayerNetID *player, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 0.0f;
    color_.b = 0.5f;

    deadPlayer_ = player;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}


// *******************************************************************************
// *
// *    gRespawnZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gRespawnZoneHack::gRespawnZoneHack( nMessage & m )
: gZone( m )
{
}


// *******************************************************************************
// *
// *    ~gRespawnZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gRespawnZoneHack::~gRespawnZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gRespawnZoneHack::Timestep( REAL time )
{
    //  if player or setting is not active, get rid of zone
    if (!deadPlayer_ || !sg_cycleRespawnZone)
    {
        Finish();
        return true;
    }

    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

bool sg_cycleRespawnZoneEnemies = true;
static tSettingItem<bool> sg_cycleRespawnZoneEnemiesConf("CYCLE_RESPAWN_ZONE_ENEMY", sg_cycleRespawnZoneEnemies);

bool sg_cycleRespawnZoneEnemiesKill = false;
static tSettingItem<bool> sg_cycleRespawnZoneEnemiesKillConf("CYCLE_RESPAWN_ZONE_ENEMY_KILL", sg_cycleRespawnZoneEnemiesKill);

void gRespawnZoneHack::OnEnter( gCycle * target, REAL time )
{
    if (deadPlayer_ && sg_cycleRespawnZone)
    {
        tOutput msg;

        if (sg_cycleRespawnZone && (deadPlayer_->CurrentTeam() == target->Team()))
        {
            if(!deadPlayer_->CanRespawn()) return;
            gCycle *newCycle = new gCycle(grid, GetPosition(), SpawnDirection(), deadPlayer_);
            deadPlayer_->ControlObject(newCycle);

            msg.SetTemplateParameter(1, deadPlayer_->GetColoredName());
            msg.SetTemplateParameter(2, target->Player()->GetColoredName());
            msg << "$cycle_respawn_zone_team";
            sn_ConsoleOut(msg);

            Collapse();

            return;
        }

        if (sg_cycleRespawnZoneEnemiesKill && (deadPlayer_->CurrentTeam() != target->Team()))
        {
            msg.SetTemplateParameter(1, deadPlayer_->GetColoredName());
            msg.SetTemplateParameter(2, target->Player()->GetColoredName());
            msg << "$cycle_respawn_zone_kill_enemy";
            sn_ConsoleOut(msg);

            target->deathReason_ << "RESPAWN_ZONE" << ePlayerNetID::FilterName(team->Name());
            target->Kill();
            return;
        }

        if (sg_cycleRespawnZoneEnemies && (deadPlayer_->CurrentTeam() != target->Team()))
        {
            if(!deadPlayer_->CanRespawn()) return;
            gCycle *newCycle = new gCycle(grid, GetPosition(), SpawnDirection(), deadPlayer_);
            deadPlayer_->ControlObject(newCycle);

            msg.SetTemplateParameter(1, deadPlayer_->GetColoredName());
            msg.SetTemplateParameter(2, target->Player()->GetColoredName());
            msg << "$cycle_respawn_zone_enemy";
            sn_ConsoleOut(msg);

            Collapse();
        }
    }
}

void gRespawnZoneHack::Finish()
{
    ClearDeadPlayer();
    Collapse();
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

bool sg_cycleRespawnZoneRespawn = false;
static tSettingItem<bool> sg_cycleRespawnZoneRespawnConf("CYCLE_RESPAWN_ZONE_RESPAWN", sg_cycleRespawnZoneRespawn);

void gRespawnZoneHack::OnVanish( void )
{
    if (deadPlayer_ && sg_cycleRespawnZone)
    {
        gRespawnZoneHack *newResZone = new gRespawnZoneHack(grid, GetPosition(), deadPlayer_, true);
        newResZone->SetRadius(sg_cycleRespawnZoneRadius * gArena::SizeMultiplier());
        newResZone->SetExpansionSpeed(sg_cycleRespawnZoneGrowth);
        newResZone->SetColor(GetColor());
        newResZone->SetSpawnDirection(SpawnDirection());
    }
}

// *******************************************************************************
// *
// *    gCheckpointZoneHack
// *
// *******************************************************************************
//!
//!     @param  grid Grid to put the zone into
//!     @param  pos  Position to spawn the zone at
//!
// *******************************************************************************

gCheckpointZoneHack::gCheckpointZoneHack( eGrid * grid, const eCoord & pos, int checkpointId, int checkpointTime, bool dynamicCreation, bool delayCreation)
:gZone( grid, pos, dynamicCreation, delayCreation)
{
    color_.r = 1.0f;
    color_.g = 0.0f;
    color_.b = 0.5f;

    checkpointId_ = checkpointId;

    checkpointTime_ = checkpointTime;
    if (checkpointTime_ < 0)
        checkpointTime_ = 0;

    for(int i=0; i<MAXCLIENTS; i++) wrongPlayerEntries_[i] = false;

    SetExpansionSpeed(0);
    SetRotationSpeed( .3f );
    RequestSync();
}

int sg_NumCheckpointZones()
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return 0;

    int checkpoint_zones = 0;
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int i = 0; i < gameObjects.Len(); i++)
    {
        gCheckpointZoneHack *checkpointZone = dynamic_cast<gCheckpointZoneHack *>(gameObjects[i]);
        if (checkpointZone) checkpoint_zones++;
    }

    return checkpoint_zones;
}

tArray<gCheckpointZoneHack *> sg_GetCheckpointZones()
{
    tArray<gCheckpointZoneHack *> checkpointZones;

    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return checkpointZones;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int i = 0; i < gameObjects.Len(); i++)
    {
        gCheckpointZoneHack *checkpointZone = dynamic_cast<gCheckpointZoneHack *>(gameObjects[i]);
        if (checkpointZone) checkpointZones.Insert(checkpointZone);
    }

    return checkpointZones;
}

// *******************************************************************************
// *
// *    gCheckpointZoneHack
// *
// *******************************************************************************
//!
//!     @param  m Message to read creation data from
//!     @param  null
//!
// *******************************************************************************

gCheckpointZoneHack::gCheckpointZoneHack( nMessage & m )
: gZone( m )
{
    for(int i=0; i<MAXCLIENTS; i++) wrongPlayerEntries_[i] = false;
}


// *******************************************************************************
// *
// *    ~gCheckpointZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gCheckpointZoneHack::~gCheckpointZoneHack( void )
{
}


// *******************************************************************************
// *
// *    Timestep
// *
// *******************************************************************************
//!
//!     @param  time    the current time
//!
// *******************************************************************************

bool gCheckpointZoneHack::Timestep( REAL time )
{
    // delegate
    bool returnStatus = gZone::Timestep( time );

    return (returnStatus);
}


// *******************************************************************************
// *
// *    OnEnter
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has been found inside the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gCheckpointZoneHack::OnEnter( gCycle * target, REAL time )
{
    gRacePlayer *racer = gRacePlayer::GetPlayer(target->Player());
    if (racer && sg_RaceTimerEnabled)
    {
        if ((racer->NextCheckpoint() == -1) && (!racer->CheckpointsDone()))
        {
            int next_checkpoint = -1;

            tArray<gCheckpointZoneHack *> checkpointZonesList = sg_GetCheckpointZones();
            for(int i = 0; i < checkpointZonesList.Len(); i++)
            {
                gCheckpointZoneHack *checkZone = checkpointZonesList[i];
                if (checkZone)
                {
                    int nextCheckZoneID = checkZone->CheckpointID();
                    bool checkpointDone = false;

                    std::deque<int>::iterator it = racer->checkpointsDoneList.begin();
                    for(; it != racer->checkpointsDoneList.end(); it++)
                    {
                        if (nextCheckZoneID == *it)
                        {
                            checkpointDone = true;
                            break;
                        }
                    }

                    //  don't process this checkpoint if it's done
                    if (checkpointDone) continue;

                    //  get the lowest checkpoint id (greater than 0)
                    if ((next_checkpoint == -1) || (nextCheckZoneID < next_checkpoint))
                        next_checkpoint = nextCheckZoneID;
                }
            }

            //  if there is a checkpoint to assign, do it.
            if (next_checkpoint > 0)
            {
                racer->SetNextCheckpoint(next_checkpoint);
                racer->SetCanFinish(false);
                racer->SetCheckpointsDone(false);
            }
            //  if not, let them finish the race
            else
            {
                racer->SetCanFinish(true);
                racer->SetCheckpointsDone(true);
            }
        }

        std::deque<int>::iterator it = racer->checkpointsDoneList.begin();
        for(; it != racer->checkpointsDoneList.end(); it++)
            if (checkpointId_ == *it)
                return;

        if (!wrongPlayerEntries_[target->Player()->ListID()])
        {
            if (sg_RaceCheckpointRequireHit == 0)
                if (sg_RaceCheckpointCountdown > 0)
                    racer->SetCountdown(racer->Countdown() + checkpointTime_);

            if ((sg_RaceCheckpointRequireHit == 1) || ((sg_RaceCheckpointRequireHit == 2) && (racer->NextCheckpoint() == checkpointId_)))
            {
                bool all_done = false;
                int next_checkpoint = -1;

                racer->checkpointsDoneList.push_back(checkpointId_);    //  add as done

                tArray<gCheckpointZoneHack *> checkpointZonesList = sg_GetCheckpointZones();
                for(int i = 0; i < checkpointZonesList.Len(); i++)
                {
                    gCheckpointZoneHack *checkZone = checkpointZonesList[i];
                    if (checkZone)
                    {
                        int nextCheckZoneID = checkZone->checkpointId_;
                        bool checkpointDone = false;

                        std::deque<int>::iterator it = racer->checkpointsDoneList.begin();
                        for(; it != racer->checkpointsDoneList.end(); it++)
                        {
                            if (nextCheckZoneID == *it)
                            {
                                checkpointDone = true;
                                break;
                            }
                        }

                        //  don't process this checkpoint if it's done
                        if (checkpointDone) continue;

                        if ((next_checkpoint == -1) || (nextCheckZoneID < next_checkpoint))
                            next_checkpoint = nextCheckZoneID;
                    }
                }

                //  check if a new checkpoint is found
                if (next_checkpoint > 0)
                    racer->SetNextCheckpoint(next_checkpoint);    //  move to next
                else
                    all_done = true;

                if (sg_RaceCheckpointRequireHit == 1)
                {
                    sn_ConsoleOut(tOutput("$race_checkpoint_done", checkpointId_), racer->Player()->Owner());

                    if (all_done)
                    {
                        sn_ConsoleOut(tOutput("$race_checkpoints_complete"), racer->Player()->Owner());
                        racer->SetCanFinish(true);
                        racer->SetCheckpointsDone(true);
                    }
                }
                else if (sg_RaceCheckpointRequireHit == 2)
                {
                    //  if they've gone through all the checkpoints
                    //  they can then finish the race by crossing the
                    //  finish line (win/target zone)
                    if (all_done)
                    {
                        sn_ConsoleOut(tOutput("$race_checkpoints_complete"), racer->Player()->Owner());
                        racer->SetCanFinish(true);
                        racer->SetCheckpointsDone(true);
                    }
                    else
                    {
                        sn_ConsoleOut(tOutput("$race_checkpoint_next", checkpointId_, racer->NextCheckpoint()), racer->Player()->Owner());
                    }
                }

                //  add the time to the countdown
                if (sg_RaceCheckpointCountdown > 0)
                    racer->SetCountdown(racer->Countdown() + checkpointTime_);
            }
            else
            {
                if (sg_RaceCheckpointRequireHit == 2)
                    sn_ConsoleOut(tOutput("$race_checkpoint_wrong", checkpointId_, racer->NextCheckpoint()), racer->Player()->Owner());
            }
        }
    }
    wrongPlayerEntries_[target->Player()->ListID()] = true;
}

// *******************************************************************************
// *
// *    OnExit
// *
// *******************************************************************************
//!
//!     @param  target  the cycle that has left the zone
//!     @param  time    the current time
//!
// *******************************************************************************

void gCheckpointZoneHack::OnExit( gCycle * target, REAL time )
{
    if (wrongPlayerEntries_[target->Player()->ListID()])
        wrongPlayerEntries_[target->Player()->ListID()] = false;
}

// *******************************************************************************
// *
// *    OnVanish
// *
// *******************************************************************************

void gCheckpointZoneHack::OnVanish( void )
{
}

// *******************************************************************************
// *
// *    Spawn_Zone
// *
// *******************************************************************************

bool styctcompat_se_spawnZone = false;
static tConfItem<bool> styctcompat_se_spawnZoneConf("STYCT_COMPATIBILITY_SPAWN_ZONE",styctcompat_se_spawnZone);


static void sg_CreateZone_conf(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if(!grid || grid->GameObjects().Len() == 0)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    tString params;
    params.ReadLine( s, true );
    //retrieve the size multiplier once, using it a lot
    float sizeMultiplier = gArena::SizeMultiplier();

    // first parse the line to get the params : <win|death> <x> <y> <size> <growth>
    eTeam *zoneTeam=NULL;
    int pos = 0;                 //
    tString zoneTypeStr = params.ExtractNonBlankSubString(pos);
    tString zoneNameStr;
    if ((zoneTypeStr=="n")||(zoneTypeStr=="N"))
    {
        zoneNameStr = params.ExtractNonBlankSubString(pos);
        zoneTypeStr = params.ExtractNonBlankSubString(pos);
    }
    else
    {
        zoneNameStr = tString("");
    }
    tString zoneTeamStr = tString("");
    if ((zoneTypeStr=="fortress")||(zoneTypeStr=="flag")||(zoneTypeStr=="deathTeam")||(zoneTypeStr=="ballTeam"))
    {
        zoneTeamStr = ePlayerNetID::FilterName(params.ExtractNonBlankSubString(pos));
        for (int i = eTeam::teams.Len() - 1; i>=0; --i)
        {
            tString teamName = eTeam::teams(i)->Name();
            if (zoneTeamStr==ePlayerNetID::FilterName(teamName))
            {
                zoneTeam = eTeam::teams(i);
                break;
            }
        }
    }

    REAL setSpeed = 0;
    if ((zoneTypeStr == "acceleration") || (zoneTypeStr == "speed"))
    {
        setSpeed = atof(params.ExtractNonBlankSubString(pos));
    }

    ePlayerNetID *zonePlayer = 0;
    ePlayerNetID *zoneOwner = 0;
    if(zoneTypeStr=="zombie" || zoneTypeStr == "zombieOwner")
    {
        tString targetPlayer = params.ExtractNonBlankSubString(pos);
        zonePlayer = ePlayerNetID::FindPlayerByName(targetPlayer, NULL);

        if(!zonePlayer)
        {
            return;
        }
    }
    if(zoneTypeStr == "zombieOwner")
    {
        tString targetPlayer = params.ExtractNonBlankSubString(pos);
        zoneOwner = ePlayerNetID::FindPlayerByName(targetPlayer, NULL);
        if(zoneOwner)
        {
            zoneTeam = zoneOwner->CurrentTeam();
        }
    }

    tString zonePosXStr = params.ExtractNonBlankSubString(pos);
    tString zonePosYStr;

    //  handle x position and get y position
    std::vector<eCoord> route;
    if(zonePosXStr == "L")
    {
        tString x,y;
        while(true)
        {
            x = params.ExtractNonBlankSubString(pos);
            if(x == "Z" || x == "z" || x == "") break;
            y = params.ExtractNonBlankSubString(pos);
            route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
        }
    }
    else
    {
        zonePosYStr = params.ExtractNonBlankSubString(pos);
    }

    const tString zoneSizeStr = params.ExtractNonBlankSubString(pos);
    const tString zoneGrowthStr = params.ExtractNonBlankSubString(pos);
    const tString zoneDirXStr = params.ExtractNonBlankSubString(pos);
    const tString zoneDirYStr = params.ExtractNonBlankSubString(pos);
    tString  zoneRubberStr;
    REAL zoneRubber;
    if(zoneTypeStr == "rubber"){
        zoneRubberStr = params.ExtractNonBlankSubString(pos);
        zoneRubber = atof(zoneRubberStr);
        con << zoneRubber;
    }
    eCoord zoneJump;
    int relJump=0;
    eCoord ndir;
    REAL reloc=0;
    if(zoneTypeStr == "teleport"){
        int prevPos = pos;
        
        tString  zoneJumpXStr;
        tString  zoneJumpYStr;
        zoneJumpXStr = params.ExtractNonBlankSubString(pos);
        char chint = zoneJumpXStr.ToLower()[0];
        bool isInter = (chint == 't' || chint == 'f');
        zoneJumpYStr = params.ExtractNonBlankSubString(pos);
        if(isInter)
            zoneJump = eCoord(0.f,0.f);
        else
            zoneJump = eCoord(atof(zoneJumpXStr)*sizeMultiplier,atof(zoneJumpYStr)*sizeMultiplier);
        tString zoneRelAbsStr = params.ExtractNonBlankSubString(pos);
        if (isInter || zoneRelAbsStr=="") zoneRelAbsStr="rel";
        if (zoneRelAbsStr=="rel") relJump=1;
        else if (zoneRelAbsStr=="cycle") relJump=2;
        else relJump = 0;
        
        if(isInter) pos = prevPos;

        prevPos = pos;
        const tString xdir_str = params.ExtractNonBlankSubString(pos);
        chint = xdir_str.ToLower()[0];
        isInter = (chint == 't' || chint == 'f');
        
        REAL xdir = atof(xdir_str);
        const tString ydir_str = params.ExtractNonBlankSubString(pos);
        REAL ydir = atof(ydir_str);
        if (isInter || xdir_str == "") xdir = 0.0;
        if (isInter || ydir_str == "") ydir = 0.0;
        ndir = eCoord(xdir*sizeMultiplier,ydir*sizeMultiplier);
        const tString reloc_str = params.ExtractNonBlankSubString(pos);
        reloc = atof(reloc_str);
        if (isInter || reloc_str == "") reloc = 1.0;
        
        if(isInter) pos = prevPos;
    }
    const tString zoneInteractive = params.ExtractNonBlankSubString(pos);
    const tString zoneRedStr      = params.ExtractNonBlankSubString(pos);
    const tString zoneGreenStr    = params.ExtractNonBlankSubString(pos);
    const tString zoneBlueStr     = params.ExtractNonBlankSubString(pos);
    const tString targetRadiusStr = params.ExtractNonBlankSubString(pos);
    const tString zonePenetrate   = params.ExtractNonBlankSubString(pos);

    // second convert params ( to do : check validity).
    eCoord zonePos = route.empty() ? eCoord(atof(zonePosXStr)*sizeMultiplier,atof(zonePosYStr)*sizeMultiplier) : route.front();
    const REAL zoneSize = atof(zoneSizeStr)*sizeMultiplier;
    const REAL zoneGrowth = atof(zoneGrowthStr)*sizeMultiplier;
    //const REAL zoneRubber = atof(zoneRubberStr);
    eCoord zoneDir = eCoord(atof(zoneDirXStr)*sizeMultiplier,atof(zoneDirYStr)*sizeMultiplier);
    gRealColor zoneColor;
    bool setColorFlag = false;
    if ((zoneRedStr!="")&&(zoneGreenStr!="")&&(zoneBlueStr!=""))
    {
        if (zoneRedStr == "r_rand")
        {
            tRandomizer &randomizer = tRandomizer::GetInstance();
            zoneColor.r = randomizer.Get(0, 15.0) / 15.0;
        }
        else if(styctcompat_se_spawnZone)
            zoneColor.r = atof(zoneRedStr);
        else
            zoneColor.r = atof(zoneRedStr) / 15.0;

        if (zoneGreenStr == "g_rand")
        {
            tRandomizer &randomizer = tRandomizer::GetInstance();
            zoneColor.g = randomizer.Get(0, 15.0) / 15.0;
        }
        else if(styctcompat_se_spawnZone)
            zoneColor.g = atof(zoneGreenStr);
        else
            zoneColor.g = atof(zoneGreenStr) / 15.0;

        if (zoneBlueStr == "b_rand")
        {
            tRandomizer &randomizer = tRandomizer::GetInstance();
            zoneColor.b = randomizer.Get(0, 15.0) / 15.0;
        }
        else if(styctcompat_se_spawnZone)
            zoneColor.b = atof(zoneBlueStr);
        else
            zoneColor.b = atof(zoneBlueStr) / 15.0;

        setColorFlag = true;
    }

    // if no_team was pass, assigns one: the one that has a player spawned closest
    if ( zoneTeamStr=="no_team" )
    {
        //REAL teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = grid->GameObjects();
        gCycle * closest = 0;
        REAL closestDistance = 0;
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

            if (other )
            {
                //eTeam * otherTeam = other->Player()->CurrentTeam();
                eCoord otherpos = other->Position() - zonePos;
                REAL distance = otherpos.NormSquared();
                if ( !closest || distance < closestDistance )
                {
                    closest = other;
                    closestDistance = distance;
                }
            }
        }

        if ( closest )
        {
            // take over team
            zoneTeam = closest->Player()->CurrentTeam();
        }
    }
    // then create zone ...
    gZone *Zone = NULL;
    if ( zoneTypeStr=="death" || zoneTypeStr=="deathTeam" )
    {
        Zone = tNEW( gDeathZoneHack( grid, zonePos, true, zoneTeam ) );
        //sn_ConsoleOut( "$instant_death_activated" );
        if (zoneTeam!=NULL)
        {
            zoneColor.r = zoneTeam->R()/15.0;
            zoneColor.g = zoneTeam->G()/15.0;
            zoneColor.b = zoneTeam->B()/15.0;
            zoneColor.r += (1.0 - zoneColor.r) / 1.8;
            zoneColor.g += (1.0 - zoneColor.g) / 1.8;
            zoneColor.b += (1.0 - zoneColor.b) / 1.8;
            setColorFlag = true;
        }
    }
    else if ( zoneTypeStr=="win")
    {
        Zone = tNEW( gWinZoneHack( grid, zonePos, true ) );
        //sn_ConsoleOut( "$instant_win_activated" );
    }
    else if ( zoneTypeStr=="rubber")
    {
        gRubberZoneHack *rZone = tNEW(gRubberZoneHack(grid, zonePos, true));
        rZone->SetRubberType(gRubberZoneHack::TYPE_RUBBER);
        rZone->SetRubber(zoneRubber);
        Zone = rZone;
    }
    else if ( zoneTypeStr=="rubberadjust")
    {
        gRubberZoneHack *rZone = tNEW(gRubberZoneHack(grid, zonePos, true));
        rZone->SetRubberType(gRubberZoneHack::TYPE_ADJUST);
        rZone->SetRubber(zoneRubber);
        Zone = rZone;
    }
    else if ( zoneTypeStr=="teleport")
    {
        gTeleportZoneHack *rZone = tNEW(gTeleportZoneHack(grid, zonePos, true));
        rZone->SetJump(zoneJump,relJump);
        rZone->SetNewDir(ndir);
        rZone->SetReloc(reloc);
        Zone = rZone;
    }
    else if ( zoneTypeStr=="ball" || zoneTypeStr=="ballTeam" )
    {
        Zone = tNEW( gBallZoneHack( grid, zonePos, true, zoneTeam ) );
        //sn_ConsoleOut( "$instant_win_activated" );
        if (zoneTeam!=NULL)
        {
            zoneColor.r = zoneTeam->R()/15.0;
            zoneColor.g = zoneTeam->G()/15.0;
            zoneColor.b = zoneTeam->B()/15.0;
            zoneColor.r += (1.0 - zoneColor.r) / 1.8;
            zoneColor.g += (1.0 - zoneColor.g) / 1.8;
            zoneColor.b += (1.0 - zoneColor.b) / 1.8;
            setColorFlag = true;
        }
    }
    else if ( zoneTypeStr=="flag" )
    {
        Zone = tNEW( gFlagZoneHack( grid, zonePos, true, zoneTeam ) );
        //sn_ConsoleOut( "$instant_win_activated" );
        if (zoneTeam!=NULL)
        {
            zoneColor.r = zoneTeam->R()/15.0;
            zoneColor.g = zoneTeam->G()/15.0;
            zoneColor.b = zoneTeam->B()/15.0;
            zoneColor.r += (1.0 - zoneColor.r) / 1.8;
            zoneColor.g += (1.0 - zoneColor.g) / 1.8;
            zoneColor.b += (1.0 - zoneColor.b) / 1.8;
            setColorFlag = true;
        }
    }
    else if ( zoneTypeStr=="fortress" )
    {
        gBaseZoneHack *fZone = tNEW( gBaseZoneHack( grid, zonePos, true, zoneTeam ) );
        // if this zone does not belong to a team, discard it.
        if ( !fZone->CheckTeamAssignmentOnTeam() )
        {
            fZone->RemoveFromGame();
            return;
        }
        Zone = fZone;
        //sn_ConsoleOut( "$instant_win_activated" );
        if (Zone->Team()!=NULL)
        {
            zoneColor.r = Zone->Team()->R()/15.0;
            zoneColor.g = Zone->Team()->G()/15.0;
            zoneColor.b = Zone->Team()->B()/15.0;
            setColorFlag = true;
        }
    }
    else if ( zoneTypeStr=="sumo" )
    {
        Zone = tNEW( gSumoZoneHack( grid, zonePos, true ));
    }
    else if ( zoneTypeStr=="zombie" || zoneTypeStr=="zombieOwner" )
    {
        gCycle *target = dynamic_cast<gCycle *>(zonePlayer->Object());
        if(target)
        {
            gDeathZoneHack *dZone = tNEW(gDeathZoneHack(grid, zonePos, true));
            dZone->SetSeekingCycle(target);
            dZone->SetType(gDeathZoneHack::TYPE_ZOMBIE_ZONE);
            if(zoneOwner)
            {
                dZone->SetOwner(zoneOwner);
                if (zoneTeam!=NULL)
                {
                    zoneColor.r = zoneTeam->R()/15.0;
                    zoneColor.g = zoneTeam->G()/15.0;
                    zoneColor.b = zoneTeam->B()/15.0;
                    setColorFlag = true;
                }
            }
            Zone = dZone;
        }
        else
        {
            goto usage;
        }
    }
    else if ( zoneTypeStr=="target" )
    {
        Zone = tNEW( gTargetZoneHack( grid, zonePos, true ) );
    }
    else if ( zoneTypeStr=="blast" )
    {
        Zone = tNEW( gBlastZoneHack( grid, zonePos, true ) );
    }
    else if (zoneTypeStr=="koh"){
        Zone = tNEW( gKOHZoneHack( grid, zonePos, true ) );
    }
    else if (zoneTypeStr == "acceleration")
    {
        gSpeedZoneHack *bZone = tNEW(gSpeedZoneHack(grid, zonePos, true));
        bZone->SetSpeedType(gSpeedZoneHack::TYPE_ACCEL);
        bZone->SetSpeedVal(setSpeed);

        Zone = bZone;
    }
    else if (zoneTypeStr == "speed")
    {
        gSpeedZoneHack *bZone = tNEW(gSpeedZoneHack(grid, zonePos, true));
        bZone->SetSpeedType(gSpeedZoneHack::TYPE_SPEED);
        bZone->SetSpeedVal(setSpeed);

        Zone = bZone;
    }
    else if (zoneTypeStr=="object"){
        Zone = tNEW( gObjectZoneHack( grid, zonePos, true ) );
    }
    else
    {
        usage:
        con << "Usage:\n"
            "SPAWN_ZONE <win|death|ball|target|blast|object|koh> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE <acceleration|speed> <speed> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE <rubber|rubberadjust> <x> <y> <size> <growth> <xdir> <ydir> <rubber> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE teleport <x> <y> <size> <growth> <xdir> <ydir> [<xjump> <yjump> <rel|abs> [<xdir> <ydir> <reloc>]] <interactive=true|false> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE <fortress|flag|deathTeam|ballTeam> <team> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE sumo <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE zombie <player> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "SPAWN_ZONE zombieOwner <player> <owner> <x> <y> <size> <growth> <xdir> <ydir> <interactive> <r> <g> <b> <target_size> <penetrate> \n"
            "instead of <x> <y> one can write: L <x1> <y1> <x2> <y2> [...] Z\n";
        return;
    }

    REAL targetRadius = atof(targetRadiusStr) * sizeMultiplier;
    bool zoneInteractiveBool =false;
    if ((zoneInteractive.ToLower() == "true") || (zoneInteractive.ToLower() == "1")){
        zoneInteractiveBool=true;
    }
    bool zonePenetrateBool = false;
    if (zonePenetrate.ToLower() == "true" || zonePenetrate.ToLower() == "1")
        zonePenetrateBool = true;
    CreateZone(zoneTypeStr, Zone, zoneSize, zoneGrowth, zoneDir, setColorFlag, zoneColor, zoneInteractiveBool, targetRadius, route, zoneNameStr, zonePenetrateBool);
}


static tConfItemFunc sg_CreateZone_c("SPAWN_ZONE",&sg_CreateZone_conf);

std::map<REAL, std::set<gZone*> > gZone::delayedZones_;
void gZone::Timesteps(REAL currentTime)
{
    if (delayedZones_.empty()) return;
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return;

    std::map<REAL, std::set<gZone*> >::iterator it = delayedZones_.begin(), itNext=it;
    for (; it != delayedZones_.end(); it=itNext)
    {
        itNext++;
        if (currentTime >= it->first)
        {
            std::set<gZone*>::iterator sit = it->second.begin();
            for(; sit != it->second.end(); ++sit)
            {
                gZone *Zone = *sit;
                if (Zone)
                {
                    Zone->AddToList();
                    sg_Zones.push_back(Zone);
                    Zone->RequestSync();
                }
            }
            delayedZones_.erase(it);
        }
    }
}

void gZone::AddDelay(REAL delayTime, gZone *Zone)
{
    delayedZones_[delayTime].insert(Zone);
}

void gZone::ClearDelay()
{
    delayedZones_.clear();
}

static void sg_zoneDelayClear(std::istream &s)
{
    gZone::ClearDelay();
}
static tConfItemFunc sg_zoneDelayClearConf("ZONE_DELAY_CLEAR", &sg_zoneDelayClear);


gZone &gZone::AddWaypoint(eCoord const &point)
{
    route_.push_back(point);
    return *this;
}
gZone &gZone::ClearWaypoints()
{
    route_.clear();
    lastCoord_ = 0;
    nextUpdate_ = -1;
    return *this;
}


static eLadderLogWriter sg_zoneGridPosWriter("ZONE_GRIDPOS", false);

void gZone::WriteLadderLog()
{
    sg_zoneGridPosWriter << GetEffect() << GetID() << GetName();
    sg_zoneGridPosWriter << GetRadius() << GetExpansionSpeed();
    sg_zoneGridPosWriter << MapPosition().x << MapPosition().y << GetVelocity().x << GetVelocity().y;
    sg_zoneGridPosWriter << color_.r << color_.g << color_.b;

    sg_zoneGridPosWriter.write();
}

static void sg_writeZoneGridposNow(std::istream &s,bool byId)
{
    if(!sg_zoneGridPosWriter.isEnabled())
    {
        con << "This won't do anything until you turn on LADDERLOG_WRITE_ZONE_GRIDPOS.\nTIP: Set ZONE_GRIDPOS_INTERVAL to -1 to avoid automatic output.\n";
    }
    
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }
    
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }
    
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id != -1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->WriteLadderLog();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId,zone_id);
        else
            zone_id=gZone::FindNext(object_id_str,zone_id);
    }
}
static void sg_writeZoneGridposNow(std::istream &s) { sg_writeZoneGridposNow(s,false); }
static tConfItemFunc sg_writeZoneGridposNowConf("LOG_ZONE_GRIDPOS", &sg_writeZoneGridposNow);

static void sg_writeZoneGridposNowId(std::istream &s) { sg_writeZoneGridposNow(s,true); }
static tConfItemFunc sg_writeZoneGridposNowIdConf("LOG_ZONE_GRIDPOS_ID", &sg_writeZoneGridposNowId);

void gZone::GridPosLadderLog()
{
    if (!sg_zoneGridPosWriter.isEnabled())
        return;

    for(std::deque<gZone *>::const_iterator i = sg_Zones.begin(); i != sg_Zones.end(); ++i)
    {
        gZone *zone = *i;
        if (zone && !zone->destroyed_)
        {
            zone->WriteLadderLog();
        }
    }
}


static eLadderLogWriter sg_collapsezoneWriter("ZONE_COLLAPSED", false);

static void sg_CollapseZone(std::istream &s,bool byId,bool destroyZone)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id != -1)
    {
        // get the zone ...
        gZone *zone = dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            sg_collapsezoneWriter << zone_id << object_id_str << zone->MapPosition().x << zone->MapPosition().y;
            sg_collapsezoneWriter.write();

            if(destroyZone)
                zone->Collapse();
            else
                zone->Vanish(0.5);
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_CollapseZone(std::istream &s) { sg_CollapseZone(s,false,false); }
static tConfItemFunc sg_CollapseZone_conf("COLLAPSE_ZONE",&sg_CollapseZone);

static void sg_CollapseZoneId(std::istream &s) { sg_CollapseZone(s,true,false); }
static tConfItemFunc sg_CollapseZoneId_conf("COLLAPSE_ZONE_ID",&sg_CollapseZoneId);

static void sg_DestroyZone(std::istream &s) { sg_CollapseZone(s,false,true); }
static tConfItemFunc sg_DestroyZone_conf("DESTROY_ZONE",&sg_DestroyZone);

static void sg_DestroyZoneId(std::istream &s) { sg_CollapseZone(s,true,true); }
static tConfItemFunc sg_DestroyZoneId_conf("DESTROY_ZONE_ID",&sg_DestroyZoneId);

static void sg_CollapseAll(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for(int i = 0; i < gameObjects.Len(); i++)
    {
        gZone *Zone = dynamic_cast<gZone *>(gameObjects[i]);
        if (Zone)
        {
            Zone->Vanish(0.5);
            Zone->RequestSync();
        }
    }
}
static tConfItemFunc sg_CollapseAllConf("COLLAPSE_ALL", &sg_CollapseAll);

static void sg_DestroyAll(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid) return;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for(int i = 0; i < gameObjects.Len(); i++)
    {
        gZone *Zone = dynamic_cast<gZone *>(gameObjects[i]);
        if (Zone)
        {
            Zone->Collapse();
            Zone->RequestSync();
        }
    }
}
static tConfItemFunc sg_DestroyAllConf("DESTROY_ALL", &sg_DestroyAll);

static void sg_SetZoneRadius(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, radius, speed
    
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    tString radius_str; s >> radius_str;
    REAL radius = atof(radius_str);
    
    tString speed_str; s >> speed_str;
    REAL speed = atof(speed_str);

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id != -1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            // set new radius and speed to reach it ...
            if (speed==0)
                zone->SetRadiusSmoothly( radius*gArena::SizeMultiplier() );
            else if(speed < 0)
                zone->SetRadius(radius*gArena::SizeMultiplier());
            else
                zone->SetRadiusSmoothly( radius*gArena::SizeMultiplier(), speed *gArena::SizeMultiplier());
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId,zone_id);
        else
            zone_id=gZone::FindNext(object_id_str,zone_id);
    }
}

static void sg_SetZoneRadius(std::istream &s) { sg_SetZoneRadius(s,false); }
static tConfItemFunc sg_SetZoneRadius_conf("SET_ZONE_RADIUS",&sg_SetZoneRadius);

static void sg_SetZoneIdRadius(std::istream &s) { sg_SetZoneRadius(s,true); }
static tConfItemFunc sg_SetZoneIdRadius_conf("SET_ZONE_ID_RADIUS",&sg_SetZoneIdRadius);

static void sg_SetZoneCoord(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }
    
    tString obj_id;
    s >> obj_id;
    int objIntId = atoi(obj_id);
    
    REAL posX, posY;
    s >> posX; posX *= gArena::SizeMultiplier();
    s >> posY; posY *= gArena::SizeMultiplier();
    eCoord pos(posX,posY);
    
    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(obj_id);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while(zone_id != -1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if(zone)
        {
            zone->SetPosition(pos);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId,zone_id);
        else
            zone_id=gZone::FindNext(obj_id,zone_id);
    }
}

static void sg_SetZoneCoord(std::istream &s) { sg_SetZoneCoord(s,false); }
static tConfItemFunc sg_SetZoneCoord_conf("SET_ZONE_COORD",&sg_SetZoneCoord);

static void sg_SetZoneIdCoord(std::istream &s) { sg_SetZoneCoord(s,true); }
static tConfItemFunc sg_SetZoneIdCoord_conf("SET_ZONE_ID_COORD",&sg_SetZoneIdCoord);

static void sg_SetZonePosition(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    tString params;
    params.ReadLine( s, true );

    // parse the line to get the param : object_id, positions ...
    int pos = 0;                 //
    const tString object_id_str = params.ExtractNonBlankSubString(pos);
    //        const tString mode_str = params.ExtractNonBlankSubString(pos);
    std::vector<eCoord> route;
/*  tString speedstr;
    speedstr = params.ExtractNonBlankSubString(pos);
    REAL speed = atof(speedstr);*/
    tString x,y;
    float sizeMultiplier=gArena::SizeMultiplier();
    while(true)
    {
        x = params.ExtractNonBlankSubString(pos);
        if(x == "") break;
        y = params.ExtractNonBlankSubString(pos);
        route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
    }

    // first check for the name
    int zone_id = -1;
    zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = atoi(object_id_str);
        if (zone_id < 0) return;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            eCoord zoneDir = eCoord(0,0);
            if(!route.empty())
            {
                eCoord curPos = zone->GetPosition();
                zone->AddWaypoint(curPos);
                zoneDir = route.front();
                for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
                {
                    zone->AddWaypoint(*iter + curPos);
                }
            }
/*          REAL magnitude = zoneDir.Norm();
            if (speed<=0) speed = magnitude;
            if (magnitude!=0.0) zoneDir.Normalize();
            zoneDir*=speed;*/
            zone->SetVelocity(zoneDir);
            zone->RequestSync();
        }
        zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static tConfItemFunc sg_SetZonePosition_conf("SET_ZONE_POSITION",&sg_SetZonePosition);

static void sg_AddZoneRoute(std::istream &s,bool byId,bool clear)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, path, speed ...
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    std::vector<eCoord> route;
    tString x,y;
    REAL sizeMultiplier=gArena::SizeMultiplier();
    REAL newSpeed = 100*sizeMultiplier;
    bool speedovr = false;
    while(true)
    {
        s >> x;
        if(x == "") break;
        s >> y;
        if(y == "")
        {
            newSpeed = atof(x)*sizeMultiplier;
            speedovr = true;
            break;
        }
        route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
    }

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while(zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if(zone)
        {
            zone->SetReferenceTime();
            if(clear)
                zone->ClearWaypoints();
            eCoord dir = zone->GetVelocity();
            REAL speed = dir.Norm();
            if(!route.empty())
            {
                if(clear || zone->HasNoWaypoints())
                {
                    if(route.size() == 1)
                    {
                        eCoord curPos = zone->GetPosition();
                        zone->AddWaypoint(curPos);
                        
                        dir = route.front() - curPos;
                        if(dir.Norm() != 0)
                        {
                            dir.Normalize();
                            if(speedovr || speed == 0)
                                dir *= newSpeed;
                            else
                                dir *= speed;
                        }
                    }
                    else
                    {
                        zone->SetPosition(route.front());
                    }
                }
                if(route.size() != 1 && (speed == 0 || speedovr))
                {
                    if(speed == 0)
                        dir = route.at(1) - zone->GetPosition();
                    if(dir.Norm() != 0)
                        dir.Normalize();
                    if(speedovr || speed == 0)
                        dir *= newSpeed;
                    else
                        dir *= speed;
                }
                
                for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
                {
                    zone->AddWaypoint(*iter);
                    if(clear && route.size() == 1)
                        zone->AddWaypoint(*iter);
                }
            }
            zone->SetVelocity(dir);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_AddZoneRoute(std::istream &s) { sg_AddZoneRoute(s,false,false); }
static tConfItemFunc sg_AddZoneRoute_conf("ADD_ZONE_ROUTE",&sg_AddZoneRoute);

static void sg_AddZoneIdRoute(std::istream &s) { sg_AddZoneRoute(s,true,false); }
static tConfItemFunc sg_AddZoneIdRoute_conf("ADD_ZONE_ID_ROUTE",&sg_AddZoneIdRoute);

static void sg_SetZoneRoute(std::istream &s) { sg_AddZoneRoute(s,false,true); }
static tConfItemFunc sg_SetZoneRoute_conf("SET_ZONE_ROUTE",&sg_SetZoneRoute);

static void sg_SetZoneIdRoute(std::istream &s) { sg_AddZoneRoute(s,true,true); }
static tConfItemFunc sg_SetZoneIdRoute_conf("SET_ZONE_ID_ROUTE",&sg_SetZoneIdRoute);

static void sg_SetZoneSpeed(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, speed ...
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);

    tString speedstr; s >> speedstr;
    REAL speed = atof(speedstr)*gArena::SizeMultiplier();

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            eCoord zoneDir = zone->GetVelocity();
            REAL magnitude = zoneDir.Norm();
            //if (speed<=0) speed = magnitude;
            if (magnitude!=0.0) zoneDir.Normalize();
            zoneDir*=speed;
            zone->SetVelocity(zoneDir);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZoneSpeed(std::istream &s) { sg_SetZoneSpeed(s,false); }
static tConfItemFunc sg_SetZoneSpeed_conf("SET_ZONE_SPEED",&sg_SetZoneSpeed);

static void sg_SetZoneIdSpeed(std::istream &s) { sg_SetZoneSpeed(s,true); }
static tConfItemFunc sg_SetZoneIdSpeed_conf("SET_ZONE_ID_SPEED",&sg_SetZoneIdSpeed);

static void sg_SetZoneDir(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, xdir, ydir ...
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);

    REAL dirX, dirY;
    s >> dirX; dirX *= gArena::SizeMultiplier();
    s >> dirY; dirY *= gArena::SizeMultiplier();
    eCoord dir(dirX,dirY);

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while(zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            zone->SetVelocity(dir);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZoneDir(std::istream &s) { sg_SetZoneDir(s,false); }
static tConfItemFunc sg_SetZoneDir_conf("SET_ZONE_DIR",&sg_SetZoneDir);

static void sg_SetZoneIdDir(std::istream &s) { sg_SetZoneDir(s,true); }
static tConfItemFunc sg_SetZoneIdDir_conf("SET_ZONE_ID_DIR",&sg_SetZoneIdDir);

static void sg_SetZoneColor(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, r, g, b ...
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    tString zoneRedStr,zoneGreenStr,zoneBlueStr;
    s >> zoneRedStr; s >> zoneGreenStr; s >> zoneBlueStr;
    
    gRealColor zoneColor;
    if ((zoneRedStr=="")||(zoneGreenStr=="")||(zoneBlueStr=="")) return;
    zoneColor.r = atof(zoneRedStr);
    zoneColor.g = atof(zoneGreenStr);
    zoneColor.b = atof(zoneBlueStr);

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            zoneColor.r = (zoneColor.r>1.0)?1.0:zoneColor.r;
            zoneColor.g = (zoneColor.g>1.0)?1.0:zoneColor.g;
            zoneColor.b = (zoneColor.b>1.0)?1.0:zoneColor.b;
            zone->SetColor(zoneColor);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZoneColor(std::istream &s) { sg_SetZoneColor(s,false); }
static tConfItemFunc sg_SetZoneColor_conf("SET_ZONE_COLOR",&sg_SetZoneColor);

static void sg_SetZoneIdColor(std::istream &s) { sg_SetZoneColor(s,true); }
static tConfItemFunc sg_SetZoneIdColor_conf("SET_ZONE_ID_COLOR",&sg_SetZoneIdColor);

static void sg_SetZoneExpansion(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }
    
    // parse the line to get the param : object_id, expansion
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    tString expansion_str; s >> expansion_str;
    REAL expansion = atof(expansion_str)*gArena::SizeMultiplier();

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = atoi(object_id_str);
        if (zone_id < 0) return;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetReferenceTime();
            zone->SetExpansionSpeed( expansion );
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZoneExpansion(std::istream &s) { sg_SetZoneExpansion(s,false); }
static tConfItemFunc sg_SetZoneExpansion_conf("SET_ZONE_EXPANSION",&sg_SetZoneExpansion);

static void sg_SetZoneIdExpansion(std::istream &s) { sg_SetZoneExpansion(s,true); }
static tConfItemFunc sg_SetZoneIdExpansion_conf("SET_ZONE_ID_EXPANSION",&sg_SetZoneIdExpansion);

static void sg_SetZoneRotation(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, expansion
    int pos = 0;                 //
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    tString rotation_str; s >> rotation_str;
    REAL rotation = atof(rotation_str);

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id!=-1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetRotationSpeed(rotation);
            zone->RequestSync();
        }
        if(byId)
            zone_id=gZone::FindIdNext(objIntId, zone_id);
        else
            zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZoneRotation(std::istream &s) { sg_SetZoneRotation(s,false); }
static tConfItemFunc sg_SetZoneRotationConf("SET_ZONE_ROTATION", &sg_SetZoneRotation);

static void sg_SetZoneIdRotation(std::istream &s) { sg_SetZoneRotation(s,true); }
static tConfItemFunc sg_SetZoneIdRotationConf("SET_ZONE_ID_ROTATION", &sg_SetZoneIdRotation);


static void sg_SetZonePenetrate(std::istream &s,bool byId)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    // parse the line to get the param : object_id, expansion
    tString object_id_str; s >> object_id_str;
    int objIntId = atoi(object_id_str);
    
    tString penetrate_str; s >> penetrate_str;
    int penetrate_int = atoi(penetrate_str);
    bool penetrate = false;
    if (penetrate_int > 0)
        penetrate = true;

    // first check for the name
    int zone_id = -1;
    if(byId)
        zone_id = gZone::FindIdFirst(objIntId);
    else
        zone_id = gZone::FindFirst(object_id_str);
    if (zone_id <= -1)
    {
        zone_id = gZone::FindIdFirst(objIntId);
        if(zone_id < 0)
        {
            con << "Couldn't find the zone specified.\n";
            return;
        }
        byId = true;
    }

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id >= gameObjects.Len()) return;

    while (zone_id != -1)
    {
        // get the zone ...
        gZone *zone=dynamic_cast<gZone *>(gameObjects(zone_id));
        if (zone)
        {
            zone->SetWallPenetrate(penetrate);
            zone->RequestSync();
        }
        zone_id=gZone::FindNext(object_id_str, zone_id);
    }
}
static void sg_SetZonePenetrate(std::istream &s) { sg_SetZonePenetrate(s,false); }
static tConfItemFunc sg_SetZonePenetrateConf("SET_ZONE_PENETRATE", &sg_SetZonePenetrate);

static void sg_SetZoneIdPenetrate(std::istream &s) { sg_SetZonePenetrate(s,true); }
static tConfItemFunc sg_SetZoneIdPenetrateConf("SET_ZONE_ID_PENETRATE", &sg_SetZoneIdPenetrate);


static void sg_SetZoneIdPosition(std::istream &s)
{
    eGrid *grid = eGrid::CurrentGrid();
    if (!grid)
    {
        con << "Must be called while a grid exists!\n";
        return;
    }

    tString params;
    params.ReadLine( s, true );

    // parse the line to get the param : object_id, positions ...
    int pos = 0;                 //
    const tString object_id_str = params.ExtractNonBlankSubString(pos);
    //        const tString mode_str = params.ExtractNonBlankSubString(pos);
    std::vector<eCoord> route;
/*  tString speedstr;
    speedstr = params.ExtractNonBlankSubString(pos);
    REAL speed = atof(speedstr);*/
    tString x,y;
    float sizeMultiplier=gArena::SizeMultiplier();
    while(true)
    {
        x = params.ExtractNonBlankSubString(pos);
        if(x == "") break;
        y = params.ExtractNonBlankSubString(pos);
        route.push_back(eCoord(atof(x)*sizeMultiplier, atof(y)*sizeMultiplier));
    }

    // first check for the name
    int zone_id = -1;
    zone_id = atoi(object_id_str);
    if (zone_id < 0) return;

    const tList<eGameObject>& gameObjects = grid->GameObjects();
    if (zone_id > gameObjects.Len()) return;

    // get the zone ...
    for(int i = 0; i < gameObjects.Len(); i++)
    {
        gZone *Zone = dynamic_cast<gZone *>(gameObjects[i]);
        if (Zone)
        {
            if (Zone->GetID() == zone_id)
            {
                Zone->SetReferenceTime();
                eCoord zoneDir = eCoord(0,0);
                if(!route.empty())
                {
                    eCoord curPos = Zone->GetPosition();
                    Zone->AddWaypoint(curPos);
                    zoneDir = route.front();
                    for(std::vector<eCoord>::const_iterator iter = route.begin(); iter != route.end(); ++iter)
                    {
                        Zone->AddWaypoint(*iter + curPos);
                    }
                }
                /*REAL magnitude = zoneDir.Norm();
                if (speed<=0) speed = magnitude;
                if (magnitude!=0.0) zoneDir.Normalize();
                zoneDir*=speed;*/
                Zone->SetVelocity(zoneDir);
                Zone->RequestSync();

                break;
            }
        }
    }
}

static tConfItemFunc sg_SetZoneIdPosition_conf("SET_ZONE_ID_POSITION",&sg_SetZoneIdPosition);
