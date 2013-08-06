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

#ifndef ArmageTron_TIMER_H
#define ArmageTron_TIMER_H

//#include <time>
#include "tSysTime.h"
#include "nNetObject.h"

class eTimer:public nNetObject{
public:
    REAL speed; // the time acceleration

    eTimer();
    eTimer(nMessage &m);
    virtual ~eTimer();
    virtual void WriteSync(nMessage &m);
    virtual void ReadSync(nMessage &m);
    virtual nDescriptor &CreatorDescriptor() const;

    REAL Time();
    REAL TimeNoSync(){return REAL(Time()+(tSysTimeFloat()-lastTime_)*speed);}

    void pause(bool p);

    void SyncTime();
    void Reset(REAL t=0);

    REAL AverageFPS(){return 1/(averageSpf_.GetAverage()+EPS);}
    REAL AverageFrameTime(){return averageSpf_.GetAverage();}
    REAL FrameTime(){return spf_;}

    bool IsSynced() const; //!< returns whether the timer is synced sufficiently well to allow rendering

private:
    mutable double creationSystemTime_; //!< the rough system time this timer was created at
    double smoothedSystemTime_;         //!< the smoothed system time
    double startTime_;                  //!< when was the last game started?
    nAverager startTimeOffset_;         //!< the smoothed average of this averager is added to the start time on the client
    nAverager  startTimeDrift_;         //!< drift of effective start time
    REAL startTimeSmoothedOffset_;      //!< the smoothed average of startTimeOffset_
    nAverager qualityTester_;           //!< averager that tells us about the quality of the sync messages

    REAL lastStartTime_;                //!< last received start time
    REAL lastRemoteTime_;               //!< last received time

    bool sync_;                         //!< is a sync ready to process?
    REAL remoteCurrentTime_, remoteSpeed_; //!< if so, the sync data
    void ProcessSync();                 //!< processes the sync data

    // the current game time is always smoothedSystemTime_ - ( startTime_ + startTimeSmoothedOffset_ ).

    REAL spf_;               //!< last frame time
    nAverager averageSpf_;   //!< averager over seconds per frame

    double lastTime_;        //!< the smoothed system time of the last update
    double nextSync_;        //!< system time of the next sync to the clients
};

REAL se_GameTime();
REAL se_GameTimeNoSync();
void se_SyncGameTimer();

void se_ResetGameTimer(REAL t=0);
void se_MakeGameTimer();
void se_KillGameTimer();
void se_PauseGameTimer(bool p);

REAL se_PredictTime();
REAL se_Time();
REAL se_AverageFrameTime();
REAL se_AverageFPS();

extern eTimer *se_mainGameTimer;
#endif


