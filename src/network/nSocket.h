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

// extremely losely based on the GNU Quake 1 base network code, so technicaly:
// Copyright (C) 1996-1997 Id Software, Inc.
// Modified for Armagetron by Manuel Moos (manuel@moosnet.de)

#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#include <iostream>
#include "tConsole.h"
#include "tException.h"
#include <vector>

class nScoket;
struct nHostInfo;
// struct addrinfo;
struct hostentry;
struct sockaddr;

#ifdef WIN32
#include  <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

typedef  char int8;

//! union of supported internet addresses
union nAddressBase
{
    struct sockaddr     addr;       //!< generic address
    struct sockaddr_in  addr_in;    //!< IPV4 address
    // struct sockaddr_in6 addr_in6;   //!< IPV6 address ( not supported yet )
};

//! internet address
class nAddress
{
public:
    nAddress();         //!< constructor
    ~nAddress();        //!< destructor

    // these functions handle address representation in the form hostname:port
    nAddress const & 	ToString    ( tString & string ) const   ; //!< turns address to complete string
    tString 			ToString    ()                   const   ; //!< turns address to complete string
    int    			 	FromString  ( const char * string )      ; //!< turns complete string into address

    //  void                FromAddrInfo( const addrinfo & info )    ; //!< copy address from addrinfo

    void                FromHostent ( int l, const char * addr ) ; //!< copy address from hostent data

    nAddress & 			SetHostname	( const char * hostname )    ; //!< Sets the hostname part of the address (with DNS lookup)
    tString 			GetHostname	( void ) const               ; //!< Gets the hostname part of the address (with DNS lookup)
    nAddress const & 	GetHostname	( tString & hostname ) const ; //!< Gets the hostname part of the address (with DNS lookup)

    nAddress & 			SetAddress	( const char * address )     ; //!< Sets the hostname part of the address
    tString 			GetAddress	( void ) const               ; //!< Gets the hostname part of the address
    nAddress const & 	GetAddress	( tString & address ) const  ; //!< Gets the hostname part of the address

    nAddress & 			SetPort		( int port )                 ; //!< Sets the port of the address
    int 				GetPort		( void ) const               ; //!< Gets the port of the address
    nAddress const & 	GetPort		( int & port ) const         ; //!< Gets the port of the address

    static int 	Compare ( const nAddress & a1, const nAddress & a2 );	//!< compares two addresses

    operator struct sockaddr *      ()       { return &addr_.addr; }   //!< conversion to sockaddr
    operator struct sockaddr const *() const { return &addr_.addr; }   //!< conversion to sockaddr

    enum{ size = sizeof( nAddressBase ) };

    unsigned int GetAddressLength( void ) const;	//!< Gets the length of the stored address
private:
    nAddressBase addr_;	    //!< the lowlevel network address
    unsigned int addrLen_;  //!< the length of the really stored address
};

//! wrapper for low level sockets.
class nSocket
{
public:
    nSocket();          //!< constructor
    ~nSocket();         //!< destructor

    //! exception thrown on probably unrecoverable error
class PermanentError: public tException
    {
    public:
        PermanentError();                                       //!< default constructor
        PermanentError( const tString & details );              //!< constructor giving details on error
        ~PermanentError();                                      //!< destructor
    private:
        virtual tString DoGetName()         const;              //!< returns the name of the exception
        virtual tString DoGetDescription()  const;              //!< returns a detailed description

        tString description_;
    };

    int Open ();                            //!< open socket for sending data ( not bound to a specific IP or port )
    int Open ( nAddress const & address );  //!< open socket for listening on the specified address info
    int Open ( int port );                  //!< open socket for listening bound to the specified port
    //  int Open ( const addrinfo & addr );     //!< open socket for listening on the specified address info
    int Open ( const nHostInfo & addr );    //!< open socket for listening on the specified host info
    int Close ();                           //!< close the socket
    bool IsOpen () const;                   //!< returns whether the socket is open
    void Reset () const;                    //!< resets ( closes and reopens ) the socket

    int Connect ( const nAddress & addr );          //!< connects the socket to the specified address
    const nSocket * CheckNewConnection() const;     //!< listens for new data

    int Read        ( int8 *buf, int len, nAddress & addr )             const; //!< reads data from the socket
    int Write       ( const int8 *buf, int len, const nAddress & addr ) const; //!< writes data to the socket
    int Broadcast   ( const char *buf, int len, unsigned int port )     const; //!< broadcasts data over the socket to the LAN

    nAddress const &   GetAddress( void )                  const   ;	//!< Gets the address the socket is bound to
    nSocket const &    GetAddress( nAddress & address )    const   ;	//!< Gets the address the socket is bound to

    inline int             GetSocket( void )         const; //!< Gets the raw socket
    inline nSocket const & GetSocket( int & socket ) const;	//!< Gets the raw socket

    // moving copy semantics
    void MoveFrom( const nSocket & other );       //!< move data from other to this
    nSocket( const nSocket & other );             //!< copy constructor. Warning: uses data move semantics
    nSocket & operator=( const nSocket & other ); //!< copy operator. Warning: uses data move semantics
private:
    int Create ();                            //!< creates the socket without binding it
    int Bind ( nAddress const & address );    //!< binds the socket to a specific address

    int Write       ( const int8 *buf, int len, const sockaddr * addr, int addrlen ) const; //!< writes data to the socket

    nSocket & SetAddress( nAddress const & address );  //!< Sets the address the socket is bound to
    inline nSocket & SetSocket( int socket );	       //!< Sets the raw socket

    int socket_;            //!< the raw socket
    nAddress address_;      //!< the address the socket is bound to
    nAddress trueAddress_;  //!< the address the socket is really bound to
    int family_, socktype_, protocol_;    //!< more low level data determining the socket type
    mutable bool broadcast_;              //!< flag indicating whether this socket has been prepared for broadcasts
};

//! collection of listening server sockets
class nSocketListener
{
public:
    nSocketListener();      //!< constructor
    ~nSocketListener();     //!< destructor

    bool Listen ( bool state );                                 //!< enables/disables listening state

    typedef std::vector< nSocket > SocketArray;                 //!< array of sockets
    typedef SocketArray::const_iterator const_iterator;         //!< iterator in that array
    typedef SocketArray::const_iterator iterator;               //!< iterator in that array

    // almost std:: compliant iterator functions
    iterator begin() const;                                    //!< returns iterator to first socket
    iterator end() const;                                      //!< returns iterator to end of sockets

    nSocketListener &       SetPort     ( unsigned int port );	        //!<  Sets the network port to listen on
    unsigned int            GetPort     ( void ) const;	                //!< Gets the network port to listen on
    nSocketListener const & GetPort     ( unsigned int & port ) const;	//!< Gets the network port to listen on
    nSocketListener &       SetIpList   ( tString const & ipList );	    //!< Sets list of IPs to bind to
    tString const &         GetIpList   ( void ) const;	                //!< Gets list of IPs to bind to
    nSocketListener const & GetIpList   ( tString & ipList ) const;	    //!< Gets list of IPs to bind to
    inline SocketArray const     & GetSockets ( void ) const;	                //!< Gets the listening sockets
    inline nSocketListener const & GetSockets ( SocketArray & sockets ) const;	//!< Gets the listening sockets
private:
    SocketArray  sockets_;   //!< the listening sockets
    unsigned int port_;      //!< the network port to listen on
    tString      ipList_;    //!< list of IPs to bind to

    // forbid copying
    nSocketListener( const nSocketListener & );
    nSocketListener & operator=( const nSocketListener & );

    inline nSocketListener & SetSockets( SocketArray const & sockets );	//!< Sets the listening sockets
};

//! basic network system: manages sockets
class nBasicNetworkSystem
{
public:
    nBasicNetworkSystem();              //!< constructor
    ~nBasicNetworkSystem();             //!< destructor

    nSocket*  Init (void);              //!< initializes the network
    void      Shutdown (void);          //!< shuts donw the network

    bool      Select (REAL dt);         //!< waits the specified time for data to arrive on the sockets

    nSocketListener &             AccessListener      ( void )                                ;	//!< Accesses listening sockets
    nBasicNetworkSystem &         SetListener         ( nSocketListener const & listener )    ;	//!< Sets listening sockets
    nSocketListener const &       GetListener         ( void ) const                          ;	//!< Gets listening sockets
    // nBasicNetworkSystem const &   GetListener         ( nSocketListener & listener ) const    ;	//!< Gets listening sockets

    // nBasicNetworkSystem &         SetControlSocket    ( nSocket const & controlSocket )       ;	//!< Sets network control socket
    nSocket const &               GetControlSocket    ( void ) const                          ;	//!< Gets network control socket
    // nBasicNetworkSystem const &   GetControlSocket    ( nSocket & controlSocket ) const       ;	//!< Gets network control socket
private:

    nSocketListener listener_;          //!< listening sockets
    nSocket         controlSocket_;     //!< network control socket

    // forbid copying
    nBasicNetworkSystem( const nBasicNetworkSystem & );
    nBasicNetworkSystem & operator=( const nBasicNetworkSystem & );

    nSocket &                     AccessControlSocket ( void )                                ; //!< Accesses network control socket
};

// *******************************************************************************
// *
// *	GetSockets
// *
// *******************************************************************************
//!
//!		@return		the listening sockets
//!
// *******************************************************************************

nSocketListener::SocketArray const & nSocketListener::GetSockets( void ) const
{
    return this->sockets_;
}

// *******************************************************************************
// *
// *	GetSockets
// *
// *******************************************************************************
//!
//!		@param	sockets	the listening sockets to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nSocketListener const & nSocketListener::GetSockets( SocketArray & sockets ) const
{
    sockets = this->sockets_;
    return *this;
}

// *******************************************************************************
// *
// *	SetSockets
// *
// *******************************************************************************
//!
//!		@param	sockets	the listening sockets to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nSocketListener & nSocketListener::SetSockets( SocketArray const & sockets )
{
    this->sockets_ = sockets;
    return *this;
}

// *******************************************************************************
// *
// *	GetSocket
// *
// *******************************************************************************
//!
//!		@return		the raw socket
//!
// *******************************************************************************

int nSocket::GetSocket( void ) const
{
    return this->socket_;
}

// *******************************************************************************
// *
// *	GetSocket
// *
// *******************************************************************************
//!
//!		@param	socket	the raw socket to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nSocket const & nSocket::GetSocket( int & socket ) const
{
    socket = this->socket_;
    return *this;
}

// *******************************************************************************
// *
// *	SetSocket
// *
// *******************************************************************************
//!
//!		@param	socket	the raw socket to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************

nSocket & nSocket::SetSocket( int socket )
{
    this->socket_ = socket;
    return *this;
}

#endif
