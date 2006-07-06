/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "eLagCompensation.h"

#include "tSysTime.h"

#include "nNetwork.h"
#include "nConfig.h"

// client side settings
static REAL se_maxLagSpeedup=.2;        // maximal speed increase of timer while lag is compensated for
static REAL se_lagSlowDecayTime=120.0;  // timescale the slow lag measurement decays on
static REAL se_lagFastDecayTime=5.0;    // timescale the fast lag measurement decays on
static REAL se_lagSlowWeight=.1f;       // extra weight lag reports from the server influence the slow lag compensation with
static REAL se_lagFastWeight=1.0f;      // extra weight lag reports from the server influence the fast lag compensation with

static tSettingItem< REAL > se_maxLagSpeedupConf( "LAG_MAX_SPEEDUP_TIMER", se_maxLagSpeedup );
static tSettingItem< REAL > se_lagSlowDecayTimeConf( "LAG_SLOW_TIME", se_lagSlowDecayTime );
static tSettingItem< REAL > se_lagFastDecayTimeConf( "LAG_FAST_TIME", se_lagFastDecayTime );
static tSettingItem< REAL > se_lagSlowWeightConf( "LAG_SLOW_WEIGHT", se_lagSlowWeight );
static tSettingItem< REAL > se_lagFastWeightConf( "LAG_FAST_WEIGHT", se_lagFastWeight );

//! lag tracker on client
class nClientLag
{
public:
    nClientLag():lagSlow_(0), lagFast_(0), smoothLag_(0) {}

    REAL SmoothLag(){ return smoothLag_; }

    void ReportLag( REAL lag, REAL weight )
    {
#ifdef DEBUG
        con << "Received message of " << lag << " seconds of lag, weight " << weight << "\n";
#endif

        lag *= weight;

        lagFast_ = smoothLag_ + lag * se_lagFastWeight;
        lagSlow_ = ( smoothLag_ > lagSlow_ ? lagSlow_ : smoothLag_ ) + lag * se_lagSlowWeight;
    }

    void Timestep(REAL dt)
    {
        if ( dt > .5 )
            dt = .5;

        // increase smooth lag
        REAL speedup = se_maxLagSpeedup * dt;
        smoothLag_ += speedup;

        // clamp fast lag with slow lag
        if ( lagFast_ < lagSlow_ )
            lagFast_ = lagSlow_;

        // clam smooth lag with fast lag
        if (  smoothLag_ > lagFast_ )
            smoothLag_ = lagFast_;

        // let regular lag decay slowly
        lagSlow_ *= se_lagSlowDecayTime/( se_lagSlowDecayTime + dt );
        lagFast_ *= se_lagFastDecayTime/( se_lagFastDecayTime + dt );
    }
private:
    REAL lagSlow_;   //! most accurate estimate of lag
    REAL lagFast_;   //! faster adapting, but less accurate estimate of lag
    REAL smoothLag_; //! smoothed estimate of lag
};

static nClientLag se_clientLag;

static void se_receiveLagMessage( nMessage & m )
{
    if ( sn_GetNetState() != nCLIENT )
        return;

    REAL lag;
    m >> lag;

    REAL weight;
    m >> weight;

    se_clientLag.ReportLag( lag, weight );
}

static nDescriptor se_receiveLagMessageDescriptor( 240, se_receiveLagMessage,"lag message" );

// maximal seconds of lag credit
static REAL se_lagCredit = .5f;

// sweet spot, the fill ratio of lag credit the server tries to keep the client at
static REAL se_lagCreditSweetSpot = .5f;

// timescale lag credit is restored on
static REAL se_lagCreditTime = 600.0f;

static tSettingItem< REAL > se_lagCreditConf( "LAG_CREDIT", se_lagCredit );
static tSettingItem< REAL > se_lagCreditSweetSpotConf( "LAG_SWEET_SPOT", se_lagCreditSweetSpot );
static tSettingItem< REAL > se_lagCreditTimeConf( "LAG_CREDIT_TIME", se_lagCreditTime );

//! lag tracker on server
class nServerLag
{
public:
    nServerLag()
    {
        Reset();
    }

    REAL Ping()
    {
        return sn_Connections[client_].ping.GetPing();
    }

    REAL Credit()
    {
        return se_lagCredit;
    }

    void Reset()
    {
        creditUsed_ = se_lagCreditSweetSpot * Credit();
        lastTime_ = lastLag_ = tSysTimeFloat();
        client_ = 0;
    }

    void SetClient( int client )
    {
        tASSERT( 1 <= client && client <= MAXCLIENTS );

        client_ = client;
    }

    void Report( REAL lag )
    {
        // get info
        REAL credit = Credit();
        if ( credit < EPS )
            credit = EPS;
        REAL ping = Ping();

        // don't report too often
        double time = tSysTimeFloat();
        if ( time - lastLag_ < 4 * ping )
            return;
        lastLag_ = time;

        // send a simple message to the client
        nMessage * mess = tNEW( nMessage )( se_receiveLagMessageDescriptor );
        *mess << lag;

        // propose a weight to the client, it will determine how much impact the lag report has
        REAL weight = 1;
        if ( se_lagCreditSweetSpot > 0 )
        {
            weight = ( (creditUsed_+2*lag)/credit )/se_lagCreditSweetSpot;
            weight = weight > 1 ? 1 : weight;
        }
        *mess << weight;

        mess->Send( client_ );
    }

    REAL CreditLeft()
    {
        return Credit() - creditUsed_;
    }

    REAL TakeCredit( REAL lag )
    {
#ifdef DEBUG
        con << "Requesting " << lag << " seconds of lag credit for client " << client_;
#endif

        // get values
        REAL credit = Credit();

        // sanity check
        if ( se_lagCreditTime < EPS )
            se_lagCreditTime = EPS;

        // timestep credit
        double time = tSysTimeFloat();
        REAL dt = time - lastTime_;
        lastTime_ = time;
        creditUsed_ -= dt * credit/se_lagCreditTime;
        if ( creditUsed_ < 0 )
            creditUsed_ = 0;

        // see how much credit is left to be used
        REAL creditLeft = credit - creditUsed_;
        if ( lag > creditLeft )
            lag = creditLeft;
        if ( lag < 0 )
            lag = 0;

        // use it and return it
        creditUsed_ += lag;

#ifdef DEBUG
        con << ", granting " << lag << ".\n";
#endif

        return lag;
    }
private:
    REAL creditUsed_; //!< the used lag credit
    double lastTime_; //!< the last time lag credit was calculated
    double lastLag_;  //!< the last time lag was reported to the client
    int client_;      //!< the client this object is responsible for
};

nServerLag se_serverLag[MAXCLIENTS+1];

// callback resetting the lag credit on login/logout
static void login_callback(){
    int user = nCallbackLoginLogout::User();
    if ( sn_GetNetState() != nSERVER || user == 0 || user > MAXCLIENTS )
        return;

    se_serverLag[user].Reset();
    se_serverLag[user].SetClient(user);
}
static nCallbackLoginLogout nlc(&login_callback);

// offsets added to the lag measurements so server admins who know their connection is crappy
// can force the clients to add a bit of (predictable and therefore good) lag

static REAL se_lagOffsetClient = 0.0f;
static REAL se_lagOffsetServer = 0.0f;

static tSettingItem< REAL > se_lagOffsetClientConf( "LAG_OFFSET_CLIENT", se_lagOffsetClient );
static nSettingItem< REAL > se_lagOffsetServerConf( "LAG_OFFSET_SERVER", se_lagOffsetServer );

// *******************************************************************************
// *
// *	Report
// *
// *******************************************************************************
//!
//!		@param	client	the ID of the client the lag is happening for
//!		@param	lag	    the amount of lag
//!		@return		    the amount of lag that is allowed to be compensated by the game code
//!
// *******************************************************************************

void eLag::Report( int client, REAL lag )
{
    tVERIFY( 1 <= client && client <= MAXCLIENTS );

    se_serverLag[client].Report( lag );
}

// *******************************************************************************
// *
// *	TakeCredit
// *
// *******************************************************************************
//!
//!		@param	client	the ID of the client the lag is happening for
//!		@param	lag	    the amount of lag that is to be compensated
//!		@return		    the amount of lag that is allowed to be compensated by the game code
//!
// *******************************************************************************

REAL eLag::TakeCredit( int client, REAL lag )
{
    tVERIFY( 1 <= client && client <= MAXCLIENTS );

    return se_serverLag[client].TakeCredit( lag );
}


// *******************************************************************************
// *
// *	Credit
// *
// *******************************************************************************
//!
//!		@param	client	the ID of the client the lag is happening for
//!		@return		    the amount of lag that is allowed to be compensated by the game code
//!
// *******************************************************************************

REAL eLag::Credit( int client )
{
    tVERIFY( 1 <= client && client <= MAXCLIENTS );

    return se_serverLag[client].Credit();
}

// *******************************************************************************
// *
// *	Current
// *
// *******************************************************************************
//!
//!		@return		the amount of lag that needs to be compensated
//!
// *******************************************************************************

REAL eLag::Current( void )
{
    return se_clientLag.SmoothLag() + se_lagOffsetClient + se_lagOffsetServer;
}

// *******************************************************************************
// *
// *	Timestep
// *
// *******************************************************************************
//!
//!		@param	dt	time passed since last call
//!
// *******************************************************************************

void eLag::Timestep( REAL dt )
{
    se_clientLag.Timestep( dt );
}
