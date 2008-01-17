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

#ifndef ArmageTron_TEXTURE_H
#define ArmageTron_TEXTURE_H

#include "tString.h"
#include "tList.h"
#include "rGL.h"
#include "rGLuintObject.h"

struct SDL_Surface;

//! class organizing textures into groups (very crudely...)
class rTextureGroups
{
public:
    //! enum defining texture groups
    enum Group
    {
        TEX_FLOOR=0,
        TEX_WALL=1,
        TEX_OBJ=2,
        TEX_FONT=3,
        TEX_GROUPS
    };

    static int TextureMode[TEX_GROUPS];                 //!< the OpenGL texture modes for the groubs
    static char const * TextureGroupDescription[TEX_GROUPS];   //!< descriptions for the groups
};

//! wrapper for SDL surface
class rSurface
{
public:
    explicit rSurface( char const * fileName );       //!< constructor creating the surface from a file
    ~rSurface();                                      //!< destructor
    rSurface( rSurface const & other );               //!< copy constructor
    rSurface & operator = ( rSurface const & other ); //!< copy operator
protected:
    rSurface();                             //!< default constructor, not creating a real surface
    void Init();                            //!< initialize data members
    void Clear();                           //!< destroys data members
    void Create( char const * fileName );   //!< create surface from file
    void Create( SDL_Surface * surface );   //!< take ownership of surface

private:
    // attributes
    SDL_Surface * surface_;                  //!< the surface itself
    GLenum format_;                          //!< OpenGL texture format to use

    void CopyFrom( rSurface const & other ); //!< copy function

    // accessors
public:
    inline SDL_Surface * GetSurface( void ) const;	                     //!< Gets the surface itself
    inline rSurface const & GetSurface( SDL_Surface * & surface ) const; //!< Gets the surface itself
    inline GLenum const & GetFormat( void ) const;	                     //!< Gets openGL texture format to use
    inline rSurface const & GetFormat( GLenum & format ) const;	         //!< Gets openGL texture format to use
protected:
    inline rSurface & SetSurface( SDL_Surface * surface );	             //!< Sets the surface itself
    inline rSurface & SetFormat( GLenum const & format );	             //!< Sets openGL texture format to use
private:
};

// ******************************************************************************************

//! abstract OpenGL texture base class
class rITexture
{
public:
    virtual ~rITexture() = 0;                //!< destructor

    static void UnloadAll();                 //!< Unloads all textures from memory
    static void LoadAll();                   //!< Loads all active textures

    inline void Select(bool enforce=false);  //!< Selects the texture for rendering
    inline void Unload();                    //!< Unloads the texture from OpenGL and memory
protected:
    rITexture(); //!< constructor

    virtual void OnSelect(bool enforce) = 0; //!< Selects the texture for rendering
    virtual void OnUnload() = 0;             //!< Unloads the texture from OpenGL and memory

private:
    static tList<rITexture> s_textures_;     //!< list of all textures
    int id_;                                 //!< ID number of this texture

    // disable copying
    rITexture( rITexture const & );
    rITexture & operator = ( rITexture const & );
};

// ******************************************************************************************

//! abstract texture class working with SDL surfaces
class rISurfaceTexture: public rITexture
{
public:
    rISurfaceTexture(int group, bool repx=0, bool repy=0,
                     bool storeAlpha=false);        //!< constructor setting flags
    virtual ~rISurfaceTexture();                    //!< destructor

    bool Loaded(){ return textureModeLast_ >= 0; }  //!< returns whether the texture is currently loaded

    //! when set, this flag inhibits storage of the alpha channel (for compatibility with some cards)
    static bool storageHack_;

protected:
    virtual void ProcessImage(SDL_Surface *);       //!< process the surface before uploading it to GL

    void Upload( rSurface & surface );              //!< Uploads the passed surface to OpenGL (for use in OnSelect)

    virtual void OnSelect(bool enforce);            //!< Selects the texture for rendering
    virtual void OnSelect()=0;                      //!< Selects the texture for rendering (core part)
    virtual void OnUnload();                        //!< Unloads the texture from OpenGL and memory

    void StoreAlpha();                              //!< sets the alpha store flag

    static bool s_reportErrors_;                    //!< flag indicating whether errors should be reported
private:

    int group_;             //!< the texture group (determines rendering options)

    int textureModeLast_;   //!< the last texture storage mode this texture was used with

    rGLuintObjectTexture tint_;   //!< the OpenGL id of this texture
    bool repx_,repy_;             //!< flags indicating whether the texture repeats in x and y direction
    bool storeAlpha_;             //!< flag indicating whether the alpha value should be stored
};

// ******************************************************************************************

//! texture class loading the data from a file
class rFileTexture: public rISurfaceTexture
{
public:
    rFileTexture(int group, char const * fileName, bool repx=0, bool repy=0,
                 bool storeAlpha=false);    //!< constructor setting flags
    virtual ~rFileTexture();                //!< destructor

protected:
    virtual void OnSelect();                //!< Selects the texture for rendering (core part)
private:
    tString fileName_;                      //!< the texture's filename

public:
    inline tString const & GetFileName( void ) const;	                  //!< Gets the texture's filename
    inline rFileTexture const & GetFileName( tString & fileName ) const;  //!< Gets the texture's filename
protected:
private:
    inline rFileTexture & SetFileName( tString const & fileName );	      //!< Sets the texture's filename
};

// ******************************************************************************************

//! texture class getting its data from a surface
class rSurfaceTexture: public rISurfaceTexture
{
public:
    rSurfaceTexture(int group, rSurface const & surface, bool repx=0, bool repy=0,
                    bool storeAlpha=false );    //!< constructor setting flags
    virtual ~rSurfaceTexture();                 //!< destructor

protected:
    virtual void OnSelect();                    //!< Selects the texture for rendering (core part)
private:
    rSurface const & surface_;                  //!< Surface to use as texture data

public:
    inline const rSurface & GetSurface( void ) const; //!< Gets surface to use as texture data
protected:
private:
};

// ******************************************************************************************
// *
// *	GetSurface
// *
// ******************************************************************************************
//!
//!		@return		the surface itself
//!
// ******************************************************************************************

SDL_Surface * rSurface::GetSurface( void ) const
{
    return this->surface_;
}

// ******************************************************************************************
// *
// *	GetSurface
// *
// ******************************************************************************************
//!
//!		@param	surface	the surface itself to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

rSurface const & rSurface::GetSurface( SDL_Surface * & surface ) const
{
    surface = this->surface_;
    return *this;
}

// ******************************************************************************************
// *
// *	SetSurface
// *
// ******************************************************************************************
//!
//!		@param	surface	the surface itself to set
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

rSurface & rSurface::SetSurface( SDL_Surface * surface )
{
    this->surface_ = surface;
    return *this;
}

// ******************************************************************************************
// *
// *	GetFormat
// *
// ******************************************************************************************
//!
//!		@return		OpenGL texture format to use
//!
// ******************************************************************************************

GLenum const & rSurface::GetFormat( void ) const
{
    return this->format_;
}

// ******************************************************************************************
// *
// *	GetFormat
// *
// ******************************************************************************************
//!
//!		@param	format	OpenGL texture format to use to fill
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

rSurface const & rSurface::GetFormat( GLenum & format ) const
{
    format = this->format_;
    return *this;
}

// ******************************************************************************************
// *
// *	SetFormat
// *
// ******************************************************************************************
//!
//!		@param	format	OpenGL texture format to use to set
//!		@return		A reference to this to allow chaining
//!
// ******************************************************************************************

rSurface & rSurface::SetFormat( GLenum const & format )
{
    this->format_ = format;
    return *this;
}

// ******************************************************************************************
// *
// *	Select
// *
// ******************************************************************************************
//!
//!		@param	enforce when set to true, the texture should be loaded even if the configuration says it should not
//!
// ******************************************************************************************

void rITexture::Select( bool enforce )
{
    this->OnSelect( enforce );
}

// ******************************************************************************************
// *
// *	Unload
// *
// ******************************************************************************************
//!
//!
// ******************************************************************************************

void rITexture::Unload()
{
    this->OnUnload();
}

// ****************************************************************************
// *
// *	GetFileName
// *
// ****************************************************************************
//!
//!		@return		the texture's filename
//!
// ****************************************************************************

tString const & rFileTexture::GetFileName( void ) const
{
    return this->fileName_;
}

// ****************************************************************************
// *
// *	GetFileName
// *
// ****************************************************************************
//!
//!		@param	fileName	the texture's filename to fill
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

rFileTexture const & rFileTexture::GetFileName( tString & fileName ) const
{
    fileName = this->fileName_;
    return *this;
}

// ****************************************************************************
// *
// *	SetFileName
// *
// ****************************************************************************
//!
//!		@param	fileName	the texture's filename to set
//!		@return		A reference to this to allow chaining
//!
// ****************************************************************************

rFileTexture & rFileTexture::SetFileName( tString const & fileName )
{
    this->fileName_ = fileName;
    return *this;
}

// ****************************************************************************
// *
// *	GetSurface
// *
// ****************************************************************************
//!
//!		@return		Surface to use as texture data
//!
// ****************************************************************************

const rSurface & rSurfaceTexture::GetSurface( void ) const
{
    return this->surface_;
}

#endif


