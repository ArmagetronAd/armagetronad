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

#include "nAuthentication.h"
#include "tMemManager.h"
#include "tToDo.h"
#include "tLocale.h"
#include "tRecorder.h"
#include "tSysTime.h"

#include "nNetwork.h"
#include "nNetObject.h"
#include "nSocket.h"
#include "nServerInfo.h"
#include "nProtoBuf.h"

#include "nAuthentication.pb.h"

#include <memory>
#include <string>
#include <string.h>

#include <deque>

#ifdef HAVE_LIBZTHREAD
#include <zthread/Thread.h>
#include <zthread/LockedQueue.h>
//#include <zthread/ClassLockable.h>
#include <zthread/FastMutex.h>
#include <zthread/FastRecursiveMutex.h>
#include <zthread/Guard.h>
// #include <zthread/SynchronousExecutor.h>
#include <zthread/ThreadedExecutor.h>
typedef ZThread::ThreadedExecutor nExecutor;
//typedef ZThread::SynchronousExecutor nExecutor;
typedef ZThread::FastMutex nMutex;
#define nQueue ZThread::LockedQueue
#elif defined(HAVE_PTHREAD)
#include "pthread-binding.h"
typedef tPThreadMutex nMutex;
#define nQueue tPThreadQueue
#else
typedef tNonMutex nMutex;
#endif

bool sn_supportRemoteLogins = false;

// authority black and whitelists
static tString sn_AuthorityBlacklist, sn_AuthorityWhitelist;
tConfItemLine  sn_AuthorityBlacklistConf( "AUTHORITY_BLACKLIST", sn_AuthorityBlacklist );
tConfItemLine  sn_AuthorityWhitelistConf( "AUTHORITY_WHITELIST", sn_AuthorityWhitelist );

#ifdef DEBUG
// list of authorities that get accepted as valid authorities, no questions asked
static tString sn_AuthorityNoCheck;
tConfItemLine  sn_AuthorityNoCheckConf( "AUTHORITY_NO_CHECK", sn_AuthorityNoCheck );
#endif

static nAuthentication::UserPasswordCallback* S_UserPasswordCallback = NULL;
static nAuthentication::LoginResultCallback*  S_LoginResultCallback  = NULL;

// let the game register the callbacks
void nAuthentication::SetUserPasswordCallback(nAuthentication::UserPasswordCallback* callback)
{
    S_UserPasswordCallback = callback;
}

void nAuthentication::SetLoginResultCallback (nAuthentication::LoginResultCallback* callback)
{
    S_LoginResultCallback = callback;
}

// network handler declarations

static nProtoBufDescriptor< Network::PasswordRequest > sn_passwordRequestDescriptor(40, &nAuthentication::HandlePasswordRequest );

static nProtoBufDescriptor< Network::PasswordAnswer > sn_passwordAnswerDescriptor(41, &nAuthentication::HandlePasswordAnswer );

// password request and answer
static nKrawall::nPasswordRequest sn_request;
static nKrawall::nPasswordAnswer sn_answer;
static nKrawall::nSalt sn_salt;
static int s_inUse = false;

// finish the request for username and password
static void FinishHandlePasswordRequest()
{
    nKrawall::nScrambledPassword egg;

    // if the callback exists, get the scrambled password of the wanted user
    if (S_UserPasswordCallback)
        (*S_UserPasswordCallback)( sn_request, sn_answer );

    // scramble the salt with the server address
    sn_GetAdr( 0, sn_answer.serverAddress );
    sn_request.ScrambleSalt( sn_salt, sn_answer.serverAddress );

    // scramble it with the given salt
    sn_request.ScrambleWithSalt( nKrawall::nScrambleInfo(sn_answer.username), sn_answer.scrambled, sn_salt, egg);

    // destroy the original password
    sn_answer.scrambled.Clear();

    // and send it back
    Network::PasswordAnswer & answer = sn_passwordAnswerDescriptor.Send(0);
    nKrawall::WriteScrambledPassword(egg, *answer.mutable_answer() );
    answer.set_username_raw( sn_answer.username );
    answer.set_aborted( sn_answer.aborted );
    answer.set_automatic( sn_answer.automatic );
    answer.set_server_address( sn_answer.serverAddress );

    s_inUse = false;
}

// receive a password request
void nAuthentication::HandlePasswordRequest( Network::PasswordRequest const & request, nSenderInfo const & sender )
{
    if ( sender.SenderID() > 0 || sn_GetNetState() != nCLIENT )
        Cheater( sender.SenderID() );

    sn_answer = nKrawall::nPasswordAnswer();
    sn_request = nKrawall::nPasswordRequest();

    // already in the process: return without answer
    if ( s_inUse )
        return;
    s_inUse = true;

    // read salt and username from the message
    ReadSalt( request.nonce(), sn_salt );

    // read the username as raw as sanely possible
    sn_answer.username = request.username_raw();
    sn_request.message = request.message();

    sn_request.failureOnLastTry = request.fail_last();

    sn_request.method = request.method();
    sn_request.prefix = request.prefix_raw();
    sn_request.suffix = request.suffix_raw();

    // postpone the answer for a better opportunity since it
    // most likely involves opening a menu and waiting a while (and we
    // are right now in the process of fetching network messages...)
    st_ToDo( &FinishHandlePasswordRequest );
}

#ifdef KRAWALL_SERVER

static int sn_UserID( nNetObject * o )
{
    if ( !o )
    {
        return -1;
    }
    return o->Owner();
}

class nLoginProcess;

//! persistent information between login processes
class nLoginPersistence: 
    public nMachineDecorator
{
    friend class nLoginProcess;

    nLoginPersistence( int userID )
    : nMachineDecorator( nMachine::GetMachine( userID ) ),
      userAuthFailedLastTime( false )
    {
    }

    static nLoginPersistence & Find( int userID )
    {
        nMachine & machine = nMachine::GetMachine( userID );
        nLoginPersistence * ret = machine.GetDecorator< nLoginPersistence >();
        if ( !ret )
        {
            ret = new nLoginPersistence( userID );
        }

        return *ret;
    }
    
    virtual void OnDestroy()
    {
        delete this;
    }

    bool userAuthFailedLastTime;
};

//! template that runs void member functions of reference countable objects
template< class T > class nMemberFunctionRunnerTemplate
#ifdef HAVE_LIBZTHREAD
    : public ZThread::Runnable
#endif
{
private:
#if defined(HAVE_PTHREAD) && !defined(HAVE_LIBZTHREAD)
    static void* DoCall( void *o ) {
        nMemberFunctionRunnerTemplate * functionRunner = (nMemberFunctionRunnerTemplate*) o;
        ( (functionRunner->object_)->*(functionRunner->function_) )();
    }
#endif
public:
    nMemberFunctionRunnerTemplate( T & object, void (T::*function)() )
    : object_( &object ), function_( function )
    {
    }

    // runs the function
    void run()
    {
        (object_->*function_)();
    }

    //! schedule a task for execution at the next convenient break, between game rounds for example
    static void ScheduleBreak( T & object, void (T::*function)()  )
    {
        pendingForBreak_.push_back( nMemberFunctionRunnerTemplate( object, function ) );
    }

    //! schedule a task for execution in a background thread
    static void ScheduleBackground( T & object, void (T::*function)()  )
    {
#if defined(HAVE_LIBZTHREAD) || defined(HAVE_PTHREAD)
        // schedule the task into a background thread
        if ( !tRecorder::IsRunning() )
        {
#if !defined(HAVE_LIBZTHREAD)
            nMemberFunctionRunnerTemplate<T> * runner = new nMemberFunctionRunnerTemplate<T>( object, function );

            pthread_t thread;
            pthread_create(&thread, NULL, (nMemberFunctionRunnerTemplate::DoCall), (void*) runner);
#else
            static nExecutor executor;
            executor.execute( ZThread::Task( new nMemberFunctionRunnerTemplate( object, function ) ) );
#endif
        }
        else
        {
            // don't start threads when we're recording, just do the task at the next opportunity
            ScheduleBreak( object, function );

        }
#else
        // do it when you can without getting interrupted.
        ScheduleBreak( object, function );
#endif
    }

    //! schedule a task for execution in the next tToDo call
    static void ScheduleForeground( T & object, void (T::*function)()  )
    {
#if defined(HAVE_LIBZTHREAD) || defined(HAVE_PTHREAD)
        Pending().add( nMemberFunctionRunnerTemplate( object, function ) );
        st_ToDo( FinishAll );
#else
        // execute it immedeately
        (object.*function)();
#endif

    }

    // function that calls tasks scheduled for the next break
    static void OnBreak()
    {
        // finish all pending tasks
        while( pendingForBreak_.size() > 0 )
        {
            // sync the network so built up auth requests don't cause connection drops so quickly
            sn_Receive();
            nNetObject::SyncAll();
            tAdvanceFrame();
            sn_SendPlanned();

            nMemberFunctionRunnerTemplate & next = pendingForBreak_.front();
            next.run();
            pendingForBreak_.pop_front();
        }
    }
private:
    //! pointer to the object we should so something with
    tJUST_CONTROLLED_PTR< T > object_;
    
    //! the function to call
    void (T::*function_)();

    // taks for the break
    static std::deque< nMemberFunctionRunnerTemplate > pendingForBreak_;

#if defined(HAVE_LIBZTHREAD) || defined(HAVE_PTHREAD)
    // queue of foreground tasks
    static nQueue< nMemberFunctionRunnerTemplate, nMutex > & Pending()
    {
        static nQueue< nMemberFunctionRunnerTemplate, nMutex > pending;
        return pending;
    }

    // function that calls them
    static void FinishAll()
    {
        // finish all pending tasks
        while( Pending().size() > 0 )
        {
            nMemberFunctionRunnerTemplate next = Pending().next();
            next.run();
        }
    }
#endif
};

template< class T >
std::deque< nMemberFunctionRunnerTemplate<T> >
nMemberFunctionRunnerTemplate<T>::pendingForBreak_;

// convenience wrapper
class nMemberFunctionRunner
{
public:
    enum ScheduleType
    {
        Break,
        Foreground,
        Background
    };

    template< class T > static void ScheduleBreak( T & object, void (T::*function)() )
    {
        nMemberFunctionRunnerTemplate<T>::ScheduleBreak( object, function );
    }

    template< class T > static void ScheduleBackground( T & object, void (T::*function)() )
    {
        nMemberFunctionRunnerTemplate<T>::ScheduleBackground( object, function );
    }

    template< class T > static void ScheduleForeground( T & object, void (T::*function)() )
    {
        nMemberFunctionRunnerTemplate<T>::ScheduleForeground( object, function );
    }

    template< class T > static void ScheduleMayBlock( T & object, void (T::*function)(), bool block )
    {
        if ( block )
        {
#if defined(HAVE_LIBZTHREAD) || defined(HAVE_PTHREAD)
            ScheduleBackground( object, function );
#else
            ScheduleBreak( object, function );
#endif
        }
        else
        {
            ScheduleForeground( object, function );
        }
    }
};


//! manager for logon processes
class nLoginProcess: 
    public nMachineDecorator, 
    public nKrawall::nCheckResult,
    public nKrawall::nPasswordCheckData,
    public tReferencable< nLoginProcess, nMutex >
{
    // reference counting pointer 
    typedef tJUST_CONTROLLED_PTR< nLoginProcess > SelfPointer;
public:
    nLoginProcess( int userID )
    : nMachineDecorator( nMachine::GetMachine( userID ) )
    , checkAddress( true )
    {
        // install self reference to keep this object alive
        selfReference_ = this;

    // inform the user about delays
        bool delays = false;
#if defined(HAVE_LIBZTHREAD) || defined(HAVE_PTHREAD)
        delays = tRecorder::IsRunning();
#endif
        if ( delays )
        {
            sn_ConsoleOut( tOutput( "$login_message_delayed" ), userID );
        }
    }

    ~nLoginProcess()
    {
    }

    // returns true if this process is still to be considered active
    bool IsActive() const
    {
        return IsInList() && sn_UserID( user ) > 0;
    }

    static nLoginProcess * Find( int userID )
    {
        nMachine & machine = nMachine::GetMachine( userID );
        return machine.GetDecorator< nLoginProcess >();
    }

    // OK, authentication goes in several steps. First, we initialize everything
    void Init( tString const & authority, tString const & username, nNetObject & user, tString const & message )
    {
        this->user = &user;
        this->username = username;
        this->message = message;
        this->authority = authority;

        clientSupportedMethods = sn_Connections[user.Owner()].supportedAuthenticationMethods_;

        nMemberFunctionRunner::ScheduleMayBlock( *this, &nLoginProcess::FetchInfoFromAuthority, authority != "" );
    }

    // That function triggers fetching of authentication relevant data from the authentication
    // server in this function which is supposed to run in the background:
    void FetchInfoFromAuthority();

    // report an authority info query error to the higher level system
    bool ReportAuthorityError( tOutput const & error )
    {
        // prepare failure report
        this->success = false;
        this->error = error;
        
        Abort();
        
        return false;
    }

    // that function again triggers the following foreground action that queries 
    // the credentials from the client. This object then goes to sleep, 
    // waiting for a client answer or logout, whichever comes first.
    void QueryFromClient();

    // authentication data received from the client is processed here:
    void ProcessClientAnswer( Network::PasswordAnswer const & answer, nSenderInfo const & sender );

    // sanity check the server address
    bool CheckServerAddress();

    // and here we go again: a background task talks with the authority
    // and determines whether the client is authorized or not.
    void Authorize();

    // which, when finished, triggers the foreground task of updating the
    // game state and informing the client of the success of the operation.
    void Finish();

    // the finish task can also be triggered any time by this function:
    void Abort();
private:
    // helper functions
    
    // fetches info from remote authority
    bool FetchInfoFromAuthorityRemote();

    // fetches info from local authority
    bool FetchInfoFromAuthorityLocal();

    tString                message;     //!< message to present to user


    tString clientSupportedMethods;     //!< methods supported by the client


    // called when the machine gets destroyed, which happens a bit after
    // the client logged out. If no process is currently running, destroy the object.
    virtual void OnDestroy()
    {
        SelfPointer keepAlive( this );
        selfReference_ = 0;
    }

    //! pointer to self to keep the object alive while the machine exists
    SelfPointer selfReference_;

    //! address of socket receiving the login message
    nAddress serverSocketAddress;
    
    //! address of login message sender
    tString peerAddress;

    //! flag indicating whether the sent server address needs checking
    bool checkAddress;
};



// That function triggers fetching of authentication relevant data from the authentication
// server in this function which is supposed to run in the background:
void nLoginProcess::FetchInfoFromAuthority()
{
    // set method to defaults
    method.method = "bmd5";
    method.prefix = "";
    method.suffix = "";
    
    if( !IsActive() )
    {
        Abort();
        return;
    }

    bool ret = false;
    if ( !tRecorder::IsPlayingBack() )
    {
        if ( authority.Len() <= 1 )
        {
            // local logins are easy, handle them first
            ret = FetchInfoFromAuthorityLocal();
        }
        else
        {
            // remote logins are harder.
            ret = FetchInfoFromAuthorityRemote();
        }
    }

    // record and playback result.
    static char const * section = "AUTH_INFO";
    tRecorder::Playback( section, ret );
    tRecorder::Playback( section, method.method );
    tRecorder::Playback( section, method.prefix );
    tRecorder::Playback( section, method.suffix );
    tRecorder::Playback( section, authority );
    tRecorder::Playback( section, error );
    tRecorder::Record( section, ret );
    tRecorder::Record( section, method.method );
    tRecorder::Record( section, method.prefix );
    tRecorder::Record( section, method.suffix );
    tRecorder::Record( section, authority );
    tRecorder::Record( section, error );

    if ( !ret )
    {
        if ( tRecorder::IsPlayingBack() )
        {
            Abort();
        }

        return;
    }

    // and go on
    nMemberFunctionRunner::ScheduleForeground( *this, &nLoginProcess::QueryFromClient );
}

static tSettingItem< bool > sn_supportRemoteLoginsConf( "GLOBAL_ID", sn_supportRemoteLogins );

// legal characters in authority hostnames(besides alnum and dots)
static bool sn_IsLegalSpecialChar( char c )
{
    switch (c)
    {
    case '-': // well, ok, this character actually happens to be in many URLs :)
    case '+': // these not, but let's consider them legal.
    case '=':
    case '_':
        return true;
    default:
        return false;
    }
}

// fetches info from remote authority
bool nLoginProcess::FetchInfoFromAuthorityRemote()
{
    if ( !sn_supportRemoteLogins )
    {
        return ReportAuthorityError( tOutput("$login_error_noremote") );
    }

    {
        // the hostname part of the authority should not contain uppercase letters
#ifdef DEBUG
        if ( tIsInList( sn_AuthorityNoCheck, authority ) )
        {
            fullAuthority = authority;
        }
        else
#endif
        {
            std::istringstream in( static_cast< const char * >( authority ) );
            std::ostringstream outShort; // stream for shorthand authority
            std::ostringstream outFull;  // stream for full authority URL that is to be used for lookups
            std::ostringstream outDirectory;
            int c = in.get();

            // is the authority an abreviation?
            bool shortcut = true;
            // does it contain a path?
            bool hasPath = false;

            // is the server a raw IP?
            bool rawIP = true;

            // which part we're currently parsing
            bool inHostName = true;
            bool inPort = false;
            bool slash = false;
            int port = 0;

            while( !in.eof() )
            {
                if ( inHostName )
                {
                    // check validity of hostname part
                    if ( c == '.' )
                    {
                        shortcut = false;
                    }
                    else if ( isalnum(c) )
                    {
                        c = tolower(c);
                        if ( !isdigit( c ) )
                        {
                            rawIP = false;
                        }
                    }
                    else if ( c == ':' )
                    {
                        inPort = true;
                        inHostName = false;
                    }
                    else if ( c == '/' )
                    {
                        slash = true;
                        inHostName = false;
                    }
                    else if ( !sn_IsLegalSpecialChar(c) )
                    {
                        return ReportAuthorityError( tOutput( "$login_error_invalidurl_illegal_hostname", authority ) );
                    }
                }
                else if ( inPort )
                {
                    if ( c == '/' )
                    {
                        inPort = false;
                        slash = true;
                    }
                    else if ( !isdigit( c ) )
                    {
                        return ReportAuthorityError( tOutput( "$login_error_invalidurl_illegal_port", authority ) );
                    }
                    else
                    {
                        port *= 10;
                        port += c - '0';
                    }
                }
                else // must be in path
                {
                    if ( c == '/' )
                    {
                        if ( slash )
                        {
                            return ReportAuthorityError( tOutput( "$login_error_invalidurl_slash", authority ) );
                        }

                        slash = true;
                    }
                    else
                    {
                        if (!isalnum(c) && c != '.' && c != '~' && !sn_IsLegalSpecialChar(c) )
                        {
                            return ReportAuthorityError( tOutput( "$login_error_invalidurl_illegal_path", authority )  );
                        }

                        slash = false;
                        hasPath = true;
                    }
                }

                // shorthand authority must consist of lowercase letters only
                if( inHostName || inPort )
                {
                    outShort.put(tolower(c));
                    outFull.put( (char) c );
                }
                else
                {
                    outDirectory.put( (char) c);
                }


                c = in.get();
            }
            if ( slash )
            {
                return ReportAuthorityError( tOutput( "$login_error_invalidurl_slash", authority ) );
            }
            if ( port == 80 )
            {
                return ReportAuthorityError( tOutput( "$login_error_invalidurl_defaultport", authority ) );
            }

            if ( rawIP )
            {
                return ReportAuthorityError( tOutput( "$login_error_invalidurl_rawip", authority ) );
            }


            authority = outShort.str().c_str();
            fullAuthority = outFull.str().c_str();

            static const char * def = ".authentication.armagetronad.net";

            // append default authority path
            if ( authority.Len() > 1 && shortcut )
            {
                fullAuthority += def;
            }

            // check if the pased authority contains the default ending
            if ( !shortcut && authority.Reverse().StartsWith( tString( def ).Reverse() ) )
            {
                // strip it
                authority = authority.SubStr( 0, authority.Len() - strlen( def ) - 1 );
                shortcut = true;
            }

            if( hasPath )
            {
                fullAuthority += outDirectory.str().c_str();
                authority += outDirectory.str().c_str();
            }
        }

        // check for authority in black and whitelist
        if ( tIsInList( sn_AuthorityBlacklist, authority ) )
        {
            return ReportAuthorityError( tOutput( "$login_error_blacklist", authority ) );
        }

        if ( sn_AuthorityWhitelist != "" && !tIsInList( sn_AuthorityWhitelist, authority ) )
        {
            return ReportAuthorityError( tOutput( "$login_error_whitelist", authority ) );
        }

        // try yo find a better method, fetch method list
        std::stringstream answer;
        int rc = nKrawall::FetchURL( fullAuthority, "?query=methods", answer );

        if ( rc == -1 )
        {
            return ReportAuthorityError( tOutput( "$login_error_invalidurl_notfound", authority ) );
        }
         
        tString id;
        answer >> id;
        tToLower(id);

        tString methods;
        std::ws(answer);
        methods.ReadLine( answer );
        tToLower(methods);

        if ( rc != 200 || id != "methods" )
        {
            return ReportAuthorityError( tOutput( "$login_error_nomethodlist", authority, rc, id + " " + methods ) );
        }
       

        method.method = nKrawall::nMethod::BestMethod( 
            methods,
            clientSupportedMethods
            );
        
        // check whether a method can be found
        if ( method.method.Len() <= 1 )
        {
            return ReportAuthorityError(
                tOutput( "$login_error_nomethod", 
                         clientSupportedMethods,
                         nKrawall::nMethod::SupportedMethods(),
                         methods )
                );
        }
    }
        
    // fetch md5 prefix and suffix
    {
        std::ostringstream query;
        query << "?query=params";
        query << "&method=" << nKrawall::EncodeString( method.method );
        std::ostringstream data;
        int rc = nKrawall::FetchURL( fullAuthority, query.str().c_str(), data );
        
        if ( rc != 200 )
        {
            if ( rc == -1 )
            {
                return ReportAuthorityError( tOutput( "$login_error_invalidurl_notfound", authority ) );
            }
            
            return ReportAuthorityError( tOutput( "$login_error_nomethodproperties", authority, rc, data.str().c_str() ) );
        }
        
        // read the properties
        std::istringstream read( data.str() );
        method = nKrawall::nMethod( static_cast< char const * >( method.method ), read );
    }
    
    return true;
}

// fetches info from local authority
bool nLoginProcess::FetchInfoFromAuthorityLocal()
{
    // try yo find a better method
    if ( !nKrawall::nMethod::BestLocalMethod( 
             clientSupportedMethods,
             method
             )
        )
    {
        return ReportAuthorityError(
            tOutput( "$login_error_nomethod", 
                     clientSupportedMethods,
                     nKrawall::nMethod::SupportedMethods(),
                     nKrawall::nMethod::SupportedMethods() )
            );
    }
    
    return true;
}

// that function again triggers the following foreground action that queries 
// the credentials from the client. This object then goes to sleep, 
// waiting for a client answer or logout, whichever comes first.
void nLoginProcess::QueryFromClient()
{
    // check whether the user disappeared by now (this is run in the main thread,
    // so no risk of the user disconnecting while the function runs)
    int userID = sn_UserID( user );
    if ( !IsActive() )
        return;

    // create a random salt value
    nKrawall::RandomSalt(salt);
    
    // send the salt value and the username to the
    Network::PasswordRequest & request = ::sn_passwordRequestDescriptor.Send( userID ); 
    nKrawall::WriteSalt( salt, *request.mutable_nonce() );
    request.set_username_raw( username );
    request.set_message( message );
    request.set_fail_last( nLoginPersistence::Find( userID ).userAuthFailedLastTime );
    
    // write method info
    request.set_method( method.method );
    request.set_prefix_raw( method.prefix );
    request.set_suffix_raw( method.suffix );
    
    // well, then we wait for the answer.
    con << tOutput( "$login_message_responded", userID, username, method.method, message );
}

// authentication data received from the client is processed here:
void nLoginProcess::ProcessClientAnswer( Network::PasswordAnswer const & answer, nSenderInfo const & sender )
{
    success = false;
    
    // read password and username from remote
    nKrawall::ReadScrambledPassword( answer.answer(), hash );

    username = answer.username_raw();

    aborted = answer.aborted();
    automatic = answer.automatic();

    if ( answer.has_server_address() )
    {
        // read the server address the client used for scrambling
        serverAddress = answer.server_address();

        // sanity check it later
    }
    else
    {
        serverAddress = sn_GetMyAddress();
        
        if ( method.method != "bmd5" )
        {
            con << "WARNING, client did not send the server address. Password checks may fail.\n";
        }
        else
        {
            checkAddress = false;
        }
    }

    // store receiving socket address
    nSocket const * socket = sn_Connections[sender.SenderID()].socket;
    if ( !socket )
    {
        ReportAuthorityError( "Internal error, no receiving socket of authentication message." );
    }
    serverSocketAddress = socket->GetAddress();

    // store peer address
    sn_GetAdr( sender.SenderID(), peerAddress );
  
    // and go on
    nMemberFunctionRunner::ScheduleMayBlock( *this, &nLoginProcess::Authorize, authority != "" );
}

static bool sn_trustLAN = false;
static tSettingItem< bool > sn_TrustLANConf( "TRUST_LAN", sn_trustLAN );

// sanity check the server address
bool nLoginProcess::CheckServerAddress()
{
    // if no check is requested (only can happen for old bmd5 protocol), don't check.
    if ( !checkAddress )
    {
        return true;
    }

    // serverAddress given from client never can be *.*.*.*:*. This would
    // give false positive check results because some of the methods below
    // compare serverAddress to strings that may be *.*.*.*:*, and checking
    //  serverAddress once here is the safest and easiest way.
    if ( serverAddress.StartsWith("*") )
    {
        return ReportAuthorityError( tOutput("$login_error_pharm_cheap" ) );
    }

    // check whether we can read our IP from the socket
    tString compareAddress = serverSocketAddress.ToString();
    if ( compareAddress == serverAddress )
    {
        // everything is fine, adresses match
        return true;
    }

    // check the incoming address, clients from the LAN should be safe
    if ( sn_trustLAN )
    {
        if ( sn_IsLANAddress( peerAddress ) && sn_IsLANAddress( serverAddress ) )
        {
            return true;
        }
    }

    // std::cout << serverAddress;

    // fetch our server address. First, try the basic networking system.
    tString trueServerAddress = sn_GetMyAddress();

    if ( trueServerAddress == serverAddress )
    {
        // all's well
        return true;
    }

    // if SERVER_DNS is set, the client most likely will connect over that IP. Use it.
    if ( sn_GetMyDNSName().Len() > 1 )
    {
        // resolve DNS. Yes, do this every time someone logs in, IPs can change.
        // after all, that's the point of setting SERVER_DNS :)
        nAddress address;
        address.SetHostname( sn_GetMyDNSName() );
        
        address.SetPort( serverSocketAddress.GetPort() );

        // transform back to string
        trueServerAddress = address.ToString();
    }

    if ( trueServerAddress == serverAddress )
    {
        // all's well
        return true;
    }

    // Z-Man: can't remember what this swapping is for. Possibly to accept
    // the login anyway for debugging purposes.
    tString hisServerAddress = serverAddress;
    serverAddress = trueServerAddress;

    // reject authentication.
    return ReportAuthorityError( tOutput("$login_error_pharm", hisServerAddress, trueServerAddress ) );
}

// and here we go again: a background task talks with the authority
// and determines whether the client is authorized or not.
void nLoginProcess::Authorize()
{
    if( !IsActive() )
    {
        Abort();
    }

    if ( aborted )
    {
        success = false;
        
        error = tOutput("$login_error_aborted");
    }
    else
    {
        // sanity check it server address
        if ( !CheckServerAddress() )
        {
            // no use going on, the server address won't match, password checking will fail.
            return;
        }

        if ( !tRecorder::IsPlayingBack() )
        {
            nKrawall::CheckScrambledPassword( *this, *this );
        }

        // record and playback result (required because on playback, a new
        // salt is generated and this way, a recoding does not contain ANY
        // exploitable information for password theft: the scrambled password
        // stored in the incoming network stream has an unknown salt value. )
        static char const * section = "AUTH_RESULT";
        tRecorder::Playback( section, username );
        tRecorder::Playback( section, success );
        tRecorder::Playback( section, authority );
        tRecorder::Playback( section, error );
        tRecorder::Record( section, username );
        tRecorder::Record( section, success );
        tRecorder::Record( section, authority );
        tRecorder::Record( section, error );
    }
    
    Abort();
}

// the finish task can also be triggered any time by this function:
void nLoginProcess::Abort()
{
    nMemberFunctionRunner::ScheduleBackground( *this, &nLoginProcess::Finish );
}

// which, when finished, triggers the foreground task of updating the
// game state and informing the client of the success of the operation.
void nLoginProcess::Finish()
{
    // again, userID is safe in this function
    int userID = sn_UserID( user );
    if ( !IsActive() )
        return;

    // decorate console with correct sender ID
    nCurrentSenderID currentSender( userID );
    
    // store success for next time
    nLoginPersistence::Find( userID ).userAuthFailedLastTime = !success;

    // remove this decorator from public view
    Remove();
    
    if (S_LoginResultCallback)
        (*S_LoginResultCallback)( *this );

    // bye-bye!
    Destroy();
}

static void sn_Reset(){
    int userID = nCallbackLoginLogout::User();

    // kill/detach pending login process
    nLoginProcess * process = nLoginProcess::Find( userID );
    if ( process )
    {
        process->Remove();
        process->Destroy();
    }
}

static nCallbackLoginLogout reset(&sn_Reset);

#endif // KRAWALL_SERVER
    
void nAuthentication::HandlePasswordAnswer( Network::PasswordAnswer const & answer, nSenderInfo const & sender )
{
#ifdef KRAWALL_SERVER
    // find login pricess
    nLoginProcess * process = nLoginProcess::Find( sender.SenderID() );

    // and delegate to it
    if ( process )
    {
        process->ProcessClientAnswer( answer, sender );
    }
#endif
}
        
// on the server: request user authentification from login slot
bool nAuthentication::RequestLogin(const tString & authority, const tString& username, nNetObject & user, const tOutput& message )
{
#ifdef KRAWALL_SERVER
    int userID = user.Owner();
    if ( userID <= 0 )
    {
        return false;
    }

    con << tOutput( "$login_message_requested", userID, username, authority );

    // do nothing if there is another login in process for that client
    if ( nLoginProcess::Find( userID ) )
    {
        return false;
    }

    // trigger function cascade bouncing between threads
    (new nLoginProcess( userID ))->Init( authority, username, user, tString(message) );
#endif

    return true;
}

//! call when you have some time for lengthy authentication queries to servers
void nAuthentication::OnBreak()
{
#ifdef KRAWALL_SERVER
    nMemberFunctionRunnerTemplate< nLoginProcess >::OnBreak();
#endif
    st_DoToDo();
}

//! returns whether a login is currently in process for the given user ID
bool nAuthentication::LoginInProcess( nNetObject * user )
{
#ifdef KRAWALL_SERVER
    if( !user )
    {
        return false;
    }

    // fetch the process
    nLoginProcess * process = nLoginProcess::Find( user->Owner() );

    // compare the user
    return ( process && user == process->user );
#else
    return false;
#endif
}
