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

#ifndef ArmageTron_NET_H
#define ArmageTron_NET_H

#include "tError.h"
#include "tString.h"
#include "tHeap.h"
#include "tLinkedList.h"
#include "tCallback.h"
#include "nObserver.h"
//#include "tCrypt.h"
#include "tException.h"
#include <memory>

class nSocket;
class nAddress;
class nBasicNetworkSystem;
class nServerInfoBase;

extern nBasicNetworkSystem sn_BasicNetworkSystem;

class nMessage;
class tCrypt;
class tOutput;

typedef double nTimeAbsolute;				// typedef for absolute time variables in network code
typedef double nTimeRolling;				// typedef for time variables in network code that don't have to measure large time differences

extern tString sn_bigBrotherString; // the string that is sent
// to the server for system information

extern tString sn_programVersion; // our version

extern tString sn_serverName;     // telling name of the server

extern  unsigned int sn_serverPort;       // port we listen on when in server mode
extern const unsigned int sn_defaultPort; // default port a server listens on

extern int sn_defaultDelay;

extern  bool sn_decorateTS;

extern tString sn_DenyReason;		// the reason the server gave for sending a login_deny packet

// rate control
extern int sn_maxRateIn,sn_maxRateOut;

//! exception that is thrown on any unexpected network error;
//! causes the owner of the current nNetObject or the sender of
//! the currently processed netmessage to be killed.
class nKillHim: public tException
{
public:
    nKillHim();
    ~nKillHim();

private:
    virtual tString DoGetName()         const;              //!< returns the name of the exception
    virtual tString DoGetDescription()  const;              //!< returns a detailed description
};

//! exception that is thrown on errors where it is safe to just ignore the
//! offending packet.
class nIgnore: public nKillHim
{
public:
    nIgnore();
    ~nIgnore();

private:
    virtual tString DoGetName()         const;              //!< returns the name of the exception
    virtual tString DoGetDescription()  const;              //!< returns a detailed description
};

// call this function on any error occuring while reading a message:
void nReadError( bool critical = true );

#ifndef MAXCLIENTS
#ifdef DEDICATED
#define MAXCLIENTS 32
#else
#define MAXCLIENTS 16
#endif
#endif

// We can be single player, multiplayer server/client.
typedef enum {nSTANDALONE,nSERVER,nCLIENT} nNetState;
typedef enum {nOK, nTIMEOUT, nDENIED, nABORT}   nConnectError;


// set/get the state
nConnectError sn_GetLastError();
nNetState sn_GetNetState();
void sn_SetNetState(nNetState x);
void sn_DisconnectUser(int i, const tOutput& reason, nServerInfoBase * redirectTo = 0 ); //!< terminate connection with user i (peacefully)
void sn_KickUser(int i, const tOutput& reason, REAL severity = 1, nServerInfoBase * redirectTo = 0 );   //!< throw out user i (violently)

void sn_GetAdr(int user,  tString& name);
unsigned int sn_GetPort(int user);
unsigned int sn_GetServerPort();
int sn_NumUsers();
int sn_MaxUsers();
int sn_MessagesPending(int user);

// information about currently supported versions
class nVersion
{
public:
    nVersion();
    nVersion( int min, int max );
    bool Supported( int version ) const;	// check if a particular version is supported
    bool Merge( const nVersion& a,
                const nVersion& b);	// merges two versions to one; the new version supports only features both versions understand. false is returned if no common denominator could be found

    int Min() const	{ return min_;}
    int Max() const	{ return max_;}

    bool operator != ( const nVersion& other ){ return !operator==(other); }
    bool operator == ( const nVersion& other );
    nVersion& operator = ( const nVersion& other );
private:
    int min_, max_;
};

nMessage& operator >> ( nMessage& m, nVersion& ver );
nMessage& operator << ( nMessage& m, const nVersion& ver );

std::istream& operator >> ( std::istream& s, nVersion& ver );
std::ostream& operator << ( std::ostream& s, const nVersion& ver );

const nVersion& sn_MyVersion();			//!< the version this progam maximally supports
const nVersion& sn_CurrentVersion();	//!< the version currently supported by all connected players
void sn_UpdateCurrentVersion();         //!< updates the sn_CurrentVersion()

// features that are not available in all currently supported versions
class nVersionFeature
{
public:
    nVersionFeature( int min, int max = -1 ); // creates a feature that is supported from version min to max; values of -1 indicate no border
    bool Supported() const;                   // returns whether this feature is supported by everyone
    bool Supported( int client ) const;       // returns whether this feature is supported by a certain client ( resp. the server in client mode )
private:
    int min_, max_;
};

class nBandwidthControl;

// send buffer: stores network messages temporarily before really sending them
class nSendBuffer
{
public:
    int Len				() const { return sendBuffer_.Len(); }		// returns the length of the buffer

    void AddMessage		( nMessage&			message
                       , nBandwidthControl* control );				// adds a message to the buffer
    void Send			( nSocket const & 	socket
                  , const nAddress &	peer
                  , nBandwidthControl* control );				// send the contents of the buffer to a specific socket
    void Broadcast		( nSocket const &   socket
                      , int				    port
                      , nBandwidthControl* control );				// broadcast the contents of the buffer

    void Clear();													// clears the buffer

private:
    tArray<unsigned short> sendBuffer_;
};

class nBandwidthControl
{
public:
    enum Usage
    {
        Usage_Planning,
        Usage_Execution
    };

    nBandwidthControl( nBandwidthControl* parent = NULL );
    ~nBandwidthControl();
    void Reset();

    void			SetRate( unsigned short rate ){ rate_ = rate; }
    unsigned short	Rate()	{ return rate_; }

REAL			Control( Usage planned ){ return Usage_Planning == planned ? rateControlPlanned_ : rateControl_;}
    void			Use( Usage planned, REAL bandwidth );

    bool			CanSend(){ return rateControlPlanned_ > 0; }
    REAL			Score(){ return rateControlPlanned_ / rate_; }

    void			Update( REAL ts);
private:
    REAL						rateControlPlanned_;
    REAL						rateControl_;
    unsigned short				rate_;
    nBandwidthControl*			parent_;
#ifdef DEBUG
    int							numChildren_;
#endif
};

//! class responsible for calculating averages
class nAverager
{
public:
    nAverager();                           //!< constructor
    ~nAverager();                          //!< destructor
    REAL GetAverage() const;               //!< returns the average value
    REAL GetDataVariance() const;          //!< returns the variance of the data ( average of (value-average)^2 )
    REAL GetAverageVariance() const;       //!< returns the expected variance of the returned average
    REAL GetWeight() const;                //!< returns the current weight
    void Timestep( REAL decay );           //!< lets all values decay, so they can be replaced by new ones
    void Add( REAL value, REAL weight=1 ); //!< adds a value to the average
    void Reset();                          //!< resets average to zero

    std::istream & operator << ( std::istream & stream );       //!< read operator
    std::ostream & operator >> ( std::ostream & stream ) const; //!< write operator
private:
    REAL weight_;       //!< the total statistical weight
    REAL sum_;          //!< the total sum of value*weight
    REAL sumSquared_;   //!< the total sum of value*value*weight
    REAL weightSquared_;//!< the sum of all weights, squared
};

//! read operator for nAveragers
std::istream & operator >> ( std::istream & stream, nAverager & averager );

//! write operator for nAveragers
std::ostream & operator << ( std::ostream & stream, nAverager const & averager );

//! averager for pings, detects lag
class nPingAverager
{
public:
    nPingAverager();              //!< constructor
    ~nPingAverager();             //!< destructor

    operator REAL() const;        //!< conversion to number == GetPing()

    REAL GetPing() const;         //!< returns the current best estimate for the ping
    REAL GetPingSnail() const;    //!< returns the extremely longterm average ping
    REAL GetPingSlow() const;     //!< returns the longterm average ping
    REAL GetPingFast() const;     //!< returns the shortterm average ping
    bool IsSpiking() const ;      //!< returns whether a lag spike is currently in process
    void Timestep( REAL decay );  //!< lets all values decay, so they can be replaced by new ones
    void Add( REAL value, REAL weight ); //!< adds a value to the average
    void Add( REAL value );              //!< adds a value to the average
    void Reset();                 //!< resets average to zero
private:
    nAverager snail_;    //!< extremely slow averager
    nAverager slow_;     //!< slow, reliable averager
    nAverager fast_;     //!< fast averager for detecting ping spikes
    static REAL weight_; //!< current default weight
public:
    // accessors
    inline static void SetWeight( REAL const & weight ); //!< Sets the default statistical weight
    inline static REAL GetWeight( void );	             //!< Gets the default statistical weight
    inline static void GetWeight( REAL & weight );	     //!< Gets the default statistical weight
    inline nAverager const & GetSnailAverager( void ) const;	//!< Gets extremely slow averager
    inline nPingAverager const & GetSnailAverager( nAverager & snail ) const;	//!< Gets extremely slow averager
    inline nAverager const & GetSlowAverager( void ) const;	//!< Gets slow, reliable averager
    inline nPingAverager const & GetSlowAverager( nAverager & slow ) const;	//!< Gets slow, reliable averager
    inline nAverager const & GetFastAverager( void ) const;	//!< Gets fast averager for detecting ping spikes
    inline nPingAverager const & GetFastAverager( nAverager & fast ) const;	//!< Gets fast averager for detecting ping spikes
protected:
    inline nPingAverager & SetSnailAverager( nAverager const & snail );	//!< Sets very slow averager
    inline nPingAverager & SetSlowAverager( nAverager const & slow );	//!< Sets slow, reliable averager
    inline nPingAverager & SetFastAverager( nAverager const & fast );	//!< Sets fast averager for detecting ping spikes
};

struct nConnectionInfo     // everything that is needed to manage a connection
{
    nSocket const *        socket;             // the network UDP socket
    int                    ackPending;

    nPingAverager          ping;

    // tCrypt*                crypt;

    // rate control
    nBandwidthControl		bandwidthControl_;

    // send buffer
    nSendBuffer				sendBuffer_;

    // version info
    nVersion				version;

    // ack messages
    tJUST_CONTROLLED_PTR< nMessage >          ackMess;

    // authentication
    // tString                userName;

    // supported authentication methods of the client in a comma separated list
    tString                 supportedAuthenticationMethods_;

    nConnectionInfo();
    ~nConnectionInfo();

    void Clear();
    void Timestep( REAL dt );    //!< call to update time related information

    void ReliableMessageSent();  //!< call whenever an an reliable message got sent
    void AckReceived();          //!< call whenever an ackownledgement message arrives
    REAL PacketLoss() const;     //!< returns the average packet loss ratio
    private:
    // packet loss measurement
    nAverager              packetLoss_;

};

extern nConnectionInfo sn_Connections[MAXCLIENTS+2];

extern int sn_maxNoAck;
//extern int sn_ackPending[MAXCLIENTS+2];
//extern int sn_sockets[MAXCLIENTS+2];
//extern REAL sn_ping[MAXCLIENTS+2];

// types of login
enum nLoginType
{
    Login_Pre0252,  // use the login method known to pre-0.2.5.2 versions
    Login_Post0252, // use the newer login method
    Login_All       // first attempt the new one, then the old one
};

// go to client mode and connect to server
nConnectError sn_Connect(nAddress const & server, nLoginType loginType = Login_All, nSocket const * socket = NULL );

// let the server connection socket point to a new server (EVIL!)
void sn_Bend( nAddress const & server );
void sn_Bend( tString  const & server, unsigned int port );

extern int sn_myNetID; // our network identification:  0: server
//                                                1..MAXCLIENTS: client

// Network messages and functions that know how to handle them:

class nMessage;
typedef void nHandler(nMessage &m);


// types of network messages
class nDescriptor:public tListItem<nDescriptor>{
    friend class nMessage;

    static unsigned short s_nextID;

    unsigned short id;     // our id
    nHandler *handler;  // function responsible for our type of message

    const char *name;

    const bool acceptWithoutLogin;
public:
    nDescriptor(unsigned short identification,nHandler *handle
                ,const char *name, bool acceptEvenIfNotLoggedIn = false);
    //  nDescriptor(nHandler *handle,
    //		const char *name);
    static void HandleMessage(nMessage &message);

    unsigned short ID(){return id;}
};

// register the routine that gives the peer the server/client information
// (game type, number of players online....)
void RequestInfoHandler(nHandler *handle);

// the first sn_myNetID available for external use (give some room!)
#define NET_ID_FIRST 100

// Network messages. Allways to be created with new, get deleted automatically.

class nMessage: public tReferencable< nMessage >{
    //friend class nMessage_planned_send;
    friend class tControlledPTR< nMessage >;
    friend class tReferencable< nMessage >;

    friend class nDescriptor;
    friend class nNetObject;
    friend class nWaitForAck;

    //	void AddRef();
    //	void Release();

protected:
    unsigned short descriptor;    // the network message type id
    // unsigned short messageID_;    // the network message id, every message gets its own (up to overflow)
    unsigned long messageIDBig_;  // uniquified message ID, the last 16 bits are the same as messageID_
    short          senderID;      // sender's identification
    tArray<unsigned short> data;  // assuming ints are 32 bit wide...

    unsigned int readOut;

    ~nMessage();
public:
    unsigned short Descriptor() const{
        return descriptor;
    }

    unsigned short SenderID() const{
        return senderID;
    }

    unsigned short MessageID() const{
        return messageIDBig_ & 0xFFFF;
    }

    unsigned long MessageIDBig() const{
        return messageIDBig_;
    }

    unsigned short DataLen() const{
        return data.Len();
    }

    unsigned short Data(unsigned short n){
        return data(n);
    }

    void ClearMessageID(){ // clear the message ID so no acks are sent for it
        messageIDBig_ = 0;
    }

    void BendMessageID( int id ){ // bends the message ID. Use with extreme caution.
        messageIDBig_ = id;
    }

    nMessage(const nDescriptor &);  // create a new message
    nMessage(unsigned short*& buffer, short sn_myNetID, int lenLeft );
    // read a message from the network stream

    // immediately send the message WITHOUT the queue; dangerous!
    void SendImmediately(int peer,bool ack=true);

    // flush the buffers of that peer
    static void SendCollected(int peer);

    // broadcast the same information across the LAN
    static void BroadcastCollected(int peer, unsigned int port);

    // send it to anyone who is interested
    // (the server in client mode, all clients in server mode)
    void BroadCast(bool ack=true);

    // put the message into the send heap
    void Send(int peer,REAL priority=0,bool ack=true);

    void Write(const unsigned short &x)
    {
        data[data.Len()]=x;
    }

    nMessage& operator<< (const REAL &x);
    nMessage& operator>> (REAL &x);

    nMessage& operator<< (const unsigned short &x){Write(x);return *this;}
    nMessage& operator>> (unsigned short &x){Read(x);return *this;}

    nMessage& operator<< (const double &x){
        return operator<<(REAL(x));
    }

    nMessage& operator>> (double &x){
        REAL y;
        operator>>(y);
        x=y;

        return *this;
    }

    // read a string without any kind of filtering
    nMessage& ReadRaw(tString &s);

    nMessage& operator >> (tString &s);
    nMessage& operator >> (tColoredString &s);
    nMessage& operator << (const tString &s);
    nMessage& operator << (const tColoredString &s);
    nMessage& operator << (const tOutput &o);

    template<class T> void BinWrite (const T &x){
        for(unsigned int i=0;i<sizeof(T)/2;i++)
            Write((reinterpret_cast<const unsigned short *>(&x))[i]);
    }

bool End(){return readOut>=static_cast<unsigned int>(data.Len());}

    void Reset(){readOut=0;}

    void Read(unsigned short &x);

    template<class T> void BinRead (const T &x){
        for(unsigned int i=0;i<sizeof(T)/2;i++)
            Read(reinterpret_cast<unsigned short *>(&x)[i]);
    }


    nMessage& operator<< (const short &x);
    nMessage& operator>> (short &x);

    nMessage& operator<< (const int &x);
    nMessage& operator>> (int &x);

nMessage& operator<< (const unsigned int &x){operator<<(reinterpret_cast<const int&>(x));return *this;}
    nMessage& operator>> (unsigned int &x){operator>>(reinterpret_cast<int&>(x));return *this;}

    nMessage& operator<< (const bool &x);
    nMessage& operator>> (bool &x);

    template <class T> nMessage& operator<<(const tArray<T>& a)
    {
        unsigned short len = a.Len();
        Write(len);
        for (int i=a.Len()-1; i>=0; i--)
            operator<< (a(i));

        return *this;
    }

    template <class T> nMessage& operator>>(tArray<T>& a)
    {
        unsigned short len;
        Read(len);
        a.SetLen(len);
        for (int i=a.Len()-1; i>=0; i--)
            operator >> (a(i));

        return *this;
    }

    template<class T> nMessage& operator << (const T* p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }

    //    template<class T> nMessage& operator >> ( T*& p );

    template<class T> nMessage& operator << (const tControlledPTR<T> p)
    {
        if (p)
            Write( p->ID() );
        else
            Write(0);

        return *this;
    }

    //    template<class T> nMessage& operator >> ( tControlledPTR<T>& p );
};



// the class that is responsible for getting acknowleEdgement for
// netmessages

class nWaitForAck{
protected:
    int id;
    tCONTROLLED_PTR(nMessage) message;  // the message
    int           receiver;      // the computer who should send the ack
    REAL          timeout;       // the time in seconds between send attempts
    nTimeRolling  timeSendAgain; // for timeout
    nTimeRolling  timeFirstSent; // for ping calculation
    nTimeRolling  timeLastSent;  // for ping calculation
    int           timeouts;

public:
    nWaitForAck(nMessage* m,int rec);
    virtual ~nWaitForAck();

    virtual void AckExtraAction(){}

    static void Ackt(unsigned short id,unsigned short peer);

    static void AckAllPeer(unsigned short peer);

    static void Resend();
};


//! pause a bit, abort pause on network activity
void sn_Delay();

// process the messages from all hosts and send acks
void sn_Receive();

// receive and process data from control socket (used on master server to ping servers)
extern void sn_ReceiveFromControlSocket();

// receive and discard data from control socket (used on regular servers to keep the pipe clean)
extern void sn_DiscardFromControlSocket();

// attempts to sync with server/all clients (<=> wait for all acks)
// sync_netObjects: if set, network objects are synced as well
// otherEnd: if set, the client instructs the server to send all packets and waits for completion.
// if unset, the client just sends all packets and hopes for the best.
void sn_Sync(REAL timeout,bool sync_sn_netObjects=false, bool otherEnd=true); // defined in nNetObject.C

// causes the connected clients to print a message
void sn_ConsoleOut(const tOutput &message,int client=-1);
nMessage* sn_ConsoleOutMessage( tString const & message );

// causes the connected clients to print a message in the center of the screeen
void sn_CenterMessage(const tOutput &message,int client=-1);


// **********************************************


class nCallbackLoginLogout: public tCallback{
    static int  user;
    static bool login;
public:
    static int User(){return user;}
    static int Login(){return login;}

    nCallbackLoginLogout(VOIDFUNC *f);
    static void UserLoggedIn(int user);
    static void UserLoggedOut(int user);
};

class nCallbackAcceptPackedWithoutConnection: public tCallbackOr{
    static unsigned short descriptor;	// the descriptor of the incoming packet
public:
    static unsigned int Descriptor(){return descriptor;}

    nCallbackAcceptPackedWithoutConnection(BOOLRETFUNC *f);

    static bool Accept( const nMessage& m );
};

class nCallbackReceivedComplete: public tCallback
{
public:
    nCallbackReceivedComplete(VOIDFUNC *f);
    static void ReceivedComplete();
};

void sn_SendPlanned();
int sn_QueueLen(int user);

void sn_Statistics();

//! return the public IP address and port of this machine, or "*.*.*.*:*" if it is unknown..
tString const & sn_GetMyAddress();

//! checks wheter a given address is on the user's LAN (or on loopback).
bool sn_IsLANAddress( tString const & address );

// the SenderID of the currently handled message is stored here for reference
class nCurrentSenderID
{
public:
    nCurrentSenderID():lastSenderID_( currentSenderID_ ){}
    nCurrentSenderID( int senderID ):lastSenderID_( currentSenderID_ ){ SetID( senderID ); }
    ~nCurrentSenderID(){ currentSenderID_ = lastSenderID_; }

    static int GetID(){ return currentSenderID_; }
    void SetID( int senderID ){ currentSenderID_ = senderID; }
private:
    int lastSenderID_;
    static int currentSenderID_;
};

class nMachine;

//! a decorator for machines for additional information to attach to them
class nMachineDecorator: public tListItem< nMachineDecorator >
{
public:
    inline void Destroy();         //!< called when machine gets destroyed
protected:
    virtual void OnDestroy();      //!< called when machine gets destroyed

    nMachineDecorator( nMachine & machine );   //!< constructor
    virtual ~nMachineDecorator();  //!< destructor
private:
    nMachineDecorator();           //!< constructor
};

//! class trying to collect information about a certain client, persistent between connections
class nMachine
{
    friend class nMachineDecorator;
    friend class nMachinePersistor;
public:
    nMachine();          //!< constructor
    virtual ~nMachine(); //!< destructor

    bool operator == ( nMachine const & other ) const; //!< equality operator
    bool operator != ( nMachine const & other ) const; //!< inequality operator

    static nMachine & GetMachine( unsigned short userID ); //!< fetches the machine information of a user, creating it on demand
    static nMachine * PeekMachine( unsigned short userID ); //!< fetches the machine information of a user, returning NULL if none is found
    static void Expire();                       //!< expires machine information that is no longer needed
    static void KickSpectators();               //!< remove clients without players from the server

    // kicking
    void     OnKick( REAL severity = 1 );       //!< call this when you kick a user from this machine, frequent kickees will be banned

    // banning
    void     Ban( REAL time );                         //!< ban users from this machine for the given time
    void     Ban( REAL time, tString const & reason ); //!< ban users from this machine for the given time
    REAL     IsBanned() const; //!< returns the number of seconds users from this machine are currently banned for

    bool      IsValidated() const { return validated_; }
    void      Validate()          { validated_ = true; }

    // player accounting
    void      AddPlayer();     //!< call when a player joins from this machine
    void      RemovePlayer();  //!< call when a player rom this machine leaves
    int       GetPlayerCount(); //!< returns the number of currently registered players
private:
    nMachine( nMachine const & other );               //!< copy constructor
    nMachine & operator = ( nMachine const & other ); //!< copy operator

    // variables
    mutable double lastUsed_;     //!< system time where this machine was last used
    mutable double banned_;       //!< system time until that this machine is banned
    tString        banReason_;    //!< reason for a ban
    nAverager      kph_;          //!< averaged kicks per hour of players from this machine
    int            players_;      //!< number of players coming from this machine currently
    REAL           lastPlayerAction_; //!< time of the last player action

    bool           validated_;    //!< true if the machine has been validated as a real client without spoofed IP

    tString        IP_;           //!< IP address of the machine
    nMachineDecorator * decorators_; //!< list of decorators

    // accessors
public:
    REAL GetKicksPerHour( void ) const;                    //!< Gets averaged kicks per hour of players from this machine
    nMachine const & GetKicksPerHour( REAL & kph ) const;  //!< Gets averaged kicks per hour of players from this machine
    nMachine & SetIP( tString const & IP );	               //!< Sets IP address of the machine
    tString const & GetIP( void ) const;	               //!< Gets IP address of the machine
    nMachine const & GetIP( tString & IP ) const;	       //!< Gets IP address of the machine
    tString const & GetBanReason( void ) const;	           //!< Gets the reason of the ban
    nMachine const & GetBanReason( tString & reason )const;//!< Gets the reason of the ban
    inline nMachineDecorator * GetDecorators( void ) const;	//!< Gets list of decorators
    inline nMachine const & GetDecorators( nMachineDecorator * & decorators ) const;	//!< Gets list of decorators

    //! returns the next machine decorator of a given type
    template< class T > static T * GetNextDecorator( nMachineDecorator * run )
    {
        while ( run )
        {
            T * ret = dynamic_cast< T * >( run );
            if ( ret )
            {
                return ret;
            }
            run = run->Next();
        }

        return 0;
    }

    //! returns the first machine decorator of a given type
    template< class T > T * GetDecorator()
    {
        return GetNextDecorator< T >( GetDecorators() );
    }
protected:
private:
    inline nMachine & SetDecorators( nMachineDecorator * decorators );	//!< Sets list of decorators
};

//! while an object of this class exists, the main network sockets won't get reopened
class nSocketResetInhibitor
{
public:
    nSocketResetInhibitor();
    ~nSocketResetInhibitor();
};

// on disconnection, this returns a server we should be redirected to (or NULL if we should not be redirected)
std::auto_ptr< nServerInfoBase > sn_GetRedirectTo();

// take a peek at the same info
nServerInfoBase * sn_PeekRedirectTo();


// *******************************************************************************************
// *
// *	GetWeight
// *
// *******************************************************************************************
//!
//!		@return		the default statistical weight
//!
// *******************************************************************************************

REAL nPingAverager::GetWeight( void )
{
    return weight_;
}

// *******************************************************************************************
// *
// *	GetWeight
// *
// *******************************************************************************************
//!
//!		@param	weight	the default statistical weight to fill
//!
// *******************************************************************************************

void nPingAverager::GetWeight( REAL & weight )
{
    weight = weight_;
}

// *******************************************************************************************
// *
// *	SetWeight
// *
// *******************************************************************************************
//!
//!		@param	weight	the default statistical weight to set
//!
// *******************************************************************************************

void nPingAverager::SetWeight( REAL const & weight )
{
    weight_ = weight;
}

// *******************************************************************************************
// *
// *	GetSnailAverager
// *
// *******************************************************************************************
//!
//!		@return		slow, reliable averager
//!
// *******************************************************************************************

nAverager const & nPingAverager::GetSnailAverager( void ) const
{
    return this->snail_;
}

// *******************************************************************************************
// *
// *	GetSnailAverager
// *
// *******************************************************************************************
//!
//!		@param	snail	very slow averager to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager const & nPingAverager::GetSnailAverager( nAverager & snail ) const
{
    snail = this->snail_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetSnailAverager
// *
// *******************************************************************************************
//!
//!		@param	snail	very slow averager to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager & nPingAverager::SetSnailAverager( nAverager const & snail )
{
    this->snail_ = snail;
    return *this;
}
// *******************************************************************************************
// *
// *	GetSlowAverager
// *
// *******************************************************************************************
//!
//!		@return		slow, reliable averager
//!
// *******************************************************************************************

nAverager const & nPingAverager::GetSlowAverager( void ) const
{
    return this->slow_;
}

// *******************************************************************************************
// *
// *	GetSlowAverager
// *
// *******************************************************************************************
//!
//!		@param	slow	slow, reliable averager to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager const & nPingAverager::GetSlowAverager( nAverager & slow ) const
{
    slow = this->slow_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetSlowAverager
// *
// *******************************************************************************************
//!
//!		@param	slow	slow, reliable averager to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager & nPingAverager::SetSlowAverager( nAverager const & slow )
{
    this->slow_ = slow;
    return *this;
}

// *******************************************************************************************
// *
// *	GetFastAverager
// *
// *******************************************************************************************
//!
//!		@return		fast averager for detecting ping spikes
//!
// *******************************************************************************************

nAverager const & nPingAverager::GetFastAverager( void ) const
{
    return this->fast_;
}

// *******************************************************************************************
// *
// *	GetFastAverager
// *
// *******************************************************************************************
//!
//!		@param	fast	fast averager for detecting ping spikes to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager const & nPingAverager::GetFastAverager( nAverager & fast ) const
{
    fast = this->fast_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetFastAverager
// *
// *******************************************************************************************
//!
//!		@param	fast	fast averager for detecting ping spikes to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nPingAverager & nPingAverager::SetFastAverager( nAverager const & fast )
{
    this->fast_ = fast;
    return *this;
}

// *******************************************************************************
// *
// *	Destroy
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void nMachineDecorator::Destroy( void )
{
    this->OnDestroy();
}

// *******************************************************************************
// *
// *	GetDecorators
// *
// *******************************************************************************
//!
//!		@return		list of decorators
//!
// *******************************************************************************

nMachineDecorator * nMachine::GetDecorators( void ) const
{
    return this->decorators_;
}

// *******************************************************************************
// *
// *	GetDecorators
// *
// *******************************************************************************
//!
//!		@param	decorators	list of decorators to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine const & nMachine::GetDecorators( nMachineDecorator * & decorators ) const
{
    decorators = this->decorators_;
    return *this;
}

// *******************************************************************************
// *
// *	SetDecorators
// *
// *******************************************************************************
//!
//!		@param	decorators	list of decorators to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nMachine & nMachine::SetDecorators( nMachineDecorator * decorators )
{
    this->decorators_ = decorators;
    return *this;
}

#endif




