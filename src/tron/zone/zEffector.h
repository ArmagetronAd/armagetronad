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

#ifndef ARMAGETRONAD_H_EFFECTOR
#define ARMAGETRONAD_H_EFFECTOR

class gCycle;
#include <vector>
#include <memory>
#include "gVectorExtra.h"
#include "ePlayer.h"
#include "gCycle.h"
#include "eTeam.h"
#include "tLocale.h"

class zEffector
{ 
 public: 
  static zEffector* create() { return new zEffector(); };
  zEffector():count(0),message() { }; //<! Constructor
  zEffector(zEffector const &other) { };
  void operator=(zEffector const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffector *copy(void) const { return new zEffector(*this); };
  virtual ~zEffector() {};

  void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets) { };

  void setCount(int _count) {count = _count;};
  void setMessage(tString unformated);

 protected:
  template <typename T>
    T pickOne(std::vector <T> const &sources);

  int count; 
  tOutput message;

};


class zEffectorWin : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorWin(); };
  zEffectorWin():zEffector(){ }; //<! Constructor
  zEffectorWin(zEffectorWin const &other):zEffector(other) { };
  void operator=(zEffectorWin const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorWin *copy(void) const { return new zEffectorWin(*this); };
  virtual ~zEffectorWin() {};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorDeath : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorDeath(); };
  zEffectorDeath():zEffector(){ }; //<! Constructor
  zEffectorDeath(zEffectorDeath const &other):zEffector(other) { };
  void operator=(zEffectorDeath const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorDeath *copy(void) const { return new zEffectorDeath(*this); };
  virtual ~zEffectorDeath() {};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorPoint : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorPoint(); };
  zEffectorPoint():zEffector(){ }; //<! Constructor
  zEffectorPoint(zEffectorPoint const &other):zEffector(other),d_score(other.getPoint()) { };
  void operator=(zEffectorPoint const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorPoint *copy(void) const { return new zEffectorPoint(*this); };
  virtual ~zEffectorPoint() {};

  void setPoint(int p) {d_score = p;};
  int getPoint() const {return d_score;};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
 protected:
  int d_score;
};

class zEffectorCycleRubber : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorCycleRubber(); };
  zEffectorCycleRubber():zEffector(){ }; //<! Constructor
  zEffectorCycleRubber(zEffectorCycleRubber const &other):zEffector(other) { };
  void operator=(zEffectorCycleRubber const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorCycleRubber *copy(void) const { return new zEffectorCycleRubber(*this); };
  virtual ~zEffectorCycleRubber() {};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorCycleBrake : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorCycleBrake(); };
  zEffectorCycleBrake():zEffector(){ }; //<! Constructor
  zEffectorCycleBrake(zEffectorCycleBrake const &other):zEffector(other) { };
  void operator=(zEffectorCycleBrake const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorCycleBrake *copy(void) const { return new zEffectorCycleBrake(*this); };
  virtual ~zEffectorCycleBrake() {};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class gArena;

class zEffectorSpawnPlayer : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorSpawnPlayer(); };
  zEffectorSpawnPlayer():zEffector(){ }; //<! Constructor
  zEffectorSpawnPlayer(zEffectorSpawnPlayer const &other):zEffector(other) { };
  void operator=(zEffectorSpawnPlayer const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorSpawnPlayer *copy(void) const { return new zEffectorSpawnPlayer(*this); };
  virtual ~zEffectorSpawnPlayer() {};

  void setGrid(eGrid *_grid) {grid = _grid;};
  void setArena(gArena *_arena) {arena = _arena;};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
 protected:
  eGrid *grid;
  gArena *arena;
};

class zEffectorSetting : public zEffector
{ 
 public: 
  static zEffector* create() { return new zEffectorSetting(); };
  zEffectorSetting():zEffector(),settingName(),settingValue() { }; //<! Constructor
  zEffectorSetting(zEffectorSetting const &other):
    zEffector(other),
    settingName(other.getSettingName()),
    settingValue(other.getSettingValue())  { };
  void operator=(zEffectorSetting const &other) { this->zEffector::operator=(other); }; //!< overloaded assignment operator
  virtual zEffectorSetting *copy(void) const { return new zEffectorSetting(*this); };
  virtual ~zEffectorSetting() {};

  void setSettingName(tString name) {settingName = name;};
  void setSettingValue(tString value) {settingValue = value;};
  tString getSettingName() const {return settingName;};
  tString getSettingValue() const {return settingValue;};

  virtual void effect(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
 protected:
  tString settingName;
  tString settingValue;
};




#endif
