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

#include "nAuthentication.h"
#include "tMemManager.h"
#include "tToDo.h"
#include "tLocale.h"
#include "tRecorder.h"
#include "tSysTime.h"

#include "nNetwork.h"
#include "nNetObject.h"
#include "nSocket.h"

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
#include <zthread/SynchronousExecutor.h>
#include <zthread/ThreadedExecutor.h>
typedef ZThread::ThreadedExecutor nExecutor;
//typedef ZThread::SynchronousExecutor nExecutor;
typedef ZThread::FastMutex nMutex;
#else
typedef tNonMutex nMutex;
#endif

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

static nDescriptor nPasswordRequest(40, &nAuthentication::HandlePasswordRequest, "password_request");

static nDescriptor nPasswordAnswer(41, &nAuthentication::HandlePasswordAnswer, "password_answer");

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
    nMessage *ret = tNEW(nMessage)(nPasswordAnswer);
    nKrawall::WriteScrambledPassword(egg, *ret);
    *ret << sn_answer.username;
    *ret << sn_answer.aborted;
    *ret << sn_answer.automatic;
    *ret << sn_answer.serverAddress;
    ret->Send(0);

    s_inUse = false;
}

// receive a password request
void nAuthentication::HandlePasswordRequest(nMessage& m)
{
    if (m.SenderID() > 0 || sn_GetNetState() != nCLIENT)
        Cheater(m.SenderID());

    sn_answer = nKrawall::nPasswordAnswer();
    sn_request = nKrawall::nPasswordRequest();

    // already in the process: return without answer
    if ( s_inUse )
        return;
    s_inUse = true;

    // read salt and username from the message
    ReadSalt(m, sn_salt);

    // read the username as raw as sanely possible
    m.ReadRaw(sn_answer.username);
    sn_answer.username.NetFilter();

    m >> sn_request.message;
    if (!m.End())
    {
        m >> sn_request.failureOnLastTry;
    }
    else
    {
        sn_request.failureOnLastTry = true;
    }
    if (!m.End())
    {
        // read method, prefix and suffiox
        m >> sn_request.method;
        m.ReadRaw(sn_request.prefix);
        m.ReadRaw(sn_request.suffix);
        sn_request.prefix.NetFilter();
        sn_request.suffix.NetFilter();
    }
    else
    {
        // clear them
        sn_request.method = "bmd5";
        sn_request.prefix = "";
        sn_request.suffix = "";
    }

    // postpone the answer for a better opportunity since it
    // most likely involves opening a menu and waiting a while (and we
    // are right now in the process of fetching network messages...)
    st_ToDo(&FinishHandlePasswordRequest);
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
#ifdef HAVE_LIBZTHREAD
        // schedule the task into a background thread
        static nExecutor executor;
        if ( !tRecorder::IsRunning() )
        {
            executor.execute( ZThread::Task( new nMemberFunctionRunnerTemplate( object, function ) ) );
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
#ifdef HAVE_LIBZTHREAD
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

#ifdef HAVE_LIBZTHREAD
    // queue of foreground tasks
     static ZThread::LockedQueue< nMemberFunctionRunnerTemplate, ZThread::FastMutex > & Pending()
     {
         static ZThread::LockedQueue< nMemberFunctionRunnerTemplate, ZThread::FastMutex > pending;
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
#ifdef HAVE_LIBZTHREAD
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
    {
        // install self reference to keep this object alive
        selfReference_ = this;

    // inform the user about delays
        bool delays = false;
#ifdef HAVE_LIBZTHREAD
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
    void ProcessClientAnswer( nMessage & answer );

    // sanity check the server address
    bool CheckServerAddress( nMessage & m );

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
};



// That function triggers fetching of authentication relevant data from the authentication
// server in this function which is supposed to run in the background:
void nLoginProcess::FetchInfoFromAuthority()
{
    // set method to defaults
    method.method = "bmd5";
    method.prefix = "";
    method.suffix = "";
    
    bool ret;
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
    tRecorder::Playback( section, authority );
    tRecorder::Record( section, ret );
    tRecorder::Record( section, method.method );
    tRecorder::Record( section, authority );

    if ( !ret )
    {
        return;
    }

    // and go on
    nMemberFunctionRunner::ScheduleForeground( *this, &nLoginProcess::QueryFromClient );
}

static bool sn_supportRemoteLogins = false;
static tSettingItem< bool > sn_supportRemoteLoginsConf( "GLOBAL_ID", sn_supportRemoteLogins );

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
            int c = in.get();
            
            // is the authority an abreviation?
            bool shortcut = true;

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
                        shortcut = false;
                        slash = true;
                        inHostName = false;
                    }
                    else
                    {
                        return ReportAuthorityError( tOutput( "$login_error_invalidurl_illegal_hostname", authority ) );
                    }
                }
                else if ( inPort )
                {
                    if ( c == '/' )
                    {
                        shortcut = false;
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
                        if (!isalnum(c) && c != '.' && c != '~' )
                        {
                            return ReportAuthorityError( tOutput( "$login_error_invalidurl_illegal_path", authority )  );
                        }

                        slash = false;
                    }
                }

                // shorthand authority must consist of lowercase letters only
                outShort.put(tolower(c));

                outFull.put(c);
                
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
    if ( userID <= 0 )
        return;

    // create a random salt value
    nKrawall::RandomSalt(salt);
    
    // send the salt value and the username to the
    nMessage *m = tNEW(nMessage)(::nPasswordRequest);
    nKrawall::WriteSalt(salt, *m);
    *m << username;
    *m << static_cast<tString>(message);
    *m << nLoginPersistence::Find( userID ).userAuthFailedLastTime;
    
    // write method info
    *m << method.method;
    *m << method.prefix;
    *m << method.suffix;
    
    m->Send(userID);

    // well, then we wait for the answer.
    con << tOutput( "$login_message_responded", userID, username, method.method, message );
}

// authentication data received from the client is processed here:
void nLoginProcess::ProcessClientAnswer( nMessage & m )
{
    success = false;
    
    // read password and username from remote
    nKrawall::ReadScrambledPassword(m, hash);

    m.ReadRaw(username);
    username.NetFilter();

    aborted = false;
    automatic = false;
    if ( !m.End() )
    {
        m >> aborted;
    }
    if ( !m.End() )
    {
        m >> automatic;
    }
    if (!m.End())
    {
        // read the server address the client used for scrambling
        m >> serverAddress;

        // sanity check it, of course :)
        if ( !CheckServerAddress( m ) )
        {
            // no use going on, the server address won't match, password checking will fail.
            return;
        }
    }
    else
    {
        serverAddress = sn_GetMyAddress();

        if ( method.method != "bmd5" )
        {
            con << "WARNING, client did not send the server address. Password checks may fail.\n";
        }
    }
  
    // and go on
    nMemberFunctionRunner::ScheduleMayBlock( *this, &nLoginProcess::Authorize, authority != "" );
}

static bool sn_trustLAN = false;
static tSettingItem< bool > sn_TrustLANConf( "TRUST_LAN", sn_trustLAN );

// sanity check the server address
bool nLoginProcess::CheckServerAddress( nMessage & m )
{
    // check whether we can read our IP from the socket
    nSocket const * socket = sn_Connections[m.SenderID()].socket;
    if ( socket )
    {
        tString compareAddress = socket->GetAddress().ToString();
        if ( !compareAddress.StartsWith("*") && compareAddress == serverAddress )
        {
            // everything is fine, adresses match
            return true;
        }
    }

    // check the incoming address, clients from the LAN should be safe
    if ( sn_trustLAN )
    {
        tString peerAddress;
        sn_GetAdr( m.SenderID(), peerAddress );
        if ( sn_IsLANAddress( peerAddress ) && sn_IsLANAddress( serverAddress ) )
        {
            return true;
        }
    }

    else if ( sn_GetMyAddress() == serverAddress )
    {
        // all's well
        return true;
    }

    tString hisServerAddress = serverAddress;
    serverAddress = sn_GetMyAddress();

    // if we don't know our own address, 
    if ( sn_GetMyAddress().StartsWith("*") )
    {
        // reject authentication.
        return ReportAuthorityError( tOutput("$login_error_pharm", hisServerAddress, sn_GetMyAddress() ) );
    }

    // reject authentication.
    return ReportAuthorityError( tOutput("$login_error_pharm", hisServerAddress, sn_GetMyAddress() ) );
}

// and here we go again: a background task talks with the authority
// and determines whether the client is authorized or not.
void nLoginProcess::Authorize()
{
    if ( aborted )
    {
        success = false;
        
        error = tOutput("$login_error_aborted");
    }
    else
    {
        if ( !tRecorder::IsPlayingBack() )
        {
            nKrawall::CheckScrambledPassword( *this, *this );
        }

        // record and playback result (required because on playback, a new
        // salt is generated and this way, a recoding does not contain ANY
        // exploitable information for password theft: the scrambled password
        // stored in the incoming network stream has an unknown salt value. )
        static char const * section = "AUTH_RESULT";
        tRecorder::Playback( section, success );
        tRecorder::Playback( section, authority );
        tRecorder::Record( section, success );
        tRecorder::Record( section, authority );
    }
    
    Abort();
}

// the finish task can also be triggered any time by this function:
void nLoginProcess::Abort()
{
    nMemberFunctionRunner::ScheduleForeground( *this, &nLoginProcess::Finish );
}

// which, when finished, triggers the foreground task of updating the
// game state and informing the client of the success of the operation.
void nLoginProcess::Finish()
{
    // again, userID is safe in this function
    int userID = sn_UserID( user );
    if ( userID <= 0 )
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
    
void nAuthentication::HandlePasswordAnswer(nMessage& m)
{
#ifdef KRAWALL_SERVER
    // find login pricess
    nLoginProcess * process = nLoginProcess::Find( m.SenderID() );

    // and delegate to it
    if ( process )
    {
        process->ProcessClientAnswer( m );
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
}

