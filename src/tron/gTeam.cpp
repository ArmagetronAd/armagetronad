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

#include "gTeam.h"
#include "uMenu.h"
#include "ePlayer.h"
#include "eTeam.h"

static tString se_TeamMenu_Team_Ok("0xffffff");
static tString se_TeamMenu_Team_Full("0xaaaaaa");
static tConfItem<tString> se_TeamMenu_Team_Ok_Cfg("TEAM_MENU_COLOR_TEAM_OK",
        "$team_menu_color_team_ok_help",
        se_TeamMenu_Team_Ok);
static tConfItem<tString> se_TeamMenu_Team_Full_Cfg("TEAM_MENU_COLOR_TEAM_FULL",
        "$team_menu_color_team_full_help",
        se_TeamMenu_Team_Full);

static tOutput* PrepareTeamText(tOutput* text, eTeam* team, const ePlayerNetID* player, const char* textTemplate)
{
    // Required for Spectators in display status item
    if (team==NULL)
    {
        text->SetTemplateParameter(1 , tOutput("$team_empty") );
        //TODO Easier detection of/cached number of spectators
        int spectators = 0;
        for (int i = se_PlayerNetIDs.Len()-1; i>=0; i--)
        {
            ePlayerNetID *p = se_PlayerNetIDs(i);
            if (p->CurrentTeam()==NULL)
                spectators++;
        }
        text->SetTemplateParameter(2 , spectators);
        text->SetTemplateParameter(3, "");
        *text << textTemplate;
        return text;
    }

    // Handling the $team_join template ;)
    text->SetTemplateParameter(1 , team->Name() );
    text->SetTemplateParameter(2 , team->NumPlayers() );
    if (team->PlayerMayJoin(player))
        text->SetTemplateParameter(3, se_TeamMenu_Team_Ok);
    else
        text->SetTemplateParameter(3, se_TeamMenu_Team_Full);
    *text << textTemplate << "0xffffff";
    return text;
}

// sets the spectator mode of a local player
static void SetSpectator( ePlayerNetID * player, bool spectate )
{
    for ( int i = MAX_PLAYERS; i>=0; --i )
    {
        if ( ePlayer::PlayerIsInGame(i))
        {
            ePlayer* localPlayer = ePlayer::PlayerConfig( i );
            ePlayerNetID* pni = localPlayer->netPlayer;
            if ( pni == player && localPlayer->spectate != spectate )
            {
                localPlayer->spectate = spectate;
                con << tOutput( spectate ? "$player_toggle_spectator_on" : "$player_toggle_spectator_off", localPlayer->name );
                ePlayerNetID::Update();
            }
        }
    }
}


class gMenuItemPlayerTeam: public uMenuItem
{
    tJUST_CONTROLLED_PTR< ePlayerNetID > 	player;
    tJUST_CONTROLLED_PTR< eTeam >			team;
public:
    gMenuItemPlayerTeam(uMenu *M,ePlayerNetID* p, eTeam* t )
            : uMenuItem( M, tOutput("$team_menu_join_help", t->Name())),
            player ( p ),
            team ( t)
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        tOutput text;
        PrepareTeamText(&text, team, player, "$team_menu_join");
        DisplayTextSpecial( x, y, text, selected, alpha );
    }

    virtual void Enter()
    {
        SetSpectator( player, false );
        player->SetTeamWish( team );
        menu->Exit();
    }
};

class gMenuItemNewTeam: public uMenuItem
{
    ePlayerNetID* 	player;
public:
    gMenuItemNewTeam( uMenu *M,ePlayerNetID* p )
            : uMenuItem( M, tOutput("$team_menu_create_help") ),
            player ( p )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        DisplayTextSpecial( x, y, se_TeamMenu_Team_Ok+(const char*)tOutput(+"$team_menu_create"), selected, alpha );
    }

    virtual void Enter()
    {
        SetSpectator( player, false );
        player->CreateNewTeamWish();
        menu->Exit();
    }
};

class gMenuItemSpectate: public uMenuItem
{
    ePlayerNetID* 	player;
public:
    gMenuItemSpectate( uMenu *M,ePlayerNetID* p )
            : uMenuItem( M, tOutput("$team_menu_spectate_help") ),
            player ( p )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        DisplayTextSpecial( x, y, se_TeamMenu_Team_Ok+(const char*)tOutput("$team_menu_spectate"), selected, alpha );
    }

    virtual void Enter()
    {
        player->SetTeamWish(NULL);
        menu->Exit();
    }
};

class gMenuItemAutoSelect: public uMenuItem
{
    ePlayerNetID* 	player;
public:
    gMenuItemAutoSelect( uMenu *M,ePlayerNetID* p )
            : uMenuItem( M, tOutput("$team_menu_auto_help") ),
            player ( p )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        DisplayTextSpecial( x, y, se_TeamMenu_Team_Ok+(const char*)tOutput("$team_menu_auto"), selected, alpha );
    }

    virtual void Enter()
    {
        player->SetDefaultTeam();
        menu->Exit();
    }
};

class gMenuItemSpacer: public uMenuItem
{
public:
    gMenuItemSpacer(uMenu *M)
            : uMenuItem( M, tOutput() )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
    }

    virtual bool IsSelectable()
    {
        return false;
    }
};

class gMenuItemStatus: public uMenuItem
{
    ePlayerNetID *player;
    bool currentTeam;
public:
    gMenuItemStatus(uMenu *M, ePlayerNetID *playerID, bool displayCurrentTeam)
            : uMenuItem( M, tOutput() ), player(playerID), currentTeam(displayCurrentTeam)
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        tOutput text;
        if  (currentTeam)
            PrepareTeamText(&text, player->CurrentTeam(), player, "$team_menu_info_current_team");
        else
            PrepareTeamText(&text, player->NextTeam(), player, "$team_menu_info_next_team");
        DisplayTextSpecial( x, y, text, false, alpha );
    }

    virtual bool IsSelectable()
    {
        return false;
    }
};

class gMenuItemPlayer: public uMenuItem
{
    ePlayerNetID* player;
public:
    gMenuItemPlayer(uMenu *M,ePlayerNetID* p,
                    const tOutput& help)
            : uMenuItem( M, help ),
            player ( p )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        DisplayTextSpecial( x, y, player->GetName(), selected, alpha );
    }

    virtual void Enter()
    {
        tOutput title;
        title.SetTemplateParameter( 1, player->GetName() );
        title << "$team_menu_player_title";
        uMenu playerMenu( title );
        tArray<uMenuItem*> items;

        if ( !player->IsSpectating() )
        {
            items[ items.Len() ] = tNEW( gMenuItemSpectate ) ( &playerMenu, player );
        }

        if ( player->NextTeam()!=NULL)
        {
            items[items.Len()] = tNEW(gMenuItemSpectate) (&playerMenu, player);
        }

        // first pass add teams who probably can't be joined
        for ( int i = 0; i<eTeam::teams.Len(); i++ )
        {
            eTeam *team = eTeam::teams(i);
            if ( team && team != player->NextTeam() && !team->PlayerMayJoin( player ) )
            {
                items[ items.Len() ] = tNEW( gMenuItemPlayerTeam ) ( &playerMenu, player, team );
            }
        }
        // second pass add teams who probably can be joined
        // Note: these will appear above the unjoinable ones.
        for ( int i = 0; i<eTeam::teams.Len(); i++ )
        {
            eTeam *team = eTeam::teams(i);
            if ( team && team != player->NextTeam() && team->PlayerMayJoin( player ))
                items[ items.Len() ] = tNEW( gMenuItemPlayerTeam ) ( &playerMenu, player, team );
        }

        if ( player->IsSpectating() ||
            !( player->NextTeam() && player->NextTeam()->NumHumanPlayers() == 1 &&
               player->CurrentTeam() && player->CurrentTeam()->NumHumanPlayers() == 1 )
        )
        {
            items[ items.Len() ] = tNEW( gMenuItemNewTeam ) ( &playerMenu, player );
        }

        items[items.Len()] = tNEW ( gMenuItemSpacer ) ( &playerMenu );
        items[items.Len()] = tNEW ( gMenuItemStatus ) ( &playerMenu, player, false);
        items[items.Len()] = tNEW ( gMenuItemStatus ) ( &playerMenu, player, true);

        playerMenu.Enter();

        for ( int i = items.Len()-1; i>=0; --i )
        {
            delete items(i);
        }
    }
};




// bring up the team selection menu
void gTeam::TeamMenu()
{
    int i;

    uMenu Menu( tOutput("$team_menu_title") );
    tArray<uMenuItem*> items;

    for ( i = MAX_PLAYERS; i>=0; --i )
    {
        if ( ePlayer::PlayerIsInGame(i))
        {
            ePlayer* player = ePlayer::PlayerConfig( i );
            tOutput help;
            help.SetTemplateParameter(1, player->Name() );
            help << "$team_menu_player_help";
            ePlayerNetID* pni = player->netPlayer;
            if ( pni )
                items[ items.Len() ] = tNEW( gMenuItemPlayer ) ( &Menu, pni, help );
        }
    }

    if ( items.Len() > 1 )
    {
        Menu.Enter();
    }
    else if ( items.Len() >= 1 )
    {
        items[0]->Enter();
    }

    for ( i = items.Len()-1; i>=0; --i )
    {
        delete items(i);
    }
}
