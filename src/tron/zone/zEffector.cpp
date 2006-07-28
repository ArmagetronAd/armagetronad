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

#include "zEffector.h"
#include "tRandom.h"
#include "gGame.h"

zEffector::zEffector()
{ }

zEffector::zEffector(zEffector const &other)
{ }

void zEffector::operator=(zEffector const &other)
{ 
  if(this != &other) {
  }
}

zEffector * zEffector::copy(void) const
{
  return new zEffector(*this);
}

//
//
//
zEffector* zEffectorWin::create()
{
  return new zEffectorWin();
}

zEffectorWin::zEffectorWin(): 
  zEffector()
{ }

zEffectorWin::zEffectorWin(zEffectorWin const &other):
  zEffector(other)
{ }

void zEffectorWin::operator=(zEffectorWin const &other)
{ 
  this->zEffector::operator=(other);
}

zEffectorWin * zEffectorWin::copy(void) const
{
  return new zEffectorWin(*this);
}

void zEffectorWin::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  // BOP
  static const char* message="$player_win_instant";
  // EOP

  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	sg_DeclareWinner((*iter)->CurrentTeam(), message );
      }
    if (count > 0) 
      count --;
  }
}

//
//
//
zEffector* zEffectorDeath::create()
{
  return new zEffectorDeath();
}

zEffectorDeath::zEffectorDeath(): 
  zEffector()
{ }

zEffectorDeath::zEffectorDeath(zEffectorDeath const &other):
  zEffector(other)
{ }

void zEffectorDeath::operator=(zEffectorDeath const &other)
{ 
  this->zEffector::operator=(other);
}

zEffectorDeath * zEffectorDeath::copy(void) const
{
  return new zEffectorDeath(*this);
}

void zEffectorDeath::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	static_cast<gCycle *>((*iter)->Object())->Kill();
      }
    if (count > 0) 
      count --;
  }
}

//
//
//
zEffector* zEffectorPoint::create()
{
  return new zEffectorPoint();
}

zEffectorPoint::zEffectorPoint(): 
  zEffector(),
  d_score(1)
{ }

zEffectorPoint::zEffectorPoint(zEffectorPoint const &other):
  zEffector(other),
  d_score(other.d_score)
{ }

void 
zEffectorPoint::setPoint(int p) {
  d_score = p;
}

void
zEffectorPoint::operator=(zEffectorPoint const &other)
{ 
  this->zEffector::operator=(other);
  d_score = other.d_score;
}

zEffectorPoint * zEffectorPoint::copy(void) const
{
  return new zEffectorPoint(*this);
}

void zEffectorPoint::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	(*iter)->AddScore(d_score, tOutput(), "$player_lose_suicide");
      }
    if (count > 0) 
      count --;
  }
}

//
//
//
zEffector* zEffectorCycleRubber::create()
{
  return new zEffectorCycleRubber();
}

zEffectorCycleRubber::zEffectorCycleRubber(): 
  zEffector()
{ }

zEffectorCycleRubber::zEffectorCycleRubber(zEffectorCycleRubber const &other):
  zEffector(other)
{ }

void zEffectorCycleRubber::operator=(zEffectorCycleRubber const &other)
{ 
  this->zEffector::operator=(other);
}

zEffectorCycleRubber * zEffectorCycleRubber::copy(void) const
{
  return new zEffectorCycleRubber(*this);
}

void zEffectorCycleRubber::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	static_cast<gCycle *>((*iter)->Object())->SetRubber(0.0);
      }
    if (count > 0) 
      count --;
  }
}


//
//
//
zEffector* zEffectorCycleBrake::create()
{
  return new zEffectorCycleBrake();
}

zEffectorCycleBrake::zEffectorCycleBrake(): 
  zEffector()
{ }

zEffectorCycleBrake::zEffectorCycleBrake(zEffectorCycleBrake const &other):
  zEffector(other)
{ }

void zEffectorCycleBrake::operator=(zEffectorCycleBrake const &other)
{ 
  this->zEffector::operator=(other);
}

zEffectorCycleBrake * zEffectorCycleBrake::copy(void) const
{
  return new zEffectorCycleBrake(*this);
}

void zEffectorCycleBrake::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	static_cast<gCycle *>((*iter)->Object())->SetBrakingReservoir(1.0);
      }
    if (count > 0) 
      count --;
  }
}

//
//
//
zEffector* zEffectorSpawnPlayer::create()
{
  return new zEffectorSpawnPlayer();
}

zEffectorSpawnPlayer::zEffectorSpawnPlayer(): 
  zEffector(),
  grid(0),
  arena(0)
{ }

zEffectorSpawnPlayer::zEffectorSpawnPlayer(zEffectorSpawnPlayer const &other):
  zEffector(other)
{ }

void zEffectorSpawnPlayer::operator=(zEffectorSpawnPlayer const &other)
{ 
  this->zEffector::operator=(other);
}

zEffectorSpawnPlayer * zEffectorSpawnPlayer::copy(void) const
{
  return new zEffectorSpawnPlayer(*this);
}

void zEffectorSpawnPlayer::apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets)
{
  if (count == -1 || count > 0) {
    gVectorExtra<ePlayerNetID *>::iterator iter;
    for(iter = d_calculatedTargets.begin();
	iter != d_calculatedTargets.end();
	++iter)
      {
	sg_RespawnPlayer(grid, arena, (*iter));
      }
    if (count > 0) 
      count --;
  }
}

