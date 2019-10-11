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

#include "rSDL.h"

#include "aa_config.h"

#include "rTexture.h"
#include "rDisplayList.h"
#include "tString.h"
#include "rScreen.h"
#include "tDirectories.h"
#include "tLocale.h"
#include "tConsole.h"
#include "tException.h"
#include "tResourceManager.h"

#include <sstream>
#include <set>

#ifndef DEDICATED
#include "rRender.h"
#include "rGL.h"

// Load the right SDL_IMAGE header

#ifdef _MSC_VER
#include <SDL_image.h>
#else
#ifdef __MINGW32__
#include <SDL_image.h>
#else
#ifdef HAVE_SDL_IMG_H
#include <SDL_image.h>
#else
#ifdef HAVE_SDL_SDL_IMAGE_H
#include <SDL/SDL_image.h>
#else
#ifdef HAVE_IMG_H
#include <IMG.h>
#else
#ifdef HAVE_SDL_IMG_H
#include <SDL/IMG.h>
#else
#ifdef HAVE_LIBSDL
#include <SDL_image.h>
#else
#ifdef HAVE_LIBIMG
#include <IMG.h>
#else
// if the following include ( or one of the earlier ones ) fails, you don't have SDL_image properly installed.
#include <SDL_image.h>
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

// MS OpenGL headers don't include this define
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE GL_CLAMP
#endif

// ******************************************************************************************
// *
// *	rSurface
// *
// ******************************************************************************************
//!
//!		@param	fileName	name of the file to load sufrace from
//!
// ******************************************************************************************

rSurface::rSurface( char const * fileName, tPath const * path )
{
    Init();
    Create( fileName, path );
}

// ******************************************************************************************
// *
// *	~rSurface
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rSurface::~rSurface( void )
{
    Clear();
}

// ******************************************************************************************
// *
// *	rSurface
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rSurface::rSurface( void )
{
    Init();
}

// ******************************************************************************************
// *
// *   rSurface
// *
// ******************************************************************************************
//!
//!        @param  other   source to copy from
//!
// ******************************************************************************************

rSurface::rSurface( rSurface const & other )
{
    Init();
    CopyFrom( other );
}

// ******************************************************************************************
// *
// *   operator =
// *
// ******************************************************************************************
//!
//!        @param  other
//!        @return
//!
// ******************************************************************************************

rSurface & rSurface::operator =( rSurface const & other )
{
    if ( &other != this )
    {
        Clear();
        CopyFrom( other );
    }

    return *this;
}

// ******************************************************************************************
// *
// *	Init
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rSurface::Init( void )
{
    surface_ = 0;
    format_ = 0;
}

// ******************************************************************************************
// *
// *   Clear
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rSurface::Clear( void )
{
#ifndef DEDICATED
    // delete surface
    if ( surface_ )
        SDL_FreeSurface( surface_ );

#endif
    surface_ = 0;
}

// ******************************************************************************************
// *
// *	Create
// *
// ******************************************************************************************
//!
//!		@param	fileName	name of the image file to load
//!
// ******************************************************************************************

void rSurface::Create( char const * fileName, tPath const *path )
{
#ifndef DEDICATED
    sr_LockSDL();

#if !SDL_VERSION_ATLEAST(2,0,0)
    // this function was already a no-op for backward compatibility in SDL_image 1.2, as stated in corresponding header
    IMG_InvertAlpha(true);
#endif

    // find path of image and load it
    SDL_Surface *surface;
    if(path) {
        tString s = path->GetReadPath( fileName );
        surface = IMG_Load(s);
    } else {
        surface = IMG_Load(fileName);
    }
    Create(surface);

    //if ( surface_ )
    //    std::cerr << "loaded surface " << fileName << "\n";

    sr_UnlockSDL();
#endif
}

// ******************************************************************************************
// *
// *	Create
// *
// ******************************************************************************************
//!
//!		@param	surface
//!
// ******************************************************************************************

void rSurface::Create( SDL_Surface * surface )
{
#ifndef DEDICATED
    // clear previous surface
    Clear();

    // take ownership
    surface_ = surface;

    // determine texture format
    if ( surface_ )
    {
        switch (surface_->format->BytesPerPixel){
        case 1:
            format_ = GL_LUMINANCE;
            break;

        case 2:
            format_ = GL_LUMINANCE8_ALPHA8;
            break;

        case 3:
#ifdef GL_BRG
            if (surface_->format->Rmask == 0x000000ff)
                format_ = GL_RGB;
            else
                format_ = GL_BGR;
#else
            format_ = GL_RGB;
#endif
            break;

        case 4:
#ifdef GL_BGRA
            if (surface_->format->Rmask == 0x000000ff)
                format_ = GL_RGBA;
            else
                format_ = GL_BGRA;
#else
            format_ = GL_RGBA;
#endif
            break;

        default:
            {
                // fallback: convert the texture into a known format.
                SDL_Surface *dummy =
                    SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1,
                                         32,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                                         0xFF0000, 0x00FF00, 0x0000FF
#else
                                         0x0000FF, 0x00FF00, 0xFF0000
#endif
                                         ,0xFF000000);

                SDL_Surface *convtex =
                    SDL_ConvertSurface(surface_, dummy->format, SDL_SWSURFACE);

                SDL_FreeSurface(surface_);
                surface_ = convtex;
                SDL_FreeSurface(dummy);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                format_ = GL_BGRA;
#else
                format_ = GL_RGBA;
#endif
            }
            break;
        }
    }
#endif
}

// ******************************************************************************************
// *
// *	CreateQuarter
// *
// ******************************************************************************************
//!
//!		@param	surface to scale down
//!
// ******************************************************************************************

void rSurface::CreateQuarter( rSurface const & big )
{
#ifndef DEDICATED
    // clear previous surface
    Clear();

    tASSERT( big.surface_ );
    format_ = big.format_;

    // determine dimensions
    int sourceW = big.surface_->w;
    int sourceH = big.surface_->h;
    int w = (sourceW+1)/2;
    int h = (sourceH+1)/2;

    // create new surface of new sizes
    surface_ = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
                                    big.surface_->format->BitsPerPixel,
                                    big.surface_->format->Rmask,
                                    big.surface_->format->Gmask,
                                    big.surface_->format->Bmask,
                                    big.surface_->format->Amask);

    tASSERT( surface_ );

    int bytesPerPixel = surface_->format->BytesPerPixel;
    int sourcePitch = big.surface_->pitch;
    int pitch = surface_->pitch;
    unsigned char const * source = (unsigned char const *)big.surface_->pixels;
    unsigned char * dest = (unsigned char *) surface_->pixels;
    for( int i = 0; i < h; ++i )
    {
        int off = i * pitch;
        int soff1 = (i<<1)*sourcePitch;
        int soff2 = (((i<<1)+1)%sourceH)*sourcePitch;
        for( int j = 0; j < w; ++j )
        {
            int ind = j*bytesPerPixel;
            int sind1 = (j<<1)*bytesPerPixel;
            int sind2 = (((j<<1)+1)%sourceW)*bytesPerPixel;

            for( int b = 0; b < bytesPerPixel; ++b )
            {
                // box filter
                dest[off+ind+b] = ( source[soff1+sind1+b] + source[soff2+sind1+b] + source[soff1+sind2+b] + source[soff2+sind2+b] )>>2;
            }
        }
    }
#endif
}

// ******************************************************************************************
// *
// *	CopyFrom
// *
// ******************************************************************************************
//!
//!		@param	other
//!
// ******************************************************************************************

void rSurface::CopyFrom( rSurface const & other )
{
#ifndef DEDICATED
    tASSERT( 0 == surface_ );
    if( other.surface_ )
    {
        // copy surface
        surface_ = SDL_ConvertSurface(other.surface_, other.surface_->format, SDL_SWSURFACE);

        // copy flags
        format_ = other.format_;
    }
#endif
}

// surface cache
typedef std::pair< tString, tPath const * > rSurfaceCacheKey;

class rSurfaceCacheValue
{
public:
    rSurface surface;
    bool used;

    rSurfaceCacheValue()
    : used( true )
    {}
};

typedef std::map< rSurfaceCacheKey, rSurfaceCacheValue > rSurfaceCacheMap;
static rSurfaceCacheMap sr_surfaceCacheMap;

// ******************************************************************************************
// *
// *	GetSurface
// *
// ******************************************************************************************
//!
//!		@param	fileName the file name of the surface relative to the given path
//!     @param  path     search path
//!
// ******************************************************************************************
rSurface const * rSurfaceCache::GetSurface( char const * fileName, tPath const *path )
{
    rSurfaceCacheKey key( tString( fileName ), path );
    // bool inCache = ( sr_surfaceCacheMap.find(key) != sr_surfaceCacheMap.end() );

    rSurfaceCacheValue & val = sr_surfaceCacheMap[ key ];
    if( !val.surface.GetSurface() )
    {
        // first time, load
        val.surface.Create( fileName, path );
    }

    if( !val.surface.GetSurface() )
    {
        // previous error, don't retry
        return NULL;
    }

    // mark as used and return
    val.used = true;
    return &val.surface;
}

// ******************************************************************************************
// *
// *	CycleCache
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************
void rSurfaceCache::CycleCache()
{
    for( rSurfaceCacheMap::iterator i = sr_surfaceCacheMap.begin(); i != sr_surfaceCacheMap.end();  )
    {
        rSurfaceCacheMap::iterator next = i;
        next++;

        rSurfaceCacheValue & value = (*i).second;
        if( !value.used )
        {
            sr_surfaceCacheMap.erase( i );
        }
        else
        {
            value.used = false;
        }

        i = next;
    }
}

// ******************************************************************************************
// *
// *	ClearCache
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************
void rSurfaceCache::ClearCache()
{
    sr_surfaceCacheMap.clear();
}

// ******************************************************************************************
// *
// *	~rITexture
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rITexture::~rITexture( void )
{
    s_textures_.Remove(this,id_);
}

// ******************************************************************************************
// *
// *	UnloadAll
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rITexture::UnloadAll( void )
{
    for(int i=s_textures_.Len()-1;i>=0;i--)
    {
        s_textures_(i)->Unload();
    }
    rSurfaceCache::ClearCache();
}

// ******************************************************************************************
// *
// *	LoadAll
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rITexture::LoadAll( void )
{
    // s_reportErrors=false;
    for(int i=s_textures_.Len()-1;i>=0;i--)
    {
        s_textures_(i)->Select();
        //if (i>=s_textures.Len())
        //    i=s_textures.Len()-1;
    }
    // s_reportErrors=true;
}

// ******************************************************************************************
// *
// *	rITexture
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rITexture::rITexture( void )
        : id_( -1 )
{
}

// ******************************************************************************************
// *
// *	OnSelect
// *
// ******************************************************************************************
//!
//!		@param	enforce enforce when set to true, the texture should be loaded even if the configuration says it should not
//!
// ******************************************************************************************

void rITexture::OnSelect( bool enforce )
{
    if ( id_ < 0 )
        s_textures_.Add(this,id_);
}

// ******************************************************************************************
// *
// *	OnUnload
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rITexture::OnUnload( void )
{
}

// ******************************************************************************************
// *
// *	rISurfaceTexture
// *
// ******************************************************************************************
//!
//!		@param	group	texture group ( floor/wall)
//!		@param	repx    flag indicating the x repeat mode
//!		@param	repy    flag indicating the y repeat mode
//!		@param	storeAlpha flag indicating whether the alpha channel should be stored
//!
// ******************************************************************************************

rISurfaceTexture::rISurfaceTexture( int group, bool repx, bool repy, bool storeAlpha )
        : group_( group ), textureModeLast_( -1), repx_( repx ), repy_( repy ), storeAlpha_( storeAlpha )
{
}

// ******************************************************************************************
// *
// *	~rISurfaceTexture
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rISurfaceTexture::~rISurfaceTexture( void )
{
    if (tint_.IsValid() )
    {
        rDisplayList::ClearAll();
    }
}

// ******************************************************************************************
// *
// *	ProcessImage
// *
// ******************************************************************************************
//!
//!		@param	surface the surface to process
//!
// ******************************************************************************************

void rISurfaceTexture::ProcessImage( SDL_Surface * surface )
{
}

#ifndef DEDICATED
static bool sr_IsPowerOfTwo( int i )
{
    return i == 1 || ( ( (i & 1) == 0  ) && sr_IsPowerOfTwo( i >> 1 ) );
}

static int sr_GetMaxTextureSizeCore()
{
    // guaranteed supported size
    GLint maxSize = 64;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    return maxSize;
}

static int sr_GetMaxTextureSize()
{
    static int maxSize = sr_GetMaxTextureSizeCore();
    return maxSize;
}
#endif

// ******************************************************************************************
// *
// *	Upload
// *
// ******************************************************************************************
//!
//!		@param	surface
//!
// ******************************************************************************************

void rISurfaceTexture::Upload( rSurface const & surface )
{
#ifndef DEDICATED
    sr_LockSDL();
    GLenum texformat = surface.GetFormat();
    SDL_Surface * tex = surface.GetSurface();
    tASSERT( tex );

    bool texalpha=tex->format->Amask;

    ProcessImage(tex);

    if(repx_)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    else
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    if(repy_)
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    else
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    int format;
    if (sr_texturesTruecolor)
        if (storageHack_ || ( storeAlpha_ && texalpha ) )
            format=GL_RGBA8;
        else
            format=GL_RGB8;
    else
        if (storageHack_ || ( storeAlpha_ && texalpha ) )
            format=GL_RGBA4;
        else
            format=GL_RGB5;

    if( !sr_IsPowerOfTwo( tex->w ) || !sr_IsPowerOfTwo( tex->h ) )
    {
        static bool warn = true;
        if( warn )
        {
            warn = false;
            rFileTexture * texture = dynamic_cast< rFileTexture * >( this );
            if( texture )
            {
                con << "\nWARNING: non-power-of-two texture dimensions in texture " << texture->GetFileName() << ". If you're the artist creating it, correct it by rescaling, please; it may cease to work in future versions or even not work right now for some people.\n\n";
            }
            else
            {
                con << "\nWARNING: non-power-of-two texture dimensions in unknown texture. If you're the artist creating one, recheck your work, it may cease to work in future versions or even not work right now for some people.\n\n";
            }
        }

        // no power of two, delegate to legacy function without checks
        gluBuild2DMipmaps(GL_TEXTURE_2D,format,tex->w,tex->h,
                          texformat,GL_UNSIGNED_BYTE,tex->pixels);
    }
    else
    {
        int level = 0;

        // mipmap generation pipeline
        rSurface even(surface), odd(surface);
        rSurface * current = &even;
        rSurface * next = &odd;

        bool sizeOK = false;
        while(true)
        {
            // upload current as mipmap level.
            tex = current->GetSurface();
            tASSERT( tex );

            // test whether the size is OK
            if( !sizeOK && tex->w <= sr_GetMaxTextureSize() && tex->h <= sr_GetMaxTextureSize() )
            {
                // so far, so good; check via proxy
                glTexImage2D(GL_PROXY_TEXTURE_2D,level,format,tex->w,tex->h,0,
                             texformat,GL_UNSIGNED_BYTE,tex->pixels);
                GLint width;
                glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
                sizeOK = ( width != 0 );
            }

            if( sizeOK )
            {
                // upload and increase level
                glTexImage2D(GL_TEXTURE_2D,level,format,tex->w,tex->h,0,
                             texformat,GL_UNSIGNED_BYTE,tex->pixels);
                level++;
            }

            // scale down for next level
            if( tex->w == 1 && tex->h == 1 )
            {
                break;
            }
            else
            {
                next->CreateQuarter( *current );
                rSurface * swap = next;
                next = current;
                current = swap;
            }
        }
    }

    sr_UnlockSDL();
 #endif
}

// ******************************************************************************************
// *
// *	OnSelect
// *
// ******************************************************************************************
//!
//!		@param	enforce enforce when set to true, the texture should be loaded even if the configuration says it should not
//!
// ******************************************************************************************

void rISurfaceTexture::OnSelect( bool enforce )
{
#ifndef DEDICATED
    if(sr_glOut)
    {
        RenderEnd(true);

        int texmod=rTextureGroups::TextureMode[group_];
        if (enforce && texmod<0) texmod=GL_NEAREST_MIPMAP_NEAREST;

        if(textureModeLast_!=texmod)
        {
            // unload texture if the mode changed
            Unload();
            // std::cerr << "loading texture " << fileName << ':' << tint << "\n";

            if (texmod>0){
                // don't generate textures inside display lists
                rDisplayList::Cancel();

                glBindTexture(GL_TEXTURE_2D,tint_);

                if (textureModeLast_<0)
                {
                    // delegate core loading work to derived class
                    OnSelectCore();
                }

                //glEnable(GL_TEXTURE);
                glEnable(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
                                texmod);

                switch(texmod)
                {
                case GL_NEAREST:
                case GL_NEAREST_MIPMAP_NEAREST:
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,
                                    GL_NEAREST);
                    break;
                default:
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
                    break;
                }

            }
            else
            {
                glDisable(GL_TEXTURE_2D);
            }
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D,tint_);
            if (texmod>0)
            {
                glEnable(GL_TEXTURE_2D);
            }
            else
            {
                glDisable(GL_TEXTURE_2D);
            }
        }
        textureModeLast_=texmod;
    }
    rITexture::OnSelect(enforce);
#endif
}

// ******************************************************************************************
// *
// *	OnUnload
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rISurfaceTexture::OnUnload( void )
{
#ifndef DEDICATED
    if ( tint_.IsValid() )
    {
        rDisplayList::ClearAll();
    }

    tint_.Delete();
    textureModeLast_=-100;
    rITexture::OnUnload();
#endif
}

// ******************************************************************************************
// *
// *	StoreAlpha
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rISurfaceTexture::StoreAlpha( void )
{
    storeAlpha_ = true;
}

// ******************************************************************************************
// *
// *	rFileTexture
// *
// ******************************************************************************************
//!
//!		@param	group	texture group ( floor/wall)
//!		@param	fileName the filename of the picture to load
//!		@param	repx    flag indicating the x repeat mode
//!		@param	repy    flag indicating the y repeat mode
//!		@param	storeAlpha flag indicating whether the alpha channel should be stored
//!
// ******************************************************************************************

rFileTexture::rFileTexture( int group, char const * fileName, bool repx, bool repy, bool storeAlpha, tPath const *path )
        : rISurfaceTexture( group, repx, repy, storeAlpha )
        ,  fileName_( fileName )
        ,  path_(path)
{
}

// ******************************************************************************************
// *
// *	~rFileTexture
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rFileTexture::~rFileTexture( void )
{
}

// ******************************************************************************************
// *
// *	OnSelectCore
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rFileTexture::OnSelectCore()
{
#ifndef DEDICATED
    // std::cerr << "loading texture " << fileName_ << "\n";
    rSurface const * surface = rSurfaceCache::GetSurface( fileName_, path_ );
    if ( surface )
    {
        this->Upload( *surface );
    }
    else if (s_reportErrors_)
    {
        throw tGenericException( tOutput( "$texture_error_filenotfound", fileName_ ), tOutput("$texture_error_filenotfound_title") );
    }
#endif
}

// ******************************************************************************************
// *
// *	rSurfaceTexture
// *
// ******************************************************************************************
//!
//!		@param	group	texture group ( floor/wall)
//!		@param	surface
//!		@param	repx    flag indicating the x repeat mode
//!		@param	repy    flag indicating the y repeat mode
//!		@param	storeAlpha flag indicating whether the alpha channel should be stored
//!
// ******************************************************************************************

rSurfaceTexture::rSurfaceTexture( int group, rSurface const & surface, bool repx, bool repy, bool storeAlpha )
        : rISurfaceTexture( group, repx, repy, storeAlpha )
        , surface_( surface )
{
}

// ******************************************************************************************
// *
// *	~rSurfaceTexture
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

rSurfaceTexture::~rSurfaceTexture( void )
{
}

// ******************************************************************************************
// *
// *	OnSelectCore
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rSurfaceTexture::OnSelectCore()
{
#ifndef DEDICATED
    // upload a copy of the surface ( it may get modified )
    if ( surface_.GetSurface() )
    {
        rSurface copy( surface_ );
        this->Upload( copy );
    }
#endif
}


bool rISurfaceTexture::s_reportErrors_=false;
tList<rITexture> rITexture::s_textures_;

int rTextureGroups::TextureMode[rTextureGroups::TEX_GROUPS]
#ifndef DEDICATED
={GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }
#endif
;

char const * rTextureGroups::TextureGroupDescription[rTextureGroups::TEX_GROUPS]=
    {
        "$texture_mode_0_help",
        "$texture_mode_1_help",
        "$texture_mode_2_help",
        "$texture_mode_3_help",
    };

bool rISurfaceTexture::storageHack_ = false;

//rTexture ArmageTron_eWall("wWall.png",1,0);
//rTexture ArmageTron_dir_eWall("wall.png",1,0);


static rCallbackBeforeScreenModeChange unload(&rITexture::UnloadAll);

// static rCallbackAfterScreenModeChange load(&rITexture::LoadAll);

rResourceTexture::texlist_t rResourceTexture::textures;

rResourceTexture::rResourceTexture(tResourcePath const &path, bool repx, bool repy) : repx_(repx), repy_(repy) {
    if(!path.Valid()) {
        tex_ = 0;
        return;
    }
    for(texlist_t::iterator iter = textures.begin(); iter != textures.end(); ++iter) {
        if((*iter)->path_ == path) {
            tex_ = *iter;
            tex_->Use();
            return;
        }
    }
    tex_ = new tex_t(path);
}

// verifies a resource texture, redownloads it if it is corrupted (once per session)
static void sr_VerifyResourceTexture( tString const & resourcePath )
{
#ifndef DEDICATED
    static std::set< tString > verified;
    if( verified.find(resourcePath) == verified.end() )
    {
        tString filePath = tResourceManager::locateResource( resourcePath, "", false  );
        if( filePath != "" )
        {
            // check read and write path
            tString w = tDirectories::Resource().GetWritePath( filePath );
            tString r = tDirectories::Resource().GetReadPath( filePath );

            // if they're equal, that means the resource has been downloaded before. Check it
            // by loading it once
            if( w == r )
            {
                rSurface const * surface = rSurfaceCache::GetSurface( filePath, &tDirectories::Resource() );
                if( !surface )
                {
                    // trigger a redownload
                    tResourceManager::locateResource( resourcePath, "", true, true );
                }
            }
        }

        // mark as checked
        verified.insert( resourcePath );
    }
#endif
}

rResourceTexture::InternalTex::InternalTex(tResourcePath const &path) : rFileTexture(rTextureGroups::TEX_OBJ, tResourceManager::locateResource(path.Path().c_str()).c_str(), true, true, true, 0), use_(1), path_(path) {
    sr_VerifyResourceTexture( path.Path() );

    textures.push_back(this);
}

void rResourceTexture::InternalTex::Release() {
    if(--use_ < 1) {
        for(texlist_t::iterator iter = textures.begin(); iter != textures.end(); ++iter) {
            if((*iter)->path_ == path_) {
                textures.erase(iter);
                break;
            }
        }
        Unload();
        delete this;
    }
}

rResourceTexture &rResourceTexture::operator=(rResourceTexture const &other) {
    if(tex_ != other.tex_) {
        repx_ = other.repx_;
        repy_ = other.repy_;
        if(tex_) {
            tex_->Release();
        }
        tex_ = other.tex_;
        if(tex_) {
            tex_->Use();
        }
    }
    return *this;
}

void rResourceTexture::Select() {
#ifndef DEDICATED
    if(tex_) {
        tex_->Select();
        // Override the actual tecture's settings
        if(repx_)
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        else
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        if(repy_)
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        else
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    } else {
        tERR_WARN("Trying to select a resource texture that's not loaded");
    }
#endif
}
