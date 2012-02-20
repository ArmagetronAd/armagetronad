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
#include "eTeam.h"
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
#include <algorithm>
#include "rRender.h"
#include "rFont.h"
#include "rSysdep.h"
#include "nAuthentication.h"
#include "tDirectories.h"
#include "eVoter.h"
#include "tReferenceHolder.h"
#include "tRandom.h"
#include "uInputQueue.h"
#include "nServerInfo.h"
#include "tRecorder.h"
#include "nConfig.h"
#include "nNetwork.h"
#include "nProtoBuf.h"
#include <time.h>
#include "tRuby.h"

#include "ePlayer.pb.h"

int se_lastSaidMaxEntries = 8;

// call on commands that only work on the server; quit if it returns true
bool se_NeedsServer(char const * command, std::istream & s, bool strict )
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

// tracking play time (in minutes). These times are tracked on the client and yes, you can "cheat" and increase them to get access to servers you are not ready for.

// total play time, network or not
static REAL se_playTimeTotal = 60*8;
static tConfItem< REAL > se_playTimeTotalConf( "PLAY_TIME_TOTAL", se_playTimeTotal );

// total online time
static REAL se_playTimeOnline = 60*8;
static tConfItem< REAL > se_playTimeOnlineConf( "PLAY_TIME_ONLINE", se_playTimeOnline );

// total online team time
static REAL se_playTimeTeam = 60*8;
static tConfItem< REAL > se_playTimeTeamConf( "PLAY_TIME_TEAM", se_playTimeTeam );

// total play time, network or not
static REAL se_minPlayTimeTotal = 0;
static nSettingItem< REAL > se_minPlayTimeTotalConf( "MIN_PLAY_TIME_TOTAL", se_minPlayTimeTotal );

// total online time
static REAL se_minPlayTimeOnline = 0;
static nSettingItem< REAL > se_minPlayTimeOnlineConf( "MIN_PLAY_TIME_ONLINE", se_minPlayTimeOnline );

// total online team time
static REAL se_minPlayTimeTeam = 0;
static nSettingItem< REAL > se_minPlayTimeTeamConf( "MIN_PLAY_TIME_TEAM", se_minPlayTimeTeam );

bool se_assignTeamAutomatically = true;
static tSettingItem< bool > se_assignTeamAutomaticallyConf( "AUTO_TEAM", se_assignTeamAutomatically );

static bool se_specSpam = true;
static tSettingItem< bool > se_specSpamConf( "AUTO_TEAM_SPEC_SPAM", se_specSpam );

static bool se_allowTeamChanges = true;
static tSettingItem< bool > se_allowTeamChangesConf( "ALLOW_TEAM_CHANGE", se_allowTeamChanges );

static bool se_enableChat = true;    //flag indicating whether chat should be allowed at all (logged in players can always chat)
static tSettingItem< bool > se_enaChat("ENABLE_CHAT", se_enableChat);

static tString se_hiddenPlayerPrefix ("0xaaaaaa");
static tConfItemLine se_hiddenPlayerPrefixConf( "PLAYER_LIST_HIDDEN_PLAYER_PREFIX", se_hiddenPlayerPrefix );

static tConfItemFunc Rename_conf("RENAME", &ForceName);
static tAccessLevelSetter Rename_confLevel( Rename_conf, tAccessLevel_Moderator );

static tReferenceHolder< ePlayerNetID > se_PlayerReferences;

class PasswordStorage
{
public:
    tString username; // name of the user the password belongs to
    tString methodCongested;   // method of scrambling
    nKrawall::nScrambledPassword password;
    bool save;

    PasswordStorage(): save(false){}
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
#if defined(KRAWALL_SERVER)
bool se_legacyLogNames = false;
#else
bool se_legacyLogNames = true;
#endif
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

            // don't chain escapes
            c = 'x';
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
    ~tConfItemPassword(){}

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
        con << "INTERNAL ERROR: Unknown password scrambling method requested.";
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
        if(username.Len() <= 1 || id.Len() <= username.Len() || id(username.Len() - 1) != '@') {
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

    se_ChatState( ePlayerNetID::ChatFlags_Menu, true );

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

    se_ChatState( ePlayerNetID::ChatFlags_Menu, false );
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

// flags indicating whether shouting should be the default chat action; if not, it's /team.
enum eShoutDefault
{
    eShoutDefault_Team = 0,             // default to /team chat
    eShoutDefault_Shout = 1,            // default to /shout chat
    eShoutDefault_ShoutAndOverride = 2  // default to /shout chat and override access level restrictions
};
tCONFIG_ENUM( eShoutDefault );

static eShoutDefault se_shoutSpectator=eShoutDefault_Shout;
tSettingItem< eShoutDefault > se_shoutSpectatorConf( "DEFAULT_SHOUT_SPECTATOR", se_shoutSpectator );
static eShoutDefault se_shoutPlayer=eShoutDefault_Shout;
tSettingItem< eShoutDefault > se_shoutPlayerConf( "DEFAULT_SHOUT_PLAYER", se_shoutPlayer );

#ifdef KRAWALL_SERVER
// minimal access level to shout
static tAccessLevel se_shoutAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_shoutAccessLevelConf( "ACCESS_LEVEL_SHOUT", se_shoutAccessLevel );
static tAccessLevelSetter se_shoutAccessLevelConfLevel( se_shoutAccessLevelConf, tAccessLevel_Owner );

// minimal access level to play
static tAccessLevel se_playAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_playAccessLevelConf( "ACCESS_LEVEL_PLAY", se_playAccessLevel );
static tAccessLevelSetter se_playAccessLevelConfLevel( se_playAccessLevelConf, tAccessLevel_Owner );

static tAccessLevel se_playAccessLevelInvited = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_playAccessLevelInvitedConf( "ACCESS_LEVEL_PLAY_INVITED", se_playAccessLevelInvited );
static tAccessLevelSetter se_playAccessLevelInvitedConfLevel( se_playAccessLevelInvitedConf, tAccessLevel_Owner );

// minimal sliding access level to play (slides up as soon as enoughpeople of higher access level get authenticated )
static tAccessLevel se_playAccessLevelSliding = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_playAccessLevelSlidingConf( "ACCESS_LEVEL_PLAY_SLIDING", se_playAccessLevelSliding );
static tAccessLevelSetter se_playAccessLevelSlidingConfLevel( se_playAccessLevelSlidingConf, tAccessLevel_Owner );

// that many high level players are reuqired to drag the access level up
static int se_playAccessLevelSliders = 4;
static tSettingItem< int > se_playAccessLevelSlidersConf( "ACCESS_LEVEL_PLAY_SLIDERS", se_playAccessLevelSliders );
static tAccessLevelSetter se_playAccessLevelSlidersConfLevel( se_playAccessLevelSlidersConf, tAccessLevel_Owner );

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
static tAccessLevel se_hideAccessLevelOf = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_hideAccessLevelOfConf( "ACCESS_LEVEL_HIDE_OF", se_hideAccessLevelOf );
static tAccessLevelSetter se_hideAccessLevelOfConfLevel( se_hideAccessLevelOfConf, tAccessLevel_Owner );

// but they are only hidden to players with a lower access level than this
static tAccessLevel se_hideAccessLevelTo = tAccessLevel_Armatrator;
static tSettingItem< tAccessLevel > se_hideAccessLevelToConf( "ACCESS_LEVEL_HIDE_TO", se_hideAccessLevelTo );
static tAccessLevelSetter se_hideAccessLevelToConfLevel( se_hideAccessLevelToConf, tAccessLevel_Owner );

// determines whether hider can hide from seeker
static bool se_Hide( ePlayerNetID const * hider, tAccessLevel currentLevel )
{
    tASSERT( hider );

    return
    hider->GetAccessLevel() <= se_hideAccessLevelOf &&
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

static bool se_CanHide ( ePlayerNetID const * hider )
{
    return hider->GetAccessLevel() <= se_hideAccessLevelOf;
}

#endif

typedef bool (*CANHIDEFUNC)( ePlayerNetID const * hider );
typedef bool (*HIDEFUNC)( ePlayerNetID const * hider, ePlayerNetID const * seeker );

// secret console messages: If CanHideFunc returns false it is displayed to everyone. Else, for each player we apply HideFunc, to see if one can hide his message to the other. The two exceptions get a message anyway.
void se_SecretConsoleOut( tOutput const & message, ePlayerNetID const * hider, HIDEFUNC HideFunc, ePlayerNetID const * exception1 = 0, ePlayerNetID const * exception2 = 0, CANHIDEFUNC CanHideFunc = 0 )
{
    // high enough access levels are never secret
    if ( CanHideFunc != 0 && !(*CanHideFunc)( hider ) )
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
        
        // well, the admin will want to see it.
        canSee[0] = true;

        // look which clients have someone who can see the message
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);
            if ( player == exception1 || player == exception2 || !(*HideFunc)( hider, player ) )
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

#ifdef KRAWALL_SERVER

static eLadderLogWriter se_authorityBlurbWriter("AUTHORITY_BLURB", true);

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
        ePlayerNetID* player2 = se_PlayerNetIDs(i);
        if ( player2->IsAuthenticated() && player2->GetRawAuthenticatedName() == authName )
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

            se_authorityBlurbWriter << token << player->GetFilteredAuthenticatedName() << rest;
            se_authorityBlurbWriter.write();
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
    tCONTROLLED_PTR( ePlayerNetID ) player_;        // keep player referenced
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
     "Only idiots miss holes!",
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

static char const * se_defaultGID = "@forums";

ePlayer::ePlayer():cockpit(0){
    nAuthentication::SetUserPasswordCallback(&PasswordCallback);
#ifdef KRAWALL_SERVER
    nAuthentication::SetLoginResultCallback (&ResultCallback);
#endif

    lastTooltip_ = -100;

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

    // default global ID so logins are redirected to the forums
    globalID = se_defaultGID;

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
                                        "$auto_login_confitem_help",
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
    startCamera=CAMERA_CUSTOM;
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
                                        "$hide_identity_confitem_help",
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

extern uActionTooltip::Level su_helpLevel;

#ifndef DEDICATED
void ePlayer::Render(){
    if (cam) cam->Render();

    // present tooltip help
    double now = tSysTimeFloat();
    if( se_GameTime() > 1 && now-lastTooltip_ > 1 && !rConsole::CenterDisplayActive() )
    {
        if( uActionTooltip::Help( ID()+1 ) || uActionTooltip::Help( 0 ) || VetoActiveTooltip(ID()+1) || su_helpLevel != uActionTooltip::Level_Expert )
            lastTooltip_ = now;
        else
            lastTooltip_ = now+60;
    }
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
static void se_LoginRequestHandler( Engine::LoginRequest const & request, nSenderInfo const & sender )
{
#ifdef KRAWALL_SERVER

    // read player
    ePlayerNetID * p;
    nNetObject::IDToPointer( request.player_id(), p );

    if ( p && sender.SenderID() == p->Owner() && !p->IsAuthenticated() )
    {
        // read wanted flag
        p->loginWanted = request.login_wanted();
        tString authName = request.username();

        // read authentication name
        p->SetRawAuthenticatedName( authName );

        // check for stupid bans
        if ( se_IsUserBanned( p, authName ) )
        {
            return;
        }

        se_RequestLogin( p );
    }
#else
    sn_ConsoleOut( tOutput( "$login_not_supported" ), sender.SenderID() );
#endif
}

static nProtoBufDescriptor< Engine::LoginRequest > se_loginRequestDescriptor( 204, se_LoginRequestHandler );

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

    Engine::LoginRequest & request = se_loginRequestDescriptor.Send(0);

    // write player
    request.set_player_id( nNetObject::PointerToID(  p ) );

    // write flag and name
    request.set_login_wanted( p->loginWanted );

    // write authentication name
    request.set_username( lp->globalID );

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

static bool se_silenceEnemies = false;
static tSettingItem< bool > se_silenceEnemiesConf( "SILENCE_ENEMIES", se_silenceEnemies );

static void se_HighlightName( tString const & name, tString & message )
{
    if ( name.size() > 0 )
    {
        //find all occureces of the name...
        for(tString::size_type pos = message.find(name); pos != tString::npos; pos = message.find(name, pos+16)) {
            //find the last color code
            tString::size_type lastcolorpos = message.rfind("0x", pos);
            tString lastcolorstring;
            if(lastcolorpos != tString::npos) {
                lastcolorstring = message.SubStr(lastcolorpos, 8);
            } else {
                lastcolorstring = "0xffff7f";
            }
            
            if(lastcolorpos == 0 || lastcolorpos >= pos - 8) {
                //the name we matched is within a color code... bad idea to substitute it.
                pos -= 16 - name.size();
                continue;
            }
            
            //actually insert the color codes around the name
            message.insert(pos+name.size(), lastcolorstring);
            message.insert(pos, "0xff887f");
        }
    }
}

static void se_DisplayChatLocallyClient( ePlayerNetID* p, const tString& message )
{
    if ( p && !p->IsSilenced() && se_enableChat )
    {
        tString actualMessage(message);

        bool sentFromTeamMember = false;
        if( se_enableNameHilighting || se_silenceEnemies )
        {
            rViewportConfiguration* viewportConfiguration = rViewportConfiguration::CurrentViewportConfiguration();
            if(viewportConfiguration == 0) return;
            
            
            for ( int viewport = viewportConfiguration->num_viewports-1; viewport >= 0; --viewport ) {
                ePlayer* player = ePlayer::PlayerConfig( sr_viewportBelongsToPlayer[ viewport ] );
                
                if ( !player )
                    continue;
                
                ePlayerNetID *me = player->netPlayer;
                if ( me )
                {
                    if ( se_enableNameHilighting && me != p )
                        se_HighlightName( me->GetName(), actualMessage );
                    
                    if ( se_silenceEnemies )
                    {
                        // If we're dead, display all chat
                        if ( ( !me->Object() || !me->Object()->Alive() ) || me->CurrentTeam() == p->CurrentTeam() )
                            sentFromTeamMember = true;
                    }
                }
            }
        }
        
        if ( se_silenceEnemies && !sentFromTeamMember )
            return;
        
        if ( p->CurrentTeam() && ( !p->Object() || !p->Object()->Alive() ) )
            con << tOutput("$dead_console_decoration");
        
        con << actualMessage << "\n";
    }
}

static nVersionFeature se_chatRelay( 3 );

static nVersionFeature se_chatHandlerClient( 6 );

// chat message from client to server
// void se_ChatHandlerServer( nMessage & );
void se_ChatHandlerServer( Engine::Chat const &, nSenderInfo const & );
static nProtoBufDescriptor< Engine::Chat > se_chatDescriptorServer( 200, se_ChatHandlerServer );

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


ePlayerNetID * ePlayerNetID::FindPlayerByName( tString const & name, ePlayerNetID * requester, bool print )
{
    int num_matches = 0;

    // try filtering the names before comparing them, this makes the matching case-insensitive
    SE_NameFilter Filter = &ePlayerNetID::FilterName;

    // look for matches in the filtered screen names
    ePlayerNetID * ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetName, Filter, &se_NonHide );
    if ( ret && num_matches == 1 )
    {
        return ret;
    }

    // look for matches in the exact screen names.
    // No check for prior matches here, because the previous
    // search was less specific.
    ret = CompareBufferToPlayerNames( name, num_matches, requester, &ePlayerNetID::GetName, &se_NameFilterID, &se_NonHide );
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
        if (print)
        {
            if ( requester )
                sn_ConsoleOut(toSender,requester->Owner() );
            else
                con << toSender;
        }
        return NULL;
    }
    // 0 matches. The user can't spell. Complain about that, too.
    else {
        tOutput toSender( "$msg_nomatch", name );
        if (print)
        {
            if ( requester )
                sn_ConsoleOut(toSender,requester->Owner() );
            else
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

    return ePlayerNetID::FindPlayerByName( player, sender );
}

// chat message from server to client
void se_ChatHanderClient( Engine::Chat const & chat, nSenderInfo const & sender );
static nProtoBufDescriptor< Engine::Chat > se_chatDescriptorClient( 203, se_ChatHanderClient );

static bool se_FilterOutName( tString const & name, tString & toFilter, bool startOnly = false )
{
    int pos = toFilter.StrPos( name );
    if ( pos < 0 )
    {
        return false;
    }
    else
    {
        if ( startOnly && pos > 0 )
        {
            return false;
        }

        toFilter = toFilter.SubStr( 0, pos ) + toFilter.SubStr( pos + name.size() );
        return true;
    }
}

// chat message transformation for old clients
class nMessageTranslatorChat: public nMessageTranslator< Engine::Chat >
{
public:
    nMessageTranslatorChat(): nMessageTranslator< Engine::Chat >( se_chatDescriptorClient ){}

    //! convert current message format to format suitable for old client
    virtual nMessageBase * Translate( Engine::Chat const & source, int receiver ) const
    {
        // client old enough for formatted chat message?
        if ( se_chatHandlerClient.Supported( receiver ) )
        {
            return NULL;
        }

        ePlayerNetID *p;
        nNetObject::IDToPointer( source.player_id(), p );

        if ( se_chatRelay.Supported( receiver ) && (!p || p->Owner() != receiver ) )
        {
            // transform message to server chat message, the client will understand that, too
            nProtoBufMessage< Engine::Chat > * m = se_chatDescriptorServer.CreateMessage();
            Engine::Chat & dest = m->AccessProtoBuf();
            
            dest.set_player_id( source.player_id() );

            tString chat = source.chat_line();

            // filter player name from chat line
            if( p )
            {
                se_FilterOutName( p->GetColoredName(), chat ) ||
                se_FilterOutName( p->GetName(), chat );
            }

            // filter colon from chat line
            {
                tColoredString colon;
                colon << tColoredString::ColorString( 1,1,.5 ) << ": ";
                se_FilterOutName( colon, chat, true );
            }

            dest.set_chat_line( chat );

            return m;
        }
        else
        {
            // very old clients require console messages.
            return sn_ConsoleOutMessage( source.chat_line() + "\n" );

            // not so very old clients get this type of translation too if the sender
            // is a local player on the receiver and unlikely to be silenced.
        }
    }
    
};

static nMessageTranslatorChat se_chatTranslator;


void se_ChatHanderClient( Engine::Chat const & chat, nSenderInfo const & sender )
{
    if(sn_GetNetState()!=nSERVER)
    {
        unsigned short id = chat.player_id();
        tColoredString say( chat.chat_line().c_str() );

        tJUST_CONTROLLED_PTR< ePlayerNetID > p=dynamic_cast<ePlayerNetID *>(nNetObject::ObjectDangerous(id));

        se_DisplayChatLocallyClient( p, say );
    }
}

// appends chat message to something
template< class TARGET >
void se_AppendChat( TARGET & out, tString const & message )
{
    if ( message.Len() <= se_SpamMaxLen*2 || se_SpamMaxLen == 0 )
        out << message;
    else
    {
        tString cut( message );
        cut.SetLen( se_SpamMaxLen*2 );
        out << cut;
    }
}

// builds a regular colored chat string
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

    if( !team )
    {
        // foo --> Spectatos: message
        console << tColoredString::ColorString(1,1,.5) << " --> " << tOutput("$player_spectator_message");
    }
    else if (sender->CurrentTeam() == team)
    {
        // foo --> Teammates: some message here
        console << tColoredString::ColorString(1,1,.5) << " --> ";
        // We don't << team here, because we want "Teammates" to show instead of the team name
        console << tColoredString::ColorString( team->R() / 15.0, team->G() / 15.0, team->B() / 15.0 ) << tOutput("$player_team_message");
    }
    else {
        // foo (Red Team) --> Blue Team: some message here
        eTeam *senderTeam = sender->CurrentTeam();
        console << tColoredString::ColorString(1,1,.5) << " (";
        console << *senderTeam;
        console << tColoredString::ColorString(1,1,.5) << ") --> ";
        console << *team;
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
static nMessageBase* se_ServerControlledChatMessageConsole( ePlayerNetID const * player, tString const & toConsole )
{
    tASSERT( player );

    nProtoBufMessage< Engine::Chat > * m = se_chatDescriptorClient.CreateMessage();
    Engine::Chat & chat = m->AccessProtoBuf();

    chat.set_player_id( player->ID() );
    chat.set_chat_line( toConsole );

    return m;
}

// prepares a chat message with a chat string (only the message, not the whole console line) originating from the given player
static nMessageBase * se_ServerControlledChatMessage( ePlayerNetID const * sender, tString const & message )
{
    tASSERT( sender );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString( sender, message ) );
}

// prepares a chat message with a chat message originating from the given player to the given receiver
static nMessageBase * se_ServerControlledChatMessage( ePlayerNetID const * sender, ePlayerNetID const * receiver, tString const & message )
{
    tASSERT( sender );
    tASSERT( receiver );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString( sender, receiver, message ) );
}

// prepares a chat message with a chat message originating from the given player to the given team
static nMessageBase * se_ServerControlledChatMessage(  eTeam const * team, ePlayerNetID const * sender, tString const & message )
{
    tASSERT( sender );

    return se_ServerControlledChatMessageConsole( sender, se_BuildChatString(team, sender, message) );
}

// pepares a chat message for the server
static nMessageBase * se_ChatMessageForServer( ePlayerNetID const * player, tString const & message )
{
    tASSERT( player );

    // clean up chat
    std::ostringstream cleanup;
    se_AppendChat( cleanup, message );

    nProtoBufMessage< Engine::Chat > * m = se_chatDescriptorServer.CreateMessage();
    Engine::Chat & chat = m->AccessProtoBuf();
    chat.set_player_id( player->ID() );
    chat.set_chat_line( cleanup.str() );
    
    return m;
}

// sends the full chat line to receiver, marked as originating from <sender> so
// it can be silenced.
// ( the client will see <fullLine> resp. <sender name> : <forOldClients> if it is pre-0.2.8 and at least 0.2.6 )
static
void se_SendChatLine( ePlayerNetID* sender, const tString& fullLine, int receiver )
{
    if ( sn_Connections[ receiver ].socket )
    {
        tJUST_CONTROLLED_PTR< nMessageBase > mServerControlled = se_ServerControlledChatMessageConsole( sender, fullLine );
        mServerControlled->Send( receiver );
    }
}

// sends the full chat line to all connected clients, marked as originating from <sender> so
// it can be silenced.
// ( the client will see <fullLine> resp. <sender name> : <forOldClients> if it is pre-0.2.8 and at least 0.2.6 )
static
void se_BroadcastChatLine( ePlayerNetID* sender, const tString& line )
{
    se_ServerControlledChatMessageConsole( sender, line )->BroadCast();
}

// send the chat of player p to all connected clients after properly formatting it
// ( the clients will see <player>: <say>
static
void se_BroadcastChat( ePlayerNetID* sender, const tString& say )
{
    se_ServerControlledChatMessage( sender, say )->BroadCast();
}


// sends a private message from sender to receiver, and really sends it to eavesdropper (will usually be equal to receiver)
static
void se_SendPrivateMessage( ePlayerNetID const * sender, ePlayerNetID const * receiver, ePlayerNetID const * eavesdropper, tString const & message )
{
    tASSERT( sender );
    tASSERT( receiver );

    // determine receiver client id
    int cid = eavesdropper->Owner();

    // determine wheter the receiver knows about the server controlled chat message
    se_ServerControlledChatMessage( sender, receiver, message )->Send( cid );
}

// Sends a /team message
static
void se_SendTeamMessage( eTeam const * team, ePlayerNetID const * sender ,ePlayerNetID const * receiver, tString const & message )
{
    tASSERT( receiver );
    tASSERT( sender );

    int clientID = receiver->Owner();
    if ( clientID == 0 )
        return;

    se_ServerControlledChatMessage( team, sender, message )->Send( clientID );
}

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

#ifndef KRAWALL_SERVER
//The Base Remote Admin Password
static tString sg_adminPass( "NONE" );
static tConfItemLine sg_adminPassConf( "ADMIN_PASS", sg_adminPass );
#endif

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
        unsigned long len = message_.Len();
        tRecorderSync< unsigned long >::Archive( "_MESSAGE_LEN", 3, len );
        if (len > 600)
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
static tAccessLevelSetter se_adminAccessLevelConfLevel( se_adminAccessLevelConf, tAccessLevel_Owner );

void handle_command_intercept( ePlayerNetID *p, tString const & command, std::istream & s, tString const & say ) {
    static eLadderLogWriter se_commandWriter( "COMMAND", true );
    
    tString commandArguments;
    commandArguments.ReadLine( s );
    
    se_commandWriter << command << p->GetLogName() << commandArguments;
    se_commandWriter.write();
    
    con << "[cmd] " << p->GetLogName() << ": " << say << '\n';
}

#ifdef KRAWALL_SERVER

// minimal access level for /op/deop
static tAccessLevel se_opAccessLevel = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_opAccessLevelConf( "ACCESS_LEVEL_OP", se_opAccessLevel );
static tAccessLevelSetter se_opAccessLevelConfLevel( se_opAccessLevelConf, tAccessLevel_Owner );

// maximal result thereof
static tAccessLevel se_opAccessLevelMax = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_opAccessLevelMaxConf( "ACCESS_LEVEL_OP_MAX", se_opAccessLevelMax );
static tAccessLevelSetter se_opAccessLevelMaxConfLevel( se_opAccessLevelMaxConf, tAccessLevel_Owner );

static bool se_CanChangeAccess( ePlayerNetID * admin, ePlayerNetID * victim, char const * command )
{
    tASSERT( admin );
    tASSERT( victim );

    if ( admin->GetAccessLevel() > se_opAccessLevel ) // Can he even use this command?
    {
        sn_ConsoleOut( tOutput( "$access_level_op_denied", command ), admin->Owner() );
    }
    else if ( victim == admin )
    {
        sn_ConsoleOut( tOutput( "$access_level_op_self", command ), admin->Owner() );
    }
    else if ( admin->GetAccessLevel() >= victim->GetAccessLevel()  )
    {
        sn_ConsoleOut( tOutput( "$access_level_op_overpowered", command ), admin->Owner() );
    }
    else
    {
        return true;
    }

    return false;

}

// an operation that changes the access level of another player
typedef void (*OPFUNC)( ePlayerNetID * admin, ePlayerNetID * victim, tAccessLevel accessLevel );

static void se_ChangeAccess( ePlayerNetID * admin, std::istream & s, char const * command, OPFUNC F )
{
    bool isexplicit = false;

    ePlayerNetID * victim = se_FindPlayerInChatCommand( admin, command, s );

    if ( victim && se_CanChangeAccess( admin, victim, command ) )
    {
        // read optional access level, this part is merly a copypaste from the /shuffle code
        int level = se_opAccessLevelMax;
        if ( victim->IsAuthenticated() )
        {
            level = victim->GetAccessLevel();
        }
        char first;
        s >> first;
        if ( !s.eof() && !s.fail() )
        {
            isexplicit = true;
            s.unget();
            int newLevel = 0;
            s >> newLevel;

            if ( first == '+' || first == '-' )
            {
                level += newLevel;
            }
            else
            {
                level = newLevel;
            }
        }

        if ( !isexplicit && victim->IsAuthenticated() )
        {
            level = se_opAccessLevelMax;
        }

        s >> level;

        // Make a last safety check on the given AL, then DON'T TOUCH IT ANYMORE
        if ( level <= admin->GetAccessLevel() )
            level = admin->GetAccessLevel() + 1;

        tAccessLevel accessLevel;
        accessLevel = static_cast< tAccessLevel >( level );

        if ( accessLevel == victim->GetAccessLevel() )
        {
            if ( isexplicit )
            {
                sn_ConsoleOut( tOutput( "$access_level_op_same", command ), admin->Owner() );
            }
            else
            {
                sn_ConsoleOut( tOutput( "$access_level_op_unclear", command ), admin->Owner() );
            }
        }
        else if ( accessLevel > admin->GetAccessLevel() )
        {
            (*F)( admin, victim, accessLevel );
        }
    }
}

// Promote changes the access rights of an already authed player
void se_Promote( ePlayerNetID * admin, ePlayerNetID * victim, tAccessLevel accessLevel )
{
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
        tAccessLevel oldAccessLevel = victim->GetAccessLevel();
        victim->SetAccessLevel( accessLevel );

        if ( accessLevel < oldAccessLevel )
        {
            se_SecretConsoleOut( tOutput( "$access_level_promote",
                                          victim->GetLogName(),
                                          tCurrentAccessLevel::GetName( accessLevel ),
                                          admin->GetLogName() ), victim, &se_Hide, admin, 0, &se_CanHide );
        }
        else if ( accessLevel > oldAccessLevel )
        {
            se_SecretConsoleOut( tOutput( "$access_level_demote",
                                 victim->GetLogName(),
                                 tCurrentAccessLevel::GetName( accessLevel ),
                                 admin->GetLogName() ), victim, &se_Hide, admin, 0, &se_CanHide );

        }
    }
}

// our operations of this kind: op grants access
void se_OpBase( ePlayerNetID * admin, ePlayerNetID * victim, char const * command, tAccessLevel accessLevel )
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
        return;
    }

    victim->Authenticate( authName, accessLevel, admin );
}

void se_Op( ePlayerNetID * admin, ePlayerNetID * victim, tAccessLevel level )
{
    int accessLevel = admin->GetAccessLevel() + 1;

    // respect passed in level
    if ( accessLevel < level )
    {
        accessLevel = level;
    }

    if ( victim->IsAuthenticated() )
    {
        se_Promote( admin, victim, static_cast< tAccessLevel >( accessLevel ) );
    }
    else
    {
        se_OpBase( admin, victim, "/op", static_cast< tAccessLevel >( accessLevel ) );
    }
}

// DeOp takes it away
void se_DeOp( ePlayerNetID * admin, std::istream & s, char const * command )
{
    ePlayerNetID * victim = se_FindPlayerInChatCommand( admin, command, s );

    if ( victim && se_CanChangeAccess ( admin, victim, command ) )
    {
        if ( victim->IsAuthenticated() )
        {
            victim->DeAuthenticate( admin );
        }
        else
        {
            sn_ConsoleOut( tOutput( "$access_level_op_same", command ), admin->Owner() );
        }
    }
}

// Changes the access level of a player, from console or other anonymous sources
// and which is not to be called when you can call an OPFUNC
// This isn't meant to be an user command, rather a simple switch between promoting and logging in, so make necessary checks yourself
void se_AnonOp( ePlayerNetID * victim, tAccessLevel accessLevel, bool messages=true )
{
    if ( !victim->IsHuman() )
    {
        return;
    }

    if ( victim->IsAuthenticated() )
    {
        if ( accessLevel > tAccessLevel_Authenticated )
        {
            accessLevel = tAccessLevel_Authenticated;
        }

        tAccessLevel oldAccessLevel = victim->GetAccessLevel();
        victim->SetAccessLevel( accessLevel );


        if ( accessLevel < oldAccessLevel && messages )
        {
            se_SecretConsoleOut( tOutput( "$access_level_promote_anon",
                                          victim->GetLogName(),
                                          tCurrentAccessLevel::GetName( accessLevel )
                                            ), victim, &se_Hide, 0, 0, &se_CanHide );
        }
        else if ( accessLevel > oldAccessLevel && messages )
        {
            se_SecretConsoleOut( tOutput( "$access_level_demote_anon",
                                 victim->GetLogName(),
                                 tCurrentAccessLevel::GetName( accessLevel )
                                        ), victim, &se_Hide, 0, 0, &se_CanHide );
        }
    }
    else
    {
        tString authName = victim->GetUserName() + "@L_OP";
        if ( victim->IsAuthenticated() )
        {
            authName = victim->GetRawAuthenticatedName();
        }

        victim->Authenticate( authName, accessLevel, 0, messages );
    }
}

// Console command for it
void se_OpConf( std::istream &s )
{
    if ( se_NeedsServer( "OP", s ) )
    {
        return;
    }

    ePlayerNetID * victim = se_FindPlayerInChatCommand( 0, "OP", s );
    bool isexplicit = false;

    if ( victim )
    {
        // read optional access level, this part is merly a copypaste from the /shuffle code
        int level = se_opAccessLevelMax;
        if ( victim->IsAuthenticated() )
        {
            level = victim->GetAccessLevel();
        }
        char first;
        s >> first;
        if ( !s.eof() && !s.fail() )
        {
            isexplicit = true;
            s.unget();
            int newLevel = 0;
            s >> newLevel;

            if ( first == '+' || first == '-' )
            {
                level += newLevel;
            }
            else
            {
                level = newLevel;
            }
        }

        s >> level;

        tAccessLevel accessLevel;
        accessLevel = static_cast< tAccessLevel >( level );

        if ( accessLevel == victim->GetAccessLevel() )
        {
            if ( isexplicit )
            {
                sn_ConsoleOut( tOutput( "$access_level_op_same", "OP" ), 0 );
            }
            else
            {
                sn_ConsoleOut( tOutput( "$access_level_op_unclear", "OP" ), 0 );
            }
        }
        else
        {
            se_AnonOp( victim, accessLevel );
        }
    }

}

static tConfItemFunc se_opConf( "OP", &se_OpConf );
static tAccessLevelSetter se_opConfLevel( se_opConf, tAccessLevel_Owner );

void se_AnonDeOp( ePlayerNetID * victim, bool messages=true )
{
    if ( victim->IsAuthenticated() )
    {
        victim->DeAuthenticate( 0, messages );
    }
}
void se_DeOpConf( std::istream &s )
{
    if ( se_NeedsServer( "DEOP", s ) )
    {
        return;
    }

    ePlayerNetID * victim = se_FindPlayerInChatCommand( 0, "DEOP", s );

    if ( victim )
    {
        se_AnonDeOp( victim, true );
    }
}

static tConfItemFunc se_deOpConf( "DEOP", &se_DeOpConf );
static tAccessLevelSetter se_deOpConfLevel( se_deOpConf, tAccessLevel_Owner );

void se_MakeReferee( ePlayerNetID * victim, ePlayerNetID * admin )
{
    se_AnonOp( victim, tAccessLevel_Referee, false );
    if ( admin )
    {
        sn_ConsoleOut( tOutput( "$player_referee", admin->GetColoredName(), victim->GetColoredName() ) );
    }
    else
    {
        sn_ConsoleOut( tOutput( "$player_referee_anon", victim->GetColoredName() ) );
    }
}

void se_CancelReferee( ePlayerNetID * victim, ePlayerNetID * admin )
{
    se_AnonDeOp( victim, false );
    if ( admin )
    {
        sn_ConsoleOut( tOutput( "$player_referee_nomore", admin->GetColoredName(), victim->GetColoredName() ) );
    }
    else
    {
        sn_ConsoleOut( tOutput( "$player_referee_nomore_anon", victim->GetColoredName() ) );
    }
}

// minimal access level for /team management
static tAccessLevel se_teamAccessLevel = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_teamAccessLevelConf( "ACCESS_LEVEL_TEAM", se_teamAccessLevel );
static tAccessLevelSetter se_teamAccessLevelConfLevel( se_teamAccessLevelConf, tAccessLevel_Owner );

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
static void se_AdminLogout( ePlayerNetID * p, char const * command )
{
#ifdef KRAWALL_SERVER
    // revoke the other kind of authentication as well
    if ( p->IsAuthenticated() )
    {
        p->DeAuthenticate();
    }
    else
    {
        sn_ConsoleOut( tOutput( "$access_level_op_same", command ), p->Owner() );
    }
#else
    if ( p->IsLoggedIn() )
    {
        sn_ConsoleOut("You have been logged out!\n",p->Owner());
    }
    p->BeNotLoggedIn();
#endif
}

// access level a user has to have to be able to see what's being typed at /admin
static tAccessLevel se_consoleSpyAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_consoleSpyAccessLevelConf( "ACCESS_LEVEL_SPY_CONSOLE", se_consoleSpyAccessLevel );
static tAccessLevelSetter se_consoleSpyAccessLevelConfLevel( se_consoleSpyAccessLevelConf, tAccessLevel_Owner );

static bool se_cannotSeeConsole( ePlayerNetID const *, ePlayerNetID const * seeker )
{
    return seeker->GetAccessLevel() > se_consoleSpyAccessLevel;
}

// /admin chat command
static void se_AdminAdmin( ePlayerNetID * p, std::istream & s )
{
    if ( p->GetAccessLevel() > se_adminAccessLevel )
    {
        sn_ConsoleOut( tOutput( "$access_level_admin_denied" ), p->Owner() );
        return;
    }

    tString str;
    str.ReadLine(s);
    tColoredString msg;
    msg << tColoredString::ColorString(1,0,0) << "Remote admin command" << tColoredString::ColorString(-1,-1,-1) << " by " << tColoredString::ColorString(1,1,.5) << p->GetUserName() << tColoredString::ColorString(-1,-1,-1) << ": " << tColoredString::ColorString(.5,.5,1) << str << "\n";
    se_SecretConsoleOut( msg, p, &se_cannotSeeConsole, p );
    std::istringstream stream(&str(0));

    // install filter
    eAdminConsoleFilter consoleFilter( p->Owner() );
    try
    {
        // forbid CASACL
        tCasaclPreventer preventer;

        tConfItemBase::LoadLine(stream);
    }
    catch (tAbortLoading)
    {
        con << tOutput("$config_abort");
    }
}

static void handle_chat_admin_commands( ePlayerNetID * p, tString const & command, tString const & say, std::istream & s, eChatSpamTester &spam )
{
    if  (command == "/login")
    {
        // Really, there's no reason one would log in and log out all the time
        spam.factor_ = 1;
        if ( spam.Block() )
        {
            return;
        }

        // the following function really is only supposed to be called from here and nowhere else
        // (access right escalation risk)
        se_AdminLogin_ReallyOnlyCallFromChatKTHNXBYE( p, s );
    }
    else  if (command == "/logout")
    {
        spam.factor_ = 1;
        if( spam.Block() )
        {
            return;
        }

        se_AdminLogout( p, command );
    }
#ifdef KRAWALL_SERVER
    else if ( command == "/op" )
    {
        se_ChangeAccess( p, s, "/op", &se_Op );
    }
    else if ( command == "/deop" )
    {
        se_DeOp( p, s, "/deop" );
    }
    else if ( command == "/invite" )
    {
        spam.factor_ = 0.4;
        if( spam.Block() )
        {
            return;
        }

        se_Invite( command, p, s, &eTeam::Invite );
    }
    else if ( command == "/uninvite" )
    {
        spam.factor_ = 0.4;
        if( spam.Block() )
        {
            return;
        }

        se_Invite( command, p, s, &eTeam::UnInvite );
    }
    else if ( command == "/lock" )
    {
        spam.factor_ = 0.4;
        if( spam.Block() )
        {
            return;
        }

        se_Lock( command, p, s, true );
    }
    else if ( command == "/unlock" )
    {
        spam.factor_ = 0.4;
        if( spam.Block() )
        {
            return;
        }

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
            handle_command_intercept(p, command, s, say);
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

REAL se_alreadySaidTimeout=5.0;
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
static tAccessLevelSetter se_shuffleUpAccessLevelConfLevel( se_shuffleUpAccessLevelConf, tAccessLevel_Owner );
#endif

static bool se_silenceDefault = false;        // flag indicating whether new players should be silenced

// minimal access level for chat
tAccessLevel se_chatAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_chatAccessLevelConf( "ACCESS_LEVEL_CHAT", se_chatAccessLevel );
static tAccessLevelSetter se_chatAccessLevelConfLevel( se_chatAccessLevelConf, tAccessLevel_Owner );

// time between public chat requests, set to 0 to disable
REAL se_chatRequestTimeout = 60;
static tSettingItem< REAL > se_chatRequestTimeoutConf( "ACCESS_LEVEL_CHAT_TIMEOUT", se_chatRequestTimeout );

// access level a spectator has to have to be able to listen to /team messages
static tAccessLevel se_teamSpyAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_teamSpyAccessLevelConf( "ACCESS_LEVEL_SPY_TEAM", se_teamSpyAccessLevel );
static tAccessLevelSetter se_teamSpyAccessLevelConfLevel( se_teamSpyAccessLevelConf, tAccessLevel_Owner );

// access level a user has to have to be able to listen to /msg messages
static tAccessLevel se_msgSpyAccessLevel = tAccessLevel_Owner;
static tSettingItem< tAccessLevel > se_msgSpyAccessLevelConf( "ACCESS_LEVEL_SPY_MSG", se_msgSpyAccessLevel );
static tAccessLevelSetter se_msgSpyAccessLevelConfLevel( se_msgSpyAccessLevelConf, tAccessLevel_Owner );

// access level for freedom from spam protection
static tAccessLevel se_spamAccessLevel = tAccessLevel_Armatrator;
static tSettingItem< tAccessLevel > se_spamAccessLevelConf( "ACCESS_LEVEL_SPAM", se_spamAccessLevel );
static tAccessLevelSetter se_spamAccessLevelConfLevel( se_spamAccessLevelConf, tAccessLevel_Owner );

// access level a user has to have to get IP addresses in /players output
static tAccessLevel se_ipAccessLevel = tAccessLevel_Armatrator;
static tSettingItem< tAccessLevel > se_ipAccessLevelConf( "ACCESS_LEVEL_IPS", se_ipAccessLevel );
static tAccessLevelSetter se_ipAccessLevelConfLevel( se_ipAccessLevelConf, tAccessLevel_Owner );

static tAccessLevel se_nVerAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_nVerAccessLevelConf( "ACCESS_LEVEL_NVER", se_nVerAccessLevel );
static tAccessLevelSetter se_nVerAccessLevelConfLevel( se_nVerAccessLevelConf, tAccessLevel_Owner );

static tSettingItem<bool> se_silAll("SILENCE_DEFAULT",
                                    se_silenceDefault);

// checks whether a player is silenced, giving him appropriate warnings if he is
bool IsSilencedWithWarning( ePlayerNetID const * p )
{
    if ( p->GetAccessLevel() <= se_spamAccessLevel )
	return false;

    if ( !se_enableChat && ! p->IsLoggedIn() )
    {
        // everyone except the admins is silenced
        sn_ConsoleOut( tOutput( "$spam_protection_silenceall" ), p->Owner() );
        return true;
    }
    else if ( p->IsSilenced() )
    {
        if(se_silenceDefault) {
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

// returns true if the player is allowed to shout
static bool se_CheckAccessLevelShoutNoWarn( ePlayerNetID * p )
{
#ifdef KRAWALL_SERVER
    eShoutDefault shout = se_GetManagedTeam( p ) ? se_shoutPlayer : se_shoutSpectator;
    if( shout == eShoutDefault_ShoutAndOverride )
    {
        return true;
    }

    // check if the player has the right to shout
    return p->GetAccessLevel() <= se_shoutAccessLevel;
#else
    return true;
#endif
}

// returns true if the player is allowed to shout
static bool se_CheckAccessLevelShout( ePlayerNetID * p )
{
    if( !se_CheckAccessLevelShoutNoWarn( p ) )
    {
        sn_ConsoleOut( tOutput("$access_level_shout_denied" ), p->Owner() );
        return false;
    }
    else
    {
        return true;
    }
}

// /me chat commant
static void se_ChatMe( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    // check for global chat access right
    if ( !se_CheckAccessLevelShout( p ) )
    {
        return;
    }

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

    se_BroadcastChatLine( p, console );
    console << "\n";
    sn_ConsoleOut(console,0);

    tString str;
    str << p->GetUserName() << " /me " << msg;
    se_SaveToChatLog(str);
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

    if ( se_matches < 0 ) // allow quickleaving in warmup mode
    {
        if( p->Object() && p->Object()->Alive() )
        {
            p->Object()->Kill();
        }
        p->SetTeam(0);
        p->SetTeamWish(0);
        p->UpdateTeam();
        p->RequestSync();
    }
    else if ( leftTeam )
    {
        p->SetTeamWish(0);
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
}

static bool se_filterColorTeam=false;
tSettingItem< bool > se_coloredTeamConf( "FILTER_COLOR_TEAM", se_filterColorTeam );
static bool se_filterDarkColorTeam=false;
tSettingItem< bool > se_coloredDarkTeamConf( "FILTER_DARK_COLOR_TEAM", se_filterDarkColorTeam );

// regular chat; reaches all players
static void se_ChatShout( ePlayerNetID * p, tString const & say, eChatSpamTester & spam )
{
    if ( !se_CheckAccessLevelShout( p ) )
    {
        return;
    }

    // check for spam
    if ( spam.Block() )
    {
        return;
    }

    if ( ( say.Len() <= se_SpamMaxLen+2 || p->GetAccessLevel() <= se_spamAccessLevel ) && !IsSilencedWithWarning(p) )
    {
        se_BroadcastChat( p, say );
        se_DisplayChatLocally( p, say);
        
        tString s;
        s << p->GetUserName() << ' ' << say;
        se_SaveToChatLog(s);
    }
}

static void se_ChatShout( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    // parse string
    tString say;
    say.ReadLine( s );
    
    // delegate
    se_ChatShout( p, say, spam );
}

// /team chat commant: talk to your team
static void se_ChatTeam( ePlayerNetID * p, tString msg, eChatSpamTester & spam )
{
    eTeam *currentTeam = se_GetManagedTeam( p );

    // team messages are less spammy than public chat, take care of that.
    // we don't care too much about AI players (but don't remove them from the denominator because we're too lazy to count the total number of human players).
    spam.factor_ = ( currentTeam ? currentTeam->NumHumanPlayers() : 1 )/REAL( se_PlayerNetIDs.Len() );

    // silencing only affects spectators here
    if ( ( !currentTeam && IsSilencedWithWarning(p) ) || spam.Block() )
    {
        return;
    }

    // Apply filters if we don't already
    if ( se_filterColorTeam )
        msg = tColoredString::RemoveColors ( msg, false );
    else if ( se_filterDarkColorTeam )
        msg = tColoredString::RemoveColors ( msg, true );

    // Log message to server (and in previous revisions, the sender)
    tColoredString messageForServerAndSender = se_BuildChatString(currentTeam, p, msg);
    messageForServerAndSender << "\n";

    if (currentTeam != NULL) // If a player has just joined the game, he is not yet on a team. Sending a /team message will crash the server
    {
        sn_ConsoleOut(messageForServerAndSender, 0);

        // Send message to team-mates
        int numTeamPlayers = currentTeam->NumPlayers();
        for (int teamPlayerIndex = 0; teamPlayerIndex < numTeamPlayers; teamPlayerIndex++)
        {
            se_SendTeamMessage(currentTeam, p, currentTeam->Player(teamPlayerIndex), msg);
        }

        // check for other players who are authorized to hear the message
        for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
        {
            ePlayerNetID * admin = se_PlayerNetIDs(i);

            if (
                // two cases:
                   (
                    // You are a speccing admin, and you aren't invited to anything:
                    se_GetManagedTeam( admin ) == 0 &&
                    admin->GetAccessLevel() <=  se_teamSpyAccessLevel
                    ) ||
                   (
                    // You are invited and spectating
                    admin->CurrentTeam() == NULL &&
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

        // check for other spectators
        for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
        {
            ePlayerNetID * spectator = se_PlayerNetIDs(i);

            if ( se_GetManagedTeam( spectator ) == 0 )
            {
                se_SendTeamMessage(currentTeam, p, spectator, msg);
            }
        }
    }
}

// /team chat commant: talk to your team
static void se_ChatTeam( ePlayerNetID * p, std::istream & s, eChatSpamTester & spam )
{
    tString msg;
    msg.ReadLine( s );

    se_ChatTeam( p, msg, spam );
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

        if ( p->CurrentTeam() == receiver->CurrentTeam() || !IsSilencedWithWarning(p) )
        {
            // log locally
            sn_ConsoleOut(toServer,0);

            // log to sender's console
            sn_ConsoleOut(toServer, p->Owner());

            // send to receiver
            if ( p->Owner() != receiver->Owner() )
                se_SendPrivateMessage( p, receiver, receiver, msg_core );

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

// prints an indented team member with position marker
static void se_SendTeamMember( ePlayerNetID const * player, int indent, std::ostream & tos, int index, int width )
{
    tos << '#' << std::setw( width ) << index+1 << ' ';

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
    tos << team->GetColoredName();
    if ( team->IsLocked() )
    {
        tos << " " << tOutput( "$invite_team_locked_list" );
    }
    if ( se_matches < 0 )
    {
        if ( team->IsReady() )
            tos << " " << tOutput( "$team_list_ready" );
        else
            tos << " " << tOutput( "$team_list_not_ready" );
    }
    tos << ":\n";

    // send team members
    int teamMembers = team->NumPlayers();
    int width = teamMembers >= 10 ? 2 : 1;

    int indent = 0;
    // print left wing, the odd team members
    for( int i = (teamMembers/2)*2-1; i>=0; i -= 2 )
    {
        se_SendTeamMember( team->Player(i), indent, tos, i, width );
        indent += 2;
    }
    // print right wing, the even team members
    for( int i = 0; i < teamMembers; i += 2 )
    {
        se_SendTeamMember( team->Player(i), indent, tos, i, width );
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

static void se_ListPlayers( ePlayerNetID * receiver, std::istream &s, tString command )
{
    tString search;
    bool doSearch = false;

    search.ReadLine( s );
    tToLower( search );

    if ( search.Len() > 1 )
    doSearch = true;

    bool hidden = false;

    int count = 0;

    for ( int i2 = se_PlayerNetIDs.Len()-1; i2>=0; --i2 )
    {
        ePlayerNetID* p2 = se_PlayerNetIDs(i2);
        tColoredString tos;
        if ( doSearch )
            tos << "  ";
        tos << p2->Owner();
        tos << ": ";
        tos << p2->GetColoredName()
            << tColoredString::ColorString( -1, -1, -1)
            << " ";
        if ( se_matches < 0 && p2->IsHuman() && p2->CurrentTeam() )
        {
            tos << "( ";
            if ( p2->ready )
            {
                tos << tOutput("$player_list_ready")
                    << tColoredString::ColorString( -1, -1, -1);
            }
            else
            {
                tos << tOutput("$player_list_not_ready")
                    << tColoredString::ColorString( -1, -1, -1);
            }
            tos << " )";
        }
        if ( p2->GetAccessLevel() < tAccessLevel_Default && !se_Hide( p2, receiver ) )
        {
#ifdef KRAWALL_SERVER
            hidden = p2->GetAccessLevel() <= se_hideAccessLevelOf && p2->StealthMode();
#else
            hidden = false;
#endif
            tos << "( ";
            if ( hidden )
                tos << se_hiddenPlayerPrefix;
#ifdef KRAWALL_SERVER
            tos << p2->GetFilteredAuthenticatedName();
            if ( hidden )
                tos << tColoredString::ColorString( -1 ,-1 ,-1 )
                    << ", "
                    << se_hiddenPlayerPrefix;
            else
                tos << ", ";
#endif
            tos << tCurrentAccessLevel::GetName( p2->GetAccessLevel() );
            if ( hidden )
                tos << tColoredString::ColorString( -1 ,-1 ,-1 );
                tos << " )";
        }
        else
        {
            tos << "( )";
        }
        if ( ( p2->Owner() != 0 && tCurrentAccessLevel::GetAccessLevel() <= se_ipAccessLevel ) || ( p2->Owner() != 0 && p2->Owner() == receiver->Owner() ) )
        {
            tString IP = p2->GetMachine().GetIP();
            if ( IP.Len() > 1 )
            {
                tos << ", IP = " << IP;
            }
        }
        if ( ( p2->Owner() != 0 && tCurrentAccessLevel::GetAccessLevel() <= se_nVerAccessLevel ) || ( p2->Owner() != 0 && p2->Owner() == receiver->Owner() ) )
        {
            tos << ", " << sn_GetClientVersionString( sn_Connections[ p2->Owner() ].version.Max() ) << " (ID: " << sn_Connections[ p2->Owner() ].version.Max() << ")";
        }

        tos << "\n";

        if ( !doSearch )
        {
            sn_ConsoleOut( tos, receiver->Owner() );
            count++;
        }
        else
        {
            tString tosLowercase( tColoredString::RemoveColors(tos) );
            tToLower( tosLowercase );
            // looks quite like a hack, but i guess it's faster( esp. for me :) ) than checking on each parameter individually
            if ( tosLowercase.StrPos( search ) != -1 )
            {
                count++;
                if ( count == 1 )
                {
                    sn_ConsoleOut( tOutput( "$player_list_search", command, search ) , receiver->Owner() );
                }
                sn_ConsoleOut( tos, receiver->Owner() );
            }
        }
    }

    if ( doSearch && !count )
    {
        sn_ConsoleOut( tOutput( "$player_list_search_no_results", command, search ) , receiver->Owner() );
    }
    else if ( doSearch )
    {
        sn_ConsoleOut( tOutput( "$player_list_search_end", command, count ) , receiver->Owner() );
    }
    else
    {
        sn_ConsoleOut( tOutput( "$player_list_end", command, count ) , receiver->Owner() );
    }
}

static void players_conf(std::istream &s)
{
    se_ListPlayers( 0, s, tString("PLAYERS") );
}

static tConfItemFunc players("PLAYERS",&players_conf);
static tAccessLevelSetter players_AccessLevel( players, tAccessLevel_Owner );


// /players gives a player list
static void se_ChatPlayers( ePlayerNetID * p, std::istream &s, tString command )
{
    se_ListPlayers( p, s, command );
}


// team shuffling: reorders team formation
static void se_ChatShuffle( ePlayerNetID * p, std::istream & s )
{
    // team position shuffling. Allow players to change their team setup.
    // syntax:
    // /teamshuffle: shuffles you all the way to the outside.
    // /teamshuffle <pos>: shuffles you to position pos
    // /teamshuffle +/-<dist>: shuffles you dist to the outside/inside
    // con << msgRest << "\n";
    int IDNow = p->TeamListID();
    int len = eTeam::maxPlayers;
    if (!p->CurrentTeam())
    {
        // players start on the outside by default
        if( IDNow < 0 )
        {
            IDNow = len-1;
        }
    }
    else
    {
        len = p->CurrentTeam()->NumPlayers();
    }

    // but read the target position as additional parameter
    int IDWish = len-1; // default to shuffle to the outside

    // peek at the first nonwhite character
    std::ws( s );
    char first = s.get();
    if ( !s.eof() && !s.fail() )
    {
        s.unget();

        int shuffle = 0;
        s >> shuffle;

        if ( s.fail() )
        {
            sn_ConsoleOut( tOutput("$player_shuffle_error"), p->Owner() );
            return;
        }

        if ( first == '+' || first == '-' )
        {
            IDWish = IDNow;
            IDWish += shuffle;
        }
        else
        {
            IDWish = shuffle-1;
        }
    }

    if (IDWish < 0)
        IDWish = 0;
    if (IDWish >= len)
        IDWish = len-1;

    if( !p->CurrentTeam() || IDWish < IDNow )
    {
#ifndef KRAWALL_SERVER
        if ( !se_allowShuffleUp )
        {
            sn_ConsoleOut(tOutput("$player_noshuffleup"), p->Owner());
            return;
        }
#else
        if ( p->GetAccessLevel() > se_shuffleUpAccessLevel )
        {
            sn_ConsoleOut(tOutput("$access_level_shuffle_up_denied", tCurrentAccessLevel::GetName(se_shuffleUpAccessLevel), tCurrentAccessLevel::GetName(p->GetAccessLevel())), p->Owner());
            return;
        }
#endif
    }

    if( IDNow == IDWish )
    {
        sn_ConsoleOut(tOutput("$player_noshuffle"), p->Owner());
        return;
    }

    if( p->CurrentTeam() )
    {
        // really shuffle
        p->CurrentTeam()->Shuffle( IDNow, IDWish );
        se_ListTeam( p, p->CurrentTeam() );
    }
    else
    {
        // just store the shuffle wish for later
        p->SetShuffleWish( IDWish );
    }
}

void ePlayerNetID::SetShuffleWish( int pos )
{
    tASSERT( !CurrentTeam() );

    if ( GetShuffleSpam().ShouldAnnounce() )
    {
        sn_ConsoleOut( GetShuffleSpam().ShuffleMessage( this, pos+1 ) );
    }

    teamListID = pos;
}

void se_Ready( ePlayerNetID * p )
{
    if( !p->CurrentTeam() )
    {
        sn_ConsoleOut(tOutput("$player_ready_noteam"), p->Owner());
        return;
    }
    if( se_matches >= 0 )
    {
        sn_ConsoleOut(tOutput("$player_ready_onlywarmup"), p->Owner());
        return;
    }

    p->ready = !p->ready;

    p->RequestSync();

    if( p->ready )
    {
        sn_ConsoleOut(tOutput("$player_ready", p->GetColoredName()), p->Owner());
    }
    else
    {
        sn_ConsoleOut(tOutput("$player_ready_not", p->GetColoredName()), p->Owner());
    }
}
class eHelpTopic {
    tString m_shortdesc, m_text;

    // singleton accessor
    static std::map<tString, eHelpTopic> & GetHelpTopics();
public:
    eHelpTopic() {}
    eHelpTopic(tString const &shortdesc, tString const &text) : m_shortdesc(shortdesc), m_text(text) {
    }

    void write(tColoredString &s) const {
        s << tColoredString::ColorString(.5,.5,1.) << tOutput((const char *)m_shortdesc) << ":\n" << tOutput((const char *)m_text) << '\n';
    }

    static void addHelpTopic(std::istream &s) {
        tString name, shortdesc, text;
        s >> name >> shortdesc;
        if(s.fail()) {
            if(tConfItemBase::printErrors) {
                con << tOutput("$add_help_topic_usage");
            }
            return;
        }
        s >> text;
        GetHelpTopics()[name] = eHelpTopic(shortdesc, text);
        if(tConfItemBase::printChange) {
            con << tOutput("$add_help_topic_success", name);
        }
    }

    static void removeHelpTopic(std::istream &s) {
        tString name;
        s >> name;
        if(GetHelpTopics().erase(name)) {
            if(tConfItemBase::printChange) {
                con << tOutput("$remove_help_topic_success", name);
            }
        } else {
            if(tConfItemBase::printErrors) {
                con << tOutput("$remove_help_topic_notfound", name);
            }
        }
    }

private:

    static void listTopics(tColoredString &s, tString const &base, std::map<tString, eHelpTopic>::const_iterator begin, std::map<tString, eHelpTopic>::const_iterator const &end) {
        bool printed_start = false;
        for(;begin != end; ++begin) {
            if(!begin->first.StartsWith(base)) {
                continue;
            }
            // would be easier with std::string...
            if(begin->first.SubStr(base.Len() - 1).StrPos("_") >= 0) {
                // don't print subcommands
                continue;
            }
            if(!printed_start) {
                printed_start = true;
                s << tOutput("$help_topics_list_start");
            }
            s << tColoredString::ColorString(.5,.5,1.) << begin->first << tColoredString::ColorString(1.,1.,.5) << ": " << tOutput((const char *)begin->second.m_shortdesc) << "\n";
        }
    }

public:

    static void listTopics(tColoredString &s, tString const &base=tString()) {
        listTopics(s, base, GetHelpTopics().begin(), GetHelpTopics().end());
    }

    static void printTopic(tColoredString &s, tString const &name) {
        std::map<tString, eHelpTopic>::const_iterator iter = GetHelpTopics().find(name);
        if(iter != GetHelpTopics().end()) {
            iter->second.write(s);
            listTopics(s, name + "_");
        } else {
            s << tOutput("$help_topic_not_found", name);
        }
    }
};

static void se_makeDefaultHelpTopic(  std::map<tString, eHelpTopic> & helpTopics, char const *topic) {
    tString topicStr(topic);
    tString helpTopic(tString("$help_") + topicStr);
    helpTopics[topicStr] = eHelpTopic(helpTopic + "_shortdesc", helpTopic + "_text");
}

// singleton accessor implementation
static std::map<tString, eHelpTopic> & FillHelpTopics()
{
    static std::map<tString, eHelpTopic> helpTopics;

    se_makeDefaultHelpTopic(helpTopics, "commands");
    se_makeDefaultHelpTopic(helpTopics, "commands_chat");
    se_makeDefaultHelpTopic(helpTopics, "commands_team");
#ifdef KRAWALL_SERVER
    se_makeDefaultHelpTopic(helpTopics, "commands_auth");
    se_makeDefaultHelpTopic(helpTopics, "commands_auth_levels");
    se_makeDefaultHelpTopic(helpTopics, "commands_tourney");
#else
    se_makeDefaultHelpTopic(helpTopics, "commands_ra");
#endif
    se_makeDefaultHelpTopic(helpTopics, "commands_misc");
    se_makeDefaultHelpTopic(helpTopics, "commands_pp");

    se_makeDefaultHelpTopic(helpTopics, "version");

    return helpTopics;
}

// singleton accessor implementation
std::map<tString, eHelpTopic> & eHelpTopic::GetHelpTopics()
{
    static std::map<tString, eHelpTopic> & helpTopics = FillHelpTopics();
    return helpTopics;
}

static tConfItemFunc add_help_topic_conf("ADD_HELP_TOPIC",&eHelpTopic::addHelpTopic);
static tConfItemFunc remove_help_topic_conf("REMOVE_HELP_TOPIC",&eHelpTopic::removeHelpTopic);
static tString se_helpIntroductoryBlurb;
static tConfItemLine se_helpIntroductoryBlurbConf("HELP_INTRODUCTORY_BLURB",se_helpIntroductoryBlurb);

// Sty compatibility
static tConfItemLine se_helpStyCompat("HELP_MESSAGE",se_helpIntroductoryBlurb);

static void se_Help( ePlayerNetID * sender, ePlayerNetID * receiver, std::istream & s ) {
    std::ws(s);
    tColoredString reply;
    if(s.eof() || s.fail()) {
        if(se_helpIntroductoryBlurb.Len() > 1) {
            reply << se_helpIntroductoryBlurb << "\n\n";
        }
        eHelpTopic::listTopics(reply);
    } else {
        tString name;
        s >> name;
        eHelpTopic::printTopic(reply, name);
    }
    if ( sender == receiver )
    {
        // just send a console message, the player asked for help himself
        sn_ConsoleOut(reply, receiver->Owner());
    }
    else
    {
        // send help disguised as a chat message (with disabled spam limit)
        int spamMaxLenBack = se_SpamMaxLen;
        se_SpamMaxLen = 0;
        se_SendChatLine( sender, reply, receiver->Owner() );
        se_SpamMaxLen = spamMaxLenBack;
    }
}

static tAccessLevel se_rtfmAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_rtfmAccessLevelConf( "ACCESS_LEVEL_RTFM", se_rtfmAccessLevel );
static tAccessLevelSetter se_rtfmAccessLevelConfLevel( se_rtfmAccessLevelConf, tAccessLevel_Owner );

#ifdef DEDICATED
static void se_Rtfm( tString const &command, ePlayerNetID *p, std::istream &s, eChatSpamTester &spam ) {
#ifdef KRAWALL_SERVER
    if ( p->GetAccessLevel() > se_rtfmAccessLevel ) {
        sn_ConsoleOut(tOutput("$access_level_rtfm_denied", tCurrentAccessLevel::GetName(se_rtfmAccessLevel), tCurrentAccessLevel::GetName(p->GetAccessLevel())), p->Owner());
        return;
    }
#else
    if (!p->IsLoggedIn()) {
        sn_ConsoleOut(tOutput("$rtfm_denied"));
        return;
    }
#endif
    if(IsSilencedWithWarning(p)) {
        return;
    }
    if(spam.Block()) {
        return;
    }
    ePlayerNetID *newbie = se_FindPlayerInChatCommand(p, command, s);
    if(newbie) {
        // somewhat hacky, but what the hack...
        tString str;
        str.ReadLine(s);
        std::istringstream s1(&str(0)), s2(&str(0));
        tColoredString name;
        name << *p << tColoredString::ColorString(1,1,1);
        tColoredString newbie_name;
        newbie_name << *newbie << tColoredString::ColorString(1,1,1);

        tString announcement( tOutput("$rtfm_announcement", str, name, newbie_name) );
        se_BroadcastChatLine( p, announcement );
        con << announcement;

        se_Help(p, newbie, s1);

        // avoid sending you the same doc twice if you tell yourself to rtfm :)
        if ( newbie != p )
        {
            se_Help(p, p, s2);
        }
    }
}
#endif

#if defined(DEDICATED) && defined(KRAWALL_SERVER)
void se_ListAdmins( ePlayerNetID *, std::istream &s, tString command );
#endif

void se_ChatHandlerServer( unsigned short id, tColoredString const & say, nMessageBase const & m )
{
    nTimeRolling currentTime = tSysTimeFloat();

    tJUST_CONTROLLED_PTR< ePlayerNetID > p=dynamic_cast<ePlayerNetID *>(nNetObject::ObjectDangerous(id));

    // register player activity
    if ( p )
        p->lastActivity_ = currentTime;

    if(sn_GetNetState()==nSERVER){
        if (p)
        {
            // for the duration of this function, set the access level to the level of the user.
            tCurrentAccessLevel levelOverride( p->GetAccessLevel(), true );

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
                    handle_command_intercept(p, command, s, say);
                    return;
                }
                else
#endif
                    if (command == "/me") {
                        spam.lastSaidType_ = eChatMessageType_Me;
                        spam.say_ = spam.say_.SubStr(4); // cut /me prefix
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
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_ChatTeamLeave( p );
                        return;
                    }
                    else if (command == "/teamleave" || command=="/spectate") {
                        p->SetTeamWish(NULL);
                        return;
                    }
                    else if (command == "/teamshuffle" || command == "/shuffle") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_ChatShuffle( p, s );
                        return;
                    }
                    else if (command == "/team")
                    {
                        spam.lastSaidType_ = eChatMessageType_Team;
                        se_ChatTeam( p, s, spam );
                        return;
                    }
                    else if (command == "/shout")
                    {
                        spam.lastSaidType_ = eChatMessageType_Public;
                        spam.say_ = spam.say_.SubStr(7); // cut /shout prefix
                        se_ChatShout( p, s, spam );
                        return;
                    }
                    else if (command == "/msg" ) {
                        spam.lastSaidType_ = eChatMessageType_Private;
                        se_ChatMsg( p, s, spam );
                        return;
                    }
                    else if (command == "/players" || command == "/listplayers") {
                        spam.lastSaidType_= eChatMessageType_Command;
                        se_ChatPlayers( p, s, command );
                        return;
                    }
#if defined(DEDICATED) && defined(KRAWALL_SERVER)
                    else if (command == "/admins" || command == "/listadmins") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_ListAdmins( p, s, command );
                        return;
                    }
#endif
                    else if (command == "/vote" || command == "/callvote") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        eVoter::HandleChat( p, s );
                        return;
                    }
                    else if (command == "/teams") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_ChatTeams( p );
                        return;
                    }
                    else if (command == "/myteam") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        eTeam *currentTeam = se_GetManagedTeam( p );
                        if( currentTeam )
                        {
                            se_ListTeam( p, currentTeam );
                        }
                        return;
                    }
                    else if (command == "/ready")
                    {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_Ready( p );
                        return;
                    }
                    else if (command == "/help") {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_Help( p, p, s );
                        return;
                    }
#ifdef DEDICATED
                    else  if ( command == "/rtfm" || command == "/teach" )
                    {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        se_Rtfm( command, p, s, spam );
                        return;
                    }
                    else {
                        spam.lastSaidType_ = eChatMessageType_Command;
                        handle_chat_admin_commands( p, command, say, s, spam );
                        return;
                    }
#endif
            }

            // well, that leaves only regular, boring chat.
            eShoutDefault shout = se_GetManagedTeam( p ) ? se_shoutPlayer : se_shoutSpectator;
            if( shout != eShoutDefault_Team && se_CheckAccessLevelShoutNoWarn( p ) )
            {
                // if it's the default and the player is allowed to, shout it out
                se_ChatShout( p, say, spam );
            }
            else
            {
                // otherwise, fall back to team chat.
                se_ChatTeam( p, say, spam );
            }
        }
    }
    else
    {
        se_DisplayChatLocally( p, say );
    }
}

void se_ChatHandlerServer( Engine::Chat const & message, nSenderInfo const & sender )
{
    se_ChatHandlerServer( message.player_id(), tColoredString( message.chat_line().c_str() ), sender.envelope );
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

#ifndef DEDICATED
    // check for direct console commands
    if( s_orig.StartsWith("/console") )
    {
        // direct commands are executed at owner level
        tCurrentAccessLevel level( tAccessLevel_Owner, true );

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
    }
    else
#endif
    {
        switch (sn_GetNetState())
        {
        case nCLIENT:
        {
            se_ChatMessageForServer( this, s )->BroadCast();
            break;
        }
        case nSERVER:
        {
            se_BroadcastChat( this, s );

            // falling through on purpose
            // break;
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
#ifdef DEBUG
                std::cout << "console selected\n";
#endif
            } else
                se_DisplayChatLocally( this, s );

            break;
        }
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

static char const * const se_simplifiedCommands[] = {
    "/msg ",
    "/invite ",
    "/uninvite ",
    "/teach ",
    "/rtfm ",
    "/op ",
    "/deop ",
    "/promote ",
    "/demote ",
    "/vote kick ",
    "/vote suspend "
};

//! Completer with the addition that it adds a : to a fully completed name if it's at the beginning of the line
class eAutoCompleterChat : public uAutoCompleter{
public:
    //! Constructor
    //! @param words the possible completions
    eAutoCompleterChat(std::deque<tString> &words):uAutoCompleter(words) {};
    int DoFullCompletion(tString &string, int pos, int len, tString &match) {
        tString actualString;
        if(pos - len == 0 || ( pos - len == 6 && string.StartsWith("/team ") ) ) {
            actualString = match + ": ";
        } else if(string.StartsWith("/admin ") || string.StartsWith("/vote shuffle ")) {
            actualString = Simplify(match) + " ";
        } else {
            int i;
            for(i = sizeof(se_simplifiedCommands)/sizeof(se_simplifiedCommands[0]) - 1; i >= 0; --i) {
                if(pos - len == (int)strlen(se_simplifiedCommands[i]) && string.StartsWith(se_simplifiedCommands[i])) {
                    actualString = Quote(Simplify(match)) + " ";
                    break;
                }
            }
            if(i < 0) {
                actualString = match + " ";
            }
        }
        return DoCompletion(string, pos, len, actualString);
    }
    tString Quote(tString const &str) {
        tString ret = str;
        if(ret[0] == '\'' || ret[0] == '"') {
            char otherQuote = ret[0] == '"' ? '\'' : '"';
            char const *backspace = "\\";
            tString::size_type pos = -1;
            while((pos = ret.find(otherQuote, pos+2)) != tString::npos) {
                ret.insert(pos, backspace);
            }
            return otherQuote + ret + otherQuote;
        }
        return ret;
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
        {
            return su_HandleEvent(e, true);
        }
        else if (e.type==SDL_KEYDOWN &&
                 e.key.keysym.sym == SDLK_ESCAPE)
        {
            // escape needs to be handled by the surrounding menu, otherwise it
            // probably brings up the ingame menu via global bind.
            return false;
        }
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
                        *content = content->SubStr( 0, realCursorPos ) + insertion.insertion_ + content->SubStr( realCursorPos );
                        realCursorPos += insertion.insertion_.Len()-1;
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

uActionPlayer se_toggleSpectator("TOGGLE_SPECTATOR", -7);

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
        if(x>0 && sn_GetNetState() != nSTANDALONE ) {
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

// veto function for tooltips that require a controllable game object
bool ePlayer::VetoActiveTooltip(int player)
{
    // check if the player exists and controls a living object
    if( player == 0 )
    {
        return true;
    }
    ePlayer * p = PlayerConfig(player-1);
    if ( !p )
    {
        return true;
    }
    ePlayerNetID * pn = p->netPlayer;
    if ( !pn )
    {
        return true;
    }
    eNetGameObject *o = pn->Object();
    if (!o || !o->Alive())
    {
        return true;
    }

    return false;
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
        ePlayer::se_instantChatAction[i]=tNEW(uActionPlayer) (id, desc, help, 100);
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

uActionPlayer ePlayer::s_chat("CHAT",-8);

// only display chat in multiplayer games
static bool se_ChatTooltipVeto(int)
{
    return sn_GetNetState() == nSTANDALONE;
}

uActionTooltip ePlayer::s_chatTooltip(uActionTooltip::Level_Expert, ePlayer::s_chat, 1, &se_ChatTooltipVeto);
uActionTooltip s_toggleSpectatorTooltip(uActionTooltip::Level_Expert, se_toggleSpectator, 1, &se_ChatTooltipVeto);

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

bool ePlayerNetID::Scramble = false;
std::vector<ePlayerNetID*> ePlayerNetID::ScramblePlayerIDs;

class eMachineDecoratorSpam: public nMachineDecorator
{
public:
    nSpamProtection chatSpam;
    eShuffleSpamTester shuffleSpam;
    eChatLastSaid lastSaid;

    eMachineDecoratorSpam( nMachine & m ): nMachineDecorator( m ), chatSpam( se_chatSpamSettings ){}

    virtual void OnDestroy()
    {
        delete this;
    }
};

static eMachineDecoratorSpam & se_GetSpam( ePlayerNetID & p )
{
    nMachine & machine = p.GetMachine();
    eMachineDecoratorSpam * spam = machine.GetDecorator< eMachineDecoratorSpam >();
    if( !spam )
    {
        spam = tNEW(eMachineDecoratorSpam)( machine );
    }
    return *spam;
}

nSpamProtection & ePlayerNetID::GetChatSpam()
{
    return se_GetSpam( *this ).chatSpam;
}

eChatLastSaid & ePlayerNetID::GetLastSaid()
{
    return se_GetSpam( *this ).lastSaid;
}

eShuffleSpamTester & ePlayerNetID::GetShuffleSpam()
{
    return se_GetSpam( *this ).shuffleSpam;
}

ePlayerNetID::ePlayerNetID(int p):nNetObject(),listID(-1), teamListID(-1), timeCreated_( tSysTimeFloat() ), allowTeamChange_(false), registeredMachine_(0), pID(p)
{
    // default access level
    lastAccessLevel = tAccessLevel_Default;

    favoriteNumberOfPlayersPerTeam = 1;

    nameTeamAfterMe = false;

    color.r_ = color.g_ = color.b_ = 15;

    greeted             = false;
    chatting_           = false;
    spectating_         = false;
    stealth_            = false;
    chatFlags_          = 0;
    disconnected        = false;
    suspended_          = 0;

    loginWanted = false;


    if (p>=0){
        ePlayer *P = ePlayer::PlayerConfig(p);
        if (P){
            SetName( P->Name() );
            teamname = P->Teamname();
            color.r_=   P->rgb[0];
            color.g_=   P->rgb[1];
            color.b_=   P->rgb[2];

            sg_ClampPingCharity();
            pingCharity=::pingCharity;

            loginWanted = P->autoLogin;
        }
    }

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

// should unworthy spectators be kicked if the server is full?
static bool se_kickUnworthy=false;
tSettingItem< bool > se_kickUnworthyConf( "KEEP_PLAYER_SLOT", se_kickUnworthy );

// access level needed to grant immunity against all forms of idle kicks
static tAccessLevel se_autokickImmunity = tAccessLevel_TeamLeader;
static tSettingItem< tAccessLevel > se_autokickImmunityConf( "ACCESS_LEVEL_AUTOKICK_IMMUNITY", se_autokickImmunity );

// compares the worth of two players. Returns 1 if the first is better, -1 if the second is better, 0 if they are equal.
static int se_comparePlayerWorth( ePlayerNetID * a, ePlayerNetID * b, int nullPointerSign=1 )
{
    // check for NULL pointers
    if( !a )
    {
        if ( !b )
        {
            return 0;
        }
        else
        {
            return -nullPointerSign;
        }
    }
    else
    {
        if ( !b )
        {
            return nullPointerSign;
        }
    }

    // potential players > spectators
    if ( !se_GetManagedTeam( a ) )
    {
        if( se_GetManagedTeam( b ) )
        {
            return -1;
        }
    }
    else
    {
        if( !se_GetManagedTeam( b ) )
        {
            return 1;
        }
    }

    // actual players > potential players
    if ( !a->CurrentTeam() )
    {
        if( b->CurrentTeam() )
        {
            return -1;
        }
    }
    else
    {
        if( !b->CurrentTeam() )
        {
            return 1;
        }
    }

    // access level counts
#ifdef KRAWALL_SERVER
    if( a->GetAccessLevel() < b->GetAccessLevel() )
    {
        return 1;
    }
    else if( a->GetAccessLevel() > b->GetAccessLevel() )
    {
        return -1;
    }
#endif

    // players with login process > players without
    if( !nAuthentication::LoginInProcess( a ) )
    {
        if( nAuthentication::LoginInProcess( b ) )
        {
            return -1;
        }
    }
    else
    {
        if( !nAuthentication::LoginInProcess( b ) )
        {
            return 1;
        }
    }

    // players online for shorter times are more worthy
    if( a->GetTimeCreated() < b->GetTimeCreated() )
    {
        return 1;
    }
    else if( a->GetTimeCreated() > b->GetTimeCreated() )
    {
        return 1;
    }

    return 0;
}

// the user that last logged in; spare him.
static int se_kickUnworthySpare=-1;

// clear out one unworthy spectator
static void se_KickUnworthy()
{
    int userToSpare=se_kickUnworthySpare;

    static REAL banForMinutes = 0;
    static double roundBegin = -1;
    double lastRoundBegin = roundBegin;

    // each player joining increases the ban, rounds passing decrease it
    if( userToSpare >= 0 )
    {
        banForMinutes += .05f;
    }
    else
    {
        banForMinutes *= 0.875;
        roundBegin = tSysTimeFloat();
    }

    // nothing to do?
    if( !se_kickUnworthy || sn_NumUsers() < sn_MaxUsers() )
    {
        return;
    }

    // find the most worthy player for each client
    ePlayerNetID * mostWorthy[MAXCLIENTS+2];
    for( int i = MAXCLIENTS+1; i >= 0; --i )
    {
        mostWorthy[i] = NULL;
    }
    for( int i = se_PlayerNetIDs.Len() - 1; i >=0; --i )
    {
        ePlayerNetID * p = se_PlayerNetIDs(i);
        ePlayerNetID * & rival = mostWorthy[p->Owner()];
        if( se_comparePlayerWorth( p, rival ) > 0 )
        {
            rival = p;
        }
    }

    // find the most unworthy client
    int mostUnworthyUser = 0;

    for( int i = MAXCLIENTS; i >= 1; --i )
    {
        // NULL players are actually more worthy here now
        ePlayerNetID * player = mostWorthy[i];
        if( ( i != userToSpare && sn_Connections[i].socket ) && 
            // don't kick players with immune access level
            !( player && se_autokickImmunity >= player->GetAccessLevel() ) && 
            // give players one round to sign in
            !( player && nAuthentication::LoginInProcess(player) && player->GetTimeCreated() >= lastRoundBegin ) && 
            ( !mostUnworthyUser || se_comparePlayerWorth( mostWorthy[mostUnworthyUser], mostWorthy[i], -1 ) > 0 ) )
        {   
            mostUnworthyUser = i;
        }
    }

    // take care not to auto-kick someone with a high access level
    if( mostUnworthyUser )
    {
        tString name;
        ePlayerNetID * player = mostWorthy[mostUnworthyUser];
        if( player )
        {
            mostWorthy[mostUnworthyUser]->PrintName( name );
        }
        else
        {
            name = "?";
        }
        
        con << tOutput( "$network_kill_unworthy_log", name, banForMinutes );
        if( banForMinutes > 1 )
        {
            nMachine::GetMachine( mostUnworthyUser ).Ban( banForMinutes*60, 
                                                          tString( 
                                                              tOutput( "$network_kill_unworthy_banreason" ) ) );
            sn_DisconnectUser( mostUnworthyUser, tOutput( "$network_kill_unworthy_ban", banForMinutes ) );
        }
        else
        {   
            sn_DisconnectUser( mostUnworthyUser, tOutput( "$network_kill_unworthy" ) );
        }
    }
}

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

static void se_PlayerLoginLogoutCallback()
{
    // clear the legacy spectator when a client enters or leaves
    se_ClearLegacySpectator( nCallbackLoginLogout::User() );

    // always keep one slot free
    if( nCallbackLoginLogout::Login() )
    { 
        se_kickUnworthySpare=nCallbackLoginLogout::User();
        st_ToDo( se_KickUnworthy );
    }
}
static nCallbackLoginLogout se_playerLoginLogoutCallback( se_PlayerLoginLogoutCallback );



void ePlayerNetID::MyInitAfterCreation()
{
    this->CreateVoter();

    this->silenced_ = se_silenceDefault;
    this->renameAllowed_ = true;

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

        // get suspension count
        if ( GetVoter() )
        {
            suspended_ = GetVoter()->suspended_;
            silenced_ = GetVoter()->silenced_;
        }
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
    // se_PlayerNetIDs.Remove(this,listID);
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
}

static void se_PlayerRemovedHandler( Engine::PlayerRemoved const & removed, nSenderInfo const & sender )
{
    // and the ID of the player that was removed
    unsigned short id = removed.player_id();
    ePlayerNetID* p = dynamic_cast< ePlayerNetID* >( nNetObject::ObjectDangerous( id ) );
    if ( p && sn_GetNetState() != nSERVER )
    {
        p->RemoveFromGame();
    }
}

static nProtoBufDescriptor< Engine::PlayerRemoved > se_playerRemovedDescriptor(202,&se_PlayerRemovedHandler);

static eLadderLogWriter se_playerLeftWriter("PLAYER_LEFT", true);

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

    bool logLeave = false;

    if ( sn_GetNetState() != nCLIENT )
    {
        nameFromClient_ = nameFromServer_;

        Engine::PlayerRemoved & removed = se_playerRemovedDescriptor.Broadcast();
        removed.set_player_id( this->ID() );

        if ( listID >= 0 ){
            if ( ( IsSpectating() || IsSuspended() || !se_assignTeamAutomatically ) && ( se_assignTeamAutomatically || se_specSpam ) && CurrentTeam() == NULL )
            {
                // get colored player name
                tColoredString playerName;
                playerName << *this << tColoredString::ColorString(1,.5,.5);

                // announce a generic leave
                sn_ConsoleOut( tOutput( "$player_left_spectator", playerName ) );
            }

            if ( IsHuman() && sn_GetNetState() == nSERVER && NULL != sn_Connections[Owner()].socket )
            {
                logLeave = true;
                tString notificationMessage(userName_);
                notificationMessage << " left the grid";
                se_sendEventNotification(tString("Player left"), notificationMessage);
            }
        }
    }

    se_PlayerNetIDs.Remove(this, listID);

    if ( sn_GetNetState() == nCLIENT )
    {
        SetTeamWish( NULL );
    }
    SetTeam( NULL );
    UpdateTeam();
    ControlObject( NULL );

    if( logLeave )
    {
        se_playerLeftWriter << userName_ << nMachine::GetMachine(Owner()).GetIP();
        se_playerLeftWriter.write();
    }
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

tString se_GetAuthorityFromGid ( const tString gid )
{
    int pos;
    if ( ( pos = gid.StrPos( "@" ) ) != -1 )
    {
        return gid.SubStr( pos +1 );
    }
    return gid;
}

// changes various user aspects
template< class P > class eUserConfig: public tConfItemBase
{
public:
    typedef typename std::map< tString, P > Properties;
    P Get(tString const & name ) const
    {
        typename Properties::const_iterator iter = properties.find( name );
        if ( iter != properties.end() )
        {
            return (*iter).second;
        }

        return GetDefault();
    }
    Properties GetMap () const
    {
        return properties;
    }
protected:
   eUserConfig( char const * name )
    : tConfItemBase( name )
    {
        requiredLevel = tAccessLevel_Owner;
    }

    virtual P ReadRawVal(tString const & name, std::istream &s) const = 0;
    virtual P GetDefault() const = 0;
    virtual void TransformName( tString & name ) const {}

    virtual void ReadVal(std::istream &s)
    {
        tString name;
        s >> name;
        
        if ( name == "" )
        {
            tString usageKey("$");
            usageKey += GetTitle();
            usageKey += "_usage";
            tToLower( usageKey );
            con << tOutput( (const char *)usageKey );
            return;
        }

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

    // CAN this be saved at all?
    virtual bool CanSave(){
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
        requiredLevel = tAccessLevel_Owner;
    }

    tAccessLevel GetDefault() const
    {
        return tAccessLevel_DefaultAuthenticated;
    }

    virtual tAccessLevel ReadRawVal(tString const & name, std::istream &s) const
    {
        int levelInt;
        s >> levelInt;
        tAccessLevel level = static_cast< tAccessLevel >( levelInt );

        if ( s.fail() )
        {
            if(printErrors)
            {
                con << tOutput( "$user_level_usage" );
            }
            return GetDefault();
        }

        if(printChange)
        {
            con << tOutput( "$user_level_change", name, tCurrentAccessLevel::GetName( level ) );
        }

        return level;
    }
    
    virtual void TransformName( tString & name ) const
    {
        name = se_EscapeName( name ).c_str();
    }
};

static eUserLevel se_userLevel;

class eAuthorityLevel: public eUserConfig< tAccessLevel >
{
public:
    eAuthorityLevel()
    : eUserConfig< tAccessLevel >( "AUTHORITY_LEVEL" )
    {
        requiredLevel = tAccessLevel_Owner;
    }

    tAccessLevel GetDefault() const
    {
        return tAccessLevel_DefaultAuthenticated;
    }

    virtual tAccessLevel ReadRawVal(tString const & name, std::istream &s) const
    {
        int levelInt;
        s >> levelInt;
        tAccessLevel level = static_cast< tAccessLevel >( levelInt );

        if ( s.fail() || name.StrPos( "@" ) != -1 )
        {
            if(printErrors)
            {
                con << tOutput( "$authority_level_usage" );
            }
            return GetDefault();
        }

        if(printChange)
        {
            con << tOutput( "$authority_level_change", name, tCurrentAccessLevel::GetName( level ) );
        }

        return level;
    }
};

static eAuthorityLevel se_authorityLevel;

#ifdef KRAWALL_SERVER

static tAccessLevel se_adminListMinAccessLevel = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_adminListMinAccessLevelConf( "ADMIN_LIST_MIN_ACCESS_LEVEL", se_adminListMinAccessLevel );
static tAccessLevelSetter se_adminListMinAccessLevelConfLevel( se_adminListMinAccessLevelConf, tAccessLevel_Owner );

static tAccessLevel se_accessLevelListAdmins = tAccessLevel_Moderator; /* This command is sensible, especially if admin rights are set to an
                                                                        * account that can still be registered at.
                                                                        */
static tSettingItem< tAccessLevel > se_accessLevelListAdminsConf( "ACCESS_LEVEL_LIST_ADMINS", se_accessLevelListAdmins );
static tAccessLevelSetter se_accessLevelListAdminsConfLevel( se_accessLevelListAdminsConf, tAccessLevel_Owner );

static tAccessLevel se_accessLevelListAdminsSeeEveryone = tAccessLevel_Moderator;
static tSettingItem< tAccessLevel > se_accessLevelListAdminsSeeEveryoneConf( "ACCESS_LEVEL_LIST_ADMINS_SEE_EVERYONE", se_accessLevelListAdminsSeeEveryone );
static tAccessLevelSetter se_accessLevelListAdminsSeeEveryoneConfLevel( se_accessLevelListAdminsSeeEveryoneConf, tAccessLevel_Owner );

static int se_adminListColors_BestRed = 15;
static tSettingItem< int > se_adminListColors_BetterRed_Conf( "ADMIN_LIST_COLORS_BEST_RED", se_adminListColors_BestRed );

static int se_adminListColors_WorstRed = 15;
static tSettingItem< int > se_adminListColors_WorstRed_Conf( "ADMIN_LIST_COLORS_WORST_RED", se_adminListColors_WorstRed );

static int se_adminListColors_BestGreen = 0;
static tSettingItem< int > se_adminListColors_BetterGreen_Conf( "ADMIN_LIST_COLORS_BEST_GREEN", se_adminListColors_BestGreen );

static int se_adminListColors_WorstGreen = 15;
static tSettingItem< int > se_adminListColors_WorstGreen_Conf( "ADMIN_LIST_COLORS_WORST_GREEN", se_adminListColors_WorstGreen );

static int se_adminListColors_BestBlue = 0;
static tSettingItem< int > se_adminListColors_BetterBlue_Conf( "ADMIN_LIST_COLORS_BEST_BLUE", se_adminListColors_BestBlue );

static int se_adminListColors_WorstBlue = 7;
static tSettingItem< int > se_adminListColors_WorstBlue_Conf( "ADMIN_LIST_COLORS_WORST_BLUE", se_adminListColors_WorstBlue );

void se_ListAdmins ( ePlayerNetID * receiver, std::istream &s, tString command )
{
    // What's going to be sent ? But wait..are we sending anything at all?
    if ( receiver != 0 && receiver->GetAccessLevel() > se_accessLevelListAdmins )
    {
        sn_ConsoleOut( tOutput("$chat_command_accesslevel", command,
                               tCurrentAccessLevel::GetName( receiver->GetAccessLevel() ),
                               tCurrentAccessLevel::GetName( se_accessLevelListAdmins ) ),
                       receiver->Owner() );
        return;
    }

    bool canSeeEverything = false;
    if ( receiver == 0 || receiver->GetAccessLevel() <= se_accessLevelListAdminsSeeEveryone )
    {
        canSeeEverything = true;
    }

    int firstNumber, secondNumber;
    bool gotFirstNumber = true, gotSecondNumber = true;
    s >> firstNumber;
    if ( s.fail() )
    {
        gotFirstNumber = false;
        gotSecondNumber = false;
    }
    else
    {
        s >> secondNumber;
        if ( s.fail() )
        {
            gotSecondNumber = false;
        }
    }

    int userCount = 0, authorityCount = 0;

    std::set< tAccessLevel > accessLevelsToList;

    if ( gotFirstNumber && !gotSecondNumber )
    {
        if ( canSeeEverything || firstNumber <= se_adminListMinAccessLevel )
        {
            accessLevelsToList.insert( static_cast<tAccessLevel>( firstNumber ) );
        }
        else
        {
            sn_ConsoleOut( tOutput( "$admin_list_cant_see", command ), receiver->Owner() );
            return;
        }
    }
    else if ( ( gotFirstNumber && gotSecondNumber ) || ( !gotFirstNumber && !gotSecondNumber ) )
    {
        int lowest, highest;
        if ( !gotFirstNumber && !gotSecondNumber )
        {
            lowest = 0;
            highest = se_adminListMinAccessLevel;
        }
        else
        {
            if ( secondNumber > firstNumber )
            {
                lowest = firstNumber;
                highest = secondNumber;
            }
            else
            {
                lowest = secondNumber;
                highest = firstNumber;
            }
            if ( !canSeeEverything && highest > se_adminListMinAccessLevel )
            {
                sn_ConsoleOut( tOutput("$admin_list_cant_see"), receiver->Owner() );
                return;
            }
        }

        for ( int al = lowest; al <= highest; ++al )
        {
            accessLevelsToList.insert( static_cast<tAccessLevel>(al) );
        }
    }

    // Now browse trough all users in se_userLevel and sort them by access level in adminLevelsMap, then output it
    typedef std::map< tString, tString > aSetOfAdmins;
    typedef std::map< tAccessLevel, aSetOfAdmins > AdminLevelsMap;

    AdminLevelsMap adminLevelsMap;
    tString user;
    tAccessLevel accessLevel;
    AdminLevelsMap::iterator usersAccessLevelInSet;

    eUserLevel::Properties gidMap = se_userLevel.GetMap();

    for ( eUserLevel::Properties::iterator iter = gidMap.begin(); iter != gidMap.end(); ++iter )
    {
        tColoredString userinfo, lowerUser;

        user = (*iter).first;
        accessLevel = (*iter).second;
        if ( accessLevelsToList.find( accessLevel ) != accessLevelsToList.end() )
        {
            // Prepare a string with the info about that user
//            userinfo << "  " << user;

            usersAccessLevelInSet = adminLevelsMap.find( accessLevel );

            if ( usersAccessLevelInSet == adminLevelsMap.end() )
            {

                adminLevelsMap[ accessLevel ] = aSetOfAdmins();

                usersAccessLevelInSet = adminLevelsMap.find( accessLevel );
            }

            aSetOfAdmins & theRightSet = (*usersAccessLevelInSet).second;

            lowerUser = user;
            tToLower( lowerUser );
            theRightSet[ lowerUser ] = user;

            userCount++;
        }
    }

    // Do it again for authorities
    eAuthorityLevel::Properties authoritiesMap = se_authorityLevel.GetMap();

    for ( eUserLevel::Properties::iterator iter = authoritiesMap.begin(); iter != authoritiesMap.end(); ++iter )
    {
        tColoredString userinfo, lowerUser;

        user = (*iter).first;
        accessLevel = (*iter).second;
        if ( accessLevelsToList.find( accessLevel ) != accessLevelsToList.end() )
        {
            // Prepare a string with the info about that user
            usersAccessLevelInSet = adminLevelsMap.find( accessLevel );

            if ( usersAccessLevelInSet == adminLevelsMap.end() )
            {

                adminLevelsMap[ accessLevel ] = aSetOfAdmins();

                usersAccessLevelInSet = adminLevelsMap.find( accessLevel );
            }

            aSetOfAdmins & theRightSet = (*usersAccessLevelInSet).second;

            lowerUser = user;
            tToLower( lowerUser );
            user = tOutput ( "$admin_list_authoritylevel", user );
            lowerUser = tColoredString( tString("AAA") + lowerUser);
            theRightSet[ lowerUser ] = user;

            authorityCount++;
        }
    }

    // Now we have'em sorted by access level, it's show-time!
    // for each access level out there
    AdminLevelsMap::iterator it;

    int numberOfAccessLevelsShown = adminLevelsMap.size() -1;
    int advancement = 0;
    REAL red, green, blue;
    tColoredString accessLevelColor;
    tColoredString output;

    for ( it = adminLevelsMap.begin(); it != adminLevelsMap.end(); ++it )
    {
        output = "";

        // First, print the access level's name
        tAccessLevel accessLevel = (*it).first;

        // Find the appropriate color for it
        if (numberOfAccessLevelsShown <= 0)
            numberOfAccessLevelsShown = 1;
        red = (
                    se_adminListColors_BestRed * ( numberOfAccessLevelsShown - advancement )
                  + se_adminListColors_WorstRed  * ( advancement )
              ) / 16.0 / numberOfAccessLevelsShown;
        green = (
                    se_adminListColors_BestGreen * ( numberOfAccessLevelsShown - advancement )
                  + se_adminListColors_WorstGreen  * ( advancement )
              ) / 16.0 / numberOfAccessLevelsShown;
        blue = (
                    se_adminListColors_BestBlue * ( numberOfAccessLevelsShown - advancement )
                  + se_adminListColors_WorstBlue  * ( advancement )
              ) / 16.0 / numberOfAccessLevelsShown;

        accessLevelColor << tColoredString::ColorString( red, green, blue );
        output << accessLevelColor
               << tCurrentAccessLevel::GetName( accessLevel ) << ": ";
//        accessLevelColor = "";

        // Then print the admin's names
        for ( aSetOfAdmins::iterator userIt = (*it).second.begin(); userIt != (*it).second.end(); ++userIt )
        {
            if ( userIt == (*it).second.begin() )
            {
                output << (*userIt).second;
            }
            else
            {
                output << ", " << (*userIt).second;
            }
        }

        output << "\n";
        sn_ConsoleOut( output, receiver->Owner() );

        ++advancement;
    }

    tOutput sumup;
    sumup = "";

    sn_ConsoleOut( tOutput( "$admin_list_end", command, userCount, authorityCount, static_cast<int>( adminLevelsMap.size() ) ), receiver->Owner() );
}

static void se_ListAdmins_conf( std::istream &s )
{
    se_ListAdmins( 0, s, tString( "PLAYERS" ) );
}

static tConfItemFunc se_ListAdminsConf("ADMINS",&se_ListAdmins_conf);
static tAccessLevelSetter se_ListAdminsConfLevel( se_ListAdminsConf, tAccessLevel_Owner );

#endif

// reserves a nickname
class eReserveNick: public eUserConfig< tString >
{
#ifdef DEBUG
#ifdef KRAWALL_SERVER
    static void TestEscape( char const * t )
    {
        tString test(t);
        tString esc(se_EscapeName( test, false ).c_str());
        tString rev(se_UnEscapeName( esc ).c_str());
        tASSERT( test == rev );
    }
#endif

    static void TestEscape()
    {
#ifdef KRAWALL_SERVER
        TestEscape("ä@%bla:");
        TestEscape("a b@%bl%:");
        TestEscape("a\\_b@%bl%:");
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
            con << tOutput( "$user_alias_usage" );
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
    tAccessLevel newLevel;
    tString authority;
    tString user;

    nKrawall::SplitUserName( authName, user, authority );

    newLevel = se_authorityLevel.Get( authority );
    if ( newLevel < level || newLevel > tAccessLevel_DefaultAuthenticated )
    {
        level = newLevel;
    }

    newLevel = se_userLevel.Get( authName );
    if ( newLevel < level || newLevel > tAccessLevel_DefaultAuthenticated )
    {
        level = newLevel;
    }
}

void ePlayerNetID::Authenticate( tString const & authName, tAccessLevel accessLevel_, ePlayerNetID const * admin, bool messages )
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
            accessLevel_ = static_cast< tAccessLevel >( tAccessLevel_Program - 1 );
        }

        {
            // authorize access level change (careful, don't blindly copy/paste to other places)
            tCurrentAccessLevel level( tAccessLevel_Owner, true );

            // take over the access level
            SetAccessLevel( accessLevel_ );
        }

        if ( messages )
        {
            tString order( "" );
            if ( admin )
            {
                order = tOutput( "$login_message_byorder",
                                 admin->GetLogName() );
            }

            if ( IsHuman() )
            {
                if ( GetAccessLevel() != tAccessLevel_Remote )
                {
                    se_SecretConsoleOut( tOutput( "$login_message_special",
                                                  GetName(),
                                                  newAuthenticatedName,
                                                  tCurrentAccessLevel::GetName( GetAccessLevel() ),
                                                  order ), this, &se_Hide, admin, 0, &se_CanHide );
                }
                else
                {
                    se_SecretConsoleOut( tOutput( "$login_message", GetName(), newAuthenticatedName, order ), this, &se_Hide, admin, 0, &se_CanHide );
                }
            }
        }
    }

    GetScoreFromDisconnectedCopy();

    // force update (removed again to fix name change possibility during a round)
    // UpdateName();
}

void ePlayerNetID::DeAuthenticate( ePlayerNetID const * admin, bool messages ){
    if ( IsAuthenticated() )
    {
        if ( admin && messages )
        {
            se_SecretConsoleOut( tOutput( "$logout_message_deop", GetName(), GetFilteredAuthenticatedName(), admin->GetLogName() ), this, &se_Hide, admin, 0, &se_CanHide );
        }
        else if ( messages )
        {
            se_SecretConsoleOut( tOutput( "$logout_message", GetName(), GetFilteredAuthenticatedName() ), this, &se_Hide, 0, 0, &se_CanHide );
        }
    }

    // force falling back to regular user name on next update
    SetAccessLevel( tAccessLevel_Default );

    rawAuthenticatedName_ = "";

    // force update (removed again to fix name change possibility during a round)
    // UpdateName();
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

//! writes sync data
void ePlayerNetID::WriteSync( Engine::PlayerNetIDSync & sync, bool init )
{
    lastSync=tSysTimeFloat();
    nNetObject::WriteSync( *sync.mutable_base(), init );

    color.WriteSync( *sync.mutable_color() );

    // write ping charity; spectators get a fake (high) value
    if ( currentTeam || nextTeam )
        sync.set_ping_charity( pingCharity );
    else
        sync.set_ping_charity( 1000 );

    if ( sn_GetNetState() == nCLIENT )
    {
        sync.set_player_name( nameFromClient_ );
    }
    else
    {
        sync.set_player_name( nameFromServer_ );
    }

    sync.set_ping( ping );

    // pack chat, spectator and stealth status together
    unsigned short flags = ( chatting_ ? 1 : 0 ) | ( spectating_ ? 2 : 0 ) | ( stealth_ ? 4 : 0 );
    sync.set_flags( flags );

    sync.set_score( score );
    sync.set_disconnected( disconnected );

    sync.set_next_team_id( PointerToID( nextTeam ) );
    sync.set_current_team_id( PointerToID( currentTeam ) );

    sync.set_favorite_number_of_players_per_team( favoriteNumberOfPlayersPerTeam );
    sync.set_name_team_after_me( nameTeamAfterMe );

    sync.set_team_name( teamname );

    sync.set_ready( ready );
}

// makes sure the passed string is not longer than the given maximum
static void se_CutString( tColoredString & string, int maxLen )
{
    if ( string.LenUtf8() > maxLen )
    {
        // nibble away one character at a time (not very efficient)
        while ( string.LenUtf8() > maxLen )
        {
            string.RemoveSubStrUtf8( string.size() - 1, 1 );
        }
        string.RemoveTrailingColor();
    }
}

static void se_CutString( tString & string, int maxLen )
{
    se_CutString( reinterpret_cast< tColoredString & >( string ), maxLen );
}

static bool se_bugColorOverflow=true;
tSettingItem< bool > se_bugColorOverflowColor( "BUG_COLOR_OVERFLOW", se_bugColorOverflow );
void Clamp( unsigned char & colorComponent )
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

static bool se_allowImposters = false;
static tSettingItem< bool > se_allowImposters1( "ALLOW_IMPOSTERS", se_allowImposters );
static tSettingItem< bool > se_allowImposters2( "ALLOW_IMPOSTORS", se_allowImposters );

// test if a user name is used by anyone else than the passed player
static bool se_IsNameTaken( tString const & name, ePlayerNetID const * exception )
{
    if ( name.Len() <= 1 )
        return false;

    if ( !se_allowImposters )
    {
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
    }

#ifdef KRAWALL_SERVER
    // check for reserved nicknames
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

static bool se_filterColorNames=false;
tSettingItem< bool > se_coloredNamesConf( "FILTER_COLOR_NAMES", se_filterColorNames );
static bool se_filterDarkColorNames=false;
tSettingItem< bool > se_coloredDarkNamesConf( "FILTER_DARK_COLOR_NAMES", se_filterDarkColorNames );

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
        remoteName = tColoredString::RemoveColors( remoteName, false );
    }
    else if ( se_filterDarkColorNames )
    {
        remoteName = tColoredString::RemoveColors( remoteName, true );
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

//! reads incremental sync data
void ePlayerNetID::ReadSync( Engine::PlayerNetIDSync const & sync, nSenderInfo const & sender )
{
    // check whether this is the first sync
    bool firstSync = ( this->ID() == 0 );

    nNetObject::ReadSync( sync.base(), sender );

    color.ReadSync( sync.color() );

    if ( !se_bugColorOverflow )
    {
        // clamp color values
        Clamp(color.r_);
        Clamp(color.g_);
        Clamp(color.b_);
    }

    pingCharity = sync.ping_charity();
    sg_ClampPingCharity(pingCharity);

    // name as sent from the other end
    tString & remoteName = ( sn_GetNetState() == nCLIENT ) ? nameFromServer_ : nameFromClient_;

    // name handling
    {
        // read and shorten name, but don't update it yet
        remoteName = sync.player_name();

        // filter
        se_OptionalNameFilters( remoteName );

        se_CutString( remoteName, MAX_NAME_LENGTH );
    }

    // directly apply name changes sent from the server, they are safe.
    if ( sn_GetNetState() != nSERVER )
    {
        UpdateName();
    }

    if (sn_GetNetState()!=nSERVER)
    {
        ping = sync.ping();
    }

    {
        // read chat and spectator status
        unsigned short flags = sync.flags();

        if( sync.has_ready() && (sn_GetNetState() != nSERVER || se_matches < 0 ))
        {
            ready = sync.ready();
        }

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

    {
        if(sn_GetNetState()!=nSERVER)
            score = sync.score();
    }

    if (sync.has_disconnected()){
        if (Owner() != ::sn_myNetID && sn_GetNetState()!=nSERVER)
            disconnected = sync.disconnected();
    }

    if ( sync.has_next_team_id() )
    {
        if ( nSERVER != sn_GetNetState() )
        {
            eTeam *newCurrentTeam, *newNextTeam;

            IDToPointer( sync.next_team_id()   , newNextTeam    );
            IDToPointer( sync.current_team_id(), newCurrentTeam );

            // update team
            tJUST_CONTROLLED_PTR< eTeam > oldTeam( currentTeam );
            if ( newCurrentTeam != currentTeam )
            {
                if ( newCurrentTeam )
                {
                    newCurrentTeam->AddPlayerDirty( this );
                    newCurrentTeam->UpdateProperties();
                }
                else
                {
                    currentTeam->RemovePlayer( this );
                }
            }

            nextTeam = newNextTeam;

            // update properties of the old current team in all cases
            if ( oldTeam )
            {
                oldTeam->UpdateProperties();
            }
        }

        favoriteNumberOfPlayersPerTeam = sync.favorite_number_of_players_per_team();
        nameTeamAfterMe = sync.name_team_after_me();
    }

    if( sync.has_team_name() )
    {
        teamname = sync.team_name();
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

    if( sn_GetNetState() == nCLIENT )
    {
        greeted = true;
    }
}

//! creates a netobject form sync data
ePlayerNetID::ePlayerNetID( Engine::PlayerNetIDSync const & sync, nSenderInfo const & sender )
: nNetObject( sync.base(), sender ),listID(-1), teamListID(-1), timeCreated_( tSysTimeFloat() )
 , allowTeamChange_(false), registeredMachine_(0)
{
    // default access level
    lastAccessLevel = tAccessLevel_Default;

    greeted     =false;
    chatting_   =false;
    spectating_ =false;
    stealth_    =false;
    disconnected=false;
    suspended_  = 0;
    chatFlags_  =0;
    ready       =false;

    color = tShortColor(15,15,15);

    nameTeamAfterMe = false;
    teamname = "";

    pID=-1;
    se_PlayerNetIDs.Add(this,listID);
    object=NULL;
    ping=sn_Connections[sender.SenderID()].ping;
    lastSync=tSysTimeFloat();

    loginWanted = false;

    score=0;
    lastScore_=IMPOSSIBLY_LOW_SCORE;
    // rubberstatus=0;
}

static nNetObjectDescriptor< ePlayerNetID, Engine::PlayerNetIDSync > se_pbdescriptor( 201 );

//! returns the descriptor responsible for this class
nNetObjectDescriptorBase const & ePlayerNetID::DoGetDescriptor() const
{
    return se_pbdescriptor;
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
        greeted=true;
        if( Owner() == sn_myNetID )
        {
            return;
        }

        tOutput o;
        o.SetTemplateParameter(1, GetName() );
        o.SetTemplateParameter(2, st_programVersion);
        o << "$player_welcome";
        tString s;
        s << o;
        s << "\n";
        GreetHighscores(s);
        s << "\n";
        //std::cout << s;
        sn_ConsoleOut(s,Owner());
    }
}

eNetGameObject *ePlayerNetID::Object() const{
    return object;
}

static bool se_consoleLadderLog = false;
static tSettingItem< bool > se_consoleLadderLogConf( "CONSOLE_LADDER_LOG", se_consoleLadderLog );

static bool se_ladderlogDecorateTS = false;
static tSettingItem< bool > se_ladderlogDecorateTSConf( "LADDERLOG_DECORATE_TIMESTAMP", se_ladderlogDecorateTS );
extern bool sn_decorateTS; // from nNetwork.cpp

void se_SaveToLadderLog( tOutput const & out )
{
    if (se_consoleLadderLog)
    {
        std::cout << "[L";
        if(sn_decorateTS) {
            std::cout << st_GetCurrentTime(" TS=%Y/%m/%d-%H:%M:%S");
        }
        std::cout << "] " << out << std::endl;
    }
    if (sn_GetNetState()!=nCLIENT && !tRecorder::IsPlayingBack())
    {
        std::ofstream o;
        if ( tDirectories::Var().Open(o, "ladderlog.txt", std::ios::app) )
        {
            std::stringstream s;
            if(se_ladderlogDecorateTS) {
                s << st_GetCurrentTime("%Y/%m/%d-%H:%M:%S ");
            }
            s << out << std::endl;
            sr_InputForScripts( s.str().c_str() );
            o << s.str();
        }
    }
}

static bool se_chatLog = false;
static tSettingItem<bool> se_chatLogConf("CHAT_LOG", se_chatLog);

static eLadderLogWriter se_chatWriter("CHAT", false);

void se_SaveToChatLog(tOutput const &out) {
    if(sn_GetNetState() != nCLIENT && !tRecorder::IsPlayingBack()) {
        if(se_chatWriter.isEnabled()) {
            se_chatWriter << out;
            se_chatWriter.write();
        }
        if(se_chatLog) {
            std::ofstream o;
            if ( tDirectories::Var().Open(o, "chatlog.txt", std::ios::app) ) {
                o << st_GetCurrentTime("[%Y/%m/%d-%H:%M:%S] ") << out << std::endl;
            }
        }
    }
}

std::list<eLadderLogWriter *> &eLadderLogWriter::writers() {
    static std::list<eLadderLogWriter *> list;
    return list;
}

eLadderLogWriter::eLadderLogWriter(char const *ID, bool enabledByDefault) :
    id(ID),
    enabled(enabledByDefault),
    conf(new tSettingItem<bool>(&(tString("LADDERLOG_WRITE_") + id)(0),
    enabled)),
    cache(id) {
    writers().push_back(this);
}

eLadderLogWriter::~eLadderLogWriter() {
    if(conf) {
        delete conf;
    }
    // generic algorithms aren't exactly easier to understand than regular
    // code, but anyways, let's try one...
    std::list<eLadderLogWriter *> list = writers();
    list.erase(std::find_if(list.begin(), list.end(), std::bind2nd(std::equal_to<eLadderLogWriter *>(), this)));
}

void eLadderLogWriter::write() {
    if(enabled) {
        se_SaveToLadderLog(cache);
    }
    cache = id;
}

void eLadderLogWriter::setAll(bool enabled) {
    std::list<eLadderLogWriter *> list = writers();
    std::list<eLadderLogWriter *>::iterator end = list.end();
    for(std::list<eLadderLogWriter *>::iterator iter = list.begin(); iter != end; ++iter) {
        (*iter)->enabled = enabled;
    }
}

static void LadderLogWriteAll(std::istream &s) {
    bool enabled;
    s >> enabled;
    if(s.fail()) {
        if(tConfItemBase::printErrors) {
            con << tOutput("$ladderlog_write_all_usage") << '\n';
        }
        return;
    }
    if(tConfItemBase::printChange) {
        if(enabled) {
            con << tOutput("$ladderlog_write_all_enabled") << '\n';
        } else {
            con << tOutput("$ladderlog_write_all_disabled") << '\n';
        }
    }
    eLadderLogWriter::setAll(enabled);
}

static tConfItemFunc LadderLogWriteAllConf("LADDERLOG_WRITE_ALL", &LadderLogWriteAll);

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
                            const tOutput& reasonlose,
                            const tOutput& reasonfree)
{
    if (se_matches < 0)
        return;

    score += points;
    if (currentTeam)
        currentTeam->AddScore( points );

    tColoredString name;
    name << *this << tColoredString::ColorString(-1,-1,-1);

    tOutput message;
    message.SetTemplateParameter(1, name);
    message.SetTemplateParameter(2, points > 0 ? points : -points);


    if(points == 0 )
    {
        if (reasonfree.IsEmpty())
            return;
        else
            message.Append(reasonfree);
    }
    else if (points>0)
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

    if( points )
    {
        RequestSync(true);
        se_SaveToScoreFile(message);
    }
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

    bool inorder = false;
    while ( !inorder )
    {
        inorder = true;
        for( int i = se_PlayerNetIDs.Len() - 2; i >= 0; i-- )
        {
            ePlayerNetID *a = se_PlayerNetIDs( i );
            ePlayerNetID *b = se_PlayerNetIDs( i + 1 );
            if ( a->TotalScore() < b->TotalScore() || ( a->TotalScore() == b->TotalScore() && !a->CurrentTeam() && b->CurrentTeam() ) )
            {
                SwapPlayersNo( i, i + 1 );
                inorder = false;
            }
        }
    }
}

void ePlayerNetID::ResetScore(){
    int i;
    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        ePlayerNetID * p = se_PlayerNetIDs(i);
        p->score = 0;
        p->ready = false;
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

// flag memorizing whether the scores already have been rendered this frame
static bool se_alreadyDisplayedScores = false;

static bool show_scores=false;

void ePlayerNetID::DisplayScores()
{
    if( !show_scores || !se_mainGameTimer || se_alreadyDisplayedScores )
    {
        return;
    }
    se_alreadyDisplayedScores = true;

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
        for ( int i = eTeam::teams.Len() - 1; i >= 0; --i )
        {
            if ( eTeam::teams[i]->NumPlayers() > 1 ||
                    ( eTeam::teams[i]->NumPlayers() == 1 && eTeam::teams[i]->Player(0)->Score() != eTeam::teams[i]->Score() ) )
            {
                y = eTeam::RankingGraph(y);
                y-=.06;
                maxPlayers -= ( eTeam::teams.Len() > 6 ? 6 : eTeam::teams.Len() ) + 2;
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

            // This line is example of how we manually get the player color... could come in useful.
            // line << tColoredString::ColorString( p->r/15.0, p->g/15.0, p->b/15.0 ) << name << tColoredString::ColorString(1,1,1);

            // however, using the streaming operator is a lot cleaner. The example is left, as it really can be usefull in some situations.
            line << *p;
            line.SetPos(17, false );
            if ( p->Object() && p->Object()->Alive() )
            {
                line << tColoredString::ColorString(0,1,0) << tOutput("$player_scoretable_alive_yes") << tColoredString::ColorString(-1,-1,-1);
            }
            else
            {
                line << tColoredString::ColorString(1,0,0) << tOutput("$player_scoretable_alive_no") << tColoredString::ColorString(-1,-1,-1);
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

            tColoredString prefix;
            if(p->chatting_)
                prefix << '*';
            if(se_matches < 0 && p->ready)
                prefix << tColoredString::ColorString(.5,1,.5) << 'R';
            if(prefix.Len())
                DisplayText(-.704 - (prefix.Len() * .00075), y, .06, prefix.c_str(), sr_fontScoretable, 1);

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

static eLadderLogWriter se_onlinePlayerWriter("ONLINE_PLAYER", false);
static eLadderLogWriter se_numHumansWriter("NUM_HUMANS", false);

void ePlayerNetID::RankingLadderLog() {
    SortByScore();

    int num_humans = 0;
    int max = se_PlayerNetIDs.Len();
    for(int i = 0; i < max; ++i) {
        ePlayerNetID *p = se_PlayerNetIDs(i);
        if(p->Owner() == 0) continue; // ignore AIs

        se_onlinePlayerWriter << p->GetLogName();

        if(p->IsActive()) {
            se_onlinePlayerWriter << p->ping;
            if(p->currentTeam) {
                se_onlinePlayerWriter << FilterName(p->currentTeam->Name());
                ++num_humans;
            }
            else
            {
                se_onlinePlayerWriter << "";
            }
        }
        se_onlinePlayerWriter << p->GetAccessLevel();
        se_onlinePlayerWriter.write();
    }
    se_numHumansWriter << num_humans;
    se_numHumansWriter.write();
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

bool se_randomizeColor = false;
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
            int color[3] = { other->color.r_, other->color.g_, other->color.b_ };
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

static bool se_ForceSpectate( REAL & time, REAL minTime, char const * message, char const * & cantPlayMessage, int & experienceNeeded )
{
    if ( time < 0 )
    {
        time = 0;
    }

    if ( time >= minTime )
    {
        return false;
    }
    else
    {
        // store message. The player should be presented with a single
        // message telling him the easyest way to gain the required experience;
        // if 10 minutes of total play time are needed and 5 minutes of online play time,
        // we tell the player 10 more minutes of online play are required.
        int needed = int( minTime - time + 2 );
        if ( needed > experienceNeeded )
        {
            experienceNeeded = needed;
        }
        cantPlayMessage = message;

        return true;
    }
}

// sorted storage for pre-join shuffle commands
typedef std::multimap< int, tJUST_CONTROLLED_PTR< ePlayerNetID > >
ePrejoinShuffleMap;
typedef std::pair< int, tJUST_CONTROLLED_PTR< ePlayerNetID > >
ePrejoinPair;
static ePrejoinShuffleMap se_prejoinShuffles;

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
        if ( player->GetAccessLevel() > required && ( player->GetAccessLevel() > se_playAccessLevelInvited || player->GetInvitations().size() < 1 ) && player->IsHuman() )
        {
            player->SetTeamWish(0);
        }
    }
#endif

#ifdef DEDICATED
    if (sr_glOut)
#endif
    {
        // the last update time
        static nTimeRolling lastTime = tSysTimeFloat();
        bool playing = false; // set to true if someone is playing
        bool teamPlay = false; // set to true if someone is playing in a team

        char const * cantPlayMessage = NULL;
        int experienceNeeded = 0;

        for(int i=0; i<MAX_PLAYERS; ++i ){
            bool in_game=ePlayer::PlayerIsInGame(i);
            ePlayer *local_p=ePlayer::PlayerConfig(i);
            tASSERT(local_p);
            tCONTROLLED_PTR(ePlayerNetID) &p=local_p->netPlayer;

            if (!p && in_game && ( !local_p->spectate || se_VisibleSpectatorsSupported() ) ) // insert new player
            {
                // reset last time so idle time in the menus does not count as play time
                lastTime = tSysTimeFloat();

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

                p->color.r_=ePlayer::PlayerConfig(i)->rgb[0];
                p->color.g_=ePlayer::PlayerConfig(i)->rgb[1];
                p->color.b_=ePlayer::PlayerConfig(i)->rgb[2];

                sg_ClampPingCharity();
                p->pingCharity=::pingCharity;
                p->SetTeamname(local_p->Teamname());

                // update spectator status
                bool spectate = local_p->spectate;

                // force to spectator mode if the player lacks experience
                if ( !spectate )
                {
                    se_ForceSpectate( se_playTimeTotal, se_minPlayTimeTotal, "$play_time_total_lacking", cantPlayMessage, experienceNeeded );
                    se_ForceSpectate( se_playTimeOnline, se_minPlayTimeOnline, "$play_time_online_lacking", cantPlayMessage, experienceNeeded  );
                    se_ForceSpectate( se_playTimeTeam, se_minPlayTimeTeam, "$play_time_team_lacking", cantPlayMessage, experienceNeeded );
                    if ( cantPlayMessage )
                    {
                        spectate = true;

                        // print message to console (but don't spam)
                        static nTimeRolling lastTime = -100;
                        nTimeRolling now = tSysTimeFloat();
                        if ( now - lastTime > 30 )
                        {
                            lastTime = now;
                            con << tOutput( cantPlayMessage, experienceNeeded );
                        }
                    }
                }

                if ( p->spectating_ != spectate )
                    p->RequestSync();
                p->spectating_ = spectate;

                // set playing flags
                if ( !spectate )
                {
                    playing = true;
                }


                if ( p->CurrentTeam() && p->CurrentTeam()->NumHumanPlayers() > 1 )
                {
                    teamPlay = true;
                }

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

                p->SetName( local_p->Name() );
            }
        }

        // account for play time
        nTimeRolling now = tSysTimeFloat();
        REAL time = (now - lastTime)/60.0f;
        lastTime = now;

        if ( playing )
        {
            // all activity gets logged here
            se_playTimeTotal += time;

            if ( sn_GetNetState() == nCLIENT )
            {
                // all online activity counts here
                se_playTimeOnline += time;

                if ( teamPlay )
                {
                    // all online team play counts here
                    se_playTimeTeam += time;
                }
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
                    tJUST_CONTROLLED_PTR< eTeam > wish = player->NextTeam();
                    bool assignBack = se_assignTeamAutomatically;
                    se_assignTeamAutomatically = true;
                    player->SetDefaultTeam();
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
                // if the player can't change teams or their wish team emptied, delete the team wish
                if(!player->TeamChangeAllowed() || player->NextTeam()->NumPlayers() == 0)
                {
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

    // execute all prejoin shuffles
    for( ePrejoinShuffleMap::iterator i = se_prejoinShuffles.begin(); i!= se_prejoinShuffles.end(); ++i )
    {
        ePlayerNetID * player = (*i).second;
        if( player && player->IsActive() )
        {
            eTeam * team = player->CurrentTeam();
            if( team )
            {
                int wish = (*i).first;

                // sanity check
                if( wish < 0 )
                {
                    wish = 0;
                }
                if( wish >= team->NumPlayers() )
                {
                    wish = team->NumPlayers()-1;
                }

                // delegate shuffling work
                team->Shuffle( player->TeamListID(), wish );
            }
        }
    }
    se_prejoinShuffles.clear();
             

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
        if ( player->CurrentTeam() && !player->IsSilenced() && player->chatting_ && ( !se_playerWaitTeamleader || player->CurrentTeam()->OldestHumanPlayer() == player ) )
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
                if ( !p->CurrentTeam() && !ePlayerNetID::Scramble )
                {
                    p->SetDefaultTeam();
                }

                if (ePlayerNetID::Scramble)
                {
                    if ( p->CurrentTeam() ) p->SetTeam(NULL);
                    ScramblePlayerIDs.push_back(p);
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
            if ( se_idleKickTime > 0 && se_idleKickTime < p->LastActivity() - roundTime && se_autokickImmunity < p->GetAccessLevel() )
            {
                sn_KickUser( p->Owner(), tOutput( "$network_kill_idle" ) );

                // if many players get kicked with one client, the array may have changed
                if ( i >= se_PlayerNetIDs.Len() )
                    i = se_PlayerNetIDs.Len()-1;
            }
        }
    }

    // kick unworthy spectators (in case MAX_CLIENTS has changed)
    se_kickUnworthySpare = -1;
    se_KickUnworthy();
}

// Set players to be scrambled next round
void ePlayerNetID::SetScramble()
{
    // nothing to be done on the clients
    if ( nCLIENT == sn_GetNetState() )
        return;

    if (eTeam::maxPlayers > 1 && se_PlayerNetIDs.Len() > 2)
    {
        ePlayerNetID::Scramble = true;
        sn_ConsoleOut("$gamestate_scramble_teams");
    } else {
        sn_ConsoleOut("$scramble_teams_too_small");
    }
}

//Scramble up the teams
void ePlayerNetID::ScrambleTeams()
{
    // nothing to be done on the clients
    if ( nCLIENT == sn_GetNetState() )
        return;

#ifdef KRAWALL_SERVER
    // update access level
    UpdateAccessLevelRequiredToPlay();
#endif

    ePlayerNetID::Scramble = false;
    sn_CenterMessage("$gamestate_scramble_teams_center");

    ePlayerNetID::Update();
    std::random_shuffle(ScramblePlayerIDs.begin(), ScramblePlayerIDs.end());

    for ( int i = ScramblePlayerIDs.size()-1; i>=0; i--)
    {
        ScramblePlayerIDs[i]->SetDefaultTeam( true );
    }

    ePlayerNetID::Update();
    ScramblePlayerIDs.clear();
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
            // object->ePlayer = this;
            pni->object = NULL;

            if (bool(object))
                chatting_ = true;

            pni->disconnected = false;
            se_PlayerNetIDs.Remove(pni, pni->listID);
            se_PlayerReferences.Remove( pni );        // really delete it without a message
        }
    }
}


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
    ePlayerNetID::DisplayScores();
    se_alreadyDisplayedScores = false;
}


static rPerFrameTask pf(&scores);

// static bool force_small_cons(){
//    return show_scores;
// }

// static rSmallConsoleCallback sc(&force_small_cons);

//static void cd(){
//    show_scores = false;
//}
//static rCenterDisplayCallback c_d(&cd);

static uActionGlobal score("SCORE");


static bool sf(REAL x){
    if (x>0) show_scores = !show_scores;
    return true;
}

static uActionGlobalFunc saf(&score,&sf);


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
    /* z-man can't remember why this exception was made; probably
       just do disable the chat indicator while you play in local menus.
    if ( sn_GetNetState() == nSTANDALONE && flag == ChatFlags_Menu )
    {
        chatting = false;
    }
    */

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

bool ePlayerNetID::TeamChangeAllowed( bool informPlayer ) const {
    if (!( allowTeamChange_ || se_allowTeamChanges ))
    {
        if ( informPlayer )
        {
            sn_ConsoleOut(tOutput("$player_teamchanges_disallowed"), Owner());
        }
        return false;
    }

    int suspended = GetSuspended();
    if ( suspended > 0 )
    {
        if ( informPlayer )
        {
            sn_ConsoleOut(tOutput("$player_teamchanges_suspended", suspended ), Owner());
        }
        return false;
    }

#ifdef KRAWALL_SERVER
    // only allow players with enough access level to enter the game, everyone is free to leave, though
    if (!( GetAccessLevel() <= AccessLevelRequiredToPlay() || CurrentTeam() || ( GetAccessLevel() <= se_playAccessLevelInvited && GetInvitations().size() > 0 ) ))
    {
        if ( informPlayer )
        {
            sn_ConsoleOut(tOutput("$player_teamchanges_accesslevel",
                                  tCurrentAccessLevel::GetName( GetAccessLevel() ),
                                  tCurrentAccessLevel::GetName( AccessLevelRequiredToPlay() ) ),
                          Owner());
        }
        return false;
    }
#endif

    return true;
}

// put a new player into a default team
void ePlayerNetID::SetDefaultTeam( bool isScrambleCommand )
{
    // only the server should do this, the client does not have the full information on how to do do it right.
    if ( sn_GetNetState() == nCLIENT || ( !se_assignTeamAutomatically && !isScrambleCommand ) || spectating_ || !TeamChangeAllowed() )
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
        if ( t->IsHuman() && !t->IsLocked() && ( !min || min->NumHumanPlayers() > t->NumHumanPlayers() ) )
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
    if (sn_GetNetState() != nCLIENT && 
        bool(currentTeam) && currentTeam->OldestHumanPlayer() &&
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

    if ( nCLIENT != sn_GetNetState() && bool( nextTeam ) && !nextTeam->PlayerMayJoin( this ) )
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

    // create voter
    if( !GetVoter() )
    {
        CreateVoter();
    }
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
    {
        if( nCLIENT != sn_GetNetState() && !oldTeam && teamListID >= 0 )
        {
            // clear current team list ID, it was just a shuffle wish
            // but store the shuffle wish first
            se_prejoinShuffles.insert( ePrejoinPair( teamListID, this ) );
            teamListID = -1;
        }
        nextTeam->AddPlayer ( this );
    }
    else if ( oldTeam )
    {
        oldTeam->RemovePlayer( this );
    }

    if ( nCLIENT != sn_GetNetState() && GetRefcount() > 0 )
    {
        RequestSync();
    }
}

// teams this player is invited to
ePlayerNetID::eTeamSet const & ePlayerNetID::GetInvitations() const
{
    return invitations_;
}

// sends a team change notification message to everyone's console
static void se_TeamChangeMessage( ePlayerNetID const & player )
{
    if ( player.NextTeam() )
    {
        // send notification
        tOutput message;
        tString playerName = tColoredString::RemoveColors(player.GetName());
        tString teamName = tColoredString::RemoveColors(player.NextTeam()->Name());
        message.SetTemplateParameter(1, playerName );
        if ( playerName != teamName )
        {
            message.SetTemplateParameter(2, teamName );
            message << "$player_joins_team";
        }
        else
        {
            message << "$player_joins_team_solo";
        }

        sn_ConsoleOut( message );
    }
}

// create a new team and join it (on the server)
void ePlayerNetID::CreateNewTeam()
{
    // check if the team change is legal
    tASSERT ( nCLIENT !=  sn_GetNetState() );
    
    if ( !TeamChangeAllowed( true ) )
        return;
#ifdef KRAWALL_SERVER
    if( GetAccessLevel() > AccessLevelRequiredToPlay())
        return;
#endif

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
    else
    {
        nextTeam->UpdateAppearance();
        se_TeamChangeMessage( *this );
    }
}

const unsigned short TEAMCHANGE = 0;
const unsigned short NEW_TEAM   = 1;
const unsigned short AUTO_TEAM  = 2;

// AUTO_TEAM is only available since 0.4_alpha
nVersionFeature se_defaultTeamWish(22);

// express the with to put a new player into a default team
void ePlayerNetID::SetDefaultTeamWish()
{
    if ( nCLIENT ==  sn_GetNetState() )
    {
        Engine::PlayerNetIDControl & control = *BroadcastControl().MutableExtension( Engine::player_control );

        control.set_type( AUTO_TEAM );
    }
    else
        // create a new team if possible otherwise spectate
        SetDefaultTeam();
}

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
        Engine::PlayerNetIDControl & control = *BroadcastControl().MutableExtension( Engine::player_control );

        control.set_type( TEAMCHANGE );
        control.set_team_id( PointerToID( newTeam ) );

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
        Engine::PlayerNetIDControl & control = *BroadcastControl().MutableExtension( Engine::player_control );

        control.set_type( NEW_TEAM );
    }
    else
        // create a new team if possible otherwise spectate
        CreateNewTeam();
}

// receive the team control wish
void ePlayerNetID::ReceiveControlNet( Network::NetObjectControl const & controlBase )
{
    tASSERT( controlBase.HasExtension( Engine::player_control ) );
    Engine::PlayerNetIDControl const & control = controlBase.GetExtension( Engine::player_control );

    short messageType = control.type();

    switch (messageType)
    {
    case AUTO_TEAM:
        {
            if (NextTeam()==NULL)
            {
                SetDefaultTeam(true);
            }

            break;
        }
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

            IDToPointer( control.team_id(), newTeam );

            if(!TeamChangeAllowed( true )) {
                break;
            }

            // annihilate team if it no longer is in the game
            if ( bool(newTeam) && newTeam->TeamID() < 0 )
                newTeam = 0;

            // NULL team, but non-null team ID probably means that the change target does not
            // exist any more. Create a new team instead.
            if ( !newTeam && control.team_id() != 0 )
            {
                if ( currentTeam )
                    sn_ConsoleOut( tOutput( "$player_joins_team_noex", tColoredString::RemoveColors(GetName()) ), Owner() );
                break;
            }

            // check if the resulting message is obnoxious
            bool redundant = ( nextTeam == newTeam );
            bool obnoxious = ( nextTeam != currentTeam || redundant );

            SetTeam( newTeam );

            // announce the change
            if ( bool(nextTeam) && !redundant )
            {

                // count it as spam if it is obnoxious
                if ( obnoxious )
                    GetChatSpam().CheckSpam( 4.0, Owner(), tOutput("$spam_teamchage") );

                if( se_matches < 0 && nextTeam != currentTeam
                    && nextTeam->PlayerMayJoin( this ) )
                {
                    // Join immediately during warmup if possible
                    if( Object() && Object()->Alive() )
                        Object()->Kill();
                    UpdateTeam();
                    RequestSync();
                }
                else
                    se_TeamChangeMessage( *this );
            }

            break;
        }
    default:
        {
            tASSERT(0);
        }
    }
}

// easier to implement conversion helpers: just extract the relevant sub-protbuf.
nProtoBuf       * ePlayerNetID::ExtractControl( Network::NetObjectControl       & control )
{
    return control.MutableExtension( Engine::player_control );
}

nProtoBuf const * ePlayerNetID::ExtractControl( Network::NetObjectControl const & control )
{
    tASSERT( control.HasExtension( Engine::player_control ) );
    return & control.GetExtension( Engine::player_control );
}


void ePlayerNetID::Color( REAL&a_r, REAL&a_g, REAL&a_b ) const
{
    if ( ( static_cast<bool>(currentTeam) ) && ( ( currentTeam->IsHuman() || currentTeam->TeamNamedAfterColor() ) ) )
    {
        REAL w = 5;
        REAL r_w = 2;
        REAL g_w = 1;
        REAL b_w = 2;

        int r = this->color.r_;
        int g = this->color.g_;
        int b = this->color.b_;

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
        a_r = color.r_/15.0;
        a_g = color.g_/15.0;
        a_b = color.b_/15.0;
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
        _b=(2*b + w*currentTeam->B())/( 15.0 * ( w + 2 ) );
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
        ePlayerNetID * p = ePlayerNetID::FindPlayerByName( name, requester );
        if ( p )
        {
            return p->Owner();
        }
    }

    return 0;
}

static void se_PlayerMessageConf(std::istream &s)
{
    if ( se_NeedsServer( "PLAYER_MESSAGE", s ) )
    {
        return;
    }

    int receiver = se_ReadUser( s );

    tColoredString msg;
    s >> msg;

    if ( receiver <= 0 || s.good() )
    {
        con << tOutput("$player_message_usage");
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
        con << tOutput("$kick_usage");
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

static void se_MoveToConf(std::istream &s, REAL severity, const char * command )
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

    int pos, port = se_defaultKickToPort;
    if ( ( pos = server.StrPos( ":" ) ) != -1 )
    {
        port = atoi( server.SubStr( pos + 1 ) );
        server = server.SubStr( 0, pos );
    }

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
        con << tOutput( "$kickmove_usage", command );
        return;
    }
}

static void se_KickToConf(std::istream &s )
{
    se_MoveToConf( s, 1, "KICK_TO" );
}

static tConfItemFunc se_kickToConf("KICK_TO",&se_KickToConf);
static tAccessLevelSetter se_kickToConfLevel( se_kickToConf, tAccessLevel_Moderator );

static void se_MoveToConf(std::istream &s )
{
    se_MoveToConf( s, 0, "MOVE_TO" );
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
        con << tOutput( "$ban_usage" );
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

    try
    {
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
    // name.ToInt will throw a tGenericException when name doesn't look like an int, but it's not fatal, so we will just go on
    } catch ( tGenericException ) {}

    return ePlayerNetID::FindPlayerByName( name );
}

static void Kill_conf(std::istream &s)
{
    if ( se_NeedsServer( "KILL", s, false ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );

    if ( p && p->Object() && p->Object()->Alive() )
    {
        p->Object()->Kill();
        sn_ConsoleOut( tOutput( "$player_admin_kill", p->GetColoredName() ) );
    }
}

static tConfItemFunc kill_conf("KILL",&Kill_conf);
static tAccessLevelSetter se_killConfLevel( kill_conf, tAccessLevel_Moderator );

static void Slap_conf(std::istream &s)
{
    if ( se_NeedsServer( "SLAP", s, false ) )
    {
        return;
    }

    ePlayerNetID * victim = ReadPlayer( s );

    if ( victim )
    {
        int points = 1;

        s >> points;

        tOutput win;
        tOutput lose;
        tOutput free;
        win << "$player_admin_slap_win";
        lose << "$player_admin_slap_lose";
        free << "$player_admin_slap_free";
        victim->AddScore( -points, win, lose, free );
    }
}

static tConfItemFunc slap_conf("SLAP", &Slap_conf);

static int se_suspendDefault = 5;
static tSettingItem< int > se_suspendDefaultConf( "SUSPEND_DEFAULT_ROUNDS", se_suspendDefault );

static void Suspend_conf_base(std::istream &s, int rounds )
{
    if ( se_NeedsServer( "SUSPEND", s, false ) )
    {
        return;
    }

    ePlayerNetID * p = ReadPlayer( s );

    if ( rounds > 0 )
    {
        s >> rounds;
    }

    if ( p )
    {
        p->Suspend( rounds );
    }
}

static void Suspend_conf(std::istream &s )
{
    Suspend_conf_base( s, se_suspendDefault );
}


static void UnSuspend_conf(std::istream &s )
{
    Suspend_conf_base( s, 0 );
}

static tConfItemFunc suspend_conf("SUSPEND",&Suspend_conf);
static tAccessLevelSetter se_suspendConfLevel( suspend_conf, tAccessLevel_Moderator );

static tConfItemFunc unsuspend_conf("UNSUSPEND",&UnSuspend_conf);
static tAccessLevelSetter se_unsuspendConfLevel( unsuspend_conf, tAccessLevel_Moderator );

static void Silence_conf(std::istream &s)
{
    ePlayerNetID * p = ReadPlayer( s );
    if ( p && !p->IsSilenced() )
    {
#ifndef DEDICATED
        if ( sn_GetNetState() == nCLIENT )
            sn_ConsoleOut( tOutput( "$player_silenced_local", p->GetColoredName() ) );
        else
#endif
            sn_ConsoleOut( tOutput( "$player_silenced", p->GetColoredName() ) );
        p->SetSilenced( true );
    }
}

static tConfItemFunc silence_conf("SILENCE",&Silence_conf);
static tAccessLevelSetter se_silenceConfLevel( silence_conf, tAccessLevel_Moderator );

static void Voice_conf(std::istream &s)
{
    ePlayerNetID * p = ReadPlayer( s );
    if ( p && p->IsSilenced() )
    {
#ifndef DEDICATED
        if ( sn_GetNetState() == nCLIENT )
            sn_ConsoleOut( tOutput( "$player_voiced_local", p->GetColoredName() ) );
        else
#endif
            sn_ConsoleOut( tOutput( "$player_voiced", p->GetColoredName() ) );
        p->SetSilenced( false );
    }
}

static tConfItemFunc voice_conf("VOICE",&Voice_conf);
static tAccessLevelSetter se_voiceConfLevel( voice_conf, tAccessLevel_Moderator );
static tConfItemFunc unsilence_conf("UNSILENCE",&Voice_conf);
static tAccessLevelSetter se_unsilenceConfLevel( unsilence_conf, tAccessLevel_Moderator );

static tString sg_url;
static tSettingItem< tString > sg_urlConf( "URL", sg_url );

static tString sg_options("Nothing special.");
#ifdef DEDICATED
static tConfItemLine sg_optionsConf( "SERVER_OPTIONS", sg_options );
#endif

class gServerInfoAdmin: public nServerInfoAdmin
{
public:
    gServerInfoAdmin(){}

private:
    virtual tString GetUsers() const
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

    virtual tString GetGlobalIDs() const
    {
        tString ret;
#ifdef KRAWALL_SERVER
        int count = 0;

        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);
            if ( p->IsHuman() )
            {
                if( p->IsAuthenticated() && !se_Hide(p, tAccessLevel_Default) )
                {
                    for(; count > 0; --count)
                    {
                        ret << "\n";
                    }
                    ret << p->GetFilteredAuthenticatedName();
                }
                ++count;
            }
        }
#endif
        return ret;
    }

    virtual tString GetOptions() const
    {
        se_CutString( sg_options, 240 );
        return sg_options;
    }

    virtual tString GetUrl() const
    {
        se_CutString( sg_url, 75 );
        return sg_url;
    }

    // levels of values
    enum Classifications
    {
        Classification_None,
        Classification_LudicrouslyLow,
        Classification_Nano,
        Classification_Micro,
        Classification_ExtremlyLow,
        Classification_VeryLow,
        Classification_Low,
        Classification_Medium,
        Classification_High,
        Classification_VeryHigh,
        Classification_ExtremlyHigh,
        Classification_Mega,
        Classification_Giga,
        Classification_LudicrouslyHigh,
        Classification_Infinite
    };

    static tString ComposeClassification( int index, char const * classification_root, char const * item )
    {
        std::ostringstream s;
        s << classification_root << index;
        return tString(tOutput( s.str().c_str(), tOutput(item) ) );
    }

    class Classifier
    {
    public:
        // classifies lowerBoundX just as classificationX, any lower value as classificationX-1
        Classifier( REAL lowerBound1, int classification1,
                    REAL lowerBound2, int classification2 )
        : logBound1_(log(lowerBound1))
          , logBound2_(log(lowerBound2))
          , classification1_( classification1 )
          , classification2_( classification2 )
        {
        }

        int Classify( REAL value, int invalid = Classification_None )
        {
            if( value <= 0 )
            {
                return invalid;
            }
            REAL a = ( log( value ) - logBound1_ )/( logBound2_ - logBound1_ );
            REAL c = classification1_ + a * ( classification2_ - classification1_ );
            int ret = int(floor( c ));
            if ( ret < Classification_LudicrouslyLow )
                ret = Classification_LudicrouslyLow;
            if ( ret > Classification_LudicrouslyHigh )
                ret = Classification_LudicrouslyHigh;
            return ret;
        }
    private:
        REAL logBound1_, logBound2_;
        REAL classification1_, classification2_;
    };

    // adds a +1 penalty for every level value goes over the limit
    static int Penalty( int limit, int value, int max = 3 )
    {
        int ret = value - limit;
        if ( ret < 0 )
        {
            ret = 0;
        }
        else if ( ret > max )
        {
            ret = max;
        }

        return ret;
    }

    virtual void Classify( nServerInfo::SettingsDigest const & in, 
                           nServerInfo::Classification & out ) const
    {
        // various classifiers
        static Classifier delayClassifier( .06, Classification_Medium, .12, Classification_High );
        static Classifier accelerationClassifier( 4, Classification_Medium, 8, Classification_High );
        static Classifier rubberHumpClassifier( 1, Classification_Medium, 100, Classification_LudicrouslyHigh );
        static Classifier rubberWallClassifier( 0.015, Classification_Medium, .3, Classification_LudicrouslyHigh );
        static Classifier wallsLengthClassifier( 1, Classification_Low, 10, Classification_VeryHigh );
        
        // apply them
        int delay        = delayClassifier.Classify( in.cycleDelay_ );
        int acceleration = accelerationClassifier.Classify( in.acceleration_ );
        int rubberHump   = rubberHumpClassifier.Classify( in.rubberWallHump_ );
        int rubberWall   = rubberWallClassifier.Classify( in.rubberHitWallRatio_ );
        int length       = wallsLengthClassifier.Classify( in.wallsLength_, Classification_Infinite );

        // fuse rubber values into one
        int rubber = rubberHump;
        if( rubberWall + 1 < rubber )
        {
            rubber--;
        }
        if( rubberWall > rubber + 1 )
        {
            rubber = rubberWall - 1;
        }
        if( rubber <= Classification_Micro )
        {
            rubber = Classification_None;
        }

        bool digestIsAccurate = in.GetFlag( nServerInfo::SettingsDigest::Flags_SettingsDigestSent );

        std::ostringstream compose;
        if( digestIsAccurate )
        {
            compose 
            << ComposeClassification( rubber, "$classification_", "$classification_rubber" ) << ", "
            << ComposeClassification( acceleration, "$classification_", "$classification_acceleration" ) << ", "
            << ComposeClassification( delay, "$classification_", "$classification_delay" ) << ", "
            << ComposeClassification( length, "$classification_wall_", "$classification_walls" );
            if( in.GetFlag( nServerInfo::SettingsDigest::Flags_TeamPlay ) )
            {
                compose << ", " << tOutput( "$classification_teams" );
            }
            if( in.GetFlag( nServerInfo::SettingsDigest::Flags_NondefaultMap ) )
            {
                compose << ", " << tOutput( "$classification_maps" );
            }
            compose << "\n";
        }

        // determine experience level required.
        int experienceLevel = 0;
        if( in.GetFlag( nServerInfo::SettingsDigest::Flags_TeamPlay ) )
        {
            // team play is difficult.
            experienceLevel+=2;
        }
        if( in.GetFlag( nServerInfo::SettingsDigest::Flags_NondefaultMap ) )
        {
            // non default maps hint at complicated gameplay.
            experienceLevel++;
        }
        experienceLevel += Penalty( Classification_Medium, rubber );
        experienceLevel += Penalty( delay, Classification_Medium );
        // low delay and high rubber usually go hand in hand, let's not overpenalize them
        if( experienceLevel > 4 )
        {
            experienceLevel = 4;
        }
        experienceLevel += Penalty( Classification_High, acceleration, 1 );
        experienceLevel += Penalty( length, Classification_Low, 2 );

        if( !digestIsAccurate )
        {
            experienceLevel = 1;
        }

        // calculate the experience level the player has
        int levelDistance = 10;
        int experienceLevelThere = int(floor( se_playTimeTotal/levelDistance ));
        int missingForThisLevel = int( experienceLevelThere*levelDistance - se_playTimeTotal + 2 );

        out.noJoin_ = "";
        out.sortOverride_ = experienceLevel - experienceLevelThere;
        if( out.sortOverride_ > 0 )
        {
            compose << tOutput( digestIsAccurate ? "$network_master_exp_recommended_long" : "$network_master_exp_recommended_long_oldserver", missingForThisLevel + out.sortOverride_*levelDistance );
            // if( digestIsAccurate )
            {
                out.noJoin_ = tOutput("$network_master_exp_recommended");
            }
        }
        else
        {
            out.sortOverride_ = 0;
        }

        // check whether a GID has been registered
        bool registered = false;
        for( int i = MAX_PLAYERS-1; i >= 0; --i )
        {
            registered |= ( ePlayer::PlayerConfig(i)->globalID != se_defaultGID && ePlayer::PlayerConfig(i)->globalID.Len() > 1 );
        }

        // find reasons why the player should not join
        if( !registered && in.GetFlag( nServerInfo::SettingsDigest::Flags_AuthenticationRequired ) )
        {
            out.noJoin_ = tOutput("$network_master_login_needed");
            compose << tOutput("$network_master_login_needed_long");

            // keep servers you need authentication to play on at the bottom until
            // the player is really experienced
            if( out.sortOverride_ < 1 && se_playTimeOnline < 120 )
            {
                out.sortOverride_ = 1;
            }
        }
        else
        {
            REAL timeLacking=0;
            if( in.minPlayTimeTotal_ - se_playTimeTotal > 0 )
            {
                timeLacking = in.minPlayTimeTotal_ - se_playTimeTotal;
            }
            if( in.minPlayTimeOnline_ - se_playTimeOnline > timeLacking )
            {
                timeLacking = in.minPlayTimeOnline_ - se_playTimeOnline;
            }
            if( in.minPlayTimeTeam_ - se_playTimeTeam > timeLacking )
            {
                timeLacking = in.minPlayTimeTeam_ - se_playTimeTeam;
            }
            if ( timeLacking > 0 )
            {
                out.noJoin_ = tOutput("$network_master_exp_needed");
                compose << tOutput("$network_master_exp_needed_long", timeLacking+2 );
                int minLevel = int(timeLacking/10)+1;
                if( out.sortOverride_ < minLevel )
                {
                    out.sortOverride_ = minLevel;
                }
            }
        }

        out.description_ = compose.str();
    }
};

static gServerInfoAdmin sg_serverAdmin;

static eLadderLogWriter se_playerEnteredWriter("PLAYER_ENTERED", true);
static eLadderLogWriter se_playerRenamedWriter("PLAYER_RENAMED", true);

class eNameMessenger
{
public:
    eNameMessenger( ePlayerNetID & player )
    : player_( player )
    , oldScreenName_( player.GetName() )
    , oldLogName_( player.GetLogName() )
    , adminRename_( !player.IsAllowedToRename() )
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
                se_playerEnteredWriter << logName << nMachine::GetMachine(player_.Owner()).GetIP() << screenName;
                se_playerEnteredWriter.write();

                player_.Greet();

                // print spectating join message (regular join messages are handled by eTeam)
                if ( ( player_.IsSpectating() || player_.IsSuspended() || !se_assignTeamAutomatically ) && ( se_assignTeamAutomatically || se_specSpam ) )
                {
                    mess << "$player_entered_spectator";
                    sn_ConsoleOut(mess);
                }
            }
        }
        else if ( logName != oldLogName_ || screenName != oldScreenName_ )
        {
            se_playerRenamedWriter << oldLogName_ << logName << nMachine::GetMachine(player_.Owner()).GetIP() << screenName;
            se_playerRenamedWriter.write();

            if ( oldScreenName_ != screenName )
            {
                if ( bool(player_.GetVoter() ) )
                {
                    player_.GetVoter()->PlayerChanged();
                }

                if ( adminRename_ )
                {
                    // if our player isn't able to rename, he didn't do it alone, did he ? :)
                    mess << "$player_got_renamed";
                }
                else
                {
                    mess << "$player_renamed";
                }
                sn_ConsoleOut(mess);
            }
        }
    }

    ePlayerNetID & player_;
    tString oldScreenName_, oldLogName_;
    tColoredString oldPrintName_;
    bool adminRename_;
};

// ******************************************************************************************
// *
// *    UpdateName
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
    if (
            sn_GetNetState() == nSTANDALONE
        || ( Owner() == 0 && sn_GetNetState() != nCLIENT )
        || (
                IsHuman()
            && ( sn_GetNetState() == nCLIENT || !messenger.adminRename_ )
            && ( sn_GetNetState() != nCLIENT || Owner() == sn_myNetID )
           )
       )
    {

        if ( sn_GetNetState() == nSTANDALONE || Owner() == 0 || ( IsHuman() && ( nameFromServer_ != nameFromClient_ && !messenger.adminRename_ ) ) )
        {
            // apply name filters only on remote players
            if ( Owner() != 0 )
            se_OptionalNameFilters( nameFromClient_ );

            // nothing wrong ? proceed to renaming
            nameFromAdmin_ = nameFromServer_ = nameFromClient_;
        }
        else
        {
            // revert name
            nameFromClient_ = nameFromServer_ = nameFromAdmin_;
        }
    }
    // remove colors from name
    tString newName = tColoredString::RemoveColors( nameFromServer_ );
    tString newUserName = se_UnauthenticatedUserName( newName );

    // test if it is already taken, find an alternative name if so.
    if ( sn_GetNetState() != nCLIENT && se_IsNameTaken( newUserName, this ) )
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
        nameFromAdmin_ = nameFromServer_ = newName;
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

// ******************************************************************************************
// *
// *    AllowRename
// *
// ******************************************************************************************
//!
//!        @param    allow  let the player rename or not ?
//!
// ******************************************************************************************

void ePlayerNetID::AllowRename( bool allow )
{
    // simple eh ? :-)
    renameAllowed_ = allow;
}
// Now we're here, set commands for it
static void AllowRename( std::istream & s , bool allow , bool announce )
{
    if ( se_NeedsServer( "(DIS)ALLOW_RENAME_PLAYER", s ) )
    {
        return;
    }
    ePlayerNetID * p;
    p = ReadPlayer( s );
    if ( p )
    {
//      announce it?
        if ( announce && allow )
        {
            sn_ConsoleOut( tOutput("$player_allowed_rename", p->GetColoredName() ) );
        }
        else if ( announce && !allow )
        {
            sn_ConsoleOut( tOutput("$player_disallowed_rename", p->GetColoredName() ) );
        }

        p->AllowRename ( allow );
    }
}
static void AllowRename( std::istream & s )
{
    if ( se_NeedsServer( "ALLOW_RENAME_PLAYER", s ) )
    {
        return;
    }
    AllowRename( s , true, true );
}
static tConfItemFunc AllowRename_conf("ALLOW_RENAME_PLAYER", &AllowRename);
static tAccessLevelSetter AllowRename_confLevel( AllowRename_conf, tAccessLevel_Moderator );

static void DisAllowRename( std::istream & s )
{
    if ( se_NeedsServer( "DISALLOW_RENAME_PLAYER", s ) )
    {
        return;
    }
    AllowRename( s , false, true );
}
static tConfItemFunc DisAllowRename_conf("DISALLOW_RENAME_PLAYER", &DisAllowRename);
static tAccessLevelSetter DisAllowRename_confLevel( DisAllowRename_conf, tAccessLevel_Moderator );

// ******************************************************************************************
// *
// *    IsAllowedToRename
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

bool ePlayerNetID::HasRenameCapability ( ePlayerNetID const * victim, ePlayerNetID const * admin )
{
    return Rename_conf.GetRequiredLevel() <= admin->GetAccessLevel();
}

// ******************************************************************************************
// *
// *    IsAllowedToRename
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************


bool ePlayerNetID::IsAllowedToRename ( void )
{
    if ( !IsHuman() || ( nameFromServer_ == nameFromClient_ && nameFromServer_ == nameFromAdmin_ ) || nameFromServer_.Len() < 1 || sn_GetNetState() == nCLIENT )
    {
        // Don't complain about people who either are bots, doesn't change name, or are entering the grid
        return true;
    }
    if ( nameFromServer_ != nameFromAdmin_ )
    {
        // We are going to rename this player, so, no need to tell him he's trying to rename from his old name to his old name :-P
        // Just return false so the server thinks the player can't be renamed in any other way than admin-rename.
        return false;
    }
    // didn't we disallow this player to rename via a command ?
    if ( !this->renameAllowed_ )
    {
        tOutput message( "$player_rename_rejected_admin", nameFromServer_, nameFromClient_ );
        se_SecretConsoleOut( message, this, &ePlayerNetID::HasRenameCapability, this );
        con << message;
        return false;
    }
    // don't let the player rename if s/he is silenced.
    if ( this->IsSilenced() )
    {
        tOutput message("$player_rename_silenced");
        sn_ConsoleOut( message, Owner() );
        return false;
    }
    // disallow name changes if there was a kick vote recently
    if ( !( !bool(this->voter_) || voter_->AllowNameChange() || nameFromServer_.Len() <= 1 ) && nameFromServer_ != nameFromClient_ )
    {
        // inform victim to avoid complaints
        tOutput message( "$player_rename_rejected_votekick", nameFromServer_, nameFromClient_ );
        sn_ConsoleOut( message, Owner() );
        con << message;
        return false;
    }

    // Nothing to complain about ? fine, he can rename
    return true;
}

class ePlayerCharacterFilter :public tCharacterFilter
{
public:
    ePlayerCharacterFilter( void );
};

// filters illegal player characters
ePlayerCharacterFilter::ePlayerCharacterFilter()
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

static bool se_IsUnderscore( char c )
{
    return c == '_';
}

// ******************************************************************************************
// *
// *    FilterName
// *
// ******************************************************************************************
//!
//!        @param    in   input name
//!        @param    out  output name cleared to be usable as a username
//!
// ******************************************************************************************

void ePlayerNetID::FilterName( tString const & in, tString & out )
{
    static ePlayerCharacterFilter filter;
    tString temp = tColoredString::RemoveColors( in );
    out = filter.FilterString( temp );

    // strip leading and trailing unknown characters
    se_StripMatchingEnds( out, se_IsUnderscore, se_IsUnderscore );
}

// ******************************************************************************************
// *
// *    FilterName
// *
// ******************************************************************************************
//!
//!        @param    in   input name
//!        @return        output name cleared to be usable as a username
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
// * ForceName
// *
// ******************************************************************************************
//!
//!      @param  name    this player's name without colors. to set
//!
// ******************************************************************************************
ePlayerNetID & ePlayerNetID::ForceName( tString const & name )
{
    // This just sets nameFromAdmin_ waiting for the server to look at it next round (this->UpdateName())
    // Also forbid the player from renameing then, because it would be useless :-P
    if ( sn_GetNetState() != nCLIENT )
    {
        tColoredString oldName;
        tColoredString newName;

        oldName << this->GetColoredName() << tColoredString::ColorString( -1, -1, -1 );

        this->nameFromAdmin_ = name;
        this->nameFromAdmin_.NetFilter();
        se_CutString( this->nameFromAdmin_, MAX_NAME_LENGTH );

        // crappiest line ever :-/
        newName << tColoredString::ColorString( color.r_/15.0, color.g_/15.0, color.b_/15.0 ) << this->nameFromAdmin_ << tColoredString::ColorString( -1, -1, -1 );

        con << tOutput("$player_will_be_renamed", newName, oldName);

        AllowRename ( false );
    }

    return *this;
}

// Set an admin command for it
void ForceName ( std::istream & s )
{
    ePlayerNetID * p;
    p = ReadPlayer( s );
    if ( p )
    {
        tString newname;
        newname.ReadLine( s );
        p->ForceName ( newname );
    }
}
// the tConfItemFunc is defined in ePlayer.h
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

class eEnemiesWhitelist
{
public:
    eEnemiesWhitelist() :usernames_whitelist_(), ip_addresses_whitelist_() {}
    
    // Returns true if the two players can be enemies.
    // 
    // Assumes both "a" and "b" are from the same IP address.
    bool CanBeEnemies( const ePlayerNetID * a, const ePlayerNetID * b ) const
    {
        bool enemies = HasEntry( ip_addresses_whitelist_, a->GetMachine().GetIP() );
#ifdef KRAWALL_SERVER
        enemies |= a->IsAuthenticated() && HasEntry( usernames_whitelist_, a->GetLogName() );
        enemies |= b->IsAuthenticated() && HasEntry( usernames_whitelist_, b->GetLogName() );
#endif
        return enemies;
    }
    
    void AddUsernames( std::istream & s )
    {
        Parse( usernames_whitelist_, s );
    }
    
    void AddIPAddresses( std::istream & s )
    {
        Parse( ip_addresses_whitelist_, s );
    }
protected:
    typedef std::set< tString > StringSet;
    
    void Parse( StringSet & whitelist, std::istream & s )
    {
        int entries_count = 0;
        while ( s.good() )
        {
            tString name;
            s >> name;
            if ( name.Len() > 1 )
            {
                std::pair< StringSet::iterator, bool > ret = whitelist.insert( name );
                if ( ret.second ) entries_count++;
            }
        }
        con << tOutput( "$whitelist_enemies_success", entries_count ) << '\n';
    }
    
    bool HasEntry( const StringSet & whitelist, const tString & value ) const
    {
        return whitelist.find( value ) != whitelist.end();
    }
    
    StringSet usernames_whitelist_;
    StringSet ip_addresses_whitelist_;    
};

static eEnemiesWhitelist se_enemiesWhitelist;

#ifdef KRAWALL_SERVER
void se_WhiteListEnemiesUsername( std::istream & s )
{
    se_enemiesWhitelist.AddUsernames( s );
}
static tConfItemFunc se_whiteListEnemiesUsernameConfItemFunc( "WHITELIST_ENEMIES_USERNAME", se_WhiteListEnemiesUsername );
#endif

void se_WhiteListEnemiesIP( std::istream & s )
{
    se_enemiesWhitelist.AddIPAddresses( s );
}
static tConfItemFunc se_whiteListEnemiesIPConfItemFunc( "WHITELIST_ENEMIES_IP", se_WhiteListEnemiesIP );

// allow enemies from the same IP?
static bool se_allowEnemiesSameIP = false;
static tSettingItem< bool > se_allowEnemiesSameIPConf( "ALLOW_ENEMIES_SAME_IP", se_allowEnemiesSameIP );
// allow enemies from the same client?
static bool se_allowEnemiesSameClient = false;
static tSettingItem< bool > se_allowEnemiesSameClientConf( "ALLOW_ENEMIES_SAME_CLIENT", se_allowEnemiesSameClient );

// *******************************************************************************
// *
// *    Enemies
// *
// *******************************************************************************
//!
//!        @param    a    first player to compare
//!        @param    b    second player to compare
//!        @return        true if a should be able to score against b or vice versa
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
    if ( !se_allowEnemiesSameIP && a->Owner() != 0 && a->GetMachine() == b->GetMachine() && !se_enemiesWhitelist.CanBeEnemies( a, b ) )
        return false;

    // no scoring for two players from the same client
    if ( !se_allowEnemiesSameClient && a->Owner() != 0 && a->Owner() == b->Owner() )
        return false;

    // no objections
    return true;
}

// *******************************************************************************
// *
// *    RegisterWithMachine
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
// *    UnregisterWithMachine
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::UnregisterWithMachine( void )
{
    if ( registeredMachine_ )
    {
        // store suspension count
        if ( GetVoter() )
        {
            GetVoter()->suspended_ = suspended_;
            GetVoter()->silenced_ = silenced_;
        }

        registeredMachine_->RemovePlayer();
        registeredMachine_ = 0;
    }
}

// *******************************************************************************
// *
// *    DoGetMachine
// *
// *******************************************************************************
//!
//!        @return        the machine this object belongs to
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
// *    LastActivity
// *
// *******************************************************************************
//!
//!        @return
//!
// *******************************************************************************

REAL ePlayerNetID::LastActivity( void ) const
{
    return tSysTimeFloat() - lastActivity_;
}

// *******************************************************************************
// *
// *    ResetScoreDifferences
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

    eTeam::ResetScoreDifferences();
}

// *******************************************************************************
// *
// *    Suspend
// *
// *******************************************************************************
//!
//!  @param rounds number of rounds to suspend
//!
// *******************************************************************************

void ePlayerNetID::Suspend( int rounds )
{
    if ( rounds < 0 )
    {
        rounds = 0;
    }

    int & suspended = AccessSuspended();

    if ( suspended == rounds )
    {
        return;
    }

    suspended = rounds;

    if ( suspended == 0 )
    {
        sn_ConsoleOut( tOutput( "$player_no_longer_suspended", GetColoredName() ) );
        SetDefaultTeam();
    }
    else
    {
        sn_ConsoleOut( tOutput( "$player_suspended", GetColoredName(), suspended ) );
        SetTeam( NULL );
        if ( Object() && Object()->Alive() )
            Object()->Kill();
    }
}

// *******************************************************************************
// *
// *    LogScoreDifferences
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

    eTeam::LogScoreDifferences();
}

void ePlayerNetID::UpdateSuspensions() {
    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);

        int suspended = p->GetSuspended();

        // update suspension count
        if ( suspended > 0 )
        {
            if ( p->CurrentTeam() && !p->NextTeam() )
            {
                p->UpdateTeam();
            }
            else
            {
                p->Suspend( suspended - 1 );
            }
        }
    }
}

void ePlayerNetID::UpdateShuffleSpamTesters()
{
    if( sn_GetNetState() != nSERVER )
    {
        return;
    }

    for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID *p = se_PlayerNetIDs( i );
        p->GetShuffleSpam().Reset();
    }
}

// *******************************************************************************
// *
// *    AnalyzeTiming
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

void ePlayerNetID::AnalyzeTiming( REAL timing )
{
    // just delegate safely
    if( GetRefcount() > 0 )
    {
        tJUST_CONTROLLED_PTR< ePlayerNetID > keep( this );
        uncannyTimingDetector_.Analyze( timing, this );
    }
}

eUncannyTimingDetector::eUncannyTimingSettings::~eUncannyTimingSettings()
{
#ifdef DEBUG_X
#ifdef DEDICATED
    con << "Best ratio achieved for " << timescale*1000 << "ms stat: " << bestRatio << "\n";
#endif
#endif
}

REAL eUncannyTimingDetector::eUncannyTimingAnalysis::Analyze( REAL timing, eUncannyTimingSettings const & settings )
{
    if( timing < settings.timescale )
    {
        REAL increment = 1.0/turnsSoFar;
        if( turnsSoFar < settings.averageOverEvents )
        {
            turnsSoFar++;
        }
        
        // don't rate failed timings too much. The user may not even have
        // attempted to time something.
        if( timing < 0 )
        {
            increment /= 4;
        }

        // event falls into the buckets
        if( 2*timing < settings.timescale && timing > 0 )
        {
            // event falls into the 'good' bucket, count it
            accurateRatio += increment;
        }

        // let ratio decay
        accurateRatio /= 1+increment;
    }

    REAL ratio = accurateRatio/(1-accurateRatio);
    
    // keep stats
    if (ratio > settings.bestRatio)
    {
        settings.bestRatio = ratio;
    }

    REAL ret = (ratio-settings.goodHumanRatio)/(settings.maxGoodRatio-settings.goodHumanRatio);
    if( ret < 0 )
    {
        ret = 0;
    }
    return ret;
}

eUncannyTimingDetector::eUncannyTimingAnalysis::eUncannyTimingAnalysis()
: accurateRatio( .3 ), turnsSoFar(10)
{}

/* stats; ladle 37:
prematch:
[0] Best ratio achieved for 125ms stat: 0.653367, 591turns.
[0] Best ratio achieved for 62.5ms stat: 1.07667, 124turns.
[0] Best ratio achieved for 31.25ms stat: 0.464286, 43turns.
up to zealous assertion failure:
[0] Best ratio achieved for 125ms stat: 1.45674
[0] Best ratio achieved for 62.5ms stat: 1.08824
[0] Best ratio achieved for 31.25ms stat: 0.942842
proper, after assertions had been removed:
[0] Best ratio achieved for 125ms stat: 1.71929, 4686turns.
[0] Best ratio achieved for 62.5ms stat: 1.08824, 713turns.
[0] Best ratio achieved for 31.25ms stat: 0.952494, 163turns.
z-man, trying really hard:
[0] Best ratio achieved for 125ms stat: 4.71739
[0] Best ratio achieved for 62.5ms stat: 0.945886
[0] Best ratio achieved for 31.25ms stat: 0.444307
tst prematch, only counting enemy grinds:
[0] Best ratio achieved for 125ms stat: 6.04985
[0] Best ratio achieved for 62.5ms stat: 1.80428
[0] Best ratio achieved for 31.25ms stat: 0.819629
all grinds:
[0] Best ratio achieved for 125ms stat: 2.09805
[0] Best ratio achieved for 62.5ms stat: 2.32939
[0] Best ratio achieved for 31.25ms stat: 1
tst proper, enemies only: 
[0] Best ratio achieved for 125ms stat: 5.60549
[0] Best ratio achieved for 62.5ms stat: 1.01227
[0] Best ratio achieved for 31.25ms stat: 0.681035
all grinds:
[0] Best ratio achieved for 125ms stat: 1.72932
[0] Best ratio achieved for 62.5ms stat: 1.68816
[0] Best ratio achieved for 31.25ms stat: 0.750099
ladle41, hamar and partner team actions included:
[0] Best ratio achieved for 125ms stat: 23.7208
[0] Best ratio achieved for 62.5ms stat: 2.01171
[0] Best ratio achieved for 31.25ms stat: 0.90623
*/

static eUncannyTimingDetector::eUncannyTimingSettings 
se_uncannyTimingSettingsFast(1/32.0, 1, 1.5),
se_uncannyTimingSettingsMedium(1/16.0, 2, 4);
// se_uncannyTimingSettingsSlow(1/8.0, 7, 15);

static REAL se_Max( REAL a, REAL b )
{
    return a > b ? a : b;
}

eUncannyTimingDetector::eUncannyTimingDetector()
: dangerLevel( DangerLevel_Low )
{
}

// opportunity to tune timebot detection sensitivity
static REAL se_timebotSensitivity = 1.0;
static tSettingItem< REAL > se_timebotSensitivityConf( "TIMEBOT_SENSITIVITY", se_timebotSensitivity );

// different ways to react to timebot detection
enum eTimebotAction
{
    eTimebotAction_Nothing = 0, // do nothing
    eTimebotAction_Log     = 1, // log it
    eTimebotAction_NotifyModerator = 2, // notify moderators that happen to be online
    eTimebotAction_NotifyEveryone  = 3, // notify all players
    eTimebotAction_Kick  = 4            // kick the player
};
tCONFIG_ENUM( eTimebotAction );

static eTimebotAction se_timebotActionMedium = eTimebotAction_Log;
static eTimebotAction se_timebotActionHigh   = eTimebotAction_Log;
static eTimebotAction se_timebotActionMax    = eTimebotAction_Log;
static tSettingItem< eTimebotAction > se_timebotActionMediumConf( "TIMEBOT_ACTION_MEDIUM", se_timebotActionMedium );
static tSettingItem< eTimebotAction > se_timebotActionHighConf( "TIMEBOT_ACTION_HIGH", se_timebotActionHigh );
static tSettingItem< eTimebotAction > se_timebotActionMaxConf( "TIMEBOT_ACTION_MAX", se_timebotActionMax );

// severity of kick (for autobans)
static REAL se_timebotKickSeverity = 0.5;
static tSettingItem< REAL > se_timebotKickSeverityConf( "TIMEBOT_KICK_SEVERITY", se_timebotKickSeverity );

static void se_TimebotAction( eTimebotAction action, ePlayerNetID * player, char const * message )
{
    if (action == eTimebotAction_Nothing)
    {
        return;
    }
    // look up message
    tOutput m (message, player->GetName());
    switch (action)
    {
    case eTimebotAction_Kick:
        if( player->Owner() > 0 )
        {
            sn_KickUser( player->Owner(), m, se_timebotKickSeverity );
        }
        // no break on purpose.
    case eTimebotAction_NotifyEveryone:
        sn_ConsoleOut( m );
        break;
    case eTimebotAction_NotifyModerator:
#ifdef DEDICATED
        se_SecretConsoleOut( m, player, &se_cannotSeeConsole );
        break;
#endif
    case eTimebotAction_Log:
        con << m;
        break;
    case eTimebotAction_Nothing:
        break;
    }
}

//! analzye a timing event
void eUncannyTimingDetector::Analyze( REAL timing, ePlayerNetID * player )
{
    // ignore this system on the client itself
    if( sn_GetNetState() != nSERVER || se_timebotSensitivity <= 0 )
    {
        return;
    }

    // apply sensitivity
    timing /= se_timebotSensitivity;

    REAL maxUncanny = fast.Analyze( timing, se_uncannyTimingSettingsFast );
    maxUncanny = se_Max( maxUncanny, medium.Analyze( timing, se_uncannyTimingSettingsMedium ) );
    // maxUncanny = se_Max( maxUncanny, slow.Analyze( timing, se_uncannyTimingSettingsSlow ) );

    switch( dangerLevel )
    {
    case DangerLevel_Low:
        if( maxUncanny > .25 )
        {
            dangerLevel =DangerLevel_Medium;
            se_TimebotAction( se_timebotActionMedium, player, "$timebot_action_medium" );
        }
        break;
    case DangerLevel_Medium:
        if( maxUncanny > .5 )
        {
            dangerLevel = DangerLevel_High;
            se_TimebotAction( se_timebotActionHigh, player, "$timebot_action_high" );
        }
        else if( maxUncanny <= 0.01 )
        {
            dangerLevel = DangerLevel_Low;
        }
        break;
    case DangerLevel_High:
        if( maxUncanny > 1 )
        {
            dangerLevel = DangerLevel_Max;
            se_TimebotAction( se_timebotActionMax, player, "$timebot_action_max" );
        }
        else if( maxUncanny < .25 )
        {
            dangerLevel = DangerLevel_Medium;
        }
        break;
    case DangerLevel_Max:
        if( maxUncanny < .75 )
        {
            dangerLevel = DangerLevel_High;
        }
    }
}

// *******************************************************************************
// *
// *    LogScoreDifference
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

static eLadderLogWriter se_roundScoreWriter("ROUND_SCORE", true);

void ePlayerNetID::LogScoreDifference( void )
{
    if ( lastScore_ > IMPOSSIBLY_LOW_SCORE && IsHuman() )
    {
        int scoreDifference = score - lastScore_;
        lastScore_ = IMPOSSIBLY_LOW_SCORE;
        se_roundScoreWriter << scoreDifference << GetUserName();
        if ( currentTeam )
            se_roundScoreWriter << FilterName( currentTeam->Name() );
        se_roundScoreWriter.write();
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
        p->SetTeamChangeAllowed( allow );
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

//! accesses the suspension count
int & ePlayerNetID::AccessSuspended()
{
    return suspended_;
}

//! returns the suspension count
int ePlayerNetID::GetSuspended() const
{
    return suspended_;
}

static void se_FillServerSettings()
{
    nServerInfo::SettingsDigest & digest = *nCallbackFillServerInfo::ToFill();
    
    digest.minPlayTimeTotal_ = int(se_minPlayTimeTotal);
    digest.minPlayTimeOnline_ = int(se_minPlayTimeOnline);
    digest.minPlayTimeTeam_ = int(se_minPlayTimeTeam);

    digest.SetFlag( nServerInfo::SettingsDigest::Flags_AuthenticationRequired,
#ifdef KRAWALL_SERVER
                    se_accessLevelRequiredToPlay < tAccessLevel_Program ||
                    se_playAccessLevel < tAccessLevel_Program
#else
                    false
#endif
        );
}

static nCallbackFillServerInfo se_fillServerSettings(se_FillServerSettings);
