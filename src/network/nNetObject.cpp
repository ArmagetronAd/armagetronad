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

#include <deque>
#include <set>
#include <map>

// debug watchs
#ifdef DEBUG
int sn_WatchNetID = 0;
extern nMessage* sn_WatchMessage;
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
static void sn_BreakOnObjectID( unsigned short id )
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
                throw nKillHim();

            // or just exit silently
            con << "Emergency exit: desperately ran out of IDs.\n";
            exit(-1);
        }
    }
}

void req_id_handler(nMessage &m){
    unsigned short stop = distribute;
    if (distribute == 0)
        stop = ID_PREFETCH;

    if (sn_GetNetState()==nSERVER)
        Cheater(m.SenderID());
    else{
        while (!m.End())
        {
            unsigned short id, count=1;
            m.Read(id);
            if (!m.End())
                m.Read(count);

            for (unsigned short i=id + count - 1; i>= id && request+1 != stop; i--)
            {
                if (sn_netObjects[i])
                {
                    con << "Warning! Network id receive error on ID " << i << " belonging to client " << sn_netObjects[i]->Owner() << "\n";
                    con << "while recieving ID block " << id << "-" << id+count-1 << " from netmessage " << m.MessageID() << ".\n";

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

nDescriptor req_id(20,req_id_handler,"req_id");

void id_req_handler(nMessage &m){
    // Add security: keep clients from fetching too many ids

    if (sn_GetNetState()==nSERVER && m.SenderID()<=MAXCLIENTS)
    {
        if (m.End())
        { // old style request; send only one ID back.
            tJUST_CONTROLLED_PTR< nMessage > rep=new nMessage(req_id);
            unsigned short id=next_free_server(true);
            sn_netObjectsOwner[id]=m.SenderID();
            //	  con << "Assigning ID " << id << "\n";
            rep->Write(id);
            rep->Send(m.SenderID());

#ifdef DEBUG
            //con << "distributed id " << net_current_id-1 << " to user " << m.SenderID() << '\n';
#endif
        }
        else
        {
            // new style request: many IDs
            unsigned short num;
            m.Read(num);

            // but not too many. kick violators.
            if ( num > ID_PREFETCH*4 )
            {
                throw nKillHim();
            }

            tJUST_CONTROLLED_PTR< nMessage > rep=new nMessage(req_id);

            unsigned short begin_block=0;	// begin of the block of currently assigned IDs
            unsigned short block_len=0;		// and it's length

            for (int i = num-1; i>=0; i--)
            {
                nNetObjectID id = next_free_server(true);

                sn_netObjectsOwner[id]=m.SenderID();

                if (begin_block + block_len == id)	// RLE for allocated IDs
                    block_len++;
                else
                {
                    if (block_len > 0)
                    {
                        //		      con << "Assigning block " << begin_block << " - " << begin_block + block_len - 1 << "\n";
                        rep->Write(begin_block);
                        rep->Write(block_len);
                    }
                    begin_block = id;
                    block_len = 1;
                }
            }
            if (block_len > 0)
            {
                //	      con << "Assigning block " << begin_block << " - " << begin_block + block_len - 1 << "\n";
                rep->Write(begin_block);
                rep->Write(block_len);
            }

            rep->Send(m.SenderID());
        }
    }
}

nDescriptor id_req(21,id_req_handler,"id_req_handler");

unsigned short next_free(){
    unsigned short ret=0;

    do{
        if (sn_GetNetState()==nCLIENT){
            unsigned short need_soon = request + ID_PREFETCH - distribute;
            if (need_soon > ID_PREFETCH)
                need_soon -= ID_PREFETCH;
            if (need_soon < (ID_PREFETCH >> 1))
            {
                tJUST_CONTROLLED_PTR< nMessage > m = new nMessage(id_req);
                m->Write(ID_PREFETCH >> 2);
                m->Send(0);
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

    tJUST_CONTROLLED_PTR< nMessage > m = new nMessage(id_req);
    m->Write(ID_PREFETCH - 10);
    m->Send(0);
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

// static unsigned long int global_lastSync=0;

bool nNetObject::SyncIsNew(nMessage &m){
    unsigned long int bigID =  m.MessageIDBig();
    //    sn_Update(global_lastSync,bigID);
    return sn_Update(lastSyncID_,bigID);
}

/*
static unsigned short global_lastSync=0;

bool nNetObject::SyncIsNew(nMessage &m){
    sn_Update(global_lastSync,m.MessageID());
    return sn_Update(lastSyncID,m.MessageID());
}
*/

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
#ifdef DEBUG
        sn_BreakOnObjectID( registrar.id );
#endif

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
        sn_netObjects[id]=this;
    }

    if (sn_GetNetState()!=nCLIENT)
        owner=registrar.sender; // to make sure noone is given a nNetObject from
    // someone else.

    sn_netObjectsOwner[id]=owner;
    sn_netObjects_AcceptClientSync[id]=false;
}

nNetObject::nNetObject(nMessage &m):lastSyncID_(m.MessageIDBig()),refCtr_(0){
    // sn_Update(global_lastSync,lastSyncID_);

    id = 0;
    owner = 0;

    syncListID_ = -1;

    tASSERT( sn_Registrar );
    nNetObjectRegistrar& registrar = *sn_Registrar;

    createdLocally = false;

    m.Read( registrar.id );
#ifdef DEBUG
    //con << "Netobject " << id << " created on remote order.\n";
    //  if (id == 383)
    //  st_Breakpoint();
#endif
    m.Read( owner );

    // clients are only allowed to create self-owned objects
    if ( sn_GetNetState() == nSERVER )
    {
        if ( owner != m.SenderID() )
        {
            throw nKillHim();
        }
    }

    registrar.object = this;
    registrar.sender = m.SenderID();

    knowsAbout[m.SenderID()].knowsAboutExistence=true;
#ifdef DEBUG
    // con << "Netobject " << id  << " created (remote order).\n";
#endif
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


static void net_destroy_handler(nMessage &m){
    //con << "destroy begin\n";
    unsigned short id;
    //int count=0;
    while (!m.End()){
        m.Read(id);
#ifdef DEBUG
        sn_BreakOnObjectID( id );
#endif
        // see if there was a local destruction; if yes, ignore.
        if (sn_WasDeletedLocally( id ))
            continue;

        nDestroyInfo& info = sn_Destroyed[ sn_Destroyed.Len() ];
        info.id = id;
        info.sender = m.SenderID();
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

                    no->TakeOwnership();
                    tJUST_CONTROLLED_PTR< nNetObject > bounce( no );
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

static nDescriptor net_destroy(22,net_destroy_handler,"net_destroy");

static tJUST_CONTROLLED_PTR< nMessage > destroyers[MAXCLIENTS+2];
static REAL                             destroyersTime[MAXCLIENTS+2];
static int                              destroyersCount[MAXCLIENTS+2];

// special ack for destroy messages
class nWaitForAckDestroy: public nWaitForAck{
public:
    nWaitForAckDestroy(nMessage* m,int receiver)
            :nWaitForAck(m,receiver)
    {
        ++destroyersCount[receiver];
    }
    virtual ~nWaitForAckDestroy()
    {
        --destroyersCount[receiver];
    }
};

static void sn_SendDestroyer( int client )
{
    tASSERT( 0 <= client && client <= MAXCLIENTS+1 );
    if ( destroyers[ client ] )
    {
        destroyers[ client ]->SendImmediately( client, false );
        new nWaitForAckDestroy( destroyers[ client ], client );
        destroyers[ client ] = 0;
    }
}

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
                    destroyers[user]=new nMessage(net_destroy);
                    destroyersTime[user]=tSysTimeFloat();
                }
                destroyers[user]->Write(id);

                if (destroyers[user]->DataLen() > ( destroyersCount[user] ? 1000 : 100 ) )
                {
                    sn_SendDestroyer( user );
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



nNetObject *nNetObject::Object(int i){
    if (i==0) // the NULL nNetObject
        return NULL;

    // the last deleted object with specified ID
    nNetObject* deleted = NULL;
    nDeletedInfos::const_iterator found = sn_netObjectsDeleted.find( id );
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

                con << "Now we need to leave the\n"
                << "system in an undefined state. I hope this works...\n";

                Cheater(owner); // kill this bastard
                return NULL; // pray that noone references this pointer
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

void nNetObject::WriteSync(nMessage &m, int run )
{
    if ( run == 0 )
    {
        WriteSync( m );
    }
}

void nNetObject::ReadSync(nMessage &m, int run )
{
    if ( run == 0 )
    {
        ReadSync( m );
    }
}

void nNetObject::WriteCreate(nMessage &m, int run )
{
    if ( run == 0 )
    {
        WriteCreate( m );
    }
}

void nNetObject::ReadCreate(nMessage &m, int run )
{
    // run == 0 would call the constructor, but run == 0 never happens.
    tASSERT( run > 0 );
}

void nNetObject::WriteAll( nMessage & m, bool create )
{
    int lastLen = -1;
    int run = 0;

    // write as long as the read functions do something
    while ( m.DataLen() > lastLen )
    {
        lastLen = m.DataLen();
        if ( create )
        {
            WriteCreate(m,run);
        }

        WriteSync(m,run);
        ++run;
    }
}

void nNetObject::ReadAll ( nMessage & m, bool create )
{
    int lastRead = -1;
    int run = 0;

    // read as long as the read functions do something
    while ( m.ReadSoFar() > lastRead )
    {
        lastRead = m.ReadSoFar();
        if ( create && run > 0 )
        {
            ReadCreate(m,run);
        }

        ReadSync(m,run);
        ++run;
    }
}

void nNetObject::WriteSync(nMessage &m){
#ifdef DEBUG
    if (sn_GetNetState()!=nSERVER && !AcceptClientSync())
        tERR_ERROR("WriteSync should only be called server-side!");
#endif
} // nothing to do yet



void nNetObject::ReadSync(nMessage &m){
    if (sn_GetNetState()==nSERVER){
        bool back=knowsAbout[m.SenderID()].syncReq;
        RequestSync(); // tell the others about it
        knowsAbout[m.SenderID()].syncReq=back;
        // but not the sender of the message; he
        // knows already.
    }
}

extern bool deb_net;

// read and write id and owner
void nNetObject::WriteCreate(nMessage &m){
    m.Write(id);
    m.Write(owner);

    // store the info needed in the destructor
    sn_netObjects_AcceptClientSync[id]=this->AcceptClientSync();

    if (deb_net)
        con << "Sending creation message for nNetObject " << id << "\n";
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


static void net_control_handler(nMessage &m){
    //con << "control\n";
    if (sn_GetNetState()==nSERVER){
        unsigned short id;
        m.Read(id);
        nNetObject *o = sn_netObjects[id];
        if ( o ){
            if (m.SenderID()==o->Owner()) // only the owner is
                // allowed to control the object
                o->ReceiveControlNet(m);
            else
                Cheater(m.SenderID()); // another lame cheater.
        }
    }
}

static nDescriptor net_control(23,net_control_handler,"net_control");

void nNetObject::ReceiveControlNet(nMessage &){
#ifdef DEBUG
    if (sn_GetNetState()==nCLIENT)
        tERR_ERROR("rec_cont should not be called client-side!");
#endif

    // after control is received, we better sync this object with
    // the clients:

    RequestSync();
}

// control functions:
nMessage * nNetObject::NewControlMessage(){
    nMessage *m=new nMessage(net_control);
    m->Write(id);
    return m;
}



class nWaitForAckSync: public nWaitForAck{
    unsigned short netobj;
public:
    nWaitForAckSync(nMessage* m,int rec,unsigned short obj)
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
            if ( obj->knowsAbout[receiver].acksPending)
            {
                obj->knowsAbout[receiver].acksPending--;
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

            obj->knowsAbout[receiver].knowsAboutExistence=true;
        }
        else
        {
            //		st_Breakpoint();
        }
    }
};




static void net_sync_handler(nMessage &m){
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
            Cheater(m.SenderID());
#ifdef DEBUG
            tERR_ERROR("sync should only be called client-side!");
#endif
        }
        else if (obj->SyncIsNew(m)){
            m.Reset();
            m.Read(id);
            obj->ReadAll(m, false);
        }
    }
}

static nDescriptor net_sync(24,net_sync_handler,"net_sync");

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

//! returns the user that the current WriteSync() is intended for
int nNetObject::SyncedUser()
{
    return sn_syncedUser;
}

void nNetObject::SyncAll(){
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

                    if (// nos->knowsAbout[user].syncReq &&
                        !nos->knowsAbout[user].knowsAboutExistence)
                    {
                        // look for the object ID in the current destruction message
                        {
                            if ( destroyers[user] )
                            {
                                for( int i = destroyers[user]->DataLen()-1; i >= 0; --i )
                                {
                                    if ( destroyers[user]->Data(i) == s )
                                    {
                                        // found it. Send the destruction
                                        // message and wait for its ack.
                                        sn_SendDestroyer( user );
                                        break;
                                    }
                                }
                            }
                        }

                        if ( destroyersCount[user] )
                        {
                            // don't send creation messages while destruction
                            // messages are unacknowledged.
                            --currentSync;
                            continue;
                        }

                        if (!nos->knowsAbout[user].acksPending){
#ifdef DEBUG
                            //con << "remotely creating object " << s << '\n';
#endif
                            /*
                              con << "creating object " << s << " at user " << user
                              << " owned by " << sn_netObjects(s)->owner << '\n';
                            */
                            // send a creation message

                            tJUST_CONTROLLED_PTR< nMessage > m=new nMessage
                                                               (nos->CreatorDescriptor());

#ifdef DEBUG
                            if (s == sn_WatchNetID)
                                sn_WatchMessage = m;
#endif

                            nos->WriteAll(*m,true);
                            new nWaitForAckSync(m,user,s);
                            unsigned long id = m->MessageIDBig();
                            m->SendImmediately(user, false);
                            m->messageIDBig_ = id;

                            nos->knowsAbout[user].syncReq = false;
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
                    else if (nos->knowsAbout[user].syncReq
                             && sn_Connections[user].bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) >50
                             && nos->knowsAbout[user].acksPending<=1){
                        // send a sync
                        tJUST_CONTROLLED_PTR< nMessage > m = new nMessage(net_sync);

                        m->Write(s);
                        nos->WriteAll(*m,false);
                        nos->knowsAbout[user].syncReq=false;

                        if (nos->knowsAbout[user].nextSyncAck){
                            new nWaitForAckSync(m,user,s);
                            nos->knowsAbout[user].nextSyncAck=false;
                        }
#ifndef nSIMULATE_PING
                        unsigned long id = m->MessageIDBig();
                        //m->Send(user,0,false);
                        m->SendImmediately(user,false);
                        m->messageIDBig_ = id;
#endif
                    }

                }

                currentSync--;
            }

            // send the destroy messages
            if (destroyers[user] && !destroyersCount[user] )
            {
                // but check whether the opportunity is good (big destroyers message, or a sync packet in the pipe) first
                if ( destroyers[user]->DataLen() > 75 ||
                        sn_Connections[user].sendBuffer_.Len() > 0 ||
                        destroyersTime[user] < tSysTimeFloat()-.5-2*sn_Connections[user].ping.GetPing() )
                {
                    sn_SendDestroyer( user );
                }
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


static void ready_handler(nMessage &m)
{
    is_ready_to_get_objects[m.SenderID()]=true;

    // reset peer's ping, it's probably unreliable
    sn_Connections[m.SenderID()].ping.Timestep(100);
}

static nDescriptor ready(25,ready_handler,"ready to get objects");


static void net_clear_handler(nMessage &m){
    if (sn_GetNetState()!=nSERVER){
        nNetObject::ClearAll();
        first_fill_ids();
    }
}

static nDescriptor net_clear(26,net_clear_handler,"net_clear");


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
        if ( sn_Connections[i].socket )
        {
            sn_SendDestroyer(i);
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

    (tNEW(nMessage)(net_clear))->BroadCast(); // just to make sure..
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
#ifdef DEBUG
                        sn_BreakOnObjectID(i);
#endif
                        if (no->ActionOnQuit())
                        {
                            no->createdLocally=true;
                            tControlledPTR< nNetObject > bounce( no ); // destroy it, if noone wants it
                        }
                        else
                        {
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
            {
#ifdef DEBUG
                sn_BreakOnObjectID(i);
#endif
                sn_netObjectsOwner(i)=0;
            }
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
    (new nMessage(ready))->Send(0);
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
    sn_SendDestroyer(user);

    destroyersCount[user] = 0;
}

static nCallbackLoginLogout nlc(&login_callback);


static bool sync_ack[MAXCLIENTS+2];
static unsigned short c_sync=0;

static void sync_ack_handler(nMessage &m){
    unsigned short id;
    m.Read(id);
    if (id==c_sync)
        sync_ack[m.SenderID()]=true;
}

static nDescriptor sync_ack_nd(27,sync_ack_handler,"sync_ack");


static void sync_msg_handler(nMessage &m);
static nDescriptor sync_nd(28,sync_msg_handler,"sync_msg");

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
                tJUST_CONTROLLED_PTR< nMessage > m=new nMessage(sync_nd);
                *m << timeout;
                m->Write(sync_sn_netObjects);
                m->Write(c_sync);
                m->Send(0);
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
                tJUST_CONTROLLED_PTR< nMessage > m=new nMessage(sync_nd);
                *m << timeout;
                m->Write(sync_sn_netObjects);
                m->Write(c_sync);
                m->Send(user);
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

static void sync_msg_handler(nMessage &m){
    static bool recursion=false;
    if (!recursion){
        recursion=true;

        REAL timeout;
        unsigned short sync_sn_netObjects;

        // sync and write sync response
        m >> timeout;
        m.Read(sync_sn_netObjects);
        unsigned short c_sync;
        m.Read(c_sync);

        if (sn_GetNetState()!=nSERVER){
            sn_Sync(timeout+4,sync_sn_netObjects!=0,false);
            tJUST_CONTROLLED_PTR< nMessage > m=new nMessage(sync_ack_nd);
            m->Write(c_sync);
            m->Send(0);
        }
        else
        {
            // no time for syncing! write sync response immediately with low priority so the client
            // gets it as soon as all other things are sent
            tJUST_CONTROLLED_PTR< nMessage > response=new nMessage(sync_ack_nd);
            response->Write(c_sync);
            response->Send(m.SenderID(),10000000);
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
void nBandwidthTaskSync::DoExecute( nSendBuffer& buffer, nBandwidthControl& control )
{
    tJUST_CONTROLLED_PTR< nMessage > message = tNEW( nMessage )( net_sync );
    Object().WriteAll( *message, false );
    buffer.AddMessage( *message, &control );
}

// executes whatever it has to do
void nBandwidthTaskCreate::DoExecute( nSendBuffer& buffer, nBandwidthControl& control )
{
    tJUST_CONTROLLED_PTR< nMessage > message = tNEW( nMessage )( Object().CreatorDescriptor() );
    Object().WriteAll( *message, true );

    buffer.AddMessage( *message, &control );
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


