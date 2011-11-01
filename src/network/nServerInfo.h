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

#ifndef ArmageTron_ServerInfo_H
#define ArmageTron_ServerInfo_H

#include "tString.h"
#include "tLinkedList.h"
#include "tArray.h"
#include "nNetwork.h"

#include "tCallbackString.h"

#include <iosfwd>
#include <memory>

class nSocket;
class nAddress;
class nServerInfo;
class tPath;

namespace Network
{ 
    class SmallServerInfoBase; 
    class SmallServerInfo; 
    class BigServerInfo; 
    class SettingsDigest;
    class RequestSmallServerInfo; 
    class RequestBigServerInfo; 
    class RequestBigServerInfoMaster; 
}

class nSenderInfo;
template< class T > class nProtoBufDescriptor;

typedef nServerInfo* (sn_ServerInfoCreator)();

//! sort helper function: if this returns true, it pushes a server to the top of the list
typedef bool SortHelper(nServerInfoBase const * server);

bool SortHelperNoop(nServerInfoBase const * server);

//! return the DNS name of this machine, if set
tString const & sn_GetMyDNSName();

//! Basic server information: everything you need to connect
class nServerInfoBase
{
public:
    nServerInfoBase();
    virtual ~nServerInfoBase();

    bool operator == ( const nServerInfoBase & other ) const;
    bool operator != ( const nServerInfoBase & other ) const;

    void WriteSync( Network::SmallServerInfoBase & info ) const;  //!< writes data to network message
    void ReadSync ( Network::SmallServerInfoBase const & info,
                    nSenderInfo                  const & sender ); //!< reads data from network message

    inline void GetFrom( nSocket const * socket );  //!< fills data from this server and the given socket

    //    nConnectError Connect();                      //!< connect to this server
    nConnectError Connect( nLoginType loginType = Login_All, const nSocket * socket = NULL );  //!< connect to this server ( using the specified socket )

    void CopyFrom( const nServerInfoBase & other );   //!< copies server info

    nServerInfoBase & operator = ( const nServerInfoBase & other );

    inline const tString & GetName() const;     //!< returns the server's name
protected:
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
public:
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
    }
};

//! Full server information
class nServerInfo: public tListItem<nServerInfo>, public nServerInfoBase
{
    int pollID;

public:
    void WriteSync( Network::BigServerInfo & info ) const;  //!< writes all data to network message
    void ReadSync ( Network::BigServerInfo const & info,
                    nSenderInfo            const & sender); //!< reads all data from network message

    void WriteSyncThis( Network::BigServerInfo & info ) const;  //!< writes data to network message, new data of this class only
    void ReadSyncThis ( Network::BigServerInfo const & info,
                        nSenderInfo            const & sender); //!< reads data from network message, new data of this class only

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

    tString nameForSorting;     //!< Name used when sorting servers.

    tString userNames_;		// names of the connected users
    tString userGlobalIDs_;		// IDs of the connected users
    tString userNamesOneLine_;// names of the connected users in one line
    tString options_;			// description of non-default options
    tString url_;				// url asociated with the server

    REAL    score;            // score based on ping and number of users (and game mode...)
    int     scoreBias_;         // score bias for this server

    virtual void DoGetFrom( nSocket const * socket );  //!< fills data from this server and the given socket

    // common subfunctions of the two BigServerInfo functions
    static nServerInfo* GetBigServerInfoCommon( Network::BigServerInfo const & info, nSenderInfo const & sender );
    static void GiveBigServerInfoCommon( int receiver, const nServerInfo & info, nProtoBufDescriptor< Network::BigServerInfo > & descriptor );

    nServerInfo( nServerInfo const & other );
    nServerInfo & operator = ( nServerInfo const & other );
public:
    nServerInfo();
    virtual ~nServerInfo();

    nServerInfo* Prev();

    virtual void CalcScore(); // calculates the score from other data

    virtual void Save(std::ostream &s) const;
    virtual void Load(std::istream &s);


    struct SettingsDigest
    {
        enum Flags
        {
            Flags_AuthenticationRequired = 0x1, //!< is authentication required to play on this server?
            Flags_NondefaultMap = 0x2, //!< custom map, indicating complicated gameplay
            Flags_TeamPlay = 0x4, //!< team play is possible
            Flags_SettingsDigestSent = 0x8000, //!< Did this server send a settings digest?
            Flags_All = 0xffff
        };

        unsigned short flags_;      //!< flags

        void SetFlag( Flags flag, bool set ); //!< sets a certain flag to a value
        bool GetFlag( Flags flag ) const; //!< returns the value of a certain flag

        void WriteSync( Network::SettingsDigest & sync ) const;
        void ReadSync( Network::SettingsDigest const & sync );
        
        // play time requirements, filled by ePlayer.cpp
        int minPlayTimeTotal_;    //!< minimum number of minutes spent playing up to now
        int minPlayTimeOnline_;   //!< minimum number of minutes spent playing online
        int minPlayTimeTeam_;     //!< minimum number of minutes spent playing team games

        // filled by gCycleMovement.cpp
        REAL cycleDelay_;         //!< cycle delay (max .05s if doublebinding has been disabled)
        REAL acceleration_;       //!< acceleration strength (relative to base speed)
        REAL rubberWallHump_;     //!< characteristic rubber number: rubber/(base speed*cycle_delay), the number of times you can hump a wall without suiciding
        REAL rubberHitWallRatio_;  //!< characteristic rubber number: maximum ratio of time spent sitting on walls to total time
        REAL wallsLength_;         //!< length of walls (divided by speed, so it's in seconds)

        SettingsDigest();
    };

    struct Classification
    {
        Classification();

        int sortOverride_;    //!< values 0 get priority in sorting server lists
        tString noJoin_;      //!< is there a reason we shouldn't join?
        tString description_; //!< classification description or reason why play is impossible
    };

    // sort key selection
    enum PrimaryKey
    {
        KEY_NAME,       // alphanumerically by name
        KEY_PING,       // by ping
        KEY_USERS,      // by number of players
        KEY_SCORE,      // by combined score
        KEY_MAX         // max value
    };

    //! priority of the helper value
    enum SortHelperPriority
    {
        PRIORITY_NONE,      // no influence
        PRIORITY_SECONDARY, // user's sort choice has priority, the helper is secondary
        PRIORITY_PRIMARY    // the helper function's return value is the most important
    };

    static nServerInfo *GetFirstServer();  // get the first (best) server
    static void Sort( PrimaryKey key, SortHelper helper=&SortHelperNoop, SortHelperPriority helperPriority=PRIORITY_PRIMARY );
                                           // sort the servers by score
    static tString SortableName( const char * ); // gives a sanitized name for sorting
    static void CalcScoreAll();            // calculate the score for all servers
    static void DeleteAll(bool autosave=true);     // delete all server infos

    virtual void Alive();					// called whenever the server gave a signal of life

    // reads small server information (adress, port, public key)
    // from the master server or response to a broadcast to the client
    static void GetSmallServerInfo( Network::SmallServerInfo const & info,
                                    nSenderInfo              const & sender );

    // reads request for small server information from master server/broadcast
    static void GiveSmallServerInfo( Network::RequestSmallServerInfo const & info,
                                     nSenderInfo                     const & sender );

    // reads rest of the server info (name, number of players,
    // etc) from the server
    static void GetBigServerInfo( Network::BigServerInfo const & info,
                                  nSenderInfo            const & sender );

    // reads request for big server information on a server
    static void GiveBigServerInfo( Network::RequestBigServerInfo const & info,
                                   nSenderInfo                   const & sender );

    // reads the rest of the server info (name, number of players,
    // etc) from the master
    static void GetBigServerInfoMaster( Network::BigServerInfo const & info,
                                        nSenderInfo            const & sender );

    // reads request for big server information on a master
    static void GiveBigServerInfoMaster( Network::RequestBigServerInfoMaster const & info,
                                         nSenderInfo                         const & sender );

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

    // Get all the basic infos from the master server.
    // 
    // @param masterInfo If non-NULL, then this master server will be polled for data.
    //                   Otherwise, a random master server will be selected from the defaults.
    // @param fileSuffix A suffix used to determine the filename where master data is stored to on disk.
    static void GetFromMaster(nServerInfoBase *masterInfo=NULL, char const * fileSuffix = NULL );

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

    static void DeleteUnreachable();     //!< Removes unreachable servers from the list

    static void StartQueryAll( QueryType query = QUERY_ALL );     // start querying the advanced info of each of the servers in our list

    static bool DoQueryAll(int simultaneous=10);         // continue querying the advanced info of each of the servers in our list; return value: do we need to go on with this?

    void QueryServer();     // start to get advanced info from this server itself
    void SetQueryType( QueryType query );     // set the query type for this server
    void ClearInfoFlags();                               //!< clears information sent flags

    void SetFromMaster();                               //!< indicate that this server was fetched through the master

    static void RunMaster();                             // run a master server

    static void GetSenderData( int sender, tString& name, int& port );

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
    const tString& UserGlobalIDs()      const   { return userGlobalIDs_; }
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

    SettingsDigest const & GetSettings() const //!< most important settings
    {
        return settings_;
    }

    Classification const & GetClassification() const //!< classification according to settings
    {
        return classification_;
    }

protected:
    virtual const tString& DoGetName() const;   //!< returns the server's name

private:
    QueryType queryType_; //!< the query type to use for this server
    SettingsDigest settings_; //!< most important settings
    Classification classification_; //!< classification according to settings
};

class nMasterLoader
{
public:
    nMasterLoader();
    ~nMasterLoader();
    void AddMaster( const tString & connectionName, unsigned port );
private:
    nServerInfo *realFirstServer_;
};

//! callback to give other components a chance to help fill in the server info
class nCallbackFillServerInfo: public tCallback
{
    static nServerInfo::SettingsDigest * settings_;
public:
    nCallbackFillServerInfo( AA_VOIDFUNC * f );

    //! the server settings to fill
    static nServerInfo::SettingsDigest *ToFill(){ return settings_; }

    //! fills all server infos
    static void Fill( nServerInfo::SettingsDigest * settings );
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
    virtual void Classify( nServerInfo::SettingsDigest const & in, 
                           nServerInfo::Classification & out ) const = 0;  //!< classifies the server according to its setting digest

    static nServerInfoAdmin* GetAdmin();
};

class nServerInfoCharacterFilter: public tCharacterFilter
{
public:
    nServerInfoCharacterFilter( bool filterIPAddressCharacters );
    tString FilterServerName( tString const & s );
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
