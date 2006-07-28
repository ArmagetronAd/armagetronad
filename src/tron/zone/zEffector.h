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

class zEffector
{ 
 public: 
  static zEffector* create();
  zEffector(); //<! Constructor
  zEffector(zEffector const &other);
  void operator=(zEffector const &other); //!< overloaded assignment operator
  virtual zEffector *copy(void) const;
  virtual ~zEffector() {};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets) { };

  void setCount(int _count) {count = _count;};
 protected:
  template <typename T>
    T pickOne(std::vector <T> const &sources);

  int count; 
};


class zEffectorWin : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorWin(); //<! Constructor
  zEffectorWin(zEffectorWin const &other);
  void operator=(zEffectorWin const &other); //!< overloaded assignment operator
  virtual zEffectorWin *copy(void) const;
  virtual ~zEffectorWin() {};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorDeath : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorDeath(); //<! Constructor
  zEffectorDeath(zEffectorDeath const &other);
  void operator=(zEffectorDeath const &other); //!< overloaded assignment operator
  virtual zEffectorDeath *copy(void) const;
  virtual ~zEffectorDeath() {};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorPoint : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorPoint(); //<! Constructor
  zEffectorPoint(zEffectorPoint const &other);
  void operator=(zEffectorPoint const &other); //!< overloaded assignment operator
  virtual zEffectorPoint *copy(void) const;
  virtual ~zEffectorPoint() {};

  void setPoint(int p);

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
 protected:
  int d_score;
};

class zEffectorCycleRubber : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorCycleRubber(); //<! Constructor
  zEffectorCycleRubber(zEffectorCycleRubber const &other);
  void operator=(zEffectorCycleRubber const &other); //!< overloaded assignment operator
  virtual zEffectorCycleRubber *copy(void) const;
  virtual ~zEffectorCycleRubber() {};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class zEffectorCycleBrake : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorCycleBrake(); //<! Constructor
  zEffectorCycleBrake(zEffectorCycleBrake const &other);
  void operator=(zEffectorCycleBrake const &other); //!< overloaded assignment operator
  virtual zEffectorCycleBrake *copy(void) const;
  virtual ~zEffectorCycleBrake() {};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
};

class gArena;

class zEffectorSpawnPlayer : public zEffector
{ 
 public: 
  static zEffector* create();
  zEffectorSpawnPlayer(); //<! Constructor
  zEffectorSpawnPlayer(zEffectorSpawnPlayer const &other);
  void operator=(zEffectorSpawnPlayer const &other); //!< overloaded assignment operator
  virtual zEffectorSpawnPlayer *copy(void) const;
  virtual ~zEffectorSpawnPlayer() {};

  void setGrid(eGrid *_grid) {grid = _grid;};
  void setArena(gArena *_arena) {arena = _arena;};

  virtual void apply(gVectorExtra<ePlayerNetID *> &d_calculatedTargets);
 protected:
  eGrid *grid;
  gArena *arena;
};


#endif
