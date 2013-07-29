/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2011  Armagetron Advanced Development Team

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

#ifndef ArmageTron_THREAD_H
#define ArmageTron_THREAD_H

#include "defs.h"

#ifdef HAVE_LIBBOOST_THREAD

#include <boost/thread/thread.hpp>

#define HAVE_THREADS

#else // HAVE_LIBBOOST_THREAD

#ifdef HAVE_PTHREAD

#define HAVE_THREADS

#include <pthread.h>

// replicate the little we actually use with PThreads
namespace boost
{
class thread
{
public:
    template< class T>
    thread( T const & t )
    {
        // we don't currently hang on to thread objects, so no need to store handles
        pthread_t thread;

        // make a copy of the object to call
        T * o = new T(t);

        pthread_create(&thread, NULL, &run<T>, (void*) o);
    }
private:
    // worker function
    template< class T >
    static void * run( void * o )
    {
        T * t = static_cast< T * >(o);

        // do the actual call
        (*t)();

        // clean up
        delete t;

        return NULL;
    }
};
}

#else  // HAVE_PTHREAD

#include "tError.h"

namespace boost
{
class thread
{
public:
    template< class T>
    thread( T const & t )
    {
        // should never be called, then
        tVERIFY(0);
    }
};
}

#endif // HAVE_PTHREAD

#endif // HAVE_LIBBOOST_THREAD

#endif
