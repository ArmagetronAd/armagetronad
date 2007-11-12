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

#ifndef DEDICATED
#include "SDL_thread.h"
#include "SDL_mutex.h"

#include <png.h>
#include <unistd.h>
#define SCREENSHOT_PNG_BITDEPTH 8
#define SCREENSHOT_BYTES_PER_PIXEL 3
#ifndef SDL_OPENGL
#ifndef DIRTY
#define DIRTY
#endif
#endif

//#ifndef SDL_OPENGL
//#error "need SDL 1.1"
//#endif

#ifndef DIRTY

// nothing to be done.

/*
//#elif defined(HAVE_FXMESA)
 #include <GL/gl>
 #include <GL/fxmesa>

 static fxMesaContext ctx=NULL;
*/

#elif defined(WIN32)

 #include <windows.h>
 #include <windef.h>
 #include "rGL.h"
static HDC hDC=NULL;
static HGLRC hRC=NULL;

#elif defined(unix) || defined(__unix__)

#include <GL/glx.h>
static GLXContext cx;
Display *dpy=NULL;
Window  win;

#endif

#ifdef DIRTY
#include <SDL_syswm.h>

// graphics initialisation and cleanup:
bool  rSysDep::InitGL(){
    SDL_SysWMinfo system;
    SDL_VERSION(&system.version);
    if (!SDL_GetWMInfo(&system)){
        std::cerr << "Video information not available!\n";
        return(false);
    }

    /*
    con << "SDL version: " << (int)system.version.major
         << "." <<  (int)system.version.minor << "." <<  (int)system.version.patch << '\n';
    */

    /*
    //#ifdef HAVE_FXMESA
    if(!ctx){
      int x=fxQueryHardware();
      if(x){
        std::cerr << "No 3Dfx hardware available.\n" << x << '\n';
        return(false);
      }

      GLint attribs[]={FXMESA_DOUBLEBUFFER,FXMESA_DEPTH_SIZE,16,FXMESA_NONE};
      ctx=fxMesaCreateBestContext(0,sr_screenWidth,sr_screenHeight,attribs);

      if (!ctx){
        std::cerr << "Could not create FX rendering context!\n";
        return(false);
      }

      fxMesaMakeCurrent(ctx);
    }
    */
#ifdef WIN32
    // windows GL initialisation stolen from
    // http://www.geocities.com/SiliconValley/Code/1219/opengl32.html

    if (!hRC){
        HWND hWnd=system.window;

        PIXELFORMATDESCRIPTOR pfd;
        int iFormat;

        // get the device context (DC)
        hDC = GetDC( hWnd );
        if (!hDC) return false;

        // set the pixel format for the DC
        ZeroMemory( &pfd, sizeof( pfd ) );
        pfd.nSize = sizeof( pfd );
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                      PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = currentScreensetting.colorDepth ? 24 : 16;
        pfd.cDepthBits = 16;
        pfd.iLayerType = PFD_MAIN_PLANE;
        iFormat = ChoosePixelFormat( hDC, &pfd );
        SetPixelFormat( hDC, iFormat, &pfd );

        // create and enable the render context (RC)
        hRC = wglCreateContext( hDC );
        if (!hRC || !wglMakeCurrent( hDC, hRC ))
            return false;
    }

#elif defined(unix) || defined(__unix__)
    if (system.subsystem!=SDL_SYSWM_X11){
        std::cerr << "System is not X11!\n";
        std::cerr << (int)system.subsystem << "!=" << (int)SDL_SYSWM_X11 <<'\n';
        return false;
    }

    if(!dpy){

        dpy=system.info.x11.display;
        win=system.info.x11.window;

        int errorbase,tEventbase;
        if (glXQueryExtension(dpy,&errorbase,&tEventbase) == False){
            std::cerr << "OpenGL through GLX not supported.\n";
            return false;
        }

        int configuration[]={GLX_DOUBLEBUFFER,GLX_RGBA,GLX_DEPTH_SIZE ,12, GLX_RED_SIZE,1,
                             GLX_BLUE_SIZE,1,GLX_GREEN_SIZE,1,None};

        XVisualInfo *vi=glXChooseVisual(dpy,DefaultScreen(dpy),configuration);

        if(vi== NULL){
            std::cerr << "Could not initialize Visual.\n";
            return false;
        }

        cx=glXCreateContext(dpy,vi,
                            NULL,True);

        if(cx== NULL){
            std::cerr << "Could not initialize GL context.\n";
            return false;
        }

        if (!glXMakeCurrent(dpy,win,cx)){
            dpy=0;
            return false;
        }
    }

#endif

    return true;
}

void  rSysDep::ExitGL(){
    SDL_SysWMinfo system;
    SDL_GetWMInfo(&system);

    /*
    #ifdef HAVE_FXMESA

    if(ctx){
      fxMesaDestroyContext(ctx);
      ctx=NULL;
      fxCloseHardware();
    }
    */

#if defined(WIN32)
    HWND hWnd=system.window;

    // windows GL cleanup stolen from
    // http://www.geocities.com/SiliconValley/Code/1219/opengl32.html
    if(hRC){

        wglMakeCurrent( NULL, NULL );
        wglDeleteContext( hRC );
        ReleaseDC( hWnd, hDC );

        hRC=NULL;
        hDC=NULL;
    }
#elif defined(unix) || defined(__unix__)
    if(dpy){

        //    glXReleaseBuffersMESA( dpy, win );
        glXMakeCurrent(dpy,None,NULL);
        glXDestroyContext(dpy, cx );
        dpy=NULL;
    }
#endif
}
#endif // DIRTY

bool sr_screenshotIsPlanned=false;
static bool   s_videoout    =false;
static int    s_videooutDest=fileno(stdout);

static bool png_screenshot=true;
static tConfItem<bool> pns("PNG_SCREENSHOT",png_screenshot);
#ifndef DEDICATED

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
    if(!(row_ptrs = (png_byte**) malloc(sr_screenHeight * sizeof(png_byte*)))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return;
    }

    for(i = 0; i < sr_screenHeight; i++) {
        row_ptrs[i] = (png_byte *)image->pixels + (sr_screenHeight - i - 1)
                      * SCREENSHOT_BYTES_PER_PIXEL * sr_screenWidth;
    }

    png_write_image(png_ptr, row_ptrs);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    free(row_ptrs);
    fclose(fp);
}
#endif

static void make_screenshot(){
#ifndef DEDICATED
    // screenshot count
    static int number=0;
    number++;

    SDL_Surface *image;
    SDL_Surface *temp;
    int idx;
    image = SDL_CreateRGBSurface(SDL_SWSURFACE, sr_screenWidth, sr_screenHeight,
                                 24, 0x0000FF, 0x00FF00, 0xFF0000 ,0);
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, sr_screenWidth, sr_screenHeight,
                                24, 0x0000FF, 0x00FF00, 0xFF0000, 0);

    // make upside down screenshot
    glReadPixels(0,0,sr_screenWidth, sr_screenHeight, GL_RGB,
                 GL_UNSIGNED_BYTE, image->pixels);

    // turn image around
    for (idx = 0; idx < sr_screenHeight; idx++)
    {
        memcpy(reinterpret_cast<char *>(temp->pixels) + 3 * sr_screenWidth * idx,
               reinterpret_cast<char *>(image->pixels)+ 3
               * sr_screenWidth*(sr_screenHeight - idx-1),
               3*sr_screenWidth);
    }

    if (s_videoout)
        write(s_videooutDest, temp->pixels, sr_screenWidth * sr_screenHeight * 3);

    if (sr_screenshotIsPlanned) {
        // save screenshot in unused slot
        bool done = false;
        while ( !done )
        {
            // generate filename
            tString fileName("screenshot_");
            fileName << number;
            if(png_screenshot)
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
            if(png_screenshot)
                SDL_SavePNG(image, tDirectories::Screenshot().GetWritePath( fileName ));
            else
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
    PerformanceCounter(): count_(0){ tRealSysTimeFloat(); }
    unsigned int Count(){ return count_++; }
    ~PerformanceCounter()
    {
        double time = tRealSysTimeFloat();
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

/*
static double lastFrame = -1;
static void sr_DelayFrame( int targetFPS )
{
    // calculate microseconds per frame
    int uSecsPerFrame = MILLION/(targetFPS + 10);

    // calculate microseconds spent rendering
    double thisFrame = tRealSysTimeFloat();

    int uSecsPassed = static_cast<int>( MILLION * ( thisFrame - lastFrame ) );

//    con << uSecsPassed << "\n";

    // wait
    int uSecsToWait = uSecsPerFrame - uSecsPassed;
    if ( uSecsToWait > 0 )
        tDelay( uSecsToWait );

    // call glFinish to wait for GPU
    glFinish();
}
*/

rSysDep::rSwapMode rSysDep::swapMode_ = rSysDep::rSwap_glFlush;
//rSysDep::rSwapMode rSysDep::swapMode_ = rSysDep::rSwap_60Hz;

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
    sr_netSyncThread = SDL_CreateThread( sr_NetSyncThread, sr_netLock );
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

    // determine best texture dimensions, fitting the screen resolution
    int tWidth = 128;
    while ( tWidth < sr_screenWidth )
        tWidth *= 2;

    int tHeight = 128;
    while ( tHeight < sr_screenHeight )
        tHeight *= 2;

    // Read the current front buffer into a texture
    GLenum target;
    glGenTextures(1, &target );
    glBindTexture(GL_TEXTURE_2D,target);

#if 0
    error = glGetError();
    if ( error != GL_NO_ERROR )
        con << "GL error2 " << error << "\n";
#endif

    glReadBuffer( GL_FRONT );

    glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
                      0, 0,
                      tWidth, tHeight, 0 );

#if 0
    error = glGetError();
    if ( error != GL_NO_ERROR )
        con << "GL error3 " << error << "\n";
#endif

    // determine the texture coordinates of the lower right corner
    REAL maxu = REAL(sr_screenWidth)/tWidth;
    REAL maxv = REAL(sr_screenHeight)/tHeight;

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

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

    // clean up: destroy the texture
    glDeleteTextures(1, &target );
    glDepthMask(1);
}

// frames from about this far apart get blended together
static REAL sr_motionBlurTime = .0075;
static tSettingItem<REAL> c_mb( "MOTION_BLUR_TIME",
                                sr_motionBlurTime );

// blurs the motion, time is the current time
bool sr_MotionBlur( double time, std::auto_ptr< rTextureRenderTarget > & blurTarget )
{
    static bool lastActive = true;
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
            blurTarget = std::auto_ptr< rTextureRenderTarget >();
        }
        
        // create blur texture
        if ( !blurTarget.get() )
        {
            blurTarget = std::auto_ptr< rTextureRenderTarget >( new rTextureRenderTarget( blurWidth, blurHeight  ) );
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
	static std::auto_ptr< rTextureRenderTarget > blurTarget(0);

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
        if ( s_fastForward && ( time > s_nextFastForwardFrameRecorded || realTime > s_nextFastForwardFrameReal ) || next_glOut )
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
    SDL_mutexV(  sr_netLock );
    sr_LockSDL();

    // actiate motion blur (does not use the game state, so it's OK to call here )
    bool shouldSwap = sr_MotionBlur( time, blurTarget );

    switch( swapMode_ )
    {
    case rSwap_Fastest:
        break;
    case rSwap_glFlush:
        glFlush();
        break;
    case rSwap_glFinish:
        glFinish();
        break;
    }

	if ( shouldSwap )
	{
#if defined(SDL_OPENGL)
		if (lastSuccess.useSDL)
			SDL_GL_SwapBuffers();
		//#elif defined(HAVE_FXMESA)
		//fxMesaSwapBuffers();
#endif

#ifdef DIRTY
		if (!lastSuccess.useSDL){
#if defined(WIN32)
			SwapBuffers( hDC );
#elif defined(unix) || defined(__unix__)
			glXSwapBuffers(dpy,win);
#endif
		}
#endif
	}

    if (sr_screenshotIsPlanned){
        make_screenshot();
        sr_screenshotIsPlanned=false;
    }
    else if (s_videoout)
        make_screenshot();

    sr_UnlockSDL();
    // lock mutex again
    SDL_mutexP(  sr_netLock );


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
    if (sr_glOut){

        /*
        if (sr_screenshotIsPlanned){
          make_screenshot();
          sr_screenshotIsPlanned=false;
        }
        */

        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
#endif


