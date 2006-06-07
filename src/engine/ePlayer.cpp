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
#include "nAuthentification.h"
#include "tDirectories.h"
#include "eTeam.h"
#include "eVoter.h"
#include "tReferenceHolder.h"
#include "tRandom.h"
#include "nServerInfo.h"
#include "tRecorder.h"
#include "nConfig.h"
#include <time.h>

tCONFIG_ENUM( eCamMode );

tList<ePlayerNetID> se_PlayerNetIDs;
static ePlayer* se_Players = NULL;

static bool se_assignTeamAutomatically = true;
static tSettingItem< bool > se_assignTeamAutomaticallyConf( "AUTO_TEAM", se_assignTeamAutomatically );

static tReferenceHolder< ePlayerNetID > se_PlayerReferences;

class PasswordStorage
{
public:
    tString username;
    nKrawall::nScrambledPassword password;
    bool save;

    PasswordStorage(): save(false){};
};


static tArray<PasswordStorage> S_passwords;

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
            if (storage.save)
            {
                if (!first)
                    s << "\nPASSWORD\t";
                first = false;

                s << "1 ";
                nKrawall::WriteScrambledPassword(storage.password, s);
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
            storage.username.ReadLine(s);
            storage.save = true;
        }
    }
};

static tConfItemPassword p;

// password request menu
class eMenuItemPassword: public uMenuItemString
{
public:
    eMenuItemPassword(uMenu *M,tString &c):
    uMenuItemString(M,"$login_password_title","$login_password_help",c){}
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

            MyMenu()->Exit();
            return true;
        }
        //    else if (e.type==SDL_KEYDOWN &&
        //	     uActionGlobal::IsBreakingGlobalBind(e.key.keysym.sym))
        //      return su_HandleEvent(e);
        else
#endif
            return uMenuItemString::Event(e);
    }
};


static bool tr(){return true;}

static uMenu *S_login=NULL;

static void cancelLogin()
{
    if (S_login)
        S_login->Exit();
    S_login = NULL;
}

// password storage mode
int se_PasswordStorageMode = 0; // 0: store in memory, -1: don't store, 1: store on file
static tConfItem<int> pws("PASSWORD_STORAGE",
                          "$password_storage_help",
                          se_PasswordStorageMode);

static void PasswordCallback(tString& username,
                             const tString& message,
                             nKrawall::nScrambledPassword& scrambled,
                             bool failure)
{
    int i;

    // find the player with the given username:
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

    // try to find the username in the saved passwords:
    PasswordStorage *storage = NULL;
    for (i = S_passwords.Len()-1; i>=0; i--)
        if (p->name == S_passwords(i).username)
            storage = &S_passwords(i);

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

    // immediately return the stored password if it was not marked as wrong:
    if (!failure)
    {
        username = storage->username;
        memcpy(scrambled, storage->password, sizeof(nKrawall::nScrambledPassword));
        return;
    }
    else
        storage->username.Clear();

    // password was not stored. Request it from user:

    uMenu login(message, false);

    // password storage;
    tString password;


    eMenuItemPassword pw(&login, password);
    uMenuItemString us(&login, "$login_username","$login_username_help", p->name);

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

    uMenuItemFunction cl(&login, "$login_cancel", "$login_cancel_help", &cancelLogin);

    login.SetSelected(1);

    // force a small console while we are in here
    rSmallConsoleCallback cb(&tr);

    S_login = &login;
    login.Enter();

    // return username/scrambled password
    if (S_login)
    {
        username = p->name;
        nKrawall::ScramblePassword(password, scrambled);

        // clear the PW from memory
        for (i = password.Len()-2; i>=0; i--)
            password(i) = 'a';

        if (se_PasswordStorageMode >= 0)
        {
            storage->username = p->name;
            memcpy(storage->password, scrambled, sizeof(nKrawall::nScrambledPassword));
            storage->save = (se_PasswordStorageMode > 0);
        }
    }
    else // or log out
        sn_SetNetState(nSTANDALONE);

    S_login = NULL;
}



static void ResultCallback(const tString& usernameUnfiltered,
                           const tString& origUsername,
                           int user, bool success)
{
    int i;

    // filter the user name
    tString username;
    ePlayerNetID::FilterName( usernameUnfiltered, username );

    // record and playback result (required because on playback, a new
    // salt is generated and this way, a recoding does not contain ANY
    // exploitable information for password theft: the scrambled password
    // stored in the incoming network stream has an unknown salt value. )
    static char const * section = "AUTH";
    tRecorder::Playback( section, success );
    tRecorder::Record( section, success );

    if (success)
    {
        bool found = false;

        // authenticate the user that logged in and change his name
        for (i = se_PlayerNetIDs.Len()-1; i>=0 && !found; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            if (p->Owner() == user && p->GetUserName() == origUsername)
            {
                p->SetUserName(username);
                // right now, user name and player name should match. Take care of that.
                // the next line is scheduled to be removed for real authentication.
                p->SetName(username);
                p->Auth();
                found = true;
            }
        }

        // if the first step was not successful,
        // authenticate the player from that client with the new user name (maybe he renamed?)
        for (i = se_PlayerNetIDs.Len()-1; i>=0 && !found; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            if (p->Owner() == user && p->GetUserName() == username)
            {
                p->Auth();
                found = true;
            }
        }

        // last attempt: authenticate any player from that client.
        //!TODO: think about whether we actually want that.
        for (i = se_PlayerNetIDs.Len()-1; i>=0 && !found; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            if (p->Owner() == user)
            {
                p->SetUserName( username );
                p->Auth();
                found = true;
            }
        }

        // request other logins
        for (i = se_PlayerNetIDs.Len()-1; i>=0; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            p->IsAuth();
        }
    }
    else
    {
        if (sn_GetNetState() == nSERVER  && user != sn_myNetID )
            nAuthentification::RequestLogin(username, user, "$login_request_failed", true);
    }
}




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

























static char *default_instant_chat[]=
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
     NULL};




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
#include <Lmcons.h>
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

ePlayer::ePlayer(){
    nAuthentification::SetUserPasswordCallback(&PasswordCallback);
    nAuthentification::SetLoginResultCallback (&ResultCallback);

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
    for(i=CAMERA_SMART_IN;i>=0;i--){
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

static void se_DisplayChatLocally( ePlayerNetID* p, const tString& say )
{
#ifdef DEBUG_X
    if (strstr( say, "BUG" ) )
    {
        st_Breakpoint();
    }
#endif

    if ( p && !p->IsSilenced() )
    {
        //tColoredString say2( say );
        //say2.RemoveHex();
        tColoredString message;
        message << *p;
        message << tColoredString::ColorString(1,1,.5);
        message << ": " << say << '\n';

        con << message;
    }
}

static void se_DisplayChatLocallyClient( ePlayerNetID* p, const tString& message )
{
    if ( p && !p->IsSilenced() )
    {
        if (p->Object() && p->Object()->Alive())
            con << message << "\n";
        else {
            con << tOutput("$dead_console_decoration");
            con << message << "\n";
        }
    }
}

static nVersionFeature se_chatRelay( 3 );

//!todo: lower this number or increase network version before next release
static nVersionFeature se_chatHandlerClient( 6 );

// chat message from client to server
void handle_chat( nMessage& );
static nDescriptor chat_handler(200,handle_chat,"Chat");

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

    if (sender->CurrentTeam() == team) {
        // foo --> Teammates: some message here
        console << tColoredString::ColorString(1,1,.5) << " --> ";
        console << tColoredString::ColorString(team->R(),team->G(),team->B()) << "Teammates";
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
    tASSERT( team );
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

// sends a private message from sender to receiver
void se_SendPrivateMessage( ePlayerNetID const * sender, ePlayerNetID const * receiver, tString const & message )
{
    tASSERT( sender );
    tASSERT( receiver );

    // determine receiver client id
    int cid = receiver->Owner();

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
    tASSERT( team );
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

        // ( foo --> Teammates ) some message here
        if (sender->CurrentTeam() == team) {
            say << tColoredString::ColorString(1,1,.5) << " --> ";
            say << tColoredString::ColorString(team->R(),team->G(),team->B()) << "Teammates";
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

ePlayerNetID * CompareBufferToPlayerNames( const tString & current_buffer, int & num_matches ) {
    num_matches = 0;
    ePlayerNetID * match = 0;

    // run through all players and look for a match
    for ( int a = se_PlayerNetIDs.Len()-1; a>=0; --a ) {
        ePlayerNetID* toMessage = se_PlayerNetIDs(a);

        // exact match?
        if ( current_buffer == toMessage->GetUserName() )
        {
            num_matches = 1;
            return toMessage;
        }

        if ( Contains(current_buffer, toMessage->GetUserName())) {
            match= toMessage; // Doesn't matter that this is wrote over everytime, when we only have one match it will be there.
            num_matches+=1;
        }
    }

    // return result
    return match;
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

void handle_chat_admin_commands(tJUST_CONTROLLED_PTR< ePlayerNetID > &p, tString say){
    if  (say.StartsWith("/login")) {
        tString params("");
        if (say.StrPos(" ") == -1)
            return;
        else
            params = say.SubStr(say.StrPos(" ") + 1);

        //change this later to read from a password file or something...
        //or integrate it with auth if we ever get that done...
        if (params == sg_adminPass && sg_adminPass != "NONE") {
            p->beLoggedIn();
            p->setAccessLevel(1);
            sn_ConsoleOut("You have been logged in!\n",p->Owner());
            tString serverLoginMessage;
            serverLoginMessage << "Remote admin login for user \"" << p->GetUserName() << "\" accepted.\n";
            sn_ConsoleOut(serverLoginMessage, 0);
        }
        else
        {
            tString failedLogin;
            sn_ConsoleOut("Login denied!\n",p->Owner());
            failedLogin << "Remote admin login for user \"" << p->GetUserName();
            failedLogin << "\" using password \"" << params << "\" rejected.\n";
            sn_ConsoleOut(failedLogin, 0);
        }
    }
    else  if (say.StartsWith("/logout")) {
        if ( p->isLoggedIn() )
        {
            sn_ConsoleOut("You have been logged out!\n",p->Owner());
        }
        p->beNotLoggedIn();
        p->setAccessLevel(0);
    }
    else  if (say.StartsWith("/admin")) {
        if (!p->isLoggedIn())
        {
            sn_ConsoleOut("You are not logged in.\n",p->Owner());
            return;
        }

        tString params("");
        if (say.StrPos(" ") == -1)
            return;
        else
            params = say.SubStr(say.StrPos(" ") + 1);

        // install filter
        eAdminConsoleFilter consoleFilter( p->Owner() );

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
    {
        sn_ConsoleOut("Unknown chat command.\n",p->Owner());
    }
}
#endif

// time during which no repeaded chat messages are printed
static REAL se_alreadySaidTimeout=5.0;
static tSettingItem<REAL> se_alreadySaidTimeoutConf("SPAM_PROTECTION_REPEAT",
        se_alreadySaidTimeout);

// flag set to allow players to shuffle themselves up in a team
static bool se_allowShuffleUp=false;
static tSettingItem<bool> se_allowShuffleUpConf("TEAM_ALLOW_SHUFFLE_UP",
        se_allowShuffleUp);

void handle_chat(nMessage &m){
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
            // check if the player is owned by the sender to avoid cheating
            if( p->Owner() != m.SenderID() )
            {
                Cheater( m.SenderID() );
                nReadError();
                return;
            }

            // spam protection:
            REAL lengthMalus = say.Len() / 20.0;
            if ( lengthMalus > 4.0 )
            {
                lengthMalus = 4.0;
            }

            // check if the player already said the same thing not too long ago
            for (short c = 0; c < p->lastSaid.Len(); c++) {
                if( (say.StripWhitespace() == p->lastSaid[c].StripWhitespace()) && ( (currentTime - p->lastSaidTimes[c]) < se_alreadySaidTimeout) ) {
                    sn_ConsoleOut( tOutput("$spam_protection_repeat", say ), m.SenderID() );
                    return;
                }
            }

            // update last said record
            {
                for( short zz = 12; zz>=1; zz-- )
                {
                    p->lastSaid[zz] = p->lastSaid[zz-1];
                    p->lastSaidTimes[zz] = p->lastSaidTimes[zz-1];
                }

                p->lastSaid[0] = say;
                p->lastSaidTimes[0] = currentTime;
            }

            nSpamProtection::Level spamLevel = p->chatSpam_.CheckSpam( 1.0f + lengthMalus, m.SenderID() );
            bool pass = false;

            if (say.StartsWith("/")) {
                std::string sayStr(say);
                std::istringstream s(sayStr);
                tString command;
                s >> command;
                tString msg;
                tConfItemBase::EatWhitespace(s);
                msg.ReadLine(s);
                if (command == "/me") {
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
                    return;
                }
                else if (command == "/teamname") {
					tString teamname(((const char*)say)+6);
					p->SetTeamname(teamname);
					return;
                }
                else if (command == "/teamleave") {
                    if ( se_assignTeamAutomatically )
                    {
                        sn_ConsoleOut("Sorry, does not work with automatic team assignment.\n", p->Owner() );
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
                    return;
                }
                else if (command == "/teamshuffle" || command == "/shuffle") {
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

                    if ( !se_allowShuffleUp && IDWish < IDNow )
                    {
                        sn_ConsoleOut(tOutput("$player_noshuffleup"), p->Owner());
                        return;
                    }

                    if( IDNow == IDWish )
                    {
                        sn_ConsoleOut(tOutput("$player_noshuffle"), p->Owner());
                    }

                    p->CurrentTeam()->Shuffle( IDNow, IDWish );
                    return;
                }
                // Send a message to your team
                else if (command == "/team") {
                    eTeam *currentTeam = p->CurrentTeam();

                    if (currentTeam != NULL) // If a player has just joined the game, he is not yet on a team. Sending a /team message will crash the server
                    {
                        // Log message to server and sender
                        tColoredString messageForServerAndSender = se_BuildChatString(currentTeam, p, msg);
                        messageForServerAndSender << "\n";
                        sn_ConsoleOut(messageForServerAndSender, 0);
                        sn_ConsoleOut(messageForServerAndSender, p->Owner());

                        // Send message to team-mates
                        int numTeamPlayers = currentTeam->NumPlayers();
                        for (int teamPlayerIndex = 0; teamPlayerIndex < numTeamPlayers; teamPlayerIndex++) {
                            if (currentTeam->Player(teamPlayerIndex)->Owner() != p->Owner()) // Do not resend the message to yourself
                                se_SendTeamMessage(currentTeam, p, currentTeam->Player(teamPlayerIndex), msg);
                        }
                    }
                    else
                    {
                        sn_ConsoleOut(tOutput("$player_not_on_team"), p->Owner());
                    }

                    return;
                }
                else if (command == "/msg") {
                    size_t current_place=0; // current place in buffer_name.

                    // search for end of recipient and store recipient in buffer_name
                    tString buffer_name;
                    while (current_place < msg.size() && !isspace(msg[current_place])) {
                        buffer_name+=msg[current_place];
                        current_place++;
                    }
                    buffer_name = ePlayerNetID::FilterName(buffer_name);

                    // Check for match
                    int num_matches=-1;
                    ePlayerNetID * receiver = CompareBufferToPlayerNames(buffer_name, num_matches);

                    // One match, send it.
                    if (num_matches == 1) {
                        // extract rest of message: it is the true message to send
                        tString msg_core;
                        while (current_place < msg.size() - 1) {
                            current_place++;
                            msg_core += msg[current_place];
                        }

                        // build chat string
                        tColoredString toServer = se_BuildChatString( p, receiver, msg_core );
                        toServer << '\n';

                        // log locally
                        sn_ConsoleOut(toServer,0);

                        // log to sender's console
                        sn_ConsoleOut(toServer, p->Owner());

                        // send to receiver
                        if ( p->Owner() != receiver->Owner())
                            se_SendPrivateMessage( p, receiver, msg_core );
                        return;
                    }
                    // More than than one match for the current buffer. Complain about that.
                    else if (num_matches > 1) {
                        tOutput toSender;
                        toSender.SetTemplateParameter(1, buffer_name);
                        toSender << "$msg_toomanymatches";
                        sn_ConsoleOut(toSender,p->Owner());
                        return;
                    }
                    // 0 matches. The user can't spell. Complain about that, too.
                    else {
                        tOutput toSender;
                        toSender.SetTemplateParameter(1, buffer_name);
                        toSender << "$msg_nomatch";
                        sn_ConsoleOut(toSender, p->Owner());
                        return;
                    }
                }
#ifdef DEDICATED
                else if (command == "/players") {
                    for ( int i2 = se_PlayerNetIDs.Len()-1; i2>=0; --i2 )
                    {
                        ePlayerNetID* p2 = se_PlayerNetIDs(i2);
                        tString tos;
                        tos << p2->Owner();
                        tos << ": ";
                        tos << p2->GetUserName();
                        if (p2->isLoggedIn())
                            tos << " (logged in)";
                        else
                            tos << " (logged out)";
                        tos << "\n";
                        sn_ConsoleOut(tos, p->Owner());
                    }
                    return;
                }
                else {
                    handle_chat_admin_commands(p, say);
                    return;
                }
#endif
            }

            if ( spamLevel < nSpamProtection::Level_Mild && say.Len() <= se_SpamMaxLen+2 && ( !p->IsSilenced() ) && pass != true )
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


static std::deque<tString> se_chatHistory; // global since the class doesn't live beyond the execution of the command
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
        if(pos - len == 0)
            actualString = match + ": ";
        else
            actualString = match + " ";
        return DoCompletion(string, pos, len, actualString);
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
    uMenuItemStringWithHistory(M,"$chat_title_text","",c, se_SpamMaxLen, se_chatHistory, se_chatHistoryMaxSize, completer),me(Me) {}

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

static int IMPOSSIBLY_LOW_SCORE=(-1 << 31);

static nSpamProtectionSettings se_chatSpamSettings( 1.0f, "SPAM_PROTECTION_CHAT", tOutput("$spam_protection") );

ePlayerNetID::ePlayerNetID(int p):nNetObject(),listID(-1), teamListID(-1)
        ,registeredMachine_(0), pID(p),chatSpam_( se_chatSpamSettings )
{
    favoriteNumberOfPlayersPerTeam = 1;

    nameTeamAfterMe = false;

    r = g = b = 15;

    greeted				= true;
    auth				= true;
    chatting_			= false;
    spectating_         = false;
    chatFlags_			= 0;
    disconnected		= false;

    if (p>=0){
        ePlayer *P = ePlayer::PlayerConfig(p);
        if (P){
            SetName( P->Name() );
			teamname = P->Teamname();
            r=   P->rgb[0];
            g=   P->rgb[1];
            b=   P->rgb[2];
            pingCharity=::pingCharity;
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

    //Remote Addmin add-ins...
    loggedIn = false;
    accessLevel = 0;

    MyInitAfterCreation();
}




ePlayerNetID::ePlayerNetID(nMessage &m):nNetObject(m),listID(-1), teamListID(-1)
        , registeredMachine_(0), chatSpam_( se_chatSpamSettings )
{
    greeted     =false;
    chatting_   =false;
    spectating_ =false;
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

    if(sn_GetNetState()==nSERVER)
        RequestSync();

    score=0;
    lastScore_=IMPOSSIBLY_LOW_SCORE;
    // rubberstatus=0;

    //Remote Addmin add-ins...
    loggedIn = false;
    accessLevel = 0;

#ifdef KRAWALL_SERVER
    auth=false;
#else
    auth=true;
#endif
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

static bool se_SilenceAll = false;		// flag indicating whether everyone should be silenced

static tSettingItem<bool> se_silAll("SILENCE_ALL",
                                    se_SilenceAll);

static int se_maxPlayersPerIP = 4;
static tSettingItem<int> se_maxPlayersPerIPConf( "MAX_PLAYERS_SAME_IP", se_maxPlayersPerIP );

void ePlayerNetID::MyInitAfterCreation()
{
    this->CreateVoter();

    this->silenced_ = se_SilenceAll;

    this->UpdateName();

    // register with machine and kick user if too many players are present
    if ( Owner() != 0 && sn_GetNetState() == nSERVER )
    {
        this->RegisterWithMachine();
        if ( se_maxPlayersPerIP < nMachine::GetMachine(Owner()).GetPlayerCount() )
        {
            // kill them
            sn_DisconnectUser( Owner(), "$network_kill_too_many_players" );
        }
    }

    this->wait_ = 0;

    this->lastActivity_ = tSysTimeFloat();
}

void ePlayerNetID::InitAfterCreation()
{
    MyInitAfterCreation();

    if (sn_GetNetState() == nSERVER)
    {
#ifndef KRAWALL_SERVER
        GetScoreFromDisconnectedCopy();
#endif
		SetDefaultTeam();
    }
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
            if ( IsSpectating() )
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
    UnregisterWithMachine();

    tControlledPTR< ePlayerNetID > holder( this );

    //	else
    {
        this->RemoveFromGame();

        se_PlayerNetIDs.Remove(this,listID);

        return true;
    }
}

void ePlayerNetID::ActionOnDelete()
{
    UnregisterWithMachine();

    tControlledPTR< ePlayerNetID > holder( this );

    this->RemoveFromGame();

    se_PlayerNetIDs.Remove(this,listID);
}

void ePlayerNetID::PrintName(tString &s) const
{
    s << "ePlayerNetID nr. " << ID() << ", name " << GetName();
}


bool ePlayerNetID::AcceptClientSync() const{
    return true;
}

void ePlayerNetID::Auth(){
    auth = true;
    GetScoreFromDisconnectedCopy();
}

bool ePlayerNetID::IsAuth() const{
#ifdef KRAWALL_SERVER
    if (!auth && sn_GetNetState() == nSERVER && Owner() != sn_myNetID )
        nAuthentification::RequestLogin(GetUserName(), Owner(), "$login_request_first");
#endif

    return auth;
}

// create our voter or find it
void ePlayerNetID::CreateVoter()
{
    // only count nonlocal players with voting support as voters
    if ( sn_GetNetState() != nCLIENT && this->Owner() != 0 && sn_Connections[ this->Owner() ].version.Max() >= 3 )
    {
        this->voter_ = eVoter::GetVoter( this->Owner() );
    }
}

void ePlayerNetID::WriteSync(nMessage &m){
    lastSync=tSysTimeFloat();
    nNetObject::WriteSync(m);
    m.Write(r);
    m.Write(g);
    m.Write(b);
    m.Write(pingCharity);
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

    // pack chat and spectator status together
    unsigned short flags = ( chatting_ ? 1 : 0 ) | ( spectating_ ? 2 : 0 );
    m << flags;

    m << score;
    m << static_cast<unsigned short>(disconnected);

    m << nextTeam;
    m << currentTeam;

    m << favoriteNumberOfPlayersPerTeam;
    m << nameTeamAfterMe;
	m << teamname;
}

// makes sure the passed string is not longer than the given maximum
static void se_CutString( tColoredString & string, int maxLen )
{
    if (string.Len() > maxLen )
    {
        string.SetLen(maxLen);
        string[string.Len()-1]='\0';
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
static bool se_IsSpace( char c )
{
    return isspace( c );
}

// enf of player names should neither be space or :
static bool se_IsInvalidNameEnd( char c )
{
    return isspace( c ) || c == ':' || c == '.';
}

// filter name ends
static void se_StripNameEnds( tString & name )
{
    se_StripMatchingEnds( name, se_IsSpace, se_IsInvalidNameEnd );
}

// test if a user name is used by anyone else than the passed player
static bool se_IsNameTaken( tString const & name, ePlayerNetID const * exception )
{
    for (int i = se_PlayerNetIDs.Len()-1; i >= 0; --i )
    {
        ePlayerNetID * player = se_PlayerNetIDs(i);
        if ( player != exception )
        {
            if ( name == player->GetUserName() )
                return true;
        }
    }

    return false;
}

static bool se_allowImposters = false;
static tSettingItem< bool > se_allowImposters1( "ALLOW_IMPOSTERS", se_allowImposters );
static tSettingItem< bool > se_allowImposters2( "ALLOW_IMPOSTORS", se_allowImposters );

static bool se_filterColorNames=false;
tSettingItem< bool > se_coloredNamesConf( "FILTER_COLOR_NAMES", se_filterColorNames );
static bool se_stripNames=true;
tSettingItem< bool > se_stripConf( "FILTER_NAME_ENDS", se_stripNames );

void ePlayerNetID::ReadSync(nMessage &m){
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

    // name as sent from the other end
    tString & remoteName = ( sn_GetNetState() == nCLIENT ) ? nameFromServer_ : nameFromClient_;

    // name handling
    {
        tString oldName = remoteName;

        // read and shorten name, but don't update it yet
        m >> remoteName;

        // filter colors
        if ( se_filterColorNames )
            remoteName = tColoredString::RemoveColors( remoteName );

        // strip whitespace
        if ( se_stripNames )
            se_StripNameEnds( remoteName );

        se_CutString( remoteName, 16 );

        // zero strings or color code only strings are illegal
        if ( !IsLegalPlayerName( remoteName ) )
        {
            // revert to old name if possible
            if ( IsLegalPlayerName( oldName ) )
            {
                remoteName = oldName;
            }
            else
            {
                // or replace it by a default value
                remoteName = "Player 1";
            }
        }
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

            if ( chatting_ != newChat || spectating_ != newSpectate )
                lastActivity_ = tSysTimeFloat();
            chatting_   = newChat;
            spectating_ = newSpectate;
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

// message of day presented to clients logging in
tString se_greeting("");
static tConfItemLine a_mod("MESSAGE_OF_DAY",se_greeting);

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
        if (se_greeting.Len()>1)
            s << se_greeting << "\n";
        s << "\n";
        //std::cout << s;
        sn_ConsoleOut(s,Owner());
        greeted=true;
    }
}

eNetGameObject *ePlayerNetID::Object() const{
    return object;
}

void se_SaveToLadderLog( tOutput const & out )
{
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
        RankingGraph( y , maxPlayers, showTeam );
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
float ePlayerNetID::RankingGraph( float y, int MAX, bool showTeam ){
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
	if (showTeam) {
	    tColoredString team;
	    team << tOutput("$player_scoretable_team");
	    DisplayText(.3, y, .06, team.c_str(), sr_fontScoretable, -1);
	}
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
                if ( p->currentTeam && showTeam )
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



tColoredString & operator << (tColoredString &s,const ePlayer &p){
    return s << tColoredString::ColorString(p.rgb[0]/15.0,
                                            p.rgb[1]/15.0,
                                            p.rgb[2]/15.0)
           << p.Name();
}

tColoredString & operator << (tColoredString &s,const ePlayerNetID &p){
    return s << p.GetColoredName();
}

void ePlayerNetID::ClearAll(){
    for(int i=MAX_PLAYERS-1;i>=0;i--){
        ePlayer *local_p=ePlayer::PlayerConfig(i);
        if (local_p)
            local_p->netPlayer = NULL;
    }
}

void ePlayerNetID::CompleteRebuild(){
    ClearAll();
    Update();
}

static bool se_VisubleSpectatorsSupported()
{
    static nVersionFeature se_visibleSpectator(13);
    return sn_GetNetState() != nCLIENT || se_visibleSpectator.Supported(0);
}

// Update the netPlayer_id list
void ePlayerNetID::Update(){
#ifdef DEDICATED
    if (sr_glOut)
#endif
    {
        for(int i=0; i<MAX_PLAYERS; ++i ){
            bool in_game=ePlayer::PlayerIsInGame(i);
            ePlayer *local_p=ePlayer::PlayerConfig(i);
            tASSERT(local_p);
            tCONTROLLED_PTR(ePlayerNetID) &p=local_p->netPlayer;

            if (!p && in_game && ( !local_p->spectate || se_VisubleSpectatorsSupported() ) ) // insert new player
            {
                p=tNEW(ePlayerNetID) (i);
                p->SetDefaultTeam();
                p->RequestSync();
            }

            if (bool(p) && (!in_game || ( local_p->spectate && !se_VisubleSpectatorsSupported() ) ) && // remove player
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
                p->r=ePlayer::PlayerConfig(i)->rgb[0];
                p->g=ePlayer::PlayerConfig(i)->rgb[1];
                p->b=ePlayer::PlayerConfig(i)->rgb[2];
                p->pingCharity=::pingCharity;
				p->SetTeamname(local_p->Teamname());

                // update spectator status
                if ( p->spectating_ != local_p->spectate )
                    p->RequestSync();
                p->spectating_ = local_p->spectate;

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
    // update the ping charity
    int old_c=sn_pingCharityServer;
    sn_pingCharityServer=::pingCharity;
#ifndef DEDICATED
    if (sn_GetNetState()==nCLIENT)
#endif
        sn_pingCharityServer+=100000;

    int i;
    for(i=se_PlayerNetIDs.Len()-1;i>=0;i--){
        ePlayerNetID *pni=se_PlayerNetIDs(i);
        pni->UpdateName();
        int new_ps=pni->pingCharity;
        new_ps+=int(pni->ping*500);

        if (new_ps< sn_pingCharityServer)
            sn_pingCharityServer=new_ps;
    }
    if (sn_pingCharityServer<0)
        sn_pingCharityServer=0;
    if (old_c!=sn_pingCharityServer)
    {
        tOutput o;
        o.SetTemplateParameter(1, old_c);
        o.SetTemplateParameter(2, sn_pingCharityServer);
        o << "$player_pingcharity_changed";
        con << o;
    }

    // update team assignment
    bool assigned = true;
    while ( assigned )
    {
        assigned = false;
        for(i=se_PlayerNetIDs.Len()-1;i>=0;i--)
        {
            ePlayerNetID* player = se_PlayerNetIDs(i);

            // only assign new team if it is possible
            if ( player->NextTeam() != player->CurrentTeam() &&
                    ( !player->NextTeam() || player->NextTeam()->PlayerMayJoin( player ) )
               )
            {
                player->UpdateTeam();
                if ( player->NextTeam() == player->CurrentTeam() )
                    assigned = true;
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
        if (pni->disconnected && pni->GetName() == GetName() && pni->Owner() == 0)
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

// put a new player into a default team
void ePlayerNetID::SetDefaultTeam( )
{
    // only the server should do this, the client does not have the full information on how to do do it right.
    if ( sn_GetNetState() == nCLIENT || !se_assignTeamAutomatically || spectating_ )
        return;

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
//    if ( !IsHuman() )
//    {
//        SetTeam( NULL );
//        return;
//    }
    // find the team with the least number of players on it
    eTeam *min = NULL;
    for ( int i=eTeam::teams.Len()-1; i>=0; --i )
    {
        eTeam *t = eTeam::teams( i );
        if ( t->IsHuman()  && ( !min || min->NumHumanPlayers() > t->NumHumanPlayers() ) )
            min = t;
    }

    if ( min &&
            eTeam::teams.Len() >= eTeam::minTeams &&
            min->PlayerMayJoin( this ) &&
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

// create a new team and join it (on the server)
void ePlayerNetID::CreateNewTeam()
{
    // check if the team change is legal
    tASSERT ( nCLIENT !=  sn_GetNetState() );

    if ( !eTeam::NewTeamAllowed() ||
            ( bool( currentTeam ) && ( currentTeam->NumHumanPlayers() == 1 ) ) ||
            IsSpectating() )
    {
        tOutput message;
        message.SetTemplateParameter(1, GetName() );
        message << "$player_nocreate_team";

        sn_ConsoleOut( message, Owner() );

// TODO Check
        if ( !currentTeam )
            SetDefaultTeam();
// Shouldn't CreateNewTeam just create a team or fail ?
// if it can't create a team => just idle?
//         //If team==NULL then the player joins NO team (and perhaps can join one later)
//	       SetTeamWish(FindDefaultTeam());

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
            CreateNewTeam();

            break;
        }
    case TEAMCHANGE:
        {
            eTeam *newTeam;

            m >> newTeam;

            // NULL team probably means that the change target does not
            // exist any more. Create a new team instead.
            if ( !newTeam )
            {
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
                    chatSpam_.CheckSpam( 4.0, Owner() );
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
static unsigned short se_ReadUser( std::istream &s )
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
        // compare the read name with the players' user names
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);
            if ( p && p->GetUserName() == name )
            {
                int owner = p->Owner();
                if ( owner > 0 )
                {
                    return owner;
                }

                con << tOutput( "$network_kick_notfound", name );
            }
        }
    }

    return 0;
}

static void se_KickConf(std::istream &s)
{
    // get user ID
    int num = se_ReadUser( s );

    tString reason;
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


static void se_BanConf(std::istream &s)
{
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

static void Kill_conf(std::istream &s)
{
    // read name of player to be killed
    tString name;
    name.ReadLine( s );

    int i;
    for ( i = se_PlayerNetIDs.Len()-1; i>=0; --i )
    {
        ePlayerNetID* p = se_PlayerNetIDs(i);

        // check whether it's p who should be killed by comparing the name.
        if ( p->GetUserName() == name )
        {
            // kill the player's game object
            if ( p->Object() )
                p->Object()->Kill();

            return;
        }
    }

    int num;
    bool isNum = name.Convert( num );
    if ( isNum )
        for ( int i = se_PlayerNetIDs.Len()-1; i>=0; --i )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);

            // check whether it's p who should be killed,
            // by comparing the owner.
            if ( p->Owner() == num )
            {
                // kill the player's game object
                if ( p->Object() )
                    p->Object()->Kill();

                return;
            }
        }

    tOutput o;
    o.SetTemplateParameter( 1, name );
    o << "$network_kick_notfound";
    con << o;
}

static tConfItemFunc kill_conf("KILL",&Kill_conf);

static void players_conf(std::istream &s)
{
    /*    for ( int i=0; i < se_PlayerNetIDs.Len(); i++ )
        {
            ePlayerNetID* p = se_PlayerNetIDs(i);
            tOutput o;
            o << "Player " << p->Owner() << ": " << p->name;
            if (p->isLoggedIn())
                o << " (logged in)\n";
            else
                o << " (logged out)\n";
            con << o;
        } */
    for ( int i2 = se_PlayerNetIDs.Len()-1; i2>=0; --i2 )
    {
        ePlayerNetID* p2 = se_PlayerNetIDs(i2);
        tOutput tos;
        tos << p2->Owner();
        tos << ": ";
        tos << p2->GetUserName();
        if (p2->isLoggedIn())
            tos << " (logged in)";
        else
            tos << " (logged out)";
        tos << "\n";
        con << tos;
    }
}

static tConfItemFunc players("PLAYERS",&players_conf);

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
    // store old name for password re-request and name change message
    tColoredString oldprintname;
    tString oldUserName( GetUserName() );
    oldprintname << *this << tColoredString::ColorString(.5,1,.5);

    // apply client change, stripping excess spaces
    if ( sn_GetNetState() != nCLIENT )
    {
        if ( se_stripNames )
            se_StripNameEnds( nameFromClient_ );

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
    tString newUserName;
    FilterName( newName, newUserName );

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

            FilterName( testName, newUserName );

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

    if ( name_ != newName )
    {
        // copy it to the name, removing colors of course
        name_ = newName;

        // remove spaces and possibly other nasties for the user name
        FilterName( name_, userName_ );

        if (sn_GetNetState()!=nCLIENT)
        {
            // sync the new name
            RequestSync();

            tOutput mess;

            tColoredString printname;
            printname << *this << tColoredString::ColorString(.5,1,.5);

            mess.SetTemplateParameter(1, printname);
            mess.SetTemplateParameter(2, oldprintname);

            if (oldUserName.Len()<=1 && GetUserName().Len()>=1){

#ifdef KRAWALL_SERVER
                if (sn_GetNetState() == nSERVER && Owner() != sn_myNetID )
                {
                    auth = false;
                    nAuthentification::RequestLogin(GetUserName(), Owner(), "$login_request_first");
                }
#endif

                // print spectating join message (regular join messages are handled by eTeam)
                if ( IsSpectating() )
                {
                    mess << "$player_entered_spectator";
                    sn_ConsoleOut(mess);
                }

                if ( IsHuman() )
                {
                    tString ladder;
                    ladder << "PLAYER_ENTERED " << userName_ << " " << nMachine::GetMachine(Owner()).GetIP() << "\n";
                    se_SaveToLadderLog(ladder);

                    Greet();
                }
            }
            else if (strcmp(oldUserName,GetUserName()))
            {
#ifdef KRAWALL_SERVER
                if (sn_GetNetState() == nSERVER && Owner() != sn_myNetID )
                {
                    nAuthentification::RequestLogin(GetUserName(), Owner(), "$login_request_namechange");
                    auth = false;
                    userName_ = oldUserName; // restore the old name until the new one is authenticated
                }
#endif
                mess << "$player_renamed";
                sn_ConsoleOut(mess);

                {
                    tString ladder;
                    ladder << "PLAYER_RENAMED " <<  oldUserName << " "  << userName_ << " " << nMachine::GetMachine(Owner()).GetIP() << "\n";
                    se_SaveToLadderLog(ladder);
                }
            }
        }
    }
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
            ret << " " << FilterName( currentTeam->Name() );
        ret << "\n";
        se_SaveToLadderLog( ret );
    }
}


