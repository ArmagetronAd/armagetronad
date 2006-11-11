#ifndef AT_SDL_H
#define AT_SDL_H

#include "aa_config.h"

#ifndef DEDICATED
#include <SDL.h>
#else
#define SDLK_LAST 300
typedef int SDL_keysym;
typedef int SDL_Event;
typedef unsigned char Uint8;
typedef unsigned int Uint32;
typedef int SDL_AudioSpec;
#endif

#endif
