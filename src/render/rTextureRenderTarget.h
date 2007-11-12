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

#ifndef ArmageTron_TEXTURERENDERTARGET_H
#define ArmageTron_TEXTURERENDERTARGET_H

#include "rGLEW.h"
#include "rTexture.h"
#include "rGLuintObject.h"

//! texture that can be used as a target for rendering
class rTextureRenderTarget: public rITexture
{
public:
    //! clears the texture
    void Clear();

    ~rTextureRenderTarget();

    //! creates a render target texture of the specified dimensions
    rTextureRenderTarget( int width, int height );

    //! makes this texture the active render target
    void Push();

    //! removes this texture as the active render target
    void Pop();

    //! restores current render target if it was overwritten by an evil manual glBindFramebuffer call
    static void Restore();

    bool IsTarget() const
    {
        return anchor == this;
    }

    int GetHeight() const
    {
        return height_;
    }

    int GetWidth() const
    {
        return width_;
    }
protected:
    virtual void OnSelect(bool);

    // nothing special to do here
    virtual void OnUnload(){}
private:
    rGLuintObjectTexture texture_;           //!< the texture render target
    rGLuintObjectRenderbuffer depthBuffer_;  //!< the depth buffer
    rGLuintObjectFramebuffer frameBuffer_;   //!< the frame buffer


    static rTextureRenderTarget * anchor;
    rTextureRenderTarget * previous;

    int width_, height_;
    GLenum colorMode, depthMode;
public:
};

#endif


