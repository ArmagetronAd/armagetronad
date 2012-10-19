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

#include "eFloor.h"
#include "eTimer.h"
#include "eGrid.h"
#include "gCycle.h"
#include "gGame.h"
#include "gParser.h"
#include "eTeam.h"
#include "ePlayer.h"
#include "rRender.h"
#include "nConfig.h"
#include "tString.h"
#include "tPolynomial.h"
#include "rScreen.h"
#include "eSoundMixer.h"

#include "zone/zFortress.h"
#include "zone/zZone.h"

#include <time.h>
#include <algorithm>
#include <functional>
#include <deque>


#define sg_segments sz_zoneSegments


// *******************************************************************************
// *
// *	zFortressZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!
// *******************************************************************************

zFortressZone::zFortressZone( eGrid * grid )
        :zZone( grid ), onlySurvivor_( false ), currentState_( State_Safe )
{
    enemiesInside_ = ownersInside_ = 0;
    conquered_ = 0;
    lastSync_ = -10;
    teamDistance_ = 0;
    lastEnemyContact_ = se_GameTime();
    touchy_ = false;
}

// *******************************************************************************
// *
// *	~zFortressZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

zFortressZone::~zFortressZone( void )
{
}

REAL sg_conquestRate = .5;
REAL sg_defendRate = .25;
REAL sg_conquestDecayRate = .1;

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

// Score factor if zone is conquered but no enemies inside the zone when it collapses.
static REAL sg_onConquestEmptyScoreFactor = .5;
static tSettingItem< REAL > sg_onConquestEmptyScoreFactorConfig( "FORTRESS_CONQUERED_EMPTY_SCORE_FACTOR", sg_onConquestEmptyScoreFactor );

// flag indicating whether the team conquering the first zone wins (good for one on one matches)
static int sg_onConquestWin = 1;
static tSettingItem< int > sg_onConquestConquestWinConfig( "FORTRESS_CONQUERED_WIN", sg_onConquestWin );

// maximal number of base zones ownable by a team
static int sg_baseZonesPerTeam = 0;
static tSettingItem< int > sg_baseZonesPerTeamConfig( "FORTRESS_MAX_PER_TEAM", sg_baseZonesPerTeam );

// flag indicating whether base will respawn team if a team player enters it
static bool sg_baseRespawn = false;
static tSettingItem<bool> sg_baseRespawnConfig ("BASE_RESPAWN", sg_baseRespawn);

// flag indicating whether base will respawn team if an enemy player enters it
static bool sg_baseEnemyRespawn = false;
static tSettingItem<bool> sg_baseEnemyRespawnConfig ("BASE_ENEMY_RESPAWN", sg_baseEnemyRespawn);

void zFortressZone::setupVisuals(gParser::State_t & state)
{
    REAL tpR[] = {.0f, .3f};
    tPolynomial tpr(tpR, 2);
    state.set("rotation", tpr);
}

void zFortressZone::readXML(tXmlParser::node const & node)
{
}

// count zones belonging to the given team.
// fill in count and the zone that is farthest to the team.
void zFortressZone::CountZonesOfTeam( eGrid const * grid, eTeam * otherTeam, int & count, zFortressZone * & farthest )
{
    count = 0;
    farthest = NULL;

    // check whether other zones are already registered to that team
    const tList<eGameObject>& gameObjects = grid->GameObjects();
    for (int j=gameObjects.Len()-1;j>=0;j--)
    {
        zFortressZone *otherZone=dynamic_cast<zFortressZone *>(gameObjects(j));

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
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	time    the current time
//!
// *******************************************************************************

bool zFortressZone::Timestep( REAL time )
{
    // no team?!? Get rid of this zone ASAP.
    if ( !team )
    {
        return true;
    }

    if ( currentState_ == State_Conquering )
    {
        if ( shape )
        {
            // let zone vanish
            shape->setReferenceTime(lastTime);

            // let it light up in agony
            if ( sg_collapseSpeed < .4 )
            {
                rColor color_ = shape->getColor();
                color_.r_ = color_.g_ = color_.b_ = 1;
                shape->setColor(color_);
            }

            shape->collapse( sg_collapseSpeed );
            shape->SetRotationAcceleration( -shape->GetRotationSpeed()*.4 );
            shape->RequestSync();
        }
        else
        {
            OnVanish();
        }

        currentState_ = State_Conquered;
    }
    else if ( currentState_ == State_Conquered && ( !shape || shape->GetRotationSpeed() < 0 ) )
    {
        if (shape)
        {
            // stop zone rotation
            shape->setReferenceTime(lastTime);
            shape->SetRotationSpeed( 0 );
            shape->SetRotationAcceleration( 0 );

            rColor color_ = shape->getColor();
            color_.r_ = color_.g_ = color_.b_ = .5;
            shape->setColor(color_);
            shape->RequestSync();
        }
        else
        {
            OnVanish();
        }

        // vanish the zone completely once it shrinks to zero size
        if( shape && shape->GetCurrentScale() < 0 )
        {
            OnVanish();
            return true;
        }
    }

    REAL dt = time - lastTime;

    // conquest going on
    REAL conquest = sg_conquestRate * enemiesInside_ - sg_defendRate * ownersInside_ - sg_conquestDecayRate;
//fprintf(stderr, "conq %f + (dt %f * (conqRate %f * badIn %d - defRate %f * OwnIn %d - decay %f))\n", conquered_,dt,sg_conquestRate,enemiesInside_,sg_defendRate,ownersInside_,sg_conquestDecayRate);
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
        REAL maxSpeed = 10 * ( 2 * M_PI ) / sg_segments;
        REAL omega = .3 + conquered_ * conquered_ * maxSpeed;
        REAL omegaDot = 2 * conquered_ * conquest * maxSpeed;

        // determine the time since the last sync (exaggerate for smoother motion in local games)
        REAL timeStep = lastTime - lastSync_;
        if ( sn_GetNetState() != nSERVER )
            timeStep *= 100;

        if ( sn_GetNetState() != nCLIENT &&
                shape &&
                ( ( fabs( omega - shape->GetRotationSpeed() ) + fabs( omegaDot - shape->GetRotationAcceleration() ) ) * timeStep > .5 ) )
        {
            shape->SetRotationSpeed( omega );
            shape->SetRotationAcceleration( omegaDot );
            shape->setReferenceTime(lastSync_);
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

    if (( ownersInside_ && sg_baseRespawn     )
     || (enemiesInside_ && sg_baseEnemyRespawn))
    {
        /*
        eGameObject * near = NULL;
        if (shape)
            near = shape;
        */

        for (int i = team->NumPlayers() - 1; i >= 0; --i)
        {
            ePlayerNetID *pPlayer = team->Player(i);
            eGameObject *pGameObject = pPlayer->Object();

            // FIXME: don't hardcode respawn delay
            if (!((!pGameObject) ||
                ((!pGameObject->Alive()) &&
                (pGameObject->DeathTime() < (time - 1)))))
                continue;

            sg_respawnWriter.write();
            sg_RespawnPlayer(grid, sg_GetArena(), pPlayer);

            // FIXME: only announce group respawns once, regardless of player count
            sg_respawnWriter << pPlayer->GetUserName()
                             << ePlayerNetID::FilterName(pPlayer->CurrentTeam()->Name());
            tColoredString playerName;
            playerName << *pPlayer << tColoredString::ColorString(1,1,1);
            if (ownersInside_ && sg_baseRespawn)
            {
                sg_respawnWriter << teamPlayerName_;
                tColoredString spawnerName;
                spawnerName << teamPlayerName_ << tColoredString::ColorString(1,1,1);
                sn_ConsoleOut( tOutput( "$player_base_respawn", playerName, spawnerName ) );
            }
            else
            {
                sg_respawnWriter << enemyPlayerName_;
                tColoredString spawnerName;
                spawnerName << enemyPlayerName_ << tColoredString::ColorString(1,1,1);
                sn_ConsoleOut( tOutput( "$player_base_enemy_respawn", playerName, spawnerName ) );
            }
            sg_respawnWriter.write();

            // send a console message to the player
            // FIXME: maybe move to sg_RespawnPlayer ?
            sn_CenterMessage(tOutput("$player_respawn_center_message"), pPlayer->Owner());
        }
    }

    // reset counts
    enemiesInside_ = ownersInside_ = 0;

    // delegate
    bool ret = zZone::Timestep( time );

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
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFortressZone::OnVanish( void )
{
    if (!team)
        return;

    CheckSurvivor();

    // kill the closest owners of the zone
    if ( currentState_ != State_Safe
        && ( enemies_.size() > 0 || sg_defendRate < 0 || sg_conquestTimeout < 0) )
    {
        int kills = int( sg_onConquestKillRatio * team->NumPlayers() );
        kills = kills > sg_onConquestKillMin ? kills : sg_onConquestKillMin;
        tCoord pos;
        if (shape)
            pos = shape->Position();
        // FIXME: What should we use for origin if there is no shape?

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
                tColoredString playerName;
                playerName = closest->GetColoredName();
                playerName << tColoredStringProxy(-1,-1,-1);
                sn_ConsoleOut( tOutput("$player_kill_collapse", playerName ) );
                closest->Object()->Kill();
            }
        }
    }
}

static bool sz_sortTeams(eTeam* a, eTeam* b)
{
    return a->GetLogName() < b->GetLogName();
}

// *******************************************************************************
// *
// *	OnConquest
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter sg_basezoneConqueredWriter("BASEZONE_CONQUERED", true);
static eLadderLogWriter sg_basezoneConquererWriter("BASEZONE_CONQUERER", true);

void zFortressZone::OnConquest( void )
{
    if ( team )
    {
        if (shape)
        {
            tCoord p = shape->Position();

            sg_basezoneConqueredWriter << ePlayerNetID::FilterName(team->Name()) << p.x << p.y;
        }
        else
            sg_basezoneConqueredWriter << ePlayerNetID::FilterName(team->Name());
        sg_basezoneConqueredWriter.write();
    }
    if (shape)
    {
        for(int i = se_PlayerNetIDs.Len()-1; i >=0; --i) {
            ePlayerNetID *player = se_PlayerNetIDs(i);
            if(!player) {
                continue;
            }
            if (shape->isInteracting(player->Object())) {
                sg_basezoneConquererWriter << player->GetUserName();
                sg_basezoneConquererWriter.write();
            }
        }
    }

    // calculate score. If nobody really was inside the zone any more, multiply by factor.
    // TODO: set message for when factor is less than 0; default is "lost X points for a very strange reason".
    int totalScore = sg_onConquestScore;
    if ( 0 == enemiesInside_ )
        totalScore *= sg_onConquestEmptyScoreFactor;

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

        int score = totalScore / (int)enemies_.size();
        if(enemies_.size() > 1)
        {
            sort(enemies_.begin(), enemies_.end(), sz_sortTeams);
            for ( TeamArray::iterator iter = enemies_.begin(); iter != enemies_.end(); ++iter )
            {
                    (*iter)->AddScore( score);
            }

            tOutput message;
            message.SetTemplateParameter(1, enemies_);
            message.SetTemplateParameter(2, abs(score));
            if( team )
            {
                message.SetTemplateParameter(3, team->GetColoredName());
                message << "$players_win_conquest_specific";
            }
            else
            {
                message << "$players_win_conquest";
            }

            sn_ConsoleOut(message);
            }
        else
        {
            (*enemies_.begin())->AddScore(score, win, tOutput());
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

void zFortressZone::CheckSurvivor( void )
{
    // test if there is only one team with non-conquered zones left
    if ( sg_onSurviveWin )
    {
        // find surviving team and test whether it is the only one
        zFortressZone * survivor = 0;
        bool onlySurvivor = true;

        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        for (int i=gameObjects.Len()-1;i>=0 && onlySurvivor;i--){
            zFortressZone *other=dynamic_cast<zFortressZone *>(gameObjects(i));

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

void zFortressZone::ZoneWasHeld( void )
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
// *   OnRoundBegin
// *
// *******************************************************************************
//!
//! @return shall the hole process be repeated?
//!
// *******************************************************************************

void zFortressZone::OnRoundBegin( void )
{
    // determine the owning team: the one that has a player spawned closest
    // find the closest player
    if ( !team )
    {
        teamDistance_ = 0;
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        gCycle * closest = NULL;
        REAL closestDistance = 0;
        tCoord pos;
        if (shape)
            pos = shape->Position();
        // FIXME: What should we use for origin if there is no shape?
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
                    zFortressZone * farthest = NULL;
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
            if (shape)
            {
            rColor color_ = shape->getColor();
            color_.r_ = team->R()/15.0;
            color_.g_ = team->G()/15.0;
            color_.b_ = team->B()/15.0;
            shape->setColor(color_);

                shape->RequestSync();
            }
            teamDistance_ = closestDistance;
        }

        // if this zone does not belong to a team, discard it.
        if ( !team )
        {
            RemoveFromGame();
            return;
        }

        // check other zones owned by the same team. Discard the one farthest away
        // if the max count is exceeded
        if ( team && sg_baseZonesPerTeam > 0 )
        {
            zFortressZone * farthest = 0;
            int count = 0;
            CountZonesOfTeam( Grid(), team, count, farthest );

            // discard team of farthest zone
            if ( count > sg_baseZonesPerTeam )
            {
                farthest->team = NULL;
                farthest->RemoveFromGame();
            }
        }
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

void zFortressZone::OnRoundEnd( void )
{
    // survived?
    if ( currentState_ == State_Safe )
    {
        ZoneWasHeld();
    }
}

// *******************************************************************************
// *
// *	OnInside
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************

void zFortressZone::OnInside( gCycle * target, REAL time )
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
            // store the name
            teamPlayerName_ = target->Player()->GetColoredName();

        ++ ownersInside_;
    }
    else if ( team )
    {
        if ( enemiesInside_ == 0 )
        {
            enemies_.clear();

            // store the name
            enemyPlayerName_ = target->Player()->GetColoredName();
        }

        ++ enemiesInside_;
        if ( std::find( enemies_.begin(), enemies_.end(), otherTeam ) == enemies_.end() )
            enemies_.push_back( otherTeam );

        lastEnemyContact_ = time;
    }
}

static zZoneExtRegistration regFortress("fortress", "", zFortressZone::create);
