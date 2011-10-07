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

#ifndef ArmageTron_LOCKED_QUEUE_H
#define ArmageTron_LOCKED_QUEUE_H

#include "tMutex.h"

#include <deque>

//! thread safe queue
template< typename T, typename MUTEX >
class tLockedQueue 
{
public:
    //! exception thrown if next() is called on an empty queue
    class Empty: public std::exception{};

    void add(const T& item)
    {
        boost::lock_guard< MUTEX > lock(mutex_);
        
        queue_.push_back(item);
    }
    
    size_t size()
    {
        boost::lock_guard< MUTEX > lock(mutex_);
        
        return queue_.size();
    }
    
    T next()
    {
        boost::lock_guard< MUTEX > lock(mutex_);

        if(queue_.size() == 0)
            throw Empty();

        T item = queue_.front();
        queue_.pop_front();
        
        return item;
    }
private:
    std::deque<T> queue_;
    MUTEX mutex_;
};

#endif
