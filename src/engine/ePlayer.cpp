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

#include "eEventNotification.h"
#include "tMemManager.h"
#include "ePlayer.h"
//#include "tInitExit.h"
#include "tConfiguration.h"
#include "eNetGameObject.h"
#include "rConsole.h"
#include "eTimer.h"
#include "tSysTime.h"
#include "rFont.h"
#include "uMenu.h"
#include "tToDo.h"
#include "rScreen.h"
#include <string>
#include <fstream>
#include <iostream>
#include <deque>
#include "rRender.h"
#include "rFont.h"
#include "rSysdep.h"
#include "nAuthentication.h"
#include "tDirectories.h"
#include "eTeam.h"
#include "eVoter.h"
#include "tReferenceHolder.h"
#include "tRandom.h"
#include "uInputQueue.h"
#include "nServerInfo.h"
#include "tRecorder.h"
#include "nConfig.h"
#include <time.h>
#include "tRuby.h"

tColoredString & operator << (tColoredString &s,const ePlayer &p){
    return s << tColoredString::ColorString(p.rgb[0]/15.0,
                                            p.rgb[1]/15.0,
                                            p.rgb[2]/15.0)
           << p.Name();
}

tColoredString & operator << (tColoredString &s,const ePlayerNetID &p){
    return s << p.GetColoredName();
}

std::ostream & operator << (std::ostream &s,const ePlayerNetID &p){
    return s << p.GetColoredName();
}

eAccessLevelHolder::eAccessLevelHolder()
{
    accessLevel = tAccessLevel_Default;
}

void eAccessLevelHolder::SetAccessLevel( tAccessLevel level )
{
    // sanity check. The access level must not be set higher than that of the current context.
    // since accessLevel is private, all access level changes need to run over this function;
    // it is therefore impossible to get access level escalation bugs without memory overwrites
    // or, of course, evil intentions.
    if ( level < tCurrentAccessLevel::GetAccessLevel() )
    {
        con << "INTERNAL ERROR, security violation attempted. Clamping it.\n";
        st_Breakpoint();
        accessLevel = tCurrentAccessLevel::GetAccessLevel();
    }

    accessLevel = level;
}

tCONFIG_ENUM( eCamMode );

tList<ePlayerNetID> se_PlayerNetIDs;
static ePlayer* se_Players = NULL;

bool se_assignTeamAutomatically = true;
static tSettingItem< bool > se_assignTeamAutomaticallyConf( "AUTO_TEAM", se_assignTeamAutomatically );

static bool se_allowTeamChanges = true;
static tSettingItem< bool > se_allowTeamChangesConf( "ALLOW_TEAM_CHANGE", se_allowTeamChanges );

static bool se_enableChat = true;	//flag indicating whether chat should be allowed at all (logged in players can always chat)
static tSettingItem<bool> se_enaChat("ENABLE_CHAT", se_enableChat);

static tReferenceHolder< ePlayerNetID > se_PlayerReferences;

class PasswordStorage
{
public:
    tString username; // name of the user the password belongs to
    tString methodCongested;   // method of scrambling
    nKrawall::nScrambledPassword password;
    bool save;

    PasswordStorage(): save(false){};
};

static bool operator == ( PasswordStorage const & a, PasswordStorage const & b )
{
    return
    a.username == b.username &&
    a.methodCongested == b.methodCongested;
}
    
static tArray<PasswordStorage> S_passwords;

// if set, user names of non-authenticated players are left as they are,
// and usernames of authenticated players get a 0: prepended.
// if unsed, usernames of non-authenticated players get all special characters escaped (especially all @)
// and usernames of authenticated players get left as they are (with all special characters except the last
// escaped.)
bool se_legacyLogNames = false;
static tSettingItem<bool> se_llnConf("LEGACY_LOG_NAMES", se_legacyLogNames );

// transform special characters in name to escape sequences
static std::string se_EscapeName( tString const & original, bool keepAt = true )
{
    std::ostringstream filter; 

    int lastC = 'x';
    for( int i = 0; i < original.Len()-1; ++i )
    {
        unsigned int c = static_cast< unsigned char >( original[i] );

        // a valid character
        switch (c)
        {
            // characters to escape
        case ' ':
            filter << "\\_";
            break;
        case '@':
            if ( keepAt )
            {
                filter << '@';
            }
            else
            {
                filter << "\\@";
            }
            break;
        case '\\':
        case '%':
        case ':':
            filter.put('\\');
            filter.put(c);
            break;
        case 'x':
            // color codes?
            if ( lastC == '0' )
            {
                filter << "\\x";
                break;
            }    
        default:
            if ( 0x20 < c && 0x7f >= c )
            {
                // normal ascii, print 
                filter.put(c);
            }
            else
            {
                // encode as hexcode
                filter << '%' << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << c;
            }
        }
        
        lastC = c;
    }

    // done! wrap up stream as string.
    return filter.str();
}

#ifdef KRAWALL_SERVER
// reverses se_EscapeName (left inverse only, UnEscape(Escape(x)) == x, but not Escape(UnEscape(y)) == y. )
static std::string se_UnEscapeName( tString const & original )
{
    std::ostringstream filter; 

    int lastC = 'x';
    for( int i = 0; i < original.Len()-1; ++i )
    {
        int c = original[i];

        if ( lastC == '\\' )
        {
            if ( c == '_' )
            {
                c = ' ';
            }
            filter.put(c);
        }
        else if ( c == '%' )
        {
            // decode hex code
            char hex[3];
            hex[0] = original[i+1];
            hex[1] = original[i+2];
            hex[2] = 0;
            i += 2;

            int decoded;
            std::istringstream s(hex);
            s >> std::hex >> decoded;
            tASSERT( !s.fail() );
            filter.put(decoded);
        }
        else if ( c != '\\' )
        {
            filter.put(c);
        }

        lastC = c;
    }

    // done! wrap up stream as string.
    return filter.str();
}
#endif

// generate the user name for an unauthenticated user
static tString se_UnauthenticatedUserName( tString const & name )
{
    tString ret;
    ePlayerNetID::FilterName( name, ret );
    if ( se_legacyLogNames )
    {
        return ret;
    }
    else
    {
        return tString( se_EscapeName( ret, false ).c_str() );
    }
}

void se_DeletePasswords(){
    S_passwords.SetLen(0);

    st_SaveConfig();

    /*

      REAL timeout = tSysTimeFloat() + 3;

      while(tSysTimeFloat() < timeout){

      sr_ResetRenderState(true);
      rViewport::s_viewportFullscreen.Select();

      sr_ClearGL();

      uMenu::GenericBackground();

      REAL w=16*3/640.0;
      REAL h=32*3/480.0;


      //REAL middle=-.6;

      Color(1,1,1);
      DisplayText(0,.8,w,h,tOutput("$network_opts_deletepw_complete"));

      sr_SwapGL();
      }

    */

    tConsole::Message("$network_opts_deletepw_complete", tOutput(), 5);
}

class tConfItemPassword:public tConfItemBase{
public:
    tConfItemPassword():tConfItemBase("PASSWORD"){}
    ~tConfItemPassword(){};

    // write the complete passwords
    virtual void WriteVal(std::ostream &s){
        int i;
        bool first = 1;
        for (i = S_passwords.Len()-1; i>=0; i--)
        {
            PasswordStorage &storage = S_passwords[i];
            if (storage.save )
            {
                if (!first)
                    s << "\nPASSWORD\t";
                first = false;

                s << "1 ";
                nKrawall::WriteScrambledPassword(storage.password, s);
                s << '\t' << storage.methodCongested;
                s << '\t' << storage.username;
            }
        }
        if (first)
            s << "0 ";
    }

    // read one password
    virtual void ReadVal(std::istream &s){
        //    static char in[20];
        int test;
        s >> test;
        if (test != 0)
        {
            PasswordStorage &storage = S_passwords[S_passwords.Len()];
            nKrawall::ReadScrambledPassword(s, storage.password);
            s >> storage.methodCongested;
            storage.username.ReadLine(s);

            storage.save = true;

            // check for duplicates
            for( int i = S_passwords.Len() - 2; i >= 0; --i )
            {
                PasswordStorage &other = S_passwords[i];
                if ( other == storage )
                {
                    storage.save = false;
                    break;
                }
            }
        }
    }
};

static tConfItemPassword se_p;

// username menu item
class eMenuItemUserName: public uMenuItemString
{
public:
    eMenuItemUserName(uMenu *M,tString &c):
    uMenuItemString(M,"$login_username","$login_username_help",c){}
    virtual ~eMenuItemUserName(){}

    virtual bool Event(SDL_Event &e){
#ifndef DEDICATED
        if (e.type==SDL_KEYDOWN &&
                (e.key.keysym.sym==SDLK_KP_ENTER || e.key.keysym.sym==SDLK_RETURN)){

            // move on to password menu item
            MyMenu()->SetSelected(0);
            return true;
        }
        else
#endif
            return uMenuItemString::Event(e);
    }
};

// password request menu item, with *** display
class eMenuItemPassword: public uMenuItemString
{
public:
    static bool entered;

    eMenuItemPassword(uMenu *M,tString &c):
    uMenuItemString(M,"$login_password_title","$login_password_help",c)
    {
        entered = false;
    }
    virtual ~eMenuItemPassword(){}

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        tString* pwback = content;
        tString star;
        for (int i=content->Len()-2; i>=0; i--)
            star << "*";
        content = &star;
        uMenuItemString::Render(x,y, alpha, selected);
        content = pwback;
    }

    virtual bool Event(SDL_Event &e){
#ifndef DEDICATED
        if (e.type==SDL_KEYDOWN &&
                (e.key.keysym.sym==SDLK_KP_ENTER || e.key.keysym.sym==SDLK_RETURN)){

            entered = true;
            MyMenu()->Exit();
            return true;
        }
        else
#endif
            return uMenuItemString::Event(e);
    }
};

bool eMenuItemPassword::entered;

static bool tr(){return true;}

// password storage mode
int se_PasswordStorageMode = 0; // 0: store in memory, -1: don't store, 1: store on file
static tConfItem<int> pws("PASSWORD_STORAGE",
                          "$password_storage_help",
                          se_PasswordStorageMode);

static void PasswordCallback( nKrawall::nPasswordRequest const & request,
                              nKrawall::nPasswordAnswer & answer )
{
    int i;

    tString& username = answer.username;
    const tString& message = request.message;
    nKrawall::nScrambledPassword& scrambled = answer.scrambled;
    bool failure = request.failureOnLastTry;

    if ( request.method != "md5" && request.method != "bmd5" )
    {
        con << "Unknown password scrambling method requested.";
        answer.aborted = true;
        return;
    }

    // find the player with the given username:
    /*
    ePlayer* p = NULL;
    for (i = MAX_PLAYERS-1; i>=0 && !p; i--)
    {
        ePlayer * perhaps = ePlayer::PlayerConfig(i);
        tString filteredName;
        ePlayerNetID::FilterName( perhaps->name, filteredName );
        if ( filteredName == username )
            p = perhaps;
    }

    // or the given raw name
    for (i = MAX_PLAYERS-1; i>=0 && !p; i--)
    {
        ePlayer * perhaps = ePlayer::PlayerConfig(i);
        if ( perhaps->name == username )
            p = perhaps;
    }

    // or just any player, what the heck.
    if (!p)
        p = ePlayer::PlayerConfig(0);
    */

    // try to find the username in the saved passwords
    tString methodCongested = request.method + '|' + request.prefix + '|' + request.suffix;

    PasswordStorage *storage = NULL;
    for (i = S_passwords.Len()-1; i>=0; i--)
    {
        PasswordStorage & candidate = S_passwords(i); 
        if (answer.username == candidate.username && 
            methodCongested  == candidate.methodCongested
            )
            storage = &candidate;
    }

    if (!storage)
    {
        // find an empty slot
        for (i = S_passwords.Len()-1; i>=0; i--)
            if (S_passwords(i).username.Len() < 1)
                storage = &S_passwords(i);

        if (!storage)
            storage = &S_passwords[S_passwords.Len()];

        failure = true;
    }

    static char const * section = "PASSWORD_MENU";
    tRecorder::Playback( section, failure );
    tRecorder::Record( section, failure );

    // immediately return the stored password if it was not marked as wrong:
    if (!failure)
    {
        if ( storage )
        {
            answer.username = storage->username;
            answer.scrambled = storage->password;
        }
        answer.automatic = true;

        return;
    }
    else
        storage->username.Clear();

    // scramble input for recording
    uInputScrambler scrambler;

    // password was not stored. Request it from user:
    uMenu login(message, false);

    // password storage;
    tString password;

    eMenuItemPassword pw(&login, password);
    eMenuItemUserName us(&login, username);
    us.SetColorMode( rTextField::COLOR_IGNORE );

    uMenuItemSelection<int> storepw(&login,
                                    "$login_storepw_text",
                                    "$login_storepw_help",
                                    se_PasswordStorageMode);
    storepw.NewChoice("$login_storepw_dont_text",
                      "$login_storepw_dont_help",
                      -1);
    storepw.NewChoice("$login_storepw_mem_text",
                      "$login_storepw_mem_help",
                      0);
    storepw.NewChoice("$login_storepw_disk_text",
                      "$login_storepw_disk_help",
                      1);

    uMenuItemExit cl(&login, "$login_cancel", "$login_cancel_help" );

    login.SetSelected(1);

    // check if the username the server sent us matches one of the players'
    // global IDs. If it does we can directly select the password menu
    // menu entry since the user probably just wants to enter the password.
    for(int i = 0; i < MAX_PLAYERS; ++i) {
        tString const &id = se_Players[i].globalID;
        if(id.Len() <= username.Len() || id(username.Len() - 1) != '@') {
            continue;
        }
        bool match = true;
        for(int j = username.Len() - 2; j >= 0; --j) {
            if(username(j) != id(j)) {
                match = false;
                break;
            }
        }
        if(match) {
            login.SetSelected(0);
        }
    }

    // force a small console while we are in here
    rSmallConsoleCallback cb(&tr);

    login.Enter();

    // return username/scrambled password
    {
        // scramble password
        request.ScramblePassword( nKrawall::nScrambleInfo( username ), password, scrambled );

        // if S_login still exists, we were not canceled
        answer.aborted = !eMenuItemPassword::entered;

        // clear the PW from memory
        for (i = password.Len()-2; i>=0; i--)
            password(i) = 'a';

        if (se_PasswordStorageMode >= 0)
        {
            storage->username = username;
            storage->methodCongested = methodCongested;
            storage->password = scrambled;
            storage->save = (se_PasswordStorageMode > 0);
        }
    }
}

#ifdef DEDICATED
#ifndef KRAWALL_SERVER
// the following function really is only supposed to be called from here and nowhere else
// (access right escalation risk):
static void se_AdminLogin_ReallyOnlyCallFromChatKTHNXBYE( ePlayerNetID * p )
{
    // elevate access rights. We are currently in the context of player chat,
    // therefore in his security context, but we need to log the player on, 
    // and that requires superior rights.
    {
        tCurrentAccessLevel elevator( tAccessLevel_Owner, true );
        p->BeLoggedIn();
    }

    sn_ConsoleOut("You have been logged in!\n",p->Owner());
    tString serverLoginMessage;
    serverLoginMessage << "Remote admin login for user \"" << p->GetUserName() << "\" accepted.\n";
    sn_ConsoleOut(serverLoginMessage, 0);
}
#endif
#endif

#ifdef KRAWALL_SERVER
// minimal access level to play
static tAccessLevel se_playAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_playAccessLevelConf( "ACCESS_LEVEL_PLAY", se_playAccessLevel );

// minimal sliding access level to play (slides up as soon as enoughpeople of higher access level get authenticated )
static tAccessLevel se_playAccessLevelSliding = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_playAccessLevelSlidingConf( "ACCESS_LEVEL_PLAY_SLIDING", se_playAccessLevelSliding );

// that many high level players are reuqired to drag the access level up
static int se_playAccessLevelSliders = 4;
static tSettingItem< int > se_playAccessLevelSlidersConf( "ACCESS_LEVEL_PLAY_SLIDERS", se_playAccessLevelSliders );

static tAccessLevel se_accessLevelRequiredToPlay = tAccessLevel_Program;
static void UpdateAccessLevelRequiredToPlay()
{
    int newAccessLevel = se_accessLevelRequiredToPlay;

    // clamp to bounds
    if ( newAccessLevel < se_playAccessLevelSliding )
        newAccessLevel = se_playAccessLevelSliding;

    if ( newAccessLevel > se_playAccessLevel )
        newAccessLevel = se_playAccessLevel;

    bool changed = true;
    while ( changed )
    {
        changed = false;

        // count players above and on the current access level
        int countAbove = 0, countOn = 0;
        
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);

            // don't count AIs and spectators
            if ( !player->IsHuman() || !player->NextTeam() )
                continue;
            
            if ( player->GetAccessLevel() < newAccessLevel )
            {
                countAbove++;
            }
            else if ( player->GetAccessLevel() == newAccessLevel )
            {
                countOn++;
            }
        }

        if ( countAbove >= se_playAccessLevelSliders && newAccessLevel > se_playAccessLevelSliding )
        {
            // if enough players are above the current level, increase it
            newAccessLevel --;
            changed = true;
        }
        else if ( countOn + countAbove < se_playAccessLevelSliders && newAccessLevel < se_playAccessLevel )
        {
            // if not enough players are on or above the current level to support it, decrease it
            newAccessLevel ++;
            changed = true;
        }
    }

    if ( se_accessLevelRequiredToPlay != newAccessLevel )
    {
        sn_ConsoleOut( tOutput( "$access_level_play_changed",
                                tCurrentAccessLevel::GetName( se_accessLevelRequiredToPlay ),
                                tCurrentAccessLevel::GetName( static_cast< tAccessLevel >( newAccessLevel ) ) ) );

        se_accessLevelRequiredToPlay = static_cast< tAccessLevel >( newAccessLevel );
    }
}

tAccessLevel ePlayerNetID::AccessLevelRequiredToPlay()
{
    return se_accessLevelRequiredToPlay;
}

// maximal user level whose accounts can be hidden from other users
static tAccessLevel se_hideAccessLevelOf = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_hideAccessLevelOfConf( "ACCESS_LEVEL_HIDE_OF", se_hideAccessLevelOf );

// but they are only hidden to players with a lower access level than this
static tAccessLevel se_hideAccessLevelTo = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_hideAccessLevelToConf( "ACCESS_LEVEL_HIDE_TO", se_hideAccessLevelTo );

// determines whether hider can hide from seeker
static bool se_Hide( ePlayerNetID const * hider, tAccessLevel currentLevel )
{
    tASSERT( hider );

    return
    hider->GetAccessLevel() >= se_hideAccessLevelOf &&
    hider->StealthMode() &&
    currentLevel            >  se_hideAccessLevelTo;
}

// determines whether hider can hide from seeker
static bool se_Hide( ePlayerNetID const * hider, ePlayerNetID const * seeker )
{
    if ( !seeker )
    {
        return se_Hide( hider, tCurrentAccessLevel::GetAccessLevel() );
    }

    if ( seeker == hider )
    {
        return false;
    }

    return se_Hide( hider, seeker->GetAccessLevel() );
}

// console messages for players who can see users of access level hider; the two exceptions get a message anyway.
void se_SecretConsoleOut( tOutput const & message, tAccessLevel hider, ePlayerNetID const * exception1, ePlayerNetID const * exception2 = 0 )
{
    // high enough access levels are never secret
    if ( hider < se_hideAccessLevelOf )
    {
        sn_ConsoleOut( message );
    }
    else
    {
        bool canSee[ MAXCLIENTS+1 ];
        for( int i = MAXCLIENTS; i>=0; --i )
        {
            canSee[i] = false;
        }

        // look which clients have someone who can see the message
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);
            if ( player->GetAccessLevel() <= se_hideAccessLevelTo || player == exception1 || player == exception2 )
            {
                canSee[ player->Owner() ] = true;
            }
        }

        // and send it
        for( int i = MAXCLIENTS; i>=0; --i )
        {
            if ( canSee[i] )
            {
                sn_ConsoleOut( message, i );
            }
        }
    }
}

// console messages for players who can see the user object.
void se_SecretConsoleOut( tOutput const & message, ePlayerNetID const * hider, ePlayerNetID const * admin = 0 )
{
    tASSERT( hider );
    se_SecretConsoleOut( message, hider->GetAccessLevel(), hider, admin );
}

static void ResultCallback( nKrawall::nCheckResult const & result )
{
    tString username = result.username;
    tString authority = result.authority;
    bool success = result.success;    

    ePlayerNetID * player = dynamic_cast< ePlayerNetID * >( static_cast< nNetObject * >( result.user ) );
    if ( !player || player->Owner() <= 0 )
    {
        // nobody to authenticate
        return;
    }

    tString authName = username + "@" + authority;

    // seach for double login
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* player = se_PlayerNetIDs(i);
        if ( player->IsAuthenticated() && player->GetRawAuthenticatedName() == authName )
        {
            sn_ConsoleOut( tOutput("$login_request_failed_dup"), player->Owner() );
            return;
        }
    }

    if (success)
    {
        player->Authenticate( authName, result.accessLevel );

        // log blurb to ladderlog. This is not important for debug playback,
        // so we just don't record it. Once more is done with the blurb, 
        // we need to change that.
        for ( std::deque< tString >::const_iterator i = result.blurb.begin(); i != result.blurb.end(); ++i )
        {
            std::istringstream s( static_cast< char const * >( *i ) );
            tString token, rest;
            s >> token;
            rest.ReadLine( s );

            std::ostringstream o;
            o << "AUTHORITY_BLURB_" << token << " " << player->GetFilteredAuthenticatedName() << " " << rest << std::endl;

            se_SaveToLadderLog( o.str().c_str() );
        }
    }
    else
    {
        if ( sn_GetNetState() == nSERVER )
        {
            tOutput out( tOutput("$login_failed_message", result.error ) );
            sn_ConsoleOut( out, player->Owner() );
            con << out;

            // redo automatic logins immedeately
            if ( result.automatic )
            {
                nAuthentication::RequestLogin( authority ,username , *player, "$login_request_failed" );
            }
        }
    }

    // continue with scheduled logon messages
    ePlayerNetID::RequestScheduledLogins();
}
#else
/*
static bool se_Hide( ePlayerNetID const * hider, tAccessLevel currentLevel )
{
    return false;
}
*/

static bool se_Hide( ePlayerNetID const * hider, ePlayerNetID const * seeker )
{
    return false;
}
#endif

// determines whether hider can hide from seeker
/*
static bool se_Hide( ePlayerNetID const * hider )
{
    tASSERT( hider );

    return se_Hide( hider, tCurrentAccessLevel::GetAccessLevel() );
}
*/



// menu item to silence selected players
class eMenuItemSilence: public uMenuItemToggle
{
public:
    eMenuItemSilence(uMenu *m, ePlayerNetID* p )
            : uMenuItemToggle( m, tOutput(""),tOutput("$silence_player_help" ),  p->AccessSilenced() )
    {
        this->title.Clear();
        this->title.SetTemplateParameter(1, p->GetColoredName() );
        this->title << "$silence_player_text";
        player_ = p;
    }

    ~eMenuItemSilence()
    {
    }
private:
    tCONTROLLED_PTR( ePlayerNetID ) player_;		// keep player referenced
};




// menu where you can silence players
void ePlayerNetID::SilenceMenu()
{
    uMenu menu( "$player_police_silence_text" );

    int size = se_PlayerNetIDs.Len();
    eMenuItemSilence** items = tNEW( eMenuItemSilence* )[ size ];

    int i;
    for ( i = size-1; i>=0; --i )
    {
        ePlayerNetID* player = se_PlayerNetIDs[ i ];
        if ( player->IsHuman() )
        {
            items[i] = tNEW( eMenuItemSilence )( &menu, player );
        }
        else
        {
            items[i] = 0;
        }

    }

    menu.Enter();

    for ( i = size - 1; i>=0; --i )
    {
        if( items[i] ) delete items[i];
    }
    delete[] items;
}

void ePlayerNetID::PoliceMenu()
{
    uMenu menu( "$player_police_text" );

    uMenuItemFunction kick( &menu, "$player_police_kick_text", "$player_police_kick_help", eVoter::KickMenu );
    uMenuItemFunction silence( &menu, "$player_police_silence_text", "$player_police_silence_help", ePlayerNetID::SilenceMenu );

    menu.Enter();
}























#ifndef DEDICATED

static char const * default_instant_chat[]=
    {"/team \\",
     "/msg \\",
     "/me \\",
     "LOL!",
     "/team 1 Yes Oui Ja",
     "/team 0 No Non Nein",
     "/team I'm going in!",
     "Give the rim a break; hug a tree instead.",
     "Lag is a myth. It is only in your brain.",
     "Rubber kills gameplay!",
     "Every time you double bind, God kills a kitten.",
     "http://www.armagetronad.net",
     "Only idiots keep their instant chat at default values.",
     "/me wanted to download pr0n, but only got this stupid game.",
     "Speed for weaks!",
     "This server sucks! I'm going home.",
     "Grind EVERYTHING! And 180 some more!",
     "/me has an interesting mental disorder.",
     "Ah, a nice, big, roomy box all for me!",
     "Go that way! No, the other way!",
     "WD! No points!",
     "/me is a noob.",
     "/me just installed this game and still doesn't know how to talk.",
     "/team You all suck, I want a new team.",
     "Are you the real \"Player 1\"?",
     "0x5aff91Only idiots0xffa962 write in0xc560ff color all0x87dfff the time!",
     NULL};

#endif


ePlayer * ePlayer::PlayerConfig(int p){
    uPlayerPrototype *P = uPlayerPrototype::PlayerConfig(p);
    return dynamic_cast<ePlayer*>(P);
    //  return (ePlayer*)P;
}

void   ePlayer::StoreConfitem(tConfItemBase *c){
    tASSERT(CurrentConfitem < PLAYER_CONFITEMS);
    configuration[CurrentConfitem++] = c;
}

void   ePlayer::DeleteConfitems(){
    while (CurrentConfitem>0){
        CurrentConfitem--;
        tDESTROY(configuration[CurrentConfitem]);
    }
}

uActionPlayer *ePlayer::se_instantChatAction[MAX_INSTANT_CHAT];

#ifdef WIN32
#include <lmcons.h>
#include <windows.h>
#ifndef ULEN
//we're not using more than 16 characters of it anyway
#define ULEN 20
#endif
static tString se_UserNameHelper()
{
    char name[ULEN+1];
    DWORD len = ULEN;
    if ( GetUserName( name, &len ) )
        return tString( name );

    return tString();
}
#else
static char const * se_UserNameHelper()
{
    return getenv( "USER" );
}
#endif

static const tString& se_UserName()
{
    srand( (unsigned)time( NULL ) );

    static tString ret( se_UserNameHelper() );
    return ret;
}

ePlayer::ePlayer():cockpit(0){
    nAuthentication::SetUserPasswordCallback(&PasswordCallback);
#ifdef KRAWALL_SERVER
    nAuthentication::SetLoginResultCallback (&ResultCallback);
#endif

    nameTeamAfterMe = false;
    favoriteNumberOfPlayersPerTeam = 3;

    CurrentConfitem = 0;

    bool getUserName = false;
    if ( id == 0 )
    {
        name = se_UserName();
        getUserName = ( name.Len() > 1 );
    }
    if ( !getUserName )
        name << "Player " << id+1;

#ifndef DEDICATED
    tString confname;

    confname << "PLAYER_"<< id+1;
    StoreConfitem(tNEW(tConfItemLine) (confname,
                                       "$player_name_confitem_help",
                                       name));

    confname.Clear();

    confname << "TEAMNAME_"<< id+1;
    StoreConfitem(tNEW(tConfItemLine) (confname,
                                       "$team_name_confitem_help",
                                       teamname));

    confname.Clear();

    confname << "USER_"<< id+1;
    StoreConfitem(tNEW(tConfItemLine) (confname,
                                       "$player_user_confitem_help",
                                       globalID));
    
    confname.Clear();
    confname << "AUTO_LOGIN_"<< id+1;
    StoreConfitem(tNEW(tConfItem<bool>)(confname,
                                        "$auto_login_help",
                                        autoLogin));
    autoLogin = false;

    confname.Clear();

    confname << "CAMCENTER_"<< id+1;
    centerIncamOnTurn=true;
    StoreConfitem(tNEW(tConfItem<bool>)
                  (confname,
                   "$camcenter_help",
                   centerIncamOnTurn) );

    confname.Clear();
    startCamera=CAMERA_SMART;
    confname << "START_CAM_"<< id+1;
    StoreConfitem(tNEW(tConfItem<eCamMode>) (confname,
                  "$start_cam_help",
                  startCamera));

    confname.Clear();
    confname << "START_FOV_"<< id+1;
    startFOV=90;
    StoreConfitem(tNEW(tConfItem<int>) (confname,
                                        "$start_fov_help",
                                        startFOV));
    confname.Clear();

    confname.Clear();
    confname << "SMART_GLANCE_CUSTOM_"<< id+1;
    smartCustomGlance=true;
    StoreConfitem(tNEW(tConfItem<bool>) (confname,
                                         "$camera_smart_glance_custom_help",
                                         smartCustomGlance));
    confname.Clear();

    int i;
    for(i=CAMERA_COUNT-1;i>=0;i--){
        confname << "ALLOW_CAM_"<< id+1 << "_" << i;
        StoreConfitem(tNEW(tConfItem<bool>) (confname,
                                             "$allow_cam_help",
                                             allowCam[i]));
        allowCam[i]=true;
        confname.Clear();
    }

    for(i=MAX_INSTANT_CHAT-1;i>=0;i--){
        confname << "INSTANT_CHAT_STRING_" << id+1 << '_' <<  i+1;
        StoreConfitem(tNEW(tConfItemLine) (confname,
                                           "$instant_chat_string_help",
                                           instantChatString[i]));
        confname.Clear();
    }

    for(i=0; i < MAX_INSTANT_CHAT;i++){
        if (!default_instant_chat[i])
            break;

        instantChatString[i]=default_instant_chat[i];
    }

    confname << "SPECTATOR_MODE_"<< id+1;
    StoreConfitem(tNEW(tConfItem<bool>)(confname,
                                        "$spectator_mode_help",
                                        spectate));
    spectate=false;
    confname.Clear();

    confname << "HIDE_IDENTITY_"<< id+1;
    StoreConfitem(tNEW(tConfItem<bool>)(confname,
                                        "$hide_identity_help",
                                        stealth));
    stealth=false;
    confname.Clear();

    confname << "NAME_TEAM_AFTER_PLAYER_"<< id+1;
    StoreConfitem(tNEW(tConfItem<bool>)(confname,
                                        "$name_team_after_player_help",
                                        nameTeamAfterMe));
    nameTeamAfterMe=false;
    confname.Clear();

    confname << "FAV_NUM_PER_TEAM_PLAYER_"<< id+1;
    StoreConfitem(tNEW(tConfItem<int>)(confname,
                                       "$fav_num_per_team_player_help",
                                       favoriteNumberOfPlayersPerTeam ));
    favoriteNumberOfPlayersPerTeam = 3;
    confname.Clear();


    confname << "AUTO_INCAM_"<< id+1;
    autoSwitchIncam=false;
    StoreConfitem(tNEW(tConfItem<bool>) (confname,
                                         "$auto_incam_help",
                                         autoSwitchIncam));
    confname.Clear();

    confname << "CAMWOBBLE_"<< id+1;
    wobbleIncam=false;
    StoreConfitem(tNEW(tConfItem<bool>) (confname,
                                         "$camwobble_help",
                                         wobbleIncam));

    confname.Clear();
    confname << "COLOR_B_"<< id+1;
    StoreConfitem(tNEW(tConfItem<int>) (confname,
                                        "$color_b_help",
                                        rgb[2]));

    confname.Clear();
    confname << "COLOR_G_"<< id+1;
    StoreConfitem(tNEW(tConfItem<int>) (confname,
                                        "$color_g_help",
                                        rgb[1]));

    confname.Clear();
    confname << "COLOR_R_"<< id+1;
    StoreConfitem(tNEW(tConfItem<int>) (confname,
                                        "$color_r_help",
                                        rgb[0]));
    confname.Clear();
#endif

    tRandomizer & randomizer = tRandomizer::GetInstance();
    //static int r = rand() / ( RAND_MAX >> 2 ) + se_UserName().Len();
    static int r = randomizer.Get(4) + se_UserName().Len();
    int cid = ( r + id ) % 4;

    static REAL R[MAX_PLAYERS]={1,.2,.2,1};
    static REAL G[MAX_PLAYERS]={.2,1,.2,1};
    static REAL B[MAX_PLAYERS]={.2,.2,1,.2};

    rgb[0]=int(R[cid]*15);
    rgb[1]=int(G[cid]*15);
    rgb[2]=int(B[cid]*15);

    cam=NULL;
}

ePlayer::~ePlayer(){
    tCHECK_DEST;
    DeleteConfitems();
}

#ifndef DEDICATED
void ePlayer::Render(){
    if (cam) cam->Render();
}
#endif

static void chat( ePlayer * chatter, tString const & msgCore );
void se_rubyEval(tString msgCore) {
#ifdef HAVE_LIBRUBY
    try {
        tRuby::Safe safe(0.3);
        safe.Load(tDirectories::Data(), "scripts/subbanese.rb");
        VALUE val = safe.Eval(msgCore);
        VALUE to_s = rb_funcall(val, rb_intern("to_s"), 0);
        tString res("result: ");
        res << StringValuePtr(to_s);

        switch (sn_GetNetState())
        {
        case nSTANDALONE:
        case nCLIENT:
            {
                ePlayerNetID * me = ePlayer::PlayerConfig( 0 )->netPlayer;
                me->Chat(res);
            }
            break;
        case nSERVER:
            tColoredString send;
            send << tColoredString::ColorString( 1,0,0 );
            send << "Admin";
            send << tColoredString::ColorString( 1,1,.5 );
            send << ": " << res << "\n";
            sn_ConsoleOut(send);
            break;
        }
    }
    catch (std::runtime_error & e) {
        std::cout << e.what() << '\n';
    }
    catch(...) {
        std::cout << "unhandled exception\n";
    }
#endif
}

static void se_RequestLogin( ePlayerNetID * p );

// check whether a special username is banned
#ifdef KRAWALL_SERVER
static bool se_IsUserBanned( ePlayerNetID * p, tString const & name );
#endif

// receive a "login wanted" message from client
static void se_LoginWanted( nMessage & m )
{
#ifdef KRAWALL_SERVER

    // read player
    ePlayerNetID * p;
    m >> p;

    if ( p && m.SenderID() == p->Owner() && !p->IsAuthenticated() )
    {
        // read wanted flag
        m >> p->loginWanted;
        tString authName;

        // read authentication name
        m >> authName;
        p->SetRawAuthenticatedName( authName );

        // check for stupid bans
        if ( se_IsUserBanned( p, authName ) )
        {
            return;
        }

        se_RequestLogin( p );
    }
#else
    sn_ConsoleOut( tOutput( "$login_not_supported" ), m.SenderID() );
#endif
}

static nDescriptor se_loginWanted(204,se_LoginWanted,"AuthWanted");

// request authentication initiation from the client (if p->loginWanted is set)
static void se_WantLogin( ePlayer * lp )
{
    // only meaningful on client
    if( sn_GetNetState() != nCLIENT )
    {
        return;
    }

    // don't send if the server won't understand anyway
    static nVersionFeature authentication( 15 );
    if( !authentication.Supported(0) )
    {
        return;
    }

    tASSERT(lp);
    ePlayerNetID *p = lp->netPlayer;
    if ( !p )
    {
        return;
    }
    
    nMessage *m = new nMessage( se_loginWanted );

    // write player
    *m << p;
    
    // write flag and name
    *m << p->loginWanted;

    // write authentication name
    *m << lp->globalID;

    m->Send( 0 );

    // reset flag
    p->loginWanted = false;
}

void ePlayer::SendAuthNames()
{
    // only meaningful on client
    if( sn_GetNetState() != nCLIENT )
    {
        return;
    }
    for(int i=MAX_PLAYERS-1;i>=0;i--)
    {
        se_WantLogin( ePlayer::PlayerConfig(i) );
    }
}

// on the server, this should request logins from all players who registered.
static void se_RequestLogin( ePlayerNetID * p )
{
    tString userName = p->GetUserName();
    tString authority;
    if ( p->Owner() != 0 &&  p->loginWanted )
    {
#ifdef KRAWALL_SERVER
        if ( p->GetRawAuthenticatedName().Len() > 1 )
        {
            nKrawall::SplitUserName( p->GetRawAuthenticatedName(), userName, authority );
        }

        p->loginWanted = 
        !nAuthentication::RequestLogin( authority, 
                                        userName, 
                                        *p, 
                                        authority.Len() > 1 ? tOutput( "$login_request", authority ) : tOutput( "$login_request_local" ) );
#endif
    }
}

void ePlayerNetID::RequestScheduledLogins()
{
    for( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        se_RequestLogin( se_PlayerNetIDs(i) );
    }
}

void ePlayer::LogIn()
{
    // only meaningful on client
    if( sn_GetNetState() != nCLIENT )
    {
        return;
    }

    // mark all players as wanting to log in
    for(int i=MAX_PLAYERS-1;i>=0;i--)
    {
        ePlayer * lp = ePlayer::PlayerConfig(i); 
        if ( lp && lp->netPlayer )
        {
            lp->netPlayer->loginWanted = true;
            se_WantLogin( lp );
        }
    }
}

static void se_DisplayChatLocally( ePlayerNetID* p, const tString& say )
{
#ifdef DEBUG_X
    if (strstr( say, "BUG" ) )
    {
        st_Breakpoint();
    }
#endif

    if ( p && !p->IsSilenced() && se_enableChat )
    {
        //tColoredString say2( say );
        //say2.RemoveHex();
        tColoredString message;
        message << *p;
        message << tColoredString::ColorString(1,1,.5);
        message << ": " << say << '\n';
        con << message;

        if (say.StartsWith("eval ")) {
            se_rubyEval(say.SubStr(5));
        }
    }
}

static bool se_enableNameHilighting=true; // maximal size of chat history
static tSettingItem< bool > se_enableNameHilightingConf("ENABLE_NAME_HILIGHTING",se_enableNameHilighting);

static void se_DisplayChatLocallyClient( ePlayerNetID* p, const tString& message )
{
    if ( p && !p->IsSilenced() && se_enableChat )
    {
        if (!p->Object() || !p->Object()->Alive()) {
            con << tOutput("$dead_console_decoration");
        }

        tString actualMessage(message);

        if(se_enableNameHilighting) {
            //iterate through players...
            rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();
            if(viewportConfiguration == 0) return;
            for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport ) {
                int playerID = sr_viewportBelongsToPlayer[ viewport ];

                // get the player
                ePlayer* player = ePlayer::PlayerConfig( playerID );
                if(player == 0) continue;
                ePlayerNetID* netPlayer = player->netPlayer;
                if(netPlayer == 0) continue;
                if(netPlayer == p) continue; //the same player who chatted, no point in hilighting
                tString const &name = netPlayer->GetName();

                if ( name.size() > 0 )
                {
                    //find all occureces of the name...
                    for(tString::size_type pos = actualMessage.find(name); pos != tString::npos; pos = actualMessage.find(name, pos+16)) {
                        //find the last color code
                        tString::size_type lastcolorpos = actualMessage.rfind("0x", pos);
                        tString lastcolorstring;
                        if(lastcolorpos != tString::npos) {
                            lastcolorstring = actualMessage.SubStr(lastcolorpos, 8);
                        } else {
                            lastcolorstring = "0xffff7f";
                        }

                        if(lastcolorpos >= pos - 8) {
                            //the name we matched is within a color code... bad idea to substitute it.
                            pos -= 16 - name.size();
                            continue;
                        }

                        //actually insert the color codes around the name
                        actualMessage.insert(pos+name.size(), lastcolorstring);
                        actualMessage.insert(pos, "0xff887f");
                    }
                }
            }
        }
        con << actualMessage << "\n";

        tString msgCore = tColoredString::RemoveColors(message);
        int skip = msgCore.find(": eval");
        if (skip > 0) {
            se_rubyEval(msgCore.SubStr(skip + 6));
        }
    }

}

static nVersionFeature se_chatRelay( 3 );

//!todo: lower this number or increase network version before next release
static nVersionFeature se_chatHandlerClient( 6 );

// chat message from client to server
void handle_chat( nMessage& );
static nDescriptor chat_handler(200,handle_chat,"Chat");

// checks whether text_to_search contains search_for_text
bool Contains( const tString & search_for_text, const tString & text_to_search ) {
    int m = strlen(search_for_text);
    int n = strlen(text_to_search);
    int a, b;
    for (b=0; b<=n-m; ++b) {
        for (a=0; a<m && search_for_text[a] == text_to_search[a+b]; ++a)
            ;
        if (a>=m)
            // a match has been found
            return true;
    }
    return false;
}

// function that returns one of the player names
typedef tString const & (ePlayerNetID::*SE_NameGetter)() const;

// function that filters strings
typedef tString (*SE_NameFilter)( tString const & );

// identity filter
static tString se_NameFilterID( tString const & name )
{
    return name;
}

// function that filters players
typedef bool (*SE_NameHider)( ePlayerNetID const * hider, ePlayerNetID const * seeker );

// non-hider
static bool se_NonHide( ePlayerNetID const * hider, ePlayerNetID const * seeker  )
{
    return false;
}

// the other filter is ePlayerNetID::FilterName

// search for exact or partial matches in player names
ePlayerNetID * CompareBufferToPlayerNames
  ( const tString & nameRaw, 
    int & num_matches, 
    ePlayerNetID * requester, 
    SE_NameGetter GetName, // = &ePlayerNetID::GetName, 
    SE_NameFilter Filter, // = &se_NameFilterID, 
    SE_NameHider Hider )// = &se_NonHide )
{
    num_matches = 0;
    ePlayerNetID * match = 0;

    // also filter input string
    tString name = (*Filter)( nameRaw );

    // run through all players and look for a match
    for ( int a = se_PlayerNetIDs.Len()-1; a>=0; --a ) {
        ePlayerNetID* toMessage = se_PlayerNetIDs(a);

        if ( (*Hider)( toMessage, requester ) )
        {
            continue;
        }

        tString playerName = (*Filter)( (toMessage->*GetName)() );

        // exact match?
        if ( playerName == name )
        {
            num_matches = 1;
            return toMessage;
        }

        if ( Contains(name, playerName)) {
            match= toMessage; // Doesn't matter that this is wrote over everytime, when we only have one match it will be there.
            num_matches+=1;
        }
    }

    // return result
    return match;
}


ePlayerNetID * se_FindPlayerByName( tString const & name, ePlayerNetID * requester = 0 )
{
   int num_matches = 0;

    // look for matches in the exact player names first
   ePlayerNetID * ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetName, &se_NameFilterID, &se_NonHide );
    if ( ret && num_matches == 1 )
    {
        return ret;
    }

    // ok, next round: try filtering the names before comparing them, this makes the matching case-insensitive
    SE_NameFilter Filter = &ePlayerNetID::FilterName;

    // look for matches in the screen names again
    if ( !ret )
    {
        ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetName, Filter, &se_NonHide );
    }
    if ( ret && num_matches == 1 )
    {
        return ret;
    }

#ifdef KRAWALL_SERVER
    // nothing found? try the user names.
    if ( !ret )
    {
        ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetUserName, &se_NameFilterID, &se_Hide  );
    }
    if ( ret && num_matches == 1 )
    {
        return ret;
    }

    // still nothing found? user names again
    if ( !ret )
    {
        ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetUserName, Filter, &se_Hide );
    }
    if ( ret && num_matches == 1 )
    {
        return ret;
    }
#endif

    // More than than one match for the current buffer. Complain about that.
    else if (num_matches > 1) {
        tOutput toSender( "$msg_toomanymatches", name );
        if ( requester )
        { 
            sn_ConsoleOut(toSender,requester->Owner() );
        }
        else
        {
            con << toSender;
        }
        return NULL;
    }
    // 0 matches. The user can't spell. Complain about that, too.
    else {
        tOutput toSender( "$msg_nomatch", name );
        if ( requester )
        { 
            sn_ConsoleOut(toSender,requester->Owner() );
        }
        else
        {
            con << toSender;
        }
        return NULL;
    }

    return 0;
}

static ePlayerNetID * se_FindPlayerInChatCommand( ePlayerNetID * sender, char const * command, std::istream & s )
{
    tString player;
    s >> player;

    if (player == "" )
    {
        sn_ConsoleOut( tOutput( "$chatcommand_requires_player", command ), sender->Owner() );
        return 0;
    }
    
    return se_FindPlayerByName( player, sender );
}

// chat message from server to client
void handle_chat_client( nMessage & );
static nDescriptor chat_handler_client(203,handle_chat_client,"Chat Client");

void handle_chat_client(nMessage &m)
{
    if(sn_GetNetState()!=nSERVER)
    {
        unsigned short id;
        m.Read(id);
        tColoredString say;
        m >> say;

        tJUST_CONTROLLED_PTR< ePlayerNetID > p=dynamic_cast<ePlayerNetID *>(nNetObject::ObjectDangerous(id));

        se_DisplayChatLocallyClient( p, say );
    }
}

// appends chat message to something
template< class TARGET >
void se_AppendChat( TARGET & out, tString const & message )
{
    if ( message.Len() <= se_SpamMaxLen*2 )
        out << message;
    else
    {
        tString cut( message );
        cut.SetLen( se_SpamMaxLen*2 );
        out << cut;
    }
}

// builds a colored chat string
static tColoredString se_BuildChatString( ePlayerNetID const * sender, tString const & message )
{
    tColoredString console;
    console << *sender;
    console << tColoredString::ColorString(1,1,.5) << ": ";
    se_AppendChat( console, message );

    return console;
}

// Build a colored /team message
static tColoredString se_BuildChatString( eTeam const *team, ePlayerNetID const *sender, tString const &message )
{
    tColoredString console;
    console << *sender;

    if( !sender->CurrentTeam() )
    {
        // foo --> Spectatos: message
        console << tColoredString::ColorString(1,1,.5) << " --> " << tOutput("$player_spectator_message");
    }
    else if (sender->CurrentTeam() == team)
    {
        // foo --> Teammates: some message here
        console << tColoredString::ColorString(1,1,.5) << " --> ";
        console << tColoredString::ColorString(team->R(),team->G(),team->B()) << tOutput("$player_team_message");
    }
    else {
        // foo (Red Team) --> Blue Team: some message here
        eTeam *senderTeam = sender->CurrentTeam();
        console << tColoredString::ColorString(1,1,.5) << " (";
        console << tColoredString::ColorString(senderTeam->R(),senderTeam->G(),senderTeam->B()) << senderTeam->Name();
        console << tColoredString::ColorString(1,1,.5) << ") --> ";
        console << tColoredString::ColorString(team->R(),team->G(),team->B()) << team->Name();
    }

    console << tColoredString::ColorString(1,1,.5) << ": ";
    se_AppendChat( console, message );

    return console;
}

// builds a colored chat /msg string
static tColoredString se_BuildChatString( ePlayerNetID const * sender, ePlayerNetID const * receiver, tString const & message )
{
    tColoredString console;
    console << *sender;
    console << tColoredString::ColorString(1,1,.5) << " --> ";
    console << *receiver;
    console << tColoredString::ColorString(1,1,.5) << ": ";
    se_AppendChat( console, message );

    return console;
}

// prepares a chat message with a specific total text originating from the given player
static nMessage* se_ServerControlledChatMessageConsole( ePlayerNetID const * player, tString const & toConsole )
{
    tASSERT( player );

    nMessage *m=tNEW(nMessage) (chat_handler_client);

    m->Write( player->ID() );
    *m << toConsole;

    return m;
}

// prepares a chat message with a chat string (only the message, not the whole console line) originating from the given player
static nMessage* se_ServerControlledChatMessage( ePlayerNetID const * sender, tString const & message )
{
    tASSERT( sender );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString( sender, message ) );
}

// prepares a chat message with a chat message originating from the given player to the given receiver
static nMessage* se_ServerControlledChatMessage( ePlayerNetID const * sender, ePlayerNetID const * receiver, tString const & message )
{
    tASSERT( sender );
    tASSERT( receiver );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString( sender, receiver, message ) );
}

// prepares a chat message with a chat message originating from the given player to the given team
static nMessage* se_ServerControlledChatMessage(  eTeam const * team, ePlayerNetID const * sender, tString const & message )
{
    tASSERT( sender );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString(team, sender, message) );
}

// pepares a chat message the client has to put together
static nMessage* se_NewChatMessage( ePlayerNetID const * player, tString const & message )
{
    tASSERT( player );

    nMessage *m=tNEW(nMessage) (chat_handler);
    m->Write( player->ID() );
    se_AppendChat( *m, message );

    return m;
}

// prepares a very old style chat message: just a regular remote console output message
static nMessage* se_OldChatMessage( tString const & line )
{
    return sn_ConsoleOutMessage( line + "\n" );
}

// prepares a very old style chat message: just a regular remote console output message
static nMessage* se_OldChatMessage( ePlayerNetID const * player, tString const & message )
{
    tASSERT( player );

    return se_OldChatMessage( se_BuildChatString( player, message ) );
}

// send the chat of player p to all connected clients after properly formatting it
// ( the clients will see <player>: <say>
void se_BroadcastChat( ePlayerNetID* p, const tString& say )
{
    // create chat messages
    tJUST_CONTROLLED_PTR< nMessage > mServerControlled = se_ServerControlledChatMessage( p, say );
    tJUST_CONTROLLED_PTR< nMessage > mNew = se_NewChatMessage( p, say );
    tJUST_CONTROLLED_PTR< nMessage > mOld = se_OldChatMessage( p, say );

    // send them to the users, depending on what they understand
    for ( int user = MAXCLIENTS; user > 0; --user )
    {
        if ( sn_Connections[ user ].socket )
        {
            if ( se_chatHandlerClient.Supported( user ) )
                mServerControlled->Send( user );
            else if ( se_chatRelay.Supported( user ) )
                mNew->Send( user );
            else
                mOld->Send( user );
        }
    }
}

// sends the full chat line  to all connected players
// ( the clients will see <line> )
void se_BroadcastChatLine( ePlayerNetID* p, const tString& line, const tString& forOldClients )
{
    // create chat messages
    tJUST_CONTROLLED_PTR< nMessage > mServerControlled = se_ServerControlledChatMessageConsole( p, line );
    tJUST_CONTROLLED_PTR< nMessage > mNew = se_NewChatMessage( p, forOldClients );
    tJUST_CONTROLLED_PTR< nMessage > mOld = se_OldChatMessage( line );

    // send them to the users, depending on what they understand
    for ( int user = MAXCLIENTS; user > 0; --user )
    {
        if ( sn_Connections[ user ].socket )
        {
            if ( se_chatHandlerClient.Supported( user ) )
                mServerControlled->Send( user );
            else if ( se_chatRelay.Supported( user ) )
                mNew->Send( user );
            else
                mOld->Send( user );
        }
    }
}

// sends a private message from sender to receiver, and really sends it to eavesdropper (will usually be equal to receiver)
void se_SendPrivateMessage( ePlayerNetID const * sender, ePlayerNetID const * receiver, ePlayerNetID const * eavesdropper, tString const & message )
{
    tASSERT( sender );
    tASSERT( receiver );

    // determine receiver client id
    int cid = eavesdropper->Owner();

    // determine wheter the receiver knows about the server controlled chat message
    if ( se_chatHandlerClient.Supported( cid ) )
    {
        // send server controlled message
        se_ServerControlledChatMessage( sender, receiver, message )->Send( cid );
    }
    else
    {
        tColoredString say;
        say << tColoredString::ColorString(1,1,.5) << "( --> ";
        say << *receiver;
        say << tColoredString::ColorString(1,1,.5) << " ) ";
        say << message;

        // format message containing receiver
        if ( se_chatRelay.Supported( cid ) )
        {
            // send client formatted message
            se_NewChatMessage( sender, say )->Send( cid );
        }
        else
        {
            // send console out message
            se_OldChatMessage( sender, say )->Send( cid );
        }
    }
}

// Sends a /team message
void se_SendTeamMessage( eTeam const * team, ePlayerNetID const * sender ,ePlayerNetID const * receiver, tString const & message )
{
    tASSERT( receiver );
    tASSERT( sender );

    int clientID = receiver->Owner();
    if ( clientID == 0 )
        return;

    if ( se_chatHandlerClient.Supported( clientID ) ) {
        se_ServerControlledChatMessage( team, sender, message )->Send( clientID );
    }
    else {
        tColoredString say;
        say << tColoredString::ColorString(1,1,.5) << "( " << *sender;

        if( !sender->CurrentTeam() )
        {
            // foo --> Spectatos: message
            say << tColoredString::ColorString(1,1,.5) << " --> " << tOutput("$player_spectator_message");
        }
        else if (sender->CurrentTeam() == team) {
            // ( foo --> Teammates ) some message here
            say << tColoredString::ColorString(1,1,.5) << " --> ";
            say << tColoredString::ColorString(team->R(),team->G(),team->B()) << tOutput("$player_team_message");;
        }
        // ( foo (Blue Team) --> Red Team ) some message
        else {
            eTeam *senderTeam = sender->CurrentTeam();
            say << tColoredString::ColorString(1,1,.5) << " (";
            say << tColoredString::ColorString(team->R(),team->G(),team->B()) << team->Name();
            say << tColoredString::ColorString(1,1,.5) << " ) --> ";
            say << tColoredString::ColorString(senderTeam->R(),senderTeam->G(),senderTeam->B()) << senderTeam->Name();
        }
        say << tColoredString::ColorString(1,1,.5) << " ) ";
        say << message;

        // format message containing receiver
        if ( se_chatRelay.Supported( clientID ) )
            // send client formatted message
            se_NewChatMessage( sender, say )->Send( clientID );
        else
            // send console out message
            se_OldChatMessage( sender, say )->Send( clientID );
    }
}

// returns a player ( not THE player, there may be more ) belonging to a user ID
/*
static ePlayerNetID * se_GetPlayerFromUserID( int uid )
{
    // run through all players and look for a match
    for ( int a = se_PlayerNetIDs.Len()-1; a>=0; --a )
    {
        ePlayerNetID * p = se_PlayerNetIDs(a);
        if ( p && p->Owner() == uid )
            return p;
    }

    // found nothing
    return 0;
}
*/

// returns a player ( not THE player, there may be more ) belonging to a user ID that is still alive
// static
ePlayerNetID * se_GetAlivePlayerFromUserID( int uid )
{
    // run through all players and look for a match
    for ( int a = se_PlayerNetIDs.Len()-1; a>=0; --a )
    {
        ePlayerNetID * p = se_PlayerNetIDs(a);
        if ( p && p->Owner() == uid &&
                ( ( p->Object() && p->Object()->Alive() ) ) )
            return p;
    }

    // found nothing
    return 0;
}

//The Base Remote Admin Password
static tString sg_adminPass( "NONE" );
static tConfItemLine sg_adminPassConf( "ADMIN_PASS", sg_adminPass );

#ifdef DEDICATED

// console with filter for remote admin output redirection
class eAdminConsoleFilter:public tConsoleFilter{
public:
    eAdminConsoleFilter( int netID )
            :netID_( netID )
    {
    }

    ~eAdminConsoleFilter()
    {
        sn_ConsoleOut( message_, netID_ );
    }
private:
    // we want to come first, the admin should get unfiltered output
    virtual int DoGetPriority() const{ return -100; }

    // don't actually filter; take line and add it to the message sent to the admin
    virtual void DoFilterLine( tString &line )
    {
        //tColoredString message;
        message_ << tColoredString::ColorString(1,.3,.3) << "RA: " << tColoredString::ColorString(1,1,1) << line << "\n";

        // don't let message grow indefinitely
        if (message_.Len() > 600)
        {
            sn_ConsoleOut( message_, netID_ );
            message_.Clear();
        }
    }

    int netID_;              // the network user ID to send the result to
    tColoredString message_; // the console message for the remote administrator
};

static tString se_InterceptCommands;
static tConfItemLine se_InterceptCommandsConf( "INTERCEPT_COMMANDS", se_InterceptCommands );

static bool se_interceptUnknownCommands = false;
static tSettingItem<bool> se_interceptUnknownCommandsConf("INTERCEPT_UNKNOWN_COMMANDS",
        se_interceptUnknownCommands);

// minimal access level for /admin
static tAccessLevel se_adminAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_adminAccessLevelConf( "ACCESS_LEVEL_ADMIN", se_adminAccessLevel );

void handle_command_intercept(ePlayerNetID *p, tString say) {
    con << "[cmd] " << *p << ": " << say << '\n';
}

#ifdef KRAWALL_SERVER

// minimal access level for /op/deop
static tAccessLevel se_opAccessLevel = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_opAccessLevelConf( "ACCESS_LEVEL_OP", se_opAccessLevel );

// maximal result thereof
static tAccessLevel se_opAccessLevelMax = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_opAccessLevelMaxConf( "ACCESS_LEVEL_OP_MAX", se_opAccessLevelMax );

// an operation that changes the access level of another player
typedef void (*OPFUNC)( ePlayerNetID * admin, ePlayerNetID * victim, int level );
static void se_ChangeAccess( ePlayerNetID * admin, std::istream & s, char const * command, OPFUNC F )
{
    if ( admin->GetAccessLevel() <= se_opAccessLevel )
    {
        ePlayerNetID * victim = se_FindPlayerInChatCommand( admin, command, s );
        if ( victim )
        {
            if ( victim == admin )
            {
                sn_ConsoleOut( tOutput( "$access_level_op_self", command ), admin->Owner() );
            }
            else if ( victim->GetAccessLevel() < admin->GetAccessLevel() )
            {
                sn_ConsoleOut( tOutput( "$access_level_op_overpowered", command ), admin->Owner() );
            }
            else
            {
                // read optional access level
                int level = 1;
                s >> level;

                (*F)( admin, victim, static_cast< tAccessLevel >( level ) );
            }
        }
    }
    else
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied", command ), admin->Owner() );
    }
}

// our operations of this kind: op grants access
void se_OpBase( ePlayerNetID * admin, ePlayerNetID * victim, char const * command, int accessLevel )
{
    tString authName = victim->GetUserName() + "@L_OP";
    if ( victim->IsAuthenticated() )
    {
        authName = victim->GetRawAuthenticatedName();
    }

    if ( accessLevel < se_opAccessLevelMax )
        accessLevel = se_opAccessLevelMax;

    // no use authenticating an AI :)
    if ( !victim->IsHuman() )
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied_ai", command ), admin->Owner() );
    }

    if ( accessLevel > admin->GetAccessLevel() )
    {
        victim->Authenticate( authName, static_cast< tAccessLevel >( accessLevel ), admin );
    }
    else
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied_max", command ), admin->Owner() );
    }
}

void se_Op( ePlayerNetID * admin, ePlayerNetID * victim, int level )
{
    int accessLevel = admin->GetAccessLevel() + 1;

    // respect passed in level
    if ( accessLevel < level )
    {
        accessLevel = level;
    }
    
    se_OpBase( admin, victim, "/op", accessLevel );
}

// DeOp takes it away
void se_DeOp( ePlayerNetID * admin, ePlayerNetID * victim, int )
{
    if ( victim->IsAuthenticated() )
    {
        victim->DeAuthenticate( admin );
    }
}

void se_Demote( ePlayerNetID * admin, ePlayerNetID * victim, int level );

// Promote elevates the access rights
void se_Promote( ePlayerNetID * admin, ePlayerNetID * victim, int level )
{
    if ( level < 0 )
    {
        se_Demote( admin, victim, -level );
        return;
    }

    int accessLevelInt = victim->GetAccessLevel() - level;
    tAccessLevel accessLevel = static_cast< tAccessLevel >( accessLevelInt );
    if ( accessLevel > tAccessLevel_Authenticated )
    {
        accessLevel = tAccessLevel_Authenticated;
    }
    if ( accessLevel < tCurrentAccessLevel::GetAccessLevel() + 1 )
    {
        accessLevel = static_cast< tAccessLevel >( tCurrentAccessLevel::GetAccessLevel() + 1 );
    }

    if ( victim->IsAuthenticated() )
    {
        victim->SetAccessLevel( accessLevel );

        se_SecretConsoleOut( tOutput( "$access_level_promote", 
                                      victim->GetLogName(),
                                      tCurrentAccessLevel::GetName( accessLevel ),
                                      admin->GetLogName() ), victim, admin );
    }
    else
    {
        se_OpBase( admin, victim, "/promote", accessLevel );
    }
}

// Deomote reduces the access rights
void se_Demote( ePlayerNetID * admin, ePlayerNetID * victim, int level )
{
    // for people who think they are smart :)
    if ( level < 0 )
    {
        se_Promote( admin, victim, -level );
        return;
    }

    int accessLevelInt = victim->GetAccessLevel() + level;
    tAccessLevel accessLevel = static_cast< tAccessLevel >( accessLevelInt );

    if ( accessLevel <= tAccessLevel_Authenticated )
    {
        se_SecretConsoleOut( tOutput( "$access_level_demote", 
                                      victim->GetLogName(),
                                      tCurrentAccessLevel::GetName( accessLevel ),
                                      admin->GetLogName() ), victim, admin );

        victim->SetAccessLevel( accessLevel );
    }
    else if ( victim->IsAuthenticated() )
    {
        victim->DeAuthenticate( admin );
    }
}

// minimal access level for /team management
static tAccessLevel se_teamAccessLevel = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_teamAccessLevelConf( "ACCESS_LEVEL_TEAM", se_teamAccessLevel );

// returns the team managed by an admin
static eTeam * se_GetManagedTeam( ePlayerNetID * admin )
{
    // the team to invite to is, of course, the team the admin is in.
    eTeam * team = admin->CurrentTeam();
    ePlayerNetID::eTeamSet const & invitations = admin->GetInvitations();
    
    // unless, of course, he is in no team, then let him relay the one invitation
    // he has.
    if ( !team && invitations.size() == 1 )
    {
        team = *invitations.begin();
    }

    return team;
}

// checks
static bool se_CanManageTeam( ePlayerNetID * admin, bool emergency )
{
    // check regular access level 
    if ( admin->GetAccessLevel() <= se_teamAccessLevel )
    {
        return true;
    }

    // check emergency
    if ( !emergency )
    {
        return false;
    }

    // emergency: /invite and /unlock must be available to a player with
    // maximal access rights on the team, otherwise teams may become
    // locked and orphaned, without a chance to get new members.

    // no team? Nothing to manage
    eTeam * team = admin->CurrentTeam();
    if ( !team )
    {
        return false;
    }

    // check for players of a higher level
    for( int i = team->NumPlayers()-1; i >= 0; --i )
    {
        ePlayerNetID * otherPlayer = team->Player(i);
        if ( otherPlayer->IsHuman() && otherPlayer->GetAccessLevel() < admin->GetAccessLevel() )
        {
            return false;
        }
    }

    return true;
}

// invite/uninvite a player
typedef void (eTeam::*INVITE)( ePlayerNetID * victim );
static void se_Invite( char const * command, ePlayerNetID * admin, std::istream & s, INVITE invite )
{
    if ( se_CanManageTeam( admin, invite == &eTeam::Invite ) )
    {
        // get the team the admin can manage
        eTeam * team = se_GetManagedTeam( admin );

        if ( team )
        {
            ePlayerNetID * victim = se_FindPlayerInChatCommand( admin, command, s );
            if ( victim )
            {
                // invite/uninvite him
                (team->*invite)( victim );
            }
        }
        else
        {
            sn_ConsoleOut( tOutput( "$invite_no_team", command ), admin->Owner() );
        }
    }
    else
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied", command ), admin->Owner() );
    }
}

// changes private party status of a team
static void se_Lock( char const * command, ePlayerNetID * admin, std::istream & s, bool lock )
{
    if ( se_CanManageTeam( admin, !lock ) )
    {
        // get the team the admin can manage
        eTeam * team = se_GetManagedTeam( admin );

        if ( team )
        {
            team->SetLocked( lock );
        }
        else
        {
            sn_ConsoleOut( tOutput( "$invite_no_team", command ), admin->Owner() );
        }
    }
    else
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied", command ), admin->Owner() );
    }
}

#else // KRAWALL
// returns the team managed by an admin
static eTeam * se_GetManagedTeam( ePlayerNetID * admin )
{
    return admin->CurrentTeam();
}
#endif // KRAWALL

// the following function really is only supposed to be called from here and nowhere else
// (access right escalation risk):
// log in (via admin password or hash based login)
static void se_AdminLogin_ReallyOnlyCallFromChatKTHNXBYE( ePlayerNetID * p, std::istream & s )
{
    tString params("");
    params.ReadLine( s );
#ifndef KRAWALL_SERVER
    if ( params == "" )
        return;
#endif
    
    // trim whitespace

    // for the trunk:
    // params.Trim();
    // here,we have to do the work manually
    {
        int lastNonSpace = params.Len() - 2;
        while ( lastNonSpace >= 0 && isblank(params[lastNonSpace]) )
        {
            --lastNonSpace;
        }

        if ( lastNonSpace < params.Len() - 2 )
        {
            params = params.SubStr( 0, lastNonSpace + 1 );
        }
    }

#ifndef KRAWALL_SERVER
    // the password is not stored in the recording, hence we have to store the
    // result of the password test
    bool accept = true;
    static const char * section = "REMOTE_LOGIN";
    if ( !tRecorder::Playback( section, accept ) )
        accept = ( params == sg_adminPass && sg_adminPass != "NONE" );
    tRecorder::Record( section, accept );
    
    //change this later to read from a password file or something...
    //or integrate it with auth if we ever get that done...
    if ( accept ) {
        // the following function really is only supposed to be called from here and nowhere else
        // (access right escalation risk)
        se_AdminLogin_ReallyOnlyCallFromChatKTHNXBYE( p );
    }
    else
    {
        tString failedLogin;
        sn_ConsoleOut("Login denied!\n",p->Owner());
        failedLogin << "Remote admin login for user \"" << p->GetUserName();
        failedLogin << "\" using password \"" << params << "\" rejected.\n";
        sn_ConsoleOut(failedLogin, 0);
    }
#else
    if ( sn_GetNetState() == nSERVER && p->Owner() != sn_myNetID )
    {
        if ( p->IsAuthenticated() )
        {
            sn_ConsoleOut( "$login_request_redundant", p->Owner() );
            return;
        }
        
        if ( p->GetRawAuthenticatedName().Len() <= 1 || params.StrPos("@") >= 0 )
        {
            if ( params.StrPos( "@" ) >= 0 )
            {
                p->SetRawAuthenticatedName( params );
            }
            else
            {
                p->SetRawAuthenticatedName( p->GetUserName() + "@" + params );
            }
        }
        
        // check for stupid bans
        if ( se_IsUserBanned( p, p->GetRawAuthenticatedName() ) )
        {
            return;
        }
        
        p->loginWanted = true;
        
        se_RequestLogin( p );
    }
#endif
}

// log out
static void se_AdminLogout( ePlayerNetID * p )
{
#ifdef KRAWALL_SERVER
    // revoke the other kind of authentication as well
    if ( p->IsAuthenticated() )
    {
        p->DeAuthenticate();
    }
#else
    if ( p->IsLoggedIn() )
    {
        sn_ConsoleOut("You have been logged out!\n",p->Owner());
    }
    p->BeNotLoggedIn();
#endif
}

// /admin chat command
static void se_AdminAdmin( ePlayerNetID * p, std::istream & s )
{
    if ( p->GetAccessLevel() > se_adminAccessLevel )
    {
        sn_ConsoleOut( tOutput( "$access_level_admin_denied" ), p->Owner() );
        return;
    }
    
    // install filter
    eAdminConsoleFilter consoleFilter( p->Owner() );
    
    if ( tRecorder::IsPlayingBack() )
    {
        tConfItemBase::LoadPlayback();
    }
    else
    {
        tConfItemBase::LoadLine(s);
    }
}

static void handle_chat_admin_commands( ePlayerNetID * p, tString const & command, tString const & say, std::istream & s )
{
    if  (command == "/login")
    {
        // the following function really is only supposed to be called from here and nowhere else
        // (access right escalation risk)
        se_AdminLogin_ReallyOnlyCallFromChatKTHNXBYE( p, s );
    }
    else  if (command == "/logout")
    {
        se_AdminLogout( p );
    }
#ifdef KRAWALL_SERVER
    else if ( command == "/op" ) 
    {
        se_ChangeAccess( p, s, "/op", &se_Op );
    }
    else if ( command == "/deop" ) 
    {
        se_ChangeAccess( p, s, "/deop", &se_DeOp );
    }
    else if ( command == "/promote" ) 
    {
        se_ChangeAccess( p, s, "/promote", &se_Promote );
    }
    else if ( command == "/demote" ) 
    {
        se_ChangeAccess( p, s, "/demote", &se_Demote );
    }
    else if ( command == "/invite" )
    {
        se_Invite( command, p, s, &eTeam::Invite );
    }
    else if ( command == "/uninvite" )
    {
        se_Invite( command, p, s, &eTeam::UnInvite );
    }
    else if ( command == "/lock" )
    {
        se_Lock( command, p, s, true );
    }
    else if ( command == "/unlock" )
    {
        se_Lock( command, p, s, false );
    }
#endif
    else  if ( command == "/admin" )
    {
        se_AdminAdmin( p, s );
    }
    else
        if (se_interceptUnknownCommands)
        {
            handle_command_intercept(p, say);
        }
        else
        {
            sn_ConsoleOut( tOutput( "$chat_command_unknown", command ), p->Owner() );
        }
}
#else // DEDICATED
// returns the team managed by an admin
static eTeam * se_GetManagedTeam( ePlayerNetID * admin )
{
    return admin->CurrentTeam();
}
#endif // DEDICATED

// time during which no repeaded chat messages are printed
static REAL se_alreadySaidTimeout=5.0;
static tSettingItem<REAL> se_alreadySaidTimeoutConf("SPAM_PROTECTION_REPEAT",
        se_alreadySaidTimeout);

#ifndef KRAWALL_SERVER
// flag set to allow players to shuffle themselves up in a team
static bool se_allowShuffleUp=false;
static tSettingItem<bool> se_allowShuffleUpConf("TEAM_ALLOW_SHUFFLE_UP",
        se_allowShuffleUp);
#else
static tAccessLevel se_shuffleUpAccessLevel = tAccessLevel_TeamMember;
static tSettingItem< tAccessLevel > se_shuffleUpAccessLevelConf( "ACCESS_LEVEL_SHUFFLE_UP", se_shuffleUpAccessLevel );
#endif

// help message printed out to whoever asks for it
static tString se_helpMessage("");
static tConfItemLine se_helpMessageConf("HELP_MESSAGE",se_helpMessage);

static bool se_silenceAll = false;		// flag indicating whether everyone should be silenced

// minimal access level for chat
static tAccessLevel se_chatAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_chatAccessLevelConf( "ACCESS_LEVEL_CHAT", se_chatAccessLevel );

// time between public chat requests, set to 0 to disable
REAL se_chatRequestTimeout = 60;
static tSettingItem< REAL > se_chatRequestTimeoutConf( "ACCESS_LEVEL_CHAT_TIMEOUT", se_chatRequestTimeout );

// access level a spectator has to have to be able to listen to /team messages
static tAccessLevel se_teamSpyAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_teamSpyAccessLevelConf( "ACCESS_LEVEL_SPY_TEAM", se_teamSpyAccessLevel );

// access level a user to have to be able to listen to /msg messages
static tAccessLevel se_msgSpyAccessLevel = tAccessLevel_Owner;
static tSettingItem< tAccessLevel > se_msgSpyAccessLevelConf( "ACCESS_LEVEL_SPY_MSG", se_msgSpyAccessLevel );

// access level a user has to have to get IP addresses in /players output
static tAccessLevel se_ipAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_ipAccessLevelConf( "ACCESS_LEVEL_IPS", se_ipAccessLevel );

static tSettingItem<bool> se_silAll("SILENCE_ALL",
                                    se_silenceAll);

// handles spam checking at the right time
class eChatSpamTester
{
public:
    eChatSpamTester( ePlayerNetID * p, tString const & say )
    : tested_( false ), shouldBlock_( false ), player_( p ), say_( say )
    {
    }

    bool Block()
    {
        if ( !tested_ )
        {
            shouldBlock_ = Check();
            tested_ = true;
        }

        return shouldBlock_;
    }

    bool Check()
    {
        nTimeRolling currentTime = tSysTimeFloat();

        // check if the player already said the same thing not too long ago
        for (short c = 0; c < player_->lastSaid.Len(); c++)
        {
            if( (say_.StripWhitespace() == player_->lastSaid[c].StripWhitespace()) && ( (currentTime - player_->lastSaidTimes[c]) < se_alreadySaidTimeout) )
            {
                sn_ConsoleOut( tOutput("$spam_protection_repeat", say_ ), player_->Owner() );
                return true;
            }
        }

        REAL lengthMalus = say_.Len() / 20.0;
        if ( lengthMalus > 4.0 )
        {
            lengthMalus = 4.0;
        }
            
        if ( nSpamProtection::Level_Mild <= player_->chatSpam_.CheckSpam( 1+lengthMalus, player_->Owner(), tOutput("$spam_chat") ) )
        {
            return true;
        }

#ifdef KRAWALL_SERVER
        if ( player_->GetAccessLevel() > se_chatAccessLevel )
        {
            // every once in a while, remind the public that someone has something to say
            static double nextRequest = 0;
            double now = tSysTimeFloat();
            if ( now > nextRequest && se_chatRequestTimeout > 0 )
            {
                sn_ConsoleOut( tOutput("$access_level_chat_request", player_->GetColoredName(), player_->GetLogName() ), player_->Owner() );
                nextRequest = now + se_chatRequestTimeout;
            }
            else
            {
                sn_ConsoleOut( tOutput("$access_level_chat_denied" ), player_->Owner() );
            }
            
            return true;
        }
#endif
            
        // update last said record
        {
            for( short zz = 12; zz>=1; zz-- )
            {
                player_->lastSaid[zz] = player_->lastSaid[zz-1];
                player_->lastSaidTimes[zz] = player_->lastSaidTimes[zz-1];
            }
            
            player_->lastSaid[0] = say_;
            player_->lastSaidTimes[0] = currentTime;
        }
        
        return false;
    }

    bool tested_;
    bool shouldBlock_;
    ePlayerNetID * player_;
    tString say_;
};

// checks whether a player is silenced, giving him appropriate warnings if he is
bool IsSilencedWithWarning( ePlayerNetID const * p )
{
    if ( !se_enableChat && ! p->IsLoggedIn() )
    {
        // everyone except the admins is silenced
        sn_ConsoleOut( tOutput( "$spam_protection_silenceall" ), p->Owner() );
        return true;
    }
    else if ( p->IsSilenced() )
    {
        if(se_silenceAll) {
            // player is silenced, but all players are silenced by default
            sn_ConsoleOut( tOutput( "$spam_protection_silenced_default" ), p->Owner() );
        } else {
            // player is specially silenced
            sn_ConsoleOut( tOutput( "$spam_protection_silenced" ), p->Owner() );
        }
        return true;
    }

    return false;
}

// /me chat commant
static void se_ChatMe( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    if ( IsSilencedWithWarning(p) || spam.Block() )
    {
        return;
    }
    
    tString msg;
    msg.ReadLine( s );

    tColoredString console;
    console << tColoredString::ColorString(1,1,1)  << "* ";
    console << *p;
    console << tColoredString::ColorString(1,1,.5) << " " << msg;
    console << tColoredString::ColorString(1,1,1)  << " *";
    
    tColoredString forOldClients;
    forOldClients << tColoredString::ColorString(1,1,1)  << "*"
                  << tColoredString::ColorString(1,1,.5) << msg
                  << tColoredString::ColorString(1,1,1)  << "*";
    
    se_BroadcastChatLine( p, console, forOldClients );
    console << "\n";
    sn_ConsoleOut(console,0);
    return;
}

// /teamleave chat command: leaves the current team
static void se_ChatTeamLeave( ePlayerNetID * p )
{
    if ( se_assignTeamAutomatically )
    {
        sn_ConsoleOut(tOutput("$player_teamleave_disallowed"), p->Owner() );
        return;
    }
    if(!p->TeamChangeAllowed()) {
        sn_ConsoleOut(tOutput("$player_disallowed_teamchange"), p->Owner() );
        return;
    }
    
    eTeam * leftTeam = p->NextTeam();
    if ( leftTeam )
    {
        if ( !leftTeam )
            leftTeam = p->CurrentTeam();
        
        if ( leftTeam->NumPlayers() > 1 )
        {
            sn_ConsoleOut( tOutput( "$player_leave_team_wish",
                                    tColoredString::RemoveColors(p->GetName()),
                                    tColoredString::RemoveColors(leftTeam->Name()) ) );
        }
        else
        {
            sn_ConsoleOut( tOutput( "$player_leave_game_wish",
                                    tColoredString::RemoveColors(p->GetName()) ) );
        }
    }
    
    p->SetTeamWish(0);
}

// /team chat commant: talk to your team
static void se_ChatTeam( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    eTeam *currentTeam = se_GetManagedTeam( p );

    // silencing only affects spectators here
    if ( ( !currentTeam && IsSilencedWithWarning(p) ) || spam.Block() )
    {
        return;
    }
    
    tString msg;
    msg.ReadLine( s );

    // Log message to server and sender
    tColoredString messageForServerAndSender = se_BuildChatString(currentTeam, p, msg);
    messageForServerAndSender << "\n";
    
    if (currentTeam != NULL) // If a player has just joined the game, he is not yet on a team. Sending a /team message will crash the server
    {
        sn_ConsoleOut(messageForServerAndSender, 0);
        sn_ConsoleOut(messageForServerAndSender, p->Owner());

        // Send message to team-mates
        int numTeamPlayers = currentTeam->NumPlayers();
        for (int teamPlayerIndex = 0; teamPlayerIndex < numTeamPlayers; teamPlayerIndex++) {
            if (currentTeam->Player(teamPlayerIndex)->Owner() != p->Owner()) // Do not resend the message to yourself
                se_SendTeamMessage(currentTeam, p, currentTeam->Player(teamPlayerIndex), msg);
        }

        // check for other players who are authorized to hear the message
        for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
        {
            ePlayerNetID * admin = se_PlayerNetIDs(i);
            
            if (
                // well, you have to be a spectator. No spying on the enemy.
                se_GetManagedTeam( admin ) == 0 && admin != p &&
                (
                    // let spectating admins of sufficient rights eavesdrop
                    admin->GetAccessLevel() <=  se_teamSpyAccessLevel ||
                    // invited players are also authorized
                    currentTeam->IsInvited( admin )
                    )
                )
            {
                se_SendTeamMessage(currentTeam, p, admin, msg);
            }
        }
    }
    else
    {
        sn_ConsoleOut(messageForServerAndSender, 0);
        sn_ConsoleOut(messageForServerAndSender, p->Owner());

        // check for other spectators
        for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
        {
            ePlayerNetID * spectator = se_PlayerNetIDs(i);
            
            if ( se_GetManagedTeam( spectator ) == 0 && spectator != p )
            {
                se_SendTeamMessage(currentTeam, p, spectator, msg);
            }
        }
    }
}

// /msg chat commant: talk to anyone team
static void se_ChatMsg( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    // odd, the refactored original did not check for silence. Probably by design.
    if ( /* IsSilencedWithWarning(player) || */ spam.Block() )
    {
        return;
    }
    
    // Check for player
    ePlayerNetID * receiver =  se_FindPlayerInChatCommand( p, "/msg", s );
    
    // One match, send it.
    if ( receiver ) {
        // extract rest of message: it is the true message to send
        std::ws(s);

        // read the rest of the message
        tString msg_core;
        msg_core.ReadLine(s);
        
        // build chat string
        tColoredString toServer = se_BuildChatString( p, receiver, msg_core );
        toServer << '\n';
        
        // log locally
        sn_ConsoleOut(toServer,0);
        
        if ( p->CurrentTeam() == receiver->CurrentTeam() || !IsSilencedWithWarning(p) )
        {
            // log to sender's console
            sn_ConsoleOut(toServer, p->Owner());
            
            // send to receiver
            if ( p->Owner() != receiver->Owner() )
                se_SendPrivateMessage( p, receiver, receiver, msg_core );
        }

        // let admins of sufficient rights eavesdrop
        for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
        {
            ePlayerNetID * admin = se_PlayerNetIDs(i);
            
            if ( admin != receiver && admin != p && admin->GetAccessLevel() <=  se_msgSpyAccessLevel )
            {
                se_SendPrivateMessage( p, receiver, admin, msg_core );
            }
        }
    }
}

static void se_SendTo( std::string const & message, ePlayerNetID * receiver )
{
    if ( receiver )
    {
        sn_ConsoleOut(message.c_str(), receiver->Owner());
    }
    else
    {
        con << message;
    }
}

// prints an indented team member
static void se_SendTeamMember( ePlayerNetID const * player, int indent, std::ostream & tos )
{
    // send team name
    for( int i = indent-1; i >= 0; --i )
    {
        tos << ' ';
    }

    tos << *player << "\n";
}

// list teams with their formation
static void se_ListTeam( ePlayerNetID * receiver, eTeam * team )
{
    std::ostringstream tos;
    
    // send team name
    tColoredString teamName;
    teamName << tColoredStringProxy( team->R()/15.0f, team->G()/15.0f, team->B()/15.0f ) << team->Name();
    tos << teamName << "0xRESETT";
    if ( team->IsLocked() )
    {
        tos << " " << tOutput( "$invite_team_locked_list" );
    }
    tos << ":\n";
    
    // send team members
    int teamMembers = team->NumPlayers();
    
    int indent = 0;
    // print left wing, the odd team members
    for( int i = (teamMembers/2)*2-1; i>=0; i -= 2 )
    {
        se_SendTeamMember( team->Player(i), indent, tos );
        indent += 2;
    }
    // print right wing, the even team members
    for( int i = 0; i < teamMembers; i += 2 )
    {
        se_SendTeamMember( team->Player(i), indent, tos );
        indent -= 2;
    }
    
    tos << "\n";
    
    se_SendTo( tos.str(), receiver );
}

static void se_ListTeams( ePlayerNetID * receiver )
{
    int numTeams = 0;
    
    for ( int i = eTeam::teams.Len() - 1; i >= 0; --i )
    {
        eTeam * team = eTeam::teams[i];
        if ( team->NumPlayers() > 1 || team->IsLocked() )
        {
            numTeams++;
            se_ListTeam( receiver, team );
        }
    }

    if ( numTeams == 0 )
    {
        se_SendTo( std::string( tString( tOutput("$no_real_teams") ) ), receiver );
    }
}

static void teams_conf(std::istream &s)
{
    se_ListTeams( 0 );
}

static tConfItemFunc teams("TEAMS",&teams_conf);

// /teams gives a teams list
static void se_ChatTeams( ePlayerNetID * p )
{
    se_ListTeams( p );
}

static void se_ListPlayers( ePlayerNetID * receiver )
{
    for ( int i2 = se_PlayerNetIDs.Len()-1; i2>=0; --i2 )
    {
        ePlayerNetID* p2 = se_PlayerNetIDs(i2);
        std::ostringstream tos;
        tos << p2->Owner();
        tos << ": ";
        if ( p2->GetAccessLevel() < tAccessLevel_Default && !se_Hide( p2, receiver ) )
        {
            // player username comes from authentication name and may be much different from
            // the screen name
            tos << p2->GetFilteredAuthenticatedName() << " ( " << p2->GetName() << ", " 
                << tCurrentAccessLevel::GetName( p2->GetAccessLevel() )
                << " )";
        }
        else
        {
            tos << p2->GetName();
        }
        if ( tCurrentAccessLevel::GetAccessLevel() <= se_ipAccessLevel )
        {
            tos << ", IP = " << p2->GetMachine().GetIP();
        }
        tos << "\n";

        se_SendTo( tos.str(), receiver );
    }
}

static void players_conf(std::istream &s)
{
    se_ListPlayers( 0 );
}

static tConfItemFunc players("PLAYERS",&players_conf);

// /players gives a player list
static void se_ChatPlayers( ePlayerNetID * p )
{
    se_ListPlayers( p );
}

// team shuffling: reorders team formation
static void se_ChatShuffle( ePlayerNetID * p, std::istream & s )
{
    tString msg;
    msg.ReadLine( s );

    // team position shuffling. Allow players to change their team setup.
    // syntax:
    // /teamshuffle: shuffles you all the way to the outside.
    // /teamshuffle <pos>: shuffles you to position pos
    // /teamshuffle +/-<dist>: shuffles you dist to the outside/inside
    // con << msgRest << "\n";
    int IDNow = p->TeamListID();
    if (!p->CurrentTeam())
    {
        sn_ConsoleOut(tOutput("$player_not_on_team"), p->Owner());
        return;
    }
    int len = p->CurrentTeam()->NumPlayers();
    int IDWish = len-1; // default to shuffle to the outside
    
                        // but read the target position as additional parameter
    if (msg.Len() > 1)
    {
        IDWish = IDNow;
        if ( msg[0] == '+' )
            IDWish += msg.ToInt(1);
        else if ( msg[0] == '-' )
            IDWish -= msg.ToInt(1);
        else
            IDWish = msg.ToInt()-1;
    }

    if (IDWish < 0)
        IDWish = 0;
    if (IDWish >= len)
        IDWish = len-1;

	if(IDWish < IDNow)
	{
#ifndef KRAWALL_SERVER
		if ( !se_allowShuffleUp )
		{
			sn_ConsoleOut(tOutput("$player_noshuffleup"), p->Owner());
			return;
		}
#else
		if ( !p->GetAccessLevel() > se_shuffleUpAccessLevel )
		{
			sn_ConsoleOut(tOutput("$access_level_shuffle_up_denied"), p->Owner());
			return;
		}
#endif
	}

    if( IDNow == IDWish )
    {
        sn_ConsoleOut(tOutput("$player_noshuffle"), p->Owner());
        return;
    }

    p->CurrentTeam()->Shuffle( IDNow, IDWish );
    se_ListTeam( p, p->CurrentTeam() );
}

void handle_chat( nMessage &m )
{
    nTimeRolling currentTime = tSysTimeFloat();
    unsigned short id;
    m.Read(id);
    tColoredString say;
    m >> say;

    tJUST_CONTROLLED_PTR< ePlayerNetID > p=dynamic_cast<ePlayerNetID *>(nNetObject::ObjectDangerous(id));

    // register player activity
    if ( p )
        p->lastActivity_ = currentTime;

    if(sn_GetNetState()==nSERVER){
        if (p)
        {
            // for the duration of this function, set the access level to the level of the user.
            tCurrentAccessLevel levelOverride( p->GetAccessLevel() );

            // check if the player is owned by the sender to avoid cheating
            if( p->Owner() != m.SenderID() )
            {
                Cheater( m.SenderID() );
                nReadError();
                return;
            }

            eChatSpamTester spam( p, say );
            
            if (say.StartsWith("/")) {
                std::string sayStr(say);
                std::istringstream s(sayStr);

                tString command;
                s >> command;

                // filter to lowercase
                tToLower( command );

                tConfItemBase::EatWhitespace(s);

                // now, s is ready for reading the rest of the message.
#ifdef DEDICATED
                if (se_InterceptCommands.StrPos(command) != -1)
                {
                    handle_command_intercept(p, say);
                    return;
                }
                else
#endif
                    if (command == "/me") {
                        se_ChatMe( p, s, spam );
                        return;
                    }
                    else if (command == "/teamname") {
                        tString msg;
                        msg.ReadLine( s );
                        p->SetTeamname(msg);
                        return;
                    }
                    else if (command == "/teamleave") {
                        se_ChatTeamLeave( p );
                        return;
                    }
                    else if (command == "/teamleave" || command=="/spectate") {
                        p->SetTeamWish(NULL);
                        return;
                    }
                    else if (command == "/teamshuffle" || command == "/shuffle") {
                        se_ChatShuffle( p, s );
                        return;
                    }
                    else if (command == "/team")
                    {
                        se_ChatTeam( p, s, spam );
                        return;
                    }
                    else if (command == "/msg" ) {
                        se_ChatMsg( p, s, spam );
                        return;
                    }
                    else if (command == "/help")
                    {
                        sn_ConsoleOut(se_helpMessage + "\n", p->Owner());
                        se_DisplayChatLocally(p, say);
                        return;
                    }
                    else if (command == "/players") {
                        se_ChatPlayers( p );
                        return;
                    }
                    else if (command == "/teams") {
                        se_ChatTeams( p );
                        return;
                    }
#ifdef DEDICATED
                    else {
                        handle_chat_admin_commands( p, command, say, s );
                        return;
                    }
#endif
            }

            // check for spam
            if ( spam.Block() )
            {
                return;
            }

            // well, that leaves only regular, boring chat.
            if ( say.Len() <= se_SpamMaxLen+2 && !IsSilencedWithWarning(p) )
            {
                se_BroadcastChat( p, say );
                se_DisplayChatLocally( p, say);
            }
        }
    }
    else
    {
        se_DisplayChatLocally( p, say );
    }
}

// check if a string is a legal player name
// a name is only legal if it contains at least one non-witespace character.
static bool IsLegalPlayerName( tString const & name )
{
    // strip colors
    tString stripped( tColoredString::RemoveColors( name ) );

    // and count non-whitespace characters
    for ( int i = stripped.Len()-2; i>=0; --i )
    {
        if ( !isblank( stripped(i) ) )
            return true;
    }

    return false;
}

void ePlayerNetID::Chat(const tString &s_orig)
{
    tColoredString s( s_orig );
    s.NetFilter();

    switch (sn_GetNetState())
    {
    case nCLIENT:
        {
            if(s_orig.StartsWith("/console") ) {
                tString params("");
                if (s_orig.StrPos(" ") == -1)
                    return;
                else
                    params = s_orig.SubStr(s_orig.StrPos(" ") + 1);

                if ( tRecorder::IsPlayingBack() )
                {
                    tConfItemBase::LoadPlayback();
                }
                else
                {
                    std::stringstream s(static_cast< char const * >( params ) );
                    tConfItemBase::LoadAll(s);
                }
                std::cout << "console selected\n";
            } else
                se_NewChatMessage( this, s )->BroadCast();
            break;
        }
    case nSERVER:
        {
            se_BroadcastChat( this, s );
        }
    default:
        {
            if(s_orig.StartsWith("/console") ) {
                tString params("");
                if (s_orig.StrPos(" ") == -1)
                    return;
                else
                    params = s_orig.SubStr(s_orig.StrPos(" ") + 1);

                if ( tRecorder::IsPlayingBack() )
                {
                    tConfItemBase::LoadPlayback();
                }
                else
                {
                    std::stringstream s(static_cast< char const * >( params ) );
                    tConfItemBase::LoadAll(s);
                }
                std::cout << "console selected\n";
            } else
                se_DisplayChatLocally( this, s );

            break;
        }
    }
}

//return the playernetid for a given name
/*
static ePlayerNetID* identifyPlayer(tString inname)
{
    for(int i=0; i<se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];

        if( inname == p->GetUserName() )
            return p;
    }
    return NULL;
}
*/

// identify a local player
static ePlayerNetID* se_GetLocalPlayer()
{
    for(int i=0; i<se_PlayerNetIDs.Len(); i++)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];

        if( p->Owner() == sn_myNetID )
            return p;
    }
    return NULL;
}

static void ConsoleSay_conf(std::istream &s)
{
    // read the message
    tString message;
    message.ReadLine( s, true );

    switch (sn_GetNetState())
    {
    case nCLIENT:
        {
            ePlayerNetID *me = se_GetLocalPlayer();

            if (me)
                me->Chat( message );

            break;
        }
    case nSERVER:
        {
            tColoredString send;
            send << tColoredString::ColorString( 1,0,0 );
            send << "Admin";
            send << tColoredString::ColorString( 1,1,.5 );
            send << ": " << message << "\n";

            // display it
            sn_ConsoleOut( send );

            break;
        }
    default:
        {
            ePlayerNetID *me = se_GetLocalPlayer();

            if ( me )
                se_DisplayChatLocally( me, message );

            break;
        }
    }
}

static tConfItemFunc ConsoleSay_c("SAY",&ConsoleSay_conf);

struct eChatInsertionCommand
{
    tString insertion_;

    eChatInsertionCommand( tString const & insertion )
            : insertion_( insertion )
    {}
};


static uMenuItemStringWithHistory::history_t &se_chatHistory() {
    static uMenuItemStringWithHistory::history_t instance("chat_history.txt");
    return instance;
}
static int se_chatHistoryMaxSize=10; // maximal size of chat history
static tSettingItem< int > se_chatHistoryMaxSizeConf("HISTORY_SIZE_CHAT",se_chatHistoryMaxSize);

//! Completer with the addition that it adds a : to a fully completed name if it's at the beginning of the line
class eAutoCompleterChat : public uAutoCompleter{
public:
    //! Constructor
    //! @param words the possible completions
    eAutoCompleterChat(std::deque<tString> &words):uAutoCompleter(words) {};
    int DoFullCompletion(tString &string, int pos, int len, tString &match) {
        tString actualString;
        if(pos - len == 0 || pos - len == 6 && string.StartsWith("/team ")) {
            actualString = match + ": ";
        } else if(pos - len == 5 && string.StartsWith("/msg ") || string.StartsWith("/admin ")) {
            actualString = Simplify(match) + " ";
        } else {
            actualString = match + " ";
        }
        return DoCompletion(string, pos, len, actualString);
    }
    tString Simplify(tString const &str) {
        return ePlayerNetID::FilterName(str);
    }
};

//! Handles the chat prompt
class eMenuItemChat : protected uMenuItemStringWithHistory{
    ePlayer *me; //!< The player the chat prompt is for
public:
    //! Constructor
    //! @param M         passed on to uMenuItemStringWithHistory
    //! @param c         passed on to uMenuItemStringWithHistory
    //! @param Me        the player the chat prompt is for
    //! @param completer the completer to be used
    eMenuItemChat(uMenu *M,tString &c,ePlayer *Me,uAutoCompleter *completer):
    uMenuItemStringWithHistory(M,"$chat_title_text","",c, se_SpamMaxLen, se_chatHistory(), se_chatHistoryMaxSize, completer),me(Me) {}

    virtual ~eMenuItemChat(){ }

    virtual bool Event(SDL_Event &e){
#ifndef DEDICATED
        if (e.type==SDL_KEYDOWN &&
                (e.key.keysym.sym==SDLK_KP_ENTER || e.key.keysym.sym==SDLK_RETURN)){

            for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
                if (se_PlayerNetIDs(i)->pID==me->ID())
                    se_PlayerNetIDs(i)->Chat(*content);

            MyMenu()->Exit();
            return true;
        }
        else if (e.type==SDL_KEYDOWN &&
                 uActionGlobal::IsBreakingGlobalBind(e.key.keysym.sym))
            return su_HandleEvent(e, true);
        else
        {
            if ( uMenuItemStringWithHistory::Event(e) )
            {
                return true;
            }
            // exclude modifier keys from possible control triggers
            else if ( e.key.keysym.sym < SDLK_NUMLOCK || e.key.keysym.sym > SDLK_COMPOSE )
            {
                // maybe it's an instant chat button?
                try
                {
                    return su_HandleEvent(e, false);
                }
                catch ( eChatInsertionCommand & insertion )
                {
                    if ( content->Len() + insertion.insertion_.Len() <= maxLength_ )
                    {
                        *content = content->SubStr( 0, cursorPos ) + insertion.insertion_ + content->SubStr( cursorPos );
                        cursorPos += insertion.insertion_.Len()-1;
                    }

                    return true;
                }
            }
        }
#endif // DEDICATED

        return false;
    }
};


void se_ChatState(ePlayerNetID::ChatFlags flag, bool cs){
    for(int i=se_PlayerNetIDs.Len()-1;i>=0;i--)
    {
        ePlayerNetID *p = se_PlayerNetIDs[i];
        if (p->Owner()==sn_myNetID && p->pID >= 0){
            p->SetChatting( flag, cs );
        }
    }
}

static ePlayer * se_chatterPlanned=NULL;
static ePlayer * se_chatter =NULL;
static tString se_say;
static void do_chat(){
    if (se_chatterPlanned){
        su_ClearKeys();

        se_chatter=se_chatterPlanned;
        se_chatterPlanned=NULL;
        se_ChatState( ePlayerNetID::ChatFlags_Chat, true);

        sr_con.SetHeight(15,false);
        se_SetShowScoresAuto(false);

        uMenu chat_menu("",false);

        std::deque<tString> players;
        unsigned short int max=se_PlayerNetIDs.Len();
        for(unsigned short int i=0;i<max;i++){
            ePlayerNetID *p=se_PlayerNetIDs(i);
            players.push_back(p->GetName());
        }
        eAutoCompleterChat completer(players);


        eMenuItemChat s(&chat_menu,se_say,se_chatter,&completer);
        chat_menu.SetCenter(-.75);
        chat_menu.SetBot(-2);
        chat_menu.SetTop(-.7);
        chat_menu.Enter();

        se_ChatState( ePlayerNetID::ChatFlags_Chat, false );

        sr_con.SetHeight(7,false);
        se_SetShowScoresAuto(true);
    }
    se_chatter=NULL;
    se_chatterPlanned=NULL;
}

static void chat( ePlayer * chatter, tString const & say )
{
    se_chatterPlanned = chatter;
    se_say = say;
    st_ToDo(&do_chat);
}

static void chat( ePlayer * chatter )
{
    chat( chatter, tString() );
}

static bool se_allowControlDuringChat = false;
static nSettingItem<bool> se_allowControlDuringChatConf("ALLOW_CONTROL_DURING_CHAT",se_allowControlDuringChat);

uActionPlayer se_toggleSpectator("TOGGLE_SPECTATOR", 0);

bool ePlayer::Act(uAction *act,REAL x){
    eGameObject *object=NULL;

    if ( act == &se_toggleSpectator && x > 0 )
    {
        spectate = !spectate;
        con << tOutput(spectate ? "$player_toggle_spectator_on" : "$player_toggle_spectator_off", name );
        if ( !spectate )
        {
            ePlayerNetID::Update();
        }
        return true;
    }
    else if (!se_chatter && s_chat==*reinterpret_cast<uActionPlayer *>(act)){
        if(x>0) {
            chat( this );
        }
        return true;
    }
    else{
        if ( x > 0 )
        {
            int i;
            uActionPlayer* pact = reinterpret_cast<uActionPlayer *>(act);
            for(i=MAX_INSTANT_CHAT-1;i>=0;i--){
                uActionPlayer* pcompare = se_instantChatAction[i];
                if (pact == pcompare && x>=0){
                    for(int j=se_PlayerNetIDs.Len()-1;j>=0;j--)
                        if (se_PlayerNetIDs(j)->pID==ID())
                        {
                            tString say = instantChatString[i];
                            bool sendImmediately = true;
                            if ( say.Len() > 2 && say[say.Len()-2] == '\\' )
                            {
                                // cut away trailing slash and note for later
                                // that the message should not be sent immediately.
                                say = say.SubStr( 0, say.Len()-2 );
                                sendImmediately = false;
                            }

                            if ( se_chatter == this )
                            {
                                // a chat is already active, insert the chat string
                                throw eChatInsertionCommand( say );
                            }
                            else
                            {
                                if ( sendImmediately )
                                {
                                    // fire out chat string immediately
                                    se_PlayerNetIDs(j)->Chat(say);
                                }
                                else
                                {
                                    // chat with instant chat string as template
                                    chat( this, say );
                                }
                            }
                            return true;
                        }
                }
            }
        }

        // no other command should get through during chat
        if ( se_chatter && !se_allowControlDuringChat )
            return false;

        int i;
        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--)
            if (se_PlayerNetIDs[i]->pID==id && se_PlayerNetIDs[i]->object)
                object=se_PlayerNetIDs[i]->object;

        bool objectAct = false; // set to true if an action of the object was triggered
        bool ret = ((cam    && cam->Act(reinterpret_cast<uActionCamera *>(act),x)) ||
                    ( objectAct = (object && se_GameTime()>=0 && object->Act(reinterpret_cast<uActionPlayer *>(act),x))));

        if (bool(netPlayer) && (objectAct || !se_chatter))
            netPlayer->Activity();

        return ret;
    }

}

rViewport * ePlayer::PlayerViewport(int p){
    if (!PlayerConfig(p)) return NULL;

    for (int i=rViewportConfiguration::CurrentViewportConfiguration()->num_viewports-1;i>=0;i--)
        if (sr_viewportBelongsToPlayer[i] == p)
            return rViewportConfiguration::CurrentViewport(i);

    return NULL;
}

bool ePlayer::PlayerIsInGame(int p){
    return PlayerViewport(p) && PlayerConfig(p);
}

static tConfItemBase *vpbtp[MAX_VIEWPORTS];

void ePlayer::Init(){
    se_Players = tNEW( ePlayer[MAX_PLAYERS] );

    int i;
    for(i=MAX_INSTANT_CHAT-1;i>=0;i--){
        tString id;
        id << "INSTANT_CHAT_";
        id << i+1;
        tOutput desc;
        desc.SetTemplateParameter(1, i+1);
        desc << "$input_instant_chat_text";

        tOutput help;
        help.SetTemplateParameter(1, i+1);
        help << "$input_instant_chat_help";
        ePlayer::se_instantChatAction[i]=tNEW(uActionPlayer) (id, desc, help);
        //,desc,       "Issues a special instant chat macro.");
    }


    for(i=MAX_VIEWPORTS-1;i>=0;i--){
        tString id;
        id << "VIEWPORT_TO_PLAYER_";
        id << i+1;
        vpbtp[i] = tNEW(tConfItem<int>(id,"$viewport_belongs_help",
                                       s_newViewportBelongsToPlayer[i]));
        s_newViewportBelongsToPlayer[i]=i;
    }
}

void ePlayer::Exit(){
    int i;
    for(i=MAX_INSTANT_CHAT-1;i>=0;i--)
        tDESTROY(ePlayer::se_instantChatAction[i]);

    for(i=MAX_VIEWPORTS-1;i>=0;i--)
        tDESTROY(vpbtp[i]);

    delete[] se_Players;
    se_Players = NULL;
}

uActionPlayer ePlayer::s_chat("CHAT");

int pingCharity = 100;
static const int maxPingCharity = 300;

static void sg_ClampPingCharity( int & pingCharity )
{
    if (pingCharity < 0 )
        pingCharity = 0;
    if (pingCharity > maxPingCharity )
        pingCharity = maxPingCharity;
}

static void sg_ClampPingCharity( unsigned short & pingCharity )
{
    if (pingCharity > maxPingCharity )
        pingCharity = maxPingCharity;
}

static void sg_ClampPingCharity()
{
    sg_ClampPingCharity( ::pingCharity );
}

static int IMPOSSIBLY_LOW_SCORE=(-1 << 31);

static nSpamProtectionSettings se_chatSpamSettings( 1.0f, "SPAM_PROTECTION_CHAT", tOutput("$spam_protection") );

ePlayerNetID::ePlayerNetID(int p):nNetObject(),listID(-1), teamListID(-1), allowTeamChange_(false), registeredMachine_(0), pID(p),chatSpam_( se_chatSpamSettings )
{
    // default access level
    lastAccessLevel = tAccessLevel_Default;

    favoriteNumberOfPlayersPerTeam = 1;

    nameTeamAfterMe = false;

    r = g = b = 15;

    greeted				= true;
    chatting_			= false;
    spectating_         = false;
    stealth_            = false;
    chatFlags_			= 0;
    disconnected		= false;

    loginWanted = false;
    

    if (p>=0){
        ePlayer *P = ePlayer::PlayerConfig(p);
        if (P){
            SetName( P->Name() );
            teamname = P->Teamname();
            r=   P->rgb[0];
            g=   P->rgb[1];
            b=   P->rgb[2];

            sg_ClampPingCharity();
            pingCharity=::pingCharity;

            loginWanted = P->autoLogin;
        }
    }
    /*
    else
    {
        SetName( "AI" );
    teamname = "";
    }
    */
    lastSaid.SetLen(12);
    lastSaidTimes.SetLen(12);

    se_PlayerNetIDs.Add(this,listID);
    object=NULL;

    /*
      if(sn_GetNetState()!=nSERVER)
      ping=sn_Connections[0].ping;
      else
    */
    ping=0; // hehe! server has no ping.

    lastSync=tSysTimeFloat();

    RequestSync();
    score=0;
    lastScore_=IMPOSSIBLY_LOW_SCORE;
    // rubberstatus=0;

    MyInitAfterCreation();

    if(sn_GetNetState()==nSERVER)
        RequestSync();
}




ePlayerNetID::ePlayerNetID(nMessage &m):nNetObject(m),listID(-1), teamListID(-1)
        , allowTeamChange_(false), registeredMachine_(0), chatSpam_( se_chatSpamSettings )
{
    // default access level
    lastAccessLevel = tAccessLevel_Default;

    greeted     =false;
    chatting_   =false;
    spectating_ =false;
    stealth_    =false;
    disconnected=false;
    chatFlags_	=0;

    r = g = b = 15;

    nameTeamAfterMe = false;
    teamname = "";

    lastSaid.SetLen(12);
    lastSaidTimes.SetLen(12);

    pID=-1;
    se_PlayerNetIDs.Add(this,listID);
    object=NULL;
    ping=sn_Connections[m.SenderID()].ping;
    lastSync=tSysTimeFloat();
 
    loginWanted = false;

    score=0;
    lastScore_=IMPOSSIBLY_LOW_SCORE;
    // rubberstatus=0;
}

void ePlayerNetID::Activity()
{
    // the player was active; therefore, he cannot possibly be chatting_ or disconnected.

    // but do nothing if we are in client mode and the player is not local to this computer
    if (sn_GetNetState() != nSERVER && Owner() != ::sn_myNetID)
        return;

    if (chatting_ || disconnected)
    {
#ifdef DEBUG
        con << *this << " showed activity and lost chat status.\n";
#endif
        RequestSync();
    }

    chatting_ = disconnected = false;

    // store time
    this->lastActivity_ = tSysTimeFloat();
}

static int se_maxPlayersPerIP = 4;
static tSettingItem<int> se_maxPlayersPerIPConf( "MAX_PLAYERS_SAME_IP", se_maxPlayersPerIP );

// array of players in legacy spectator mode: they have been
// deleted by their clients, but no new player has popped up for them yet
static tJUST_CONTROLLED_PTR< ePlayerNetID > se_legacySpectators[MAXCLIENTS+2];

static void se_ClearLegacySpectator( int owner )
{
    ePlayerNetID * player = se_legacySpectators[ owner ];

    // fully remove player
    if ( player )
    {
        player->RemoveFromGame();
    }

    se_legacySpectators[ owner ] = NULL;
}

// callback clearing the legacy spectator when a client enters or leaves
static void se_LegacySpectatorClearer()
{
    se_ClearLegacySpectator( nCallbackLoginLogout::User() );
}
static nCallbackLoginLogout se_legacySpectatorClearer( se_LegacySpectatorClearer );



void ePlayerNetID::MyInitAfterCreation()
{
    this->CreateVoter();

    this->silenced_ = se_silenceAll;

    // register with machine and kick user if too many players are present
    if ( Owner() != 0 && sn_GetNetState() == nSERVER )
    {
        this->RegisterWithMachine();
        if ( se_maxPlayersPerIP < nMachine::GetMachine(Owner()).GetPlayerCount() )
        {
            // kill them
            sn_DisconnectUser( Owner(), "$network_kill_too_many_players" );

            // technically has the same effect as the above function, but we also want
            // to abort registering this player object and this exception will do that as well.
            throw nKillHim();
        }

        // clear old legacy spectator that may be lurking around
        se_ClearLegacySpectator( Owner() );
    }

    this->wait_ = 0;

    this->lastActivity_ = tSysTimeFloat();
}

void ePlayerNetID::InitAfterCreation()
{
    MyInitAfterCreation();
}

bool ePlayerNetID::ClearToTransmit(int user) const{
    return ( ( !nextTeam || nextTeam->HasBeenTransmitted( user ) ) &&
             ( !currentTeam || currentTeam->HasBeenTransmitted( user ) ) ) ;
}

ePlayerNetID::~ePlayerNetID()
{
    //	se_PlayerNetIDs.Remove(this,listID);
    if ( sn_GetNetState() == nSERVER && disconnected )
    {
        tOutput mess;
        tColoredString name;
        name << *this << tColoredString::ColorString(1,.5,.5);
        mess.SetTemplateParameter(1, name);
        mess.SetTemplateParameter(2, score);
        mess << "$player_left_game";
        sn_ConsoleOut(mess);
    }

    UnregisterWithMachine();

    RemoveFromGame();

    ClearObject();
    //con << "Player info sent.\n";

    for(int i=MAX_PLAYERS-1;i>=0;i--){
        ePlayer *p = ePlayer::PlayerConfig(i);

        if (p && static_cast<ePlayerNetID *>(p->netPlayer)==this)
            p->netPlayer=NULL;
    }

    if ( currentTeam )
    {
        currentTeam->RemovePlayer( this );
    }

#ifdef DEBUG
    con << *this << " destroyed.\n";
#endif
}

static void player_removed_from_game_handler(nMessage &m)
{
    // and the ID of the player that was removed
    unsigned short id;
    m.Read(id);
    ePlayerNetID* p = dynamic_cast< ePlayerNetID* >( nNetObject::ObjectDangerous( id ) );
    if ( p && sn_GetNetState() != nSERVER )
    {
        p->RemoveFromGame();
    }
}

static nDescriptor player_removed_from_game(202,&player_removed_from_game_handler,"player_removed_from_game");

void ePlayerNetID::RemoveFromGame()
{
    // unregister with the machine
    this->UnregisterWithMachine();

    // release voter
    if ( this->voter_ )
        this->voter_->RemoveFromGame();
    this->voter_ = 0;

    // log scores
    LogScoreDifference();

    if ( sn_GetNetState() != nCLIENT )
    {
        nameFromClient_ = nameFromServer_;

        nMessage *m=new nMessage(player_removed_from_game);
        m->Write(this->ID());
        m->BroadCast();

        if ( listID >= 0 ){
            if ( ( IsSpectating() || !se_assignTeamAutomatically ) && CurrentTeam() == NULL )
            {
                // get colored player name
                tColoredString playerName;
                playerName << *this << tColoredString::ColorString(1,.5,.5);

                // announce a generic leave
                sn_ConsoleOut( tOutput( "$player_left_spectator", playerName ) );
            }

            if ( IsHuman() && sn_GetNetState() == nSERVER && NULL != sn_Connections[Owner()].socket )
            {
                tString ladder;
                ladder << "PLAYER_LEFT " << userName_ << " " << nMachine::GetMachine(Owner()).GetIP() << "\n";
                se_SaveToLadderLog(ladder);
                tString notificationMessage(userName_);
                notificationMessage << " left the grid";
                se_sendEventNotification(tString("Player left"), notificationMessage);
            }
        }
    }

    se_PlayerNetIDs.Remove(this, listID);
    SetTeamWish( NULL );
    SetTeam( NULL );
    UpdateTeam();
    ControlObject( NULL );
    //	currentTeam = NULL;
}

bool ePlayerNetID::ActionOnQuit()
{
    tControlledPTR< ePlayerNetID > holder( this );

    // clear legacy spectator
    se_ClearLegacySpectator( Owner() );

    this->RemoveFromGame();
    return true;
}

void ePlayerNetID::ActionOnDelete()
{
    tControlledPTR< ePlayerNetID > holder( this );

    if ( sn_GetNetState() == nSERVER )
    {
        // get the number of players registered from this client
        // int playerCount = nMachine::GetMachine(Owner()).GetPlayerCount();
        int playerCount = 0;
        for ( int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
        {
            if ( se_PlayerNetIDs[i]->Owner() == Owner() )
                playerCount++;
        }

        // if this is the last player, store it
        if ( playerCount == 1 )
        {
            // log scores
            LogScoreDifference();

            // clear legacy spectator
            se_ClearLegacySpectator( Owner() );

            // store new legacy spectator
            se_legacySpectators[ Owner() ] = this;
            spectating_ = true;

            // quit team already
            SetTeamWish( NULL );
            SetTeam( NULL );
            UpdateTeam();

            // and kill controlled object
            ControlObject( NULL );

            // inform other clients that the player left
            TakeOwnership();
            RequestSync();

            return;
        }
    }

    // standard behavior: just get us out of here
    this->RemoveFromGame();
}

void ePlayerNetID::PrintName(tString &s) const
{
    s << "ePlayerNetID nr. " << ID() << ", name " << GetName();
}


bool ePlayerNetID::AcceptClientSync() const{
    return true;
}

#ifdef KRAWALL_SERVER

// changes various user aspects
template< class P > class eUserConfig: public tConfItemBase
{
public:
    P Get(tString const & name ) const
    {
        typename Properties::const_iterator iter = properties.find( name );
        if ( iter != properties.end() )
        {
            return (*iter).second;
        }
        
        return GetDefault();
    }
protected:
    typedef std::map< tString, P > Properties;

   eUserConfig( char const * name )
    : tConfItemBase( name )
    {
        requiredLevel = tAccessLevel_Owner;
    }

    virtual P ReadRawVal(tString const & name, std::istream &s) const = 0;
    virtual P GetDefault() const = 0;
    virtual void TransformName( tString & name ) const {};

    virtual void ReadVal(std::istream &s)
    {
        tString name;
        s >> name;
        
        TransformName( name );
        
        P value = ReadRawVal(name, s);
        
        properties[name] = value;
    }
    
    Properties properties;
private:

    virtual void WriteVal(std::ostream &s)
    {
        tASSERT(0);
    }
    
    virtual bool Writable(){
        return false;
    }

    virtual bool Save(){
        return false;
    }
};

// changes the access level of a player
class eUserLevel: public eUserConfig< tAccessLevel >
{
public:
    eUserLevel()
    : eUserConfig< tAccessLevel >( "USER_LEVEL" )
    {
    }

    tAccessLevel GetDefault() const
    {
        return tAccessLevel_Authenticated;
    }

    virtual tAccessLevel ReadRawVal(tString const & name, std::istream &s) const
    {
        int levelInt;
        s >> levelInt;
        tAccessLevel level = static_cast< tAccessLevel >( levelInt );
        
        if ( s.fail() )
        {
            con << tOutput( "$user_level_usage" );
            return GetDefault();
        }

        con << tOutput( "$user_level_change", name, tCurrentAccessLevel::GetName( level ) );

        return level;
    }
};

static eUserLevel se_userLevel;

// reserves a nickname
class eReserveNick: public eUserConfig< tString >
{
#ifdef DEBUG
    static void TestEscape()
    {
#ifdef KRAWALL_SERVER
        tString test("ä@%bla:");
        tString esc(se_EscapeName( test, false ).c_str());
        tString rev(se_UnEscapeName( esc ).c_str());
        tASSERT( test == rev );
#endif
    }
#endif
public:
    eReserveNick()
    : eUserConfig< tString >( "RESERVE_SCREEN_NAME" )
    {
#ifdef DEBUG
        TestEscape();
#endif
    }
    
    // filter the name so it is compatible with old school user names; those
    // will be used later for comparison
    virtual void TransformName( tString & name ) const
    {
        name = ePlayerNetID::FilterName( name );
    }

    tString GetDefault() const
    {
        return tString();
    }

    virtual tString ReadRawVal(tString const & name, std::istream &s) const
    {
        tString user;
        s >> user;

        if ( user == "" )
        {
            con << tOutput( "$reserve_screen_name_usage" );
            return GetDefault();
        }

        user = se_UnEscapeName( user ).c_str();

        con << tOutput( "$reserve_screen_name_change", name, user );

        return user;
    }
};

static eReserveNick se_reserveNick;

// authentication alias to map a local account to a global one
class eAlias: public eUserConfig< tString >
{
public:
    eAlias()
    : eUserConfig< tString >( "USER_ALIAS" )
    {
    }

    tString GetDefault() const
    {
        return tString();
    }

    virtual tString ReadRawVal(tString const & name, std::istream &s) const
    {
        tString alias;
        s >> alias;

        if ( alias == "" )
        {
            con << tOutput( "$alias_usage" );
            return GetDefault();
        }

        alias = se_UnEscapeName( alias ).c_str();

        con << tOutput( "$alias_change", name, alias );

        return alias;
    }
};

static eAlias se_alias;

// bans for players too stupid to disable autologin
class eBanUser: public eUserConfig< bool >
{
public:
    eBanUser()
    : eUserConfig< bool >( "BAN_USER" )
    {
    }

    // unbans the user again
    void UnBan( tString const & name )
    {
        properties[name] = false;
        con << tOutput( "$unban_user_message", name );
    }

    void List()
    {
        bool one = false;
        for ( Properties::iterator i = properties.begin(); i != properties.end(); ++i )
        {
            if ( (*i).second )
            {
                if ( one )
                {
                    con << ", ";
                }
                con << (*i).first;
                one = true;
            }
        }
        if ( one )
        {
            con << "\n";
        }
    }

    bool GetDefault() const
    {
        return false;
    }

    virtual bool ReadRawVal(tString const & name, std::istream &s) const
    {
        con << tOutput( "$ban_user_message", name );
        return true;
    }
};

static eBanUser se_banUser;

// check whether a special username is banned
static bool se_IsUserBanned( ePlayerNetID * p, tString const & name )
{
    if( se_banUser.Get( name ) )
    {
        sn_KickUser( p->Owner(), tOutput( "$network_kill_banned", 60, "" ) );
        return true;
    }

    return false;
}

class eUnBanUser: public eUserConfig< bool >
{
public:
    eUnBanUser( )
    : eUserConfig< bool >( "UNBAN_USER" )
    {
    }

    bool GetDefault() const
    {
        return false;
    }

    virtual bool ReadRawVal(tString const & name, std::istream &s) const
    {
        se_banUser.UnBan(name);
        return false;
    }
};

static eUnBanUser se_unBanUser;

static void se_ListBannedUsers( std::istream & )
{
    se_banUser.List();
}

static tConfItemFunc sn_listBanConf("BAN_USER_LIST",&se_ListBannedUsers);

static void se_CheckAccessLevel( tAccessLevel & level, tString const & authName )
{
    tAccessLevel newLevel = se_userLevel.Get( authName );
    if ( newLevel < level )
    {
        level = newLevel;
    }
}

void ePlayerNetID::Authenticate( tString const & authName, tAccessLevel accessLevel_, ePlayerNetID const * admin )
{
    tString newAuthenticatedName( se_EscapeName( authName ).c_str() );

    // no use authenticating an AI :)
    if ( !IsHuman() )
    {
        return;
    }

    if ( !IsAuthenticated() )
    {
        // elevate access level for registered users
        se_CheckAccessLevel( accessLevel_, newAuthenticatedName );

        rawAuthenticatedName_ = authName;
        tString alias = se_alias.Get( newAuthenticatedName );
        if ( alias != "" )
        {
            rawAuthenticatedName_ = alias;
            newAuthenticatedName = GetFilteredAuthenticatedName();

            // elevate access level again according to the new alias
            se_CheckAccessLevel( accessLevel_, newAuthenticatedName );
        }

        // check for stupid bans
        if ( se_IsUserBanned( this, newAuthenticatedName ) )
        {
            return;
        }

        // minimal access level
        if ( accessLevel_ > tAccessLevel_Authenticated )
        {
            accessLevel_ = tAccessLevel_Authenticated;
        }

        // take over the access level
        SetAccessLevel( accessLevel_ );

        tString order( "" );
        if ( admin )
        {
            order = tOutput( "$login_message_byorder", 
                             admin->GetLogName() );
        }

        if ( IsHuman() )
        {
            if ( GetAccessLevel() < tAccessLevel_Authenticated )
            {
                se_SecretConsoleOut( tOutput( "$login_message_special", 
                                              GetName(), 
                                              newAuthenticatedName,
                                              tCurrentAccessLevel::GetName( GetAccessLevel() ),
                                              order ), this, admin );
            }
            else
            {
                se_SecretConsoleOut( tOutput( "$login_message", GetName(), newAuthenticatedName, order ), this, admin );
            }

        }
    }

    GetScoreFromDisconnectedCopy();
}

void ePlayerNetID::DeAuthenticate( ePlayerNetID const * admin ){
    if ( IsAuthenticated() )
    {
        if ( admin )
        {
            se_SecretConsoleOut( tOutput( "$logout_message_deop", GetName(), GetFilteredAuthenticatedName(), admin->GetLogName() ), this, admin );
        }
        else
        {
            se_SecretConsoleOut( tOutput( "$logout_message", GetName(), GetFilteredAuthenticatedName() ), this );
        }
    }

    // force falling back to regular user name on next update
    SetAccessLevel( tAccessLevel_Default );

    rawAuthenticatedName_ = "";
}

bool ePlayerNetID::IsAuthenticated() const
{
    return int(GetAccessLevel()) <= int(tAccessLevel_Authenticated);
}
#endif

// create our voter or find it
void ePlayerNetID::CreateVoter()
{
    // only count nonlocal players with voting support as voters
    if ( sn_GetNetState() != nCLIENT && this->Owner() != 0 && sn_Connections[ this->Owner() ].version.Max() >= 3 )
    {
        this->voter_ = eVoter::GetVoter( this->Owner() );
        if ( this->voter_ )
            this->voter_->PlayerChanged();
    }
}

void ePlayerNetID::WriteSync(nMessage &m){
    lastSync=tSysTimeFloat();
    nNetObject::WriteSync(m);
    m.Write(r);
    m.Write(g);
    m.Write(b);

    // write ping charity; spectators get a fake (high) value
    if ( currentTeam || nextTeam )
        m.Write(pingCharity);
    else
        m.Write(1000);

    if ( sn_GetNetState() == nCLIENT )
    {
        m << nameFromClient_;
    }
    else
    {
        m << nameFromServer_;
    }

    //if(sn_GetNetState()==nSERVER)
    m << ping;

    // pack chat, spectator and stealth status together
    unsigned short flags = ( chatting_ ? 1 : 0 ) | ( spectating_ ? 2 : 0 ) | ( stealth_ ? 4 : 0 );
    m << flags;

    m << score;
    m << static_cast<unsigned short>(disconnected);

    m << nextTeam;
    m << currentTeam;

    m << favoriteNumberOfPlayersPerTeam;
    m << nameTeamAfterMe;
    //TODO: Only update between rounds
    m << teamname;
}

// makes sure the passed string is not longer than the given maximum
static void se_CutString( tColoredString & string, int maxLen )
{
    if (string.Len() > maxLen )
    {
        string = string.SubStr(0, maxLen);
        //string[string.Len()-1]='\0';
        string.RemoveTrailingColor();
    }
}

static void se_CutString( tString & string, int maxLen )
{
    se_CutString( reinterpret_cast< tColoredString & >( string ), maxLen );
}

static bool se_bugColorOverflow=true;
tSettingItem< bool > se_bugColorOverflowColor( "BUG_COLOR_OVERFLOW", se_bugColorOverflow );
void Clamp( unsigned short & colorComponent )
{
    if ( colorComponent > 15 )
        colorComponent = 15;
}

// function prototype for character testing functions
typedef bool TestCharacter( char c );

// strips characters matching a test beginnings and ends of names
static void se_StripMatchingEnds( tString & stripper, TestCharacter & beginTester, TestCharacter & endTester )
{
    int len = stripper.Size() - 1;
    int first = 0, last = len;

    // eat whitespace from beginnig and end
    while ( first < len && beginTester( stripper[first] ) ) ++first;
    while ( last > 0 && ( !stripper[last] || endTester(stripper[last] ) ) ) --last;

    // strip everything?
    if ( first > last )
    {
        stripper = "";
        return;
    }

    // strip
    if ( first > 0 || last < stripper.Len() - 1 )
        stripper = stripper.SubStr( first, last + 1 - first );
}

// removed convenience function, VisualC 6 cannot cope with it...
// strips characters matching a test beginnings and ends of names
//static void se_StripMatchingEnds( tString & stripper, TestCharacter & tester )
//{
//    se_StripMatchingEnds( stripper, tester, tester );
//}

// function wrapper for what may be a macro
static bool se_IsBlank( char c )
{
    return isblank( c );
}

// enf of player names should neither be space or :
static bool se_IsInvalidNameEnd( char c )
{
    return isblank( c ) || c == ':' || c == '.';
}

// filter name ends
static void se_StripNameEnds( tString & name )
{
    se_StripMatchingEnds( name, se_IsBlank, se_IsInvalidNameEnd );
}

// test if a user name is used by anyone else than the passed player
static bool se_IsNameTaken( tString const & name, ePlayerNetID const * exception )
{
    if ( name.Len() <= 1 )
        return false;

    // check for other players with the same name
    for (int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID * player = se_PlayerNetIDs(i);
        if ( player != exception )
        {
            if ( name == player->GetUserName() || name == ePlayerNetID::FilterName( player->GetName() ) )
                return true;
        }
    }

#ifdef KRAWALL_SERVER
    // check for reserved nicknames
    if ( !exception )
    {
        tString reservedFor = se_reserveNick.Get( name );
        if ( reservedFor != "" && 
             ( !exception || exception->GetAccessLevel() >= tAccessLevel_Default ||
               exception->GetRawAuthenticatedName() != reservedFor ) )
        {
            return true;
        }
    }
#endif

    return false;
}

static bool se_allowImposters = false;
static tSettingItem< bool > se_allowImposters1( "ALLOW_IMPOSTERS", se_allowImposters );
static tSettingItem< bool > se_allowImposters2( "ALLOW_IMPOSTORS", se_allowImposters );

static bool se_filterColorNames=false;
tSettingItem< bool > se_coloredNamesConf( "FILTER_COLOR_NAMES", se_filterColorNames );
static bool se_stripNames=true;
tSettingItem< bool > se_stripConf( "FILTER_NAME_ENDS", se_stripNames );
static bool se_stripMiddle=true;
tSettingItem< bool > se_stripMiddleConf( "FILTER_NAME_MIDDLE", se_stripMiddle );

// do the optional filtering steps
static void se_OptionalNameFilters( tString & remoteName )
{
    // filter colors
    if ( se_filterColorNames )
    {
        remoteName = tColoredString::RemoveColors( remoteName );
    }

    // don't do the fancy stuff on the client, it only makes names on score tables and
    // console messages go out of sync.
    if ( sn_GetNetState() == nCLIENT )
        return;

    // strip whitespace
    if ( se_stripNames )
        se_StripNameEnds( remoteName );

    if ( se_stripMiddle )
    {
        // first, scan for double whitespace
        bool whitespace=false;
        int i;
        for ( i = 0; i < remoteName.Len()-1; ++i )
        {
            bool newWhitespace=isblank(remoteName[i]);
            if ( newWhitespace && whitespace )
            {
                // yes, double whitespace there. Filter it out.
                whitespace=newWhitespace=false;
                tString copy;
                for ( i = 0; i < remoteName.Len()-1; ++i )
                {
                    char c = remoteName[i];
                    newWhitespace=isblank(c);
                    if ( !whitespace || !newWhitespace )
                    {
                        copy+=c;
                    }
                    whitespace=newWhitespace;
                }
                remoteName=copy;

                // abort loop.
                break;
            }

            whitespace=newWhitespace;
        }
    }

    // zero strings or color code only strings are illegal
    if ( !IsLegalPlayerName( remoteName ) )
    {
        tString oldName = remoteName;

        // revert to old name if possible
        if ( IsLegalPlayerName( oldName ) )
        {
            remoteName = oldName;
        }
        else
        {
            // or replace it by a default value
            // (no, not bad localization, this is only a punishment for people who think they're smart.)
            remoteName = "Player 1";
        }
    }
}

void ePlayerNetID::ReadSync(nMessage &m){
    // check whether this is the first sync
    bool firstSync = ( this->ID() == 0 );

    nNetObject::ReadSync(m);

    m.Read(r);
    m.Read(g);
    m.Read(b);

    if ( !se_bugColorOverflow )
    {
        // clamp color values
        Clamp(r);
        Clamp(g);
        Clamp(b);
    }

    m.Read(pingCharity);
    sg_ClampPingCharity(pingCharity);

    // name as sent from the other end
    tString & remoteName = ( sn_GetNetState() == nCLIENT ) ? nameFromServer_ : nameFromClient_;

    // name handling
    {
        // read and shorten name, but don't update it yet
        m >> remoteName;

        // filter
        se_OptionalNameFilters( remoteName );

        se_CutString( remoteName, 16 );
    }

    // directly apply name changes sent from the server, they are safe.
    if ( sn_GetNetState() != nSERVER )
    {
        UpdateName();
    }

    REAL p;
    m >> p;
    if (sn_GetNetState()!=nSERVER)
        ping=p;

    //  if (!m.End())
    {
        // read chat and spectator status
        unsigned short flags;
        m >> flags;

        if (Owner() != ::sn_myNetID)
        {
            bool newChat = ( ( flags & 1 ) != 0 );
            bool newSpectate = ( ( flags & 2 ) != 0 );
            bool newStealth = ( ( flags & 4 ) != 0 );

            if ( chatting_ != newChat || spectating_ != newSpectate || newStealth != stealth_ )
                lastActivity_ = tSysTimeFloat();
            chatting_   = newChat;
            spectating_ = newSpectate;
            stealth_    = newStealth;
        }
    }

    //  if (!m.End())
    {
        if(sn_GetNetState()!=nSERVER)
            m >> score;
        else{
            int s;
            m >> s;
        }
    }

    if (!m.End()){
        unsigned short newdisc;
        m >> newdisc;

        if (Owner() != ::sn_myNetID && sn_GetNetState()!=nSERVER)
            disconnected = newdisc;
    }

    if (!m.End())
    {
        if ( nSERVER != sn_GetNetState() )
        {
            eTeam *newCurrentTeam, *newNextTeam;

            m >> newNextTeam;
            m >> newCurrentTeam;

            // update team
            if ( newCurrentTeam != currentTeam )
            {
                if ( newCurrentTeam )
                    // automatically removed player from currentTeam
                    newCurrentTeam->AddPlayerDirty( this );
                else
                    currentTeam->RemovePlayer( this );
            }
            // update nextTeam
            nextTeam = newNextTeam;
        }
        else
        {
            eTeam* t;
            m >> t;
            m >> t;
        }

        m >> favoriteNumberOfPlayersPerTeam;
        m >> nameTeamAfterMe;
    }
    if (!m.End())
    {
        m >> teamname;
    }
    // con << "Player info updated.\n";

    // make sure we did not accidentally overwrite values
    // ePlayer::Update();

    // update the team
    if ( nSERVER == sn_GetNetState() )
    {
        if ( nextTeam )
            nextTeam->UpdateProperties();

        if ( currentTeam )
            currentTeam->UpdateProperties();
    }

    // first sync message
    if ( firstSync && sn_GetNetState() == nSERVER )
    {
        UpdateName();

#ifndef KRAWALL_SERVER
        GetScoreFromDisconnectedCopy();
#endif
        SetDefaultTeam();

        RequestSync();
    }

}


nNOInitialisator<ePlayerNetID> ePlayerNetID_init(201,"ePlayerNetID");

nDescriptor &ePlayerNetID::CreatorDescriptor() const{
    return ePlayerNetID_init;
}



void ePlayerNetID::ControlObject(eNetGameObject *c){
    if (bool(object) && c!=object)
        ClearObject();

    if (!c)
    {
        return;
    }


    object=c;
    c->team = currentTeam;

    if (bool(object))
        object->SetPlayer(this);
#ifdef DEBUG
    //con << "Player " << name << " controlles new object.\n";
#endif

    NewObject();
}

void ePlayerNetID::ClearObject(){
    if (object)
    {
        tJUST_CONTROLLED_PTR< eNetGameObject > x=object;
        object=NULL;
        x->RemoveFromGame();
        x->SetPlayer( NULL );
    }
#ifdef DEBUG
    //con << "Player " << name << " controlles nothing.\n";
#endif
}

void ePlayerNetID::Greet(){
    if (!greeted){
        tOutput o;
        o.SetTemplateParameter(1, GetName() );
        o.SetTemplateParameter(2, sn_programVersion);
        o << "$player_welcome";
        tString s;
        s << o;
        s << "\n";
        GreetHighscores(s);
        s << "\n";
        //std::cout << s;
        sn_ConsoleOut(s,Owner());
        greeted=true;
    }
}

eNetGameObject *ePlayerNetID::Object() const{
    return object;
}

static bool se_consoleLadderLog = false;
static tSettingItem< bool > se_consoleLadderLogConf( "CONSOLE_LADDER_LOG", se_consoleLadderLog );

void se_SaveToLadderLog( tOutput const & out )
{
    if (se_consoleLadderLog)
    {
        std::cout << "[L] " << out;
        std::cout.flush();
    }
    if (sn_GetNetState()!=nCLIENT && !tRecorder::IsPlayingBack())
    {
        std::ofstream o;
        if ( tDirectories::Var().Open(o, "ladderlog.txt", std::ios::app) )
            o << out;
    }
}

void se_SaveToScoreFile(const tOutput &o){
    tString s(o);

#ifdef DEBUG
    if (sn_GetNetState()!=nCLIENT){
#else
    if (sn_GetNetState()==nSERVER && !tRecorder::IsPlayingBack()){
#endif

        std::ofstream o;
        if ( tDirectories::Var().Open(o, "scorelog.txt", std::ios::app) )
            o << tColoredString::RemoveColors(s);
    }
#ifdef DEBUG
}
#else
}
#endif

// void ePlayerNetID::SetRubber(REAL rubber2) {rubberstatus = rubber2;}

void ePlayerNetID::AddScore(int points,
                            const tOutput& reasonwin,
                            const tOutput& reasonlose)
{
    if (points==0)
        return;

    score += points;
    if (currentTeam)
        currentTeam->AddScore( points );

    tColoredString name;
    name << *this << tColoredString::ColorString(1,1,1);

    tOutput message;
    message.SetTemplateParameter(1, name);
    message.SetTemplateParameter(2, points > 0 ? points : -points);


    if (points>0)
    {
        if (reasonwin.IsEmpty())
            message << "$player_win_default";
        else
            message.Append(reasonwin);
    }
    else
    {
        if (reasonlose.IsEmpty())
            message << "$player_lose_default";
        else
            message.Append(reasonlose);
    }

    sn_ConsoleOut(message);
    RequestSync(true);

    se_SaveToScoreFile(message);
}




int ePlayerNetID::TotalScore() const
{
    if ( currentTeam )
    {
        return score;// + currentTeam->Score() * 5;
    }
    else
    {
        return score;
    }
}


void ePlayerNetID::SwapPlayersNo(int a,int b){
    if (0>a || se_PlayerNetIDs.Len()<=a)
        return;
    if (0>b || se_PlayerNetIDs.Len()<=b)
        return;
    if (a==b)
        return;

    ePlayerNetID *A=se_PlayerNetIDs(a);
    ePlayerNetID *B=se_PlayerNetIDs(b);

    se_PlayerNetIDs(b)=A;
    se_PlayerNetIDs(a)=B;
    A->listID=b;
    B->listID=a;
}


void ePlayerNetID::SortByScore(){
    // bubble sort (AAARRGGH! but good for lists that change not much)

    bool inorder=false;
    while (!inorder){
        inorder=true;
        int i;
        for(i=se_PlayerNetIDs.Len()-2;i>=0;i--)
            if (se_PlayerNetIDs(i)->TotalScore() < se_PlayerNetIDs(i+1)->TotalScore() ){
                SwapPlayersNo(i,i+1);
                inorder=false;
            }
    }
}

void ePlayerNetID::ResetScore(){
    int i;
    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        se_PlayerNetIDs(i)->score=0;
        if (sn_GetNetState()==nSERVER)
            se_PlayerNetIDs(i)->RequestSync();
    }

    for(i=eTeam::teams.Len()-1;i>=0;i--){
        eTeam::teams(i)->ResetScore();
        if (sn_GetNetState()==nSERVER)
            eTeam::teams(i)->RequestSync();
    }

    ResetScoreDifferences();
}

void ePlayerNetID::DisplayScores(){
    sr_ResetRenderState(true);

    REAL W=sr_screenWidth;
    REAL H=sr_screenHeight;

    REAL MW=400;
    REAL MH=(MW*3)/4;

    if(W>MW)
        W=MW;

    if(H>MH)
        H=MH;

#ifndef DEDICATED
    if (sr_glOut){
        ::Color(1,1,1);
        float y=.6;

        // print team ranking if there actually is a team with more than one player
        int maxPlayers = 20;
        bool showTeam = false;
        for ( int i = eTeam::teams.Len() - 1; i >= 0; --i )
        {
            if ( eTeam::teams[i]->NumPlayers() > 1 ||
                    ( eTeam::teams[i]->NumPlayers() == 1 && eTeam::teams[i]->Player(0)->Score() != eTeam::teams[i]->Score() ) )
            {
                y = eTeam::RankingGraph(y);
                y-=.06;
                maxPlayers -= ( eTeam::teams.Len() > 6 ? 6 : eTeam::teams.Len() ) + 2;
                showTeam = true;
                break;
            }
        }

        // print player ranking
        RankingGraph( y , maxPlayers );
    }
#endif
}


tString ePlayerNetID::Ranking( int MAX, bool cut ){
    SortByScore();

    tString ret;

    if (se_PlayerNetIDs.Len()>0){
        ret << tOutput("$player_scoretable_name");
        ret.SetPos(17, cut );
        ret << tOutput("$player_scoretable_alive");
        ret.SetPos(24, cut );
        ret << tOutput("$player_scoretable_score");
        ret.SetPos(31, cut );
        ret << tOutput("$player_scoretable_ping");
        ret.SetPos(37, cut );
        ret << tOutput("$player_scoretable_team");
        ret.SetPos(53, cut );
        ret << "\n";

        int max = se_PlayerNetIDs.Len();

        // wasting the last line with ... is as stupid if it stands for only
        // one player
        if ( MAX == max + 1 )
            MAX = max;

        if ( max > MAX && MAX > 0 )
        {
            max = MAX ;
        }

        for(int i=0;i<max;i++){
            tColoredString line;
            ePlayerNetID *p=se_PlayerNetIDs(i);
            // tColoredString name( p->GetColoredName() );
            // name.SetPos(16, cut );

            //	This line is example of how we manually get the player color... could come in useful.
            // line << tColoredString::ColorString( p->r/15.0, p->g/15.0, p->b/15.0 ) << name << tColoredString::ColorString(1,1,1);

            // however, using the streaming operator is a lot cleaner. The example is left, as it really can be usefull in some situations.
            line << *p;
            line.SetPos(17, false );
            if ( p->Object() && p->Object()->Alive() )
            {
                line << tOutput("$player_scoretable_alive_yes");
            }
            else
            {
                line << tOutput("$player_scoretable_alive_no");
            }
            line.SetPos(24, cut );
            line << p->score;

            if (p->IsActive())
            {
                line.SetPos(31, cut );
                //line << "ping goes here";
                line << int(p->ping*1000);
                line.SetPos(37, cut );
                if ( p->currentTeam )
                {
                    //tString teamtemp = p->currentTeam->Name();
                    //teamtemp.RemoveHex();
                    line << tColoredString::RemoveColors(p->currentTeam->Name());
                    line.SetPos(53, cut );
                }
            }
            else
                line << tOutput("$player_scoretable_inactive");
            ret << line << "\n";
        }
        if ( max < se_PlayerNetIDs.Len() )
        {
            ret << "...\n";
        }

    }
    else
        ret << tOutput("$player_scoretable_nobody");
    return ret;
}
float ePlayerNetID::RankingGraph( float y, int MAX ){
    SortByScore();

    if (se_PlayerNetIDs.Len()>0){
        tColoredString name;
        name << tColoredString::ColorString(1.,.5,.5)
        << tOutput("$player_scoretable_name");
        DisplayText(-.7, y, .06, name.c_str(), sr_fontScoretable, -1);
        tColoredString alive;
        alive << tOutput("$player_scoretable_alive");
        DisplayText(-.3, y, .06, alive.c_str(), sr_fontScoretable, -1);
        tColoredString score;
        score << tOutput("$player_scoretable_score");
        DisplayText(.05, y, .06, score.c_str(), sr_fontScoretable, 1);
        tColoredString ping;
        ping << tOutput("$player_scoretable_ping");
        DisplayText(.25, y, .06, ping.c_str(), sr_fontScoretable, 1);
        tColoredString team;
        team << tOutput("$player_scoretable_team");
        DisplayText(.3, y, .06, team.c_str(), sr_fontScoretable, -1);
        y-=.06;

        int max = se_PlayerNetIDs.Len();

        // wasting the last line with ... is as stupid if it stands for only
        // one player
        if ( MAX == max + 1 )
            MAX = max;

        if ( max > MAX && MAX > 0 )
        {
            max = MAX ;
        }

        for(int i=0;i<max;i++){
            ePlayerNetID *p=se_PlayerNetIDs(i);
            if(p->chatting_)
                DisplayText(-.705, y, .06, "*", sr_fontScoretable, 1);
            tColoredString name;
            name << *p;
            DisplayText(-.7, y, .06, name.c_str(), sr_fontScoretable, -1);
            tColoredString alive;
            if ( p->Object() && p->Object()->Alive() )
            {
                alive << tColoredString::ColorString(0,1,0)
                << tOutput("$player_scoretable_alive_yes");
            }
            else
            {
                alive << tColoredString::ColorString(1,0,0)
                << tOutput("$player_scoretable_alive_no");
            }
            DisplayText(-.3, y, .06, alive.c_str(), sr_fontScoretable, -1);
            tColoredString score;
            score << p->score;
            DisplayText(.05, y, .06, score.c_str(), sr_fontScoretable, 1);
            if (p->IsActive())
            {
                tColoredString ping;
                ping << int(p->ping*1000);
                DisplayText(.25, y, .06, ping.c_str(), sr_fontScoretable, 1);
                if ( p->currentTeam )
                {
                    tColoredString team;
                    eTeam *t = p->currentTeam;
                    team << tColoredStringProxy(t->R()/15.f, t->G()/15.f, t->B()/15.f) << t->Name();
                    DisplayText(.3, y, .06, team.c_str(), sr_fontScoretable, -1);
                }
            }
            else {
                tColoredString noone;
                noone << tOutput("$player_scoretable_inactive");
                DisplayText(.1, y, .06, noone.c_str(), sr_fontScoretable, -1);
            }
            y-=.06;
        }
        if ( max < se_PlayerNetIDs.Len() )
        {
            DisplayText(-.7, y, .06, "...", sr_fontScoretable, -1);
            y-=.06;
        }

    }
    else {
        tColoredString noone;
        noone << tOutput("$player_scoretable_nobody");
        DisplayText(-.7, y, .06, noone.c_str(), sr_fontScoretable, -1);
    }
    return y;
}

void ePlayerNetID::RankingLadderLog() {
    SortByScore();

    int num_humans = 0;
    int max = se_PlayerNetIDs.Len();
    for(int i = 0; i < max; ++i) {
        ePlayerNetID *p = se_PlayerNetIDs(i);
        if(p->Owner() == 0) continue; // ignore AIs

        tString line("ONLINE_PLAYER ");

        line << p->GetLogName();

        if(p->IsActive()) {
            line << " " << p->ping;
            if(p->currentTeam) {
                line << " " << FilterName(p->currentTeam->Name());
                ++num_humans;
            }
        }

        line << '\n';
        se_SaveToLadderLog(line);
    }
    tString line("NUM_HUMANS ");
    line << num_humans << '\n';
    se_SaveToLadderLog(line);
}

void ePlayerNetID::ClearAll(){
    for(int i=MAX_PLAYERS-1;i>=0;i--){
        ePlayer *local_p=ePlayer::PlayerConfig(i);
        if (local_p)
            local_p->netPlayer = NULL;
    }
}

static bool se_VisibleSpectatorsSupported()
{
    static nVersionFeature se_visibleSpectator(13);
    return sn_GetNetState() != nCLIENT || se_visibleSpectator.Supported(0);
}

// @param spectate if true
void ePlayerNetID::SpectateAll( bool spectate ){
    for(int i=MAX_PLAYERS-1;i>=0;i--){
        ePlayer *local_p=ePlayer::PlayerConfig(i);
        if (local_p)
        {
            if ( se_VisibleSpectatorsSupported() && bool(local_p->netPlayer) )
            {
                local_p->netPlayer->spectating_ = spectate || local_p->spectate;

                local_p->netPlayer->RequestSync();
            }
            else
                local_p->netPlayer = NULL;
        }
    }
}

void ePlayerNetID::CompleteRebuild(){
    ClearAll();
    Update();
}

static int se_ColorDistance( int a[3], int b[3] )
{
    int distance = 0;
    for( int i = 2; i >= 0; --i )
    {
        int diff = a[i] - b[i];
        distance += diff * diff;
        //diff = diff < 0 ? -diff : diff;
        //if ( diff > distance )
        //{
        //    distance = diff;
        //}
    }

    return distance;
}

bool se_randomizeColor = true;
static tSettingItem< bool > se_randomizeColorConf( "PLAYER_RANDOM_COLOR", se_randomizeColor );

static void se_RandomizeColor( ePlayer * l, ePlayerNetID * p )
{
    int currentRGB[3];
    int newRGB[3];
    int nullRGB[3]={0,0,0};

    static tReproducibleRandomizer randomizer;

    for( int i = 2; i >= 0; --i )
    {
        currentRGB[i] = l->rgb[i];
        newRGB[i] = randomizer.Get(15);
    }

    int currentMinDiff = se_ColorDistance( currentRGB, nullRGB )/2;
    int newMinDiff = se_ColorDistance( newRGB, nullRGB )/2;

    // check the minimal distance of the new random color with all players
    for ( int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID * other = se_PlayerNetIDs(i);
        if ( other != p )
        {
            int color[3] = { other->r, other->g, other->b };
            int currentDiff = se_ColorDistance( currentRGB, color );
            int newDiff     = se_ColorDistance( newRGB, color );
            if ( currentDiff < currentMinDiff )
            {
                currentMinDiff = currentDiff;
            }
            if ( newDiff < newMinDiff )
            {
                newMinDiff = newDiff;
            }
        }
    }

    // update current color
    if ( currentMinDiff < newMinDiff )
    {
        for( int i = 2; i >= 0; --i )
        {
            l->rgb[i] = newRGB[i];
        }
    }
}

static nSettingItem<int> se_pingCharityServerConf("PING_CHARITY_SERVER",sn_pingCharityServer );
static nVersionFeature   se_pingCharityServerControlled( 14 );

static int se_pingCharityMax = 500, se_pingCharityMin = 0;
static tSettingItem<int> se_pingCharityMaxConf( "PING_CHARITY_MAX", se_pingCharityMax );
static tSettingItem<int> se_pingCharityMinConf( "PING_CHARITY_MIN", se_pingCharityMin );

// Update the netPlayer_id list
void ePlayerNetID::Update(){
#ifdef KRAWALL_SERVER
        // update access level
        UpdateAccessLevelRequiredToPlay();

        // remove players with insufficient access rights
        tAccessLevel required = AccessLevelRequiredToPlay();
        for( int i=se_PlayerNetIDs.Len()-1; i >= 0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);
            if ( player->GetAccessLevel() > required && player->IsHuman() )
            {
                player->SetTeamWish(0);
            }
        }
#endif

#ifdef DEDICATED
    if (sr_glOut)
#endif
    {
        for(int i=0; i<MAX_PLAYERS; ++i ){
            bool in_game=ePlayer::PlayerIsInGame(i);
            ePlayer *local_p=ePlayer::PlayerConfig(i);
            tASSERT(local_p);
            tCONTROLLED_PTR(ePlayerNetID) &p=local_p->netPlayer;

            if (!p && in_game && ( !local_p->spectate || se_VisibleSpectatorsSupported() ) ) // insert new player
            {
                p=tNEW(ePlayerNetID) (i);
                p->SetDefaultTeam();
                p->RequestSync();
            }

            if (bool(p) && (!in_game || ( local_p->spectate && !se_VisibleSpectatorsSupported() ) ) && // remove player
                    p->Owner() == ::sn_myNetID )
            {
                p->RemoveFromGame();

                if (p->object)
                    p->object->player = NULL;

                p->object = NULL;
                p = NULL;
            }

            if (bool(p) && in_game){ // update
                p->favoriteNumberOfPlayersPerTeam=ePlayer::PlayerConfig(i)->favoriteNumberOfPlayersPerTeam;
                p->nameTeamAfterMe=ePlayer::PlayerConfig(i)->nameTeamAfterMe;

                if ( se_randomizeColor )
                {
                    se_RandomizeColor(local_p,p);
                }

                p->r=ePlayer::PlayerConfig(i)->rgb[0];
                p->g=ePlayer::PlayerConfig(i)->rgb[1];
                p->b=ePlayer::PlayerConfig(i)->rgb[2];

                sg_ClampPingCharity();
                p->pingCharity=::pingCharity;
                p->SetTeamname(local_p->Teamname());

                // update spectator status
                if ( p->spectating_ != local_p->spectate )
                    p->RequestSync();
                p->spectating_ = local_p->spectate;

                // update stealth status
                if ( p->stealth_ != local_p->stealth )
                    p->RequestSync();
                p->stealth_ = local_p->stealth;

                // update name
                tString newName( ePlayer::PlayerConfig(i)->Name() );

                if ( ::sn_GetNetState() != nCLIENT || newName != p->nameFromClient_ )
                {
                    p->RequestSync();
                }

                p->SetName( ePlayer::PlayerConfig(i)->Name() );
            }
        }

    }

    int i;


    // update the ping charity, but
    // don't do so on the client if the server controls the ping charity completely
    if ( sn_GetNetState() == nSERVER || !se_pingCharityServerControlled.Supported(0) )
    {
        int old_c=sn_pingCharityServer;
        sg_ClampPingCharity();
        sn_pingCharityServer=::pingCharity;

#ifndef DEDICATED
        if (sn_GetNetState()==nCLIENT)
#endif
            sn_pingCharityServer+=100000;

        // set configurable maximum
        if ( se_pingCharityServerControlled.Supported() )
        {
            sn_pingCharityServer = se_pingCharityMax;
        }

        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
            ePlayerNetID *pni=se_PlayerNetIDs(i);
            pni->UpdateName();
            int new_ps=pni->pingCharity;
            new_ps+=int(pni->ping*500);

            // only take ping charity into account for non-spectators
            if ( sn_GetNetState() != nSERVER || pni->currentTeam || pni->nextTeam )
                if (new_ps < sn_pingCharityServer)
                    sn_pingCharityServer=new_ps;
        }
        if (sn_pingCharityServer<0)
            sn_pingCharityServer=0;

        // set configurable minimum
        if ( se_pingCharityServerControlled.Supported() )
        {
            if ( sn_pingCharityServer < se_pingCharityMin )
                sn_pingCharityServer = se_pingCharityMin;
        }

        if (old_c!=sn_pingCharityServer)
        {
            tOutput o;
            o.SetTemplateParameter(1, old_c);
            o.SetTemplateParameter(2, sn_pingCharityServer);
            o << "$player_pingcharity_changed";
            con << o;

            // trigger transmission to the clients
            if (sn_GetNetState()==nSERVER)
                se_pingCharityServerConf.nConfItemBase::WasChanged(true);
        }
    }

    // update team assignment
    bool assigned = true;
    while ( assigned && sn_GetNetState() != nCLIENT )
    {
        assigned = false;

        int players = se_PlayerNetIDs.Len();

        // start with the players that came in earlier
        for( i=0; i<players; ++i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);

            // only assign new team if it is possible
            if ( player->NextTeam() != player->CurrentTeam() && player->TeamChangeAllowed() && 
                    ( !player->NextTeam() || player->NextTeam()->PlayerMayJoin( player ) )
               )
            {
                player->UpdateTeam();
                if ( player->NextTeam() == player->CurrentTeam() )
                    assigned = true;
            }
        }

        // assign players that are not currently on a team, but want to, to any team. Start with the late comers here.
        if ( !assigned )
        {
            for( i=players-1; i>=0; --i )
            {
                ePlayerNetID* player = se_PlayerNetIDs(i);

                // if the player is not currently on a team, but wants to join a specific team, let it join any, but keep the wish stored
                if ( player->NextTeam() && !player->CurrentTeam() && player->TeamChangeAllowed() )
                {
                    eTeam * wish = player->NextTeam();
                    bool assignBack = se_assignTeamAutomatically;
                    se_assignTeamAutomatically = true;
                    player->FindDefaultTeam();
                    se_assignTeamAutomatically = assignBack;
                    player->SetTeamForce( wish );

                    if ( player->CurrentTeam() )
                    {
                        assigned = true;
                        break;
                    }
                }
            }
        }
    }

    if ( sn_GetNetState() != nCLIENT )
    {
        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);

            // announce unfullfilled wishes
            if ( player->NextTeam() != player->CurrentTeam() && player->NextTeam() )
            {
                //if the player can't change teams delete the team wish
                if(!player->TeamChangeAllowed()) {
                    player->SetTeam( player->CurrentTeam() );
                    continue;
                }

                tOutput message( "$player_joins_team_wish",
                                 player->GetName(),
                                 player->NextTeam()->Name() );

                sn_ConsoleOut( message );

                // if team change is futile because team play is disabled,
                // delete the team change wish
                if ( eTeam::maxPlayers <= 1 )
                    player->SetTeam( player->CurrentTeam() );
            }
        }
    }

    // update the teams as well
    for (i=eTeam::teams.Len()-1; i>=0; --i)
    {
        eTeam::teams(i)->UpdateProperties();
    }

    // get rid of deleted netobjects
    nNetObject::ClearAllDeleted();
}

// wait at most this long for any player to leave chat state...
static REAL se_playerWaitMax      = 10.0f;
static tSettingItem< REAL > se_playerWaitMaxConf( "PLAYER_CHAT_WAIT_MAX", se_playerWaitMax );

// and no more than this much measured relative to his non-chatting time.
static REAL se_playerWaitFraction = .05f;
static tSettingItem< REAL > se_playerWaitFractionConf( "PLAYER_CHAT_WAIT_FRACTION", se_playerWaitFraction );

// flag to only account one player for the chat break
static bool se_playerWaitSingle = false;
static tSettingItem< bool > se_playerWaitSingleConf( "PLAYER_CHAT_WAIT_SINGLE", se_playerWaitSingle );

// flag to only let the team leader pause the timer
static bool se_playerWaitTeamleader = true;
static tSettingItem< bool > se_playerWaitTeamleaderConf( "PLAYER_CHAT_WAIT_TEAMLEADER", se_playerWaitTeamleader );

// wait for players to leave chat state
bool ePlayerNetID::WaitToLeaveChat()
{
    static bool lastReturn = false;
    static double lastTime = 0;
    static ePlayerNetID * lastPlayer = 0; // the last player that caused a pause. Use only for comparison, the pointer may be bad. Don't dereference!
    double time = tSysTimeFloat();
    REAL dt = time - lastTime;
    bool ret = false;

    if ( !lastReturn )
    {
        // account for non-break pause: give players additional pause time
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);
            if ( dt > 1.0 || !player->chatting_ )
            {
                player->wait_ += se_playerWaitFraction * dt;
                if ( player->wait_ > se_playerWaitMax )
                {
                    player->wait_ = se_playerWaitMax;
                }
            }
        }

        if ( dt > 1.0 )
            lastPlayer = 0;

        dt = 0;
    }

    // the chatting player with the most wait seconds left
    ePlayerNetID * maxPlayer = 0;
    REAL maxWait = .2;

    // iterate over chatting players
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* player = se_PlayerNetIDs(i);
        if ( player->CurrentTeam() && player->chatting_ && ( !se_playerWaitTeamleader || player->CurrentTeam()->OldestHumanPlayer() == player ) )
        {
            // account for waiting time if everyone is to get his time reduced
            if ( !se_playerWaitSingle )
            {
                player->wait_ -= dt;

                // hold back
                if ( player->wait_ > 0 )
                {
                    ret = true;
                }
                else
                {
                    player->wait_ = 0;
                }
            }

            // determine player we'll wait for longest
            if (  ( maxPlayer != lastPlayer || NULL == maxPlayer ) && ( player->wait_ > maxWait || player == lastPlayer ) && player->wait_ > 0 )
            {
                maxWait = player->wait_;
                maxPlayer = player;
            }
        }
    }

    // account for waiting time if only one player should get his waiting time reduced
    if ( se_playerWaitSingle && maxPlayer )
    {
        maxPlayer->wait_ -= dt;

        // hold back
        if ( maxPlayer->wait_ < 0 )
        {
            maxPlayer->wait_ = 0;
        }
    }

    static double lastPrint = -2;

    // print information: who are we waiting for?
    if ( maxPlayer && maxPlayer != lastPlayer && tSysTimeFloat() - lastPrint > 1 )
    {
        sn_ConsoleOut( tOutput( "$gamestate_chat_wait", maxPlayer->GetName(), int(10*maxPlayer->wait_)*.1f ) );
        lastPlayer = maxPlayer;
    }

    if ( lastPlayer == maxPlayer )
    {
        lastPrint = tSysTimeFloat();
    }

    // store values for future reference
    lastReturn = ret;
    lastTime = time;

    return ret;
}

// time in chat mode before a player is no longer spawned
static REAL se_chatterRemoveTime = 180.0;
static tSettingItem< REAL > se_chatterRemoveTimeConf( "CHATTER_REMOVE_TIME", se_chatterRemoveTime );

// time without keypresses before a player is no longer spawned
static REAL se_idleRemoveTime = 0;
static tSettingItem< REAL > se_idleRemoveTimeConf( "IDLE_REMOVE_TIME", se_idleRemoveTime );

// time without keypresses before a player is kicked
static REAL se_idleKickTime = 0;
static tSettingItem< REAL > se_idleKickTimeConf( "IDLE_KICK_TIME", se_idleKickTime );

//removes chatbots and idling players from the game
void ePlayerNetID::RemoveChatbots()
{
    // nothing to be done on the clients
    if ( nCLIENT == sn_GetNetState() )
        return;

#ifdef KRAWALL_SERVER
    // update access level
    UpdateAccessLevelRequiredToPlay();
#endif

    // determine the length of the last round
    static double lastTime = 0;
    double currentTime = tSysTimeFloat();
    REAL roundTime = currentTime - lastTime;
    lastTime = currentTime;

    // go through all players that don't have a team assigned currently, and assign one
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID *p = se_PlayerNetIDs(i);
        if ( p && p->IsHuman() )
        {
            // time allowed to be idle
            REAL idleTime = p->IsChatting() ? se_chatterRemoveTime : se_idleRemoveTime;

            // determine whether the player should have a team
            bool shouldHaveTeam = idleTime <= 0 || p->LastActivity() - roundTime < idleTime;
            shouldHaveTeam &= !p->IsSpectating();

            tColoredString name;
            name << *p << tColoredString::ColorString(1,.5,.5);

            // see to it that the player has or has not a team.
            if ( shouldHaveTeam )
            {
                if ( !p->CurrentTeam() )
                {
                    p->SetDefaultTeam();
                }
            }
            else
            {
                if ( p->CurrentTeam() )
                {
                    p->SetTeam( NULL );
                    p->UpdateTeam();
                }
            }

            // kick idle players (Removes player from list, this must be the last operation of the loop)
            if ( se_idleKickTime > 0 && se_idleKickTime < p->LastActivity() - roundTime )
            {
                sn_KickUser( p->Owner(), tOutput( "$network_kill_idle" ) );

                // if many players get kicked with one client, the array may have changed
                if ( i >= se_PlayerNetIDs.Len() )
                    i = se_PlayerNetIDs.Len()-1;
            }
        }
    }
}

void ePlayerNetID::ThrowOutDisconnected()
{
    int i;
    // find all disconnected players

    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        ePlayerNetID *pni=se_PlayerNetIDs(i);
        if (pni->disconnected)
        {
            // remove it from the list of players (so it won't be deleted twice...)
            se_PlayerNetIDs.Remove(pni, pni->listID);
        }
    }

    se_PlayerReferences.ReleaseAll();
}

void ePlayerNetID::GetScoreFromDisconnectedCopy()
{
    int i;
    // find a copy
    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        ePlayerNetID *pni=se_PlayerNetIDs(i);
        if (pni->disconnected && pni->GetUserName() == GetUserName() && pni->Owner() == 0)
        {
#ifdef DEBUG
            con << GetName() << " reconnected.\n";
#endif

            pni->RequestSync();
            RequestSync();

            score = pni->score;

            ControlObject(pni->Object());
            //	 object->ePlayer = this;
            pni->object = NULL;

            if (bool(object))
                chatting_ = true;

            pni->disconnected = false;
            se_PlayerNetIDs.Remove(pni, pni->listID);
            se_PlayerReferences.Remove( pni );        // really delete it without a message
        }
    }
}


static bool show_scores=false;
static bool ass=true;

void se_AutoShowScores(){
    if (ass)
        show_scores=true;
}


void se_UserShowScores(bool show){
    show_scores=show;
}

void se_SetShowScoresAuto(bool a){
    ass=a;
}


static void scores(){
    if (show_scores){
        if ( se_mainGameTimer )
            ePlayerNetID::DisplayScores();
        else
            show_scores = false;
    }
}


static rPerFrameTask pf(&scores);

static bool force_small_cons(){
    return show_scores;
}

static rSmallConsoleCallback sc(&force_small_cons);

static void cd(){
    show_scores = false;
}



static uActionGlobal score("SCORE");


static bool sf(REAL x){
    if (x>0) show_scores = !show_scores;
    return true;
}

static uActionGlobalFunc saf(&score,&sf);


static rCenterDisplayCallback c_d(&cd);

tOutput& operator << (tOutput& o, const ePlayerNetID& p)
{
    tColoredString x;
    x << p;
    o << x;
    return o;
}



tCallbackString *eCallbackGreeting::anchor = NULL;
ePlayerNetID* eCallbackGreeting::greeted = NULL;

tString eCallbackGreeting::Greet(ePlayerNetID* player)
{
    greeted = player;
    return Exec(anchor);
}

eCallbackGreeting::eCallbackGreeting(STRINGRETFUNC* f)
        :tCallbackString(anchor, f)
{
}

void ePlayerNetID::GreetHighscores(tString &s){
    s << eCallbackGreeting::Greet(this);
    //  tOutput o;
    //  gHighscoresBase::Greet(this,o);
    //  s << o;
}


// *******************
// *      chatting_   *
// *******************
void ePlayerNetID::SetChatting ( ChatFlags flag, bool chatting )
{
    if ( sn_GetNetState() == nSTANDALONE && flag == ChatFlags_Menu )
    {
        chatting = false;
    }

    if ( chatting )
    {
        chatFlags_ |= flag;
        if ( !chatting_ )
            this->RequestSync();

        chatting_ = true;
    }
    else
    {
        chatFlags_ &= ~flag;
        if ( 0 == chatFlags_ )
        {
            if ( chatting_ )
                this->RequestSync();

            chatting_ = false;
        }
    }
}

// *******************
// * team management *
// *******************

bool ePlayerNetID::TeamChangeAllowed() const {
    return ( allowTeamChange_ || se_allowTeamChanges )
#ifdef KRAWALL_SERVER
       // only allow players with enough access level to enter the game, everyone is free to leave, though
       && ( GetAccessLevel() <= AccessLevelRequiredToPlay() || CurrentTeam() )
#endif
    ;
}

// put a new player into a default team
void ePlayerNetID::SetDefaultTeam( )
{
    // only the server should do this, the client does not have the full information on how to do do it right.
    if ( sn_GetNetState() == nCLIENT || !se_assignTeamAutomatically || spectating_ || !TeamChangeAllowed() )
        return;

    //    if ( !IsHuman() )
    //    {
    //        SetTeam( NULL );
    //        return;
    //    }

    eTeam* defaultTeam = FindDefaultTeam();
    if (defaultTeam)
        SetTeamWish(defaultTeam);
    else if ( eTeam::NewTeamAllowed() )
        CreateNewTeamWish();
    // yes, if all teams are full and no team creation is allowed, the player stays without team and will not be spawned.
    //TODO: Might add AI to null team here.
}

// put a new player into a default team
eTeam* ePlayerNetID::FindDefaultTeam( )
{
    // find the team with the least number of players on it
    eTeam *min = NULL;
    for ( int i=eTeam::teams.Len()-1; i>=0; --i )
    {
        eTeam *t = eTeam::teams( i );
        if ( t->IsHuman() && ( !min || min->NumHumanPlayers() > t->NumHumanPlayers() ) )
            min = t;
    }

    if ( min &&
            // create new teams until minTeams is matched
            eTeam::teams.Len() >= eTeam::minTeams &&
            min->PlayerMayJoin( this ) &&
            // no new team allowed or not more than favoriteNumberOfPlayers on team
            ( !eTeam::NewTeamAllowed() || ( min->NumHumanPlayers() > 0 && min->NumHumanPlayers() < favoriteNumberOfPlayersPerTeam ) )
       )
    {
        // return the team
        return min;
    }
    // return NULL to indicate no team was found => "create a new team" (?)
    return NULL;
}

// register me in the given team (callable on the server)
void ePlayerNetID::SetTeam( eTeam* newTeam )
{
    // check if the team change is legal
    tASSERT ( !newTeam || nCLIENT !=  sn_GetNetState() );

    SetTeamForce( newTeam );

    if (newTeam && ( !newTeam->PlayerMayJoin( this ) || IsSpectating() ) )
    {
        tOutput message;
        message.SetTemplateParameter( 1, GetName() );
        if ( newTeam )
            message.SetTemplateParameter(2, newTeam->Name() );
        else
            message.SetTemplateParameter(2, "NULL");
        message << "$player_nojoin_team";

        sn_ConsoleOut( message, Owner() );
        // return;
    }
}

// set teamname to be used for my own team
void ePlayerNetID::SetTeamname(const char* newTeamname)
{
    teamname = newTeamname;
    if (bool(currentTeam) && currentTeam->OldestHumanPlayer() &&
            currentTeam->OldestHumanPlayer()->ID()==ID())
    {
        currentTeam->UpdateAppearance();
    }
}

// register me in the given team (callable on the server)
void ePlayerNetID::SetTeamForce( eTeam* newTeam )
{
    // check if the team change is legal
    //tASSERT ( !newTeam || nCLIENT !=  sn_GetNetState() );
#ifdef DEBUG
    std::cout << "DEBUG: Player:" << this->GetName() << " SetTeamForce:" << ((newTeam)?(const char*)newTeam->Name():"NULL") << "\n";
#endif
    nextTeam = newTeam;
}

// register me in the given team (callable on the server)
void ePlayerNetID::UpdateTeam()
{
    // check if work is needed
    if ( nextTeam == currentTeam )
    {
        return;
    }

    // check if the team change is legal
    if ( nCLIENT ==  sn_GetNetState() )
    {
        return;
    }

    if ( bool( nextTeam ) && !nextTeam->PlayerMayJoin( this ) )
    {
        tOutput message;
        message.SetTemplateParameter(1, GetName() );
        if ( nextTeam )
            message.SetTemplateParameter(2, nextTeam->Name() );
        else
            message.SetTemplateParameter(2, "NULL");
        message << "$player_nojoin_team";

        sn_ConsoleOut( message, Owner() );
        return;
    }

    UpdateTeamForce();
}

void ePlayerNetID::UpdateTeamForce()
{
    // check if work is needed
    if ( nextTeam == currentTeam )
    {
        return;
    }

    eTeam *oldTeam = currentTeam;

    if ( nextTeam )
        nextTeam->AddPlayer ( this );
    else if ( oldTeam )
        oldTeam->RemovePlayer( this );

    if( nCLIENT !=  sn_GetNetState() && GetRefcount() > 0 )
    {
        RequestSync();
    }
}

// teams this player is invited to
ePlayerNetID::eTeamSet const & ePlayerNetID::GetInvitations() const
{
    return invitations_;
}

// create a new team and join it (on the server)
void ePlayerNetID::CreateNewTeam()
{
    // check if the team change is legal
    tASSERT ( nCLIENT !=  sn_GetNetState() );

    if(!TeamChangeAllowed()) {
        sn_ConsoleOut(tOutput("$player_teamchanges_disallowed"), Owner());
        return;
    }

    if ( !eTeam::NewTeamAllowed() ||
            ( bool( currentTeam ) && ( currentTeam->NumHumanPlayers() == 1 ) ) ||
            IsSpectating() )
    {
        tOutput message;
        message.SetTemplateParameter(1, GetName() );
        message << "$player_nocreate_team";

        sn_ConsoleOut( message, Owner() );

        // yarrt: Should CreateNewTeam only try to create a team ?
        // yarrt: yes, let's try
        // if it can't create a team => just idle?
        //Old code
        //if ( !currentTeam )
        //    SetDefaultTeam();

        return;
    }

    // create the new team and join it
    tJUST_CONTROLLED_PTR< eTeam > newTeam = tNEW( eTeam );
    nextTeam = newTeam;
    nextTeam->AddScore( score );

    // directly if possible
    if ( !currentTeam )
    {
        UpdateTeam();
    }
}

const unsigned short TEAMCHANGE = 0;
const unsigned short NEW_TEAM   = 1;


// express the wish to be part of the given team (always callable)
void ePlayerNetID::SetTeamWish(eTeam* newTeam)
{
    if (NextTeam()==newTeam)
        return;

    if ( nSERVER ==  sn_GetNetState() && newTeam==NULL && CurrentTeam()!=NULL && NextTeam()!=NULL)
    {
        if ( se_assignTeamAutomatically )
        {
            sn_ConsoleOut("$no_spectators_allowed", Owner() );
            return;
        }

        eTeam * leftTeam = NextTeam();
        if ( leftTeam )
        {
            if ( !leftTeam )
                leftTeam = CurrentTeam();

            if ( leftTeam->NumPlayers() > 1 )
            {
                sn_ConsoleOut( tOutput( "$player_leave_team_wish",
                                        tColoredString::RemoveColors(GetName()),
                                        tColoredString::RemoveColors(leftTeam->Name()) ) );
            }
            else
            {
                sn_ConsoleOut( tOutput( "$player_leave_game_wish",
                                        tColoredString::RemoveColors(GetName()) ) );
            }
        }
    }
    if ( nCLIENT ==  sn_GetNetState() && Owner() == sn_myNetID )
    {
        nMessage* m = NewControlMessage();

        (*m) << TEAMCHANGE;
        (*m) << newTeam;

        m->BroadCast();
        // update local client data, should be ok (dangerous?, why?)
        SetTeamForce( newTeam );
    }
    else
    {
        SetTeam( newTeam );

        // directly join if possible to keep counts up to date
        if (!currentTeam)
            UpdateTeam();
    }
}

// express the wish to create a new team and join it
void ePlayerNetID::CreateNewTeamWish()
{
    if ( nCLIENT ==  sn_GetNetState() )
    {
        nMessage* m = NewControlMessage();

        (*m) << NEW_TEAM;

        m->BroadCast();
    }
    else
        // create a new team if possible otherwise spectate
        CreateNewTeam();
}

// receive the team control wish
void ePlayerNetID::ReceiveControlNet(nMessage &m)
{
    short messageType;
    m >> messageType;

    switch (messageType)
    {
    case NEW_TEAM:
        {
            // create a new team if possible otherwise spectate or...
            CreateNewTeam();
            // if auto_team is enabled join the default team
            if (se_assignTeamAutomatically && NextTeam()==NULL)
                SetDefaultTeam();

            break;
        }
    case TEAMCHANGE:
        {
            eTeam *newTeam;

            m >> newTeam;

            if(!TeamChangeAllowed()) {
                sn_ConsoleOut( tOutput( "$player_teamchanges_disallowed" ), Owner() );
                break;
            }

            // annihilate team if it no longer is in the game
            if ( bool(newTeam) && newTeam->TeamID() < 0 )
                newTeam = 0;

            // NULL team probably means that the change target does not
            // exist any more. Create a new team instead.
            if ( !newTeam )
            {
                if ( currentTeam )
                    sn_ConsoleOut( tOutput( "$player_joins_team_noex" ), Owner() );
                break;
            }

            // check if the resulting message is obnoxious
            bool redundant = ( nextTeam == newTeam );
            bool obnoxious = ( nextTeam != currentTeam || redundant );

            SetTeam( newTeam );

            // announce the change
            if ( bool(nextTeam) && !redundant )
            {
                tOutput message;
                message.SetTemplateParameter(1, tColoredString::RemoveColors(GetName()));
                message.SetTemplateParameter(2, tColoredString::RemoveColors(nextTeam->Name()) );
                message << "$player_joins_team";

                sn_ConsoleOut( message );

                // count it as spam if it is obnoxious
                if ( obnoxious )
                    chatSpam_.CheckSpam( 4.0, Owner(), tOutput("$spam_teamchage") );
            }

            break;
        }
    default:
        {
            tASSERT(0);
        }
    }
}

void ePlayerNetID::Color( REAL&a_r, REAL&a_g, REAL&a_b ) const
{
    if ( ( static_cast<bool>(currentTeam) ) && ( currentTeam->IsHuman() ) )
    {
        REAL w = 5;
        REAL r_w = 2;
        REAL g_w = 1;
        REAL b_w = 2;

        int r = this->r;
        int g = this->g;
        int b = this->b;

        // don't tolerate color overflow in a real team
        if ( currentTeam->NumPlayers() > 1 )
        {
            if ( r > 15 )
                r = 15;
            if ( g > 15 )
                g = 15;
            if ( b > 15 )
                b = 15;
        }

        a_r=(r_w*r + w*currentTeam->R())/( 15.0 * ( w + r_w ) );
        a_g=(g_w*g + w*currentTeam->G())/( 15.0 * ( w + g_w ) );
        a_b=(b_w*b + w*currentTeam->B())/( 15.0 * ( w + b_w ) );
    }
    else
    {
        a_r = r/15.0;
        a_g = g/15.0;
        a_b = b/15.0;
    }
}

void ePlayerNetID::TrailColor( REAL&a_r, REAL&a_g, REAL&a_b ) const
{
    Color( a_r, a_g, a_b );

    /*
    if ( ( static_cast<bool>(currentTeam) ) && ( currentTeam->IsHuman() ) )
    {
    	int w = 6;
    	a_r=(2*r + w*currentTeam->R())/( 15.0 * ( w + 2 ) );
    	a_g=(2*g + w*currentTeam->G())/( 15.0 * ( w + 2 ) );
    	a_b=(2*b + w*currentTeam->B())/( 15.0 * ( w + 2 ) );
    }
    else
    {
    	a_r = r/15.0;
    	a_g = g/15.0;
    	a_b = b/15.0;
    }
    */
}

/*
void ePlayerNetID::AddRef()
{
	nNetObject::AddRef();
}

void ePlayerNetID::Release()
{
	nNetObject::Release();
}
*/

// reads a network ID from the stream, either the number or my user name
static unsigned short se_ReadUser( std::istream &s, ePlayerNetID * requester = 0 )
{
    // read name of player to be kicked
    tString name;
    s >> name;

    // try to convert it into a number and reference it as that
    int num;
    if ( name.Convert(num) && num >= 1 && num <= MAXCLIENTS && sn_Connections[num].socket )
    {
        return num;
    }
    else
    {
        // standard name lookup
        ePlayerNetID * p = se_FindPlayerByName( name, requester );
        if ( p )
        {
            return p->Owner();
        }
    }

    return 0;
}


// call on commands that only work on the server; quit if it returns true
static bool se_NeedsServer(char const * command, std::istream & s, bool strict = true )
{
    if ( sn_GetNetState() != nSERVER && ( strict || sn_GetNetState() != nSTANDALONE ) )
    {
        tString rest;
        rest.ReadLine( s );
        con << tOutput("$only_works_on_server", command, rest );
        return true;
    }

    return false;
}

static void se_PlayerMessageConf(std::istream &s)
{
    if ( se_NeedsServer( "PLAYER_MESSAGE", s ) )
    {
        return;
    }

    int receiver = se_ReadUser( s );

    tColoredString msg;
    msg.ReadLine(s);

    if ( receiver <= 0 || s.good() )
    {
        con << "Usage: PLAYER_MESSAGE <user ID or name> <Message>\n";
        return;
    }

    msg << '\n';

    sn_ConsoleOut(msg, 0);
    sn_ConsoleOut(msg, receiver);
}

static tConfItemFunc se_PlayerMessage_c("PLAYER_MESSAGE", &se_PlayerMessageConf);
static tAccessLevelSetter se_messConfLevel( se_PlayerMessage_c, tAccessLevel_Moderator );

static tString se_defaultKickReason("");
static tConfItemLine se_defaultKickReasonConf( "DEFAULT_KICK_REASON", se_defaultKickReason );

static void se_KickConf(std::istream &s)
{
    if ( se_NeedsServer( "KICK", s ) )
    {
        return;
    }

    // get user ID
    int num = se_ReadUser( s );

    tString reason = se_defaultKickReason;
    if ( !s.eof() )
        reason.ReadLine(s);

    // and kick.
    if ( num > 0 && !s.good() )
    {
        sn_KickUser( num ,  reason.Len() > 1 ? static_cast< char const *>( reason ) : "$network_kill_kick" );
    }
    else
    {
        con << "Usage: KICK <user ID or name> <Reason>\n";
        return;
    }
}

static tConfItemFunc se_kickConf("KICK",&se_KickConf);
static tAccessLevelSetter se_kickConfLevel( se_kickConf, tAccessLevel_Moderator );

static tString se_defaultKickToServer("");
static int se_defaultKickToPort = 4534;
static tString se_defaultKickToReason("");

static tSettingItem< tString > se_defaultKickToServerConf( "DEFAULT_KICK_TO_SERVER", se_defaultKickToServer );
static tSettingItem< int > se_defaultKickToPortConf( "DEFAULT_KICK_TO_PORT", se_defaultKickToPort );
static tConfItemLine se_defaultKickToReasonConf( "DEFAULT_KICK_TO_REASON", se_defaultKickToReason );

static void se_MoveToConf(std::istream &s, REAL severity )
{
    if ( se_NeedsServer( "KICK/MOVE_TO", s ) )
    {
        return;
    }

    // get user ID
    int num = se_ReadUser( s );

    // read redirection target
    tString server = se_defaultKickToServer;
    if ( !s.eof() )
    {
        s >> server;
    }

    int port = se_defaultKickToPort;
    if ( !s.eof() )
        s >> port;

    nServerInfoRedirect redirect( server, port );

    tString reason = se_defaultKickToReason;
    if ( !s.eof() )
        reason.ReadLine(s);

    // and kick.
    if ( num > 0 && !s.good() )
    {
        sn_KickUser( num ,  reason.Len() > 1 ? static_cast< char const *>( reason ) : "$network_kill_kick", severity, &redirect );
    }
    else
    {
        con << "Usage: KICK_TO <user ID or name> <server IP to kick to>:<server port to kick to> <Reason>\n";
        return;
    }
}

static void se_KickToConf(std::istream &s )
{
    se_MoveToConf( s, 1 );
}

static tConfItemFunc se_kickToConf("KICK_TO",&se_KickToConf);
static tAccessLevelSetter se_kickToConfLevel( se_kickToConf, tAccessLevel_Moderator );

static void se_MoveToConf(std::istream &s )
{
    se_MoveToConf( s, 0 );
}

static tConfItemFunc se_moveToConf("MOVE_TO",&se_MoveToConf);
static tAccessLevelSetter se_moveConfLevel( se_moveToConf, tAccessLevel_Moderator );

static void se_BanConf(std::istream &s)
{
    if ( se_NeedsServer( "BAN", s ) )
    {
        return;
    }

    // get user ID
    int num = se_ReadUser( s );

    if ( num == 0 && !s.good() )
    {
        con << "Usage: BAN <user ID or name> <time in minutes(defaults to 60)> <Reason>\n";
        return;
    }

    // read time to ban
    REAL banTime = 60;
    s >> banTime;
    std::ws(s);

    tString reason;
    reason.ReadLine(s);

    // and ban.
    if ( num > 0 )
    {
        nMachine::GetMachine( num ).Ban( banTime * 60, reason );
        sn_DisconnectUser( num , reason.Len() > 1 ? static_cast< char const *>( reason ) : "$network_kill_kick" );
    }
}

static tConfItemFunc se_banConf("BAN",&se_BanConf);
static tAccessLevelSetter se_banConfLevel( se_banConf, tAccessLevel_Moderator );

static ePlayerNetID * ReadPlayer( std::istream & s )
{
    // read name of player to be returned
    tString name;
    s >> name;

    int num = name.ToInt();
    if ( num > 0 )
    {
        // look for a player from that client
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);

            // check whether it's p who should be returned
            if ( p->Owner() == num )
            {
                return p;
            }
        }
    }

    return se_FindPlayerByName( name );
}

static void Kill_conf(std::istream &s)
{
    if ( se_NeedsServer( "KILL", s, false ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );
    if ( p && p->Object() )
        p->Object()->Kill();
}

static tConfItemFunc kill_conf("KILL",&Kill_conf);
static tAccessLevelSetter se_killConfLevel( kill_conf, tAccessLevel_Moderator );

static void Silence_conf(std::istream &s)
{
    if ( se_NeedsServer( "SILENCE", s ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );
    if ( p )
    {
        sn_ConsoleOut( tOutput( "$player_silenced", p->GetName() ) );
        p->SetSilenced( true );
    }
}

static tConfItemFunc silence_conf("SILENCE",&Silence_conf);
static tAccessLevelSetter se_silenceConfLevel( silence_conf, tAccessLevel_Moderator );

static void Voice_conf(std::istream &s)
{
    if ( se_NeedsServer( "VOICE", s ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );
    if ( p )
    {
        sn_ConsoleOut( tOutput( "$player_voiced", p->GetName() ) );
        p->SetSilenced( false );
    }
}

static tConfItemFunc voice_conf("VOICE",&Voice_conf);
static tAccessLevelSetter se_voiceConfLevel( voice_conf, tAccessLevel_Moderator );

static tString sg_url;
static tSettingItem< tString > sg_urlConf( "URL", sg_url );

static tString sg_options("Nothing special.");
#ifdef DEDICATED
static tConfItemLine sg_optionsConf( "SERVER_OPTIONS", sg_options );
#endif

class gServerInfoAdmin: public nServerInfoAdmin
{
public:
    gServerInfoAdmin(){};

private:
    virtual tString GetUsers()		const
    {
        tString ret;

        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
            if ( p->IsHuman() )
            {
                ret << p->GetName() << "\n";
            }
        }

        return ret;
    }

    virtual tString	GetOptions()	const
    {
        se_CutString( sg_options, 240 );
        return sg_options;
    }

    virtual tString GetUrl()		const
    {
        se_CutString( sg_url, 75 );
        return sg_url;
    }
};

static gServerInfoAdmin sg_serverAdmin;

class eNameMessenger
{
public:
    eNameMessenger( ePlayerNetID & player )
    : player_( player )
    , oldScreenName_( player.GetName() )
    , oldLogName_( player.GetLogName() )
    {
        // store old name for password re-request and name change message
        oldPrintName_ << player_ << tColoredString::ColorString(.5,1,.5);
    }

    ~eNameMessenger()
    {
        if ( sn_GetNetState() == nCLIENT )
        {
            return;
        }

        tString logName = player_.GetLogName();
        tString const & screenName = player_.GetName();

        // messages for the users
        tColoredString printName;
        printName << player_ << tColoredString::ColorString(.5,1,.5);

        tOutput mess;
        
        mess.SetTemplateParameter(1, printName);
        mess.SetTemplateParameter(2, oldPrintName_);
        
        // is the player new?
        if ( oldLogName_.Len() <= 1 && logName.Len() > 0 )
        {
            if ( player_.IsHuman() )
            {
                tString ladder;
                ladder << "PLAYER_ENTERED " << logName << " " << nMachine::GetMachine(player_.Owner()).GetIP() << " " << screenName << "\n";
                se_SaveToLadderLog(ladder);
                
                player_.Greet();

                // print spectating join message (regular join messages are handled by eTeam)
                if ( player_.IsSpectating() || !se_assignTeamAutomatically )
                {
                    mess << "$player_entered_spectator";
                    sn_ConsoleOut(mess);
                }
            }
        }
        else if ( logName != oldLogName_ || screenName != oldScreenName_ )
        {
            tString ladder;
            ladder << "PLAYER_RENAMED " << oldLogName_  << " "  << logName << " " << nMachine::GetMachine(player_.Owner()).GetIP() << " " << screenName << "\n";
            se_SaveToLadderLog(ladder);


            if ( bool(player_.GetVoter() ) )
            {
                player_.GetVoter()->PlayerChanged();
            }

            if ( oldPrintName_ != printName )
            {
                mess << "$player_renamed";

                sn_ConsoleOut(mess);
            }
        }
    }

    ePlayerNetID & player_;
    tString oldScreenName_, oldLogName_;
    tColoredString oldPrintName_;
};

// ******************************************************************************************
// *
// *	UpdateName
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void ePlayerNetID::UpdateName( void )
{
    // don't do a thing if we're not fully synced
    if ( this->ID() == 0 && nameFromClient_.Len() <= 1 && sn_GetNetState() == nSERVER )
        return;
    
    // monitor name changes
    eNameMessenger messenger( *this );

    // apply client change, stripping excess spaces
    if ( sn_GetNetState() != nCLIENT )
    {
        // apply name filters only on remote players
        if ( Owner() != 0 )
            se_OptionalNameFilters( nameFromClient_ );

        // disallow name changes if there was a kick vote recently
        if ( !bool(this->voter_) || voter_->AllowNameChange() || nameFromServer_.Len() <= 1 )
        {
            nameFromServer_ = nameFromClient_;
        }
        else if ( nameFromServer_ != nameFromClient_ )
        {
            // inform victim to avoid complaints
            tOutput message( "$player_rename_rejected", nameFromServer_, nameFromClient_ );
            sn_ConsoleOut( message, Owner() );
            con << message;

            // revert name
            nameFromClient_ = nameFromServer_;
        }
    }

    // remove colors from name
    tString newName = tColoredString::RemoveColors( nameFromServer_ );
    tString newUserName = se_UnauthenticatedUserName( newName );

    // test if it is already taken, find an alternative name if so.
    if ( sn_GetNetState() != nCLIENT && !se_allowImposters && se_IsNameTaken( newUserName, this ) )
    {
        // Remove possilble trailing digit.
        if ( newName.Len() > 2 && isdigit(newName(newName.Len()-2)) )
        {
            newName = newName.SubStr( 0, newName.Len()-2 );
        }

        // append numbers until the name is free
        for ( int i=2; i<1000; ++i )
        {
            tString testName(newName);
            testName << i;

            // false the beginning if the name is too long
            if ( testName.Len() > 17 )
                testName = testName.SubStr( testName.Len() - 17 );

            newUserName = se_UnauthenticatedUserName( testName );

            if ( !se_IsNameTaken( newUserName, this ) )
            {
                newName = testName;
                break;
            }
        }

        // rudely overwrite name from client
        nameFromServer_ = newName;
    }

    // set the colored name to the name from the client, set trailing color to white
    coloredName_.Clear();
    REAL r,g,b;
    Color(r,g,b);
    coloredName_ << tColoredString::ColorString(r,g,b) << nameFromServer_;

    if ( name_ != newName || lastAccessLevel != GetAccessLevel() )
    {
        // copy it to the name, removing colors of course
        name_ = newName;

        // remove spaces and possibly other nasties for the user name
        userName_ = se_UnauthenticatedUserName( name_ );

        if (sn_GetNetState()!=nCLIENT)
        {
            // sync the new name
            RequestSync();
        }
    }

    // store access level for next update
    lastAccessLevel = GetAccessLevel();

#ifdef KRAWALL_SERVER
    // take the user name to be the authenticated name
    if ( IsAuthenticated() )
    {
        userName_ = GetFilteredAuthenticatedName();
        if ( se_legacyLogNames )
        {
            userName_ = tString( "0:" ) + userName_;
        }
    }
    else
    {
        rawAuthenticatedName_ = "";
    }
#endif
}

// filters illegal player characters
class ePlayerCharacterFilter
{
public:
    ePlayerCharacterFilter()
    {
        int i;
        filter[0]=0;

        // map all unknown characters to underscores
        for (i=255; i>0; --i)
        {
            filter[i] = '_';
        }

        // leave ASCII characters as they are
        for (i=126; i>32; --i)
        {
            filter[i] = i;
        }
        // but convert uppercase characters to lowercase
        for (i='Z'; i>='A'; --i)
        {
            filter[i] = i + ('a' - 'A');
        }

        //! map umlauts and stuff to their base characters
        SetMap(0xc0,0xc5,'a');
        SetMap(0xd1,0xd6,'o');
        SetMap(0xd9,0xdD,'u');
        SetMap(0xdf,'s');
        SetMap(0xe0,0xe5,'a');
        SetMap(0xe8,0xeb,'e');
        SetMap(0xec,0xef,'i');
        SetMap(0xf0,0xf6,'o');
        SetMap(0xf9,0xfc,'u');

        // ok, some of those are a bit questionable, but still better than _...
        SetMap(161,'!');
        SetMap(162,'c');
        SetMap(163,'l');
        SetMap(165,'y');
        SetMap(166,'|');
        SetMap(167,'s');
        SetMap(168,'"');
        SetMap(169,'c');
        SetMap(170,'a');
        SetMap(171,'"');
        SetMap(172,'!');
        SetMap(174,'r');
        SetMap(176,'o');
        SetMap(177,'+');
        SetMap(178,'2');
        SetMap(179,'3');
        SetMap(182,'p');
        SetMap(183,'.');
        SetMap(185,'1');
        SetMap(187,'"');
        SetMap(198,'a');
        SetMap(199,'c');
        SetMap(208,'d');
        SetMap(209,'n');
        SetMap(215,'x');
        SetMap(216,'o');
        SetMap(221,'y');
        SetMap(222,'p');
        SetMap(231,'c');
        SetMap(241,'n');
        SetMap(247,'/');
        SetMap(248,'o');
        SetMap(253,'y');
        SetMap(254,'p');
        SetMap(255,'y');

        //map 0 to o because they look similar
        SetMap('0','o');

        // TODO: make this data driven.
    }

    char Filter( unsigned char in )
    {
        return filter[ static_cast< unsigned int >( in )];
    }
private:
    void SetMap( int in1, int in2, unsigned char out)
    {
        tASSERT( in2 <= 0xff );
        tASSERT( 0 <= in1 );
        tASSERT( in1 < in2 );
        for( int i = in2; i >= in1; --i )
            filter[ i ] = out;
    }

    void SetMap( unsigned char in, unsigned char out)
    {
        filter[ static_cast< unsigned int >( in ) ] = out;
    }

    char filter[256];
};

static bool se_IsUnderscore( char c )
{
    return c == '_';
}

// ******************************************************************************************
// *
// *	FilterName
// *
// ******************************************************************************************
//!
//!		@param	in   input name
//!		@param	out  output name cleared to be usable as a username
//!
// ******************************************************************************************

void ePlayerNetID::FilterName( tString const & in, tString & out )
{
    int i;
    static ePlayerCharacterFilter filter;
    out = tColoredString::RemoveColors( in );
    
    // filter out illegal characters
    for ( i = out.Size()-1; i>=0; --i )
    {
        char & c = out[i];

        c = filter.Filter( c );
    }

    // strip leading and trailing unknown characters
    se_StripMatchingEnds( out, se_IsUnderscore, se_IsUnderscore );
}

// ******************************************************************************************
// *
// *	FilterName
// *
// ******************************************************************************************
//!
//!		@param	in   input name
//!		@return      output name cleared to be usable as a username
//!
// ******************************************************************************************

tString ePlayerNetID::FilterName( tString const & in )
{
    tString out;
    FilterName( in, out );
    return out;
}

// ******************************************************************************************
// *
// * SetName
// *
// ******************************************************************************************
//!
//!      @param  name    this player's name without colors. to set
//!       @return     A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID & ePlayerNetID::SetName( tString const & name )
{
    this->nameFromClient_ = name;
    this->nameFromClient_.NetFilter();

    // replace empty name
    if ( !IsLegalPlayerName( nameFromClient_ ) )
        nameFromClient_ = "Player 1";

    if ( sn_GetNetState() != nCLIENT )
        nameFromServer_ = nameFromClient_;

    UpdateName();

    return *this;
}

// ******************************************************************************************
// *
// * SetName
// *
// ******************************************************************************************
//!
//!      @param  name    this player's name without colors. to set
//!       @return     A reference to this to allow chaining
//!
// ******************************************************************************************

ePlayerNetID & ePlayerNetID::SetName( char const * name )
{
    SetName( tString( name ) );
    return *this;
}

// ******************************************************************************************
// *
// * GetFilteredAuthenticatedName
// *
// ******************************************************************************************
//!
//!       @return     The filtered authentication name, or "" if no authentication is supported or the player is not authenticated
//!
// ******************************************************************************************

tString ePlayerNetID::GetFilteredAuthenticatedName( void ) const
{
#ifdef KRAWALL_SERVER
    return tString( se_EscapeName( GetRawAuthenticatedName() ).c_str() );
#else
    return tString("");
#endif
}

// allow enemies from the same IP?
static bool se_allowEnemiesSameIP = false;
static tSettingItem< bool > se_allowEnemiesSameIPConf( "ALLOW_ENEMIES_SAME_IP", se_allowEnemiesSameIP );
// allow enemies from the same client?
static bool se_allowEnemiesSameClient = false;
static tSettingItem< bool > se_allowEnemiesSameClientConf( "ALLOW_ENEMIES_SAME_CLIENT", se_allowEnemiesSameClient );

// *******************************************************************************
// *
// *	Enemies
// *
// *******************************************************************************
//!
//!		@param	a	first player to compare
//!		@param	b	second player to compare
//!		@return		true if a should be able to score against b or vice versa
//!
// *******************************************************************************

bool ePlayerNetID::Enemies( ePlayerNetID const * a, ePlayerNetID const * b )
{
    // the client does not need a true answer
    if ( sn_GetNetState() == nCLIENT )
        return true;

    // no scoring if one of them does not exist
    if ( !a || !b )
        return false;

    // no scoring for two players from the same IP
    if ( !se_allowEnemiesSameIP && a->Owner() != 0 && a->GetMachine() == b->GetMachine() )
        return false;

    // no scoring for two players from the same client
    if ( !se_allowEnemiesSameClient && a->Owner() != 0 && a->Owner() == b->Owner() )
        return false;

    // no objections
    return true;
}

// *******************************************************************************
// *
// *	RegisterWithMachine
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::RegisterWithMachine( void )
{
    if ( !registeredMachine_ )
    {
        // store machine (it won't get deleted while this object exists; the player count prevents that)
        registeredMachine_ = &this->nNetObject::DoGetMachine();
        registeredMachine_->AddPlayer();
    }
}

// *******************************************************************************
// *
// *	UnregisterWithMachine
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::UnregisterWithMachine( void )
{
    if ( registeredMachine_ )
    {
        registeredMachine_->RemovePlayer();
        registeredMachine_ = 0;
    }
}

// *******************************************************************************
// *
// *	DoGetMachine
// *
// *******************************************************************************
//!
//!		@return		the machine this object belongs to
//!
// *******************************************************************************

nMachine & ePlayerNetID::DoGetMachine( void ) const
{
    // return machine I'm registered at, otherwise whatever the base class thinks
    if ( registeredMachine_ )
        return *registeredMachine_;
    else
        return nNetObject::DoGetMachine();
}

// *******************************************************************************
// *
// *	LastActivity
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

REAL ePlayerNetID::LastActivity( void ) const
{
    return tSysTimeFloat() - lastActivity_;
}

// *******************************************************************************
// *
// *	ResetScoreDifferences
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::ResetScoreDifferences( void )
{
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        if ( bool(p->Object()) && p->IsHuman() )
            p->lastScore_ = p->score;
    }
}

// *******************************************************************************
// *
// *	LogScoreDifferences
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::LogScoreDifferences( void )
{
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);
        p->LogScoreDifference();
    }
}

// *******************************************************************************
// *
// *	LogScoreDifference
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::LogScoreDifference( void )
{
    if ( lastScore_ > IMPOSSIBLY_LOW_SCORE && IsHuman() )
    {
        tString ret;
        int scoreDifference = score - lastScore_;
        lastScore_ = IMPOSSIBLY_LOW_SCORE;
        ret << "ROUND_SCORE " << scoreDifference << " " << GetUserName();
        if ( currentTeam )
            ret << " " << FilterName( currentTeam->Name() ) << " " << currentTeam->Score();
        ret << "\n";
        se_SaveToLadderLog( ret );
    }
}

static void se_allowTeamChangesPlayer(bool allow, std::istream &s) {
    if ( se_NeedsServer( "(DIS)ALLOW_TEAM_CHANGE_PLAYER", s, false ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );
    if ( p )
    {
        sn_ConsoleOut( tOutput( (allow ? "$player_allowed_teamchange" : "$player_disallowed_teamchange"), p->GetName() ) );
        p->TeamChangeAllowed( allow );
    }
}
static void se_allowTeamChangesPlayer(std::istream &s) {
    se_allowTeamChangesPlayer(true, s);
}
static void se_disallowTeamChangesPlayer(std::istream &s) {
    se_allowTeamChangesPlayer(false, s);
}
static tConfItemFunc se_allowTeamChangesPlayerConf("ALLOW_TEAM_CHANGE_PLAYER", &se_allowTeamChangesPlayer);
static tConfItemFunc se_disallowTeamChangesPlayerConf("DISALLOW_TEAM_CHANGE_PLAYER", &se_disallowTeamChangesPlayer);
static tAccessLevelSetter se_atcConfLevel( se_allowTeamChangesPlayerConf, tAccessLevel_TeamLeader );
static tAccessLevelSetter se_dtcConfLevel( se_disallowTeamChangesPlayerConf, tAccessLevel_TeamLeader );

