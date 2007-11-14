#ifndef AT_GLEW_H
#define AT_GLEW_H

#include "aa_config.h"
#include "tException.h"
#include "tString.h"

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

//! Exception for GLEW (also defined when we don't have GLEW)
class rExceptionGLEW: public tException
{
public:
    rExceptionGLEW( char const * description )
    : description_( description )
    {
    }
private:
    virtual tString DoGetName() const { return tString("GLEW Exception"); }

    virtual tString DoGetDescription() const { return description_; }
    tString description_;
};

#endif
