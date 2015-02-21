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

#ifndef AT_GL_H
#define AT_GL_H

#include "aa_config.h"

#ifndef DEDICATED

#ifdef WIN32
#include <windows.h>
#endif

// GLEW, if active, needs to be included before gl.h
#include "rGLEW.h"
#include "GL/glu.h"

// and we don't want the SDL extension definitions, they conflict with GLEW.
#define NO_SDL_GLEXT

#include <SDL_opengl.h>

#else

typedef float GLfloat;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
#endif

#ifdef DEBUG
#ifndef DEDICATED
#define AA_GL_ERROR_CHECKING
#endif
#endif

#ifdef AA_GL_ERROR_CHECKING
//! for debugging purposes: checks for OpenGL errors and prints them to the console.
void sr_CheckGLError();
#else
inline void sr_CheckGLError(){}
#endif

#endif
