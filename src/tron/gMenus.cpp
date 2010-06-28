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

#include "gMenus.h"
#include "ePlayer.h"
#include "rScreen.h"
#include "nConfig.h"
#include "rConsole.h"
#include "tToDo.h"
#include "rGL.h"
#include "eTimer.h"
#include "eFloor.h"
#include "rRender.h"
#include "rModel.h"
#include "gGame.h"
#include "gCycle.h"
#include "gHud.h"
#include "tRecorder.h"
#include "rSysdep.h"

#include <sstream>
#include <set>

static tConfItem<int>   tm0("TEXTURE_MODE_0",rTextureGroups::TextureMode[0]);
static tConfItem<int>   tm1("TEXTURE_MODE_1",rTextureGroups::TextureMode[1]);
static tConfItem<int>   tm2("TEXTURE_MODE_2",rTextureGroups::TextureMode[2]);
static tConfItem<int>   tm3("TEXTURE_MODE_3",rTextureGroups::TextureMode[3]);

uMenu sg_screenMenu("$display_settings_menu");

static uMenuItemFunction defaul
(&sg_screenMenu,"$graphics_load_defaults_text",
 "$graphics_load_defaults_help",
 &sr_LoadDefaultConfig);
uMenu screen_menu_detail("$detail_settings_menu");
uMenu screen_menu_tweaks("$performance_tweaks_menu");
uMenu screen_menu_prefs("$preferences_menu");
uMenu hud_prefs("$hud_menu");

static void sg_ScreenModeMenu();

static uMenuItemSubmenu smt(&sg_screenMenu,&screen_menu_tweaks,
                            "$performance_tweaks_menu_help");
static uMenuItemSubmenu smd(&sg_screenMenu,&screen_menu_detail,
                            "$detail_settings_menu_help");
static uMenuItemSubmenu smp(&sg_screenMenu,&screen_menu_prefs,
                            "$preferences_menu_help");
static uMenuItemFunction smm(&sg_screenMenu,"$screen_mode_menu",
                             "$screen_mode_menu_help", sg_ScreenModeMenu );

static uMenuItemSubmenu huds(&screen_menu_prefs,&hud_prefs,
                             "$hud_menu_help");


static tConfItemLine c_ext("GL_EXTENSIONS",gl_extensions);
static tConfItemLine c_ver("GL_VERSION",gl_version);
static tConfItemLine c_rEnd("GL_RENDERER",gl_renderer);
static tConfItemLine c_vEnd("GL_VENDOR",gl_vendor);
// static tConfItemLine a_ver("ARMAGETRON_VERSION",sn_programVersion);

static std::deque<tString> sg_consoleHistory; // global since the class doesn't live beyond the execution of the command
static int sg_consoleHistoryMaxSize=10; // size of the console history
static tSettingItem< int > sg_consoleHistoryMaxSizeConf("HISTORY_SIZE_CONSOLE",sg_consoleHistoryMaxSize);

class ArmageTron_feature_menuitem: public uMenuItemSelection<int>{
    void NewChoice(uSelectItem<bool> *){};
    void NewChoice(char *,bool ){};
public:
    ArmageTron_feature_menuitem(uMenu *m,char const * tit,char const * help,int &targ)
            :uMenuItemSelection<int>(m,tit,help,targ){
        uMenuItemSelection<int>::NewChoice(
            "$feature_disabled_text",
            "$feature_disabled_help",
            rFEAT_OFF);
        uMenuItemSelection<int>::NewChoice(
            "$feature_default_text",
            "$feature_default_help",
            rFEAT_DEFAULT);
        uMenuItemSelection<int>::NewChoice(
            "$feature_enabled_text",
            "$feature_enabled_help",
            rFEAT_ON);
    }

    ~ArmageTron_feature_menuitem(){};
};


class ArmageTron_texmode_menuitem: public uMenuItemSelection<int>{
    void NewChoice(uSelectItem<bool> *){};
    void NewChoice(char *,bool ){};
public:
    ArmageTron_texmode_menuitem(uMenu *m,char const * tit,int &targ,
                                bool font=false)
            :uMenuItemSelection<int>
    (m,tit,"$texture_menuitem_help",targ){

        if(!font)
            uMenuItemSelection<int>::NewChoice
            ("$texture_off_text","$texture_off_help",-1);
#ifndef DEDICATED
        uMenuItemSelection<int>::NewChoice
        ("$texture_nearest_text","$texture_nearest_help",GL_NEAREST);

        uMenuItemSelection<int>::NewChoice
        ("$texture_bilinear_text","$texture_bilinear_help",GL_LINEAR);

        if(!font)
        {
            uMenuItemSelection<int>::NewChoice
            ("$texture_mipmap_nearest_text",
             "$texture_mipmap_nearest_help",
             GL_NEAREST_MIPMAP_NEAREST);
            uMenuItemSelection<int>::NewChoice
            ("$texture_mipmap_bilinear_text",
             "$texture_mipmap_bilinear_help",
             GL_LINEAR_MIPMAP_NEAREST);
            uMenuItemSelection<int>::NewChoice
            ("$texture_mipmap_trilinear_text",
             "$texture_mipmap_trilinear_help",
             GL_LINEAR_MIPMAP_LINEAR);
        }
    #endif
    }

    ~ArmageTron_texmode_menuitem(){};
};

static tConfItem<bool>    ab("ALPHA_BLEND",sr_alphaBlend);
static tConfItem<bool>    ss("SMOOTH_SHADING",sr_smoothShading);
static tConfItem<bool>    to("TEXT_OUT",sr_textOut);
static tConfItem<bool>    fps("SHOW_FPS",sr_FPSOut);
// tConfItem<> ("",&);
static tConfItem<int> fm("FLOOR_MIRROR",sr_floorMirror);
static tConfItem<int> fd("FLOOR_DETAIL",sr_floorDetail);
static tConfItem<bool> hr("HIGH_RIM",sr_highRim);
static tConfItem<bool> dt("DITHER",sr_dither);
static tConfItem<bool> us("UPPER_SKY",sr_upperSky);
static tConfItem<bool> ls("LOWER_SKY",sr_lowerSky);
static tConfItem<bool> wos("SKY_WOBBLE",sr_skyWobble);
static tConfItem<bool> ip("INFINITY_PLANE",sr_infinityPlane);

extern bool sg_axesIndicator;

static tConfItem<bool> lm("LAG_O_METER",sr_laggometer);
static tConfItem<bool> ai("AXES_INDICATOR",sg_axesIndicator);
static tConfItem<bool> po("PREDICT_OBJECTS",sr_predictObjects);
static tConfItem<bool> t32("TEXTURES_HI",sr_texturesTruecolor);

static tConfItem<bool> kwa("KEEP_WINDOW_ACTIVE",sr_keepWindowActive);
#ifdef USE_HEADLIGHT
static tConfItem<bool> chl("HEADLIGHT",headlights);
#endif

#ifndef SDL_OPENGL
#ifndef DIRTY
#define DIRTY
#endif
#endif

bool operator < ( rScreenSize const & a, rScreenSize const & b )
{
    return a.Compare(b) < 0;
}

class gResMenEntry
{
    uMenuItemSelection<rScreenSize> res_men; // menu item
    std::set< rScreenSize > sizes;           // set of already added modes

    // adds a single custom screen resolution
    void NewChoice( rScreenSize const & size )
    {
        if ( sizes.find( size ) == sizes.end() )
        {
            sizes.insert(size);
        }
    }

    // adds a single predefined resolution
    void NewChoice( rResolution res )
    {
        rScreenSize size( res );
        NewChoice( size );
    }

public:
    gResMenEntry( uMenu & screen_menu_mode, rScreenSize& res, const tOutput& text, const tOutput& help, bool addFixed )
            :res_men
            (&screen_menu_mode,
             text,
             help,
             res)
    {
#ifndef DEDICATED
        // fetch valid screen modes from SDL
        SDL_Rect **modes;
        modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);

        // Check is there are any modes available
        int i;
        if(modes == 0 || modes == (SDL_Rect **)-1)
        {
            // add all fixed resolutions
            for ( i = ArmageTron_Custom; i>=0; --i )
            {
                NewChoice( rResolution(i) );
            }
        }
        else
        {
            // add custom resolution
            NewChoice( ArmageTron_Custom );

            // add desktop resolution
            if ( sr_DesktopScreensizeSupported() && !addFixed )
                NewChoice( ArmageTron_Desktop );

            // the maximal allowed screen size
            rScreenSize maxSize(0,0);

            // fill in available modes (avoid duplicates)
            for(i=0;modes[i];++i)
            {
                // add mode (if it's new)
                rScreenSize size(modes[i]->w, modes[i]->h);
                NewChoice( size );
                if ( maxSize.width < size.width )
                    maxSize.width = size.width;
                if ( maxSize.height < size.height )
                    maxSize.height = size.height;
            }

            // add fixed resolutions (as window sizes)
            if ( addFixed )
            {
                for ( i = ArmageTron_Custom; i>=ArmageTron_Min; --i )
                {
                    rScreenSize size( static_cast< rResolution >(i) );

                    // only add those that fit the maximal resolution
                    if ( maxSize.height >= size.height && maxSize.width >= size.width )
                        NewChoice( size );
                }
            }
        }

        // insert sorted resolutions into menu
        for( std::set< rScreenSize >::iterator iter = sizes.begin(); iter != sizes.end(); ++iter )
            {
                rScreenSize const & size = *iter;

                std::stringstream s;
                if ( size.width + size.height > 0 )
                    s << size.width << " x " << size.height;
                else
                    s << tOutput("$screen_size_desktop");

                res_men.NewChoice( s.str().c_str(), help, size );
            }

#endif
    }
};

static void sg_ScreenModeMenu()
{
    uMenu screen_menu_mode("$screen_mode_menu");

    uMenuItemFunction appl
    (&screen_menu_mode,
     "$screen_apply_changes_text",
     "$screen_apply_changes_help",
     &sr_ReinitDisplay);

    uMenuItemToggle kwa_t(
        &screen_menu_mode,
        "$screen_keep_window_active_text",
        "$screen_keep_window_active_help",
        sr_keepWindowActive);

    uMenuItemToggle ie_t
    (&screen_menu_mode,
     "$screen_check_errors_text",
     "$screen_check_errors_help",
     currentScreensetting.checkErrors);


#ifdef SDL_OPENGL
#ifdef DIRTY
    uMenuItemToggle sdl_t
    (&screen_menu_mode,
     "$screen_use_sdl_text",
     "$screen_use_sdl_help",
     currentScreensetting.useSDL);
#endif
#endif

    uMenuItemToggle gm(
        &screen_menu_mode,
        "$screen_grab_mouse_text",
        "$screen_grab_mouse_help",
        su_mouseGrab);

    uMenuItemSelection<rColorDepth> zd_t
    (&screen_menu_mode,
     "$screen_zdepth_text",
     "$screen_zdepth_help",
     currentScreensetting.zDepth);

    uSelectEntry<rColorDepth> zd_16(zd_t,"$screen_zdepth_16_text","$screen_zdepth_16_help",ArmageTron_ColorDepth_16);
    uSelectEntry<rColorDepth> zd_d(zd_t,"$screen_zdepth_desk_text","$screen_zdepth_desk_help",ArmageTron_ColorDepth_Desktop);
    uSelectEntry<rColorDepth> zd_32(zd_t,"$screen_zdepth_32_text","$screen_zdepth_32_help",ArmageTron_ColorDepth_32);

    uMenuItemSelection<rColorDepth> cd_t
    (&screen_menu_mode,
     "$screen_colordepth_text",
     "$screen_colordepth_help",
     currentScreensetting.colorDepth);

    uSelectEntry<rColorDepth> cd_16(cd_t,"$screen_colordepth_16_text","$screen_colordepth_16_help",ArmageTron_ColorDepth_16);
    uSelectEntry<rColorDepth> cd_d(cd_t,"$screen_colordepth_desk_text","$screen_colordepth_desk_help",ArmageTron_ColorDepth_Desktop);
    uSelectEntry<rColorDepth> cd_32(cd_t,"$screen_colordepth_32_text","$screen_colordepth_32_help",ArmageTron_ColorDepth_32);

    uMenuItemToggle fs_t
    (&screen_menu_mode,
     "$screen_fullscreen_text",
     "$screen_fullscreen_help",
     currentScreensetting.fullscreen);


    gResMenEntry res( screen_menu_mode, currentScreensetting.res, "$screen_resolution_text", "$screen_resolution_help", false );
    gResMenEntry winsize( screen_menu_mode, currentScreensetting.windowSize, "$window_size_text", "$window_size_help", true );

    /*
    uMenuItemSelection<rResolution> res_men
    (&screen_menu_mode,
     "$screen_resolution_text",
     "$screen_resolution_help",
     currentScreensetting.res);


    uSelectEntry<rResolution> a(res_men,"320x200","",ArmageTron_320_200);
     static uSelectEntry<rResolution> b(res_men,"320x240","",ArmageTron_320_240);
    static uSelectEntry<rResolution> c(res_men,"400x300","",ArmageTron_400_300);
    static uSelectEntry<rResolution> d(res_men,"512x384","",ArmageTron_512_384);
    static uSelectEntry<rResolution> e(res_men,"640x480","",ArmageTron_640_480);
    static uSelectEntry<rResolution> f(res_men,"800x600","",ArmageTron_800_600);
    static uSelectEntry<rResolution> g(res_men,"1024x768","",ArmageTron_1024_768);
    static uSelectEntry<rResolution> h(res_men,"1280x1024","",ArmageTron_1280_1024);
    static uSelectEntry<rResolution> i(res_men,"1600x1200","",ArmageTron_1600_1200);
    static uSelectEntry<rResolution> j(res_men,"2048x1572","",ArmageTron_2048_1572);
    static uSelectEntry<rResolution> jj(res_men,
    				    "$screen_custom_text",
    				    "$screen_custom_help"
    				    ,ArmageTron_Custom);
    */

    screen_menu_mode.Enter();
}


static uMenuItemSelection<int> mfm
(&screen_menu_detail,
 "$detail_floor_mirror_text",
 "$detail_floor_mirror_help",
 sr_floorMirror);
static uSelectEntry<int> mfma(mfm,"$detail_floor_mirror_off_text","$detail_floor_mirror_off_help",rMIRROR_OFF);


static uSelectEntry<int> mfmb(mfm,"$detail_floor_mirror_obj_text",
                              "$detail_floor_mirror_obj_help",
                              rMIRROR_OBJECTS);

static uSelectEntry<int> mfmc(mfm,"$detail_floor_mirror_ow_text",
                              "$detail_floor_mirror_ow_help",
                              rMIRROR_WALLS);

static uSelectEntry<int> mfme(mfm,"$detail_floor_mirror_ev_text","$detail_floor_mirror_ev_help",rMIRROR_ALL);

static uMenuItemToggle fs_dither
(&screen_menu_detail,"$detail_dither_text",
 "$detail_dither_help",
 sr_dither);

static uMenuItemSelection<int> mfd
(&screen_menu_detail,
 "$detail_floor_text",
 "$detail_floor_help",
 sr_floorDetail);

static uSelectEntry<int> mfda(mfd,"$detail_floor_no_text",
                              "$detail_floor_no_help",
                              rFLOOR_OFF);
static uSelectEntry<int> mfdb(mfd,"$detail_floor_grid_text",
                              "$detail_floor_grid_help",
                              rFLOOR_GRID);
static uSelectEntry<int> mfdc(mfd,"$detail_floor_tex_text",
                              "$detail_floor_tex_help",
                              rFLOOR_TEXTURE);
static uSelectEntry<int> mfdd(mfd,"$detail_floor_2tex_text",
                              "$detail_floor_2tex_help",
                              rFLOOR_TWOTEXTURE);

static uMenuItemToggle  abm
(&screen_menu_detail,"$detail_alpha_text",
 "$detail_alpha_help",
 sr_alphaBlend);

static uMenuItemToggle  ssm
(&screen_menu_detail,"$detail_smooth_text",
 "$detail_smooth_help",
 sr_smoothShading);

extern bool crash_sparks;		// from gCycle.cpp
extern bool white_sparks;		// from gSparks.cpp
static tConfItem<bool> cs2("SPARKS",crash_sparks);
static tConfItem<bool> wsp("WHITE_SPARKS",white_sparks);

extern bool sg_crashExplosion;   // from gExplosion.cpp
static tConfItem<bool> crexp("EXPLOSION",sg_crashExplosion);

#ifndef DEDICATED
//extern bool png_screenshot;		// from rSysdep.cpp
//static tConfItem<bool> pns("PNG_SCREENSHOT",png_screenshot);
#endif

static uMenuItemToggle  t32b
(&screen_menu_detail,"$detail_text_truecolor_text",
 "$detail_text_truecolor_help"
 ,sr_texturesTruecolor);


static ArmageTron_texmode_menuitem tmm0(&screen_menu_detail,
                                        rTextureGroups::TextureGroupDescription[0],
                                        rTextureGroups::TextureMode[0]);

static ArmageTron_texmode_menuitem tmm1(&screen_menu_detail,
                                        rTextureGroups::TextureGroupDescription[1],
                                        rTextureGroups::TextureMode[1]);

static ArmageTron_texmode_menuitem tmm2(&screen_menu_detail,
                                        rTextureGroups::TextureGroupDescription[2],
                                        rTextureGroups::TextureMode[2]);

static ArmageTron_texmode_menuitem tmm3(&screen_menu_detail,
                                        rTextureGroups::TextureGroupDescription[3],
                                        rTextureGroups::TextureMode[3],true);


static uMenuItemToggle s2
(&screen_menu_prefs,"$pref_highrim_text",
 "$pref_highrim_help",sr_highRim);

static uMenuItemToggle us2
(&screen_menu_prefs,"$pref_uppersky_text",
 "$pref_uppersky_help",
 sr_upperSky);

static uMenuItemToggle ls2
(&screen_menu_prefs,"$pref_lowersky_text",
 "$pref_lowersky_help",
 sr_lowerSky);

uMenuItemToggle fps2
(&screen_menu_prefs,"$misc_fps_text",
 "$misc_fps_help",sr_FPSOut);

#ifdef USE_HEADLIGHT
static uMenuItemToggle uchl(&screen_menu_prefs,"$pref_headlight_text","$pref_headlight_help",
                            headlights);
#endif


static uMenuItemToggle ws2
(&screen_menu_prefs,"$pref_skymove_text",
 "$pref_skymove_help",
 sr_skyWobble);

static uMenuItemToggle crexp2
(&screen_menu_prefs,"$pref_explosion_text",
 "$pref_explosion_help",
 sg_crashExplosion);

static uMenuItemToggle cs
(&screen_menu_prefs,"$pref_sparks_text",
 "$pref_sparks_help",
 crash_sparks);

static uMenuItemSelection<rDisplayListUsage> dl
(&screen_menu_tweaks,"$tweaks_displaylists_text",
 "$tweaks_displaylists_help", sr_useDisplayLists);
static uSelectEntry<rDisplayListUsage> dl_off(dl,"$tweaks_displaylists_off_text","$tweaks_displaylists_off_help",rDisplayList_Off);
static uSelectEntry<rDisplayListUsage> dl_cac(dl,"$tweaks_displaylists_cac_text","$tweaks_displaylists_cac_help",rDisplayList_CAC);
static uSelectEntry<rDisplayListUsage> dl_cae(dl,"$tweaks_displaylists_cae_text","$tweaks_displaylists_cae_help",rDisplayList_CAE);

static uMenuItemToggle infp
(&screen_menu_tweaks,"$tweaks_infinity_text",
 "$tweaks_infinity_help"
 ,sr_infinityPlane);

uMenuItemSelection<rSysDep::rSwapMode> swapMode
(&screen_menu_tweaks,
 "$swapmode_text",
 "$swapmode_help",
 rSysDep::swapMode_);

static uSelectEntry<rSysDep::rSwapMode> swapMode_fastest(swapMode,"$swapmode_fastest_text","$swapmode_fastest_help",rSysDep::rSwap_Fastest);
static uSelectEntry<rSysDep::rSwapMode> swapMode_glFlush(swapMode,"$swapmode_glflush_text","$swapmode_glflush_help",rSysDep::rSwap_glFlush);
static uSelectEntry<rSysDep::rSwapMode> swapMode_glFinish(swapMode,"$swapmode_glfinish_text","$swapmode_glfinish_help",rSysDep::rSwap_glFinish);

/*
uMenuItemSelection<int> targetFPS
(&screen_menu_tweaks,
 "$targetfps_text",
 "$targetfps_help",
rSysDep::swapMode_);

static uSelectEntry<rSysDep::rSwapMode> swapMode_150Hz(swapMode,"$swapmode_150hz_text","$swapmode_150hz_help",rSysDep::rSwap_150Hz);
static uSelectEntry<rSysDep::rSwapMode> swapMode_100Hz(swapMode,"$swapmode_100hz_text","$swapmode_100hz_help",rSysDep::rSwap_100Hz);
static uSelectEntry<rSysDep::rSwapMode> swapMode_80Hz(swapMode,"$swapmode_80hz_text","$swapmode_80hz_help",rSysDep::rSwap_80Hz);
static uSelectEntry<rSysDep::rSwapMode> swapMode_60Hz(swapMode,"$swapmode_60hz_text","$swapmode_60hz_help",rSysDep::rSwap_60Hz);
*/

tCONFIG_ENUM( rSysDep::rSwapMode );

static tConfItem< rSysDep::rSwapMode > swapModeCI("SWAP_MODE", rSysDep::swapMode_ );

static tConfItem<REAL> sgs("SPEED_GAUGE_SIZE",subby_SpeedGaugeSize);
static tConfItem<REAL> sgx("SPEED_GAUGE_LOCX",subby_SpeedGaugeLocX);
static tConfItem<REAL> sgy("SPEED_GAUGE_LOCY",subby_SpeedGaugeLocY);

static tConfItem<REAL> bgs("BRAKE_GAUGE_SIZE",subby_BrakeGaugeSize);
static tConfItem<REAL> bgx("BRAKE_GAUGE_LOCX",subby_BrakeGaugeLocX);
static tConfItem<REAL> bgy("BRAKE_GAUGE_LOCY",subby_BrakeGaugeLocY);

static tConfItem<REAL> rgs("RUBBER_GAUGE_SIZE",subby_RubberGaugeSize);
static tConfItem<REAL> rgx("RUBBER_GAUGE_LOCX",subby_RubberGaugeLocX);
static tConfItem<REAL> rgy("RUBBER_GAUGE_LOCY",subby_RubberGaugeLocY);

static tConfItem<bool> showh("SHOW_HUD",subby_ShowHUD);
static tConfItem<bool> showf("SHOW_FASTEST",subby_ShowSpeedFastest);
static tConfItem<bool> shows("SHOW_SCORE",subby_ShowScore);
static tConfItem<bool> showae("SHOW_ALIVE",subby_ShowAlivePeople);
static tConfItem<bool> showp("SHOW_PING",subby_ShowPing);
static tConfItem<bool> showsm("SHOW_SPEED",subby_ShowSpeedMeter);
static tConfItem<bool> showbm("SHOW_BRAKE",subby_ShowBrakeMeter);
static tConfItem<bool> showrm("SHOW_RUBBER",subby_ShowRubberMeter);
static tConfItem<bool> showtim("SHOW_TIME",showTime);
static tConfItem<bool> show24("SHOW_TIME_24",show24hour);

static tConfItem<REAL> scorex("SCORE_LOCX",subby_ScoreLocX);
static tConfItem<REAL> scorey("SCORE_LOCY",subby_ScoreLocY);
static tConfItem<REAL> scores("SCORE_SIZE",subby_ScoreSize);

static tConfItem<REAL> fastx("FASTEST_LOCX",subby_FastestLocX);
static tConfItem<REAL> fasty("FASTEST_LOCY",subby_FastestLocY);
static tConfItem<REAL> fasts("FASTEST_SIZE",subby_FastestSize);

static tConfItem<REAL> aex("ALIVE_LOCX",subby_AlivePeopleLocX);
static tConfItem<REAL> aey("ALIVE_LOCY",subby_AlivePeopleLocY);
static tConfItem<REAL> aes("ALIVE_SIZE",subby_AlivePeopleSize);

static tConfItem<REAL> px("PING_LOCX",subby_PingLocX);
static tConfItem<REAL> py("PING_LOCY",subby_PingLocY);
static tConfItem<REAL> ps("PING_SIZE",subby_PingSize);

uMenuItemToggle hud3
(&hud_prefs,"$pref_showfastest_text",
 "$pref_showfastest_help",subby_ShowSpeedFastest);

uMenuItemToggle mud4
(&hud_prefs,"$pref_showscore_text",
 "$pref_showscore_help",subby_ShowScore);

uMenuItemToggle hud5
(&hud_prefs,"$pref_showenemies_text",
 "$pref_showenemies_help",subby_ShowAlivePeople);

uMenuItemToggle hud6
(&hud_prefs,"$pref_showping_text",
 "$pref_showping_help",subby_ShowPing);

uMenuItemToggle hud7
(&hud_prefs,"$pref_showspeed_text",
 "$pref_showspeed_help",subby_ShowSpeedMeter);

uMenuItemToggle hud8
(&hud_prefs,"$pref_showbrake_text",
 "$pref_showbrake_help",subby_ShowBrakeMeter);

uMenuItemToggle hud9
(&hud_prefs,"$pref_showrubber_text",
 "$pref_showrubber_help",subby_ShowRubberMeter);

uMenuItemToggle hud10
(&hud_prefs,"$pref_showtime_text",
 "$pref_showtime_help",showTime);

uMenuItemToggle hud11
(&hud_prefs,"$pref_show24hour_text",
 "$pref_show24hour_help",show24hour);

uMenuItemToggle hud2
(&hud_prefs,"$pref_showhud_text",
 "$pref_showhud_help",subby_ShowHUD);



static tConfItem<bool> WRAP("WRAP_MENU",uMenu::wrap);



#ifndef DEDICATED

class gMemuItemConsole: uMenuItemStringWithHistory{
public:
    gMemuItemConsole(uMenu *M,tString &c):
    uMenuItemStringWithHistory(M,"Con:","", c, 1024, sg_consoleHistory, sg_consoleHistoryMaxSize) {}

    virtual ~gMemuItemConsole(){}

    //virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0);

    virtual bool Event(SDL_Event &e){
        if (e.type==SDL_KEYDOWN &&
                (e.key.keysym.sym==SDLK_KP_ENTER || e.key.keysym.sym==SDLK_RETURN)){

            con << tColoredString::ColorString(.5,.5,1) << " > " << *content << '\n';

            // direct commands are executed at owner level
            tCurrentAccessLevel level( tAccessLevel_Owner, true );

            // pass the console command to the configuration system
            std::stringstream s(&((*content)[0]));
            tConfItemBase::LoadAll( s, false );

            MyMenu()->Exit();
            return true;
        }
        else if (e.type==SDL_KEYDOWN &&
                 uActionGlobal::IsBreakingGlobalBind(e.key.keysym.sym))
            return su_HandleEvent(e, true);
        else
            return uMenuItemStringWithHistory::Event(e);
    }
};

void do_con(){
    su_ClearKeys();
        
    se_ChatState( ePlayerNetID::ChatFlags_Console, true );
    sr_con.SetHeight(20,false);
    se_SetShowScoresAuto(false);
    tString c;

    uMenu con_menu("",false);
    gMemuItemConsole s(&con_menu,c);
    con_menu.SetCenter(-.75);
    con_menu.SetBot(-2);
    con_menu.SetTop(-.7);
    con_menu.Enter();

    se_ChatState( ePlayerNetID::ChatFlags_Console, false );

    se_SetShowScoresAuto(true);
    sr_con.SetHeight(7,false);
}
#endif

void sg_ConsoleInput(){
#ifndef DEDICATED
    st_ToDo(&do_con);
#endif
}















class ArmageTron_viewport_menuitem:public uMenuItemInt{
public:
    ArmageTron_viewport_menuitem(uMenu *m):
            uMenuItemInt(m,"$viewport_menu_title",
                         "$viewport_menu_help",
                         rViewportConfiguration::next_conf_num,
                 0,rViewportConfiguration::s_viewportNumConfigurations-1){
        m->RequestSpaceBelow(.9);
    }

    virtual REAL SpaceRight(){return 1;}

    virtual void RenderBackground(){
        uMenuItem::RenderBackground();

        if (rViewportConfiguration::next_conf_num<0) rViewportConfiguration::next_conf_num=0;
        if (rViewportConfiguration::next_conf_num>=rViewportConfiguration::s_viewportNumConfigurations)
            rViewportConfiguration::next_conf_num=rViewportConfiguration::s_viewportNumConfigurations-1;

        tString  titles[MAX_VIEWPORTS];

        for(int i=MAX_VIEWPORTS-1;i>=0;i--)
            titles[i] << i+1;
#ifndef DEDICATED
        rViewportConfiguration::DemonstrateViewport(titles);
#endif
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0){
        if (rViewportConfiguration::next_conf_num<0) rViewportConfiguration::next_conf_num=0;
        if (rViewportConfiguration::next_conf_num>=rViewportConfiguration::s_viewportNumConfigurations)
            rViewportConfiguration::next_conf_num=rViewportConfiguration::s_viewportNumConfigurations-1;

        tOutput disp;

        disp << "$viewport_conf_text";
        disp.AddSpace();
        disp << rViewportConfiguration::s_viewportConfigurationNames[rViewportConfiguration::next_conf_num];
        DisplayText(x-.02,y,disp,selected,alpha);
    }

};



class ArmageTronPlayer_to_viewport_menuitem:public uMenuItemInt{
    int    vp;
public:
    ArmageTronPlayer_to_viewport_menuitem(uMenu *m,int v):
            uMenuItemInt(m,"",
                         "$viewport_assign_help",
                         s_newViewportBelongsToPlayer[v],
                 0,MAX_PLAYERS-1),vp(v){
        m->RequestSpaceBelow(.9);
    }

    virtual REAL SpaceRight(){return 1;}

    virtual void LeftRight(int x){
        rViewport::SetDirectionOfCorrection(vp,x);
        target=(target + MAX_PLAYERS + x) % MAX_PLAYERS;
    }

    virtual void RenderBackground(){
        rViewport::CorrectViewport(vp, MAX_PLAYERS);

        uMenuItem::RenderBackground();

        tString titles[MAX_VIEWPORTS];
        for(int i=MAX_VIEWPORTS-1;i>=0;i--){
            titles[i] << s_newViewportBelongsToPlayer[i]+1;
            titles[i] << " (";
            titles[i] << ePlayer::PlayerConfig(s_newViewportBelongsToPlayer[i])->Name();
            titles[i] << ")";
        }
#ifndef DEDICATED
        rViewportConfiguration::DemonstrateViewport(titles);
#endif
    }

    virtual void Render(REAL x,REAL y,REAL alpha=1,bool selected=0){

        tOutput disp;

        disp.SetTemplateParameter(1, vp +1);
        disp.SetTemplateParameter(2, s_newViewportBelongsToPlayer[vp]+1);
        disp.SetTemplateParameter(3, ePlayer::PlayerConfig(s_newViewportBelongsToPlayer[vp])->Name());
        disp << "$viewport_belongs_text";

        DisplayText(x-.02,y,disp,selected,alpha);
    }

};

#include "rSysdep.h"
extern void Render(int);


class ArmageTron_color_menuitem:public uMenuItemInt{
protected:
    int *rgb;
    unsigned short me;
public:
    ArmageTron_color_menuitem(uMenu *m,const char *tit,
                              const char *help, int *RGB,int Me)
            :uMenuItemInt(m,tit,help,RGB[Me],0,15),
    rgb(RGB),me(Me) {
        m->RequestSpaceBelow(.2);
    }

    ~ArmageTron_color_menuitem(){};

    virtual REAL SpaceRight(){return .2;}


    virtual void RenderBackground(){
        //    static int count=0;
        /*
        while(rgb[0]+rgb[1]+rgb[2]<13){
          if (rgb[count]<15)
        rgb[count]++;
          count++;
          if (count>2)
        count=0;
        }
        */
#ifndef DEDICATED
        uMenuItem::RenderBackground();
        if (!sr_glOut)
            return;
        REAL r = rgb[0]/15.0;
        REAL g = rgb[1]/15.0;
        REAL b = rgb[2]/15.0;
        se_MakeColorValid(r, g, b, 1.0f);
        RenderEnd();
        glColor3f(r, g, b);
        glRectf(.8,-.8,.98,-.98);
#endif
    }

};



void sg_PlayerMenu(int Player){
    tOutput name;
    name.SetTemplateParameter(1, Player+1);

    name << "$player_menu_text";


    uMenu playerMenu(name);

    uMenu camera_menu("$player_camera_text");
    uMenu chat_menu("$player_chat_text");
    //  name.Clear();
    chat_menu.SetCenter(-.5);

    uMenuItemString *ic[MAX_INSTANT_CHAT];

    ePlayer *p = ePlayer::PlayerConfig(Player);
    if (!p)
        return;

    int i;
    for(i=MAX_INSTANT_CHAT-1;i>=0;i--){
        tOutput name;
        name.SetTemplateParameter(1, i+1);
        name << "$player_chat_chat";
        ic[i]=new uMenuItemString
              (&chat_menu,name,
               "$player_chat_chat_help",
               p->instantChatString[i], se_SpamMaxLen);
    }

    uMenuItemToggle al
    (&playerMenu,"$player_autologin_text",
     "$player_autologin_help",
     p->autoLogin);

    uMenuItemString gid(&playerMenu,
                      "$player_global_id_text",
                      "$player_global_id_help",
                      p->globalID, 400);
    gid.SetColorMode( rTextField::COLOR_IGNORE );

    uMenuItemToggle st
    (&playerMenu,"$player_stealth_text",
     "$player_stealth_help",
     p->stealth);

    uMenuItemToggle sp
    (&playerMenu,"$player_spectator_text",
     "$player_spectator_help",
     p->spectate);

    uMenuItemToggle pnt
    (&playerMenu,"$player_name_team_text",
     "$player_name_team_help",
     p->nameTeamAfterMe);

    uMenuItemInt npt
    (&playerMenu,"$player_num_per_team_text",
     "$player_num_per_team_help",
     p->favoriteNumberOfPlayersPerTeam, 1, 16, 1);

    ArmageTron_color_menuitem B(&playerMenu,"$player_blue_text",
                                "$player_blue_help",
                                p->rgb,2);

    ArmageTron_color_menuitem G(&playerMenu,"$player_green_text",
                                "$player_green_help",
                                p->rgb,1);

    ArmageTron_color_menuitem R(&playerMenu,"$player_red_text",
                                "$player_red_help",
                                p->rgb,0);



    uMenuItemSubmenu chm(&playerMenu,&chat_menu,
                         "$player_chat_chat_help");

    uMenuItemSubmenu cm(&playerMenu,&camera_menu,
                        "$player_camera_help");

    uMenuItemFunctionInt icc(&playerMenu,"$player_camera_input_text",
                             "$player_camera_input_help",
                             &su_InputConfigCamera,Player);

    uMenuItemFunctionInt inc(&playerMenu,"$player_input_text",
                             "$player_input_help",
                             &su_InputConfig,Player);


    camera_menu.SetCenter(.3);

    uMenuItemToggle cam_glance
    (&camera_menu,
     "$camera_smart_glance_custom_text",
     "$camera_smart_glance_custom_help",
     p->smartCustomGlance);

    uMenuItemToggle cis(&camera_menu,
                        "$player_camera_autoin_text",
                        "$player_camera_autoin_help",
                        p->autoSwitchIncam);

    uMenuItemToggle cim(&camera_menu,
                        "$player_camera_wobble_text",
                        "$player_camera_wobble_help",
                        p->wobbleIncam);

    uMenuItemToggle cic(&camera_menu,
                        "$player_camera_center_int_text",
                        "$player_camera_center_int_help",
                        p->centerIncamOnTurn);

    uMenuItemToggle al_s
    (&camera_menu,
     "$player_camera_smartcam_text",
     "$player_camera_smartcam_help",
     p->allowCam[CAMERA_SMART]);

    uMenuItemToggle al_f
    (&camera_menu,
     "$player_camera_fixed_text",
     "$player_camera_fixed_help",
     p->allowCam[CAMERA_FOLLOW]);


    uMenuItemToggle al_fr
    (&camera_menu,
     "$player_camera_free_text",
     "$player_camera_free_help",
     p->allowCam[CAMERA_FREE]);

    uMenuItemToggle al_c
    (&camera_menu,
     "$player_camera_custom_text",
     "$player_camera_custom_help",
     p->allowCam[CAMERA_CUSTOM]);

    uMenuItemToggle al_sc
    (&camera_menu,
     "$player_camera_server_custom_text",
     "$player_camera_server_custom_help",
     p->allowCam[CAMERA_SERVER_CUSTOM]);

    uMenuItemToggle al_i
    (&camera_menu,
     "$player_camera_incam_text",
     "$player_camera_incam_help",
     p->allowCam[CAMERA_IN]);


    uMenuItemInt cam_fov
    (&camera_menu,
     "$player_camera_fov_text",
     "$player_camera_fov_help",
     p->startFOV,30,120,5);

    uMenuItemSelection<eCamMode> cam_s
    (&camera_menu,
     "$player_camera_initial_text",
     "$player_camera_initial_help",
     p->startCamera);

    cam_s.NewChoice("$player_camera_initial_scust_text","$player_camera_initial_scust_help",CAMERA_SERVER_CUSTOM);
    cam_s.NewChoice("$player_camera_initial_cust_text","$player_camera_initial_cust_help",CAMERA_CUSTOM);
    cam_s.NewChoice("$player_camera_initial_int_text","$player_camera_initial_int_help",CAMERA_IN);
    cam_s.NewChoice("$player_camera_initial_smrt_text","$player_camera_initial_smrt_help",CAMERA_SMART);
    cam_s.NewChoice("$player_camera_initial_ext_text","$player_camera_initial_ext_help",CAMERA_FOLLOW);
    cam_s.NewChoice("$player_camera_initial_free_text","$player_camera_initial_free_help",CAMERA_FREE);

    uMenuItemString n(&playerMenu,
                      "$player_name_text",
                      "$player_name_help",
                      p->name, 16);

    playerMenu.Enter();

    for(i=MAX_INSTANT_CHAT-1; i>=0; i--)
        delete ic[i];


    // request network synchronisation if the server can handle it
    static nVersionFeature inGameRenames( 5 );
    if ( inGameRenames.Supported() )
    {
        ePlayerNetID::Update();
        ePlayer::SendAuthNames();
    }

    /*
    for (i=MAX_PLAYERS-1; i>=0; i--)
    {
        if (ePlayer::PlayerIsInGame(i))
        {
            ePlayer* p = ePlayer::PlayerConfig(i);
            if (p->netPlayer)
                p->netPlayer->RequestSync();
        }
    }
    */
}


VOIDFUNC viewport_menu_x;






// from gGame.C
//extern int pingCharity;


void sg_PlayerMenu(){
    uMenu Player_men("$player_mainmenu_text");


    uMenuItemFunction vp_selec(&Player_men,
                               "$viewport_assign_text",
                               "$viewport_assign_help",
                               &viewport_menu_x);

    ArmageTron_viewport_menuitem vp(&Player_men);
    uMenuItemFunctionInt  *names[MAX_PLAYERS];

    int i;

    for(i=MAX_PLAYERS-1;i>=0;i--){
        tOutput title;
        title.SetTemplateParameter(1, i+1);
        title << "$player_menu_text";

        tOutput help;
        help.SetTemplateParameter(1, i+1);
        help << "$player_menu_help";

        names[i]=new uMenuItemFunctionInt(&Player_men,
                                          title,
                                          help,
                                          sg_PlayerMenu,i);
    }


    Player_men.Enter();

    //  ePlayerNetID::Update();
    for(i=MAX_VIEWPORTS-1;i>=0;i--){
        delete names[i];
    }
}




void viewport_menu_x(void){
    uMenu sg_PlayerMenu("$viewport_assign_text");

    ArmageTronPlayer_to_viewport_menuitem *select[MAX_VIEWPORTS];

    int i;
    for(i=rViewportConfiguration::s_viewportConfigurations[rViewportConfiguration::next_conf_num]->num_viewports-1;i>=0;i--){
        select[i]=new ArmageTronPlayer_to_viewport_menuitem(&sg_PlayerMenu,i);
    }

    // ArmageTron_viewport_menuitem vp(&sg_PlayerMenu);

    sg_PlayerMenu.Enter();

    for(i=rViewportConfiguration::s_viewportConfigurations[rViewportConfiguration::next_conf_num]->num_viewports-1;i>=0;i--){
        delete select[i];
    }
}


static uActionGlobal con_input( "CONSOLE_INPUT" );


static uActionGlobal screenshot( "SCREENSHOT" );

static uActionGlobal togglefullscreen( "TOGGLE_FULLSCREEN" );

static bool screenshot_func(REAL x){
    if (x>0){
#ifndef DEDICATED
        sr_screenshotIsPlanned=true;
#endif
    }

    return true;
}

static bool con_func(REAL x){
    if (x>0){
        sg_ConsoleInput();
    }

    return true;
}

static bool toggle_fullscreen_func( REAL x )
{
#ifndef DEDICATED
#ifdef DEBUG
    // don't toggle fullscreen while playing back in debug mode, that's annoying
    if ( tRecorder::IsPlayingBack() )
        return true;
#endif

    // only do anything if the application is active (work around odd bug)
    if ( x > 0 && ( SDL_GetAppState() & SDL_APPACTIVE ) )
    {
        currentScreensetting.fullscreen = !currentScreensetting.fullscreen;
        sr_ReinitDisplay();
    }
#endif

    return true;
}

static uActionGlobalFunc gaf_ss(&screenshot,&screenshot_func, true );
static uActionGlobalFunc gaf_md(&con_input,&con_func);
static uActionGlobalFunc gaf_tf(&togglefullscreen,&toggle_fullscreen_func, true );
