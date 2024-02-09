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
#include "eTimer.h"
#include "eNetGameObject.h"
#include "nSimulatePing.h"
#include "tRecorder.h"
#include "tConfiguration.h"
#include "eLagCompensation.h"

#include <memory>
#include <numeric>
#include <array>
// #include <fstream>

class eFPSCounter
{
public:
    // call when a frame has ended
    void Tick(REAL dt) noexcept
    {
        PrivateTick(dt);
    }

    // gets the current best value for FPS
    int GetFPS() const noexcept
    {
        return bestFPS_;
    }

    eFPSCounter(int initialFPS = 0)
    {
        for (auto& lastFPS : lastFPS_)
            lastFPS = initialFPS;
        bestFPS_ = initialFPS;

        // divide FPS evenly over buckets
        for (int i = bucketCount - 1; i >= 0; --i)
        {
            buckets_[i] = initialFPS / (i + 1);
            initialFPS -= buckets_[i];
            bucketsOverhang_[i] = 0.0f;
        }
    }

private:
    // counting FPS works by dividing each second up into bucketCount buckets
    // we cound frames until each bucket is full, then rotate to the next one
    // every time a bucket is full, we sum up all of the buckets
#ifdef DEBUG
    //  prime to provoke problems
    static constexpr size_t bucketCount = 7;
    // or a single bucket
    // static constexpr size_t bucketCount = 1;
#else
    // nice and round, no problems, divisor of popular refresh rates
    static constexpr size_t bucketCount = 12;
#endif
    // the length in seconds of each bucket
    static constexpr REAL bucketLength = 1.0f / bucketCount;

    REAL leftInCurrentBucket_ = bucketLength - 1.0f / 300;
    int currentBucketFrameCount_ = 0;
    size_t currentBucket_ = 0;
    // number of frames in each bucket
    std::array<int, bucketCount> buckets_;
    // time overhang when we last left this bucket
    std::array<REAL, bucketCount> bucketsOverhang_;

    // if the bcket is full, we write the current frames per second
    // into these rolling buffers
    std::array<int, 3> lastFPS_;
    int bestFPS_;

    int GetBestFPS() const noexcept
    {
        // get maximum and minimum
        const int maxFPS = std::accumulate(lastFPS_.begin(), lastFPS_.end(),
                                           0, [](int a, int b) { return std::max(a, b); });
        const int minFPS = std::accumulate(lastFPS_.begin(), lastFPS_.end(),
                                           maxFPS, [](int a, int b) { return std::min(a, b); });
        if (maxFPS >= bestFPS_ && minFPS <= bestFPS_)
            return bestFPS_; // all is well, current best is still within the limits

        // reset best FPS to median of the collected values; since we only have three, that is easy
        if (minFPS == maxFPS)
        {
            return minFPS;
        }

        int maxCount{0};
        int minCount{0};
        for (auto fps : lastFPS_)
        {
            if (maxFPS == fps)
                maxCount++;
            else if (minFPS == fps)
                minCount++;
            else
                return fps;
        }

        if (maxCount > minCount)
            return maxFPS;
        else
            return minFPS;
    }

    void AddDataPoint(int fps) noexcept
    {
        // record FPS
        for (int i = 1; i >= 0; --i)
            lastFPS_[i] = lastFPS_[i + 1];
        lastFPS_.back() = fps;

        bestFPS_ = GetBestFPS();
#ifdef DEBUG
        bestFPS_ = fps; // for testing; errors in basic calculation are washed away by GetBestFPS()
#endif
    }

    void AdvanceBucket() noexcept
    {
        const auto overhang = leftInCurrentBucket_;
        const auto lastOverhang = bucketsOverhang_[currentBucket_];
        bucketsOverhang_[currentBucket_] = overhang;
        buckets_[currentBucket_] = currentBucketFrameCount_;

        // advance
        leftInCurrentBucket_ += bucketLength;
        currentBucketFrameCount_ = 0;
        currentBucket_ = (currentBucket_ + 1) % bucketCount;

        // sum up all buckets
        const int rawFPS = std::accumulate(buckets_.begin(), buckets_.end(), 0);

        // take overhangs into account. If they differ by more than half the frame time,
        // we overcounted or undercounted because the bucket cutoff counted one frame
        // on one second, but not the other
        const int fps = [&]() {
            const REAL delta = .5f / std::max(1, rawFPS);
            if (overhang < lastOverhang - delta)
                return rawFPS - 1;
            if (overhang > lastOverhang + delta)
                return rawFPS + 1;
            return rawFPS;
        }();

        // record
        AddDataPoint(fps);
    }

    void PrivateTick(REAL dt) noexcept
    {
        // count in current bucket, leave if it is not yet empty
        currentBucketFrameCount_++;
        leftInCurrentBucket_ -= dt;
#ifdef DEBUG
        // fluctuate bucket overflow a bit to test one-off compensation
        const REAL threshold = random() / (REAL(RAND_MAX) * bestFPS_ * 2);
#else
        const REAL threshold = 0.0f;
#endif
        if (leftInCurrentBucket_ > threshold)
            return;

        AdvanceBucket();
    }
};

eTimer * se_mainGameTimer=NULL;
tJUST_CONTROLLED_PTR<eTimer> se_mainGameTimerHolder=NULL;

// from nNetwork.C; used to sync time with the server
//extern REAL sn_ping[MAXCLIENTS+2];

eTimer::eTimer() : nNetObject(), startTimeSmoothedOffset_(0), fpsCounter_{new eFPSCounter}
{
    //con << "Creating own eTimer.\n";

    speed = 1.0;

    Reset(0);

    se_mainGameTimerHolder=nullptr;
    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;
    se_mainGameTimerHolder=this;
    if (sn_GetNetState()==nSERVER)
        RequestSync();

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    creationSystemTime_ = tSysTimeFloat();
}

eTimer::eTimer(nMessage& m) : nNetObject(m), startTimeSmoothedOffset_(0), fpsCounter_{new eFPSCounter}
{
    //con << "Creating remote eTimer.\n";

    speed = 1.0;

    Reset(0);

    se_mainGameTimerHolder=nullptr;
    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=this;

    averageSpf_.Reset();
    averageSpf_.Add(1/60.0,5);

    creationSystemTime_ = tSysTimeFloat();

    lastStartTime_ = lastRemoteTime_ = 0;

    // assume VERY strongly the clocks are in sync
    startTimeDrift_.Add( 0, 10000 );
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

void eTimer::WriteSync(nMessage &m){
    nNetObject::WriteSync(m);
    REAL time = Time();

    if ( SyncedUser() > 0 && !se_clientLagCompensation.Supported( SyncedUser() ) )
        time += se_lagOffsetLegacy;

    m << time;
    m << speed;
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

void eTimer::ReadSync(nMessage &m){
    nNetObject::ReadSync(m);

    if( sn_GetNetState() != nCLIENT || m.SenderID() != 0 )
    {
        return;
    }

    m >> remoteCurrentTime_; // read in the remote time
    m >> remoteSpeed_;
    sync_ = true;

    // record ping in case we desync on playback
    if(sn_GetNetState() == nCLIENT)
        sn_Connections[0].ping.Record();

    //std::cerr << "Got sync:" << remote_currentTime << ":" << speed << '\n';
}

static REAL se_cullDelayEnd=3.0;        // end of extra delay cull
static REAL se_cullDelayPerSecond=3.0;  // delay cull drop per second
static REAL se_cullDelayMin=10.0;       // plateau of delay cull after extra 

static tSettingItem< REAL > se_cullDelayEndConf( "CULL_DELAY_END", se_cullDelayEnd );
static tSettingItem< REAL > se_cullDelayPerSecondConf( "CULL_DELAY_PER_SECOND", se_cullDelayPerSecond );
static tSettingItem< REAL > se_cullDelayMinConf( "CULL_DELAY_MIN", se_cullDelayMin );

void eTimer::ProcessSync()
{
    sync_ = false;

    bool checkDrift = true;

    // determine the best estimate of the start time offset that reproduces the sent remote
    // time and its expected quality.
    REAL remoteStartTimeOffset = 0;
    REAL remoteTimeNonQuality = 0;

    // determine ping
    nPingAverager & pingAverager = sn_Connections[0].ping;
    REAL ping = pingAverager.GetPing();
    REAL pingVariance = pingAverager.GetFastAverager().GetAverageVariance();

    {
        // adapt remote time a little, we're processing syncs with a frame delay
        REAL remote_currentTime = remoteCurrentTime_ + averageSpf_.GetAverage();
        REAL remoteSpeed = remoteSpeed_;
 
        // forget about earlier syncs if the speed changed
        if ( fabs( speed - remoteSpeed ) > 10 * EPS )
        {
            qualityTester_.Timestep( 100 );
            startTimeOffset_.Reset();
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

#ifdef DEBUG_X
    con << "T=" << smoothedSystemTime_ << " " << lastRemoteTime_ << ", Q= " << remoteTimeNonQuality << ", P= " << ping << " +/- " << sqrt(pingVariance) << " Time offset: " << remoteStartTimeOffset << " : " << startTimeOffset_.GetAverage() << "\n";

    std::ofstream plot("timesync.txt",std::ios_base::app);
    plot << smoothedSystemTime_ << " " << remoteStartTimeOffset << " " << startTimeOffset_.GetAverage() << "\n";
#endif

    // check for unusually delayed packets, give them lower weight
    REAL delay =  remoteStartTimeOffset - startTimeOffset_.GetAverage();
    if( delay > 0 && !tRecorder::DesyncedPlayback())
    {
        // fluctuations up to the ping variance are acceptable,
        // (plus some extra factors, like a 1 ms offset and 10% of the current ping)
        // severity will be < 1 for those
        REAL severity = delay*delay/(.000001 + pingVariance + .01*ping*ping);
        // but it is culled above 1
        if( severity > 1 )
        {
            severity = pow(severity,.25);
        }

        REAL cullFactor = (se_cullDelayEnd-lastRemoteTime_)*se_cullDelayPerSecond;
        if( cullFactor < 0 )
        {
            cullFactor = 0;
        }
        cullFactor += se_cullDelayMin;

#ifdef DEBUG_X
        con << "severity=" << severity << ", cull= " << cullFactor << "\n";
#endif
        remoteTimeNonQuality *= 1+(cullFactor-1)*severity;
    }

    // add the offset to the statistics, weighted by the quality
    startTimeOffset_.Add( remoteStartTimeOffset, 1/remoteTimeNonQuality );
}

static nNOInitialisator<eTimer> eTimer_init(210,"eTimer");

nDescriptor &eTimer::CreatorDescriptor() const{
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

REAL eTimer::Time()
{
    return ( smoothedSystemTime_ - startTime_ ) - startTimeSmoothedOffset_ + eLag::Current();
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

    // process sync now before an unusually large timestep messes up averageSpf_.
    if( sync_ )
    {
        ProcessSync();
    }

    if ( timeStep > 0 && speed > 0 )
    {
        averageSpf_.Add(timeStep);
        averageSpf_.Timestep(timeStep);
        fpsCounter_->Tick(timeStep);
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

        if ( !std::isfinite( startTimeSmoothedOffset_ ) )
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

    sync_ = false;

    qualityTester_.Reset();
    static const REAL qual = sqrt(1/EPS);
    qualityTester_.Add(qual,EPS);
    qualityTester_.Add(-qual,EPS);

    // reset times of actions
    lastTime_ = nextSync_ = smoothedSystemTime_;
    startTimeSmoothedOffset_ = startTimeOffset_.GetAverage();
}

int eTimer::FPS() const noexcept
{
    return fpsCounter_->GetFPS();
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
    se_mainGameTimerHolder=nullptr;
    if (se_mainGameTimer)
        delete se_mainGameTimer;
    se_mainGameTimer=NULL; // to make sure
}


void se_ResetGameTimer(REAL t){
    if (se_mainGameTimer)
        se_mainGameTimer->Reset(t);
}

void se_PauseGameTimer(bool p,  eTimerPauseSource source){
    static int pausedBySources = eTimerPauseSource::eTIMER_PAUSE_NONE;
    if(p)
    {
        pausedBySources |= source;
    }
    else
    {
        pausedBySources &= ~source;
    }

    if (se_mainGameTimer && sn_GetNetState()!=nCLIENT)
    {
        se_mainGameTimer->pause(pausedBySources != eTimerPauseSource::eTIMER_PAUSE_NONE);
    }
}

REAL se_AverageFrameTime() noexcept
{
    if (se_mainGameTimer)
        return se_mainGameTimer->AverageFrameTime();
    else
        return 1 / 60.0;
}

int se_FPS() noexcept
{
    if (se_mainGameTimer)
        return se_mainGameTimer->FPS();
    else
        return (60);
}

REAL se_PredictTime() noexcept
{
    return se_AverageFrameTime()*.5;
}
