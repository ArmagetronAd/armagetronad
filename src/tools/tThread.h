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

#ifdef HAVE_BOOST_THREAD

#include <boost/thread/thread.hpp>

#else // HAVE_BOOST_THREAD

#ifdef HAVE_PTHREAD

#include <pthread.h>

// replicate the little we actually use with PThreads
namespace boost
{
class thread
{
public:
    struct attributes{
        size_t stack_size{};

        void set_stack_size(size_t s){stack_size = s;}
    };

    template< class T>
    void launch( attributes const & a, T const & t )
    {
        // we don't currently hang on to thread objects, so no need to store handles
        pthread_t thread;

        // make a copy of the object to call
        T * o = new T(t);

        if(a.stack_size)
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, a.stack_size);

            pthread_create(&thread, &attr, &run<T>, (void*) o);

            pthread_attr_destroy(&attr);
        }
        else
        {
            pthread_create(&thread, nullptr, &run<T>, (void*) o);
        }
    }

    template< class T>
    thread( attributes const & a, T const & t )
    {
        launch(a, t);
    }

    template< class T>
    thread( T const & t )
    {
        launch(attributes{}, t);
    }

    void detach(){}
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

#endif // HAVE_PTHREAD

#endif // HAVE_BOOST_THREAD

#endif
