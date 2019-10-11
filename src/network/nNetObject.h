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

#ifndef ArmageTron_NETOBJECT_H
#define ArmageTron_NETOBJECT_H

#include "nNetwork.h"
#include "nProtoBufForward.h"
#include "tArray.h"
#include "tConsole.h"
#include <string.h>

class nObserver;

class nSenderInfo;
class nNetObjectDescriptorBase;
class nProtoBufDescriptorBase;
class nProtoBufMessageBase;

namespace Network{ class NetObjectSync; class NetObjectControl; }

// checks whether n is newer than old; if so, old is set to n and
// true is returned.
bool sn_Update(unsigned short &old,unsigned short n);
bool sn_Update(unsigned long &old,unsigned long n);

// here, the high level network protocol is specified.
// every entity server and client need to constantly exchange
// information about should be a nNetObject.

typedef unsigned short nNetObjectID;

struct nNetObjectRegistrar;

class nNetObject{
    friend class nWaitForAckSync;
    friend class nProtoBufNetControlDescriptor;

    bool createdLocally;		 // was the object created on this computer? (alternative: it was created on remote order)
    unsigned long int lastSyncID_;  // the extended id of the last accepted sync message

private:
    unsigned short id;  // the global id; this is the same on all
    // computers.

    mutable int refCtr_; // how many references from
    // other netobjects point to here?

    unsigned short owner; // who owns this object?
    // on the server, this is the client number of the computer who
    // controls this object; only from this client,
    // control messages are accepted.
    // On the clients, this is just a bool indicating whether we own it
    // or not.

    mutable tCONTROLLED_PTR( nObserver ) observer_;  // this object's observer

    //! the last sync message written
    mutable tJUST_CONTROLLED_PTR< nProtoBufMessageBase > lastSync_;

    //! the last creation message written
    mutable tJUST_CONTROLLED_PTR< nProtoBufMessageBase > lastCreate_;

    int syncListID_;                                 // ID for the list of objects to sync
public:
    struct nKnowsAboutInfo
    {
    public:
        //! the last sync message known to have been received by the peer
        tJUST_CONTROLLED_PTR< nProtoBufMessageBase > lastSync_;

        bool knowsAboutExistence:
        1; // is the creation message through?
        bool nextSyncAck:
        1;         // should the next sync message wait
        // for it's ack?
        bool syncReq:
        1;              // should a sync message be sent?
        unsigned char  acksPending:
        4;          // how many messages are underway?

        ~nKnowsAboutInfo();

        nKnowsAboutInfo();
        
        void Reset();

    private:
        nKnowsAboutInfo( nKnowsAboutInfo const & );
        nKnowsAboutInfo & operator = ( nKnowsAboutInfo const & );
    };

    static nNetObject *Object(int i);
    // returns a pointer to the nNetObject
    // with id=i. If that does not exist yet, wait for it to spawn,
    // or, on the server, kill the person responsible.
    // should be only called in constructors.

protected:

    nKnowsAboutInfo knowsAbout[MAXCLIENTS+2];

    void DoBroadcastExistence();
public:
    static bool DoDebugPrint(); // tells ClearToTransmit to print reason of failure

    static nNetObject *ObjectDangerous(int i );
    // the same thin but returns NULL if the object is not yet available.
    
    // clears an eventually deleted object of the given ID out of the main lists
    static void ClearDeleted( unsigned short ID );

    virtual void AddRef(); // call this every time you add a pointer
    // to this nNetObject from another nNetObject, so we know when it is
    // safe to delete this.
    virtual void Release(); // the same if you remove a pointer to this.
    // AND: it should be called instead of delete.
    int GetRefcount() const; // get refcount. Use only for Debgging purposes, never base any decisions on it.

    virtual void ReleaseOwnership(); // release the object only if it was created on this machine
    virtual void TakeOwnership(); // treat an object like it was created locally
    bool Owned(){
        return createdLocally;    //!< returns whether the object is owned by this machine
    }

    nObserver& GetObserver() const;    // retunrs the observer of this object

    void ClearCache() const; //!< clears the message cache, call on object changes

    virtual void Dump( tConsole& con ); // dumps object stats

    static unsigned short ID(nNetObject const *pThis)
    {
        if (pThis)
            return pThis->id;
        else
            return 0;
    }

    unsigned short ID() const{
        tASSERT_THIS();
        return id;
    }

    static unsigned short Owner(nNetObject const *pThis)
    {
        if (pThis)
            return pThis->owner;
        else
            return ::sn_myNetID;
    }

    unsigned short Owner() const{
        tASSERT_THIS();
        return owner;
    }

    inline nMachine & GetMachine() const;  //!< returns the machine this object belongs to

    nNetObject(int owner=-1); // sn_netObjects can be normally created on the server
    // and will
    // send the clients a notification that
    // follows exaclty the same format as the sync command,
    // but has a different descriptor (the one from CreatorDescriptor() )
    // and the id and owner are sent, too.

    // owner=-1 means: this object belongs to us!

    virtual void InitAfterCreation(); // after remote creation,
    // this routine is called

    // for the internal works, don't call them
    //  static void RegisterRegistrar( nNetObjectRegistrar& r );	// tell the basic nNetObject constructor where to store itself
    void Register( const nNetObjectRegistrar& r );    // register with the object database
protected:
    virtual ~nNetObject();
    // if called on the server, the destructor will send
    // destroy messages to the clients.

    // you normally should not call this

    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to
public:

    // what shall we do if the owner quits the game?
    // return value: should this object be destroyed?
    virtual bool ActionOnQuit(){
        return true;
    }

    // what shall we do if the owner decides to delete the object?
    virtual void ActionOnDelete(){
    }

    // should every other networked computer be informed about
    // this objects existance?
    virtual bool BroadcastExistence(){
        return true;
    }

    // print out an understandable name in to s
    virtual void PrintName(tString &s) const;

    // indicates whether this object is created at peer user.
    bool HasBeenTransmitted(int user) const;
    bool syncRequested(int user) const{
        return knowsAbout[user].syncReq;
    }

    // we must not transmit an object that contains pointers
    // to non-transmitted objects. this function is supposed to check that.
    virtual bool ClearToTransmit(int user) const;

    // turn an ID into an object pointer
    template<class T>  static void IDToPointer( unsigned short id, T * & p )
    {
        if ( 0 != id )
            p = dynamic_cast<T*> ( nNetObject::ObjectDangerous(id) );
        else
            p = NULL;
    }

    template<class T> static void IDToPointer( unsigned short id, tControlledPTR<T> & p )
    {
        if ( 0 != id )
            p = dynamic_cast<T*> ( nNetObject::ObjectDangerous(id) );
        else
            p = NULL;
    }

    template<class T> static void IDToPointer( unsigned short id, tJUST_CONTROLLED_PTR<T> & p )
    {
        if ( 0 != id )
            p = dynamic_cast<T*> ( nNetObject::ObjectDangerous(id) );
        else
            p = NULL;
    }

    template<class T> static void IDToPointer( unsigned short id, nObserverPtr<T> & p )
    {
        if ( 0 != id )
            p = dynamic_cast<T*> ( nNetObject::ObjectDangerous(id) );
        else
            p = NULL;
    }

    // turn an object pointer int an ID
    template<class T>  static unsigned short PointerToID( T * p )
    {
        if ( !p )
        {
            return 0;
        }
        else
        {
            return p->ID();
        }
    }

    // turn an object pointer int an ID
    template<class T>  static unsigned short PointerToID( T const & p )
    {
        if ( !p )
        {
            return 0;
        }
        else
        {
            return p->ID();
        }
    }

    //! returns a creation message for this object
    nProtoBufMessageBase * CreationMessage();

    //! returns a sync message for this object
    nProtoBufMessageBase * SyncMessage();

    //! creates a netobject form sync data
    nNetObject( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Network::NetObjectSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Network::NetObjectSync const & sync, nSenderInfo const & sender );

    //! returns the user that the current WriteSync() messages are for
    static int SyncedUser();

    //! returns the descriptor responsible for this class
    inline nNetObjectDescriptorBase const & GetDescriptor() const { return DoGetDescriptor(); }
private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const = 0;

    // control functions:

protected:
    Network::NetObjectControl & BroadcastControl();
    // creates a new control message that can be used to control other
    // copies of this nNetObject; control is received with ReceiveControlNet().
    // It is automatically broadcast (sent to the server in client mode).

private:
    // conversion for stream legacy control messages.
    virtual void StreamControl( Network::NetObjectControl const & control, nStreamMessage & stream );
    virtual void UnstreamControl( nStreamMessage & stream, Network::NetObjectControl & control );

    // easier to implement conversion helpers: just extract the relevant sub-protbuf.
    virtual nProtoBuf       * ExtractControl( Network::NetObjectControl       & control );
    virtual nProtoBuf const * ExtractControl( Network::NetObjectControl const & control );
public:

    virtual void ReceiveControlNet( Network::NetObjectControl const & control );
    // receives the control message. the data written to the message created
    // by *NewControlMessage() can be read directly from m.

    // shall the server accept sync messages from the clients?
    virtual bool AcceptClientSync() const;

    void GetID();			// get a network ID
    void RequestSync(bool ack=true);  // request a sync
    void RequestSync(int user,bool ack); // only for a single user

    // global functions:

    static void SyncAll();
    // on the server, this will send sync tEvents to all clients
    // for as many sn_netObjects as possible (currently: simply all...)

    static void ClearAll();
    // this reinits the list of sn_netObjects. If called on the server,
    // the clients are cleared, too.

    static void ClearAllDeleted();
    // this reinits the list of deleted Objects.

    static void ClearKnows(int user, bool clear);

    //give the sn_netObjects new id's after connecting to a server
    static void RelabelOnConnect();
};

struct nNetObjectRegistrar
{
    nNetObject * object;
    unsigned short sender;
    unsigned short id;
    nNetObjectRegistrar* oldRegistrar;

    nNetObjectRegistrar();
    ~nNetObjectRegistrar();
};

// the list of netobjects for better reference
extern tArray<tJUST_CONTROLLED_PTR<nNetObject> > sn_netObjects;

// deletes the knowleEdge information of all sn_netObjects for user user
void ClearKnows(int user, bool clear = false);

void Cheater(int user);

extern tArray<unsigned short> sn_netObjectsOwner;

// ************************************************************************************
// *
// *	GetMachine
// *
// ************************************************************************************
//!
//!		@return		the machine this NetObject belongs to
//!
// ************************************************************************************

nMachine & nNetObject::GetMachine( void ) const
{
    return DoGetMachine();
}

#endif

