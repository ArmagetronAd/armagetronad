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

Although it is already included in the GPL, let me clarify:
anybody, especially the Krawall Gaming network, is allowed
to run a server/master server build from modified versions of
this file WHITHOUT releasing the source to the public (provided
the executable is not distributed).

*/

#ifndef ArmageTron_NKRAWALL_H
#define ArmageTron_NKRAWALL_H

//#include <iosfwd>
//#include <iostream>

#include "defs.h"
#include "aa_config.h"
#include "md5.h"
#include "tSafePTR.h"
#include "tString.h"
// #include "nNetObject.h"
#include "tConfiguration.h"

class nStreamMessage;
typedef nStreamMessage nMessage;

#include <deque>

class nNetObject;

class tString;

//! base class for authentication, unaware of armagetron network messages
class nKrawall
{
public:
    // the scrambled password data types
    class nScrambledPassword
    {
        friend class nKrawall;

        md5_byte_t content[16];
    public:
        md5_byte_t operator[]( int i ) const
        {
            tASSERT( i >= 0 && i < 16 );
            return content[i];
        }

        md5_byte_t & operator[]( int i )
        {
            tASSERT( i >= 0 && i < 16 );
            return content[i];
        }

        void Clear()
        {
            memset( &content, 0, sizeof(content));
        }
    };

    typedef nScrambledPassword nSalt;          // (freely changable)

    //! extra information for password scrambling
    struct nScrambleInfo
    {
        tString username;           //!< the username

        nScrambleInfo( tString const & username_ )
        : username( username_)
        {
        }
    };

    //! authentication method information
    struct nMethod
    {
        tString method;          //!< scrambling method; "bmd5" for old school
        tString prefix;          //!< thing to prepend the password before hashing it
        tString suffix;          //!< thing to append to the password before hashing it

        // returns a comma separated list of supported methods
        static tString SupportedMethods();

        // from two strings of supported-method-lists, select the best one
        static tString BestMethod( tString const & a, tString const & b );

        // scramble a password using this
        void ScramblePassword( nScrambleInfo const & info, tString const & password, nScrambledPassword & scrambled ) const;

        //! extra salt scrambling step, done on client and server, not on authentication server
        void ScrambleSalt( nSalt & salt, tString const & serverIP ) const;

        //! scramble a password hash with a salt
        void ScrambleWithSalt( nScrambleInfo const & info, nScrambledPassword const & scrambled, nSalt const & salt, nScrambledPassword & result ) const;

        //! fetch NULL-terminated list of locally supported methods
        static nMethod const * const * LocalMethods();

        //! fetch best local method supported by the client
        static bool BestLocalMethod( tString const & supportedOnClient, nMethod & result );

        //! compare two methods
        static bool Equal( nMethod const & a, nMethod const & b );

        nMethod(){}

        // construct a method from the type and a stream with properties
        // the stream is supposed to consist of lines of the "property_name property_value" form.
        nMethod( char const * method_, std::istream & properties );

        nMethod( char const * method_, char const * prefix_ = "", char const * suffix_ = "");
    };

    // structure for a password request to the user
    struct nPasswordRequest: public nMethod
    {   
        tString message;         // message to show to the user
        bool failureOnLastTry;   // did the last attempt fail?

        nPasswordRequest()
            : failureOnLastTry ( false ){}
    };

    // structure for read-write data for password requests
    struct nPasswordAnswer
    {
        tString serverAddress;         // the address of the server
        tString username;              // username, read-write property
        nScrambledPassword scrambled;  // the scrambled password
        bool aborted;                  // did the user abort the operation?
        bool automatic;                // was the answer provided automatically without user interaction?

        nPasswordAnswer()
            : aborted( false ), automatic( false ){}
    };

    struct nPasswordCheckData
    {
        tString fullAuthority;          // authority (no shorthand version); authenticated name is username@authority
        nMethod method;                 //!< method of authentication
        nSalt salt;                     //!< the salt used in the authentication process
        nScrambledPassword hash;        //!< hash as sent from client
        tString serverAddress;          //!< server address used for MITM protection
    };

    // return structure for an authentication request
    struct nCheckResultBase
    {   
        tString username;        // username as sent from client
        tString authority;       // authority (shorthand version allowed); authenticated name is username@authority
        bool success;            // was the operation successful?
        tString error;           // potential error message
        std::deque< tString > blurb; // additional blurb data the authority may give on successful login

        tAccessLevel accessLevel;// access level of user

        nCheckResultBase()
            : success( false ), accessLevel( tAccessLevel_Authenticated ){}
    };

    struct nCheckResult: public nCheckResultBase
    {   
        tJUST_CONTROLLED_PTR< nNetObject > user; // net object identifying the user (will be ePlayerNetID, but we don't know about that here)
        bool aborted;            // did the user abort the operation?
        bool automatic;          // was the answer provided automatically without user interaction?

        nCheckResult();
        ~nCheckResult();
        nCheckResult( nCheckResult const & other );
    };

    // encode scrambled passwords and salts as hexcode strings
    static tString EncodeScrambledPassword( nScrambledPassword const & scrambled );

    // encode a string for safe inclusion into an URL
    static tString EncodeString( tString const & original );

    // network read/write operations of these data types
    static void WriteScrambledPassword(const nScrambledPassword& scrambled,
                                       nMessage &m);

    static void ReadScrambledPassword( nMessage &m,
                                       nScrambledPassword& scrambled);

    // file read/write operations of these data types
    static void WriteScrambledPassword(const nScrambledPassword& scrambled,
                                       std::ostream &s);

    static void ReadScrambledPassword( std::istream &s,
                                       nScrambledPassword& scrambled);


    static void WriteSalt(const nSalt& salt,
                          nMessage &m)
    {
        WriteScrambledPassword(salt, m);
    }

    static void ReadSalt( nMessage &m,
                          nSalt& salt)
    {
        ReadScrambledPassword(m, salt);
    }

    // compare two passwords
    static bool ArePasswordsEqual(const nScrambledPassword& a,
                                  const nScrambledPassword& b);


    // scramble a password locally (so it does not have to be stored on disk)
    static void ScramblePassword(const tString& password,
                                 nScrambledPassword &scrambled);

    // scramble a password locally (so it does not have to be stored on disk), old broken method that includes the trailing \0.
    static void BrokenScramblePassword(const tString& password,
                                       nScrambledPassword &scrambled);

    // scramble it again before transfering it over the network
    static void ScrambleWithSalt2(const nScrambledPassword& source,
                                 const nSalt& salt,
                                 nScrambledPassword& dest);


    // get a random salt value
    static void RandomSalt(nSalt& salt);
#ifdef KRAWALL_SERVER
    //! split a fully qualified user name in authority and username part
    static void SplitUserName( tString const & original, tString & username, tString & authority );
    static void SplitBaseAuthorityName( tString const & authority, tString & base );
    static bool CanClaim( tString contacted, tString claimed );

    // check whether username's password, when run through ScrambleWithSalt( ScramblePassword(password), salt ), equals scrambledRemote. result.userName and result.authority need to be set by the caller, success and error are filled by this function, and authority may be modified.
    static void CheckScrambledPassword( nCheckResultBase & result,
                                        nPasswordCheckData const & data );

    // fetches an URL content, return http return code (-1 if total failure), fill result stream.
    static int FetchURL( tString const & authority, char const * query, std::ostream & target, int maxlen = 10000 );

#ifdef KRAWALL_SERVER_LEAGUE
    // secret key to encrypt server->master server league transfer
    static const nScrambledPassword& SecretLeagueKey();

    // called on the servers to create a league message
    static void SendLeagueMessage(const tString& message = *reinterpret_cast<tString *>(0));

    // called on the master server when the league message is received
    static void ReceiveLeagueMessage(const tString& message);

    // league message generation functions: fill message with the message to be
    // sent

    // called when victim drives against killer's wall
    static void Frag(const tString &killer, const tString& victim, tString& message);

    // called at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void RoundEnd(const tString* players, int numPlayers, tString& message);

    // called ON THE SERVER when victim drives against killer's wall
    static void ServerFrag(const tString &killer, const tString& victim);

    // called ON THE SERVER at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void ServerRoundEnd(const tString* players, int numPlayers);


    // called ON THE MASTER when victim drives against killer's wall
    static void MasterFrag(const tString &killer, const tString& victim);

    // called ON THE MASTER at the end of a round; the last survivor is stored in
    // players[numPlayers-1], the first death in players[0]
    static void MasterRoundEnd(const tString* players, int numPlayers);

    // Adress checking functions

    // first validity check for the league messages
    static bool IsFromKrawall(tString& adress, unsigned int port);

    // check if a user is from germany (so the master server will require
    // a password check)
    static bool RequireMasterLogin(tString& adress, unsigned int port);

    // only servers acknowledged by this funktion are from Krawall and
    // are allowed to request logins
    static bool MayRequirePassword(tString& adress, unsigned int port);
#endif
#endif
};

#endif
