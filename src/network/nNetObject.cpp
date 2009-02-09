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


#include "tMemManager.h"
#include "nNetObject.h"
#include "tLocale.h"
//#include "nNet.h"
#include "nSimulatePing.h"
#include "tSysTime.h"
#include "tToDo.h"
#include "nObserver.h"
#include "nConfig.h"
#include "tRecorder.h"

#include "nProtoBuf.h"
#include "nBinary.h"
#include "nNetObject.pb.h"
#include "nNetObjectPrivate.pb.h"

#include <deque>
#include <set>
#include <map>

// debug watchs
#ifdef DEBUG
int sn_WatchNetID = 0;
extern nMessageBase* sn_WatchMessage;
#endif

tDEFINE_REFOBJ( nNetObject )

// max ping to equalize;
int sn_pingCharityServer=0;

// first, we need a mechanism to distribute the nNetObject id's
// among the clients and the server.

// only used on the server: the next free id.
static unsigned short net_current_id=1;


// the maximal value of net_max_current_id
static const unsigned short net_max_current_id_max = 32000;
// the minimal value of net_max_current_id
//#ifdef DEBUG
//static const unsigned short net_max_current_id_min = 1000;
//#else
static const unsigned short net_max_current_id_min = 16000;
//#endif

// the maximal ID we currently want to assign
static unsigned short net_max_current_id = net_max_current_id_min;

#ifdef DEBUG
static void sn_BreakOnObjectID( unsigned int id )
{
#ifdef DEBUG_X
    static unsigned int breakOnID = sn_WatchNetID;
    static REAL minTime = 0;
    if ( id == breakOnID  && tSysTimeFloat() > minTime )
        st_Breakpoint();
#endif
}

static void sn_BreakOnObject( nNetObject * object )
{
    if ( object )
        sn_BreakOnObjectID( object->ID() );
}
#endif

static unsigned short inc_id(){
    unsigned short ret=net_current_id++;

    if ( net_current_id > net_max_current_id )
    {
        net_current_id = 1;
    }

    if (net_current_id==0)
        net_current_id++;
    return ret;
}
// the number of ID's that is to be requested from the server
#define ID_PREFETCH 50

// the requested id FIFO
unsigned short net_reserved_id[ID_PREFETCH];
unsigned short distribute=0,request=0;


// the reserved id's are stored here
tArray<unsigned short> sn_netObjectsOwner;
// so the server knows who owns wich id

// queue of recently freed IDs ready for reuse
static std::deque<nNetObjectID> sn_freedIDs;

// stored information needed when the objects are destroeyed
static tArray<bool> sn_netObjects_AcceptClientSync;
// and the function AcceptClientSync() is no longer valid (bummer.)

tArray<tJUST_CONTROLLED_PTR<nNetObject> > sn_netObjects(1024);

static const REAL nDeletedTimeout = 60.0f;

struct nDeletedInfo
{
    tJUST_CONTROLLED_PTR<nNetObject> object_;	// deleted object
    nTimeAbsolute time_;						// time it was deleted

    nDeletedInfo()
    {
        this->UnSet();
    }

    void Set( nNetObject* object )
    {
#ifdef DEBUG
        sn_BreakOnObject( object );
#endif
        time_ = tSysTimeFloat();
        object_ = object;
    }

    void UnSet( )
    {
        time_ = - nDeletedTimeout * 2.0f;
        object_ = NULL;
    }
};

// info about deleted objects, sorted by their former ID
typedef std::map< nNetObjectID, nDeletedInfo > nDeletedInfos;
static nDeletedInfos sn_netObjectsDeleted;

static bool free_server( nNetObjectID id )
{
    if ( bool(sn_netObjectsOwner[id]) || bool(sn_netObjects[id]) )
    {
        return false;
    }

    nDeletedInfos::iterator found = sn_netObjectsDeleted.find( id );
    if ( found != sn_netObjectsDeleted.end() )
    {
        nDeletedInfo  & deleted = (*found).second;
        if ( deleted.time_ > tSysTimeFloat() - nDeletedTimeout )
        {
            return false;
        }

        if ( deleted.object_ )
        {
#ifdef DEBUG
            sn_BreakOnObjectID( id );
#endif
            // clear the deletion info, but don't reuse the ID just yet
            deleted.UnSet();
            return false;
        }
    }

    return true;
}

static unsigned short next_free_server_nokill(){
    // slowly decrease max user ID
    if ( net_max_current_id > net_max_current_id_min )
    {
        net_max_current_id--;
        if ( net_current_id > net_max_current_id )
            net_current_id = 1;
    }

    // recycle old used IDs
    while ( sn_freedIDs.size() > 1000 && sn_freedIDs.size() * 20 > net_max_current_id )
    {
        nNetObjectID freedID = sn_freedIDs.front();
        sn_freedIDs.pop_front();
#ifdef DEBUG
        sn_BreakOnObjectID( freedID );
#endif
        sn_netObjectsOwner[freedID] = 0;
    }

    nNetObjectID start_id = net_current_id;
    if (sn_GetNetState()==nSERVER || sn_GetNetState()==nSTANDALONE)
    {
        do
        {
            inc_id();
        }
        while ( !free_server( net_current_id ) && net_current_id != start_id );

        if ( net_current_id != start_id )
        {
            // no problem!
#ifdef DEBUG
            sn_BreakOnObjectID( net_current_id );
#endif
            return net_current_id;
        }
        else
        {
            // we ran out of IDs.
            return 0;
        }
    }

    else
    {
        tERR_ERROR("next_free_server is not available for clients.");
        return 0;
    }
}

// kills the guy with the most registered IDs
static int kill_id_hog()
{
    int i;
    int grabbedIDs[MAXCLIENTS+2];
    int usedIDs[MAXCLIENTS+2];

    for ( i = MAXCLIENTS+1; i>=0; --i )
    {
        grabbedIDs[i] = 0;
        usedIDs[i] = 0;
    }

    // find out how many IDs a user has reserved with or without using them
    for ( i = sn_netObjectsOwner.Len()-1; i>=0; --i )
    {
        int owner = sn_netObjectsOwner( i );
        if ( owner >= 0 && owner <= MAXCLIENTS )
        {
            if ( sn_netObjects[i] )
            {
                usedIDs[ owner ] ++;
            }
            else if ( owner > 0 )
            {
                grabbedIDs[ owner ] ++;
            }
        }
    }

    // find the user with most used/grabbed IDs
    int maxGrabbed = 2 * ID_PREFETCH + 1;
    int maxUsed = 0;
    int maxGrabbedUser = -1;
    int maxUsedUser = -1;

    for ( i = MAXCLIENTS+1; i > 0; --i )
    {
        if ( grabbedIDs[i] > maxGrabbed )
        {
            maxGrabbedUser = i;
            maxGrabbed = grabbedIDs[i];
        }

        if ( usedIDs[i] > maxUsed )
        {
            maxUsedUser = i;
            maxUsed = usedIDs[i];
        }
    }

    // kick the top grabber
    if ( maxGrabbedUser > 0 )
    {
        con << "Killing top ID grabber.\n";
        st_Breakpoint();
        sn_DisconnectUser( maxGrabbedUser, "$network_kill_maxidgrabber" );
        return maxGrabbedUser;
    }

    // kick the top user
    else if ( maxUsedUser > 0 )
    {
        con << "Killing top ID user.\n";
        st_Breakpoint();
        sn_DisconnectUser( maxUsedUser, "$network_kill_maxiduser" );
        return maxUsedUser;
    }

    return -1;
}

// wrapper for toto tools
static bool sg_todoKillHog=false;
static void kill_id_hog_todo()
{
    if ( sg_todoKillHog )
        kill_id_hog();
    sg_todoKillHog=false;
}

static unsigned short next_free_server( bool kill ){
    nNetObjectID id = next_free_server_nokill();
    if ( id > 0 )
    {
        return id;
    }
    else
    {
        // plan to kill the worst ID user
        sg_todoKillHog=true;
        st_ToDo( kill_id_hog_todo );

        // increase ID limit
        int increase = 1000;
        if ( net_max_current_id + 2 * increase < net_max_current_id_max )
            net_max_current_id += increase;
        else
            net_max_current_id = net_max_current_id + ( net_max_current_id_max - net_max_current_id );

        id = next_free_server_nokill();
        if ( id > 0 )
        {
            return id;
        }
        else
        {
            // throw an error
            if ( kill )
            {
                nReadError( true );
            }

            // or just exit silently
            con << "Emergency exit: desperately ran out of IDs.\n";
            exit(-1);
        }
    }
}

static void sn_GrantIDsHandler( Network::GrantIDs const & ids, nSenderInfo const & sender )
{
    unsigned short stop = distribute;
    if (distribute == 0)
        stop = ID_PREFETCH;

    if ( sn_GetNetState()==nSERVER )
    {
        Cheater( sender.SenderID() );
    }
    else{
        for( int i = ids.blocks_size() - 1; i >= 0; --i )
        {
            Network::IDBlock const & block = ids.blocks(i);

            unsigned short id = block.start();
            unsigned short count = block.length();

            for (unsigned short i=id + count - 1; i>= id && request+1 != stop; i--)
            {
                if (sn_netObjects[i])
                {
                    con << "Warning! Network id receive error on ID " << i << " belonging to client " << sn_netObjects[i]->Owner() << "\n";
                    con << "while recieving ID block " << id << "-" << id+count-1 << " from netmessage " << sender.MessageID() << ".\n";

                }
                else
                {
                    net_reserved_id[request] = i;
#ifdef DEBUG
                    // con << "got id " << net_reserved_id[request] << '\n';
#endif
                    request++;
                    if (request>=ID_PREFETCH)
                        request=0;
                }
            }
        }
    }
}

nProtoBufDescriptor< Network::GrantIDs > sn_grantIDsDescriptor( 20, sn_GrantIDsHandler, "req_id" );

void sn_RequestIDsHandler( Network::RequestIDs const & request, nSenderInfo const & sender )
{
    // Add security: keep clients from fetching too many ids

    if (sn_GetNetState()==nSERVER && sender.SenderID()<=MAXCLIENTS)
    {
        unsigned short num = request.num();
        
        // not too many requests accepted. kick violators.
        if ( num > ID_PREFETCH*4 )
        {
            nReadError( true );
        }
        
        Network::GrantIDs & grant = sn_grantIDsDescriptor.Send( sender.SenderID() );
        
        unsigned short begin_block=0;	// begin of the block of currently assigned IDs
        unsigned short block_len=0;		// and it's length
        
        for (int i = num-1; i>=0; i--)
        {
            nNetObjectID id = next_free_server(true);
            
            sn_netObjectsOwner[id] = sender.SenderID();
            
            if (begin_block + block_len == id)	// RLE for allocated IDs
                block_len++;
            else
            {
                if (block_len > 0)
                {
                    //		      con << "Assigning block " << begin_block << " - " << begin_block + block_len - 1 << "\n";
                    Network::IDBlock & block = *grant.add_blocks();
                    block.set_start( begin_block );
                    block.set_length( block_len );
                }
                begin_block = id;
                block_len = 1;
            }
        }
        if (block_len > 0)
        {
            //	      con << "Assigning block " << begin_block << " - " << begin_block + block_len - 1 << "\n";
            Network::IDBlock & block = *grant.add_blocks();
            block.set_start( begin_block );
            block.set_length( block_len );
        }
    }
}

nProtoBufDescriptor< Network::RequestIDs > sn_requestIDsDescriptor( 21, sn_RequestIDsHandler );

unsigned short next_free(){
    unsigned short ret=0;

    do{
        if (sn_GetNetState()==nCLIENT){
            unsigned short need_soon = request + ID_PREFETCH - distribute;
            if (need_soon > ID_PREFETCH)
                need_soon -= ID_PREFETCH;
            if (need_soon < (ID_PREFETCH >> 1))
            {
                // request some IDs
                Network::RequestIDs & request = sn_requestIDsDescriptor.Send(0);
                request.set_num( ID_PREFETCH >> 2 );
            }

            double timeout=tSysTimeFloat()+60;
            while (sn_Connections[0].socket && distribute==request && tSysTimeFloat()<timeout){
                // wait for new ids from the server
#ifdef DEBUG
                // con << distribute << ":" << request << '\n';
#endif
                sn_Receive();
                nNetObject::SyncAll();
                sn_SendPlanned();
                //	st_Breakpoint();
                tAdvanceFrame(1000000);
            }
            if (tSysTimeFloat()>=timeout)
                tERR_ERROR_INT("Not enough nNetObject IDs to distribute. Sorry!\n");

            ret=net_reserved_id[distribute];

            distribute++;
            if (distribute>=ID_PREFETCH)
                distribute=0;
            //      con << "used id " << ret << '\n';
            net_current_id=ret+1;
        }
        else
        {
            ret=next_free_server(false);
        }

        if (sn_netObjects[ret]){
            con << "Warning! Network id assignment error on ID " << ret << " belonging to client " << sn_netObjects[ret]->Owner() << "\n";
            ret=0;
        }

    }while (ret==0 && sn_Connections[0].socket);

    return ret;
}

void first_fill_ids(){
    if (sn_GetNetState()!=nCLIENT)
        tERR_ERROR("first_fill_ids is only for clients!");

    distribute=request=0;

    // request some IDs
    Network::RequestIDs & request = sn_requestIDsDescriptor.Send(0);
    request.set_num( ID_PREFETCH - 10 );
}


void Cheater(int i)
{
    // anything to do at all?
    if (i != 0 && !sn_Connections[i].socket)
    {
        return;
    }

    con << "User " << i << " tried to cheat.\n";
    //	st_Breakpoint();

    if ( i == 0 )
        // terminate connection to server
        throw tGenericException("There was a network error, the connection to the server had to be terminated.", "Network Error");
    else
        sn_DisconnectUser(i, "$network_kill_cheater" );
}


/*
nDescriptor& nNetObject::CreatorDescriptor() const{
  return nNetObject_initialisator.desc;
}
*/

void nNetObject::AddRef(){
    tASSERT ( this );

    if ( this )
    {
        tASSERT( refCtr_ >= 0 );
        refCtr_++;
        tASSERT( refCtr_ >= 0 );
    }
}

void nNetObject::ReleaseOwnership(){
    if ( this->createdLocally )
    {
        this->createdLocally = false;
        Release();
    }
}

void nNetObject::TakeOwnership(){
    if ( !this->createdLocally )
    {
        this->createdLocally = true;
        //		AddRef();
    }
}

void nNetObject::Release(){
    tASSERT( this );

    if (this){
        if (refCtr_>0)
            refCtr_--;
        else
        {
#ifdef DEBUG
            tERR_ERROR("Negative recfcount!");
#else
            return;
#endif
        }
        int extra=0;

        // account for the reference held by the creator of the object

        // only if the object is validly entered in our object-array
        if (id > 0 && static_cast<nNetObject*>(sn_netObjects[id])==this)
        {
            if ( createdLocally )
                extra = -1;
        }
        //    else
        //      extra = -1;

        if ( refCtr_ + extra <= 0 )
        {
            refCtr_ = -100;
            delete this;
        }
    }
}

// get refcount. Use only for Debgging purposes, never base any decisions on it.
int nNetObject::GetRefcount() const
{
    int extra=0;

    // account for the reference held by the creator of the object

    // only if the object is validly entered in our object-array
    if (id > 0 && static_cast<nNetObject*>(sn_netObjects[id])==this)
    {
        if ( createdLocally )
            extra = -1;
    }
    //    else
    //      extra = -1;

    return this->refCtr_ + extra;
}

nObserver& nNetObject::GetObserver() const
{
    if ( !this->observer_ )
    {
        this->observer_ = tNEW( nObserver );
        this->observer_->SetObject( this );
    }

    return *this->observer_;
}

void nNetObject::ClearCache() const
{
    lastSync_ = 0;
    lastCreate_ = 0;
}

// dumps object stats
void nNetObject::Dump( tConsole& con )
{
    tString str;
    this->PrintName( str );
    con << str;
}

// checks whether n is newer than old (exploits integer overflow)
bool sn_Update(unsigned short &old,unsigned short n){
    if ( 0 == old )
    {
        old = n;
        return true;
    }

    short diff=old-n;
    if (diff<0){
        old=n;
        return true;
    }
    else
        return false;
}

// same for longs
bool sn_Update(unsigned long &old,unsigned long n){
    if ( 0 == old )
    {
        old = n;
        return true;
    }

    long diff=old-n;
    if (diff<0){
        old=n;
        return true;
    }
    else
        return false;
}

nNetObject::nNetObject(int own):lastSyncID_(0),
id(0),refCtr_(0),owner(own){
#ifdef DEBUG
    //con << "Netobject " << id  << " created.\n";
    //  if (id == 383)
    //   st_Breakpoint();
#endif
    syncListID_ = -1;

    createdLocally = true;

    if (own<0) owner=::sn_myNetID;
}

static nNetObjectRegistrar* sn_Registrar = NULL;

nNetObjectRegistrar::nNetObjectRegistrar()
{
    sender = 100;
    id = 0;
    oldRegistrar = sn_Registrar;
    sn_Registrar = this;
}

nNetObjectRegistrar::~nNetObjectRegistrar()
{
    tASSERT( sn_Registrar == this );
    sn_Registrar = oldRegistrar;
}

// gegister with the object database
void nNetObject::Register( const nNetObjectRegistrar& registrar )
{
    tASSERT( this == registrar.object );
    tASSERT( id == 0 || id == registrar.id );

    if ( this->id == registrar.id )
    {
        return;
    }

    id = registrar.id;

    if (sn_netObjectsOwner[id]!= registrar.sender || sn_netObjects[id]){
#ifdef DEBUG
        con << "Netobject " << id << " is already reserved!\n";
#endif
        if (sn_netObjectsOwner[id]!=registrar.sender){
            Cheater( registrar.sender );
            nReadError();
        }
    }
    else
    {
#ifdef DEBUG
        sn_BreakOnObjectID( id );
#endif
        sn_netObjects[id]=this;
    }

    if (sn_GetNetState()!=nCLIENT)
        owner=registrar.sender; // to make sure noone is given a nNetObject from
    // someone else.

    sn_netObjectsOwner[id]=owner;
    sn_netObjects_AcceptClientSync[id]=false;
}

void nNetObject::DoBroadcastExistence(){
    if (BroadcastExistence() &&
            ( sn_GetNetState()!=nCLIENT ||
              ( owner == ::sn_myNetID && AcceptClientSync() )
            )
       )
    {
        int maxUser = (sn_GetNetState() == nSERVER) ? MAXCLIENTS : 0;
        int minUser  = (sn_GetNetState() == nSERVER) ? 1 : 0;
        for (int user = maxUser; user >= minUser; --user)
        {
            // sync the object only to users that don't know about it yet
            if ( !knowsAbout[user].knowsAboutExistence && sn_Connections[user].socket ) // && !knowsAbout[user].syncReq )
                RequestSync( user, true );
        }
    }
}


struct nDestroyInfo
{
    unsigned short id;
    unsigned short sender;
    bool actionOnDeleteExecuted;
    nTimeAbsolute timeout;
};

static tArray< nDestroyInfo > sn_Destroyed; // accepted destroy messages to be executed later
static std::set< unsigned short > sn_LocallyDestroyedIDs; // IDs of locally destroyed objects

// returns whether the given ID belongs to a locally deleted object and deletes the entry
bool sn_WasDeletedLocally( unsigned short id )
{
    // delete local destruction log, if there was any
    std::set< unsigned short >::iterator found = sn_LocallyDestroyedIDs.find(id);
    if ( found != sn_LocallyDestroyedIDs.end() )
    {
        // yes. Ignore the message and remove the local destroy thing.
        sn_LocallyDestroyedIDs.erase( found );
        return true;
    }

    return false;
}

void nNetObject::InitAfterCreation(){
    DoBroadcastExistence();
    // con << "InitAfterCreation\n";

#ifdef DEBUG
    sn_BreakOnObjectID( id );
#endif

    // it just got created, all local deletions must be fakes
    sn_WasDeletedLocally( id );
} // after remote creation,


static void sn_DestroyObjectsHandler( Network::DestroyObjects const & destroy, nSenderInfo const & sender )
{
    //con << "destroy begin\n";
    for( int i = destroy.ids_size()-1; i >= 0; --i )
    {
        unsigned short id = destroy.ids(i);
#ifdef DEBUG
        sn_BreakOnObjectID( id );
#endif
        // see if there was a local destruction; if yes, ignore.
        if (sn_WasDeletedLocally( id ))
            continue;

        nDestroyInfo& info = sn_Destroyed[ sn_Destroyed.Len() ];
        info.id = id;
        info.sender = sender.SenderID();
        info.actionOnDeleteExecuted=false;
        info.timeout=tSysTimeFloat()+nDeletedTimeout;

        // notify object of pending deletion
        if (nNetObject *no=sn_netObjects[id])
        {
            tASSERT( !no->Owned() );

            no->ActionOnDelete();
            info.actionOnDeleteExecuted=true;

            // if the object is now suddenly owned by this machine, there must be a reason.
            // undo the deletion. Whoever did this has now the responsibility for the object.
            if ( no->Owned() )
                sn_Destroyed.SetLen( sn_Destroyed.Len() - 1 );
        }

#ifdef DEBUG
        //count ++;
        //con << count;
        //con << " destroying object " << id << " by remote order.\n";
#endif

    }
    //con << "destroy end.\n";
}

static void sn_DoDestroy()
{
#ifdef DEBUG
    static bool recursion = false;
    tASSERT( !recursion );
    recursion = true;
#endif

    for ( int i = sn_Destroyed.Len()-1 ; i>=0; --i )
    {
        nDestroyInfo& info = sn_Destroyed( i );
        unsigned short id = info.id;

        // check if the message timed out
        bool timedOut = ( info.timeout < tSysTimeFloat() );

        // get object
        nNetObject *no=sn_netObjects[id];

        // destroy it!
        if (bool(no) || timedOut){
#ifdef DEBUG
            sn_BreakOnObjectID( id );
#endif
            // see if there was a local destruction; if yes, ignore.
            if (!sn_WasDeletedLocally( id ) && !timedOut )
            {
                if (no->Owner()==info.sender || info.sender==0){
                    sn_netObjectsDeleted [ id ].Set( no );

                    if (!info.actionOnDeleteExecuted)
                        no->ActionOnDelete();

                    sn_netObjects(id)=NULL;
                    sn_netObjectsOwner(id)=0;
                }
                else
                    Cheater(info.sender);
            }

            // delete message by overwriting it with the last message in the array
            int last = sn_Destroyed.Len()-1;
            info = sn_Destroyed( last );
            sn_Destroyed.SetLen( last );
        }
    }
#ifdef DEBUG
    recursion = false;
#endif
}

static nCallbackReceivedComplete sn_ReceivedComplete( sn_DoDestroy );

/*
// tell the basic nNetObject constructor where to store itself
void nNetObject::RegisterRegistrar( nNetObjectRegistrar& r )
{
	sn_Registrar = &r;
}
*/

static nProtoBufDescriptor< Network::DestroyObjects > sn_destroyObjectsDescriptor( 22, sn_DestroyObjectsHandler );

static tJUST_CONTROLLED_PTR< nProtoBufMessage< Network::DestroyObjects > > destroyers[MAXCLIENTS+2];
static REAL                             destroyersTime[MAXCLIENTS+2];

// list of netobjects that have a sync request running
static tList< nNetObject > sn_SyncRequestedObject;

nNetObject::~nNetObject(){
    // remove from sync list
    if ( syncListID_ >= 0 )
        sn_SyncRequestedObject.Remove( this, syncListID_ );

    // release observer
    if ( this->observer_ )
    {
        this->observer_->SetObject( NULL );
    }

#ifdef DEBUG
    int extra=0;

    // account for the reference held by the creator of the object

    // only if the object is validly entered in our object-array
    if (id > 0 && static_cast<nNetObject*>(sn_netObjects[id])==this)
    {
        if ( createdLocally )
            extra = -1;
    }

    if (refCtr_ + extra > 0)
        tERR_ERROR("Hey! There is stil someone interested in this nNetObject!\n");
    //con << "Netobject " << id  << " deleted.\n";
#endif

    // mark id as soon-to-be free
    if (id)
    {
#ifdef DEBUG
        sn_BreakOnObjectID( id );
#endif
        if ( sn_GetNetState() == nSERVER )
        {
            sn_netObjectsOwner[id]=0xFFFF;
            sn_freedIDs.push_back( id );
        }
        else
        {
            sn_netObjectsOwner[id]=0;
        }
    }

    // determine whether a destroy message should be sent: the server definitely should...
    bool sendDestroyMessage = (sn_GetNetState()==nSERVER);
    if (!sendDestroyMessage && id && sn_netObjects_AcceptClientSync[id] && owner==sn_myNetID)
    {
        // ... but also the client if the object belongs to it.
        sendDestroyMessage = true;

        // the server will send a response destroy message later. We need to ignore it.
        sn_LocallyDestroyedIDs.insert(id);
    }

    // send it
    if ( sendDestroyMessage )
    {
        int idsync=id;
        tRecorderSync< int >::Archive( "_NETOBJECT_DESTROYED", 3, idsync );

        // con << "Destroying object " << id << '\n';
        for(int user=MAXCLIENTS;user>=0;user--){
            if( ( user!=sn_myNetID && knowsAbout[user].knowsAboutExistence ) ||
                    knowsAbout[user].acksPending){
                if (destroyers[user]==NULL)
                {
                    destroyers[user] = sn_destroyObjectsDescriptor.CreateMessage();
                    destroyersTime[user]=tSysTimeFloat();
                }
                destroyers[user]->AccessProtoBuf().add_ids(id);

                if (destroyers[user]->AccessProtoBuf().ids_size() > 100){
                    destroyers[user]->Send(user);
                    destroyers[user]=NULL;
                }

#ifdef DEBUG
                //con << "remotely destroying object " << id << '\n';
#endif
            }
        }
    }
    refCtr_=100;
    if (id)
    {
        if (this == sn_netObjects[id])
        {
#ifdef DEBUG
            sn_BreakOnObjectID( id );
#endif
            sn_netObjects[id] = NULL;
        }

        // clear object from info arrays
        nDeletedInfos::iterator found = sn_netObjectsDeleted.find( id );
        if ( found != sn_netObjectsDeleted.end() )
        {
            nDeletedInfo  & deleted = (*found).second;
            deleted.Set( NULL );
            sn_netObjectsDeleted.erase(found);
        }
    }

    refCtr_=-100;
    tCHECK_DEST;
}


nNetObject::nKnowsAboutInfo::~nKnowsAboutInfo(){}

nNetObject::nKnowsAboutInfo::nKnowsAboutInfo(){
    Reset();
    syncReq=false;
}

void nNetObject::nKnowsAboutInfo::Reset()
{
    knowsAboutExistence=false;
    nextSyncAck=true;
    syncReq=true;
    acksPending=0;
    lastSync_ = 0;
}

nNetObject *nNetObject::Object(int i){
    if (i==0) // the NULL nNetObject
        return NULL;

    // the last deleted object with specified ID
    nNetObject* deleted = NULL;
    nDeletedInfos::const_iterator found = sn_netObjectsDeleted.find( i );
    if ( found != sn_netObjectsDeleted.end() )
    {
        deleted = (*found).second.object_;
    }

    // return immediately if the object is there
    nNetObject *ret=ObjectDangerous( i );
    if ( ret )
    {
        return ret;
    }

    // set new timeout value
    const REAL totalTimeout=10;
    const REAL printMessageTimeout=.5;

    double newTimeout=tSysTimeFloat()+totalTimeout;
    static double timeout=-1;
    if ( tSysTimeFloat() > timeout )
        timeout = newTimeout;

    bool printMessage=true;
    while (sn_Connections[0].socket &&
            NULL==(ret=sn_netObjects[i]) && timeout >tSysTimeFloat()){ // wait until it is spawned
        if (tSysTimeFloat()>timeout-(totalTimeout + printMessageTimeout))
        {
            if (printMessage)
            {
                con << "Waiting for NetObject with ID " << i << " to spawn...\n";
                printMessage=false;
            }
            if (sn_GetNetState()==nSERVER){
                // in server mode, we cannot wait.

                // the deleted object with the same ID must have been meant
                if ( deleted )
                {
                    return deleted;
                }

                nReadError();
            }
        }
        tAdvanceFrame(10000);
        sn_Receive();
        sn_SendPlanned();
    }

    if (!ret)
        ret = deleted;

    if (!ret)
        tERR_WARN("Netobject " << i << " requested, but was never spawned.");

    return ret;
}


nNetObject *nNetObject::ObjectDangerous(int i ){
    if (i==0) // the NULL nNetObject
    {
        return NULL;
    }
    else
    {
        nNetObject* ret = sn_netObjects[i];
        if ( ret )
        {
            return ret;
        }
        else
        {
            nDeletedInfos::const_iterator found = sn_netObjectsDeleted.find( i );
            if ( found != sn_netObjectsDeleted.end() )
            {
                nDeletedInfo const & deleted = (*found).second;
                if ( deleted.time_ > tSysTimeFloat() - nDeletedTimeout )
                {
                    return deleted.object_;
                }
            }
        }
    }

    return NULL;
}

void nNetObject::PrintName(tString &s) const
{
    s << "Nameless NetObject nr. " << id;
}

bool nNetObject::HasBeenTransmitted(int user) const{
    return (knowsAbout[user].knowsAboutExistence);
}

// we must not transmit an object that contains pointers
// to non-transmitted objects. this function is supposed to check that.
bool nNetObject::ClearToTransmit(int user) const{
    return true;
}

nProtoBufMessageBase * nNetObject::CreationMessage()
{
    if( !lastCreate_ )
    {
        lastCreate_ = GetDescriptor().WriteSync( *this, true );
    }

    return lastCreate_;
}

nProtoBufMessageBase * nNetObject::SyncMessage()
{
    if( !lastSync_ )
    {
        lastSync_ = GetDescriptor().WriteSync( *this, false );
    }

    return lastSync_;
}

void nNetObject::GetID()
{
    if ( !id && GetRefcount() >= 0 )
    {
        if ( bool( sn_Registrar ) && this == sn_Registrar->object )
        {
            id = sn_Registrar->id;
        }
        else
        {
            id = next_free();
        }

        if (sn_netObjects[id])
            tERR_ERROR("Dublicate nNetObject id " << id);

        sn_netObjectsOwner[id]=owner;
        sn_netObjects_AcceptClientSync[id]=false;

        sn_netObjects[id]=this;
    }
}

// request a sync
void nNetObject::RequestSync(int user,bool ack){ // only for a single user
#ifdef nSIMULATE_PING
    ack=true;
#endif
    this->GetID();

    // object has likely changed
    this->ClearCache();

    if (sn_GetNetState()==nSERVER || (AcceptClientSync() && owner==::sn_myNetID)){
        knowsAbout[user].syncReq=true;
        knowsAbout[user].nextSyncAck |=ack;
    }
#ifdef DEBUG
    else
        tERR_ERROR("RequestSync should only be called server-side!");
#endif

    if ( syncListID_ < 0 )
        sn_SyncRequestedObject.Add( this, syncListID_ );
}

void nNetObject::RequestSync(bool ack){
    this->GetID();

    // object has likely changed
    this->ClearCache();

#ifdef nSIMULATE_PING
    ack=true;
#endif

#ifdef DEBUG
    if (sn_GetNetState()==nCLIENT && (!AcceptClientSync() || owner!=::sn_myNetID))
        tERR_ERROR("RequestSync should only be called server-side!");
#endif

    for (int i=MAXCLIENTS;i>=0;i--){
        knowsAbout[i].syncReq=true;
        knowsAbout[i].nextSyncAck |=ack;
    }

    if ( syncListID_ < 0 )
        sn_SyncRequestedObject.Add( this, syncListID_ );
}


static void net_control_handler( Network::NetObjectControl const & control, nSenderInfo const & sender )
{
    //con << "control\n";
    if (sn_GetNetState()==nSERVER){
        unsigned short id = control.object_id();
        nNetObject *o = sn_netObjects[id];
        if ( o ){
            if ( sender.SenderID()==o->Owner() ) // only the owner is
                // allowed to control the object
                o->ReceiveControlNet( control );
            else
                Cheater( sender.SenderID() ); // another lame cheater.
        }
    }
}

class nProtoBufNetControlDescriptor: 
    public nProtoBufDescriptor< Network::NetObjectControl >,
    public nMessageStreamer
{
private:
    //! function responsible for turning message into old stream message
    virtual void StreamFromProtoBuf( nProtoBufMessageBase const & source, nStreamMessage & target ) const
    {
        // cast to correct type
        Network::NetObjectControl const & control =
        static_cast< Network::NetObjectControl const & >( source.GetProtoBuf() );

        // fetch object ID and object
        unsigned short id = control.object_id();
        target.Write( id );
        nNetObject * object = nNetObject::ObjectDangerous( id );
        if ( object )
        {
            // delegate core work
            object->StreamControl( control, target );
        }

        // bend the descriptor
        target.BendDescriptorID( source.DescriptorID() & ~nProtoBufDescriptorBase::protoBufFlag );
    }
    
    //! function responsible for reading an old message
    virtual void StreamToProtoBuf( nStreamMessage & source, nProtoBufMessageBase & target ) const
    {
        // cast to correct type
        Network::NetObjectControl & control =
        static_cast< Network::NetObjectControl & >( target.AccessProtoBuf() );

        // fetch object ID and object
        unsigned short id;
        source.Read( id );
        control.set_object_id( id );
        nNetObject * object = nNetObject::ObjectDangerous( id );
        if ( object )
        {
            // delegate core work
            object->UnstreamControl( source, control );
        }
        else
        {
            // ignore this message
            nReadError( false );
        }
    }

public:
    nProtoBufNetControlDescriptor()
    : nProtoBufDescriptor< Network::NetObjectControl >( 23, net_control_handler )
    {
        SetStreamer( this );
    }
};

static nProtoBufNetControlDescriptor net_control;

void nNetObject::ReceiveControlNet( Network::NetObjectControl const & )
{
#ifdef DEBUG
    if (sn_GetNetState()==nCLIENT)
        tERR_ERROR("rec_cont should not be called client-side!");
#endif

    // after control is received, we better sync this object with
    // the clients:

    RequestSync();
}

// control functions:
Network::NetObjectControl & nNetObject::BroadcastControl(){
    nProtoBufMessage< Network::NetObjectControl > * m = net_control.CreateMessage();

    // broadcast the message. This keeps it alive until the next time the network queue
    // is flushed, so it's safe to operate on its internal data afterwards.
    m->BroadCast();

    Network::NetObjectControl & control = m->AccessProtoBuf();

    control.set_object_id( ID() );

    return control;
}

// conversion for stream legacy control messages
void nNetObject::StreamControl( Network::NetObjectControl const & control, nStreamMessage & stream )
{
    nProtoBuf const * relevant = ExtractControl( control );
    if ( relevant )
    {
        nProtoBufDescriptorBase::StreamToStatic( *relevant, stream, nProtoBufDescriptorBase::SECTION_Default );
    }
}

void nNetObject::UnstreamControl( nStreamMessage & stream, Network::NetObjectControl & control )
{
    nProtoBuf * relevant = ExtractControl( control );
    if ( relevant )
    {
        nProtoBufDescriptorBase::StreamFromStatic( stream, *relevant, nProtoBufDescriptorBase::SECTION_Default );
    }
}

// easier to implement conversion helpers: just extract the relevant sub-protbuf.
nProtoBuf * nNetObject::ExtractControl( Network::NetObjectControl & control )
{
    return NULL;
}
nProtoBuf const * nNetObject::ExtractControl( Network::NetObjectControl const & control )
{
    return NULL;
}

// ack that doesn't resend, just wait
class nDontWaitForAck: public nWaitForAck{
public:
    nDontWaitForAck(nMessageBase* m,int rec)
    : nWaitForAck( m, rec ){}
    virtual ~nDontWaitForAck(){};

    virtual bool DoResend()
    { 
        // explicitly don't add message to cache
        sn_Connections[receiver].messageCacheOut_.AddMessage( message, false, false );

        return false;
    }
};


class nWaitForAckSync: public nWaitForAck{
    unsigned short netobj;
public:
    nWaitForAckSync(nMessageBase* m,int rec,unsigned short obj)
            :nWaitForAck(m,rec),netobj(obj){
        if (sn_netObjects(obj)->knowsAbout[rec].acksPending<15)
        {
            sn_netObjects(obj)->knowsAbout[rec].acksPending++;
        }
        else
        {
            st_Breakpoint();
        }
    }
    virtual ~nWaitForAckSync(){
        tCHECK_DEST;
    }

    virtual void AckExtraAction()
    {
        nNetObject* obj = sn_netObjects[netobj];
        if ( obj )
        {
            nNetObject::nKnowsAboutInfo & knows = obj->knowsAbout[receiver];

            // ack accounting
            if ( knows.acksPending )
            {
                knows.acksPending--;
            }
            else
            {
                //			st_Breakpoint();
            }

#ifdef DEBUG
            /*
            if ( !obj->knowsAbout[receiver].knowsAboutExistence )
              {
            	tString str;
            	obj->PrintName( str );
            	con << "Received ack for object " << str << "\n";
              }
            */
#endif

            knows.knowsAboutExistence=true;

            // store last acked sync for caching
            nProtoBufMessageBase * m = dynamic_cast< nProtoBufMessageBase * >( message.operator->() );
            if ( m && ( !knows.lastSync_ || ( m->MessageIDBig() - knows.lastSync_->MessageIDBig() ) > 0 ) )
            {
                knows.lastSync_ = m;
            }
        }
        else
        {
            //		st_Breakpoint();
        }
    }
};

static void net_sync_handler( nStreamMessage & m )
{
    unsigned short id;
    m.Read(id);
#ifdef DEBUG
    // sn_BreakOnObjectID( id );
#endif
    nNetObject * obj = nNetObject::ObjectDangerous(id);
    if (obj){
        if (sn_GetNetState()!=nCLIENT &&
                (!obj->AcceptClientSync()
                 || obj->Owner()!=m.SenderID())
           ){
            Cheater( m.SenderID() );
#ifdef DEBUG
            tERR_ERROR("sync should only be called client-side!");
#endif
        }
        else
        {
            obj->GetDescriptor().ReadSync( *obj, m, false );
        }
    }
}

static nStreamDescriptor net_sync(24,net_sync_handler,"net_sync");

//! class converting sync messages
class nMessageStreamerSync: public nMessageStreamer
{
public:
    static const nProtoBufDescriptorBase::StreamSections sections;

    //! function responsible for turning message into old stream message
    virtual void StreamFromProtoBuf( nProtoBufMessageBase const & source, nStreamMessage & target ) const
    {
        // stream to message
        source.GetDescriptor().StreamTo( source.GetProtoBuf(), target, sections );
    
        // bend the descriptor
        target.BendDescriptorID( net_sync.ID() );
    }

    //! function responsible for reading an old message
    virtual void StreamToProtoBuf( nStreamMessage & source, nProtoBufMessageBase & target ) const
    {
        // stream from message
        target.GetDescriptor().StreamFrom( source, target.AccessProtoBuf(), sections  );
    }
};

const nProtoBufDescriptorBase::StreamSections nMessageStreamerSync::sections( nProtoBufDescriptorBase::StreamSections(nProtoBufDescriptorBase::SECTION_ID | nProtoBufDescriptorBase::SECTION_Second) );

//! class converting creation messages
class nMessageStreamerCreate: public nMessageStreamer
{
public:
    //! function responsible for turning message into old stream message
    virtual void StreamFromProtoBuf( nProtoBufMessageBase const & source, nStreamMessage & target ) const
    {
        // stream to message
        source.GetDescriptor().StreamTo( source.GetProtoBuf(), target, nProtoBufDescriptorBase::SECTION_All );
    
        // bend the descriptor
        target.BendDescriptorID( source.DescriptorID() & ~nProtoBufDescriptorBase::protoBufFlag );
    }

    //! function responsible for reading an old message
    virtual void StreamToProtoBuf( nStreamMessage & source, nProtoBufMessageBase & target ) const
    {
        // stream from message
        target.GetDescriptor().StreamFrom( source, target.AccessProtoBuf(), nProtoBufDescriptorBase::SECTION_All );
    }
};

//! converter for creation messages
nMessageStreamer * nNetObjectDescriptorBase::CreationStreamer()
{
    static nMessageStreamerCreate create;
    return &create;
}

//! converter for sync messages
nMessageStreamer * nNetObjectDescriptorBase::SyncStreamer()
{
    static nMessageStreamerSync sync;
    return &sync;
}

bool nNetObject::AcceptClientSync() const{
    return false;
}


// global functions:


//static int current_sync[MAXCLIENTS+2];
static bool is_ready_to_get_objects[MAXCLIENTS+2];

// from nNetwork.C
//extern REAL planned_rate_control[MAXCLIENTS+2];

static bool s_DoPrintDebug = false;
bool nNetObject::DoDebugPrint()
{
    return s_DoPrintDebug;
}

static int sn_syncedUser = -1;

void nNetObject::SyncAll()
{
#ifdef DEBUG
    s_DoPrintDebug = false;

    static nTimeRolling debugtime = 0;
    nTimeRolling time = tSysTimeFloat();
    if (time > debugtime && sn_GetNetState() == nSERVER)
    {
        debugtime = time+5;
        //      s_DoPrintDebug = true;
    }
#endif

    for (int user=MAXCLIENTS;user>=0;user--)
        if (is_ready_to_get_objects[user] &&
                sn_Connections[user].socket && sn_netObjects.Len()>0 && user!=sn_myNetID){

            sn_syncedUser = user;

            // send the destroy messages
            if (destroyers[user])
            {
                // but check whether the opportunity is good (big destroyers message, or a sync packet in the pipe) first
                if ( destroyers[user]->AccessProtoBuf().ids_size() > 75 ||
                        sn_Connections[user].sendBuffer_.Len() > 0 ||
                        destroyersTime[user] < tSysTimeFloat()-.5 )
                {
                    destroyers[user]->Send(user);
                    destroyers[user]=NULL;
                }
            }

            // con << sn_SyncRequestedObject.Len() << "/" << sn_netObjects.Len() << "\n";

            int currentSync = sn_SyncRequestedObject.Len()-1;
            while (sn_Connections[user].socket>0 &&
                    sn_Connections[user].bandwidthControl_.CanSend() &&
                    sn_Connections[user].ackPending<sn_maxNoAck &&
                    currentSync >= 0){
                nNetObject *nos = sn_SyncRequestedObject( currentSync );

                if (nos && nos->ClearToTransmit(user) && ( nos == sn_netObjects( nos->id ) )
                        && (sn_GetNetState()!=nCLIENT ||
                            nos->AcceptClientSync()))
                {
                    short s = nos->id;

                    nNetObject::nKnowsAboutInfo & knowledge = nos->knowsAbout[user];

                    if (// knowledge.syncReq &&
                        !knowledge.knowsAboutExistence)
                    {
                        if (!knowledge.acksPending){
#ifdef DEBUG
                            //con << "remotely creating object " << s << '\n';
#endif
                            /*
                              con << "creating object " << s << " at user " << user
                              << " owned by " << sn_netObjects(s)->owner << '\n';
                            */
                            // send a creation message

                            tJUST_CONTROLLED_PTR< nMessageBase > m = nos->CreationMessage();
                            new nWaitForAckSync(m,user,s);
                            // unsigned long id = m->MessageIDBig();
                            m->SendImmediately(user, false);
                            // m->messageIDBig_ = id;

                            knowledge.syncReq = false;
                        }
#ifdef DEBUG
                        else if (DoDebugPrint())
                        {
                            tString s;
                            s << "Not remotely creating object ";
                            nos->PrintName(s);
                            s << " on user " << user << " again because there is an Ack pending.\n";
                            con << s;
                        }
#endif
                    }
                    else if (knowledge.syncReq
                             && sn_Connections[user].bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) >50
                             && knowledge.acksPending<=1){
                        // send a sync
                        tJUST_CONTROLLED_PTR< nProtoBufMessageBase > m = nos->SyncMessage();
                        knowledge.syncReq=false;

                        if (knowledge.nextSyncAck){
                            new nWaitForAckSync(m,user,s);
                            knowledge.nextSyncAck=false;
                        }
                        else
                        {
                            new nDontWaitForAck(m,user);
                        }
#ifndef nSIMULATE_PING
                        // unsigned long id = m->MessageIDBig();
                        //m->Send(user,0,false);
                        m->SetCacheHint( knowledge.lastSync_ );
                        m->SendImmediately(user,false);
                        // m->messageIDBig_ = id;
#endif
                    }

                }

                currentSync--;
            }

            sn_syncedUser = -1;

#ifdef DEBUG
            bool inc=false;
            static int warn=0;
            static int nextWarnOverflow=0;
            static int nextWarnAck=0;
            if (sn_Connections[user].bandwidthControl_.Control( nBandwidthControl::Usage_Planning )<-100){
                if (warn>=nextWarnOverflow)
                {
                    nextWarnOverflow = 2+(warn*17)/16;
                    con << "Warning! Network overflow: "
                    << -100-sn_Connections[user].bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) << "\n";
                }
                inc=true;
            }
            if (sn_Connections[user].ackPending>=sn_maxNoAck){
                if (warn>=nextWarnAck)
                {
                    nextWarnAck = 2+(warn*17)/16;
                    std::cerr << "Warning! Too many acks pending: "
                    << sn_Connections[user].ackPending << "\n";
                }
                inc=true;
            }
            if (inc)
                warn++;
#endif
        }


    // clear out objects that no longer need to be in the list because
    // all requested syncs have been sent
    {
        for ( int currentSync = sn_SyncRequestedObject.Len()-1; currentSync >= 0; --currentSync ){
            nNetObject *nos = sn_SyncRequestedObject( currentSync );

            if ( nos )
            {
                // check if nos can be removed from sync request list
                bool canRemove = true;

                // check if there is anything preventing the removal
                if ( nos == sn_netObjects( nos->id ) )
                {
                    int start = ( sn_GetNetState() == nSERVER ) ? MAXCLIENTS : 0;
                    int stop  = ( sn_GetNetState() == nSERVER ) ? 1 : 0;
                    for ( int i = start; i>=stop && canRemove; --i )
                    {
                        const nKnowsAboutInfo& knows = nos->knowsAbout[i];
                        if ( sn_Connections[i].socket && knows.syncReq )
                            canRemove = false;
                    }
                }
                if ( canRemove )
                    sn_SyncRequestedObject.Remove( nos, nos->syncListID_ );
            }
        }
    }
}


static void sn_ReadyObjectsHandler( Network::ReadyObjects const & ready, nSenderInfo const & sender )
{
    is_ready_to_get_objects[ sender.SenderID() ]=true;

    // reset peer's ping, it's probably unreliable
    sn_Connections[ sender.SenderID() ].ping.Timestep(100);
}

static nProtoBufDescriptor< Network::ReadyObjects > sn_readyObjectsDescriptor( 25, sn_ReadyObjectsHandler );

static void sn_ClearObjectsHandler( Network::ClearObjects const & clear, nSenderInfo const & sender )
{
    if ( sn_GetNetState()!=nSERVER )
    {
        nNetObject::ClearAll();
        first_fill_ids();
    }
}

static nProtoBufDescriptor< Network::ClearObjects > sn_clearObjectsDescriptor( 26, sn_ClearObjectsHandler );


void nNetObject::ClearAllDeleted()
{
    // forget about objects that were deleted in the past. The swap trick is to
    // avoid that the objects try to remove themselves from the list while it is cleared.
    nDeletedInfos swap;
    swap.swap( sn_netObjectsDeleted );
    swap.clear();

    // send out object deletion messages
    for ( int i = MAXCLIENTS;i>=0;i--)
    {
        if ( sn_Connections[i].socket && destroyers[i] )
        {
            destroyers[i]->Send(i);
            destroyers[i] = NULL;
        }
    }
}

void nNetObject::ClearAll(){
    ClearAllDeleted();
    sn_freedIDs.clear();

    //con << "WARNING! BAD DESIGN. nNetObject::clear all() called.\n";
    int i;
    for (i=sn_netObjects.Len()-1;i>=0;i--)
        if (tJUST_CONTROLLED_PTR< nNetObject > no=sn_netObjects(i)){
            sn_netObjects(i)=NULL;
            sn_netObjectsOwner(i)=0;
            no->id = 0;
        }
    sn_netObjects.SetLen(0);
    sn_netObjectsOwner.SetLen(0);

    for (i=sn_SyncRequestedObject.Len()-1;i>=0;i--)
    {
        nNetObject * n = sn_SyncRequestedObject[i];
        tASSERT(n);
        sn_SyncRequestedObject.Remove( n, n->syncListID_ );
    }

    sn_clearObjectsDescriptor.Broadcast(); // just to make sure..
}


void nNetObject::ClearKnows(int user, bool clear){
    if (0<=user && user <=MAXCLIENTS){
        is_ready_to_get_objects[user]=false;
        for (int i=sn_netObjects.Len()-1;i>=0;i--){
            nNetObject *no=sn_netObjects(i);
            if (no){
                no->knowsAbout[user].Reset();

                no->DoBroadcastExistence();  // immediately transfer the thing

                if (clear){
                    if (no->owner==user && user!=sn_myNetID){
                        if (no->ActionOnQuit())
                            sn_netObjects(i)=NULL; // destroy it
                        else{
                            no->owner=::sn_myNetID; // or make it mine.
                            sn_netObjectsOwner(i)=::sn_myNetID;
                            if (no->AcceptClientSync()){
                                tControlledPTR< nNetObject > bounce( no ); // destroy it, if noone wants it
                            }
                        }
                    }
                }
            }
        }
    }
}

void ClearKnows(int user, bool clear){
    nNetObject::ClearKnows(user, clear);

    if (clear)
        for (int i=sn_netObjectsOwner.Len()-1;i>=0;i--){
            if (sn_netObjectsOwner(i)==user)
                sn_netObjectsOwner(i)=0;
        }
}


/* If we switch from standalone to client mode, all the sn_netObjects
 * need new id's.
 */


void nNetObject::RelabelOnConnect(){
    if (sn_GetNetState()==nCLIENT){
        tArray<tJUST_CONTROLLED_PTR<nNetObject> > sn_netObjects_old;

        // transfer the sn_netObjects to sn_netObjects_old:
        int i;
        for (i=sn_netObjects.Len()-1;i>=0;i--){
            sn_netObjects_old[i]=sn_netObjects(i);
            sn_netObjects(i)=NULL;
            sn_netObjectsOwner[i]=0;
        }

        // assign new id's and transfer them back to sn_netObjects:
        for (i=sn_netObjects_old.Len()-1;i>=0;i--){
            nNetObject *no = sn_netObjects_old(i);
            if (no){
                unsigned short id=next_free();

#ifdef DEBUG
                sn_BreakOnObjectID( id );
#endif

#ifdef DEBUG
                //con << "object " <<i << " gets new id " << id << '\n';
#endif
                no->id=id;
                no->owner=::sn_myNetID;
                sn_netObjectsOwner[id]=::sn_myNetID;
                for (int j=MAXCLIENTS;j>=0;j--){
                    no->knowsAbout[j].Reset();
                    no->DoBroadcastExistence();
                }

                if (sn_netObjects[id])
                    st_Breakpoint();

                sn_netObjects[id]=no;
                sn_netObjects_old(i)=NULL;
            }
        }
    }
    sn_readyObjectsDescriptor.Send(0);
    is_ready_to_get_objects[0]=true;
}


static void login_callback(){
    int user = nCallbackLoginLogout::User();
    ClearKnows( user, !nCallbackLoginLogout::Login() );

    if ( user == 0 )
    {
        if (nCallbackLoginLogout::Login())
            nNetObject::RelabelOnConnect();
        else
        {
            sn_Destroyed.SetLen(0);
            sn_LocallyDestroyedIDs.clear();
        }
    }

    // send and delete the  remaining destroyer message
    if ( destroyers[user] )
    {
        destroyers[user]->Send(user);
        destroyers[user]=NULL;
    }
}

static nCallbackLoginLogout nlc(&login_callback);


static bool sync_ack[MAXCLIENTS+2];
static unsigned short c_sync=0;

static void sn_SyncAckHandler( Network::SyncAck const & ack, nSenderInfo const & sender )
{
    unsigned short id = ack.token();
    if (id==c_sync)
        sync_ack[ sender.SenderID() ] = true;
}

static nProtoBufDescriptor< Network::SyncAck > sn_syncAckDescriptor( 27, sn_SyncAckHandler );

static void sn_SyncHandler( Network::Sync const & sync, nSenderInfo const & sender );
static nProtoBufDescriptor< Network::Sync > sn_syncDescriptor( 28, sn_SyncHandler );

// from nNetwork.C

void sn_Sync(REAL timeout,bool sync_sn_netObjects, bool otherEnd){
    nTimeAbsolute endTime=timeout+tSysTimeFloat();

#ifdef DEBUG
    //con << "Start sync...\n";
#endif

    if (sn_GetNetState()==nCLIENT){
        // repeat as often as packet loss suggests: of 100 packets, we want a single lost packet left probability of 1%.
        REAL failureProbability = 1;
        while ( failureProbability > .0001 && timeout > .01 )
        {
            failureProbability *= sn_Connections[0].PacketLoss();
            // con << failureProbability << "\n";

            // server responding to sync request message
            static nVersionFeature sg_ServerSync( 10 );

            // ask server when we're fully synced from his side
            sync_ack[0]=true;
            if ( sg_ServerSync.Supported( 0 ) && otherEnd )
            {
                Network::Sync & sync = sn_syncDescriptor.Send(0);
                sync.set_timeout( timeout );
                sync.set_sync_netobjects( sync_sn_netObjects );
                sync.set_token( c_sync );
                sync_ack[0]=false;
            }
            else
            {
                // repeating the procedure is useless anyway
                failureProbability = 0;
            }

            // wait for all packets to be sent and the sync ack packet to be received
            while ( sn_Connections[0].socket && ( sync_ack[0] == false || sn_Connections[0].ackPending>0 || sn_QueueLen(0)) &&
                    tSysTimeFloat()<endTime){
                sn_Delay();
                sn_Receive();
                if (sync_sn_netObjects)
                    nNetObject::SyncAll();
                sn_SendPlanned();
            }

            // decrease timeout for next try
            timeout *= .5;
            endTime=timeout+tSysTimeFloat();
        }
    }
    else if (sn_GetNetState()==nSERVER){
        for (int user=MAXCLIENTS;user>0;user--){
            sync_ack[user]=false;
            if (sn_Connections[user].socket){
                Network::Sync & sync = sn_syncDescriptor.Send( user );
                sync.set_timeout( timeout );
                sync.set_sync_netobjects( sync_sn_netObjects );
                sync.set_token( c_sync );
            }
        }

        bool goon=true;
        while (goon){
            sn_Delay();
            sn_Receive();
            if (sync_sn_netObjects)
                nNetObject::SyncAll();
            sn_SendPlanned();

            goon=false;
            for (int user=MAXCLIENTS;user>0;user--)
            {
                if (sn_Connections[user].socket &&
                        (!sync_ack[user] || sn_Connections[user].ackPending>0 || sn_QueueLen(user)))
                {
                    goon=true;
                }
            }

            if (tSysTimeFloat()>endTime)
            {
                goon=false;
            }
        }
    }

#ifdef DEBUG
    //con << "Stop sync.\n";
#endif
}

static void sn_SyncHandler( Network::Sync const & sync, nSenderInfo const & sender )
{
    static bool recursion=false;
    if (!recursion){
        recursion=true;

        REAL timeout = sync.timeout();
        bool sync_sn_netObjects = sync.sync_netobjects();
        unsigned short c_sync = sync.token();

        if (sn_GetNetState()!=nSERVER){
            // clients obediently sync...
            sn_Sync(timeout+4,sync_sn_netObjects!=0,false);
            
            // and reply.
            sn_syncAckDescriptor.Send(0).set_token( c_sync );
        }
        else
        {
            // no time for syncing! write sync response immediately with low priority so the client
            // gets it as soon as all other things are sent
            nProtoBufMessage< Network::SyncAck > * response = sn_syncAckDescriptor.CreateMessage();
            response->AccessProtoBuf().set_token( c_sync );
            response->Send( sender.SenderID(), 10000000 );
        }
        recursion=false;
    }
}

void clear_owners(){
    sn_freedIDs.clear();
    for (int i=sn_netObjectsOwner.Len()-1;i>=0;i--)
        sn_netObjectsOwner(i)=0;
}


#include "nPriorizing.h"

// *********************************************************
// nBandwidthTaskSync/Create: object syncing bandwidth tasks
// *********************************************************

nBandwidthTaskObject::nBandwidthTaskObject( nType type, nNetObject& object )
        :nBandwidthTask( type ), object_( &object )
{
}

nBandwidthTaskObject::~nBandwidthTaskObject()
{

}

// estimate bandwidth usage
int  nBandwidthTaskObject::DoEstimateSize() const
{
    return 16;
}

// executes whatever it has to do
void nBandwidthTaskSync::DoExecute( nSendBuffer& buffer, nBandwidthControl& control, int peer )
{
    tJUST_CONTROLLED_PTR< nMessageBase > message = Object().SyncMessage();
    buffer.AddMessage( *message, &control, peer );
}

// executes whatever it has to do
void nBandwidthTaskCreate::DoExecute( nSendBuffer& buffer, nBandwidthControl& control, int peer )
{
    tJUST_CONTROLLED_PTR< nMessageBase > message = Object().CreationMessage();
    buffer.AddMessage( *message, &control, peer );
}


// *******************************************************************************
// *
// *	DoGetMachine
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nMachine & nNetObject::DoGetMachine( void ) const
{
    return nMachine::GetMachine( Owner() );
}

// protocol buffer stuff

//! creates a netobject form sync data
nNetObject::nNetObject( Network::NetObjectSync const & sync, nSenderInfo const & sender )
:lastSyncID_(sender.envelope.MessageIDBig() - 1),refCtr_(0)
{
    id = 0;
    owner = 0;

    syncListID_ = -1;

    tASSERT( sn_Registrar );
    nNetObjectRegistrar& registrar = *sn_Registrar;

    createdLocally = false;

    registrar.id = sync.object_id();
#ifdef DEBUG
    //con << "Netobject " << id << " created on remote order.\n";
    //  if (id == 383)
    //  st_Breakpoint();
#endif
    owner = sync.owner_id();

    // clients are only allowed to create self-owned objects
    if ( sn_GetNetState() == nSERVER )
    {
        if ( owner != sender.SenderID() )
        {
            nReadError( true );
        }
    }

    registrar.object = this;
    registrar.sender = sender.SenderID();

    knowsAbout[sender.SenderID()].knowsAboutExistence=true;
#ifdef DEBUG
    // con << "Netobject " << id  << " created (remote order).\n";
#endif
}

//! reads incremental sync data
void nNetObject::ReadSync( Network::NetObjectSync const & sync, nSenderInfo const & sender )
{
    if (sn_GetNetState()==nSERVER){
        bool back=knowsAbout[sender.SenderID()].syncReq;
        RequestSync(); // tell the others about it
        knowsAbout[sender.SenderID()].syncReq=back;
        // but not the sender of the message; he
        // knows already.
    }
}

//! reads incremental sync data
bool nNetObject::SyncIsNew( Network::NetObjectSync const & sync, nSenderInfo const & sender )
{
    // check if sync is new
    unsigned long int bigID =  sender.envelope.MessageIDBig();
    if ( ID() && !sn_Update(lastSyncID_,bigID) && bigID != 0 )
    {
        return false;
    }
    return true;
}

//! writes sync data
void nNetObject::WriteSync( Network::NetObjectSync & sync, bool init ) const
{
    sync.set_object_id( id );
    if( init )
    {
        sync.set_owner_id( owner );
    }
}

// nNetObjectDescriptor< nNetObject, Network::NetObjectTotal > sn_pbdescriptor( 0 );

//! returns the descriptor responsible for this class
nNetObjectDescriptorBase const & nNetObject::DoGetDescriptor() const
{
    tASSERT(0);
    nNetObjectDescriptorBase * ret = 0;
    return *ret;
}

