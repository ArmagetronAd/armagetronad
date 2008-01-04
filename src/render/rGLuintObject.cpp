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

#include "rGLuintObject.h"

// heh, no parameters to document :)

rGLuintObject::rGLuintObject()
        : object_(0)
{
}

//! @return the GLUint representing the object, always guaranteed to be valid
rGLuintObject::operator GLuint()
{
    // auto-generate the object
    Gen();
    return object_;
}

//! @return true if the object is currently valid (meaning: Gen() has been called after Delete())
bool rGLuintObject::IsValid() const
{
    return object_;
}

rGLuintObject::~rGLuintObject()
{
}

void rGLuintObject::Gen()
{
    if ( !object_ )
    {
        sr_CheckGLError();
        DoGen();
        sr_CheckGLError();
    }
}

void rGLuintObject::Delete()
{
    if ( object_ )
    {
        sr_CheckGLError();
        DoDelete();
        sr_CheckGLError();
    }
    object_ = 0;
}

