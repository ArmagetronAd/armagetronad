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

#ifndef ArmageTron_TIMER_H
#define ArmageTron_TIMER_H

//#include <time>
#include "tSysTime.h"
#include "nNetObject.h"

namespace Engine { class TimerSync; }

//! gets the mininum of the last X samples
template< int NUM >
class eSampleMin
{
public:
    eSampleMin()
    {
        Clear();
    }
    
    void Clear()
    {
        current = 0;
        for( int i = NUM-1; i >= 0; --i )
        {
            samples[i] = 1E+30;
        }
    }

    void Add( double sample )
    {
        samples[current] = sample;
        current = (current+1)%NUM;
    }

    double GetMin() const
    {
        double ret = samples[NUM-1];
        for( int i = NUM-2; i >= 0; --i )
        {
            if( samples[i] < ret )
            {
                ret = samples[i];
            }
        }

        return ret;
    }
private:
    double samples[NUM];
    int  current;
};

//! timer class
class eTimer:public nNetObject{
public:
    REAL speed; // the time acceleration

    eTimer();
    virtual ~eTimer();

    eTimer( Engine::TimerSync const &, nSenderInfo const & );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Engine::TimerSync const &, nSenderInfo const & );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Engine::TimerSync &, bool init ) const;

    REAL Time() const;
    REAL TimeNoSync(){return REAL(Time()+(tSysTimeFloat()-lastTime_)*speed);}

    void pause(bool p);

    void SyncTime();
    void Reset(REAL t=0,bool force=false);

    REAL AverageFPS(){return 1/(averageSpf_.GetAverage()+EPS);}
    REAL AverageFrameTime(){return averageSpf_.GetAverage();}
    REAL FrameTime(){return spf_;}

    bool IsSynced() const; //!< returns whether the timer is synced sufficiently well to allow rendering

private:
    void UpdateIsSynced(); //!< updates the synced flag

    REAL Drift() const;    //!< returns the drift
private:
    void ResetAveragers(); //!< resets the internal averagers

    bool synced_;                       //!< set to true when the client timer is synced up
    double creationSystemTime_;         //!< the rough system time this timer was created at
    double smoothedSystemTime_;         //!< the smoothed system time
    double startTime_;                  //!< when was the last game started?
    double startTimeExtrapolated_;      //!< when was the last game started (local extrapolation)?
    nAverager startTimeOffset_;         //!< the smoothed average of this averager is added to the start time on the client
    nAverager  startTimeDrift_;         //!< drift of effective start time
    bool drifting_;                     //!< set if drifting was detected, never unset
    double startTimeSmoothedOffset_;    //!< the smoothed average of startTimeOffset_
    nAverager qualityTester_;           //!< averager that tells us about the quality of the sync messages


    // for drift calculation
    double lastStartTime_;              //!< last received start time
    double lastRemoteTime_;             //!< last received time

    bool sync_;                         //!< is a sync ready to process?
    double remoteCurrentTime_, remoteSpeed_; //!< if so, the sync data
    double remoteStartTime_;            //!< if so, the sync data
    bool remoteStartTimeSent_;          //!< set if the server is sending the start time
    double lastRemoteStartTime_;        //!< last receined remote start time
    double remoteStartTimeOffsetClamped_; //!< the remote start time offset, sanity checked against 
    eSampleMin<16> remoteStartTimeOffsetMin_; //!< minimal remote start time offset of the last X syncs
    double remoteStartTimeOffsetSanitized_; //!< the remote start time offset, sanity checked against too large deviations and spikes

    void ProcessSync();                 //!< processes the sync data

    // the current game time is always smoothedSystemTime_ - ( startTime_ + startTimeSmoothedOffset_ ).

    REAL spf_;               //!< last frame time
    nAverager averageSpf_;   //!< averager over seconds per frame

    double lastTime_;        //!< the smoothed system time of the last update
    mutable double nextSync_; //!< system time of the next sync to the clients

    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const;
};

REAL se_GameTime();
REAL se_GameTimeNoSync();
void se_SyncGameTimer();

void se_ResetGameTimer(REAL t=0);
void se_MakeGameTimer();
void se_KillGameTimer();
void se_PauseGameTimer(bool p);

REAL se_PredictTime();
REAL se_AverageFrameTime();
REAL se_AverageFPS();

extern eTimer *se_mainGameTimer;
#endif


