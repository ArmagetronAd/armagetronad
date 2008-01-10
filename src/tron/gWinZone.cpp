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

#include "gWinZone.h"
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

#include <time.h>
#include <algorithm>

static int sg_zoneDeath = 1;
static tSettingItem<int> sg_zoneDeathConf( "WIN_ZONE_DEATHS", sg_zoneDeath );

static REAL sg_expansionSpeed = 1.0f;
static REAL sg_initialSize = 5.0f;

static nSettingItem< REAL > sg_expansionSpeedConf( "WIN_ZONE_EXPANSION", sg_expansionSpeed );
static nSettingItem< REAL > sg_initialSizeConf( "WIN_ZONE_INITIAL_SIZE", sg_initialSize );

//! creates a win or death zone (according to configuration) at the specified position
gZone * sg_CreateWinDeathZone( eGrid * grid, const eCoord & pos )
{
    gZone * ret = NULL;
    if ( sg_zoneDeath )
    {
        ret = tNEW( gDeathZoneHack( grid, pos ) );
        sn_ConsoleOut( "$instant_death_activated" );
    }
    else
    {
        ret = tNEW( gWinZoneHack( grid, pos ) );
        sn_ConsoleOut( "$instant_win_activated" );
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
// *	gZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gZone::gZone( eGrid * grid, const eCoord & pos )
        :eNetGameObject( grid, pos, eCoord( 0,0 ), NULL, true ), rotation_(1,0)
{
    // store creation time
    referenceTime_ = createTime_ = lastTime = 0;

    // add to game grid
    this->AddToList();

    // initialize position functions
    SetPosition( pos );
}

// *******************************************************************************
// *
// *	gZone
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!
// *******************************************************************************

gZone::gZone( nMessage & m )
        :eNetGameObject( m ), rotation_(1,0)
{
    // read creation time
    m >> createTime_;
    referenceTime_ = lastTime = createTime_;

    // initialize color to white, ReadSync will fill in the true value if available
    color_.r = color_.g = color_.b = 1.0f;

    // add to game grid
    this->AddToList();

    // initialize position functions
    SetPosition( pos );
}

// *******************************************************************************
// *
// *	~gZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gZone::~gZone( void )
{
}

// *******************************************************************************
// *
// *	WriteCreate
// *
// *******************************************************************************
//!
//!		@param	m Message to write creation data to
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
// *	WriteSync
// *
// *******************************************************************************
//!
//!		@param	m Message to write sync data to
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
// *	ReadSync
// *
// *******************************************************************************
//!
//!		@param	m Message to read sync data from
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
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool gZone::Timestep( REAL time )
{
    // rotate
    REAL speed = GetRotationSpeed();
    REAL angle = ( time - lastTime ) * speed;
    // angle /= ( 1 + 2 * 3.14159 * angle/sg_segments );
    rotation_ = rotation_.Turn( cos( angle ), sin( angle ) );

    // move to new position
    REAL dt = time - referenceTime_;
    Move( eCoord( posx_( dt ), posy_( dt ) ), lastTime, time );

    // update time
    lastTime = time;

    // kill this zone if it shrunk down to zero radius
    if ( GetExpansionSpeed() < 0 && GetRadius() <= 0 )
    {
        OnVanish();
        return true;
    }

    return false;
}

// *******************************************************************************
// *
// *	OnVanish
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
// *	InteractWith
// *
// *******************************************************************************
//!
//!		@param	target        the other game object
//!		@param	time          the current time
//!		@param	recursion     if set to true, don't recurse into other InteractWith functions (quite silly now that I think about it...)
//!
// *******************************************************************************

void gZone::InteractWith( eGameObject * target, REAL time, int recursion )
{
    gCycle* prey = dynamic_cast< gCycle* >( target );
    if ( prey )
    {
        REAL r = this->Radius();
        if ( ( prey->Position() - this->Position() ).NormSquared() < r*r )
        {
            if ( prey->Player() && prey->Alive() )
            {
                OnEnter( prey, time );
            }
        }
    }
}

// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gZone::OnEnter( gCycle * target, REAL time )
{
}

// the zone's network initializator
static nNOInitialisator<gZone> zone_init(340,"zone");

// *******************************************************************************
// *
// *	CreatorDescriptor
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nDescriptor & gZone::CreatorDescriptor( void ) const
{
    return zone_init;
}

// *******************************************************************************
// *
// *	Radius
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

REAL gZone::Radius( void ) const
{
    return GetRadius();
}

// *******************************************************************************
// *
// *	Render
// *
// *******************************************************************************
//!
//!		@param	cam  the camera used for rendering
//!
// *******************************************************************************

void gZone::Render( const eCamera * cam )
{
#ifndef DEDICATED

    REAL alpha = ( lastTime - createTime_ ) * .2f;
    if ( alpha > .7f )
        alpha = .7f;
    if ( alpha <= 0 )
        return;

    GLfloat m[4][4]={{rotation_.x,rotation_.y,0,0},
                     {-rotation_.y,rotation_.x,0,0},
                     {0,0,1,0},
                     {pos.x,pos.y,0,1}};

    ModelMatrix();
    glPushMatrix();

    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    //glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);

    //	glTranslatef(pos.x,pos.y,0);

    glMultMatrixf(&m[0][0]);
    //	glScalef(.5,.5,.5);

    if ( sr_alphaBlend )
        BeginQuads();
    else
        BeginLineStrip();

    const REAL seglen = .2f;
    const REAL bot = 0.0f;
    const REAL top = 5.0f; // + ( lastTime - createTime_ ) * .1f;

    glColor4f( color_.r ,color_.g,color_.b, alpha );

    REAL r = Radius();
    for ( int i = sg_segments - 1; i>=0; --i )
    {
        REAL a = i * 2 * 3.14159 / REAL( sg_segments );
        REAL b = a + seglen;

        REAL sa = r * sin(a);
        REAL ca = r * cos(a);
        REAL sb = r * sin(b);
        REAL cb = r * cos(b);

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

    RenderEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask(GL_TRUE);

    glPopMatrix();
#endif
}

// *******************************************************************************
// *
// *	gWinZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gWinZoneHack::gWinZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos )
{
    color_.r = 0.0f;
    color_.g = 1.0f;
    color_.b = 0.0f;
}

// *******************************************************************************
// *
// *	gWinZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gWinZoneHack::gWinZoneHack( nMessage & m )
        : gZone( m )
{
}

// *******************************************************************************
// *
// *	~gWinZoneHack
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
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gWinZoneHack::OnEnter( gCycle * target, REAL time )
{
    static const char* message="$player_win_instant";
    sg_DeclareWinner( target->Player()->CurrentTeam(), message );

    // let zone vanish
    if ( GetExpansionSpeed() >= 0 )
    {
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius()*.5 );
        RequestSync();
    }
}

// *******************************************************************************
// *
// *	gDeathZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gDeathZoneHack::gDeathZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos )
{
    color_.r = 1.0f;
    color_.g = 0.0f;
    color_.b = 0.0f;
}

// *******************************************************************************
// *
// *	gDeathZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
//!		@param	null
//!
// *******************************************************************************

gDeathZoneHack::gDeathZoneHack( nMessage & m )
        : gZone( m )
{
}

// *******************************************************************************
// *
// *	~gDeathZoneHack
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

gDeathZoneHack::~gDeathZoneHack( void )
{
}

static int score_deathzone=-1;
static tSettingItem<int> s_dz("SCORE_DEATHZONE",score_deathzone);

// *******************************************************************************
// *
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void gDeathZoneHack::OnEnter( gCycle * target, REAL time )
{
    target->Player()->AddScore(score_deathzone, tOutput(), "$player_lose_suicide");
    target->Kill();
}

// *******************************************************************************
// *
// *	gBaseZoneHack
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!		@param	pos	 Position to spawn the zone at
//!
// *******************************************************************************

gBaseZoneHack::gBaseZoneHack( eGrid * grid, const eCoord & pos )
        :gZone( grid, pos), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
}

// *******************************************************************************
// *
// *	gBaseZoneHack
// *
// *******************************************************************************
//!
//!		@param	m Message to read creation data from
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
}

// *******************************************************************************
// *
// *	~gBaseZoneHack
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
static int sg_onConquestWin = 1;
static tSettingItem< int > sg_onConquestConquestWinConfig( "FORTRESS_CONQUERED_WIN", sg_onConquestWin );

// maximal number of base zones ownable by a team
static int sg_baseZonesPerTeam = 0;
static tSettingItem< int > sg_baseZonesPerTeamConfig( "FORTRESS_MAX_PER_TEAM", sg_baseZonesPerTeam );

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


// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool gBaseZoneHack::Timestep( REAL time )
{
    if ( currentState_ == State_Conquering )
    {
        // let zone vanish
        SetReferenceTime();
        SetExpansionSpeed( -GetRadius()*.5 );
        SetRotationAcceleration( -GetRotationSpeed()*.4 );
        RequestSync();

        currentState_ = State_Conquered;
    }

    REAL dt = time - lastTime;

    // conquest going on
    REAL conquest = sg_conquestRate * enemiesInside_ - sg_defendRate * ownersInside_ - sg_conquestDecayRate;
    conquered_ += dt * conquest;

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
        REAL maxSpeed = 10 * ( 2 * 3.141 ) / sg_segments;
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
                    sn_ConsoleOut( tOutput( "$zone_collapse_harmless", team->Name()  ) );
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


    // reset counts
    enemiesInside_ = ownersInside_ = 0;

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
                    if ( sg_baseZonesPerTeam == 0 || count < sg_baseZonesPerTeam || farthest->teamDistance_ > distance )
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

        // if this zone does not belong to a team, discard it.
        if ( !team )
        {
            return true;
        }

        // check other zones owned by the same team. Discard the one farthest away
        // if the max count is exceeded
        if ( team && sg_baseZonesPerTeam > 0 )
        {
            gBaseZoneHack * farthest = 0;
            int count = 0;
            CountZonesOfTeam( Grid(), team, count, farthest );

            // discard team of farthest zone
            if ( count > sg_baseZonesPerTeam )
                farthest->team = NULL;
        }
    }


    // delegate
    bool ret = gZone::Timestep( time );

    // reward survival
    if ( !ret && onlySurvivor_ )
    {
        const char* message= ( eTeam::teams.Len() > 2 || sg_onConquestScore ) ? "$player_win_survive" : "$player_win_conquest";
        sg_DeclareWinner( team, message );
    }

    return ret;
}

// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::OnVanish( void )
{
    if (!team)
        return;

    CheckSurvivor();

    // kill the closest owners of the zone
    if ( currentState_ != State_Safe && ( enemies_.size() > 0 || sg_defendRate < 0 ) )
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
                sn_ConsoleOut( tOutput("$player_kill_collapse", closest->GetName() ) );
                closest->Object()->Kill();
            }
        }
    }
}

// *******************************************************************************
// *
// *	OnConquest
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void gBaseZoneHack::OnConquest( void )
{
    tString log;
    log << "BASEZONE_CONQUERED " << ePlayerNetID::FilterName(team->Name()) << " " << GetPosition().x << " " << GetPosition().y << '\n';
    se_SaveToLadderLog(log);
    float rr = GetRadius();
    rr *= rr;
    for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i) {
        ePlayerNetID *player = se_PlayerNetIDs(i);
        if(!player) {
            continue;
        }
        gCycle *cycle = dynamic_cast<gCycle *>(player->Object());
        if(!cycle) {
            continue;
        }
        if(cycle->Alive() && (cycle->Position() - Position()).NormSquared() < rr) {
            tString log;
            log << "BASEZONE_CONQUERER " << player->GetUserName() << '\n';
            se_SaveToLadderLog(log);
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
            win.SetTemplateParameter( 3, team->Name() );
            win << "$player_win_conquest_specific";
        }
        else
        {
            win << "$player_win_conquest";
        }

        int score = totalScore / enemies_.size();
        for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
        {
            (*iter)->AddScore( score, win, tOutput() );
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
static int sg_onSurviveWin = 1;
static tSettingItem< int > sg_onSurviveWinConfig( "FORTRESS_SURVIVE_WIN", sg_onSurviveWin );

// *******************************************************************************
// *
// *	CheckSurvivor
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
        for (int i=gameObjects.Len()-1;i>=0 && onlySurvivor;i--){
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
// *	OnEnter
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
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
        ++ ownersInside_;
    }
    else if ( team )
    {
        if ( enemiesInside_ == 0 )
            enemies_.clear();

        ++ enemiesInside_;
        if ( std::find( enemies_.begin(), enemies_.end(), otherTeam ) == enemies_.end() )
            enemies_.push_back( otherTeam );

        lastEnemyContact_ = time;
    }
}

// *******************************************************************************
// *
// *	tFunction
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
// *	~tFunction
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
// *	Evaluate
// *
// *******************************************************************************
//!
//!		@param	argument
//!		@return
//!
// *******************************************************************************

REAL tFunction::Evaluate( REAL argument ) const
{
    return offset_ + slope_ * argument;
}

// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	m	message to write to
//!		@param	f	function to write
//!		@return		reference to message for chaining
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
// *	operator >>
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
// *	GetPosition
// *
// *******************************************************************************
//!
//!		@return		the current position
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
// *	GetPosition
// *
// *******************************************************************************
//!
//!		@param	position	the current position to fill
//!		@return		A reference to this to allow chaining
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
// *	SetPosition
// *
// *******************************************************************************
//!
//!		@param	position	the current position to set
//!		@return		A reference to this to allow chaining
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
// *	GetVelocity
// *
// *******************************************************************************
//!
//!		@return		the current velocity
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
// *	GetVelocity
// *
// *******************************************************************************
//!
//!		@param	velocity	the current velocity to fill
//!		@return		A reference to this to allow chaining
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
// *	SetVelocity
// *
// *******************************************************************************
//!
//!		@param	velocity	the current velocity to set
//!		@return		A reference to this to allow chaining
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
// *	GetRadius
// *
// *******************************************************************************
//!
//!		@return		the current radius
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
// *	GetRadius
// *
// *******************************************************************************
//!
//!		@param	radius	the current radius to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRadius( REAL & radius ) const
{
    radius = GetRadius();

    return *this;
}

// *******************************************************************************
// *
// *	SetRadius
// *
// *******************************************************************************
//!
//!		@param	radius	the current radius to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRadius( REAL radius )
{
    SetFunctionNow( this->radius_, radius );

    return *this;
}

// *******************************************************************************
// *
// *	GetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@return		the current expansion speed
//!
// *******************************************************************************

REAL gZone::GetExpansionSpeed( void ) const
{
    return this->radius_.GetSlope();
}

// *******************************************************************************
// *
// *	GetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@param	expansionSpeed	the current expansion speed to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetExpansionSpeed( REAL & expansionSpeed ) const
{
    expansionSpeed = this->radius_.GetSlope();

    return *this;
}

// *******************************************************************************
// *
// *	SetExpansionSpeed
// *
// *******************************************************************************
//!
//!		@param	expansionSpeed	the current expansion speed to set
//!		@return		A reference to this to allow chaining
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
// *	SetReferenceTime
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
// *	GetRotationSpeed
// *
// *******************************************************************************
//!
//!		@return		The current rotation speed
//!
// *******************************************************************************

REAL gZone::GetRotationSpeed( void ) const
{
    return EvaluateFunctionNow( rotationSpeed_ );
}

// *******************************************************************************
// *
// *	GetRotationSpeed
// *
// *******************************************************************************
//!
//!		@param	rotationSpeed	The current rotation speed to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationSpeed( REAL & rotationSpeed ) const
{
    rotationSpeed = this->GetRotationSpeed();
    return *this;
}

// *******************************************************************************
// *
// *	SetRotationSpeed
// *
// *******************************************************************************
//!
//!		@param	rotationSpeed	The current rotation speed to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationSpeed( REAL rotationSpeed )
{
    SetFunctionNow( this->rotationSpeed_, rotationSpeed );
    return *this;
}

// *******************************************************************************
// *
// *	GetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@return		the current acceleration of the rotation
//!
// *******************************************************************************

REAL gZone::GetRotationAcceleration( void ) const
{
    return this->rotationSpeed_.GetSlope();
}

// *******************************************************************************
// *
// *	GetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@param	rotationAcceleration	the current acceleration of the rotation to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone const & gZone::GetRotationAcceleration( REAL & rotationAcceleration ) const
{
    rotationAcceleration = this->GetRotationAcceleration();
    return *this;
}

// *******************************************************************************
// *
// *	SetRotationAcceleration
// *
// *******************************************************************************
//!
//!		@param	rotationAcceleration	the current acceleration of the rotation to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

gZone & gZone::SetRotationAcceleration( REAL rotationAcceleration )
{
    REAL omega = this->GetRotationSpeed();
    this->rotationSpeed_.SetSlope( rotationAcceleration );
    SetRotationSpeed( omega );

    return *this;
}



