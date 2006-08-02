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

#ifndef ARMAGETRONAD_H_SELECTOR
#define ARMAGETRONAD_H_SELECTOR

class gCycle;
#include <vector>
#include <boost/shared_ptr.hpp>
#include "gVectorExtra.h"
#include "ePlayer.h"
#include "gCycle.h"
#include "eTeam.h"
#include "zMisc.h"

class zEffector;

typedef boost::shared_ptr<zEffector> zEffectorPtr;
typedef std::vector< zEffectorPtr >  zEffectorPtrs;

enum LivingStatus {
    _either, // Any player should be considered
    _alive,  // A player that has a working vehicule
    _dead    // A player without a working vehicule
};

class zSelector
{
public:
    static zSelector* create();
    zSelector(); //<! Constructor
    zSelector(zSelector const &other);
    void operator=(zSelector const &other); //!< overloaded assignment operator
    virtual zSelector *copy(void) const;
    virtual ~zSelector() {};

    void apply(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
    void setCount(int _count) {count = _count;};

    void addEffector(zEffectorPtr newEffector) {effectors.push_back( newEffector );};
protected:
    virtual gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
protected:
    zEffectorPtrs effectors;
    int count;

    template <typename T>
    T pickOne(std::vector <T> const &sources);

    std::vector <ePlayerNetID *>
    getAllValid(std::vector <ePlayerNetID *> &results, std::vector <ePlayerNetID *> const &sources, LivingStatus living);
};


/*
 *
 */
class zSelectorSelf : public zSelector
{
public:
    static zSelector* create();
    zSelectorSelf(); //<! Constructor
    zSelectorSelf(zSelectorSelf const &other);
    void operator=(zSelectorSelf const &other); //!< overloaded assignment operator
    virtual zSelectorSelf *copy(void) const;
    virtual ~zSelectorSelf() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
class zSelectorTeammate : public zSelector
{
public:
    static zSelector* create();
    zSelectorTeammate(); //<! Constructor
    zSelectorTeammate(zSelectorTeammate const &other);
    void operator=(zSelectorTeammate const &other); //!< overloaded assignment operator
    virtual zSelectorTeammate *copy(void) const;
    virtual ~zSelectorTeammate() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
class zSelectorTeam : public zSelector
{
public:
    static zSelector* create();
    zSelectorTeam(); //<! Constructor
    zSelectorTeam(zSelectorTeam const &other);
    void operator=(zSelectorTeam const &other); //!< overloaded assignment operator
    virtual zSelectorTeam *copy(void) const;
    virtual ~zSelectorTeam() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
class zSelectorAll : public zSelector
{
public:
    static zSelector* create();
    zSelectorAll(); //<! Constructor
    zSelectorAll(zSelectorAll const &other);
    void operator=(zSelectorAll const &other); //!< overloaded assignment operator
    virtual zSelectorAll *copy(void) const;
    virtual ~zSelectorAll() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
class zSelectorAllButSelf : public zSelector
{
public:
    static zSelector* create();
    zSelectorAllButSelf(); //<! Constructor
    zSelectorAllButSelf(zSelectorAllButSelf const &other);
    void operator=(zSelectorAllButSelf const &other); //!< overloaded assignment operator
    virtual zSelectorAllButSelf *copy(void) const;
    virtual ~zSelectorAllButSelf() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
/*
class zSelectorAllButTeam : public zSelector
{ 
 public: 
  static zSelector* create();
  zSelectorAllButTeam(); //<! Constructor
  zSelectorAllButTeam(zSelectorAllButTeam const &other);
  void operator=(zSelectorAllButTeam const &other); //!< overloaded assignment operator
  virtual zSelectorAllButTeam *copy(void) const;
  virtual ~zSelectorAllButTeam() {};

  gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};
*/

/*
 *
 */
class zSelectorAnother : public zSelector
{
public:
    static zSelector* create();
    zSelectorAnother(); //<! Constructor
    zSelectorAnother(zSelectorAnother const &other);
    void operator=(zSelectorAnother const &other); //!< overloaded assignment operator
    virtual zSelectorAnother *copy(void) const;
    virtual ~zSelectorAnother() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};


/*
zSelectorAnotherTeam::create;
zSelectorAnotherTeammate::create;
zSelectorAnotherNotTeammate::create;
*/

/*
 *
 */
class zSelectorOwner : public zSelector
{
public:
    static zSelector* create();
    zSelectorOwner(); //<! Constructor
    zSelectorOwner(zSelectorOwner const &other);
    void operator=(zSelectorOwner const &other); //!< overloaded assignment operator
    virtual zSelectorOwner *copy(void) const;
    virtual ~zSelectorOwner() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */
class zSelectorOwnerTeam : public zSelector
{
public:
    static zSelector* create();
    zSelectorOwnerTeam(); //<! Constructor
    zSelectorOwnerTeam(zSelectorOwnerTeam const &other);
    void operator=(zSelectorOwnerTeam const &other); //!< overloaded assignment operator
    virtual zSelectorOwnerTeam *copy(void) const;
    virtual ~zSelectorOwnerTeam() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};



/*
 *
 */


class zSelectorOwnerTeamTeammate : public zSelector
{
public:
    static zSelector* create();
    zSelectorOwnerTeamTeammate(); //<! Constructor
    zSelectorOwnerTeamTeammate(zSelectorOwnerTeamTeammate const &other);
    void operator=(zSelectorOwnerTeamTeammate const &other); //!< overloaded assignment operator
    virtual zSelectorOwnerTeamTeammate *copy(void) const;
    virtual ~zSelectorOwnerTeamTeammate() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */

class zSelectorAnyDead : public zSelector
{
public:
    static zSelector* create();
    zSelectorAnyDead(); //<! Constructor
    zSelectorAnyDead(zSelectorAnyDead const &other);
    void operator=(zSelectorAnyDead const &other); //!< overloaded assignment operator
    virtual zSelectorAnyDead *copy(void) const;
    virtual ~zSelectorAnyDead() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */

class zSelectorAllDead : public zSelector
{
public:
    static zSelector* create();
    zSelectorAllDead(); //<! Constructor
    zSelectorAllDead(zSelectorAllDead const &other);
    void operator=(zSelectorAllDead const &other); //!< overloaded assignment operator
    virtual zSelectorAllDead *copy(void) const;
    virtual ~zSelectorAllDead() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */

class zSelectorSingleDeadOwner : public zSelector
{
public:
    static zSelector* create();
    zSelectorSingleDeadOwner(); //<! Constructor
    zSelectorSingleDeadOwner(zSelectorSingleDeadOwner const &other);
    void operator=(zSelectorSingleDeadOwner const &other); //!< overloaded assignment operator
    virtual zSelectorSingleDeadOwner *copy(void) const;
    virtual ~zSelectorSingleDeadOwner() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};


/*
 *
 */

class zSelectorAnotherTeammateDead : public zSelector
{
public:
    static zSelector* create();
    zSelectorAnotherTeammateDead(); //<! Constructor
    zSelectorAnotherTeammateDead(zSelectorAnotherTeammateDead const &other);
    void operator=(zSelectorAnotherTeammateDead const &other); //!< overloaded assignment operator
    virtual zSelectorAnotherTeammateDead *copy(void) const;
    virtual ~zSelectorAnotherTeammateDead() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};

/*
 *
 */

class zSelectorAnotherNotTeammateDead : public zSelector
{
public:
    static zSelector* create();
    zSelectorAnotherNotTeammateDead(); //<! Constructor
    zSelectorAnotherNotTeammateDead(zSelectorAnotherNotTeammateDead const &other);
    void operator=(zSelectorAnotherNotTeammateDead const &other); //!< overloaded assignment operator
    virtual zSelectorAnotherNotTeammateDead *copy(void) const;
    virtual ~zSelectorAnotherNotTeammateDead() {};

    gVectorExtra<ePlayerNetID *> select(gVectorExtra<ePlayerNetID *> &owners, gVectorExtra<eTeam *> &teamOwners, gCycle * triggerer);
};


#include "zone/zEffector.h"

#endif
