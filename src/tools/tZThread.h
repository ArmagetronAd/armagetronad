/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

***************************************************************************

*/

#ifdef HAVE_LIBZTHREAD

#include <zthread/Thread.h>
#include <zthread/LockedQueue.h>
#include <zthread/FastMutex.h>
#include <zthread/FastRecursiveMutex.h>
#include <zthread/Guard.h>
#include <zthread/ThreadedExecutor.h>

// these are the classes we actually use
// typedef ZThread::ThreadedExecutor nExecutor;
// typedef ZThread::FastMutex nMutex;
// typedef ZThread::FastRecursiveMutex tRecursiveMutex;
// #define nQueue ZThread::LockedQueue
// ZThread::Runnable
// ZThread::Task

#elif defined(HAVE_PTHREAD)

#include "pthread-binding.h"

namespace ZThread
{
using FastMutex = tPThreadMutex;
using FastRecursiveMutex = tPThreadRecursiveMutex;

template <class T, class MutexT = FastRecursiveMutex>
using LockedQueue = tPThreadQueue<T, MutexT>;

class Runnable
{
public:
    virtual void run() = 0;
    virtual ~Runnable() = default;
};

using Task = std::unique_ptr<Runnable>;

class ThreadedExecutor
{
    static void* DoCall(void* o)
    {
        Task runnable{(Runnable*)o};
        runnable->run();
        return nullptr;
    }

public:
    static void execute(Task&& task)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, DoCall, task.release());
    }
};

}; // namespace ZThread

// from now on, we can pretend to have ZThread
#define HAVE_LIBZTHREAD

// #elif defined(HAVE_BOOST_THREAD)

#else

// very minimal non-implementation
class tMockMutex
{
public:
    void acquire() {}
    void release() {}
};

namespace ZThread
{
using FastMutex = tMockMutex;
using FastRecursiveMutex = tMockMutex;
} // namespace ZThread

#endif
