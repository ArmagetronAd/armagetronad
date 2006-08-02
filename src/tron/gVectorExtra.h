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

#ifndef ARMAGETRONAD_VECTOREXTRA_H
#define ARMAGETRONAD_VECTOREXTRA_H

#include <vector>

template <typename T>
class gVectorExtra: public std::vector<T>
{
public:
    gVectorExtra();
    gVectorExtra(std::vector<T> const oldOne);
    virtual ~gVectorExtra() {};
    void remove(T el);
    void removeAll(std::vector<T> els);
    void insertAll(std::vector<T> els);
};

template <typename T>
void
gVectorExtra<T>::remove(T singleElement) {
    typename gVectorExtra<T>::iterator it;
    for(it = this->begin();
            it != this->end();
            it++) {
        if ( *it == singleElement ) {
            this->erase(it);
            if (it == this->end())
                break;
        }
    }
}

template <typename T>
void
gVectorExtra<T>::removeAll(std::vector<T> multipleElements) {
    typename gVectorExtra<T>::iterator it;
    typename std::vector<T>::iterator elsIt;
    for(elsIt = multipleElements.begin();
            elsIt != multipleElements.end();
            elsIt++) {
        for(it = this->begin();
                it != this->end();
                it++) {
            if (*elsIt == *it) {
                this->erase(it);
                if (it == this->end())
                    break;
            }
        }
    }
}

template <typename T>
void
gVectorExtra<T>::insertAll(std::vector<T> multipleElements) {
    this->insert(this->end(), multipleElements.begin(), multipleElements.end());
}


template <typename T>
gVectorExtra<T>::gVectorExtra() : std::vector<T>() {}

template <typename T>
gVectorExtra<T>::gVectorExtra(std::vector<T> const oldOne) {
    typename std::vector<T>::const_iterator iter;
    for(iter = oldOne.begin();
            iter != oldOne.end();
            iter++) {
        push_back((*iter));
    }
}



#endif
