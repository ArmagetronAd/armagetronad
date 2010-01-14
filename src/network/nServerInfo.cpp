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
#include "tSysTime.h"
#include "tConsole.h"
#include "tList.h"
#include "tLocale.h"
#include "tToDo.h"
#include "tDirectories.h"
#include "tConfiguration.h"
#include "tRandom.h"
#include "tRecorder.h"

#include "nSocket.h"

#include "nServerInfo.h"
#include "nNetObject.h"
#ifdef KRAWALL_SERVER
#include "nAuthentication.h"
#endif

#include <fstream>

static nServerInfo*          sn_masterList  = NULL;
static nServerInfo*          sn_FirstServer = NULL;
static sn_ServerInfoCreator* sn_Creator     = NULL;

static nServerInfo*          sn_Transmitting[MAXCLIENTS+2];
static unsigned int          sn_LastKnown   [MAXCLIENTS+2];
static bool                  sn_SendAll     [MAXCLIENTS+2];
static bool                  sn_Requested   [MAXCLIENTS+2];
static REAL                  sn_Timeout     [MAXCLIENTS+2];

#ifdef KRAWALL_SERVER
static bool                  sn_Auth        [MAXCLIENTS+2];
#endif

static nServerInfo*          sn_Requesting=NULL;
static unsigned int          sn_NextTransactionNr = 0;

static bool sn_AcceptingFromBroadcast = false;
bool sn_AcceptingFromMaster    = false;
static bool sn_IsMaster               = false;

static nServerInfo*           sn_QuerySoon =NULL;
static nTimeRolling           sn_QueryTimeout = 0;

static REAL sn_queryDelay = 1.5f;	// time delay between queries of the same server
static REAL sn_queryDelayGlobal = 0.1f;	// time delay between all queries
static int sn_numQueries = 3;	// number of queries per try
static int sn_TNALostContact = 4;  // minimum TNA value to be considered contact loss

static tSettingItem< REAL > sn_queryDelayConf( "BROWSER_QUERY_DELAY_SINGLE", sn_queryDelay );
static tSettingItem< REAL > sn_queryDelayGlobalConf( "BROWSER_QUERY_DELAY_GLOBAL", sn_queryDelayGlobal );
static tSettingItem< int > sn_numQueriesConf( "BROWSER_NUM_QUERIES", sn_numQueries );
static tSettingItem< int > sn_TNALostContactConf( "BROWSER_CONTACTLOSS", sn_TNALostContact );

// number of big server info queries to accept (-1 for infinity)
static int sn_numAcceptQueries = 0;

static int sn_MaxUnreachable()
{
    return sn_IsMaster ? 10 : 5;
}

static int sn_MaxTNA()
{
    return sn_IsMaster ? 30 : 10;
}

static int sn_MaxTNATotal()
{
    return sn_IsMaster ? 300 : 50;
}

static tList<nServerInfo>	sn_Polling;

static tString sn_LastLoaded;  // stores the filename of the last loaded server list

// make sure every new client gets a new server list
static void login_callback(){
    sn_Transmitting[nCallbackLoginLogout::User()] = NULL;
    sn_LastKnown   [nCallbackLoginLogout::User()] = 0;
    sn_SendAll     [nCallbackLoginLogout::User()] = true;
    sn_Requested   [nCallbackLoginLogout::User()] = false;
    sn_Timeout     [nCallbackLoginLogout::User()] = tSysTimeFloat() + 70;
#ifdef KRAWALL_SERVER
    sn_Auth        [nCallbackLoginLogout::User()] = false;
#endif

}

// helper function: server info to string
tString ToString( const nServerInfoBase & info )
{
    tString ret;
    ret << info.GetConnectionName() << ":" << info.GetPort();
    return ret;
}

// authentification stuff
#ifdef KRAWALL_SERVER_LEAGUE
void ResultCallback(const tString& username,
                    const tString& origUsername,
                    int user, bool success)
{
    if (success)
    {
        sn_Auth[user] = true;
        sn_Transmitting[user] = nServerInfo::GetFirstServer();
    }
    else
        nAuthentication::RequestLogin(username, user, tOutput("$login_request_failed"), true);
}
#endif // KRAWALL_SERVER_LEAGUE



static nCallbackLoginLogout nlc(&login_callback);

static nServerInfo *CreateServerInfo()
{
    if (sn_Creator)
        return (*sn_Creator)();
    else
        return tNEW(nServerInfo());
}

nServerInfo::nServerInfo()
        :tListItem<nServerInfo>(sn_FirstServer),
        pollID(-1),
        transactionNr(sn_NextTransactionNr),
        method(0),
        key(0),
        advancedInfoSet(false),
        advancedInfoSetEver(false),
        queried(0),
        timeQuerySent(0),
        ping(10),
        release_("pre_0.2.5"),
        login2_(true),
        timesNotAnswered(5),
        stillOnMasterServer(false),
        name(""),
        users(0),
        maxUsers_(MAXCLIENTS),
        score(-10000),
        scoreBias_(0),
        queryType_( QUERY_ALL )
{
    if (sn_IsMaster)
    {
        sn_NextTransactionNr++;
        if (0 == sn_NextTransactionNr)
            sn_NextTransactionNr++;
    }
}

nServerInfo::~nServerInfo()
{
    sn_Polling.Remove(this, pollID);

    for (int i = MAXCLIENTS+1; i>=0; i--)
        if (sn_Transmitting[i] == this)
            sn_Transmitting[i] = sn_Transmitting[i]->Next();

    if (sn_Requesting == this)
        sn_Requesting = sn_Requesting->Next();

    if (sn_QuerySoon == this)
    {
        sn_Requesting = NULL;
        sn_QuerySoon  = NULL;
    }
}


// calculates the score from other data
void nServerInfo::CalcScore()
{
    static int userScore[8] = { -100, 0, 100, 250, 300, 300, 250, 100 };

    // do nothing if we are querying
    if ( !this->advancedInfoSet )
    {
        // score = scoreBias_;

        return;
    }

    score = 100;
    if (ping > .1)
        score  -= (ping - .1) * 300;

    if (users < 8 && users >= 0)
        score += userScore[users];

    if (users >= maxUsers_ )
        score -= 200;

    if ( Compat_Ok != this->Compatibility() )
    {
        score -= 400;
    }

    score -= fabsf( this->Version().Max() - sn_MyVersion().Max() ) * 10;

    score += scoreBias_;
}


// read/write all the information a normal server will broadcast
/*
  void nServerInfo::NetWrite(nMessage &m)
  {
  m << name;
  m << users;
  }
*/


// the same for the information the master server is responsible for
/*
void nServerInfo::MasterNetWrite(nMessage &m)
{
    m << transactionNr;
    m << connectionName;
    m << port;
    m << method;
    m << key;
}
*/

/*
void nServerInfo::MasterNetRead (nMessage &m)
{
  m >> transactionNr;
  m >> connectionName;
  m >> port;
  m >> method;
  m >> key;
}
*/

static const tString TRANSACTION("transaction");
static const tString CONNECTION ("connection");
static const tString PORT       ("port");
static const tString METHOD     ("method");
static const tString KEY        ("key");
static const tString TNA        ("tna");
static const tString NAME       ("name");
static const tString VERSION_TAG    ("version");
static const tString RELEASE    ("release");
static const tString SCOREBIAS  ("scorebias");
static const tString SCORE      ("score");
static const tString URL		("url");
static const tString END        ("ServerEnd");
static const tString START      ("ServerBegin");

void nServerInfo::Save(std::ostream &s) const
{
    s << CONNECTION << "\t" << GetConnectionName() << "\n";
    s << PORT       << "\t" << GetPort()           << "\n";
    s << METHOD     << "\t" << method         << "\n";

    s << KEY  << "\t" << key.Len()      << "  ";
    for (int i = key.Len()-1; i>=0; i--)
        s << "\t" << key(i);
    s << "\n";

    s << TRANSACTION << "\t" << transactionNr << "\n";
    s << VERSION_TAG	<< "\t" << version_ << "\n";
    s << RELEASE	<< "\t" << release_ << "\n";

    s << URL		<< "\t" << url_ << "\n";

    s << SCOREBIAS  << "\t" << scoreBias_ << "\n";
    s << SCORE      << "\t" << score      << "\n";
    s << NAME  << "\t" << name             << "\n";
    s << TNA  << "\t" << timesNotAnswered  << "\n";
    s << END   << "\t" << "\n\n";
}

void nServerInfo::Load(std::istream &s)
{
    static bool warnedAboutUnknownOptions = false;
    bool warnedAboutUnknownOptionsBefore = warnedAboutUnknownOptions;

    bool end = false;
    while (!end && s.good() && !s.eof())
    {
        tString id;
        s >> id;
        int dummy;

        if (id == START)
            continue;
        else if (id == END)
            end = true;
        else if (id == METHOD)
            s >> dummy;
        else if (id == TRANSACTION)
            s >> transactionNr;
        else if ( id == VERSION_TAG )
            s >> version_;
        else if ( id == SCOREBIAS )
        {
            s >> scoreBias_;
            score = scoreBias_;
        }
        else if ( id == SCORE )
        {
            s >> score;
        }
        else if ( id == RELEASE )
            release_.ReadLine( s );
        else if ( id == URL )
            url_.ReadLine( s );
        else if (id == CONNECTION)
        {
            tString connectionName;
            connectionName.ReadLine( s );
            SetConnectionName( connectionName );
        }
        else if (id == PORT)
        {
            unsigned int port;
            s >> port;
            SetPort( port );
        }
        else if (id == KEY)
        {
            int len;
            s >> len;
            key.SetLen(len);
            for (int i=len-1; i>=0; i--)
                s >> key(i);
        }
        else if(id == TNA)
            s >> timesNotAnswered;
        else if (id == NAME)
            name.ReadLine(s);
        else
        {
            // ignore rest of line
            tString dummy;
            dummy.ReadLine( s );

            // warn, but only on first entry
            if ( !warnedAboutUnknownOptionsBefore )
            {
                con << "Warning: unknown tag " << id << " found in server config file.\n";
                warnedAboutUnknownOptions = true;
            }
        }            
    }

    queried = 0;
    advancedInfoSet = false;
    advancedInfoSetEver =false;
}

nServerInfo *nServerInfo::GetFirstServer()
{
    return sn_FirstServer;
}


nServerInfo *nServerInfo::Prev()
{
    if (reinterpret_cast<nServerInfo **>(anchor) == &sn_FirstServer)
        return NULL;

    static nServerInfo& info = *this; // will not cause recursion since it is called only once.

    // uaaa. Pointer magic...
    // TODO: perhaps there is a better way using member pointers?
    return reinterpret_cast<nServerInfo *>
           ( reinterpret_cast<char*> (anchor) +
             ( reinterpret_cast<char*>( &info ) - reinterpret_cast<char*> (&info.next) )
           );
}

// Sort server list
void nServerInfo::Sort( PrimaryKey key )
{
    // insertion sort
    nServerInfo *run = GetFirstServer();
    while (run)
    {
        nServerInfo* ascend = run;
        run = run->Next();

        while (1)
        {
            if ( ascend == GetFirstServer() )
                break;

            nServerInfo *prev = ascend->Prev();


            // check if ascend is well in place
            if (!prev)
                break;

            //	  if (prev->queried > ascend->queried)
            //	    break;
            int compare = 0;
            bool previousPolling = prev->Polling();
            bool previousUnreachable  = !prev->Reachable() && !previousPolling;
            bool ascendPolling = ascend->Polling();
            bool ascendUnreachable  = !ascend->Reachable() && !ascendPolling;

            switch ( key )
            {

            case KEY_NAME:
                // Unreachable servers should be displayed at the end of the list
                if ( !previousUnreachable && !ascendUnreachable ) {
                    compare = tColoredString::RemoveColors(prev->name).Compare( tColoredString::RemoveColors(ascend->name), true );
                }

                break;
            case KEY_PING:
                if ( ascend->ping > prev->ping )
                    compare = -1;
                else if ( ascend->ping < prev->ping )
                    compare = 1;
                break;
            case KEY_USERS:
                compare = ascend->users - prev->users;
                break;
            case KEY_SCORE:
                if ( previousUnreachable )
                    compare ++;
                if ( ascendUnreachable )
                    compare --;
                if ( ascend->score > prev->score )
                    compare = 1;
                else if ( ascend->score < prev->score )
                    compare = -1;
                break;
            case KEY_MAX:
                break;
            }

            if (0 == compare)
            {
                if ( previousPolling )
                    compare++;
                if ( ascendPolling )
                    compare--;
            }

            if ( compare <= 0 )
                break;

            prev->Remove();
            prev->Insert( ascend->next );
        }
    }
}

void nServerInfo::CalcScoreAll()        // calculate the score for all servers
{
    nServerInfo *run = GetFirstServer();
    while (run)
    {
        run->CalcScore();
        run = run->Next();
    }
}

void nServerInfo::DeleteAll(bool autosave)               // delete all server infos
{
    if (autosave)
    {
        Save();
    }
    sn_LastLoaded.SetLen(0);

    while (GetFirstServer())
        delete GetFirstServer();
}

// set the function that creates new server infos (so the server infos
// generated by calls from the master server can be of a derived class).
// returns the old function, so you can resore it later.
sn_ServerInfoCreator* nServerInfo::SetCreator(sn_ServerInfoCreator* creator)
{
    sn_ServerInfoCreator* ret = sn_Creator;
    sn_Creator = creator;
    return ret;
}


void nServerInfo::Save()
{
    if ( sn_LastLoaded.Len() <= 1 )
    {
        return;
    }

    Save( tDirectories::Var(), sn_LastLoaded );
}

void nServerInfo::Save(const tPath& path, const char *filename)      // save/load all server infos
{
    // don't save on playback
    if ( tRecorder::IsPlayingBack() )
        return;

    std::ofstream s;
    if ( path.Open( s, filename ) )
    {
        nServerInfo *run = GetFirstServer();
        while (run && run->Next())
        {
            run = run->Next();
        }

        while (run)
        {
            s << START << "\n";
            run->Save(s);
            run = run->Prev();
        }
    }
}

// streaming operators
std::istream & operator >> ( std::istream & s, nServerInfo & info )
{
    info.Load( s );

    return s;
}

std::ostream & operator << ( std::ostream & s, nServerInfo const & info )
{
    info.Save( s );

    return s;
}

// checks whether server is in list twice, deletes it if that's the case
static void CheckDuplicate( nServerInfo * server )
{
    // remove double servers
    bool IsDouble = 0;
    nServerInfo *run = nServerInfo::GetFirstServer();
    while(!IsDouble && run)
    {
        if (run != server && *run == *server )
            IsDouble = true;
        
        run = run->Next();
    }
    
    if (IsDouble)
    {
#ifdef DEBUG
        con << "Deleting duplicate server " << server->GetName() << "\n";
#endif  
        delete server;
    }
}

void nServerInfo::Load(const tPath& path, const char *filename)
{
    sn_LastLoaded = filename;

    // read server info from archive
    static char const * section = "SERVERINFO";
    static char const * sectionEnd = "SERVERINFOEND";
    if ( tRecorder::IsPlayingBack() )
    {
        nServerInfo *server = CreateServerInfo();
        while ( tRecorder::Playback( section, *server ) )
        {
            // preemptively resolve DNS
            server->GetAddress();

            tRecorder::Record( section, *server );

            CheckDuplicate( server );

            server = CreateServerInfo();
        }
        delete server;

        // acknowledge end of playback
        tRecorder::PlaybackStrict( sectionEnd );
        tRecorder::Record( sectionEnd );
        return;
    }


    std::ifstream s;
    path.Open( s, filename );

    while (s.good() && !s.eof())
    {
        tString id;
        s >> id;
        if (id == START)
        {
            nServerInfo *server = CreateServerInfo();
            server->Load(s);

            // record server
            tRecorder::Record( section, *server );
            
            // preemptively resolve DNS
            server->GetAddress();

            CheckDuplicate( server );
        }
        else
            break;
    }

    // mark end of recording
    tRecorder::Record( sectionEnd );
}



// **********************************
// now the real network protocol part
// **********************************

// the network handlers and descriptors used by the master protocol

// used to transfer small server information (adress, port, public key)
// from the master server or response to a broadcast to the client
static nDescriptor SmallServerDescriptor(50,nServerInfo::GetSmallServerInfo,"small_server", true);

// used to transfer the rest of the server info (name, number of players, etc)
// from the server directly to the client
static nDescriptor BigServerDescriptor(51,nServerInfo::GetBigServerInfo,"big_server", true);
static nDescriptor BigServerMasterDescriptor(54,nServerInfo::GetBigServerInfoMaster,"big_server_master", true);

// request small server information from master server/broadcast
static nDescriptor RequestSmallServerInfoDescriptor(52,nServerInfo::GiveSmallServerInfo,"small_request", true);

// request big server information from master server/broadcast
static nDescriptor RequestBigServerInfoDescriptor(53,nServerInfo::GiveBigServerInfo,"big_request", true);
static nDescriptor RequestBigServerInfoMasterDescriptor(55,nServerInfo::GiveBigServerInfoMaster,"big_request_master", true);

// used to transfer the rest of the server info (name, number of players, etc)
// from the server directly to the client
//atic nDescriptor ExtraServerDescriptor(54,nServerInfo::GetExtraServerInfo,"extra_server", true);

// request big server information from master server/broadcast
//atic nDescriptor RequestExtraServerInfoDescriptor(55,nServerInfo::GiveExtraServerInfo,"extra_request", true);

static bool net_Accept()
{
    return
        nCallbackAcceptPackedWithoutConnection::Descriptor()==SmallServerDescriptor.ID() ||
        nCallbackAcceptPackedWithoutConnection::Descriptor()==BigServerDescriptor.ID() ||
        nCallbackAcceptPackedWithoutConnection::Descriptor()==BigServerMasterDescriptor.ID();
}

static nCallbackAcceptPackedWithoutConnection net_acc( &net_Accept );

static int sn_ServerCount = 0;

/*
static void ReadServerInfo(nMessage &m, unsigned int& port, tString& connectionName, bool acceptDirect, bool acceptMaster)
{
    m >> port;            // get the port
    m >> connectionName;  // and the name
    if (connectionName.Len()<=1) // no valid name (must come directly from the server who does not know his own address)
    {
        sn_GetAdr(m.SenderID(), connectionName);

        // remove the port
        for (int i=connectionName.Len(); i>=0; i--)
            if (':' == connectionName[i])
            {
                connectionName[i] = '\0';
                connectionName.SetLen(i+1);
            }

        if (!acceptDirect && !acceptMaster)
        {
            //	  Cheater(m.SenderID());
            return;
        }
    }
    else
    {
        if (!acceptMaster)
        {
            //	  Cheater(m.SenderID());
            return;
        }
    }
}

static void WriteServerInfo(nMessage &m, unsigned int port, const tString connectionName)
{
    m << port;
    if (connectionName.Len() > 3 && connectionName[0] == '1' && connectionName[1] == '2' &&  connectionName[2] == '7')
        m << tString("");
    else
        m << connectionName;
}

static void WriteMyInfo(nMessage &m)
{
    m << sn_GetServerPort();
    m << tString("");
}
*/


// TODO: make these dynamic
/*
static tString sn_localName("127.0.0.1");
static tString sn_globalName("armagetron.kicks-ass.net");

static tSettingItem< tString > sn_localNameSetting( "LOCAL_NETNAME", sn_localName );
static tSettingItem< tString > sn_globalNameSetting( "GLOBAL_NETNAME", sn_globalName );

static const tString s_LocalName("127.0.0.2");
static const tString s_GlobalName("armagetron.kicks-ass.net");

static void S_GlobalizeName(tString &connectionName)
{
    if ( sn_IsMaster )
    {
        if ( connectionName == sn_localName )
        {
            connectionName = sn_globalName;
        }
    }
}

static tString S_LocalizeName(const tString &connectionName)
{
    if ( !sn_IsMaster )
    {
        if ( connectionName[0] < '0' || connectionName[0] > '9' )
        {
            // get the IP adress
            struct sockaddr temp;
            ANET_GetAddrFromName (connectionName, &temp);
            tString connectionNameTemp( ANET_AddrToString (&temp) );
            // remove the port part
            for(int pos = connectionNameTemp.Len()-1; pos>=0; pos--)
                if (':' == connectionNameTemp[pos])
                {
                    connectionNameTemp[pos]='\0';
                    connectionNameTemp.SetLen(pos+1);
                }

            return connectionNameTemp;
        }
    }

    return connectionName;
}
*/

// dummy functions
static void S_GlobalizeName(tString &connectionName)
{
}

static const tString & S_LocalizeName(const tString &connectionName)
{
    return connectionName;
}

static tString sn_connectionName("");
//static tSettingItem< tString > sn_connectionNameSetting( "CONNECTION_NAME", sn_connectionName );

/*
static void WriteMyInfo(nMessage &m)
{
    m << sn_GetServerPort();
    m << sn_connectionName;
}
*/

void nServerInfo::Alive()
{
    // give it a new transaction number if it was down temporarily
    if ( sn_IsMaster && TimesNotAnswered() >= sn_TNALostContact )
    {
        transactionNr = sn_NextTransactionNr++;
        if (!sn_NextTransactionNr)
            sn_NextTransactionNr++;
    }
}

// size of flood protection array
static const int sn_minPingCount=4;
// number of consecutive pings that get tracked
static const int sn_minPingCounts[sn_minPingCount] = { 10,20,50,100 };
// time limits for pings to come in
static REAL sn_minPingTimes[sn_minPingCount] = { 1,4,20,100 };

// setting items to make flood protection configurable
static tSettingItem< REAL > sn_minPingTime10 ( "PING_FLOOD_TIME_10" , sn_minPingTimes[0] );
static tSettingItem< REAL > sn_minPingTime20 ( "PING_FLOOD_TIME_20" , sn_minPingTimes[1] );
static tSettingItem< REAL > sn_minPingTime50 ( "PING_FLOOD_TIME_50" , sn_minPingTimes[2] );
static tSettingItem< REAL > sn_minPingTime100( "PING_FLOOD_TIME_100", sn_minPingTimes[3] );

// machine decorator to store ping statistics from individual clients
class nQueryMessageStats: public nMachineDecorator
{
public:
    nQueryMessageStats( nMachine & machine )
            : nMachineDecorator( machine )
            , machine_( machine )
            , warned_(false)
    {
        tASSERT( MAX > sn_minPingCounts[ sn_minPingCount-1 ] );

        for ( int i = 0; i < MAX; ++i )
            lastTime_[i] = -1E10;

        lastTimeIndex_ = 0;
    }

    enum{ MAX=101 };
    nMachine & machine_;     // the machine this belongs to
    double lastTime_[MAX];   // log of the last times the server was pinged by this client
    int lastTimeIndex_;      // the current index in the array
    bool warned_;            // flag to avoid warning spam in the log file

    tString GetIP()
    {
        tString ret = machine_.GetIP();
        if ( ret.Len() < 4 )
            ret = "everyone";

        return ret;
    }

    // determine whether a warning should be printed now
    bool PrintWarning()
    {
        if ( warned_ )
            return false;

        double time = tSysTimeFloat();
        static double lastTime = time-10;
        if ( time < lastTime + 5 )
            return false;

        warned_ = true;
        return true;
    }

    virtual void OnDestroy()
    {
        if ( warned_ )
            con << "Flood protection ban of " << GetIP() << " removed because machine is discarded.\n";

        delete this;
    }

    void Block()
    {
        // block a DOS attacker for a while so he thinks his attack
        // brought the server down
        double time = tSysTimeFloat();

        if ( lastTime_[0] < time )
        {
            for ( int i = 0; i < MAX; ++i )
            {
                lastTime_[i] = time+10;
            }
        }

        if ( PrintWarning() )
        {
            con << "Flood protection bans " << GetIP() << " for sending multiple querty messages with one packet.\n";
        }
    }

    // determines whether this client should be considered flooding
    bool FloodProtection( REAL timeFactor )
    {
        int i;
        double now = tSysTimeFloat();

        bool protect = false;
        REAL diff = 0;
        int count = 0;

        // go through the different levels
        for ( i = sn_minPingCount-1; i >= 0; --i )
        {
            // this many pings should be tracked
            count = sn_minPingCounts[i];
            diff = now - lastTime_[(lastTimeIndex_ + MAX - count) % MAX];
            REAL tolerance = sn_minPingTimes[i]*timeFactor;
            if ( tolerance > 0 && diff < tolerance )
            {
                protect = true;
                break;
            }
        }

        lastTimeIndex_ = (lastTimeIndex_ + 1) % MAX;
        lastTime_[lastTimeIndex_] = now;

        if ( protect )
        {
            if ( PrintWarning() )
                con << "Flood protection bans " << GetIP() << " after " << count << " pings in " << int((diff)*100)/100.0 << " seconds.\n";

            return true;
        }

        // reset warning flag
        if ( warned_ && now - lastTime_[(lastTimeIndex_ + MAX-2 ) % MAX] > sn_minPingTimes[ sn_minPingCount-1 ] )
        {
            con << "Flood protection ban of " << GetIP() << " removed.\n";
            warned_ = false;
        }

        return false;
    }
};

// finds the query message stats for a machine
nQueryMessageStats & GetQueryMessageStats( nMachine & machine )
{
    // get the decorator
    nQueryMessageStats * stats = NULL;
    nMachineDecorator * decorator = machine.GetDecorators();
    while ( decorator && !stats )
    {
        stats = dynamic_cast< nQueryMessageStats * >( decorator );
        decorator = decorator->Next();
    }
    if ( !stats )
        stats = tNEW( nQueryMessageStats )( machine );

    // and delegate
    return *stats;
}

// determines wheter the machine is causing a flood attack
bool FloodProtection( nMachine & machine, REAL timeFactor=1.0 )
{
    // get the decorator and delegate
    return GetQueryMessageStats( machine ).FloodProtection( timeFactor );
}

// flag reset after each packet was received
static bool sn_firstInPacket = true;
static void sn_ResetFirstInPacket()
{
    sn_firstInPacket = true;
}
static nCallbackReceivedComplete sn_resetFirstInPacket( sn_ResetFirstInPacket );
static REAL sn_minPingTimeGlobalFactor = 0.1;
static tSettingItem< REAL > sn_minPingTimeGlobal( "PING_FLOOD_GLOBAL", sn_minPingTimeGlobalFactor );

// determines wheter the message comes from a flood attack; if so, reject it (return true)
bool FloodProtection( nMessage const & m )
{
    // get the machine infos
    nMachine & server = nMachine::GetMachine( 0 );
    nMachine & peer   = nMachine::GetMachine( m.SenderID() );

    // only accept one ping per packet
    if ( !sn_firstInPacket )
    {
        GetQueryMessageStats( peer ).Block();

        return true;
    }

    // don't accept further queries from this packet
    sn_firstInPacket = false;

    if ( sn_minPingTimes[sn_minPingCount - 1] <= 0 )
        return false;

    // and delegate
    return FloodProtection( peer ) || ( sn_minPingTimeGlobalFactor > 0 && FloodProtection( server, sn_minPingTimeGlobalFactor ) );
}

void nServerInfo::GetSmallServerInfo(nMessage &m){
    nServerInfoBase baseInfo;
    baseInfo.NetRead( m );

    // ReadServerInfo(m, port, connectionName, sn_AcceptingFromBroadcast, sn_AcceptingFromMaster);

    // master server should not listen to LAN games
    //if ( sn_IsMaster && baseInfo.GetConnectionName().Len() >= 3 && 0 == strncmp( baseInfo.GetConnectionName(), "192", 3 ) )
    //{
    //    return;
    //}

    // S_GlobalizeName( connectionName );
    sn_ServerCount++;

    nServerInfo *n = NULL;

    // check if we already have that server lised
    nServerInfo *run = GetFirstServer();
    int countSameAdr = 0;
    while(run)
    {
        if (run->GetAddress() == baseInfo.GetAddress() )
        {
            if (countSameAdr++ > 32)
                n = run;

            if ( run->GetPort() == baseInfo.GetPort() )
                n = run;
        }
        run = run->Next();
    }

    if (m.End())
        return;

    if (!n)
    {
        // so far no objections have been found; create the new server info.
        n = CreateServerInfo();
        n->timesNotAnswered = 0;
        if ( sn_IsMaster )
        {
            con << "Received new server: " << ToString( baseInfo ) << "\n";
            n->timesNotAnswered = 5;
        }
    }
    else
    {
        n->Alive();

        if ( sn_IsMaster )
        {
            con << "Updated server: " <<  ToString( baseInfo ) << "\n";
        }
    }

    n->nServerInfoBase::CopyFrom( baseInfo );
    n->stillOnMasterServer = true;

    if (n->name.Len() <= 1)
        n->name <<  ToString( baseInfo );

    //  n->advancedInfoSet = false;
    n->queried         = 0;

    if (!sn_IsMaster)
        m >> n->transactionNr;
    else
    {
        // n->timesNotAnswered = 5;

        if ( n->timesNotAnswered >= 1 )
        {
            if (sn_QuerySoon)
                sn_QuerySoon->QueryServer();

            sn_QuerySoon = n;
            sn_QueryTimeout = tSysTimeFloat() + 5.0f;
        }

        unsigned int dummy;
        m >> dummy;
    }

    if (sn_IsMaster)
    {
        Save();
    }
}

static bool TransIsNewer(unsigned int newTrans, unsigned int oldTrans)
{
    int diff = newTrans - oldTrans;
    return (diff > 0);
}

void nServerInfo::GiveSmallServerInfo(nMessage &m)
{
    // start transmitting the server list in master server mode
    if (sn_IsMaster)
    {
        con << "Giving server info to user " << m.SenderID() << "\n";

        sn_Requested[m.SenderID()] = true;
#ifdef KRAWALL_SERVER_LEAGUE
        // one moment! check if we need authentification
        tString adr;
        unsigned int port = sn_GetPort(m.SenderID());
        sn_GetAdr(m.SenderID(), adr);
        if (nKrawall::RequireMasterLogin(adr, port))
        {
            nAuthentication::SetLoginResultCallback(&ResultCallback);
            nAuthentication::RequestLogin(tString(""), m.SenderID(), tOutput("$login_request_master"));
        }
        else
        {
            sn_Transmitting[m.SenderID()] = GetFirstServer();
            sn_Auth[m.SenderID()]         = true;
        }
#else
        sn_Transmitting[m.SenderID()] = GetFirstServer();
#endif

        if (m.End())
            sn_SendAll[m.SenderID()] = true;
        else
        {
            sn_SendAll[m.SenderID()] = false;
            m >> sn_LastKnown[m.SenderID()];
        }

        // give out all server info if there is a disagreement
        // if ( static_cast< unsigned int > ( sn_NextTransactionNr - sn_lastKnown[m.SenderID] ) < 1000 )
        sn_SendAll[m.SenderID()] = true;
    }

    else
    {
        if ( FloodProtection( m ) )
            return;

        // allow some followup queries
        if ( sn_numAcceptQueries >= 0 )
        {
            sn_numAcceptQueries += 3;
            if ( sn_numAcceptQueries > 10 )
            {
                sn_numAcceptQueries = 10;
            }
            if ( sn_numAcceptQueries < 5 )
            {
                sn_numAcceptQueries = 5;
            }
            // con << sn_numAcceptQueries << "\n";
        }

        // immediately respond with a small info
        tJUST_CONTROLLED_PTR< nMessage > ret = tNEW(nMessage)(SmallServerDescriptor);

        // get small server info
        nServerInfoBase info;
        info.GetFrom( sn_Connections[m.SenderID()].socket );

        // fill it in
        info.NetWrite(*ret);

        unsigned int notrans = 0;
        *ret << notrans;

        ret->ClearMessageID();
        ret->SendImmediately(m.SenderID(), false);
        nMessage::SendCollected(m.SenderID());
    }
}

// from nNetwork.cpp
int sn_NumRealUsers();

nServerInfo* nServerInfo::GetBigServerInfoCommon(nMessage &m)
{
    // read server info
    nServerInfoBase baseInfo;
    baseInfo.NetRead( m );

    // find the server
    nServerInfo *server = GetFirstServer();
    while( server && *server != baseInfo )
        server = server->Next();

    if ( server )
    {
        server->NetReadThis( m );
        server->Alive();
        server->CalcScore();
    }
    else
    {
        if ( sn_IsMaster )
        {
            tOutput message;
            message.SetTemplateParameter(1, ToString( baseInfo ) );
            message.SetTemplateParameter(2, sn_Connections[m.MessageID()].socket->GetAddress().ToString() );
            message << "$network_browser_unidentified";
            con << message;
        }
        else
        {
            // add the server, but ping it again
            nServerInfo * n = CreateServerInfo();
            n->CopyFrom( baseInfo );
            n->name = ToString( baseInfo );
            n->QueryServer();
#ifdef DEBUG
            con << "Recevied unknown server " << n->name << ".\n";
#endif
        }
    }

    return server;
}

void nServerInfo::GiveBigServerInfoCommon(nMessage &m, const nServerInfo & info, nDescriptor& descriptor )
{
    // create message
    nMessage *ret = tNEW(nMessage)( descriptor );

    // write info
    info.NetWrite( *ret );

    // send info
    ret->ClearMessageID();
    ret->SendImmediately(m.SenderID(), false);
    nMessage::SendCollected(m.SenderID());
}

void nServerInfo::GetBigServerInfo(nMessage &m)
{
    nServerInfo * server = GetBigServerInfoCommon( m );

    if (!server)
        return;
}

void nServerInfo::GiveBigServerInfo(nMessage &m)
{
    // check whether we should respond
    if ( sn_numAcceptQueries >= 0 )
    {
        if( sn_numAcceptQueries == 0 )
        {
            // ignore
            return;
        }

        --sn_numAcceptQueries;
        // con << sn_numAcceptQueries << "\n";
    }

    if ( FloodProtection( m ) )
        return;

    if (sn_IsMaster)
        return;

    // collect info
    nServerInfo me;
    me.GetFrom( sn_Connections[m.SenderID()].socket );

    // delegate
    GiveBigServerInfoCommon(m, me, BigServerDescriptor );
}

void nServerInfo::SetFromMaster()
{
    // no information about ping
    ping = .999;
    users = 0;
    userNames_ = userNamesOneLine_ = "Sever polled over master server, no reliable user data available.";
    userGlobalIDs_ = "";
    advancedInfoSet = true;

    CalcScore();
}

void nServerInfo::GetBigServerInfoMaster(nMessage &m)
{
    if ( sn_GetNetState() == nSERVER && FloodProtection( m ) )
        return;

    nServerInfo *server = GetBigServerInfoCommon( m );

    if (!server)
        return;

    server->SetFromMaster();
}

void nServerInfo::GiveBigServerInfoMaster(nMessage &m)
{
    if ( FloodProtection( m ) )
        return;

    if ( !sn_IsMaster )
        return;

    // read info of desired server from message
    nServerInfoBase baseInfo;
    baseInfo.NetRead( m );

    // find the server
    nServerInfo *server = GetFirstServer();
    while( server && *server != baseInfo )
        server = server->Next();

    if (!server)
        return;

    // delegate
    GiveBigServerInfoCommon(m, *server, BigServerMasterDescriptor );
}

/*
#define nUSERNAMES 0
#define nOPTIONS 1
#define nURL 2


void nServerInfo::GiveExtraServerInfo(nMessage &m)
{
	if (sn_IsMaster)
		Cheater(m.SenderID());
  
	unsigned short extraType;
	WriteMyInfo(m);
	m >> extraType;

	tJUST_CONTROLLED_PTR< nMessage > pm = tNEW( nMessage( ExtraServerDescriptor ) );
	nMessage& mRet = *pm;

	mRet << extraType;

	tString ret="UNKNOWN";

	if ( nServerInfoAdmin::GetAdmin() )
	{
		switch ( extraType )
		{
			case nUSERNAMES:
				ret = nServerInfoAdmin::GetAdmin()->GetUsers();
				break;
			case nOPTIONS:
				ret = nServerInfoAdmin::GetAdmin()->GetOptions();
				break;
			case nURL:
				ret = nServerInfoAdmin::GetAdmin()->GetUrl();
				break;
		}
	}

	mRet << ret;
	mRet.Send( m.SenderID() );
	mRet.ClearMessageID();
	mRet.SendImmediately(m.SenderID(), false);
	nMessage::SendCollected(m.SenderID());
}

void nServerInfo::GetExtraServerInfo(nMessage &m)
{
	unsigned int port;
	tString connectionName;

	ReadServerInfo(m, port, connectionName, true, false);

	//  S_GlobalizeName( connectionName );

	// find the server
	nServerInfo *server = GetFirstServer();
	while(server && !(server->port == port && S_LocalizeName(server->connectionName) == connectionName))
		server = server->Next();

	if (!server)
		return;

	unsigned short extraType;
	m >> extraType;

	tString value;
	m >> value;

	switch ( extraType )
	{
		case nUSERNAMES:
			server->userNames_ = value;
			break;
		case nOPTIONS:
			server->options_ = value;
			break;
		case nURL:
			server->url_ = value;
			break;
	}

	server->Alive();
	server->CalcScore();
}

// queries extra informationn
bool nServerInfo::QueryExtraInfo()
{
	static float lastTime = 0.0f;
	static const float Interval	
}

RequestExtraServerInfoDescriptor

nConnectError nServerInfo::Connect( nLoginType loginType, const nSocket * socket )
{
    unsigned int portBack = sn_clientPort;
    sn_clientPort = port;
    nConnectError error = sn_Connect( connectionName, login2_, socket );
    sn_clientPort = portBack;

    return error;
}
*/

tString MasterFile( char const * suffix )
{
    std::ostringstream filename;
    filename << "frommaster" << suffix << ".srv";
    return tString( filename.str().c_str() );
}

void nServerInfo::GetFromMaster(nServerInfoBase *masterInfo, char const * fileSuffix )
{
    sn_AcceptingFromMaster = true;

    if ( !fileSuffix )
    {
        fileSuffix = "";
    }

    bool multiMaster = false;
    if (!masterInfo)
    {
        multiMaster = true;
        masterInfo = GetRandomMaster();
    }

    if (!masterInfo)
        return;

    DeleteAll();

    // load all the servers we know
    Load( tDirectories::Var(), MasterFile( fileSuffix ) );

    // find the latest server we know about
    unsigned int latest=0;
    nServerInfo *run = GetFirstServer();
    if (run)
    {
        latest = run->TransactionNr();
        run = run->Next();
        while (run)
        {
            if (TransIsNewer(run->TransactionNr(), latest))
                latest = run->TransactionNr();
            run = run->Next();
        }
    }

    // connect to the master server
    con << tOutput("$network_master_connecting", masterInfo->GetName() );
    switch(masterInfo->Connect( Login_Post0252 ))
    {
    case nOK:
        break;
    case nABORT:
    {
        return;
    }
    case nTIMEOUT:
        // delete the master and select a new one
        if ( multiMaster )
        {
            delete masterInfo;
            masterInfo = sn_masterList;
        }
        else
        {
            masterInfo = 0;
        }

        if ( masterInfo )
        {
            con << tOutput( "$network_master_timeout_retry" );
            GetFromMaster();
        }
        else
        {
            tConsole::Message("$network_master_timeout_title", "$network_master_timeout_inter", 3600);
        }
        return;
        break;

    case nDENIED:
        tConsole::Message("$network_master_denied_title", "$network_master_denied_inter", 20);
        return;
        break;

    }


    // send the server list request message
    con << tOutput("$network_master_reqlist");

    nMessage *m=tNEW(nMessage)(RequestSmallServerInfoDescriptor);
    if (GetFirstServer())
        *m << latest;
    m->BroadCast();

    sn_ServerCount = 0;
    int lastReported = 10;

    // just wait for the data to pour in
    REAL timeout = tSysTimeFloat() + 60;
    while(sn_GetNetState() == nCLIENT && timeout > tSysTimeFloat())
    {
        sn_Receive();
        sn_SendPlanned();
        tAdvanceFrame(100000);
        st_DoToDo();
        if (sn_ServerCount > lastReported + 9)
        {
            tOutput o;
            o.SetTemplateParameter(1, lastReported);
            o << "$network_master_status";
            con << o;
            lastReported = (sn_ServerCount/10) * 10;
        }
    }

    tOutput o;
    o.SetTemplateParameter(1, sn_ServerCount);
    o << "$network_master_finish";
    con << o;

    // remove servers that are no longer listed on the master
    run = GetFirstServer();
    while (run)
    {
        nServerInfo * next = run->Next();
        if ( !run->stillOnMasterServer )
        {
            // if the server has still positive score bias, just reduce that
            if ( run->scoreBias_ > 0 )
            {
                run->scoreBias_ -= 10;
            }
            else
            {
#ifdef DEBUG_X
                con << "Deleting outdated server " << run->GetName() << ".\n";
#endif
                // kill it
                delete run;
            }
        }
        run = next;
    }

    Save(tDirectories::Var(), MasterFile( fileSuffix ));

    sn_SetNetState(nSTANDALONE);

    sn_AcceptingFromMaster = false;

    tAdvanceFrame();
}

void nServerInfo::GetFromLAN(unsigned int pollBeginPort, unsigned int pollEndPort)
{
    sn_AcceptingFromBroadcast = true;

    sn_LastLoaded.SetLen(0);

    // enter client state
    if (sn_GetNetState() != nCLIENT)
        sn_SetNetState(nCLIENT);

    // prepare the request message and broadcast is
    con << tOutput("$network_master_reqlist");
    for (unsigned int port = pollBeginPort; port <= pollEndPort; port++)
    {
        nMessage *m=tNEW(nMessage)(RequestSmallServerInfoDescriptor);
        m->ClearMessageID();
        m->SendImmediately(0, false);
        nMessage::BroadcastCollected(0, port);
        tDelay(1000);
    }

    sn_ServerCount = 0;
    int lastReported = 10;

    // and just wait a bit for the answers to arrive
    REAL timeout = tSysTimeFloat() + 1.5f;
    while(sn_GetNetState() == nCLIENT && timeout > tSysTimeFloat())
    {
        tAdvanceFrame();
        sn_Receive();
        sn_SendPlanned();
        tAdvanceFrame(10000);
        if (sn_ServerCount > lastReported)
        {
            tOutput o;
            o.SetTemplateParameter(1, lastReported);
            o << "$network_master_status";
            con << 0;
            lastReported = (sn_ServerCount/10) * 10;
        }
    }

    tOutput o;
    o.SetTemplateParameter(1, sn_ServerCount);
    o << "$network_master_finish";
    con << o;

    sn_AcceptingFromBroadcast = false;

    sn_SetNetState(nSTANDALONE);
}

void nServerInfo::GetFromLANContinuously(unsigned int pollBeginPort, unsigned int pollEndPort)
{
    sn_AcceptingFromBroadcast = true;

    sn_LastLoaded.SetLen(0);

    // enter client state
    if (sn_GetNetState() != nCLIENT)
        sn_SetNetState(nCLIENT);

    // prepare the request message and broadcast it
    for (unsigned int port = pollBeginPort; port <= pollEndPort; port++)
    {
        nMessage *m=tNEW(nMessage)(RequestSmallServerInfoDescriptor);
        m->ClearMessageID();
        m->SendImmediately(0, false);
        nMessage::BroadcastCollected(0, port);
        tDelay(1000);
    }
}

void nServerInfo::GetFromLANContinuouslyStop()
{
    sn_AcceptingFromBroadcast = false;

    sn_SetNetState(nSTANDALONE);
}

extern bool sn_supportRemoteLogins; // nAuthentication.cpp

void nServerInfo::TellMasterAboutMe(nServerInfoBase *masterInfo)
{
    // accept infinite queries
    sn_numAcceptQueries = -1;

    // don't reinitialize the network system
    nSocketResetInhibitor inhibitor;

    static unsigned int lastPort = 0;

    // enter server state so we know our true port number
    sn_SetNetState(nSERVER);
    unsigned int port = sn_GetServerPort();
    //if (port == lastPort)
    //    return; // the master already knows about us

    lastPort = port;

    //    sn_SetNetState(nSTANDALONE);

    if (!masterInfo)
    {
        // recurse, logging in to all masters
        nServerInfo * run = GetMasters();

        while ( run )
        {
            if ( run->GetAddress().IsSet() )
            {
                TellMasterAboutMe( run );
            }
            run = run->Next();
        }

        return;
    }

    con << tOutput("$network_master_connecting", masterInfo->GetName() );

    // the socket that the master server connection runs on
    nSocket const * connection = NULL;

    // connect to the master server; try the listening sockets first
    {
        nConnectError result = nDENIED;

        // iterate through listening sockets
        const nSocketListener & listener = sn_BasicNetworkSystem.GetListener();
        for ( nSocketListener::iterator i = listener.begin(); i != listener.end(); ++i )
        {
            result = masterInfo->Connect( Login_Post0252, &(*i) );
            if ( nOK == result )
            {
                connection = &(*i);
                break;
            }
        }

        // try a generic socket next ( a shot in the dark, but worth a try ), except if we have GLOBAL_ID on, because it causes mismatches the server being on it's control port instead of it's listening port.
        // when GLOBAL_ID is off, this does not have much incidence, so let it do
        if ( result != nOK && !sn_supportRemoteLogins )
        {
            // leave connection at NULL so the server info will be filled with generic info
            connection = NULL;
            result = masterInfo->Connect( Login_Post0252 );
        }

        // give up
        if ( result != nOK )
            return;
    }

    // collect the server information of this system
    nServerInfoBase info;
    info.GetFrom( connection );

    // write it to a network message message
    nMessage *m=tNEW(nMessage)(SmallServerDescriptor);
    info.NetWrite(*m);
    unsigned int dummy = 0;
    *m << dummy;

    // send it
    con << tOutput("$network_master_send");
    m->BroadCast();
    sn_Receive();
    sn_SendPlanned();

    // wait for the data to be accepted
    nTimeRolling timeout = tSysTimeFloat() + 20;
    while(sn_GetNetState() == nCLIENT && timeout > tSysTimeFloat() && sn_Connections[0].ackPending > 0)
    {
        sn_Receive();
        sn_SendPlanned();
        tAdvanceFrame(10000);
    }

    sn_SetNetState(nSTANDALONE);
}

void nServerInfo::QueryServer()                                  // start to get advanced info from this server itself
{
    // determine whether we should query directly or via the master
    bool queryDirectly = false;
    switch( queryType_ )
    {
    case QUERY_ALL:
        queryDirectly = true;
        break;
    case QUERY_OPTOUT:
        queryDirectly = ( scoreBias_ >= 0 );
        break;
    case QUERY_OPTIN:
        queryDirectly = ( scoreBias_ > 0 );
        break;
    case QUERY_NONE:
        queryDirectly = false;
        break;
    }

    // early exit if there is nothing to poll
    if ( !queryDirectly )
    {
        // see if the server name is just IP:port; if no, we already successfully polled it
        if ( name != ToString( *this ) )
        {
            advancedInfoSetEver = true;
        }

        if( advancedInfoSetEver )
        {
            // but only 90% of the time, we want to know if the server goes down sometime.
            static tReproducibleRandomizer randomizer;
            if ( randomizer.Get() > .1 && timesNotAnswered == 0 )
            {
                SetFromMaster();
                return;
            }
        }
    }

    sn_Polling.Add(this, pollID);

#ifdef DEBUG
    if ( sn_IsMaster )
    {
        con << "Querying server " <<  ToString( *this ) << "\n";
    }
#endif

    if (timesNotAnswered > sn_MaxTNA() )
    {
        // server was inactive too long. Delete it if possible.

        // check if this server is the one with the highest TAN
        unsigned int latest=0;
        nServerInfo *run = GetFirstServer();
        nServerInfo *best = NULL;
        while (run == this)
            run = run->Next();

        if (run)
        {
            latest  = run->TransactionNr();
            best    = run;
            run     = run->Next();

            while (run)
            {
                if ((run != this) && TransIsNewer(run->TransactionNr(), latest))
                {
                    latest = run->TransactionNr();
                    best   = run;
                }
                run = run->Next();
            }
        }

        // now, best points to the latest (except this) server and
        // latest is its TAN.

        // continue if this server is the only one available
        if (best)
        {
            // if THIS server has the latest TAN, simpy transfer it to the second latest.
            if (TransIsNewer(TransactionNr(), latest))
                best->transactionNr = TransactionNr();

            timesNotAnswered = 1000;
            if ( sn_IsMaster )
            {
                con << "Deleted unreachable server: " <<  ToString( *this ) << "\n";

                delete this;
            }
            return;
        }
    }

    if ( queryDirectly )
    {
        // send information query directly to server
        sn_Bend( GetAddress() );

#ifdef DEBUG_X
        con << "Pinging " << GetName() << "\n";
#endif

        tJUST_CONTROLLED_PTR< nMessage > req = tNEW(nMessage)(RequestBigServerInfoDescriptor);
        req->ClearMessageID();
        req->SendImmediately(0, false);
        nMessage::SendCollected(0);
    }
    else
    {
        // send information query to master
        sn_Bend( GetMasters()->GetAddress() );

        tJUST_CONTROLLED_PTR< nMessage > req = tNEW(nMessage)(RequestBigServerInfoMasterDescriptor);
        req->ClearMessageID();

        // write server info into request packet
        nServerInfoBase::NetWriteThis( *req );

        req->SendImmediately(0, false);
        nMessage::SendCollected(0);
    }

    timeQuerySent = tSysTimeFloat();
    if ( queried == 0 )
    {
        if ( ++timesNotAnswered == sn_TNALostContact && sn_IsMaster )
        {
            con << "Lost contact with server: " <<  ToString( *this ) << "\n";
        }
        else if ( sn_IsMaster && timesNotAnswered == 2 )
        {
            con << "Starting to lose contact with server: " <<  ToString( *this ) << ", name \"" << tColoredString::RemoveColors(name) << "\"\n";
        }
    }

    queried++;

    if ( !this->advancedInfoSetEver )
    {
        score = -1E+32f;
    }
}

void nServerInfo::SetQueryType( QueryType queryType )
{
    queryType_ = queryType;
}

// make it appear that we never queried the server (but keep the information)
void nServerInfo::ClearInfoFlags()
{
    advancedInfoSetEver = advancedInfoSet = false;
    ping = 10;
}

void GetSenderData(const nMessage &m,tString& name, int& port)
{
    sn_GetAdr(m.SenderID(),name);
    port = sn_GetPort(m.SenderID());
}

void nServerInfo::StartQueryAll( QueryType queryType )                         // start querying the advanced info of each of the servers in our list
{
    Sort(KEY_SCORE);
    sn_Requesting     = GetFirstServer();

    while (sn_Polling.Len())
        sn_Polling.Remove(sn_Polling(0), sn_Polling(0)->pollID);

    nServerInfo *run = GetFirstServer();

    while(run)
    {
        // set the query type
        run->SetQueryType( queryType );

        // do a DNS query of the server
        run->ClearAddress();
        run->GetAddress();

        run->queried         = 0;
        run->advancedInfoSet = 0;

        int TNA = run->TimesNotAnswered();

        // remove known status
        if ( TNA > 0 )
        {
            run->advancedInfoSetEver = false;
        }
        else
        {
            // run->advancedInfoSetEver = true;
        }

        run = run->Next();
    }

    int totalTNAMax = sn_MaxTNATotal();
    int maxUnreachable = sn_MaxUnreachable();
    int totalTNA = totalTNAMax + 1;
    int lastTNA = totalTNA + 1;
    int unreachableCount = 0;

    while ( ( totalTNA > totalTNAMax || unreachableCount > maxUnreachable ) && lastTNA != totalTNA  )
    {
        lastTNA = totalTNA;
        totalTNA = 0;
        unreachableCount = 0;

        nServerInfo *kickOut = NULL;
        int maxTNA = 0;
        int minTNA = 100;

        run = GetFirstServer();

        while(run)
        {
            // run->score = run->scoreBias_;

            int TNA = run->TimesNotAnswered();
            // sum up TNAs and determine server with maximum TNA
            if ( TNA > 0 && TNA <= sn_MaxTNA() )
            {
                unreachableCount++;
                totalTNA += TNA;
                if ( TNA > maxTNA )
                {
                    maxTNA = TNA;
                    kickOut = run;
                }
            }

            if ( TNA < minTNA  )
            {
                minTNA = TNA;
            }

            run = run->Next();
        }

        if ( minTNA > 0 )
        {
            // no server was reachable at all! Ehternet cable is probably pulled.
            return;
        }

        // mark worst server for kickout if total TNA is too high
        if ( kickOut && ( ( totalTNA > totalTNAMax && maxTNA >= sn_TNALostContact ) || unreachableCount > maxUnreachable ) )
        {
#ifdef DEBUG_X
            con << "Deleting outdated server " << kickOut->GetName() << ".\n";
#endif
            // just delete the bastard!
            delete kickOut;
            //			kickOut->timesNotAnswered = sn_MaxTNA() + 100;
        }
    }
}

bool nServerInfo::DoQueryAll(int simultaneous)         // continue querying the advanced info of each of the servers in our list; return value: do we need to go on with this?
{
    REAL time = tSysTimeFloat();

    static REAL globalTimeout = time;
    if ( time < globalTimeout )
    {
        return true;
    }

    globalTimeout = time + sn_queryDelayGlobal;

    for (int i=sn_Polling.Len()-1; i>=0; i--)
    {
        nServerInfo* poll = sn_Polling(i);
        if (poll->timeQuerySent + sn_queryDelay < time)
            sn_Polling.Remove(poll, poll->pollID);
    }

    if (sn_Requesting && sn_Polling.Len() < simultaneous)
    {
        nServerInfo* next = sn_Requesting->Next();

        if (!sn_Requesting->advancedInfoSet && sn_Requesting->pollID < 0 && sn_Requesting->queried <= sn_numQueries)
        {
            sn_Requesting->QueryServer();
        }
#ifdef DEBUG
        else
        {
            int x;
            x = 1;
        }
#endif

        sn_Requesting = next;
    }

    sn_Receive();
    sn_SendPlanned();

    if (sn_Requesting)
        return true;
    else
    {
        bool ret = false;

        nServerInfo *run = GetFirstServer();
        while(run && !ret)
        {
            if (!run->advancedInfoSet && (run->queried <= sn_numQueries-1 || run->pollID >= 0))
                ret = true;

            run = run->Next();
        }

        if (ret)
        {
            sn_Requesting = GetFirstServer();
            Save();
        }
        else
            Save();

        return ret;
    }
}



































void nServerInfo::RunMaster()
{
    sn_IsMaster               = true;
    sn_AcceptingFromBroadcast = true;

    nTimeRolling time = tSysTimeFloat();
    if (time > sn_QueryTimeout && sn_QuerySoon)
    {
        sn_QuerySoon->QueryServer();
        sn_QuerySoon = NULL;
        std::cout.flush();
        std::cerr.flush();
    }

    if (sn_NextTransactionNr == 0)
        // find the latest server we know about
    {
        unsigned int latest=0;
        nServerInfo *run = GetFirstServer();
        if (run)
        {
            latest = run->TransactionNr();
            run = run->Next();
            while (run)
            {
                if (TransIsNewer(run->TransactionNr(), latest))
                    latest = run->TransactionNr();
                run = run->Next();
            }
        }

        latest++;
        if (latest == 0)
            latest ++;

        sn_NextTransactionNr = latest;
    }

    // start with random transaction number
    if (sn_NextTransactionNr == 0)
    {
        sn_NextTransactionNr = rand();
    }

    for (int i=MAXCLIENTS; i>0; i--)
    {
        if(sn_Connections[i].socket)
        {
            // kick the user soon when the transfer is completed
            if ((sn_Requested[i] && !sn_Transmitting[i]
#ifdef KRAWALL_SERVER
                    && sn_Auth[i]
#endif
                    && sn_MessagesPending(i) == 0))
            {
                if (sn_Timeout[i] > tSysTimeFloat() + .2f)
                    sn_Timeout[i] = tSysTimeFloat() + .2f;
            }

            // defend against DOS attacks: Kill idle clients
            if(sn_Timeout[i] < tSysTimeFloat())
                sn_DisconnectUser(i, "$network_kill_timeout");

            if (!sn_Requested[i] && sn_Timeout[i] < tSysTimeFloat() + 60.0f)
                sn_DisconnectUser(i, "$network_kill_timeout");

        }

        if (sn_Transmitting[i] && sn_MessagesPending(i) < 3)
        {

            for (int j = 10-sn_MessagesPending(i); j>=0 && sn_Transmitting[i]; j--)
            {
                // skip known servers
                if (!sn_SendAll[i])
                {
                    while (sn_Transmitting[i] && !TransIsNewer(sn_Transmitting[i]->TransactionNr(), sn_LastKnown[i]))
                        sn_Transmitting[i] = sn_Transmitting[i]->Next();
                }

                if (!sn_Transmitting[i])
                    continue;

                if (sn_Transmitting[i]->TimesNotAnswered() < sn_TNALostContact )
                {
                    // tell user i about server sn_Transmitting[i]
                    nMessage *m = tNEW(nMessage)(SmallServerDescriptor);
                    sn_Transmitting[i]->nServerInfoBase::NetWriteThis( *m );
                    *m << sn_Transmitting[i]->TransactionNr();
                    m->Send(i);
                }

                sn_Transmitting[i] = sn_Transmitting[i]->Next();
            }
        }

    }

    sn_Receive();
    sn_SendPlanned();
}

bool nServerInfo::Reachable() const
{
    return advancedInfoSetEver && TimesNotAnswered() < sn_TNALostContact;
}

bool nServerInfo::Polling() const
{
    return (!advancedInfoSetEver && queried <= 3 && TimesNotAnswered() < sn_TNALostContact );// || ( !advancedInfoSet && queried > 1 && queried <= 3 );
}




/*
class nMasterServerInfo: public nServerInfo
{
public:
    nMasterServerInfo()
    {
        Remove();  // the master server should not appear on any list

        // automatically load the master server config file
        tString f;

        std::ifstream s;

        tDirectories::Config().Open( s, "master.srv" );

        Load(s);
    }
};
*/

nServerInfo *nServerInfo::GetMasters()
{
    // reload master list at least once per minute
    double time = tSysTimeFloat();
    static double deleteTime = time + 60.0;
    if ( time > deleteTime )
    {
        deleteTime = time + 60.0;

        while (sn_masterList)
            delete sn_masterList;
    }

    if (!sn_masterList)
    {
        // back up the regular server list
        nServerInfo * oldFirstServer = sn_FirstServer;
        sn_FirstServer = NULL;

        // load the master server list cleanly
        Load( tDirectories::Config(), "master.srv" );

        // transfer list
        while ( sn_FirstServer )
        {
            nServerInfo * server = sn_FirstServer;
            server->Remove();
            server->Insert( sn_masterList );
        }

        // restore up the regular server list
        sn_FirstServer = oldFirstServer;
    }

    return sn_masterList;
}

class DeleteMaster
{
public:
    ~DeleteMaster()
    {
        while (sn_masterList)
            delete sn_masterList;
    }
};

static DeleteMaster sn_delm;

nServerInfo* nServerInfo::GetRandomMaster()
{
    // select a random master server
    nServerInfo * masterInfo = GetMasters();

    // count masters
    int count = 0;
    while( masterInfo )
    {
        masterInfo = masterInfo->Next();
        ++count;
    }

    // select randomly
    static tReproducibleRandomizer randomizer;
    REAL ran = randomizer.Get();
    // REAL ran = rand()/REAL(RAND_MAX);
    int r = 1;

    masterInfo = GetMasters();
    while( masterInfo && masterInfo->Next() && r < ran * count )
    {
        masterInfo = masterInfo->Next();
        ++r;
    }

    tASSERT( masterInfo );

    // store and return
    masterInfo->Remove();
    masterInfo->Insert( sn_masterList );

    return masterInfo;
}

nServerInfo::Compat	nServerInfo::Compatibility() const
{
    if ( sn_MyVersion().Min() > version_.Max() )
    {
        return Compat_Downgrade;
    }

    if ( sn_MyVersion().Max() < version_.Min() )
    {
        return Compat_Upgrade;
    }

    return Compat_Ok;
}


static nServerInfoAdmin* sn_serverInfoAdmin = NULL;

nServerInfoAdmin::nServerInfoAdmin()
{
    tASSERT( NULL == sn_serverInfoAdmin );
    sn_serverInfoAdmin = this;
}

nServerInfoAdmin::~nServerInfoAdmin()
{
    sn_serverInfoAdmin = NULL;
}

nServerInfoAdmin* nServerInfoAdmin::GetAdmin()
{
    return sn_serverInfoAdmin;
}

// *******************************************************************************************
// *
// *   nServerInfoBase
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nServerInfoBase::nServerInfoBase()
        : connectionName_(""),
          port_(0)
{
}

// *******************************************************************************************
// *
// *   ~nServerInfoBase
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

nServerInfoBase::~nServerInfoBase()
{
}

// *******************************************************************************************
// *
// *   operator ==
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

bool nServerInfoBase::operator ==( const nServerInfoBase & other ) const
{
    return GetAddress() == other.GetAddress() && port_ == other.port_;
}

// *******************************************************************************************
// *
// *   operator !=
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

bool nServerInfoBase::operator !=( const nServerInfoBase & other ) const
{
    return ! operator == ( other );
}

// *******************************************************************************************
// *
// *   Connect
// *
// *******************************************************************************************
//!
//!        @return     error code
//!
// *******************************************************************************************

nConnectError nServerInfoBase::Connect( nLoginType loginType, const nSocket * socket )
{
    // refuse to connect without address
    if ( !GetAddress().IsSet() )
    {
        // well, not really a timeout. But we would timeout if we tried to connect.
        return nTIMEOUT;
    }

    //unsigned int portBack = sn_clientPort;
    //sn_clientPort = port_;
    nConnectError error = sn_Connect( GetAddress(), loginType, socket );
    //sn_clientPort = portBack;

    return error;
}

// *******************************************************************************************
// *
// *   CopyFrom
// *
// *******************************************************************************************
//!
//!        @param  other
//!
// *******************************************************************************************

void nServerInfoBase::CopyFrom( const nServerInfoBase & other )
{
    port_               = other.port_;
    connectionName_     = other.connectionName_;
}

// *******************************************************************************************
// *
// *   operator =
// *
// *******************************************************************************************
//!
//!        @param  other
//!        @return
//!
// *******************************************************************************************

nServerInfoBase & nServerInfoBase::operator =( const nServerInfoBase & other )
{
    CopyFrom( other );
    return *this;
}

// *******************************************************************************************
// *
// *   DoNetWrite
// *
// *******************************************************************************************
//!
//!        @param  m message to write to
//!
// *******************************************************************************************

void nServerInfoBase::DoNetWrite( nMessage & m ) const
{
    NetWriteThis( m );
}

// *******************************************************************************************
// *
// *   DoNetRead
// *
// *******************************************************************************************
//!
//!        @param  m message to read from
//!
// *******************************************************************************************

void nServerInfoBase::DoNetRead( nMessage & m )
{
    NetReadThis( m );
}

// *******************************************************************************************
// *
// *	NetWriteThis
// *
// *******************************************************************************************
//!
//!        @param  m message to write to
//!
// *******************************************************************************************

void nServerInfoBase::NetWriteThis( nMessage & m ) const
{
    m << port_;            // write the port
    m << connectionName_;  // and the name
}

// reads a string, filtering out unwanted characters regardless of the network mode
static void sn_ReadFiltered( nMessage & m, tString & s )
{
    tColoredString raw;
    m >> raw;
    raw.NetFilter();
    s = raw;
}

// *******************************************************************************************
// *
// *	NetReadThis
// *
// *******************************************************************************************
//!
//!		@param	m   message to read from
//!
// *******************************************************************************************

void nServerInfoBase::NetReadThis( nMessage & m )
{
    m >> port_;                            // get the port
    sn_ReadFiltered( m, connectionName_ ); // get the connection name

    if ( ( sn_IsMaster || sn_AcceptingFromBroadcast || sn_AcceptingFromMaster ) && connectionName_.Len()>1 ) // no valid name (must come directly from the server who does not know his own address)
    {
        // resolve DNS
        connectionName_ = S_LocalizeName( connectionName_ );
    }
    else
    {
#ifdef DEBUG_X
        if ( connectionName_.Len() > 1 )
        {
            std::cout << "Overwriting source from " << connectionName_ << ".\n";
        }
#endif

        sn_GetAdr( m.SenderID(), connectionName_ );

        // remove the port
        for (int i=connectionName_.Len(); i>=0; i--)
            if (':' == connectionName_[i])
            {
                connectionName_[i] = '\0';
                connectionName_.SetLen(i+1);
            }

        S_GlobalizeName( connectionName_ );
    }
}

// the official IP address or DNS hostname of this server. If left blank, the recipient of the
// message will figure the IP out on its own.
static tString net_dns("");

static tConfItemLine sn_sbtip_official("SERVER_DNS", net_dns);

tString const & sn_GetMyDNSName()
{
    return net_dns;
}

// *******************************************************************************************
// *
// *    DoGetFrom
// *
// *******************************************************************************************
//!
//!     @param socket   socket to get bare network information from
//!
// *******************************************************************************************

void nServerInfoBase::DoGetFrom( nSocket const * socket )
{
    // better not set connection name, the message recipient can figure it out more reliably
    connectionName_ = net_dns;

    if ( ! socket )
    {
        port_ = sn_GetServerPort();
    }
    else
    {
        // fill port information from socket
        nAddress const & address = socket->GetAddress();
        port_ = address.GetPort();
    }
}

// *******************************************************************************************
// *
// *   DoNetWrite
// *
// *******************************************************************************************
//!
//!        @param  m message to write to
//!
// *******************************************************************************************

void nServerInfo::DoNetWrite( nMessage & m ) const
{
    nServerInfoBase::DoNetWrite( m );
    NetWriteThis( m );
}

// *******************************************************************************************
// *
// *   DoNetRead
// *
// *******************************************************************************************
//!
//!        @param  null
//!
// *******************************************************************************************

void nServerInfo::DoNetRead( nMessage & m )
{
    nServerInfoBase::DoNetRead( m );
    NetReadThis( m );
}

// *******************************************************************************************
// *
// * DoGetFrom
// *
// *******************************************************************************************
//!
//!     @param socket   socket to get bare network information from
//!
// *******************************************************************************************

void nServerInfo::DoGetFrom( nSocket const * socket )
{
    // delegate
    nServerInfoBase::DoGetFrom( socket );

    // fill

    users           = sn_NumRealUsers();
    version_        = sn_CurrentVersion();
    release_        = sn_programVersion;

#ifdef WIN32
    release_ += " win";
#else
#ifdef MACOSX
    release_ += " mac";
#else
#endif
    release_ += " unix";
#endif

#ifdef DEDICATED
    release_ += " dedicated";
#else
    release_ += " hybrid";
#endif

    maxUsers_       = sn_MaxUsers();

    // filter newlines and stuff from the server name
    tColoredString filteredName( sn_serverName );
    filteredName.NetFilter();
    name            = filteredName;

    if ( nServerInfoAdmin::GetAdmin() )
    {
        userNames_  = nServerInfoAdmin::GetAdmin()->GetUsers();
        userGlobalIDs_  = nServerInfoAdmin::GetAdmin()->GetGlobalIDs();
        options_    = nServerInfoAdmin::GetAdmin()->GetOptions();
        url_        = nServerInfoAdmin::GetAdmin()->GetUrl();
    }
    else
    {
        tString str("UNKNOWN");

        userNames_  = str;
        options_    = str;
        url_        = str;
        userGlobalIDs_ = "";
    }
}

// *******************************************************************************************
// *
// *	NetWriteThis
// *
// *******************************************************************************************
//!
//!        @param  m message to write to
//!
// *******************************************************************************************

void nServerInfo::NetWriteThis( nMessage & m ) const
{
    m << name;
    m << users;

    m << version_;
    m << release_;

    m << maxUsers_;

    m << userNames_;
    m << options_;
    m << url_;

    m << userGlobalIDs_;
}

// *******************************************************************************************
// *
// *	NetReadThis
// *
// *******************************************************************************************
//!
//!		@param	m   message to read from
//!
// *******************************************************************************************

void nServerInfo::NetReadThis( nMessage & m )
{
    tString oldName = name;

    sn_ReadFiltered( m, name  ); // get the server name
    m >> users;                 // get the playing users

    if ( !m.End() )
    {
        m >> version_;
        sn_ReadFiltered( m, release_ );
        login2_ = true;
    }
    else
    {
        login2_ = false;
    }

    if ( !m.End() )
    {
        m >> maxUsers_;
    }

    if ( !m.End() )
    {
        m >> userNames_;
        m >> options_;
        sn_ReadFiltered( m, url_ );

        if (options_.Len() > 240)
            options_.SetPos( 240, true );

        if (url_.Len() > 75)
            url_.SetPos( 75, true );
    }
    else
    {
        userNames_ = "No Info\n";
        options_ = "No Info\n";
        url_ = "No Info\n";
    }
    if ( !m.End() )
    {
        m >> userGlobalIDs_;
    }
    else
    {
        userGlobalIDs_ = "";
    }

    userNamesOneLine_.Clear();
    for ( int i = 0, j = 0; i < userNames_.Len()-1 ; ++i )
    {
        char c = userNames_[i];
        if ( c == '\n' )
        {
            userNamesOneLine_ << "0xffffff";
            if( j < userGlobalIDs_.Len()-2 && userGlobalIDs_[j] != '\n' ) {
                userNamesOneLine_ << " (";
                do
                {
                    userNamesOneLine_ << userGlobalIDs_[j];
                }
                while ( ++j < userGlobalIDs_.Len()-1 && userGlobalIDs_[j] != '\n' );
                userNamesOneLine_ << ")";
            }
            ++j;
            if ( i < userNames_.Len()-2 )
                userNamesOneLine_ << ", ";
        }
        else
            userNamesOneLine_ << c;
    }

    timesNotAnswered = 0;

    if (!advancedInfoSet)
    {
        if ( sn_IsMaster )
        {
            if ( !advancedInfoSetEver )
            {
                con << "Acknowledged server: " <<  ToString( *this ) << ", name: \"" << tColoredString::RemoveColors(name) << "\"\n";
                con << "\n";
                Save();
            }
            else if ( name != oldName )
            {
                con << "Name of server " <<  ToString( *this )
                << " changed from \"" << tColoredString::RemoveColors(oldName)
                << "\" to \"" << tColoredString::RemoveColors(name) << "\"\n";
            }

            // broadcast the server to the other master servers
            nServerInfo * master = GetMasters();
            while( master )
            {
                tJUST_CONTROLLED_PTR< nMessage > ret = tNEW(nMessage)(SmallServerDescriptor);

                // get small server info
                nServerInfoBase::NetWriteThis(*ret);
                unsigned int notrans = 0;
                *ret << notrans;
                ret->ClearMessageID();

                // send message
                sn_Bend( master->GetAddress() );
                ret->SendImmediately(0, false);
                nMessage::SendCollected(0);

                master = master->Next();
            }
        }

        advancedInfoSet = true;
        advancedInfoSetEver = true;
        ping = tSysTimeFloat() - timeQuerySent;

        CalcScore();
    }

    queried = 0;

    //  queried = true;
    sn_Polling.Remove(this, pollID);
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@return		the network address of the server
//!
// *******************************************************************************************

nAddress const & nServerInfoBase::GetAddress( void ) const
{
    return this->AccessAddress();
}

// *******************************************************************************************
// *
// *	GetAddress
// *
// *******************************************************************************************
//!
//!		@param	address	the network address of the server to fill
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase const & nServerInfoBase::GetAddress( nAddress & address ) const
{
    address = this->AccessAddress();
    return *this;
}

// *******************************************************************************************
// *
// *	ClearAddress
// *
// *******************************************************************************************
//!
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase const & nServerInfoBase::ClearAddress() const
{
    std::auto_ptr< nAddress > clearedAddress;
    address_ = clearedAddress;
    return *this;
}

// *******************************************************************************************
// *
// *	SetAddress
// *
// *******************************************************************************************
//!
//!		@param	address	the network address of the server to set
//!		@return		A reference to this to allow chaining
//!
// *******************************************************************************************

nServerInfoBase & nServerInfoBase::SetAddress( nAddress const & address )
{
    AccessAddress() = address;
    return *this;
}

// *******************************************************************************************
// *
// *	AccessAddress
// *	This function is dangerous; use only if you absolutely have to and do not store the returned reference longer than required.
// *
// *******************************************************************************************
//!
//!		@return		the network address of the server as a modifiable reference
//!
// *******************************************************************************************

nAddress & nServerInfoBase::AccessAddress( void ) const
{
    // create address if it is not already there
    if ( !this->address_.get() )
    {
        std::auto_ptr< nAddress > address( tNEW( nAddress ) );

        // fill it with hostname and port
        address->SetHostname( this->GetConnectionName() );
        address->SetPort( this->GetPort() );

        this->address_ = address;

#ifdef DEBUG
        tString unresolved = ToString( *this );
        tString resolved = this->address_->ToString();
        if ( unresolved != resolved )
            con << "Address of server " << unresolved << " determined to be " << resolved << "\n";
#endif
    }

    return *this->address_;
}

// *******************************************************************************************
// *
// *	DoGetName
// *
// *******************************************************************************************
//!
//!        @return     this server's name
//!
// *******************************************************************************************

const tString & nServerInfoBase::DoGetName( void ) const
{
    return connectionName_;
}

// *******************************************************************************************
// *
// *	DoGetName
// *
// *******************************************************************************************
//!
//!        @return     this server's name
//!
// *******************************************************************************************

const tString & nServerInfo::DoGetName( void ) const
{
    return name;
}


