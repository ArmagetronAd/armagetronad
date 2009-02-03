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
class nOProtoBufDescriptorBase;
class nProtoBufDescriptorBase;

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

    int syncListID_;                                 // ID for the list of objects to sync
public:
    struct nKnowsAboutInfo{
    public:
    bool knowsAboutExistence:
        1; // is the creation message through?
    bool nextSyncAck:
        1;         // should the next sync message wait
        // for it's ack?
    bool syncReq:
        1;              // should a sync message be sent?
    unsigned char  acksPending:
        4;          // how many messages are underway?

        nKnowsAboutInfo(){
            memset(this, 0, sizeof(nKnowsAboutInfo) );
            Reset();
            syncReq=false;
        }

        void Reset(){
            knowsAboutExistence=false;
            nextSyncAck=true;
            syncReq=true;
            acksPending=0;
        }
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

    virtual void Dump( tConsole& con ); // dumps object stats

    unsigned short ID() const{
        if (this)
            return id;
        else
            return 0;
    }

    unsigned short Owner() const{
        if (this)
            return owner;
        else
            return ::sn_myNetID;
    }

    inline nMachine & GetMachine() const;  //!< returns the machine this object belongs to

    virtual nDescriptor& CreatorDescriptor() const;

    nNetObject(int owner=-1); // sn_netObjects can be normally created on the server
    // and will
    // send the clients a notification that
    // follows exaclty the same format as the sync command,
    // but has a different descriptor (the one from CreatorDescriptor() )
    // and the id and owner are sent, too.

    // owner=-1 means: this object belongs to us!


    nNetObject(nMessage &m); // or, if initially created on the
    // server, via a creation nMessage on the client.

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

    // syncronisation functions (old):
    virtual void WriteSync(nMessage &m); // store sync message in m
    virtual void ReadSync(nMessage &m); // guess what

    // new
    virtual void WriteSync(nMessage &m, int run ); // store sync message in m
    virtual void ReadSync(nMessage &m, int run ); // guess what

    virtual bool SyncIsNew(nMessage &m); // is the pure sync message newer than the last accepted sync

    // the extra information sent on creation (old):
    virtual void WriteCreate(nMessage &m); // store sync message in m
    // the information written by this function should
    // be read from the message in the "message"- connstructor

    // new
    virtual void WriteCreate(nMessage &m, int run );
    virtual void ReadCreate(nMessage &m, int run );

    // the "run" parameter in the new versions is intended to fix the
    // compatibility problems when you want to extend the data written in
    // WriteCreate(). The old method of writing a creation message was
    //
    // WriteCreate(m); WriteSync(m);
    // and it was read by
    // Constructor(m); ReadSync(m);
    //
    // the new sequence of calls is
    // WriteCreate(m,0); WriteSync(m,0); WriteCreate(m,1),WriteSync(m,1)...
    // and the new read sequence is
    // Constructor(m); ReadSync(m,0), ReadCreate(m,1), ReadSync(m,1)...
    // where the write operation continues until no data was written and
    // the read operation continues until no data was read.
    // the default of the new functions is to call the old ones if "run"
    // equals to 0 and do nothing otherwise.

    // these functions handle the process. Note that they are non-virtual.
    nMessageBase * WriteAll();
    void WriteAll( nStreamMessage & message, bool create );
    void ReadAll ( nStreamMessage & message, bool create );

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

    // even better new stuff: protocol buffers. The functions are non-virtual; they
    // get called over the descriptor:
    //! creates a netobject form sync data
    nNetObject( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Network::NetObjectSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Network::NetObjectSync const & sync, nSenderInfo const & sender );

    //! returns the descriptor responsible for this class
    inline nOProtoBufDescriptorBase const * GetDescriptor() const { return DoGetDescriptor(); }
private:
    //! returns the descriptor responsible for this class
    virtual nOProtoBufDescriptorBase const * DoGetDescriptor() const;

    // control functions:

protected:
    //! returns the user that the current WriteSync() is intended for
    static int SyncedUser();

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


// create one of those for every new class of sn_netObjects you define.
// you can then remotely spawn other T's
// by sending netpackets of type net_initialisator<T>.desc
// (correctly initialised, of course...)

template<class T> class nNOInitialisator:public nDescriptor{
    // create a new nNetObject
    static void Init(nMessage &m){
        try
        {
            if (m.DataLen()<2)
            {
                nReadError();
            }

            unsigned short id=m.Data(0);
            //unsigned short owner=m.data(1);

            if (sn_netObjectsOwner[id]!=m.SenderID() || bool(sn_netObjects[id]))
            {
#ifdef DEBUG
                st_Breakpoint();
                if (!sn_netObjects[id])
                {
                    con << "Netobject " << id << " is already reserved!\n";
                }
                else
                {
                    con << "Netobject " << id << " is already defined!\n";
                }
#endif
                if (sn_netObjectsOwner[id]!=m.SenderID())
                {
                    Cheater(m.SenderID());
                    nReadError();
                }
            }
            else
            {
                nNetObjectRegistrar registrar;
                //			nNetObject::RegisterRegistrar( registrar );
                tJUST_CONTROLLED_PTR< T > n=new T(m);
                n->InitAfterCreation();
                nNetObject * no = n;
                no->ReadAll(m,true);
                n->Register( registrar );

#ifdef DEBUG
                /*
                tString str;
                n->PrintName( str );
                con << "Received object " << str << "\n";
                */
#endif

                if (sn_GetNetState()==nSERVER && !n->AcceptClientSync())
                {
                    Cheater(m.SenderID()); // cheater!
                    n->Release();
                }
                else if ( static_cast< nNetObject* >( sn_netObjects[ n->ID() ] ) != n )
                {
                    // object was unable to be registered
                    n->Release(); // silently delete it.
                }
            }
        }
        catch (nKillHim)
        {
            con << "nKillHim signal caught.\n";
            Cheater(m.SenderID());
        }
    }

public:
    //nDescriptor desc;

    //  nNOInitialisator(const char *name):nDescriptor(init,name){};
    nNOInitialisator(unsigned short id,const char *name):nDescriptor(id,Init,name){};
};

// nonmember pointer read operators.
template<class T> nMessage& operator >> ( nMessage& m, T*& p )
{
    unsigned short id;
    m.Read(id);

    nNetObject::IDToPointer( id, p );

    return m;
}

template<class T> nMessage& operator >> ( nMessage& m, tControlledPTR<T>& p )
{
    unsigned short id;
    m.Read(id);

    nNetObject::IDToPointer( id, p );

    return m;
}

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

