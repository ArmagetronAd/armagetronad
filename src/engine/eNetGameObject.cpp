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

#include "eNetGameObject.h"
#include "ePlayer.h"
#include "eLagCompensation.h"
#include "eTimer.h"
#include "uInput.h"
#include "eGrid.h"
#include "eTeam.h"
#include "eTess2.h"

#include "eNetGameObject.pb.h"

#define MAX_PING_OVERFLOW 3
#define PING_OVERFLOW_TS .1

// determines the effective ping of a player, taking ping charity into account
static REAL se_GetPing( ePlayerNetID * player )
{
    REAL peer_ping=0;
    REAL my_ping=0;

    if (bool(player) && player->Owner()!=::sn_myNetID)
    {
        if (sn_GetNetState()!=nSERVER)
            peer_ping+=player->ping;
        else
            peer_ping+=sn_Connections[player->Owner()].ping;
    }
    if (sn_GetNetState()!=nSERVER && (!player || player->Owner()!=::sn_myNetID))
        my_ping+=sn_Connections[0].ping;

    REAL ping = (peer_ping+my_ping)*.5;

    if (ping>my_ping+sn_pingCharityServer*.001)
        ping=my_ping+sn_pingCharityServer*.001;

    if (ping<my_ping-sn_pingCharityServer*.001)
        ping=my_ping-sn_pingCharityServer*.001;

    return ping;
}

void eNetGameObject::MyInitAfterCreation(){
    laggometer=laggometerSmooth=0;

    if (player){
        player->ControlObject(this);
    }
    clientside_action();

    pingOverflow=0;
}

void eNetGameObject::InitAfterCreation(){
    nNetObject::InitAfterCreation();
    MyInitAfterCreation();
}

eNetGameObject::eNetGameObject(eGrid *grid, const eCoord &pos,const eCoord &dir,
                               ePlayerNetID* p,bool autodelete)
        :eGameObject(grid, pos,dir,NULL,autodelete),
nNetObject(p->Owner()),player(p){
    lastClientsideAction=0;
    if (sn_GetNetState()!=nCLIENT)
        RequestSync();
    MyInitAfterCreation();
}



eNetGameObject::eNetGameObject( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender )
        :eGameObject(eGrid::CurrentGrid(), eCoord(0,0), eCoord(0,0), NULL),
nNetObject( sync.base(), sender ){
    tASSERT(grid);

    lastClientsideAction=0;
    unsigned short pid = sync.player_id();
    player=static_cast<ePlayerNetID *>(Object(pid));
    autodelete = sync.autodelete();

    laggometerSmooth=laggometer=0;

    pingOverflow=0;
}

void eNetGameObject::DoRemoveFromGame(){
    // let the object get deleted on exit if nobody else is interested
    tControlledPTR< eNetGameObject > bounce;
    if ( this->GetRefcount() >= 0 )
        bounce = this;

    // unregister from player
    if ( player && this == player->object )
    {
        player->object = NULL;
    }

    team = NULL;
}

eNetGameObject::~eNetGameObject(){
    if (player){
#ifdef DEBUG
        //con << "Player " << ePlayer->name << " controls no object.\n";
#endif
        if (player->object==this){
            player->object=NULL;
        }
    }
}



bool eNetGameObject::SyncIsNew(  Engine::NetGameObjectSync const & sync, nSenderInfo const & sender )
{
    bool ret = nNetObject::SyncIsNew( sync.base(), sender );
    
    lastAttemptedSyncTime = sync.last_time();

#ifdef DEBUG
    if (Owner()==::sn_myNetID && lastAttemptedSyncTime>lastClientsideAction+100){
        con << "Warning! time overflow!\n";
    }
#endif

    return ret;
}

void eNetGameObject::AddRef()
{
    nNetObject::AddRef();
}

void eNetGameObject::Release()
{
    nNetObject::Release();
}

// control functions:
void eNetGameObject::ReceiveControlNet( Network::NetObjectControl const & controlBase )
{
    tASSERT( controlBase.HasExtension( Engine::net_game_object_control ) );
    Engine::NetGameObjectControl const & control = controlBase.GetExtension( Engine::net_game_object_control );

    REAL time = control.time();
    unsigned short act_id = control.action_id();
    REAL x = control.action_level();

    REAL backdate=Lag();//sn_ping[m.SenderID()]*.5;
    if (backdate>sn_pingCharityServer*.001)
        backdate=sn_pingCharityServer*.001;

    REAL mintime=se_GameTime()-backdate*1.5-.1;
    if (time<mintime){
        con << "mintime\n";
        REAL pov_needed=mintime-time;
        if (pov_needed+pingOverflow > MAX_PING_OVERFLOW*backdate){
            pov_needed = MAX_PING_OVERFLOW*backdate-pingOverflow;
            con << "Mintime\n";
        }
        if (pov_needed<0)
            pov_needed=0;

        time=mintime-pov_needed;
        pingOverflow+=pov_needed;
    }

    if (time>se_GameTime()+1)
        time=se_GameTime()+1;

    uActionPlayer *Act=uActionPlayer::Find(act_id);

    ReceiveControl(time,Act,x);
}


void eNetGameObject::SetPlayer(ePlayerNetID* a_player)
{
    tASSERT( !a_player || Owner() == player->Owner() );
    player  = a_player;
    if ( laggometerSmooth == 0 && sn_GetNetState() != nCLIENT )
        laggometerSmooth = laggometer = se_GetPing( player );
}

void eNetGameObject::SendControl(REAL time,uActionPlayer *Act,REAL x){
    if (sn_GetNetState()==nCLIENT && Owner()==::sn_myNetID){
        //con << "sending control at " << time << "\n";

        Engine::NetGameObjectControl & control = *BroadcastControl().MutableExtension( Engine::net_game_object_control );
        control.set_time( time );
        control.set_action_id( Act->ID() );
        control.set_action_level( x );
    }
}

// easier to implement conversion helpers: just extract the relevant sub-protbuf.
nProtoBuf       * eNetGameObject::ExtractControl( Network::NetObjectControl       & control )
{
    return control.MutableExtension( Engine::net_game_object_control );
}

nProtoBuf const * eNetGameObject::ExtractControl( Network::NetObjectControl const & control )
{
    tASSERT( control.HasExtension( Engine::net_game_object_control ) );
    return & control.GetExtension( Engine::net_game_object_control );
}

void eNetGameObject::ReceiveControl(REAL time,uActionPlayer *Act,REAL x){
#ifdef DEBUG
    if (sn_GetNetState()==nCLIENT)
        tERR_ERROR("rec_cont should not be called client-side!");
#endif

    // after control is received, we better sync this object with
    // the clients:

    RequestSync();
}

void eNetGameObject::WriteSync( Engine::NetGameObjectSync & sync, bool init ) const
{
    nNetObject::WriteSync( *sync.mutable_base(), init );

    if ( init )
    {
        sync.set_player_id( player->ID() );
        sync.set_autodelete( autodelete );
    }

    sync.set_last_time( lastTime );
    Direction().WriteSync( *sync.mutable_direction() );
    Position() .WriteSync( *sync.mutable_position()  );
}

void eNetGameObject::ReadSync( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender )
{
    nNetObject::ReadSync( sync.base(), sender );
    lastTime = sync.last_time();
    dir.ReadSync( sync.direction() );
    pos.ReadSync( sync.position() );
}

bool eNetGameObject::ClearToTransmit(int user) const{
#ifdef DEBUG
    if (nNetObject::DoDebugPrint())
    {
        if (eTransferInhibitor::no_transfer(user))
        {
            con << "Not transfering eNetGameObject " << ID()
            << " for user " << user << " because of some obscure reason.\n";
            st_Breakpoint();
            eTransferInhibitor::no_transfer(user);
        }
        else if (!player)
            con << "Not transfering eNetGameObject " << ID()
            << " for user " << user << " because it has no player!\n";
        else if (!player->HasBeenTransmitted(user))
        {
            tString s;
            s << "No transfering eNetGameObject " << ID()
            << " for user " << user << " because ";
            player->PrintName(s);
            s << " has not been transmitted.\n";
            con << s;
        }
    }
#endif


    return
        nNetObject::ClearToTransmit(user) &&
        !eTransferInhibitor::no_transfer(user) &&
        (!player ||
         player->HasBeenTransmitted(user));
}

bool eNetGameObject::Timestep(REAL currentTime){
    // calculate new sr_laggometer
    if (sn_GetNetState() == nSTANDALONE){
        laggometerSmooth=0;
        return false;
    }

    REAL animts=(currentTime-lastTime);
    if (animts<0)
        animts=0;

    if (sn_GetNetState() == nSERVER || Owner() == ::sn_myNetID ||
            ( player && laggometer <= 0 ) )
    {
        // calculate laggometer from ping
        laggometer = se_GetPing( player );
    }

    // legitimate, but does not look good:
    // if ( laggometerSmooth <= 0 )
    //    laggometerSmooth = laggometer;

    laggometerSmooth=(laggometerSmooth+laggometer*animts)/(1 + animts);
    lastTime=currentTime;

    // Update ping overflow
    pingOverflow/=(1+animts*PING_OVERFLOW_TS);

    return false;
}

REAL eNetGameObject::Lag() const{
    return laggometerSmooth;
}

REAL eNetGameObject::LagThreshold() const{
    // ask the lag compensation framework
    if ( sn_GetNetState() != nSERVER )
        return 0;
    if ( Owner() == 0 )
        return 0;
    return eLag::Threshold();
}

static tCallbackOr* transfer_anchor;

eTransferInhibitor::eTransferInhibitor(BOOLRETFUNC *f)
    :tCallbackOr(transfer_anchor,f){}

bool eTransferInhibitor::no_transfer(int u){
    user = u;
    return Exec(transfer_anchor);
}

int eTransferInhibitor::user;

nMessage & operator<< (nMessage &m, const eCoord &x){
    m << x.x;
    m << x.y;

    return m;
}

nMessage & operator>> (nMessage &m, eCoord &x){
    m >> x.x;
    m >> x.y;

    return m;
}
// *******************************************************************************
// *
// *	DoGetMachine
// *
// *******************************************************************************
//!
//!		@return
//!
// *******************************************************************************

nMachine & eNetGameObject::DoGetMachine( void ) const
{
    if ( Player() )
        return Player()->GetMachine();
    else
        return nNetObject::DoGetMachine();
}


