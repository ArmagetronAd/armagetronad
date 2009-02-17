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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#include "eLagCompensation.h"

#include "tSysTime.h"

#include "nNetwork.h"
#include "nConfig.h"
#include "nProtoBuf.h"

#include "eLagCompensation.pb.h"

#ifdef DEBUG
#define DEBUG_LAG
#endif

// client side settings
static REAL se_maxLagSpeedup=.2;        // maximal speed increase of timer while lag is compensated for
static REAL se_lagSlowDecayTime=30.0;   // timescale the slow lag measurement decays on
static REAL se_lagFastDecayTime=5.0;    // timescale the fast lag measurement decays on
static REAL se_lagSlowWeight=.2f;       // extra weight lag reports from the server influence the slow lag compensation with
static REAL se_lagFastWeight=1.0f;      // extra weight lag reports from the server influence the fast lag compensation with

static REAL se_lagCreditSingle = .1f;   // maximal seconds of lag credit given or accepted in a single event

static tSettingItem< REAL > se_maxLagSpeedupConf( "LAG_MAX_SPEEDUP_TIMER", se_maxLagSpeedup );
static tSettingItem< REAL > se_lagSlowDecayTimeConf( "LAG_SLOW_TIME", se_lagSlowDecayTime );
static tSettingItem< REAL > se_lagFastDecayTimeConf( "LAG_FAST_TIME", se_lagFastDecayTime );
static tSettingItem< REAL > se_lagSlowWeightConf( "LAG_SLOW_WEIGHT", se_lagSlowWeight );
static tSettingItem< REAL > se_lagFastWeightConf( "LAG_FAST_WEIGHT", se_lagFastWeight );

// averages over lag events
static nAverager se_lagAverager;

class eLagAveragerInitializer
{
    public:
    eLagAveragerInitializer()
    {
        // start with a large variance
        se_lagAverager.Add(-.1,.1f);
        se_lagAverager.Add(.1,.1f);
    }
};
static eLagAveragerInitializer se_lagInitializer;

//! lag tracker on client
class nClientLag
{public:
    nClientLag():lagLast_(0), lagSlow_(0), lagFast_(0), smoothLag_(0) {}

    REAL SmoothLag(){ return smoothLag_; }

    void ReportLag( REAL lag, REAL weight )
    {
#ifdef DEBUG
        con << "Received message of " << lag << " seconds of lag, weight " << weight << "\n";
#endif

        // clamp
        lag = lag > se_lagCreditSingle ? se_lagCreditSingle : lag;

        // memorize the time of serious reports
        if ( weight > 1 )
            lagLast_ = tSysTimeFloat();

        REAL slowWeight = weight * se_lagSlowWeight;
        slowWeight = slowWeight > 1 ? 1 : slowWeight;
        REAL fastWeight = weight * se_lagFastWeight;
        fastWeight = fastWeight > 1 ? 1 : fastWeight;

        lagFast_ = smoothLag_ + lag * fastWeight;
        lagSlow_ = ( smoothLag_ > lagSlow_ ? lagSlow_ : smoothLag_ ) + lag * slowWeight;
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

        // clamp smooth lag with fast lag
        if (  smoothLag_ > lagFast_ )
            smoothLag_ = lagFast_;

        // the last serious lag report came from this many seconds ago
        REAL lastLag = tSysTimeFloat() - lagLast_;

        // let regular lag decay
        if ( lastLag > se_lagSlowDecayTime )
            lagSlow_ *= se_lagSlowDecayTime/( se_lagSlowDecayTime + dt );
        if ( lastLag > se_lagFastDecayTime )
            lagFast_ *= se_lagFastDecayTime/( se_lagFastDecayTime + dt );
    }
private:
    REAL lagLast_;   //!< the last time a serious lag report came in
    REAL lagSlow_;   //!< most accurate estimate of lag
    REAL lagFast_;   //!< faster adapting, but less accurate estimate of lag
    REAL smoothLag_; //!< smoothed estimate of lag
};

static nClientLag se_clientLag;

static void se_LagNotificationHandler( Engine::LagNotification const & notification, nSenderInfo const & )
{
    if ( sn_GetNetState() != nCLIENT )
        return;

    se_clientLag.ReportLag( notification.lag(), notification.weight() );
}

static nProtoBufDescriptor< Engine::LagNotification > se_lagNotificationDescriptor( 240, se_LagNotificationHandler );

// maximal seconds of lag credit
static REAL se_lagCredit = .5f;

// maximal lag credit for a single event when compared to the lag variance
static REAL se_lagCreditVariance = 3.0f;

// cached value of that times the variance of lag
static REAL se_lagCreditVarianceCache = 0.0f;

// sweet spot, the fill ratio of lag credit the server tries to keep the client at
static REAL se_lagCreditSweetSpot = .5f;

// timescale lag credit is restored on
static REAL se_lagCreditTime = 600.0f;

static tSettingItem< REAL > se_lagCreditConf( "LAG_CREDIT", se_lagCredit );
static tSettingItem< REAL > se_lagCreditSingleConf( "LAG_CREDIT_SINGLE", se_lagCreditSingle );
static tSettingItem< REAL > se_lagCreditVarianceConf( "LAG_CREDIT_VARIANCE", se_lagCreditVariance );
static tSettingItem< REAL > se_lagCreditSweetSpotConf( "LAG_SWEET_SPOT", se_lagCreditSweetSpot );
static tSettingItem< REAL > se_lagCreditTimeConf( "LAG_CREDIT_TIME", se_lagCreditTime );

// threshold
static REAL se_lagThreshold = 0.0f;
static tSettingItem< REAL > se_lagThresholdConf( "LAG_THRESHOLD", se_lagThreshold );

// see if a client supports lag compensation
static nVersionFeature se_clientLagCompensation( 14 );
    //! the version feature telling whether this is supported
nVersionFeature const & eLag::Feature()
{
    return se_clientLagCompensation;
}

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
        // see if it is useful to report
        if ( ! se_clientLagCompensation.Supported( client_ ) )
            return;

        // clamp
        lag = lag > se_lagCreditSingle ? se_lagCreditSingle : lag;

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
        Engine::LagNotification & notification = se_lagNotificationDescriptor.Send( client_ );
        notification.set_lag( lag );

        // propose a weight to the client, it will determine how much impact the lag report has
        REAL weight = 1;
        if ( se_lagCreditSweetSpot > 0 )
        {
            weight = ( (creditUsed_+2*lag)/credit )/se_lagCreditSweetSpot;
        }
        notification.set_weight( weight );
    }

    REAL CreditLeft()
    {
        return Credit() - creditUsed_;
    }

    // clamps a single credit event to the configured bounds
    static void ClampSingleCredit( REAL & credit )
    {
        credit = credit > se_lagCreditSingle ? se_lagCreditSingle : credit;
        credit = credit > se_lagCreditVarianceCache ? se_lagCreditVarianceCache : credit;
    }
        
    REAL TakeCredit( REAL lag )
    {
        lag -= se_lagThreshold;

        se_lagAverager.Add( lag, 1 );
        se_lagAverager.Timestep( .01 );
        REAL lagVariance = se_lagAverager.GetDataVariance();

        if ( lag > 0 )
        {
#ifdef DEBUG_LAG
            REAL lagBefore = lag;
#endif

            // if everyone is lagging, delete the used credit
            Balance();

            // update variance cache
            if( se_lagCreditVariance > 0 )
            {
                se_lagCreditVarianceCache = sqrt( lagVariance ) * se_lagCreditVariance;
            }

            // clamp
            ClampSingleCredit( lag );

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

#ifdef DEBUG_LAG
            {
                if ( lag < lagBefore )
                    con << "Requesting " << lagBefore << " seconds of lag credit, granting " << lag << ".\n";
                else
                    con << "Granting " << lag << " seconds of lag credit.\n";
            }
#endif

            lag += se_lagThreshold;

        }

        return lag;
    }

    static void Balance();
private:
    REAL creditUsed_; //!< the used lag credit
    double lastTime_; //!< the last time lag credit was calculated
    double lastLag_;  //!< the last time lag was reported to the client
    int client_;      //!< the client this object is responsible for
};

nServerLag se_serverLag[MAXCLIENTS+1];

// credit amnesty: if everyone is lagging, it must be the server's fault. Delete the used credit.
void nServerLag::Balance()
{
    int i;

    // only if many users are online
    if ( sn_NumUsers() <= 1 )
        return;

    // find the minimum used credit
    REAL minCredit = se_lagCredit;
    for ( i = MAXCLIENTS; i>0; --i )
    {
        if ( sn_Connections[i].socket )
        {
            REAL credit = se_serverLag[i].creditUsed_;
            if ( credit < minCredit )
                minCredit = credit;
        }
    }

    // find out how much you can take away
    REAL amnesty = minCredit - se_lagCredit * se_lagCreditSweetSpot;

    // and take it away from everyone
    if ( amnesty > 0 )
        for ( i = MAXCLIENTS; i>0; --i )
            se_serverLag[i].creditUsed_ -= amnesty;
}

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
    // don't give credit on the client
    if ( sn_GetNetState() != nSERVER )
        return 0;

    tVERIFY( 1 <= client && client <= MAXCLIENTS );

    // see how much total credit is left
    REAL credit = se_serverLag[client].CreditLeft();

    // and clam pit
    nServerLag::ClampSingleCredit( credit );

    return credit;
}

// *******************************************************************************
// *
// *	Threshold
// *
// *******************************************************************************
//!
//!		@return		the amount of lag that is always tolerated
//!
// *******************************************************************************

REAL eLag::Threshold( void )
{
    return se_lagThreshold;
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
