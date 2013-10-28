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
#include "rSysdep.h"

#include "eTimer.pb.h"

#ifdef DEBUG
// #define DEBUG_X
#endif

#ifdef DEBUG_X
#include <fstream>
#endif

eTimer *se_mainGameTimer=NULL;

// from nNetwork.C; used to sync time with the server
//extern REAL sn_ping[MAXCLIENTS+2];

eTimer::eTimer():nNetObject(), startTimeSmoothedOffset_(0){
    //con << "Creating own eTimer.\n";

    smoothedSystemTime_ = tSysTimeFloat();

    drifting_ = false;

    speed = 1.0;

    Reset(0);

    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;
    if (sn_GetNetState()==nSERVER)
        RequestSync();

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    remoteStartTimeOffsetClamped_ = 0;
    remoteStartTimeOffsetSanitized_ = 0;

    synced_ = true;
    creationSystemTime_ = tSysTimeFloat();
}

eTimer::eTimer( Engine::TimerSync const & sync, nSenderInfo const & sender )
: nNetObject( sync.base(), sender )
, startTimeSmoothedOffset_(0)
{
    //con << "Creating remote eTimer.\n";

    smoothedSystemTime_ = tSysTimeFloat();

    // start assuming bad syncs
    drifting_ = false;

    speed = 1.0;

    Reset(sync.time(),true);

    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    synced_ = false;
    creationSystemTime_ = tSysTimeFloat();

    lastStartTime_ = lastRemoteTime_ = 0;

    remoteStartTimeSent_ = sync.has_start_time();
    remoteStartTime_ = lastRemoteStartTime_ = sync.start_time();

    remoteStartTimeOffsetClamped_ = 0;
    remoteStartTimeOffsetSanitized_ = 0;
    
    // assume the clocks are in sync, speed wise
    startTimeDrift_.Add( 0, .001 );
#ifdef DEBUG_X
    // startTimeDrift_.Add( .1, 1000 );
#endif
}

eTimer::~eTimer(){
    //con << "Deleting eTimer.\n";
    se_mainGameTimer=NULL;
}

// see if a client supports lag compensation
static nVersionFeature se_clientLagCompensation( 14 );

void eTimer::WriteSync( Engine::TimerSync & sync, bool init ) const
{
    nNetObject::WriteSync( *sync.mutable_base(), init );
    REAL time = Time();

    sync.set_time ( time  );
    sync.set_speed( speed );
    sync.set_start_time( startTime_ );

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

void eTimer::ReadSync( Engine::TimerSync const & sync, nSenderInfo const & sender )
{
    nNetObject::ReadSync( sync.base(), sender );

    if( sn_GetNetState() != nCLIENT || sender.SenderID() != 0 )
    {
        return;
    }

    // read in the remote time
    remoteCurrentTime_ = sync.time();
    remoteSpeed_       = sync.speed();
    remoteStartTime_   = sync.start_time();

    sync_ = true;

    //std::cerr << "Got sync:" << remote_currentTime << ":" << speed << '\n';
}

static REAL se_cullDelayEnd=3.0;        // end of extra delay cull
static REAL se_cullDelayPerSecond=3.0;  // delay cull drop per second
static REAL se_cullDelayMin=3.0;        // plateau of delay cull after extra 

static tSettingItem< REAL > se_cullDelayEndConf( "CULL_DELAY_END", se_cullDelayEnd );
static tSettingItem< REAL > se_cullDelayPerSecondConf( "CULL_DELAY_PER_SECOND", se_cullDelayPerSecond );
static tSettingItem< REAL > se_cullDelayMinConf( "CULL_DELAY_MIN", se_cullDelayMin );

void eTimer::ProcessSync()
{
    sync_ = false;

    bool checkDrift = true;

    // determine the best estimate of the start time offset that reproduces the sent remote
    // time and its expected quality.
    double remoteStartTimeOffsetMax, remoteStartTimeOffsetMin = 0;

    // and our best guess for it
    double remoteStartTimeOffset;

    REAL remoteTimeNonQuality = 0;

    // shift start time
    if ( remoteStartTimeSent_ )
    {
        double step = remoteStartTime_ - lastRemoteStartTime_;
        startTime_ += step;
        lastStartTime_ += step;
        lastRemoteStartTime_ = remoteStartTime_;
    }

    startTimeExtrapolated_ = startTime_;
    
    // determine ping
    nPingAverager & pingAverager = sn_Connections[0].ping;
    REAL ping = pingAverager.GetPing();
    REAL pingVariance = pingAverager.GetFastAverager().GetAverageVariance();

    // and the average frame time
    REAL spf = averageSpf_.GetAverage();

    {
        // adapt remote time a little, we're processing syncs with a frame delay
        REAL remote_currentTime = remoteCurrentTime_ + spf * remoteSpeed_;
        REAL remoteSpeed = remoteSpeed_;
 
        // forget about earlier syncs if the speed changed
        if ( fabs( speed - remoteSpeed ) > 10 * EPS )
        {
            if ( !remoteStartTimeSent_ )
            {
                ResetAveragers();
            }
            checkDrift = false;
        }

        speed = remoteSpeed;

        // add half our ping (see Einsteins SRT on clock syncronisation)
        REAL real_remoteTime=remote_currentTime+ping*speed*.5;

        // and the normal time including ping charity
        REAL min_remoteTime=remote_currentTime+ping*speed-
                            sn_pingCharityServer*.001;

        if (real_remoteTime<min_remoteTime)
            remote_currentTime=min_remoteTime;
        else
            remote_currentTime=real_remoteTime;

        // determine quality from ping variance
        remoteTimeNonQuality = pingVariance;

        // determine start time offset
        remoteStartTimeOffsetMax = smoothedSystemTime_ - startTime_ - remote_currentTime;
        remoteStartTimeOffsetMin = lastTime_ - startTime_ - remote_currentTime;

        // bring offset in line
        if( remoteStartTimeOffsetMax < remoteStartTimeOffsetClamped_ )
        {
            remoteStartTimeOffsetClamped_ = remoteStartTimeOffsetMax;
        }
        if( remoteStartTimeOffsetMin > remoteStartTimeOffsetClamped_ )
        {
            remoteStartTimeOffsetClamped_ = remoteStartTimeOffsetMin;
        }

        // add to min sample record and use the minimized one for further calculations
        remoteStartTimeOffsetMin_.Add( remoteStartTimeOffsetClamped_ );
        remoteStartTimeOffset = remoteStartTimeOffsetMin_.GetMin();

        // calculate drift
        if( checkDrift )
        {
            REAL timeDifference = remote_currentTime - lastRemoteTime_;
            REAL drift = lastStartTime_ - remoteStartTimeOffset;

            if ( timeDifference > 0 && timeDifference < 3 && fabs(drift) < .5 && remote_currentTime > 5 && smoothedSystemTime_ - creationSystemTime_ > 10 )
            {
                REAL driftAverage = Drift();
                // con << "Drift: " << driftAverage << "\n";
                REAL driftDifference = drift/timeDifference;
                // the drift difference so far is how the remote timer drifts relative to this;
                // we need to "invert" this to get the correct compensation drift.
                driftDifference = 1/(1-driftDifference)-1;

                timeDifference = timeDifference > 1 ? 1 : timeDifference;

                startTimeDrift_.Timestep( timeDifference * ( drifting_ ? .01 : .001 ) );

                startTimeDrift_.Add( driftDifference + driftAverage, timeDifference );

                driftAverage = startTimeDrift_.GetAverage();
                // if( driftAverage < -.0005 && smoothedSystemTime_ > 100 )
                // {
                //     con << "Big drift!\n";
                // }

                if( !drifting_ && startTimeDrift_.GetWeight() > 20 && fabs(driftAverage) > .0001 && fabs(driftAverage)*(smoothedSystemTime_ - creationSystemTime_-10) > 0.1 )
                {
                    drifting_ = true;

                    // typically, drift measurements get better after compensation has been turned
                    // on, so forget precious measurements a little.
                    startTimeDrift_.Timestep(2);
#ifdef DEBUG
                    con << "Timer drift of " << driftAverage*100 << "% detected. Compensating.\n";
#endif
                }
            }
        }

        lastStartTime_ = remoteStartTimeOffset;
        lastRemoteTime_ = remote_currentTime;

        // let the averagers decay faster at the beginning
        if ( remote_currentTime < 0 && !remoteStartTimeSent_ )
        {
            qualityTester_.Timestep( .2 );
            startTimeOffset_.Timestep( .2 );
        }
    }

    // try to get independend quality measure: get the variance of the received start times
    qualityTester_.Add( remoteStartTimeOffsetClamped_ );

    // add the variance to the non-quality, along with an offset to avoid division by zero
    remoteTimeNonQuality += 0.00001 + 4 * qualityTester_.GetAverageVariance();

    // add the offset to the statistics, weighted by the quality
    startTimeOffset_.Add( remoteStartTimeOffsetClamped_, 1/remoteTimeNonQuality );

    // sanity check offset average.
    {
        REAL tolerance = spf * .25 + .001;
        double minOffset = remoteStartTimeOffset - tolerance;
        double maxOffset = remoteStartTimeOffsetClamped_ + tolerance;

        REAL deviation = 0;
        remoteStartTimeOffsetSanitized_ = startTimeOffset_.GetAverage(); 
        if( remoteStartTimeOffsetSanitized_ > maxOffset )
        {
            deviation = remoteStartTimeOffsetSanitized_ - maxOffset;
            remoteStartTimeOffsetSanitized_ = maxOffset;
        }
        else if( remoteStartTimeOffsetSanitized_ < minOffset )
        {
            deviation = - remoteStartTimeOffsetSanitized_ + minOffset;
            remoteStartTimeOffsetSanitized_ = minOffset;
        }

        // if the average does not fall into expected ranges, do some extra decay
        if( deviation > 0 )
        {
            REAL delta = maxOffset - minOffset;
            if( delta > 0 )
            {
                deviation /= delta;

                startTimeOffset_.Timestep( deviation );
            }
        }
    }


#ifdef DEBUG_X
    if ( !tRecorder::IsRecording() )
    {
        con << "T=" << smoothedSystemTime_ << " " << lastRemoteTime_ << ", Q= " << remoteTimeNonQuality << ", P= " << ping << " +/- " << sqrt(pingVariance) << " Time offset: " << remoteStartTimeOffsetClamped_ << " : " << startTimeOffset_.GetAverage() << "\n";

        std::ofstream plot("timesync.txt",std::ios_base::app);
        plot << smoothedSystemTime_ << " " << remoteStartTimeOffsetMax << " " << remoteStartTimeOffsetClamped_ << " " << remoteStartTimeOffset << " " << startTimeOffset_.GetAverage() << " " << remoteStartTimeOffsetSanitized_ << " " << startTimeSmoothedOffset_ << " " << startTimeDrift_.GetAverage() << "\n";
    }
#endif

    UpdateIsSynced();
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
    return ( smoothedSystemTime_ - startTimeExtrapolated_ ) - startTimeSmoothedOffset_ + eLag::Current();
}

void eTimer::SyncTime(){
    {
        // get current system time and store it
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

#ifdef DEBUG
#ifndef DEDICATED
    // maximum effective timestep in SP debug mode: .1s
    if (timeStep > .1f && sn_GetNetState() == nSTANDALONE && !tRecorder::IsRunning() )
    {
        startTime_ += timeStep - .1f;
        startTimeExtrapolated_ = startTime_;
        timeStep = .1f;
    }
#endif
#endif

    REAL decayFactor = 1;
    // if( badSyncs_ > 2 )
    // {
        // decayFactor *= ( badSyncs_ - 2 );
    // }

    // update lag compensation
    eLag::Timestep( timeStep*decayFactor );

    // store and average frame times
    spf_ = timeStep;

    // process sync now before an unusually large timestep messes up averageSpf_.
    if( sync_ )
    {
        ProcessSync();
    }

    if ( timeStep > 0 && speed > 0 )
    {
        averageSpf_.Add( timeStep );
        averageSpf_.Timestep( timeStep );

#ifndef DEDICATED
        // inform renderer that a game is going on and that frame drops are for real.
        rSysDep::IsInGame();
#endif
    }

    // let averagers decay
    if ( sn_GetNetState() == nCLIENT && remoteStartTimeSent_ )
    {
        // in this mode, the time syncs are extra-reliable and stable; we can
        // use long term averages.
        decayFactor *= .1;
    }

    startTimeOffset_.Timestep( timeStep * .1 * decayFactor );
    qualityTester_  .Timestep( timeStep * .2 );

    // if we're not running at default speed, update the virtual start time
    startTimeExtrapolated_ += ( 1.0 - speed - Drift() ) * timeStep;
    if ( sn_GetNetState() != nCLIENT || !remoteStartTimeSent_ )
    {
        startTime_ = startTimeExtrapolated_;
    }
    else
    {
        // we need to take the drift into account (but just the drift, the speed drift is
        // going to get compensated by syncs)
        startTime_ -= Drift() * timeStep;
    }

    // smooth time offset
    {
        double startTimeOffset = remoteStartTimeOffsetSanitized_;

        // correct huge deviations (compared to variance) faster
        REAL deviation = startTimeSmoothedOffset_ - startTimeOffset;
        REAL extraSmooth = deviation * deviation / ( startTimeOffset_.GetAverageVariance() + .0001 );

        // allow for faster smoothing at the beginning of the round
        REAL time = Time();
        if ( time < 0 )
            extraSmooth -= 4 * time;

        REAL smooth = timeStep * ( .5 + extraSmooth );
        startTimeSmoothedOffset_ = ( startTimeSmoothedOffset_ + startTimeOffset * smooth )/(1 + smooth);

        if ( !isfinite( startTimeSmoothedOffset_ ) )
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

    lastTime_ = smoothedSystemTime_;
}

void eTimer::ResetAveragers()
{
    startTimeSmoothedOffset_ = remoteStartTimeOffsetSanitized_;
    remoteStartTimeOffsetClamped_ = remoteStartTimeOffsetSanitized_;

    remoteStartTimeOffsetMin_.Clear();
    remoteStartTimeOffsetMin_.Add( remoteStartTimeOffsetSanitized_ );

    startTimeOffset_.Reset();
    startTimeOffset_.Add(remoteStartTimeOffsetSanitized_ + 100,EPS);
    startTimeOffset_.Add(remoteStartTimeOffsetSanitized_ - 100,EPS);

    qualityTester_.Reset();
    static const REAL qual = sqrt(1/EPS);
    qualityTester_.Add(qual,EPS);
    qualityTester_.Add(-qual,EPS);
}

void eTimer::Reset(REAL t, bool force){
    if (sn_GetNetState()!=nCLIENT)
        speed=1;

    sync_ = false;

    // check whether a full reset is required
    bool fullReset = force || sn_GetNetState() != nCLIENT || !remoteStartTimeSent_;
    if( fullReset )
    {
        remoteStartTimeOffsetSanitized_ = 0;
    }

    startTimeSmoothedOffset_ = remoteStartTimeOffsetSanitized_;

    // reset start time
    smoothedSystemTime_ = tSysTimeFloat();
    startTimeExtrapolated_ = smoothedSystemTime_ - t - startTimeSmoothedOffset_;

    // nothing further to do for clients on modern servers
    if( !fullReset )
    {
        return;
    }

    // reset start time for real
    startTime_ = startTimeExtrapolated_;

    // reset averagers
    ResetAveragers();

    // reset times of actions
    lastTime_ = nextSync_ = smoothedSystemTime_;
}

bool eTimer::IsSynced() const
{
    return synced_;
}

void eTimer::UpdateIsSynced()
{
    // synced status is never lost, we don't want the screen to go black during play
    if( synced_ )
    {
        return;
    }

    // allow non-synced status only during the first ten seconds of a connection
    if ( smoothedSystemTime_ - creationSystemTime_ > 10 || sn_GetNetState() != nCLIENT )
    {
        synced_ = true;
        return;
    }

    // the quality of the start time offset needs to be good enough (to .1 s, variance is the square of the expected)
    bool synced = startTimeOffset_.GetAverageVariance() < .01 &&
                  fabs( startTimeOffset_.GetAverage() - startTimeSmoothedOffset_ ) < .01;

    static char const * section = "TIMER_SYNCED";

    if ( tRecorder::IsPlayingBack() ? tRecorder::Playback( section ) : synced )
    {
        tRecorder::Record( section );

        // and make sure the next calls also return true.
        creationSystemTime_ = smoothedSystemTime_ - 11;
        synced_ = true;
    }
}

REAL eTimer::Drift() const
{
    return drifting_ ? startTimeDrift_.GetAverage() : 0;
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




