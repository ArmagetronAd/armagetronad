#ifndef AT_SDL_H
#define AT_SDL_H

#include "aa_config.h"

#ifndef DEDICATED
#include <SDL.h>
#include <SDL_version.h>
#if SDL_VERSION_ATLEAST(2,0,0)
#define SDL_OPENGL
#endif
#else
#define SDLK_LAST 300
#define SDL_NUM_SCANCODES 300
#define SDL_VERSION_ATLEAST(a,b,c) true
typedef int SDL_keysym;
typedef int SDL_Event;
typedef unsigned char Uint8;
typedef unsigned int Uint32;
typedef int SDL_AudioSpec;
#endif

#endif
