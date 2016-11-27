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

#include "defs.h"

#include "rGL.h"

#ifndef DEDICATED
#include "rSDL.h"
#include "rGLEW.h"
#endif

#include "rSysdep.h"
#include "tInitExit.h"
#include "tDirectories.h"
#include "tSysTime.h"
#include "rConsole.h"
#include "aa_config.h"
#include <iostream>
#include "rScreen.h"
#include "rTexture.h"
#include "tCommandLine.h"
#include "tConfiguration.h"
#include "tRecorder.h"
#include "rTextureRenderTarget.h"
#include <memory>

#ifndef DEDICATED
#include "SDL_thread.h"
#include "SDL_mutex.h"

//#ifndef WIN32
//#define PNG_SCREENSHOT
//#else
//#if !SDL_VERSION_ATLEAST(2, 0, 0)
#define PNG_SCREENSHOT
//#endif
//#endif

#ifdef PNG_SCREENSHOT
#include <png.h>
#endif
#include <unistd.h>
#define SCREENSHOT_PNG_BITDEPTH 8
#define SCREENSHOT_BYTES_PER_PIXEL 3

#ifndef SDL_OPENGL
#error "need SDL 1.1"
#endif

#ifndef DEDICATED
// fence support
class rGLFence: public rGLuintObject
{
public:
    // check whether fences are availalbe
    static bool Available()
    {
        static bool ret = CheckAvailable();
        return ret;
    }

    // sets the fence up
    void Set()
    {
        sr_CheckGLError();
#ifdef GLEW_NV_fence
        if( GLEW_NV_fence )
        {
            glSetFenceNV( *this, GL_ALL_COMPLETED_NV );
        }
#endif
#ifdef GLEW_APPLE_fence
        if( GLEW_APPLE_fence )
        {
            glSetFenceAPPLE( *this );
        }
#endif
        sr_CheckGLError();
    }

    // finishes the fence (waits for all commands sent before it was set to be finished)
    void Finish()
    {
        sr_CheckGLError();
#ifdef GLEW_NV_fence
        if( GLEW_NV_fence )
        {
            glFinishFenceNV( *this );
        }
#endif
#ifdef GLEW_APPLE_fence
        if( GLEW_APPLE_fence )
        {
            glFinishFenceAPPLE( *this );
        }
#endif
        sr_CheckGLError();
    }

    virtual ~rGLFence()
    {
        Delete();
    }
private:
    static bool CheckAvailable()
    {
#ifdef HAVE_GLEW
#ifdef GLEW_NV_fence
        if( GLEW_NV_fence )
        {
            return true;
        }
#endif
#ifdef GLEW_APPLE_fence
        if( GLEW_APPLE_fence )
        {
            return true;
        }
#endif
#endif
        // fallback: no fence
        return false;
    }

    virtual void DoGen()       //!< really reserves the object
    {
#ifdef GLEW_NV_fence
        if( GLEW_NV_fence )
        {
            glGenFencesNV( 1, &object_ );
        }
#endif
#ifdef GLEW_APPLE_fence
        if( GLEW_APPLE_fence )
        {
            glGenFencesAPPLE( 1, &object_ );
        }
#endif
    }

    virtual void DoDelete()    //!< really frees the object
    {
#ifdef GLEW_NV_fence
        if( GLEW_NV_fence )
        {
            glDeleteFencesNV( 1, &object_ );
        }
#endif
#ifdef GLEW_APPLE_fence
        if( GLEW_APPLE_fence )
        {
            glDeleteFencesAPPLE( 1, &object_ );
        }
#endif
    }
};

// returns the fence to be used for syncing the GPU and CPU
static rGLFence & sr_GetFence()
{
    static rGLFence fence;
    fence.Set();
    return fence;
}

#endif // DEDICATED

bool sr_screenshotIsPlanned=false;
tString sr_screenshotName("screenshot");
static bool   s_videoout    =false;
static int    s_videooutDest=fileno(stdout);

static bool png_screenshot=true;
static tConfItem<bool> pns("PNG_SCREENSHOT",png_screenshot);
#ifndef DEDICATED

#ifdef PNG_SCREENSHOT
static void SDL_SavePNG(SDL_Surface *image, tString filename){
    png_structp png_ptr;
    png_infop info_ptr;
    png_byte **row_ptrs;
    int i;
    static FILE *fp;

    if (!(fp = fopen(filename, "wb"))) {
        fprintf(stderr, "can't open file for writing\n");
        return;
    }

    if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
        return;
    }

    if (!(info_ptr = png_create_info_struct(png_ptr))) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, sr_screenWidth, sr_screenHeight,
                 SCREENSHOT_PNG_BITDEPTH, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    // get pointers
    if (!(row_ptrs = (png_byte**) malloc(sr_screenHeight * sizeof(png_byte*)))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return;
    }

    for (i = 0; i < sr_screenHeight; i++) {
        row_ptrs[i] = (png_byte *)image->pixels + (sr_screenHeight - i - 1)
                      * image->pitch;
    }

    png_write_image(png_ptr, row_ptrs);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_ptrs);
    fclose(fp);
}
#endif
#endif

static void make_screenshot(){
#ifndef DEDICATED
    // duplicate count (if we already took a screenshot the same second/with the same name)
    int number=0;

    SDL_Surface *image;
    SDL_Surface *temp;
    int idx;
    image = SDL_CreateRGBSurface(SDL_SWSURFACE, sr_screenWidth, sr_screenHeight,
                                 24, 0x0000FF, 0x00FF00, 0xFF0000 ,0);
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, sr_screenWidth, sr_screenHeight,
                                24, 0x0000FF, 0x00FF00, 0xFF0000, 0);

    // make upside down screenshot (make sure it comes from the screen)

    glReadPixels(0,0,sr_screenWidth, sr_screenHeight, GL_RGB,
                 GL_UNSIGNED_BYTE, image->pixels);

    // turn image around
    for (idx = 0; idx < sr_screenHeight; idx++)
    {
        memcpy(reinterpret_cast<char *>(temp->pixels) + temp->pitch * idx,
               reinterpret_cast<char *>(image->pixels)
               + image->pitch*(sr_screenHeight - idx-1),
               3*sr_screenWidth); // Optionally, use the pitch of either surface here
    }

    if (s_videoout)
    {
        for (idx = 0; idx < sr_screenHeight; idx++)
        {
            Ignore( write(s_videooutDest, reinterpret_cast<char *>(temp->pixels) + temp->pitch * idx, sr_screenWidth * 3) );
        }
    }

    if (sr_screenshotIsPlanned) {
        // save screenshot in unused slot
        bool done = false;
        while ( !done )
        {
            // generate filename
            tString fileName(sr_screenshotName);
            if(number)
            {
                fileName << '_' << number;
            }
            if (png_screenshot)
                fileName << ".png";
            else
                fileName << ".bmp";

            // test if file exists
            std::ifstream s;
            if ( tDirectories::Screenshot().Open( s, fileName ) )
            {
                // yes! try next number
                number++;
                continue;
            }

            // save image
#ifdef PNG_SCREENSHOT
            if (png_screenshot)
                SDL_SavePNG(image, tDirectories::Screenshot().GetWritePath( fileName ));
            else
#endif
                SDL_SaveBMP(temp, tDirectories::Screenshot().GetWritePath( fileName ) );
            done = true;
        }
    }

    // cleanup
    SDL_FreeSurface(image);
    SDL_FreeSurface(temp);
#endif
}

class PerformanceCounter
{
public:
    PerformanceCounter(): count_(0){
        start_ = tRealSysTimeFloat();
    }
    unsigned int Count(){
        return count_++;
    }
    ~PerformanceCounter()
    {
        double time = tRealSysTimeFloat()-start_;
        std::stringstream s;
        s << count_ << " frames in " << time << " seconds: " << count_ / time << " fps.\n";
#ifdef WIN32
        MessageBox (NULL, s.str().c_str() , "Performance", MB_OK);
#else
        std::cout << s.str();
#endif
    }
private:
    unsigned int count_;
    double start_;
};

static double s_nextFastForwardFrameRecorded=0; // the next frame to render in recorded time
static double s_nextFastForwardFrameReal=0;     // the next frame to render in real time
#endif // DEDICATED

// settings for fast forward mode
static REAL sr_FF_Maxstep=1; // maximum step between rendered frames
static tSettingItem<REAL> c_ff( "FAST_FORWARD_MAXSTEP",
                                sr_FF_Maxstep );

static REAL sr_FF_MaxstepReal=.05; // maximum step in real time between rendered frames
static tSettingItem<REAL> c_ffre( "FAST_FORWARD_MAXSTEP_REAL",
                                  sr_FF_MaxstepReal );

static REAL sr_FF_MaxstepRel=1; // maximum step between rendered frames relative to end of FF mode
static tSettingItem<REAL> c_ffr( "FAST_FORWARD_MAXSTEP_REL",
                                 sr_FF_MaxstepRel );


static double s_fastForwardTo=0;
static bool   s_fastForward =false;
static bool   s_benchmark   =false;

class rFastForwardCommandLineAnalyzer: public tCommandLineAnalyzer
{
private:
    virtual bool DoAnalyze( tCommandLineParser & parser )
    {
        // get option
        tString forward;
        if ( parser.GetOption( forward, "--fastforward" ) )
        {
            // set fast forward mode
            s_fastForward = true;

            // read time
            std::stringstream str(static_cast< char const * >( forward ) );
            str >> s_fastForwardTo;

            return true;
        }

        if ( parser.GetSwitch( "--benchmark" ) )
        {
            // set benchmark mode
            s_benchmark = true;
            return true;
        }

#ifndef DEDICATED
        if ( parser.GetSwitch( "--videoout" ) )
        {
            // redirect all regular output to stderr
            if ((s_videooutDest = dup(fileno(stdout))) == -1)
                std::cout << "Warning: Failed to duplicate stdout descriptor for video\n";
            else {
                if (-1 == dup2(fileno(stderr), fileno(stdout)))
                    std::cout << "Warning: Failed to redirect default output to stderr\n";
                else
                    std::cout << "Video Output: normal output redirected to stderr\n";
            }
            // set video out mode
            s_videoout = true;
            return true;
        }
#endif

        return false;
    }

    virtual void DoHelp( std::ostream & s )
    {                                      //
        s << "--fastforward <time>         : lets time run very fast until the given time is reached\n";
        s << "--benchmark                  : renders frames as they were recorded\n";
#ifndef DEDICATED
        s << "--videoout                   : writes a raw video stream of frames to stdout\n";
#endif
    }
};

static rFastForwardCommandLineAnalyzer analyzer;

// #define MILLION 1000000

rSysDep::rSwapOptimize rSysDep::swapOptimize_ = rSysDep::rSwap_Auto;
rSysDep::rFramedropTolerance rSysDep::framedropTolerance_ = rSysDep::rSwap_Normal;

// random benchmarks. Median of three runs.
// rSwap_Latency          : 382.841
// rSwap_Throughput       : 405.552
// rSwap_ThrougputFlush   : 405.194
// rSwap_ThrougputFastest : 406.676
// same recording, old client:
// rSwap_glFinish         : 395.892
// rSwap_LateFinish       : 398.132
// rSwap_Fence            : 400.248
// rSwap_glFlush          : 398.037
// rSwap_Fastest          : 396.392

#ifndef DEDICATED

// determines minimum of past values
class rRollingMinimum
{
public:
    rRollingMinimum( int level, REAL value )
    :levels_( level )
    {
        for( int i = level-1; i >= 0; --i )
        {
            levels_[i].Add(value);
            levels_[i].Add(value);
        }

        min_ = value;
    }

    // returns the current minimum
    REAL GetMin() const
    {
        return min_;
    }

    // returns sublevel minimum
    REAL GetSubMin(int level) const
    {
        return levels_[level].GetMin();
    }

    int Size()
    {
        return 1 << levels_.Len();
    }

    // adds a value to the list to find a minimum of
    void Add( REAL value )
    {
        if( min_ > value )
        {
            min_ = value;
        }

        int level = levels_.Len()-1;
        while( level >= 0 && levels_[level].Add(value) )
        {
            value = levels_[level].GetMin();
            level--;
        }

        // trown out old data? Recalculate min. Actually, we just
        // can copy it.
        if( level <= 0 )
        {
            min_ = levels_[0].GetMin();
        }
    }
private:
    struct Level
    {
        REAL value[2];
        bool toggle;

        Level( REAL v=0 )
        : toggle(true)
        {
            value[0] = value[1] = v;
        }

        // adds a value, returns true on every second call
        bool Add( REAL v )
        {
            value[0] = value[1];
            value[1] = v;

            toggle = !toggle;
            return toggle;
        }

        // returns the current minimum
        REAL GetMin() const
        {
            return value[0] < value[1] ? value[0] : value[1];
        }
    };

    tArray<Level> levels_;
    REAL min_;
};

#ifdef DEBUG
// #define DEBUG_SWAP
#endif

#ifdef DEBUG_SWAP
#include "tRandom.h"
#endif

#define SWAP_TIMESCALE_LOG 8
// #define SWAP_TIMESCALE_LOG 3
#define SWAP_TIMESCALE (1 << SWAP_TIMESCALE_LOG)

/* LATENCY order:
input
simulate
render
swap
sync
delay

THROUGHPUT order:
input
simulate
render
sync
swap
*/

// flag indicating that the next call to glClear needs execution.
// sometimes, we preemptively call it after swaps.
static bool sr_needClear = true;

static REAL sr_swapDelayFactor = .5f;
static tConfItem< REAL > sr_swapDelayFactorCI("SWAP_LATENCY_DELAY_FACTOR", sr_swapDelayFactor );


// measures time wasted on waiting for swaps
class rSwapTime
{
public:
    rSwapTime()
    : frameTimes_(SWAP_TIMESCALE_LOG+6, 1/20.0)
    , frameTimesMax_(4, -1/20.0)
    , waitTimes_(SWAP_TIMESCALE_LOG+1, 0)
    , delay_(0)
    , smoothDelay_(0)
    , badFrame_( -200 )
    , counter_ ( 100 )
    , inGame_( false )
    , lowFrameTime_( 0.001 )
    {
        lastTime_ = Time();

        // clamp delay factor
        if( sr_swapDelayFactor < .2 )
        {
            sr_swapDelayFactor = .2;
        }
        if( sr_swapDelayFactor > .95 )
        {
            sr_swapDelayFactor = .95;
        }
    }

    static double Time()
    {
#ifdef DEBUG_SWAP
        tAdvanceFrame();
        return tSysTimeFloat();
#else
        return tRealSysTimeFloat();
#endif
    }

    // return the delay for next frame
    REAL GetDelay() const
    {
        return delay_;
    }

    // call while in game
    void IsInGame()
    {
        inGame_ = true;
    }

    rSysDep::rSwapOptimize GetCurrentSwapOptimizeMode() const
    {
        switch( rSysDep::swapOptimize_ )
        {
        case rSysDep::rSwap_Auto:
            // check for ridiculously high framerate
            if (frameTimesMax_.GetMin() > -lowFrameTime_)
            {
                lowFrameTime_ = 1/200.0;

                return rSysDep::rSwap_Throughput;
            }
            else
            {
                lowFrameTime_ = 1/400.0;
            }

            return badFrame_ > 0 ? rSysDep::rSwap_Throughput : rSysDep::rSwap_Latency;
            break;
        default:
            return rSysDep::swapOptimize_;
            break;
        }
    }

    void Swap( bool reallyDoIt ) const
    {
        // do the actual buffer swap.
        if( reallyDoIt )
        {
#if SDL_VERSION_ATLEAST(2,0,0)
            SDL_GL_SwapWindow(sr_screen);
#else
            SDL_GL_SwapBuffers();
#endif

#ifdef DEBUG_SWAP_X
            {
                static int roughStart = SWAP_TIMESCALE*5;
                if( roughStart-- > 0 && (roughStart % 10) == 0 )
                {
                    tDelay( 1000 * 20 );
                }
                else if( roughStart == -1 )
                {
                    st_Breakpoint();
                }
            }
#endif



        }
    }

    // Clears the buffer in advance of the main code requesting it later
    void AdvanceClear()
    {
        sr_needClear = true;
        rSysDep::ClearGL();
        sr_needClear = false;
    }

    // swaps for minimum latency mode, returning the time needed to wait for the GPU
    REAL LatencySwap(bool swap)
    {
        // flush
        if( delay_ > smallDelay/10 )
        {
            // another extra finish if we're planning to to add delays.
            // we only want to measure the time spent waiting for vsync,
            // not the time waiting for rendering to finish.
            glFinish();
        }

        Swap(swap);

        // flush
        double start = Time();
        glFinish();
        REAL ret = Time() - start;

        AdvanceClear();

        return ret;
    }

    // swaps for maximum througput mode, returning the time needed to wait for the GPU
    REAL ThroughputSwap(bool swap)
    {
        REAL ret;

        if( rGLFence::Available() )
        {
            // oddly enough, on NVidia cards, the Swap() already
            // eats up the idle time; for the return value to
            // properly represent time waiting for stuff to finish,
            // we need to include it in the measurements.
            double start = Time();

            Swap(swap);

            static rGLFence & fence = sr_GetFence();

            // finish last frame's fence
            fence.Finish();

            ret = Time() - start;

            // set new fence
            fence.Set();

            AdvanceClear();
        }
        else
        {
            double start = Time();

            // flush
            glFinish();
            ret = Time() - start;

            Swap(swap);

            AdvanceClear();
        }

#ifdef DEBUG_SWAP_X
        static int count = 0;
        if( count-- < 0 )
        {
            count = 60;
            con << "SwapTime " << ret*1000 << "\n";
        }
#endif

        return ret;
    }

    // call after swapping buffers with an argument of true
    // and once just before rendering with an argument
    void Finish( bool swap = false )
    {
        switch( rSysDep::swapOptimize_ )
        {
        case rSysDep::rSwap_ThroughputFastest:
            Swap(swap);
            AdvanceClear();
            break;
        case rSysDep::rSwap_ThroughputFlush:
            Swap(swap);
            glFlush();
            AdvanceClear();
            break;
        case rSysDep::rSwap_Throughput:
            ThroughputSwap(swap);
            break;
        case rSysDep::rSwap_Auto:
            // some special cases. In vsync off situations, don't optimize
            // for latency. The high framerate already takes care of that.
            switch( currentScreensetting.vSync )
            {
            case ArmageTron_VSync_Off:
            case ArmageTron_VSync_MotionBlur:
                ThroughputSwap(swap);
                return;
            default:
                break;
            }
        default:
            FinishComplicated( swap );
        }
    }

    // call after swapping buffers with an argument of true
    // and once just before rendering with an argument
    void FinishComplicated( bool swap = false )
    {
#ifdef DEBUG_SWAP_X
        {
            static tRandomizer randomDelay;
            static int counter = -1000;
            if( counter++ > 0 )
            {
                tDelay( 1+randomDelay.Get( counter ) );
            }
        }
#endif

        rSysDep::rSwapOptimize opt = GetCurrentSwapOptimizeMode();

        REAL neededToWait = ( opt == rSysDep::rSwap_Throughput ) ? ThroughputSwap(swap) : LatencySwap(swap);
        StopSwap( neededToWait, opt );

        // delay
        if( opt == rSysDep::rSwap_Latency && delay_ > smallDelay )
        {
            tDelay( int(delay_ * 1000 * 1000) );
        }
    }
protected:
    // one frame every so many seconds is tolarated
    static REAL FrameDropTolerance()
    {
        switch (rSysDep::framedropTolerance_)
        {
        case rSysDep::rSwap_Lenient:
            return 20;
            break;
        case rSysDep::rSwap_Normal:
            return 60;
            break;
        case rSysDep::rSwap_Strict:
            return 600;
            break;
        default:
            return 60*60*24;
            break;
        }
    }

    // call after swapping
    void StopSwap( REAL timeSpentWaiting, rSysDep::rSwapOptimize opt )
    {
        double now = Time();
        REAL timeSpent = now - lastTime_;

        lastTime_ = now;
        frameTimesMax_.Add( -timeSpent );
        frameTimes_.Add( -frameTimesMax_.GetMin() );

        // check for unusually long frames
        REAL referenceFrameTime = 1/10.0;
        REAL minFrameTime = frameTimes_.GetMin();
        if( minFrameTime > referenceFrameTime )
        {
            minFrameTime = referenceFrameTime;
        }

        // tolerance factor for dropped frames
        static const REAL frameDropTolerance = 1.5;
        static const int framePenaltyMax = 1200;
        static const int framePenaltySingle = framePenaltyMax/10;
        static const int framePenaltyMin = -framePenaltySingle*3;

        // the real time spent waiting, not doing anything, during the last frame
        timeSpentWaiting += delay_;

        bool frameDrop = inGame_ &&
            (
                timeSpent > frameDropTolerance * minFrameTime
                ||
                timeSpentWaiting < smallDelay/10
                );
        inGame_ = false;

        if( frameDrop )
        {
            REAL waitTime = waitTimes_.GetMin();

            // forge the wait time so the next delays get lower.
            if( delay_ > smallDelay/10 )
            {
                timeSpentWaiting = waitTime * .8f;

                // and lower the delay factor for longterm reduction of the delay;
                // count rapid fire framedrops as less important, as well as isolated
                // drops.
                static double lastDrop = now-1;
                REAL weight = (now - lastDrop)/3.0f;
                weight = weight * exp(-3*weight/FrameDropTolerance());
                lastDrop = now;
                if( weight > 1 )
                {
                    weight = 1;
                }
#ifdef DEBUG_SWAP
                con << "delayFactor " << sr_swapDelayFactor;
#endif

                sr_swapDelayFactor *= (1-delayFactorPenalty*weight);

#ifdef DEBUG_SWAP
                con << " -> " << sr_swapDelayFactor << "\n";
#endif
            }

#ifdef DEBUG_SWAP
            if( badFrame_ < framePenaltyMax/2 && opt == rSysDep::rSwap_Latency )
            {
                con << "Framedrop! " << timeSpent*1000 << " > " << frameTimes_.GetMin()*1000 << ", minWait=" << waitTime*1000 << " at " << counter_ << "\n";
            }
#endif

            if( badFrame_ < framePenaltyMax )
            {
                badFrame_ += framePenaltySingle;

                // hysteresis: if we are just disabling latency mode, go all the way and some
                if ( badFrame_ > 0 && badFrame_ <= framePenaltySingle )
                {
#ifdef DEBUG_SWAP
                    con << "Too many framedrops.\n";
#endif
                    badFrame_ = framePenaltyMax*2;
                }
            }
        }
        else
        {
            badFrame_--;

            if( badFrame_ == 0 )
            {
#ifdef DEBUG_SWAP
                con << "Rendering fine again.\n";
#endif
                badFrame_ = framePenaltyMin;

                // just dropping out of throughput mode; better still be a bit careful
                smoothDelay_ = delay_ = 0;
                counter_ = 0;
            }
            else if( badFrame_ < framePenaltyMin )
            {
                badFrame_ = framePenaltyMin;

#ifdef DEBUG_SWAP
                if ( counter_ == 20 )
                {
                    con << "Rendering OK:" << timeSpent*1000 << " < " << frameTimes_.GetMin()*frameDropTolerance*1000 << ", delay " << delay_*1000 << "\n";
                }
#endif
            }
        }

        waitTimes_.Add( timeSpentWaiting );

        // calculate optimal delay
        REAL newDelay = waitTimes_.GetMin() * sr_swapDelayFactor;

        // clear artificial delay if the current swap mode says so
        if( opt == rSysDep::rSwap_Latency &&
            rSysDep::framedropTolerance_ != rSysDep::rSwap_Draconic )
        {
            // let delay factor recover so that there will be about at
            // most one dropped frame every so many
            REAL recovery=2*timeSpent/FrameDropTolerance();
            sr_swapDelayFactor = 1 - (1-sr_swapDelayFactor)*(1-(1-sr_swapDelayFactor)*delayFactorPenalty*recovery);
        }
        else
        {
            delay_ = newDelay = 0;
        }


        // smooth out delay changes. Lowering is immediate
        if( newDelay < smoothDelay_ )
        {
            smoothDelay_ = newDelay;
        }
        else
        {
            // while increases are smoothed with a sub-second timescale.
            REAL w = timeSpent * 3;
            smoothDelay_ = ( smoothDelay_ + newDelay * w )/(1 + w );
        }
        delay_ = smoothDelay_;

        // clear artificial delay if the current swap mode says so
        if( opt != rSysDep::rSwap_Latency ||
            rSysDep::framedropTolerance_ == rSysDep::rSwap_Draconic )
        {
            delay_ = 0;
        }

        // every so many frames, do a test frame with lower delay
        if( counter_-- <= 0 )
        {
            // con << "d=" << delay_ << ", bf=" << badFrame_ << "\n";

            counter_ = waitTimes_.Size();

            // this reduction is enough to get rendering stuck at 30 FPS back
            // to 60 FPS, if that's at all possible.
            delay_ *= .4;
        }
    }
private:
    // minimum total frame time
    rRollingMinimum frameTimes_;

    // maximum short-time frame time (contains negative frametimes so rRollingMinimum
    // can be abused to get the maximum frame time)
    rRollingMinimum frameTimesMax_;

    // minimum wait time per frame
    rRollingMinimum waitTimes_;

    // time sync is started
    double startTime_;

    // last time swap was complete
    double lastTime_;

    // current pre-simulation delay
    REAL delay_;

    // smoothed pre-simulation delay
    REAL smoothDelay_;

    // if a frame dropped, this is set to a finite value and counted down
    int badFrame_;

    // random frame counter
    int counter_;

    // flag indicating whether we're in a game
    bool inGame_;

    // ridiculously low frametime limit
    mutable REAL lowFrameTime_;

    // a frame swap delay that is considered small
    static const REAL smallDelay;

    // reduction in the delay factor whenever a frame is dropped
    static const REAL delayFactorPenalty;
};

const REAL rSwapTime::smallDelay = 1E-3f;
const REAL rSwapTime::delayFactorPenalty = .05;

static rSwapTime & sr_SwapTime()
{
    static rSwapTime ret;
    return ret;
}

#endif // DEDICATED

// buffer swap:
#ifndef DEDICATED
// for setting breakpoints in optimized mode, too
static void breakpoint(){}

static bool sr_netSyncThreadGoOn = true;
static rSysDep::rNetIdler * sr_netIdler = NULL;
int sr_NetSyncThread(void *lockVoid)
{
    SDL_mutex *lock = (SDL_mutex *)lockVoid;

    SDL_mutexP(lock);

    while ( sr_netSyncThreadGoOn )
    {
        SDL_mutexV(lock);
        // wait for network data
        bool toDo = sr_netIdler->Wait();
        SDL_mutexP(lock);

        if ( toDo )
        {
            // disable rendering (during auto-scrolling of console, for example)
            bool glout = sr_glOut;
            sr_glOut = false;

            // new network data arrived, handle it
            sr_netIdler->Do();

            // enable rendering again
            sr_glOut = glout;
        }
    }

    SDL_mutexV(lock);

    return 0;
}

static SDL_Thread * sr_netSyncThread = NULL;
static SDL_mutex * sr_netLock = NULL;
void rSysDep::StartNetSyncThread( rNetIdler * idler )
{
    sr_netIdler = idler;

    return; // BUG This thread is crashing ruby

    // can't use thrading trouble while recording
    if ( tRecorder::IsRunning() )
        return;

    if ( sr_netSyncThread )
        return;

    // create lock
    if ( !sr_netLock )
        sr_netLock = SDL_CreateMutex();

    // start thread
#if SDL_VERSION_ATLEAST(2,0,0)
    sr_netSyncThread = SDL_CreateThread( sr_NetSyncThread, "net_sync", sr_netLock );
#else
    sr_netSyncThread = SDL_CreateThread( sr_NetSyncThread, sr_netLock );
#endif
    if ( !sr_netSyncThread )
        return;

    // lock mutex, the thread should only do work while the main thread is waiting for the refresh
    SDL_mutexP( sr_netLock );
}

void rSysDep::StopNetSyncThread()
{
    // stop and delete thread
    if ( sr_netSyncThread )
    {
        SDL_mutexV(  sr_netLock );
        sr_netSyncThreadGoOn = false;
        SDL_WaitThread( sr_netSyncThread, NULL );
        sr_netSyncThread = NULL;
        sr_netIdler = NULL;
    }

    // delete lock
    if ( sr_netLock )
    {
        SDL_DestroyMutex( sr_netLock );
        sr_netLock = NULL;
    }
}

int NextPowerOfTwo( int in )
{
    int x = 1;
    while ( x * 32 <= in )
        x <<= 5;
    while ( x < in )
        x <<= 1;

    return x;
}

bool sr_MotionBlurCore( REAL alpha, rTextureRenderTarget & blurTarget )
{
    sr_CheckGLError();

    if ( alpha < 0 )
        alpha = 0;

    {
        if ( blurTarget.IsTarget() )
        {
            blurTarget.Pop();

            blurTarget.Select();

            glDrawBuffer( GL_FRONT );

            sr_CheckGLError();

            // determine the texture coordinates of the lower right corner
            REAL maxu = REAL(sr_screenWidth)/blurTarget.GetWidth();
            REAL maxv = REAL(sr_screenHeight)/blurTarget.GetHeight();

            glEnable(GL_TEXTURE_2D);

            // blend the last frame and the current frame with the specified alpha value
            glDisable( GL_DEPTH_TEST );
            glDepthMask(0);

            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();
            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
            glViewport(0,0,sr_screenWidth, sr_screenHeight);

            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,
                            GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST);

            glDisable(GL_ALPHA_TEST);
            // glDisable(GL_BLEND);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

            glDisable(GL_LIGHTING);

            glBegin( GL_QUADS );
            glColor4f( 1,1,1,alpha );

            glTexCoord2f( 0, 0 );
            glVertex2f( -1, -1 );

            glTexCoord2f( maxu, 0 );
            glVertex2f( 1, -1 );

            glTexCoord2f( maxu, maxv );
            glVertex2f( 1, 1 );

            glTexCoord2f( 0, maxv );
            glVertex2f( -1, 1 );
            glEnd();

            sr_CheckGLError();

            // clean up
            glDepthMask(1);

            glDrawBuffer( GL_BACK );
        }

        blurTarget.Push();

        sr_CheckGLError();

        return false;
    }

    return true;

#if 0
    GLenum error = glGetError();
    if ( error != GL_NO_ERROR )
        con << "GL error " << error << "\n";
#endif
}

// frames from about this far apart get blended together
static REAL sr_motionBlurTime = .0075;
static tSettingItem<REAL> c_mb( "MOTION_BLUR_TIME",
                                sr_motionBlurTime );

// blurs the motion, time is the current time
bool sr_MotionBlur( double time, std::unique_ptr< rTextureRenderTarget > & blurTarget )
{
    static bool lastActive = false;
    bool active = false;

    // measure frame rendering time
    static double lastTime = time;
    REAL frameTime = time - lastTime;

    if ( currentScreensetting.vSync == ArmageTron_VSync_MotionBlur )
    {
        // use hysteresis to autodisable motion blurring if rendering gets
        // far too slow and reenable it if rendering gets fast enough again
        static int hyster = 0;
        static int thresh = 100;
        active = lastActive;

        if ( frameTime * 2 < sr_motionBlurTime )
        {
            if ( ++hyster > thresh )
            {
                active = true;
                hyster = thresh;
            }
        }
        else if ( frameTime > sr_motionBlurTime * 2 )
        {
            if ( --hyster < -thresh )
            {
                active = false;
                hyster = -thresh;
            }
        }
        else
        {
            if ( hyster < 0 )
                ++hyster;
            else
                --hyster;
        }

        lastTime = time;
    }

    // really blur.
    if ( lastActive )
    {
        // determine blur texture size
        int blurWidth = NextPowerOfTwo( sr_screenWidth );
        int blurHeight = NextPowerOfTwo( sr_screenHeight );

        // destroy existing blur texture if it is too small
        if ( blurTarget.get() && ( blurTarget->GetWidth() < blurWidth || blurTarget->GetHeight() < blurHeight ) )
        {
            blurTarget.reset();
        }

        // create blur texture
        if ( !blurTarget.get() )
        {
            try
            {
                blurTarget.reset( tNEW( rTextureRenderTarget )( blurWidth, blurHeight ) );
            }
            catch( rExceptionGLEW const & e )
            {
                // unsupported. Disable motion blur.
                currentScreensetting.vSync = ArmageTron_VSync_Off;
                lastActive = false;

                con << tOutput("$screen_vsync_motionblur_unsupported");

                return true;
            }
        }

        // really blur
        bool ret = sr_MotionBlurCore( 1 - frameTime / sr_motionBlurTime, *blurTarget );

        // store active value for the next frame
        lastActive = active;

        // if the next frame won't be blurred, deactivate rendering to the texture
        if ( !active )
        {
            blurTarget->Pop();
        }

        return ret;
    }

    lastActive = active;

    // no motion blur happened when we got here
    if ( blurTarget.get() && blurTarget->IsTarget() )
    {
        blurTarget->Pop();
    }

    return true;
}

void rSysDep::SwapGL(){
    static std::unique_ptr< rTextureRenderTarget > blurTarget;

    if ( s_benchmark )
    {
        static PerformanceCounter counter;
        counter.Count();
    }

    double time = tSysTimeFloat();
    double realTime = tRealSysTimeFloat();

    bool next_glOut = sr_glOut;

    /* static double mytime = time; //ljr
    if (false && time < mytime + 1. / 29.97) {
        printf("skipping! %f %f\n", time, mytime);
        next_glOut = false;
    } else {
        printf("rendering %f %f\n", time, mytime);
        mytime = time;
        next_glOut = true;
    } */

    // adapt playback speed to recorded speed
    if ( !s_benchmark && !s_fastForward && tRecorder::IsPlayingBack() )
    {
        static double timeOffset=0;
        static double lastRendered=0;

        // calculate how much we're behind the rendering schedule
        double behind = - time + realTime + timeOffset;
        // std::cout << behind << " " << sr_glOut << "\n";

        // large delays can only be caused by breakpoints or map downloads; ignore them
        if ( behind > .5 || realTime > lastRendered + .2 )
        {
            timeOffset -= behind;
            next_glOut = true;
        }
        else
        {
            // we're a bit behind, skip the next frame
            if ( behind > .1 )
            {
                next_glOut = false;
            }
            else if ( sr_glOut )
            {
                lastRendered=realTime;
                // we're ahead, pause a bit
                if  ( behind < -.5 )
                    timeOffset -= behind;
                else if ( behind < -.1 )
                {
                    int delay = int( -( behind + .1 ) * 1000000 );
                    // std::cout << behind << ":" << delay << "\n";
                    tDelayForce( delay );
                }
            }
            else
            {
                // we're not behind any more. Reactivate rendering.
                next_glOut = true;
            }
        }

        if ( next_glOut )
            lastRendered=realTime;
    }

    if (!sr_glOut)
    {
        // display next frame in fast foward mode
        if ( ( s_fastForward && ( time > s_nextFastForwardFrameRecorded || realTime > s_nextFastForwardFrameReal ) ) || next_glOut )
        {
            sr_glOut = true;
            rSysDep::ClearGL();
        }

        // in playback or recording mode, always execute frame tasks, they may be improtant for consistency
        if ( tRecorder::IsRunning() ) {
            rPerFrameTask::DoPerFrameTasks();
#ifdef HAVE_LIBRUBY
            rPerFrameTaskRuby::DoPerFrameTasks();
#endif
        }


        return;
    }


    rPerFrameTask::DoPerFrameTasks();
#ifdef HAVE_LIBRUBY
    rPerFrameTaskRuby::DoPerFrameTasks();
#endif

    // unlock the mutex while waiting for the swap operation to finish
    // SDL_mutexV(  sr_netLock );
    // sr_LockSDL();

    if (sr_screenshotIsPlanned){
        make_screenshot();
        sr_screenshotIsPlanned=false;
    }
    else if (s_videoout)
        make_screenshot();

    // actiate motion blur (does not use the game state, so it's OK to call here )
    bool shouldSwap = sr_MotionBlur( time, blurTarget );
    sr_SwapTime().Finish( shouldSwap );

    // sr_UnlockSDL();
    // lock mutex again
    // SDL_mutexP(  sr_netLock );


    // disable output in fast forward mode
    if ( s_fastForward && tRecorder::IsPlayingBack() )
    {
        if ( time < s_fastForwardTo )
        {
            // next displayed frame should be ten percent closer to the target, but at most 10 seconds
            s_nextFastForwardFrameRecorded = ( s_fastForwardTo - time ) * sr_FF_MaxstepRel;
            if ( s_nextFastForwardFrameRecorded > sr_FF_Maxstep )
                s_nextFastForwardFrameRecorded = sr_FF_Maxstep ;
            s_nextFastForwardFrameRecorded += time;
            s_nextFastForwardFrameReal = realTime + sr_FF_MaxstepReal ;

            next_glOut = false;
        }
        else
        {
            std::cout << "End of fast forward mode.\n";
            st_Breakpoint();
            s_fastForward = false;
        }
    }

    //#ifdef DEBUG
    if ( !s_fastForward )
    {
        breakpoint();
    }
    //#endif

    // store frame time for next frame
    // lastFrame = tRealSysTimeFloat();

    sr_glOut = next_glOut;
}

#endif // dedicated

#ifndef DEDICATED
static SDL_mutex *mut;

static void stuff_init(){
    mut=SDL_CreateMutex();
}

static tInitExit stuff_ie(&stuff_init);
#endif

void sr_LockSDL(){
    //std::cerr << "locking...";
#ifndef DEDICATED
#ifndef WIN32
    //SDL_mutexP(mut);
#endif
#endif
    //std::cerr << " locked!\n";
}

void sr_UnlockSDL(){
    //std::cerr << "unlocking...";
#ifndef DEDICATED
#ifndef WIN32
    //SDL_mutexV(mut);
#endif
#endif
    //std::cerr << " unlocked!\n";
}

#ifndef DEDICATED
void  rSysDep::ClearGL(){
    if (sr_glOut && sr_needClear )
    {
        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    sr_needClear = true;
}

bool rSysDep::IsBenchmark()
{
    return s_benchmark;
}

 //!< call while game content is rendered
void rSysDep::IsInGame()
{
    sr_SwapTime().IsInGame();
}
#endif


