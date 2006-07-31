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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

//! @file
//! Contains the tAutoDeque class for handling pointers

#ifndef ArmageTron_tAUTOCONTAINER_H
#define ArmageTron_tAUTOCONTAINER_H
#include <memory>
#include <deque>

//! Class for handling sets of allocated pointers
//!
//! This is not working exactly like an std::auto_ptr, but close to it.
//! Not all functionality of std::deque is implemented, if anything you need is missing, feel free to add it.
template<typename T> class tAutoDeque : public std::deque<T *> {
public:
    //! Destructor deletes all posessed elements
    ~tAutoDeque() {
        clear();
    }
    //! Copy constructor clears the other object
    //! @param other the tAutoDeque to construct this object from
    tAutoDeque(tAutoDeque &other)
            :std::deque<T *>(other) {
        other.release();
    }
    //! Default Constructor.
    tAutoDeque()
            :std::deque<T *>() {
    }
    //! Constructor from a std::deque leaves the deque alone
    //! @param other the std::deque to construct this object from
    tAutoDeque(typename std::deque<T *> const &other)
            :std::deque<T *>(other) {
    }
    //! Empties the deque, unallocating all contents
    void clear() {
        while(!empty()) {
            pop_back();
        }
    }
    //! Empties the deque but doesn't unallocate anything. Use with caution.
    void release() {
        std::deque<T *>::clear();
    }
    //! Returns the last element of the deque
    //! @returns a pointer to that element
    T *back() {
        return std::deque<T *>::back();
    }
    //! Returns the first element of the deque
    //! @returns a pointer to that element
    T *front() {
        return std::deque<T *>::front();
    }
    //! Deletes and unallocates the last element of the deque
    void pop_back() {
        delete back();
        std::deque<T *>::pop_back();
    }
    //! Deletes and unallocates the first element of the deque
    void pop_front() {
        delete front();
        std::deque<T *>::pop_front();
    }
    //! Deletes the last element of the deque without deallocating it
    //! @returns a pointer to the stored object
    T *release_back() {
        T ret = back();
        std::deque<T *>::pop_back();
        return ret;
    }
    //! Deletes the first element of the deque without deallocating it
    //! @returns a pointer to the stored object
    T *release_front() {
        T ret = front();
        std::deque<T *>::pop_front();
        return ret;
    }
    //! Adds a new element to the back
    //! @param ptr a pointer to the object that should be added
    void push_back(T *ptr) {
        std::deque<T *>::push_back(ptr);
    }
    //! Adds a new element to the front
    //! @param ptr a pointer to the object that should be added
    void push_front(T *ptr) {
        std::deque<T *>::push_front(ptr);
    }
    //! Adds a new element to the back
    //! @param ptr a std::auto_ptr to the object that should be added. It will be cleared.
    void push_back(std::auto_ptr<T *> &ptr) {
        std::deque<T *>::push_back(ptr.release());
    }
    //! Adds a new element to the front
    //! @param ptr a std::auto_ptr to the object that should be added. It will be cleared.
    void push_front(std::auto_ptr<T *> &ptr) {
        std::deque<T *>::push_front(ptr.release());
    }
    //! Deletes an element from the deque, deallocating the contents
    //! @param pos an iterator pointing to the element that is to be deleted
    //! @returns ++pos
    typename std::deque<T *>::iterator erase(typename std::deque<T *>::iterator const &pos) {
        delete *pos;
        return std::deque<T *>::erase(pos);
    }
    //! Deletes elements from the deque, deallocating the contents
    //! @param first an iterator pointing to the first element that is to be deleted
    //! @param beyond an iterator pointing to the first element that is not to be deleted
    //! @returns beyond
    typename std::deque<T *>::iterator erase(typename std::deque<T *>::iterator first, typename std::deque<T *>::iterator const &beyond) {
        for(; first != beyond; ++first) {
            erase(first);
        }
        return beyond;
    }
    //! Gives access to the beginning iterator
    //! @returns an iterator pointing to the first element
    typename std::deque<T *>::iterator begin() {
        return std::deque<T *>::begin();
    }
    //! Gives access to the end iterator
    //! @returns an iterator pointing beyond the last element
    typename std::deque<T *>::iterator end() {
        return std::deque<T *>::end();
    }
    //! Is this thing empty?
    //! @returns true if empty, false if not (d'oh!)
    bool empty() {
        return std::deque<T *>::empty();
    }
    //! How long is this thing?
    //! @returns the number of elements
    unsigned size() {
        return std::deque<T *>::size();
    }
    //! Gives direct access to an element
    //! @param i the position of the element
    //! @returns a pointer to the requested element
    T *operator[](int i) {
        return std::deque<T *>::operator[](i);
    }
};

#endif
