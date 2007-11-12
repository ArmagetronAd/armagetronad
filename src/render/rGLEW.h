#ifndef AT_GLEW_H
#define AT_GLEW_H

#include "aa_config.h"

// check whether GLEW is available
#ifndef DEDICATED
#ifdef HAVE_GL_GLEW_H
#ifdef HAVE_LIBGLEW

// it is! define one handy macro indicating so
#define HAVE_GLEW

// and include the headers
#include <GL/glew.h>

#endif
#endif
#endif

#endif
