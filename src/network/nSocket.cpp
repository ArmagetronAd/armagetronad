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

// nSocket.h

// loosely based on the GNU Quake 1 base network code, so technicaly:
// Copyright (C) 1996-1997 Id Software, Inc.
// Modified for Armagetron by Manuel Moos (manuel@moosnet.de)

#include "config.h"
#include "tRandom.h"
#include "tSysTime.h"
#include "tRandom.h"

#include <string>
#include <stdio.h>

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <vector>

#ifndef WIN32
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#else
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <Ws2tcpip.h>
//#include <Wspiapi.h>

#define ioctl(a,b,c) ioctlsocket((a),(b),(u_long *)(c))

#undef EINVAL
#undef EINTR
#define EADDRINUSE WSAEADDRINUSE
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ECONNREFUSED WSAECONNREFUSED
#define ENOTSOCK WSAENOTSOCK
#define ECONNRESET WSAECONNRESET
#define ECONNREFUSED WSAECONNREFUSED
#define ENETDOWN WSAENETDOWN
#ifndef EINTR
#define EINTR WSAEINTR
#endif
#define EINVAL WSAEINVAL
#define EISCONN WSAEISCONN
#define ENETRESET WSAENETRESET
#define EOPNOTSUPP WSAEOPNOTSUPP
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define ESHUTDOWN WSAESHUTDOWN
#define EMSGSIZE WSAEMSGSIZE
#define ETIMEDOUT WSAETIMEDOUT
#define ENETUNREACH WSAENETUNREACH 
#define close closesocket
#define snprintf _snprintf
#endif


#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "nSocket.h"
#include "tConfiguration.h"
#include "tRecorder.h"

// print packet sources and destinations
#ifdef DEBUG
#ifdef DEDICATED
//#define PRINTPACKETS
#endif
#endif

//! Exception to throw when a single socket was opened by request
class OneSocketOpened{};

//! structure defining a socket type
struct nSocketType
{
    int family;   //! socket family
    int type;     //! socket type
    int protocol; //! protocol

    //! default constructor
    nSocketType()
            : family(PF_INET)
            , type(SOCK_DGRAM)
            , protocol(IPPROTO_UDP)
    {}
};

//! all information to reach a host
struct nHostInfo
{
    nSocketType type;     //! type of socket to connect to
    nAddress    address;  //! raw address
    tString     name;     //! canonical hostname
};

// streaming operators for addresses, socket types and host infos
std::istream & operator >> ( std::istream & s, nAddress & address )
{
    // read address
    tLineString line;
    s >> line;
    address.FromString( line );

    return s;
}

std::istream & operator >> ( std::istream & s, nSocketType & type )
{
    // read data
    s >> type.family >> type.type >> type.protocol;

    return s;
}

std::istream & operator >> ( std::istream & s, nHostInfo & hostInfo )
{
    // read name
    tLineString name;
    s >> name;
    hostInfo.name = name;

    // read address and type
    s >> hostInfo.address;
    s >> hostInfo.type;

    return s;
}

std::ostream & operator << ( std::ostream & s, nAddress const & address )
{
    // write address
    s << tLineString( address.ToString() );

    return s;
}

std::ostream & operator << ( std::ostream & s, nSocketType const & type )
{
    // write data
    s << type.family << ' ' << type.type << ' ' << type.protocol;

    return s;
}

std::ostream & operator << ( std::ostream & s, nHostInfo const & hostInfo )
{
    // write name
    s << tLineString( hostInfo.name );

    // write address and type
    s << hostInfo.address;
    s << hostInfo.type;

    return s;
}

typedef std::vector< nHostInfo > nHostList;

enum nSocketError
{
    nSocketError_Ignore,				// nothing special happened
    nSocketError_Reset					// socket needs to be reset
};

tRECORD_AS( nSocketError, int );

// anonymous namespace of helper functions
namespace
{

char *ANET_AddrToString (const struct sockaddr *addr);
int ANET_GetSocketAddr (int sock, struct sockaddr *addr);

#define NET_NAMELEN 100

// typedef int Socket;

static inline void Sys_Error(const char *x){
    con << x;
    exit(-1);
}

static inline void Con_Printf(const char *x){
    con << x;
}

static inline void Con_SafePrintf(const char *x){
    con << x;
}

static inline void Con_DPrintf(const char *x){
    con << x;
}

#ifdef HAVE_SOCKLEN_T
typedef socklen_t NET_SIZE;
#else
typedef int NET_SIZE;
#endif

static nSocketError ANET_Error()
{
    int error = 0;
    nSocketError aError = nSocketError_Ignore;

    // play back error message
    static char const * section = "NETERROR";
    if ( !tRecorder::PlaybackStrict( section, aError ) )
    {
        // get error from OS
#ifdef WIN32
        error = WSAGetLastError();
#else
        error = errno;
#endif
        switch ( error )
        {
        case EADDRINUSE:
            break;
        case ENOTSOCK:
            aError = nSocketError_Reset;
            break;
        case ECONNRESET:
            aError = nSocketError_Reset;
            break;
        case ECONNREFUSED:
            break;
        case EWOULDBLOCK:
        case ENETUNREACH:
            aError = nSocketError_Ignore;
            break;
            //		case NOTINITIALISED:
            //			break;
        case ENETDOWN:
            break;
        case EFAULT:
            aError = nSocketError_Reset;
            break;
        case EINTR:
            break;
        case EINVAL:
            break;
        case EISCONN:
            break;
        case ENETRESET:
            break;
        case EOPNOTSUPP:
        case EAFNOSUPPORT:
        case EADDRNOTAVAIL:
            aError = nSocketError_Reset;
            break;
        case ESHUTDOWN:
            break;
    #ifndef WIN32
        case EMSGSIZE:
            break;
    #endif
        case ETIMEDOUT:
            break;
        default:
    #ifdef DEBUG
            con << "Unknown network error " << error << "\n";
    #endif
            break;
        }
    }

    // record error message
    tRecorder::Record( section, aError );


    //	st_Breakpoint();
    return aError;
}

typedef int Socket;


/*
int ANET_NewSocket ()
{
    int newsocket=-1;
    bool _true = true;

    // create socket
    if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        return -1;

    // unblock it
    if (ioctl (newsocket, FIONBIO, reinterpret_cast<char *>(&_true)) == -1)
        goto ErrorReturn;

    return newsocket;

ErrorReturn:
    close (newsocket);
    return -1;
}

int ANET_OpenSocket (int port )
{
    struct sockaddr_in address;

    // create socket
    int newsocket = ANET_NewSocket();
    if ( newsocket < 0 )
        return -1;

    // bind to no specific IP
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;

    address.sin_port = htons(port);
    if( bind (newsocket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1)
        goto ErrorReturn;

    return newsocket;

ErrorReturn:
    close (newsocket);
    return -1;
}
*/

/*
int ANET_OpenSocket (int port, const tString& ip )
{
    if ( ip.Len() > 1 && ip != "ANY" && ip != "ALL" )
    {
        return ANET_OpenSocket( port, static_cast< const char * >( ip ) );
    }
    else
    {
        return ANET_OpenSocket( port, NULL );
    }
}

int ANET_OpenSocket (int port)
{
    return ANET_OpenSocket( port, net_hostip );
}
*/

//=============================================================================
static bool sn_listen = false;

class nOSNetworking
{
#ifdef WIN32
public:
    nOSNetworking()
    {
        WSADATA		winsockdata;
        WSAStartup( MAKEWORD(1, 1), &winsockdata );
    }

    ~nOSNetworking()
    {
        WSACleanup();
    }
#endif
};

//! initialize OS networking system and automatically shut it down later
static nOSNetworking const & sn_InitOSNetworking()
{
    static nOSNetworking osNetworking;
    return osNetworking;
}

static char const * hostnameSection = "HOSTNAME";

//! determine name of this host
tString const & GetMyHostName()
{
    static tString ret;
    if ( ret.Len() <= 1 )
    {
        // read hostname from recording
        if ( !tRecorder::PlaybackStrict( hostnameSection, ret ) )
        {
            // look up hostname
            char buff[MAXHOSTNAMELEN]="\0";
            if ( 0 == gethostname(buff, MAXHOSTNAMELEN) )
                ret = buff;
            else
                ret = "localhost";
        }

        // write hostname to recording
        tRecorder::Record( hostnameSection, ret );
    }

    return ret;
}

// open ports that listens on the specified address list, returns true only if sockets for all addresses were created
bool ANET_ListenOn( nHostList const & addresses, nSocketListener::SocketArray & sockets, bool onesocketonly, bool ignoreErrors )
{
    bool ret = true;

    for (nHostList::const_iterator iter = addresses.begin(); iter != addresses.end(); ++iter)
    {
        nHostInfo const & address = *iter;

        nSocket socket;
        int error = -1;
        try
        {
            error = socket.Open( address );
        }
        catch ( nSocket::PermanentError & e )
        {
            // ignore errors on request
            if ( ignoreErrors )
            {
                con << "sn_SetNetState: can't setup listening socket. Reason given:\n"
                << e.GetName() << ": " << e.GetDescription() << "\nIgnoring on user request.\n";
                return true;
            }
            else
            {
                throw;
            }
        }

        if ( error == 0 )
        {
            // store the socket
            sockets.push_back( socket );

            // return if we only want one socket
            if ( onesocketonly )
            {
#ifndef NOEXCEPT
                throw OneSocketOpened();
#endif
                return true;
            }
        }
        else
        {
            // report error
            ret = false;
        }
    }

    // return result
    return ret;
}


// determines addresses of hostentry and adds them to hostList
void ANET_GetHostList( hostent const * hostentry, nHostList & hostList, int net_hostport, bool server = true )
{
    // iterate over addresses
    char * * addressList = hostentry->h_addr_list;
    while (*addressList)
    {
        // prepare host info (TODO: IPV6/TCP compatibility)
        nHostInfo info;
        info.type.type = hostentry->h_addrtype;
        info.name      = hostentry->h_name;
        info.address.FromHostent( hostentry->h_length, *addressList );
        info.address.SetPort( net_hostport );

        // add it to list
        hostList.push_back( info );

        ++addressList; // next address
    }
}

// determines addresses of hostname and fills them into to hostList
void ANET_GetHostList( char const * hostname, nHostList & hostList, int net_hostport, bool server = true )
{
    sn_InitOSNetworking();

    hostList.clear();

    // if hostname is NULL, add generic host entry
    if ( !hostname )
    {
        nHostInfo info;
        info.name      = "localhost";
        info.address.SetPort( net_hostport );

        // add it to list and return
        hostList.push_back( info );
        return;
    }

    // read addresses from recording
    static char const * section = "MULTIHOSTBYNAME";
    static char const * sectionEnd = "MULTIHOSTBYNAMEEND";
    nHostInfo read;
    while ( tRecorder::Playback( section, read ) )
    {
        hostList.push_back( read );
    }
    if ( !tRecorder::Playback( sectionEnd ) )
    {
        // look up hostname
        struct hostent *hostentry;
        hostentry = gethostbyname (hostname);
        if (!hostentry)
        {
#ifndef WIN32
            con << "Error looking up " << ( hostname ? hostname : "localhost" ) << " : " << gai_strerror( h_errno ) << "\n";
#endif
            return;
        }

        // delegate
        ANET_GetHostList( hostentry, hostList, net_hostport, server );
    }

    // write addresses to recording
    for ( nHostList::const_iterator iter = hostList.begin(); iter != hostList.end(); ++iter )
        tRecorder::Record( section, read );
    tRecorder::Record( sectionEnd );
}

bool ANET_ListenOnConst( char const * hostname, int net_hostport,  nSocketListener::SocketArray & sockets, bool & onesocketonly, bool & relaxed, bool & ignoreErrors )
{
    if ( hostname )
    {
        // check if only one of the following sockets is required
        if ( 0 == strcmp( hostname, "ONEOF" ) )
        {
            onesocketonly = true;
            return true;
        }

        // check if partial failures are tolerated
        if ( 0 == strcmp( hostname, "RELAXED" ) )
        {
            relaxed = true;
            return true;
        }

        // check if errors should be ignored
        if ( 0 == strcmp( hostname, "IGNOREERRORS" ) )
        {
            ignoreErrors = true;
            return true;
        }

        // replace "ALL" by hostname
        else if ( 0 == strcmp( hostname, "ALL" ) )
        {
            hostname = GetMyHostName();
        }

        // replace "ANY" by 0
        if ( 0 == strcmp( hostname, "ANY" ) )
            hostname = NULL;
    }

    // look up hostname
    nHostList hostList;
    ANET_GetHostList( hostname, hostList, net_hostport );

    if ( hostList.empty() )
    {
        // name lookup failures won't be tolerated here
#ifndef NOEXCEPT
        if ( !ignoreErrors )
        {
            tString details( "Hostname lookup failed for " );
            details += hostname ? hostname : "localhost";
            details += " while trying to open listening sockets.";
            throw nSocket::PermanentError( details );
        }
#endif

        // fallback for nonexceptional systems
        if ( hostname )
            con << "ANET_ListenOn: Could not resolve " << hostname << " to bind socket to its IPs.\n";
        else
            con << "ANET_ListenOn: Could not determine generic IP to bind socket on.\n";

        return ignoreErrors;
    }
    bool ret = ANET_ListenOn( hostList, sockets, onesocketonly, ignoreErrors );

    // in relaxed mode, only one socket needs to be opened ( the caller needs to check that finally ). Otherwise, all socket openings need to have succeeded.
    return relaxed || ret;
}

class nColonRestaurator
{
public:
    nColonRestaurator( char * colon ): colon_(colon){}
    ~nColonRestaurator()
    {
        if ( colon_ )
            *colon_ = ':';
    }

    char * colon_;
};

bool ANET_ListenOn( char * hostname, int net_hostport,  nSocketListener::SocketArray & sockets, bool & onesocketonly, bool & relaxed, bool & ignoreErrors )
{
    // skip whitespace
    while ( isblank( *hostname ) )
        hostname ++;

    // quit silently on empty hostname
    if ( !*hostname )
        return true;

    // replace colon by null temporarily, add port offset to net_hostport
    char * colon = NULL;
    if ( hostname )
        colon = strrchr (hostname, ':');
    if ( colon )
    {
        *colon = 0;
        net_hostport += atoi ( colon + 1 );
    }

    // don't forget to restore the colon when we're done here
    nColonRestaurator restaurator( colon );

    // delegate to function that does not modify the hostname
    return ANET_ListenOnConst( hostname, net_hostport, sockets, onesocketonly, relaxed, ignoreErrors );
}

bool ANET_Listen (bool state, int net_hostport, const tString & net_hostip, nSocketListener::SocketArray & sockets )
{
    sn_listen = state;

    // enable listening
    if (state)
    {
        // reset sockets if we are already listening
        if (!sockets.empty())
            ANET_Listen( false, net_hostport, net_hostip, sockets );

        bool relaxed = false, onesocketonly = false, ignoreErrors = false;

        // first try: generate sockets for explicitly requested IP
        tString ips = net_hostip;   // copy of the hostname string
        int lastpos = 0;            // position where the current IP starts
        for ( int pos = 0; pos < ips.Len(); ++pos )
        {
            if (isblank(ips[pos]))
            {
                ips[pos] = '\0';
                if ( pos > lastpos )
                    if ( !ANET_ListenOn( &ips[lastpos], net_hostport, sockets, onesocketonly, relaxed, ignoreErrors ) )
                        return false;
                ips[pos] = ' ';
                lastpos = pos + 1;
            }
        }
        // don't forget the last IP entry
        if ( ips.Len() > lastpos )
            if ( !ANET_ListenOn( &ips[lastpos], net_hostport, sockets, onesocketonly, relaxed, ignoreErrors ) )
                return false;

        //#else
        //        old fallback code
        //        nSocket socket;
        //        socket.Open( net_hostport );
        //        sockets.push_back( socket );
        //#endif

        // report success or failure ( in relaxed mode when no socket was created )
        return !relaxed || !sockets.empty();
    }

    // disable listening
    if (sockets.empty())
        return false;

    // close sockets
    for ( nSocketListener::SocketArray::iterator iter = sockets.begin(); iter != sockets.end(); ++iter )
    {
        (*iter).Close();
    }
    sockets.clear();

    return true;
}

//=============================================================================

int ANET_CloseSocket (int sock)
{
    //if (sock == net_broadcastsocket)
    //    net_broadcastsocket = 0;
    return close (sock);
}


//=============================================================================
/*
  ============
  PartialIPAddress

  this lets you type only as much of the net address as required, using
  the passed defaults to fill in the rest
  ============
*/
static int PartialIPAddress (const char *in, struct sockaddr *hostaddr, int default_port, int default_addr )
{
    char buff[256];
    char *b;
    int addr;
    int num;
    int mask;
    int run;
    int port;

    buff[0] = '.';
    b = buff;
    strncpy(buff+1, in,254);
    if (buff[1] == '.')
        b++;

    addr = 0;
    mask=-1;
    while (*b == '.')
    {
        b++;
        num = 0;
        run = 0;
        while (!( *b < '0' || *b > '9'))
        {
            num = num*10 + *b++ - '0';
            if (++run > 3)
                return -1;
        }
        if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
            return -1;
        if (num < 0 || num > 255)
            return -1;
        mask<<=8;
        addr = (addr<<8) + num;
    }

    if (*b++ == ':')
        port = atoi(b);
    else
        port = default_port;

    hostaddr->sa_family = AF_INET;
    ((struct sockaddr_in *)hostaddr)->sin_port = htons((short)port);
    ((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (default_addr & htonl(mask)) | htonl(addr);

    return 0;
}

//=============================================================================

char *ANET_AddrToString (const struct sockaddr *addr)
{
    static char buffer[23];
    int haddr;

    haddr = ntohl(((struct sockaddr_in const *)addr)->sin_addr.s_addr);
    snprintf(buffer,22, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, ntohs(((struct sockaddr_in const *)addr)->sin_port));
    return buffer;
}

//=============================================================================

int ANET_GetSocketAddr (int sock, struct sockaddr *addr)
{
    NET_SIZE addrlen = sizeof(struct sockaddr);
    // unsigned int a;

    memset(addr, 0, sizeof(struct sockaddr));
    getsockname(sock, (struct sockaddr *)addr, &addrlen);
    // a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
    //if (a == 0 || a == inet_addr("127.0.0.1"))
    //    ((struct sockaddr_in *)addr)->sin_addr.s_addr = 0x7f000001;

    return 0;
}

//=============================================================================

int ANET_AddrCompare (struct sockaddr *addr1, struct sockaddr *addr2)
{
    if (addr1->sa_family != addr2->sa_family)
        return -1;

    if (((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
        return -1;

    if (((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
        return 1;

    return 0;
}

}   // namespace

//=============================================================================


// *******************************************************************************************
// *
// *	GetListener
// *
// *******************************************************************************************
//!
//!		@return		listening sockets
//!
// *******************************************************************************************

nSocketListener const & nBasicNetworkSystem::GetListener( void ) const
{
    return this->listener_;
}

/*
// *******************************************************************************************
// *	
// *	GetListener
// *	
// *******************************************************************************************
//!		
//!		@param	listener	listening sockets to fill
//!		@return		A reference to this to allow chaining
//!		
// *******************************************************************************************

nBasicNetworkSystem const & nBasicNetworkSystem::GetListener( nSocketListener & listener ) const
{
	listener = this->listener_;
	return *this;
}

// *******************************************************************************************
// *	
// *	SetListener
// *	
// *******************************************************************************************
//!		
//!		@param	listener	listening sockets to set
//!		@return		A reference to this to allow chaining
//!		
// *******************************************************************************************

nBasicNetworkSystem & nBasicNetworkSystem::SetListener( nSocketListener const & listener )
{
	this->listener_ = listener;
	return *this;
}
*/

// *******************************************************************************************
// *
// *	accessListener
// *	This function is dangerous; use only if you absolutely have to and do not store the returned reference longer than required.
// *
// *******************************************************************************************
//!
//!		@return		listening sockets as a modifiable reference
//!
// *******************************************************************************************

nSocketListener & nBasicNetworkSystem::AccessListener( void )
{
    return this->listener_;
}

// *******************************************************************************************
// *
// *	GetControlSocket
// *
// *******************************************************************************************
//!
//!		@return		network control socket
//!
// *******************************************************************************************

nSocket const & nBasicNetworkSystem::GetControlSocket( void ) const
{
    return this->controlSocket_;
}

/*
// *******************************************************************************************
// *	
// *	GetControlSocket
// *	
// *******************************************************************************************
//!		
//!		@param	controlSocket	network control socket to fill
//!		@return		A reference to this to allow chaining
//!		
// *******************************************************************************************

nBasicNetworkSystem const & nBasicNetworkSystem::GetControlSocket( nSocket & controlSocket ) const
{
	controlSocket = this->controlSocket_;
	return *this;
}

// *******************************************************************************************
// *	
// *	SetControlSocket
// *	
// *******************************************************************************************
//!		
//!		@param	controlSocket	network control socket to set
//!		@return		A reference to this to allow chaining
//!		
// *******************************************************************************************

nBasicNetworkSystem & nBasicNetworkSystem::SetControlSocket( nSocket const & controlSocket )
{
	this->controlSocket_ = controlSocket;
	return *this;
}
*/

// *******************************************************************************************
// *
// *	accessControlSocket
// *	This function is dangerous; use only if you absolutely have to and do not store the returned reference longer than required.
// *
// *******************************************************************************************
//!
//!		@return		network control socket as a modifiable reference
//!
// *******************************************************************************************

nSocket & nBasicNetworkSystem::AccessControlSocket( void )
{
    return this->controlSocket_;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *	nAddress
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nAddress::nAddress( void )
{
    // clear address data, it's only POD
    memset( &addr_, 0, size );
    addrLen_ = size;

    // set to unspecific IP4 address
    addr_.addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_.addr_in.sin_family = AF_INET;
    addr_.addr_in.sin_port = 0;
}

// *******************************************************************************************
// *
// *	~nAddress
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nAddress::~nAddress( void )
{
    // nothing to clean up
}

// *******************************************************************************************
// *
// *	ToString
// *
// *******************************************************************************************
//!
//!		@param	string	the string to fill
//!		@return		    reference to this for chaining
//!
// *******************************************************************************************

const nAddress & nAddress::ToString( tString & string ) const
{
    if( addr_.addr_in.sin_addr.s_addr != INADDR_ANY )
        string = ANET_AddrToString( *this );
    else
    {
        string = "*.*.*.*";
        string << ":" << GetPort();
    }
    return *this;
}

// *******************************************************************************************
// *
// *	ToString
// *
// *******************************************************************************************
//!
//!		@return		    the string represendation of the address
//!
// *******************************************************************************************

tString nAddress::ToString( void ) const
{
    // delegate
    tString ret;
    ToString( ret );
    return ret;
}

// *******************************************************************************************
// *
// *	FromString
// *
// *******************************************************************************************
//!
//!		@param	string	the string represendation of the address
//!		@return		    0 on success
//!
// *******************************************************************************************

int nAddress::FromString( const char * string )
{
    int ha1, ha2, ha3, ha4, hp;
    int ipaddr;

    // parse IP address if the passed name looks like one
    if (string[0] >= '0' && string[0] <= '9')
    {
        // parse IP address
        sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
        ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

        // store values in address
        addr_.addr   .sa_family = AF_INET;
        addr_.addr_in.sin_addr.s_addr = htonl(ipaddr);
        addr_.addr_in.sin_port = htons(hp);
        return 0;
    }
    else
    {
        addr_.addr   .sa_family = AF_INET;

        // copy input string
        tString stringCopy ( string );

        // find colon
        char * colon = const_cast< char * >( strrchr (stringCopy, ':') );
        if ( colon )
        {
            *colon = 0;
            // extract port
            SetPort( atoi( colon + 1 ) );

            // extract hostname
            if ( stringCopy == "*.*.*.*" )
                addr_.addr_in.sin_addr.s_addr = INADDR_ANY;
            else
                SetHostname( stringCopy );
            *colon = ':';

            return 0;
        }
        else
        {
            tERR_ERROR( "Invalid string representation ( IP:PORT ): " << string );

            return -1;
        }
    }
}

// *******************************************************************************************
// *
// *	FromAddrInfo
// *
// *******************************************************************************************
//!
//!		@param	info	the addrinfo structure to copy the address from
//!
// *******************************************************************************************

/*
void nAddress::FromAddrInfo( const addrinfo & info )
{
#ifdef HAVE_ADDRINFO
    // check address size
    tASSERT( info.ai_addrlen <= size );

    // copy the address over
    memcpy( &addr_, info.ai_addr, info.ai_addrlen );

    // store length
    addrLen_ = info.ai_addrlen;
#endif
}
*/

// *******************************************************************************************
// *
// *   FromHostent
// *
// *******************************************************************************************
//!
//!        @param  l     length of address
//!        @param  addr  pointer to address
//!
// *******************************************************************************************

void nAddress::FromHostent( int length, const char * addr )
{
    // check address size
    tASSERT( length == 4 );

    // copy the address over
    addr_.addr   .sa_family = AF_INET;
    addr_.addr_in.sin_addr.s_addr = *reinterpret_cast< int const * >( addr );

    // store length
    addrLen_ = sizeof( sockaddr_in );
}

// *******************************************************************************************
// *
// *	GetHostname
// *
// *******************************************************************************************
//!
//!		@param	hostname	the hostname to fill
//!		@return		        reference to this for chaining
//!
// *******************************************************************************************

const nAddress & nAddress::GetHostname( tString & hostname ) const
{
    // initialize networking at OS level
    sn_InitOSNetworking();

    int haddr;
    haddr = ntohl(addr_.addr_in.sin_addr.s_addr);

    hostname.Clear();
    hostname << ((haddr >> 24) & 0xff) << "." << ((haddr >> 16) & 0xff) << "." << ((haddr >> 8) & 0xff) << "." << (haddr & 0xff);

    // read hostname from recording
    static char const * section = "HOSTBYADDR";
    if ( !tRecorder::PlaybackStrict( section, hostname ) )
    {
        struct hostent *hostentry;

        hostentry = gethostbyaddr ( reinterpret_cast< const char * >( &addr_ ), size, AF_INET);
        if (hostentry)
        {
            hostname = tString( (char *)hostentry->h_name );
        }
    }

    // write hostname to recording
    tRecorder::Record( section, hostname );

    return *this;
}

// *******************************************************************************************
// *
// *	GetHostname
// *
// *******************************************************************************************
//!
//!		@return		    the hostname part of the address
//!
// *******************************************************************************************

tString nAddress::GetHostname( void ) const
{
    tString ret;
    GetHostname( ret );
    return ret;
}

// *******************************************************************************************
// *
// *	SetHostname
// *
// *******************************************************************************************
//!
//!		@param	hostname	the hostname to set
//!		@return		        reference to this for chaining
//!
// *******************************************************************************************

nAddress & nAddress::SetHostname( const char * hostname )
{
    // initialize networking at OS level
    sn_InitOSNetworking();

    // parse IP address if the passed name looks like one
    if (hostname[0] >= '0' && hostname[0] <= '9')
    {
        PartialIPAddress (hostname, &addr_.addr, GetPort(), 0x7f000001 );
        return *this;
    }

    // read address from recording
    static char const * section = "SINGLEHOSTNAME";
    if ( !tRecorder::PlaybackStrict( section, *this ) )
    {
        // look up hostname ( TODO: error handling )
        struct hostent *hostentry;
        hostentry = gethostbyname (hostname);
        if (hostentry)
        {
            // store values
            addr_.addr   .sa_family = AF_INET;
            addr_.addr_in.sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];
        }
        else
        {
            // invalidate
            *this = nAddress();
        }
    }

    // write address to recording
    tRecorder::Record( section, *this );

    return *this;
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@param	address 	the raw IP to fill
//!		@return		        reference to this for chaining
//!
// *******************************************************************************************

const nAddress & nAddress::GetAddress( tString & hostname ) const
{
    // initialize networking at OS level
    sn_InitOSNetworking();

    int haddr;
    haddr = ntohl(addr_.addr_in.sin_addr.s_addr);

    hostname << ((haddr >> 24) & 0xff) << "." << ((haddr >> 16) & 0xff) << "." << ((haddr >> 8) & 0xff) << "." << (haddr & 0xff);

    return *this;
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@return		    the raw IP part of the address
//!
// *******************************************************************************************

tString nAddress::GetAddress( void ) const
{
    tString ret;
    GetAddress( ret );
    return ret;
}

// *******************************************************************************************
// *
// *	SetAddress
// *
// *******************************************************************************************
//!
//!		@param	address 	the raw IP to set
//!		@return		        reference to this for chaining
//!
// *******************************************************************************************

nAddress & nAddress::SetAddress( const char * hostname )
{
    // initialize networking at OS level
    sn_InitOSNetworking();

    // parse IP address if the passed name looks like one
    tVERIFY(hostname[0] >= '0' && hostname[0] <= '9')

    PartialIPAddress (hostname, &addr_.addr, GetPort(), 0x7f000001 );
    return *this;
}

// *******************************************************************************************
// *
// *	SetPort
// *
// *******************************************************************************************
//!
//!		@param	port	the port to set
//!		@return		    reference to this for chaining
//!
// *******************************************************************************************

nAddress & nAddress::SetPort( int port )
{
    // store the port
    addr_.addr_in.sin_port = htons(port);

    return *this;
}

// *******************************************************************************************
// *
// *	GetPort
// *
// *******************************************************************************************
//!
//!		@return        port of this address
//!
// *******************************************************************************************

int nAddress::GetPort( void ) const
{
    return ntohs(addr_.addr_in.sin_port);
}

// *******************************************************************************************
// *
// *	GetPort
// *
// *******************************************************************************************
//!
//!		@param	port    port of this address to fill
//!		@return		    reference to this for chaining
//!
// *******************************************************************************************

const nAddress & nAddress::GetPort( int & port ) const
{
    port = GetPort();

    return *this;
}

// *******************************************************************************************
// *
// *	Compare
// *
// *******************************************************************************************
//!
//!		@param	a1  first address to compare
//!		@param	a2  second address to compare
//!		@return
//!
// *******************************************************************************************

int nAddress::Compare( const nAddress & a1, const nAddress & a2 )
{
    if (a1.addr_.addr.sa_family != a2.addr_.addr.sa_family)
        return -1;

    if (a1.addr_.addr_in.sin_addr.s_addr != a2.addr_.addr_in.sin_addr.s_addr)
        return -1;

    if (a1.addr_.addr_in.sin_port != a2.addr_.addr_in.sin_port)
        return 1;

    return 0;
}

// *******************************************************************************************
// *
// *   GetAddressLength
// *
// *******************************************************************************************
//!
//!        @return     the length of the stored address
//!
// *******************************************************************************************

unsigned int nAddress::GetAddressLength( void ) const
{
    return this->addrLen_;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *	nSocket
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocket::nSocket( void )
        :socket_( -1 ), family_( PF_INET ), socktype_( SOCK_DGRAM ), protocol_( IPPROTO_UDP ), broadcast_( false )
{
}

// *******************************************************************************************
// *
// *	~nSocket
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocket::~nSocket( void )
{
    if ( IsOpen() )
        Close();
}

// *******************************************************************************************
// *
// *	Create
// *
// *******************************************************************************************
//!
//!		@return		 0 on success
//!
// *******************************************************************************************

int nSocket::Create( void )
{
    tASSERT( !IsOpen() );

    // initialize networking at OS level
    sn_InitOSNetworking();

    // open new socket
    socket_ = socket( family_, socktype_, protocol_ );
    if ( socket_ < 0 )
        return -1;

    // set TOS to low latency ( see manpages getsockopt(2), ip(7) and socket(7) )
    // maybe this works for Windows, too?
#ifndef WIN32
    char tos = IPTOS_LOWDELAY;

#   ifdef MACOSX
    int ret = setsockopt( socket_, IPPROTO_IP, IP_TOS, &tos, sizeof(char) );
#   else
    int ret = setsockopt( socket_, SOL_IP, IP_TOS, &tos, sizeof(char) );
#   endif

    // remove this error reporting some time later, the success is not critical
    if ( ret != 0 )
    {
        static bool warn=true;
        if ( warn )
            con << "Setting TOS to LOWDELAY failed.\n";
        warn=false;
    }
#endif    

    // unblock it
    bool _true = true;
    return ioctl (socket_, FIONBIO, reinterpret_cast<char *>(&_true)) == -1;
}

// archives the binding procedure
static char const * recordingSection = "BIND";

template< class Archiver > class BindArchiver
{
public:
    static void Archive( Archiver & archiver, nAddress & trueAddress  )
    {
        // byte-archive the address
        struct sockaddr * addr = static_cast< struct sockaddr * >( trueAddress );
        unsigned char * addr_char = reinterpret_cast< unsigned char *>( addr );
        for( int i = sizeof( sockaddr )-1; i>=0; --i )
            archiver.Archive( addr_char[i] ).DontSeparate();

        archiver.Separator();
    }

    static bool Archive( int & ret, nAddress & trueAddress )
    {
        // start archive block if archiving is active
        Archiver archive;
        if ( archive.Initialize( recordingSection ) )
        {
            archive.Archive( ret );
            archive.Archive( trueAddress );
            //Archive( archive, trueAddress );

            return true;
        }

        return false;
    }
};

// *******************************************************************************************
// *
// *	Bind
// *
// *******************************************************************************************
//!
//!     @param      addr address to bind to
//!		@return		0 on success
//!
// *******************************************************************************************

int nSocket::Bind( nAddress const & addr )
{
    tASSERT( IsOpen() );

    // copy address
    address_ = addr;

    int ret = 0;

    // see if the process was archived; if yes, return without action
    if ( !BindArchiver< tPlaybackBlock >::Archive( ret, trueAddress_ ) )
    {
        // just delegate
        ret = bind( socket_, addr, addr.GetAddressLength() );

        // read true address
        if ( 0 == ret )
            ANET_GetSocketAddr( socket_, trueAddress_ );
    }

    // record the bind
    BindArchiver< tRecordingBlock >::Archive( ret, trueAddress_ );

    if ( 0 == ret )
    {
        // report success
#ifdef DEDICATED
        static nAddress nullAddress;
        if ( 0 == nAddress::Compare( trueAddress_, addr ) || 0 == nAddress::Compare( nullAddress, addr ) )
            con << "Bound socket to " << trueAddress_.ToString() << ".\n";
        else
            con << "Bound socket to " << trueAddress_.ToString() << " ( " << address_.ToString() << " was requested ).\n";
#endif

        return 0;
    }
    else
    {
        // con << "nSocket::Open: Failed to bind socket to " << addr.ToString() << ".\n";

        // close the socket and report an error
        ANET_CloseSocket( socket_ );
        socket_ = -1;

        // throw exception on fatal error
#ifndef NOEXCEPT
        // name lookup failures won't be tolerated here
        if ( ANET_Error() == nSocketError_Reset )
        {
            tString details( "Unable to bind to " );
            details += address_.ToString();
            details += " because that is not an address of the local machine.";
            throw nSocket::PermanentError( details );
        }
#endif

        return -1;
    }

    return ret;
}

// *******************************************************************************************
// *
// *	Open
// *
// *******************************************************************************************
//!
//!     @param      addr address to bind to
//!		@return		0 on success
//!
// *******************************************************************************************

int nSocket::Open( nAddress const & addr )
{
    tASSERT( !IsOpen() );

    // create
    if ( Create() != 0 )
    {
        return -2;
    }

    // bind
    return Bind( addr );
}

// *******************************************************************************************
// *
// *	Open
// *
// *******************************************************************************************
//!
//!		@return		0 on success
//!
// *******************************************************************************************

int nSocket::Open( void )
{
    // bind to unspecific IP
    nAddress addr;
    return Open( addr );
}

// *******************************************************************************************
// *
// *	Open
// *
// *******************************************************************************************
//!
//!		@param	port	port to bind to
//!		@return		    0 on success
//!
// *******************************************************************************************

int nSocket::Open( int port )
{
    // bind to unspecific IP, but specific port
    nAddress addr;
    addr.SetPort( port );
    return Open( addr );
}

// *******************************************************************************************
// *
// *	Open
// *
// *******************************************************************************************
//!
//!		@param	addr	address to bind to
//!		@return		    0 on success
//!
// *******************************************************************************************

/*
int nSocket::Open( const addrinfo & addr )
{
#ifdef HAVE_ADDRINFO
    tASSERT( !IsOpen() );

    // copy socket type data
    family_ = addr.ai_family;
    socktype_ = addr.ai_socktype;
    protocol_ = addr.ai_protocol;

    // open
    nAddress address;
    address.FromAddrInfo( addr );
    return Open( address );
#endif
    return -1;
}
*/

// *******************************************************************************************
// *
// *   Open
// *
// *******************************************************************************************
//!
//!        @param  addr    host info to take opening information from
//!        @return         0 on success
//!
// *******************************************************************************************

int nSocket::Open( const nHostInfo & addr )
{
    tASSERT( !IsOpen() );

    family_ = addr.type.family;
    socktype_ = addr.type.type;
    protocol_ = addr.type.protocol;

    return Open( addr.address );
}

// *******************************************************************************************
// *
// *	Close
// *
// *******************************************************************************************
//!
//!		@return		0 on success
//!
// *******************************************************************************************

int nSocket::Close( void )
{
#ifdef DEDICATED
    if ( socket_ != -1 )
        con << "Closing socket bound to " << trueAddress_.ToString() << "\n";
#endif

    ANET_CloseSocket( socket_ );
    socket_ = -1;
    broadcast_ = false;

    return 0;
}

// *******************************************************************************************
// *
// *	Reset
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

void nSocket::Reset( void ) const
{
    // logically, resetting is const. Physically not. const_cast is required.
    nSocket * reset = const_cast< nSocket * >( this );

    // remember the true address bound to
    nAddress trueAddress = trueAddress_;

    // try to reset a socket without disturbing the rest of the system
    reset->Close();

    // store old address
    nAddress oldAddress = address_;

    // first try: reopen with the same port as last time
    nAddress wishAddress = address_;
    wishAddress.SetPort( trueAddress.GetPort() );
    if ( 0 == reset->Open( wishAddress ) )
        return;

    // second try: reopen with the same premises
    if ( 0 == reset->Open( oldAddress ) )
        return;

    // last try: reopen with the last true address ( this is most probably wrong )
    reset->Open( trueAddress );
}

// *******************************************************************************************
// *
// *   IsOpen
// *
// *******************************************************************************************
//!
//!        @return    true if the socket is open, false otherwise
//!
// *******************************************************************************************

bool nSocket::IsOpen( void ) const
{
    return socket_ >= 0;
}

// *******************************************************************************************
// *
// *	Connect
// *
// *******************************************************************************************
//!
//!		@param	addr
//!		@return
//!
// *******************************************************************************************

int nSocket::Connect( const nAddress & addr )
{
    // not implemented, does not make sense here
    return 0;
}

#ifdef DEBUG
// reset sockets just for fun
bool sn_ResetSocket = false;
void ResetSocket( std::istream& s )
{
    sn_ResetSocket = true;
}

static tConfItemFunc sn_cireset ( "RESET_SOCKET", ResetSocket );
#endif


// static char const * recordingSectionConnect = "CONNECT";

// *******************************************************************************************
// *
// *	CheckNewConnection
// *
// *******************************************************************************************
//!
//!		@return		   the new socket if a connection is there or NULL
//!
// *******************************************************************************************

const nSocket * nSocket::CheckNewConnection( void ) const
{
    tASSERT( IsOpen() );

    int	available=-1;

    // see if the playback has anything to say
    //if ( tRecorder::Playback( recordingSectionConnect, available ) )
    //    return this;

    // always return this when recoring or playback are running
    if ( tRecorder::IsRunning() )
        return this;


#ifdef DEBUG
    if ( sn_ResetSocket )
    {
        sn_ResetSocket = false;
        Reset();
    }
#endif

    //    for ( SocketArray::iterator iter = sockets.begin(); iter != sockets.end(); ++iter )
    int ret = ioctl (socket_, FIONREAD, &available);
    if ( ret == -1)
    {
        switch ( ANET_Error() )
        {
        case nSocketError_Reset:
            Reset();
            break;
        case nSocketError_Ignore:
            break;
        default:
            Sys_Error ("UDP: ioctlsocket (FIONREAD) failed\n");
            break;
        }
    }

    if ( ret >= 0 )
    {
        // record result
        // tRecorder::Record( recordingSectionConnect, available );
        return this;
    }

    return NULL;
}


static char const * recordingSectionRead = "READ";

//! Read or write network read data
template< class Archiver > class ReadArchiver
{
public:
    static bool Archive( int8 * buf, int & len, nAddress & addr )
    {
        // start archive block if archiving is active
        Archiver archive;
        if( archive.Initialize( recordingSectionRead ) )
        {
            // archive length of message
            archive.Archive( len );
            if ( len < 0 )
                return true;

            // archive source address
            // BindArchiver< Archiver >::Archive( archive, addr );
            archive.Archive( addr );

            // archive data
            for( int i = 0; i < len; ++i )
                archive.Archive( buf[ i ] );

            archive.Separator();

            return true;
        }

        return false;
    }
};

#ifdef DEBUG
static REAL sn_simulateReceivePacketLoss = 0;
static tSettingItem<REAL> sn_sumulateReceivePacketLossConfig( "SIMULATE_RECEIVE_PACKET_LOSS", sn_simulateReceivePacketLoss );
#endif

// *******************************************************************************************
// *
// *	Read
// *
// *******************************************************************************************
//!
//!		@param	buf	    buffer to read to
//!		@param	len	    maximum number of bytes to read
//!		@param	addr	the address where the data came from is stored here
//!		@return		    number of bytes truly read or something negative on failure
//!
// *******************************************************************************************

int nSocket::Read( int8 * buf, int len, nAddress & addr ) const
{
    tASSERT( IsOpen() );

    // return value: real number of bytes read
    int ret = 0;

    static tReproducibleRandomizer randomizer;
#ifdef DEBUG
    // pretend nothing was received
    if ( sn_simulateReceivePacketLoss > randomizer.Get() )
        return -1;
#endif

    // check playback
    if ( !ReadArchiver< tPlaybackBlock >::Archive( buf, ret, addr ) )
    {

#ifdef DEBUG
        if ( sn_ResetSocket )
        {
            sn_ResetSocket = false;
            Reset();
        }
#endif

        // really receive
        NET_SIZE addrlen = addr.GetAddressLength();
        ret = recvfrom (socket_, buf, len, 0, addr, &addrlen );
        tASSERT( addrlen <= static_cast< NET_SIZE >( addr.GetAddressLength() ) );
    }

    // write recording
    ReadArchiver< tRecordingBlock >::Archive( buf, ret, addr );

    if ( ret <= 0 )
    {
        switch ( ANET_Error() )
        {
        case nSocketError_Reset:
            {
                Reset();
                return -1;
            }
            break;
        case nSocketError_Ignore:
            return -1;
            break;
        }
    }

    if ( ret >= 0 )
    {
        // con << "Read " << ret << " bytes from socket " << GetSocket() << ".\n";
    }

#ifdef PRINTPACKETS
    con << trueAddress_.ToString() << " << " << addr.ToString() << "\n";
#endif

    return ret;
}

#ifdef DEBUG
static REAL sn_simulateSendPacketLoss = 0;
static tSettingItem<REAL> sn_sumulateSendPacketLossConfig( "SIMULATE_SEND_PACKET_LOSS", sn_simulateSendPacketLoss );
#endif

// *******************************************************************************************
// *
// *	Write
// *
// *******************************************************************************************
//!
//!		@param	buf	    pointer to data to send
//!		@param	len	    length of data to send
//!		@param	addr	address to send data to
//!		@param	addrlen	length of address data structure
//!		@return		    the number of bytes sent or something negative on error
//!
// *******************************************************************************************

int nSocket::Write( const int8 * buf, int len, const sockaddr * addr, int addrlen ) const
{
    tASSERT( IsOpen() );

#ifdef DEBUG
    if ( sn_ResetSocket )
    {
        sn_ResetSocket = false;
        Reset();
    }
#endif

    int ret = 0;

    // check if return value was archived
    static char const * section = "SEND";
    if ( !tRecorder::Playback( section, ret ) )
    {
        static tReproducibleRandomizer randomizer;
#ifdef DEBUG
        // pretend send was successful in packet loss simulation
        if ( sn_simulateSendPacketLoss > randomizer.Get() )
            ret = len;
        else
#endif
        {
            // don't send if a playback is running
            if ( !tRecorder::IsPlayingBack() )
                ret = sendto (socket_, buf, len, 0, addr, addrlen );
        }
    }

    if ( ret < 0 )
    {
        // log error
        tRecorder::Record( section, ret );

        // handle error
        switch ( ANET_Error() )
        {
        case nSocketError_Reset:
            {
                Reset();
                return -1;
            }
            break;
        case nSocketError_Ignore:
            return -1;
            break;
        }
    }

    return ret;
}

// *******************************************************************************************
// *
// *	Write
// *
// *******************************************************************************************
//!
//!		@param	buf	    pointer to data to send
//!		@param	len	    length of data to send
//!		@param	addr	address to send data to
//!		@return		    the number of bytes sent or something negative on error
//!
// *******************************************************************************************

int nSocket::Write( const int8 * buf, int len, const nAddress & addr ) const
{
#ifdef PRINTPACKETS
    con << trueAddress_.ToString() << " >> " << addr.ToString() << "\n";
#endif

    // delegate to low level write
    return Write( buf, len, addr, addr.GetAddressLength() );
}

// *******************************************************************************************
// *
// *	Broadcast
// *
// *******************************************************************************************
//!
//!		@param	buf	        pointer to data to send
//!		@param	len	        length of data to send
//!		@param	port        port to broadcast on
//!		@return		        number of bytes truly send or something negative on error
//!
// *******************************************************************************************

int nSocket::Broadcast( const char * buf, int len, unsigned int port ) const
{
    tASSERT( IsOpen() );

    if ( !broadcast_ )
    {
        int				i = 1;

        // make this socket broadcast capable
        if (setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
        {
            Con_Printf("Unable to make socket broadcast capable\n");
            return -1;
        }

        broadcast_ = true;
    }

    // prepare broadcasting address
    sockaddr_in broadcastaddr;
    broadcastaddr.sin_family = AF_INET;
    broadcastaddr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcastaddr.sin_port = htons(port);

    // delegate to usual write function
    return Write ( buf, len, reinterpret_cast< sockaddr *>( &broadcastaddr ), sizeof( sockaddr_in ) );
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@return		the address the socket is bound to
//!
// *******************************************************************************************

const nAddress & nSocket::GetAddress( void ) const
{
    return trueAddress_;
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@param	    address the address that the socket is bound to to fill
//!		@return		reference to this for chaining
//!
// *******************************************************************************************

const nSocket & nSocket::GetAddress( nAddress & address ) const
{
    address = trueAddress_;

    return *this;
}

// *******************************************************************************************
// *
// *	SetAddress
// *
// *******************************************************************************************
//!
//!		@param	address	address the address that the socket should be bound to
//!		@return
//!
// *******************************************************************************************

nSocket & nSocket::SetAddress( nAddress const & address )
{
    address_ = address;
    trueAddress_ = address;

    return *this;
}


// *******************************************************************************************
// *
// *   MoveFrom
// *
// *******************************************************************************************
//!
//!        @param  other   socket to move data from
//!
// *******************************************************************************************

void nSocket::MoveFrom( const nSocket & other )
{
    // close this socket
    if ( IsOpen() )
        Close();


    // copy uncritical data
    address_ = other.address_;
    trueAddress_ = other.trueAddress_;
    family_ = other.family_;
    socktype_ = other.socktype_;
    protocol_ = other.protocol_;
    broadcast_ = other.broadcast_;

    // move socket
    socket_ = other.socket_;
    const_cast< nSocket & >( other ).socket_ = -1;
}

// *******************************************************************************************
// *
// *   nSocket
// *
// *******************************************************************************************
//!
//!        @param  other   socket to move data from
//!
// *******************************************************************************************

nSocket::nSocket( const nSocket & other )
{
    socket_ = -1;
    MoveFrom( other );
}

// *******************************************************************************************
// *
// *   operator =
// *
// *******************************************************************************************
//!
//!        @param  other   socket to move data from
//!        @return          reference to this
//!
// *******************************************************************************************

nSocket & nSocket::operator =( const nSocket & other )
{
    MoveFrom( other );
    return *this;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *	nSocketListener
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocketListener::nSocketListener( void )
        : port_( 0 ), ipList_( "ANY" )
{
}

// *******************************************************************************************
// *
// *	~nSocketListener
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocketListener::~nSocketListener( void )
{
    // not really required since the sockets close themselves automatically, but stop listening anyway
    Listen( false );
}

// *******************************************************************************************
// *
// *	Listen
// *
// *******************************************************************************************
//!
//!		@param	state   listening state ( on or off )
//!		@return		    true on success
//!
// *******************************************************************************************

bool nSocketListener::Listen( bool state )
{
    bool ret = false;
    try{
        // delegate to helper function
        ret = ANET_Listen( state, port_, ipList_, sockets_ );
    }
    catch ( OneSocketOpened const & )
    {
        // only one socket was upened on user request; this counts as succes
        ret = true;
    }

    // close sockets if opening failed
    if ( state && !ret )
        Listen( false );

    // return result
    return ret;
}

// *******************************************************************************************
// *
// *	begin
// *
// *******************************************************************************************
//!
//!		@return		iterator to the beginning of the sockets
//!
// *******************************************************************************************

nSocketListener::iterator nSocketListener::begin( void ) const
{
    return sockets_.begin();
}

// *******************************************************************************************
// *
// *	end
// *
// *******************************************************************************************
//!
//!		@return		iterator past the end of the sockets
//!
// *******************************************************************************************

nSocketListener::iterator nSocketListener::end( void ) const
{
    return sockets_.end();
}

// *******************************************************************************************
// *
// *   GetPort
// *
// *******************************************************************************************
//!
//!        @return     the network port to listen on
//!
// *******************************************************************************************

unsigned int nSocketListener::GetPort( void ) const
{
    return this->port_;
}

// *******************************************************************************************
// *
// *   GetPort
// *
// *******************************************************************************************
//!
//!        @param  port    the network port to listen on to fill
//!       @return     A reference to this to allow chaining
//!
// *******************************************************************************************

nSocketListener const & nSocketListener::GetPort( unsigned int & port ) const
{
    port = this->port_;
    return *this;
}

// *******************************************************************************************
// *
// *   SetPort
// *
// *******************************************************************************************
//!
//!        @param  port    the network port to listen on to set
//!        @return     A reference to this to allow chaining
//!
// *******************************************************************************************

nSocketListener & nSocketListener::SetPort( unsigned int port )
{
    Listen( false );

    this->port_ = port;
    return *this;
}

// *******************************************************************************************
// *
// *   GetIpList
// *
// *******************************************************************************************
//!
//!        @return     list of IPs to bind to
//!
// *******************************************************************************************

tString const & nSocketListener::GetIpList( void ) const
{
    return this->ipList_;
}

// *******************************************************************************************
// *
// *   GetIpList
// *
// *******************************************************************************************
//!
//!        @param  ipList  list of IPs to bind to to fill
//!      @return     A reference to this to allow chaining
//!
// *******************************************************************************************

nSocketListener const & nSocketListener::GetIpList( tString & ipList ) const
{
    ipList = this->ipList_;
    return *this;
}

// *******************************************************************************************
// *
// *   SetIpList
// *
// *******************************************************************************************
//!
//!        @param  ipList  list of IPs to bind to to set
//!       @return     A reference to this to allow chaining
//!
// *******************************************************************************************

nSocketListener & nSocketListener::SetIpList( tString const & ipList )
{
    Listen( false );

    this->ipList_ = ipList;
    return *this;
}

// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************
// *******************************************************************************************

// *******************************************************************************************
// *
// *	nBasicNetworkSystem
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nBasicNetworkSystem::nBasicNetworkSystem( void )
{
    // nothing to do
}

// *******************************************************************************************
// *
// *	~nBasicNetworkSystem
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nBasicNetworkSystem::~nBasicNetworkSystem( void )
{
    // shut down the system
    Shutdown();
}

// *******************************************************************************************
// *
// *	Init
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

nSocket * nBasicNetworkSystem::Init()
{
    // test if network was already initialized
    if ( controlSocket_.IsOpen() )
        return &controlSocket_;

    // initialize networking at OS level
    sn_InitOSNetworking();

    if ( 0 != controlSocket_.Open() )
        Sys_Error("ANET_Init: Unable to open control socket\n");

    return &controlSocket_;

    //struct hostent *local;
    //char	buff[MAXHOSTNAMELEN]="\0";
    //struct sockaddr addr;
    //char *colon;

    /* not for armagetron
       if (COM_CheckParm ("-noudp"))
       return -1;
    */

    // determine my name & address
    //int myAddr = 0;
    //int hostnameres = gethostname(buff, MAXHOSTNAMELEN);
    //if ( 0 == hostnameres )
    //{
    //   local = gethostbyname(buff);
    //   if ( local )
    //   {
    //        myAddr = *reinterpret_cast<int *>(local->h_addr_list[0]);
    //   }
    //   else
    //    {
    //        Con_Printf ("ANET_Init: Unable to determine IP adress.\n");
    //    }
    //}
    //else
    //{
    //    Con_Printf ("ANET_Init: Unable to determine hostname.\n");
    // }

    // fallback: use loopback
    //if ( myAddr == 0 )
    //{
    //    myAddr = inet_addr("127.0.0.1");
    //}

    //tString hostname;

    // if the armagetron hostname isn't set, set it to the clamped machine name
    //if (strcmp(hostname, "UNNAMED") == 0 && buff[0] && hostnameres == 0 )
    //{
    //    buff[15] = 0;
    //    hostname=buff;
    //}

    //ANET_GetSocketAddr (controlSocket_, &addr);
    //my_tcpip_address=ANET_AddrToString (&addr);
    //colon = strrchr (my_tcpip_address, ':');
    //if (colon)
    //    *colon = 0;

    //  Con_Printf("UDP Initialized\n");
    //tcpipAvailable = true;

}

// *******************************************************************************
// *
// *	Select
// *
// *******************************************************************************
//!
//!		@param	dt	the time in seconds to wait at max
//!		@return		true if data came in
//!
// *******************************************************************************

bool nBasicNetworkSystem::Select( REAL dt )
{
    int retval = 0;
    static char const * section = "NETSELECT";
    if ( !tRecorder::PlaybackStrict( section, retval ) )
    {
        if ( controlSocket_.GetSocket() < 0 )
        {
            tDelay( int( dt * 1000000 ) );
            return false;
        }

        fd_set rfds; // set of sockets to wathc
        struct timeval tv; // time value to pass to select()

        FD_ZERO( &rfds );

        // watch the control socket
        FD_SET( controlSocket_.GetSocket(), &rfds );
        // con << "Watching " << controlSocket_.GetSocket();

        int max = controlSocket_.GetSocket();

        // watch listening sockets
        for( nSocketListener::SocketArray::const_iterator iter = listener_.GetSockets().begin(); iter != listener_.GetSockets().end(); ++iter )
        {
            FD_SET( (*iter).GetSocket(), &rfds );
            if ( (*iter).GetSocket() > max )
                max = (*iter).GetSocket();
            // con << ", " << (*iter).GetSocket();
        }

        // set time
        tv.tv_sec  = static_cast< long int >( dt );
        tv.tv_usec = static_cast< long int >( (dt-tv.tv_sec)*1000000 );

        // delegate to system select
        retval = select(max+1, &rfds, NULL, NULL, &tv);
    }
    tRecorder::Record( section, retval );

    // con << " : " << retval << "\n";

    // return result
    return ( retval > 0 );
}

// *******************************************************************************************
// *
// *	Shutdown
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void nBasicNetworkSystem::Shutdown()
{
    // stop listening
    listener_.Listen(false);

    // close the control socket
    controlSocket_.Close();
}


// *******************************************************************************************
// *
// *	PermanentError
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocket::PermanentError::PermanentError( void )
        : description_( "No details available" )
{
}

// *******************************************************************************************
// *
// *	PermanentError
// *
// *******************************************************************************************
//!
//!		@param	details
//!
// *******************************************************************************************

nSocket::PermanentError::PermanentError( const tString & details )
        : description_( details )
{
}

// *******************************************************************************************
// *
// *	~PermanentError
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nSocket::PermanentError::~PermanentError( void )
{
}

// *******************************************************************************************
// *
// *	DoGetName
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

tString nSocket::PermanentError::DoGetName( void ) const
{
    return tString( "Permanent network error" );
}

// *******************************************************************************************
// *
// *	DoGetDescription
// *
// *******************************************************************************************
//!
//!		@return
//!
// *******************************************************************************************

tString nSocket::PermanentError::DoGetDescription( void ) const
{
    return description_;
}


