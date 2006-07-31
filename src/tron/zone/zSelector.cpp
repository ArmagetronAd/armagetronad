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

#include "zSelector.h"
#include "tRandom.h"


//
// Copy all the valid players from source and copy them in result. Valid depends on the condition
// living. 
//
std::vector <ePlayerNetID *>
zSelector::getAllValid(std::vector <ePlayerNetID *> &results, std::vector <ePlayerNetID *> const &sources, LivingStatus living)
{
    unsigned short int max = sources.size();
    for(unsigned short int i=0;i<max;i++)
      {
	ePlayerNetID *p=sources[i];
	switch (living) {
	case _either:
	  results.push_back(p);
	  break;
	case _alive:
	  if ( p->Object() && p->Object()->Alive() )
	    {
	      results.push_back(p);
	    }
	  break;
	case _dead: 
	  if(  !(p->Object()) || !(p->Object()->Alive()) )
	    {
	      results.push_back(p);
	    }
	  break;
	}
      }
    return results;
}

zSelector::zSelector()
{ }

zSelector::zSelector(zSelector const &other)
{ }

void zSelector::operator=(zSelector const &other)
{ 
  if(this != &other) {
  }
}

zSelector * zSelector::copy(void) const
{
  return new zSelector(*this);
}

gVectorExtra<ePlayerNetID *> zSelector::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  gVectorExtra<ePlayerNetID *> empty;
  return empty;
}

void 
zSelector::apply(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  if (count == -1 || count > 0) 
    {
      gVectorExtra<ePlayerNetID *> d_calculatedTargets = select(owners, teamOwners, triggerer);
      zEffectorPtrs::const_iterator iter;
      for(iter=effectors.begin();
	  iter!=effectors.end();
	  ++iter)
	{
	  (*iter)->apply(d_calculatedTargets);
	}
      if (count > 0) 
	count --;
    }
}

//
// TargetSelf
//
zSelector* zSelectorSelf::create() 
{
  return new zSelectorSelf();
}

zSelectorSelf::zSelectorSelf(): 
  zSelector()
{ }

zSelectorSelf::zSelectorSelf(zSelectorSelf const &other):
  zSelector(other)
{ }

void zSelectorSelf::operator=(zSelectorSelf const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorSelf * zSelectorSelf::copy(void) const
{
  return new zSelectorSelf(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorSelf::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  gVectorExtra<ePlayerNetID *> self;
  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0)
    self.push_back( triggererPlayer );

  return self;
}


//
// zSelectorTeammate
//
zSelector* zSelectorTeammate::create() 
{
  return new zSelectorTeammate();
}

zSelectorTeammate::zSelectorTeammate(): 
  zSelector()
{ }

zSelectorTeammate::zSelectorTeammate(zSelectorTeammate const &other):
  zSelector(other)
{ }

void zSelectorTeammate::operator=(zSelectorTeammate const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorTeammate * zSelectorTeammate::copy(void) const
{
  return new zSelectorTeammate(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorTeammate::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // A single, randomly selected player that is member of the team of
  // the player that triggered the Zone receives the effect. This
  // includes the player that triggered the Zone as a candidate.
  gVectorExtra <ePlayerNetID *> teammates;
  gVectorExtra <ePlayerNetID *> singleTeammate;

  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0) {
    getAllValid(teammates, triggererPlayer->CurrentTeam()->GetAllMembers(), _alive);

    // Remove the triggerer if it is there 
    teammates.remove(triggererPlayer);
    
    // Who is our lucky candidate ? 
    if(teammates.size() != 0)
      singleTeammate.push_back(pickOne(teammates));
  }
  return singleTeammate;
}

//
// zSelectorTeam
//
zSelector* zSelectorTeam::create() 
{
  return new zSelectorTeam();
}

zSelectorTeam::zSelectorTeam(): 
  zSelector()
{ }

zSelectorTeam::zSelectorTeam(zSelectorTeam const &other):
  zSelector(other)
{ }

void zSelectorTeam::operator=(zSelectorTeam const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorTeam * zSelectorTeam::copy(void) const
{
  return new zSelectorTeam(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorTeam::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // All the members of the team of the player triggering the
  // Zone will receive its effect. 

  gVectorExtra <ePlayerNetID *> allMemberOfTeam;

  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0) {
    getAllValid(allMemberOfTeam, triggererPlayer->CurrentTeam()->GetAllMembers(), _alive);
  }

  return allMemberOfTeam;
}

//
// zSelectorAll
//
zSelector* zSelectorAll::create() 
{
  return new zSelectorAll();
}

zSelectorAll::zSelectorAll(): 
  zSelector()
{ }

zSelectorAll::zSelectorAll(zSelectorAll const &other):
  zSelector(other)
{ }

void zSelectorAll::operator=(zSelectorAll const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAll * zSelectorAll::copy(void) const
{
  return new zSelectorAll(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAll::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // Everybody in the game receives the effect.
  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }

  gVectorExtra<ePlayerNetID *> everybodyValid;
  getAllValid(everybodyValid, everybody, _alive);

  return everybodyValid;
}

//
// zSelectorAllButSelf
//
zSelector* zSelectorAllButSelf::create() 
{
  return new zSelectorAllButSelf();
}

zSelectorAllButSelf::zSelectorAllButSelf(): 
  zSelector()
{ }

zSelectorAllButSelf::zSelectorAllButSelf(zSelectorAllButSelf const &other):
  zSelector(other)
{ }

void zSelectorAllButSelf::operator=(zSelectorAllButSelf const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAllButSelf * zSelectorAllButSelf::copy(void) const
{
  return new zSelectorAllButSelf(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAllButSelf::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // Everybody in the game but the player who triggered the
  // Zone receives the effect. This excludes the player
  // that triggered the Zone.

  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }

  gVectorExtra<ePlayerNetID *> everybodyValid;
  getAllValid(everybodyValid, everybody, _alive);
  // Remove the triggerer from the list of targets

  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0) {
    everybodyValid.remove(triggererPlayer);
  }

  return everybodyValid;
}

/*
//
// zSelectorAllButTeam
//
zSelector* zSelectorAllButTeam::create() 
{
  return new zSelectorAllButTeam();
}

zSelectorAllButTeam::zSelectorAllButTeam(): 
  zSelector()
{ }

zSelectorAllButTeam::zSelectorAllButTeam(zSelectorAllButTeam const &other):
  zSelector(other)
{ }

void zSelectorAllButTeam::operator=(zSelectorAllButTeam const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAllButTeam * zSelectorAllButTeam::copy(void) const
{
  return new zSelectorAllButTeam(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAllButTeam::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{


  // Everybody in the game but members of the team of the
  // player who triggered the Zone receive the effect. 
  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }

  gVectorExtra<ePlayerNetID *> everybodyValid;
  getAllValid(everybodyValid, everybody, _alive);

  // Remove the trigerer's team member
  gVectorExtra<ePlayerNetID *> teamMember;
  everybodyValid.removeAll(getTeammembers(teamMember, triggerer));

  return everybodyValid;
}

*/

//
// zSelectorAnother
//

zSelector* zSelectorAnother::create() 
{
  return new zSelectorAnother();
}

zSelectorAnother::zSelectorAnother(): 
  zSelector()
{ }

zSelectorAnother::zSelectorAnother(zSelectorAnother const &other):
  zSelector(other)
{ }

void zSelectorAnother::operator=(zSelectorAnother const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAnother * zSelectorAnother::copy(void) const
{
  return new zSelectorAnother(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAnother::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // Another single, randomly selected player that is the the one that
  // triggered the Zone receive the effect. This excludes the player
  // that triggered the Zone.
  
  gVectorExtra <ePlayerNetID *> allValidPlayer;
  gVectorExtra <ePlayerNetID *> anotherPlayer;

  gVectorExtra <ePlayerNetID *> allPlayer;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    allPlayer.push_back( se_PlayerNetIDs(i) );
  }

  getAllValid(allValidPlayer, allPlayer, _alive);

  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0) {
    allValidPlayer.remove(triggererPlayer);
  }

  if(allValidPlayer.size() != 0)
    anotherPlayer.push_back(pickOne(allValidPlayer));
      
  return anotherPlayer;
}

//
// zSelectorOwner
//

zSelector* zSelectorOwner::create() 
{
  return new zSelectorOwner();
}

zSelectorOwner::zSelectorOwner(): 
  zSelector()
{ }

zSelectorOwner::zSelectorOwner(zSelectorOwner const &other):
  zSelector(other)
{ }

void zSelectorOwner::operator=(zSelectorOwner const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorOwner * zSelectorOwner::copy(void) const
{
  return new zSelectorOwner(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorOwner::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // The owner of the Zone receives the effect upon trigger,
  // irrelevantly of the user who triggers it.
  
  return owners;
}

//
// zSelectorOwnerTeam
//

zSelector* zSelectorOwnerTeam::create() 
{
  return new zSelectorOwnerTeam();
}

zSelectorOwnerTeam::zSelectorOwnerTeam(): 
  zSelector()
{ }

zSelectorOwnerTeam::zSelectorOwnerTeam(zSelectorOwnerTeam const &other):
  zSelector(other)
{ }

void zSelectorOwnerTeam::operator=(zSelectorOwnerTeam const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorOwnerTeam * zSelectorOwnerTeam::copy(void) const
{
  return new zSelectorOwnerTeam(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorOwnerTeam::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // The members of the team that owns the Zone receives the
  // effect upon trigger, irrelevantly of the user who triggers it.

  // Pick all the members of the teams
  gVectorExtra <ePlayerNetID *> allOwnerTeamMembers;
  gVectorExtra<eTeam *>::const_iterator iter;
  for(iter = teamOwners.begin();
      iter != teamOwners.end();
      ++iter)
    {
      allOwnerTeamMembers.insertAll((*iter)->GetAllMembers());
    }

  // Keep only the valid ones
  gVectorExtra <ePlayerNetID *> OwnerTeamMembers;
  getAllValid(OwnerTeamMembers, allOwnerTeamMembers, _alive);

  return OwnerTeamMembers;
}


//
// zSelectorOwnerTeamTeammate
//
zSelector* zSelectorOwnerTeamTeammate::create() 
{
  return new zSelectorOwnerTeamTeammate();
}

zSelectorOwnerTeamTeammate::zSelectorOwnerTeamTeammate(): 
  zSelector()
{ }

zSelectorOwnerTeamTeammate::zSelectorOwnerTeamTeammate(zSelectorOwnerTeamTeammate const &other):
  zSelector(other)
{ }

void zSelectorOwnerTeamTeammate::operator=(zSelectorOwnerTeamTeammate const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorOwnerTeamTeammate * zSelectorOwnerTeamTeammate::copy(void) const
{
  return new zSelectorOwnerTeamTeammate(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorOwnerTeamTeammate::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // Pick all the members of the teams
  gVectorExtra <ePlayerNetID *> allTeammates;
  gVectorExtra<eTeam *>::const_iterator iter;
  for(iter = teamOwners.begin();
      iter != teamOwners.end();
      ++iter)
    {
      allTeammates.insertAll((*iter)->GetAllMembers());
    }

  // Keep only the valid ones
  gVectorExtra <ePlayerNetID *> teammates;
  getAllValid(teammates, allTeammates, _alive);

  // Select a random one 
  gVectorExtra<ePlayerNetID *> singleTeammate;
  if(teammates.size() != 0)
    singleTeammate.push_back(pickOne(teammates));
  return singleTeammate;
}

//
// zSelectorAnyDead
//
zSelector* zSelectorAnyDead::create() 
{
  return new zSelectorAnyDead();
}

zSelectorAnyDead::zSelectorAnyDead(): 
  zSelector()
{ }

zSelectorAnyDead::zSelectorAnyDead(zSelectorAnyDead const &other):
  zSelector(other)
{ }

void zSelectorAnyDead::operator=(zSelectorAnyDead const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAnyDead * zSelectorAnyDead::copy(void) const
{
  return new zSelectorAnyDead(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAnyDead::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // A single, randomly selected dead player receives the
  // effect.

  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }

  gVectorExtra<ePlayerNetID *> everybodyDead;
  getAllValid(everybodyDead, everybody, _dead);

  gVectorExtra<ePlayerNetID *> singleDead;
  if(everybodyDead.size() != 0)
    singleDead.push_back(pickOne(everybodyDead));

  return singleDead;
}

//
// zSelectorAllDead
//
zSelector* zSelectorAllDead::create() 
{
  return new zSelectorAllDead();
}

zSelectorAllDead::zSelectorAllDead(): 
  zSelector()
{ }

zSelectorAllDead::zSelectorAllDead(zSelectorAllDead const &other):
  zSelector(other)
{ }

void zSelectorAllDead::operator=(zSelectorAllDead const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAllDead * zSelectorAllDead::copy(void) const
{
  return new zSelectorAllDead(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAllDead::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // All dead players receives the effect.

  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }

  gVectorExtra<ePlayerNetID *> everybodyDead;
  getAllValid(everybodyDead, everybody, _dead);

  return everybodyDead;
}

//
// zSelectorSingleDeadOwner
//
zSelector* zSelectorSingleDeadOwner::create() 
{
  return new zSelectorSingleDeadOwner();
}

zSelectorSingleDeadOwner::zSelectorSingleDeadOwner(): 
  zSelector()
{ }

zSelectorSingleDeadOwner::zSelectorSingleDeadOwner(zSelectorSingleDeadOwner const &other):
  zSelector(other)
{ }

void zSelectorSingleDeadOwner::operator=(zSelectorSingleDeadOwner const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorSingleDeadOwner * zSelectorSingleDeadOwner::copy(void) const
{
  return new zSelectorSingleDeadOwner(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorSingleDeadOwner::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // A single, randomly selected dead member of
  // the team of the player that triggered the Zone receives the
  // effect.

  gVectorExtra <ePlayerNetID *> deadOwners;
  gVectorExtra <ePlayerNetID *> singleDeadOwner;

  getAllValid(deadOwners, owners, _dead);
  
  // Who is our lucky candidate ? 
  if(deadOwners.size() != 0)
    singleDeadOwner.push_back(pickOne(deadOwners));
  
  return singleDeadOwner;
}



//
// zSelectorAnotherTeammateDead
//
zSelector* zSelectorAnotherTeammateDead::create() 
{
  return new zSelectorAnotherTeammateDead();
}

zSelectorAnotherTeammateDead::zSelectorAnotherTeammateDead(): 
  zSelector()
{ }

zSelectorAnotherTeammateDead::zSelectorAnotherTeammateDead(zSelectorAnotherTeammateDead const &other):
  zSelector(other)
{ }

void zSelectorAnotherTeammateDead::operator=(zSelectorAnotherTeammateDead const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAnotherTeammateDead * zSelectorAnotherTeammateDead::copy(void) const
{
  return new zSelectorAnotherTeammateDead(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAnotherTeammateDead::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // A single, randomly selected dead member of
  // the team of the player that triggered the Zone receives the
  // effect.

  gVectorExtra <ePlayerNetID *> deadTeammates;
  gVectorExtra <ePlayerNetID *> singleDeadTeammate;

  ePlayerNetID* triggererPlayer =  triggerer->Player();
  if (triggererPlayer != 0) {
    getAllValid(deadTeammates, triggererPlayer->CurrentTeam()->GetAllMembers(), _dead);

    // Remove the triggerer if it is there 
    deadTeammates.remove(triggererPlayer);
    
    // Who is our lucky candidate ? 
    if(deadTeammates.size() != 0)
      singleDeadTeammate.push_back(pickOne(deadTeammates));
  }
  return singleDeadTeammate;
}


//
// zSelectorAnotherNotTeammateDead
//
zSelector* zSelectorAnotherNotTeammateDead::create() 
{
  return new zSelectorAnotherNotTeammateDead();
}

zSelectorAnotherNotTeammateDead::zSelectorAnotherNotTeammateDead(): 
  zSelector()
{ }

zSelectorAnotherNotTeammateDead::zSelectorAnotherNotTeammateDead(zSelectorAnotherNotTeammateDead const &other):
  zSelector(other)
{ }

void zSelectorAnotherNotTeammateDead::operator=(zSelectorAnotherNotTeammateDead const &other)
{ 
  this->zSelector::operator=(other);
}

zSelectorAnotherNotTeammateDead * zSelectorAnotherNotTeammateDead::copy(void) const
{
  return new zSelectorAnotherNotTeammateDead(*this);
}

gVectorExtra<ePlayerNetID *> zSelectorAnotherNotTeammateDead::select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer)
{
  // A single, randomly selected dead player receives the
  // effect.

  // Get all the dead players
  gVectorExtra<ePlayerNetID *> everybody;
  for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
    everybody.push_back( se_PlayerNetIDs(i) );
  }
  gVectorExtra<ePlayerNetID *> everybodyDead;
  getAllValid(everybodyDead, everybody, _dead);
  
  gVectorExtra <ePlayerNetID *> singleDeadNotTeammate;
  if (everybodyDead.size() != 0)
    {
      gVectorExtra <ePlayerNetID *> teammates;
      // Remove all the teammates
      ePlayerNetID* triggererPlayer =  triggerer->Player();
      if (triggererPlayer != 0) 
	{
	  getAllValid(teammates, triggererPlayer->CurrentTeam()->GetAllMembers(), _either);

	  // Remove the teammates
	  everybodyDead.removeAll(teammates);
    
	  // Who is our lucky candidate ? 
	  if(everybodyDead.size() != 0)
	    singleDeadNotTeammate.push_back(pickOne(everybodyDead));
	}
    }
  return singleDeadNotTeammate;
}

























//
// Count the number of other alive players in the same team as thePlayer, ie: excluding thePlayer
//
/*
unsigned short int
zSelector::countAliveTeammates(ePlayerNetID *thePlayer, bool requiredAlive){
    int alivemates=0;
    eTeam *playersTeam = thePlayer->CurrentTeam();
    unsigned short max = playersTeam->NumPlayers();

    if(thePlayer && playersTeam)
    {
	for(unsigned short int i=0; i<max; i++){
	    ePlayerNetID *p = playersTeam->Player(i);
	    if(p->Object() && ( p->Object()->Alive() || !requiredAlive ) && p != thePlayer)
		alivemates++;
	}
    }
    return alivemates;
}
*/

//
// Count the total of alive players in the same team as thePlayer, ie: including thePlayer, if it is alive.
//
/*
unsigned short int
zSelector::countAliveTeamMembers(ePlayerNetID *thePlayer, bool requiredAlive){
    int alivemates=0;
    eTeam *playersTeam = thePlayer->CurrentTeam();
    unsigned short max = playersTeam->NumPlayers();

    if(thePlayer && playersTeam)
    {
	for(unsigned short int i=0; i<max; i++){
	    ePlayerNetID *p=playersTeam->Player(i);
	    if(p->Object() && ( p->Object()->Alive() || !requiredAlive ))
		alivemates++;
	}
    }
    return alivemates;
}
*/


//
// Given a list, will pick a single element at random
// If result is given, the element is added to it.
// In all cases, the element choosen is returned by the method
//
template <typename T>
T
zSelector::pickOne(std::vector <T> const &sources)
{
    int number = sources.size();
    tRandomizer & randomizer = tRandomizer::GetInstance();
    int ran = randomizer.Get(number);

    return sources[ran];
}

/*
std::vector <ePlayerNetID *>
::getCalculatedTarget( gCycle * triggerer )
{
    gVectorExtra<ePlayerNetID*> targets;

    switch(d_triggererType)
    {
    case TargetSelf: 
      // Only the player that caused the trigger will be receipient
      // of the effect of the Zone. This has been the effect of the WinZone
      // and DeathZone. This will be the default behavior.
	targets.push_back(triggerer->Player());
    break;
    case TargetTeammate:
      {
	// A single, randomly selected player that is member of the team of
	// the player that triggered the Zone receives the effect. This
	// includes the player that triggered the Zone as a candidate.
	gVectorExtra <ePlayerNetID *> teammates;
	getAllValid(teammates, triggerer->Player()->CurrentTeam()->GetAllMembers(), true);

	// Remove the triggerer if it is there 
	teammates.remove(triggerer->Player());

	// Who is our lucky candidate ? 
	targets.push_back(pickOne(teammates));
      }
    break; 
    case TargetTeam:
      {
	// All the members of the team of the player triggering the
	// Zone will receive its effect. 
	gVectorExtra <ePlayerNetID *> temp;
	getAllValid(temp, triggerer->Player()->CurrentTeam()->GetAllMembers(), true);
	targets.insert(targets.end(), temp.begin(), temp.end()+1);
      }
    break; 
    case TargetAll: 
      {
	// Everybody in the game receives the effect.
	gVectorExtra<ePlayerNetID *> temp;
	for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
	  temp.push_back( se_PlayerNetIDs(i) );
	}

	getAllValid(targets, temp, true);
      }
    break; 
    case TargetAllButSelf: 
      {
	// Everybody in the game but the player who triggered the
	// Zone receives the effect. This excludes the player
	// that triggered the Zone.
	gVectorExtra<ePlayerNetID *> temp;
	for (int i=0; i<se_PlayerNetIDs.Len(); i++) {
	  temp.push_back( se_PlayerNetIDs(i) );
	}
	getAllValid(targets, temp, true);
	targets.remove(triggerer->Player());
      }
    break; 
    case TargetAllButTeam: 
      // Everybody in the game but members of the team of the
      // player who triggered the Zone receive the effect. 

      //
      // targets.insertAll(se_PlayerNetIDs);
      // targets.removeAll(getTeammembers(temp, triggerer));
      //
    break; 
    case TargetAnother: 
      // Another single, randomly selected player that is the the one that
      // triggered the Zone receive the effect. This excludes the player
      // that triggered the Zone.

      //
      // gVectorExtra <ePlayerNetID *> temp;
      // getAllValid(temp, se_PlayerNetIDs, true);
      // temp.remove(triggerer);
      // targets.push_back(pickOne(temp));
      //
    break; 
    case TargetAnotherTeam: 
      // All the members of a single, randomly selected team that is
      // not the team of the player who triggered the effect receives the
      // effect. 

      //
      // gVectorExtra <eTeam *> temp(teams);
      // temp.remove(triggerer->Player()->CurrentTeam());
      // getAllValid(targets, pickOne(temp)->GetAllMembers(), true);
      //
    break; 
    case TargetAnotherTeammate: 
      // A single, randomly selected member of the team of the
      // player that triggered the Zone receive the effect. This exclude
      // the player that triggered the Zone.
    break; 
    case TargetAnotherNotTeammate: 
      // A single, randomly selected player that is not member
      // of the team of the player that triggered the Zone receives the effect.
    break; 
    case TargetOwner: 
      // The owner of the Zone receives the effect upon trigger,
      // irrelevantly of the user who triggers it.
    break; 
    case TargetOwnerTeam: 
      // The members of the team that owns the Zone receives the
      // effect upon trigger, irrelevantly of the user who triggers it.
    break; 
    case TargetOwnerTeamTeammate:
      // A single, randomly selected member of the team that owns the Zone receives the
      // effect upon trigger, irrelevantly of the user who triggers it.
    break;

    // Target types that select among the players that are dead
    case TargetAnyDead: 
      // A single, randomly selected dead player receives the
      // effect.
    break; 
    case TargetAllDead: 
      // All dead players receives the effect.
    break; 
    case TargetAnotherTeammateDead: 
      // A single, randomly selected dead member of
      // the team of the player that triggered the Zone receives the
      // effect.
    break; 
    case TargetAnotherNotTeammateDead: 
      // A single, randomly selected dead player
      // that is not memeber of the team of the player that triggered the
      // Zone receives the effect.
    break;
    }
    return targets;
}
*/
