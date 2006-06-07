// configuration header common to all IDE builds that don't use autoconf

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* define if you wish to compile a dedicated server */
/* #undef DEDICATED */

#ifndef KRAWALL
//#define KRAWALL
#endif

/* define if you have the SDL */
#define HAVE_LIBSDL 1

/* PowerPak 2D debugging output */
/* #undef POWERPAK_DEB */
/* #undef HAVE_LIBPP */

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the <IMG> header file.  */
/* #undef HAVE_IMG_H */

/* Define if you have the <SDL/IMG> header file.  */
/* #undef HAVE_SDL_IMG_H */

/* Define if you have the <SDL/SDL_image> header file.  */
#define HAVE_SDL_SDL_IMAGE_H 1

/* Define if you have the <SDL_image> header file.  */
/* #undef HAVE_SDL_IMAGE_H */

#define HAVE_GL_GL_H

/* Define if you have the <unistd> header file.  */
/* #define HAVE_UNISTD_H */

/* Define if you have the GL library (-lGL).  */
#define HAVE_LIBGL 1

/* Define if you have the GLU library (-lGLU).  */
#define HAVE_LIBGLU 1

/* Define if you have the SDL_image library (-lSDL_image).  */
#define HAVE_LIBSDL_IMAGE 1

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1

/* Define if you have the png library (-lpng).  */
#define HAVE_LIBPNG 1

/* Define if you have the wsock32 library (-lwsock32).  */
/* #undef HAVE_LIBWSOCK32 */

/* Define if you have the z library (-lz).  */
#define HAVE_LIBZ 1
#ifdef DEDICATED
#define HAVE_LIBSDL 1
/* #undef HAVE_FXMESA */
/* #undef HAVE_LIBIMG */
#define HAVE_LIBSDL_IMAGE 1
/* #undef POWERPAK_DEB */
#endif

/* Define to 1 if you have the `sinf' function. */
#define HAVE_SINF 1

/* Define to 1 if you have the `cosf' function. */
#define HAVE_COSF 1

/* Define to 1 if you have the `tanf' function. */
#define HAVE_TANF 1

/* Define to 1 if you have the `atan2f' function. */
#define HAVE_ATAN2F 1

/* Define to 1 if you have the `sqrtf' function. */
#define HAVE_SQRTF 1

/* Define to 1 if you have the `logf' function. */
#define HAVE_LOGF 1

/* Define to 1 if you have the `expf' function. */
#define HAVE_EXPF 1

/* Define to 1 if you have the `fabsf' function. */
#define HAVE_FABSF 1

/* Define to 1 if you have the `floorf' function. */
#define HAVE_FLOORF 1

/* Define to 1 if you have the `wmemset' function. */
#define HAVE_WMEMSET 1
