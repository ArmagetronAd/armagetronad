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

    IMG_InvertAlpha(true);

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
            format_ = GL_RGB;
            break;

        case 4:
            format_ = GL_RGBA;
            break;

        default:
            {
                // fallback: convert the texture into a known format.

                SDL_Surface *dummy =
                    SDL_CreateRGBSurface(SDL_SWSURFACE, 1, 1,
                                         32,
                                         0x0000FF, 0x00FF00,
                                         0xFF0000 ,0xFF000000);

                SDL_Surface *convtex =
                    SDL_ConvertSurface(surface_, dummy->format, SDL_SWSURFACE);

                SDL_FreeSurface(surface_);
                surface_ = convtex;
                SDL_FreeSurface(dummy);

                format_ = GL_RGBA;
            }
            break;
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
    tASSERT( other.surface_ );

    // copy surface
    surface_ = SDL_ConvertSurface(other.surface_, other.surface_->format, SDL_SWSURFACE);

    // copy flags
    format_ = other.format_;
#endif
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

// ******************************************************************************************
// *
// *	Upload
// *
// ******************************************************************************************
//!
//!		@param	surface
//!
// ******************************************************************************************

void rISurfaceTexture::Upload( rSurface & surface )
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

    gluBuild2DMipmaps(GL_TEXTURE_2D,format,tex->w,tex->h,
                      texformat,GL_UNSIGNED_BYTE,tex->pixels);

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
                    OnSelect();
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
// * OnSelect
// *
// ******************************************************************************************
//!
//!  In derived classes, this routine is supposed to do the work of loading the texture
//!  into memory and using the Upload() function to upload it to OpenGL.
//!
// ******************************************************************************************

void rISurfaceTexture::OnSelect()
{
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
// *	OnSelect
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rFileTexture::OnSelect()
{
#ifndef DEDICATED
    // std::cerr << "loading texture " << fileName_ << "\n";
    rSurface surface( fileName_, path_ );
    if ( surface.GetSurface() )
    {
        this->Upload( surface );
    }
    else if (s_reportErrors_)
    {
        throw tGenericException( tOutput( "$texture_error_filenotfound", fileName_ ), tOutput("$texture_error_filenotfound_title") );
    }
    rISurfaceTexture::OnSelect();
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
// *	OnSelect
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rSurfaceTexture::OnSelect()
{
#ifndef DEDICATED
    // upload a copy of the surface ( it may get modified )
    if ( surface_.GetSurface() )
    {
        rSurface copy( surface_ );
        this->Upload( copy );
        rISurfaceTexture::OnSelect();
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

rResourceTexture::InternalTex::InternalTex(tResourcePath const &path) : rFileTexture(rTextureGroups::TEX_OBJ, tResourceManager::locateResource(path.Path().c_str()).c_str(), true, true, true, 0), use_(1), path_(path) {
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
