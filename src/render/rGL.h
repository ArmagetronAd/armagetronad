#ifndef AT_GL_H
#define AT_GL_H

#ifndef DEDICATED

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL_opengl.h>

/*
// include OpenGL header
#ifdef HAVE_SDL_OPENGL_H
#include <SDL_opengl.h>
#else
#ifdef HAVE_SDL_SDL_OPENGL_H
#include <SDL/SDL_opengl.h>
#else
#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef HAVE_GL_GL_H
#include <GL/gl.h>
#include <GL/glu.h>
#else
#error No suitable OpenGL header found by configure!
#endif
#endif
#endif
#endif
*/

#else

typedef float GLfloat;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
#endif

#endif
