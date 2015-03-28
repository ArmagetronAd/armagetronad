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

    #include "rTexture.h"

    #include "defs.h"

    #include <string>
    #include "rTexture.h"
    #include "rScreen.h"
    #include "rSysdep.h"
    #include "rConsole.h"
    #include "rViewport.h"
    #include "tConfiguration.h"
    #include "tRecorder.h"
    #include "tSysTime.h"

    #ifndef DEDICATED
// #include "../network/nNetwork.h"
    #include "rGL.h"
    #include "rSDL.h"

    #ifdef POWERPAK_DEB
    #include <PowerPak/powerdraw>
    #endif
    #endif

    #ifdef DEBUG
//#ifdef WIN32
    #define FORCE_WINDOW
//#endif
    #endif

tCONFIG_ENUM( rResolution );
tCONFIG_ENUM( rColorDepth );
tCONFIG_ENUM( rVSync );

#if SDL_VERSION_ATLEAST(2,0,0)
SDL_Window   *sr_screen=NULL;
SDL_Renderer *sr_screenRenderer=NULL;
#else
SDL_Surface  *sr_screen=NULL; // our window
#endif

    #ifndef DEDICATED
#ifndef SDL_OPENGL
#error "need SDL 1.1"
#endif

static int default_texturemode = GL_LINEAR_MIPMAP_LINEAR;
    #endif

rDisplayListUsage sr_useDisplayLists=rDisplayList_Off;
bool              sr_blacklistDisplayLists=false;

static int width[ArmageTron_Custom+2]  = {0, 320, 320, 400, 512, 640, 800, 1024	, 1280, 1280, 1280, 1600, 1680, 2048,800,320};
static int height[ArmageTron_Custom+2] = {0, 200, 240, 300, 384, 480, 600,  768	,  800,  854, 1024, 1200, 1050, 1572,600,200};
static REAL aspect[ArmageTron_Custom+2]= {1, 1	, 1  , 1  , 1  , 1  , 1	 , 1	,    1,    1, 1   ,    1,    1,    1,1,  1};

int sr_screenWidth,sr_screenHeight;

static tSettingItem<int>  at_ch("CUSTOM_SCREEN_HEIGHT"	, height[ArmageTron_Custom]);
static tSettingItem<int>  at_cw("CUSTOM_SCREEN_WIDTH" 	, width	[ArmageTron_Custom]);
static tSettingItem<REAL> at_ca("CUSTOM_SCREEN_ASPECT" , aspect[ArmageTron_Custom]);

    #define MAXEMERGENCY 5

rScreenSettings lastSuccess(ArmageTron_640_480, false);

/*
std::ostream & operator << ( std::ostream & s, rScreenSize const & size )
{
    return s;
}

std::istream & operator >> ( std::istream & s, rScreenSize const & size )
{
    return s;
}
*/

static rScreenSettings em5(ArmageTron_320_240, false, ArmageTron_ColorDepth_16, false);
static rScreenSettings em4(ArmageTron_320_240, false, ArmageTron_ColorDepth_Desktop, false);
static rScreenSettings em3(ArmageTron_640_480, false,ArmageTron_ColorDepth_16);
static rScreenSettings em2(ArmageTron_640_480, true, ArmageTron_ColorDepth_16);
static rScreenSettings em1(ArmageTron_640_480);

static rScreenSettings *emergency[MAXEMERGENCY+2]={ &lastSuccess, &lastSuccess, &em1, &em2 , &em3, &em4, &em5};

    #ifdef DEBUG
rScreenSettings currentScreensetting(ArmageTron_640_480);
    #else
rScreenSettings currentScreensetting(sr_DesktopScreensizeSupported() ? ArmageTron_Desktop : ArmageTron_800_600, true);
    #endif

bool sr_DesktopScreensizeSupported()
{
#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
#else
    SDL_version const & sdlVersion = *SDL_Linked_Version();
#endif

    return
    sdlVersion.major > 1 ||
    ( sdlVersion.major == 1 &&
      ( sdlVersion.minor > 2 ||
        ( sdlVersion.minor == 2 &&
          ( sdlVersion.patch >= 10 ) ) ) );
#else
    return false;
#endif
}

static int failed_attempts = 0;

#if SDL_VERSION_ATLEAST(2, 0, 0)
static tConfItem<int>  at_di("ARMAGETRON_DISPLAY_INDEX"	, currentScreensetting.displayIndex);
static tConfItem<int>  at_ldi("ARMAGETRON_LAST_DISPLAY_INDEX"	, lastSuccess.displayIndex);

static tConfItem<int>  at_rr("ARMAGETRON_REFRESH_RATE"	, currentScreensetting.refreshRate);
static tConfItem<int>  at_lrr("ARMAGETRON_LAST_REFRESH_RATE"	, lastSuccess.refreshRate);
#endif

static tConfItem<rResolution> screenres("ARMAGETRON_SCREENMODE",currentScreensetting.res.res);
static tConfItem<rResolution> screenresLast("ARMAGETRON_LAST_SCREENMODE",lastSuccess.res.res);

static tConfItem<rResolution> winsize("ARMAGETRON_WINDOWSIZE",currentScreensetting.windowSize.res);
static tConfItem<rResolution> winsizeLast("ARMAGETRON_LAST_WINDOWSIZE",lastSuccess.windowSize.res);

static tConfItem<rVSync> vSync("ARMAGETRON_VSYNC",currentScreensetting.vSync);
static tConfItem<rVSync> vSyncLast("ARMAGETRON_VSYNC_LAST",lastSuccess.vSync);

static tConfItem<int> screenres_w("ARMAGETRON_SCREENMODE_W",currentScreensetting.res.width);
static tConfItem<int> screenresLast_w("ARMAGETRON_LAST_SCREENMODE_W", lastSuccess.res.width);

static tConfItem<int> winsize_w("ARMAGETRON_WINDOWSIZE_W",currentScreensetting.windowSize.width);
static tConfItem<int> winsizeLast_w("ARMAGETRON_LAST_WINDOWSIZE_W",lastSuccess.windowSize.width);

static tConfItem<int> screenres_h("ARMAGETRON_SCREENMODE_H",currentScreensetting.res.height);
static tConfItem<int> screenresLast_h("ARMAGETRON_LAST_SCREENMODE_H", lastSuccess.res.height);

// static tConfItem<rScreenSize> winsize_wh("ARMAGETRON_WINDOWSIZE_WH",currentScreensetting.windowSize);

static tConfItem<int> winsize_h("ARMAGETRON_WINDOWSIZE_H",currentScreensetting.windowSize.height);
static tConfItem<int> winsizeLast_h("ARMAGETRON_LAST_WINDOWSIZE_H",lastSuccess.windowSize.height);

static tConfItem<bool> fs_ci("FULLSCREEN",currentScreensetting.fullscreen);
static tConfItem<bool> fs_lci("LAST_FULLSCREEN",currentScreensetting.fullscreen);

static tConfItem<rColorDepth> tc("COLORDEPTH",currentScreensetting.colorDepth);
static tConfItem<rColorDepth> ltc("LAST_COLORDEPTH",lastSuccess.colorDepth);
static tConfItem<rColorDepth> tzd("ZDEPTH",currentScreensetting.zDepth);
static tConfItem<rColorDepth> ltzd("LAST_ZDEPTH",lastSuccess.zDepth);

#if !SDL_VERSION_ATLEAST(2,0,0)
static tConfItem<bool> check_errors("CHECK_ERRORS",currentScreensetting.checkErrors);
static tConfItem<bool> check_errorsl("LAST_CHECK_ERRORS",lastSuccess.checkErrors);
#endif

static tConfItem<int> fa("FAILED_ATTEMPTS", failed_attempts);

// *******************************************

static tCallback *rPerFrameTask_anchor;

    #ifdef HAVE_LIBRUBY
static tCallbackRuby * rPerFrameTaskRuby_anchor;
    #endif

bool sr_True(){return true;}

rPerFrameTask::rPerFrameTask(AA_VOIDFUNC *f):tCallback(rPerFrameTask_anchor, f){}
void rPerFrameTask::DoPerFrameTasks(){
    // prevent console rendering, that can cause nasty recursions
    rNoAutoDisplayAtNewlineCallback noAutoDisplay( sr_True );
    Exec(rPerFrameTask_anchor);
}

    #ifdef HAVE_LIBRUBY
rPerFrameTaskRuby::rPerFrameTaskRuby()
        :tCallbackRuby(rPerFrameTaskRuby_anchor)
{
}

void rPerFrameTaskRuby::DoPerFrameTasks(){
    rNoAutoDisplayAtNewlineCallback noAutoDisplay( sr_True );
    Exec(rPerFrameTaskRuby_anchor);
}
    #endif



// *******************************************

static tCallbackString *RenderId_anchor;

rRenderIdCallback::rRenderIdCallback(STRINGRETFUNC *f)
        :tCallbackString(RenderId_anchor, f){}
tString rRenderIdCallback::RenderId(){return Exec(RenderId_anchor);}

// *******************************************

// *******************************************************************************************
// *
// *   rScreenSize
// *
// *******************************************************************************************
//!
//!        @param  w   screen width
//!        @param  h  screen height
//!
// *******************************************************************************************

rScreenSize::rScreenSize( int w, int h )
        :res( ArmageTron_Invalid ), width(w), height(h)
{
}

// *******************************************************************************************
// *
// *   rScreenSize
// *
// *******************************************************************************************
//!
//!        @param  r
//!
// *******************************************************************************************

rScreenSize::rScreenSize( rResolution r )
        :res( r ), width(0), height(0)
{
    UpdateSize();
}

// *******************************************************************************************
// *
// *   UpdateSize
// *
// *******************************************************************************************
//!
//!
// *******************************************************************************************

void rScreenSize::UpdateSize( void )
{
    if ( res != ArmageTron_Invalid )
    {
        width = ::width[res];
        height = ::height[res];
        // res = ArmageTron_Invalid;
    }
}

// *******************************************************************************************
// *
// *   operator ==
// *
// *******************************************************************************************
//!
//!        @param  other   size to compare with
//!        @return true iff equal
//!
// *******************************************************************************************

bool rScreenSize::operator ==( rScreenSize const & other ) const
{
    return Compare( other ) == 0;
}

// *******************************************************************************************
// *
// *   operator !=
// *
// *******************************************************************************************
//!
//!        @param  other   size to compare with
//!        @return  true iff not equal
//!
// *******************************************************************************************

bool rScreenSize::operator !=( rScreenSize const & other ) const
{
    return Compare( other ) != 0;
}

// *******************************************************************************************
// *
// *   Compare
// *
// *******************************************************************************************
//!
//!        @param  other   size to compare with
//!        @return         0 if eqal, -1 if this is smaller, +1 if other is smaller
//!
// *******************************************************************************************

int rScreenSize::Compare( rScreenSize const & other ) const
{
    // desktop size dominates all
    if ( width == 0 && other.width != 0 )
        return 1;
    if ( other.width == 0 && width != 0 )
        return -1;

    if ( width < other.width )
        return -1;
    else if ( width > other.width )
        return 1;

    if ( height < other.height )
        return -1;
    else if ( height > other.height )
        return 1;

    /* res is not really a criterion, ignore it
    if ( res < other.res )
        return -1;
    else if ( res > other.res )
        return 1;
    */

    return 0;
}


// *******************************************************************************************
// *
// *   rScreenSettings
// *
// *******************************************************************************************
//!
//!        @param  r   the resolution
//!        @param  fs  fullscreen flag
//!        @param  cd  color depth
//!        @param  sdl use clean sdl initialization
//!        @param  ce  check for errors
//!
// *******************************************************************************************

rScreenSettings::rScreenSettings( rResolution r, bool fs, rColorDepth cd, bool ce )
:res(r), windowSize(r), fullscreen(fs), colorDepth(cd), zDepth( ArmageTron_ColorDepth_Desktop ), checkErrors(true), displayIndex(0), refreshRate(0), vSync( ArmageTron_VSync_Default ), aspect (1)
{
    // special case for desktop resolution: window size of 640x480
    if ( r == ArmageTron_Desktop )
    {
        windowSize = rScreenSize( ArmageTron_640_480 );
    }
}

void sr_ReinitDisplay(){
#if !SDL_VERSION_ATLEAST(2, 0, 0)
    sr_ExitDisplay();
#endif
    if (!sr_InitDisplay()){
        tERR_ERROR("Oops. Failed to reinit video hardware. "
                   "Resetting to defaults..\n");
        exit(-1);
    }
}


// *******************************************



// GL information

tString gl_vendor;
tString gl_renderer;
tString gl_version;
tString gl_extensions;

bool software_renderer=false;
bool last_software_renderer=false;

static tConfItem<bool> lsr("SOFTWARE_RENDERER",last_software_renderer);

tString lastError("Unknown");

#if SDL_VERSION_ATLEAST(2,0,0)
#else
#ifndef DEDICATED
static int countBits(unsigned int count)
{
    int ret = 0;
    while (count)
    {
        ret    += count & 1;
        count >>= 1;
    }

    return ret;
}
#endif
#endif

    #ifndef DEDICATED
    #ifdef SDL_OPENGL
// sets the number of vsync signals to wait for each frame
static void sr_SetSwapControl( int frames, bool after = false )
{
    bool success = false;

    #ifdef LINUX
    // set environment variable for the linux nvidia driver.
    // darn, changes to this can't be made while the program is running,
    // a restart is required.
    char hack[2];
    hack[0] = '0' + frames;
    hack[1] = 0;
    setenv( "__GL_SYNC_TO_VBLANK", hack, 1 );
    #endif

    #ifdef WIN32
    // special Windows code
    typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );
    PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;

    {
        const char *extensions = gl_extensions;

        if( extensions && strstr( extensions, "WGL_EXT_swap_control" ) )
        {
            wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress( "wglSwapIntervalEXT" );

            if( wglSwapIntervalEXT )
            {
                success = true;
                if ( after )
                    wglSwapIntervalEXT( frames );
            }
        }
    }
    #endif

    // use SDL, requires 1.2.10
    #if SDL_VERSION_ATLEAST(1, 2, 10)
    if ( !success )
    #if SDL_VERSION_ATLEAST(2, 0, 0)
        SDL_GL_SetSwapInterval( frames );
    #else
        SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, frames );
    #endif
    #endif
}

static void sr_SetSwapControlAuto( bool after = false )
{
    // requires SDL 1.2.10
    if ( tRecorder::IsRecording() )
    {
        // recordings are always done with VSync enabled
#ifndef DEBUG
        sr_SetSwapControl( 1, after );
#endif
    }
    else if( rSysDep::IsBenchmark() )
    {
#ifndef DEBUG
        sr_SetSwapControl( 0, after );
#endif
    }
    else
    {
        switch (currentScreensetting.vSync)
        {
        case ArmageTron_VSync_On:
            sr_SetSwapControl( 1, after );
            break;
        case ArmageTron_VSync_Off:
        case ArmageTron_VSync_MotionBlur:
            sr_SetSwapControl( 0, after );
            break;
        case ArmageTron_VSync_Default:
            break;
        }
    }
}

static void sr_SetGLAttributes( int rDepth, int gDepth, int bDepth, int zDepth )
{
    // SDL 1.1 required
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, rDepth );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, gDepth );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, bDepth );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, zDepth );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    sr_SetSwapControlAuto();
}

// to be called after screen initialization
static void sr_CompleteGLAttributes()
{
    sr_SetSwapControlAuto( true );
}
    #endif // SDL_OPENGL
    #endif // DEDICATED

// flag indicating whether directX is supposed to be used for input (defaults to false, crashes on my Win7)
// bool sr_useDirectX = false;
// static bool use_directx_back = false;

#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2,0,0)
int SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags) {
    int i, actual_bpp = 0;

    if (!(flags & SDL_WINDOW_FULLSCREEN)) {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(0, &mode);
        return SDL_BITSPERPIXEL(mode.format);
    }

    for (i = 0; i < SDL_GetNumDisplayModes(0); ++i) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(0, i, &mode);
        if (!mode.w || !mode.h || (width == mode.w && height == mode.h)) {
            if (!mode.format) {
                return bpp;
            }
            if (SDL_BITSPERPIXEL(mode.format) >= (Uint32) bpp) {
                actual_bpp = SDL_BITSPERPIXEL(mode.format);
            }
        }
    }
    return actual_bpp;
}

bool IsWindowActive(void) {
    Uint32 flags = 0;

    flags = SDL_GetWindowFlags(sr_screen);
    if ((flags & SDL_WINDOW_SHOWN) && !(flags & SDL_WINDOW_MINIMIZED)) {
        return true;
    }
    return false;
}

int SDL_EnableUNICODE(int enable) {
    static int SDL_enabled_UNICODE=0;
    int previous = SDL_enabled_UNICODE;

    switch (enable) {
    case 1:
        SDL_enabled_UNICODE = 1;
        SDL_StartTextInput();
        break;
    case 0:
        SDL_enabled_UNICODE = 0;
        SDL_StopTextInput();
        break;
    }
    return previous;
}
#endif
#endif


#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2,0,0)
static bool lowlevel_sr_InitDisplay(){
    rScreenSize & res = currentScreensetting.fullscreen ? currentScreensetting.res : currentScreensetting.windowSize;

    // update pixel aspect ratio
    if ( res.res != ArmageTron_Invalid && size_t(res.res) < sizeof(aspect)/sizeof(aspect[0]) )
        currentScreensetting.aspect = aspect[res.res];

    res.UpdateSize();
    sr_screenWidth = res.width;
    sr_screenHeight= res.height;

    // desktop color depth
    static int desktopCD_R = 5;
    static int desktopCD_G = 5;
    static int desktopCD_B = 5;
    // static int desktopCD   = 16;
    // desktop resolution
    static int sr_desktopWidth = 0, sr_desktopHeight = 0;

    int minWidth = 640;
    int minHeight = 480;

    // last diplay index in use
    static int sr_lastDisplayIndex = -1;

    // last window position
    static int lastWindowX = SDL_WINDOWPOS_CENTERED, lastWindowY = SDL_WINDOWPOS_CENTERED;

    static int lastFactualDisplayIndex = currentScreensetting.displayIndex;
    if( sr_screen )
    {
        // fetch actual display index in case user dragged window
        int factualDisplayIndex = SDL_GetWindowDisplayIndex(sr_screen);
        if(factualDisplayIndex != lastFactualDisplayIndex)
        {
            currentScreensetting.displayIndex = factualDisplayIndex;
            lastFactualDisplayIndex = factualDisplayIndex;
        }

        // last window position
        if(!lastSuccess.fullscreen)
        {
            SDL_GetWindowPosition(sr_screen, &lastWindowX, &lastWindowY);
        }
    }

    if(0 > currentScreensetting.displayIndex || currentScreensetting.displayIndex >= SDL_GetNumVideoDisplays())
        currentScreensetting.displayIndex = 0;
    lastFactualDisplayIndex = currentScreensetting.displayIndex;

    if ( sr_lastDisplayIndex != currentScreensetting.displayIndex )
    {
        // determine desktop mode

        // select sane defaults in case the following operation fails
        sr_desktopWidth = minWidth;
        sr_desktopHeight = minHeight;

        SDL_DisplayMode mode;
        if (!SDL_GetDesktopDisplayMode(currentScreensetting.displayIndex, &mode)) {
            sr_desktopWidth  = mode.w;
            sr_desktopHeight = mode.h;

            int bpp;
            Uint32 Rmask, Gmask, Bmask, Amask;

            if (!SDL_PixelFormatEnumToMasks(mode.format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
                // desktopCD    = bpp;
                desktopCD_R  = Rmask;
                desktopCD_G  = Gmask;
                desktopCD_B  = Bmask;
            }
        }
    }

    // determine layout of current screen
    SDL_Rect screenBounds;
    SDL_GetDisplayBounds(currentScreensetting.displayIndex, &screenBounds);

    // default start window size and position
    int defaultWidth = sr_desktopWidth;
    int defaultHeight = sr_desktopHeight;
    if(!currentScreensetting.fullscreen)
    {
        defaultWidth = sr_screenWidth;
        defaultHeight = sr_screenHeight;
    }

    int defaultX = screenBounds.x + (screenBounds.w-defaultWidth)/2;
    int defaultY = screenBounds.y + (screenBounds.h-defaultHeight)/2;

    if(!currentScreensetting.fullscreen && (
           lastWindowX != SDL_WINDOWPOS_CENTERED || lastWindowY != SDL_WINDOWPOS_CENTERED))
    {
        defaultX = lastWindowX;
        defaultY = lastWindowY;
    }

    if (!sr_screen)
    {
        int singleCD_R	= 5;
        int singleCD_G	= 5;
        int singleCD_B	= 5;
        // int fullCD		= 16;
        int zDepth		= 16;

        switch (currentScreensetting.colorDepth)
        {
        case ArmageTron_ColorDepth_16:
            // parameters already set for this depth
            break;
        case ArmageTron_ColorDepth_Desktop:
            {
                // fullCD     = desktopCD;
                singleCD_R = desktopCD_R;
                singleCD_G = desktopCD_G;
                singleCD_B = desktopCD_B;
            }
            break;
        case ArmageTron_ColorDepth_32:
            singleCD_R	= 8;
            singleCD_G	= 8;
            singleCD_B	= 8;
            // fullCD		= 24;
            zDepth		= 32;
            break;
        }

        switch ( currentScreensetting.zDepth )
        {
        case ArmageTron_ColorDepth_16: zDepth = 16; break;
        case ArmageTron_ColorDepth_32: zDepth = 32; break;
        default: break;
        }

        sr_SetGLAttributes( singleCD_R, singleCD_G, singleCD_B, zDepth );

        int attrib=SDL_WINDOW_OPENGL;
        SDL_SetRelativeMouseMode(SDL_FALSE);

    #ifdef FORCE_WINDOW
    #ifdef WIN32
        //		sr_screenWidth  = 400;
        //		sr_screenHeight = 300;
    #else
        //		sr_screenWidth  = minWidth;
        //		sr_screenHeight = minHeight;
    #endif
    #endif
        // int CD = fullCD;

#ifdef FORCE_WINDOW_X
        #undef SDL_WINDOW_FULLSCREEN_DESKTOP
        #define SDL_WINDOW_FULLSCREEN_DESKTOP 0
        #undef SDL_WINDOW_FULLSCREEN
        #define SDL_WINDOW_FULLSCREEN 0
#endif

        // try fullscreen first if requested and sensible (only display 0 is supported)
        if (currentScreensetting.fullscreen && currentScreensetting.displayIndex == 0 && SDL_CreateWindowAndRenderer(sr_desktopWidth, sr_desktopHeight, attrib | SDL_WINDOW_FULLSCREEN_DESKTOP, &sr_screen, &sr_screenRenderer)) 
        {
            sr_screen = NULL;
            sr_screenRenderer = NULL;
        }

        // only reinit the screen if the desktop res detection hasn't left us
        // with a perfectly good one.
        if (!sr_screen &&
            (
                !(sr_screen = SDL_CreateWindow("", defaultX, defaultY, defaultWidth, defaultHeight, attrib)) ||
                !(sr_screenRenderer = SDL_CreateRenderer(sr_screen, -1, 0))
                )
            )
        {
            lastError.Clear();
            lastError << "Couldn't set video mode: ";
            lastError << SDL_GetError();
            std::cerr << lastError << '\n';
            return false;
        }

        sr_SetWindowTitle();

        sr_CompleteGLAttributes();

        SDL_EnableUNICODE(1);
    }

    if(!sr_screen)
        return false;

    if (SDL_GetWindowDisplayIndex(sr_screen) != currentScreensetting.displayIndex ||
        (sr_lastDisplayIndex >= 0 && sr_lastDisplayIndex != currentScreensetting.displayIndex) ||
        (currentScreensetting.fullscreen && !lastSuccess.fullscreen)
        )
    {
        // go to window mode, position window on center of selected display
        SDL_SetWindowFullscreen(sr_screen, 0);
        SDL_SetWindowSize(sr_screen, defaultWidth, defaultHeight);
        SDL_SetWindowPosition(sr_screen, defaultX, defaultY);

        SDL_Delay(10);
    }

    // SDL2 can resize window or toggle fullscreen without recreating a new window and therefore keeping existing GL context.
    if (currentScreensetting.fullscreen)
    {
        bool fullscreenSuccess = false;

        if ( sr_screenWidth + sr_screenHeight > 0 ) 
        {
            // find best display mode
            SDL_DisplayMode desiredMode, mode;
            desiredMode.format = 0;
            desiredMode.w = sr_screenWidth;
            desiredMode.h = sr_screenHeight;
            desiredMode.refresh_rate = currentScreensetting.refreshRate;
            desiredMode.driverdata = NULL;
            SDL_DisplayMode *closest = SDL_GetClosestDisplayMode(currentScreensetting.displayIndex, &desiredMode, &mode);
            if(closest)
            {
                // set the display mode
                sr_screenWidth = closest->w;
                sr_screenHeight = closest->h;

                if(0 == SDL_SetWindowDisplayMode(sr_screen, closest))
                {
                    fullscreenSuccess = (0 == SDL_SetWindowFullscreen(sr_screen, SDL_WINDOW_FULLSCREEN));
                    SDL_SetWindowSize(sr_screen, sr_screenWidth, sr_screenHeight);
                }
            }

            if(!fullscreenSuccess)
            {
                lastError.Clear();
                lastError << "Couldn't set video mode: ";
                lastError << SDL_GetError();
                std::cerr << lastError << '\n';
            }
        }

        // if desktop resolution was selected or custom mode setting failed, pick desktop mode
        if(!fullscreenSuccess)
        {
            sr_screenWidth = sr_desktopWidth;
            sr_screenHeight = sr_desktopHeight;
            fullscreenSuccess = (0 == SDL_SetWindowFullscreen(sr_screen, SDL_WINDOW_FULLSCREEN_DESKTOP));
            SDL_SetWindowSize(sr_screen, sr_screenWidth, sr_screenHeight);
        } 

        if(fullscreenSuccess)
        {
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }
        else
        {
            lastError.Clear();
            lastError << "Couldn't set desktop video mode: ";
            lastError << SDL_GetError();
            std::cerr << lastError << '\n';

            currentScreensetting.fullscreen = false;
            
            sr_screenWidth  = currentScreensetting.windowSize.width;
            sr_screenHeight = currentScreensetting.windowSize.height;
        }
    }
    if (!currentScreensetting.fullscreen)
    {
        // Set windowed mode and size accordingly
        if (!SDL_SetWindowFullscreen(sr_screen, 0)) 
        {
            SDL_SetWindowSize(sr_screen, sr_screenWidth, sr_screenHeight);
            SDL_SetWindowPosition(sr_screen, defaultX, defaultY);
            SDL_SetRelativeMouseMode(SDL_FALSE);
        } 
        else
        {
            lastError.Clear();
            lastError << "Couldn't set windowed mode: ";
            lastError << SDL_GetError();
            std::cerr << lastError << '\n';
            return false;
        }
    }

    #ifndef DEDICATED
    gl_vendor.Clear();
    gl_renderer.Clear();
    gl_version.Clear();
    gl_extensions.Clear();
    renderer_identification.Clear();

    // sanity check texture modes
    for(int i=rTextureGroups::TEX_GROUPS-1; i>=0; --i)
    {
        int & texmode = rTextureGroups::TextureMode[i];

        // don't do anything for deliberately disabled textures
        if( i == rTextureGroups::TEX_FONT || texmode >= 0 )
        {
            // to default if the modes have been reset for some reason
            if( texmode == 0 )
            {
                texmode = default_texturemode;
            }
            if( texmode < GL_NEAREST )
            {
                texmode = GL_NEAREST;
            }
            if( texmode > GL_LINEAR_MIPMAP_LINEAR )
            {
                texmode = GL_LINEAR_MIPMAP_LINEAR;
            }
        }
    }

    gl_vendor     << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    gl_renderer   << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    gl_version    << reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_extensions << reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

    // display list blacklist
    sr_blacklistDisplayLists=false;

    if(strstr(gl_version,"Mesa 7.0") || strstr(gl_version,"Mesa 7.1"))
    {
        // mesa DRI and software has problems in the 7.0/7.1 series
        sr_blacklistDisplayLists=true;
    }

    if(strstr(gl_vendor,"SiS"))
    {
        // almost nobody has those cards/chips, and we have
        // at least one bluescreen problem reported.
        sr_blacklistDisplayLists=true;
    }


#ifndef WIN32
    if(!strstr(gl_renderer,"Voodoo3"))
    #endif
    {
        if(currentScreensetting.fullscreen)
            SDL_ShowCursor(0);
        else
            SDL_ShowCursor(1);
    }

    #ifdef WIN32
    renderer_identification << "WIN32 ";
    #else
    #ifdef MACOSX
    renderer_identification << "MACOSX ";
    #else
    renderer_identification << "LINUX ";
    #endif
    #endif
    renderer_identification << rRenderIdCallback::RenderId() << ' ';
    renderer_identification << "SDL 1.2\n";
    renderer_identification << "CD=" << currentScreensetting.colorDepth  << '\n';
    renderer_identification << "FS=" << currentScreensetting.fullscreen  << '\n';
    renderer_identification << "GL_VENDOR=" << gl_vendor   << '\n';
    renderer_identification << "GL_RENDERER=" << gl_renderer << '\n';
    renderer_identification << "GL_VERSION=" << gl_version  << '\n';
    #endif

    if (// test for Windows software GL (be a little flexible...)
        (
            strstr(gl_vendor,"icrosoft") || strstr(gl_vendor,"SGI")
        )
        && strstr(gl_renderer,"eneric")
    )
        software_renderer=true;

    if ( // test for Mesa software GL
        strstr(gl_vendor,"rian") && strstr(gl_renderer,"X11") &&
        strstr(gl_renderer,"esa")
    )
        software_renderer=true;

    if ( // test for Mesa software GL, new versions
        strstr(gl_vendor,"Mesa") &&
        strstr(gl_renderer,"Software Rasterizer")
        )
        software_renderer=true;

    if ( // test for GLX software GL
        strstr(gl_renderer,"GLX") &&
        strstr(gl_renderer,"ndirect") &&
        strstr(gl_renderer,"esa")
    )
        software_renderer=true;

    // disable storage of non-alpha textures on Savage MX
    if ( strstr( gl_renderer, "SavageMX" ) )
    {
        rISurfaceTexture::storageHack_ = true;
    }

    // fonts look best in bilinear filtering, no mipmaps
    if ( rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] > GL_LINEAR )
        rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] = GL_LINEAR;

    // disable trilinear filtering for ATI cards
    if ( strstr( gl_vendor, "ATI" ) )
    {
        default_texturemode = GL_LINEAR_MIPMAP_NEAREST;
    }

    // wait for activation if we were ALT-Tabbed away:
    while ( !IsWindowActive() )
    {
        SDL_Delay(100);
        SDL_PumpEvents();
    }

    if (software_renderer && !last_software_renderer && !tRecorder::IsPlayingBack())
        sr_LoadDefaultConfig();

    last_software_renderer=software_renderer;


    // wait for activation if we were ALT-Tabbed away:
    while ( !IsWindowActive() )
    {
        SDL_Delay(100);
        SDL_PumpEvents();
    }

    sr_ResetRenderState(true);

    rCallbackAfterScreenModeChange::Exec();

    // store last display index
    sr_lastDisplayIndex = currentScreensetting.displayIndex;

    lastSuccess=currentScreensetting;
    failed_attempts = 0;
//    sr_useDirectX = use_directx_back;
    st_SaveConfig();

    return true;
}
#else // #if SDL_VERSION_ATLEAST(2,0,0)
static bool lowlevel_sr_InitDisplay(){
    rScreenSize & res = currentScreensetting.fullscreen ? currentScreensetting.res : currentScreensetting.windowSize;

    // update pixel aspect ratio
    if ( res.res != ArmageTron_Invalid  && size_t(res.res) < sizeof(aspect)/sizeof(aspect[0]) )
        currentScreensetting.aspect = aspect[res.res];

    res.UpdateSize();
    sr_screenWidth = res.width;
    sr_screenHeight= res.height;

    // desktop color depth
    static int desktopCD_R = 5;
    static int desktopCD_G = 5;
    static int desktopCD_B = 5;
    static int desktopCD   = 16;
    // desktop resolution
    static int sr_desktopWidth = 0, sr_desktopHeight = 0;

    static const int minWidth = 640;
    static const int minHeight = 480;

    // determine those values
    if ( sr_desktopWidth == 0 && !sr_screen )
    {
        // select sane defaults in case the following operation fails
        sr_desktopWidth = minWidth;
        sr_desktopHeight = minHeight;

        const SDL_VideoInfo* videoInfo     = SDL_GetVideoInfo( );
        if( videoInfo )
        {
            const SDL_PixelFormat* pixelFormat = videoInfo->vfmt;

            // don't accept anything less than 15 bpp, OpenGL doesn't like indexed colors.
            if( pixelFormat && 15 <= pixelFormat->BitsPerPixel && NULL == pixelFormat->palette )
            {
                desktopCD    = pixelFormat->BitsPerPixel;
                desktopCD_R  = countBits(pixelFormat->Rmask);
                desktopCD_G  = countBits(pixelFormat->Gmask);
                desktopCD_B  = countBits(pixelFormat->Bmask);
            }

            // the struct components we read here only exist since
            // SDL 1.2.10. The version check here is to safeguard against
            // code compiled against SDL 1.2.10, but linked with an earlier
            // version, accessing data out of bounds.
#if SDL_VERSION_ATLEAST(1, 2, 10)
            if( sr_DesktopScreensizeSupported() )
            {
                sr_desktopWidth  = videoInfo->current_w;
                sr_desktopHeight = videoInfo->current_h;
            }
#endif
        }
    }

    if (!sr_screen)
    {
        int singleCD_R	= 5;
        int singleCD_G	= 5;
        int singleCD_B	= 5;
        int fullCD		= 16;
        int zDepth		= 16;

        switch (currentScreensetting.colorDepth)
        {
        case ArmageTron_ColorDepth_16:
            // parameters already set for this depth
            break;
        case ArmageTron_ColorDepth_Desktop:
            {
                fullCD     = desktopCD;
                singleCD_R = desktopCD_R;
                singleCD_G = desktopCD_G;
                singleCD_B = desktopCD_B;
            }
            break;
        case ArmageTron_ColorDepth_32:
            singleCD_R	= 8;
            singleCD_G	= 8;
            singleCD_B	= 8;
            fullCD		= 24;
            zDepth		= 32;
            break;
        }

        switch ( currentScreensetting.zDepth )
        {
        case ArmageTron_ColorDepth_16: zDepth = 16; break;
        case ArmageTron_ColorDepth_32: zDepth = 32; break;
        default: break;
        }

        sr_SetGLAttributes( singleCD_R, singleCD_G, singleCD_B, zDepth );

        /*
          #ifdef POWERPAK_DEB
          PD_SetGFXMode(sr_screenWidth, sr_screenHeight, 32, PD_DEFAULT);
          sr_screen=DoubleBuffer;
          #else
        */

        int attrib;

#ifndef FORCE_WINDOW
        if (currentScreensetting.fullscreen)
        {
            attrib=SDL_OPENGL | SDL_FULLSCREEN;
        }
        else
#endif
        {
            attrib=SDL_OPENGL;
        }

    #ifdef FORCE_WINDOW
    #ifdef WIN32
        //		sr_screenWidth  = 400;
        //		sr_screenHeight = 300;
    #else
        //		sr_screenWidth  = minWidth;
        //		sr_screenHeight = 480;
    #endif
    #endif
        int CD = fullCD;

        // only check for errors if requested and if we're not about to set the
        // desktop resolution, where SDL_VideoModeOK apparently doesn't work.
        if (currentScreensetting.checkErrors && sr_screenWidth + sr_screenHeight > 0)
        {
            // check if the video mode should be OK:
            CD = SDL_VideoModeOK
                 (sr_screenWidth, sr_screenHeight,   fullCD,
                  attrib);

            // if not quite right
            if (CD < 15){
                // check if the other fs/windowed mode is better
                int CD_fsinv = SDL_VideoModeOK
                               (sr_screenWidth, sr_screenHeight,   fullCD,
                                attrib^SDL_FULLSCREEN);

                if (CD_fsinv >= 15){
                    // yes! change the mode
                    currentScreensetting.fullscreen=!currentScreensetting.fullscreen;
                    attrib ^= SDL_FULLSCREEN;
                    CD = CD_fsinv;
                }
            }

            if (CD < fullCD && currentScreensetting.colorDepth != ArmageTron_ColorDepth_16)
            {
                currentScreensetting.colorDepth = ArmageTron_ColorDepth_16;

                sr_SetGLAttributes( 5, 5, 5, 16 );
            }
        }

        // if desktop resolution was selected, pick it
        if ( sr_screenWidth + sr_screenHeight == 0 )
        {
            sr_screenWidth = sr_desktopWidth;
            sr_screenHeight = sr_desktopHeight;
        }
        else
        {
            // have the screen reinited
            sr_screen = NULL;
        }

        // only reinit the screen if the desktop res detection hasn't left us
        // with a perfectly good one.
        if ( !sr_screen && (sr_screen=SDL_SetVideoMode (sr_screenWidth, sr_screenHeight, CD, attrib)) == NULL) {
            if((sr_screen=SDL_SetVideoMode (sr_screenWidth, sr_screenHeight, CD, attrib^SDL_FULLSCREEN))==NULL ) {
                lastError.Clear();
                lastError << "Couldn't set video mode: ";
                lastError << SDL_GetError();
                std::cerr << lastError << '\n';
                return false;
            }
            else
            {
                currentScreensetting.fullscreen=!currentScreensetting.fullscreen;
            }
        }

        sr_SetWindowTitle();

        sr_CompleteGLAttributes();

        SDL_EnableUNICODE(1);
    }

    #ifndef DEDICATED
    gl_vendor.Clear();
    gl_renderer.Clear();
    gl_version.Clear();
    gl_extensions.Clear();
    renderer_identification.Clear();

    // sanity check texture modes
    for(int i=rTextureGroups::TEX_GROUPS-1; i>=0; --i)
    {
        int & texmode = rTextureGroups::TextureMode[i];

        // don't do anything for deliberately disabled textures
        if( i == rTextureGroups::TEX_FONT || texmode >= 0 )
        {
            // to default if the modes have been reset for some reason
            if( texmode == 0 )
            {
                texmode = default_texturemode;
            }
            if( texmode < GL_NEAREST )
            {
                texmode = GL_NEAREST;
            }
            if( texmode > GL_LINEAR_MIPMAP_LINEAR )
            {
                texmode = GL_LINEAR_MIPMAP_LINEAR;
            }
        }
    }

    gl_vendor     << reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    gl_renderer   << reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    gl_version    << reinterpret_cast<const char *>(glGetString(GL_VERSION));
    gl_extensions << reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

    // display list blacklist
    sr_blacklistDisplayLists=false;

    if(strstr(gl_version,"Mesa 7.0") || strstr(gl_version,"Mesa 7.1"))
    {
        // mesa DRI and software has problems in the 7.0/7.1 series
        sr_blacklistDisplayLists=true;
    }

    if(strstr(gl_vendor,"SiS"))
    {
        // almost nobody has those cards/chips, and we have
        // at least one bluescreen problem reported.
        sr_blacklistDisplayLists=true;
    }


#ifndef WIN32
    if(!strstr(gl_renderer,"Voodoo3"))
    #endif
    {
        if(currentScreensetting.fullscreen)
            SDL_ShowCursor(0);
        else
            SDL_ShowCursor(1);
    }

    #ifdef WIN32
    renderer_identification << "WIN32 ";
    #else
    #ifdef MACOSX
    renderer_identification << "MACOSX ";
    #else
    renderer_identification << "LINUX ";
    #endif
    #endif
    renderer_identification << rRenderIdCallback::RenderId() << ' ';
    renderer_identification << "SDL 1.2\n";
    renderer_identification << "CD=" << currentScreensetting.colorDepth  << '\n';
    renderer_identification << "FS=" << currentScreensetting.fullscreen  << '\n';
    renderer_identification << "GL_VENDOR=" << gl_vendor   << '\n';
    renderer_identification << "GL_RENDERER=" << gl_renderer << '\n';
    renderer_identification << "GL_VERSION=" << gl_version  << '\n';
    #endif

    if (// test for Windows software GL (be a little flexible...)
        (
            strstr(gl_vendor,"icrosoft") || strstr(gl_vendor,"SGI")
        )
        && strstr(gl_renderer,"eneric")
    )
        software_renderer=true;

    if ( // test for Mesa software GL
        strstr(gl_vendor,"rian") && strstr(gl_renderer,"X11") &&
        strstr(gl_renderer,"esa")
    )
        software_renderer=true;

    if ( // test for Mesa software GL, new versions
        strstr(gl_vendor,"Mesa") &&
        strstr(gl_renderer,"Software Rasterizer")
        )
        software_renderer=true;

    if ( // test for GLX software GL
        strstr(gl_renderer,"GLX") &&
        strstr(gl_renderer,"ndirect") &&
        strstr(gl_renderer,"esa")
    )
        software_renderer=true;

    // disable storage of non-alpha textures on Savage MX
    if ( strstr( gl_renderer, "SavageMX" ) )
    {
        rISurfaceTexture::storageHack_ = true;
    }

    // fonts look best in bilinear filtering, no mipmaps
    if ( rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] > GL_LINEAR )
        rTextureGroups::TextureMode[rTextureGroups::TEX_FONT] = GL_LINEAR;

    // disable trilinear filtering for ATI cards
    if ( strstr( gl_vendor, "ATI" ) )
    {
        default_texturemode = GL_LINEAR_MIPMAP_NEAREST;
    }

    // wait for activation if we were ALT-Tabbed away:
    while ( (SDL_GetAppState() & SDL_APPACTIVE) == 0)
    {
        SDL_Delay(100);
        SDL_PumpEvents();
    }

    if (software_renderer && !last_software_renderer && !tRecorder::IsPlayingBack())
        sr_LoadDefaultConfig();

    last_software_renderer=software_renderer;


    // wait for activation if we were ALT-Tabbed away:
    while ( (SDL_GetAppState() & SDL_APPACTIVE) == 0)
    {
        SDL_Delay(100);
        SDL_PumpEvents();
    }

    sr_ResetRenderState(true);

    rCallbackAfterScreenModeChange::Exec();

    lastSuccess=currentScreensetting;
    failed_attempts = 0;
//    sr_useDirectX = use_directx_back;
    st_SaveConfig();

    return true;
}
#endif // #if SDL_VERSION_ATLEAST(2,0,0)
#else // #ifdef DEDICATED
static bool lowlevel_sr_InitDisplay()
{
    return true;
}
#endif

bool cycleprograminited = false;

bool sr_InitDisplay(){
//    use_directx_back = sr_useDirectX;

    cycleprograminited = false;
    while (failed_attempts <= MAXEMERGENCY+1)
    {
        if (failed_attempts)
        {
#ifdef DEBUG
            std::cout << "failed attempts:" << failed_attempts << "\n";
            std::cout.flush();
#endif
            currentScreensetting = *emergency[failed_attempts];

//            sr_useDirectX = false;
        }

        // prepare for crash, note failure and save config
        failed_attempts++;
        st_SaveConfig();

    #ifdef MACOSX
    #if !SDL_VERSION_ATLEAST(2,0,0)
        // init the screen once in windowed mode
        static bool first = true;
        if ( first && currentScreensetting.fullscreen )
        {
            first = false;
            currentScreensetting.fullscreen = false;

            sr_LockSDL();
            if (lowlevel_sr_InitDisplay())
            {
                sr_ExitDisplay();
            }
            sr_UnlockSDL();

            currentScreensetting.fullscreen = true;
        }
    #endif
    #endif

        sr_LockSDL();
        if (lowlevel_sr_InitDisplay())
        {
            sr_UnlockSDL();
            failed_attempts = 0;
            st_SaveConfig();
            return true;
        }

        st_SaveConfig();

        if (lowlevel_sr_InitDisplay())
        {
            sr_UnlockSDL();
            failed_attempts = 0;
            st_SaveConfig();
            return true;
        }
        sr_UnlockSDL();


    }

    failed_attempts = 1;
    st_SaveConfig();

    tERR_ERROR("\nSorry, played all my cards trying to "
               "initialize your video system.\n"
               << tOutput("$program_name") << " won't run on your computer. Reason:\n\n"
               << lastError
               << "\n\nI'll try again from the beginning, but the "
               << "chances of success are minimal.\n"
              );

    return false;
}


void sr_ExitDisplay(){
    #ifndef DEDICATED
    rCallbackBeforeScreenModeChange::Exec();

    if (sr_screen){
        sr_LockSDL();
#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_SetWindowFullscreen(sr_screen, 0);
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_DestroyRenderer(sr_screenRenderer);
        SDL_DestroyWindow(sr_screen);
#else
        // z-man: according to man SDL_SetVideoSurface, screen should not bee freed.
        // SDL_FreeSurface(sr_screen);
        sr_screen=NULL;
#endif
        sr_UnlockSDL();
        //SDL_Quit();
    }
    #endif
}

bool    sr_alphaBlend=true;
bool    sr_glOut=true;
bool    sr_smoothShading=true;


int sr_floorMirror=0;
int sr_floorDetail=rFLOOR_TEXTURE;
bool sr_highRim=true;
bool sr_upperSky=false;
bool sr_lowerSky=false;
bool sr_skyWobble=true;
bool sr_dither=true;
bool sr_infinityPlane=false;
bool sr_laggometer=true;
bool sr_predictObjects=false;
bool sr_texturesTruecolor=true;

bool sr_textOut=false;
bool sr_FPSOut=true;

bool sr_keepWindowActive=true;

tString renderer_identification;

void sr_LoadDefaultConfig(){

    // High detail defaults; no problem for your ordinary 3d-card.
    sr_alphaBlend=true;
    sr_useDisplayLists=rDisplayList_Off;
    sr_textOut=true;
    sr_dither=true;
    sr_smoothShading=true;
    int i;
    #ifndef DEDICATED
    for (i=rTextureGroups::TEX_GROUPS-1;i>=0;i--)
        rTextureGroups::TextureMode[i]=default_texturemode;

    // fonts look best in bilinear filtering, no mipmaps
    rTextureGroups::TextureMode[rTextureGroups::TEX_FONT]=GL_LINEAR;
    #endif
    sr_floorDetail=rFLOOR_TWOTEXTURE;
    sr_floorMirror=rMIRROR_OFF;
    sr_infinityPlane=false;
    sr_lowerSky=false;
    sr_upperSky=false;
    sr_keepWindowActive=true;

    if (software_renderer){
        // A software renderer! Poor soul. Set low details:
        for (i=rTextureGroups::TEX_GROUPS-1;i>=0;i--)
            rTextureGroups::TextureMode[i]=-1;

    #ifndef DEDICATED
        rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ]=GL_NEAREST_MIPMAP_NEAREST;
        rTextureGroups::TextureMode[rTextureGroups::TEX_FONT]=GL_NEAREST_MIPMAP_NEAREST;
    #endif

        sr_highRim=false;
        sr_dither=false;
        sr_alphaBlend=false;
        sr_smoothShading=true; // smooth shading does not slow down the
        // two tested renderers; leave it it.
        sr_floorDetail=rFLOOR_GRID;
        sr_floorMirror=rMIRROR_OFF;
    }
    else if(strstr(gl_vendor,"3Dfx")){
        //workaround for 3dfx renderer: aliasing must be turned on
        //sr_lineAntialias=rFEAT_OFF;
    }
    else if(strstr(gl_vendor,"NVIDIA")){
        // infinity , display lists and glFlush swapping work for NVIDIA
        sr_infinityPlane=true;
        sr_useDisplayLists=rDisplayList_CAC;
        // rSysDep::swapMode_=rSysDep::rSwap_glFlush;
    }
    #ifdef MACOSX
    else if(strstr(gl_vendor,"ATI")){
        // glFlush swapping work for ATI on the mac
        // rSysDep::swapMode_=rSysDep::rSwap_glFlush;
    }
    #endif
    else if(strstr(gl_vendor,"Matrox")){
        sr_floorDetail = rFLOOR_TEXTURE;  // double textured floor does not work
    }

    /*
    else if(strstr(gl_version,"Mesa"))
    {
        sr_useDisplayLists=rDisplayList_Off;
    }
    */
}

void sr_ResetRenderState(bool menu){
    if(!sr_glOut)
        return;
    #ifndef DEDICATED

    // Z-Buffering and perspective correction

    if (menu){
        glDisable(GL_DEPTH_TEST);
        glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        glViewport (0, 0, GLsizei(sr_screenWidth), GLsizei(sr_screenHeight));
    }
    else{
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }

    if (sr_dither)
        glEnable(GL_DITHER);
    else
        glDisable(GL_DITHER);

    glDisable(GL_LIGHTING);

    // disable texture mapping (selecting textures will reactivate it)

    //  glDisable(GL_TEXTURE);
    glDisable(GL_TEXTURE_2D);


    // flat or smooth shading
    if (sr_smoothShading)
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);

    // alpha blending
    if (sr_alphaBlend){
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    }
    else{
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
    }

    // reset matrices
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    #endif
}


/*
static uMenuItemFunction apply
(&sg_screenMenu_mode,"Apply Changes",
"This activates the changes to the resolution and fullscreen/windowed mode "
"made above. This does not work on all systems; exit and reenter Armagetron "
"instead if you experience problems.",
 sr_ReinitDisplay);
*/





//static bool offs=false;

void sr_DepthOffset(bool offset){
    // return;
    //  if(offset!=offs){
    //offs=offset;
    #ifndef DEDICATED
    if (offset){
        //glMatrixMode(GL_PROJECTION);
        //glScalef(.9,.9,.9);
        glPolygonOffset(-2,-5);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glEnable(GL_POLYGON_OFFSET_POINT);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }
    else{
        glPolygonOffset(0,0);
        glDisable(GL_POLYGON_OFFSET_POINT);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        //glMatrixMode(GL_PROJECTION);
        //glScalef(1/.9,1/.9,1/.9);
    }
    //  }
    #endif
}

// set activation status
void sr_Activate(bool active)
{
    #ifndef DEDICATED
    if ( !currentScreensetting.fullscreen && !active && sr_keepWindowActive )
    {
        sr_glOut=!active;
    }
    else
    {
        sr_glOut=active;
    }

    // unload textures and stuff if rendering gets disabled
    if (!sr_glOut)
        rCallbackBeforeScreenModeChange::Exec();

    // Jonathans fullscreen bugfix.
    // z-man's ammendmend: apparently, doing this in Linux is painful as well.
    // Only on Windows, you get a deactivation event when you ALT-TAB away
    // from th application, then iconification is the right thing to do.
    // On Linux at least, there is no standard alt-tab for fullscreen applications.
#ifdef WIN32
    if ( currentScreensetting.fullscreen && !active )
    {
        SDL_WM_IconifyWindow();
    }
    #endif
    #endif
}

tString & sr_CurrentWindowTitle()
{
    static tString title(tOutput("$window_title_menu"));
    return title;
}

void sr_SetWindowTitle(tOutput o)
{
    tString s;
    s << o;

    sr_SetWindowTitle(s);
}

void sr_SetWindowTitle(tString s)
{
    sr_CurrentWindowTitle() = s;
#ifdef MACOSX
    if(!currentScreensetting.fullscreen)
#endif
    {
#ifndef DEDICATED
#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_SetWindowTitle(sr_screen, s);
#else
        SDL_WM_SetCaption(s, s);
#endif
#endif
    }
}

void sr_SetWindowTitle()
{
    sr_SetWindowTitle(sr_CurrentWindowTitle());
}

//**************************************
//** Screen mode callbacks            **
//**************************************


static tCallback *sr_BeforeAnchor;

rCallbackBeforeScreenModeChange::rCallbackBeforeScreenModeChange(AA_VOIDFUNC *f)
        :tCallback(sr_BeforeAnchor, f){}

void rCallbackBeforeScreenModeChange::Exec()
{
    tCallback::Exec(sr_BeforeAnchor);
}

static tCallback *sr_AfterAnchor;

rCallbackAfterScreenModeChange::rCallbackAfterScreenModeChange(AA_VOIDFUNC *f)
        :tCallback(sr_AfterAnchor, f){}

void rCallbackAfterScreenModeChange::Exec()
{
    tCallback::Exec(sr_AfterAnchor);
}

