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

#ifndef ArmageTron_TMUTEX_H
#define ArmageTron_TMUTEX_H

#include "defs.h"

#ifdef HAVE_LIBBOOST_THREAD

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#else // HAVE_LIBBOOST_THREAD

namespace boost
{
// replicate the little we actually use with PThreads; for documentation, see boost.
class mutex
{
private:
    mutex( mutex const & );
    mutex & operator = ( mutex const & );
protected:
#ifdef HAVE_PTHREAD
#define HAVE_REAL_MUTEX
    pthread_mutex_t mutex_;
#endif

    // special constructor, do not initialize mutex
    explicit mutex( int );
public:
    mutex();
    ~mutex();
    void lock();
    void unlock();
};

class recursive_mutex
 : public mutex
{
public:
    recursive_mutex();
};

template <class T>
class lock_base
{
private:
    T & mutex_;
    lock_base( lock_base const & );
    lock_base & operator = ( lock_base const & );
protected:
lock_base(T & m)
    : mutex_(m)
    {}

    void lock_()
    {
        mutex_.lock();
    };

    void unlock_()
    {
        mutex_.unlock();
    }
};

template <class T>
class lock_guard: public lock_base<T>
{
public:
    lock_guard(T & m)
    : lock_base<T>(m)
    {
        this->lock_();
    };

    ~lock_guard()
    {
        this->unlock_();
    };
};

template <class T>
class unique_lock: public lock_base<T>
{
private:
    bool locked_;
public:
    unique_lock(T & m)
    : lock_base<T>(m), locked_(false)
    {
        this->lock();
    };

    ~unique_lock()
    {
        this->unlock();
    };

    void lock()
    {
        if( !locked_ )
        {
            locked_ = true;
            this->lock_();
        }
    }

    void unlock()
    {
        if( locked_ )
        {
            locked_ = false;
            this->unlock_();
        }
    }
};

}

#endif // HAVE_LIBBOOST_THREAD

#endif
