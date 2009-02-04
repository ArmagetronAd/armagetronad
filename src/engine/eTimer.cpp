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

#include "tMemManager.h"
#include "nProtoBuf.h"
#include "eTimer.h"
#include "eNetGameObject.h"
#include "nSimulatePing.h"
#include "tRecorder.h"
#include "tMath.h"
#include "tConfiguration.h"
#include "eLagCompensation.h"

#include "eTimer.pb.h"

eTimer *se_mainGameTimer=NULL;

// from nNetwork.C; used to sync time with the server
//extern REAL sn_ping[MAXCLIENTS+2];

eTimer::eTimer():nNetObject(), startTimeSmoothedOffset_(0){
    //con << "Creating own eTimer.\n";

    speed = 1.0;

    Reset(0);

    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;
    if (sn_GetNetState()==nSERVER)
        RequestSync();

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    creationSystemTime_ = tSysTimeFloat();
}

eTimer::eTimer( Engine::TimerSync const & sync, nSenderInfo const & sender )
: nNetObject( sync.base(), sender )
, startTimeSmoothedOffset_(0)
{
    //con << "Creating remote eTimer.\n";

    speed = 1.0;

    Reset(0);

    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    creationSystemTime_ = tSysTimeFloat();

    lastStartTime_ = lastRemoteTime_ = 0;

    // assume strongly the clocks are in sync
    startTimeDrift_.Add( 0, 100 );
}

eTimer::~eTimer(){
    //con << "Deleting eTimer.\n";
    se_mainGameTimer=NULL;
}

// see if a client supports lag compensation
static nVersionFeature se_clientLagCompensation( 14 );

// old clients need a default lag compensation
static REAL se_lagOffsetLegacy = 0.0f;
static tSettingItem< REAL > se_lagOffsetLegacyConf( "LAG_OFFSET_LEGACY", se_lagOffsetLegacy );

void eTimer::WriteSync( Engine::TimerSync & sync, bool init ) const
{
    nNetObject::WriteSync( *sync.mutable_base(), init );
    REAL time = Time();

    if ( SyncedUser() > 0 && !se_clientLagCompensation.Supported( SyncedUser() ) )
        time += se_lagOffsetLegacy;

    sync.set_time ( time  );
    sync.set_speed( speed );
    //std::cerr << "syncing:" << currentTime << ":" << speed << '\n';

    // plan next sync
    const REAL stopFast = 3;
    const REAL maxFast  = 3;
    if ( time < stopFast )
    {
        nextSync_ = smoothedSystemTime_ + maxFast/(stopFast+maxFast-time);
    }
    else
    {
        nextSync_ = smoothedSystemTime_ + 1;
    }
}

static REAL se_timerStartFudge = 0.0;
static tSettingItem<REAL> se_timerStartFudgeConf("TIMER_SYNC_START_FUDGE",se_timerStartFudge);
static REAL se_timerStartFudgeStop = 2.0;
static tSettingItem<REAL> se_timerStartFudgeStopConf("TIMER_SYNC_START_FUDGE_STOP",se_timerStartFudgeStop);

void eTimer::ReadSync( Engine::TimerSync const & sync, nSenderInfo const & sender )
{
    nNetObject::ReadSync( sync.base(), sender );

    bool checkDrift = true;

    //std::cerr << "Got sync:" << remote_currentTime << ":" << speed << '\n';

    // determine the best estimate of the start time offset that reproduces the sent remote
    // time and its expected quality.
    REAL remoteStartTimeOffset = 0;
    REAL remoteTimeNonQuality = 0;
    {
        //REAL oldTime=currentTime;
        REAL remote_currentTime = sync.time();
        REAL remoteSpeed        = sync.speed();

        // forget about earlier syncs if the speed changed
        if ( fabs( speed - remoteSpeed ) > 10 * EPS )
        {
            qualityTester_.Timestep( 100 );
            startTimeOffset_.Reset();
            checkDrift = false;
        }

        speed = remoteSpeed;

        // determine ping
        nPingAverager & averager = sn_Connections[sender.SenderID()].ping;
        // pingAverager_.Add( rawAverager.GetPing(), remote_currentTime > .01 ? remote_currentTime : .01 );
        REAL ping = averager.GetPing();

        // add half our ping (see Einsteins SRT on clock syncronisation)
        REAL real_remoteTime=remote_currentTime+ping*speed*.5;

        // and the normal time including ping charity
        REAL min_remoteTime=remote_currentTime+ping*speed-
                            sn_pingCharityServer*.001;

        if (real_remoteTime<min_remoteTime)
            remote_currentTime=min_remoteTime;
        else
            remote_currentTime=real_remoteTime;

        // HACK: warp time into the future at the beginning of the round.
        // I can't explain why this is required; without it, the timer is late.
        if ( remote_currentTime < se_timerStartFudgeStop )
        {
            remote_currentTime += ping * ( se_timerStartFudgeStop - remote_currentTime ) * se_timerStartFudge;
            checkDrift = false;
        }

        // determine quality from ping variance
        remoteTimeNonQuality = averager.GetFastAverager().GetAverageVariance();

        // determine start time
        remoteStartTimeOffset = smoothedSystemTime_ - startTime_ - remote_currentTime;

        // calculate drift
        if( checkDrift )
        {
            REAL timeDifference = remote_currentTime - lastRemoteTime_;
            REAL drift = lastStartTime_ - remoteStartTimeOffset;

            if ( timeDifference > 0 )
            {
                REAL driftAverage = startTimeDrift_.GetAverage();
                // con << "Drift: " << driftAverage << "\n";
                REAL driftDifference = drift/timeDifference;
                
                timeDifference = timeDifference > 1 ? 1 : timeDifference;
                startTimeDrift_.Timestep( timeDifference * .1 );
                startTimeDrift_.Add( driftDifference + driftAverage, timeDifference );
            }
        }

        lastStartTime_ = remoteStartTimeOffset;
        lastRemoteTime_ = remote_currentTime;

        // let the averagers decay faster at the beginning
        if ( remote_currentTime < 0 )
        {
            qualityTester_.Timestep( .2 );
            startTimeOffset_.Timestep( .2 );
        }
    }

    // try to get independend quality measure: get the variance of the received start times
    qualityTester_.Add( remoteStartTimeOffset );

    // add the variance to the non-quality, along with an offset to avoid division by zero
    remoteTimeNonQuality += 0.00001 + 4 * qualityTester_.GetAverageVariance();

    // add the offset to the statistics, weighted by the quality
    startTimeOffset_.Add( remoteStartTimeOffset, 1/remoteTimeNonQuality );
}

static nNetObjectDescriptor< eTimer, Engine::TimerSync > eTimer_init(210);

nNetObjectDescriptorBase const & eTimer::DoGetDescriptor() const
{
    return eTimer_init;
}

// determines whether time should be smoothed
static bool se_SmoothTime()
{
    bool smooth = 0;

    // try to get value from recording
    char const * section = "SMOOTHTIME";
    if ( !tRecorder::PlaybackStrict( section, smooth ) )
    {
        // get value by OS type
        smooth = !tTimerIsAccurate();
    }
    // archive the decision
    tRecorder::Record( section, smooth );

    return smooth;
}

REAL eTimer::Time() const
{
    return ( smoothedSystemTime_ - startTime_ ) - startTimeSmoothedOffset_ + eLag::Current();
}

void eTimer::SyncTime(){
    // get current system time
    {
        double newTime=tSysTimeFloat();

        static bool smooth = se_SmoothTime();

        // recheck if no recording/playback is running
        if ( !tRecorder::IsRunning() )
        {
            smooth = !tTimerIsAccurate();
        }

        if ( !smooth )
        {
            // take it
            smoothedSystemTime_ = newTime;
        }
        else
        {
            // smooth it
            REAL smoothDecay = .2f;
            smoothedSystemTime_ = ( smoothedSystemTime_ + smoothDecay * newTime )/( 1 + smoothDecay );
        }
    }

    // update timers
    REAL timeStep=smoothedSystemTime_ - lastTime_;
    lastTime_ = smoothedSystemTime_;

#ifdef DEBUG
#ifndef DEDICATED
    // maximum effective timestep in SP debug mode: .1s
    if (timeStep > .1f && sn_GetNetState() == nSTANDALONE && !tRecorder::IsRunning() )
    {
        startTime_ += timeStep - .1f;
        timeStep = .1f;
    }
#endif
#endif

    // update lag compensation
    eLag::Timestep( timeStep );

    // store and average frame times
    spf_ = timeStep;
    if ( timeStep > 0 && speed > 0 )
    {
        averageSpf_.Add( timeStep );
        averageSpf_.Timestep( timeStep );
    }

    // let averagers decay
    startTimeOffset_.Timestep( timeStep * .1 );
    qualityTester_  .Timestep( timeStep * .3 );
    // pingAverager_   .Timestep( timeStep * .05);

    // if we're not running at default speed, update the virtual start time
    startTime_ += ( 1.0 - speed - startTimeDrift_.GetAverage() ) * timeStep;

    // smooth time offset
    {
        REAL startTimeOffset = startTimeOffset_.GetAverage();

        // correct huge deviations (compared to variance) faster
        REAL deviation = startTimeSmoothedOffset_ - startTimeOffset;
        REAL extraSmooth = deviation * deviation / ( startTimeOffset_.GetAverageVariance() + .0001 );

        // allow for faster smoothing at the beginning of the round
        REAL time = Time();
        if ( time < 0 )
            extraSmooth -= 4 * time;

        REAL smooth = timeStep * ( .5 + extraSmooth );
        startTimeSmoothedOffset_ = ( startTimeSmoothedOffset_ + startTimeOffset * smooth )/(1 + smooth);

        if ( !finite( startTimeSmoothedOffset_ ) )
        {
            // emergency, smoothing the timer produced infinite results
            st_Breakpoint();
            startTimeSmoothedOffset_ = startTimeOffset;
        }
    }

    // request syncs
    if (sn_GetNetState()==nSERVER && smoothedSystemTime_ >= nextSync_ )
    {
#ifdef nSIMULATE_PING
        RequestSync(); // ack.
#else
        RequestSync(false); // NO ack.
#endif
    }
}

void eTimer::Reset(REAL t){
    if (sn_GetNetState()!=nCLIENT)
        speed=1;

    // reset start time
    smoothedSystemTime_ = tSysTimeFloat();
    startTime_ = smoothedSystemTime_ - t;

    // reset averagers
    startTimeOffset_.Reset();
    startTimeOffset_.Add(100,EPS);
    startTimeOffset_.Add(-100,EPS);

    qualityTester_.Reset();
    static const REAL qual = sqrt(1/EPS);
    qualityTester_.Add(qual,EPS);
    qualityTester_.Add(-qual,EPS);

    // reset times of actions
    lastTime_ = nextSync_ = smoothedSystemTime_;
    startTimeSmoothedOffset_ = startTimeOffset_.GetAverage();
}

bool eTimer::IsSynced() const
{
    // allow non-synced status only during the first ten seconds of a connection
    if ( smoothedSystemTime_ - creationSystemTime_ > 10 || sn_GetNetState() != nCLIENT )
        return true;

    // the quality of the start time offset needs to be good enough (to .1 s, variance is the square of the expected)
    bool synced = startTimeOffset_.GetAverageVariance() < .01 &&
                  fabs( startTimeOffset_.GetAverage() - startTimeSmoothedOffset_ ) < .01;

    static char const * section = "TIMER_SYNCED";

    if ( tRecorder::IsPlayingBack() ? tRecorder::Playback( section ) : synced )
    {
        tRecorder::Record( section );

        // and make sure the next calls also return true.
        creationSystemTime_ = smoothedSystemTime_ - 11;
        return true;
    }

    return false;
}

void eTimer::pause(bool p){
    if (p){
        if(speed!=0){
            speed=0;
            if (sn_GetNetState()==nSERVER)
                RequestSync();
        }
    }
    else{
        if (speed!=1){
            speed=1;
            //Reset(currentTime_);

            if (sn_GetNetState()==nSERVER)
                RequestSync();
        }
    }
}

REAL se_GameTime(){
    if (se_mainGameTimer)
        return se_mainGameTimer->Time();
    else
        return 0;
}
REAL se_GameTimeNoSync(){
    if (se_mainGameTimer)
        return se_mainGameTimer->TimeNoSync();
    else
        return 0;
}

void se_SyncGameTimer(){
    if (se_mainGameTimer)
        se_mainGameTimer->SyncTime();
}

void se_MakeGameTimer(){
    tNEW(eTimer);
}

void se_KillGameTimer(){
    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=NULL; // to make sure
}


void se_ResetGameTimer(REAL t){
    if (se_mainGameTimer)
        se_mainGameTimer->Reset(t);
}

void se_PauseGameTimer(bool p){
    if (se_mainGameTimer && sn_GetNetState()!=nCLIENT)
        se_mainGameTimer->pause(p);
}

REAL se_AverageFrameTime(){
    return 1/se_AverageFPS();
}

REAL se_AverageFPS(){
    if (se_mainGameTimer)
        return se_mainGameTimer->AverageFPS();
    else
        return (.2);
}

REAL se_PredictTime(){
    return se_AverageFrameTime()*.5;
}




