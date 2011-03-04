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

#include "gTeam.h"
#include "uMenu.h"
#include "ePlayer.h"
#include "eTeam.h"

// sets the spectator mode of a local player
static void SetSpectator( ePlayerNetID * player, bool spectate )
{
    for ( int i = MAX_PLAYERS-1; i>=0; --i )
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
    ePlayerNetID* 	player;
    eTeam*			team;
    bool            canJoin_;
public:
    gMenuItemPlayerTeam(uMenu *M,ePlayerNetID* p, eTeam* t, bool canJoin )
            : uMenuItem( M, tOutput("$team_menu_join_help") ),
            player ( p ),
            team ( t),
            canJoin_( canJoin )
    {
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0)
    {
        tOutput text( "$team_menu_join", team->Name() );
        DisplayTextSpecial( x, y, text, selected, alpha * ( canJoin_ ? 1 : ( selected ? .8 : .5 ) ) );
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
        DisplayTextSpecial( x, y, tOutput("$team_menu_create"), selected, alpha );
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
        DisplayTextSpecial( x, y, tOutput("$team_menu_spectate"), selected, alpha );
    }

    virtual void Enter()
    {
        SetSpectator( player, true );
        menu->Exit();
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
        int i;

        tOutput title;
        title.SetTemplateParameter( 1, player->GetName() );
        title << "$team_menu_player_title";
        uMenu playerMenu( title );
        tArray<uMenuItem*> items;

        if ( !player->IsSpectating() )
        {
            items[ items.Len() ] = tNEW( gMenuItemSpectate ) ( &playerMenu, player );
        }

        for ( i = eTeam::teams.Len()-1; i>=0; --i )
        {
            eTeam *team = eTeam::teams(i);
            if ( team != player->NextTeam() )
            {
                items[ items.Len() ] = tNEW( gMenuItemPlayerTeam ) ( &playerMenu, player, team, team->PlayerMayJoin( player )  );
            }
        }

        if ( player->IsSpectating() ||
            !( player->NextTeam() && player->NextTeam()->NumHumanPlayers() == 1 &&
               player->CurrentTeam() && player->CurrentTeam()->NumHumanPlayers() == 1 )
        )
        {
            items[ items.Len() ] = tNEW( gMenuItemNewTeam ) ( &playerMenu, player );
        }

        playerMenu.Enter();

        for ( i = items.Len()-1; i>=0; --i )
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

    for ( i = MAX_PLAYERS-1; i>=0; --i )
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



