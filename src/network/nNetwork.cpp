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

#include "tMemManager.h"
#include "tInitExit.h"
#include "nSimulatePing.h"
#include "nConfig.h"
#include "nNetwork.h"
#include "tConsole.h"
#include "tDirectories.h"
#include "nSocket.h"
#include "nConfig.h"
#include "tSysTime.h"
#include "tRecorder.h"
#include "tRandom.h"
#include <stdlib.h>
#include <fstream>
#include "tMath.h"

#ifndef WIN32
#include  <netinet/in.h>
#else
#include  <windows.h>
#endif

#include <deque>

// debug watchs
#ifdef DEBUG
nMessage* sn_WatchMessage = NULL;
unsigned int sn_WatchMessageID = 76;
#endif

#define NO_ACK

tString sn_bigBrotherString;
// tString sn_greeting[5];  //made 4 = 5 (lol i broke the laws of maths. subby),  k's bug fix

tString sn_programVersion (VERSION)    ;

tString sn_serverName("Unnamed Server");

const unsigned int sn_defaultPort = 4534;
unsigned int sn_serverPort = 4534;

tString net_hostip("ANY");

bool big_brother=true;
static tConfItem<bool> sn_bb("BIG_BROTHER",big_brother);

static tConfItemLine sn_sn("SERVER_NAME", sn_serverName);

static tConfItem<int> sn_sport("SERVER_PORT",reinterpret_cast<int&>(sn_serverPort));

static tConfItemLine sn_sbtip("SERVER_IP", net_hostip);

void sn_DisconnectUserNoWarn(int i, const tOutput& reason );

int sn_defaultDelay=10000;

int sn_maxRateIn=8; // maximum data rate in kb/s
int sn_maxRateOut=8; // maximum output data rate in kb/s

static nConnectError sn_Error = nOK;

//tArray<unsigned short> send_buffer[MAXCLIENTS+2];
//REAL planned_rate_control[MAXCLIENTS+2];
//REAL rate_control[MAXCLIENTS+2];
//unsigned short  rate[MAXCLIENTS+2];

// from gGame.C
//extern unsigned short client_gamestate[MAXCLIENTS+2];

bool deb_net=false;

static REAL maxTimeout=1;  // the maximal timeout in seconds
static REAL minTimeout=.01;  // the minimal timeout in seconds
static REAL pingTimeout=1; // the normal timeout in multiples of the ping
static REAL pingVarianceTimeout=1; // the normal timeout in multiples of the ping variance
static REAL zeroTimeout=.01; // additional timeout of first packet

static REAL sn_GetTimeout( int user )
{
    tASSERT( user >= 0 && user <= MAXCLIENTS+1 );

    nPingAverager & averager = sn_Connections[ user ].ping;

    REAL timeout = pingTimeout * averager.GetPing() + pingVarianceTimeout * sqrtf( averager.GetSnailAverager().GetAverageVariance() );

    if ( timeout < minTimeout )
        timeout = minTimeout;
    if ( timeout > maxTimeout )
        timeout = maxTimeout;

    return timeout;
}

#ifndef DEBUG
static REAL killTimeout=30;
#else
static REAL killTimeout=30;
#endif

static const int kickOnDemandTimeout = 10;

static bool send_again_warn=false;

#ifdef DEBUG
static int simulate_loss=0;
#else
//static int simulate_loss=0;
#endif

int sn_maxNoAck=100; // the maximum number of not ack messages
// before more are send

//int sn_ackPending[MAXCLIENTS+2];
// int sn_ackAckPending[MAXCLIENTS+2];

//static nMessage * ack_mess[MAXCLIENTS+2];

static nNetState current_state;
//int sn_sockets[MAXCLIENTS+2];  // server mode:
// elements 1...MAXCLIENTS are the incoming connections,
// client mode: element 0 connects to the server.
// element MAXCLIENTS+1: currently logging in

nConnectionInfo sn_Connections[MAXCLIENTS+2];

static nAddress peers[MAXCLIENTS+2]; // the same logic for the peer adresses.
static int timeouts[MAXCLIENTS+2];

#define ACKBACK 1000
static unsigned short lastacks[MAXCLIENTS+2][ACKBACK];
static unsigned short lastackPos[MAXCLIENTS+2];
static unsigned short highest_ack[MAXCLIENTS+2];


//********************************************************
// Version control
//********************************************************

static int sn_MaxBackwardsCompatibility = 1000;
static tSettingItem<int> sn_mxc("BACKWARD_COMPATIBILITY",sn_MaxBackwardsCompatibility);

static int sn_newFeatureDelay = 0;
static tSettingItem<int> sn_nfd("NEW_FEATURE_DELAY",sn_newFeatureDelay);

// from nConfig.cpp. Adapt version string array there to bump protocol version.
int sn_GetCurrentProtocolVersion();

static const int sn_currentProtocolVersion              = sn_GetCurrentProtocolVersion(); // the current version number of the network protocol
static const int sn_backwardCompatibleProtocolVersion 	= 0;							// the smallest version of the network protocol this program is compatible with
static const nVersion sn_myVersion( sn_backwardCompatibleProtocolVersion, sn_currentProtocolVersion);
static nVersion sn_currentVersion( sn_myVersion );

const nVersion& sn_MyVersion()			// the version this progam maximally supports
{
    return sn_myVersion;
}

const nVersion& sn_CurrentVersion() 	// the version currently supported
{
    return sn_currentVersion;
}

nVersion::nVersion()
{
    min_=0;
    max_=0;
}

nVersion::nVersion( int min, int max )
{
    tASSERT( min <= max );
    min_=min;
    max_=max;
}

bool nVersion::Supported( int version ) const	// check if a particular version is supported
{
    tASSERT( min_ <= max_ );
    return version >= min_ && version <= max_;
}

bool nVersion::Merge( const nVersion& a,
                      const nVersion& b)	// merges two versions to one; the new version supports only features both versions understand. false is returned if no common denominator could be found
{
    int min = a.min_;
    if ( min < b.min_ )
    {
        min = b.min_;
    }

    int max = a.max_;
    if ( max > b.max_ )
    {
        max = b.max_;
    }

    if ( min <= max )
    {
        min_ = min;
        max_ = max;
        return true;
    }
    else
    {
        return false;
    }
}

bool nVersion::operator == ( const nVersion& other )
{
    return this->max_ == other.max_ && this->min_ == other.min_;
}

nVersion& nVersion::operator = ( const nVersion& other )
{
    this->min_ = other.min_;
    this->max_ = other.max_;

    return *this;
}

nMessage& operator >> ( nMessage& m, nVersion& ver )
{
    int min,max;
    m >> min;
    m >> max;

    ver = nVersion( min, max );

    return m;
}

nMessage& operator << ( nMessage& m, const nVersion& ver )
{
    m << ver.Min();
    m << ver.Max();

    return m;
}

std::istream& operator >> ( std::istream& s, nVersion& ver )
{
    int min,max;
    s >> min;
    s >> max;

    ver = nVersion( min, max );

    return s;
}

std::ostream& operator << ( std::ostream& s, const nVersion& ver )
{
    s << ver.Min() << " ";
    s << ver.Max();

    return s;
}

nVersionFeature::nVersionFeature( int min, int max ) // creates a feature that is supported from version min to max; values of -1 indicate no border
{
    tASSERT( min_ >= sn_MyVersion().Min() );
    tASSERT( max < 0 || max <= sn_MyVersion().Max() );

    min_ = min;
    max_ = max;
}

bool nVersionFeature::Supported()
{
    return ( min_ < 0 || sn_CurrentVersion().Max() >= min_ ) &&  ( max_ < 0 || sn_CurrentVersion().Min() <= max_ );
}

bool nVersionFeature::Supported( int client )
{
    if ( client < 0 || client > MAXCLIENTS )
        return false;

    // the version to check the feature for
    const nVersion * version = &sn_CurrentVersion();

    if ( sn_GetNetState() == nCLIENT )
    {
        // clientside code: override the currently active version with the server version ( if that has been sent )
        if ( sn_Connections[0].version.Max() > 0 )
            version = &sn_Connections[0].version;
    }
    else
    {
        // serverside code: override version to use with the client's version
        version = &sn_Connections[ client ].version;
    }

    // see if the feature is supported
    return ( min_ < 0 || version->Max() >= min_ ) &&  ( max_ < 0 || version->Min() <= max_ );
}

void handle_version_control( nMessage& m )
{
    if ( sn_GetNetState() == nCLIENT )
    {
        m >> sn_currentVersion;

        // inform configuration of changes
        nConfItemVersionWatcher::OnVersionChange( sn_currentVersion );
    }
}

nDescriptor versionControl(10, handle_version_control,"version" );

void sn_UpdateCurrentVersion()
{
    // update the current version from the native version and the versions of all attached clients

    // allow maximally sn_MaxBackwardsCompatibility old versions to connect
    int min = sn_myVersion.Max() - sn_MaxBackwardsCompatibility;
    if ( min < sn_myVersion.Min() )
        min = sn_myVersion.Min();

    // disable features that are too new
    int max = sn_myVersion.Max() - sn_newFeatureDelay;
    if ( max < min )
        max = min;

    nVersion version( min, max );

    // ask configuration if version is OK
    nConfItemVersionWatcher::AdaptVersion( version );

    nVersion maxVersion = version;

    if ( sn_GetNetState() == nCLIENT )
    {
        sn_currentVersion = version;
        return;
    }

    for ( int i = MAXCLIENTS; i>0; --i )
    {
        const nConnectionInfo& info = sn_Connections[i];
        if ( info.socket )
        {
            if ( ! version.Merge( version, info.version ) )
            {
                // kick user; it has gotten incompatible.
                static bool recurse = true;
                if ( recurse )
                {
                    recurse = false;
                    sn_DisconnectUser( i, "$network_kill_incompatible" );
                    recurse = true;
                }

                version = maxVersion;
            }
        }
    }

    // inform configuration of changes
    nConfItemVersionWatcher::OnVersionChange( version );

    if ( version != sn_currentVersion )
    {
        sn_currentVersion = version;

        nMessage* m = tNEW( nMessage )( versionControl );
        (*m) << version;

        m->BroadCast();
    }
}

//********************************************************

nConnectError sn_GetLastError()
{
    nConnectError ret = sn_Error;
    sn_Error = nOK;
    return ret;
}



// REAL sn_ping[MAXCLIENTS+2];

static void reset_last_acks(int i){
    for(int j=ACKBACK-1;j>=0;j--)
        lastacks[i][j]=0;
    lastackPos[i]=0;
    highest_ack[i]=0;
}


//#ifndef DEBUG
int sn_maxClients=MAXCLIENTS;

static tSettingItem< int > sn_maxClientsConf( "MAX_CLIENTS", sn_maxClients );

int sn_allowSameIPCountSoft=4;
static tSettingItem< int > sn_allowSameIPCountSoftConf( "MAX_CLIENTS_SAME_IP_SOFT", sn_allowSameIPCountSoft );

int sn_allowSameIPCountHard=8;
static tSettingItem< int > sn_allowSameIPCountHardConf( "MAX_CLIENTS_SAME_IP_HARD", sn_allowSameIPCountHard );

//#else
//int maxclients=1;
//#endif

int sn_myNetID=0; // our network identification:  0: server
//                                            1..MAXCLIENTS: client

#define IDS_RESERVED 16		 // number of message IDs reserved for special purposes: id 0 is reserved for no-ack messages.
unsigned short current_id=1; // current running network number


// the classes that are responsible for the queuing of network send tEvents:
class planned_send:public tHeapElement{
protected:
    int peer;
public:
    planned_send(REAL priority,int peer);
    ~planned_send();

    virtual tHeapBase *Heap() const; // in wich heap are we?

    // change our priority:
    void add_to_priority(REAL diff);

    // what is to be done if the sceduled tEvent is executed?
    virtual void execute()=0;
};

class nMessage_planned_send:public planned_send{
    tCONTROLLED_PTR(nMessage) m;
    bool ack;

public:
    nMessage_planned_send(nMessage *m,REAL priority,bool ack,int peer);
    ~nMessage_planned_send();

    virtual void execute();
};

// *************************************************************

unsigned short nDescriptor::s_nextID(1);

#define MAXDESCRIPTORS 400
static nDescriptor* descriptors[MAXDESCRIPTORS];

static nDescriptor* nDescriptor_anchor;

nDescriptor::nDescriptor(unsigned short identification,
                         nHandler *handle,const char *Name, bool awl)
        :tListItem<nDescriptor>(nDescriptor_anchor),
        id(identification),handler(handle),name(Name), acceptWithoutLogin(awl)
{
#ifdef DEBUG
#ifndef WIN32
    //  con << "Descriptor " << id << ": " << name << '\n';
#endif
#endif
    if (MAXDESCRIPTORS<=id || descriptors[id]!=NULL){
        con << "Descriptor " << id << " already used!\n";
        exit(-1);
    }
    s_nextID=id+1;
    descriptors[id]=this;
}

/*
nDescriptor::nDescriptor(nHandler *handle,const char *Name)
  :id(s_nextID++),handler(handle),name(Name)
{
#ifdef DEBUG
  con << "Descriptor " << id << ": " << name << '\n';
#endif

  if (descriptors.Len()>id && descriptors[id]!=NULL){
    con << "Descriptor " << id << " already used!\n";
    exit(-1);
  }
  descriptors[id]=this;
}
*/

int nCurrentSenderID::currentSenderID_ = 0;

void nDescriptor::HandleMessage(nMessage &message){
    static tArray<bool> warned;

    // store sender ID for console
    nCurrentSenderID currentSender( message.SenderID() );

#ifdef DEBUG_X
    if (message.descriptor>1)
        con << "RMT " << message.descriptor << "\n";
#endif

#ifndef NOEXCEPT
    try{
#endif
        nDescriptor *nd = 0;

        // z-man: security check ( thanks, Luigi Auriemma! )
        if ( message.descriptor  < MAXDESCRIPTORS )
            nd=descriptors[message.descriptor];

        if (nd){
            if ((message.SenderID() <= MAXCLIENTS) || nd->acceptWithoutLogin)
                nd->handler(message);
        }
        else
            if (!warned[message.Descriptor()]){
                tOutput warn;
                warn.SetTemplateParameter(1, message.Descriptor());
                warn << "$network_warn_unknowndescriptor";
                con << warn;
                warned[message.Descriptor()]=true;
            }
#ifndef NOEXCEPT
    }
    catch(nKillHim){
        // st_Breakpoint();
        con << tOutput("$network_error");
        sn_DisconnectUser(message.SenderID(), "$network_kill_error" );
    }

#endif
}

// *************************************************************


void ack_handler(nMessage &m){
    while (!m.End()){
        sn_Connections[m.SenderID()].AckReceived();

        unsigned short ack;
        m.Read(ack);
        //con << "Got ack:" << ack << ":" << m.SenderID() << '\n';
        nWaitForAck::Ackt(ack,m.SenderID());
    }
}

static nDescriptor s_Acknowledge(1,ack_handler,"ack");


class nWaitForAck;
static tList<nWaitForAck> sn_pendingAcks;

//static eTimer netTimer;
static nTimeRolling netTime;

#ifdef NET_DEBUG
static int acks=0;
static int max_acks=0;
#endif

nWaitForAck::nWaitForAck(nMessage* m,int rec)
        :id(-1),message(m),receiver(rec)
{
#ifdef DEBUG
    // don't message yourself
    if ( rec == 0 && sn_GetNetState() == nSERVER )
        st_Breakpoint();
#endif

    if (!message)
        tERR_ERROR("Null ack!");

    if (message->Descriptor()!=s_Acknowledge.ID())
        sn_Connections[receiver].ackPending++;
    else
        tASSERT(false);
    //    sn_ackAckPending[receiver]++;
#ifdef NET_DEBUG
    acks++;
#endif

    timeFirstSent=::netTime;
    timeLastSent=::netTime;

    timeouts=0;

    REAL timeout=sn_GetTimeout( rec );

#ifdef nSIMULATE_PING
    timeSendAgain=::netTime + nSIMULATE_PING;
#ifndef WIN32
    tRandomizer & randomizer = tReproducibleRandomizer::GetInstance();
    timeSendAgain+= randimizer.Get() * nSIMULATE_PING_VARIANT;
    // timeSendAgain+=(nSIMULATE_PING_VARIANT*random())/RAND_MAX;
#endif
#else
    const REAL packetLossScale = .003; // packet loss rate that is considered big
    const REAL maxTimeoutFactor = 1.2; // maximal stretching of initial timeout value for flawless connections
    // factor mutliplied to timeout; 1 if the connection loses a lot of packets, 1.2 for a
    // flawless connection
    REAL timeoutFactor = 1 + (maxTimeoutFactor-1)*packetLossScale/(sn_Connections[receiver].PacketLoss() + packetLossScale);
    timeSendAgain=::netTime + timeout*timeoutFactor + zeroTimeout;
#endif
    sn_pendingAcks.Add(this,id);
}

nWaitForAck::~nWaitForAck(){
#ifdef NET_DEBUG
    acks--;
    if (acks>max_acks){
        max_acks=acks;
        // con << "MA=" << max_acks << '\n';
    }
#endif

    if (bool(message) && message->Descriptor()!=s_Acknowledge.ID())
    {
        sn_Connections[receiver].ackPending--;
        sn_Connections[receiver].ReliableMessageSent();
    }
    else
        tASSERT(false);
    //    sn_ackAckPending[receiver]--;

    sn_pendingAcks.Remove(this,id);
    tCHECK_DEST;
}

void nWaitForAck::Ackt(unsigned short id,unsigned short peer){
    int success=0;
    for(int i=sn_pendingAcks.Len()-1;i>=0;i--){
        nWaitForAck * ack = sn_pendingAcks(i);
        if (ack->message->MessageID()==id &&
                ack->receiver==peer){
            success=1;

#ifdef DEBUG
            //      if (sn_pendingAcks(i)->message == sn_WatchMessage)
            //	st_Breakpoint();
#endif

#ifdef DEBUG_X
            if (ack->message->descriptor>1)
                con << "AT  " << ack->message->descriptor << '\n';
#endif

            // calculate and average ping
            REAL thisping=netTime - ack->timeFirstSent;
            sn_Connections[peer].ping.Add( thisping, exp(REAL(-ack->timeouts)) );

            ack->AckExtraAction();
            delete ack;
            ::timeouts[peer]=0;
            if (i<sn_pendingAcks.Len()-1) i++;
        }
    }

#ifdef DEBUG_X
    if (!success && peer!=MAXCLIENTS+1)
    {
        con << "Ack " << id << ':' << peer << " was not asked for.\n";
        if (sn_pendingAcks.Len()) con << "Expected:\n";
        for(int i=sn_pendingAcks.Len()-1;i>=0;i--){
            con << i << "\t:"
            << sn_pendingAcks[i]->message->messageIDBig_ << ":"
            << sn_pendingAcks[i]->receiver << '\n';
        }
    }
#endif
}

void nWaitForAck::AckAllPeer(unsigned short peer){
    for(int i=sn_pendingAcks.Len()-1;i>=0;i--){
        if (sn_pendingAcks(i)->receiver==peer){
            delete sn_pendingAcks(i);
            if (i<sn_pendingAcks.Len()-1) i++;
        }
    }
}

void nWaitForAck::Resend(){
    static tReproducibleRandomizer randomizer;

    for(int i=sn_pendingAcks.Len()-1;i>=0;i--){
        nWaitForAck* pendingAck = sn_pendingAcks(i);

        nConnectionInfo & connection = sn_Connections[pendingAck->receiver];

        // don't resend if you can't.
        if ( !connection.bandwidthControl_.CanSend() )
            continue;

        REAL packetLoss = connection.PacketLoss();
        REAL timeout = sn_GetTimeout( pendingAck->receiver );

        // should we resend the packet? Certainly it if it is overdue
        bool resend = (pendingAck->timeSendAgain + timeout * .1 <=netTime);

        // or if there is already a message waiting...
        if ( !resend && connection.sendBuffer_.Len() > 0 )
        {
            // and we are on time
            if ( pendingAck->timeSendAgain <= netTime )
                resend = true;
            // or the packet loss is so high that it is advisable to resend every message
            // multiple times if bandwidth is available ( we aim for 99% reliability )
            else if ( pendingAck->timeouts < 3 && pow( packetLoss, pendingAck->timeouts + 1 ) > .01 &&
                      connection.bandwidthControl_.Control( nBandwidthControl::Usage_Planning ) >100 )
                resend = true;

            /* + sn_GetTimeout( pendingAck->receiver ) *
                                ( 3.0 / ( pendingAck->timeouts + 1 ) )
                                ( packetLoss * ( randomizer.Get() + .5 ) ) )
            */
        }

        if ( resend ){
            //con << net_ticks-sn_pendingAcks[i]->ticks_first_sent << '\n';

            // update timeout counters
            ::timeouts[pendingAck->receiver]++;
            pendingAck->timeouts++;

            if(netTime - pendingAck->timeFirstSent  >  killTimeout &&
                    ::timeouts[pendingAck->receiver] > 20){
                // total timeout. Kill connection.
                if (pendingAck->receiver<=MAXCLIENTS){
                    tOutput o;
                    o.SetTemplateParameter(1, pendingAck->receiver);
                    o << "$network_error_timeout";
                    con << o;
                    sn_DisconnectUser(pendingAck->receiver, "$network_kill_timeout" );

                    sn_Error = nTIMEOUT;

                    if (i>=sn_pendingAcks.Len())
                        i=sn_pendingAcks.Len()-1;
                }
                else // it is just in the login slot. Ignore it.
                    delete pendingAck;
            }
            else{
#ifdef DEBUG
                //if (pendingAck->message == sn_WatchMessage)
                //st_Breakpoint();
#endif

                if (connection.socket){
                    //	  if(sn_Connections[].rateControlPlanned[pendingAck->receiver]>-1000)
                    {
                        REAL timeoutFactor = .9 + .1 * pendingAck->timeouts + randomizer.Get() * .1;
                        pendingAck->timeSendAgain=netTime+timeout * timeoutFactor;
                        pendingAck->timeLastSent=netTime;

                        if (send_again_warn){
                            con << "sending packet again: " ;
                            deb_net=true;
                        }
                        connection.ReliableMessageSent();
                        pendingAck->message->SendImmediately
                        (pendingAck->receiver,false);
                        deb_net=false;
                    }
                }
                else
                    delete pendingAck;
            }
        }
    }
}


// defined in netobjec.C
// void ClearKnows(int user);


#ifdef NET_DEBUG
static int nMessages=0;
static int max_nMessages=0;
#endif

#ifdef DEBUG
void BreakOnMessageID( unsigned int messageID )
{
    if (messageID == sn_WatchMessageID && messageID != 0 )
    {
        int x;
        x = 0;
    }
}
#endif

class nMessageIDExpander
{
    unsigned long quarters[4];
public:
    nMessageIDExpander()
    {
        for (int i=3; i>=0; --i)
            quarters[i]=i << 14;
    }

    unsigned long ExpandMessageID( unsigned short id )
    {
        // the current ID is in this quarter
        int thisQuarter = ( id >> 14 ) & 3;

        // the following quarter will be this
        int nextQuarter = ( thisQuarter + 1 ) & 3;

        // make sure the following quarter has a higher upper ID completion than this
        quarters[nextQuarter] = quarters[thisQuarter] + ( 1 << 14 );

        // replace high two bits of incoming ID with the counted up ID
        return quarters[thisQuarter] | id;
    }
};

//! expands a short message ID to a full int message ID, assuming it is from a message that was
// just received.
static unsigned long int sn_ExpandMessageID( unsigned short id, unsigned short sender )
{
#ifdef DEBUG
    BreakOnMessageID( id );
#endif

    static nMessageIDExpander expanders[MAXCLIENTS+2];

    tASSERT( sender <= MAXCLIENTS+2 )
    return expanders[sender].ExpandMessageID(id);
}

nMessage::nMessage(unsigned short*& buffer,short sender, int lenLeft )
        :descriptor(ntohs(*(buffer++))),messageIDBig_(sn_ExpandMessageID(ntohs(*(buffer++)),sender)),
senderID(sender),readOut(0){
#ifdef NET_DEBUG
    nMessages++;
#endif

    tRecorderSync< unsigned long >::Archive( "_MESSAGE_ID_IN", 3, messageIDBig_ );
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_DECL_IN", 3, descriptor );

    unsigned short len=ntohs(*(buffer++));
    lenLeft--;
    if ( len > lenLeft )
    {
        len = lenLeft;
#ifndef NOEXCEPT
        throw nKillHim();
#endif
    }
    for(int i=0;i<len;i++)
        data[i]=ntohs(*(buffer++));

#ifdef DEBUG
    BreakOnMessageID( messageIDBig_ );
#endif
}

nMessage::nMessage(const nDescriptor &d)
        :descriptor(d.id),
senderID(::sn_myNetID), readOut(0){
#ifdef NET_DEBUG
    nMessages++;
#endif

    current_id++;
    if (current_id <= IDS_RESERVED)
        current_id = IDS_RESERVED + 1;

    messageIDBig_ = current_id;

#ifdef DEBUG
    BreakOnMessageID( messageIDBig_ );
#endif

    tRecorderSync< unsigned long >::Archive( "_MESSAGE_ID_OUT", 3, messageIDBig_ );
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_DECL_OUT", 3, descriptor );
    unsigned short len = DataLen();
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_LEN_OUT", 3, len );
}


nMessage::~nMessage(){
#ifdef NET_DEBUG
    nMessages--;
    if (nMessages>max_nMessages){
        max_nMessages=nMessages;
        con << "MN=" << max_nMessages <<'\n';
    }
#endif

#ifdef DEBUG_X
    if (descriptor>1)
        con << "DMT " << descriptor << "\n";
#endif

    tCHECK_DEST;
}




void nMessage::BroadCast(bool ack){
    tControlledPTR< nMessage > keep( this );
    if (sn_GetNetState()==nCLIENT)
        Send(0,ack);

    if (sn_GetNetState()==nSERVER){
        for(int i=MAXCLIENTS;i>0;i--){
            if (sn_Connections[i].socket)
                Send(i,ack);
        }
    }
}


static nVersionFeature sn_ZeroMessageCrashfix( 1 );

nMessage& nMessage::operator << (const tString &s){
    if ( !sn_ZeroMessageCrashfix.Supported() && s.Len() <= 0 )
    {
        return this->operator<<( s + " " );
    }

    unsigned short len=s.Len();
    Write(len);
    int i;

    // write first pairs of bytes
    for(i=0;i+1<len;i+=2)
        Write(s[i]+(s[i+1] << 8));

    // write last byte
    if (i<len)
        Write(s[i]);

    return *this;
}

nMessage& nMessage::operator << (const tColoredString &s){
    return *this << static_cast< const tString & >( s );
}

bool sn_filterColorStrings = false;
static tConfItem<bool> cs("FILTER_COLOR_STRINGS",sn_filterColorStrings);

nMessage& nMessage::operator >> (tColoredString &s )
{
    s.Clear();
    unsigned short w,len;
    Read(len);
    if ( len > 0 )
    {
        s[len-1] = 0;
        for(int i=0;i<len;i+=2){
            Read(w);
            s[i]=w & 255;
            if (i+1<len)
                s[i+1]=(w-s[i]) >> 8;
        }
    }

    // filter client string messages
    if ( sn_GetNetState() == nSERVER )
    {
        s.NetFilter();
        s.RemoveTrailingColor();
    }

    // filter color codes away
    if ( sn_filterColorStrings )
        s = tColoredString::RemoveColors( s );

    return *this;
}

nMessage& nMessage::operator >> (tString &s )
{
    tColoredString safe;
    operator>>( safe );
    s = safe;

    return *this;
}


#define MANT 26
#define EXP (32-MANT)
#define MS (MANT-1)


typedef struct{
int mant:MANT;
unsigned int exp:EXP;
} myfloat;


nMessage& nMessage::operator<<(const REAL &x){


#ifdef DEBUG
    // con << "write x= " << x;


    if(sizeof(myfloat)!=sizeof(int))
        tERR_ERROR_INT("floating ePoint format does not work!");
#endif
    /*
      REAL nachkomma=x-floor(x);
      Write(short(x-nachkomma));
      Write(60000*nachkomma);
    */
    // no fuss. Read and write floats in binary format.
    // will likely cause problems for systems other than i386.

    //Write(((short *)&x)[0]);
    //Write(((short *)&x)[1]);

    // right. Caused severe problems with the AIX port.

    // new way: own floating ePoint format that is not good with small numbers
    // (we do not need them anyway)
    REAL y=x;

    unsigned int negative=0;
    if (y<0){
        y=-y;
        negative=1;
    }

    unsigned int exp=0;
    while ( fabs(y)>=64 && exp < (1<<EXP)-6 )
    {
        exp +=6;
        y/=64;
    }
    while ( fabs(y)>=1 && exp < (1<<EXP)-1 )
    {
        exp++;
        y/=2;
    }
    // now x=y*2^exp
    unsigned int mant=int(y*(1<<MS));
    // now x=mant*2^exp * (1/ (1<<MANT))

    // cutoffs:
    if (mant>((1<<MS))-1)
        mant=(1<<MS)-1;

    if (exp>(1<<EXP)-1){
        exp=(1<<EXP)-1;
        if (mant>0)
            mant=(1<<MS)-1;
    }

    // put them together:

    unsigned int trans=(mant & ((1<<MS)-1)) | (negative << MS) | (exp << MANT);
    /*
      myfloat trans;
      trans.exp=exp;
      trans.mant=mant;
    */

    operator<<(reinterpret_cast<int &>(trans));

#ifdef DEBUG
    /*
      con << "mant: " << mant
      << ", exp: " << exp
      << ", negative: " << negative;
    */

    unsigned int mant2=trans & ((1 << MS)-1);
    unsigned int negative2=(trans >> MS) & 1;
    unsigned int nt=trans-mant-(negative << MS);
    unsigned int exp2=nt >> MANT;

    if (mant2!=mant || negative2!=negative || exp2!=exp)
        tERR_ERROR_INT("Floating ePoint tranfer failure!");

    /*
      con << ", x: " << x;

      con << ", mant: " << mant
      << ", exp: " << exp
      << ", negative: " << negative;
    */

    // check:

    REAL z=REAL(mant)/(1<<MS);
    if (negative)
        z=-z;

    while (exp>=6){
        exp-=6;
        z*=64;
    }
    while (exp>0){
        exp--;
        z*=2;
    }

    if (fabs(z-x)>(fabs(x)+1)*.001)
        tERR_ERROR_INT("Floating ePoint tranfer failure!");

    //con << ", z: " << z << '\n';
#endif

    return *this;
}

nMessage& nMessage::operator>>(REAL &x){
    /*
      short vorkomma;
      unsigned short nachkomma;
      Read((unsigned short &)vorkomma);
      Read(nachkomma);
      x=vorkomma+nachkomma/60000.0;
     
      Read(((unsigned short *)&x)[0]);
      Read(((unsigned short *)&x)[1]);
     */

    unsigned int trans;
    operator>>(reinterpret_cast<int &>(trans));

    int mant=trans & ((1 << MS)-1);
    unsigned int negative=(trans >> MS) & 1;
    unsigned int exp=(trans-mant-(negative << MS)) >> MANT;

    x=REAL(mant)/(1<<MS);
    if (negative)
        x=-x;

#ifdef DEBUG
    //  con << "read mant: " <<mant << ", exp: " << exp;
#endif

    while (exp>=6){
        exp-=6;
        x*=64;
    }
    while (exp>0){
        exp--;
        x*=2;
    }

#ifdef DEBUG
#ifndef WIN32
    if (!finite(x))
        st_Breakpoint();
    // con << " , x= " << x << '\n';
#endif
#endif
    return *this;
}

nMessage& nMessage::operator<< (const short &x){
    Write((reinterpret_cast<const short *>(&x))[0]);

    return *this;
}

nMessage& nMessage::operator>> (short &x){
    Read(reinterpret_cast<unsigned short *>(&x)[0]);

    return *this;
}

nMessage& nMessage::operator<< (const int &x){
    unsigned short a=x & (0xFFFF);
    short b=(x-a) >> 16;

    Write(a);
    operator<<(b);

    return *this;
}

nMessage& nMessage::operator>> (int &x){
    unsigned short a;
    short b;

    Read(a);
    operator>>(b);

    x=(b << 16)+a;

    return *this;
}

nMessage& nMessage::operator<< (const bool &x){
    if (x)
        Write(1);
    else
        Write(0);

    return *this;
}

nMessage& nMessage::operator>> (bool &x){
    unsigned short y;
    Read(y);
    x= (y!=0);

    return *this;
}


void nMessage::Read(unsigned short &x){
    if (End()){
        tOutput o;
        st_Breakpoint();
        o.SetTemplateParameter(1, senderID);
        o << "$network_error_shortmessage";
        con << o;
        sn_DisconnectUser(senderID, "$network_kill_error");
        nReadError();
    }
    else
        x=data(readOut++);
}


// **********************************************
//  Basic communication classes: login
// **********************************************

static bool login_failed=false;
static bool login_succeeded=false;

static nHandler *real_req_info_handler=NULL;

void req_info_handler(nMessage &m){
    if (real_req_info_handler)
        (*real_req_info_handler)(m);
    if (m.SenderID()==MAXCLIENTS+1)
        sn_DisconnectUser(MAXCLIENTS+1, "$network_kill_logout");
}

static nDescriptor req_info(2,req_info_handler,"req_info");

void RequestInfoHandler(nHandler *handle){
    real_req_info_handler=handle;
}


void login_deny_handler(nMessage &m){
    if ( !m.End() )
    {
        //		tOutput output;
        //		m >> output;
        //		sn_DenyReason = output;
        m >> sn_DenyReason;
    }
    else
    {
        sn_DenyReason = tOutput( "$network_kill_unknown" );
    }

    if (!login_failed)
        con << tOutput("$network_login_denial");
    if (sn_GetNetState()!=nSERVER){
        login_failed=true;
        login_succeeded=false;
        sn_SetNetState(nSTANDALONE);
    }
}

static nDescriptor login_deny(3,login_deny_handler,"login_deny");

void login_handler_1( nMessage&m );
void login_handler_2( nMessage&m );
void logout_handler( nMessage&m );

nDescriptor login(6,login_handler_1,"login1", true);
nDescriptor login_2(11,login_handler_2,"login2", true);
nDescriptor logout(7,logout_handler,"logout");

tString sn_DenyReason;

void login_ignore_handler(nMessage &m){
    if (sn_GetNetState()!=nSERVER && !login_succeeded){
        /*
          login_failed=true;
          login_succeeded=false;

          // kicking the one who uses our place
          // (he is probably timing out anyway..)
          nMessage *lo=new nMessage(logout);
          lo->Write((unsigned short)sn_myNetID);
          lo->Send(0);

          sn_Sync(10);
          
          (new nMessage(login))->Send(0);
        */
    }


}

static nDescriptor login_ignore(4,login_ignore_handler,"login_ignore");


void first_fill_ids();

void login_accept_handler(nMessage &m){
    if (sn_GetNetState()!=nSERVER && m.SenderID() == 0){
        login_succeeded=true;
        unsigned short id=0;
        m.Read(id);
        sn_myNetID=id;

        // read or reset server version info
        if ( !m.End() )
        {
            m >> sn_Connections[0].version;

#ifndef DEBUG
#ifndef DEDICATED
            // expiration for public beta versions
            if ( ( strstr( VERSION, "rc" ) || strstr( VERSION, "alpha" ) || strstr( VERSION, "beta" ) ) &&
                    sn_Connections[0].version.Max() > sn_currentProtocolVersion + 1 )
            {
                throw tGenericException( tOutput("$testing_version_expired"), tOutput("$testing_version_expired_title" ) );
            }
#endif
#endif
        }
        else
            sn_Connections[0].version = nVersion( 0, 0);

        first_fill_ids();
    }
}

static nDescriptor login_accept(5,login_accept_handler,"login_accept", true);



//static short new_id=0;

// counts the number of logins with the same IP
int CountSameIP( int user, bool reset=false )
{
    static int sameIP[ MAXCLIENTS+2 ];
    tASSERT( user >= 0 && user <= MAXCLIENTS+1 );

    if ( reset )
    {
        int count = 0;
        for(int user2=1;user2<=sn_maxClients;++user2)
        {
            if(!sn_Connections[user2].socket)
                continue;

            if ( user2 != user && nAddress::Compare( peers[user], peers[user2] ) >=0 )
            {
                count++;
            }
        }

        sameIP[user] = count;
    }

    return sameIP[user];
}

// counts the number of logins with the same Connection
int CountSameConnection( int user )
{
    int count = 0;
    for(int user2=1;user2<=sn_maxClients;++user2)
    {
        if( NULL == sn_Connections[user2].socket )
            continue;

        if ( user2 != user && nAddress::Compare( peers[user], peers[user2] ) == 0 )
        {
            count++;
        }
    }

    return count;
}

// determine a free connection slot or at least one where the user won't be missed
int GetFreeSlot()
{
    int user;

    // level 1: look for free slot
    for(user=1;user<=sn_maxClients;++user)
    {
        // look for empty slot
        if(!sn_Connections[user].socket)
        {
            return user;
        }
    }

    int best = -1;

    // level 2: look for slot timing out anyway
    // z-man: not a good idea after all, causes unjustified kicks...
    /*
    //int bestTimeout = kickOnDemandTimeout;

       for(user=1;user<=sn_maxClients;++user)
       {
           int timeout = timeouts[user];
           if ( timeout > bestTimeout )
           {
               bestTimeout = timeout;
               best = user;
           }
       }
       if ( best > 0 )
           return best;
    */

    int bestCount = sn_allowSameIPCountSoft-1;

    // level 3: look for dublicate IPs
    for(user=1;user<=sn_maxClients;++user)
    {
        int count = CountSameIP( user );
        if ( count > bestCount )
        {
            bestCount = count;
            best = user;
        }
    }
    if ( best > 0 )
        return best;

    return -1;
}

static REAL sn_minBan    = 120; // minimal ban time in seconds for trying to connect while you're banned
static tSettingItem< REAL > sn_minBanSetting( "NETWORK_MIN_BAN", sn_minBan );

// defined in nServerInfo.cpp
extern bool FloodProtection( nMessage const & m );

// flag to disable 0.2.8 test version lockout
static bool sn_lockOut028tTest = true;
static tSettingItem< bool > sn_lockOut028TestConf( "NETWORK_LOCK_OUT_028_TEST", sn_lockOut028tTest );

int login_handler(const nMessage &m, const nVersion& version, unsigned short rate ){
    nCurrentSenderID senderID;

    // don't accept logins in client mode
    if (sn_GetNetState() != nSERVER)
        return -1;

    // ban users
    nMachine & machine = nMachine::GetMachine( m.SenderID() );
    REAL banned = machine.IsBanned();
    if ( banned > 0 )
    {
        // the reason for the ban
        tString const & reason = machine.GetBanReason();

        // ban user some more so he learns
        if ( banned < sn_minBan )
        {
            machine.Ban( sn_minBan );
            banned = sn_minBan;
        }
        else
            con << tOutput( "$network_ban", machine.GetIP() , int(banned/60), reason.Len() > 1 ? reason : tOutput( "$network_ban_noreason" ) );

        sn_DisconnectUser(m.SenderID(), tOutput( "$network_kill_banned", int(banned/60), reason ) );
    }

    // ignore multiple logins
    if( CountSameConnection( m.SenderID() ) > 0 )
        return -1;

    // ignore login floods
    if ( FloodProtection( m ) )
        return -1;

    bool success=false;

    int new_id = -1;

    // test
    //	sn_DisconnectUser(m.SenderID(), "$network_kill_incompatible");
    //	return -1;

    nVersion mergedVersion;
    if ( !mergedVersion.Merge( version, sn_CurrentVersion() ) )
    {
        sn_DisconnectUser(m.SenderID(), "$network_kill_incompatible");
    }

    // expire 0.2.8 test versions, they have a security flaw
    if ( sn_lockOut028tTest && version.Max() >= 5 && version.Max() <= 10 )
    {
        sn_DisconnectUser(m.SenderID(), "0.2.8_beta and 0.2.8.0_rc versions have a dangerous security flaw and are obsoleted, please upgrade to 0.2.8.0.");
    }

    if (m.SenderID()!=MAXCLIENTS+1)
    {
        //con << "Ignoring second login from " << m.SenderID() << ".\n";
        (new nMessage(login_ignore))->Send(m.SenderID());
    }
    else if (sn_Connections[m.SenderID()].socket)
    {
        if ( sn_maxClients > MAXCLIENTS )
            sn_maxClients = MAXCLIENTS;

        // count doublicate IPs
        if ( CountSameIP( m.SenderID(), true ) < sn_allowSameIPCountHard )
        {
            // find new free ( or freeable ) ID
            new_id = GetFreeSlot();
            if ( new_id > 0 )
            {
                if(sn_Connections[new_id].socket)
                    sn_DisconnectUser( new_id, "$network_kill_full" );

                success = true;

                senderID.SetID( new_id );

                sn_Connections	[ new_id ].socket	= sn_Connections[MAXCLIENTS+1].socket; // the new connection has number MC+1
                peers			[ new_id ]			= peers[MAXCLIENTS+1];
                timeouts		[ new_id ]			= kickOnDemandTimeout/2;

                // sn_Connections	[ MAXCLIENTS+1 ].socket		= NULL;
                // peers			[ MAXCLIENTS+1 ].sa_family	= 0;
                //				nCallbackLoginLogout::UserLoggedIn(i);

                // recount doublicate IPs
                CountSameIP( new_id, true );
            }
        }

        // log login to console
        tOutput o;
        o.SetTemplateParameter(1, peers[m.SenderID()].ToString() );
        o.SetTemplateParameter(2, sn_Connections[m.SenderID()].socket->GetAddress().ToString() );
        o << "$network_server_login";
        con << o;
    }
    if (success)
    {
        tOutput o;
        o.SetTemplateParameter(1, new_id);
        o << "$network_server_login_success";
        con << o;
        //      tString s;
        // s << "User " << new_id << " logged in.\n";

        nCallbackLoginLogout::UserLoggedIn(new_id);

        sn_Connections[new_id].ping.Reset();
        sn_Connections[new_id].bandwidthControl_.Reset();
        reset_last_acks(new_id);

        if (rate>sn_maxRateOut)
            rate=sn_maxRateOut;

        sn_Connections[new_id].bandwidthControl_.SetRate( rate );
        sn_Connections[new_id].version = version;

        nWaitForAck::AckAllPeer(MAXCLIENTS+1);
        reset_last_acks(MAXCLIENTS+1);
        if (sn_Connections[MAXCLIENTS+1].ackMess)
        {
            sn_Connections[MAXCLIENTS+1].ackMess=NULL;
        }

        // send login accept message with high priority
        nMessage *rep=new nMessage(login_accept);
        rep->Write(new_id);
        (*rep) << sn_myVersion;
        rep->Send(new_id, -killTimeout);

        nMessage::SendCollected( new_id );

        nConfItemBase::s_SendConfig(true, new_id);

        // fake activity
        nMachine & machine = nMachine::GetMachine( new_id );
        machine.AddPlayer();
        machine.RemovePlayer();

        //      ANET_Listen(false);
        //      ANET_Listen(true);
    }
    else if (m.SenderID()==MAXCLIENTS+1)
    {
        sn_DisconnectUser(MAXCLIENTS+1, "$network_kill_full");
    }

    sn_UpdateCurrentVersion();

    return new_id;
}

void login_handler_1(nMessage& m)
{
    nVersion version;
    unsigned short rate;

    m.Read( rate );

    if ( !m.End() ){ // we get a big brother message (ignore it)
        tString rem_bb;
        m >> rem_bb;
    }

    if ( !m.End() )
    {
        // version!
        m >> version;
    }

    login_handler( m, version, rate );
}

void login_handler_2(nMessage& m)
{
    unsigned short rate;
    unsigned short bb;

    m.Read( rate );
    m.Read( bb );
    tString rem_bb;

    if ( bb )
    { // we get a big brother message
        m >> rem_bb;
    }

    nVersion ver;
    m >> ver;

    int new_ID = login_handler( m, ver , rate );

    if ( new_ID > 0 )
    {
        nMessage* m = tNEW( nMessage )( versionControl );
        (*m) << sn_currentVersion;

        m->Send( new_ID );

        if ( bb )
        {
            std::ofstream s;
            if ( tDirectories::Var().Open(s, "big_brother",std::ios::app) )
                s << rem_bb << '\n';
        }
    }
}


void logout_handler(nMessage &m){
    unsigned short id = m.SenderID();
    //m.Read(id);

    if (sn_Connections[id].socket)
    {
        tOutput o;
        o.SetTemplateParameter(1, id);
        o << "$network_logout_server";
        con << o;
    }
    nWaitForAck::AckAllPeer(id);

    if (0<id && id<=sn_maxClients)
        sn_DisconnectUser(id, "$network_kill_logout");
}


#define MAX_MESS_LEN 300
#define OVERHEAD 32

static REAL sn_OrderPriority = 0;

// statistics
int sn_SentBytes        = 0;
int sn_SentPackets      = 0;
int sn_ReceivedBytes    = 0;
int sn_ReceivedPackets  = 0;
nTimeRolling sn_StatsTime		= 0;


// adds a message to the buffer
void nSendBuffer::AddMessage	( nMessage&			message, nBandwidthControl* control )
{
    unsigned long id = message.MessageID();
    unsigned short len = message.DataLen();
    tRecorderSync< unsigned long >::Archive( "_MESSAGE_ID_SEND", 5, id );
    tRecorderSync< unsigned short >::Archive( "_MESSAGE_SEND_LEN", 5, len );

    sendBuffer_[sendBuffer_.Len()]=htons(message.Descriptor());

    sendBuffer_[sendBuffer_.Len()]=htons(message.MessageID());

    sendBuffer_[sendBuffer_.Len()]=htons(message.DataLen());
    for(int i=0;i<len;i++)
        sendBuffer_[sendBuffer_.Len()]=htons(message.Data(i));

    if ( control )
    {
        control->Use( nBandwidthControl::Usage_Planning, len * 2 );
    }
}

// send the contents of the buffer to a specific socket
void nSendBuffer::Send			( nSocket const &				socket
                           , const nAddress &	peer
                           ,nBandwidthControl* control )
{
    if (sendBuffer_.Len()){
        sn_SentPackets++;
        sn_SentBytes  += sendBuffer_.Len() * 2 + OVERHEAD;

        // store our id
        sendBuffer_[sendBuffer_.Len()]=htons(::sn_myNetID);

        socket.Write( reinterpret_cast<int8 *>(&(sendBuffer_[0])),
                      2*sendBuffer_.Len(), peer);

        if ( control )
        {
            control->Use( nBandwidthControl::Usage_Execution, 2*sendBuffer_.Len() + OVERHEAD );
        }

        this->Clear();
    }
}

// broadcast the contents of the buffer
void nSendBuffer::Broadcast	( nSocket const &				socket
                              , int				port
                              , nBandwidthControl* control )
{
    if (sendBuffer_.Len()){
        sn_SentPackets++;
        sn_SentBytes  += sendBuffer_.Len() * 2 + OVERHEAD;

        // store our id
        sendBuffer_[sendBuffer_.Len()]=htons(::sn_myNetID);

        socket.Broadcast( reinterpret_cast<int8 *>(&(sendBuffer_[0])),
                          2*sendBuffer_.Len(), port);

        Clear();

        if ( control )
        {
            control->Use( nBandwidthControl::Usage_Execution, 2*sendBuffer_.Len() + OVERHEAD );
        }
    }
}

// clears the buffer
void nSendBuffer::Clear()
{
    for(int i=sendBuffer_.Len()-1;i>=0;i--)
        sendBuffer_(i)=0;

    sendBuffer_.SetLen( 0 );
}


nBandwidthControl::nBandwidthControl( nBandwidthControl* parent )
{
#ifdef DEBUG
    if ( parent )
        parent->numChildren_ ++;
    numChildren_ = 0;
#endif

    parent_ = parent;

    Reset();
}

nBandwidthControl::~nBandwidthControl()
{
#ifdef DEBUG
    if ( parent_ )
        parent_->numChildren_ --;

    tASSERT( numChildren_ == 0 );
#endif
}

void nBandwidthControl::Reset()
{
    rateControlPlanned_ = rateControl_ = 1000.0f;
    rate_ = 8;
}

void nBandwidthControl::Use( Usage planned, REAL bandwidth )
{
    tRecorderSync< REAL >::Archive( "_RATE_CONTROL_USAGE", 4, bandwidth );
    ( Usage_Planning == planned ? rateControlPlanned_ : rateControl_ ) -= bandwidth;
}

void nBandwidthControl::Update( REAL ts)
{
    tRecorderSync< REAL >::Archive( "_RATE_CONTROL", 12, rateControl_ );
    tRecorderSync< REAL >::Archive( "_RATE_CONTROL_PLANNED", 12, rateControlPlanned_ );

    rateControl_ += ( rate_ * 1000 ) * ts;

    if ( rateControl_ > 1000.0f )
    {
        rateControl_ = 1000.0f;
    }

    rateControlPlanned_ = rateControl_;
}

void nMessage::SendCollected(int peer)
{
    //if ( peer == 7 && sn_Connections[peer].sendBuffer_.Len() > 0 )
    //    con << tSysTimeFloat() << "\n";

    sn_OrderPriority = 0;

    if (peer<0 || peer > MAXCLIENTS+1 || !sn_Connections[peer].socket)
        tERR_ERROR("Invalid peer!");

    sn_Connections[peer].sendBuffer_.Send( *sn_Connections[peer].socket, peers[peer], &sn_Connections[peer].bandwidthControl_ );
}


void nMessage::BroadcastCollected(int peer, unsigned int port){
    if (peer<0 || peer > MAXCLIENTS+1 || !sn_Connections[peer].socket)
        tERR_ERROR("Invalid peer!");

    sn_Connections[peer].sendBuffer_.Broadcast( *sn_Connections[peer].socket, port, &sn_Connections[peer].bandwidthControl_ );
}


// TODO_NOACK
void nMessage::SendImmediately(int peer,bool ack){
    if (ack)
    {
#ifdef NO_ACK
        tASSERT(messageIDBig_);
#endif
        new nWaitForAck(this,peer);
    }

    // server: messages to yourself are a bit strange...
    if ( sn_GetNetState() == nSERVER && peer == 0 && ack )
    {
        st_Breakpoint();
        tJUST_CONTROLLED_PTR< nMessage > bounce(this);
        return;
    }

#ifdef DEBUG  
    /*
    if (descriptor>1)
      con << "SMT " << descriptor << "\n";
    */

    /*
      #ifdef DEBUG
      if (sn_Connections[].rate_control[peer]<-2000)
         tERR_ERROR("Heavy network overflow.");
      #endif
    */

    // if (peer==0 && sn_GetNetState()==nSERVER)
    //      tERR_ERROR("talking to yourself, eh?");

    if (peer==MAXCLIENTS+1){
#ifdef DEBUG
        if(descriptor==s_Acknowledge.id)
            con << "Sending ack to login slot.\n";
#endif
        //      else if (descriptor
        //	tERR_ERROR("the last user only should receive denials or acks.");
    }
#endif

    if (sn_Connections[peer].sendBuffer_.Len()+data.Len()+3 > MAX_MESS_LEN/2){
        SendCollected(peer);
        //con << "Overflow packets sent to " << peer << '\n';
    }


    if (sn_Connections[peer].socket)
    {
        sn_Connections[peer].sendBuffer_.AddMessage( *this, &sn_Connections[peer].bandwidthControl_ );

        /*
          if (sn_Connections[].rate_control[peer]>0)
          send_collected(peer);
            
          unsigned short *b=new (unsigned short)[data.Len()+3];
            
          b[0]=htons(descriptor);
          b[1]=htons(messageID);
          b[2]=htons(data.Len());
          int len=data.Len();
          for(int i=0;i<len;i++)
          b[3+i]=htons(data(i));
            
            
          ANET_Write(sn_Connections[].socket[peer],(int8 *)b,
          2*(data.Len()+3),&peers[peer]);
            
          //std::cerr << "Sent " << 2*len+6 << " bytes.\n";
          sn_Connections[].rate_control[peer]-=2*(len+3)+OVERHEAD;
            
          delete b;
        */

        if (deb_net)
            con << "Sent " <<descriptor << ':' << messageIDBig_ << ":"
            << peer << '\n';
    }

    tControlledPTR< nMessage > bounce( this ); // delete this message if nobody is interested in it any more
}

REAL sent_per_messid[MAXDESCRIPTORS+100];

void nMessage::Send(int peer,REAL priority,bool ack){
#ifdef NO_ACK
    if (!ack)
        messageIDBig_ = 0;
#endif

    // messages to yourself are a bit strange...
    if ( sn_GetNetState() == nSERVER && peer == 0 && ack )
    {
        st_Breakpoint();
        tJUST_CONTROLLED_PTR< nMessage > bounce(this);
        return;
    }

#ifdef DEBUG

    if (peer==MAXCLIENTS+1){
        if(descriptor==s_Acknowledge.id)
            con << "Sending ack to login slot.\n";
        //      else if (descriptor
        //	tERR_ERROR("the last user only should receive denials or acks.");
    }
#endif

#ifdef DEBUG_X
    if (descriptor>1)
        con << "PMT " << descriptor << "\n";
#endif

    // the next line was redundant; the send buffer handles that part of accounting.
    //sn_Connections[peer].bandwidthControl_.Use( nBandwidthControl::Usage_Planning, 2*(data.Len()+3) );

    sent_per_messid[descriptor]+=2*(data.Len()+3);

    tASSERT(Descriptor()!=s_Acknowledge.ID() || !ack);

    new nMessage_planned_send(this,priority+sn_OrderPriority,ack,peer);
    sn_OrderPriority += .01; // to roughly keep the relative order of netmessages
}

// ack messages: don't get an ID
class nAckMessage: public nMessage
{
public:
    nAckMessage(): nMessage( s_Acknowledge ){ messageIDBig_ = 0; }
};

// receive and s_Acknowledge the recently reveived network messages

typedef std::deque< tJUST_CONTROLLED_PTR< nMessage > > nMessageFifo;

static void rec_peer(unsigned int peer){
    tASSERT( sn_Connections[peer].socket );

    nMachine::Expire();

    // temporary fifo for received messages
    //static tArray< tJUST_CONTROLLED_PTR< nMessage > > receivedMessages;
    static nMessageFifo receivedMessages;

    // the growing buffer we read messages into
    const int serverMaxAcceptedSize=2000;
    static tArray< unsigned short > storage(2000);
    int maxrec = storage.Len();
    unsigned short * buff = &storage[0];

    // short buff[maxrec];
    if (sn_Connections[peer].socket){
        int count=0;
        int len=1;
        while (len>=0 && sn_Connections[peer].socket)
        {
            nAddress addrFrom; // the sender of the current packet
            len = sn_Connections[peer].socket->Read( reinterpret_cast<int8 *>(buff),maxrec*2, addrFrom);

            if (len>0){
                if ( len >= maxrec*2 )
                {
                    // the message was too long to receive. What to do?
                    if ( sn_GetNetState() != nSERVER || len < serverMaxAcceptedSize )
                    {
                        // expand the buffer. The message is lost now, but the
                        // peer will certainly resend it. Hopefully, the buffer will be large enough to hold it then.
                        storage[maxrec*2-1]=0;
                        maxrec = storage.Len();
                        buff = &storage[0];

                        tERR_WARN( "Oversized network packet received. Read buffer has been enlargened to catch it the next time.");

                        // no use in processing the truncated packet. Some messages may get lost,
                        // but that's better than the inevitable network error and connection
                        // termination that expects us if we go on.
                        continue;
                    }
                    else
                    {
                        // terminate the connection
                        sn_DisconnectUser( peer, "$network_kill_error" );
                    }
                }

                unsigned short *b=buff;
                unsigned short *bend=buff+(len/2-1);

                sn_ReceivedPackets++;
                sn_ReceivedBytes  += len + OVERHEAD;

                unsigned short claim_id=ntohs(*bend);

                // z-man: security check ( thanks, Luigi Auriemma! )
                if ( claim_id > MAXCLIENTS+1 )
                    continue;	// drop packet, maybe it was just truncated.

                /*
                  std::cerr << "Received " << len << " bytes";
                  con << " from user " << claim_id << '\n';
                */
                count ++;

                unsigned int id=peer;
                //	 for(unsigned int i=1;i<=(unsigned int)maxclients;i++)
                int comp=nAddress::Compare( addrFrom, peers[claim_id] );
                if ( comp == 0 ) // || claim_id == MAXCLIENTS+1 )
                {
                    // everything seems allright. accept the id.
                    id = claim_id;
                }
                else
                {
                    // assume it's a new connection
                    id = MAXCLIENTS+1;
                    peers[ MAXCLIENTS+1 ] = addrFrom;
                    sn_Connections[ MAXCLIENTS+1 ].socket = sn_Connections[peer].socket;
                }

                //	 if (peer!=id)
                //  con << "Changed incoming address.\n";
                int lenleft = bend - b;

#ifndef NOEXCEPT
                try
                {
#endif
                    while( lenleft > 0 ){
                        tJUST_CONTROLLED_PTR< nMessage > pmess;
                        pmess = tNEW( nMessage )(b,id,lenleft);
                        nMessage& mess = *pmess;

                        lenleft = bend - b;

                        bool mess_is_new=true;
                        // see if we have got this packet before
                        unsigned short mess_id=mess.MessageID();

#ifdef DEBUG
                        if ( (simulate_loss && rand()%simulate_loss==0)){
                            // simulate packet loss
                            con << "Losing packet " << mess_id << ":" << id << ".\n";
                        }else
#endif
                            if(// (id==MAXCLIENTS+1 && !nCallbackAcceptPackedWithoutConnection::Accept( mess ) ) ||
                                // do not accept normal packages from the login
                                // slot; just login and information packets are allowed.
                                ( sn_GetNetState() != nSERVER && !login_succeeded && !nCallbackAcceptPackedWithoutConnection::Accept( mess ) )
                                // if we are not yet logged in, accept only login and login_deny.
                            )
                            {
                                //							con << "Ignoring packet " << mess_id << ":" << id << ".\n";
                            }
                            else
                            {
                                if (id <= MAXCLIENTS && mess_id != 0)  // messages with ID 0 are non-ack messages and come really often. they are always new.
                                {
                                    unsigned short diff=mess_id-highest_ack[id];
                                    if (diff>0 && diff<10000 ||
                                            ((
                                                 mess.Descriptor() == login_accept.ID() ||
                                                 mess.Descriptor() == login_deny.ID()   ||
                                                 mess.Descriptor() == login.ID()
                                             ) && highest_ack[id] == 0)
                                       ){
                                        // the message has a more recent id than anything before.
                                        // it is surely new.
                                        highest_ack[id]=mess_id;
                                    }
                                    else{
                                        // do a better check
                                        for(int i=ACKBACK-1;i>=0;i--)
                                            if (mess_id==lastacks[id][i])
                                                mess_is_new=false;
                                    }
                                }


                                // acknowledge the message, even if it was old (the sender
                                // then thinks it got lost the first time)

                                // special situation: logout. Do not sent ack any more.
                                if ((!sn_Connections[id].socket))
                                {
                                    sn_Connections[id].ackMess=NULL;
                                }
                                else if (
#ifdef NO_ACK
                                    (mess.MessageID()) &&
#endif
                                    (mess.Descriptor()!=login_ignore.ID() ||
                                     login_succeeded )){
                                    // do not ack the login_ignore packet that did not let you in.

#ifdef DEBUG
                                    if ( id > MAXCLIENTS )
                                    {
                                        con << "Sending ack to login slot.\n";
                                    }
#endif

                                    if(sn_Connections[id].ackMess==NULL)
                                    {
                                        sn_Connections[id].ackMess=new nAckMessage();
                                    }

                                    sn_Connections[id].ackMess->Write(mess.MessageID());
                                    if (sn_Connections[id].ackMess->DataLen()>100){
                                        sn_Connections[id].ackMess->Send(id, 0, false);
                                        sn_Connections[id].ackMess=NULL;
                                    }
                                }

                                if (mess_is_new){
                                    // mark the message as old
                                    if (mess_id > 0)
                                    {
                                        lastacks[id][lastackPos[id]]=mess_id;
                                        if(++lastackPos[id]>=ACKBACK) lastackPos[id]=0;
                                    }

                                    /*
                                    								// special situation: login. Change peer number permanently
                                    								if (peer==MAXCLIENTS+1 && new_id>0){
                                    									id=peer=new_id;
                                    								}
                                    */

                                    if (sn_GetNetState() != nSTANDALONE)
                                    {
                                        // store the message for later processing
                                        receivedMessages.push_back( pmess );
                                    }
                                }
                                //else
                                //con << "Message " << mess_id << ":" << id << " was not new.\n";
                            }
                    }
#ifndef NOEXCEPT
                }

                catch(nKillHim)
                {
                    con << "nKillHim signal caught.\n";
                    sn_DisconnectUser(peer, "$network_kill_error");
                }
#endif
            }
	#ifndef NOEXCEPT
            try
            {
	#endif
                static int recursionCount = 0;
                ++recursionCount;

                // handle messages
                while ( receivedMessages.begin() != receivedMessages.end() )
                {
                    tJUST_CONTROLLED_PTR< nMessage > mess = receivedMessages.front();
                    receivedMessages.pop_front();

                    // perhaps the connection died?
                    if ( sn_Connections[ mess->SenderID() ].socket )
                        nDescriptor::HandleMessage( *mess );
                }

                if ( --recursionCount <= 0 )
                {
                    nCallbackReceivedComplete::ReceivedComplete();
                }

	#ifndef NOEXCEPT
            }

            catch(nKillHim)
            {
                con << "nKillHim signal caught.\n";
                sn_DisconnectUser(peer, "$network_kill_error");
            }
	#endif

        }
    }
}

void sn_ReceiveFromControlSocket()
{
    rec_peer(0);
}


nNetState sn_GetNetState(){
    return current_state;
}

void clear_owners();

// tries to open listening sockets according to specification, but falls back to increasing ports
static bool sn_Listen( unsigned int & net_hostport, const tString& net_hostip )
{
    unsigned int net_hostport_before = net_hostport;

    try
    {
        nSocketListener & listener = sn_BasicNetworkSystem.AccessListener();

        listener.SetIpList( net_hostip );

        bool reported = false;

        // try ports in a range
        while ( net_hostport < sn_serverPort + 100 )
        {
            if ( listener.SetPort( net_hostport ).Listen( true ) )
                return true;

            if ( !reported )
            {
                con << "sn_SetNetState: Unable to open accept socket on desired port " << net_hostport << ", Trying next ports...\n";
                reported = true;
            }

            net_hostport++;
        }

        con << "sn_SetNetState: Giving up setting up listening sockets for IP list " << net_hostip << ".\n";
    }
    catch( const tException & e )
    {
        con << "sn_SetNetState: can't setup listening sockets. Reason given:\n"
        << e.GetName() << ": " << e.GetDescription() << "\n";
    }

    // reset host port
    net_hostport = net_hostport_before;

    return false;
}

// save and load machine info
static void sn_SaveMachines();
static void sn_LoadMachines();

static void sn_DisconnectAll()
{
    for(int i=MAXCLIENTS+1;i>=0;i--)
    {
        if( sn_Connections[i].socket )
        {
            sn_DisconnectUser(i, "$network_kill_shutdown");
            tVERIFY( !sn_Connections[i].socket );
        }
    }
    nCallbackLoginLogout::UserLoggedOut(0);
}

// flag set as long as the network sockets should not be closed and reopened
static bool sn_noReset = false;
nSocketResetInhibitor::nSocketResetInhibitor()
{
    sn_noReset = true;
}
nSocketResetInhibitor::~nSocketResetInhibitor()
{
    sn_noReset = false;
}

void sn_SetNetState(nNetState x){
    static bool reentry=false;
    if(!reentry && x!=current_state){
        sn_UpdateCurrentVersion();

        //if (x == nSERVER)
        unsigned int net_hostport = sn_serverPort;

        // save/load machines on entering/leaving server mode
        if ( x == nSERVER )
            sn_LoadMachines();
        else if ( current_state == nSERVER )
            if ( !tRecorder::IsPlayingBack() )
                sn_SaveMachines();

        reentry=true;
        if (x!=nSTANDALONE)
        {
            if (x==nCLIENT)
            {
                sn_DisconnectAll();
            }
            else
            {
                sn_myNetID=0;
            }

            if (!sn_Connections[0].socket)
                sn_Connections[0].socket=sn_BasicNetworkSystem.Init();
            // bool success = true;
            if (x == nSERVER)
            {
                // bool success =
                sn_Listen( net_hostport, net_hostip ) ||    // first try: do it according to user specs
                sn_Listen( net_hostport, tString( "ANY" ) ) ||         // second try: bind to generic IP
                sn_Listen( net_hostport, tString( "ALL" ) );           // last try: bind to all available IPs

#ifdef DEDICATED                
                // save host port that worked, otherwise it may change from the port sent to the master server
                sn_serverPort = net_hostport;
#endif
            }
        }
        else
        {
            clear_owners();
            for(int i=MAXCLIENTS+1;i>=0;i--){
                if(sn_Connections[i].socket){
                    if (i==0 && current_state!=nSERVER)
                    { // logout: fire and forget
                        con << tOutput("$network_logout_process");
                        for(int j=3;j>=0;j--){ // just to make sure
                            nMessage *lo=new nMessage(logout);
                            lo->Write(static_cast<unsigned short>(sn_myNetID));
                            lo->ClearMessageID();
                            lo->SendImmediately(0, false);
                            nMessage::SendCollected(0);
                            tDelay(1000);
                        }
                        con << tOutput("$network_logout_done");

                        sn_myNetID=0; // MAXCLIENTS+1; // reset network id
                    }
                }
                sn_DisconnectUserNoWarn(i, "$network_kill_shutdown");
            }

            sn_Connections[0].socket = 0;

            // shutdown network system to get new socket
            if ( !sn_noReset )
                sn_BasicNetworkSystem.Shutdown();
        }

        current_state=x;
        reentry=false;
    }

    sn_UpdateCurrentVersion();
}



// go to client mode and connect to server


void sn_Bend( nAddress const & address )
{
    if ((sn_GetNetState() == nSTANDALONE))
        sn_SetNetState(nCLIENT);

    peers[0] = address;
}

void sn_Bend( tString const & server, unsigned int port)
{
    // fill address info
    nAddress address;
    address.SetHostname( server );
    address.SetPort( port );

    // delegate
    sn_Bend( address );
}

nConnectError sn_Connect( nAddress const & server, nLoginType loginType, nSocket const * socket ){
    sn_DenyReason = "";

    // pings in the beginning of the login are not really representative
    nPingAverager::SetWeight(.0001);

    // net_hostport = sn_clientPort;

    sn_SetNetState(nSTANDALONE);
    sn_SetNetState(nCLIENT);

    // set user requested socket
    if ( socket )
        sn_Connections[0].socket = socket;

    sn_Connections[0].ping.Reset();

    peers[0] = server;

    reset_last_acks(0);
    nCallbackLoginLogout::UserLoggedOut(0);
    sn_Connections[0].sendBuffer_.Clear();

    tASSERT( sn_Connections[0].socket );

    // sn_Connections[0].socket->Connect( peers[0] ); // useless
    sn_Connections[0].bandwidthControl_.SetRate( sn_maxRateOut );

    sn_myNetID=0; // MAXCLIENTS+1; // reset network id

    // first, get all pending messages
    sn_Receive();
    sn_Receive();
    sn_Receive();

    // reset version control until the true value is given by the server.
    sn_currentVersion = nVersion(0,0);

    // Login stuff.....
    tJUST_CONTROLLED_PTR< nMessage > mess;
    if ( loginType != Login_Pre0252 )
    {
        mess=new nMessage(login_2);
        mess->Write(sn_maxRateIn);

        unsigned short bb = big_brother;
        mess->Write( bb );
        if ( bb ){
            (*mess) << sn_bigBrotherString;
            big_brother=false;
        }

        (*mess) << sn_MyVersion();
    }
    else
    {
        mess=new nMessage(login);
        mess->Write(sn_maxRateIn);

        // send (worthless) big brother string
        if (big_brother)
        {
            (*mess) << sn_bigBrotherString;
        }
        else
        {
            (*mess) << tString("");
        }

        (*mess) << sn_MyVersion();

        big_brother=false;
    }

    mess->ClearMessageID();
    mess->SendImmediately(0,false);
    nMessage::SendCollected(0);

    con << tOutput("$network_login_process");

    login_failed=false;
    login_succeeded=false;

    nTimeRolling timeout=tSysTimeFloat()+5;

    static REAL resend = .25;
    nTimeAbsolute nextSend = tSysTimeFloat() + resend/5;
    while(sn_GetNetState()==nCLIENT && tSysTimeFloat()<timeout &&
            !login_failed && !login_succeeded){
        if ( tSysTimeFloat() > nextSend )
        {
            // con << "retrying...\n";
            nextSend = tSysTimeFloat() + resend;
            mess->SendImmediately(0,false);
            nMessage::SendCollected(0);
        }

        tAdvanceFrame(10000);
        sn_Receive();
    }
    if (login_failed)
    {
        con << tOutput("$network_login_failed");
        sn_SetNetState(nSTANDALONE);
        return nDENIED;
    }
    else if (tSysTimeFloat()>=timeout || sn_GetNetState()!=nCLIENT){
        if ( loginType == Login_All )
        {
            return 	sn_Connect( server, Login_Pre0252, socket );
        }
        else
        {
            con << tOutput("$network_login_failed_timeout");
            sn_SetNetState(nSTANDALONE);
            return nTIMEOUT;
        }
    }
    else{
        nCallbackLoginLogout::UserLoggedIn(0);

        tOutput mess;
        mess.SetTemplateParameter(1, sn_myNetID);
        mess << "$network_login_success";
        con << mess;
        con << tOutput("$network_login_sync");
        sn_Sync(40);
        con << tOutput("$network_login_relabeling");
        con << tOutput("$network_login_sync2");
        sn_Sync(40,true);
        con << tOutput("$network_login_done");

        // marginalize past ping values
        nPingAverager::SetWeight(1);

        return nOK;
    }
}


void nReadError()
{
    // st_Breakpoint();
#ifndef NOEXCEPT
    throw nKillHim();
#else
    con << "\nI told you not to use PGCC! Now we need to leave the\n"
    << "system in an undefined state. The progam will crash now.\n"
    << "\n\n";
#endif
}

#ifdef DEDICATED
static short sn_decorateID = true;
static tConfItem< short > sn_decorateIDConf( "CONSOLE_DECORATE_ID", sn_decorateID );

static short sn_decorateIP = false;
static tConfItem< short > sn_decorateIPConf( "CONSOLE_DECORATE_IP", sn_decorateIP );

// console with filter for better machine readable log format
class nConsoleFilter:public tConsoleFilter{
private:
    virtual void DoFilterLine( tString &line )
    {
        if ( sn_decorateID )
        {
            tString orig = line;

            int id = nCurrentSenderID::GetID();
            bool printIP = ( id > 0 && sn_decorateIP );

            line = "";
            line << "[";
            if ( sn_decorateID )
                line << id;
            if ( sn_decorateID && printIP )
                line << " ";
            if ( printIP )
            {
                // get IP from id
                tString IP;
                sn_GetAdr( id,  IP );
                line << "IP=" << IP;
            }

            line << "] " << orig;
        }
    }
};

static nConsoleFilter sn_consoleFilter;
#endif

static void sn_ConsoleOut_handler(nMessage &m){
    if (sn_GetNetState()!=nSERVER){
        tString s;
        m >> s;
        con << s;
    }
}


static nDescriptor sn_ConsoleOut_nd(8,sn_ConsoleOut_handler,"sn_ConsoleOut");

// causes the connected clients to print a message
nMessage* sn_ConsoleOutMessage( const tOutput& o )
{
    tString message(o);
    message  << "0xffffff";

    // truncate message to 1.4K, a safe size for all UDP packets
    static const int maxLen = 1400;
    if ( message.Len() > maxLen )
    {
        tERR_WARN( "Long console message truncated.");

        message.SetLen( maxLen+1 );
        message[maxLen]='\0';
    }

    nMessage* m=new nMessage(sn_ConsoleOut_nd);
    *m << message;

    return m;
}

void sn_ConsoleOut(const tOutput& o,int client){
    //	tString message(o);

    tJUST_CONTROLLED_PTR< nMessage > m = sn_ConsoleOutMessage( o );

    if (client<0){
        m->BroadCast();
        con << o;
    }
    else if (client==sn_myNetID)
    {
        con << o;
    }
    else
        m->Send(client);
}

static void client_cen_handler(nMessage &m){
    if (sn_GetNetState()!=nSERVER){
        tString s;
        m >> s;
        con.CenterDisplay(s);
    }
}

static nDescriptor client_cen_nd(9,client_cen_handler,"client_cen");

// causes the connected clients to print a message in the center of the screeen
void sn_CenterMessage(const tOutput &o,int client){
    tString message(o);

    nMessage *m=new nMessage(client_cen_nd);
    *m << message;
    if (client<0){
        m->BroadCast();
        con.CenterDisplay(message);
    }
    else if (client==sn_myNetID)
        con.CenterDisplay(message);
    else
        m->Send(client);
}

static void ConsoleOut_conf(std::istream &s)
{
    // read the message
    tString message;
    message.ReadLine( s, true );

    message += "\n";

    // display it
    sn_ConsoleOut( message );
}

static tConfItemFunc ConsoleOut_c("CONSOLE_MESSAGE",&ConsoleOut_conf);

static void CeterMessage_conf(std::istream &s)
{
    // read the message
    tString message;
    message.ReadLine( s, true );

    // display it
    sn_CenterMessage( message );
}

static tConfItemFunc CenterMessage_c("CENTER_MESSAGE",&CeterMessage_conf);



// ****************************************************************
//                    Send Queue
// ****************************************************************

// the network stuff planned to send:
tHeap<planned_send> send_queue[MAXCLIENTS+2];

planned_send::planned_send(REAL priority,int Peer){
    peer=Peer;

    SetVal( priority, send_queue[peer] );
}

planned_send::~planned_send(){
    RemoveFromHeap();
}

tHeapBase *planned_send::Heap() const
{
    return &send_queue[peer];
}

// change our priority:
void planned_send::add_to_priority(REAL diff)
{
    SetVal( Val() + diff, send_queue[peer] );
}

// **********************************************

nMessage_planned_send::nMessage_planned_send
(nMessage *M,REAL priority,bool Ack,int Peer)
        :planned_send(priority,Peer),m(M),ack(Ack){
    //if (m)
}

nMessage_planned_send::~nMessage_planned_send(){
}

void nMessage_planned_send::execute(){
    if ( Val() < -killTimeout-10){
        tOutput mess;
        mess.SetTemplateParameter(1, peer);
        mess << "$network_error_overflow";
        con << mess;
        st_Breakpoint();
        sn_DisconnectUser(peer, "$network_kill_overflow");
    }
    else if (m)
        m->SendImmediately(peer,ack);
}


// **********************************************

static REAL sn_SendPlanned1(){
    sn_OrderPriority = 0;

    // if possible, send waiting messages
    static double lastTime=-1;
    nTimeAbsolute time=tSysTimeFloat();
    if (lastTime<0)
        lastTime=time;

    if (time<lastTime-.01 || time>lastTime+1000)
#ifdef DEBUG
    {
        tERR_ERROR("Timer hickup!");
    }
#else
    {
        tERR_WARN("Timer hickup!");
        lastTime=time;
    }
#endif
    REAL dt = time - lastTime;

    //for(int i=MAXCLIENTS+1;i>=0;i--){
    for(int i=0;i<=MAXCLIENTS+1;i++){
        nConnectionInfo & connection = sn_Connections[i];
        if ( !connection.socket )
            continue;

        while (connection.ackPending<sn_maxNoAck &&
                connection.bandwidthControl_.CanSend()     &&
                send_queue[i].Len())
        {
            send_queue[i](0)->execute();
            if (send_queue[i].Len())
                delete send_queue[i](0);
        }

        // make everything a little more urgent:
        for(int j=send_queue[i].Len()-1;j>=0;j--)
            send_queue[i](j)->add_to_priority(-dt);
    }
    lastTime=time;

    return dt;
}

static void sn_SendPlanned2( REAL dt ){
    // empty the send buffers
    for(int i=0;i<=MAXCLIENTS+1;i++){
        nConnectionInfo & connection = sn_Connections[i];
        if (connection.sendBuffer_.Len()>0 && connection.bandwidthControl_.CanSend() )
            nMessage::SendCollected(i);

        // update bandwidth usage and other time related data
        connection.Timestep( dt );
    }
}

void sn_SendPlanned()
{
    REAL dt = sn_SendPlanned1();
    sn_SendPlanned2( dt );
}

void sn_Receive(){
    /*
      static bool reentry=false;
      if (reentry)
      return;
      reentry=true;
    */

    netTime=tSysTimeFloat();
    //	new_id=0;
    sn_Connections[MAXCLIENTS+1].ping.Reset();

    // create the ack messages
    int i;
    for(i=0;i<=MAXCLIENTS+1;i++)
        if(sn_Connections[i].ackMess==NULL)
            sn_Connections[i].ackMess=new nAckMessage();


    switch (current_state){
    case nSERVER:
        {
            memset( &peers[0], 0, sizeof(sockaddr) );

            // listen on all sockets
            nSocketListener const & listener = sn_BasicNetworkSystem.GetListener();
            for ( nSocketListener::iterator i = listener.begin(); i != listener.end(); ++i )
            {
                // clear peer info used for receiving
                memset( &peers[MAXCLIENTS+1], 0, sizeof(sockaddr) );

                if((sn_Connections[MAXCLIENTS+1].socket = (*i).CheckNewConnection() ) != NULL)
                    rec_peer(MAXCLIENTS+1);
            }
        }
        // z-man: after much thought, the server does also need to listen to the
        // network control socket. .... Thinking again, it's only important for the master
        // servers, and they call rec_peer(0) separately.
        break;

    case nCLIENT:
        rec_peer(0);
        break;

    case nSTANDALONE:
    default:
        break;
    }

    // scedule regular messages
    REAL dt = sn_SendPlanned1();

    // schedule the acks: send them if it's possible (bandwith limit) or if there already is a packet in the pipe.
    for(i=0;i<=MAXCLIENTS+1;i++)
        if(sn_Connections[i].socket && sn_Connections[i].ackMess && !sn_Connections[i].ackMess->End()
                //	&& sn_ackAckPending[i] <= 1+sn_Connections[].ackMess[i]->DataLen()
                && ( sn_Connections[i].bandwidthControl_.CanSend() || sn_Connections[i].sendBuffer_.Len() > 0 )
          ){
            sn_Connections[i].ackMess->SendImmediately(i, false);
            sn_Connections[i].ackMess=NULL;
        }

    // schedule lost messages for resending
    nWaitForAck::Resend();

    // actually resend messages
    sn_SendPlanned2( dt );
}

void sn_KickUser(int i, const tOutput& reason, REAL severity )
{
    // log it
    nMachine::GetMachine(i).OnKick( severity );

    // do it
    sn_DisconnectUser( i, reason );
}

void sn_DisconnectUser(int i, const tOutput& reason )
{
    // don't be daft and kill yourself, server!
    if ( i == 0 && sn_GetNetState() == nSERVER )
    {
        tERR_WARN( "Server tried to disconnect from itself." );
        return;
    }

    // clients can only disconnect from the server
    if ( i != 0 && sn_GetNetState() == nCLIENT )
    {
        tERR_ERROR( "Client tried to disconnect from another client: impossible and a bad idea." );
        return;
    }

    sn_DisconnectUserNoWarn( i, reason );
}

void sn_DisconnectUserNoWarn(int i, const tOutput& reason )
{
    nCurrentSenderID senderID( i );

    nWaitForAck::AckAllPeer(i);

    static bool reentry=false;
    if (reentry)
        return;
    reentry=true;

    bool printMessage = false; // is it worth printing a message for this event?

    if (sn_Connections[i].socket)
    {
        nMessage::SendCollected(i);
        printMessage = true;

        // to make sure...
        if ( i!=0 && i != MAXCLIENTS+2 && sn_GetNetState() == nSERVER ){
            for(int j=2;j>=0;j--){
                nMessage* mess = (new nMessage(login_deny));
                *mess << tString( reason );
                mess->SendImmediately(i, false);
                nMessage::SendCollected(i);
            }
        }
    }

    nWaitForAck::AckAllPeer(i);

    sn_Connections[i].ackMess=NULL;

    if (i==0 && sn_GetNetState()==nCLIENT)
        sn_SetNetState(nSTANDALONE);

    reset_last_acks(i);

    // peers[i].sa_family=0;

    sn_Connections[i].ackPending=0;
    //  sn_ackAckPending[i]=0;

    nCallbackLoginLogout::UserLoggedOut(i);

    if ( printMessage )
    {
        con << tOutput( "$network_killuser", i, sn_Connections[i].ping.GetPing() );
    }

    // clear address, socket and send queue
    sn_Connections[i].sendBuffer_.Clear();
    sn_Connections[i].socket=NULL;
    peers[i] = nAddress();
    sn_Connections[i].Clear();
    while (send_queue[i].Len())
        delete (send_queue[i](0));

    reentry=false;

    sn_UpdateCurrentVersion();
}


int sn_QueueLen(int user){
    return send_queue[user].Len();
}


static tCallback* s_loginoutAnchor=NULL;
int  nCallbackLoginLogout::user;
bool nCallbackLoginLogout::login;

nCallbackLoginLogout::nCallbackLoginLogout(VOIDFUNC *f)
        :tCallback(s_loginoutAnchor,f){}

void nCallbackLoginLogout::UserLoggedIn(int u){
    login = true;
    user = u;
    Exec(s_loginoutAnchor);
}

void nCallbackLoginLogout::UserLoggedOut(int u){
    login = false;
    user = u;
    Exec(s_loginoutAnchor);
}

unsigned short nCallbackAcceptPackedWithoutConnection::descriptor=0;	// the descriptor of the incoming packet
static tCallbackOr* s_AcceptAnchor=NULL;

nCallbackAcceptPackedWithoutConnection::nCallbackAcceptPackedWithoutConnection(BOOLRETFUNC *f)
        : tCallbackOr( s_AcceptAnchor, f )
{
}

bool nCallbackAcceptPackedWithoutConnection::Accept( const nMessage& m )
{
    descriptor=m.Descriptor();
    return Exec( s_AcceptAnchor );
}

static tCallback* s_ReceivedCompleteAnchor=NULL;

nCallbackReceivedComplete::nCallbackReceivedComplete(VOIDFUNC *f)
        : tCallback( s_ReceivedCompleteAnchor, f )
{
}

void nCallbackReceivedComplete::ReceivedComplete( )
{
    Exec( s_ReceivedCompleteAnchor );
}

static bool net_Accept()
{
    return
        nCallbackAcceptPackedWithoutConnection::Descriptor()==login_accept.ID() ||
        nCallbackAcceptPackedWithoutConnection::Descriptor()==login_deny.ID();
}

static nCallbackAcceptPackedWithoutConnection net_acc( &net_Accept );

static void net_exit(){
    for (int i=MAXCLIENTS+1;i>=0;i--)
    {
        sn_Connections[i].ackMess = NULL;
        while (send_queue[i].Len())
            delete send_queue[i].Remove(0);
    }
}

static tInitExit net_ie(NULL, &net_exit);



void sn_Statistics()
{
    nTimeRolling time = tSysTimeFloat();
    REAL dt = time - sn_StatsTime;
    sn_StatsTime = time;

    if (dt > 0 && (sn_SentPackets || sn_SentBytes))
    {
        tOutput o;
        o.SetTemplateParameter(1,dt);
        o.SetTemplateParameter(2,sn_SentBytes);
        o.SetTemplateParameter(3,sn_SentPackets);
        o.SetTemplateParameter(4,sn_SentBytes/dt);
        o.SetTemplateParameter(5,sn_ReceivedBytes);
        o.SetTemplateParameter(6,sn_ReceivedPackets);
        o.SetTemplateParameter(7,sn_ReceivedBytes/dt);
        o << "$network_statistics1";
        o << "$network_statistics2";
        o << "$network_statistics3";

        con << o;
    }

    sn_SentPackets = 0;
    sn_SentBytes   = 0;
    sn_ReceivedPackets = 0;
    sn_ReceivedBytes   = 0;
}






nConnectionInfo::nConnectionInfo(){Clear();}
nConnectionInfo::~nConnectionInfo(){}

void nConnectionInfo::Clear(){
    socket     = NULL;
    ackPending = 0;
    ping.Reset();
    crypt      = NULL;

    sendBuffer_.Clear();

    bandwidthControl_.Reset();

    ackMess = NULL;

    userName.SetLen(0);

    // start with 10% packet loss with low statistical weight
    packetLoss_.Reset();
    packetLoss_.Add(.1,10);
}

void nConnectionInfo::Timestep( REAL dt )  //!< call whenever an an reliable message got sent
{
    // update ping
    ping.Timestep( dt );

    // update bandwidth control
    bandwidthControl_.Update( dt );

    // update packet loss; average about a minute
    packetLoss_.Timestep( .02 * dt );
}

void nConnectionInfo::ReliableMessageSent()  //!< call whenever an an reliable message got sent
{
    packetLoss_.Add( 1 );
}

void nConnectionInfo::AckReceived()          //!< call whenever an ackownledgement message arrives
{
    packetLoss_.Add( -1 );
}

REAL nConnectionInfo::PacketLoss() const     //!< returns the average packet loss ratio
{
    REAL ret = packetLoss_.GetAverage();
    return ret > 0 ? ret : 0;
}

void sn_GetAdr(int user,  tString& name)
{
    peers[user].ToString( name );
}

unsigned int sn_GetPort(int user)
{
    return peers[user].GetPort();
}

unsigned int sn_GetServerPort()
{
    return sn_serverPort;
}

int sn_NumUsers( bool all )
{
    int ret = 0;
    for (int i=MAXCLIENTS; i>0; i--)
        if (sn_Connections[i].socket && ( all || ( sn_allowSameIPCountSoft > CountSameIP( i ) ) ) )
            ret++;

#ifndef DEDICATED
    ret++;
#endif

    return ret;
}

int sn_NumUsers()
{
    return sn_NumUsers( true );
}

int sn_NumRealUsers()
{
    return sn_NumUsers( false );
}

int sn_MaxUsers()
{
    return sn_maxClients;
}

int sn_MessagesPending(int user)
{
    return sn_Connections[user].ackPending + send_queue[user].Len();
}

nBasicNetworkSystem sn_BasicNetworkSystem;

// *******************************************************************************************
// *
// *	nKillHim
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nKillHim::nKillHim( void )
{
}

// *******************************************************************************************
// *
// *	~nKillHim
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nKillHim::~nKillHim( void )
{
}

// *******************************************************************************************
// *
// *	DoGetName
// *
// *******************************************************************************************
//!
//!		@return		short name
//!
// *******************************************************************************************

tString nKillHim::DoGetName( void ) const
{
    return tString( "Connektion kill request" );
}

// *******************************************************************************************
// *
// *	DoGetDescription
// *
// *******************************************************************************************
//!
//!		@return		description
//!
// *******************************************************************************************

tString nKillHim::DoGetDescription( void ) const
{
    return tString( "The currently handled peer must have done something illegal, so it should be terminated." );
}

// *******************************************************************************************
// *
// *	nAverager
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nAverager::nAverager( void )
        : weight_(0), sum_(0), sumSquared_(0), weightSquared_(0)
{
}

// *******************************************************************************************
// *
// *	~nAverager
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nAverager::~nAverager( void )
{
}

// *******************************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************************
//!
//!		@param	decay	decay factor 0 .. infinity; larger values lead to more decay.
//!
// *******************************************************************************************

void nAverager::Timestep( REAL decay )
{
    REAL factor = 1/(1+decay);

    // pretend all data so far was collected with a weight of the original weight multiplied by factor
    weight_        *= factor;
    sum_           *= factor;
    sumSquared_    *= factor;
    weightSquared_ *= factor * factor;
}

// *******************************************************************************************
// *
// *	Add
// *
// *******************************************************************************************
//!
//!		@param	value	 the value to add
//!		@param	weight   its statistical weight (importance compared to other values)
//!
// *******************************************************************************************

void nAverager::Add( REAL value, REAL weight )
{
    tASSERT( weight >= 0 );
    weight_        += weight;
    sum_           += weight * value;
    sumSquared_    += weight * value * value;
    weightSquared_ += weight * weight;
}

// *******************************************************************************************
// *
// *	Reset
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nAverager::Reset( void )
{
    weightSquared_ = weight_ = sum_ = sumSquared_ = 0.0f;
}

// *******************************************************************************************
// *
// *	GetAverage
// *
// *******************************************************************************************
//!
//!		@return		the average value over the last time
//!
// *******************************************************************************************

REAL nAverager::GetAverage( void ) const
{
    if ( weight_ > 0 )
        return sum_ / weight_;
    else
        return 0;
}

// *******************************************************************************************
// *
// *	GetDataVariance
// *
// *******************************************************************************************
//!
//!		@return		the average recent variance in the incoming data
//!
// *******************************************************************************************

REAL nAverager::GetDataVariance( void ) const
{
    if ( weight_ > 0 )
    {
        REAL average       = sum_ / weight_;
        REAL averageSquare = sumSquared_ / weight_;
        REAL ret = averageSquare - average * average;
        if ( ret < 0 )
            ret = 0;
        return ret;
    }
    else
        return 0;
}

// *******************************************************************************************
// *
// *	GetAverageVariance
// *
// *******************************************************************************************
//!
//!		@return		the expected variance of the return value of GetAverage()
//!
// *******************************************************************************************

REAL nAverager::GetAverageVariance( void ) const
{
    if ( weight_ > 0 )
    {
        REAL square = weight_ * weight_;

        REAL denominator = square - weightSquared_;
        REAL numerator = GetDataVariance() * weightSquared_;
        if ( denominator > numerator * 1E-30 )
        {
            return numerator/denominator;
        }
        else
            return 1E+30;
    }
    else
        return 0;
}

// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	stream	stream to read from
//!		@return		    stream for chaining
//!
// *******************************************************************************

std::istream & nAverager::operator <<( std::istream & stream )
{
    char c;
    stream >> c;
    tASSERT( c == '(' );

    stream >> weight_ >> sum_ >> sumSquared_ >> weightSquared_;

    stream >> c;
    tASSERT( c == ')' );

    return stream;
}

// *******************************************************************************
// *
// *	operator >>
// *
// *******************************************************************************
//!
//!		@param	stream	stream to write to
//!		@return		    stream for chaining
//!
// *******************************************************************************

std::ostream & nAverager::operator >>( std::ostream & stream ) const
{
    stream << '(' << weight_ << ' ' << sum_  << ' ' << sumSquared_  << ' ' << weightSquared_  << ')';

    return stream;
}

// *******************************************************************************
// *
// *	operator >>
// *
// *******************************************************************************
//!
//!		@param	stream	stream to read to
//!		@param  averager averager to read
//!		@return		    stream for chaining
//!
// *******************************************************************************

std::istream & operator >> ( std::istream & stream, nAverager & averager )
{
    return averager << stream;
}

// *******************************************************************************
// *
// *	operator <<
// *
// *******************************************************************************
//!
//!		@param	stream	stream to write to
//!		@param  averager averager to write
//!		@return		    stream for chaining
//!
// *******************************************************************************

std::ostream & operator << ( std::ostream & stream, nAverager const & averager )
{
    return averager >> stream;
}


// *******************************************************************************************
// *
// *	nPingAverager
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nPingAverager::nPingAverager( void )
{
    Reset();
}

// *******************************************************************************************
// *
// *	~nPingAverager
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nPingAverager::~nPingAverager( void )
{
}

// *******************************************************************************************
// *
// *	GetPing
// *
// *******************************************************************************************
//!
//!		@return		our best estimate for the ping
//!
// *******************************************************************************************

REAL nPingAverager::GetPing( void ) const
{
    // collect data
    // determine the lowest guessed value for variance.
    // lag spikes should not contribute here too much.
    REAL variance = 1;
    {
        REAL snailVariance = this->snail_.GetDataVariance();
        REAL slowVariance = this->slow_.GetDataVariance();
        REAL fastVariance = this->fast_.GetDataVariance();
        variance = variance < snailVariance ? variance : snailVariance;
        variance = variance < slowVariance ? variance : slowVariance;
        variance = variance < fastVariance ? variance : fastVariance;
    }

    REAL pingSnail  = this->GetPingSnail();
    REAL pingSlow   = this->GetPingSlow();
    REAL pingFast   = this->GetPingFast();

    // the proposed return value: defaults to the snail ping, it flucuates the least
    REAL pingReturn = pingSnail;

    // return slow average if that differs from the snail one by at least one standard deviation
    if ( ( pingSlow - pingReturn ) * ( pingSlow - pingReturn ) > variance )
    {
        // but clamp it to sane values
        if ( pingSlow > pingReturn * 2 )
            pingSlow = pingReturn * 2;

        pingReturn = pingSlow;
    }

    // same for fast ping
    if ( ( pingFast - pingReturn ) * ( pingFast - pingReturn ) > variance )
    {
        if ( pingFast > pingReturn * 2 )
            pingFast = pingReturn * 2;

        pingReturn = pingFast;
    }

    // return best estimate plus expected variance with fudge factor. It's better to err to the big ping side.
    return pingReturn + sqrtf(variance) * 1.5;
}

// *******************************************************************************************
// *
// *	operator REAL
// *
// *******************************************************************************************
//!
//!		@return		our best estimate for the ping
//!
// *******************************************************************************************

nPingAverager::operator REAL( void ) const
{
    return GetPing();
}

// *******************************************************************************************
// *
// *	GetPingSlow
// *
// *******************************************************************************************
//!
//!		@return		extremely longterm ping average
//!
// *******************************************************************************************

REAL nPingAverager::GetPingSnail( void ) const
{
    return snail_.GetAverage();
}

// *******************************************************************************************
// *
// *	GetPingSlow
// *
// *******************************************************************************************
//!
//!		@return		longterm ping average
//!
// *******************************************************************************************

REAL nPingAverager::GetPingSlow( void ) const
{
    return slow_.GetAverage();
}

// *******************************************************************************************
// *
// *	GetPingFast
// *
// *******************************************************************************************
//!
//!		@return		shortterm ping average
//!
// *******************************************************************************************

REAL nPingAverager::GetPingFast( void ) const
{
    return fast_.GetAverage();
}

// *******************************************************************************************
// *
// *	IsSpiking
// *
// *******************************************************************************************
//!
//!		@return		true if unusual high fluctuations exist in the ping
//!
// *******************************************************************************************

bool nPingAverager::IsSpiking( void ) const
{
    REAL difference = slow_.GetAverage() - fast_.GetAverage();
    return slow_.GetAverageVariance() < difference * difference;
}

// *******************************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************************
//!
//!		@param	decay	time since last timestep
//!
// *******************************************************************************************

void nPingAverager::Timestep( REAL decay )
{
    snail_.Timestep( decay * .02 );
    slow_.Timestep ( decay * .2 );
    fast_.Timestep ( decay * 2 );
}

// *******************************************************************************************
// *
// *	Add
// *
// *******************************************************************************************
//!
//!		@param	value	the value to add
//!		@param	weight	the value's statistical weight
//!
// *******************************************************************************************

void nPingAverager::Add( REAL value, REAL weight )
{
    // add value to both averagers
    snail_.Add( value, weight );
    slow_.Add ( value, weight );
    fast_.Add ( value, weight );
}

// *******************************************************************************************
// *
// *	Add
// *
// *******************************************************************************************
//!
//!		@param	value	the value to add with default weight
//!
// *******************************************************************************************

void nPingAverager::Add( REAL value )
{
    // add value to both averagers
    Add( value, weight_ );
}

// *******************************************************************************************
// *
// *	Reset
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nPingAverager::Reset( void )
{
    snail_.Reset();
    slow_. Reset();
    fast_. Reset();

    // fill in some low weight values
    Add( 1, .000001 );
    Add( 0, .000001 );

    // pin snail averager close to zero
    // snail_.Add(0,10);
    // not such a good idea after all. The above line caused massive resending of packets.
}

REAL nPingAverager::weight_=1;





// *******************************************************************************
// *
// *	nMachine
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

nMachine::nMachine( void )
        : lastUsed_(tSysTimeFloat())
        , banned_(-1)
        , players_(0)
        , decorators_(0)
{
    kph_.Add(0,.1666);
    lastPlayerAction_ = lastUsed_;
}

// *******************************************************************************
// *
// *	~nMachine
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

nMachine::~nMachine( void )
{
    // destroy and remove the decorators
    while ( decorators_ )
    {
        nMachineDecorator * decorator = decorators_;
        decorator->Remove();
        decorator->Destroy();
    }
}

// *******************************************************************************
// *
// *	operator ==
// *
// *******************************************************************************
//!
//!		@param	other	the machine to compare with
//!		@return		    true if they are equal
//!
// *******************************************************************************

bool nMachine::operator == ( nMachine const & other ) const
{
    return this == &other;
}

// *******************************************************************************
// *
// *	operator !=
// *
// *******************************************************************************
//!
//!		@param	other	the machine to compare with
//!		@return		    false if they are equal
//!
// *******************************************************************************

bool nMachine::operator !=( nMachine const & other ) const
{
    return this != &other;
}

// singleton machine map
class nMachinePTR
{
public:
    mutable nMachine * machine;
    nMachinePTR(): machine(tNEW(nMachine)()){};
    ~nMachinePTR(){tDESTROY(machine);}
    nMachinePTR(nMachinePTR const & other): machine(other.machine){other.machine=0;}
    nMachinePTR & operator=(nMachinePTR const & other){ machine = other.machine; other.machine=0;return *this;}
};

typedef std::map< tString, nMachinePTR > nMachineMap;
static nMachineMap & sn_GetMachineMap()
{
    static nMachineMap map;
    return map;
}

static nMachine & sn_LookupMachine( tString const & address )
{
    // get map of all machines and look address up
    nMachineMap & map = sn_GetMachineMap();
    return map[ address ].machine->SetIP( address );
}

// *******************************************************************************
// *
// *	GetMachine
// *
// *******************************************************************************
//!
//!		@param	userID	the user ID to fetch the machine for
//!		@return		    the machine the user ID belongs to
//!
// *******************************************************************************

nMachine & nMachine::GetMachine( unsigned short userID )
{
    // throw out old machines
    Expire();

    // hardcoding: the server itself
    if ( userID == 0 && sn_GetNetState() != nCLIENT )
    {
        static nMachine server;
        return server;
    }

    tASSERT( userID <= MAXCLIENTS+1 );

    if( sn_GetNetState() != nSERVER )
    {
        // invalid ID, return invalid machine (clients don't track machines)
        static nMachine invalid;
        return invalid;
    }

    // get address
    tVERIFY( userID <= MAXCLIENTS+1 );
    if( !sn_Connections[userID].socket )
    {
        // invalid ID, return invalid machine
        static nMachine invalid;
        return invalid;
    }
    tString address;
    peers[ userID ].GetAddress( address );

#ifdef DEBUG_X
    // add client ID so multiple connects from one machine are distinquished
    tString newIP;
    newIP << address << " " << userID;
    address = newIP;
#endif

    // delegate
    return sn_LookupMachine( address );
}

// safely delete iterator from map
static void sn_Erase( nMachineMap & map, nMachineMap::iterator & iter )
{
    if ( iter != map.end() )
    {
        map.erase( iter );
        iter = map.end();
    }
}

// *******************************************************************************
// *
// *	Expire
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachine::Expire( void )
{
    static double lastTime = tSysTimeFloat();
    double time = tSysTimeFloat();
    REAL dt = time - lastTime;
    if (dt <= 60)
        return;
    lastTime = time;

    // iterate over known machines
    nMachineMap & map = sn_GetMachineMap();
    nMachineMap::iterator toErase = map.end();
    int size = map.size();
    for( nMachineMap::iterator iter = map.begin(); iter != map.end(); ++iter )
    {
        // erase last deleted machine
        sn_Erase( map, toErase );

        nMachine & machine = *(*iter).second.machine;

        // advance the kick statistics if the user is not banned and has been active
        if ( time > machine.banned_ && ( machine.lastUsed_ > time - 300 || machine.players_ > 0 ) )
        {
            machine.kph_.Add( 0, dt / 3600 );
            machine.kph_.Timestep( dt / 3600*24 );
        }

        // if the machine is no longer in use, mark it for deletion
        if ( machine.players_ == 0 && machine.lastUsed_ < time - 300.0/size && machine.banned_ < time && machine.kph_.GetAverage() < 0.5 )
            toErase = iter;

    }

    // erase last machine
    sn_Erase( map, toErase );
}

// maximal time a client without players is tolerated
static REAL sn_spectatorTime = 0;
static tSettingItem< REAL > sn_spectatorTimeConf( "NETWORK_SPECTATOR_TIME", sn_spectatorTime );

// *******************************************************************************
// *
// *	KickSpectators
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachine::KickSpectators( void )
{
    double time = tSysTimeFloat();

    // kick spectators
    if ( sn_GetNetState() == nSERVER && sn_spectatorTime > 0 )
    {
        for ( int i = MAXCLIENTS; i >= 1; --i )
        {
            if ( sn_Connections[i].socket )
            {
                nMachine & machine = GetMachine( i );
                if ( machine.players_ == 0 && machine.lastPlayerAction_ + sn_spectatorTime < time )
                {
                    sn_KickUser( i, tOutput("$network_kill_spectator"), 0 );
                }
            }
        }
    }
}

// settings for automatic banning
static REAL sn_autobanOffset = 5;  // bias that gets subtracted from the kills per hour
static REAL sn_autobanFactor = 10; // factor that gets multiplied on top of it to determine the ban time in minutes
static REAL sn_autobanMaxKPH = 30; // maximal value of kph

static tSettingItem< REAL > sn_autobanOffsetSetting( "NETWORK_AUTOBAN_OFFSET", sn_autobanOffset );
static tSettingItem< REAL > sn_autobanFactorSetting( "NETWORK_AUTOBAN_FACTOR", sn_autobanFactor );
static tSettingItem< REAL > sn_autobanMaxKPHSetting( "NETWORK_AUTOBAN_MAX_KPH", sn_autobanMaxKPH );

// *******************************************************************************
// *
// *	OnKick
// *
// *******************************************************************************
//!
//! @param severity the severity of the offense; 1 is standard.
//!
// *******************************************************************************

void nMachine::OnKick( REAL severity )
{
    // if the user is currently banned, don't count
    if ( banned_ > tSysTimeFloat() )
        return;

    // ban the user a bit, taking the kicks per hour into account
    REAL kph = kph_.GetAverage() - sn_autobanOffset;
    if ( kph > 0 )
    {
        // the faster you get kicked when you turn up, the longer you get banned
        REAL banTime = 60 * kph * sn_autobanFactor;
        Ban( banTime, tString(tOutput( "$network_ban_kick" )) );
    }

    // add it to the statistics
    if ( sn_autobanMaxKPH > 0 )
        kph_.Add( severity * sn_autobanMaxKPH, 2/sn_autobanMaxKPH );

    con << tOutput( "$network_ban_kph", GetIP(), GetKicksPerHour() );
}

static bool sn_printBans = true;

// *******************************************************************************
// *
// *	Ban
// *
// *******************************************************************************
//!
//!		@param	time	time in seconds the ban should be in effect
//!
// *******************************************************************************

void nMachine::Ban( REAL time )
{
    lastUsed_ = tSysTimeFloat();

    // set the banning timeout to the current time plus the given time
    banned_ = tSysTimeFloat() + time;

    if ( sn_printBans )
    {
        if ( time > 0 )
            con << tOutput( "$network_ban", GetIP(), int(time/60), banReason_.Len() > 1 ? banReason_ : tOutput( "$network_ban_noreason" ) );
        else
            con << tOutput( "$network_noban", GetIP() );
    }
}

// *******************************************************************************
// *
// *	Ban
// *
// *******************************************************************************
//!
//!		@param	time	time in seconds the ban should be in effect
//!		@param	reason	the reason for the ban
//!
// *******************************************************************************

void nMachine::Ban( REAL time, tString const & reason )
{
    banReason_ = tString();
    if ( reason.Len() > 2 )
        banReason_ = reason;

    Ban( time );
}

// *******************************************************************************
// *
// *	IsBanned
// *
// *******************************************************************************
//!
//!		@return		kick time left
//!
// *******************************************************************************

REAL nMachine::IsBanned( void ) const
{
    // test for banning
    double time = tSysTimeFloat();
    if ( time > banned_ )
        return 0;

    return banned_ - time;
}

// *******************************************************************************
// *
// *	AddPlayer
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachine::AddPlayer( void )
{
    lastPlayerAction_ = lastUsed_ = tSysTimeFloat();

    players_++;
}

// *******************************************************************************
// *
// *	RemovePlayer
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachine::RemovePlayer( void )
{
    lastPlayerAction_ = lastUsed_ = tSysTimeFloat();

    players_--;
    if ( players_ < 0 )
        players_ = 0;
}

// *******************************************************************************
// *
// *	GetPlayerCount
// *
// *******************************************************************************
//!
//!		@return		the number of currently connected players
//!
// *******************************************************************************

int nMachine::GetPlayerCount( void )
{
    return players_;
}


static char const * sn_machinesFileName = "bans.txt";

class nMachinePersistor
{
public:
    // save ban info of machines
    static void SaveMachines()
    {
        std::ofstream s;
        if (tDirectories::Var().Open( s, sn_machinesFileName ) )
        {
            nMachineMap & map = sn_GetMachineMap();
            for( nMachineMap::iterator iter = map.begin(); iter != map.end(); ++iter )
            {
                nMachine & machine = *(*iter).second.machine;
                // if ( machine.IsBanned() > 0 )
                {
                    s << (*iter).first << " " << machine.IsBanned() << " " << machine.kph_ << " " << machine.GetBanReason() << "\n";
                }
            }
        }
    }

    // load and enter ban info of machines
    static void LoadMachines()
    {
        sn_printBans = false;

        tTextFileRecorder machines( tDirectories::Var(), sn_machinesFileName );
        while ( !machines.EndOfFile() )
        {
            std::stringstream line( machines.GetLine() );

            // address and ban time left
            tString address;
            REAL banTime;

            // read relevant info
            line >> address >> banTime;
            std::ws(line);

            // read kph averager
            nAverager kph;
            char c;
            line.get(c);
            line.putback(c);
            if ( c == '(' )
            {
                line >> kph;
                std::ws(line);
            }

            // read reason
            tString reason;
            reason.ReadLine( line );

            if ( address.Len() > 2 )
            {
                // ban
                nMachine & machine = sn_LookupMachine( address );
                machine.Ban( banTime, reason );
                machine.kph_ = kph;
            }
        }

        sn_printBans = true;
    }
}
;
// save ban info of machines
static void sn_SaveMachines()
{
    nMachinePersistor::SaveMachines();
}

// load and enter ban info of machines
static void sn_LoadMachines()
{
    nMachinePersistor::LoadMachines();
}

// *******************************************************************************
// *
// *	GetKicksPerHour
// *
// *******************************************************************************
//!
//!		@return		averaged kicks per hour of players from this machine
//!
// *******************************************************************************

REAL nMachine::GetKicksPerHour( void ) const
{
    return this->kph_.GetAverage();
}

// *******************************************************************************
// *
// *	GetKicksPerHour
// *
// *******************************************************************************
//!
//!		@param	kph	averaged kicks per hour of players from this machine to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine const & nMachine::GetKicksPerHour( REAL & kph ) const
{
    kph = this->kph_.GetAverage();
    return *this;
}

// *******************************************************************************
// *
// *	GetIP
// *
// *******************************************************************************
//!
//!		@return		IP address of the machine
//!
// *******************************************************************************

tString const & nMachine::GetIP( void ) const
{
    return this->IP_;
}

// *******************************************************************************
// *
// *	GetIP
// *
// *******************************************************************************
//!
//!		@param	IP	IP address of the machine to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine const & nMachine::GetIP( tString & IP ) const
{
    IP = this->IP_;
    return *this;
}

// *******************************************************************************
// *
// *	SetIP
// *
// *******************************************************************************
//!
//!		@param	IP	IP address of the machine to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine & nMachine::SetIP( tString const & IP )
{
    lastUsed_ = tSysTimeFloat();

    this->IP_ = IP;
    return *this;
}

// *******************************************************************************
// *
// *	GetBanReason
// *
// *******************************************************************************
//!
//!		@return		Reason of the ban
//!
// *******************************************************************************

tString const & nMachine::GetBanReason( void ) const
{
    return this->banReason_;
}

// *******************************************************************************
// *
// *	GetBanReason
// *
// *******************************************************************************
//!
//!		@param	reason	Reason of the ban to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine const & nMachine::GetBanReason( tString & reason ) const
{
    reason = this->banReason_;
    return *this;
}

// *******************************************************************************
// *
// *	Banning and unbanning
// *
// *******************************************************************************

// unban IPs
static void sn_UnBanConf(std::istream &s)
{
    if ( !s.good() || s.eof() )
    {
        con << "Usage: UNBAN_IP <ip>\n";
        return;
    }

    // read IP to unban
    tString address;
    s >> address;

    if ( address.Len() < 8 )
    {
        con << "Usage: UNBAN_IP <ip>, no or too short ip given.\n";
    }
    // and unban
    else
    {
        sn_LookupMachine( address ).Ban( 0 );
    }
}

static tConfItemFunc sn_unBanConf("UNBAN_IP",&sn_UnBanConf);

// ban IPs
static void sn_BanConf(std::istream &s)
{
    // read IP to unban
    tString address;
    s >> address;

    if ( !s.good() && address.Len() < 7 )
    {
        con << "Usage: BAN_IP <ip> <time in minutes (defaults to 60)> <reason>\n";
        return;
    }

    REAL duration = 60;
    s >> duration;

    // read reason
    tString reason;
    std::ws(s);
    if ( s.good() )
    {
        reason.ReadLine(s);
    }

    // and ban
    if ( address.Len() > 4 )
    {
        sn_LookupMachine( address ).Ban( duration * 60, reason );
    }
}

static tConfItemFunc sn_banConf("BAN_IP",&sn_BanConf);

// list bans
static void sn_ListBanConf(std::istream &s)
{
    nMachineMap & map = sn_GetMachineMap();
    for( nMachineMap::iterator iter = map.begin(); iter != map.end(); ++iter )
    {
        nMachine & machine = *(*iter).second.machine;
        REAL banned = machine.IsBanned();
        if ( banned > 0 )
        {
            con << tOutput( "$network_ban", machine.GetIP(), int(banned/60), machine.GetBanReason() );
        }
    }
}

static tConfItemFunc sn_listBanConf("BAN_LIST",&sn_ListBanConf);

// *******************************************************************************
// *
// *	OnDestroy
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachineDecorator::OnDestroy( void )
{
}

// *******************************************************************************
// *
// *	nMachineDecorator
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

nMachineDecorator::nMachineDecorator( void )
{
}

// *******************************************************************************
// *
// *	~nMachineDecorator
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

nMachineDecorator::~nMachineDecorator( void )
{
    Remove();
}

// *******************************************************************************
// *
// *	nMachineDecorator
// *
// *******************************************************************************
//!
//!		@param	machine
//!
// *******************************************************************************

nMachineDecorator::nMachineDecorator( nMachine & machine )
{
    Insert( machine.decorators_ );
}

