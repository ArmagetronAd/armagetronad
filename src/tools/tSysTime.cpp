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

#include "aa_config.h"

#include "tSysTime.h"
#include "tRecorder.h"
#include "tError.h"
#include "tConsole.h"
#include "tConfiguration.h"
#include "tLocale.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

// Both implementations for Windows and Linux were originally from the Q1 source,
// but by now, all similarities should be gone.

// set to true if the timer is strictly going upwards and without leaps
static bool st_timerIsStrictlyMonotonic = false;

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

// test tick count overflow by wrapping the real tick count
// #define TEST_TICK_COUNT
#ifdef TEST_TICK_COUNT
DWORD GetTickCount2()
{
    static DWORD first = GetTickCount();
    return GetTickCount() - first - 10000;
}
#define GetTickCount GetTickCount2
#endif


// flag indicating whether the HPC is reliable
static bool st_hpcReliable = false;

void GetTimeInner( tTime & time )
{
    LARGE_INTEGER mtime,frq;

    // these timers are always monotonic
    st_timerIsStrictlyMonotonic = true;

    // Check if high-resolution performance counter is supported
    if (!QueryPerformanceFrequency(&frq))
    {
        st_hpcReliable = false;
    }

    if (st_hpcReliable)
    {
        QueryPerformanceCounter(&mtime);
        time.seconds = mtime.QuadPart/frq.QuadPart;
        time.microseconds = ( ( mtime.QuadPart - time.seconds * frq.QuadPart ) * 1000000 ) / frq.QuadPart;
    }
    else
    {

#ifdef SUPPORT_WIN9X
        // Nope, not supported, do it the old way.
        struct _timeb tstruct;
        _ftime( &tstruct );
        time.microseconds = tstruct.millitm*1000;
        time.seconds = tstruct.time;

        // not monotonic :(
        st_timerIsStrictlyMonotonic = false;
#else
        // Or, better, GetTickCount(). Be aware of overflows after two months uptime.
        // (TickCount64 only exists on Vista and up, we don't want to go cutting compatibility off that far yet.)
        static DWORD lastTickCount = GetTickCount();
        DWORD tickCount = GetTickCount();
        static int64_t tick64 = 0;
        unsigned int tickUpdate = tickCount - lastTickCount;
        lastTickCount = tickCount;
        tick64 += tickUpdate;
        int milliseconds = tick64 % 1000;
        time.microseconds = milliseconds * 1000;
        time.seconds = (tick64 - milliseconds)/1000;
#endif
    }

    time.Normalize();
}

void GetTime( tTime & relative )
{
    tTime time;
    GetTimeInner( time );
    static tTime start = time;
    relative = time - start;

    // detect timer trouble
    if ( st_hpcReliable )
    {
        static struct tTime lastTime = relative;

        // test transition
        //if ( relative.seconds > 10 )
        //    lastTime.seconds = lastTime.seconds+1;

        if ( (time - lastTime).seconds < 0 )
        {
            st_hpcReliable = false;

            GetTimeInner( time );
            start = start + time - lastTime;
            relative = time - start;
        }
        lastTime = time;
    }
}

//! returns true if a timer with more than millisecond accuracy is available
bool tTimerIsAccurate()
{
    return st_hpcReliable;
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

// if possible, use clock_gettime()
#if HAVE_LIBRT && HAVE_TIME_H
#ifdef CLOCK_MONOTONIC
#define USE_CLOCK_GETTIME
// for testing, z-man on Lucid doesn't have _RAW defined yet
// #define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif
#endif

#ifdef MACOSX
//#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

void GetTime( tTime & time )
{
	// get the absolute time
	uint64_t machTime = mach_absolute_time();
	static uint64_t start = machTime;
	machTime -= start;
	static double machTimeToMicroseconds;
	if ( 0 == machTime )
	{
		static mach_timebase_info_data_t    timebaseInfo;
		(void) mach_timebase_info(&timebaseInfo);
		machTimeToMicroseconds = timebaseInfo.numer*1E-3/timebaseInfo.denom;
	}
	
	// convert to microseconds, carefully avoiding overflows and rounding trouble
	uint64_t microseconds = machTime*machTimeToMicroseconds;
	time.microseconds = microseconds % 1000000;
	time.seconds = (microseconds - time.microseconds)/1000000;
}

#else // macosx

#include <sys/time.h>

void GetTime( tTime & time )
{
#ifdef USE_CLOCK_GETTIME
    struct timespec res;
    // try several clocks
    bool success = false;
    st_timerIsStrictlyMonotonic = false;
// prefer CLOCK_MONOTONIC_RAW, but if that doesn't exist, go without _RAW
#ifdef CLOCK_MONOTONIC_RAW
    if( !clock_gettime( CLOCK_MONOTONIC_RAW, &res ) )
    {
        success = true;
        st_timerIsStrictlyMonotonic = true;
    }
    else
#endif
    {
        if( !clock_gettime( CLOCK_MONOTONIC, &res ) )
        {
            success = true;
        }
    }

    // transfer the result or go on with gettimeofday()
    if( success )
    {
        time.microseconds = res.tv_nsec/1000;
        time.seconds      = res.tv_sec;

        return;
    }
#endif

    struct timeval tp;
    struct timezone tzp;

    gettimeofday(&tp, &tzp);

    time.microseconds = tp.tv_usec;
    time.seconds = tp.tv_sec;

    time.Normalize();
}
#endif // macosx

//! returns true if a timer with more than millisecond accuracy is available
bool tTimerIsAccurate()
{
    return true; // always on unix
}
#endif // windows

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

    // test hiccupery
    // time.seconds -= time.seconds/10;

    // record starting point
    if ( start.microseconds == 0 && start.seconds == 0 )
    {
        start = time;
    }

    // detect and counter timer hiccups
    tTime newRelative = time - start;
    tTime timeStep = newRelative - relative;
    if ( !tRecorder::IsPlayingBack() && ( timeStep.seconds < 0 || ( !st_timerIsStrictlyMonotonic && timeStep.seconds > 10 ) ) )
    {
        static bool warn = true;
        if ( warn )
        {
            warn = false;
            con << tOutput( "$timer_hickup", float( timeStep.seconds + timeStep.microseconds * 1E-6  ) );
        }

        start = start + timeStep;
    }
    else
    {
        relative = newRelative;
    }

#ifdef DEBUG
    if ( relative.seconds > 20 )
    {
        int x;
        x = 0;
    }
#endif
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

    static tTime timeNewRelative;
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


#ifdef DEBUG_X
    {
        static tTime oldRelative = timeRelative;
        tTime timeStep = timeRelative - oldRelative;
        oldRelative = timeRelative;

        // detect unusually large timesteps
        static REAL bigStep = 10;
        bigStep *= .99;
        REAL step = float( timeStep.seconds + timeStep.microseconds * 1E-6  );
        if ( step > 3 * bigStep )
        {
            con << "Small timer hiccup of " << step << " seconds.\n";
            bigStep = bigStep * 1.02;
        }
        else if ( step > bigStep )
        {
            bigStep = step;
        }
    }
#endif

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

static float st_timeFactor = 1.0;
static tSettingItem< float > st_timeFactorConf( "TIME_FACTOR", st_timeFactor );

double tSysTimeFloat ()
{
#ifdef DEBUG
    // if ( ! tRecorder::IsPlayingBack() )
    // {
    // static tTime time;
    // tAdvanceFrameSys( timeStart, time );
    // tTime timeStep = time - timeRelative;
    //        if ( timeStep.seconds > 5 )
    //        {
    //            std::cout << "tAdvanceFrame not called often enough!\n";
    //            st_Breakpoint();
    //            tAdvanceFrameSys( timeRealRelative );
    //        }
    // }
#endif

    return ( timeRelative.seconds + timeRelative.microseconds*1E-6 ) * st_timeFactor;
}

static struct tTime timeRealStart;    // the real time at the start of the program
static struct tTime timeRealRelative; // the time since the system start

double tRealSysTimeFloat ()
{
    // get real time from real OS
    tAdvanceFrameSys( timeRealStart, timeRealRelative );
    return ( timeRealRelative.seconds + timeRealRelative.microseconds*1E-6 ) * st_timeFactor;
}
