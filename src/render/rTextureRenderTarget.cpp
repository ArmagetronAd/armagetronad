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

#include "rTextureRenderTarget.h"

// implementation
#include "rScreen.h"
#include "tException.h"

rTextureRenderTarget * rTextureRenderTarget::anchor = 0;

// ****************************************************************
// *
// *	rTextureRenderTarget
// *
// ****************************************************************
//!
//!		@param
//!		@return
//!
// ****************************************************************

//! texture that can be used as a target for rendering

// clears the texture
void rTextureRenderTarget::Clear()
{
    tASSERT( !IsTarget() );
    
    texture_.Delete();
    depthBuffer_.Delete();
    frameBuffer_.Delete();
}

rTextureRenderTarget::~rTextureRenderTarget()
{
    if ( IsTarget() )
    {
        Pop();
    }
    
    Clear();
}

rTextureRenderTarget::rTextureRenderTarget( int width, int height )
{
#ifndef DEDICATED
    sr_CheckGLError();
    
    width_ = width;
    height_ = height;
    previous = 0;
    
    if ( GLEW_EXT_framebuffer_object )
    {
        Push();
        sr_CheckGLError();
        
        // generate texture
        glBindTexture( GL_TEXTURE_2D, texture_ );
        sr_CheckGLError();
        
        // make texture
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
        sr_CheckGLError();
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        
        glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture_, 0 );
        sr_CheckGLError();
        
        glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, depthBuffer_ );
        sr_CheckGLError();
        
        glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT32, width, height );
        sr_CheckGLError();
        
        glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthBuffer_ );
        sr_CheckGLError();
        
        GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT);
        switch( status )
        {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            throw tGenericException( "Unsupported frame buffer operation" );
        default:
            tASSERT( 0 );
        }
        
        sr_CheckGLError();
        
        Pop();
        
        return;
    }
    
    throw tGenericException( "frame buffer extension not supported" );
#endif
}

void rTextureRenderTarget::Push()
{
#ifndef DEDICATED
    previous = anchor;
    anchor = this;
    
    sr_CheckGLError();
    
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, frameBuffer_ );
    
    sr_CheckGLError();
#endif
}

void rTextureRenderTarget::Pop()
{
#ifndef DEDICATED
    tASSERT( IsTarget() );
    
    sr_CheckGLError();
    glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, previous ? previous->frameBuffer_ : 0 );
    sr_CheckGLError();
    
    anchor = previous;
#endif
}

void rTextureRenderTarget::OnSelect(bool)
{
#ifndef DEDICATED
    glBindTexture( GL_TEXTURE_2D, texture_ );
#endif
}
