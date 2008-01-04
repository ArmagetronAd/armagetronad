/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2006, Armagetron Advanced Development Team  

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

#ifndef ArmageTron_GLUINTOBJECT_H
#define ArmageTron_GLUINTOBJECT_H

#include "rGL.h"
#include "tError.h"

//! safe wrapper for GL objects classifed by GLuints (textures, render buffers)
class rGLuintObject
{
public:
    rGLuintObject(); 	    //!< default constructor

    operator GLuint(); 	    //!< implicit conversion to GLuints, auto-creating the object
    bool IsValid() const; 	//!< checks whether the object is valid

    virtual ~rGLuintObject();  //!< NOTE: the destructor of a derived class needs to call Delete() itself

    void Gen();     //!< reserve the object
    void Delete(); 	//!< free the object
protected:
    rGLuintObject( rGLuintObject const & );                 //!< forbidden copy construcor
    rGLuintObject & operator =( rGLuintObject const & );    //! forbidden copy operator

    GLuint object_;                 //!< the wrapped "object"
private:
    virtual void DoGen() = 0;       //!< really reserves the object
    virtual void DoDelete() = 0;    //!< really frees the object
};

//! unusable dummy wrapper
class rGluintObjectDummy: public rGLuintObject
{
public:
    ~rGluintObjectDummy(){ Delete(); }
private:
    // the following operations are not supported, wrap your code in #ifdef HAVE_GLEW
    virtual void DoGen(){ tASSERT(0); }
    virtual void DoDelete(){  tASSERT(0); }
};

//! declare safe wrapper classes for GL objects handled by GLuints.
//! argument CLASS: the name of the wrapper class
//! FUNCTION: the name of the functions that hanlde the object; glGen resp. glDelete will be prepended.
#ifndef DEDICATED
#define rDEFINE_GLUINTOBJECT( CLASS, FUNCTION ) \
class CLASS: public rGLuintObject \
{ \
public: \
	~CLASS(){ Delete(); } \
private: \
	virtual void DoGen(){ glGen##FUNCTION( 1, &object_ ); } \
	virtual void DoDelete(){ glDelete##FUNCTION( 1, &object_ ); } \
}
#else // DEDICATED
// safe dummy
#define rDEFINE_GLUINTOBJECT( CLASS, FUNCTION ) typedef rGluintObjectDummy CLASS
#endif

//! declare safe wrapper classes for GL objects handled by GLuints initialized by extensions
//! argument CLASS: the name of the wrapper class
//! FUNCTION: the name of the functions that hanlde the object; glGen resp. glDelete will be prepended.
#ifdef HAVE_GLEW
#define rDEFINE_GLUINTOBJECT_EXT(CLASS, FUNCTION ) rDEFINE_GLUINTOBJECT( CLASS, FUNCTION )
#else // HAVE_GLEW, define unusable dummy object
#define rDEFINE_GLUINTOBJECT_EXT( CLASS, FUNCTION ) typedef rGluintObjectDummy CLASS
#endif // HAVE_GLEW

//! declare safe wrapper for texturs
rDEFINE_GLUINTOBJECT( rGLuintObjectTexture, Textures );

//! declare safe wrapper for render buffers
rDEFINE_GLUINTOBJECT_EXT( rGLuintObjectRenderbuffer, RenderbuffersEXT );

//! declare safe wrapper for frame buffers
rDEFINE_GLUINTOBJECT_EXT( rGLuintObjectFramebuffer, FramebuffersEXT );

#endif


