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

#ifndef ArmageTron_ServerInfo_H
#define ArmageTron_ServerInfo_H

#include "tString.h"
#include "tLinkedList.h"
#include "tArray.h"
#include "nNetwork.h"

#include <iosfwd>
#include <memory>

class nSocket;
class nAddress;
class nServerInfo;
class tPath;

typedef nServerInfo* (sn_ServerInfoCreator)();

//! Basic server information: everything you need to connect
class nServerInfoBase
{
public:
    nServerInfoBase();
    virtual ~nServerInfoBase();

    bool operator == ( const nServerInfoBase & other ) const;
    bool operator != ( const nServerInfoBase & other ) const;

    inline void NetWrite(nMessage &m) const;    //!< writes data to network message
    inline void NetRead (nMessage &m);          //!< reads data from network message
    void NetWriteThis(nMessage &m) const;       //!< writes data to network message
    void NetReadThis (nMessage &m);             //!< reads data from network message

    inline void GetFrom( nSocket const * socket );  //!< fills data from this server and the given socket

    //    nConnectError Connect();                      //!< connect to this server
    nConnectError Connect( nLoginType loginType = Login_All, const nSocket * socket = NULL );  //!< connect to this server ( using the specified socket )

    void CopyFrom( const nServerInfoBase & other );   //!< copies server info

    nServerInfoBase & operator = ( const nServerInfoBase & other );

    inline const tString & GetName() const;     //!< returns the server's name
protected:
    virtual void DoNetWrite(nMessage &m) const; //!< writes data to network message
    virtual void DoNetRead (nMessage &m);       //!< reads data from network message

    virtual void DoGetFrom( nSocket const * socket );  //!< fills data from this server and the given socket

    virtual const tString& DoGetName() const;   //!< returns the server's name
private:
    tString         connectionName_;            //!< the internet name of the server ("192.168.10.10", "atron.dyndns.org")
    unsigned int    port_;                      //!< the network port the server listens on
    mutable std::auto_ptr< nAddress > address_; //!< the network address of the server
public:
    inline tString const & GetConnectionName( void ) const;	                                 //!< Gets the internet name of the server ("192.168.10.10", "atron.dyndns.org")
    inline nServerInfoBase const & GetConnectionName( tString & connectionName ) const;	     //!< Gets the internet name of the server ("192.168.10.10", "atron.dyndns.org")
    inline unsigned int GetPort( void ) const;	                                             //!< Gets the network port the server listens on
    inline nServerInfoBase const & GetPort( unsigned int & port ) const;	                 //!< Gets the network port the server listens on
    nAddress const & GetAddress( void ) const;	                                             //!< Gets the network address of the server
    nServerInfoBase const & GetAddress( nAddress & address ) const;	                         //!< Gets the network address of the server
    nServerInfoBase const & ClearAddress() const;	                                         //!< Clears the network address of the server ( so it gets requeried )
private:
    nAddress & AccessAddress( void ) const;	                       //!< Accesses the network address of the server
    nServerInfoBase & SetAddress( nAddress const & address );	   //!< Sets the network address of the server
protected:
    inline nServerInfoBase & SetConnectionName( tString const & connectionName );	         //!< Sets the internet name of the server ("192.168.10.10", "atron.dyndns.org")
    inline nServerInfoBase & SetPort( unsigned int port );	                                 //!< Sets the network port the server listens on
};

//! server information, just to redirect and for other immediate applications
class nServerInfoRedirect: public nServerInfoBase
{
public:
    // construct a server directly with connection name and port
    nServerInfoRedirect( tString const & connectionName, unsigned int port )
    {
        nServerInfoBase::SetConnectionName( connectionName );
        nServerInfoBase::SetPort( port );
    };
};

//! Full server information
class nServerInfo: public tListItem<nServerInfo>, public nServerInfoBase
{
    int pollID;
protected:
    // information only for the master server
    unsigned int    transactionNr;     // a running number assigned to every server that connects to the master

    // encryption information (from the master server, too)
    int method;                   // encryption method identifier
    tArray<unsigned int> key;    // the public key for encrypting important messages to the server (i.e. the session key)

    // advanced information; obtained by directly querying the server.
    bool    advancedInfoSet;  // did we already get the info?
    bool    advancedInfoSetEver;  // did we already get the info during this query run?
    int     queried;          // how often did we already query for it this turn?

    nTimeRolling timeQuerySent;    //  the time the info query message was sent
    REAL    	 ping;             // the ping time

    nVersion version_;		// currently supported protocol versions
    tString release_;			// release version
    bool		login2_;		// flag indicating whether the second version of the logic can be tried

    int     timesNotAnswered; // number of times the server did not answer to information queries recently
    bool    stillOnMasterServer; // flag indicating whether the server is still listed on the master

    // human information
    tString name;             // the human name of the server ("Z-Man's Armagetron Server");
    int     users;            // number of users online
    int	  maxUsers_;		// maximum number of users allowed

    tString userNames_;		// names of the connected users
    tString userGlobalIDs_;		// IDs of the connected users
    tString userNamesOneLine_;// names of the connected users in one line
    tString options_;			// description of non-default options
    tString url_;				// url asociated with the server

    REAL    score;            // score based on ping and number of users (and game mode...)
    int     scoreBias_;         // score bias for this server

    virtual void DoNetWrite(nMessage &m) const; //!< writes data to network message
    virtual void DoNetRead (nMessage &m);       //!< reads data from network message
    void NetWriteThis(nMessage &m) const;       //!< writes data to network message
    void NetReadThis (nMessage &m);             //!< reads data from network message

    virtual void DoGetFrom( nSocket const * socket );  //!< fills data from this server and the given socket

    // common subfunctions of the two BigServerInfo functions
    static nServerInfo* GetBigServerInfoCommon(nMessage &m);
    static void GiveBigServerInfoCommon(nMessage &m, const nServerInfo & info, nDescriptor& descriptor );

    nServerInfo( nServerInfo const & other );
    nServerInfo & operator = ( nServerInfo const & other );
public:
    nServerInfo();
    virtual ~nServerInfo();

    nServerInfo* Prev();

    virtual void CalcScore(); // calculates the score from other data

    // read/write all the information a normal server will broadcast
    // virtual void NetWrite(nMessage &m);
    // virtual void NetRead (nMessage &m);

    // the same for the information the master server is responsible for
    //  virtual void MasterNetWrite(nMessage &m);
    //  virtual void MasterNetRead (nMessage &m);

    virtual void Save(std::ostream &s) const;
    virtual void Load(std::istream &s);

    // sort key selection
    enum PrimaryKey
    {
        KEY_NAME,       // alphanumerically by name
        KEY_PING,       // by ping
        KEY_USERS,      // by number of players
        KEY_SCORE,      // by combined score
        KEY_MAX         // max value
    };

    static nServerInfo *GetFirstServer();  // get the first (best) server
    static void Sort( PrimaryKey key );    // sort the servers by score
    static void CalcScoreAll();            // calculate the score for all servers
    static void DeleteAll(bool autosave=true);     // delete all server infos

    virtual void Alive();					// called whenever the server gave a signal of life

    // reads small server information (adress, port, public key)
    // from the master server or response to a broadcast to the client
    static void GetSmallServerInfo(nMessage &m);

    // reads request for small server information from master server/broadcast
    static void GiveSmallServerInfo(nMessage &m);

    // reads rest of the server info (name, number of players,
    // etc) from the server
    static void GetBigServerInfo(nMessage &m);

    // reads request for big server information on a server
    static void GiveBigServerInfo(nMessage &m);

    // reads the rest of the server info (name, number of players,
    // etc) from the master
    static void GetBigServerInfoMaster(nMessage &m);

    // reads request for big server information on a master
    static void GiveBigServerInfoMaster(nMessage &m);

    // used to transfer the extra erver info (players, settings
    // etc) from the server directly to the client
    //  static void GetExtraServerInfo(nMessage &m);

    // request extra server information from master server/broadcast
    //static void GiveExtraServerInfo(nMessage &m);

    // set the function that creates new server infos (so the server infos
    // generated by calls from the master server can be of a derived class).
    // returns the old function, so you can resore it later.
    static sn_ServerInfoCreator* SetCreator(sn_ServerInfoCreator* creator);

    static void Save();      // save/load all server infos
    static void Save(const tPath& path, const char *filename);      // save/load all server infos
    static void Load(const tPath& path, const char *filename);

    static nServerInfo* GetMasters();              //!< get the list of master servers
    static nServerInfo* GetRandomMaster();         //!< gets a random master server

    static void GetFromMaster(nServerInfoBase *masterInfo=NULL, char const * fileSuffix = NULL );  // get all the basic infos from the master server, stored in the server info file of the given suffix

    static void TellMasterAboutMe(nServerInfoBase *masterInfo=NULL);  // dedicated server: tell master server about my existence

    static void GetFromLAN(unsigned int pollBeginPort=4534, unsigned int pollEndPort=4544);                            // get all the basic infos from a LAN broadcast

    static void GetFromLANContinuously(unsigned int pollBeginPort=4534, unsigned int pollEndPort=4544);                            // get all the basic infos from a LAN broadcast; return immediately, servers will accumulate later
    static void GetFromLANContinuouslyStop();       // stop accepting servers from the LAN

    // enum describing the query type logic ( all non-queried servers are queried once indirectly over the master server )
    enum QueryType
    {
        QUERY_ALL=0,    //!< query all servers directly
        QUERY_OPTOUT=1, //!< query all servers with nonnegative score bias
        QUERY_OPTIN=2,  //!< query only servers with positive score bias
        QUERY_NONE=3    //!< query only manually
    };

    static void StartQueryAll( QueryType query = QUERY_ALL );     // start querying the advanced info of each of the servers in our list

    static bool DoQueryAll(int simultaneous=10);         // continue querying the advanced info of each of the servers in our list; return value: do we need to go on with this?

    void QueryServer();     // start to get advanced info from this server itself
    void SetQueryType( QueryType query );     // set the query type for this server
    void ClearInfoFlags();                               //!< clears information sent flags

    void SetFromMaster();                               //!< indicate that this server was fetched through the master


    static void RunMaster();                             // run a master server


    static void GetSenderData(const nMessage &m,tString& name, int& port);

    // information query
    bool           Reachable()		const;
    bool           Polling()			const;

    unsigned int   TransactionNr()	const   {return transactionNr;}
    unsigned int   Method()			const   {return method;}
    const tArray<unsigned int>&Key()	const	{return key;}
    REAL           Ping()				const   {return ping;}
    const	nVersion& Version()			const	{return version_;}

    int            TimesNotAnswered() const	{return timesNotAnswered;}

    int            Users()            const	{return users;}
    int            MaxUsers()          const	{return maxUsers_;}

    const tString& UserNames()		const	{ return userNames_;  }
    const tString& UserNamesOneLine()	const	{ return userNamesOneLine_;  }
    const tString& Options()			const	{ return options_;  }
    const tString& Release()			const	{ return release_;  }
    const tString& Url()				const	{ return url_;  }

    REAL           Score()            const	{return score;}

    enum Compat
    {
        Compat_Ok,
        Compat_Downgrade,
        Compat_Upgrade
    };

    Compat	Compatibility()			const;
    inline nServerInfo & SetScoreBias( int scoreBias );	//!< Sets score bias for this server
    inline int GetScoreBias( void ) const;	//!< Gets score bias for this server
    inline nServerInfo const & GetScoreBias( int & scoreBias ) const;	//!< Gets score bias for this server

protected:
    virtual const tString& DoGetName() const;   //!< returns the server's name

private:
    QueryType queryType_; //!< the query type to use for this server
};


class nServerInfoAdmin
{
    friend class nServerInfo;

public:

protected:
    nServerInfoAdmin();
    virtual ~nServerInfoAdmin();

private:
    virtual tString GetUsers()		const = 0;
    virtual tString GetGlobalIDs()	const = 0;
    virtual tString	GetOptions()	const = 0;
    virtual tString GetUrl()		const = 0;

    static nServerInfoAdmin* GetAdmin();
};

// *******************************************************************************************
// *
// *   GetName
// *
// *******************************************************************************************
//!
//!        @return     this server's name
//!
// *******************************************************************************************

const tString & nServerInfoBase::GetName( void ) const
{
    return DoGetName();
}

// *******************************************************************************************
// *
// *	GetScoreBias
// *
// *******************************************************************************************
//!
//!		@return		score bias for this server
//!
// *******************************************************************************************

int nServerInfo::GetScoreBias( void ) const
{
    return this->scoreBias_;
}

// *******************************************************************************************
// *
// *	GetScoreBias
// *
// *******************************************************************************************
//!
//!		@param	scoreBias	score bias for this server to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfo const & nServerInfo::GetScoreBias( int & scoreBias ) const
{
    scoreBias = this->scoreBias_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetScoreBias
// *
// *******************************************************************************************
//!
//!		@param	scoreBias	score bias for this server to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfo & nServerInfo::SetScoreBias( int scoreBias )
{
    this->scoreBias_ = scoreBias;
    return *this;
}

// *******************************************************************************************
// *
// *	NetWrite
// *
// *******************************************************************************************
//!
//!		@param	message     message to write info to
//!
// *******************************************************************************************

void nServerInfoBase::NetWrite( nMessage & message ) const
{
    DoNetWrite( message );
}

// *******************************************************************************************
// *
// *	NetRead
// *
// *******************************************************************************************
//!
//!		@param	mesage      message to read from
//!
// *******************************************************************************************

void nServerInfoBase::NetRead( nMessage & message )
{
    DoNetRead( message );
}

// *******************************************************************************************
// *
// *	GetFrom
// *
// *******************************************************************************************
//!
//!     @param socket   socket to get bare network information from
//!
// *******************************************************************************************

void nServerInfoBase::GetFrom( nSocket const * socket )
{
    DoGetFrom( socket );
}

// *******************************************************************************************
// *
// *	GetConnectionName
// *
// *******************************************************************************************
//!
//!		@return		the internet name of the server ("192.168.10.10", "atron.dyndns.org")
//!
// *******************************************************************************************

tString const & nServerInfoBase::GetConnectionName( void ) const
{
    return this->connectionName_;
}

// *******************************************************************************************
// *
// *	GetConnectionName
// *
// *******************************************************************************************
//!
//!		@param	connectionName	the internet name of the server ("192.168.10.10", "atron.dyndns.org") to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase const & nServerInfoBase::GetConnectionName( tString & connectionName ) const
{
    connectionName = this->connectionName_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetConnectionName
// *
// *******************************************************************************************
//!
//!		@param	connectionName	the internet name of the server ("192.168.10.10", "atron.dyndns.org") to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase & nServerInfoBase::SetConnectionName( tString const & connectionName )
{
    this->connectionName_ = connectionName;
    return *this;
}

// *******************************************************************************************
// *
// *	GetPort
// *
// *******************************************************************************************
//!
//!		@return		the network port the server listens on
//!
// *******************************************************************************************

unsigned nServerInfoBase::GetPort( void ) const
{
    return this->port_;
}

// *******************************************************************************************
// *
// *	GetPort
// *
// *******************************************************************************************
//!
//!		@param	port	the network port the server listens on to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase const & nServerInfoBase::GetPort( unsigned & port ) const
{
    port = this->port_;
    return *this;
}

// *******************************************************************************************
// *
// *	SetPort
// *
// *******************************************************************************************
//!
//!		@param	port	the network port the server listens on to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase & nServerInfoBase::SetPort( unsigned port )
{
    this->port_ = port;
    return *this;
}

#endif
