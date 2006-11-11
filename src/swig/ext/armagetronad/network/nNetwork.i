%{
#include "nNetwork.h"
%}

%rename(Socket) nSocket;
class nSocket;
%rename(Address) nAddress;
class nAddress;
%rename(BasicNetworkSystem) nBasicNetworkSystem;
class nBasicNetworkSystem;

// extern nBasicNetworkSystem sn_BasicNetworkSystem;

class nMessage;
class tCrypt;
class tOutput;


typedef double nTimeAbsolute;
typedef double nTimeRolling;

%rename(KillHim) nKillHim;
class nKillHim: public tException
{
public:
    nKillHim();
    ~nKillHim();
};

%rename(read_error) nReadError;
void nReadError();

typedef enum {nSTANDALONE,nSERVER,nCLIENT} nNetState;
typedef enum {nOK, nTIMEOUT, nDENIED}      nConnectError;

%rename(get_last_error) sn_GetLastError;
nConnectError sn_GetLastError();

%rename(get_net_state) sn_GetNetState;
nNetState sn_GetNetState();

%rename(set_net_state) sn_SetNetState;
void sn_SetNetState(nNetState x);

%rename(disconnect_user) sn_DisconnectUser;
void sn_DisconnectUser(int i, const tOutput& reason);

%rename(kick_user) sn_KickUser;
void sn_KickUser(int i, const tOutput& reason, REAL severity = 1 );

%rename(get_adr) sn_GetAdr;
void sn_GetAdr(int user,  tString& name);

%rename(get_port) sn_GetPort;
unsigned int sn_GetPort(int user);

%rename(get_server_port) sn_GetServerPort;
unsigned int sn_GetServerPort();

%rename(num_users) sn_NumUsers;
int sn_NumUsers();

%rename(max_users) sn_MaxUsers;
int sn_MaxUsers();

%rename(message_pending) sn_MessagesPending;
int sn_MessagesPending(int user);


%rename(Version) nVersion;
class nVersion
{
public:
    nVersion();
    nVersion( int min, int max );
    bool Supported( int version ) const;
    bool Merge( const nVersion& a,
                const nVersion& b);
    int Min() const;
    int Max() const;
};

nMessage& operator >> ( nMessage& m, nVersion& ver );
nMessage& operator << ( nMessage& m, const nVersion& ver );
std::istream& operator >> ( std::istream& s, nVersion& ver );
std::ostream& operator << ( std::ostream& s, const nVersion& ver );

%rename(my_version) sn_MyVersion;
const nVersion& sn_MyVersion();
%rename(current_version) sn_CurrentVersion;
const nVersion& sn_CurrentVersion();
%rename(update_current_version) sn_UpdateCurrentVersion;
void sn_UpdateCurrentVersion();

%rename(VersionFeature) nVersionFeature;
class nVersionFeature
{
public:
    nVersionFeature( int min, int max = -1 );
    bool Supported();
    bool Supported( int client );
};

%rename(BandwidthControl) nBandwidthControl;
class nBandwidthControl;

%rename(SendBuffer) nSendBuffer;
class nSendBuffer
{
public:
    int Len() const;
    void AddMessage( nMessage& message, nBandwidthControl* control );
    void Send( nSocket const & socket, const nAddress &	peer, nBandwidthControl* control );
    void Broadcast( nSocket const &   socket, int port, nBandwidthControl* control );
    void Clear();
};

%rename(BandWidthControl) nBandwidthControl;
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

    void			SetRate( unsigned short rate );
    unsigned short	Rate();

REAL			Control( Usage planned );
    void			Use( Usage planned, REAL bandwidth );

    bool			CanSend();
    REAL			Score();

    void			Update( REAL ts);
};

%rename(Averager) nAverager;
class nAverager
{
public:
    nAverager();                          
    ~nAverager();                         
    REAL GetAverage() const;              
    REAL GetDataVariance() const;         
    REAL GetAverageVariance() const;      
    void Timestep( REAL decay );          
    void Add( REAL value, REAL weight=1 );
    void Reset();                         

    std::istream & operator << ( std::istream & stream );       //!< read operator
    std::ostream & operator >> ( std::ostream & stream ) const; //!< write operator
};

std::istream & operator >> ( std::istream & stream, nAverager & averager );
std::ostream & operator << ( std::ostream & stream, nAverager const & averager );

%rename(PingAverager) nPingAverager;
class nPingAverager
{
public:
    nPingAverager();            
    ~nPingAverager();           
                                
    operator REAL() const;      
                                
    REAL GetPing() const;       
    REAL GetPingSnail() const;  
    REAL GetPingSlow() const;   
    REAL GetPingFast() const;   
    bool IsSpiking() const ;    
    void Timestep( REAL decay );
    void Add( REAL value, REAL weight ); //!< adds a value to the average
    void Add( REAL value );              //!< adds a value to the average
    void Reset();                 //!< resets average to zero
    inline static void SetWeight( REAL const & weight ); 
    inline static REAL GetWeight( void );	             
    inline static void GetWeight( REAL & weight );	     
    inline nAverager const & GetSnailAverager( void ) const;
    inline nPingAverager const & GetSnailAverager( nAverager & snail ) const;
    inline nAverager const & GetSlowAverager( void ) const;
    inline nPingAverager const & GetSlowAverager( nAverager & slow ) const;
    inline nAverager const & GetFastAverager( void ) const;
    inline nPingAverager const & GetFastAverager( nAverager & fast ) const;
protected:
    inline nPingAverager & SetSnailAverager( nAverager const & snail );
    inline nPingAverager & SetSlowAverager( nAverager const & slow );
    inline nPingAverager & SetFastAverager( nAverager const & fast );
};

%rename(ConnectionInfo) nConnectionInfo;
struct nConnectionInfo
{
    nSocket const *        socket;
%rename(ack_pending) ackPending;
    int                    ackPending;
    nPingAverager          ping;
    tCrypt*                crypt;
%rename(bandwidth_control) bandwidthControl_;
    nBandwidthControl		bandwidthControl_;
%rename(send_buffer) sendBuffer_;
    nSendBuffer				sendBuffer_;
    nVersion				version;
%rename(ack_mess) ackMess;
    tJUST_CONTROLLED_PTR< nMessage >          ackMess;
%rename(user_name) userName;
    tString                userName;
    nConnectionInfo();
    ~nConnectionInfo();
    void Clear();
    void Timestep( REAL dt ); 
    void ReliableMessageSent();
    void AckReceived();
    REAL PacketLoss() const;
};

enum nLoginType
{
    Login_Pre0252,  // use the login method known to pre-0.2.5.2 versions
    Login_Post0252, // use the newer login method
    Login_All       // first attempt the new one, then the old one
};

%rename(connect) sn_Connect;
nConnectError sn_Connect(nAddress const & server, nLoginType loginType = Login_All, nSocket const * socket = NULL );

%rename("bend1") sn_Bend( nAddress const & server );
void sn_Bend( nAddress const & server );
%rename("bend2") sn_Bend( tString  const & server, unsigned int port );
void sn_Bend( tString  const & server, unsigned int port );

%rename(my_net_id) sn_myNetID;
extern int sn_myNetID;

%rename(Message) nMessage;
class nMessage;

typedef void nHandler(nMessage &m);

// %import "tLinkedList.i"
// ckass nDescriptor;
// %template(ListItemDescriptor) tListItem<nDescriptor>;
%rename(Descriptor) nDescriptor;
class nDescriptor : public tListItem<nDescriptor>{
    nDescriptor(unsigned short identification,nHandler *handle, const char *name, bool acceptEvenIfNotLoggedIn = false);
    static void HandleMessage(nMessage &message);
    unsigned short ID(){return id;}
};

void RequestInfoHandler(nHandler *handle);


%rename(Message) nMessage;
class nMessage: public tReferencable< nMessage >{
protected:
	unsigned short descriptor;    // the network message type id
    // unsigned short messageID_;    // the network message id, every message gets its own (up to overflow)
    unsigned long messageIDBig_;  // uniquified message ID, the last 16 bits are the same as messageID_
    short          senderID;      // sender's identification
    tArray<unsigned short> data;  // assuming ints are 32 bit wide...

    unsigned int readOut;

    ~nMessage();
    
public:
    nMessage(const nDescriptor &);
    nMessage(unsigned short*& buffer, short sn_myNetID, int lenLeft );

    unsigned short Descriptor() const;
    unsigned short SenderID() const;
    unsigned short MessageID() const;
    unsigned long MessageIDBig() const;
    unsigned short DataLen() const;
    unsigned short Data(unsigned short n);
    void ClearMessageID();
    void SendImmediately(int peer,bool ack=true);
    static void SendCollected(int peer);
    static void BroadcastCollected(int peer, unsigned int port);
    void BroadCast(bool ack=true);
    void Send(int peer,REAL priority=0,bool ack=true);
    void Write(const unsigned short &x);


	// TODO: deal with these pointers and rename the functions
    nMessage& operator<< (const REAL &x);
    nMessage& operator>> (REAL &x);
    
    nMessage& operator<< (const unsigned short &x);
    nMessage& operator>> (unsigned short &x);
    
    nMessage& operator<< (const double &x);
    nMessage& operator>> (double &x);
    nMessage& operator >> (tString &s);
    nMessage& operator >> (tColoredString &s);
    nMessage& operator << (const tString &s);
    nMessage& operator << (const tColoredString &s);
    nMessage& operator << (const tOutput &o);

	nMessage& operator<< (const short &x);
	nMessage& operator>> (short &x);

	nMessage& operator<< (const int &x);
	nMessage& operator>> (int &x);

	nMessage& operator<< (const unsigned int &x);
	nMessage& operator>> (unsigned int &x);

	nMessage& operator<< (const bool &x);
	nMessage& operator>> (bool &x);

// 	
// 	    template<class T> void BinWrite (const T &x);
	bool End();
	void Reset();
	void Read(unsigned short &x);

    // template<class T> void BinRead (const T &x){
    //     for(unsigned int i=0;i<sizeof(T)/2;i++)
    //         Read(reinterpret_cast<unsigned short *>(&x)[i]);
    //     return *this;
    // }
    
    // template <class T> nMessage& operator<<(const tArray<T>& a);
    // template <class T> nMessage& operator>>(tArray<T>& a);
    // template<class T> nMessage& operator << (const T* p);
    // template<class T> nMessage& operator << (const tControlledPTR<T> p);
};
