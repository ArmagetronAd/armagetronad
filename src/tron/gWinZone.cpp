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
#include "eSoundMixer.h"


#include <time.h>
#include <algorithm>
#include <functional>
#include <deque>
#include <iterator>


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
    /*
    if ( sg_zoneDeath )
    {
      std::vector <zEffectGroup> asdf;
      zEffectGroup iii(std::vector<ePlayerNetID *>(), std::vector<eTeam *>(), zEffectGroup::EffectDeath, zEffectGroup::UserAll, zEffectGroup::TargetSelf);
      asdf.push_back(iii);
      ret = tNEW( gZone) ( grid, pos, asdf);

        sn_ConsoleOut( "$instant_death_activated" );
    }
    else
    {
      std::vector <zEffectGroup> asdf;
      zEffectGroup iii(std::vector<ePlayerNetID *>(), std::vector<eTeam *>(), zEffectGroup::EffectWin, zEffectGroup::UserAll, zEffectGroup::TargetSelf);
      asdf.push_back(iii);
      ret = tNEW( gZone) ( grid, pos, asdf);

        sn_ConsoleOut( "$instant_win_activated" );
    }

    // initialize radius and expansion speed
    static_cast<eGameObject*>(ret)->Timestep( se_GameTime() );
    ret->SetReferenceTime();
    ret->SetRadius( sg_initialSize );
    ret->SetExpansionSpeed( sg_expansionSpeed );
    ret->SetRotationSpeed( .3f );
    */
    return ret;
}

static int score_deathzone=-1;
static tSettingItem<int> s_dz("SCORE_DEATHZONE",score_deathzone);

/*

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
            color_.r_ = team->R()/15.0;
            color_.g_ = team->G()/15.0;
            color_.b_ = team->B()/15.0;
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

*/


