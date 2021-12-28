/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#include "tMutex.h"

#ifndef HAVE_BOOST_THREAD

namespace boost
{
    // special constructor, do not initialize mutex
    mutex::mutex( int )
    {
    }

    mutex::mutex()
    {
        // TODO: error checking
        pthread_mutex_init(&mutex_, NULL);
    }

    mutex::~mutex()
    {
        pthread_mutex_destroy(&mutex_);
    }

    void mutex::lock()
    {
        pthread_mutex_lock(&mutex_);
    }

    void mutex::unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

    recursive_mutex::recursive_mutex()
    : mutex(5)
    {
        // TODO: error checking
        pthread_mutexattr_t mta;

        pthread_mutexattr_init(&mta);
        pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex_, &mta);
        pthread_mutexattr_destroy(&mta);
    }
}

#endif
