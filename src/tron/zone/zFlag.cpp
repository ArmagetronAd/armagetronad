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

#include "zone/zFlag.h"
#include "zone/zFortress.h"
#include "zone/zZone.h"

#include <time.h>
#include <algorithm>
#include <functional>
#include <deque>


#define sg_segments sz_zoneSegments


// *******************************************************************************
// *
// *	zFlagZone
// *
// *******************************************************************************
//!
//!		@param	grid Grid to put the zone into
//!
// *******************************************************************************

zFlagZone::zFlagZone( eGrid * grid )
        :zZone( grid )
{
    init_ = false;
    owner_ = NULL;
    ownerTime_ = 0;
	flagHome_ = true;
    chatBlinkUpdateTime_ = 0;
    blinkUpdateTime_ = 0;
    blinkTrackUpdateTime_ = 0;
    ownerDropped_ = NULL;
    ownerDroppedTime_ = 0;
    lastHoldScoreTime_ = -1;
    positionUpdatePending_ = false;
	ownerWarnedNotHome_ = false;

}

// *******************************************************************************
// *
// *	~zFlagZone
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

zFlagZone::~zFlagZone( void )
{
}

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

static float sg_flagDropTime = 3;
static tSettingItem<float> sg_flagDropTimeConfig( "FLAG_DROP_TIME", sg_flagDropTime );

static bool sg_flagTeam = true;
static tSettingItem<bool> sg_flagTeamConfig( "FLAG_TEAM", sg_flagTeam );

static float sg_flagHoldScoreTime = -1;
static tSettingItem<float> sg_flagHoldScoreTimeConfig( "FLAG_HOLD_SCORE_TIME", sg_flagHoldScoreTime );

static int sg_flagHoldScore = 1;
static tSettingItem<int> sg_flagHoldScoreConfig( "FLAG_HOLD_SCORE", sg_flagHoldScore );

static float sg_flagColorR = -1;
static tSettingItem<float> sg_flagColorRConfig( "FLAG_COLOR_R", sg_flagColorR );

static float sg_flagColorG = -1;
static tSettingItem<float> sg_flagColorGConfig( "FLAG_COLOR_G", sg_flagColorG );

static float sg_flagColorB = -1;
static tSettingItem<float> sg_flagColorBConfig( "FLAG_COLOR_B", sg_flagColorB );

//number of flags that must be home in order to capture     
static int sg_minFlagsHome = 0;     
static tSettingItem<int> sg_minFlagsHomeConfig( "MIN_FLAGS_HOME", sg_minFlagsHome );

// flag indicating whether flags need to be home to score
static bool sg_flagRequiredHome = true;
static tSettingItem<bool> sg_flagRequiredHomeConfig("FLAG_REQUIRED_HOME", sg_flagRequiredHome);

static int sg_scoreFlag=3;
static tSettingItem<int> sg_scoreFlagConfig("SCORE_FLAG", sg_scoreFlag);

static bool sg_flagDropHome = false;
static tSettingItem<bool> sg_flagDropHomeConfig("FLAG_DROP_HOME", sg_flagDropHome);

void zFlagZone::setupVisuals(gParser::State_t & state)
{
    REAL tpR[] = {.0f, .3f};
    tPolynomial tpr(tpR, 2);
    state.set("rotation", tpr);
}

void zFlagZone::readXML(tXmlParser::node const & node)
{
}

// *******************************************************************************
// *
// *	GoHome
// *
// *******************************************************************************

void zFlagZone::GoHome()
{
	RemoveOwner();
	flagHome_ = true;
	shape->Position() = homePosition_;
	rColor GoHomeInterfaceColor = shape->getColor();
	GoHomeInterfaceColor.a_ = 100;
	shape->setColor(GoHomeInterfaceColor);
	shape->RequestSync();
}

// *******************************************************************************
// *
// *	OwnerDropped
// *
// *******************************************************************************

void zFlagZone::OwnerDropped()
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
			flagHome_ = false;
			shape->Position() = owner_->Position();
			rColor DropColor = shape->getColor();
			DropColor.a_ = 100;
			shape->setColor(DropColor);
			shape->RequestSync();
			
            // remove the owner
            RemoveOwner();
        }
    }
}

// *******************************************************************************
// *
// *	RemoveOwner
// *
// *******************************************************************************

void zFlagZone::RemoveOwner()
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
        owner_ = NULL;
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

bool zFlagZone::Timestep( REAL time )
{

	tColor color = shape->getColor();
	
	
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

            tColoredString playerName;
            playerName << *player << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_timeout", playerName ) );
        }
    }

	//Check if player is alive or not. If not, send the flag home or drop it based on setting
	
	if(!player->Object()->Alive())
	{
		OwnerDropped();
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

                tOutput win;
                tOutput lose;

                win << "$player_flag_hold_score_win";
                lose << "$player_flag_hold_score_lose";

                player->AddScore(sg_flagHoldScore, win, lose);
            }
        }
        
        if (player)
        {
            const tList<eGameObject>& gameObjects = Grid()->GameObjects();
            for (int i=gameObjects.Len()-1;i>=0;i--)
            {
                zFortressZone *other=dynamic_cast<zFortressZone *>(gameObjects(i));
                if (other )
                {
                    if(other->getTeam() == player->CurrentTeam()){
                        if(other->getShape()->isInteracting(player->Object())){
                            //Player with Flag is at home either warn not back yet or reset it and team scores points
							// search for another flag owned by our team
							bool allFlagsHome = true;
							int flagsHome = 0;
							int flagsTaken = 0;
							const tList<eGameObject>& gameObjects = Grid()->GameObjects();
							for (int i=gameObjects.Len()-1;i>=0;i--)
							{
								zFlagZone *otherFlag=dynamic_cast<zFlagZone *>(gameObjects(i));
								
								if ((otherFlag) &&
									(otherFlag->Team() == team))
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
								WarnFlagNotHome();
							}
							else
							{
								// player has scored a flag capture
								tOutput lose;
								tOutput win;
								int score = sg_scoreFlag;
								
								win << "$player_flag_score";
								player->AddScore(score, win, lose);
								
								// tell the flag to go back home
								GoHome();
							}
						}
					}
				}
							//End flag cast
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


                shape->setReferenceTime(time);

                if (sg_flagBlinkTrackTime > 0)
                {
                    shape->Position() = owner_->Position();
                    shape->SetVelocity(owner_->Direction() * owner_->Speed());
                }
                else
                {
                    eCoord estimatedPosition =
                        (owner_->Position() +
                         (owner_->Direction() *
                          (sg_flagBlinkEstimatePosition * owner_->Speed() * onTime)));

                    shape->Position() = estimatedPosition;
                    shape->SetVelocity(se_zeroCoord);
                }
				//do the blink
				color.a_ = 75;
				shape->setColor(color);
                shape->RequestSync();
            }
            else if (color.a_ == 75)
            {
                if (time >= (blinkUpdateTime_ + onTime))
                {
                    // kill the blink until the next update time
                    shape->setReferenceTime(time);
                    shape->SetVelocity(se_zeroCoord);
					color.a_ = 0;
					shape->setColor(color);
                    shape->RequestSync();
                }
                else if ((sg_flagBlinkTrackTime > 0) &&
                         (time >= (blinkTrackUpdateTime_ + sg_flagBlinkTrackTime)))
                {
                    // track the owner again
                    shape->setReferenceTime(time);
                    shape->Position() = owner_->Position();
                    shape->SetVelocity(owner_->Direction() * owner_->Speed());
                    RequestSync();
                }
            }
        }
    }

    // delegate
    bool returnStatus = zZone::Timestep( time );

    // any pending position updates have been made
    positionUpdatePending_ = false;

    return (returnStatus);
}

// *******************************************************************************
// *
// *	OnVanish
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFlagZone::OnVanish( void )
{

}



// *******************************************************************************
// *
// *	CheckSurvivor
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void zFlagZone::CheckSurvivor( void )
{

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

void zFlagZone::OnRoundBegin( void )
{
    // save the original radius, can't do this at construction
    //originalRadius_ = GetRadius();
	
	//save the original position
	homePosition_ = shape->Position();

    if ((sg_flagColorR >= 0) &&
        (sg_flagColorG >= 0) &&
        (sg_flagColorB >= 0))
    {
        rColor color_ = shape->getColor();

        if (color_.r_ > 1.0)
        {
            color_.r_ = 1.0;
        }
        if (color_.g_ > 1.0)
        {
            color_.g_ = 1.0;
        }
        if (color_.b_ > 1.0)
        {
            color_.b_ = 1.0;
        }
        shape->setColor(color_);

        RequestSync();
    }

    // if this is a team based flag, find the team
    //??? PIG - FIX to make sure every player has a flag?
    if (sg_flagTeam)
    {
        const tList<eGameObject>& gameObjects = Grid()->GameObjects();
        gCycle * closest = NULL;
        REAL closestDistance = 0;
        tCoord pos;
        if (shape)
            pos = shape->Position();
        for (int i=gameObjects.Len()-1;i>=0;i--)
        {
            gCycle *other=dynamic_cast<gCycle *>(gameObjects(i));

            if (other )
            {
                // eTeam * otherTeam = other->Player()->CurrentTeam();
                eTeam * otherTeam = other->Player()->CurrentTeam();
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
            rColor color_ = shape->getColor();
            
            color_.r_ = team->R()/15.0;
            color_.g_ = team->G()/15.0;
            color_.b_ = team->B()/15.0;

            color_.r_ += (1.0 - color_.r_) / 1.8;
            color_.g_ += (1.0 - color_.g_) / 1.8;
            color_.b_ += (1.0 - color_.b_) / 1.8;
            shape->setColor(color_);
            
            RequestSync();
        }

        // if this zone does not belong to a team, discard it.
        if ( !team )
        {
            return;
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

void zFlagZone::OnRoundEnd( void )
{

}

// *******************************************************************************
// *
// *	OnEntry
// *
// *******************************************************************************
//!
//!		@param	target  the cycle that has been found inside the zone
//!		@param	time    the current time
//!
// *******************************************************************************
void zFlagZone::OnEntry( gCycle * target, REAL time )
{
    // make sure target, player, and their team are OK
    if ((!target) ||
        (!target->Player()) ||
        (!target->Player()->CurrentTeam()))
    {
        return;
    }

    // don't process if not initialized yet or owned or updating
    if ((owner_) ||
        (positionUpdatePending_))
    {
        return;
    }

    //check to see if player already has a flag
    bool playerHasFlag = false;
    const tList<eGameObject>& gameObjects = Grid()->GameObjects();
    for (int i=gameObjects.Len()-1;i>=0;i--)
    {
        zFlagZone *otherFlag=dynamic_cast<zFlagZone *>(gameObjects(i));
        if ((otherFlag)){
				if (otherFlag->Owner()== target){
                playerHasFlag = true;
            }
        }
    }
    
    // check if the player is on our team or not (check will fail if team not enabled)
    if (target->Player()->CurrentTeam() == team)
    {
        // player is on our team, if we're not at home, go back
        if (!IsHome())
        {
            // go home
            GoHome();

            tColoredString playerName;
            playerName << *target->Player() << tColoredString::ColorString(1,1,1);
            sn_ConsoleOut( tOutput( "$player_flag_return", playerName ) );
        }
    }
    // check if this player dropped the flag previously
    else if (((target != ownerDropped_) || (time > (ownerDroppedTime_ + sg_flagDropTime))) && (!playerHasFlag))
    {
        // take the flag
        owner_ = target;
        ownerTime_ = time;
        lastHoldScoreTime_ = time;
		rColor setAlphaTakeColor = shape->getColor();
		setAlphaTakeColor.a_ = 0;
		shape->setColor(setAlphaTakeColor);
		shape->RequestSync();
        //DO later
        //ownerWarnedNotHome_ = false;
        //owner_->flag_ = this;

        blinkUpdateTime_ = -1000;
        blinkTrackUpdateTime_ = -1000;

        // diminish the flag and put it at the original location
        shape->setReferenceTime(lastTime);
        shape->SetRotationSpeed( 0 );
        shape->SetRotationAcceleration( 0 );
        //shape->SetReferenceTime();
        //shape->SetVelocity(se_zeroCoord);
        //shape->SetPosition(originalPosition_);
        //shape->SetRadius(0);
        //shape->SetExpansionSpeed(0);
        shape->RequestSync();
        positionUpdatePending_ = true;

        tColoredString playerName;
        playerName << *target->Player() << tColoredString::ColorString(1,1,1);
        sn_ConsoleOut( tOutput( "$player_flag_take", playerName ) );
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

void zFlagZone::OnInside( gCycle * target, REAL time )
{
	/*if(sg_flagTeam)
	{
	//CTF
    // make sure target, player, and their team are OK
    if ((!target) ||
        (!target->Player()) ||
        (!target->Player()->CurrentTeam()))
    {
        return;
    }

    // don't process if owned or updating
    if ((owner_) || (positionUpdatePending_))
    {
        return;
    }
    
	eTeam * TheTeam = target->Player()->CurrentTeam();
		
	if(TheTeam == team)
	{
		//Return the Flag
		GoHome();
		tColoredString playerName;
		playerName << *target->Player() << tColoredString::ColorString(1,1,1);
		sn_ConsoleOut( tOutput( "$player_flag_return", playerName ) );
		return;
	}
		
	//check to see if player already has a flag
	bool playerHasFlag = false;
	const tList<eGameObject>& gameObjects = Grid()->GameObjects();
	for (int i=gameObjects.Len()-1;i>=0;i--)
		{
			zFlagZone* otherFlag=dynamic_cast<zFlagZone *>(gameObjects(i));
			if ((otherFlag)){
				if (otherFlag->Team() == target->Player()->CurrentTeam()){
					playerHasFlag = true;
					return;
				}
			}
		}
		
		//take the flag
		owner_ = target;
	    flagHome_ = false;
		
		tColoredString playerName;
		playerName << *target->Player() << tColoredString::ColorString(1,1,1);
		sn_ConsoleOut( tOutput( "$player_flag_take", playerName ) );
		//TODO - kill the flag at its home
	}
	else
	{
	//HTF
		
	}*/
}

bool zFlagZone::IsHome()
{
    // flag is at home if at the original position and not owned
    if ((!owner_) &&
        (shape->Position() == homePosition_))
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

void zFlagZone::WarnFlagNotHome()
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

static zZoneExtRegistration regFlag("flag", "", zFlagZone::create);
