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

#include "config.h"

#include "tSysTime.h"
#include "tRecorder.h"
#include "tError.h"

// Both implementations are stolen from Q1.

//! time structure
struct tTime
{
    int microseconds;   //! microseconds
    int seconds;        //! seconds

    #define NORMALIZER 1000000

    tTime(): microseconds(0), seconds(0){}

    //! makes sure microseconds is between 0 and NORMALIZER-1
    void Normalize()
    {
        int overflow = microseconds / NORMALIZER;
        microseconds -= overflow * NORMALIZER;
        seconds += overflow;

        while ( microseconds < 0 )
        {
            microseconds += NORMALIZER;
            seconds --;
        }
    }

    tTime operator + ( const tTime & other )
    {
        tTime ret;
        ret.microseconds = other.microseconds + microseconds;
        ret.seconds = other.seconds + seconds;

        ret.Normalize();
        return ret;
    }

    tTime operator - ( const tTime & other )
    {
        tTime ret;
        ret.microseconds = -other.microseconds + microseconds;
        ret.seconds = -other.seconds + seconds;

        ret.Normalize();
        return ret;
    }
};


#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h> 
#ifndef DEDICATED
#include "rSDL.h"
#endif

void GetTime( tTime & time )
{
    struct _timeb tstruct;
    LARGE_INTEGER mtime,frq;

    // Check if high-resolution performance counter is supported
    if (!QueryPerformanceFrequency(&frq))
    {
        // Nope, not supported, do it the old way.
        _ftime( &tstruct );
        time.microseconds = tstruct.millitm*1000;
        time.seconds = tstruct.time;
    }
    else
    {
        QueryPerformanceCounter(&mtime);
        time.seconds = mtime.QuadPart/frq.QuadPart;
        time.microseconds = ( ( mtime.QuadPart - time.seconds * frq.QuadPart ) * 1000000 ) / frq.QuadPart;
    }


    time.Normalize();

}

//! returns true if a timer with more than millisecond accuracy is available
bool tTimerIsAccurate()
{
    LARGE_INTEGER dummy;
    return QueryPerformanceFrequency(&dummy);
}

/*
void GetTime( tTime & time )
{
    struct _timeb tstruct;
    _ftime( &tstruct );

    time.microseconds = tstruct.millitm*1000;
    time.seconds = tstruct.time;

    time.Normalize();
}
*/

// static double	blocktime;

static unsigned int sleep_rest=0;
void usleep(int x)
{
    sleep_rest+=x;
    unsigned int r=sleep_rest/1000;
#ifndef DEDICATED
    SDL_Delay(r);
#else

#ifdef DEBUG
    double tb = tSysTimeFloat();
#endif

    SleepEx(r,false);

#ifdef DEBUG
    double ta = tSysTimeFloat();
    if ((tb-ta) > r*.01)
    {
        int x;
        x = 1;
    }
#endif

#endif
    sleep_rest-=r*1000;
}

#else

#include <sys/time.h>

void GetTime( tTime & time )
{
    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    time.microseconds = tp.tv_usec;
    time.seconds = tp.tv_sec;

    time.Normalize();
}

//! returns true if a timer with more than millisecond accuracy is available
bool tTimerIsAccurate()
{
    return true; // always on unix
}

#endif

static char const * recordingSection = "T";

template< class Archiver > class TimeArchiver
{
public:
    static bool Archive( tTime & time )
    {
        // start archive block if archiving is active
        Archiver archive;
        if ( archive.Initialize( recordingSection ) )
        {
            archive.Archive( time.seconds ).Archive( time.microseconds );
            return true;
        }

        return false;
    }
};

static struct tTime timeStart;        // the time at the start of the program
static struct tTime timeRelative;     // the time since the system start ( eventually from a playback )

void tAdvanceFrameSys( tTime & start, tTime & relative )
{
    struct tTime time;

    // get time from OS
    GetTime( time );
    if ( start.microseconds == 0 && start.seconds == 0 )
        start = time;
    relative = time - start;
}

static bool s_delayedInPlayback = false;
void tDelay( int usecdelay )
{
    // delay a bit if we're not playing back
    if ( ! tRecorder::IsPlayingBack() )
        usleep( usecdelay );
    else
        s_delayedInPlayback = true;
}

void tDelayForce( int usecdelay )
{
    // delay a bit
    if ( !s_delayedInPlayback )
        usleep( usecdelay );
    else
    {
        // when recording, the machine was idling around. No need to play that back.
        // Only pretend to delay.
        tTime timeDelay;
        timeDelay.microseconds = usecdelay;
        timeStart = timeStart - timeDelay;
    }

    s_delayedInPlayback = false;
}

void tAdvanceFrame( int usecdelay )
{
    // delay a bit if we're not playing back
    if ( usecdelay > 0 )
        tDelay( usecdelay );

    tTime timeNewRelative;
    tAdvanceFrameSys( timeStart, timeNewRelative );

    // try to fetch time from playback
    // tTime timeAdvance;
    if ( TimeArchiver< tPlaybackBlock >::Archive( timeRelative ) )
    {
        // timeRelative = timeRelative + timeAdvance;

        // correct start time so transition to normal time after the recording ended is smooth
        timeStart = timeStart + timeNewRelative - timeRelative;
    }
    else
    {
        // must never be called when a recording is running
        tASSERT( !tRecorder::IsPlayingBack() );

#ifdef FIXED_FRAMERATE
        tTime fixed, catchup, timeFixedRelative;
    fixed.seconds = 0;
    fixed.microseconds = (int)(1000000. / FIXED_FRAMERATE);
        timeFixedRelative = timeRelative + fixed;

    catchup = timeFixedRelative/*game-time*/ - timeNewRelative/*real-time*/;
    if (catchup.seconds >= 0) {
#ifdef DEBUG
            printf("catching up %d.%d seconds\n", catchup.seconds, catchup.microseconds);
#endif
            for (int i = catchup.seconds; i; --i)
                tDelay(1000000);
            tDelay(catchup.microseconds);
            timeNewRelative = timeFixedRelative;
        } else {
            printf("WARNING: Real time ahead of game time!\nreal: %d.%06d\ngame: %d.%06d\ncatchup: %d.%06d\n", timeNewRelative.seconds, timeNewRelative.microseconds, timeFixedRelative.seconds, timeFixedRelative.microseconds, 1 - catchup.seconds, 1000000 - catchup.microseconds);
#ifdef FIXED_FRAMERATE_PRIORITY
            timeNewRelative = timeFixedRelative;
#endif
        }
#endif

        // timeAdvance = timeRealRelative - timeRelative;
        timeRelative = timeNewRelative;
    }

    // try to archive it
    TimeArchiver< tRecordingBlock >::Archive( timeRelative );
#ifdef DEBUG
    {
        if ( timeRelative.microseconds == 337949 && timeRelative.seconds == 25 )
        {
            st_Breakpoint();
        }
    }
#endif
}

double tSysTimeFloat ()
{
#ifdef DEBUG
    if ( ! tRecorder::IsPlayingBack() )
    {
        tTime time;
        tAdvanceFrameSys( timeStart, time );
        time = time - timeRelative;
        //        if ( time.seconds > 5 )
        //        {
        //            std::cout << "tAdvanceFrame not called often enough!\n";
        //            st_Breakpoint();
        //            tAdvanceFrameSys( timeRealRelative );
        //        }
    }
#endif

    return timeRelative.seconds + timeRelative.microseconds*0.000001;
}

static struct tTime timeRealStart;    // the real time at the start of the program
static struct tTime timeRealRelative; // the time since the system start

double tRealSysTimeFloat ()
{
    // get real time from real OS
    tAdvanceFrameSys( timeRealStart, timeRealRelative );
    return timeRealRelative.seconds + timeRealRelative.microseconds*0.000001;
}
