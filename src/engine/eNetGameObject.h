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

#ifndef ArmageTron_NETGAME_H
#define ArmageTron_NETGAME_H

#include "nNetObject.h"
#include "eGameObject.h"
#include "tCallback.h"

namespace Engine { class NetGameObjectSync; }

// max ping to equalize;
extern int sn_pingCharityServer;

class ePlayerNetID;


class eNetGameObject:public eGameObject,public nNetObject{
    friend class ePlayerNetID;

    void MyInitAfterCreation();
protected:
    REAL lastClientsideAction;
    REAL lastAttemptedSyncTime;

    tCHECKED_PTR(ePlayerNetID)    player; // the player controlling this cycle.
    // NULL means the AI.
    REAL laggometer;        //!< the actual best estimate for lag
    REAL laggometerSmooth;  //!< the lag, smoothed over time

    // used to implement the control functions
    virtual void ReceiveControlNet( Network::NetObjectControl const & control );

    // easier to implement conversion helpers: just extract the relevant sub-protbuf.
    virtual nProtoBuf       * ExtractControl( Network::NetObjectControl       & control );
    virtual nProtoBuf const * ExtractControl( Network::NetObjectControl const & control );

    virtual ~eNetGameObject();

    void SetPlayer(ePlayerNetID* player);

    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to
public:
    virtual bool ActionOnQuit(){
        //    this->Kill();
        return false;
    }

    virtual void ActionOnDelete(){
        RemoveFromGame();
    }

    virtual void InitAfterCreation();

    eNetGameObject(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID* p,bool autodelete=false);

    virtual void DoRemoveFromGame(); // just remove it from the lists and unregister it

    virtual bool ClearToTransmit(int user) const;

    virtual void AddRef();          //!< adds a reference
    virtual void Release();         //!< removes a reference

    // control functions: (were once part of nNetObject.h)
    virtual void SendControl(REAL time,uActionPlayer *Act,REAL x);
    // is called on the client whenever a control key is pressed. This
    // sends a message to the server, who will call
    virtual void ReceiveControl(REAL time,uActionPlayer *Act,REAL x);
    // on his instance of the nNetObject.

    virtual bool Timestep(REAL currentTime);

    ePlayerNetID *Player()const{return player;}

    void clientside_action(){lastClientsideAction=lastTime;}

    virtual REAL Lag() const;
    virtual REAL LagThreshold() const;

    //! creates a netobject form sync data
    eNetGameObject( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Engine::NetGameObjectSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
};

class eTransferInhibitor: public tCallbackOr{
    static int user;
public:
    static int User(){return user;}

    eTransferInhibitor(BOOLRETFUNC *f);
    static bool no_transfer(int user);
};

#endif

